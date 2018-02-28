/* Applicability Preprocessor
 *
 * Preprocesses a data module's applicability statements (@applicRefId) in to a
 * simpler format which is easier for an XSL stylesheet to process.
 *
 * The applicability in the resulting output will not be semantically correct. */

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xinclude.h>
#include "elements_list.h"

/* ID for the inline <applic> element representing the whole data module's
 * applicability. */
#define DEFAULT_DM_APPLIC_ID BAD_CAST "applic-0000"
xmlChar *dmApplicId;

/* XPath to select all elements which may have applicability annotations.
 *
 * Read from elements_list.h*/
xmlChar *applicElemsXPath;

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
	xmlChar *applicRefId = xmlGetProp(node, BAD_CAST "applicRefId");

	if (!applicRefId) {
		/* Inherit applicability from
		 * - an ancestor, or
		 * - the whole data module level */
		xmlNodePtr ancestor;

		ancestor = lastXPathNode(NULL, node, "ancestor::*[@applicRefId]");

		if (!ancestor) {
			xmlSetProp(node, BAD_CAST "applicRefId", dmApplicId);
		} else {
			xmlChar *ancestorApplic = xmlGetProp(ancestor, BAD_CAST "applicRefId");
			xmlSetProp(node, BAD_CAST "applicRefId", ancestorApplic);
			xmlFree(ancestorApplic);
		}
	}

	xmlFree(applicRefId);
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
		xmlChar *applicRefId = xmlGetProp(nodes->nodeTab[i], BAD_CAST "applicRefId");

		if (xmlStrcmp(applicRefId, applic) == 0) {
			xmlUnsetProp(nodes->nodeTab[i], BAD_CAST "applicRefId");
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

	if ((referencedApplicGroup = firstXPathNode(NULL, dmodule, ".//referencedApplicGroup"))) {
		xmlNodePtr applic;
		xmlNodePtr wholeDmApplic;

		wholeDmApplic = firstXPathNode(NULL, dmodule, ".//dmStatus/applic");
		applic = xmlAddChild(referencedApplicGroup, xmlCopyNode(wholeDmApplic, 1));

		xmlSetProp(applic, BAD_CAST "id", dmApplicId);
	}
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

void processFile(const char *in, const char *out, bool xincl)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	doc = xmlReadFile(in, NULL, 0);

	if (xincl) {
		xmlXIncludeProcess(doc);
	}

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST "//dmodule", ctx);

	processDmodules(obj->nodesetval);

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	xmlSaveFile(out, doc);

	xmlFreeDoc(doc);
}

void showHelp(void)
{
	puts("Usage: s1kd-asp [-a <ID>] [-fh?] [<modules>]");
	puts("");
	puts("Options:");
	puts("  -a <ID>  Use <ID> for DM-level applic.");
	puts("  -f       Overwrite input file(s).");
	puts("  -h -?    Show help/usage message.");
}

int main(int argc, char **argv)
{
	int i;
	bool overwrite = false;
	bool xincl = false;

	dmApplicId = xmlStrdup(DEFAULT_DM_APPLIC_ID);

	applicElemsXPath = xmlStrndup(elements_list, elements_list_len);

	while ((i = getopt(argc, argv, "a:fxh?")) != -1) {
		switch (i) {
			case 'a':
				xmlFree(dmApplicId);
				dmApplicId = xmlStrdup(BAD_CAST optarg);
				break;
			case 'f':
				overwrite = true;
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
		processFile("-", "-", xincl);
	} else {
		for (i = optind; i < argc; ++i) {
			processFile(argv[i], overwrite ? argv[i] : "-", xincl);
		}
	}

	xmlFree(dmApplicId);
	xmlFree(applicElemsXPath);

	xmlCleanupParser();

	return 0;
}
