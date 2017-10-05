#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "template.h"

#define PROG_NAME "s1kd-newcom"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_BAD_CODE 1
#define EXIT_COMMENT_EXISTS 2
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

char modelIdentCode[16] = "";
char senderIdent[7] = "";
char yearOfDataIssue[6] = "";
char seqNumber[7] = "";
char commentType[3] = "";
char languageIsoCode[5] = "";
char countryIsoCode[4] = "";
char enterprise_name[256] = "";
char address_city[256] = "";
char address_country[256] = "";
char securityClassification[4] = "";
char commentPriorityCode[6] = "";
char responseType[6] = "";

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
	puts("Usage: " PROG_NAME " [options]");
	puts("");
	puts("Options:");
	puts("  -d    Specify the 'defaults' file name.");
	puts("  -p    Prompt the user for each value");
	puts("In addition, the following pieces of meta data can be set:");
	puts("  -#    Comment code");
	puts("  -L    Language ISO code");
	puts("  -C    Country ISO code");
	puts("  -c    Security classification");
	puts("  -o    Originator");
	puts("  -t    Title");
	puts("  -r    Response type");
	puts("  -b    BREX data module code");
}

void copy_default_value(const char *def_key, const char *def_val)
{
	if (strcmp(def_key, "modelIdentCode") == 0)
		strcpy(modelIdentCode, def_val);
	else if (strcmp(def_key, "senderIdent") == 0)
		strcpy(senderIdent, def_val);
	else if (strcmp(def_key, "yearOfDataIssue") == 0)
		strcpy(yearOfDataIssue, def_val);
	else if (strcmp(def_key, "seqNumber") == 0)
		strcpy(seqNumber, def_val);
	else if (strcmp(def_key, "commentType") == 0)
		strcpy(commentType, def_val);
	else if (strcmp(def_key, "languageIsoCode") == 0)
		strcpy(languageIsoCode, def_val);
	else if (strcmp(def_key, "countryIsoCode") == 0)
		strcpy(countryIsoCode, def_val);
	else if (strcmp(def_key, "originator") == 0)
		strcpy(enterprise_name, def_val);
	else if (strcmp(def_key, "city") == 0)
		strcpy(address_city, def_val);
	else if (strcmp(def_key, "country") == 0)
		strcpy(address_country, def_val);
	else if (strcmp(def_key, "securityClassification") == 0)
		strcpy(securityClassification, def_val);
	else if (strcmp(def_key, "commentPriorityCode") == 0)
		strcpy(commentPriorityCode, def_val);
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
	xmlDocPtr comment_doc;

	xmlNodePtr comment;
	xmlNodePtr identAndStatusSection;
	xmlNodePtr commentAddress;
	xmlNodePtr commentIdent;
	xmlNodePtr commentCode;
	xmlNodePtr language;
	xmlNodePtr commentAddressItems;
	xmlNodePtr issueDate;
	xmlNodePtr commentOriginator;
	xmlNodePtr dispatchAddress;
	xmlNodePtr enterprise;
	xmlNodePtr enterpriseName;
	xmlNodePtr address;
	xmlNodePtr city;
	xmlNodePtr country;
	xmlNodePtr commentStatus;
	xmlNodePtr security;
	xmlNodePtr commentPriority;
	xmlNodePtr commentResponse;

	time_t now;
	struct tm *local;
	int year, month, day;
	char year_s[5], month_s[3], day_s[3];

	FILE *defaults;

	char comment_fname[256];

	char language_fname[4];

	char code[256] = "";
	char defaults_fname[PATH_MAX] = "defaults";
	bool show_prompts = false;
	bool skip_code = false;
	char commentTitle[256] = "";

	xmlDocPtr defaults_xml;

	char brex_dmcode[256] = "";

	int i;

	while ((i = getopt(argc, argv, "d:p#:o:c:L:C:P:t:r:b:h?")) != -1) {
		switch (i) {
			case 'd':
				strncpy(defaults_fname, optarg, PATH_MAX - 1);
				break;
			case 'p':
				show_prompts = true;
				break;
			case '#':
				skip_code = true;
				strncpy(code, optarg, 255);
				break;
			case 'o':
				strncpy(enterprise_name, optarg, 255);
				break;
			case 'c':
				strncpy(securityClassification, optarg, 2);
				break;
			case 'L':
				strncpy(languageIsoCode, optarg, 3);
				break;
			case 'C':
				strncpy(countryIsoCode, optarg, 2);
				break;
			case 'P':
				strncpy(commentPriorityCode, optarg, 4);
				break;
			case 't':
				strncpy(commentTitle, optarg, 255);
				break;
			case 'r':
				strncpy(responseType, optarg, 4);
				break;
			case 'b':
				strncpy(brex_dmcode, optarg, 255);
				break;
			case 'h':
			case '?':
				show_help();
				exit(0);
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
				char *def_key;
				char *def_val;

				def_key = strtok(default_line, "\t ");
				def_val = strtok(NULL, "\t\n");

				copy_default_value(def_key, def_val);
			}

			fclose(defaults);
		}
	}

	comment_doc = xmlReadMemory((const char *) comment_xml, comment_xml_len, NULL, NULL, 0);

	if (strcmp(code, "") != 0) {
		int n;

		n = sscanf(code, "%14[^-]-%5s-%4s-%5s-%1s",
			modelIdentCode,
			senderIdent,
			yearOfDataIssue,
			seqNumber,
			commentType);
		
		if (n != 5) {
			fprintf(stderr, ERR_PREFIX "Invalid comment code: '%s'\n", code);
			exit(EXIT_BAD_CODE);
		}
	}

	if (show_prompts) {
		if (!skip_code) {
			prompt("Model ident code", modelIdentCode, 16);
			prompt("Sender ident", senderIdent, 7);
			prompt("Year of data issue", yearOfDataIssue, 6);
			prompt("Sequence numer", seqNumber, 7);
			prompt("Comment type", commentType, 3);
		}
		prompt("Language ISO code", languageIsoCode, 5);
		prompt("Country ISO code", countryIsoCode, 4);
		prompt("Originator enterprise name", enterprise_name, 256);
		prompt("Originator city", address_city, 256);
		prompt("Originator country", address_country, 256);
		prompt("Security classification", securityClassification, 4);
		prompt("Comment priority code", commentPriorityCode, 6);
	}

	if (strcmp(languageIsoCode, "") == 0) strcpy(languageIsoCode, "und");
	if (strcmp(countryIsoCode, "") == 0) strcpy(countryIsoCode, "ZZ");
	if (strcmp(securityClassification, "") == 0) strcpy(securityClassification, "01");
	if (strcmp(responseType, "") == 0) strcpy(responseType, "rt02");

	for (i = 0; languageIsoCode[i]; ++i) languageIsoCode[i] = tolower(languageIsoCode[i]);
	for (i = 0; countryIsoCode[i]; ++i) countryIsoCode[i] = toupper(countryIsoCode[i]);
	for (i = 0; commentType[i]; ++i) commentType[i] = tolower(commentType[i]);

	comment = xmlDocGetRootElement(comment_doc);
	identAndStatusSection = find_child(comment, "identAndStatusSection");
	commentAddress = find_child(identAndStatusSection, "commentAddress");
	commentIdent = find_child(commentAddress, "commentIdent");
	commentCode = find_child(commentIdent, "commentCode");
	language = find_child(commentIdent, "language");
	commentAddressItems = find_child(commentAddress, "commentAddressItems");
	issueDate = find_child(commentAddressItems, "issueDate");
	commentOriginator = find_child(commentAddressItems, "commentOriginator");
	dispatchAddress = find_child(commentOriginator, "dispatchAddress");
	enterprise = find_child(dispatchAddress, "enterprise");
	enterpriseName = find_child(enterprise, "enterpriseName");
	address = find_child(dispatchAddress, "address");
	city = find_child(address, "city");
	country = find_child(address, "country");
	commentStatus = find_child(identAndStatusSection, "commentStatus");
	security = find_child(commentStatus, "security");
	commentPriority = find_child(commentStatus, "commentPriority");
	commentResponse = find_child(commentStatus, "commentResponse");

	xmlSetProp(commentCode, BAD_CAST "modelIdentCode", BAD_CAST modelIdentCode);
	xmlSetProp(commentCode, BAD_CAST "senderIdent", BAD_CAST senderIdent);
	xmlSetProp(commentCode, BAD_CAST "yearOfDataIssue", BAD_CAST yearOfDataIssue);
	xmlSetProp(commentCode, BAD_CAST "seqNumber", BAD_CAST seqNumber);
	xmlSetProp(commentCode, BAD_CAST"commentType", BAD_CAST commentType);

	xmlSetProp(language, BAD_CAST "languageIsoCode", BAD_CAST languageIsoCode);
	xmlSetProp(language, BAD_CAST "countryIsoCode",  BAD_CAST countryIsoCode);

	if (strcmp(commentTitle, "") != 0) {
		xmlNodePtr commentTitleNode = xmlNewNode(NULL, BAD_CAST "commentTitle");
		commentTitleNode = xmlAddPrevSibling(issueDate, commentTitleNode);
		xmlNodeSetContent(commentTitleNode, BAD_CAST commentTitle);
	}

	time(&now);
	local = localtime(&now);
	year = local->tm_year + 1900;
	month = local->tm_mon + 1;
	day = local->tm_mday;
	sprintf(year_s, "%d", year);
	sprintf(month_s, "%.2d", month);
	sprintf(day_s, "%.2d", day);
	xmlSetProp(issueDate, BAD_CAST "year",  BAD_CAST year_s);
	xmlSetProp(issueDate, BAD_CAST "month", BAD_CAST month_s);
	xmlSetProp(issueDate, BAD_CAST "day",   BAD_CAST day_s);

	xmlNodeSetContent(enterpriseName, BAD_CAST enterprise_name);
	xmlNodeSetContent(city, BAD_CAST address_city);
	xmlNodeSetContent(country, BAD_CAST address_country);

	xmlSetProp(security,        BAD_CAST "securityClassification", BAD_CAST securityClassification);
	xmlSetProp(commentPriority, BAD_CAST "commentPriorityCode",    BAD_CAST commentPriorityCode);
	xmlSetProp(commentResponse, BAD_CAST "responseType", BAD_CAST responseType);

	if (strcmp(brex_dmcode, "") != 0)
		set_brex(comment_doc, brex_dmcode);

	for (i = 0; languageIsoCode[i]; ++i) {
		language_fname[i] = toupper(languageIsoCode[i]);
	}
	language_fname[i] = '\0';

	for (i = 0; commentType[i]; ++i) commentType[i] = toupper(commentType[i]);

	snprintf(comment_fname, 256, "COM-%s-%s-%s-%s-%s_%s-%s.XML",
		modelIdentCode,
		senderIdent,
		yearOfDataIssue,
		seqNumber,
		commentType,
		language_fname,
		countryIsoCode);

	if (access(comment_fname, F_OK) != -1) {
		fprintf(stderr, ERR_PREFIX "%s already exists.\n", comment_fname);
		exit(EXIT_COMMENT_EXISTS);
	}

	xmlSaveFormatFile(comment_fname, comment_doc, 1);

	xmlFreeDoc(comment_doc);

	return 0;
}
