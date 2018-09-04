#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xinclude.h>
#include <libxslt/transform.h>
#include "s1kd_tools.h"
#include "xsl.h"

#define PROG_NAME "s1kd-fmgen"
#define VERSION "1.4.2"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_NO_PM 1
#define EXIT_NO_TYPE 2
#define EXIT_BAD_TYPE 3
#define EXIT_NO_INFOCODE 4

#define S_NO_PM_ERR ERR_PREFIX "No publication module.\n"
#define S_NO_TYPE_ERR ERR_PREFIX "No FM type specified.\n"
#define S_BAD_TYPE_ERR ERR_PREFIX "Unknown front matter type: %s\n"
#define S_NO_INFOCODE_ERR ERR_PREFIX "No FM type associated with info code: %s\n"
#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const char *expr)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(doc ? doc : node->doc);
	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST expr, ctx);

	first = xmlXPathNodeSetIsEmpty(obj->nodesetval) ? NULL : obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

char *first_xpath_string(xmlDocPtr doc, xmlNodePtr node, const char *expr)
{
	return (char *) xmlNodeGetContent(first_xpath_node(doc, node, expr));
}

xmlDocPtr transform_doc(xmlDocPtr doc, const char *xslpath, const char **params)
{
	xmlDocPtr styledoc, res;
	xsltStylesheetPtr style;

	styledoc = xmlReadFile(xslpath, NULL, PARSE_OPTS);

	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, params);

	xsltFreeStylesheet(style);

	return res;
}

xmlDocPtr transform_doc_builtin(xmlDocPtr doc, unsigned char *xsl, unsigned int len)
{
	xmlDocPtr styledoc, res;
	xsltStylesheetPtr style;

	styledoc = xmlReadMemory((const char *) xsl, len, NULL, NULL, PARSE_OPTS);

	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);

	xsltFreeStylesheet(style);

	return res;
}

xmlDocPtr generate_tp(xmlDocPtr doc)
{
	return transform_doc_builtin(doc, xsl_tp_xsl, xsl_tp_xsl_len);
}

xmlDocPtr generate_toc(xmlDocPtr doc)
{
	return transform_doc_builtin(doc, xsl_toc_xsl, xsl_toc_xsl_len);
}

xmlDocPtr generate_high(xmlDocPtr doc)
{
	return transform_doc_builtin(doc, xsl_high_xsl, xsl_high_xsl_len);
}

xmlDocPtr generate_loedm(xmlDocPtr doc)
{
	return transform_doc_builtin(doc, xsl_loedm_xsl, xsl_loedm_xsl_len);
}

xmlDocPtr generate_fm_content_for_type(xmlDocPtr doc, const char *type, const char *xslpath, const char **params)
{
	xmlDocPtr res = NULL;

	if (strcasecmp(type, "TP") == 0) {
		res = generate_tp(doc);
	} else if (strcasecmp(type, "TOC") == 0) {
		res = generate_toc(doc);
	} else if (strcasecmp(type, "HIGH") == 0) {
		res = generate_high(doc);
	} else if (strcasecmp(type, "LOEDM") == 0) {
		res = generate_loedm(doc);
	} else {
		fprintf(stderr, S_BAD_TYPE_ERR, type);
		exit(EXIT_BAD_TYPE);
	}

	if (xslpath) {
		xmlDocPtr old;

		old = res;
		res = transform_doc(old, xslpath, params);

		xmlFreeDoc(old);
	}

	return res;
}

char *find_fmtype(xmlDocPtr fmtypes, char *incode)
{
	char xpath[256];
	snprintf(xpath, 256, "//fm[@infoCode='%s']/@type", incode);
	return first_xpath_string(fmtypes, NULL, xpath);
}

