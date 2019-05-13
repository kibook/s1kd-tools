/* Applicability Preprocessor
 *
 * Preprocesses a data module's applicability annotations (@applicRefId) in to a
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
#include "s1kd_tools.h"
#include "elements_list.h"
#include "generateDisplayText.h"
#include "identity.h"

#define PROG_NAME "s1kd-aspp"
#define VERSION "2.6.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define INF_PREFIX PROG_NAME ": INFO: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

#define I_PROCESS INF_PREFIX "Processing %s...\n"

/* ID for the inline <applic> element representing the whole data module's
 * applicability. */
#define DEFAULT_DM_APPLIC_ID BAD_CAST "app-0000"
xmlChar *dmApplicId;

/* XPath to select all elements which may have applicability annotations.
 *
 * Read from elements_list.h*/
xmlChar *applicElemsXPath;

/* Custom XSL for generating display text. */
char *customGenDispTextXsl = NULL;

/* Search for ACTs/CCTs recursively. */
bool recursive_search = false;

/* Directory to start search for ACTs/CCTs in. */
char *search_dir;

/* Overwrite existing display text in annotations. */
bool overwriteDispText = true;

/* Verbose output. */
bool verbose = false;

/* Delimiter for format strings. */
#define FMTSTR_DELIM '%'

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

