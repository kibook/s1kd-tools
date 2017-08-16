#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include <libxml/tree.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include "stylesheets.h"

#define PROG_NAME "s1kd-neutralize"

void neutralizeFile(const char *fname, const char *outfile)
{
	xmlDocPtr doc, res, styledoc, orig;
	xsltStylesheetPtr style;

	orig = xmlReadFile(fname, NULL, 0);

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

	xmlDocSetRootElement(orig, xmlCopyNode(xmlDocGetRootElement(res), 1));

	if (outfile)
		xmlSaveFile(outfile, orig);
	else
		xmlSaveFile(fname, orig);

	xmlFreeDoc(res);
	xmlFreeDoc(orig);
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-o <file>] [-rh?] <datamodules>");
	puts("");
	puts("Options:");
	puts("  -o <file>  Output to <file> instead of overwriting.");
	puts("  -h -?      Show usage message.");
}

int main(int argc, char **argv)
{
	int i;
	char *outfile = NULL;

	while ((i = getopt(argc, argv, "ro:h?")) != -1) {
		switch (i) {
			case 'o':
				outfile = strdup(optarg);
				break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	for (i = optind; i < argc; ++i) {
		neutralizeFile(argv[i], outfile);
	}

	free(outfile);

	return 0;
}
