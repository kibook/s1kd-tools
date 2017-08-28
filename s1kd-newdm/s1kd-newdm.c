#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

#include <libxml/tree.h>

#include "templates.h"

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
#define MAX_LANGUAGE_ISO_CODE		 3	+ 2
#define MAX_COUNTRY_ISO_CODE		 2	+ 2
#define MAX_ISSUE_NUMBER		 3	+ 2
#define MAX_IN_WORK			 2	+ 2
#define MAX_SECURITY_CLASSIFICATION	 2	+ 2

#define MAX_DATAMODULE_CODE 256

#define MAX_ENTERPRISE_NAME 256
#define MAX_ENTERPRISE_CODE 7

#define MAX_TECH_NAME 256
#define MAX_INFO_NAME 256

#define ERR_PREFIX "s1kd-newdm: ERROR: "

#define EXIT_DM_EXISTS 1
#define EXIT_UNKNOWN_DMTYPE 2
#define EXIT_BAD_DMC 3

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

char languageIsoCode[MAX_LANGUAGE_ISO_CODE] = "";
char countryIsoCode[MAX_COUNTRY_ISO_CODE] = "";

char securityClassification[MAX_SECURITY_CLASSIFICATION] = "";

char issueNumber[MAX_ISSUE_NUMBER] = "";
char inWork[MAX_IN_WORK] = "";

char responsiblePartnerCompany_enterpriseName[MAX_ENTERPRISE_NAME] = "";
char originator_enterpriseName[MAX_ENTERPRISE_NAME] = "";

char responsiblePartnerCompany_enterpriseCode[MAX_ENTERPRISE_CODE] = "";
char originator_enterpriseCode[MAX_ENTERPRISE_CODE] = "";

char techName_content[MAX_TECH_NAME] = "";
char infoName_content[MAX_INFO_NAME] = "";

char schema[1024] = "";

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

xmlNodePtr find_child(xmlNodePtr parent, const char *name)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, name) == 0) {
			return cur;
		}
	}

	return NULL;
}

void show_help(void)
{
	puts("Usage: s1kd-newdm [options]");
	puts("");
	puts("Options:");
	puts("  -p	Prompt the user for each value");
	puts("");
	puts("In addition, the following pieces of meta data can be set:");
	puts("  -#	Data module code");
	puts("  -L	Language ISO code");
	puts("  -C	Country ISO code");
	puts("  -n	Issue number");
	puts("  -w	Inwork issue");
	puts("  -c	Security classification");
	puts("  -r	Responsible partner company enterprise name");
	puts("  -R	Responsible partner company CAGE code.");
	puts("  -o	Originator enterprise name");
	puts("  -O	Originator CAGE code.");
	puts("  -t	Tech name");
	puts("  -i	Info name");
	puts("  -T	DM type (descript, proced, frontmatter, brex, brdoc)");
	puts("  -N	Omit issue/inwork from filename.");
}

