#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#ifdef _WIN32
#include <string.h>
#endif
#include <libxml/tree.h>
#include <libxml/debugXML.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include "dmrl.h"

#define PROG_NAME "s1kd-dmrl"
#define VERSION "1.3.0"

#define DEFAULT_S1000D_ISSUE "4.2"

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

void showHelp(void)
{
	puts("Usage: " PROG_NAME " [-$ <iss>] [-% <dir>] [-FfNsh?] <DML>...");
	puts("");
	puts("Options:");
	puts("  -h -?      Show usage message.");
	puts("  -$ <iss>   Which issue of the spec to use.");
	puts("  -% <dir>   Custom XML template directory.");
	#ifndef _WIN32
	puts("  -F         Fail on first error from s1kd-new* commands.");
	#endif
	puts("  -f         Overwrite existing CSDB objects.");
	puts("  -N         Omit issue/inwork numbers.");
	puts("  -q         Don't report errors if objects exist.");
	puts("  -s         Output s1kd-new* commands only.");
	puts("  -v         Print the names of newly created objects.");
	puts("  --version  Show version information.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
}

int main(int argc, char **argv)
{
	int i;
	xmlDocPtr dmrl;
	xsltStylesheetPtr dmrlStylesheet;
	int err = 0;
	bool execute = true;
	bool noIssue = false;
	#ifndef _WIN32
	bool failOnFirstErr = false;
	#endif
	bool overwrite = false;
	bool noOverwriteError = false;
	bool verbose = false;
	char *specIssue = strdup(DEFAULT_S1000D_ISSUE);
	char *templateDir = NULL;

	const char *sopts = "sNfFq$:%:vh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	dmrl = xmlReadMemory((const char *) dmrl_xsl, dmrl_xsl_len, NULL, NULL, 0);
	dmrlStylesheet = xsltParseStylesheetDoc(dmrl);

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 's':
				execute = false;
				break;
			case 'N':
				noIssue = true;
				break;
			case 'f':
				overwrite = true;
				break;
			#ifndef _WIN32
			case 'F':
				failOnFirstErr = true;
				break;
			#endif
			case 'q':
				noOverwriteError = true;
				break;
			case '$':
				free(specIssue);
				specIssue = strdup(optarg);
				break;
			case 'v':
				verbose = true;
				break;
			case '%':
				templateDir = strdup(optarg);
				break;
			case 'h':
			case '?':
				showHelp();
				return 0;
		}
	}

	for (i = optind; i < argc; ++i) {
		xmlDocPtr in, out;
		xmlChar *content;
		const char *params[13];
		char iss[8];
		char *templs = NULL;

		in = xmlReadFile(argv[i], NULL, PARSE_OPTS);

		params[0] = "no-issue";
		params[1] = noIssue ? "true()" : "false()";

		params[2] = "overwrite";
		params[3] = overwrite ? "true()" : "false()";

		params[4] = "no-overwrite-error";
		params[5] = noOverwriteError ? "true()" : "false()";

		snprintf(iss, 8, "\"%s\"", specIssue);
		params[6] = "spec-issue";
		params[7] = iss;

		params[8] = "verbose";
		params[9] = verbose ? "true()" : "false()";

		params[10] = "templates";
		if (templateDir) {
			templs = malloc(strlen(templateDir) + 3);
			sprintf(templs, "\"%s\"", templateDir);
			params[11] = templs;
		} else {
			params[11] = "false()";
		}

		params[12] = NULL;

		out = xsltApplyStylesheet(dmrlStylesheet, in, params);

		free(templs);

		xmlFreeDoc(in);

		content = xmlNodeGetContent(out->children);

		if (execute) {
			#ifdef _WIN32
			/* FIXME: Implement alternative to fmemopen and
			 * WEXITSTATUS in order to use the -F ("fail on first
			 * error") option on a Windows system.
			 */
			char *line = NULL;
			while ((line = strtok(line ? NULL : (char *) content, "\n"))) {
				system(line);
			}
			#else
			FILE *lines;
			char line[LINE_MAX];

			lines = fmemopen(content, xmlStrlen(content), "r");

			while (fgets(line, LINE_MAX, lines))
				if ((err += WEXITSTATUS(system(line))) != 0 && failOnFirstErr) break;

			fclose(lines);
			#endif
		} else {
			fputs((char *) content, stdout);
		}

		xmlFree(content);

		xmlFreeDoc(out);
	}

	xsltFreeStylesheet(dmrlStylesheet);

	free(specIssue);
	free(templateDir);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return err;
}