/* Return the value of the first node matching an XPath expression. */
xmlChar *first_xpath_value(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
{
	return xmlNodeGetContent(firstXPathNode(doc, node, xpath));
}

/* Set an explicit applicability annotation on a node.
 *
 * Nodes with the attribute @applicRefId already have an explicit annotation.
 *
 * Nodes without @applicRefId have an implicit annotation inherited from their
 * parent/ancestor which does have an explicit annotation, or ultimately from
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

/* Remove duplicate applicability annotations in document-order so that an
 * annotation is only shown when applicability changes. */
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

	identity = read_xml_mem((const char *) ___common_identity_xsl, ___common_identity_xsl_len);
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

/* Customize the display text based on a format string. */
void apply_format_str(xmlDocPtr doc, const char *fmt)
{
	xmlNodePtr t, c;
	int i;

	/* Get the assert text template. */
	t = firstXPathNode(doc, NULL, "//*[@match='assert' and @mode='text']");

	if (!t) {
		return;
	}

	/* Clear the template. */
	c = t->children;
	while (c) {
		xmlNodePtr n;
		n = c->next;
		xmlUnlinkNode(c);
		xmlFreeNode(c);
		c = n;
	}

	/* Parse the format string and generate the new template. */
	for (i = 0; fmt[i]; ++i) {
		xmlChar s[2] = {0};

		if (fmt[i] == FMTSTR_DELIM) {
			if (fmt[i + 1] == FMTSTR_DELIM) {
				s[0] = FMTSTR_DELIM;
				xmlNewChild(t, t->nsDef, BAD_CAST "text", s);
				++i;
			} else {
				const char *k, *e;
				int n;

				k = fmt + i + 1;
				e = strchr(k, FMTSTR_DELIM);
				if (!e) break;
				n = e - k;

				if (strncmp(k, "name", n) == 0) {
					c = xmlNewChild(t, t->nsDef, BAD_CAST "call-template", NULL);
					xmlSetProp(c, BAD_CAST "name", BAD_CAST "applicPropertyName");
				} else if (strncmp(k, "values", n) == 0) {
					c = xmlNewChild(t, t->nsDef, BAD_CAST "call-template", NULL);
					xmlSetProp(c, BAD_CAST "name", BAD_CAST "applicPropertyVal");
				}

				i += n + 1;
			}
		} else {
			if (fmt[i] == '\\') {
				switch (fmt[i + 1]) {
					case 'n': s[0] = '\n'; ++i; break;
					case 't': s[0] = '\t'; ++i; break;
					default: s[0] = fmt[i]; break;
				}
			} else {
				s[0] = fmt[i];
			}
			xmlNewChild(t, t->nsDef, BAD_CAST "text", s);
		}
	}
}

void generateDisplayText(xmlDocPtr doc, xmlNodePtr acts, xmlNodePtr ccts, const char *format)
{
	xmlDocPtr styledoc, res, muxdoc;
	xsltStylesheetPtr style;
	xmlNodePtr mux, cur, muxacts, muxccts, new, old;
	const char *params[3];

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
		act = read_xml_doc((char *) path);
		xmlAddChild(muxacts, xmlCopyNode(xmlDocGetRootElement(act), 1));
		xmlFreeDoc(act);
		xmlFree(path);
	}
	for (cur = ccts->children; cur; cur = cur->next) {
		xmlDocPtr cct;
		xmlChar *path;
		path = xmlNodeGetContent(cur);
		cct = read_xml_doc((char *) path);
		xmlAddChild(muxccts, xmlCopyNode(xmlDocGetRootElement(cct), 1));
		xmlFreeDoc(cct);
		xmlFree(path);
	}

	if (customGenDispTextXsl) {
		styledoc = read_xml_doc(customGenDispTextXsl);
	} else {
		styledoc = read_xml_mem((const char *) generateDisplayText_xsl,
			generateDisplayText_xsl_len);
	}

	addIdentity(styledoc);

	if (format) {
		apply_format_str(styledoc, format);
	}

	style = xsltParseStylesheetDoc(styledoc);

	params[0] = "overwrite-display-text";
	params[1] = overwriteDispText ? "true()" : "false()";
	params[2] = NULL;

	res = xsltApplyStylesheet(style, muxdoc, params);

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

/* Determine if the file is a data module. */
bool is_dm(const char *name)
{
	return strncmp(name, "DMC-", 4) == 0 && strncasecmp(name + strlen(name) - 4, ".XML", 4) == 0;
}

/* Find a data module filename in the current directory based on the dmRefIdent
 * element. */
bool find_dmod_fname(char *dst, xmlNodePtr dmRefIdent)
{
	char *model_ident_code;
	char *system_diff_code;
	char *system_code;
	char *sub_system_code;
	char *sub_sub_system_code;
	char *assy_code;
	char *disassy_code;
	char *disassy_code_variant;
	char *info_code;
	char *info_code_variant;
	char *item_location_code;
	char *learn_code;
	char *learn_event_code;
	char code[64];
	xmlNodePtr dmCode, issueInfo, language;

	dmCode = firstXPathNode(NULL, dmRefIdent, "dmCode|avee");
	issueInfo = firstXPathNode(NULL, dmRefIdent, "issueInfo|issno");
	language = firstXPathNode(NULL, dmRefIdent, "language");

	model_ident_code     = (char *) first_xpath_value(NULL, dmCode, "modelic|@modelIdentCode");
	system_diff_code     = (char *) first_xpath_value(NULL, dmCode, "sdc|@systemDiffCode");
	system_code          = (char *) first_xpath_value(NULL, dmCode, "chapnum|@systemCode");
	sub_system_code      = (char *) first_xpath_value(NULL, dmCode, "section|@subSystemCode");
	sub_sub_system_code  = (char *) first_xpath_value(NULL, dmCode, "subsect|@subSubSystemCode");
	assy_code            = (char *) first_xpath_value(NULL, dmCode, "subject|@assyCode");
	disassy_code         = (char *) first_xpath_value(NULL, dmCode, "discode|@disassyCode");
	disassy_code_variant = (char *) first_xpath_value(NULL, dmCode, "discodev|@disassyCodeVariant");
	info_code            = (char *) first_xpath_value(NULL, dmCode, "incode|@infoCode");
	info_code_variant    = (char *) first_xpath_value(NULL, dmCode, "incodev|@infoCodeVariant");
	item_location_code   = (char *) first_xpath_value(NULL, dmCode, "itemloc|@itemLocationCode");
	learn_code           = (char *) first_xpath_value(NULL, dmCode, "@learnCode");
	learn_event_code     = (char *) first_xpath_value(NULL, dmCode, "@learnEventCode");

	snprintf(code, 64, "DMC-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
		model_ident_code,
		system_diff_code,
		system_code,
		sub_system_code,
		sub_sub_system_code,
		assy_code,
		disassy_code,
		disassy_code_variant,
		info_code,
		info_code_variant,
		item_location_code);

	xmlFree(model_ident_code);
	xmlFree(system_diff_code);
	xmlFree(system_code);
	xmlFree(sub_system_code);
	xmlFree(sub_sub_system_code);
	xmlFree(assy_code);
	xmlFree(disassy_code);
	xmlFree(disassy_code_variant);
	xmlFree(info_code);
	xmlFree(info_code_variant);
	xmlFree(item_location_code);

	if (learn_code) {
		char learn[8];
		snprintf(learn, 8, "-%s%s", learn_code, learn_event_code);
		strcat(code, learn);
	}

	xmlFree(learn_code);
	xmlFree(learn_event_code);

	if (issueInfo) {
		char *issue_number;
		char *in_work;
		char iss[8];

		issue_number = (char *) first_xpath_value(NULL, issueInfo, "@issno|@issueNumber");
		in_work      = (char *) first_xpath_value(NULL, issueInfo, "@inwork|@inWork");

		snprintf(iss, 8, "_%s-%s", issue_number, in_work ? in_work : "00");
		strcat(code, iss);

		xmlFree(issue_number);
		xmlFree(in_work);
	}

	if (language) {
		char *language_iso_code;
		char *country_iso_code;
		char lang[8];

		language_iso_code = (char *) first_xpath_value(NULL, language, "@language|@languageIsoCode");
		country_iso_code  = (char *) first_xpath_value(NULL, language, "@country|@countryIsoCode");

		snprintf(lang, 8, "_%s-%s", language_iso_code, country_iso_code);
		strcat(code, lang);

		xmlFree(language_iso_code);
		xmlFree(country_iso_code);
	}

	return find_csdb_object(dst, search_dir, code, is_dm, recursive_search);
}

/* Find the filename of a referenced ACT data module. */
bool find_act_fname(char *dst, xmlDocPtr doc)
{
	xmlNodePtr actref;
	actref = firstXPathNode(doc, NULL, "//applicCrossRefTableRef/dmRef/dmRefIdent|//actref/refdm");
	return actref && find_dmod_fname(dst, actref);
}

/* Find the filename of a referenced PCT data module via the ACT. */
bool find_cct_fname(char *dst, xmlDocPtr act)
{
	xmlNodePtr pctref;
	bool found;

	pctref = firstXPathNode(act, NULL, "//condCrossRefTableRef/dmRef/dmRefIdent|//cctref/refdm");
	found = pctref && find_dmod_fname(dst, pctref);

	return found;
}

/* Add cross-reference tables by searching for them in the current directory. */
void find_cross_ref_tables(xmlDocPtr doc, xmlNodePtr acts, xmlNodePtr ccts)
{
	char act_fname[PATH_MAX];
	xmlDocPtr act = NULL;

	if (find_act_fname(act_fname, doc) && (act = read_xml_doc(act_fname))) {
		char cct_fname[PATH_MAX];

		xmlNewChild(acts, NULL, BAD_CAST "act", BAD_CAST act_fname);

		if (find_cct_fname(cct_fname, act)) {
			xmlNewChild(ccts, NULL, BAD_CAST "cct", BAD_CAST cct_fname);
		}

		xmlFreeDoc(act);
	}
}

void processFile(const char *in, const char *out, bool xincl, bool process,
	bool genDispText, xmlNodePtr acts, xmlNodePtr ccts, bool findcts,
	const char *format)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr all_acts, all_ccts;

	if (verbose) {
		fprintf(stderr, I_PROCESS, in);
	}

	doc = read_xml_doc(in);

	if (xincl) {
		xmlXIncludeProcess(doc);
	}

	if (findcts) {
		/* Copy the user-defined ACTs/CCTs. */
		all_acts = xmlCopyNode(acts, 1);
		all_ccts = xmlCopyNode(ccts, 1);
		/* Find the ACT/CCT referenced by the current DM. */
		find_cross_ref_tables(doc, all_acts, all_ccts);
	} else {
		/* Only use the user-defined ACTs/CCTs. */
		all_acts = acts;
		all_ccts = ccts;
	}

	if (process) {
		ctx = xmlXPathNewContext(doc);
		obj = xmlXPathEvalExpression(BAD_CAST "//dmodule", ctx);

		processDmodules(obj->nodesetval);

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);
	}

	if (genDispText) {
		generateDisplayText(doc, all_acts, all_ccts, format);
	}

	save_xml_doc(doc, out);

	/* The next data module could reference a different ACT/CCT, so
	 * the list must be cleared. */
	if (findcts) {
		xmlFreeNode(all_acts);
		xmlFreeNode(all_ccts);
	}

	xmlFreeDoc(doc);
}

