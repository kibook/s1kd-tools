#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "templates.h"

#define PROG_NAME "s1kd-newdml"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_DML_EXISTS 1
#define EXIT_BAD_INPUT 2
#define EXIT_BAD_CODE 3
#define EXIT_BAD_BREX_DMC 4

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
char sender_ident[7] = "";
char dml_type[3] = "";
char year_of_data_issue[6] = "";
char seq_number[7] = "";
char security_classification[4] = "";
char issue_number[5] = "";
char in_work[4] = "";

char brex_dmcode[256] = "";

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

xmlNodePtr firstXPathNode(const char *xpath, xmlDocPtr doc)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		node = NULL;
	} else {
		node = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return node;
}	

void addDmRef(const char *str, xmlDocPtr dml)
{
	xmlDocPtr dm;
	xmlNodePtr dmlContent;
	xmlNodePtr dmRef, dmRefIdent, dmRefAddressItems, dmlEntry;

	dmlContent = firstXPathNode("//dmlContent", dml);

	dm = xmlReadFile(str, NULL, 0);

	dmlEntry = xmlNewChild(dmlContent, NULL, BAD_CAST "dmlEntry", NULL);
	dmRef = xmlNewChild(dmlEntry, NULL, BAD_CAST "dmRef", NULL);
	dmRefIdent = xmlNewChild(dmRef, NULL, BAD_CAST "dmRefIdent", NULL);
	xmlAddChild(dmRefIdent, xmlCopyNode(firstXPathNode("//dmIdent/identExtension", dm), 1));
	xmlAddChild(dmRefIdent, xmlCopyNode(firstXPathNode("//dmIdent/dmCode", dm), 1));
	xmlAddChild(dmRefIdent, xmlCopyNode(firstXPathNode("//dmIdent/issueInfo", dm), 1));
	xmlAddChild(dmRefIdent, xmlCopyNode(firstXPathNode("//dmIdent/language", dm), 1));
	dmRefAddressItems = xmlNewChild(dmRef, NULL, BAD_CAST "dmRefAddressItems", NULL);
	xmlAddChild(dmRefAddressItems, xmlCopyNode(firstXPathNode("//dmAddressItems/dmTitle", dm), 1));
	xmlAddChild(dmRefAddressItems, xmlCopyNode(firstXPathNode("//dmAddressItems/issueDate", dm), 1));
	xmlAddChild(dmlEntry, xmlCopyNode(firstXPathNode("//dmStatus/security", dm), 1));
	xmlAddChild(dmlEntry, xmlCopyNode(firstXPathNode("//dmStatus/responsiblePartnerCompany", dm), 1));
}

