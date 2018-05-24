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

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

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

		styledoc = xmlReadFile((char *) path, NULL, PARSE_OPTS);

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

	doc = xmlReadFile(path, NULL, PARSE_OPTS);

	doc = transformDoc(doc, stylesheets);

	if (overwrite) {
		xmlSaveFile(path, doc);
	} else {
		xmlSaveFile(out, doc);
	}

	xmlFreeDoc(doc);
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
	puts("Usage: s1kd-transform [-fih?] [-s <stylesheet> [-p <name>=<value> ...] ...] [-o <file>] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -h -?              Show usage message.");
	puts("  -f                 Overwrite input CSDB objects.");
	puts("  -i                 Include identity template in stylesheets.");
	puts("  -o <file>          Output result of transformation to <path>.");
	puts("  -p <name>=<value>  Pass parameters to stylesheets.");
	puts("  -s <stylesheet>    Apply XSLT stylesheet to CSDB objects.");
	puts("  <object>           CSDB objects to apply transformations to.");
}

int main(int argc, char **argv)
{
	int i;

	xmlNodePtr stylesheets, lastStyle = NULL;

	char *out = strdup("-");
	bool overwrite = false;

	exsltRegisterAll();

	stylesheets = xmlNewNode(NULL, BAD_CAST "stylesheets");

	while ((i = getopt(argc, argv, "s:io:p:fh?")) != -1) {
		switch (i) {
			case 's':
				lastStyle = xmlNewChild(stylesheets, NULL, BAD_CAST "stylesheet", NULL);
				xmlSetProp(lastStyle, BAD_CAST "path", BAD_CAST optarg);
				break;
			case 'i':
				includeIdentity = true;
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
