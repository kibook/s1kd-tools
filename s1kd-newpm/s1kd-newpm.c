#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#include <libxml/tree.h>

#include "template.h"

#define ERR_PREFIX "s1kd-newpm: ERROR: "

#define EXIT_BAD_PMC 1
#define EXIT_PM_EXISTS 2

xmlNode *find_child(xmlNode *parent, char *name)
{
	xmlNode *cur;

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

void add_dm_ref(xmlNode *pmEntry, char *path)
{
	char *dm_filename;

	char *model_ident_code;
	char *system_diff_code;
	char *system_code;
	char *sub_and_sub_sub;
	char *assy_code;
	char *disassy_code_and_variant;
	char *info_code_and_variant;
	char *item_location_code;
	char *issue_number;
	char *in_work;
	char *language_iso_code;
	char *country_iso_code;

	char sub_system_code[2];
	char sub_sub_system_code[2];
	char disassy_code[3];
	char disassy_code_variant[2];
	char info_code[4];
	char info_code_variant[2];

	xmlNode *dmRef;
	xmlNode *dmRefIdent;
	xmlNode *dmCode;
	xmlNode *issueInfo;
	xmlNode *language;

	int i;

	dm_filename = basename(path);
	
	strtok(dm_filename, "-");
	model_ident_code = strtok(NULL, "-");
	system_diff_code = strtok(NULL, "-");
	system_code = strtok(NULL, "-");
	sub_and_sub_sub = strtok(NULL, "-");
	assy_code = strtok(NULL, "-");
	disassy_code_and_variant = strtok(NULL, "-");
	info_code_and_variant = strtok(NULL, "-");
	item_location_code = strtok(NULL, "_");
	issue_number = strtok(NULL, "-");
	in_work = strtok(NULL, "_");
	language_iso_code = strtok(NULL, "-");
	country_iso_code = strtok(NULL, ".");

	sub_system_code[0] = sub_and_sub_sub[0];
	sub_system_code[1] = 0;
	sub_sub_system_code[0] = sub_and_sub_sub[1];
	sub_sub_system_code[1] = 0;
	strncpy(disassy_code, disassy_code_and_variant, 2);
	disassy_code[2] = 0;
	strncpy(disassy_code_variant, disassy_code_and_variant + 2, 1);
	disassy_code_variant[1] = 0;
	strncpy(info_code, info_code_and_variant, 3);
	info_code[3] = 0;
	strncpy(info_code_variant, info_code_and_variant + 3, 1);
	info_code_variant[1] = 0;

	for (i = 0; language_iso_code[i]; ++i) {
		language_iso_code[i] = tolower(language_iso_code[i]);
	}

	dmRef = xmlNewChild(pmEntry, NULL, (xmlChar *) "dmRef", NULL);
	dmRefIdent = xmlNewChild(dmRef, NULL, (xmlChar *) "dmRefIdent", NULL);
	dmCode = xmlNewChild(dmRefIdent, NULL, (xmlChar *) "dmCode", NULL);

	xmlSetProp(dmCode, (xmlChar *) "modelIdentCode",     (xmlChar *) model_ident_code);
	xmlSetProp(dmCode, (xmlChar *) "systemDiffCode",     (xmlChar *) system_diff_code);
	xmlSetProp(dmCode, (xmlChar *) "systemCode",         (xmlChar *) system_code);
	xmlSetProp(dmCode, (xmlChar *) "subSystemCode",      (xmlChar *) sub_system_code);
	xmlSetProp(dmCode, (xmlChar *) "subSubSystemCode",   (xmlChar *) sub_sub_system_code);
	xmlSetProp(dmCode, (xmlChar *) "assyCode",           (xmlChar *) assy_code);
	xmlSetProp(dmCode, (xmlChar *) "disassyCode",        (xmlChar *) disassy_code);
	xmlSetProp(dmCode, (xmlChar *) "disassyCodeVariant", (xmlChar *) disassy_code_variant);
	xmlSetProp(dmCode, (xmlChar *) "infoCode",           (xmlChar *) info_code);
	xmlSetProp(dmCode, (xmlChar *) "infoCodeVariant",    (xmlChar *) info_code_variant);
	xmlSetProp(dmCode, (xmlChar *) "itemLocationCode",   (xmlChar *) item_location_code);

	issueInfo = xmlNewChild(dmRefIdent, NULL, (xmlChar *) "issueInfo", NULL);

	xmlSetProp(issueInfo, (xmlChar *) "issueNumber", (xmlChar *) issue_number);
	xmlSetProp(issueInfo, (xmlChar *) "inWork",      (xmlChar *) in_work);

	language = xmlNewChild(dmRefIdent, NULL, (xmlChar *) "language", NULL);

	xmlSetProp(language, (xmlChar *) "languageIsoCode", (xmlChar *) language_iso_code);
	xmlSetProp(language, (xmlChar *) "countryIsoCode",  (xmlChar *) country_iso_code);
}

void show_help(void)
{
	puts("Usage: s1kd-newpm [options]");
	puts("");
	puts("Options:");
	puts("  -d    Specify the 'defaults' file name.");
	puts("  -p    Prompt the user for each value");
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
}

int main(int argc, char **argv)
{
	xmlDocPtr pm_doc;

	xmlNode *pm;
	xmlNode *identAndStatusSection;
	xmlNode *pmAddress;
	xmlNode *pmIdent;
	xmlNode *pmCode;
	xmlNode *language;
	xmlNode *issueInfo;
	xmlNode *pmAddressItems;
	xmlNode *issueDate;
	xmlNode *pmTitle;
	xmlNode *pmStatus;
	xmlNode *security;
	xmlNode *responsiblePartnerCompany;
	xmlNode *enterpriseName;
	xmlNode *pmEntry;

	char pm_filename[256];

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

	FILE *defaults;
	char default_line[1024];
	char *def_key;
	char *def_val;

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

	while ((c = getopt(argc, argv, "pd:#:L:C:n:w:c:r:t:Nh?")) != -1) {
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
			case 't': strcpy(pm_title, optarg); break;
			case 'N': no_issue = true; break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	defaults = fopen(defaults_fname, "r");

	if (defaults) {
		while (fgets(default_line, 1024, defaults)) {
			def_key = strtok(default_line, "\t ");
			def_val = strtok(NULL, "\t\n");

			if (strcmp(def_key, "modelIdentCode") == 0)
				strcpy(model_ident_code, def_val);
			else if (strcmp(def_key, "pmIssuer") == 0)
				strcpy(pm_issuer, def_val);
			else if (strcmp(def_key, "pmNumber") == 0)
				strcpy(pm_number, def_val);
			else if (strcmp(def_key, "pmVolume") == 0)
				strcpy(pm_volume, def_val);
			else if (strcmp(def_key, "languageIsoCode") == 0)
				strcpy(language_iso_code, def_val);
			else if (strcmp(def_key, "countryIsoCode") == 0)
				strcpy(country_iso_code, def_val);
			else if (strcmp(def_key, "securityClassification") == 0)
				strcpy(security_classification, def_val);
			else if (strcmp(def_key, "responsiblePartnerCompany") == 0)
				strcpy(enterprise_name, def_val);
			else if (strcmp(def_key, "issueNumber") == 0)
				strcpy(issue_number, def_val);
			else if (strcmp(def_key, "inWork") == 0)
				strcpy(in_work, def_val);
		}

		fclose(defaults);
	}

	pm_doc = xmlReadMemory((const char *) pm_xml, pm_xml_len, NULL, NULL, 0);

	if (strcmp(pmcode, "") != 0) {
		int n;

		n = sscanf(pmcode, "%[^-]-%5s-%5s-%2s",
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
	enterpriseName = find_child(responsiblePartnerCompany, "enterpriseName");
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
	xmlNodeSetContent(enterpriseName, (xmlChar *) enterprise_name);

	for (i = optind; i < argc; ++i) {
		add_dm_ref(pmEntry, argv[i]);
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

	if (access(pm_filename, F_OK) != -1) {
		fprintf(stderr, ERR_PREFIX "Pub module %s already exists.\n", pm_filename);
		exit(EXIT_PM_EXISTS);
	}

	xmlSaveFormatFile(pm_filename, pm_doc, 1);

	xmlFreeDoc(pm_doc);

	return 0;
}
