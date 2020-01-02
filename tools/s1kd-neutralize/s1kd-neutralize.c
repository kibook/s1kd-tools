#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>

#include <libxml/tree.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include "s1kd_tools.h"
#include "stylesheets.h"

#define PROG_NAME "s1kd-neutralize"
#define VERSION "1.9.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define INF_PREFIX PROG_NAME ": INFO: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

#define I_NEUTRALIZE INF_PREFIX "Adding neutral metadata to %s...\n"

static enum verbosity { QUIET, NORMAL, VERBOSE } verbosity = NORMAL;

static void neutralizeFile(const char *fname, const char *outfile, bool overwrite, bool namesp)
{
	xmlDocPtr doc, res, styledoc, orig;
	xsltStylesheetPtr style;
	xmlNodePtr oldroot;

	if (verbosity >= VERBOSE) {
		fprintf(stderr, I_NEUTRALIZE, fname);
	}

	orig = read_xml_doc(fname);

	doc = xmlCopyDoc(orig, 1);

	styledoc = read_xml_mem((const char *) stylesheets_xlink_xsl, stylesheets_xlink_xsl_len);
	style = xsltParseStylesheetDoc(styledoc);
	res = xsltApplyStylesheet(style, doc, NULL);
	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	doc = res;

	styledoc = read_xml_mem((const char *) stylesheets_rdf_xsl, stylesheets_rdf_xsl_len);
	style = xsltParseStylesheetDoc(styledoc);
	res = xsltApplyStylesheet(style, doc, NULL);
	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	if (namesp) {
		doc = res;
		styledoc = read_xml_mem((const char *)
			stylesheets_namespace_xsl,
			stylesheets_namespace_xsl_len);
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
		save_xml_doc(orig, fname);
	} else {
		save_xml_doc(orig, outfile);
	}

	xmlFreeDoc(res);
	xmlFreeDoc(orig);
}

static void neutralizeList(const char *path, const char *outfile, bool overwrite, bool namesp)
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
		neutralizeFile(line, outfile, overwrite, namesp);
	}

	if (path) {
		fclose(f);
	}
}

static void deneutralizeFile(const char *fname, const char *outfile, bool overwrite)
{
	xmlDocPtr doc, res, styledoc, orig;
	xsltStylesheetPtr style;
	xmlNodePtr oldroot;

	if (verbosity >= VERBOSE) {
		fprintf(stderr, I_NEUTRALIZE, fname);
	}

	orig = read_xml_doc(fname);

	doc = xmlCopyDoc(orig, 1);

	styledoc = read_xml_mem((const char *) stylesheets_delete_xsl, stylesheets_delete_xsl_len);
	style = xsltParseStylesheetDoc(styledoc);
	res = xsltApplyStylesheet(style, doc, NULL);
	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	oldroot = xmlDocGetRootElement(orig);
	xmlUnlinkNode(oldroot);
	xmlFreeNode(oldroot);

	xmlDocSetRootElement(orig, xmlCopyNode(xmlDocGetRootElement(res), 1));

	if (overwrite) {
		save_xml_doc(orig, fname);
	} else {
		save_xml_doc(orig, outfile);
	}

	xmlFreeDoc(res);
	xmlFreeDoc(orig);
}

static void deneutralizeList(const char *path, const char *outfile, bool overwrite)
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
		deneutralizeFile(line, outfile, overwrite);
	}

	if (path) {
		fclose(f);
	}
}

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-o <file>] [-Dflnqvh?] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -D, --delete      Remove neutral metadata.");
	puts("  -f, --overwrite   Overwrite CSDB objects automatically.");
	puts("  -h, -?, --help    Show usage message.");
	puts("  -l, --list        Treat input as list of CSDB objects.");
	puts("  -n, --namespace   Include IETP namespaces on elements.");
	puts("  -o, --out <file>  Output to <file> instead of stdout.");
	puts("  -q, --quiet       Quiet mode.");
	puts("  -v, --verbose     Verbose output.");
	puts("  --version         Show version information.");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
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
	bool delete = false;

	const char *sopts = "Dflno:qvh?";
	struct option lopts[] = {
		{"version"  , no_argument      , 0, 0},
		{"help"     , no_argument      , 0, 'h'},
		{"delete"   , no_argument      , 0, 'D'},
		{"overwrite", no_argument      , 0, 'f'},
		{"list"     , no_argument      , 0, 'l'},
		{"namespace", no_argument      , 0, 'n'},
		{"out"      , required_argument, 0, 'o'},
		{"quiet"    , no_argument      , 0, 'q'},
		{"verbose"  , no_argument      , 0, 'v'},
		LIBXML2_PARSE_LONGOPT_DEFS
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
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'D':
				delete = true;
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
			case 'q':
				--verbosity;
				break;
			case 'v':
				++verbosity;
				break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	if (delete) {
		if (optind < argc) {
			for (i = optind; i < argc; ++i) {
				if (islist) {
					deneutralizeList(argv[i], outfile, overwrite);
				} else {
					deneutralizeFile(argv[i], outfile, overwrite);
				}
			}
		} else if (islist) {
			deneutralizeList(NULL, outfile, overwrite);
		} else {
			deneutralizeFile("-", outfile, false);
		}
	} else {
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
	}

	free(outfile);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