void copy_default_value(const char *key, const char *val)
{
	if (strcmp(key, "modelIdentCode") == 0 && strcmp(modelIdentCode, "") == 0)
		strcpy(modelIdentCode, val);
	else if (strcmp(key, "systemDiffCode") == 0 && strcmp(systemDiffCode, "") == 0)
		strcpy(systemDiffCode, val);
	else if (strcmp(key, "systemCode") == 0 && strcmp(systemCode, "") == 0)
		strcpy(systemCode, val);
	else if (strcmp(key, "subSystemCode") == 0 && strcmp(subSystemCode, "") == 0)
		strcpy(subSystemCode, val);
	else if (strcmp(key, "subSubSystemCode") == 0 && strcmp(subSubSystemCode, "") == 0)
		strcpy(subSubSystemCode, val);
	else if (strcmp(key, "assyCode") == 0 && strcmp(assyCode, "") == 0)
		strcpy(assyCode, val);
	else if (strcmp(key, "disassyCode") == 0 && strcmp(disassyCode, "") == 0)
		strcpy(disassyCode, val);
	else if (strcmp(key, "disassyCodeVariant") == 0 && strcmp(disassyCodeVariant, "") == 0)
		strcpy(disassyCodeVariant, val);
	else if (strcmp(key, "infoCode") == 0 && strcmp(infoCode, "") == 0)
		strcpy(infoCode, val);
	else if (strcmp(key, "infoCodeVariant") == 0 && strcmp(infoCodeVariant, "") == 0)
		strcpy(infoCodeVariant, val);
	else if (strcmp(key, "itemLocationCode") == 0 && strcmp(itemLocationCode, "") == 0)
		strcpy(itemLocationCode, val);
	else if (strcmp(key, "learnCode") == 0 && strcmp(learnCode, "") == 0)
		strcpy(learnCode, val);
	else if (strcmp(key, "learnEventCode") == 0 && strcmp(learnEventCode, "") == 0)
		strcpy(learnEventCode, val);
	else if (strcmp(key, "languageIsoCode") == 0 && strcmp(languageIsoCode, "") == 0)
		strcpy(languageIsoCode, val);
	else if (strcmp(key, "countryIsoCode") == 0 && strcmp(countryIsoCode, "") == 0)
		strcpy(countryIsoCode, val);
	else if (strcmp(key, "issueNumber") == 0 && strcmp(issueNumber, "") == 0)
		strcpy(issueNumber, val);
	else if (strcmp(key, "inWork") == 0 && strcmp(inWork, "") == 0)
		strcpy(inWork, val);
	else if (strcmp(key, "securityClassification") == 0 && strcmp(securityClassification, "") == 0)
		strcpy(securityClassification, val);
	else if (strcmp(key, "responsiblePartnerCompany") == 0 && strcmp(responsiblePartnerCompany_enterpriseName, "") == 0)
		strcpy(responsiblePartnerCompany_enterpriseName, val);
	else if (strcmp(key, "responsiblePartnerCompanyCode") == 0 && strcmp(responsiblePartnerCompany_enterpriseCode, "") == 0)
		strcpy(responsiblePartnerCompany_enterpriseCode, val);
	else if (strcmp(key, "originator") == 0 && strcmp(originator_enterpriseName, "") == 0)
		strcpy(originator_enterpriseName, val);
	else if (strcmp(key, "originatorCode") == 0 && strcmp(originator_enterpriseCode, "") == 0)
		strcpy(originator_enterpriseCode, val);
	else if (strcmp(key, "techName") == 0 && strcmp(techName_content, "") == 0)
		strcpy(techName_content, val);
	else if (strcmp(key, "infoName") == 0 && strcmp(infoName_content, "") == 0)
		strcpy(infoName_content, val);
	else if (strcmp(key, "schema") == 0 && strcmp(schema, "") == 0)
		strcpy(schema, val);
}

