#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <regex.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "s1kd_tools.h"

#define PROG_NAME "s1kd-metadata"
#define VERSION "4.5.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_INVALID_METADATA 1
#define EXIT_INVALID_VALUE 2
#define EXIT_NO_WRITE 3
#define EXIT_MISSING_METADATA 4
#define EXIT_NO_EDIT 5
#define EXIT_INVALID_CREATE 6
#define EXIT_NO_FILE 7
#define EXIT_CONDITION_UNMET 8

#define KEY_COLUMN_WIDTH 31

#define FMTSTR_DELIM '%'

#define DEFAULT_TIMEFMT "%Y-%m-%d"

enum verbosity {SILENT, NORMAL};

struct opts {
	xmlNodePtr conds;
	int endl;
	char *execstr;
	char *fmtstr;
	int format_all;
	xmlNodePtr keys;
	char *metadata_fname;
	int only_editable;
	int overwrite;
	char *timefmt;
	enum verbosity verbosity;
};

struct metadata {
	char *key;
	char *path;
	xmlChar *(*get)(xmlNodePtr, struct opts *);
	void (*show)(xmlNodePtr, struct opts *);
	int (*edit)(xmlNodePtr, const char *);
	int (*create)(xmlXPathContextPtr, const char *);
	char *descr;
};

struct icn_metadata {
	char *key;
	xmlChar *(*get)(const char *, struct opts *);
	void (*show)(const char *, struct opts *);
};

static xmlNodePtr first_xpath_node(char *expr, xmlXPathContextPtr ctxt)
{
	xmlXPathObjectPtr results;
	xmlNodePtr node;

	results = xmlXPathEvalExpression(BAD_CAST expr, ctxt);

	if (xmlXPathNodeSetIsEmpty(results->nodesetval)) {
		node = NULL;
	} else {
		node = results->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(results);

	return node;
}

static xmlNodePtr first_xpath_node_local(xmlNodePtr node, const xmlChar *expr)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(node->doc);
	ctx->node  = node;

	obj = xmlXPathEvalExpression(BAD_CAST expr, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		first = NULL;
	} else {
		first = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

static xmlChar *first_xpath_string(xmlNodePtr node, const xmlChar *expr)
{
	return xmlNodeGetContent(first_xpath_node_local(node, expr));
}

static xmlChar *get_issue_date(xmlNodePtr node, struct opts *opts)
{
	xmlChar *year, *month, *day, *date;
	int y, m, d;
	struct tm t = {0};

	year  = xmlGetProp(node, BAD_CAST "year");
	month = xmlGetProp(node, BAD_CAST "month");
	day   = xmlGetProp(node, BAD_CAST "day");

	y = atoi((char *) year);
	m = atoi((char *) month);
	d = atoi((char *) day);

	t.tm_mday = d;
	t.tm_mon = m - 1;
	t.tm_year = y - 1900;

	date = malloc(256);
	strftime((char *) date, 256, opts->timefmt, &t);

	xmlFree(year);
	xmlFree(month);
	xmlFree(day);

	return date;
}

static void show_issue_date(xmlNodePtr issue_date, struct opts *opts)
{
	xmlChar *date;
	date = get_issue_date(issue_date, opts);
	if (date) {
		printf("%s", (char *) date);
	}
	xmlFree(date);
}

static int edit_issue_date(xmlNodePtr issue_date, const char *val)
{
	char year[5], month[3], day[3];

	if (sscanf(val, "%4s-%2s-%2s", year, month, day) != 3) {
		return EXIT_INVALID_VALUE;
	}

	xmlSetProp(issue_date, BAD_CAST "year", BAD_CAST year);
	xmlSetProp(issue_date, BAD_CAST "month", BAD_CAST month);
	xmlSetProp(issue_date, BAD_CAST "day", BAD_CAST day);

	return 0;
}

static void show_simple_node(xmlNodePtr node, struct opts *opts)
{
	xmlChar *content = xmlNodeGetContent(node);
	if (content) {
		printf("%s", (char *) content);
	}
	xmlFree(content);
}

static int edit_simple_node(xmlNodePtr node, const char *val)
{
	xmlNodeSetContent(node, BAD_CAST val);
	return 0;
}

static int create_info_name(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr tech_name, info_name;

	tech_name = first_xpath_node("//techName|//techname", ctxt);

	if (xmlStrcmp(tech_name->name, BAD_CAST "techName") == 0) {
		info_name = xmlNewNode(NULL, BAD_CAST "infoName");
	} else {
		info_name = xmlNewNode(NULL, BAD_CAST "infoname");
	}

	info_name = xmlAddNextSibling(tech_name, info_name);
	xmlNodeSetContent(info_name, BAD_CAST val);

	return 0;
}

static int create_info_name_variant(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr info_name, info_name_variant;

	if (!(info_name = first_xpath_node("//infoName", ctxt))) {
		return 1;
	}

	info_name_variant = xmlNewNode(NULL, BAD_CAST "infoNameVariant");

	info_name_variant = xmlAddNextSibling(info_name, info_name_variant);
	xmlNodeSetContent(info_name_variant, BAD_CAST val);

	return 0;
}

static void show_simple_attr(xmlNodePtr node, const char *attr, struct opts *opts)
{
	xmlChar *text = xmlGetProp(node, BAD_CAST attr);
	if (text) {
		printf("%s", (char *) text);
	}
	xmlFree(text);
}

static int edit_simple_attr(xmlNodePtr node, const char *attr, const char *val)
{
	xmlSetProp(node, BAD_CAST attr, BAD_CAST val);
	return 0;
}

static void show_rpc_name(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "rpc") == 0) {
		show_simple_attr(node, "rpcname", opts);
	} else {
		show_simple_node(node, opts);
	}
}

static int edit_rpc_name(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "rpc") == 0) {
		return edit_simple_attr(node, "rpcname", val);
	} else {
		return edit_simple_node(node, val);
	}
}

static void show_orig_name(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "orig") == 0) {
		show_simple_attr(node, "origname", opts);
	} else {
		show_simple_node(node, opts);
	}
}

static int edit_orig_name(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "orig") == 0) {
		return edit_simple_attr(node, "origname", val);
	} else {
		return edit_simple_node(node, val);
	}
}

static void show_ent_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "orig") == 0 || xmlStrcmp(node->name, BAD_CAST "rpc") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "enterpriseCode", opts);
	}
}

static int edit_ent_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "orig") == 0 || xmlStrcmp(node->name, BAD_CAST "rpc") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "enterpriseCode", val);
	}
}

static int create_rpc_ent_code(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;
	node = first_xpath_node("//rpc|//responsiblePartnerCompany", ctxt);

	if (xmlStrcmp(node->name, BAD_CAST "rpc") == 0) {
		edit_simple_node(node, val);
	} else {
		edit_simple_attr(node, "enterpriseCode", val);
	}
	return 0;
}

static int create_orig_ent_code(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;
	node = first_xpath_node("//orig|//originator", ctxt);

	if (xmlStrcmp(node->name, BAD_CAST "orig") == 0) {
		edit_simple_node(node, val);
	} else {
		edit_simple_attr(node, "enterpriseCode", val);
	}
	return 0;
}

static void show_sec_class(xmlNodePtr node, struct opts *opts)
{
	if (xmlHasProp(node, BAD_CAST "securityClassification")) {
		show_simple_attr(node, "securityClassification", opts);
	} else {
		show_simple_attr(node, "class", opts);
	}
}

static int edit_sec_class(xmlNodePtr node, const char *val)
{
	if (xmlHasProp(node, BAD_CAST "securityClassification")) {
		return edit_simple_attr(node, "securityClassification", val);
	} else {
		return edit_simple_attr(node, "class", val);
	}
}

static xmlChar *get_issue(xmlNodePtr node, struct opts *opts)
{
	xmlChar *url;
	regex_t re;
	regmatch_t pmatch[3];
	xmlChar *iss;

	regcomp(&re, "S1000D_([0-9]+)-([0-9]+)", REG_EXTENDED);

	url = xmlGetNsProp(node, BAD_CAST "noNamespaceSchemaLocation", BAD_CAST "http://www.w3.org/2001/XMLSchema-instance");

	if (url && regexec(&re, (char *) url, 3, pmatch, 0) == 0) {
		int len1, len2, n;

		len1 = pmatch[1].rm_eo - pmatch[1].rm_so;
		len2 = pmatch[2].rm_eo - pmatch[2].rm_so;

		n = len1 + len2 + 2;

		iss = malloc(n);

		xmlStrPrintf(iss, n, "%.*s.%.*s",
			len1, url + pmatch[1].rm_so,
			len2, url + pmatch[2].rm_so);

		xmlFree(url);
	} else {
		iss = NULL;
	}

	regfree(&re);

	return iss;
}

static void show_issue(xmlNodePtr node, struct opts *opts)
{
	xmlChar *iss;
	iss = get_issue(node, opts);
	if (iss) {
		printf("%s", (char *) iss);
	}
	free(iss);
}

static int edit_issue(xmlNodePtr node, const char *val)
{
	xmlAttrPtr attr;
	char *url;
	regex_t re;
	regmatch_t pmatch[1];
	int err = 0;

	regcomp(&re, "[^/]+\\.xsd$", REG_EXTENDED);

	attr = xmlHasNsProp(node, BAD_CAST "noNamespaceSchemaLocation", BAD_CAST "http://www.w3.org/2001/XMLSchema-instance");

	url = (char *) xmlNodeGetContent((xmlNodePtr) attr);

	if (regexec(&re, url, 1, pmatch, 0) == 0) {
		xmlChar *schema, *xsi;

		schema = xmlCharStrndup(url + pmatch[0].rm_so, pmatch[0].rm_eo - pmatch[0].rm_so);

		xsi = xmlStrdup(BAD_CAST "http://www.s1000d.org/S1000D_");

		if (strcmp(val, "2.0") == 0) {
			xsi = xmlStrcat(xsi, BAD_CAST "2-0");
		} else if (strcmp(val, "2.1") == 0) {
			xsi = xmlStrcat(xsi, BAD_CAST "2-1");
		} else if (strcmp(val, "2.2") == 0) {
			xsi = xmlStrcat(xsi, BAD_CAST "2-2");
		} else if (strcmp(val, "2.3") == 0) {
			xsi = xmlStrcat(xsi, BAD_CAST "2-3");
		} else if (strcmp(val, "3.0") == 0) {
			xsi = xmlStrcat(xsi, BAD_CAST "3-0");
		} else if (strcmp(val, "4.0") == 0) {
			xsi = xmlStrcat(xsi, BAD_CAST "4-0");
		} else if (strcmp(val, "4.1") == 0) {
			xsi = xmlStrcat(xsi, BAD_CAST "4-1");
		} else if (strcmp(val, "4.2") == 0) {
			xsi = xmlStrcat(xsi, BAD_CAST "4-2");
		} else if (strcmp(val, "5.0") == 0) {
			xsi = xmlStrcat(xsi, BAD_CAST "5-0");
		} else {
			err = EXIT_INVALID_VALUE;
		}

		if (!err) {
			xsi = xmlStrcat(xsi, BAD_CAST "/xml_schema_flat/");
			xsi = xmlStrcat(xsi, schema);

			xmlSetNsProp(node, attr->ns, BAD_CAST "noNamespaceSchemaLocation", xsi);
		}

		xmlFree(schema);
		xmlFree(xsi);
	} else {
		err = EXIT_MISSING_METADATA;
	}

	regfree(&re);
	xmlFree(url);

	return err;
}

