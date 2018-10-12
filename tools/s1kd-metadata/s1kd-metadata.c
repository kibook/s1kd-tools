#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-metadata"
#define VERSION "1.3.1"

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

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

enum verbosity {SILENT, NORMAL} verbosity = NORMAL;

struct metadata {
	char *key;
	char *path;
	void (*show)(xmlNodePtr, int endl);
	int (*edit)(xmlNodePtr, const char *);
	int (*create)(xmlXPathContextPtr, const char *val);
	char *descr;
};

xmlNodePtr first_xpath_node(char *expr, xmlXPathContextPtr ctxt)
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

xmlNodePtr first_xpath_node_local(xmlNodePtr node, const char *expr)
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

char *first_xpath_string(xmlNodePtr node, const char *expr)
{
	xmlNodePtr first;
	first = first_xpath_node_local(node, expr);
	return (char *) xmlNodeGetContent(first);
}

void show_issue_date(xmlNodePtr issue_date, int endl)
{
	char *year, *month, *day;

	year  = (char *) xmlGetProp(issue_date, BAD_CAST "year");
	month = (char *) xmlGetProp(issue_date, BAD_CAST "month");
	day   = (char *) xmlGetProp(issue_date, BAD_CAST "day");

	printf("%s-%s-%s", year, month, day);
	if (endl > -1) putchar(endl);

	xmlFree(year);
	xmlFree(month);
	xmlFree(day);
}

int edit_issue_date(xmlNodePtr issue_date, const char *val)
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

void show_simple_node(xmlNodePtr node, int endl)
{
	char *content = (char *) xmlNodeGetContent(node);
	printf("%s", content);
	if (endl > -1) putchar(endl);
	xmlFree(content);
}

int edit_simple_node(xmlNodePtr node, const char *val)
{
	xmlNodeSetContent(node, BAD_CAST val);
	return 0;
}

int create_info_name(xmlXPathContextPtr ctxt, const char *val)
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

void show_simple_attr(xmlNodePtr node, const char *attr, int endl)
{
	char *text = (char *) xmlGetProp(node, BAD_CAST attr);
	printf("%s", text);
	if (endl > -1) putchar(endl);
	xmlFree(text);
}

int edit_simple_attr(xmlNodePtr node, const char *attr, const char *val)
{
	xmlSetProp(node, BAD_CAST attr, BAD_CAST val);
	return 0;
}

void show_ent_code(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "orig") == 0 || xmlStrcmp(node->name, BAD_CAST "rpc") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "enterpriseCode", endl);
	}
}

int edit_ent_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "orig") == 0 || xmlStrcmp(node->name, BAD_CAST "rpc") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "enterpriseCode", val);
	}
}

int create_rpc_ent_code(xmlXPathContextPtr ctxt, const char *val)
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

int create_orig_ent_code(xmlXPathContextPtr ctxt, const char *val)
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

void show_sec_class(xmlNodePtr node, int endl)
{
	if (xmlHasProp(node, BAD_CAST "securityClassification")) {
		show_simple_attr(node, "securityClassification", endl);
	} else {
		show_simple_attr(node, "class", endl);
	}
}

int edit_sec_class(xmlNodePtr node, const char *val)
{
	if (xmlHasProp(node, BAD_CAST "securityClassification")) {
		return edit_simple_attr(node, "securityClassification", val);
	} else {
		return edit_simple_attr(node, "class", val);
	}
}

void show_schema(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "noNamespaceSchemaLocation", endl);
}

int edit_schema(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "xsi:noNamespaceSchemaLocation", val);
}

int edit_info_name(xmlNodePtr node, const char *val)
{
	if (strcmp(val, "") == 0) {
		xmlUnlinkNode(node);
		xmlFreeNode(node);
		return 0;
	} else {
		return edit_simple_node(node, val);
	}
}

void show_type(xmlNodePtr node, int endl)
{
	printf("%s", node->name);
	if (endl > -1) putchar(endl);
}