void copy_default_value(const char *def_key, const char *def_val)
{
	if (strcmp(def_key, "modelIdentCode") == 0 && strcmp(model_ident_code, "") == 0)
		strcpy(model_ident_code, def_val);
	else if (strcmp(def_key, "senderIdent") == 0 && strcmp(sender_ident, "") == 0)
		strcpy(sender_ident, def_val);
	else if (strcmp(def_key, "dmlType") == 0 && strcmp(dml_type, "") == 0)
		strcpy(dml_type, def_val);
	else if (strcmp(def_key, "yearOfDataIssue") == 0 && strcmp(year_of_data_issue, "") == 0)
		strcpy(year_of_data_issue, def_val);
	else if (strcmp(def_key, "seqNumber") == 0 && strcmp(seq_number, "") == 0)
		strcpy(seq_number, def_val);
	else if (strcmp(def_key, "issueNumber") == 0 && strcmp(issue_number, "") == 0)
		strcpy(issue_number, def_val);
	else if (strcmp(def_key, "inWork") == 0 && strcmp(in_work, "") == 0)
		strcpy(in_work, def_val);
	else if (strcmp(def_key, "securityClassification") == 0 && strcmp(security_classification, "") == 0)
		strcpy(security_classification, def_val);
	else if (strcmp(def_key, "brex") == 0 && strcmp(brex_dmcode, "") == 0)
		strcpy(brex_dmcode, def_val);
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] <datamodules>");
	puts("");
	puts("Options:");
	puts("  -h -?    Show usage message.");
	puts("  -p       Prompt the user for each value.");
	puts("  -N       Omit issue/inwork from filename.");
	puts("  -v       Print file name of DML.");
	puts("");
	puts("In addition, the following pieces of metadata can be set:");
	puts("  -#       DML code");
	puts("  -n       Issue number");
	puts("  -w       Inwork issue");
	puts("  -c       Security classification");
	puts("  -b       BREX data module code");
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

	dmCode = firstXPathNode("//brexDmRef/dmRef/dmRefIdent/dmCode", doc);

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
	xmlDocPtr dml_doc;

	xmlNodePtr dmlCode, issueInfo, security, issueDate;
	
	xmlXPathContextPtr ctxt;
	xmlXPathObjectPtr results;

	char defaults_fname[PATH_MAX] = "defaults";
	FILE *defaults;

	bool showprompts = false;
	char code[256];
	bool skipcode = false;
	bool noissue = false;
	bool verbose = false;

	time_t now;
	struct tm *local;
	int year, month, day;
	char year_s[5], month_s[3], day_s[3];

	char dml_fname[PATH_MAX];

	int c;

	xmlDocPtr defaults_xml;

	while ((c = getopt(argc, argv, "pd:#:n:w:c:Nb:vh?")) != -1) {
		switch (c) {
			case 'p': showprompts = true; break;
			case 'd': strcpy(defaults_fname, optarg); break;
			case '#': strcpy(code, optarg); skipcode = true; break;
			case 'n': strcpy(issue_number, optarg); break;
			case 'w': strcpy(in_work, optarg); break;
			case 'c': strcpy(security_classification, optarg); break;
			case 'N': noissue = true; break;
			case 'b': strncpy(brex_dmcode, optarg, 255); break;
			case 'v': verbose = true; break;
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
	
	if (strcmp(code, "") != 0) {
		int n;

		n = sscanf(code, "%14[^-]-%5[^-]-%c-%4[^-]-%5[^-]",
			model_ident_code,
			sender_ident,
			dml_type,
			year_of_data_issue,
			seq_number);

		if (n != 5) {
			fprintf(stderr, ERR_PREFIX "Bad DML code.\n");
			exit(EXIT_BAD_CODE);
		}
	}

	if (showprompts) {
		if (!skipcode) {
			prompt("Model ident code", model_ident_code, 16);
			prompt("Sender ident", sender_ident, 7);
			prompt("DML type", dml_type, 3);
			prompt("Year of data issue", year_of_data_issue, 6);
			prompt("Sequence number", seq_number, 7);
		}
		prompt("Issue number", issue_number, 5);
		prompt("In-work issue", in_work, 4);
		prompt("Security classification", security_classification, 4);
	}

	if (strcmp(issue_number, "") == 0) strcpy(issue_number, "000");
	if (strcmp(in_work, "") == 0) strcpy(in_work, "01");
	if (strcmp(security_classification, "") == 0) strcpy(security_classification, "01");

	dml_doc = xmlReadMemory((const char *) dml_xml, dml_xml_len, NULL, NULL, 0);

	ctxt = xmlXPathNewContext(dml_doc);

	results = xmlXPathEvalExpression(BAD_CAST "//dmlIdent/dmlCode", ctxt);
	dmlCode = results->nodesetval->nodeTab[0];
	xmlXPathFreeObject(results);
	results = xmlXPathEvalExpression(BAD_CAST "//dmlIdent/issueInfo", ctxt);
	issueInfo = results->nodesetval->nodeTab[0];
	xmlXPathFreeObject(results);
	results = xmlXPathEvalExpression(BAD_CAST "//dmlStatus/security", ctxt);
	security = results->nodesetval->nodeTab[0];
	xmlXPathFreeObject(results);
	results = xmlXPathEvalExpression(BAD_CAST "//dmlAddressItems/issueDate", ctxt);
	issueDate = results->nodesetval->nodeTab[0];

	dml_type[0] = tolower(dml_type[0]);

	xmlSetProp(dmlCode, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
	xmlSetProp(dmlCode, BAD_CAST "senderIdent", BAD_CAST sender_ident);
	xmlSetProp(dmlCode, BAD_CAST "dmlType", BAD_CAST dml_type);
	xmlSetProp(dmlCode, BAD_CAST "yearOfDataIssue", BAD_CAST year_of_data_issue);
	xmlSetProp(dmlCode, BAD_CAST "seqNumber", BAD_CAST seq_number);

	xmlSetProp(issueInfo, BAD_CAST "issueNumber", BAD_CAST issue_number);
	xmlSetProp(issueInfo, BAD_CAST "inWork", BAD_CAST in_work);

	xmlSetProp(security, BAD_CAST "securityClassification", BAD_CAST security_classification);

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

	xmlXPathFreeContext(ctxt);

	if (strcmp(brex_dmcode, "") != 0)
		set_brex(dml_doc, brex_dmcode);

	dml_type[0] = toupper(dml_type[0]);

	if (noissue) {
		snprintf(dml_fname, PATH_MAX,
			"DML-%s-%s-%s-%s-%s.XML",
			model_ident_code,
			sender_ident,
			dml_type,
			year_of_data_issue,
			seq_number);
	} else {
		snprintf(dml_fname, PATH_MAX,
			"DML-%s-%s-%s-%s-%s_%s-%s.XML",
			model_ident_code,
			sender_ident,
			dml_type,
			year_of_data_issue,
			seq_number,
			issue_number,
			in_work);
	}

	if (access(dml_fname, F_OK) != -1) {
		fprintf(stderr, ERR_PREFIX "Data module list already exists.\n");
		exit(EXIT_DML_EXISTS);
	}

	for (c = optind; c < argc; ++c) {
		addDmRef(argv[c], dml_doc);
	}

	xmlSaveFile(dml_fname, dml_doc);

	if (verbose)
		puts(dml_fname);

	xmlFreeDoc(dml_doc);

	xmlCleanupParser();

	return 0;
}