static void show_schema_url(xmlNodePtr node, struct opts *opts)
{
	show_simple_attr(node, "noNamespaceSchemaLocation", opts);
}

static int edit_schema_url(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "xsi:noNamespaceSchemaLocation", val);
}

static xmlChar *get_schema(xmlNodePtr node, struct opts *opts)
{
	char *url, *s;
	xmlChar *r;

	url = (char *) xmlGetProp(node, BAD_CAST "noNamespaceSchemaLocation");

	if (url) {
		char *e;

		s = strrchr(url, '/');
		s = s ? s + 1 : url;
		e = strrchr(s, '.');
		if (e) *e = '\0';
		r = xmlCharStrdup(s);

		xmlFree(url);
	} else {
		r = NULL;
	}

	return r;
}

static void show_schema(xmlNodePtr node, struct opts *opts)
{
	xmlChar *s;
	s = get_schema(node, opts);
	if (s) {
		printf("%s", (char *) s);
	}
	free(s);
}

static int edit_info_name(xmlNodePtr node, const char *val)
{
	if (strcmp(val, "") == 0) {
		xmlUnlinkNode(node);
		xmlFreeNode(node);
		return 0;
	} else {
		return edit_simple_node(node, val);
	}
}

static void show_type(xmlNodePtr node, struct opts *opts)
{
	printf("%s", node->name);
}

static xmlChar *get_dmcode(xmlNodePtr node, struct opts *opts)
{
	xmlChar *model_ident_code;
	xmlChar *system_diff_code;
	xmlChar *system_code;
	xmlChar *sub_system_code;
	xmlChar *sub_sub_system_code;
	xmlChar *assy_code;
	xmlChar *disassy_code;
	xmlChar *disassy_code_variant;
	xmlChar *info_code;
	xmlChar *info_code_variant;
	xmlChar *item_location_code;
	xmlChar *learn_code;
	xmlChar *learn_event_code;
	xmlChar learn[6] = "";
	xmlChar *code;

	if (xmlStrcmp(node->name, BAD_CAST "dmCode") == 0) {
		model_ident_code      = xmlGetProp(node, BAD_CAST "modelIdentCode");
		system_diff_code      = xmlGetProp(node, BAD_CAST "systemDiffCode");
		system_code           = xmlGetProp(node, BAD_CAST "systemCode");
		sub_system_code       = xmlGetProp(node, BAD_CAST "subSystemCode");
		sub_sub_system_code   = xmlGetProp(node, BAD_CAST "subSubSystemCode");
		assy_code             = xmlGetProp(node, BAD_CAST "assyCode");
		disassy_code          = xmlGetProp(node, BAD_CAST "disassyCode");
		disassy_code_variant  = xmlGetProp(node, BAD_CAST "disassyCodeVariant");
		info_code             = xmlGetProp(node, BAD_CAST "infoCode");
		info_code_variant     = xmlGetProp(node, BAD_CAST "infoCodeVariant");
		item_location_code    = xmlGetProp(node, BAD_CAST "itemLocationCode");
		learn_code            = xmlGetProp(node, BAD_CAST "learnCode");
		learn_event_code      = xmlGetProp(node, BAD_CAST "learnEventCode");

		if (learn_code && learn_event_code) xmlStrPrintf(learn, 6, "-%s%s", learn_code, learn_event_code);
	} else {
		model_ident_code     = first_xpath_string(node, BAD_CAST "modelic");
		system_diff_code     = first_xpath_string(node, BAD_CAST "sdc");
		system_code          = first_xpath_string(node, BAD_CAST "chapnum");
		sub_system_code      = first_xpath_string(node, BAD_CAST "section");
		sub_sub_system_code  = first_xpath_string(node, BAD_CAST "subsect");
		assy_code            = first_xpath_string(node, BAD_CAST "subject");
		disassy_code         = first_xpath_string(node, BAD_CAST "discode");
		disassy_code_variant = first_xpath_string(node, BAD_CAST "discodev");
		info_code            = first_xpath_string(node, BAD_CAST "incode");
		info_code_variant    = first_xpath_string(node, BAD_CAST "incodev");
		item_location_code   = first_xpath_string(node, BAD_CAST "itemloc");
		learn_code = NULL;
		learn_event_code = NULL;
	}

	if (model_ident_code     &&
	    system_diff_code     &&
	    system_code          &&
	    sub_system_code      &&
	    sub_sub_system_code  &&
	    assy_code            &&
	    disassy_code         &&
	    disassy_code_variant &&
	    info_code            &&
	    info_code_variant    &&
	    item_location_code)
	{
		code = malloc(256);

		xmlStrPrintf(code, 256, "%s-%s-%s-%s%s-%s-%s%s-%s%s-%s%s",
			model_ident_code,
			system_diff_code,
			system_code,
			sub_system_code,
			sub_sub_system_code,
			assy_code,
			disassy_code,
			disassy_code_variant,
			info_code,
			info_code_variant,
			item_location_code,
			learn);
	} else {
		code = NULL;
	}

	xmlFree(model_ident_code);
	xmlFree(system_diff_code);
	xmlFree(system_code);
	xmlFree(sub_system_code);
	xmlFree(sub_sub_system_code);
	xmlFree(assy_code);
	xmlFree(disassy_code);
	xmlFree(disassy_code_variant);
	xmlFree(info_code);
	xmlFree(info_code_variant);
	xmlFree(item_location_code);
	xmlFree(learn_code);
	xmlFree(learn_event_code);

	return code;
}

static void show_dmcode(xmlNodePtr node, struct opts *opts)
{
	xmlChar *code;
	code = get_dmcode(node, opts);
	if (code) {
		printf("%s", (char *) code);
	}
	xmlFree(code);
}

static int edit_dmcode(xmlNodePtr node, const char *val)
{
	char model_ident_code[15];
	char system_diff_code[5];
	char system_code[4];
	char sub_system_code[2];
	char sub_sub_system_code[2];
	char assy_code[5];
	char disassy_code[3];
	char disassy_code_variant[4];
	char info_code[4];
	char info_code_variant[2];
	char item_location_code[2];
	char learn_code[4];
	char learn_event_code[2];
	int n, offset;

	offset = strncmp(val, "DMC-", 4) == 0 ? 4 : 0;

	n = sscanf(val + offset, "%14[^-]-%4[^-]-%3[^-]-%1s%1s-%4[^-]-%2s%3[^-]-%3s%1s-%1s-%3s%1s",
		model_ident_code,
		system_diff_code,
		system_code,
		sub_system_code,
		sub_sub_system_code,
		assy_code,
		disassy_code,
		disassy_code_variant,
		info_code,
		info_code_variant,
		item_location_code,
		learn_code,
		learn_event_code);

	if (n != 11 && n != 13) {
		return EXIT_INVALID_VALUE;
	}

	if (xmlStrcmp(node->name, BAD_CAST "dmCode") == 0) {
		edit_simple_attr(node, "modelIdentCode", model_ident_code);
		edit_simple_attr(node, "systemDiffCode", system_diff_code);
		edit_simple_attr(node, "systemCode", system_code);
		edit_simple_attr(node, "subSystemCode", sub_system_code);
		edit_simple_attr(node, "subSubSystemCode", sub_sub_system_code);
		edit_simple_attr(node, "assyCode", assy_code);
		edit_simple_attr(node, "disassyCode", disassy_code);
		edit_simple_attr(node, "disassyCodeVariant", disassy_code_variant);
		edit_simple_attr(node, "infoCode", info_code);
		edit_simple_attr(node, "infoCodeVariant", info_code_variant);
		edit_simple_attr(node, "itemLocationCode", item_location_code);

		if (n == 13) {
			edit_simple_attr(node, "learnCode", learn_code);
			edit_simple_attr(node, "learnEventCode", learn_event_code);
		}
	} else {
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "modelic"), model_ident_code);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "sdc"), system_diff_code);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "chapnum"), system_code);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "section"), sub_system_code);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "subsect"), sub_sub_system_code);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "subject"), assy_code);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "discode"), disassy_code);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "discodev"), disassy_code_variant);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "incode"), info_code);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "incodev"), info_code_variant);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "itemloc"), item_location_code);
	}

	return 0;
}

static xmlChar *get_ddncode(xmlNodePtr node, struct opts *opts)
{
	xmlChar *modelic, *sendid, *recvid, *diyear, *seqnum, *code;

	modelic = first_xpath_string(node, BAD_CAST "@modelIdentCode|modelic");
	sendid  = first_xpath_string(node, BAD_CAST "@senderIdent|sendid");
	recvid  = first_xpath_string(node, BAD_CAST "@receiverIdent|recvid");
	diyear  = first_xpath_string(node, BAD_CAST "@yearOfDataIssue|diyear");
	seqnum  = first_xpath_string(node, BAD_CAST "@seqNumber|seqnum");

	if (modelic && sendid && recvid && diyear && seqnum) {
		code = malloc(256);

		xmlStrPrintf(code, 256, "%s-%s-%s-%s-%s",
			modelic,
			sendid,
			recvid,
			diyear,
			seqnum);
	} else {
		code = NULL;
	}

	xmlFree(modelic);
	xmlFree(sendid);
	xmlFree(recvid);
	xmlFree(diyear);
	xmlFree(seqnum);

	return code;
}

static void show_ddncode(xmlNodePtr node, struct opts *opts)
{
	xmlChar *code = get_ddncode(node, opts);
	if (code) printf("%s", (char *) code);
	xmlFree(code);
}

static xmlChar *get_dmlcode(xmlNodePtr node, struct opts *opts)
{
	xmlChar *modelic, *sendid, *dmltype, *diyear, *seqnum, *code;

	modelic = first_xpath_string(node, BAD_CAST "@modelIdentCode|modelic");
	sendid  = first_xpath_string(node, BAD_CAST "@senderIdent|sendid");
	dmltype = first_xpath_string(node, BAD_CAST "@dmlType|dmltype/@type");
	diyear  = first_xpath_string(node, BAD_CAST "@yearOfDataIssue|diyear");
	seqnum  = first_xpath_string(node, BAD_CAST "@seqNumber|seqnum");

	if (modelic && sendid && dmltype && diyear && seqnum) {
		code = malloc(256);

		xmlStrPrintf(code, 256, "%s-%s-%s-%s-%s",
			modelic,
			sendid,
			dmltype,
			diyear,
			seqnum);
	} else {
		code = NULL;
	}

	xmlFree(modelic);
	xmlFree(sendid);
	xmlFree(dmltype);
	xmlFree(diyear);
	xmlFree(seqnum);

	return code;
}

