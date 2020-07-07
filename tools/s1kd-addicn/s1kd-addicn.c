#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <libgen.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/valid.h>

#include "s1kd_tools.h"

#define PROG_NAME "s1kd-addicn"
#define VERSION "1.5.0"

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-o <file>] [-s <src>] [-fh?] <ICN>...");
	puts("");
	puts("Options:");
	puts("  -F, --full-path     Include full ICN file path.");
	puts("  -f, --overwrite     Overwrite source file.");
	puts("  -h, -?, --help      Show help/usage message.");
	puts("  -o, --out <file>    Output filename.");
	puts("  -s, --source <src>  Source filename.");
	puts("  --version           Show version information.");
	puts("  <ICN>...            ICNs to add.");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int main(int argc, char **argv)
{
	int i;
	char *src;
	char *out;
	xmlDocPtr doc;
	bool fullpath = false;
	bool overwrite = false;

	const char *sopts = "s:o:fFh?";
	struct option lopts[] = {
		{"version"  , no_argument      , 0, 0},
		{"help"     , no_argument      , 0, 'h'},
		{"source"   , required_argument, 0, 's'},
		{"out"      , required_argument, 0, 'o'},
		{"overwrite", no_argument      , 0, 'f'},
		{"full-path", no_argument      , 0, 'F'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	src = strdup("-");
	out = strdup("-");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg)
				break;
			case 's':
				free(src);
				src = strdup(optarg);
				break;
			case 'o':
				free(out);
				out = strdup(optarg);
				break;
			case 'f':
				overwrite = true;
				break;
			case 'F':
				fullpath = true;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if ((doc = read_xml_doc(src))) {
		for (i = optind; i < argc; ++i) {
			add_icn(doc, argv[i], fullpath);
		}

		if (overwrite) {
			save_xml_doc(doc, src);
		} else {
			save_xml_doc(doc, out);
		}

		xmlFreeDoc(doc);
	}

	free(src);
	free(out);

	xmlCleanupParser();

	return 0;
}