void show_dmcode(xmlNodePtr node, int endl)
{
	char *model_ident_code;
	char *system_diff_code;
	char *system_code;
	char *sub_system_code;
	char *sub_sub_system_code;
	char *assy_code;
	char *disassy_code;
	char *disassy_code_variant;
	char *info_code;
	char *info_code_variant;
	char *item_location_code;
	char *learn_code;
	char *learn_event_code;
	char learn[6] = "";

	if (xmlStrcmp(node->name, BAD_CAST "dmCode") == 0) {
		model_ident_code      = (char *) xmlGetProp(node, BAD_CAST "modelIdentCode");
		system_diff_code      = (char *) xmlGetProp(node, BAD_CAST "systemDiffCode");
		system_code           = (char *) xmlGetProp(node, BAD_CAST "systemCode");
		sub_system_code       = (char *) xmlGetProp(node, BAD_CAST "subSystemCode");
		sub_sub_system_code   = (char *) xmlGetProp(node, BAD_CAST "subSubSystemCode");
		assy_code             = (char *) xmlGetProp(node, BAD_CAST "assyCode");
		disassy_code          = (char *) xmlGetProp(node, BAD_CAST "disassyCode");
		disassy_code_variant  = (char *) xmlGetProp(node, BAD_CAST "disassyCodeVariant");
		info_code             = (char *) xmlGetProp(node, BAD_CAST "infoCode");
		info_code_variant     = (char *) xmlGetProp(node, BAD_CAST "infoCodeVariant");
		item_location_code    = (char *) xmlGetProp(node, BAD_CAST "itemLocationCode");
		learn_code            = (char *) xmlGetProp(node, BAD_CAST "learnCode");
		learn_event_code      = (char *) xmlGetProp(node, BAD_CAST "learnEventCode");

		if (learn_code && learn_event_code) sprintf(learn, "-%s%s", learn_code, learn_event_code);
	} else {
		model_ident_code     = first_xpath_string(node, "modelic");
		system_diff_code     = first_xpath_string(node, "sdc");
		system_code          = first_xpath_string(node, "chapnum");
		sub_system_code      = first_xpath_string(node, "section");
		sub_sub_system_code  = first_xpath_string(node, "subsect");
		assy_code            = first_xpath_string(node, "subject");
		disassy_code         = first_xpath_string(node, "discode");
		disassy_code_variant = first_xpath_string(node, "discodev");
		info_code            = first_xpath_string(node, "incode");
		info_code_variant    = first_xpath_string(node, "incodev");
		item_location_code   = first_xpath_string(node, "itemloc");
		learn_code = NULL;
		learn_event_code = NULL;
	}

	printf("%s-%s-%s-%s%s-%s-%s%s-%s%s-%s%s",
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
	if (endl > -1) putchar(endl);

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
}

int edit_dmcode(xmlNodePtr node, const char *val)
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
		edit_simple_node(first_xpath_node_local(node, "modelic"), model_ident_code);
		edit_simple_node(first_xpath_node_local(node, "sdc"), system_diff_code);
		edit_simple_node(first_xpath_node_local(node, "chapnum"), system_code);
		edit_simple_node(first_xpath_node_local(node, "section"), sub_system_code);
		edit_simple_node(first_xpath_node_local(node, "subsect"), sub_sub_system_code);
		edit_simple_node(first_xpath_node_local(node, "subject"), assy_code);
		edit_simple_node(first_xpath_node_local(node, "discode"), disassy_code);
		edit_simple_node(first_xpath_node_local(node, "discodev"), disassy_code_variant);
		edit_simple_node(first_xpath_node_local(node, "incode"), info_code);
		edit_simple_node(first_xpath_node_local(node, "incodev"), info_code_variant);
		edit_simple_node(first_xpath_node_local(node, "itemloc"), item_location_code);
	}

	return 0;
}

void show_ddncode(xmlNodePtr node, int endl)
{
	char *modelic, *sendid, *recvid, *diyear, *seqnum;

	modelic = first_xpath_string(node, "@modelIdentCode|modelic");
	sendid  = first_xpath_string(node, "@senderIdent|sendid");
	recvid  = first_xpath_string(node, "@receiverIdent|recvid");
	diyear  = first_xpath_string(node, "@yearOfDataIssue|diyear");
	seqnum  = first_xpath_string(node, "@seqNumber|seqnum");

	printf("%s-%s-%s-%s-%s",
		modelic,
		sendid,
		recvid,
		diyear,
		seqnum);
	if (endl > -1) putchar(endl);

	xmlFree(modelic);
	xmlFree(sendid);
	xmlFree(recvid);
	xmlFree(diyear);
	xmlFree(seqnum);
}

void show_dmlcode(xmlNodePtr node, int endl)
{
	char *modelic, *sendid, *dmltype, *diyear, *seqnum;

	modelic = first_xpath_string(node, "@modelIdentCode|modelic");
	sendid  = first_xpath_string(node, "@senderIdent|sendid");
	dmltype = first_xpath_string(node, "@dmlType|dmltype/@type");
	diyear  = first_xpath_string(node, "@yearOfDataIssue|diyear");
	seqnum  = first_xpath_string(node, "@seqNumber|seqnum");

	printf("%s-%s-%s-%s-%s",
		modelic,
		sendid,
		dmltype,
		diyear,
		seqnum);
	if (endl > -1) putchar(endl);

	xmlFree(modelic);
	xmlFree(sendid);
	xmlFree(dmltype);
	xmlFree(diyear);
	xmlFree(seqnum);
}

void show_pmcode(xmlNodePtr node, int endl)
{
	char *modelic, *pmissuer, *pmnumber, *pmvolume;

	modelic  = first_xpath_string(node, "@modelIdentCode|modelic");
	pmissuer = first_xpath_string(node, "@pmIssuer|pmissuer");
	pmnumber = first_xpath_string(node, "@pmNumber|pmnumber");
	pmvolume = first_xpath_string(node, "@pmVolume|pmvolume");

	printf("%s-%s-%s-%s",
		modelic,
		pmissuer,
		pmnumber,
		pmvolume);
	if (endl > -1) putchar(endl);

	xmlFree(modelic);
	xmlFree(pmissuer);
	xmlFree(pmnumber);
	xmlFree(pmvolume);
}

void show_pm_issuer(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmissuer") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "pmIssuer", endl);
	}
}

int edit_pm_issuer(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmissuer") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "pmIssuer", val);
	}
}

void show_pm_number(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmnumber") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "pmNumber", endl);
	}
}

