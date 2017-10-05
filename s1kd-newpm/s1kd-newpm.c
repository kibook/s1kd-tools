#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "template.h"

#define ERR_PREFIX "s1kd-newpm: ERROR: "

#define EXIT_BAD_PMC 1
#define EXIT_PM_EXISTS 2
#define EXIT_BAD_BREX_DMC 3

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

char model_ident_code[16] = "";
char pm_issuer[7] = "";
char pm_number[7] = "";
char pm_volume[4] = "";
char language_iso_code[5] = "";
char country_iso_code[4] = "";
char issue_number[5] = "";
char in_work[4] = "";
char pm_title[256] = "";
char security_classification[4] = "";
char enterprise_name[256] = "";
char enterprise_code[7] = "";

char brex_dmcode[256] = "";

xmlNodePtr find_child(xmlNodePtr parent, char *name)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, name) == 0) {
			return cur;
		}
	}

	exit(1);
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

xmlNodePtr first_xpath_node(const char *xpath, xmlXPathContextPtr ctx)
{
	xmlXPathObjectPtr obj;
	xmlNodePtr node;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval))
		node = NULL;
	else
		node = obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);

	return node;
}

void add_dm_ref(xmlNodePtr pmEntry, char *path, bool include_issue_info, bool include_language)
{
	xmlNodePtr ident_extension, dm_code, issue_info, language;
	xmlNodePtr dm_ref, dm_ref_ident;

	xmlDocPtr dmodule;
	xmlXPathContextPtr ctx;

	if (access(path, F_OK) == -1) {
		fprintf(stderr, ERR_PREFIX "Could not find referenced data module '%s'.\n", path);
		return;
	}

	dmodule = xmlReadFile(path, NULL, 0);
	ctx = xmlXPathNewContext(dmodule);

	ident_extension = first_xpath_node("//dmIdent/identExtension", ctx);
	dm_code = first_xpath_node("//dmIdent/dmCode", ctx);
	issue_info = first_xpath_node("//dmIdent/issueInfo", ctx);
	language = first_xpath_node("//dmIdent/language", ctx);

	dm_ref = xmlNewNode(NULL, BAD_CAST "dmRef");
	dm_ref_ident = xmlNewChild(dm_ref, NULL, BAD_CAST "dmRefIdent", NULL);
	
	if (ident_extension) {
		xmlAddChild(dm_ref_ident, xmlCopyNode(ident_extension, 1));
	}

	xmlAddChild(dm_ref_ident, xmlCopyNode(dm_code, 1));

	if (include_issue_info) {
		xmlAddChild(dm_ref_ident, xmlCopyNode(issue_info, 1));
	}

	if (include_language) {
		xmlAddChild(dm_ref_ident, xmlCopyNode(language, 1));
	}

	xmlAddChild(pmEntry, dm_ref);
}

void show_help(void)
{
	puts("Usage: s1kd-newpm [options]");
	puts("");
	puts("Options:");
	puts("  -d    Specify the 'defaults' file name.");
	puts("  -p    Prompt the user for each value.");
	puts("  -N    Omit issue/inwork from file name.");
	puts("  -v    Print file name of pub module.");
	puts("  -f    Overwrite existing file.");
	puts("");
	puts("In addition, the following pieces of meta data can be set:");
	puts("  -#    Publication module code");
	puts("  -L    Language ISO code");
	puts("  -C    Country ISO code");
	puts("  -n    Issue number");
	puts("  -w    Inwork issue");
	puts("  -c    Security classification");
	puts("  -r    Responsible partner company enterprise name");
	puts("  -t    Publication module title");
	puts("  -b    BREX data module code");
}

void copy_default_value(const char *key, const char *val)
{
	if (strcmp(key, "modelIdentCode") == 0 && strcmp(model_ident_code, "") == 0)
		strcpy(model_ident_code, val);
	else if (strcmp(key, "pmIssuer") == 0 && strcmp(pm_issuer, "") == 0)
		strcpy(pm_issuer, val);
	else if (strcmp(key, "pmNumber") == 0 && strcmp(pm_number, "") == 0)
		strcpy(pm_number, val);
	else if (strcmp(key, "pmVolume") == 0 && strcmp(pm_volume, "") == 0)
		strcpy(pm_volume, val);
	else if (strcmp(key, "languageIsoCode") == 0 && strcmp(language_iso_code, "") == 0)
		strcpy(language_iso_code, val);
	else if (strcmp(key, "countryIsoCode") == 0 && strcmp(country_iso_code, "") == 0)
		strcpy(country_iso_code, val);
	else if (strcmp(key, "securityClassification") == 0 && strcmp(security_classification, "") == 0)
		strcpy(security_classification, val);
	else if (strcmp(key, "responsiblePartnerCompany") == 0 && strcmp(enterprise_name, "") == 0)
		strcpy(enterprise_name, val);
	else if (strcmp(key, "responsiblePartnerCompanyCode") == 0 && strcmp(enterprise_code, "") == 0)
		strcpy(enterprise_code, val);
	else if (strcmp(key, "issueNumber") == 0 && strcmp(issue_number, "") == 0)
		strcpy(issue_number, val);
	else if (strcmp(key, "inWork") == 0 && strcmp(in_work, "") == 0)
		strcpy(in_work, val);
	else if (strcmp(key, "brex") == 0 && strcmp(brex_dmcode, "") == 0)
		strcpy(brex_dmcode, val);
}