static void show_dmlcode(xmlNodePtr node, struct opts *opts)
{
	xmlChar *code = get_dmlcode(node, opts);
	if (code) printf("%s", (char *) code);
	xmlFree(code);
}

static xmlChar *get_pmcode(xmlNodePtr node, struct opts *opts)
{
	xmlChar *modelic, *pmissuer, *pmnumber, *pmvolume, *code;

	modelic  = first_xpath_string(node, BAD_CAST "@modelIdentCode|modelic");
	pmissuer = first_xpath_string(node, BAD_CAST "@pmIssuer|pmissuer");
	pmnumber = first_xpath_string(node, BAD_CAST "@pmNumber|pmnumber");
	pmvolume = first_xpath_string(node, BAD_CAST "@pmVolume|pmvolume");

	if (modelic && pmissuer && pmnumber && pmvolume) {
		code = malloc(256);

		xmlStrPrintf(code, 256, "%s-%s-%s-%s",
			modelic,
			pmissuer,
			pmnumber,
			pmvolume);
	} else {
		code = NULL;
	}

	xmlFree(modelic);
	xmlFree(pmissuer);
	xmlFree(pmnumber);
	xmlFree(pmvolume);

	return code;
}

static void show_pmcode(xmlNodePtr node, struct opts *opts)
{
	xmlChar *code = get_pmcode(node, opts);
	if (code) printf("%s", (char *) code);
	xmlFree(code);
}

static int edit_pmcode(xmlNodePtr node, const char *val)
{
	char model_ident_code[15];
	char pm_issuer[6];
	char pm_number[6];
	char pm_volume[3];
	int n, offset;

	offset = strncmp(val, "PMC-", 4) == 0 ? 4 : 0;

	n = sscanf(val + offset, "%14[^-]-%5[^-]-%5[^-]-%2s",
		model_ident_code,
		pm_issuer,
		pm_number,
		pm_volume);

	if (n != 4) {
		return EXIT_INVALID_VALUE;
	}

	if (xmlStrcmp(node->name, BAD_CAST "pmCode") == 0) {
		edit_simple_attr(node, "modelIdentCode", model_ident_code);
		edit_simple_attr(node, "pmIssuer", pm_issuer);
		edit_simple_attr(node, "pmNumber", pm_number);
		edit_simple_attr(node, "pmVolume", pm_volume);
	} else {
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "modelic"), model_ident_code);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "pmissuer"), pm_issuer);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "pmnumber"), pm_number);
		edit_simple_node(first_xpath_node_local(node, BAD_CAST "pmvolume"), pm_volume);
	}

	return 0;
}

static void show_pm_issuer(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmissuer") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "pmIssuer", opts);
	}
}

static int edit_pm_issuer(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmissuer") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "pmIssuer", val);
	}
}

static void show_pm_number(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmnumber") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "pmNumber", opts);
	}
}

static int edit_pm_number(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmnumber") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "pmNumber", val);
	}
}

static void show_pm_volume(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmvolume") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "pmVolume", opts);
	}
}

static int edit_pm_volume(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmvolume") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "pmVolume", val);
	}
}

static xmlChar *get_comment_code(xmlNodePtr node, struct opts *opts)
{
	xmlChar *model_ident_code;
	xmlChar *sender_ident;
	xmlChar *year_of_data_issue;
	xmlChar *seq_number;
	xmlChar *comment_type;
	xmlChar *code;

	if (xmlStrcmp(node->name, BAD_CAST "commentCode") == 0) {
		model_ident_code   = xmlGetProp(node, BAD_CAST "modelIdentCode");
		sender_ident       = xmlGetProp(node, BAD_CAST "senderIdent");
		year_of_data_issue = xmlGetProp(node, BAD_CAST "yearOfDataIssue");
		seq_number         = xmlGetProp(node, BAD_CAST "seqNumber");
		comment_type       = xmlGetProp(node, BAD_CAST "commentType");
	} else {
		model_ident_code   = first_xpath_string(node, BAD_CAST "modelic");
		sender_ident       = first_xpath_string(node, BAD_CAST "sendid");
		year_of_data_issue = first_xpath_string(node, BAD_CAST "diyear");
		seq_number         = first_xpath_string(node, BAD_CAST "seqnum");
		comment_type       = first_xpath_string(node, BAD_CAST "ctype/@type");
	}

	if (model_ident_code && sender_ident && year_of_data_issue && seq_number && comment_type) {
		code = malloc(256);

		xmlStrPrintf(code, 256, "%s-%s-%s-%s-%s",
			model_ident_code,
			sender_ident,
			year_of_data_issue,
			seq_number,
			comment_type);
	} else {
		code = NULL;
	}

	xmlFree(model_ident_code);
	xmlFree(sender_ident);
	xmlFree(year_of_data_issue);
	xmlFree(seq_number);
	xmlFree(comment_type);

	return code;
}

static void show_comment_code(xmlNodePtr node, struct opts *opts)
{
	xmlChar *code = get_comment_code(node, opts);
	if (code) printf("%s", (char *) code);
	xmlFree(code);
}

static xmlChar *get_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "dmCode") == 0 || xmlStrcmp(node->name, BAD_CAST "avee") == 0) {
		return get_dmcode(node, opts);
	} else if (xmlStrcmp(node->name, BAD_CAST "pmCode") == 0 || xmlStrcmp(node->name, BAD_CAST "pmc") == 0) {
		return get_pmcode(node, opts);
	} else if (xmlStrcmp(node->name, BAD_CAST "commentCode") == 0 || xmlStrcmp(node->name, BAD_CAST "ccode") == 0) {
		return get_comment_code(node, opts);
	} else if (xmlStrcmp(node->name, BAD_CAST "ddnCode") == 0 || xmlStrcmp(node->name, BAD_CAST "ddnc") == 0) {
		return get_ddncode(node, opts);
	} else if (xmlStrcmp(node->name, BAD_CAST "dmlCode") == 0 || xmlStrcmp(node->name, BAD_CAST "dmlc") == 0) {
		return get_dmlcode(node, opts);
	}

	return NULL;
}

static void show_code(xmlNodePtr node, struct opts *opts)
{
	xmlChar *code = get_code(node, opts);
	if (code) printf("%s", (char *) code);
	xmlFree(code);
}

static int edit_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "dmCode") == 0 || xmlStrcmp(node->name, BAD_CAST "avee") == 0) {
		return edit_dmcode(node, val);
	} else if (xmlStrcmp(node->name, BAD_CAST "pmCode") == 0 || xmlStrcmp(node->name, BAD_CAST "pmc") == 0) {
		return edit_pmcode(node, val);
	} else {
		return EXIT_NO_EDIT;
	}
}

static void show_issue_type(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "issno") == 0) {
		show_simple_attr(node, "type", opts);
	} else {
		show_simple_attr(node, "issueType", opts);
	}
}

static int edit_issue_type(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "issno") == 0) {
		return edit_simple_attr(node, "type", val);
	} else {
		return edit_simple_attr(node, "issueType", val);
	}
}

static void show_language_iso_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlHasProp(node, BAD_CAST "languageIsoCode")) {
		show_simple_attr(node, "languageIsoCode", opts);
	} else {
		show_simple_attr(node, "language", opts);
	}
}

static int edit_language_iso_code(xmlNodePtr node, const char *val)
{
	if (xmlHasProp(node, BAD_CAST "languageIsoCode")) {
		return edit_simple_attr(node, "languageIsoCode", val);
	} else {
		return edit_simple_attr(node, "language", val);
	}
}

static void show_country_iso_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlHasProp(node, BAD_CAST "countryIsoCode")) {
		show_simple_attr(node, "countryIsoCode", opts);
	} else {
		show_simple_attr(node, "country", opts);
	}
}

static int edit_country_iso_code(xmlNodePtr node, const char *val)
{
	if (xmlHasProp(node, BAD_CAST "countryIsoCode")) {
		return edit_simple_attr(node, "countryIsoCode", val);
	} else {
		return edit_simple_attr(node, "country", val);
	}
}

static void show_issue_number(xmlNodePtr node, struct opts *opts)
{
	if (xmlHasProp(node, BAD_CAST "issueNumber")) {
		show_simple_attr(node, "issueNumber", opts);
	} else {
		show_simple_attr(node, "issno", opts);
	}
}

static int edit_issue_number(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "issueNumber", val);
}

static void show_in_work(xmlNodePtr node, struct opts *opts)
{
	if (xmlHasProp(node, BAD_CAST "inWork")) {
		show_simple_attr(node, "inWork", opts);
	} else if (xmlHasProp(node, BAD_CAST "inwork")) {
		show_simple_attr(node, "inwork", opts);
	} else {
		printf("00");
	}
}

static int edit_in_work(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "inWork", val);
}

static xmlChar *get_issue_info(xmlNodePtr node, struct opts *opts)
{
	xmlChar *i, *w;
	i = first_xpath_string(node, BAD_CAST "@issueNumber|@issno");
	w = first_xpath_string(node, BAD_CAST "@inWork|@inwork");

	i = xmlStrcat(i, BAD_CAST "-");
	i = xmlStrcat(i, w ? w : BAD_CAST "00");

	xmlFree(w);

	return i;
}

static void show_issue_info(xmlNodePtr node, struct opts *opts)
{
	xmlChar *s;
	s = get_issue_info(node, opts);
	if (s) {
		printf("%s", (char *) s);
	}
	xmlFree(s);
}

static int create_act_ref(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;

	node = first_xpath_node("//dmStatus/originator", ctxt);

	node = xmlAddNextSibling(node, xmlNewNode(NULL, BAD_CAST "applicCrossRefTableRef"));
	node = xmlNewChild(node, NULL, BAD_CAST "dmRef", NULL);
	node = xmlNewChild(node, NULL, BAD_CAST "dmRefIdent", NULL);
	node = xmlNewChild(node, NULL, BAD_CAST "dmCode", NULL);

	return edit_dmcode(node, val);
}

static int create_comment_title(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;

	node = first_xpath_node("//commentAddressItems/issueDate", ctxt);
	
	if (!node) return EXIT_INVALID_CREATE;

	node = xmlAddNextSibling(node, xmlNewNode(NULL, BAD_CAST "commentTitle"));

	return edit_simple_node(node, val);
}

static void show_comment_priority(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "priority") == 0) {
		show_simple_attr(node, "cprio", opts);
	} else {
		show_simple_attr(node, "commentPriorityCode", opts);
	}
}

static int edit_comment_priority(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "priority") == 0) {
		return edit_simple_attr(node, "cprio", val);
	} else {
		return edit_simple_attr(node, "commentPriorityCode", val);
	}
}

static void show_comment_response(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "response") == 0) {
		show_simple_attr(node, "rsptype", opts);
	} else {
		show_simple_attr(node, "responseType", opts);
	}
}

static int edit_comment_response(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "response") == 0) {
		return edit_simple_attr(node, "rsptype", val);
	} else {
		return edit_simple_attr(node, "responseType", val);
	}
}

