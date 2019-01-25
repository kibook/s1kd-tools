#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>

#include <libxml/tree.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include "stylesheets.h"

#define PROG_NAME "s1kd-neutralize"
#define VERSION "1.2.5"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

void neutralizeFile(const char *fname, const char *outfile, bool overwrite, bool namesp)
{
	xmlDocPtr doc, res, styledoc, orig;
	xsltStylesheetPtr style;
	xmlNodePtr oldroot;

	orig = xmlReadFile(fname, NULL, PARSE_OPTS);

	doc = xmlCopyDoc(orig, 1);

	styledoc = xmlReadMemory((const char *) stylesheets_xlink_xsl, stylesheets_xlink_xsl_len, NULL, NULL, 0);
	style = xsltParseStylesheetDoc(styledoc);
	res = xsltApplyStylesheet(style, doc, NULL);
	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	doc = res;

	styledoc = xmlReadMemory((const char *) stylesheets_rdf_xsl, stylesheets_rdf_xsl_len, NULL, NULL, 0);
	style = xsltParseStylesheetDoc(styledoc);
	res = xsltApplyStylesheet(style, doc, NULL);
	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	if (namesp) {
		doc = res;
		styledoc = xmlReadMemory((const char *)
			stylesheets_namespace_xsl,
			stylesheets_namespace_xsl_len, NULL, NULL, 0);
		style = xsltParseStylesheetDoc(styledoc);
		res = xsltApplyStylesheet(style, doc, NULL);
		xmlFreeDoc(doc);
		xsltFreeStylesheet(style);
	}

	oldroot = xmlDocGetRootElement(orig);
	xmlUnlinkNode(oldroot);
	xmlFreeNode(oldroot);

	xmlDocSetRootElement(orig, xmlCopyNode(xmlDocGetRootElement(res), 1));

	if (overwrite) {
		xmlSaveFile(fname, orig);
	} else {
		xmlSaveFile(outfile, orig);
	}

	xmlFreeDoc(res);
	xmlFreeDoc(orig);
}

void neutralizeList(const char *path, const char *outfile, bool overwrite, bool namesp)
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
		neutralizeFile(line, outfile, overwrite, namesp);
	}

	if (path) {
		fclose(f);
	}
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-o <file>] [-flh?] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -f         Overwrite CSDB objects automatically.");
	puts("  -h -?      Show usage message.");
	puts("  -l         Treat input as list of CSDB objects.");
	puts("  -n         Include IETP namespaces on elements.");
	puts("  -o <file>  Output to <file> instead of stdout.");
	puts("  --version  Show version information.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

int main(int argc, char **argv)
{
	int i;
	char *outfile = strdup("-");
	bool overwrite = false;
	bool islist = false;
	bool namesp = false;

	const char *sopts = "flno:h?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 'f':
				overwrite = true;
				break;
			case 'l':
				islist = true;
				break;
			case 'n':
				namesp = true;
				break;
			case 'o':
				free(outfile);
				outfile = strdup(optarg);
				break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				neutralizeList(argv[i], outfile, overwrite, namesp);
			} else {
				neutralizeFile(argv[i], outfile, overwrite, namesp);
			}
		}
	} else if (islist) {
		neutralizeList(NULL, outfile, overwrite, namesp);
	} else {
		neutralizeFile("-", outfile, false, namesp);
	}

	free(outfile);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
