#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/uri.h>
#include <libxslt/transform.h>

#include "s1kd_tools.h"
#include "xsl.h"

#define PROG_NAME "s1kd-fmgen"
#define VERSION "3.4.1"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define INF_PREFIX PROG_NAME ": INFO: "

#define EXIT_BAD_DATE 1
#define EXIT_NO_TYPE 2
#define EXIT_BAD_TYPE 3
#define EXIT_MERGE 4
#define EXIT_BAD_STYLESHEET 5

#define S_NO_PM_ERR ERR_PREFIX "No publication module.\n"
#define S_NO_TYPE_ERR ERR_PREFIX "No FM type specified.\n"
#define S_BAD_TYPE_ERR ERR_PREFIX "Unknown front matter type: %s\n"
#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"
#define E_BAD_DATE ERR_PREFIX "Bad date: %s\n"
#define E_MERGE_NAME ERR_PREFIX "Failed to update %s: no <%s> element to merge on.\n"
#define E_MERGE_ELEM ERR_PREFIX "Failed to update %s: no front matter contents generated.\n"
#define E_BAD_STYLESHEET ERR_PREFIX "Failed to update %s: %s is not a valid stylesheet.\n"
#define I_GENERATE INF_PREFIX "Generating FM content for %s (%s)...\n"
#define I_NO_INFOCODE INF_PREFIX "Skipping %s as no FM type is associated with info code: %s%s\n"
#define I_TRANSFORM INF_PREFIX "Applying transformation %s...\n"
#define I_XPROC_TRANSFORM INF_PREFIX "Applying XProc transforation %s/%s...\n"
#define I_XPROC_TRANSFORM_NONAME INF_PREFIX "Applying XProc transformation %s/line %ld...\n"

static enum verbosity { QUIET, NORMAL, VERBOSE, DEBUG } verbosity = NORMAL;

static xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const xmlChar *expr)
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

static xmlChar *first_xpath_string(xmlDocPtr doc, xmlNodePtr node, const xmlChar *expr)
{
	return xmlNodeGetContent(first_xpath_node(doc, node, expr));
}

/* Determine whether a set of parameters already contains a name. */
static bool has_param(const char **params, const char *name)
{
	int i;

	for (i = 0; params[i]; i += 2) {
		if (strcmp(params[i], name) == 0) {
			return true;
		}
	}

	return false;
}

