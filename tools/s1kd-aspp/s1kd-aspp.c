/* Applicability Preprocessor
 *
 * Preprocesses a data module's applicability statements (@applicRefId) in to a
 * simpler format which is easier for an XSL stylesheet to process.
 *
 * The applicability in the resulting output will not be semantically correct. */

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xinclude.h>
#include <libxslt/transform.h>
#include <libexslt/exslt.h>
#include "elements_list.h"
#include "generateDisplayText.h"
#include "identity.h"

#define PROG_NAME "s1kd-aspp"
#define VERSION "1.0.1"

/* ID for the inline <applic> element representing the whole data module's
 * applicability. */
#define DEFAULT_DM_APPLIC_ID BAD_CAST "applic-0000"
xmlChar *dmApplicId;

/* XPath to select all elements which may have applicability annotations.
 *
 * Read from elements_list.h*/
xmlChar *applicElemsXPath;

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

char *customGenDispTextXsl = NULL;

/* Return the first node matching an XPath expression. */
xmlNodePtr firstXPathNode(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	if (doc) {
		ctx = xmlXPathNewContext(doc);
	} else if (node) {
		ctx = xmlXPathNewContext(node->doc);
	} else {
		return NULL;
	}

	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		first = NULL;
	} else {
		first = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

/* Return the last node matching an XPath expression. */
xmlNodePtr lastXPathNode(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr last;

	if (doc) {
		ctx = xmlXPathNewContext(doc);
	} else if (node) {
		ctx = xmlXPathNewContext(node->doc);
	} else {
		return NULL;
	}

	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		last = NULL;
	} else {
		last = obj->nodesetval->nodeTab[obj->nodesetval->nodeNr - 1];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return last;
}

/* Set an explicit applicability statement on a node.
 *
 * Nodes with the attribute @applicRefId already have an explicit statement.
 *
 * Nodes without @applicRefId have an implicit statement inherited from their
 * parent/ancestor which does have an explicit statement, or ultimately from
 * the applicability of the whole data module. */
void processNode(xmlNodePtr node)
{
	xmlNodePtr attr;

	attr = firstXPathNode(NULL, node, "@applicRefId|@refapplic");

	if (!attr) {
		/* Inherit applicability from
		 * - an ancestor, or
		 * - the whole data module level */
		xmlNodePtr ancestor;
		xmlChar *name;

		ancestor = lastXPathNode(NULL, node, "ancestor::*[@applicRefId]|ancestor::*[@refapplic]");

		name = BAD_CAST (firstXPathNode(NULL, node, "//idstatus") ? "refapplic" : "applicRefId");

		if (!ancestor) {
			xmlSetProp(node, name, dmApplicId);
		} else {
			xmlChar *ancestorApplic = xmlGetProp(ancestor, name);
			xmlSetProp(node, name, ancestorApplic);
			xmlFree(ancestorApplic);
		}
	}
}

/* Set explicit applicability on all nodes in a nodeset. */
void processNodeSet(xmlNodeSetPtr nodes)
{
	int i;

	for (i = 0; i < nodes->nodeNr; ++i) {
		processNode(nodes->nodeTab[i]);
	}
}

/* Remove duplicate applicability statements in document-order so that a
 * statement is only shown when applicability changes. */
void removeDuplicates(xmlNodeSetPtr nodes)
{
	int i;

	xmlChar *applic = xmlStrdup(dmApplicId);

	for (i = 0; i < nodes->nodeNr; ++i) {
		xmlNodePtr attr;
		xmlChar *applicRefId;

		attr = firstXPathNode(NULL, nodes->nodeTab[i], "@applicRefId|@refapplic");
		applicRefId = xmlNodeGetContent(attr);

		if (xmlStrcmp(applicRefId, applic) == 0) {
			xmlUnsetProp(nodes->nodeTab[i], attr->name);
		} else {
			xmlFree(applic);
			applic = xmlStrdup(applicRefId);
		}

		xmlFree(applicRefId);
	}

	xmlFree(applic);
}

/* Insert a new inline <applic> element representing the applicability of the
 * whole data module, based on the dmStatus/applic element. */
void addDmApplic(xmlNodePtr dmodule)
{
	xmlNodePtr referencedApplicGroup;

	if ((referencedApplicGroup = firstXPathNode(NULL, dmodule, ".//referencedApplicGroup|.//inlineapplics"))) {
		xmlNodePtr applic;
		xmlNodePtr wholeDmApplic;

		wholeDmApplic = firstXPathNode(NULL, dmodule, ".//dmStatus/applic|.//status/applic");
		applic = xmlAddChild(referencedApplicGroup, xmlCopyNode(wholeDmApplic, 1));

		xmlSetProp(applic, BAD_CAST "id", dmApplicId);
	}
}

void dumpGenDispTextXsl(void)
{
	printf("%.*s", generateDisplayText_xsl_len, generateDisplayText_xsl);
}

void addIdentity(xmlDocPtr style)
{
	xmlDocPtr identity;
	xmlNodePtr stylesheet, first, template;

	identity = xmlReadMemory((const char *) ___common_identity_xsl, ___common_identity_xsl_len, NULL, NULL, 0);
	template = xmlFirstElementChild(xmlDocGetRootElement(identity));

	stylesheet = xmlDocGetRootElement(style);

	first = xmlFirstElementChild(stylesheet);

	if (first) {
		xmlAddPrevSibling(first, xmlCopyNode(template, 1));
	} else {
		xmlAddChild(stylesheet, xmlCopyNode(template, 1));
	}

	xmlFreeDoc(identity);
}

void generateDisplayText(xmlDocPtr doc, xmlNodePtr acts, xmlNodePtr ccts)
{
	xmlDocPtr styledoc, res, muxdoc;
	xsltStylesheetPtr style;
	xmlNodePtr mux, cur, muxacts, muxccts, new, old;

	muxdoc = xmlNewDoc(BAD_CAST "1.0");
	mux = xmlNewNode(NULL, BAD_CAST "mux");
	xmlDocSetRootElement(muxdoc, mux);

	xmlAddChild(mux, xmlCopyNode(xmlDocGetRootElement(doc), 1));
	muxacts = xmlNewChild(mux, NULL, BAD_CAST "acts", NULL);
	muxccts = xmlNewChild(mux, NULL, BAD_CAST "ccts", NULL);
	for (cur = acts->children; cur; cur = cur->next) {
		xmlDocPtr act;
		xmlChar *path;
		path = xmlNodeGetContent(cur);
		act = xmlReadFile((char *) path, NULL, PARSE_OPTS);
		xmlAddChild(muxacts, xmlCopyNode(xmlDocGetRootElement(act), 1));
		xmlFreeDoc(act);
		xmlFree(path);
	}
	for (cur = ccts->children; cur; cur = cur->next) {
		xmlDocPtr cct;
		xmlChar *path;
		path = xmlNodeGetContent(cur);
		cct = xmlReadFile((char *) path, NULL, PARSE_OPTS);
		xmlAddChild(muxccts, xmlCopyNode(xmlDocGetRootElement(cct), 1));
		xmlFreeDoc(cct);
		xmlFree(path);
	}

	if (customGenDispTextXsl) {
		styledoc = xmlReadFile(customGenDispTextXsl, NULL, PARSE_OPTS);
	} else {
		styledoc = xmlReadMemory((const char *) generateDisplayText_xsl,
			generateDisplayText_xsl_len, NULL, NULL, 0);
	}

	addIdentity(styledoc);

	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, muxdoc, NULL);

	new = xmlCopyNode(firstXPathNode(res, NULL, "/mux/dmodule"), 1);
	old = xmlDocSetRootElement(doc, new);
	xmlFreeNode(old);

	xmlFreeDoc(res);
	xmlFreeDoc(muxdoc);
	xsltFreeStylesheet(style);
}