static int create_ent_name(xmlNodePtr node, const char *val)
{
	return xmlNewChild(node, NULL, BAD_CAST "enterpriseName", BAD_CAST val) == NULL;
}

static int create_rpc_name(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;
	node = first_xpath_node("//responsiblePartnerCompany|//rpc", ctxt);
	if (!node) {
		return EXIT_INVALID_CREATE;
	} else if (xmlStrcmp(node->name, BAD_CAST "rpc") == 0) {
		return edit_simple_attr(node, "rpcname", val);
	} else {
		return create_ent_name(node, val);
	}
}

static int create_orig_name(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;
	node = first_xpath_node("//originator|//orig", ctxt);
	if (!node) {
		return EXIT_INVALID_CREATE;
	} else if (xmlStrcmp(node->name, BAD_CAST "orig") == 0) {
		return edit_simple_attr(node, "origname", val);
	} else {
		return create_ent_name(node, val);
	}
}

static void show_url(xmlNodePtr node, struct opts *opts)
{
	printf("%s", node->doc->URL);
}

static void show_title(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "dmTitle") == 0 || xmlStrcmp(node->name, BAD_CAST "dmtitle") == 0) {
		xmlNodePtr tech, info, vari;
		xmlChar *tech_content;
		tech = first_xpath_node_local(node, BAD_CAST "techName|techname");
		info = first_xpath_node_local(node, BAD_CAST "infoName|infoname");
		vari = first_xpath_node_local(node, BAD_CAST "infoNameVariant");
		tech_content = xmlNodeGetContent(tech);
		printf("%s", (char *) tech_content);
		xmlFree(tech_content);
		if (info) {
			xmlChar *info_content;
			info_content = xmlNodeGetContent(info);
			printf(" - %s", info_content);
			xmlFree(info_content);

			if (vari) {
				xmlChar *vari_content;
				vari_content = xmlNodeGetContent(vari);
				printf(", %s", vari_content);
				xmlFree(vari_content);
			}
		}
	} else {
		show_simple_node(node, opts);
	}
}

static void show_model_ident_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "modelic") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "modelIdentCode", opts);
	}
}

static int edit_model_ident_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "modelic") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "modelIdentCode", val);
	}
}

static void show_system_diff_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "sdc") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "systemDiffCode", opts);
	}
}

static int edit_system_diff_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "sdc") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "systemDiffCode", val);
	}
}

static void show_system_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "chapnum") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "systemCode", opts);
	}
}

static int edit_system_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "chapnum") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "systemCode", val);
	}
}

static void show_sub_system_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "section") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "subSystemCode", opts);
	}
}

static int edit_sub_system_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "section") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "subSystemCode", val);
	}
}

static void show_sub_sub_system_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "subsect") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "subSubSystemCode", opts);
	}
}

static int edit_sub_sub_system_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "subsect") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "subSubSystemCode", val);
	}
}

static void show_assy_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "subject") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "assyCode", opts);
	}
}

static int edit_assy_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "subject") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "assyCode", val);
	}
}

static void show_disassy_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "discode") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "disassyCode", opts);
	}
}

static int edit_disassy_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "discode") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "disassyCode", val);
	}
}

static void show_disassy_code_variant(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "discodev") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "disassyCodeVariant", opts);
	}
}

static int edit_disassy_code_variant(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "discodev") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "disassyCodeVariant", val);
	}
}

static void show_info_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "incode") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "infoCode", opts);
	}
}

static int edit_info_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "incode") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "infoCode", val);
	}
}

static void show_info_code_variant(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "incodev") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "infoCodeVariant", opts);
	}
}

static int edit_info_code_variant(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "incodev") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "infoCodeVariant", val);
	}
}

static void show_item_location_code(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "itemloc") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "itemLocationCode", opts);
	}
}

static int edit_item_location_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "itemloc") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "itemLocationCode", val);
	}
}

static void show_learn_code(xmlNodePtr node, struct opts *opts)
{
	show_simple_attr(node, "learnCode", opts);
}

static int edit_learn_code(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "learnCode", val);
}

static void show_learn_event_code(xmlNodePtr node, struct opts *opts)
{
	show_simple_attr(node, "learnEventCode", opts);
}

static int edit_learn_event_code(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "learnEventCode", val);
}

static void show_skill_level(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "skill") == 0) {
		show_simple_attr(node, "skill", opts);
	} else {
		show_simple_attr(node, "skillLevelCode", opts);
	}
}

static int edit_skill_level(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "skill") == 0) {
		return edit_simple_attr(node, "skill", val);
	} else {
		return edit_simple_attr(node, "skillLevelCode", val);
	}
}

static int create_skill_level(xmlXPathContextPtr ctx, const char *val)
{
	xmlNodePtr node, skill_level;
	int iss30;
	node = first_xpath_node(
		"(//qualityAssurance|//qa|"
		"//systemBreakdownCode|//sbc|"
		"//functionalItemCode|//fic|"
		"//dmStatus/functionalItemRef|//status/ein"
		")[last()]", ctx);
	iss30 = xmlStrcmp(node->parent->name, BAD_CAST "status") == 0;
	skill_level = xmlNewNode(NULL, BAD_CAST (iss30 ? "skill" : "skillLevel"));
	xmlAddNextSibling(node, skill_level);
	xmlSetProp(skill_level, BAD_CAST (iss30 ? "skill" : "skillLevelCode"), BAD_CAST val);
	return 0;
}

static void show_comment_type(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "ctype") == 0) {
		show_simple_attr(node, "type", opts);
	} else {
		show_simple_attr(node, "commentType", opts);
	}
}

static int edit_comment_type(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "ctype") == 0) {
		return edit_simple_attr(node, "type", val);
	} else {
		return edit_simple_attr(node, "commentType", val);
	}
}

static void show_seq_number(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "seqnum") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "seqNumber", opts);
	}
}

static int edit_seq_number(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "seqnum") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "seqNumber", val);
	}
}

static void show_year_of_data_issue(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "diyear") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "yearOfDataIssue", opts);
	}
}

static int edit_year_of_data_issue(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "diyear") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "yearOfDataIssue", val);
	}
}

static void show_sender_ident(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "sendid") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "senderIdent", opts);
	}
}

static int edit_sender_ident(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "sendid") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "senderIdent", val);
	}
}

static void show_receiver_ident(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "recvid") == 0) {
		show_simple_node(node, opts);
	} else {
		show_simple_attr(node, "receiverIdent", opts);
	}
}

static int edit_receiver_ident(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "recvid") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "receiverIdent", val);
	}
}

static void show_source(xmlNodePtr node, struct opts *opts)
{
	xmlNodePtr dmc, issno, lang;

	dmc   = first_xpath_node_local(node, BAD_CAST "dmCode|pmCode|dmc/avee");
	issno = first_xpath_node_local(node, BAD_CAST "issueInfo|issno");
	lang  = first_xpath_node_local(node, BAD_CAST "language");

	if (xmlStrcmp(dmc->name, BAD_CAST "pmCode") == 0) {
		printf("PMC-");
		show_pmcode(dmc, opts);
	} else {
		printf("DMC-");
		show_dmcode(dmc, opts);
	}
	putchar('_');
	show_issue_number(issno, opts);
	putchar('-');
	show_in_work(issno, opts);
	putchar('_');
	show_language_iso_code(lang, opts);
	putchar('-');
	show_country_iso_code(lang, opts);
}

static xmlChar *get_qa(xmlNodePtr node, struct opts *opts)
{
	xmlNodePtr first, sec;

	first = first_xpath_node_local(node, BAD_CAST "firstVerification|firstver");
	sec   = first_xpath_node_local(node, BAD_CAST "secondVerification|secver");

	if (sec) {
		return xmlStrdup(BAD_CAST "secondVerification");
	}
	if (first) {
		return xmlStrdup(BAD_CAST "firstVerification");
	}

	return xmlStrdup(BAD_CAST "unverified");
}

static void show_qa(xmlNodePtr node, struct opts *opts)
{
	xmlChar *qa;
	qa = get_qa(node, opts);
	if (qa) {
		printf("%s", (char *) qa);
	}
	xmlFree(qa);
}

static void show_verification_type(xmlNodePtr node, struct opts *opts)
{
	if (xmlStrcmp(node->name, BAD_CAST "firstVerification") == 0 ||
	    xmlStrcmp(node->name, BAD_CAST "secondVerification") == 0)
	{
		show_simple_attr(node, "verificationType", opts);
	} else {
		show_simple_attr(node, "type", opts);
	}
}

static int edit_first_verification_type(xmlNodePtr node, const char *val)
{
	if (strcmp(val, "unverified") == 0) {
		xmlNodePtr qa, cur;

		qa = node->parent;

		cur = qa->children;
		while (cur) {
			xmlNodePtr next;
			next = cur->next;
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
			cur = next;
		}

		if (xmlStrcmp(node->name, BAD_CAST "firstVerification") == 0) {
			xmlNewChild(qa, qa->ns, BAD_CAST "unverified", NULL);
		} else {
			xmlNewChild(qa, qa->ns, BAD_CAST "unverif", NULL);
		}

		return 0;
	}

	if (xmlStrcmp(node->name, BAD_CAST "firstVerification") == 0) {
		return edit_simple_attr(node, "verificationType", val);
	} else {
		return edit_simple_attr(node, "type", val);
	}
}

static int edit_second_verification_type(xmlNodePtr node, const char *val)
{
	if (strcmp(val, "unverified") == 0) {
		xmlUnlinkNode(node);
		xmlFreeNode(node);
		return 0;
	}

	if (xmlStrcmp(node->name, BAD_CAST "secondVerification") == 0) {
		return edit_simple_attr(node, "verificationType", val);
	} else {
		return edit_simple_attr(node, "type", val);
	}
}

static int create_first_verification(xmlXPathContextPtr ctx, const char *val)
{
	xmlNodePtr unverif, first;

	if (!(strcmp(val, "tabtop") == 0 || strcmp(val, "onobject") == 0 || strcmp(val, "ttandoo") == 0)) {
		return 0;
	}

	if (!(unverif = first_xpath_node("//unverified|//unverif", ctx))) {
		return EXIT_INVALID_CREATE;
	}

	if (xmlStrcmp(unverif->name, BAD_CAST "unverified") == 0) {
		first = xmlNewNode(unverif->ns, BAD_CAST "firstVerification");
		xmlSetProp(first, BAD_CAST "verificationType", BAD_CAST val);
	} else {
		first = xmlNewNode(unverif->ns, BAD_CAST "firstver");
		xmlSetProp(first, BAD_CAST "type", BAD_CAST val);
	}

	xmlAddNextSibling(unverif, first);
	xmlUnlinkNode(unverif);
	xmlFreeNode(unverif);

	return 0;
}