/* Apply an XProc XSLT step to a document. */
static xmlDocPtr apply_xproc_xslt(const xmlDocPtr doc, const char *xslpath, const xmlNodePtr xslt, const char **params)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlDocPtr res;
	const char **combined_params;
	int i, nparams = 0;

	if (verbosity >= DEBUG) {
		xmlChar *name = xmlGetProp(xslt, BAD_CAST "name");

		if (name) {
			fprintf(stderr, I_XPROC_TRANSFORM, xslpath, name);
		} else {
			fprintf(stderr, I_XPROC_TRANSFORM_NONAME, xslpath, xmlGetLineNo(xslt));
		}

		xmlFree(name);
	}

	ctx = xmlXPathNewContext(xslt->doc);
	xmlXPathRegisterNs(ctx, BAD_CAST "p", BAD_CAST "http://www.w3.org/ns/xproc");
	xmlXPathSetContextNode(xslt, ctx);

	/* Get number of current params. */
	for (i = 0; params[i]; i += 2, ++nparams);

	/* Combine the current params with additional params from the XSLT step. */
	obj = xmlXPathEvalExpression(BAD_CAST "p:with-param", ctx);
	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		/* If there are no additional params, simply copy the current
		 * params. */
		combined_params = malloc((nparams * 2 + 1) * sizeof(char *));

		for (i = 0; params[i]; ++i) {
			combined_params[i] = strdup(params[i]);
		}

		combined_params[i] = NULL;
	} else {
		/* If there are additional params, copy the current params and
		 * then append the additional ones. */
		int j;

		combined_params = malloc(((nparams + obj->nodesetval->nodeNr) * 2 + 1) * sizeof(char *));

		for (j = 0; params[j]; ++j) {
			combined_params[j] = strdup(params[j]);
		}

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			char *name;

			name = (char *) xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "name");

			/* User-defined parameters override XProc parameters. */
			if (has_param(params, name)) {
				xmlFree(name);
			} else {
				char *select;

				select = (char *) xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "select");

				combined_params[j++] = name;
				combined_params[j++] = select;
			}
		}

		combined_params[j] = NULL;
	}
	xmlXPathFreeObject(obj);

	/* Read the XProc stylesheet input. */
	obj = xmlXPathEvalExpression(BAD_CAST "p:input[@port='stylesheet']/p:*", ctx);
	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		res = xmlCopyDoc(doc, 1);
	} else {
		xmlNodePtr src = obj->nodesetval->nodeTab[0];
		xmlDocPtr s;

		if (xmlStrcmp(src->name, BAD_CAST "document") == 0) {
			xmlChar *href, *URI;
			href = xmlGetProp(obj->nodesetval->nodeTab[0], BAD_CAST "href");
			URI = xmlBuildURI(href, BAD_CAST xslpath);
			s = read_xml_doc((char *) URI, false);
			xmlFree(href);
			xmlFree(URI);
		} else if (xmlStrcmp(src->name, BAD_CAST "inline") == 0) {
			s = xmlNewDoc(BAD_CAST "1.0");
			xmlDocSetRootElement(s, xmlCopyNode(xmlFirstElementChild(src), 1));
		} else {
			s = NULL;
		}

		if (s) {
			xsltStylesheetPtr style;
			style = xsltParseStylesheetDoc(s);
			res   = xsltApplyStylesheet(style, doc, combined_params);
			xsltFreeStylesheet(style);
		} else {
			res = xmlCopyDoc(doc, 1);
		}
	}
	xmlXPathFreeObject(obj);

	xmlXPathFreeContext(ctx);

	for (i = 0; combined_params[i]; i += 2) {
		xmlFree((char *) combined_params[i]);
		xmlFree((char *) combined_params[i + 1]);
	}
	free(combined_params);

	return res;
}