xmlNodePtr firstXPathNode(xmlDocPtr doc, const char *xpath)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval))
		node = NULL;
	else
		node = obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return node;
}

void set_brex(xmlDocPtr doc, const char *code)
{
	xmlNodePtr dmCode;
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

	dmCode = firstXPathNode(doc, "//brexDmRef/dmRef/dmRefIdent/dmCode");

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

int main(int argc, char **argv)
{
	xmlDocPtr pm_doc;

	xmlNodePtr pm;
	xmlNodePtr identAndStatusSection;
	xmlNodePtr pmAddress;
	xmlNodePtr pmIdent;
	xmlNodePtr pmCode;
	xmlNodePtr language;
	xmlNodePtr issueInfo;
	xmlNodePtr pmAddressItems;
	xmlNodePtr issueDate;
	xmlNodePtr pmTitle;
	xmlNodePtr pmStatus;
	xmlNodePtr security;
	xmlNodePtr responsiblePartnerCompany;
	xmlNodePtr pmEntry;

	char pm_filename[256];

	int c;
	int i;

	time_t now;
	struct tm *local;
	int year, month, day;
	char year_s[5], month_s[3], day_s[3];
	char pmcode[256] = "";
	bool showprompts = false;
	bool skippmc = false;
	char defaults_fname[256] = "defaults";
	bool no_issue = false;
	char iss[8] = "";
	bool include_issue_info = false;
	bool include_language = false;
	bool verbose = false;
	bool overwrite = false;
	xmlDocPtr defaults_xml;

	while ((c = getopt(argc, argv, "pd:#:L:C:n:w:c:r:R:t:Nilb:vfh?")) != -1) {
		switch (c) {
			case 'p': showprompts = true; break;
			case 'd': strcpy(defaults_fname, optarg); break;
			case '#': strcpy(pmcode, optarg); skippmc = true; break;
			case 'L': strcpy(language_iso_code, optarg); break;
			case 'C': strcpy(country_iso_code, optarg); break;
			case 'n': strcpy(issue_number, optarg); break;
			case 'w': strcpy(in_work, optarg); break;
			case 'c': strcpy(security_classification, optarg); break;
			case 'r': strcpy(enterprise_name, optarg); break;
			case 'R': strcpy(enterprise_code, optarg); break;
			case 't': strcpy(pm_title, optarg); break;
			case 'N': no_issue = true; break;
			case 'i': include_issue_info = true; break;
			case 'l': include_language = true; break;
			case 'b': strcpy(brex_dmcode, optarg); break;
			case 'v': verbose = true; break;
			case 'f': overwrite = true; break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	defaults_xml = xmlReadFile(defaults_fname, NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);

	if (defaults_xml) {
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
		FILE *defaults;

		defaults = fopen(defaults_fname, "r");

		if (defaults) {
			char default_line[1024];
			while (fgets(default_line, 1024, defaults)) {
				char *def_key;
				char *def_val;

				def_key = strtok(default_line, "\t ");
				def_val = strtok(NULL, "\t\n");

				copy_default_value(def_key, def_val);
			}

			fclose(defaults);
		}
	}

	pm_doc = xmlReadMemory((const char *) pm_xml, pm_xml_len, NULL, NULL, 0);

	if (strcmp(pmcode, "") != 0) {
		int n;

		n = sscanf(pmcode, "%14[^-]-%5s-%5s-%2s",
			model_ident_code,
			pm_issuer,
			pm_number,
			pm_volume);

		if (n != 4) {
			fprintf(stderr, ERR_PREFIX "Bad pub module code.\n");
			exit(EXIT_BAD_PMC);
		}
	}

	if (showprompts) {
		if (!skippmc) {
			prompt("Model ident code", model_ident_code, 16);
			prompt("PM Issuer", pm_issuer, 7);
			prompt("PM Number", pm_number, 7);
			prompt("PM Volume", pm_volume, 4);
		}
		prompt("Language ISO code", language_iso_code, 5);
		prompt("Country ISO code", country_iso_code, 4);
		prompt("Issue number", issue_number, 5);
		prompt("In work", in_work, 4);
		prompt("PM Title", pm_title, 256);
		prompt("Security classification", security_classification, 4);
		prompt("Responsible partner company", enterprise_name, 256);
	}

	if (strcmp(issue_number, "") == 0) strcpy(issue_number, "000");
	if (strcmp(in_work, "") == 0) strcpy(in_work, "01");
	if (strcmp(language_iso_code, "") == 0) strcpy(language_iso_code, "und");
	if (strcmp(country_iso_code, "") == 0) strcpy(country_iso_code, "ZZ");
	if (strcmp(security_classification, "") == 0) strcpy(security_classification, "01");

	for (i = 0; language_iso_code[i]; ++i) {
		language_iso_code[i] = tolower(language_iso_code[i]);
	}

	pm = xmlDocGetRootElement(pm_doc);
	identAndStatusSection = find_child(pm, "identAndStatusSection");
	pmAddress = find_child(identAndStatusSection, "pmAddress");
	pmIdent = find_child(pmAddress, "pmIdent");
	pmCode = find_child(pmIdent, "pmCode");
	language = find_child(pmIdent, "language");
	issueInfo = find_child(pmIdent, "issueInfo");
	pmAddressItems = find_child(pmAddress, "pmAddressItems");
	issueDate = find_child(pmAddressItems, "issueDate");
	pmTitle = find_child(pmAddressItems, "pmTitle");
	pmStatus = find_child(identAndStatusSection, "pmStatus");
	security = find_child(pmStatus, "security");
	responsiblePartnerCompany = find_child(pmStatus, "responsiblePartnerCompany");
	pmEntry = find_child(find_child(pm, "content"), "pmEntry");

	xmlSetProp(pmCode, (xmlChar *) "modelIdentCode", (xmlChar *) model_ident_code);
	xmlSetProp(pmCode, (xmlChar *) "pmIssuer",       (xmlChar *) pm_issuer);
	xmlSetProp(pmCode, (xmlChar *) "pmNumber",       (xmlChar *) pm_number);
	xmlSetProp(pmCode, (xmlChar *) "pmVolume",       (xmlChar *) pm_volume);

	xmlSetProp(language, (xmlChar *) "languageIsoCode", (xmlChar *) language_iso_code);
	xmlSetProp(language, (xmlChar *) "countryIsoCode",  (xmlChar *) country_iso_code);

	xmlSetProp(issueInfo, (xmlChar *) "issueNumber", (xmlChar *) issue_number);
	xmlSetProp(issueInfo, (xmlChar *) "inWork",      (xmlChar *) in_work);

	time(&now);
	local = localtime(&now);
	year = local->tm_year + 1900;
	month = local->tm_mon + 1;
	day = local->tm_mday;
	sprintf(year_s, "%d", year);
	sprintf(month_s, "%.2d", month);
	sprintf(day_s, "%.2d", day);
	xmlSetProp(issueDate, (xmlChar *) "year", (xmlChar *) year_s);
	xmlSetProp(issueDate, (xmlChar *) "month", (xmlChar *) month_s);
	xmlSetProp(issueDate, (xmlChar *) "day", (xmlChar *) day_s);

	xmlNodeSetContent(pmTitle, (xmlChar *) pm_title);

	xmlSetProp(security, (xmlChar *) "securityClassification", (xmlChar *) security_classification);

	if (strcmp(enterprise_name, "") != 0)
		xmlNewChild(responsiblePartnerCompany, NULL, BAD_CAST "enterpriseName", BAD_CAST enterprise_name);

	if (strcmp(enterprise_code, "") != 0)
		xmlSetProp(responsiblePartnerCompany, BAD_CAST "enterpriseCode", BAD_CAST enterprise_code);

	if (strcmp(brex_dmcode, "") != 0)
		set_brex(pm_doc, brex_dmcode);

	for (i = optind; i < argc; ++i) {
		add_dm_ref(pmEntry, argv[i], include_issue_info, include_language);
	}

	for (i = 0; language_iso_code[i]; ++i) {
		language_iso_code[i] = toupper(language_iso_code[i]);
	}

	if (!no_issue) {
		sprintf(iss, "_%s-%s", issue_number, in_work);
	}

	snprintf(pm_filename, 256, "PMC-%s-%s-%s-%s%s_%s-%s.XML",
		model_ident_code,
		pm_issuer,
		pm_number,
		pm_volume,
		iss,
		language_iso_code,
		country_iso_code);

	if (!overwrite && access(pm_filename, F_OK) != -1) {
		fprintf(stderr, ERR_PREFIX "Pub module %s already exists.\n", pm_filename);
		exit(EXIT_PM_EXISTS);
	}

	xmlSaveFormatFile(pm_filename, pm_doc, 1);

	if (verbose)
		puts(pm_filename);

	xmlFreeDoc(pm_doc);

	return 0;
}
