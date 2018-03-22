#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include "xsl.h"
#include "defaults.h"

#define PROG_NAME "s1kd-defaults"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define EXIT_NO_OVERWRITE 1

#define DEFAULT_DEFAULTS_FNAME "defaults"
#define DEFAULT_DMTYPES_FNAME "dmtypes"

enum format {TEXT, XML};
enum file {NONE, DEFAULTS, DMTYPES};

/* Show the help/usage message. */
void show_help(void)
{
	puts("Usage: " PROG_NAME " [-Ddfisth?] [<file>...]");
	puts("");
	puts("Options:");
	puts("  -h -?      Show usage message.");
	puts("  -D         Convert a dmtypes file.");
	puts("  -d         Convert a defaults file.");
	puts("  -f         Overwrite an existing file.");
	puts("  -i         Initialize a new CSDB.");
	puts("  -s         Sort entries.");
	puts("  -t         Output in the simple text format.");
}

/* Apply a built-in XSLT stylesheet to an XML document. */
xmlDocPtr transform_doc(xmlDocPtr doc, unsigned char *xml, unsigned int len)
{
	xmlDocPtr styledoc, res;
	xsltStylesheetPtr style;

	styledoc = xmlReadMemory((const char *) xml, len, NULL, NULL, 0);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);
	xsltFreeStylesheet(style);

	return res;
}

/* Sort entries in defaults/dmtypes files. */
xmlDocPtr sort_entries(xmlDocPtr doc)
{
	return transform_doc(doc, xsl_sort_xsl, xsl_sort_xsl_len);
}

/* Convert XML defaults to the simple text version. */
xmlDocPtr xml_defaults_to_text(xmlDocPtr doc)
{
	return transform_doc(doc, xsl_xml_defaults_to_text_xsl, xsl_xml_defaults_to_text_xsl_len);
}

/* Convert XML dmtypes to the simple text version. */
xmlDocPtr xml_dmtypes_to_text(xmlDocPtr doc)
{
	return transform_doc(doc, xsl_xml_dmtypes_to_text_xsl, xsl_xml_dmtypes_to_text_xsl_len);
}

/* Convert simple text defaults to the XML version. */
xmlDocPtr text_defaults_to_xml(const char *path)
{
	FILE *f;
	char line[1024];
	xmlDocPtr doc;
	xmlNodePtr defaults;

	if (!path) {
		return NULL;
	}

	if ((doc = xmlReadFile(path, NULL, XML_PARSE_NOERROR|XML_PARSE_NOWARNING))) {
		return doc;
	}

	if (strcmp(path, "-") == 0) {
		f = stdin;
	} else if (!(f = fopen(path, "r"))) {
		return NULL;
	}

	doc = xmlNewDoc(BAD_CAST "1.0");
	defaults = xmlNewNode(NULL, BAD_CAST "defaults");
	xmlDocSetRootElement(doc, defaults);

	while (fgets(line, 1024, f)) {
		char key[32], val[256];
		xmlNodePtr def;

		if (sscanf(line, "%31s %255[^\n]", key, val) != 2)
			continue;

		def = xmlNewChild(defaults, NULL, BAD_CAST "default", NULL);
		xmlSetProp(def, BAD_CAST "ident", BAD_CAST key);
		xmlSetProp(def, BAD_CAST "value", BAD_CAST val);
	}

	fclose(f);

	return doc;
}

/* Convert simple text dmtypes to the XML version. */
xmlDocPtr text_dmtypes_to_xml(const char *path)
{
	FILE *f;
	char line[1024];
	xmlDocPtr doc;
	xmlNodePtr dmtypes;

	if ((doc = xmlReadFile(path, NULL, XML_PARSE_NOERROR|XML_PARSE_NOWARNING))) {
		return doc;
	}

	if (strcmp(path, "-") == 0) {
		f = stdin;
	} else if (!(f = fopen(path, "r"))) {
		return NULL;
	}

	doc = xmlNewDoc(BAD_CAST "1.0");
	dmtypes = xmlNewNode(NULL, BAD_CAST "dmtypes");
	xmlDocSetRootElement(doc, dmtypes);

	while (fgets(line, 1024, f)) {
		char code[5], schema[64], infname[256];
		int n;
		xmlNodePtr type;

		n = sscanf(line, "%5s %64s %255[^\n]", code, schema, infname);

		if (n < 2) {
			continue;
		}

		type = xmlNewChild(dmtypes, NULL, BAD_CAST "type", NULL);
		xmlSetProp(type, BAD_CAST "infoCode", BAD_CAST code);
		xmlSetProp(type, BAD_CAST "schema", BAD_CAST schema);
		if (n > 2) {
			xmlSetProp(type, BAD_CAST "infoName", BAD_CAST infname);
		}
	}

	fclose(f);

	return doc;
}

/* Return the first node matching an XPath expression. */
xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(doc ? doc : node->doc);
	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);
	first = xmlXPathNodeSetIsEmpty(obj->nodesetval) ? NULL : obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

/* Obtain some defaults values from the environment. */
void set_defaults(xmlDocPtr doc)
{
	char *env;

	if ((env = getenv("LANG"))) {
		char *lang, *lang_l, *lang_c;
		xmlNodePtr liso, ciso;

		lang = strdup(env);
		lang_l = strtok(lang, "_");
		lang_c = strtok(NULL, ".");

		liso = first_xpath_node(doc, NULL, "//default[@ident = 'languageIsoCode']");
		ciso = first_xpath_node(doc, NULL, "//default[@ident = 'countryIsoCode']");

		if (lang_l) {
			xmlSetProp(liso, BAD_CAST "value", BAD_CAST lang_l);
		}
		if (lang_c) {
			xmlSetProp(ciso, BAD_CAST "value", BAD_CAST lang_c);
		}
	}
}

