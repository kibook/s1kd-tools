#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/valid.h>

void showHelp(void)
{
	puts("Usage: s1kd-addicn [-s <src>] [-o <out>] [-fh?] <ICN>...");
	puts("");
	puts("Options:");
	puts("  -s <src>  Source filename.");
	puts("  -o <out>  Output filename.");
	puts("  -f        Overwrite source file.");
	puts("  -h -?     Show help/usage message.");
	puts("  <ICN>...  ICNs to add.");
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
	char *full, *base;
	char *infoEntityIdent;
	char *notation;

	full = strdup(path);
	base = basename(full);

	infoEntityIdent = strtok(base, ".");
	notation = strtok(NULL, "");

	xmlAddDocEntity(doc, BAD_CAST infoEntityIdent, XML_EXTERNAL_GENERAL_UNPARSED_ENTITY, NULL, BAD_CAST path, BAD_CAST notation);
	addNotation(doc, notation, notation);

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

	src = strdup("-");
	out = strdup("-");

	while ((i = getopt(argc, argv, "s:o:fh?")) != -1) {
		switch (i) {
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
			case 'h':
			case '?':
				showHelp();
				exit(0);
		}
	}

	doc = xmlReadFile(src, NULL, 0);

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

	return 0;
}
