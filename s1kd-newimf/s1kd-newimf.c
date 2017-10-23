#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "template.h"

#define PROG_NAME "s1kd-newimf"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_IMF_EXISTS 1
#define EXIT_BAD_BREX_DMC 2
#define EXIT_BAD_DATE 3

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

char issue_number[5] = "";
char in_work[4] = "";
char security_classification[4] = "";
char responsible_partner_company[256] = "";
char responsible_partner_company_code[7] = "";
char originator[256] = "";
char originator_code[7] = "";
char icn_title[256] = "";

char brex_dmcode[256] = "";

char issue_date[16] = "";

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

void copy_def_val(char *dst, const char *target, const char *key, const char *val)
{
	if (strcmp(target, key) == 0 && strcmp(dst, "") == 0) {
		strcpy(dst, val);
	}
}

xmlNodePtr first_xpath_node(const char *xpath, xmlXPathContextPtr ctx)
{
	xmlXPathObjectPtr obj;
	xmlNodePtr result;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		result = NULL;
	} else {
		result = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);

	return result;
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] <icns>...");
	puts("");
	puts("Options:");
	puts("  -p          Show prompts.");
	puts("  -d <path>   Defaults file path.");
	puts("  -N          Omit issue/inwork numbers from filename.");
	puts("  -v          Print file name of IMF.");
	puts("  -f          Overwrite existing file.");
	puts("  <icns>      1 or more ICNs to generate a metadata file for.");
	puts("");
	puts("In addition, the following metadata can be set:");
	puts("  -n          Issue number");
	puts("  -w          Inwork issue");
	puts("  -c          Security classification");
	puts("  -r          Responsible partner company");
	puts("  -R          Responsible partner company CAGE code");
	puts("  -o          Originator");
	puts("  -O          Originator CAGE code");
	puts("  -t          ICN title");
	puts("  -b          BREX data module code");
	puts("  -I          Issue date");
}