int edit_pm_number(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmnumber") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "pmNumber", val);
	}
}

void show_pm_volume(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmvolume") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "pmVolume", endl);
	}
}

int edit_pm_volume(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "pmvolume") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "pmVolume", val);
	}
}

void show_comment_code(xmlNodePtr node, int endl)
{
	char *model_ident_code;
	char *sender_ident;
	char *year_of_data_issue;
	char *seq_number;
	char *comment_type;

	if (xmlStrcmp(node->name, BAD_CAST "commentCode") == 0) {
		model_ident_code   = (char *) xmlGetProp(node, BAD_CAST "modelIdentCode");
		sender_ident       = (char *) xmlGetProp(node, BAD_CAST "senderIdent");
		year_of_data_issue = (char *) xmlGetProp(node, BAD_CAST "yearOfDataIssue");
		seq_number         = (char *) xmlGetProp(node, BAD_CAST "seqNumber");
		comment_type       = (char *) xmlGetProp(node, BAD_CAST "commentType");
	} else {
		model_ident_code   = first_xpath_string(node, "modelic");
		sender_ident       = first_xpath_string(node, "sendid");
		year_of_data_issue = first_xpath_string(node, "diyear");
		seq_number         = first_xpath_string(node, "seqnum");
		comment_type       = first_xpath_string(node, "ctype/@type");
	}

	printf("%s-%s-%s-%s-%s",
		model_ident_code,
		sender_ident,
		year_of_data_issue,
		seq_number,
		comment_type);
	if (endl > -1) putchar(endl);

	xmlFree(model_ident_code);
	xmlFree(sender_ident);
	xmlFree(year_of_data_issue);
	xmlFree(seq_number);
	xmlFree(comment_type);
}

void show_code(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "dmCode") == 0 || xmlStrcmp(node->name, BAD_CAST "avee") == 0) {
		show_dmcode(node, endl);
	} else if (xmlStrcmp(node->name, BAD_CAST "pmCode") == 0 || xmlStrcmp(node->name, BAD_CAST "pmc") == 0) {
		show_pmcode(node, endl);
	} else if (xmlStrcmp(node->name, BAD_CAST "commentCode") == 0 || xmlStrcmp(node->name, BAD_CAST "ccode") == 0) {
		show_comment_code(node, endl);
	} else if (xmlStrcmp(node->name, BAD_CAST "ddnCode") == 0 || xmlStrcmp(node->name, BAD_CAST "ddnc") == 0) {
		show_ddncode(node, endl);
	} else if (xmlStrcmp(node->name, BAD_CAST "dmlCode") == 0 || xmlStrcmp(node->name, BAD_CAST "dmlc") == 0) {
		show_dmlcode(node, endl);
	}
}

void show_issue_type(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "issno") == 0) {
		show_simple_attr(node, "type", endl);
	} else {
		show_simple_attr(node, "issueType", endl);
	}
}

int edit_issue_type(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "issno") == 0) {
		return edit_simple_attr(node, "type", val);
	} else {
		return edit_simple_attr(node, "issueType", val);
	}
}

void show_language_iso_code(xmlNodePtr node, int endl)
{
	if (xmlHasProp(node, BAD_CAST "languageIsoCode")) {
		show_simple_attr(node, "languageIsoCode", endl);
	} else {
		show_simple_attr(node, "language", endl);
	}
}

int edit_language_iso_code(xmlNodePtr node, const char *val)
{
	if (xmlHasProp(node, BAD_CAST "languageIsoCode")) {
		return edit_simple_attr(node, "languageIsoCode", val);
	} else {
		return edit_simple_attr(node, "language", val);
	}
}

void show_country_iso_code(xmlNodePtr node, int endl)
{
	if (xmlHasProp(node, BAD_CAST "countryIsoCode")) {
		show_simple_attr(node, "countryIsoCode", endl);
	} else {
		show_simple_attr(node, "country", endl);
	}
}

int edit_country_iso_code(xmlNodePtr node, const char *val)
{
	if (xmlHasProp(node, BAD_CAST "countryIsoCode")) {
		return edit_simple_attr(node, "countryIsoCode", val);
	} else {
		return edit_simple_attr(node, "country", val);
	}
}

void show_issue_number(xmlNodePtr node, int endl)
{
	if (xmlHasProp(node, BAD_CAST "issueNumber")) {
		show_simple_attr(node, "issueNumber", endl);
	} else {
		show_simple_attr(node, "issno", endl);
	}
}

int edit_issue_number(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "issueNumber", val);
}

void show_in_work(xmlNodePtr node, int endl)
{
	if (xmlHasProp(node, BAD_CAST "inWork")) {
		show_simple_attr(node, "inWork", endl);
	} else if (xmlHasProp(node, BAD_CAST "inwork")) {
		show_simple_attr(node, "inwork", endl);
	} else {
		printf("00");
		if (endl > -1) putchar(endl);
	}
}

int edit_in_work(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "inWork", val);
}

int create_act_ref(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;

	node = first_xpath_node("//dmStatus/originator", ctxt);

	node = xmlAddNextSibling(node, xmlNewNode(NULL, BAD_CAST "applicCrossRefTableRef"));
	node = xmlNewChild(node, NULL, BAD_CAST "dmRef", NULL);
	node = xmlNewChild(node, NULL, BAD_CAST "dmRefIdent", NULL);
	node = xmlNewChild(node, NULL, BAD_CAST "dmCode", NULL);

	return edit_dmcode(node, val);
}

