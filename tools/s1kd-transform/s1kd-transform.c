#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>
#include "s1kd_tools.h"
#include "identity.h"

bool includeIdentity = false;
bool verbose = false;

#define PROG_NAME "s1kd-transform"
#define VERSION "1.3.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define INF_PREFIX PROG_NAME ": INFO: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

#define I_TRANSFORM INF_PREFIX "Transforming %s...\n"

void addIdentity(xmlDocPtr style)
{
	xmlDocPtr identity;
	xmlNodePtr stylesheet, first, template;

	identity = read_xml_mem((const char *) ___common_identity_xsl, ___common_identity_xsl_len);
	template = xmlFirstElementChild(xmlDocGetRootElement(identity));

	stylesheet = xmlDocGetRootElement(style);

	first = xmlFirstElementChild(stylesheet);

	if (first) {
		xmlAddPrevSibling(first, xmlCopyNode(template, 1));
	} else {
		xmlAddChild(stylesheet, xmlCopyNode(template, 1));
	}

	xmlFreeDoc(identity);
}

xmlDocPtr transformDoc(xmlDocPtr doc, xmlNodePtr stylesheets)
{
	xmlDocPtr src;
	xmlNodePtr cur, old;

	src = xmlCopyDoc(doc, 1);

	for (cur = stylesheets->children; cur; cur = cur->next) {
		xmlDocPtr res, styledoc;
		xmlChar *path;
		xsltStylesheetPtr style;
		const char **params = NULL;
		unsigned long nparams;
		int i;

		path = xmlGetProp(cur, BAD_CAST "path");

		if ((nparams = xmlChildElementCount(cur)) > 0) {
			xmlNodePtr param;
			int n = 0;

			params = malloc((nparams * 2 + 1) * sizeof(char *));

			for (param = cur->children; param; param = param->next) {
				char *name, *value;

				name  = (char *) xmlGetProp(param, BAD_CAST "name");
				value = (char *) xmlGetProp(param, BAD_CAST "value");

				params[n++] = name;
				params[n++] = value;
			}

			params[n] = NULL;
		}

		styledoc = read_xml_doc((char *) path);

		if (includeIdentity) {
			addIdentity(styledoc);
		}

		style = xsltParseStylesheetDoc(styledoc);

		xmlFree(path);

		res = xsltApplyStylesheet(style, doc, params);

		for (i = 0; i < nparams; ++i) {
			xmlFree((char *) params[i]);
			xmlFree((char *) params[i + 1]);
		}
		free(params);

		xsltFreeStylesheet(style);
		xmlFreeDoc(doc);

		doc = res;
	}

	old = xmlDocSetRootElement(src, xmlCopyNode(xmlDocGetRootElement(doc), 1));
	xmlFreeNode(old);

	xmlFreeDoc(doc);

	return src;
}
	
void transformFile(const char *path, xmlNodePtr stylesheets, const char *out, bool overwrite)
{
	xmlDocPtr doc;

	if (verbose) {
		fprintf(stderr, I_TRANSFORM, path);
	}

	doc = read_xml_doc(path);

	doc = transformDoc(doc, stylesheets);

	if (overwrite) {
		save_xml_doc(doc, path);
	} else {
		save_xml_doc(doc, out);
	}

	xmlFreeDoc(doc);
}

void transform_list(const char *path, xmlNodePtr stylesheets, const char *out, bool overwrite)
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
		transformFile(line, stylesheets, out, overwrite);
	}

	if (path) {
		fclose(f);
	}
}

void addParam(xmlNodePtr stylesheet, char *s)
{
	char *n, *v;
	xmlNodePtr p;

	n = strtok(s, "=");
	v = strtok(NULL, "");

	p = xmlNewChild(stylesheet, NULL, BAD_CAST "param", NULL);
	xmlSetProp(p, BAD_CAST "name", BAD_CAST n);
	xmlSetProp(p, BAD_CAST "value", BAD_CAST v);
}

void showHelp(void)
{
	puts("Usage: " PROG_NAME " [-filvh?] [-s <stylesheet> [-p <name>=<value> ...] ...] [-o <file>] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -h -?              Show usage message.");
	puts("  -f                 Overwrite input CSDB objects.");
	puts("  -i                 Include identity template in stylesheets.");
	puts("  -l                 Treat input as list of objects.");
	puts("  -o <file>          Output result of transformation to <path>.");
	puts("  -p <name>=<value>  Pass parameters to stylesheets.");
	puts("  -s <stylesheet>    Apply XSLT stylesheet to CSDB objects.");
	puts("  -v                 Verbose output.");
	puts("  --version          Show version information.");
	puts("  <object>           CSDB objects to apply transformations to.");
	LIBXML2_PARSE_LONGOPT_HELP
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s, libxslt %s and libexslt %s\n",
		xmlParserVersion, xsltEngineVersion, exsltLibraryVersion);
}

int main(int argc, char **argv)
{
	int i;

	xmlNodePtr stylesheets, lastStyle = NULL;

	char *out = strdup("-");
	bool overwrite = false;
	bool islist = false;

	const char *sopts = "s:ilo:p:fvh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	exsltRegisterAll();

	stylesheets = xmlNewNode(NULL, BAD_CAST "stylesheets");

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
				lastStyle = xmlNewChild(stylesheets, NULL, BAD_CAST "stylesheet", NULL);
				xmlSetProp(lastStyle, BAD_CAST "path", BAD_CAST optarg);
				break;
			case 'i':
				includeIdentity = true;
				break;
			case 'l':
				islist = true;
				break;
			case 'o':
				free(out);
				out = strdup(optarg);
				break;
			case 'p':
				addParam(lastStyle, optarg);
				break;
			case 'f':
				overwrite = true;
				break;
			case 'v':
				verbose = true;
				break;
			case 'h':
			case '?':
				showHelp();
				return 0;
		}
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				transform_list(argv[i], stylesheets, out, overwrite);
			} else {
				transformFile(argv[i], stylesheets, out, overwrite);
			}
		}
	} else if (islist) {
		transform_list(NULL, stylesheets, out, overwrite);
	} else {
		transformFile("-", stylesheets, out, false);
	}

	if (out) {
		free(out);
	}

	xmlFreeNode(stylesheets);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