/* Dump the built-in defaults in the XML format. */
void dump_defaults_xml(const char *fname, bool overwrite)
{
	xmlDocPtr doc;
	doc = xmlReadMemory((const char *) defaults_xml, defaults_xml_len, NULL, NULL, 0);
	set_defaults(doc);
	if (overwrite) {
		if (!fname) fname = DEFAULT_DEFAULTS_FNAME;
		xmlSaveFile(fname, doc);
	} else {
		xmlSaveFile("-", doc);
	}
	xmlFreeDoc(doc);
}

/* Dump the built-in defaults in the simple text format. */
void dump_defaults_text(const char *fname, bool overwrite)
{
	xmlDocPtr doc, res;
	FILE *f;
	doc = xmlReadMemory((const char *) defaults_xml, defaults_xml_len, NULL, NULL, 0);
	set_defaults(doc);
	res = transform_doc(doc, xsl_xml_defaults_to_text_xsl, xsl_xml_defaults_to_text_xsl_len);

	if (overwrite) {
		if (!fname) fname = DEFAULT_DEFAULTS_FNAME;
		f = fopen(fname, "w");
	} else {
		f = stdout;
	}

	fprintf(f, "%s", (char *) res->children->content);

	if (overwrite) {
		fclose(f);
	}

	xmlFreeDoc(res);
	xmlFreeDoc(doc);
}

xmlDocPtr simple_text_to_xml(const char *path, enum file f, bool sort)
{
	xmlDocPtr doc;

	switch (f) {
		case NONE:
		case DEFAULTS:
			doc = text_defaults_to_xml(path);
			break;
		case DMTYPES:
			doc = text_dmtypes_to_xml(path);
			break;
	}

	if (sort) {
		xmlDocPtr res;
		res = sort_entries(doc);
		xmlFreeDoc(doc);
		doc = res;
	}

	return doc;
}

/* Convert an XML defaults/dmtypes file to the simple text version. */
void xml_to_text(const char *path, enum file f, bool overwrite, bool sort)
{
	xmlDocPtr doc, res;

	if (!(doc = xmlReadFile(path, NULL, XML_PARSE_NOERROR|XML_PARSE_NOWARNING))) {
		doc = simple_text_to_xml(path, f, sort);
	}

	switch (f) {
		case NONE:
		case DEFAULTS:
			res = xml_defaults_to_text(doc);
			break;
		case DMTYPES:
			res = xml_dmtypes_to_text(doc);
			break;
	}

	if (res->children) {
		FILE *f;

		if (overwrite) {
			f = fopen(path, "w");
		} else {
			f = stdout;
		}
		fprintf(f, "%s", (char *) res->children->content);
		if (overwrite) {
			fclose(f);
		}
	}

	xmlFreeDoc(res);
	xmlFreeDoc(doc);
}

/* Convert a simple text defaults/dmtypes file to the XML version. */
void text_to_xml(const char *path, enum file f, bool overwrite, bool sort)
{
	xmlDocPtr doc;

	doc = simple_text_to_xml(path, f, sort);

	if (sort) {
		xmlDocPtr res;
		res = sort_entries(doc);
		xmlFreeDoc(doc);
		doc = res;
	}

	if (overwrite) {
		xmlSaveFile(path, doc);
	} else {
		xmlSaveFile("-", doc);
	}

	xmlFreeDoc(doc);
}

void convert_or_dump(enum format fmt, enum file f, const char *fname, bool overwrite, bool sort)
{
	if (fmt == TEXT) {
		if (f == NONE) {
			dump_defaults_text(fname, overwrite);
		} else {
			xml_to_text(fname, f, overwrite, sort);
		}
	} else if (fmt == XML) {
		if (f == NONE) {
			dump_defaults_xml(fname, overwrite);
		} else {
			text_to_xml(fname, f, overwrite, sort);
		}
	}
}

int main(int argc, char **argv)
{
	int i;
	enum format fmt = XML;
	enum file f = NONE;
	char *fname = NULL;
	bool overwrite = false;
	bool initialize = false;
	bool sort = false;

	while ((i = getopt(argc, argv, "Ddfisth?")) != -1) {
		switch (i) {
			case 'D':
				f = DMTYPES;
				if (!fname) fname = strdup(DEFAULT_DMTYPES_FNAME);
				break;
			case 'd':
				f = DEFAULTS;
				if (!fname) fname = strdup(DEFAULT_DEFAULTS_FNAME);
				break;
			case 'f':
				overwrite = true;
				break;
			case 'i':
				initialize = true;
				break;
			case 's':
				sort = true;
				break;
			case 't':
				fmt = TEXT;
				break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	if (initialize) {
		if (fmt == TEXT) {
			dump_defaults_text(NULL, true);
			system("s1kd-newdm -. > dmtypes");
		} else {
			dump_defaults_xml(NULL, true);
			system("s1kd-newdm -, > dmtypes");
		}
	} else if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			convert_or_dump(fmt, f, argv[i], overwrite, sort);
		}
	} else {
		convert_or_dump(fmt, f, fname, overwrite, sort);
	}

	free(fname);

	return 0;
}
