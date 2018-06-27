#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <libgen.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/valid.h>

#define PROG_NAME "s1kd-addicn"
#define VERSION "1.0.1"

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
	puts("Usage: " PROG_NAME " [-s <src>] [-o <out>] [-fh?] <ICN>...");
	puts("");
	puts("Options:");
	puts("  -s <src>   Source filename.");
	puts("  -o <out>   Output filename.");
	puts("  -f         Overwrite source file.");
	puts("  -F         Include full ICN file path.");
	puts("  -h -?      Show help/usage message.");
	puts("  <ICN>...   ICNs to add.");
	puts("  --version  Show version information.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
}

void addNotation(xmlDocPtr doc, const char *name, const char *sysId)
{
	xmlValidCtxtPtr valid;

	if (!xmlHashLookup(doc->intSubset->notations, BAD_CAST name)) {
		valid = xmlNewValidCtxt();
		xmlAddNotationDecl(valid, doc->intSubset, BAD_CAST name, NULL, BAD_CAST sysId);
		xmlFreeValidCtxt(valid);
	}
}

void addIcn(xmlDocPtr doc, const char *path, bool fullpath)
{
	char *full, *base, *name;
	char *infoEntityIdent;
	char *notation;

	full = strdup(path);
	base = basename(full);
	name = strdup(base);

	infoEntityIdent = strtok(name, ".");
	notation = strtok(NULL, "");

	xmlAddDocEntity(doc, BAD_CAST infoEntityIdent,
		XML_EXTERNAL_GENERAL_UNPARSED_ENTITY, NULL,
		BAD_CAST (fullpath ? path : base), BAD_CAST notation);
	addNotation(doc, notation, notation);

	free(name);
	free(full);
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
		{"version", no_argument, 0, 0},
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
				showHelp();
				return 0;
		}
	}

	doc = xmlReadFile(src, NULL, PARSE_OPTS);

	for (i = optind; i < argc; ++i) {
		addIcn(doc, argv[i], fullpath);
	}

	if (overwrite) {
		xmlSaveFile(src, doc);
	} else {
		xmlSaveFile(out, doc);
	}

	xmlFreeDoc(doc);
	free(src);
	free(out);

	xmlCleanupParser();

	return 0;
}