void copy_default_value(const char *def_key, const char *def_val)
{
	copy_def_val(issue_number, "issueNumber", def_key, def_val);
	copy_def_val(in_work, "inWork", def_key, def_val);
	copy_def_val(security_classification, "securityClassification", def_key, def_val);
	copy_def_val(responsible_partner_company, "responsiblePartnerCompany", def_key, def_val);
	copy_def_val(responsible_partner_company_code, "responsiblePartnerCompanyCode", def_key, def_val);
	copy_def_val(originator, "originator", def_key, def_val);
	copy_def_val(originator_code, "originatorCode", def_key, def_val);
	copy_def_val(brex_dmcode, "brex", def_key, def_val);
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

void set_issue_date(xmlNodePtr issueDate)
{
	char year_s[5], month_s[3], day_s[3];

	if (strcmp(issue_date, "") == 0) {
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
		if (sscanf(issue_date, "%4s-%2s-%2s", year_s, month_s, day_s) != 3) {
			fprintf(stderr, ERR_PREFIX "Bad issue date: %s\n", issue_date);
			exit(EXIT_BAD_DATE);
		}
	}

	xmlSetProp(issueDate, BAD_CAST "year",  BAD_CAST year_s);
	xmlSetProp(issueDate, BAD_CAST "month", BAD_CAST month_s);
	xmlSetProp(issueDate, BAD_CAST "day",   BAD_CAST day_s);
}

int main(int argc, char **argv)
{
	int i;

	bool show_prompts = false;
	bool no_issue = false;
	bool verbose = false;
	bool overwrite = false;

	FILE *defaults;
	char defaults_fname[PATH_MAX] = "defaults";

	xmlDocPtr defaults_xml;

	while ((i = getopt(argc, argv, "pd:n:w:c:r:R:o:O:Nt:b:I:vfh?")) != -1) {
		switch (i) {
			case 'p': show_prompts = true; break;
			case 'd': strncpy(defaults_fname, optarg, PATH_MAX - 1); break;
			case 'n': strncpy(issue_number, optarg, 3); break;
			case 'w': strncpy(in_work, optarg, 2); break;
			case 'c': strncpy(security_classification, optarg, 2); break;
			case 'r': strncpy(responsible_partner_company, optarg, 255); break;
			case 'R': strncpy(responsible_partner_company_code, optarg, 5); break;
			case 'o': strncpy(originator, optarg, 255); break;
			case 'O': strncpy(originator_code, optarg, 5); break;
			case 'N': no_issue = true; break;
			case 't': strncpy(icn_title, optarg, 255); break;
			case 'b': strncpy(brex_dmcode, optarg, 255); break;
			case 'I': strncpy(issue_date, optarg, 15); break;
			case 'v': verbose = true; break;
			case 'f': overwrite = true; break;
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
	} else if ((defaults = fopen(defaults_fname, "r"))) {
		char defaults_line[1024];

		while (fgets(defaults_line, 1024, defaults)) {
			char def_key[32], def_val[256];

			if (sscanf(defaults_line, "%s %[^\n]", def_key, def_val) != 2)
				continue;

			copy_default_value(def_key, def_val);
		}

		fclose(defaults);
	}

	if (show_prompts) {
		prompt("Issue number", issue_number, 5);
		prompt("In-work issue", in_work, 4);
		prompt("Security classification", security_classification, 4);
		prompt("Responsible partner company", responsible_partner_company, 256);
		prompt("Originator", originator, 256);
		prompt("ICN title", icn_title, 256);
	}

	if (strcmp(issue_number, "") == 0) strcpy(issue_number, "000");
	if (strcmp(in_work, "") == 0) strcpy(in_work, "01");
	if (strcmp(security_classification, "") == 0) strcpy(security_classification, "01");

	for (i = optind; i < argc; ++i) {
		int n;
		char icn[256];
		char fname[256];
		xmlDocPtr template;
		xmlNodePtr node;
		xmlXPathContextPtr ctx;

		n = sscanf(argv[i], "ICN-%[^.].%*s", icn);

		if (n != 1) continue;

		template = xmlReadMemory((const char *) icnmetadata_xml, icnmetadata_xml_len, NULL, NULL, 0);

		ctx = xmlXPathNewContext(template);

		node = first_xpath_node("//imfIdent/imfCode", ctx);
		xmlSetProp(node, BAD_CAST "imfIdentIcn", BAD_CAST icn);

		node = first_xpath_node("//imfIdent/issueInfo", ctx);
		xmlSetProp(node, BAD_CAST "issueNumber", BAD_CAST issue_number);
		xmlSetProp(node, BAD_CAST "inWork", BAD_CAST in_work);

		node = first_xpath_node("//imfAddressItems/icnTitle", ctx);
		xmlNodeSetContent(node, BAD_CAST icn_title);

		node = first_xpath_node("//imfAddressItems/issueDate", ctx);
		set_issue_date(node);

		node = first_xpath_node("//imfStatus/security", ctx);
		xmlSetProp(node, BAD_CAST "securityClassification", BAD_CAST security_classification);

		if (strcmp(responsible_partner_company_code, "") != 0) {
			node = first_xpath_node("//imfStatus/responsiblePartnerCompany", ctx);
			xmlSetProp(node, BAD_CAST "enterpriseCode", BAD_CAST responsible_partner_company_code);
		}

		if (strcmp(responsible_partner_company, "") != 0) {
			node = first_xpath_node("//imfStatus/responsiblePartnerCompany", ctx);
			xmlNewChild(node, NULL, BAD_CAST "enterpriseName", BAD_CAST responsible_partner_company);
		}
		
		if (strcmp(originator_code, "") != 0) {
			node = first_xpath_node("//imfStatus/originator", ctx);
			xmlSetProp(node, BAD_CAST "enterpriseCode", BAD_CAST originator_code);
		}

		if (strcmp(originator, "") != 0) {
			node = first_xpath_node("//imfStatus/originator", ctx);
			xmlNewChild(node, NULL, BAD_CAST "enterpriseName", BAD_CAST originator);
		}

		if (strcmp(brex_dmcode, "") != 0)
			set_brex(template, brex_dmcode);

		if (no_issue) {
			snprintf(fname, 256, "IMF-%s.XML", icn);
		} else {
			snprintf(fname, 256, "IMF-%s_%s-%s.XML", icn, issue_number, in_work);
		}

		if (!overwrite && access(fname, F_OK) != -1) {
			fprintf(stderr, ERR_PREFIX "%s already exists.\n", fname);
			exit(EXIT_IMF_EXISTS);
		}

		xmlSaveFile(fname, template);

		if (verbose)
			puts(fname);

		xmlFreeDoc(template);
	}

	return 0;
}
