#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include "s1kd_tools.h"
#include "xsl.h"

#define PROG_NAME "s1kd-fmgen"
#define VERSION "1.0.1"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_NO_PM 1
#define EXIT_NO_TYPE 2
#define EXIT_BAD_TYPE 3

#define S_NO_PM_ERR ERR_PREFIX "No publication module.\n"
#define S_NO_TYPE_ERR ERR_PREFIX "No FM type specified.\n"
#define S_BAD_TYPE_ERR ERR_PREFIX "Unknown front matter type: %s\n"

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const char *expr)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(doc ? doc : node->doc);
	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST expr, ctx);

	first = xmlXPathNodeSetIsEmpty(obj->nodesetval) ? NULL : obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

char *first_xpath_string(xmlDocPtr doc, xmlNodePtr node, const char *expr)
{
	return (char *) xmlNodeGetContent(first_xpath_node(doc, node, expr));
}

xmlDocPtr transform_doc(xmlDocPtr doc, unsigned char *xsl, unsigned int len)
{
	xmlDocPtr styledoc, res;
	xsltStylesheetPtr style;

	styledoc = xmlReadMemory((const char *) xsl, len, NULL, NULL, 0);

	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);

	xsltFreeStylesheet(style);

	return res;
}

xmlDocPtr generate_tp(xmlDocPtr doc)
{
	return transform_doc(doc, xsl_tp_xsl, xsl_tp_xsl_len);
}

xmlDocPtr generate_toc(xmlDocPtr doc)
{
	return transform_doc(doc, xsl_toc_xsl, xsl_toc_xsl_len);
}

xmlDocPtr generate_high(xmlDocPtr doc)
{
	return transform_doc(doc, xsl_high_xsl, xsl_high_xsl_len);
}

xmlDocPtr generate_loedm(xmlDocPtr doc)
{
	return transform_doc(doc, xsl_loedm_xsl, xsl_loedm_xsl_len);
}

xmlDocPtr generate_fm_content_for_type(xmlDocPtr doc, const char *type)
{
	xmlDocPtr res = NULL;

	if (strcasecmp(type, "TP") == 0) {
		res = generate_tp(doc);
	} else if (strcasecmp(type, "TOC") == 0) {
		res = generate_toc(doc);
	} else if (strcasecmp(type, "HIGH") == 0) {
		res = generate_high(doc);
	} else if (strcasecmp(type, "LOEDM") == 0) {
		res = generate_loedm(doc);
	} else {
		fprintf(stderr, S_BAD_TYPE_ERR, type);
		exit(EXIT_BAD_TYPE);
	}

	return res;
}

char *fmtype(xmlDocPtr fmtypes, char *incode)
{
	char xpath[256];
	snprintf(xpath, 256, "//fm[@infoCode='%s']/@type", incode);
	return first_xpath_string(fmtypes, NULL, xpath);
}

void generate_fm_content_for_dm(xmlDocPtr pm, const char *dmpath, xmlDocPtr fmtypes, bool overwrite)
{
	xmlDocPtr doc, res = NULL;
	char *incode, *type;
	xmlNodePtr content;

	doc = xmlReadFile(dmpath, NULL, PARSE_OPTS);

	incode = first_xpath_string(doc, NULL, "//@infoCode");

	type = fmtype(fmtypes, incode);

	res = generate_fm_content_for_type(pm, type);

	xmlFree(type);
	xmlFree(incode);

	content = first_xpath_node(doc, NULL, "//content");
	xmlAddNextSibling(content, xmlCopyNode(xmlDocGetRootElement(res), 1));
	xmlUnlinkNode(content);
	xmlFreeNode(content);

	if (overwrite) {
		xmlSaveFile(dmpath, doc);
	} else {
		xmlSaveFile("-", doc);
	}

	xmlFreeDoc(doc);
	xmlFreeDoc(res);
}

void dump_fmtypes(void)
{
	xmlDocPtr doc;
	doc = xmlReadMemory((const char *) fmtypes_xml, fmtypes_xml_len, NULL, NULL, PARSE_OPTS);
	xmlSaveFile("-", doc);
	xmlFreeDoc(doc);
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-F <FMTYPES>] -p <PM> [-,fh?] (-t <TYPE>|<DM>...)");
	puts("");
	puts("Options:");
	puts("  -,            Dump the built-in .fmtypes file.");
	puts("  -h -?         Show usage message.");
	puts("  -F <FMTYPES>  Specify .fmtypes file.");
	puts("  -f            Overwrite input data modules.");
	puts("  -p <PM>       Generate front matter from the specified PM.");
	puts("  -t <TYPE>     Generate the specified type of front matter.");
	puts("  <DM>          Generate front matter content based on the specified data modules.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
}

int main(int argc, char **argv)
{
	int i;

	const char *sopts = ",F:fp:t:h?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	char *pmpath = NULL;
	char *fmtype = NULL;

	xmlDocPtr pm, fmtypes = NULL;

	bool overwrite = false;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case ',':
				dump_fmtypes();
				return 0;
			case 'F':
				if (!fmtypes) {
					fmtypes = xmlReadFile(optarg, NULL, PARSE_OPTS);
				}
				break;
			case 'f':
				overwrite = true;
				break;
			case 'p':
				pmpath = strdup(optarg);
				break;
			case 't':
				fmtype = strdup(optarg);
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (!fmtypes) {
		if (access(DEFAULT_FMTYPES_FNAME, F_OK) != -1) {
			fmtypes = xmlReadFile(DEFAULT_FMTYPES_FNAME, NULL, PARSE_OPTS);
		} else {
			fmtypes = xmlReadMemory((const char *) fmtypes_xml, fmtypes_xml_len, NULL, NULL, 0);
		}
	}

	if (!pmpath) {
		pmpath = strdup("-");
	}

	pm = xmlReadFile(pmpath, NULL, PARSE_OPTS);

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			generate_fm_content_for_dm(pm, argv[i], fmtypes, overwrite);
		}
	} else if (fmtype) {
		xmlDocPtr res;
		res = generate_fm_content_for_type(pm, fmtype);
		xmlSaveFile("-", res);
		xmlFreeDoc(res);
	} else {
		fprintf(stderr, S_NO_TYPE_ERR);
		exit(EXIT_NO_TYPE);
	}

	xmlFreeDoc(pm);
	xmlFreeDoc(fmtypes);

	free(pmpath);
	free(fmtype);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