void process_list(const char *path, bool overwrite, bool xincl, bool process,
	bool genDispText, xmlNodePtr acts, xmlNodePtr ccts, bool findcts,
	const char *format)
{
	FILE *f;
	char line[PATH_MAX];

	if (path) {
		if (!(f = fopen(path, "r"))) {
			fprintf(stderr, E_BAD_LIST, path);
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");
		processFile(line, overwrite ? line : "-", xincl, process, genDispText, acts, ccts, findcts, format);
	}

	if (path) {
		fclose(f);
	}
}

void showHelp(void)
{
	puts("Usage:");
	puts("  " PROG_NAME " -h?");
	puts("  " PROG_NAME " -D");
	puts("  " PROG_NAME " -g [-A <ACT>] [-C <CCT>] [-d <dir>] [-F <fmt>] [-G <XSL>] [-cfklrvx] [<object>...]");
	puts("  " PROG_NAME " -p [-a <ID>] [-flvx] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -A <ACT>      Use <ACT> when generating display text.");
	puts("  -a <ID>       Use <ID> for DM-level applic.");
	puts("  -C <CCT>      Use <CCT> when generating display text.");
	puts("  -c            Search for ACT/CCT data modules.");
	puts("  -D            Dump built-in XSLT for generating display text.");
	puts("  -d <dir>      Directory to start search for ACT/CCT in.");
	puts("  -F <fmt>      Use a custom format string for generating display text.");
	puts("  -f            Overwrite input file(s).");
	puts("  -G <XSL>      Use custom XSLT script to generate display text.");
	puts("  -g            Generate display text for applicability annotations.");
	puts("  -k            Do not overwrite existing display text.");
	puts("  -l            Treat input as list of modules.");
	puts("  -p            Convert semantic applicability to presentation applicability.");
	puts("  -r            Search for ACT/CCT recursively.");
	puts("  -v            Verbose output.");
	puts("  -x            Perform XInclude processing.");
	puts("  -h -?         Show help/usage message.");
	puts("  --version     Show version information.");
	puts("  <object>...   CSDB objects to process.");
	LIBXML2_PARSE_LONGOPT_HELP
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s, libxslt %s and libexslt %s\n",
		xmlParserVersion, xsltEngineVersion, exsltLibraryVersion);
}

