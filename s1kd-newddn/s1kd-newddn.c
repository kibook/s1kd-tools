#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <libgen.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>

#include "templates.h"

#define PROG_NAME "s1kd-newddn"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define MODEL_IDENT_CODE_MAX	14	+ 2
#define CAGE_MAX		5	+ 2
#define YEAR_MAX		4	+ 2
#define SEQ_NO_MAX		5	+ 2

#define EXIT_DDN_EXISTS 1
#define EXIT_MALFORMED_CODE 2
#define EXIT_BAD_BREX_DMC 3
#define EXIT_BAD_DATE 4
#define EXIT_BAD_ISSUE 5

#define MAX_MODEL_IDENT_CODE		14	+ 2
#define MAX_SYSTEM_DIFF_CODE		 4	+ 2
#define MAX_SYSTEM_CODE			 3	+ 2
#define MAX_SUB_SYSTEM_CODE		 1	+ 2
#define MAX_SUB_SUB_SYSTEM_CODE		 1	+ 2
#define MAX_ASSY_CODE			 4	+ 2
#define MAX_DISASSY_CODE		 2	+ 2
#define MAX_DISASSY_CODE_VARIANT	 1	+ 2
#define MAX_INFO_CODE			 3	+ 2
#define MAX_INFO_CODE_VARIANT		 1	+ 2
#define MAX_ITEM_LOCATION_CODE		 1	+ 2
#define MAX_LEARN_CODE                   3      + 2
#define MAX_LEARN_EVENT_CODE		 1	+ 2

char model_ident_code[MODEL_IDENT_CODE_MAX] = "";
char sender_ident[CAGE_MAX] = "";
char receiver_ident[CAGE_MAX] = "";
char year_of_data_issue[YEAR_MAX] = "";
char seq_number[SEQ_NO_MAX] = "";
char sender[256] = "";
char receiver[256] = "";
char sender_city[256] = "";
char receiver_city[256] = "";
char sender_country[256] = "";
char receiver_country[256] = "";
char security_classification[4] = "";
char authorization[256] = "";

char brex_dmcode[256] = "";

char ddn_issue_date[16] = "";

#define DEFAULT_S1000D_ISSUE ISS_42
#define ISS_30_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_40_DEFAULT_BREX "S1000D-A-04-10-0301-00A-022A-D"
#define ISS_41_DEFAULT_BREX "S1000D-E-04-10-0301-00A-022A-D"

enum issue { NO_ISS, ISS_30, ISS_40, ISS_41, ISS_42 } issue = NO_ISS;

enum issue get_issue(const char *iss)
{
	if (strcmp(iss, "4.2") == 0)
		return ISS_42;
	else if (strcmp(iss, "4.1") == 0)
		return ISS_41;
	else if (strcmp(iss, "4.0") == 0)
		return ISS_40;
	else if (strcmp(iss, "3.0") == 0)
		return ISS_30;
	
	fprintf(stderr, ERR_PREFIX "Unsupported issue: %s\n", iss);
	exit(EXIT_BAD_ISSUE);

	return NO_ISS;
}

const char *issue_name(enum issue iss)
{
	switch (iss) {
		case ISS_42: return "4.2";
		case ISS_41: return "4.1";
		case ISS_40: return "4.0";
		case ISS_30: return "3.0";
		default: return "";
	}
}

xmlDocPtr toissue(xmlDocPtr doc, enum issue iss)
{
	xsltStylesheetPtr style;
	xmlDocPtr styledoc, res, orig;
	unsigned char *xml = NULL;
	unsigned int len;

	switch (iss) {
		case ISS_41:
			xml = ___common_42to41_xsl;
			len = ___common_42to41_xsl_len;
			break;
		case ISS_40:
			xml = ___common_42to40_xsl;
			len = ___common_42to40_xsl_len;
			break;
		case ISS_30:
			xml = ___common_42to30_xsl;
			len = ___common_42to30_xsl_len;
			break;
		default:
			return NULL;
	}

	orig = xmlCopyDoc(doc, 1);
			
	styledoc = xmlReadMemory((const char *) xml, len, NULL, NULL, 0);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);

	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	xmlDocSetRootElement(orig, xmlCopyNode(xmlDocGetRootElement(res), 1));

	xmlFreeDoc(res);

	return orig;
}

