#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>
#include "s1kd_tools.h"
#include "identity.h"

static bool includeIdentity = false;
static enum verbosity { QUIET, NORMAL, VERBOSE } verbosity = NORMAL;

#define PROG_NAME "s1kd-transform"
#define VERSION "1.6.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define INF_PREFIX PROG_NAME ": INFO: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

#define I_TRANSFORM INF_PREFIX "Transforming %s...\n"

/* Add identity template to stylesheet. */
static void addIdentity(xmlDocPtr style)
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

/* Apply stylesheets to a doc. */
static xmlDocPtr transformDoc(xmlDocPtr doc, xmlNodePtr stylesheets)
{
	xmlDocPtr src;
	xmlNodePtr cur, old;

	src = xmlCopyDoc(doc, 1);

	for (cur = stylesheets->children; cur; cur = cur->next) {
		xmlDocPtr res;
		xsltStylesheetPtr style;
		const char **params;

		/* Select cached stylesheet/params. */
		style = (xsltStylesheetPtr) cur->doc;
		params = (const char **) cur->children;

		res = xsltApplyStylesheet(style, doc, params);

		xmlFreeDoc(doc);

		doc = res;
	}

	old = xmlDocSetRootElement(src, xmlCopyNode(xmlDocGetRootElement(doc), 1));
	xmlFreeNode(old);

	xmlFreeDoc(doc);

	return src;
}

/* Apply stylesheets to a file. */
static void transformFile(const char *path, xmlNodePtr stylesheets, const char *out, bool overwrite)
{
	xmlDocPtr doc;

	if (verbosity >= VERBOSE) {
		fprintf(stderr, I_TRANSFORM, path);
	}

	doc = read_xml_doc(path);

	doc = transformDoc(doc, stylesheets);

	if (overwrite) {
		save_xml_doc(doc, path);
	} else {
		save_xml_doc(doc, out);
	}

	xmlFreeDoc(doc);
}

/* Apply stylesheets to a list of files. */
static void transform_list(const char *path, xmlNodePtr stylesheets, const char *out, bool overwrite)
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
		transformFile(line, stylesheets, out, overwrite);
	}

	if (path) {
		fclose(f);
	}
}

/* Add a parameter to a stylesheet. */
static void addParam(xmlNodePtr stylesheet, char *s)
{
	char *n, *v;
	xmlNodePtr p;

	n = strtok(s, "=");
	v = strtok(NULL, "");

	p = xmlNewChild(stylesheet, NULL, BAD_CAST "param", NULL);
	xmlSetProp(p, BAD_CAST "name", BAD_CAST n);
	xmlSetProp(p, BAD_CAST "value", BAD_CAST v);
}

/* Load stylesheets from disk and cache. */
static void loadStylesheets(xmlNodePtr stylesheets)
{
	xmlNodePtr cur;

	for (cur = stylesheets->children; cur; cur = cur->next) {
		xmlChar *path;
		xmlDocPtr doc;
		xsltStylesheetPtr style;
		unsigned short nparams;
		const char **params = NULL;

		path = xmlGetProp(cur, BAD_CAST "path");
		doc = read_xml_doc((char *) path);
		xmlFree(path);

		if (includeIdentity) {
			addIdentity(doc);
		}

		style = xsltParseStylesheetDoc(doc);

		cur->doc = (xmlDocPtr) style;

		if ((nparams = xmlChildElementCount(cur)) > 0) {
			xmlNodePtr param;
			int n = 0;

			params = malloc((nparams * 2 + 1) * sizeof(char *));

			param = cur->children;
			while (param) {
				xmlNodePtr next;
				char *name, *value;

				next = param->next;

				name  = (char *) xmlGetProp(param, BAD_CAST "name");
				value = (char *) xmlGetProp(param, BAD_CAST "value");

				params[n++] = name;
				params[n++] = value;

				xmlFreeNode(param);
				param = next;
			}

			params[n] = NULL;
		}

		cur->children = (xmlNodePtr) params;
		cur->line = nparams;
	}
}

/* Free cached stylesheets. */
static void freeStylesheets(xmlNodePtr stylesheets)
{
	xmlNodePtr cur;

	for (cur = stylesheets->children; cur; cur = cur->next) {
		const char **params;
		int i;
		unsigned short nparams;

		xsltFreeStylesheet((xsltStylesheetPtr) cur->doc);
		cur->doc = NULL;

		params = (const char **) cur->children;
		nparams = cur->line;
		for (i = 0; i < nparams; ++i) {
			xmlFree((char *) params[i]);
			xmlFree((char *) params[i + 1]);
		}
		free(params);
		cur->children = NULL;
	}
}

/* Show help/usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-filqvh?] [-s <stylesheet> [-p <name>=<value> ...] ...] [-o <file>] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -f, --overwrite                Overwrite input CSDB objects.");
	puts("  -h, -?, --help                 Show usage message.");
	puts("  -i, --identity                 Include identity template in stylesheets.");
	puts("  -l, --list                     Treat input as list of objects.");
	puts("  -o, --out <file>               Output result of transformation to <path>.");
	puts("  -p, --param <name>=<value>     Pass parameters to stylesheets.");
	puts("  -q, --quiet                    Quiet mode.");
	puts("  -s, --stylesheet <stylesheet>  Apply XSLT stylesheet to CSDB objects.");
	puts("  -v, --verbose                  Verbose output.");
	puts("  --version                      Show version information.");
	puts("  <object>                       CSDB objects to apply transformations to.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Show version information. */
static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s, libxslt %s and libexslt %s\n",
		xmlParserVersion, xsltEngineVersion, exsltLibraryVersion);
}

int main(int argc, char **argv)
{
	int i;

	xmlNodePtr stylesheets, lastStyle = NULL;

	char *out = strdup("-");
	bool overwrite = false;
	bool islist = false;

	const char *sopts = "s:ilo:p:qfvh?";
	struct option lopts[] = {
		{"version"   , no_argument      , 0, 0},
		{"help"      , no_argument      , 0, 'h'},
		{"identity"  , no_argument      , 0, 'i'},
		{"list"      , no_argument      , 0, 'l'},
		{"out"       , required_argument, 0, 'o'},
		{"param"     , required_argument, 0, 'p'},
		{"quiet"     , no_argument      , 0, 'q'},
		{"stylesheet", required_argument, 0, 's'},
		{"verbose"   , no_argument      , 0, 'v'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	exsltRegisterAll();

	stylesheets = xmlNewNode(NULL, BAD_CAST "stylesheets");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 's':
				lastStyle = xmlNewChild(stylesheets, NULL, BAD_CAST "stylesheet", NULL);
				xmlSetProp(lastStyle, BAD_CAST "path", BAD_CAST optarg);
				break;
			case 'i':
				includeIdentity = true;
				break;
			case 'l':
				islist = true;
				break;
			case 'o':
				free(out);
				out = strdup(optarg);
				break;
			case 'p':
				addParam(lastStyle, optarg);
				break;
			case 'q':
				--verbosity;
				break;
			case 'f':
				overwrite = true;
				break;
			case 'v':
				++verbosity;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	loadStylesheets(stylesheets);

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				transform_list(argv[i], stylesheets, out, overwrite);
			} else {
				transformFile(argv[i], stylesheets, out, overwrite);
			}
		}
	} else if (islist) {
		transform_list(NULL, stylesheets, out, overwrite);
	} else {
		transformFile("-", stylesheets, out, false);
	}

	if (out) {
		free(out);
	}

	freeStylesheets(stylesheets);
	xmlFreeNode(stylesheets);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
