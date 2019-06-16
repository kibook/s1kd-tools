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
#define VERSION "1.8.1"

static void showHelp(void)
{
	puts("Usage: " PROG_NAME " [options] <DML>...");
	puts("");
	puts("Options:");
	puts("  -$, --issue <iss>      Which issue of the spec to use.");
	puts("  -@, --out <dir>        Output to specified directory.");
	puts("  -%, --templates <dir>  Custom XML template directory.");
	puts("  -D, --dmtypes <path>   Specify .dmtypes file name.");
	puts("  -d, --defaults <path>  Specify .defaults file name.");
	#ifndef _WIN32
	puts("  -F, --fail             Fail on first error from s1kd-new* commands.");
	#endif
	puts("  -f, --overwrite        Overwrite existing CSDB objects.");
	puts("  -h, -?, --help         Show usage message.");
	puts("  -N, --omit-issue       Omit issue/inwork numbers.");
	puts("  -q, --quiet            Don't report errors if objects exist.");
	puts("  -s, --commands         Output s1kd-new* commands only.");
	puts("  -v, --verbose          Print the names of newly created objects.");
	puts("  --version              Show version information.");
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
	char *specIssue = NULL;
	char *templateDir = NULL;
	char *outDir = NULL;
	char *defaultsFname = NULL;
	char *dmtypesFname = NULL;

	const char *sopts = "D:d:sNfFq$:%:@:vh?";
	struct option lopts[] = {
		{"version"   , no_argument      , 0, 0},
		{"help"      , no_argument      , 0, 'h'},
		{"defaults"  , required_argument, 0, 'd'},
		{"dmtypes"   , required_argument, 0, 'D'},
		{"commands"  , no_argument      , 0, 's'},
		{"omit-issue", no_argument      , 0, 'N'},
		{"overwrite" , no_argument      , 0, 'f'},
		{"fail"      , no_argument      , 0, 'F'},
		{"quiet"     , no_argument      , 0, 'q'},
		{"issue"     , required_argument, 0, '$'},
		{"verbose"   , no_argument      , 0, 'v'},
		{"templates" , required_argument, 0, '%'},
		{"out"       , required_argument, 0, '@'},
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
			case 'd':
				defaultsFname = strdup(optarg);
				break;
			case 'D':
				dmtypesFname = strdup(optarg);
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
		const char *params[19];
		char iss[8];
		char *templs = NULL;
		char *outd = NULL;
		char *defname = NULL;
		char *dmtname = NULL;

		if (!(in = read_xml_doc(argv[i]))) {
			continue;
		}

		params[0] = "no-issue";
		params[1] = noIssue ? "true()" : "false()";

		params[2] = "overwrite";
		params[3] = overwrite ? "true()" : "false()";

		params[4] = "no-overwrite-error";
		params[5] = noOverwriteError ? "true()" : "false()";

		params[6] = "spec-issue";
		if (specIssue) {
			snprintf(iss, 8, "\"%s\"", specIssue);
			params[7] = iss;
		} else {
			params[7] = "false()";
		}

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

		params[14] = "defaults";
		if (defaultsFname) {
			defname = malloc(strlen(defaultsFname) + 3);
			sprintf(defname, "\"%s\"", defaultsFname);
			params[15] = defname;
		} else {
			params[15] = "false()";
		}

		params[16] = "dmtypes";
		if (dmtypesFname) {
			dmtname = malloc(strlen(dmtypesFname) + 3);
			sprintf(dmtname, "\"%s\"", dmtypesFname);
			params[17] = dmtname;
		} else {
			params[17] = "false()";
		}

		params[18] = NULL;

		out = xsltApplyStylesheet(dmrlStylesheet, in, params);

		free(templs);
		free(outd);
		free(defname);
		free(dmtname);

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
	free(defaultsFname);
	free(dmtypesFname);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return err;
}