void prompt(const char *prompt, char *str, int n)
{
	if (strcmp(str, "") == 0) {
		printf("%s: ", prompt);

		while (!strchr(fgets(str, n, stdin), '\n')) {
			fprintf(stderr, ERR_PREFIX "Bad input for \"%s\".\n", prompt);
			while (getchar() != '\n');
			printf("%s: ", prompt);
		}

		*(strchr(str, '\n')) = '\0';
	} else {
		char temp[512] = "";

		printf("%s [%s]: ", prompt, str);

		while (!strchr(fgets(temp, n, stdin), '\n')) {
			fprintf(stderr, ERR_PREFIX "Bad input for \"%s\".\n", prompt);
			while (getchar() != '\n');
			printf("%s [%s]:  ", prompt, str);
		}

		*(strchr(temp, '\n')) = '\0';

		if (strcmp(temp, "") != 0) {
			strncpy(str, temp, n);
		}
	}
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] <files>...");
	puts("");
	puts("Options:");
	puts("  -d <defaults>    Specify the 'defaults' file name.");
	puts("  -p               Prompt user for values.");
	puts("  -v               Print file name of DDN.");
	puts("  -f               Overwrite existing file.");
	puts("");
	puts("In addition, the following metadata can be set:");
	puts("  -# <code>        The DDN code (MIC-SENDER-RECEIVER-YEAR-SEQ)");
	puts("  -o <sender>      Sender enterprise name");
	puts("  -r <receiver>    Receiver enterprise name");
	puts("  -t <city>        Sender city");
	puts("  -T <city>        Receiver city");
	puts("  -n <country>     Sender country");
	puts("  -N <country>     Receiver country");
	puts("  -a <auth>        Authorization");
	puts("  -b <BREX>        BREX data module code");
	puts("  -I <date>        Issue date");
}

int matches_key_and_not_set(const char *key, const char *match, const char *var)
{
	return strcmp(key, match) == 0 && strcmp(var, "") == 0;
}