void generate_fm_content_for_dm(xmlDocPtr pm, const char *dmpath, xmlDocPtr fmtypes, const char *fmtype, bool overwrite, const char *xslpath, const char **params)
{
	xmlDocPtr doc, res = NULL;
	char *type;
	xmlNodePtr content;

	doc = xmlReadFile(dmpath, NULL, PARSE_OPTS);

	if (fmtype) {
		type = strdup(fmtype);
	} else {
		char *incode;

		incode = first_xpath_string(doc, NULL, "//@infoCode|//incode");

		type = find_fmtype(fmtypes, incode);

		if (!type) {
			fprintf(stderr, S_NO_INFOCODE_ERR, incode);
			exit(EXIT_NO_INFOCODE);
		}

		xmlFree(incode);
	}

	res = generate_fm_content_for_type(pm, type, xslpath, params);

	xmlFree(type);

	content = first_xpath_node(doc, NULL, "//content");
	xmlAddNextSibling(content, xmlCopyNode(xmlDocGetRootElement(res), 1));
	xmlUnlinkNode(content);
	xmlFreeNode(content);

	if (overwrite) {
		xmlSaveFile(dmpath, doc);
	} else {
		xmlSaveFile("-", doc);
	}

	xmlFreeDoc(doc);
	xmlFreeDoc(res);
}

void generate_fm_content_for_list(xmlDocPtr pm, const char *path, xmlDocPtr fmtypes, const char *fmtype, bool overwrite, const char *xslpath, const char **params)
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
		generate_fm_content_for_dm(pm, line, fmtypes, fmtype, overwrite, xslpath, params);
	}

	if (path) {
		fclose(f);
	}
}

void dump_fmtypes_xml(void)
{
	xmlDocPtr doc;
	doc = xmlReadMemory((const char *) fmtypes_xml, fmtypes_xml_len, NULL, NULL, PARSE_OPTS);
	xmlSaveFile("-", doc);
	xmlFreeDoc(doc);
}

void dump_fmtypes_txt(void)
{
	printf("%.*s", fmtypes_txt_len, fmtypes_txt);
}

xmlDocPtr read_fmtypes(const char *path)
{
	xmlDocPtr doc;

	if (!(doc = xmlReadFile(path, NULL, PARSE_OPTS | XML_PARSE_NOWARNING | XML_PARSE_NOERROR))) {
		FILE *f;
		char line[256];
		xmlNodePtr root;

		doc = xmlNewDoc(BAD_CAST "1.0");
		root = xmlNewNode(NULL, BAD_CAST "fmtypes");
		xmlDocSetRootElement(doc, root);

		f = fopen(path, "r");

		while (fgets(line, 256, f)) {
			char *incode, *type;
			xmlNodePtr fm;

			incode = strtok(line, " \t");
			type   = strtok(NULL, "\r\n");

			fm = xmlNewChild(root, NULL, BAD_CAST "fm", NULL);
			xmlSetProp(fm, BAD_CAST "infoCode", BAD_CAST incode);
			xmlSetProp(fm, BAD_CAST "type", BAD_CAST type);
		}

		fclose(f);
	}

	return doc;
}

