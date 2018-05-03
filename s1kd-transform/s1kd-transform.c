#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>
#include "identity.h"

bool includeIdentity = false;

void addIdentity(xmlDocPtr style)
{
	xmlDocPtr identity;
	xmlNodePtr stylesheet, first, template;

	identity = xmlReadMemory((const char *) ___common_identity_xsl, ___common_identity_xsl_len, NULL, NULL, 0);
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

		path = xmlNodeGetContent(cur);

		styledoc = xmlReadFile((char *) path, NULL, 0);

		if (includeIdentity) {
			addIdentity(styledoc);
		}

		style = xsltParseStylesheetDoc(styledoc);

		xmlFree(path);

		res = xsltApplyStylesheet(style, doc, NULL);

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

	/* Bug in libxml < 20902 where entities in DTD are substituted even when
	 * XML_PARSE_NOENT is not specified (default).
	 */
	if (LIBXML_VERSION < 20902) {
		doc = xmlReadFile(path, NULL, XML_PARSE_NONET);
	} else {
		doc = xmlReadFile(path, NULL, 0);
	}

	doc = transformDoc(doc, stylesheets);

	if (overwrite) {
		xmlSaveFile(path, doc);
	} else {
		xmlSaveFile(out, doc);
	}

	xmlFreeDoc(doc);
}

void showHelp(void)
{
	puts("Usage: s1kd-transform [-h?] [-s <stylesheet> ...] [-i] [-o <file>] <datamodules>");
	puts("");
	puts("Options:");
	puts("  -h -?    Show usage message.");
	puts("  -s <stylesheet>  Apply XSLT stylesheet to data modules.");
	puts("  -i               Include identity template.");
	puts("  -o <file>        Output to <path> instead of overwriting (- for stdout).");
	puts("  <datamodules>    Data modules to apply transformations to.");
}

int main(int argc, char **argv)
{
	int i;

	xmlNodePtr stylesheets;

	char *out = strdup("-");
	bool overwrite = false;

	exsltRegisterAll();

	stylesheets = xmlNewNode(NULL, BAD_CAST "stylesheets");

	while ((i = getopt(argc, argv, "s:io:fh?")) != -1) {
		switch (i) {
			case 's':
				xmlNewChild(stylesheets, NULL, BAD_CAST "stylesheet", BAD_CAST optarg);
				break;
			case 'i':
				includeIdentity = true;
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

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			transformFile(argv[i], stylesheets, out, overwrite);
		}
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