int main(int argc, char **argv)
{
	int i;
	bool overwrite = false;
	bool xincl = false;
	bool genDispText = false;
	bool process = false;
	bool findcts = false;
	bool islist = false;
	char *format = NULL;
	
	xmlNodePtr acts, ccts;

	const char *sopts = "A:a:C:cDd:F:fG:gklprvxh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	exsltRegisterAll();

	dmApplicId = xmlStrdup(DEFAULT_DM_APPLIC_ID);

	applicElemsXPath = xmlStrndup(elements_list, elements_list_len);

	acts = xmlNewNode(NULL, BAD_CAST "acts");
	ccts = xmlNewNode(NULL, BAD_CAST "ccts");

	search_dir = strdup(".");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'A':
				xmlNewChild(acts, NULL, BAD_CAST "act", BAD_CAST optarg);
				findcts = false;
				break;
			case 'a':
				xmlFree(dmApplicId);
				dmApplicId = xmlStrdup(BAD_CAST optarg);
				break;
			case 'C':
				xmlNewChild(ccts, NULL, BAD_CAST "cct", BAD_CAST optarg);
				findcts = false;
				break;
			case 'c':
				findcts = true;
				break;
			case 'D':
				dumpGenDispTextXsl();
				exit(0);
			case 'd':
				free(search_dir);
				search_dir = strdup(optarg);
				break;
			case 'F':
				genDispText = true;
				format = strdup(optarg);
				break;
			case 'f':
				overwrite = true;
				break;
			case 'G':
				genDispText = true;
				customGenDispTextXsl = strdup(optarg);
				break;
			case 'g':
				genDispText = true;
				break;
			case 'k':
				overwriteDispText = false;
				break;
			case 'l':
				islist = true;
				break;
			case 'p':
				process = true;
				break;
			case 'r':
				recursive_search = true;
				break;
			case 'v':
				verbose = true;
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
		if (islist) {
			process_list(NULL, overwrite, xincl, process, genDispText, acts, ccts, findcts, format);
		} else {
			processFile("-", "-", xincl, process, genDispText, acts, ccts, findcts, format);
		}
	} else {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				process_list(argv[i], overwrite, xincl, process,
					genDispText, acts, ccts, findcts, format);
			} else {
				processFile(argv[i], overwrite ? argv[i] : "-",
					xincl, process, genDispText, acts,
					ccts, findcts, format);
			}
		}
	}

	xmlFree(dmApplicId);
	xmlFree(applicElemsXPath);

	xmlFreeNode(acts);
	xmlFreeNode(ccts);

	free(customGenDispTextXsl);
	free(search_dir);
	free(format);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