int create_comment_title(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;

	node = first_xpath_node("//commentAddressItems/issueDate", ctxt);
	
	if (!node) return EXIT_INVALID_CREATE;

	node = xmlAddNextSibling(node, xmlNewNode(NULL, BAD_CAST "commentTitle"));

	return edit_simple_node(node, val);
}

void show_comment_priority(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "priority") == 0) {
		show_simple_attr(node, "cprio", endl);
	} else {
		show_simple_attr(node, "commentPriorityCode", endl);
	}
}

int edit_comment_priority(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "priority") == 0) {
		return edit_simple_attr(node, "cprio", val);
	} else {
		return edit_simple_attr(node, "commentPriorityCode", val);
	}
}

void show_comment_response(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "response") == 0) {
		show_simple_attr(node, "rsptype", endl);
	} else {
		show_simple_attr(node, "responseType", endl);
	}
}

int edit_comment_response(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "response") == 0) {
		return edit_simple_attr(node, "rsptype", val);
	} else {
		return edit_simple_attr(node, "responseType", val);
	}
}

int create_ent_name(xmlNodePtr node, const char *val)
{
	return xmlNewChild(node, NULL, BAD_CAST "enterpriseName", BAD_CAST val) == NULL;
}

int create_rpc_name(xmlXPathContextPtr ctxt, const char *val)
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

int create_orig_name(xmlXPathContextPtr ctxt, const char *val)
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

void show_url(xmlNodePtr node, int endl)
{
	printf("%s", node->doc->URL);
	if (endl > -1) putchar(endl);
}

void show_title(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "dmTitle") == 0 || xmlStrcmp(node->name, BAD_CAST "dmtitle") == 0) {
		xmlNodePtr tech, info;
		xmlChar *tech_content;
		tech = first_xpath_node_local(node, "techName|techname");
		info = first_xpath_node_local(node, "infoName|infoname");
		tech_content = xmlNodeGetContent(tech);
		printf("%s", (char *) tech_content);
		xmlFree(tech_content);
		if (info) {
			xmlChar *info_content;
			info_content = xmlNodeGetContent(info);
			printf(" - %s", info_content);
			xmlFree(info_content);
		}
		if (endl > -1) putchar(endl);
	} else {
		show_simple_node(node, endl);
	}
}

void show_model_ident_code(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "modelic") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "modelIdentCode", endl);
	}
}

int edit_model_ident_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "modelic") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "modelIdentCode", val);
	}
}

void show_system_diff_code(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "sdc") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "systemDiffCode", endl);
	}
}

int edit_system_diff_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "sdc") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "systemDiffCode", val);
	}
}

void show_system_code(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "chapnum") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "systemCode", endl);
	}
}

int edit_system_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "chapnum") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "systemCode", val);
	}
}

void show_sub_system_code(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "section") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "subSystemCode", endl);
	}
}

int edit_sub_system_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "section") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "subSystemCode", val);
	}
}

void show_sub_sub_system_code(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "subsect") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "subSubSystemCode", endl);
	}
}

int edit_sub_sub_system_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "subsect") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "subSubSystemCode", val);
	}
}

void show_assy_code(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "subject") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "assyCode", endl);
	}
}

int edit_assy_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "subject") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "assyCode", val);
	}
}

void show_disassy_code(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "discode") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "disassyCode", endl);
	}
}

int edit_disassy_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "discode") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "disassyCode", val);
	}
}

void show_disassy_code_variant(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "discodev") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "disassyCodeVariant", endl);
	}
}

int edit_disassy_code_variant(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "discodev") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "disassyCodeVariant", val);
	}
}

void show_info_code(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "incode") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "infoCode", endl);
	}
}

int edit_info_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "incode") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "infoCode", val);
	}
}

void show_info_code_variant(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "incodev") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "infoCodeVariant", endl);
	}
}

int edit_info_code_variant(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "incodev") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "infoCodeVariant", val);
	}
}

void show_item_location_code(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "itemloc") == 0) {
		show_simple_node(node, endl);
	} else {
		show_simple_attr(node, "itemLocationCode", endl);
	}
}

int edit_item_location_code(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "itemloc") == 0) {
		return edit_simple_node(node, val);
	} else {
		return edit_simple_attr(node, "itemLocationCode", val);
	}
}

void show_learn_code(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "learnCode", endl);
}

int edit_learn_code(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "learnCode", val);
}

void show_learn_event_code(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "learnEventCode", endl);
}

int edit_learn_event_code(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "learnEventCode", val);
}

void show_skill_level(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "skill") == 0) {
		show_simple_attr(node, "skill", endl);
	} else {
		show_simple_attr(node, "skillLevelCode", endl);
	}
}

int edit_skill_level(xmlNodePtr node, const char *val)
{
	if (xmlStrcmp(node->name, BAD_CAST "skill") == 0) {
		return edit_simple_attr(node, "skill", val);
	} else {
		return edit_simple_attr(node, "skillLevelCode", val);
	}
}

int create_skill_level(xmlXPathContextPtr ctx, const char *val)
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