static int create_second_verification(xmlXPathContextPtr ctx, const char *val)
{
	xmlNodePtr first, sec;

	if (!(strcmp(val, "tabtop") == 0 || strcmp(val, "onobject") == 0 || strcmp(val, "ttandoo") == 0)) {
		return 0;
	}

	if (!(first = first_xpath_node("//firstVerification|//firstver", ctx))) {
		return EXIT_INVALID_CREATE;
	}

	if (xmlStrcmp(first->name, BAD_CAST "firstVerification") == 0) {
		sec = xmlNewNode(first->ns, BAD_CAST "secondVerification");
		xmlSetProp(sec, BAD_CAST "verificationType", BAD_CAST val);
	} else {
		sec = xmlNewNode(first->ns, BAD_CAST "secver");
		xmlSetProp(sec, BAD_CAST "type", BAD_CAST val);
	}

	xmlAddNextSibling(first, sec);

	return 0;
}

static xmlChar *get_remarks_or_rfu(xmlNodePtr node, struct opts *opts)
{
	return xmlNodeGetContent(first_xpath_node_local(node, BAD_CAST "simplePara|p"));
}

static void show_remarks_or_rfu(xmlNodePtr node, struct opts *opts)
{
	xmlChar *s = get_remarks_or_rfu(node, opts);
	if (s) {
		printf("%s", (char *) s);
	}
	xmlFree(s);
}

static int edit_remarks_or_rfu(xmlNodePtr node, const char *val)
{
	xmlNodePtr p = first_xpath_node_local(node, BAD_CAST "simplePara|p");
	return edit_simple_node(p, val);
}

static int create_remarks(xmlXPathContextPtr ctx, const char *val)
{
	xmlNodePtr node, remarks;
	bool iss30;

	node = first_xpath_node(
		"("
		"//dmStatus/*|//status/*|"
		"//pmStatus/*|//pmstatus/*|"
		"//commentStatus/*|"
		"//ddnStatus/*|"
		"//dmlStatus/*|"
		"//scormContentPackageStatus/*"
		")[last()]", ctx);

	if (!node) {
		return EXIT_INVALID_CREATE;
	}

	iss30 = xmlStrcmp(node->parent->name, BAD_CAST "status") == 0 || xmlStrcmp(node->parent->name, BAD_CAST "pmstatus") == 0;

	remarks = xmlNewNode(NULL, BAD_CAST "remarks");
	xmlAddNextSibling(node, remarks);
	xmlNewTextChild(remarks, NULL, BAD_CAST (iss30 ? "p" : "simplePara"), BAD_CAST val);

	return 0;
}

static int create_rfu(xmlXPathContextPtr ctx, const char *val)
{
	xmlNodePtr node, rfu;
	bool iss30;

	node = first_xpath_node(
		"("
		"//dmStatus/*|//status/*|"
		"//pmStatus/*|//pmstatus/*|"
		"//commentStatus/*|"
		"//ddnStatus/*|"
		"//dmlStatus/*|"
		"//scormContentPackageStatus/*"
		")[not(self::productSafety or self::remarks)][last()]", ctx);

	if (!node) {
		return EXIT_INVALID_CREATE;
	}

	iss30 = xmlStrcmp(node->parent->name, BAD_CAST "status") == 0 || xmlStrcmp(node->parent->name, BAD_CAST "pmstatus") == 0;

	rfu = xmlNewNode(NULL, BAD_CAST (iss30 ? "rfu" : "reasonForUpdate"));
	xmlAddNextSibling(node, rfu);
	xmlNewTextChild(rfu, NULL, BAD_CAST (iss30 ? "p" : "simplePara"), BAD_CAST val);

	return 0;
}

static struct metadata metadata[] = {
	{"act",
		"//applicCrossRefTableRef/dmRef/dmRefIdent/dmCode",
		get_dmcode,
		show_dmcode,
		edit_dmcode,
		create_act_ref,
		"ACT data module code"},
	{"applic",
		"//applic/displayText/simplePara|//applic/displaytext/p",
		NULL,
		show_simple_node,
		edit_simple_node,
		NULL,
		"Whole data module applicability"},
	{"assyCode",
		"//@assyCode|//avee/subject",
		NULL,
		show_assy_code,
		edit_assy_code,
		NULL,
		"Assembly code"},
	{"authorization",
		"//authorization|//authrtn",
		NULL,
		show_simple_node,
		edit_simple_node,
		NULL,
		"Authorization for a DDN"},
	{"brex",
		"//brexDmRef/dmRef/dmRefIdent/dmCode|//brexref/refdm/avee",
		get_dmcode,
		show_dmcode,
		edit_dmcode,
		NULL,
		"BREX data module code"},
	{"code",
		"//dmCode|//avee|//pmCode|//pmc|//commentCode|//ccode|//ddnCode|//ddnc|//dmlCode|//dmlc",
		get_code,
		show_code,
		edit_code,
		NULL,
		"CSDB object code"},
	{"commentCode",
		"//commentCode|//ccode",
		get_comment_code,
		show_comment_code,
		NULL,
		NULL,
		"Comment code"},
	{"commentPriority",
		"//commentPriority/@commentPriorityCode|//priority/@cprio",
		NULL,
		show_comment_priority,
		edit_comment_priority,
		NULL,
		"Priority code of a comment"},
	{"commentResponse",
		"//commentResponse/@responseType|//response/@rsptype",
		NULL,
		show_comment_response,
		edit_comment_response,
		NULL,
		"Response type of a comment"},
	{"commentTitle",
		"//commentTitle|//ctitle",
		NULL,
		show_simple_node,
		edit_simple_node,
		create_comment_title,
		"Title of a comment"},
	{"commentType",
		"//@commentType|//ctype/@type",
		NULL,
		show_comment_type,
		edit_comment_type,
		NULL,
		"Type of a comment"},
	{"countryIsoCode",
		"//language/@countryIsoCode|//language/@country",
		NULL,
		show_country_iso_code,
		edit_country_iso_code,
		NULL,
		"Country ISO code (CA, US, GB...)"},
	{"ddnCode",
		"//ddnCode|//ddnc",
		get_ddncode,
		show_ddncode,
		NULL,
		NULL,
		"Data dispatch note code"},
	{"disassyCode",
		"//@disassyCode|//discode",
		NULL,
		show_disassy_code,
		edit_disassy_code,
		NULL,
		"Disassembly code"},
	{"disassyCodeVariant",
		"//@disassyCodeVariant|//discodev",
		NULL,
		show_disassy_code_variant,
		edit_disassy_code_variant,
		NULL,
		"Disassembly code variant"},
	{"dmCode",
		"//dmCode|//avee",
		get_dmcode,
		show_dmcode,
		edit_dmcode,
		NULL,
		"Data module code"},
	{"dmlCode",
		"//dmlCode|//dmlc",
		get_dmlcode,
		show_dmlcode,
		NULL,
		NULL,
		"Data management list code"},
	{"firstVerificationType",
		"//firstVerification/@verificationType|//firstver/@type",
		NULL,
		show_verification_type,
		edit_first_verification_type,
		create_first_verification,
		"First verification type"},
	{"format",
		"false()",
		NULL,
		NULL,
		NULL,
		NULL,
		"File format of the object"},
	{"icnTitle",
		"//imfAddressItems/icnTitle",
		NULL,
		show_simple_node,
		edit_simple_node,
		NULL,
		"Title of an IMF"},
	{"infoCode",
		"//@infoCode|//incode",
		NULL,
		show_info_code,
		edit_info_code,
		NULL,
		"Information code"},
	{"infoCodeVariant",
		"//@infoCodeVariant|//incodev",
		NULL,
		show_info_code_variant,
		edit_info_code_variant,
		NULL,
		"Information code variant"},
	{"infoName",
		"//infoName|//infoname",
		NULL,
		show_simple_node,
		edit_info_name,
		create_info_name,
		"Information name of a data module"},
	{"infoNameVariant",
		"//infoNameVariant",
		NULL,
		show_simple_node,
		edit_info_name,
		create_info_name_variant,
		"Information name variant of a data module"},
	{"inWork",
		"//issueInfo/@inWork|//issno",
		NULL,
		show_in_work,
		edit_in_work,
		NULL,
		"Inwork issue number (NN)"},
	{"issue",
		"//*",
		get_issue,
		show_issue,
		edit_issue,
		NULL,
		"Issue of S1000D"},
	{"issueDate",
	 	"//issueDate|//issdate",
		get_issue_date,
		show_issue_date,
		edit_issue_date,
		NULL,
		"Issue date of the CSDB object"},
	{"issueInfo",
		"//issueInfo|//issno",
		get_issue_info,
		show_issue_info,
		NULL,
		NULL,
		"Issue info (NNN-NN)"},
	{"issueNumber",
		"//issueInfo/@issueNumber|//issno/@issno",
		NULL,
		show_issue_number,
		edit_issue_number,
		NULL,
		"Issue number (NNN)"},
	{"issueType",
		"//dmStatus/@issueType|//pmStatus/@issueType|//issno/@type",
		NULL,
		show_issue_type,
		edit_issue_type,
		NULL,
		"Issue type (new, changed, deleted...)"},
	{"itemLocationCode",
		"//@itemLocationCode|//itemloc",
		NULL,
		show_item_location_code,
		edit_item_location_code,
		NULL,
		"Item location code"},
	{"languageIsoCode",
		"//language/@languageIsoCode|//language/@language",
		NULL,
		show_language_iso_code,
		edit_language_iso_code,
		NULL,
		"Language ISO code (en, fr, es...)"},
	{"learnCode",
		"//@learnCode",
		NULL,
		show_learn_code,
		edit_learn_code,
		NULL,
		"Learn code"},
	{"learnEventCode",
		"//@learnEventCode",
		NULL,
		show_learn_event_code,
		edit_learn_event_code,
		NULL,
		"Learn event code"},
	{"modelIdentCode",
		"//@modelIdentCode|//modelic",
		NULL,
		show_model_ident_code,
		edit_model_ident_code,
		NULL,
		"Model identification code"},
	{"modified",
		"false()",
		NULL,
		NULL,
		NULL,
		NULL,
		"File modification time"},
	{"originator",
		"//originator/enterpriseName|//orig/@origname",
		NULL,
		show_orig_name,
		edit_orig_name,
		create_orig_name,
		"Name of the originator"},
	{"originatorCode",
		"//originator/@enterpriseCode|//orig[. != '']",
		NULL,
		show_ent_code,
		edit_ent_code,
		create_orig_ent_code,
		"NCAGE code of the originator"},
	{"path",
		"false()",
		NULL,
		NULL,
		NULL,
		NULL,
		"Filesystem path of object"},
	{"pmCode",
		"//pmCode|//pmc",
		get_pmcode,
		show_pmcode,
		edit_pmcode,
		NULL,
		"Publication module code"},
	{"pmIssuer",
		"//@pmIssuer|//pmissuer",
		NULL,
		show_pm_issuer,
		edit_pm_issuer,
		NULL,
		"Issuing authority of the PM"},
	{"pmNumber",
		"//@pmNumber|//pmnumber",
		NULL,
		show_pm_number,
		edit_pm_number,
		NULL,
		"PM number"},
	{"pmTitle",
		"//pmTitle|//pmtitle",
		NULL,
		show_simple_node,
		edit_simple_node,
		NULL,
		"Title of a publication module"},
	{"pmVolume",
		"//@pmVolume|//pmvolume",
		NULL,
		show_pm_volume,
		edit_pm_volume,
		NULL,
		"Volume of the PM"},
	{"qualityAssurance",
		"//qualityAssurance|//qa",
		get_qa,
		show_qa,
		NULL,
		NULL,
		"Quality assurance status"},
	{"reasonForUpdate",
		"//reasonForUpdate|//rfu",
		get_remarks_or_rfu,
		show_remarks_or_rfu,
		edit_remarks_or_rfu,
		create_rfu,
		"Reason for update"},
	{"receiverIdent",
		"//@receiverIdent|//recvid",
		NULL,
		show_receiver_ident,
		edit_receiver_ident,
		NULL,
		"Receiving authority"},
	{"remarks",
		"//dmStatus/remarks|//status/remarks|//pmStatus/remarks|//pmstatus/remarks|//commentStatus/remarks|//dmlStatus/remarks|//ddnStatus/remarks|//scormContentPackageStatus/remarks|//imfStatus/remarks",
		get_remarks_or_rfu,
		show_remarks_or_rfu,
		edit_remarks_or_rfu,
		create_remarks,
		"General remarks"},
	{"responsiblePartnerCompany",
		"//responsiblePartnerCompany/enterpriseName|//rpc/@rpcname",
		NULL,
		show_rpc_name,
		edit_rpc_name,
		create_rpc_name,
		"Name of the RPC"},
	{"responsiblePartnerCompanyCode",
		"//responsiblePartnerCompany/@enterpriseCode|//rpc[. != '']",
		NULL,
		show_ent_code,
		edit_ent_code,
		create_rpc_ent_code,
		"NCAGE code of the RPC"},
	{"schema",
		"/*",
		get_schema,
		show_schema,
		NULL,
		NULL,
		"S1000D schema name"},
	{"schemaUrl",
		"/*",
		NULL,
		show_schema_url,
		edit_schema_url,
		NULL,
		"XML schema URL"},
	{"secondVerificationType",
		"//secondVerification/@verificationType|//secver/@type",
		NULL,
		show_verification_type,
		edit_second_verification_type,
		create_second_verification,
		"Second verification type"},
	{"securityClassification",
		"//security/@securityClassification|//security/@class",
		NULL,
		show_sec_class,
		edit_sec_class,
		NULL,
		"Security classification (01, 02...)"},
	{"senderIdent",
		"//@senderIdent|//sendid",
		NULL,
		show_sender_ident,
		edit_sender_ident,
		NULL,
		"Issuing authority"},
	{"seqNumber",
		"//@seqNumber|//seqnum",
		NULL,
		show_seq_number,
		edit_seq_number,
		NULL,
		"Sequence number"},
	{"shortPmTitle",
		"//shortPmTitle",
		NULL,
		show_simple_node,
		edit_simple_node,
		NULL,
		"Short title of a publication module"},
	{"skillLevelCode",
		"//dmStatus/skillLevel/@skillLevelCode|//status/skill/@skill",
		NULL,
		show_skill_level,
		edit_skill_level,
		create_skill_level,
		"Skill level code of the data module"},
	{"source",
		"//sourceDmIdent|//sourcePmIdent|//srcdmaddres",
		NULL,
		show_source,
		NULL,
		NULL,
		"Full source DM or PM identification"},
	{"sourceDmCode",
		"//sourceDmIdent/dmCode|//srcdmaddres/dmc/avee",
		get_dmcode,
		show_dmcode,
		edit_dmcode,
		NULL,
		"Source DM code"},
	{"sourcePmCode",
		"//sourcePmIdent/pmCode",
		get_pmcode,
		show_pmcode,
		edit_pmcode,
		NULL,
		"Source PM code"},
	{"sourceIssueNumber",
		"//sourceDmIdent/issueInfo/@issueNumber|//sourcePmIdent/issueInfo/@issueNumber|//srcdmaddres/issno/@issno",
		NULL,
		show_issue_number,
		edit_issue_number,
		NULL,
		"Source DM or PM issue number"},
	{"sourceInWork",
		"//sourceDmIdent/issueInfo/@inWork|//sourcePmIdent/issueInfo/@inWork|//srcdmaddres/issno",
		NULL,
		show_in_work,
		edit_in_work,
		NULL,
		"Source DM or PM inwork issue number"},
	{"sourceLanguageIsoCode",
		"//sourceDmIdent/language/@languageIsoCode|//sourcePmIdent/language/@languageIsoCode|//srcdmaddres/language/@language",
		NULL,
		show_language_iso_code,
		edit_language_iso_code,
		NULL,
		"Source DM or PM language ISO code"},
	{"sourceCountryIsoCode",
		"//sourceDmIdent/language/@countryIsoCode|//sourcePmIdent/language/@countryIsoCode|//srcdmaddres/language/@country",
		NULL,
		show_country_iso_code,
		edit_country_iso_code,
		NULL,
		"Source DM or PM country ISO code"},
	{"subSubSystemCode",
		"//@subSubSystemCode|//subsect",
		NULL,
		show_sub_sub_system_code,
		edit_sub_sub_system_code,
		NULL,
		"Subsubsystem code"},
	{"subSystemCode",
		"//@subSystemCode|//section",
		NULL,
		show_sub_system_code,
		edit_sub_system_code,
		NULL,
		"Subsystem code"},
	{"systemCode",
		"//@systemCode|//chapnum",
		NULL,
		show_system_code,
		edit_system_code,
		NULL,
		"System code"},
	{"systemDiffCode",
		"//@systemDiffCode|//sdc",
		NULL,
		show_system_diff_code,
		edit_system_diff_code,
		NULL,
		"System difference code"},
	{"techName",
		"//techName|//techname",
		NULL,
		show_simple_node,
		edit_simple_node,
		NULL,
		"Technical name of a data module"},
	{"title",
		"//dmTitle|//dmtitle|//pmTitle|//pmtitle|//commentTitle|//ctitle|//icnTitle",
		NULL,
		show_title,
		NULL,
		NULL,
		"Title of a CSDB object"},
	{"type",
		"/*",
		NULL,
		show_type,
		NULL,
		NULL,
		"Name of the root element of the document"},
	{"url",
		"/",
		NULL,
		show_url,
		NULL,
		NULL,
		"URL of the document"},
	{"yearOfDataIssue",
		"//@yearOfDataIssue|//diyear",
		NULL,
		show_year_of_data_issue,
		edit_year_of_data_issue,
		NULL,
		"Year of data issue"},
	{NULL}
};

