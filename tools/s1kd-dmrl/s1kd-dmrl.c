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
#include "s1kd_tools.h"
#include "dmrl.h"

#define PROG_NAME "s1kd-dmrl"
#define VERSION "1.5.0"

#define DEFAULT_S1000D_ISSUE "4.2"

void showHelp(void)
{
	puts("Usage: " PROG_NAME " [-$ <iss>] [-@ <dir>] [-% <dir>] [-FfNsh?] <DML>...");
	puts("");
	puts("Options:");
	puts("  -h -?      Show usage message.");
	puts("  -$ <iss>   Which issue of the spec to use.");
	puts("  -@ <dir>   Output to specified directory.");
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
	LIBXML2_PARSE_LONGOPT_HELP
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
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
	char *outDir = NULL;

	const char *sopts = "sNfFq$:%:@:vh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	dmrl = read_xml_mem((const char *) dmrl_xsl, dmrl_xsl_len);
	dmrlStylesheet = xsltParseStylesheetDoc(dmrl);

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
			case '@':
				outDir = strdup(optarg);
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
		const char *params[15];
		char iss[8];
		char *templs = NULL;
		char *outd = NULL;

		if (!(in = read_xml_doc(argv[i]))) {
			continue;
		}

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

		params[12] = "outdir";
		if (outDir) {
			outd = malloc(strlen(outDir) + 3);
			sprintf(outd, "\"%s\"", outDir);
			params[13] = outd;
		} else {
			params[13] = "false()";
		}

		params[14] = NULL;

		out = xsltApplyStylesheet(dmrlStylesheet, in, params);

		free(templs);
		free(outd);

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
	free(outDir);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return err;
}