static xmlDocPtr transform_doc(xmlDocPtr doc, const char *xslpath, const char **params)
{
	xmlDocPtr styledoc, res = NULL;
	xsltStylesheetPtr style;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	if (verbosity >= DEBUG) {
		fprintf(stderr, I_TRANSFORM, xslpath);
	}

	styledoc = read_xml_doc(xslpath, false);

	ctx = xmlXPathNewContext(styledoc);
	xmlXPathRegisterNs(ctx, BAD_CAST "p", BAD_CAST "http://www.w3.org/ns/xproc");
	obj = xmlXPathEvalExpression(BAD_CAST "/p:pipeline/p:xslt", ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		style = xsltParseStylesheetDoc(styledoc);
		res = xsltApplyStylesheet(style, doc, params);
		xsltFreeStylesheet(style);
		styledoc = NULL;
	} else {
		int i;
		xmlDocPtr d = xmlCopyDoc(doc, 1);

		for (i =0; i < obj->nodesetval->nodeNr; ++i) {
			res = apply_xproc_xslt(d, xslpath, obj->nodesetval->nodeTab[i], params);
			xmlFreeDoc(d);
			d = res;
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	xmlFreeDoc(styledoc);

	return res;
}

static xmlDocPtr transform_doc_builtin(xmlDocPtr doc, unsigned char *xsl, unsigned int len, const char **params)
{
	xmlDocPtr styledoc, res;
	xsltStylesheetPtr style;

	styledoc = read_xml_mem((const char *) xsl, len);

	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, params);

	xsltFreeStylesheet(style);

	return res;
}

static void get_builtin_xsl(const char *type, unsigned char **xsl, unsigned int *len)
{
	if (strcmp(type, "TP") == 0) {
		*xsl = xsl_tp_xsl;
		*len = xsl_tp_xsl_len;
	} else if (strcmp(type, "TOC") == 0) {
		*xsl = xsl_toc_xsl;
		*len = xsl_toc_xsl_len;
	} else if (strcmp(type, "HIGH") == 0) {
		*xsl = xsl_high_xsl;
		*len = xsl_high_xsl_len;
	} else if (strcmp(type, "LOEDM") == 0) {
		*xsl = xsl_loedm_xsl;
		*len = xsl_loedm_xsl_len;
	} else if (strcmp(type, "LOA") == 0) {
		*xsl = xsl_loa_xsl;
		*len = xsl_loa_xsl_len;
	} else if (strcmp(type, "LOASD") == 0) {
		*xsl = xsl_loasd_xsl;
		*len = xsl_loasd_xsl_len;
	} else if (strcmp(type, "LOI") == 0) {
		*xsl = xsl_loi_xsl;
		*len = xsl_loi_xsl_len;
	} else if (strcmp(type, "LOS") == 0) {
		*xsl = xsl_los_xsl;
		*len = xsl_los_xsl_len;
	} else if (strcmp(type, "LOT") == 0) {
		*xsl = xsl_lot_xsl;
		*len = xsl_lot_xsl_len;
	} else if (strcmp(type, "LOTBL") == 0) {
		*xsl = xsl_lotbl_xsl;
		*len = xsl_lotbl_xsl_len;
	} else {
		if (verbosity >= NORMAL) {
			fprintf(stderr, S_BAD_TYPE_ERR, type);
		}
		exit(EXIT_BAD_TYPE);
	}
}

static xmlDocPtr generate_fm(xmlDocPtr doc, const char *type, const char **params)
{
	unsigned char *xsl;
	unsigned int len;

	get_builtin_xsl(type, &xsl, &len);

	return transform_doc_builtin(doc, xsl, len, params);
}

static void set_def_param(const char **params, int i, const char *val)
{
	char *s;
	s = malloc(strlen(val) + 3);
	sprintf(s, "'%s'", val);
	free((char *) params[i]);
	params[i] = s;
}

static xmlDocPtr generate_fm_content_for_type(xmlDocPtr doc, const char *type, const char *fmxsl, const char *xslpath, const char **params)
{
	xmlDocPtr res = NULL;
	int i = 0;

	/* Supply values to default parameters. */
	for (i = 0; params[i]; i += 2) {
		if (strcmp(params[i], "type") == 0) {
			set_def_param(params, i + 1, type);
		}
	}

	/* Generate contents. */
	if (xslpath) {
		res = transform_doc(doc, xslpath, params);
	} else if (fmxsl) {
		res = transform_doc(doc, fmxsl, params);
	} else {
		res = generate_fm(doc, type, params);
	}

	return res;
}

static xmlNodePtr find_fm(xmlDocPtr fmtypes, const xmlChar *incode, const xmlChar *incodev)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;

	ctx = xmlXPathNewContext(fmtypes);
	xmlXPathRegisterVariable(ctx, BAD_CAST "c", xmlXPathNewString(BAD_CAST incode));
	xmlXPathRegisterVariable(ctx, BAD_CAST "v", xmlXPathNewString(BAD_CAST incodev));

	obj = xmlXPathEvalExpression(BAD_CAST "//fm[starts-with(concat($c,$v),@infoCode)]", ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		node = NULL;
	} else {
		node = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return node;
}

/* Copy elements from the source TP DM that can't be derived from the PM. */
static void copy_tp_elems(xmlDocPtr res, xmlDocPtr doc)
{
	xmlNodePtr fmtp, node;

	fmtp = first_xpath_node(res, NULL, BAD_CAST "//frontMatterTitlePage");

	if (!fmtp) {
		return;
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//productIntroName"))) {
		xmlAddPrevSibling(first_xpath_node(res, fmtp,
			BAD_CAST "pmTitle"),
			xmlCopyNode(node, 1));
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//externalPubCode"))) {
		xmlAddNextSibling(first_xpath_node(res, fmtp,
			BAD_CAST "issueDate"),
			xmlCopyNode(node, 1));
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//productAndModel"))) {
		xmlAddNextSibling(first_xpath_node(res, fmtp,
			BAD_CAST "(issueDate|externalPubCode)[last()]"),
			xmlCopyNode(node, 1));
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//productIllustration"))) {
		xmlAddNextSibling(first_xpath_node(res, fmtp,
			BAD_CAST "(security|derivativeClassification|dataRestrictions)[last()]"),
			xmlCopyNode(node, 1));
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//enterpriseSpec"))) {
		xmlAddNextSibling(first_xpath_node(res, fmtp,
			BAD_CAST "(security|derivativeClassification|dataRestrictions|productIllustration)[last()]"),
			xmlCopyNode(node, 1));
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//enterpriseLogo"))) {
		xmlAddNextSibling(first_xpath_node(res, fmtp,
			BAD_CAST "(security|derivativeClassification|dataRestrictions|productIllustration|enterpriseSpec)[last()]"),
			xmlCopyNode(node, 1));
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//barCode"))) {
		xmlAddNextSibling(first_xpath_node(res, fmtp,
			BAD_CAST "(responsiblePartnerCompany|publisherLogo)[last()]"),
			xmlCopyNode(node, 1));
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//frontMatterInfo"))) {
		xmlAddNextSibling(first_xpath_node(res, fmtp,
			BAD_CAST "(responsiblePartnerCompany|publisherLogo|barCode)[last()]"),
			xmlCopyNode(node, 1));
	}
}

static void set_issue_date(xmlDocPtr doc, xmlDocPtr pm, const char *issdate)
{
	xmlNodePtr dm_issue_date;
	xmlChar *year, *month, *day;

	if (!(dm_issue_date = first_xpath_node(doc, NULL, BAD_CAST "//issueDate|//issdate"))) {
		return;
	}

	if (strcasecmp(issdate, "pm") == 0) {
		xmlNodePtr pm_issue_date;

		if (!(pm_issue_date = first_xpath_node(pm, NULL, BAD_CAST "//issueDate|//issdate"))) {
			return;
		}

		year  = xmlGetProp(pm_issue_date, BAD_CAST "year");
		month = xmlGetProp(pm_issue_date, BAD_CAST "month");
		day   = xmlGetProp(pm_issue_date, BAD_CAST "day");
	} else {
		year  = malloc(5 * sizeof(xmlChar));
		month = malloc(3 * sizeof(xmlChar));
		day   = malloc(3 * sizeof(xmlChar));

		if (strcmp(issdate, "-") == 0) {
			time_t now;
			struct tm *local;

			time(&now);
			local = localtime(&now);

			xmlStrPrintf(year, 5, "%.4d", local->tm_year + 1900);
			xmlStrPrintf(month, 3, "%.2d", local->tm_mon + 1);
			xmlStrPrintf(day, 3, "%.2d", local->tm_mday);
		} else {
			int n;

			n = sscanf(issdate, "%4s-%2s-%2s", year, month, day);

			if (n != 3) {
				if (verbosity >= NORMAL) {
					fprintf(stderr, E_BAD_DATE, issdate);
				}
				exit(EXIT_BAD_DATE);
			}
		}
	}

	xmlSetProp(dm_issue_date, BAD_CAST "year", year);
	xmlSetProp(dm_issue_date, BAD_CAST "month", month);
	xmlSetProp(dm_issue_date, BAD_CAST "day", day);

	xmlFree(year);
	xmlFree(month);
	xmlFree(day);
}

static void generate_fm_content_for_dm(
	xmlDocPtr pm,
	const char *dmpath,
	xmlDocPtr fmtypes,
	const char *fmtype,
	bool overwrite,
	const char *xslpath,
	const char **params,
	const char *issdate)
{
	xmlDocPtr doc, res = NULL;
	char *type, *fmxsl;

	if (!(doc = read_xml_doc(dmpath, false))) {
		return;
	}

	if (fmtype) {
		type  = strdup(fmtype);
		fmxsl = NULL;
	} else {
		xmlChar *incode, *incodev;
		xmlNodePtr fm;

		incode  = first_xpath_string(doc, NULL, BAD_CAST "//@infoCode|//incode");
		incodev = first_xpath_string(doc, NULL, BAD_CAST "//@infoCodeVariant|//incodev");

		fm = find_fm(fmtypes, incode, incodev);

		if (fm) {
			type  = (char *) xmlGetProp(fm, BAD_CAST "type");
			fmxsl = (char *) xmlGetProp(fm, BAD_CAST "xsl");
		} else {
			if (verbosity >= DEBUG) {
				fprintf(stderr, I_NO_INFOCODE, dmpath, incode, incodev);
			}

			type = NULL;
			fmxsl = NULL;
		}

		xmlFree(incode);
		xmlFree(incodev);
	}

	if (type) {
		xmlNodePtr root;

		if (verbosity >= VERBOSE) {
			fprintf(stderr, I_GENERATE, dmpath, type);
		}

		if (!(res = generate_fm_content_for_type(pm, type, fmxsl, xslpath, params))) {
			if (verbosity >= NORMAL) {
				fprintf(stderr, E_BAD_STYLESHEET, dmpath, xslpath ? xslpath : fmxsl);
			}
			exit(EXIT_BAD_STYLESHEET);
		}

		if (strcmp(type, "TP") == 0) {
			copy_tp_elems(res, doc);
		}

		/* Merge the results of the transformation with the original DM
		 * based on the name of the root element. */
		if ((root = xmlDocGetRootElement(res))) {
			xmlXPathContextPtr ctx;
			xmlXPathObjectPtr obj;

			ctx = xmlXPathNewContext(doc);
			xmlXPathRegisterVariable(ctx, BAD_CAST "name", xmlXPathNewString(root->name));
			obj = xmlXPathEvalExpression(BAD_CAST "//*[name()=$name]", ctx);

			if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
				if (verbosity >= NORMAL) {
					fprintf(stderr, E_MERGE_NAME, dmpath, (char *) root->name);
				}
				exit(EXIT_MERGE);
			} else {
				xmlAddNextSibling(obj->nodesetval->nodeTab[0], xmlCopyNode(root, 1));
				xmlUnlinkNode(obj->nodesetval->nodeTab[0]);
				xmlFreeNode(obj->nodesetval->nodeTab[0]);
				obj->nodesetval->nodeTab[0] = NULL;
			}

			xmlXPathFreeObject(obj);
			xmlXPathFreeContext(ctx);
		} else {
			if (verbosity >= NORMAL) {
				fprintf(stderr, E_MERGE_ELEM, dmpath);
			}
			exit(EXIT_MERGE);
		}

		if (issdate) {
			set_issue_date(doc, pm, issdate);
		}

		if (overwrite) {
			save_xml_doc(doc, dmpath);
		} else {
			save_xml_doc(doc, "-");
		}
	}

	xmlFree(type);
	xmlFree(fmxsl);
	xmlFreeDoc(doc);
	xmlFreeDoc(res);
}