static xmlChar *get_icn_code(const char *bname, struct opts *opts)
{
	int n;
	n = strchr(bname, '.') - bname;
	return xmlCharStrndup(bname, n);
}

static void show_icn_code(const char *bname, struct opts *opts)
{
	xmlChar *s;
	s = get_icn_code(bname, opts);
	printf("%s", (char *) s);
	free(s);
}

static xmlChar *get_icn_sec(const char *bname, struct opts *opts)
{
	char *s, *e;
	int n;
	s = strrchr(bname, '-');
	++s;
	e = strchr(s, '.');
	n = e - s;
	return xmlCharStrndup(s, n);
}

static void show_icn_sec(const char *bname, struct opts *opts)
{
	xmlChar *s;
	s = get_icn_sec(bname, opts);
	printf("%s", (char *) s);
	free(s);
}

static xmlChar *get_icn_iss(const char *bname, struct opts *opts)
{
	char *s, *e;
	int n;
	s = strrchr(bname, '-');
	s = s - 3;
	e = strchr(s, '-');
	n = e - s;
	return xmlCharStrndup(s, n);
}

static void show_icn_iss(const char *bname, struct opts *opts)
{
	xmlChar *s;
	s = get_icn_iss(bname, opts);
	printf("%s", (char *) s);
	free(s);
}

static xmlChar *get_icn_type(const char *bname, struct opts *opts)
{
	return xmlCharStrdup("icn");
}

static void show_icn_type(const char *bname, struct opts *opts)
{
	printf("icn");
}

static struct icn_metadata icn_metadata[] = {
	{"code", get_icn_code, show_icn_code},
	{"issueNumber", get_icn_iss, show_icn_iss},
	{"securityClassification", get_icn_sec, show_icn_sec},
	{"type", get_icn_type, show_icn_type},
	{NULL}
};

static int show_metadata(xmlXPathContextPtr ctxt, const char *key, struct opts *opts)
{
	int i;

	for (i = 0; metadata[i].key; ++i) {
		if (strcmp(key, metadata[i].key) == 0) {
			xmlNodePtr node;
			if (!(node = first_xpath_node(metadata[i].path, ctxt))) {
				if (opts->endl > -1) putchar(opts->endl);
				return EXIT_MISSING_METADATA;
			}
			if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;
			metadata[i].show(node, opts);
			return EXIT_SUCCESS;
		}
	}

	if (opts->endl > -1) putchar(opts->endl);

	return EXIT_INVALID_METADATA;
}

static int edit_metadata(xmlXPathContextPtr ctxt, const char *key, const char *val)
{
	int i;

	for (i = 0; metadata[i].key; ++i) {
		if (strcmp(key, metadata[i].key) == 0) {
			xmlNodePtr node;
			if (!(node = first_xpath_node(metadata[i].path, ctxt))) {
				if (metadata[i].create) {
					return metadata[i].create(ctxt, val);
				} else {
					return EXIT_NO_EDIT;
				}
			} else {
				if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;
				if (metadata[i].edit) {
					return metadata[i].edit(node, val);
				} else {
					return EXIT_NO_EDIT;
				}
			}
		}
	}

	return EXIT_INVALID_METADATA;
}

static int show_all_metadata(xmlXPathContextPtr ctxt, struct opts *opts)
{
	int i;

	for (i = 0; metadata[i].key; ++i) {
		xmlNodePtr node;

		if (opts->only_editable && !metadata[i].edit) continue;

		if ((node = first_xpath_node(metadata[i].path, ctxt))) {
			if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;

			if (opts->endl == '\n') {
				printf("%s", metadata[i].key);

				if (opts->format_all) {
					int n = KEY_COLUMN_WIDTH - strlen(metadata[i].key);
					int j;
					for (j = 0; j < n; ++j) putchar(' ');
				} else {
					putchar('\t');
				}
			}

			metadata[i].show(node, opts);
			if (opts->endl > -1) putchar(opts->endl);
		}
	}

	return 0;
}

static int edit_all_metadata(FILE *input, xmlXPathContextPtr ctxt)
{
	char key[256], val[256];

	while (fscanf(input, "%255s %255[^\n]", key, val) == 2) {
		edit_metadata(ctxt, key, val);
	}

	return 0;
}