struct metadata metadata[] = {
	{"act",
		"//applicCrossRefTableRef/dmRef/dmRefIdent/dmCode",
		show_dmcode,
		edit_dmcode,
		create_act_ref,
		"ACT data module code"},
	{"applic",
		"//applic/displayText/simplePara|//applic/displaytext/p",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Whole data module applicability"},
	{"assyCode",
		"//@assyCode|//avee/subject",
		show_assy_code,
		edit_assy_code,
		NULL,
		"Assembly code"},
	{"authorization",
		"//authorization|//authrtn",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Authorization for a DDN"},
	{"brex",
		"//brexDmRef/dmRef/dmRefIdent/dmCode|//brexref/refdm/avee",
		show_dmcode,
		edit_dmcode,
		NULL,
		"BREX data module code"},
	{"code",
		"//dmCode|//avee|//pmCode|//pmc|//commentCode|//ccode|//ddnCode|//ddnc|//dmlCode|//dmlc",
		show_code,
		NULL,
		NULL,
		"CSDB object code"},
	{"commentCode",
		"//commentCode|//ccode",
		show_comment_code,
		NULL,
		NULL,
		"Comment code"},
	{"commentPriority",
		"//commentPriority/@commentPriorityCode|//priority/@cprio",
		show_comment_priority,
		edit_comment_priority,
		NULL,
		"Priority code of a comment"},
	{"commentResponse",
		"//commentResponse/@responseType|//response/@rsptype",
		show_comment_response,
		edit_comment_response,
		NULL,
		"Response type of a comment"},
	{"commentTitle",
		"//commentTitle|//ctitle",
		show_simple_node,
		edit_simple_node,
		create_comment_title,
		"Title of a comment"},
	{"countryIsoCode",
		"//language/@countryIsoCode|//language/@country",
		show_country_iso_code,
		edit_country_iso_code,
		NULL,
		"Country ISO code (CA, US, GB...)"},
	{"ddnCode",
		"//ddnCode|//ddnc",
		show_ddncode,
		NULL,
		NULL,
		"Data dispatch note code"},
	{"disassyCode",
		"//@disassyCode|//discode",
		show_disassy_code,
		edit_disassy_code,
		NULL,
		"Disassembly code"},
	{"disassyCodeVariant",
		"//@disassyCodeVariant|//discodev",
		show_disassy_code_variant,
		edit_disassy_code_variant,
		NULL,
		"Disassembly code variant"},
	{"dmCode",
		"//dmCode|//avee",
		show_dmcode,
		NULL,
		NULL,
		"Data module code"},
	{"dmlCode",
		"//dmlCode|//dmlc",
		show_dmlcode,
		NULL,
		NULL,
		"Data management list code"},
	{"url",
		"/",
		show_url,
		NULL,
		NULL,
		"URL of the document"},
	{"icnTitle",
		"//imfAddressItems/icnTitle",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Title of an IMF"},
	{"infoCode",
		"//@infoCode|//incode",
		show_info_code,
		edit_info_code,
		NULL,
		"Information code"},
	{"infoCodeVariant",
		"//@infoCodeVariant|//incodev",
		show_info_code_variant,
		edit_info_code_variant,
		NULL,
		"Information code variant"},
	{"infoName",
		"//infoName|//infoname",
		show_simple_node,
		edit_info_name,
		create_info_name,
		"Information name of a data module"},
	{"inWork",
		"//issueInfo/@inWork|//issno",
		show_in_work,
		edit_in_work,
		NULL,
		"Inwork issue number (NN)"},
	{"issueDate",
	 	"//issueDate|//issdate",
		show_issue_date,
		edit_issue_date,
		NULL,
		"Issue date in ISO 8601 format (YYYY-MM-DD)"},
	{"issueNumber",
		"//issueInfo/@issueNumber|//issno/@issno",
		show_issue_number,
		edit_issue_number,
		NULL,
		"Issue number (NNN)"},
	{"issueType",
		"//dmStatus/@issueType|//pmStatus/@issueType|//issno/@type",
		show_issue_type,
		edit_issue_type,
		NULL,
		"Issue type (new, changed, deleted...)"},
	{"itemLocationCode",
		"//@itemLocationCode|//itemloc",
		show_item_location_code,
		edit_item_location_code,
		NULL,
		"Item location code"},
	{"languageIsoCode",
		"//language/@languageIsoCode|//language/@language",
		show_language_iso_code,
		edit_language_iso_code,
		NULL,
		"Language ISO code (en, fr, es...)"},
	{"learnCode",
		"//@learnCode",
		show_learn_code,
		edit_learn_code,
		NULL,
		"Learn code"},
	{"learnEventCode",
		"//@learnEventCode",
		show_learn_event_code,
		edit_learn_event_code,
		NULL,
		"Learn event code"},
	{"modelIdentCode",
		"//@modelIdentCode|//modelic",
		show_model_ident_code,
		edit_model_ident_code,
		NULL,
		"Model identification code"},
	{"originator",
		"//originator/enterpriseName|//orig/@origname",
		show_simple_node,
		edit_simple_node,
		create_orig_name,
		"Name of the originator"},
	{"originatorCode",
		"//originator/@enterpriseCode|//orig[. != '']",
		show_ent_code,
		edit_ent_code,
		create_orig_ent_code,
		"NCAGE code of the originator"},
	{"path",
		"false()",
		NULL,
		NULL,
		NULL,
		"Filesystem path of object"},
	{"pmCode",
		"//pmCode|//pmc",
		show_pmcode,
		NULL,
		NULL,
		"Publication module code"},
	{"pmIssuer",
		"//@pmIssuer|//pmissuer",
		show_pm_issuer,
		edit_pm_issuer,
		NULL,
		"Issuing authority of the PM"},
	{"pmNumber",
		"//@pmNumber|//pmnumber",
		show_pm_number,
		edit_pm_number,
		NULL,
		"PM number"},
	{"pmTitle",
		"//pmTitle|//pmtitle",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Title of a publication module"},
	{"pmVolume",
		"//@pmVolume|//pmvolume",
		show_pm_volume,
		edit_pm_volume,
		NULL,
		"Volume of the PM"},
	{"responsiblePartnerCompany",
		"//responsiblePartnerCompany/enterpriseName|//rpc/@rpcname",
		show_simple_node,
		edit_simple_node,
		create_rpc_name,
		"Name of the RPC"},
	{"responsiblePartnerCompanyCode",
		"//responsiblePartnerCompany/@enterpriseCode|//rpc[. != '']",
		show_ent_code,
		edit_ent_code,
		create_rpc_ent_code,
		"NCAGE code of the RPC"},
	{"schema",
		"/*",
		show_schema,
		edit_schema,
		NULL,
		"XML schema URI"},
	{"securityClassification",
		"//security/@securityClassification|//security/@class",
		show_sec_class,
		edit_sec_class,
		NULL,
		"Security classification (01, 02...)"},
	{"shortPmTitle",
		"//shortPmTitle",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Short title of a publication module"},
	{"skillLevelCode",
		"//dmStatus/skillLevel/@skillLevelCode|//status/skill/@skill",
		show_skill_level,
		edit_skill_level,
		create_skill_level,
		"Skill level code of the data module"},
	{"subSubSystemCode",
		"//@subSubSystemCode|//subsect",
		show_sub_sub_system_code,
		edit_sub_sub_system_code,
		NULL,
		"Subsubsystem code"},
	{"subSystemCode",
		"//@subSystemCode|//section",
		show_sub_system_code,
		edit_sub_system_code,
		NULL,
		"Subsystem code"},
	{"systemCode",
		"//@systemCode|//chapnum",
		show_system_code,
		edit_system_code,
		NULL,
		"System code"},
	{"systemDiffCode",
		"//@systemDiffCode|//sdc",
		show_system_diff_code,
		edit_system_diff_code,
		NULL,
		"System difference code"},
	{"techName",
		"//techName|//techname",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Technical name of a data module"},
	{"title",
		"//dmTitle|//dmtitle|//pmTitle|//pmtitle|//commentTitle|//ctitle|//icnTitle",
		show_title,
		NULL,
		NULL,
		"Title of a CSDB object"},
	{"type",
		"/*",
		show_type,
		NULL,
		NULL,
		"Name of the root element of the document"},
	{NULL}
};

int show_metadata(xmlXPathContextPtr ctxt, const char *key, int endl)
{
	int i;

	for (i = 0; metadata[i].key; ++i) {
		if (strcmp(key, metadata[i].key) == 0) {
			xmlNodePtr node;
			if (!(node = first_xpath_node(metadata[i].path, ctxt))) {
				putchar(endl);
				return EXIT_MISSING_METADATA;
			}
			if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;
			metadata[i].show(node, endl);
			return EXIT_SUCCESS;
		}
	}

	return EXIT_INVALID_METADATA;
}

int edit_metadata(xmlXPathContextPtr ctxt, const char *key, const char *val)
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

int show_all_metadata(xmlXPathContextPtr ctxt, int formatall, int endl, int only_editable)
{
	int i;

	for (i = 0; metadata[i].key; ++i) {
		xmlNodePtr node;

		if (only_editable && !metadata[i].edit) continue;

		if ((node = first_xpath_node(metadata[i].path, ctxt))) {
			if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;

			if (endl == '\n') {
				printf("%s", metadata[i].key);

				if (formatall) {
					int n = KEY_COLUMN_WIDTH - strlen(metadata[i].key);
					int j;
					for (j = 0; j < n; ++j) putchar(' ');
				} else {
					putchar('\t');
				}
			}

			metadata[i].show(node, endl);
		}
	}

	return 0;
}

int edit_all_metadata(FILE *input, xmlXPathContextPtr ctxt)
{
	char key[256], val[256];

	while (fscanf(input, "%255s %255[^\n]", key, val) == 2) {
		edit_metadata(ctxt, key, val);
	}

	return 0;
}

void list_metadata_key(const char *key, const char *descr, int formatall)
{
	int n = KEY_COLUMN_WIDTH - strlen(key);
	printf("%s", key);
	if (formatall) {
		int j;
		for (j = 0; j < n; ++j) putchar(' ');
	} else {
		putchar('\t');
	}
	printf("%s", descr);
	putchar('\n');
}

int has_key(xmlNodePtr keys, const char *key)
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

void list_metadata_keys(xmlNodePtr keys, int formatall, int only_editable)
{
	int i;
	for (i = 0; metadata[i].key; ++i) {
		if (has_key(keys, metadata[i].key) && (!only_editable || metadata[i].edit)) {
			list_metadata_key(metadata[i].key, metadata[i].descr, formatall);
		}
	}
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -0           Use null-delimited fields.");
	puts("  -c <file>    Set metadata using definitions in <file> (- for stdin).");
	puts("  -e           Include only editable metadata when showing all.");
	puts("  -f           Overwrite modules when editing metadata.");
	puts("  -H           List information on available metadata.");
	puts("  -l           Input is a list of filenames.");
	puts("  -n <name>    Specific metadata name to view/edit.");
	puts("  -q           Quiet mode, do not show non-fatal errors.");
	puts("  -T           Do not format columns in output.");
	puts("  -t           Use tab-delimited fields.");
	puts("  -v <value>   The value to set or match.");
	puts("  -W <name>    Only list/edit when metadata <name> does not equal a value.");
	puts("  -w <name>    Only list/edit when metadata <name> equals a value.");
	puts("  --version    Show version information.");
	puts("  <module>     S1000D module to view/edit metadata on.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int show_err(int err, const char *key, const char *val, const char *fname)
{
	if (verbosity < NORMAL) return err;

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

int show_path(const char *fname, int endl)
{
	printf("%s", fname);
	if (endl > -1) putchar(endl);
	return 0;
}

int show_metadata_fmtstr_key(xmlXPathContextPtr ctx, const char *k, int n)
{
	int i;
	char *key;

	key = malloc(n + 1);
	sprintf(key, "%.*s", n, k);

	for (i = 0; metadata[i].key; ++i) {
		if (strcmp(metadata[i].key, key) == 0) {
			xmlNodePtr node;
			if (!(node = first_xpath_node(metadata[i].path, ctx))) {
				show_err(EXIT_MISSING_METADATA, key, NULL, NULL);
				free(key);
				return EXIT_MISSING_METADATA;
			}
			if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;
			metadata[i].show(node, -1);
			free(key);
			return EXIT_SUCCESS;
		}
	}

	show_err(EXIT_INVALID_METADATA, key, NULL, NULL);
	free(key);
	return EXIT_INVALID_METADATA;
}

int show_metadata_fmtstr(const char *fname, xmlXPathContextPtr ctx, const char *fmt)
{
	int i;
	for (i = 0; fmt[i]; ++i) {
		if (fmt[i] == FMTSTR_DELIM) {
			if (fmt[i + 1] == FMTSTR_DELIM) {
				putchar(FMTSTR_DELIM);
				++i;
			} else {
				const char *k, *e;
				int n;
				k = fmt + i + 1;
				e = strchr(k, FMTSTR_DELIM);
				if (!e) break;
				n = e - k;

				if (strncmp(k, "path", n) == 0) {
					show_path(fname, -1);
				} else {
					show_metadata_fmtstr_key(ctx, k, n);
				}

				i += n + 1;
			}
		} else if (fmt[i] == '\\') {
			switch (fmt[i + 1]) {
				case 'n': putchar('\n'); ++i; break;
				case 't': putchar('\t'); ++i; break;
				case '0': putchar('\0'); ++i; break;
				default: putchar(fmt[i]);
			}
		} else {
			putchar(fmt[i]);
		}
	}
	return 0;
}

int condition_met(xmlXPathContextPtr ctx, xmlNodePtr cond)
{
	xmlChar *key, *val, *op;
	int i, cmp = 0;

	key = xmlGetProp(cond, BAD_CAST "key");
	val = xmlGetProp(cond, BAD_CAST "val");
	op = xmlGetProp(cond, BAD_CAST "op");

	for (i = 0; metadata[i].key; ++i) {
		if (xmlStrcmp(key, BAD_CAST metadata[i].key) == 0) {
			xmlNodePtr node;
			xmlChar *content;

			node = first_xpath_node(metadata[i].path, ctx);
			content = xmlNodeGetContent(node);

			switch (op[0]) {
				case '=': cmp = xmlStrcmp(content, val) == 0; break;
				case '~': cmp = xmlStrcmp(content, val) != 0; break;
				default: break;
			}

			xmlFree(content);
			break;
		}
	}

	return cmp;
}

int show_or_edit_metadata(const char *fname, const char *metadata_fname,
	xmlNodePtr keys, int formatall, int overwrite, int endl,
	int only_editable, const char *fmtstr, xmlNodePtr conds)
{
	int err = 0;
	xmlDocPtr doc;
	xmlXPathContextPtr ctxt;
	int edit = 0;
	xmlNodePtr cond;

	doc = xmlReadFile(fname, NULL, PARSE_OPTS | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);

	ctxt = xmlXPathNewContext(doc);

	for (cond = conds->children; cond; cond = cond->next) {
		if (!condition_met(ctxt, cond)) {
			err = EXIT_CONDITION_UNMET;
		}
	}

	if (!err) {
		if (fmtstr) {
			err = show_metadata_fmtstr(fname, ctxt, fmtstr);
		} else if (keys->children) {
			xmlNodePtr cur;
			for (cur = keys->children; cur; cur = cur->next) {
				char *key = NULL, *val = NULL;

				key = (char *) xmlGetProp(cur, BAD_CAST "name");
				val = (char *) xmlGetProp(cur, BAD_CAST "value");

				if (val) {
					edit = 1;
					err = edit_metadata(ctxt, key, val);
				} else if (strcmp(key, "path") == 0) {
					err = show_path(fname, endl);
				} else {
					err = show_metadata(ctxt, key, endl);
				}

				show_err(err, key, val, fname);

				xmlFree(key);
				xmlFree(val);
			}
		} else if (metadata_fname) {
				FILE *input;

				edit = 1;

				if (strcmp(metadata_fname, "-") == 0) {
					input = stdin;
				} else {
					input = fopen(metadata_fname, "r");
				}

				err = edit_all_metadata(input, ctxt);

				fclose(input);
		} else {
				err = show_all_metadata(ctxt, formatall, endl, only_editable);
		}
	}

	xmlXPathFreeContext(ctxt);

	if (edit && !err) {
		if (overwrite) {
			if (access(fname, W_OK) != -1) {
				xmlSaveFile(fname, doc);
			} else {
				fprintf(stderr, ERR_PREFIX "%s does not have write permission.\n", fname);
				exit(EXIT_NO_WRITE);
			}
		} else {
			xmlSaveFile("-", doc);
		}
	} else if (endl != '\n') {
		putchar('\n');
	}

	xmlFreeDoc(doc);

	return err;
}

void add_key(xmlNodePtr keys, const char *name)
{
	xmlNodePtr key;
	key = xmlNewChild(keys, NULL, BAD_CAST "key", NULL);
	xmlSetProp(key, BAD_CAST "name", BAD_CAST name);
}

void add_val(xmlNodePtr keys, const char *val)
{
	xmlNodePtr key;
	key = keys->last;
	xmlSetProp(key, BAD_CAST "value", BAD_CAST val);
}

void add_cond(xmlNodePtr conds, const char *k, const char *o)
{
	xmlNodePtr cond;
	cond = xmlNewChild(conds, NULL, BAD_CAST "cond", NULL);
	xmlSetProp(cond, BAD_CAST "key", BAD_CAST k);
	xmlSetProp(cond, BAD_CAST "op", BAD_CAST o);
}

void add_cond_val(xmlNodePtr conds, const char *v)
{
	xmlNodePtr cond;
	cond = conds->last;
	xmlSetProp(cond, BAD_CAST "val", BAD_CAST v);
}

int show_or_edit_metadata_list(const char *fname, const char *metadata_fname,
	xmlNodePtr keys, int formatall, int overwrite, int endl,
	int only_editable, const char *fmtstr, xmlNodePtr conds)
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
		err += show_or_edit_metadata(path, metadata_fname, keys,
			formatall, overwrite, endl, only_editable, fmtstr, conds);
	}

	if (fname) {
		fclose(f);
	}

	return err;
}

int main(int argc, char **argv)
{
	xmlNodePtr keys, conds, last = NULL;
	int err = 0;

	int i;
	char *metadata_fname = NULL;
	int formatall = 1;
	int overwrite = 0;
	int endl = '\n';
	int list_keys = 0;
	int islist = 0;
	int only_editable = 0;
	char *fmtstr = NULL;

	const char *sopts = "0c:eF:fHln:Ttv:qW:w:h?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	keys = xmlNewNode(NULL, BAD_CAST "keys");
	conds = xmlNewNode(NULL, BAD_CAST "conds");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case '0': endl = '\0'; break;
			case 'c': metadata_fname = strdup(optarg); break;
			case 'e': only_editable = 1; break;
			case 'F': fmtstr = strdup(optarg); endl = -1; break;
			case 'f': overwrite = 1; break;
			case 'H': list_keys = 1; break;
			case 'l': islist = 1; break;
			case 'n': add_key(keys, optarg); last = keys; break;
			case 'T': formatall = 0; break;
			case 't': endl = '\t'; break;
			case 'v':
				if (last == keys)
					add_val(keys, optarg);
				else if (last == conds)
					add_cond_val(conds, optarg);
				break;
			case 'q': verbosity = SILENT; break;
			case 'w': add_cond(conds, optarg, "="); last = conds; break;
			case 'W': add_cond(conds, optarg, "~"); last = conds; break;
			case 'h':
			case '?': show_help(); return 0;
		}
	}

	if (list_keys) {
		list_metadata_keys(keys, formatall, only_editable);
	} else if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				err += show_or_edit_metadata_list(argv[i],
					metadata_fname, keys, formatall,
					overwrite, endl, only_editable, fmtstr,
					conds);
			} else {
				err += show_or_edit_metadata(argv[i],
					metadata_fname, keys, formatall,
					overwrite, endl, only_editable, fmtstr,
					conds);
			}
		}
	} else if (islist) {
		err = show_or_edit_metadata_list(NULL, metadata_fname, keys, formatall,
			overwrite, endl, only_editable, fmtstr, conds);
	} else {
		err = show_or_edit_metadata("-", metadata_fname, keys, formatall,
			overwrite, endl, only_editable, fmtstr, conds);
	}

	free(metadata_fname);
	free(fmtstr);
	xmlFreeNode(keys);
	xmlFreeNode(conds);

	xmlCleanupParser();

	return err;
}