static void generate_fm_content_for_list(
	xmlDocPtr pm,
	const char *path,
	xmlDocPtr fmtypes,
	const char *fmtype,
	bool overwrite,
	const char *xslpath,
	const char **params,
	const char *issdate)
{
	FILE *f;
	char line[PATH_MAX];

	if (path) {
		if (!(f = fopen(path, "r"))) {
			if (verbosity >= NORMAL) {
				fprintf(stderr, E_BAD_LIST, path);
			}
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");
		generate_fm_content_for_dm(pm, line, fmtypes, fmtype, overwrite, xslpath, params, issdate);
	}

	if (path) {
		fclose(f);
	}
}

static void dump_fmtypes_xml(void)
{
	xmlDocPtr doc;
	doc = read_xml_mem((const char *) fmtypes_xml, fmtypes_xml_len);
	save_xml_doc(doc, "-");
	xmlFreeDoc(doc);
}

static void dump_fmtypes_txt(void)
{
	printf("%.*s", fmtypes_txt_len, fmtypes_txt);
}

static void dump_builtin_xsl(const char *type)
{
	unsigned char *xsl;
	unsigned int len;

	get_builtin_xsl(type, &xsl, &len);

	printf("%.*s", len, xsl);
}

static void fix_fmxsl_paths(xmlDocPtr doc, const char *path)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST "//@xsl", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *xsl = xmlNodeGetContent(obj->nodesetval->nodeTab[i]);
			xmlChar *URI = xmlBuildURI(xsl, BAD_CAST path);
			xmlNodeSetContent(obj->nodesetval->nodeTab[i], URI);
			xmlFree(xsl);
			xmlFree(URI);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

static xmlDocPtr read_fmtypes(const char *path)
{
	xmlDocPtr doc;

	if (!(doc = read_xml_doc(path, false))) {
		FILE *f;
		char line[256];
		xmlNodePtr root;

		doc = xmlNewDoc(BAD_CAST "1.0");
		root = xmlNewNode(NULL, BAD_CAST "fmtypes");
		xmlDocSetRootElement(doc, root);

		f = fopen(path, "r");

		while (fgets(line, 256, f)) {
			char incode[5] = "", type[32] = "", xsl[1024] = "";
			xmlNodePtr fm;
			int n;

			n = sscanf(line, "%4s %31s %1023[^\n]", incode, type, xsl);

			fm = xmlNewChild(root, NULL, BAD_CAST "fm", NULL);
			xmlSetProp(fm, BAD_CAST "infoCode", BAD_CAST incode);
			xmlSetProp(fm, BAD_CAST "type", BAD_CAST type);
			if (n == 3) {
				xmlSetProp(fm, BAD_CAST "xsl", BAD_CAST xsl);
			}
		}

		fclose(f);
	}

	fix_fmxsl_paths(doc, path);

	return doc;
}

static void add_param(xmlNodePtr params, char *s)
{
	char *n, *v;
	xmlNodePtr p;

	n = strtok(s, "=");
	v = strtok(NULL, "");

	p = xmlNewChild(params, NULL, BAD_CAST "param", NULL);
	xmlSetProp(p, BAD_CAST "name", BAD_CAST n);
	xmlSetProp(p, BAD_CAST "value", BAD_CAST v);
}

static void add_def_param(xmlNodePtr params, const char *s)
{
	xmlNodePtr p;
	p = xmlNewChild(params, NULL, BAD_CAST "param", NULL);
	xmlSetProp(p, BAD_CAST "name", BAD_CAST s);
	xmlSetProp(p, BAD_CAST "value", BAD_CAST "");
}

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-D <TYPE>] [-F <FMTYPES>] [-I <date>] [-P <PM>] [-p <name>=<val> ...] [-t <TYPE>] [-x <XSL>] [-,flqvh?] [<DM>...]");
	puts("");
	puts("Options:");
	puts("  -,, --dump-fmtypes-xml      Dump the built-in .fmtypes file in XML format.");
	puts("  -., --dump-fmtypes          Dump the built-in .fmtypes file in simple text format.");
	puts("  -D, --dump-xsl <TYPE>       Dump the built-in XSLT for a type of front matter.");
	puts("  -F, --fmtypes <FMTYPES>     Specify .fmtypes file.");
	puts("  -f, --overwrite             Overwrite input data modules.");
	puts("  -h, -?, --help              Show usage message.");
	puts("  -I, --date <date>           Set the issue date of the generated front matter.");
	puts("  -l, --list                  Treat input as list of data modules.");
	puts("  -P, --pm <PM>               Generate front matter from the specified PM.");
	puts("  -p, --param <name>=<value>  Pass parameters to the XSLT used to generate the front matter.");
	puts("  -q, --quiet                 Quiet mode.");
	puts("  -t, --type <TYPE>           Generate the specified type of front matter.");
	puts("  -v, --verbose               Verbose output.");
	puts("  -x, --xsl <XSL>             Override built-in or user-configured XSLT.");
	puts("  --version                   Show version information.");
	puts("  <DM>                        Generate front matter content based on the specified data modules.");
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

	const char *sopts = ",.D:F:fI:lP:p:qt:vx:h?";
	struct option lopts[] = {
		{"version"         , no_argument      , 0, 0},
		{"help"            , no_argument      , 0, 'h'},
		{"dump-fmtypes-xml", no_argument      , 0, ','},
		{"dump-fmtypes"    , no_argument      , 0, '.'},
		{"dump-xsl"        , required_argument, 0, 'D'},
		{"fmtypes"         , required_argument, 0, 'F'},
		{"date"            , required_argument, 0, 'I'},
		{"overwrite"       , no_argument      , 0, 'f'},
		{"list"            , no_argument      , 0, 'l'},
		{"pm"              , required_argument, 0, 'P'},
		{"param"           , required_argument, 0, 'p'},
		{"quiet"           , no_argument      , 0, 'q'},
		{"type"            , required_argument, 0, 't'},
		{"verbose"         , no_argument      , 0, 'v'},
		{"xsl"             , required_argument, 0, 'x'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	char *pmpath = NULL;
	char *fmtype = NULL;

	xmlDocPtr pm, fmtypes = NULL;

	bool overwrite = false;
	bool islist = false;
	char *xslpath = NULL;
	xmlNodePtr params_node;
	int nparams = 0;
	const char **params = NULL;
	char *issdate = NULL;

	params_node = xmlNewNode(NULL, BAD_CAST "params");

	/* Default parameter placeholders. */
	add_def_param(params_node, "type"); ++nparams;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case ',':
				dump_fmtypes_xml();
				return 0;
			case '.':
				dump_fmtypes_txt();
				return 0;
			case 'D':
				dump_builtin_xsl(optarg);
				return 0;
			case 'F':
				if (!fmtypes) {
					fmtypes = read_fmtypes(optarg);
				}
				break;
			case 'f':
				overwrite = true;
				break;
			case 'I':
				issdate = strdup(optarg);
				break;
			case 'l':
				islist = true;
				break;
			case 'P':
				pmpath = strdup(optarg);
				break;
			case 'p':
				add_param(params_node, optarg);
				++nparams;
				break;
			case 'q':
				--verbosity;
				break;
			case 't':
				fmtype = strdup(optarg);
				break;
			case 'v':
				++verbosity;
				break;
			case 'x':
				xslpath = strdup(optarg);
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (nparams > 0) {
		xmlNodePtr p;
		int n = 0;

		params = malloc((nparams * 2 + 1) * sizeof(char *));

		for (p = params_node->children; p; p = p->next) {
			char *name, *value;

			name  = (char *) xmlGetProp(p, BAD_CAST "name");
			value = (char *) xmlGetProp(p, BAD_CAST "value");

			params[n++] = name;
			params[n++] = value;
		}

		params[n] = NULL;
	}
	xmlFreeNode(params_node);

	if (!fmtypes) {
		char fmtypes_fname[PATH_MAX];

		if (find_config(fmtypes_fname, DEFAULT_FMTYPES_FNAME)) {
			fmtypes = read_fmtypes(fmtypes_fname);
		} else {
			fmtypes = read_xml_mem((const char *) fmtypes_xml, fmtypes_xml_len);
		}
	}

	if (!pmpath) {
		pmpath = strdup("-");
	}

	pm = read_xml_doc(pmpath, false);

	if (optind < argc) {
		void (*gen_fn)(xmlDocPtr, const char *, xmlDocPtr, const char *,
			bool, const char *, const char **, const char *);

		if (islist) {
			gen_fn = generate_fm_content_for_list;
		} else {
			gen_fn = generate_fm_content_for_dm;
		}

		for (i = optind; i < argc; ++i) {
			gen_fn(pm, argv[i], fmtypes, fmtype, overwrite, xslpath, params, issdate);
		}
	} else if (fmtype) {
		xmlDocPtr res;
		res = generate_fm_content_for_type(pm, fmtype, NULL, xslpath, params);
		save_xml_doc(res, "-");
		xmlFreeDoc(res);
	} else if (islist) {
		generate_fm_content_for_list(pm, NULL, fmtypes, fmtype, overwrite, xslpath, params, issdate);
	} else {
		if (verbosity >= NORMAL) {
			fprintf(stderr, S_NO_TYPE_ERR);
		}
		exit(EXIT_NO_TYPE);
	}

	for (i = 0; i < nparams; ++i) {
		xmlFree((char *) params[i * 2]);
		xmlFree((char *) params[i * 2 + 1]);
	}
	free(params);

	xmlFreeDoc(pm);
	xmlFreeDoc(fmtypes);

	free(pmpath);
	free(fmtype);
	free(xslpath);

	xmlFree(issdate);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