char *real_path(const char *path, char *real)
{
	#ifdef _WIN32
	if (!GetFullPathName(path, PATH_MAX, real, NULL)) {
	#else
	if (!realpath(path, real)) {
	#endif
		strcpy(real, path);
	}
	return real;
}

/* Search up the directory tree to find a configuration file. */
bool find_config(char *dst, const char *name)
{
	char cwd[PATH_MAX], prev[PATH_MAX];
	bool found = true;

	real_path(".", cwd);
	strcpy(prev, cwd);

	while (access(name, F_OK) == -1) {
		char cur[PATH_MAX];

		if (chdir("..") || strcmp(real_path(".", cur), prev) == 0) {
			found = false;
			break;
		}

		strcpy(prev, cur);
	}

	if (found) {
		real_path(name, dst);
	}

	return chdir(cwd) == 0 && found;
}

void add_param(xmlNodePtr params, char *s)
{
	char *n, *v;
	xmlNodePtr p;

	n = strtok(s, "=");
	v = strtok(NULL, "");

	p = xmlNewChild(params, NULL, BAD_CAST "param", NULL);
	xmlSetProp(p, BAD_CAST "name", BAD_CAST n);
	xmlSetProp(p, BAD_CAST "value", BAD_CAST v);
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-F <FMTYPES>] [-P <PM>] [-X <XSL> [-p <name>=<val> ...]] [-,flxh?] (-t <TYPE>|<DM>...)");
	puts("");
	puts("Options:");
	puts("  -,                 Dump the built-in .fmtypes file in XML format.");
	puts("  -.                 Dump the built-in .fmtypes file in simple text format.");
	puts("  -h -?              Show usage message.");
	puts("  -F <FMTYPES>       Specify .fmtypes file.");
	puts("  -f                 Overwrite input data modules.");
	puts("  -l                 Treat input as list of data modules.");
	puts("  -P <PM>            Generate front matter from the specified PM.");
	puts("  -p <name>=<value>  Pass parameters to the XSLT specified with -X.");
	puts("  -t <TYPE>          Generate the specified type of front matter.");
	puts("  -X <XSL>           Transform generated contents.");
	puts("  -x                 Do XInclude processing.");
	puts("  --version          Show version information.");
	puts("  <DM>               Generate front matter content based on the specified data modules.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

int main(int argc, char **argv)
{
	int i;

	const char *sopts = ",.F:flP:p:t:X:xh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	char *pmpath = NULL;
	char *fmtype = NULL;

	xmlDocPtr pm, fmtypes = NULL;

	bool overwrite = false;
	bool xincl = false;
	bool islist = false;
	char *xslpath = NULL;
	xmlNodePtr params_node;
	int nparams = 0;
	const char **params = NULL;

	params_node = xmlNewNode(NULL, BAD_CAST "params");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case ',':
				dump_fmtypes_xml();
				return 0;
			case '.':
				dump_fmtypes_txt();
				return 0;
			case 'F':
				if (!fmtypes) {
					fmtypes = read_fmtypes(optarg);
				}
				break;
			case 'f':
				overwrite = true;
				break;
			case 'l':
				islist = true;
				break;
			case 'P':
				pmpath = strdup(optarg);
				break;
			case 'p':
				add_param(params_node, optarg);
				++nparams;
				break;
			case 't':
				fmtype = strdup(optarg);
				break;
			case 'X':
				xslpath = strdup(optarg);
				break;
			case 'x':
				xincl = true;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (nparams > 0) {
		xmlNodePtr p;
		int n = 0;

		params = malloc((nparams * 2 + 1) * sizeof(char *));

		for (p = params_node->children; p; p = p->next) {
			char *name, *value;

			name  = (char *) xmlGetProp(p, BAD_CAST "name");
			value = (char *) xmlGetProp(p, BAD_CAST "value");

			params[n++] = name;
			params[n++] = value;
		}

		params[n] = NULL;
	}
	xmlFreeNode(params_node);

	if (!fmtypes) {
		char fmtypes_fname[PATH_MAX];

		if (find_config(fmtypes_fname, DEFAULT_FMTYPES_FNAME)) {
			fmtypes = read_fmtypes(fmtypes_fname);
		} else {
			fmtypes = xmlReadMemory((const char *) fmtypes_xml, fmtypes_xml_len, NULL, NULL, 0);
		}
	}

	if (!pmpath) {
		pmpath = strdup("-");
	}

	pm = xmlReadFile(pmpath, NULL, PARSE_OPTS);

	if (xincl) {
		xmlXIncludeProcess(pm);
	}

	if (optind < argc) {
		void (*gen_fn)(xmlDocPtr, const char *, xmlDocPtr, const char *,
			bool, const char *, const char **);

		if (islist) {
			gen_fn = generate_fm_content_for_list;
		} else {
			gen_fn = generate_fm_content_for_dm;
		}

		for (i = optind; i < argc; ++i) {
			gen_fn(pm, argv[i], fmtypes, fmtype, overwrite, xslpath, params);
		}
	} else if (fmtype) {
		xmlDocPtr res;
		res = generate_fm_content_for_type(pm, fmtype, xslpath, params);
		xmlSaveFile("-", res);
		xmlFreeDoc(res);
	} else if (islist) {
		generate_fm_content_for_list(pm, NULL, fmtypes, fmtype, overwrite, xslpath, params);
	} else {
		fprintf(stderr, S_NO_TYPE_ERR);
		exit(EXIT_NO_TYPE);
	}

	for (i = 0; i < nparams; ++i) {
		xmlFree((char *) params[i]);
		xmlFree((char *) params[i + 1]);
	}
	free(params);

	xmlFreeDoc(pm);
	xmlFreeDoc(fmtypes);

	free(pmpath);
	free(fmtype);
	free(xslpath);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