xmlNodePtr first_xpath_node(const char *expr, xmlXPathContextPtr ctxt)
{
	xmlXPathObjectPtr results;
	xmlNodePtr ret;

	results = xmlXPathEvalExpression(BAD_CAST expr, ctxt);
	
	if (xmlXPathNodeSetIsEmpty(results->nodesetval)) {
		ret = NULL;
	} else {
		ret = results->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(results);

	return ret;
}

void populate_list(xmlNodePtr deliv, int argc, char **argv, int i)
{
	for (; i < argc; ++i) {
		xmlNodePtr item = xmlNewChild(deliv, NULL, BAD_CAST "deliveryListItem", NULL);
		xmlNewChild(item, NULL, BAD_CAST "dispatchFileName", BAD_CAST basename(argv[i]));
	}
}

void copy_default_value(const char *def_key, const char *def_val)
{
	if (matches_key_and_not_set(def_key, "modelIdentCode", model_ident_code))
		strcpy(model_ident_code, def_val);
	if (matches_key_and_not_set(def_key, "senderIdent", sender_ident))
		strcpy(sender_ident, def_val);
	if (matches_key_and_not_set(def_key, "receiverIdent", receiver_ident))
		strcpy(receiver_ident, def_val);
	if (matches_key_and_not_set(def_key, "yearOfDataIssue", year_of_data_issue))
		strcpy(year_of_data_issue, def_val);
	if (matches_key_and_not_set(def_key, "seqNumber", seq_number))
		strcpy(seq_number, def_val);
	if (matches_key_and_not_set(def_key, "originator", sender))
		strcpy(sender, def_val);
	if (matches_key_and_not_set(def_key, "receiver", receiver))
		strcpy(receiver, def_val);
	if (matches_key_and_not_set(def_key, "city", sender_city))
		strcpy(sender_city, def_val);
	if (matches_key_and_not_set(def_key, "receiverCity", receiver_city))
		strcpy(receiver_city, def_val);
	if (matches_key_and_not_set(def_key, "country", sender_country))
		strcpy(sender_country, def_val);
	if (matches_key_and_not_set(def_key, "receiverCountry", receiver_country))
		strcpy(receiver_country, def_val);
	if (matches_key_and_not_set(def_key, "securityClassification", security_classification))
		strcpy(security_classification, def_val);
	if (matches_key_and_not_set(def_key, "authorization", authorization))
		strcpy(authorization, def_val);
	if (matches_key_and_not_set(def_key, "brex", brex_dmcode))
		strcpy(brex_dmcode, def_val);
	if (strcmp(def_key, "issue") == 0 && issue == NO_ISS)
		issue = get_issue(def_val);
}

void set_brex(xmlDocPtr doc, const char *code)
{
	xmlNodePtr dmCode;
	xmlXPathContextPtr ctx;
	int n;

	char modelIdentCode[MAX_MODEL_IDENT_CODE] = "";
	char systemDiffCode[MAX_SYSTEM_DIFF_CODE] = "";
	char systemCode[MAX_SYSTEM_CODE] = "";
	char subSystemCode[MAX_SUB_SYSTEM_CODE] = "";
	char subSubSystemCode[MAX_SUB_SUB_SYSTEM_CODE] = "";
	char assyCode[MAX_ASSY_CODE] = "";
	char disassyCode[MAX_DISASSY_CODE] = "";
	char disassyCodeVariant[MAX_DISASSY_CODE_VARIANT] = "";
	char infoCode[MAX_INFO_CODE] = "";
	char infoCodeVariant[MAX_INFO_CODE_VARIANT] = "";
	char itemLocationCode[MAX_ITEM_LOCATION_CODE] = "";
	char learnCode[MAX_LEARN_CODE] = "";
	char learnEventCode[MAX_LEARN_EVENT_CODE] = "";

	ctx = xmlXPathNewContext(doc);

	dmCode = first_xpath_node("//brexDmRef/dmRef/dmRefIdent/dmCode", ctx);

	xmlXPathFreeContext(ctx);

	n = sscanf(code, "%14[^-]-%4[^-]-%3[^-]-%c%c-%4[^-]-%2s%3[^-]-%3s%c-%c-%3s%1s",
		modelIdentCode,
		systemDiffCode,
		systemCode,
		subSystemCode,
		subSubSystemCode,
		assyCode,
		disassyCode,
		disassyCodeVariant,
		infoCode,
		infoCodeVariant,
		itemLocationCode,
		learnCode,
		learnEventCode);

	if (n != 11 && n != 13) {
		fprintf(stderr, ERR_PREFIX "Bad BREX data module code.\n");
		exit(EXIT_BAD_BREX_DMC);
	}

	xmlSetProp(dmCode, BAD_CAST "modelIdentCode", BAD_CAST modelIdentCode);
	xmlSetProp(dmCode, BAD_CAST "systemDiffCode", BAD_CAST systemDiffCode);
	xmlSetProp(dmCode, BAD_CAST "systemCode", BAD_CAST systemCode);
	xmlSetProp(dmCode, BAD_CAST "subSystemCode", BAD_CAST subSystemCode);
	xmlSetProp(dmCode, BAD_CAST "subSubSystemCode", BAD_CAST subSubSystemCode);
	xmlSetProp(dmCode, BAD_CAST "assyCode", BAD_CAST assyCode);
	xmlSetProp(dmCode, BAD_CAST "disassyCode", BAD_CAST disassyCode);
	xmlSetProp(dmCode, BAD_CAST "disassyCodeVariant", BAD_CAST disassyCodeVariant);
	xmlSetProp(dmCode, BAD_CAST "infoCode", BAD_CAST infoCode);
	xmlSetProp(dmCode, BAD_CAST "infoCodeVariant", BAD_CAST infoCodeVariant);
	xmlSetProp(dmCode, BAD_CAST "itemLocationCode", BAD_CAST itemLocationCode);

	if (strcmp(learnCode, "") != 0) xmlSetProp(dmCode, BAD_CAST "learnCode", BAD_CAST learnCode);
	if (strcmp(learnEventCode, "") != 0) xmlSetProp(dmCode, BAD_CAST "learnEventCode", BAD_CAST learnEventCode);
}

void set_issue_date(xmlNodePtr issue_date)
{
	char year_s[5], month_s[3], day_s[3];

	if (strcmp(ddn_issue_date, "") == 0) {
		time_t now;
		struct tm *local;
		int year, month, day;

		time(&now);
		local = localtime(&now);
		year = local->tm_year + 1900;
		month = local->tm_mon + 1;
		day = local->tm_mday;
		sprintf(year_s, "%d", year);
		sprintf(month_s, "%.2d", month);
		sprintf(day_s, "%.2d", day);
	} else {
		if (sscanf(ddn_issue_date, "%4s-%2s-%2s", year_s, month_s, day_s) != 3) {
			fprintf(stderr, ERR_PREFIX "Bad issue date: %s\n", ddn_issue_date);
			exit(EXIT_BAD_DATE);
		}
	}

	xmlSetProp(issue_date, BAD_CAST "year", BAD_CAST year_s);
	xmlSetProp(issue_date, BAD_CAST "month", BAD_CAST month_s);
	xmlSetProp(issue_date, BAD_CAST "day", BAD_CAST day_s);
}

int main(int argc, char **argv)
{
	int c;

	FILE *defaults;
	char defaults_fname[PATH_MAX] = "defaults";

	char ddncode[256] = "";

	int showprompts = 0;
	int skipcode = 0;
	int verbose = 0;
	int overwrite = 0;

	xmlDocPtr ddn;
	xmlNodePtr ddn_code;
	xmlNodePtr issue_date;
	xmlNodePtr sender_ent_name;
	xmlNodePtr receiver_ent_name;
	xmlNodePtr send_city, recv_city;
	xmlNodePtr send_country, recv_country;
	xmlNodePtr security;
	xmlNodePtr auth;
	xmlNodePtr delivery_list;

	xmlXPathContextPtr ctxt;

	char outfile[PATH_MAX];

	xmlDocPtr defaults_xml;

	while ((c = getopt(argc, argv, "pd:#:c:o:r:t:n:T:N:a:b:I:vf$:h?")) != -1) {
		switch (c) {
			case 'p': showprompts = 1; break;
			case 'd': strncpy(defaults_fname, optarg, PATH_MAX - 1); break;
			case '#': strncpy(ddncode, optarg, 255); skipcode = 1; break;
			case 'o': strncpy(sender, optarg, 255); break;
			case 'r': strncpy(receiver, optarg, 255); break;
			case 't': strncpy(sender_city, optarg, 255); break;
			case 'n': strncpy(sender_country, optarg, 255); break;
			case 'T': strncpy(receiver_city, optarg, 255); break;
			case 'N': strncpy(receiver_country, optarg, 255); break;
			case 'a': strncpy(authorization, optarg, 255); break;
			case 'b': strncpy(brex_dmcode, optarg, 255); break;
			case 'I': strncpy(ddn_issue_date, optarg, 15); break;
			case 'v': verbose = 1; break;
			case 'f': overwrite = 1; break;
			case '$': issue = get_issue(optarg); break;
			case 'h':
			case '?': show_help(); exit(0);
		}
	}

	if ((defaults_xml = xmlReadFile(defaults_fname, NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING))) {
		xmlNodePtr cur;

		for (cur = xmlDocGetRootElement(defaults_xml)->children; cur; cur = cur->next) {
			char *def_key, *def_val;

			if (cur->type != XML_ELEMENT_NODE) continue;
			if (!xmlHasProp(cur, BAD_CAST "ident")) continue;
			if (!xmlHasProp(cur, BAD_CAST "value")) continue;

			def_key = (char *) xmlGetProp(cur, BAD_CAST "ident");
			def_val = (char *) xmlGetProp(cur, BAD_CAST "value");

			copy_default_value(def_key, def_val);

			xmlFree(def_key);
			xmlFree(def_val);
		}

		xmlFreeDoc(defaults_xml);
	} else {
		defaults = fopen(defaults_fname, "r");
		if (defaults) {
			char default_line[1024];

			while (fgets(default_line, 1024, defaults)) {
				char def_key[32], def_val[256];

				if (sscanf(default_line, "%s %[^\n]", def_key, def_val) != 2)
					continue;

				copy_default_value(def_key, def_val);
			}

			fclose(defaults);
		}
	}

	if (strcmp(ddncode, "") != 0) {
		int n;

		n = sscanf(ddncode, "%[^-]-%[^-]-%[^-]-%[^-]-%[^-]",
			model_ident_code,
			sender_ident,
			receiver_ident,
			year_of_data_issue,
			seq_number);

		if (n != 5) {
			fprintf(stderr, ERR_PREFIX "Bad DDN code.\n");
			exit(EXIT_MALFORMED_CODE);
		}
	}

	if (showprompts) {
		if (!skipcode) {
			prompt("Model identification code", model_ident_code, MODEL_IDENT_CODE_MAX);
			prompt("Sender ident", sender_ident, CAGE_MAX);
			prompt("Receiver ident", receiver_ident, CAGE_MAX);
			prompt("Year of data issue", year_of_data_issue, YEAR_MAX);
			prompt("Sequence number", seq_number, SEQ_NO_MAX);
		}
		prompt("Sender enterprise name", sender, 256);
		prompt("Sender city", sender_city, 256);
		prompt("Sender country", sender_country, 256);
		prompt("Receiver enterprise name", receiver, 256);
		prompt("Receiver city", receiver_city, 256);
		prompt("Receiver country", receiver_country, 256);
		prompt("Security classification", security_classification, 4);
		prompt("Authorization", authorization, 256);
	}

	if (issue == NO_ISS) issue = DEFAULT_S1000D_ISSUE;
	if (strcmp(security_classification, "") == 0) strcpy(security_classification, "01");

	ddn = xmlReadMemory((const char *) templates_ddn_xml, templates_ddn_xml_len, NULL, NULL, 0);

	ctxt = xmlXPathNewContext(ddn);

	ddn_code = first_xpath_node("//ddnIdent/ddnCode", ctxt);
	xmlSetProp(ddn_code, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
	xmlSetProp(ddn_code, BAD_CAST "senderIdent", BAD_CAST sender_ident);
	xmlSetProp(ddn_code, BAD_CAST "receiverIdent", BAD_CAST receiver_ident);
	xmlSetProp(ddn_code, BAD_CAST "yearOfDataIssue", BAD_CAST year_of_data_issue);
	xmlSetProp(ddn_code, BAD_CAST "seqNumber", BAD_CAST seq_number);

	issue_date = first_xpath_node("//ddnAddressItems/issueDate", ctxt);
	set_issue_date(issue_date);

	sender_ent_name = first_xpath_node("//ddnAddressItems/dispatchFrom/dispatchAddress/enterprise/enterpriseName", ctxt);
	xmlNodeSetContent(sender_ent_name, BAD_CAST sender);

	receiver_ent_name = first_xpath_node("//ddnAddressItems/dispatchTo/dispatchAddress/enterprise/enterpriseName", ctxt);
	xmlNodeSetContent(receiver_ent_name, BAD_CAST receiver);

	send_city = first_xpath_node("//ddnAddressItems/dispatchFrom/dispatchAddress/address/city", ctxt);
	xmlNodeSetContent(send_city, BAD_CAST sender_city);

	recv_city = first_xpath_node("//ddnAddressItems/dispatchTo/dispatchAddress/address/city", ctxt);
	xmlNodeSetContent(recv_city, BAD_CAST receiver_city);

	send_country = first_xpath_node("//ddnAddressItems/dispatchFrom/dispatchAddress/address/country", ctxt);
	xmlNodeSetContent(send_country, BAD_CAST sender_country);

	recv_country = first_xpath_node("//ddnAddressItems/dispatchTo/dispatchAddress/address/country", ctxt);
	xmlNodeSetContent(recv_country, BAD_CAST receiver_country);

	security = first_xpath_node("//ddnStatus/security", ctxt);
	xmlSetProp(security, BAD_CAST "securityClassification", BAD_CAST security_classification);

	auth = first_xpath_node("//ddnStatus/authorization", ctxt);
	xmlNodeSetContent(auth, BAD_CAST authorization);

	if (strcmp(brex_dmcode, "") != 0)
		set_brex(ddn, brex_dmcode);

	delivery_list = first_xpath_node("//ddnContent/deliveryList", ctxt);

	xmlXPathFreeContext(ctxt);

	populate_list(delivery_list, argc, argv, optind);

	if (issue < ISS_42) {
		if (strcmp(brex_dmcode, "") == 0) {
			switch (issue) {
				case ISS_30:
					set_brex(ddn, ISS_30_DEFAULT_BREX);
					break;
				case ISS_40:
					set_brex(ddn, ISS_40_DEFAULT_BREX);
					break;
				case ISS_41:
					set_brex(ddn, ISS_41_DEFAULT_BREX);
					break;
				default:
					break;
			}
		}

		ddn = toissue(ddn, issue);
	}

	snprintf(outfile, PATH_MAX, "DDN-%s-%s-%s-%s-%s.XML",
		model_ident_code,
		sender_ident,
		receiver_ident,
		year_of_data_issue,
		seq_number);

	if (!overwrite && access(outfile, F_OK) != -1) {
		fprintf(stderr, ERR_PREFIX "%s already exists.\n", outfile);
		exit(EXIT_DDN_EXISTS);
	}

	xmlSaveFile(outfile, ddn);

	if (verbose)
		puts(outfile);

	xmlFreeDoc(ddn);

	return 0;
}