static void list_metadata_key(const char *key, const char *descr, struct opts *opts)
{
	int n = KEY_COLUMN_WIDTH - strlen(key);
	printf("%s", key);
	if (opts->format_all) {
		int j;
		for (j = 0; j < n; ++j) putchar(' ');
	} else {
		putchar('\t');
	}
	printf("%s", descr);
	putchar('\n');
}

static int has_key(xmlNodePtr keys, const char *key)
{
	if (keys->children) {
		xmlNodePtr cur;
		for (cur = keys->children; cur; cur = cur->next) {
			xmlChar *name;
			int match;

			name = xmlGetProp(cur, BAD_CAST "name");
			match = xmlStrcmp(name, BAD_CAST key) == 0;
			xmlFree(name);

			if (match) return 1;
		}

		return 0;
	}

	return 1;
}

static void list_metadata_keys(struct opts *opts)
{
	int i;
	for (i = 0; metadata[i].key; ++i) {
		if (has_key(opts->keys, metadata[i].key) && (!opts->only_editable || metadata[i].edit)) {
			list_metadata_key(metadata[i].key, metadata[i].descr, opts);
		}
	}
}

static int show_err(int err, const char *key, const char *val, const char *fname, struct opts *opts)
{
	if (opts->verbosity < NORMAL) return err;

	switch (err) {
		case EXIT_INVALID_METADATA:
			if (val) {
				fprintf(stderr, ERR_PREFIX "Cannot edit metadata: %s\n", key);
			} else {
				fprintf(stderr, ERR_PREFIX "Invalid metadata name: %s\n", key);
			}
			break;
		case EXIT_INVALID_VALUE:
			fprintf(stderr, ERR_PREFIX "Invalid value for %s: %s\n", key, val);
			break;
		case EXIT_MISSING_METADATA:
			fprintf(stderr, ERR_PREFIX "Data has no metadata: %s\n", key);
			break;
		case EXIT_NO_EDIT:
			fprintf(stderr, ERR_PREFIX "Cannot edit metadata: %s\n", key);
			break;
		case EXIT_INVALID_CREATE:
			fprintf(stderr, ERR_PREFIX "%s is not valid metadata for %s\n", key, fname);
			break;
	}

	return err;
}

static int show_path(const char *fname, struct opts *opts)
{
	printf("%s", fname);
	return 0;
}

static char *get_format(const char *bname, struct opts *opts)
{
	char *s;
	if ((s = strchr(bname, '.'))) {
		return s + 1;
	} else {
		return "";
	}
}

static int show_format(const char *bname, struct opts *opts)
{
	printf("%s", get_format(bname, opts));
	return 0;
}

static char *get_modtime(const char *fname, struct opts *opts)
{
	struct stat st;
	char *buf;

	stat(fname, &st);
	buf = malloc(256);

	strftime(buf, 256, opts->timefmt, localtime(&st.st_mtime));

	return buf;
}

static int show_modtime(const char *fname, struct opts *opts)
{
	char *s;
	s = get_modtime(fname, opts);
	printf("%s", s);
	free(s);
	return 0;
}

static int show_metadata_fmtstr_key(xmlXPathContextPtr ctx, const char *k, int n, struct opts *opts)
{
	int i;
	char *key;

	key = malloc(n + 1);
	sprintf(key, "%.*s", n, k);

	for (i = 0; metadata[i].key; ++i) {
		if (strcmp(metadata[i].key, key) == 0) {
			xmlNodePtr node;
			if (!(node = first_xpath_node(metadata[i].path, ctx))) {
				show_err(EXIT_MISSING_METADATA, key, NULL, NULL, opts);
				free(key);
				return EXIT_MISSING_METADATA;
			}
			if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;
			metadata[i].show(node, opts);
			free(key);
			return EXIT_SUCCESS;
		}
	}

	show_err(EXIT_INVALID_METADATA, key, NULL, NULL, opts);
	free(key);
	return EXIT_INVALID_METADATA;
}

static int show_icn_metadata_fmtstr_key(const char *bname, const char *k, int n, struct opts *opts)
{
	int i;
	char *key;

	key = malloc(n + 1);
	sprintf(key, "%.*s", n, k);

	for (i = 0; icn_metadata[i].key; ++i) {
		if (strcmp(icn_metadata[i].key, key) == 0) {
			icn_metadata[i].show(bname, opts);
			free(key);
			return EXIT_SUCCESS;
		}
	}

	show_err(EXIT_INVALID_METADATA, key, NULL, NULL, opts);
	free(key);
	return EXIT_INVALID_METADATA;
}

static int show_metadata_fmtstr(const char *fname, xmlXPathContextPtr ctx, struct opts *opts)
{
	int i;
	for (i = 0; opts->fmtstr[i]; ++i) {
		if (opts->fmtstr[i] == FMTSTR_DELIM) {
			if (opts->fmtstr[i + 1] == FMTSTR_DELIM) {
				putchar(FMTSTR_DELIM);
				++i;
			} else {
				const char *k, *e;
				int n;
				char *s, *bname;

				k = opts->fmtstr + i + 1;
				e = strchr(k, FMTSTR_DELIM);
				if (!e) break;
				n = e - k;

				s = strdup(fname);
				bname = basename(s);

				if (strncmp(k, "path", n) == 0) {
					show_path(fname, opts);
				} else if (strncmp(k, "format", n) == 0) {
					show_format(bname, opts);
				} else if (strncmp(k, "modified", n) == 0) {
					show_modtime(fname, opts);
				} else if (is_icn(bname)) {
					show_icn_metadata_fmtstr_key(bname, k, n, opts);
				} else {
					show_metadata_fmtstr_key(ctx, k, n, opts);
				}

				free(s);

				i += n + 1;
			}
		} else if (opts->fmtstr[i] == '\\') {
			switch (opts->fmtstr[i + 1]) {
				case 'n': putchar('\n'); ++i; break;
				case 't': putchar('\t'); ++i; break;
				case '0': putchar('\0'); ++i; break;
				default: putchar(opts->fmtstr[i]);
			}
		} else {
			putchar(opts->fmtstr[i]);
		}
	}
	return 0;
}

static xmlChar *get_cond_content(int i, xmlXPathContextPtr ctx, const char *fname, struct opts *opts)
{
	xmlNodePtr node;

	if (strcmp(metadata[i].key, "path") == 0) {
		return xmlCharStrdup(fname);
	} else if (strcmp(metadata[i].key, "format") == 0) {
		return xmlCharStrdup(get_format(fname, opts));
	} else if (strcmp(metadata[i].key, "modified") == 0) {
		return xmlCharStrdup(get_modtime(fname, opts));
	} else if ((node = first_xpath_node(metadata[i].path, ctx))) {
		if (metadata[i].get) {
			return BAD_CAST metadata[i].get(node, opts);
		} else {
			return xmlNodeGetContent(node);
		}
	}

	return NULL;
}

static int condition_cmp(const xmlChar *op, const xmlChar *content, const xmlChar *val, const xmlChar *regex)
{
	int cmp;

	if (regex) {
		switch (op[0]) {
			case '=': cmp = val == NULL ? content != NULL : match_pattern(content, val); break;
			case '~': cmp = val == NULL ? content == NULL : !match_pattern(content, val); break;
			default: cmp = 0; break;
		}
	} else {
		switch (op[0]) {
			case '=': cmp = val == NULL ? content != NULL : xmlStrcmp(content, val) == 0; break;
			case '~': cmp = val == NULL ? content == NULL : xmlStrcmp(content, val) != 0; break;
			default: cmp = 0; break;
		}
	}

	return cmp;
}

static int condition_met(xmlXPathContextPtr ctx, xmlNodePtr cond, const char *fname, const char *bname, struct opts *opts)
{
	xmlChar *key, *val, *op, *regex;
	int i, cmp = 0;

	key = xmlGetProp(cond, BAD_CAST "key");
	val = xmlGetProp(cond, BAD_CAST "val");
	op = xmlGetProp(cond, BAD_CAST "op");
	regex = xmlGetProp(cond, BAD_CAST "regex");

	if (is_icn(bname)) {
		for (i = 0; icn_metadata[i].key; ++i) {
			if (xmlStrcmp(key, BAD_CAST icn_metadata[i].key) == 0) {
				xmlChar *content;

				content = icn_metadata[i].get(bname, opts);
				cmp = condition_cmp(op, content, val, regex);
				xmlFree(content);

				break;
			}
		}
	} else {
		for (i = 0; metadata[i].key; ++i) {
			if (xmlStrcmp(key, BAD_CAST metadata[i].key) == 0) {
				xmlChar *content;

				content = get_cond_content(i, ctx, fname, opts);
				cmp = condition_cmp(op, content, val, regex);
				xmlFree(content);

				break;
			}
		}
	}

	xmlFree(key);
	xmlFree(val);
	xmlFree(op);
	xmlFree(regex);

	return cmp;
}

static int show_icn_metadata(const char *bname, const char *key, struct opts *opts)
{
	int i;

	for (i = 0; icn_metadata[i].key; ++i) {
		if (strcmp(key, icn_metadata[i].key) == 0) {
			icn_metadata[i].show(bname, opts);
			return EXIT_SUCCESS;
		}
	}

	if (opts->endl > -1) putchar(opts->endl);

	return EXIT_INVALID_METADATA;
}

static int show_all_icn_metadata(const char *fname, struct opts *opts)
{
	int i;

	for (i = 0; icn_metadata[i].key; ++i) {
		if (opts->endl == '\n') {
			printf("%s", icn_metadata[i].key);

			if (opts->format_all) {
				int n = KEY_COLUMN_WIDTH - strlen(icn_metadata[i].key);
				int j;
				for (j = 0; j < n; ++j) putchar(' ');
			} else {
				putchar('\t');
			}
		}

		icn_metadata[i].show(fname, opts);
		if (opts->endl > -1) putchar(opts->endl);
	}

	return 0;
}