void processDmodule(xmlNodePtr dmodule)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(dmodule->doc);
	ctx->node = dmodule;
	obj = xmlXPathEvalExpression(applicElemsXPath, ctx);

	processNodeSet(obj->nodesetval);
	removeDuplicates(obj->nodesetval);
	addDmApplic(dmodule);

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

void processDmodules(xmlNodeSetPtr dmodules)
{
	int i;

	for (i = 0; i < dmodules->nodeNr; ++i) {
		processDmodule(dmodules->nodeTab[i]);
	}
}

void processFile(const char *in, const char *out, bool xincl, bool process,
	bool genDispText, xmlNodePtr acts, xmlNodePtr ccts)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	doc = xmlReadFile(in, NULL, PARSE_OPTS);

	if (xincl) {
		xmlXIncludeProcess(doc);
	}

	if (process) {
		ctx = xmlXPathNewContext(doc);
		obj = xmlXPathEvalExpression(BAD_CAST "//dmodule", ctx);

		processDmodules(obj->nodesetval);

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);
	}

	if (genDispText) {
		generateDisplayText(doc, acts, ccts);
	}

	xmlSaveFile(out, doc);

	xmlFreeDoc(doc);
}

void showHelp(void)
{
	puts("Usage: " PROG_NAME " [-g [-A <ACT>] [-C <CCT>]] [-p [-a <ID>]] [-dfxh?] [<modules>]");
	puts("");
	puts("Options:");
	puts("  -A <ACT>   Use <ACT> when generating display text.");
	puts("  -a <ID>    Use <ID> for DM-level applic.");
	puts("  -C <CCT>   Use <CCT> when generating display text.");
	puts("  -d         Dump built-in XSLT for generating display text.");
	puts("  -f         Overwrite input file(s).");
	puts("  -G <XSL>   Use custom XSLT script to generate display text.");
	puts("  -g         Generate display text for applicability statements.");
	puts("  -p         Convert semantic applicability to presentation applicability.");
	puts("  -h -?      Show help/usage message.");
	puts("  --version  Show version information.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
}

int main(int argc, char **argv)
{
	int i;
	bool overwrite = false;
	bool xincl = false;
	bool genDispText = false;
	bool process = false;
	
	xmlNodePtr acts, ccts;

	const char *sopts = "A:a:C:dfG:gpxh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	exsltRegisterAll();

	dmApplicId = xmlStrdup(DEFAULT_DM_APPLIC_ID);

	applicElemsXPath = xmlStrndup(elements_list, elements_list_len);

	acts = xmlNewNode(NULL, BAD_CAST "acts");
	ccts = xmlNewNode(NULL, BAD_CAST "ccts");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 'A':
				xmlNewChild(acts, NULL, BAD_CAST "act", BAD_CAST optarg);
				break;
			case 'a':
				xmlFree(dmApplicId);
				dmApplicId = xmlStrdup(BAD_CAST optarg);
				break;
			case 'C':
				xmlNewChild(ccts, NULL, BAD_CAST "cct", BAD_CAST optarg);
				break;
			case 'd':
				dumpGenDispTextXsl();
				exit(0);
			case 'f':
				overwrite = true;
				break;
			case 'G':
				customGenDispTextXsl = strdup(optarg);
				break;
			case 'g':
				genDispText = true;
				break;
			case 'p':
				process = true;
				break;
			case 'x':
				xincl = true;
				break;
			case 'h':
			case '?':
				showHelp();
				exit(0);
		}
	}

	if (optind >= argc) {
		processFile("-", "-", xincl, process, genDispText, acts, ccts);
	} else {
		for (i = optind; i < argc; ++i) {
			processFile(argv[i], overwrite ? argv[i] : "-", xincl,
				process, genDispText, acts, ccts);
		}
	}

	xmlFree(dmApplicId);
	xmlFree(applicElemsXPath);

	xmlFreeNode(acts);
	xmlFreeNode(ccts);

	free(customGenDispTextXsl);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