int main(int argc, char **argv)
{
	time_t now;
	struct tm *local;
	int year, month, day;
	char year_s[5], month_s[3], day_s[3];

	char dmtype[32] = "";

	char dmc[MAX_DATAMODULE_CODE];
	char learn[6] = "";
	char iss[8] = "";

	xmlDocPtr dm;
	xmlNode *dmodule;
	xmlNode *identAndStatusSection;
	xmlNode *dmAddress;
	xmlNode *dmIdent;
	xmlNode *dmCode;
	xmlNode *language;
	xmlNode *issueInfo;
	xmlNode *dmAddressItems;
	xmlNode *issueDate;
	xmlNode *dmStatus;
	xmlNode *security;
	xmlNode *dmTitle;
	xmlNode *techName;
	xmlNode *infoName;
	xmlNode *responsiblePartnerCompany;
	xmlNode *originator;
	xmlNode *enterpriseName;

	FILE *defaults;

	char defaults_fname[256] = "defaults";
	char dmtypes_fname[256] = "dmtypes";

	int i;
	int c;

	bool showprompts = false;
	char dmcode[256] = "";
	bool skipdmc = false;
	bool no_issue = false;

	xmlDocPtr defaults_xml;

	while ((c = getopt(argc, argv, "pd:D:L:C:n:w:c:r:R:o:O:t:i:T:#:NS:h?")) != -1) {
		switch (c) {
			case 'p': showprompts = true; break;
			case 'd': strcpy(defaults_fname, optarg); break;
			case 'D': strcpy(dmtypes_fname, optarg); break;
			case 'L': strcpy(languageIsoCode, optarg); break;
			case 'C': strcpy(countryIsoCode, optarg); break;
			case 'n': strcpy(issueNumber, optarg); break;
			case 'w': strcpy(inWork, optarg); break;
			case 'c': strcpy(securityClassification, optarg); break;
			case 'r': strcpy(responsiblePartnerCompany_enterpriseName, optarg); break;
			case 'R': strcpy(responsiblePartnerCompany_enterpriseCode, optarg); break;
			case 'o': strcpy(originator_enterpriseName, optarg); break;
			case 'O': strcpy(originator_enterpriseCode, optarg); break;
			case 't': strcpy(techName_content, optarg); break;
			case 'i': strcpy(infoName_content, optarg); break;
			case 'T': strcpy(dmtype, optarg); break;
			case '#': strcpy(dmcode, optarg); skipdmc = true; break;
			case 'N': no_issue = true; break;
			case 'S': strcpy(schema, optarg); break;
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
				char *def_key = strtok(default_line, "\t ");
				char *def_val = strtok(NULL, "\t\n");

				copy_default_value(def_key, def_val);
			}

			fclose(defaults);
		}
	}

	if (strcmp(dmcode, "") != 0) {
		int n;

		n = sscanf(dmcode, "%14[^-]-%4[^-]-%3[^-]-%c%c-%4[^-]-%2s%3[^-]-%3s%c-%c-%3s%1s",
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
			fprintf(stderr, ERR_PREFIX "Bad data module code.\n");
			exit(EXIT_BAD_DMC);
		}
	}

	if ((defaults_xml = xmlReadFile(dmtypes_fname, NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING))) {
		xmlNodePtr cur;

		for (cur = xmlDocGetRootElement(defaults_xml)->children; cur; cur = cur->next) {
			char *def_key, *def_val;

			if (cur->type != XML_ELEMENT_NODE) continue;
			if (!xmlHasProp(cur, BAD_CAST "infoCode")) continue;
			if (!xmlHasProp(cur, BAD_CAST "schema")) continue;

			def_key = (char *) xmlGetProp(cur, BAD_CAST "infoCode");
			def_val = (char *) xmlGetProp(cur, BAD_CAST "schema");

			if (strcmp(def_key, infoCode) == 0)
				strcpy(dmtype, def_val);

			xmlFree(def_key);
			xmlFree(def_val);
		}

		xmlFreeDoc(defaults_xml);
	} else {
		defaults = fopen(dmtypes_fname, "r");

		if (defaults) {
			char default_line[1024];

			while (fgets(default_line, 1024, defaults)) {
				char *def_key = strtok(default_line, "\t ");
				char *def_val = strtok(NULL, "\t\n");

				if (strcmp(def_key, infoCode) == 0)
					strcpy(dmtype, def_val);
			}

			fclose(defaults);
		}
	}

	if (showprompts) {
		if (!skipdmc) {
			prompt("Model identification code", modelIdentCode, MAX_MODEL_IDENT_CODE);
			prompt("System difference code", systemDiffCode, MAX_SYSTEM_DIFF_CODE);
			prompt("System code", systemCode, MAX_SYSTEM_CODE);
			prompt("Sub-system code", subSystemCode, MAX_SUB_SYSTEM_CODE);
			prompt("Sub-sub-system code", subSubSystemCode, MAX_SUB_SUB_SYSTEM_CODE);
			prompt("Assembly code", assyCode, MAX_ASSY_CODE);
			prompt("Disassembly code", disassyCode, MAX_DISASSY_CODE);
			prompt("Disassembly code variant", disassyCodeVariant, MAX_DISASSY_CODE_VARIANT);
			prompt("Information code", infoCode, MAX_INFO_CODE);
			prompt("Information code variant", infoCodeVariant, MAX_INFO_CODE_VARIANT);
			prompt("Item location code", itemLocationCode, MAX_ITEM_LOCATION_CODE);
			prompt("Learn code", learnCode, MAX_LEARN_CODE);
			prompt("Learn event code", learnEventCode, MAX_LEARN_EVENT_CODE);
		}
		prompt("Language ISO code", languageIsoCode, MAX_LANGUAGE_ISO_CODE);
		prompt("Country ISO code", countryIsoCode, MAX_COUNTRY_ISO_CODE);
		prompt("Issue number", issueNumber, MAX_ISSUE_NUMBER);
		prompt("In-work issue", inWork, MAX_IN_WORK);
		prompt("Security classification", securityClassification, MAX_SECURITY_CLASSIFICATION);
		prompt("Responsible partner company", responsiblePartnerCompany_enterpriseName, MAX_ENTERPRISE_NAME);
		prompt("Originator", originator_enterpriseName, MAX_ENTERPRISE_NAME);
		prompt("Tech name", techName_content, MAX_TECH_NAME);
		prompt("Info name", infoName_content, MAX_INFO_NAME);
		prompt("DM type", dmtype, 32);
		prompt("Schema", schema, 1024);
	}

	if (strcmp(issueNumber, "") == 0) strcpy(issueNumber, "000");
	if (strcmp(inWork, "") == 0) strcpy(inWork, "01");
	if (strcmp(languageIsoCode, "") == 0) strcpy(languageIsoCode, "und");
	if (strcmp(countryIsoCode, "") == 0) strcpy(countryIsoCode, "ZZ");
	if (strcmp(securityClassification, "") == 0) strcpy(securityClassification, "01");

	if (strcmp(dmtype, "descript") == 0)
		dm = xmlReadMemory((const char *)templates_descript_xml, templates_descript_xml_len, NULL, NULL, 0);
	else if (strcmp(dmtype, "proced") == 0)
		dm = xmlReadMemory((const char *) templates_proced_xml, templates_proced_xml_len, NULL, NULL, 0);
	else if (strcmp(dmtype, "frontmatter") == 0)
		dm = xmlReadMemory((const char *) templates_frontmatter_xml, templates_frontmatter_xml_len, NULL, NULL, 0);
	else if (strcmp(dmtype, "brex") == 0)
		dm = xmlReadMemory((const char *) templates_brex_xml, templates_brex_xml_len, NULL, NULL, 0);
	else if (strcmp(dmtype, "brdoc") == 0)
		dm = xmlReadMemory((const char *) templates_brdoc_xml, templates_brdoc_xml_len, NULL, NULL, 0);
	else if (strcmp(dmtype, "appliccrossreftable") == 0)
		dm = xmlReadMemory((const char *) templates_appliccrossreftable_xml, templates_appliccrossreftable_xml_len, NULL, NULL, 0);
	else if (strcmp(dmtype, "prdcrossreftable") == 0)
		dm = xmlReadMemory((const char *) templates_prdcrossreftable_xml, templates_prdcrossreftable_xml_len, NULL, NULL, 0);
	else if (strcmp(dmtype, "condcrossreftable") == 0)
		dm = xmlReadMemory((const char *) templates_condcrossreftable_xml, templates_condcrossreftable_xml_len, NULL, NULL, 0);
	else if (strcmp(dmtype, "comrep") == 0)
		dm = xmlReadMemory((const char *) templates_comrep_xml, templates_comrep_xml_len, NULL, NULL, 0);
	else if (strcmp(dmtype, "process") == 0)
		dm = xmlReadMemory((const char *) templates_process_xml, templates_process_xml_len, NULL, NULL, 0);
	else if (strcmp(dmtype, "ipd") == 0)
		dm = xmlReadMemory((const char *) templates_ipd_xml, templates_ipd_xml_len, NULL, NULL, 0);
	else {
		fprintf(stderr, "ERROR: Unknown dmtype %s\n", dmtype);
		exit(EXIT_UNKNOWN_DMTYPE);
	}

	dmodule = xmlDocGetRootElement(dm);
	identAndStatusSection = find_child(dmodule, "identAndStatusSection");
	dmAddress = find_child(identAndStatusSection, "dmAddress");
	dmIdent = find_child(dmAddress, "dmIdent");
	dmCode = find_child(dmIdent, "dmCode");
	language = find_child(dmIdent, "language");
	issueInfo = find_child(dmIdent, "issueInfo");
	dmAddressItems = find_child(dmAddress, "dmAddressItems");
	issueDate = find_child(dmAddressItems, "issueDate");
	dmStatus = find_child(identAndStatusSection, "dmStatus");
	security = find_child(dmStatus, "security");
	dmTitle = find_child(dmAddressItems, "dmTitle");
	techName = find_child(dmTitle, "techName");
	infoName = find_child(dmTitle, "infoName");

	if (strcmp(schema, "") != 0) {
		xmlSetProp(dmodule, BAD_CAST "xsi:noNamespaceSchemaLocation", BAD_CAST schema);
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

	xmlSetProp(language, BAD_CAST "languageIsoCode", BAD_CAST languageIsoCode);
	xmlSetProp(language, BAD_CAST "countryIsoCode", BAD_CAST countryIsoCode);

	xmlSetProp(issueInfo, BAD_CAST "issueNumber", BAD_CAST issueNumber);
	xmlSetProp(issueInfo, BAD_CAST "inWork", BAD_CAST inWork);

	time(&now);
	local = localtime(&now);
	year = local->tm_year + 1900;
	month = local->tm_mon + 1;
	day = local->tm_mday;

	sprintf(day_s, "%.2d", day);
	sprintf(month_s, "%.2d", month);
	sprintf(year_s, "%d", year);

	xmlSetProp(issueDate, BAD_CAST "year", BAD_CAST year_s);
	xmlSetProp(issueDate, BAD_CAST "month", BAD_CAST month_s);
	xmlSetProp(issueDate, BAD_CAST "day", BAD_CAST day_s);

	xmlSetProp(security, BAD_CAST "securityClassification", BAD_CAST securityClassification);

	xmlNodeAddContent(techName, BAD_CAST techName_content);

	if (strcmp(infoName_content, "") == 0) {
		xmlUnlinkNode(infoName);
		xmlFreeNode(infoName);
	} else {
		xmlNodeAddContent(infoName, BAD_CAST infoName_content);
	}

	responsiblePartnerCompany = find_child(dmStatus, "responsiblePartnerCompany");
	if (strcmp(responsiblePartnerCompany_enterpriseCode, "") != 0) {
		xmlSetProp(responsiblePartnerCompany, BAD_CAST "enterpriseCode", BAD_CAST responsiblePartnerCompany_enterpriseCode);
	}
	enterpriseName = find_child(responsiblePartnerCompany, "enterpriseName");
	xmlNodeAddContent(enterpriseName, BAD_CAST responsiblePartnerCompany_enterpriseName);

	originator = find_child(dmStatus, "originator");
	if (strcmp(originator_enterpriseCode, "") != 0) {
		xmlSetProp(originator, BAD_CAST "enterpriseCode", BAD_CAST originator_enterpriseCode);
	}
	enterpriseName = find_child(originator, "enterpriseName");
	xmlNodeAddContent(enterpriseName, BAD_CAST originator_enterpriseName);

	for (i = 0; languageIsoCode[i]; ++i) languageIsoCode[i] = toupper(languageIsoCode[i]);

	if (strcmp(learnCode, "") != 0 && strcmp(learnEventCode, "") != 0) {
		sprintf(learn, "-%s%s", learnCode, learnEventCode);
	}

	if (!no_issue) {
		sprintf(iss, "_%s-%s", issueNumber, inWork);
	}

	snprintf(dmc, MAX_DATAMODULE_CODE,
		"DMC-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s%s%s_%s-%s.XML",
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
		learn,
		iss,
		languageIsoCode,
		countryIsoCode);

	if (access(dmc, F_OK) != -1) {
		fprintf(stderr, ERR_PREFIX "Data module already exists.\n");
		exit(EXIT_DM_EXISTS);
	}

	xmlSaveFormatFile(dmc, dm, 1);

	xmlFreeDoc(dm);

	return 0;
}
