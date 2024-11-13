#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/transform.h>
#include <libexslt/exslt.h>
#include "s1kd_tools.h"
#include "resources.h"

#define PROG_NAME "s1kd-aspp"
#define VERSION "5.1.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define WRN_PREFIX PROG_NAME ": WARNING: "
#define INF_PREFIX PROG_NAME ": INFO: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

#define W_MISSING_REF WRN_PREFIX "Could not read referenced object: %s\n"

#define I_PROCESS INF_PREFIX "Processing %s...\n"

/* ID for the inline <applic> element representing the whole data module's
 * applicability. */
#define DEFAULT_DM_APPLIC_ID BAD_CAST "app-0000"
static xmlChar *dmApplicId;

/* XPath to select all elements which may have applicability annotations.
 *
 * Read from elements_list.h*/
static xmlChar *applicElemsXPath;

/* Search for ACTs/CCTs recursively. */
static bool recursive_search = false;

/* Directory to start search for ACTs/CCTs in. */
static char *search_dir;

/* Overwrite existing display text in annotations. */
static bool overwriteDispText = true;

/* Verbose output. */
static enum verbosity { QUIET, NORMAL, VERBOSE } verbosity = NORMAL;

/* Assume objects were created with -N. */
static bool no_issue = false;

/* Delimiter for format strings. */
#define FMTSTR_DELIM '%'