static int show_or_edit_metadata(const char *fname, struct opts *opts)
{
	int err = 0;
	xmlDocPtr doc;
	xmlXPathContextPtr ctxt;
	int edit = 0;
	xmlNodePtr cond;
	char *s, *bname;

	doc = read_xml_doc(fname);
	ctxt = xmlXPathNewContext(doc);

	s = strdup(fname);
	bname = basename(s);

	for (cond = opts->conds->children; cond; cond = cond->next) {
		if (!condition_met(ctxt, cond, fname, bname, opts)) {
			err = EXIT_CONDITION_UNMET;
		}
	}

	if (!err) {
		if (opts->execstr) {
			err = execfile(opts->execstr, fname) != 0;
		} else if (opts->fmtstr) {
			err = show_metadata_fmtstr(fname, ctxt, opts);
		} else if (opts->keys->children) {
			xmlNodePtr cur;
			for (cur = opts->keys->children; cur; cur = cur->next) {
				char *key = NULL, *val = NULL;

				key = (char *) xmlGetProp(cur, BAD_CAST "name");
				val = (char *) xmlGetProp(cur, BAD_CAST "value");

				if (val) {
					edit = 1;
					err = edit_metadata(ctxt, key, val);
				} else if (strcmp(key, "path") == 0) {
					err = show_path(fname, opts);
				} else if (strcmp(key, "format") == 0) {
					err = show_format(bname, opts);
				} else if (strcmp(key, "modified") == 0) {
					err = show_modtime(fname, opts);
				} else if (is_icn(bname)) {
					err = show_icn_metadata(bname, key, opts);
				} else {
					err = show_metadata(ctxt, key, opts);
				}

				if (!edit) {
					putchar(opts->endl);
				}

				show_err(err, key, val, fname, opts);

				xmlFree(key);
				xmlFree(val);
			}
		} else if (opts->metadata_fname) {
			FILE *input;

			edit = 1;

			if (strcmp(opts->metadata_fname, "-") == 0) {
				input = stdin;
			} else {
				input = fopen(opts->metadata_fname, "r");
			}

			err = edit_all_metadata(input, ctxt);

			fclose(input);
		} else if (is_icn(bname)) {
			err = show_all_icn_metadata(bname, opts);
		} else {
			err = show_all_metadata(ctxt, opts);
		}
	}

	xmlXPathFreeContext(ctxt);

	if (edit && !err) {
		if (opts->overwrite) {
			if (access(fname, W_OK) != -1) {
				save_xml_doc(doc, fname);
			} else {
				fprintf(stderr, ERR_PREFIX "%s does not have write permission.\n", fname);
				exit(EXIT_NO_WRITE);
			}
		} else {
			save_xml_doc(doc, "-");
		}
	} else if (opts->endl != '\n' && err != EXIT_CONDITION_UNMET) {
		putchar('\n');
	}

	free(s);
	xmlFreeDoc(doc);

	return err;
}

static void add_key(xmlNodePtr keys, const char *name)
{
	xmlNodePtr key;
	key = xmlNewChild(keys, NULL, BAD_CAST "key", NULL);
	xmlSetProp(key, BAD_CAST "name", BAD_CAST name);
}

static void add_val(xmlNodePtr keys, const char *val)
{
	xmlNodePtr key;
	key = keys->last;
	xmlSetProp(key, BAD_CAST "value", BAD_CAST val);
}

static void add_cond(xmlNodePtr conds, const char *k, const char *o)
{
	xmlNodePtr cond;
	cond = xmlNewChild(conds, NULL, BAD_CAST "cond", NULL);
	xmlSetProp(cond, BAD_CAST "key", BAD_CAST k);
	xmlSetProp(cond, BAD_CAST "op", BAD_CAST o);
}

static void add_cond_val(xmlNodePtr conds, const char *v, bool regex)
{
	xmlNodePtr cond;
	cond = conds->last;
	if (regex) {
		xmlSetProp(cond, BAD_CAST "regex", BAD_CAST "yes");
	}
	xmlSetProp(cond, BAD_CAST "val", BAD_CAST v);
}

static int show_or_edit_metadata_list(const char *fname, struct opts *opts)
{
	FILE *f;
	char path[PATH_MAX];
	int err = 0;

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			fprintf(stderr, ERR_PREFIX "Could not read list file '%s'.\n", fname);
			exit(EXIT_NO_FILE);
		}
	} else {
		f = stdin;
	}

	while (fgets(path, PATH_MAX, f)) {
		strtok(path, "\t\r\n");
		err += show_or_edit_metadata(path, opts);
	}

	if (fname) {
		fclose(f);
	}

	return err;
}

#ifdef LIBS1KD
xmlChar *s1kdDocGetMetadata(xmlDocPtr doc, const xmlChar *name)
{
	int i;
	xmlChar *value = NULL;
	struct opts opts;

	/* Initialize option defaults. */
	opts.conds = NULL;
	opts.endl = '\n';
	opts.execstr = NULL;
	opts.fmtstr = NULL;
	opts.format_all = 1;
	opts.keys = NULL;
	opts.metadata_fname = NULL;
	opts.only_editable = 0;
	opts.overwrite = 0;
	opts.timefmt = strdup(DEFAULT_TIMEFMT);
	opts.verbosity = NORMAL;

	for (i = 0; metadata[i].key && !value; ++i) {
		if (strcmp(metadata[i].key, name) == 0) {
			xmlXPathContextPtr ctx;
			xmlNodePtr node;

			ctx = xmlXPathNewContext(doc);

			if ((node = first_xpath_node(metadata[i].path, ctx))) {
				if (metadata[i].get) {
					value = metadata[i].get(node, &opts);
				} else {
					value = xmlNodeGetContent(node);
				}
			}

			xmlXPathFreeContext(ctx);

			break;
		}
	}

	free(opts.timefmt);

	return value;
}

char *s1kdGetMetadata(const char *object_xml, int object_size, const char *name)
{
	xmlDocPtr doc;
	char *val;

	doc = read_xml_mem(object_xml, object_size);
	val = (char *) s1kdDocGetMetadata(doc, BAD_CAST name);
	xmlFreeDoc(doc);

	return val;
}

int s1kdDocSetMetadata(xmlDocPtr doc, const xmlChar *name, const xmlChar *value)
{
	xmlXPathContextPtr ctx;
	int err;
	ctx = xmlXPathNewContext(doc);
	err = edit_metadata(ctx, (char *) name, (char *) value);
	xmlXPathFreeContext(ctx);
	return err;
}

int s1kdSetMetadata(const char *object_xml, int object_size, const char *name, const char *value, char **result_xml, int *result_size)
{
	xmlDocPtr doc;
	int err;

	doc = read_xml_mem(object_xml, object_size);
	err = s1kdDocSetMetadata(doc, BAD_CAST name, BAD_CAST value);

	if (result_xml && result_size) {
		xmlDocDumpMemory(doc, (xmlChar **) result_xml, result_size);
	}

	xmlFreeDoc(doc);

	return err;
}
#else
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -0, --null               Use null-delimited fields.");
	puts("  -c, --set <file>         Set metadata using definitions in <file> (- for stdin).");
	puts("  -d, --date-format <fmt>  Format to use for printing dates.");
	puts("  -E, --editable           Include only editable metadata when showing all.");
	puts("  -e, --exec <cmd>         Execute <cmd> for each CSDB object.");
	puts("  -F, --format <fmt>       Print a formatted line for each CSDB object.");
	puts("  -f, --overwrite          Overwrite modules when editing metadata.");
	puts("  -H, --info               List information on available metadata.");
	puts("  -l, --list               Input is a list of filenames.");
	puts("  -m, --matches <regex>    Use a pattern instead of a literal value (-v) with -w/-W.");
	puts("  -n, --name <name>        Specific metadata name to view/edit.");
	puts("  -q, --quiet              Quiet mode, do not show non-fatal errors.");
	puts("  -T, --raw                Do not format columns in output.");
	puts("  -t, --tab                Use tab-delimited fields.");
	puts("  -v, --value <value>      The value to set or match.");
	puts("  -W, --where-not <name>   Only list/edit objects where metadata <name> does not equal a value.");
	puts("  -w, --where <name>       Only list/edit objects where metadata <name> equals a value.");
	puts("  --version                Show version information.");
	puts("  <object>                 CSDB object(s) to view/edit metadata on.");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int main(int argc, char **argv)
{
	xmlNodePtr last = NULL;
	int err = 0;

	int i;
	int list_keys = 0;
	int islist = 0;
	struct opts opts;

	const char *sopts = "0d:c:Ee:F:fHlm:n:Ttv:qW:w:h?";
	struct option lopts[] = {
		{"version"    , no_argument      , 0, 0},
		{"help"       , no_argument      , 0, 'h'},
		{"null"       , no_argument      , 0, '0'},
		{"set"        , required_argument, 0, 'c'},
		{"date-format", required_argument, 0, 'd'},
		{"editable"   , no_argument      , 0, 'E'},
		{"exec"       , required_argument, 0, 'e'},
		{"format"     , required_argument, 0, 'F'},
		{"overwrite"  , no_argument      , 0, 'f'},
		{"info"       , no_argument      , 0, 'H'},
		{"list"       , no_argument      , 0, 'l'},
		{"matches"    , required_argument, 0, 'm'},
		{"name"       , required_argument, 0, 'n'},
		{"raw"        , no_argument      , 0, 'T'},
		{"tab"        , no_argument      , 0, 't'},
		{"value"      , required_argument, 0, 'v'},
		{"quiet"      , no_argument      , 0, 'q'},
		{"where"      , required_argument, 0, 'w'},
		{"where-not"  , required_argument, 0, 'W'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	/* Initialize program option defaults. */
	opts.conds = xmlNewNode(NULL, BAD_CAST "conds");
	opts.endl = '\n';
	opts.execstr = NULL;
	opts.fmtstr = NULL;
	opts.format_all = 1;
	opts.keys = xmlNewNode(NULL, BAD_CAST "keys");
	opts.metadata_fname = NULL;
	opts.only_editable = 0;
	opts.overwrite = 0;
	opts.timefmt = strdup(DEFAULT_TIMEFMT);
	opts.verbosity = NORMAL;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					goto cleanup;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg)
				break;
			case '0': opts.endl = '\0'; break;
			case 'c': opts.metadata_fname = strdup(optarg); break;
			case 'd': free(opts.timefmt); opts.timefmt = strdup(optarg); break;
			case 'E': opts.only_editable = 1; break;
			case 'e': opts.execstr = strdup(optarg); break;
			case 'F': opts.fmtstr = strdup(optarg); opts.endl = -1; break;
			case 'f': opts.overwrite = 1; break;
			case 'H': list_keys = 1; break;
			case 'l': islist = 1; break;
			case 'm':
				  if (last == opts.conds) {
					  add_cond_val(opts.conds, optarg, true);
				  }
				  break;
			case 'n': add_key(opts.keys, optarg); last = opts.keys; break;
			case 'T': opts.format_all = 0; break;
			case 't': opts.endl = '\t'; break;
			case 'v':
				if (last == opts.keys)
					add_val(opts.keys, optarg);
				else if (last == opts.conds)
					add_cond_val(opts.conds, optarg, false);
				break;
			case 'q': opts.verbosity = SILENT; break;
			case 'w': add_cond(opts.conds, optarg, "="); last = opts.conds; break;
			case 'W': add_cond(opts.conds, optarg, "~"); last = opts.conds; break;
			case 'h':
			case '?': show_help(); goto cleanup;
		}
	}

	if (list_keys) {
		list_metadata_keys(&opts);
	} else if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				err += show_or_edit_metadata_list(argv[i], &opts);
			} else {
				err += show_or_edit_metadata(argv[i], &opts);
			}
		}
	} else if (islist) {
		err = show_or_edit_metadata_list(NULL, &opts);
	} else {
		err = show_or_edit_metadata("-", &opts);
	}

cleanup:
	free(opts.metadata_fname);
	free(opts.fmtstr);
	free(opts.execstr);
	free(opts.timefmt);
	xmlFreeNode(opts.keys);
	xmlFreeNode(opts.conds);

	xmlCleanupParser();

	return err;
}
#endif
