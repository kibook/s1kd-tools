#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/debugXML.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include "dmrl.h"

void showHelp(void)
{
	puts("Usage: s1kd-dmrl [-Nh?] <DML>...");
	puts("");
	puts("Options:");
	puts("  -N       Omit issue/inwork numbers.");
	puts("  -h -?    Show usage message.");
}

int main(int argc, char **argv)
{
	int i;
	xmlDocPtr dmrl;
	xsltStylesheetPtr dmrlStylesheet;
	int err = 0;
	bool execute = true;
	bool noIssue = false;

	dmrl = xmlReadMemory((const char *) dmrl_xsl, dmrl_xsl_len, NULL, NULL, 0);
	dmrlStylesheet = xsltParseStylesheetDoc(dmrl);

	while ((i = getopt(argc, argv, "sNh?")) != -1) {
		switch (i) {
			case 's':
				execute = false;
				break;
			case 'N':
				noIssue = true;
				break;
			case 'h':
			case '?':
				showHelp();
				exit(0);
		}
	}

	for (i = optind; i < argc; ++i) {
		xmlDocPtr in, out;
		xmlChar *content;
		const char *params[3];

		in = xmlReadFile(argv[i], NULL, 0);

		if (noIssue) {
			params[0] = "no-issue";
			params[1] = "true()";
			params[2] = NULL;
		} else {
			params[0] = NULL;
		}

		out = xsltApplyStylesheet(dmrlStylesheet, in, params);

		xmlFreeDoc(in);

		content = xmlNodeGetContent(out->children);

		if (execute) {
			FILE *lines;
			char line[LINE_MAX];

			lines = fmemopen(content, xmlStrlen(content), "r");

			while (fgets(line, LINE_MAX, lines))
				if ((err = WEXITSTATUS(system(line))) != 0) break;

			fclose(lines);
		} else {
			fputs((char *) content, stdout);
		}

		xmlFree(content);

		xmlFreeDoc(out);
	}

	xsltFreeStylesheet(dmrlStylesheet);

	return err;
}