/* Return the first node matching an XPath expression. */
static xmlNodePtr firstXPathNode(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
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
static xmlNodePtr lastXPathNode(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
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
static xmlChar *first_xpath_value(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
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
static void processNode(xmlNodePtr node)
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
static void processNodeSet(xmlNodeSetPtr nodes)
{
	int i;

	for (i = 0; i < nodes->nodeNr; ++i) {
		processNode(nodes->nodeTab[i]);
	}
}

/* Remove duplicate applicability annotations in document-order so that an
 * annotation is only shown when applicability changes. */
static void removeDuplicates(xmlNodeSetPtr nodes)
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
static void addDmApplic(xmlNodePtr dmodule)
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

static xmlDocPtr parse_disptext(xmlDocPtr config)
{
	xmlDocPtr doc1, doc2, res;
	xsltStylesheetPtr style;

	doc1 = read_xml_mem((const char *) disptext_xsl, disptext_xsl_len);
	style = xsltParseStylesheetDoc(doc1);

	doc2 = xsltApplyStylesheet(style, config, NULL);
	xsltFreeStylesheet(style);

	res = xmlCopyDoc(doc2, 1);
	xmlFreeDoc(doc2);

	return res;
}

static void dumpGenDispTextXsl(void)
{
	xmlDocPtr doc1, doc2;
	doc1 = read_xml_mem((const char *) disptext_xml, disptext_xml_len);
	doc2 = parse_disptext(doc1);
	xmlFreeDoc(doc1);
	save_xml_doc(doc2, "-");
	xmlFreeDoc(doc2);
}

static void dumpDispText(void)
{
	printf("%.*s", disptext_xml_len, disptext_xml);
}

/* Customize the display text based on a format string. */
static void apply_format_str(xmlDocPtr doc, const char *fmt)
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

static void generateDisplayText(xmlDocPtr doc, xmlNodePtr acts, xmlNodePtr ccts, xsltStylesheetPtr style)
{
	xmlDocPtr res, muxdoc;
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

	params[0] = "overwrite-display-text";
	params[1] = overwriteDispText ? "true()" : "false()";
	params[2] = NULL;

	res = xsltApplyStylesheet(style, muxdoc, params);

	new = xmlCopyNode(firstXPathNode(res, NULL, "/mux/*"), 1);
	old = xmlDocSetRootElement(doc, new);
	xmlFreeNode(old);

	xmlFreeDoc(res);
	xmlFreeDoc(muxdoc);
}

static void processDmodule(xmlNodePtr dmodule)
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

static void processDmodules(xmlNodeSetPtr dmodules)
{
	int i;

	for (i = 0; i < dmodules->nodeNr; ++i) {
		processDmodule(dmodules->nodeTab[i]);
	}
}

/* Find a data module filename in the current directory based on the dmRefIdent
 * element. */
static bool find_dmod_fname(char *dst, xmlNodePtr dmRefIdent)
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

	if (!no_issue) {
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
		} else if (language) {
			strcat(code, "_\?\?\?-\?\?");
		}
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

	if (find_csdb_object(dst, search_dir, code, is_dm, recursive_search)) {
		return true;
	}

	if (verbosity >= NORMAL) {
		fprintf(stderr, W_MISSING_REF, code);
	}
	return false;
}

/* Find the filename of a referenced ACT data module. */
static bool find_act_fname(char *dst, xmlDocPtr doc)
{
	xmlNodePtr actref;
	actref = firstXPathNode(doc, NULL, "//applicCrossRefTableRef/dmRef/dmRefIdent|//actref/refdm");
	return actref && find_dmod_fname(dst, actref);
}

/* Find the filename of a referenced PCT data module via the ACT. */
static bool find_cct_fname(char *dst, xmlDocPtr act)
{
	xmlNodePtr pctref;
	bool found;

	pctref = firstXPathNode(act, NULL, "//condCrossRefTableRef/dmRef/dmRefIdent|//cctref/refdm");
	found = pctref && find_dmod_fname(dst, pctref);

	return found;
}

/* Add cross-reference tables by searching for them in the current directory. */
static void find_cross_ref_tables(xmlDocPtr doc, xmlNodePtr acts, xmlNodePtr ccts)
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

/* Add tags containing the display text of the referenecd applic annotation. */
static void addTags(xmlDocPtr doc, const char *tags)
{
	xmlDocPtr styledoc, src, res;
	xsltStylesheetPtr style;
	const char *params[3];
	xmlNodePtr old;

	styledoc = read_xml_mem((const char *) addTags_xsl, addTags_xsl_len);
	style = xsltParseStylesheetDoc(styledoc);

	params[0] = "mode";
	if (strcmp(tags, "comment") == 0) {
		params[1] = "'comment'";
	} else if (strcmp(tags, "pi") == 0) {
		params[1] = "'pi'";
	} else {
		params[1] = "'remove'";
	}
	params[2] = NULL;

	src = xmlCopyDoc(doc, 1);
	res = xsltApplyStylesheet(style, src, params);
	xmlFreeDoc(src);

	old = xmlDocSetRootElement(doc, xmlCopyNode(xmlDocGetRootElement(res), 1));
	xmlFreeNode(old);

	xmlFreeDoc(res);
	xsltFreeStylesheet(style);
}

/* Recursively delete display text nodes. */
static void deleteDisplayTextNode(xmlNodePtr node)
{
	xmlNodePtr cur;

	/* 3.0-: displaytext
	 * 4.0+: displayText
	 *
	 * Ignore display text nodes without any siblings. */
	if ((xmlStrcmp(node->name, BAD_CAST "displayText") == 0 || xmlStrcmp(node->name, BAD_CAST "displaytext") == 0) && xmlChildElementCount(node->parent) > 1) {
		xmlUnlinkNode(node);
		xmlFreeNode(node);
		return;
	}

	/* Call recursively on all children. */
	cur = node->children;
	while (cur) {
		xmlNodePtr next = cur->next;
		deleteDisplayTextNode(cur);
		cur = next;
	}

}

/* Delete all display text nodes in a document. */
static void deleteDisplayText(xmlDocPtr doc)
{
	deleteDisplayTextNode(xmlDocGetRootElement(doc));
}

static void processFile(const char *in, const char *out, bool process,
	bool genDispText, bool delDispText, xmlNodePtr acts, xmlNodePtr ccts,
	bool findcts, xsltStylesheetPtr style, const char *tags)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr all_acts, all_ccts;

	if (verbosity >= VERBOSE) {
		fprintf(stderr, I_PROCESS, in);
	}

	doc = read_xml_doc(in);

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

	if (delDispText) {
		deleteDisplayText(doc);
	} else if (genDispText) {
		generateDisplayText(doc, all_acts, all_ccts, style);
	}

	if (tags) {
		addTags(doc, tags);
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

static void process_list(const char *path, bool overwrite, bool process,
	bool genDispText, bool delDispText, xmlNodePtr acts, xmlNodePtr ccts,
	bool findcts, xsltStylesheetPtr style, const char *tags)
{
	FILE *f;
	char line[PATH_MAX];

	if (path) {
		if (!(f = fopen(path, "r"))) {
			if (verbosity >= NORMAL) {
				fprintf(stderr, E_BAD_LIST, path);
			}
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");
		processFile(line, overwrite ? line : "-", process, genDispText, delDispText, acts, ccts, findcts, style, tags);
	}

	if (path) {
		fclose(f);
	}
}

static void show_help(void)
{
	puts("Usage:");
	puts("  " PROG_NAME " [options] [<object> ...]");
	puts("");
	puts("Options:");
	puts("  -., --dump-disptext   Dump the built-in .disptext file.");
	puts("  -,, --dump-xsl        Dump the built-in XSLT for generating display text.");
	puts("  -A, --act <ACT>       Use <ACT> when generating display text.");
	puts("  -a, --id <ID>  Use <ID> for DM-level applic.");
	puts("  -C, --cct <CCT>       Use <CCT> when generating display text.");
	puts("  -c, --search          Search for ACT/CCT data modules.");
	puts("  -D, --delete          Remove all display text.");
	puts("  -d, --dir <dir>       Directory to start search for ACT/CCT in.");
	puts("  -F, --format <fmt>    Use a custom format string for generating display text.");
	puts("  -f, --overwrite       Overwrite input file(s).");
	puts("  -G, --disptext        Specify .disptext file.");
	puts("  -g, --generate        Generate display text for applicability annotations.");
	puts("  -k, --keep            Do not overwrite existing display text.");
	puts("  -l, --list            Treat input as list of modules.");
	puts("  -N, --omit-issue      Assume issue/inwork number are omitted.");
	puts("  -p, --presentation    Convert semantic applicability to presentation applicability.");
	puts("  -q, --quiet           Quiet mode.");
	puts("  -r, --recursive       Search for ACT/CCT recursively.");
	puts("  -t, --tags <mode>     Add display text tags before elements with applicability.");
	puts("  -v, --verbose         Verbose output.");
	puts("  -x, --xsl <XSL>       Use custom XSLT script to generate display text.");
	puts("  -h, -?, --help        Show help/usage message.");
	puts("  --version             Show version information.");
	puts("  <object> ...          CSDB objects to process.");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s, libxslt %s and libexslt %s\n",
		xmlParserVersion, xsltEngineVersion, exsltLibraryVersion);
}

int main(int argc, char **argv)
{
	int i;
	bool overwrite = false;
	bool genDispText = false;
	bool process = false;
	bool delDispText = false;
	bool findcts = false;
	bool islist = false;
	char *format = NULL;
	char *tags = NULL;
	char *customGenDispTextFile = NULL;
	xmlDocPtr disptext = NULL;
	xmlDocPtr styledoc;
	xsltStylesheetPtr style;
	
	xmlNodePtr acts, ccts;

	const char *sopts = ".,A:a:C:cDd:F:fG:gklNpqrt:vx:h?";
	struct option lopts[] = {
		{"version"      , no_argument      , 0, 0},
		{"help"         , no_argument      , 0, 'h'},
		{"dump-disptext", no_argument      , 0, '.'},
		{"dump-xsl"     , no_argument      , 0, ','},
		{"act"          , required_argument, 0, 'A'},
		{"id"           , required_argument, 0, 'a'},
		{"cct"          , required_argument, 0, 'C'},
		{"search"       , no_argument      , 0, 'c'},
		{"delete"       , no_argument      , 0, 'D'},
		{"dir"          , required_argument, 0, 'd'},
		{"format"       , required_argument, 0, 'F'},
		{"overwrite"    , no_argument      , 0, 'f'},
		{"disptext"     , required_argument, 0, 'G'},
		{"generate"     , no_argument      , 0, 'g'},
		{"keep"         , no_argument      , 0, 'k'},
		{"list"         , no_argument      , 0, 'l'},
		{"omit-issue"   , no_argument      , 0, 'N'},
		{"presentation" , no_argument      , 0, 'p'},
		{"quiet"        , no_argument      , 0, 'q'},
		{"recursive"    , no_argument      , 0, 'r'},
		{"tags"         , required_argument, 0, 't'},
		{"verbose"      , no_argument      , 0, 'v'},
		{"xsl"          , required_argument, 0, 'x'},
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
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg)
				break;
			case '.':
				dumpDispText();
				return 0;
			case ',':
				dumpGenDispTextXsl();
				return 0;
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
				delDispText = true;
				break;
			case 'd':
				free(search_dir);
				search_dir = strdup(optarg);
				break;
			case 'F':
				genDispText = true;
				free(format);
				format = strdup(optarg);
				break;
			case 'f':
				overwrite = true;
				break;
			case 'G':
				genDispText = true;
				xmlFreeDoc(disptext);
				disptext = read_xml_doc(optarg);
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
			case 'N':
				no_issue = true;
				break;
			case 'p':
				process = true;
				break;
			case 'q':
				--verbosity;
				break;
			case 'r':
				recursive_search = true;
				break;
			case 't':
				free(tags);
				tags = strdup(optarg);
				break;
			case 'v':
				++verbosity;
				break;
			case 'x':
				genDispText = true;
				free(customGenDispTextFile);
				customGenDispTextFile = strdup(optarg);
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (customGenDispTextFile == NULL) {
		if (disptext == NULL) {
			char disptext_fname[PATH_MAX];

			if (find_config(disptext_fname, DEFAULT_DISPTEXT_FNAME)) {
				disptext = read_xml_doc(disptext_fname);
			} else {
				disptext = read_xml_mem((const char *) disptext_xml, disptext_xml_len);
			}
		}

		styledoc = parse_disptext(disptext);
	} else {
		styledoc = read_xml_doc(customGenDispTextFile);
	}

	if (format) {
		apply_format_str(styledoc, format);
	}
	style = xsltParseStylesheetDoc(styledoc);

	if (optind >= argc) {
		if (islist) {
			process_list(NULL, overwrite, process, genDispText, delDispText, acts, ccts, findcts, style, tags);
		} else {
			processFile("-", "-", process, genDispText, delDispText, acts, ccts, findcts, style, tags);
		}
	} else {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				process_list(argv[i], overwrite, process, genDispText, delDispText, acts, ccts, findcts, style, tags);
			} else {
				processFile(argv[i], overwrite ? argv[i] : "-", process, genDispText, delDispText, acts, ccts, findcts, style, tags);
			}
		}
	}

	xmlFree(dmApplicId);
	xmlFree(applicElemsXPath);

	xmlFreeNode(acts);
	xmlFreeNode(ccts);

	free(customGenDispTextFile);
	free(search_dir);
	free(format);
	free(tags);
	xmlFreeDoc(disptext);
	xsltFreeStylesheet(style);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
