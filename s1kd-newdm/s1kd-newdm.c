#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>

#include "templates.h"
#include "dmtypes.h"
#include "sns.h"

#define MAX_MODEL_IDENT_CODE		14	+ 2
#define MAX_SYSTEM_DIFF_CODE		 4	+ 2
#define MAX_SYSTEM_CODE			 3	+ 2
#define MAX_SUB_SYSTEM_CODE		 1	+ 2
#define MAX_SUB_SUB_SYSTEM_CODE		 1	+ 2
#define MAX_ASSY_CODE			 4	+ 2
#define MAX_DISASSY_CODE		 2	+ 2
#define MAX_DISASSY_CODE_VARIANT	 3	+ 2
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

#define PROG_NAME "s1kd-newdm"
#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_DM_EXISTS 1
#define EXIT_UNKNOWN_DMTYPE 2
#define EXIT_BAD_DMC 3
#define EXIT_BAD_BREX_DMC 4
#define EXIT_BAD_DATE 5
#define EXIT_BAD_ISSUE 6

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

char dmtype[32] = "";

char schema[1024] = "";
char brex_dmcode[256] = "";
char *sns_fname = NULL;
char *maint_sns = NULL;
char issue_date[16] = "";

xmlChar *remarks = NULL;

/* Omit the issue information from the object filename. */
bool no_issue = false;
bool no_issue_set = false;

enum issue { NO_ISS, ISS_20, ISS_21, ISS_22, ISS_23, ISS_30, ISS_40, ISS_41, ISS_42 } issue = NO_ISS;

#define DEFAULT_S1000D_ISSUE ISS_42

#define ISS_22_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_23_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_30_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_40_DEFAULT_BREX "S1000D-A-04-10-0301-00A-022A-D"
#define ISS_41_DEFAULT_BREX "S1000D-E-04-10-0301-00A-022A-D"

/* ISO language and country codes if none can be determined. */
#define DEFAULT_LANGUAGE_ISO_CODE "und"
#define DEFAULT_COUNTRY_ISO_CODE "ZZ"

char *template_dir = NULL;

/* Include the previous level of SNS title in a tech name. */
bool sns_prev_title = false;
bool sns_prev_title_set = false;

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
	else if (strcmp(iss, "2.3") == 0)
		return ISS_23;
	else if (strcmp(iss, "2.2") == 0)
		return ISS_22;
	else if (strcmp(iss, "2.1") == 0)
		return ISS_21;
	else if (strcmp(iss, "2.0") == 0)
		return ISS_20;
	
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
		case ISS_23: return "2.3";
		case ISS_22: return "2.2";
		case ISS_21: return "2.1";
		case ISS_20: return "2.0";
		default: return "";
	}
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
	puts("  -f      Overwrite existing file.");
	puts("  -N      Omit issue/inwork from filename.");
	puts("  -p      Prompt the user for each value");
	puts("  -q      Don't report an error if file exists.");
	puts("  -S      Get tech name from BREX SNS.");
	puts("  -M      Use one of the maintained SNS.");
	puts("  -P      Include previous level of SNS in tech name.");
	puts("  -v      Print file name of new data module.");
	puts("  -$      Specify which S1000D issue to use.");
	puts("  -@      Output to specified file.");
	puts("  -%      Use templates in specified directory.");
	puts("  -,      Dump default dmtypes XML.");
	puts("  -.      Dump default dmtypes text file.");
	puts("");
	puts("In addition, the following pieces of meta data can be set:");
	puts("  -#      Data module code");
	puts("  -L      Language ISO code");
	puts("  -C      Country ISO code");
	puts("  -n      Issue number");
	puts("  -w      Inwork issue");
	puts("  -c      Security classification");
	puts("  -r      Responsible partner company enterprise name");
	puts("  -R      Responsible partner company CAGE code.");
	puts("  -o      Originator enterprise name");
	puts("  -O      Originator CAGE code.");
	puts("  -t      Tech name");
	puts("  -i      Info name");
	puts("  -T      DM type (descript, proced, frontmatter, etc.)");
	puts("  -b      BREX data module code");
	puts("  -s      Schema");
	puts("  -I      Issue date");
	puts("  -m      Remarks");
}

void copy_default_value(const char *key, const char *val)
{
	if (strcmp(key, "modelIdentCode") == 0 && strcmp(modelIdentCode, "") == 0)
		strncpy(modelIdentCode, val, MAX_MODEL_IDENT_CODE - 2);
	else if (strcmp(key, "systemDiffCode") == 0 && strcmp(systemDiffCode, "") == 0)
		strncpy(systemDiffCode, val, MAX_SYSTEM_DIFF_CODE - 2);
	else if (strcmp(key, "systemCode") == 0 && strcmp(systemCode, "") == 0)
		strncpy(systemCode, val, MAX_SYSTEM_CODE - 2);
	else if (strcmp(key, "subSystemCode") == 0 && strcmp(subSystemCode, "") == 0)
		strncpy(subSystemCode, val, MAX_SUB_SYSTEM_CODE - 2);
	else if (strcmp(key, "subSubSystemCode") == 0 && strcmp(subSubSystemCode, "") == 0)
		strncpy(subSubSystemCode, val, MAX_SUB_SUB_SYSTEM_CODE - 2);
	else if (strcmp(key, "assyCode") == 0 && strcmp(assyCode, "") == 0)
		strncpy(assyCode, val, MAX_ASSY_CODE - 2);
	else if (strcmp(key, "disassyCode") == 0 && strcmp(disassyCode, "") == 0)
		strncpy(disassyCode, val, MAX_DISASSY_CODE - 2);
	else if (strcmp(key, "disassyCodeVariant") == 0 && strcmp(disassyCodeVariant, "") == 0)
		strncpy(disassyCodeVariant, val, MAX_DISASSY_CODE_VARIANT - 2);
	else if (strcmp(key, "infoCode") == 0 && strcmp(infoCode, "") == 0)
		strncpy(infoCode, val, MAX_INFO_CODE - 2);
	else if (strcmp(key, "infoCodeVariant") == 0 && strcmp(infoCodeVariant, "") == 0)
		strncpy(infoCodeVariant, val, MAX_INFO_CODE_VARIANT - 2);
	else if (strcmp(key, "itemLocationCode") == 0 && strcmp(itemLocationCode, "") == 0)
		strncpy(itemLocationCode, val, MAX_ITEM_LOCATION_CODE - 2);
	else if (strcmp(key, "learnCode") == 0 && strcmp(learnCode, "") == 0)
		strncpy(learnCode, val, MAX_LEARN_CODE - 2);
	else if (strcmp(key, "learnEventCode") == 0 && strcmp(learnEventCode, "") == 0)
		strncpy(learnEventCode, val, MAX_LEARN_EVENT_CODE - 2);
	else if (strcmp(key, "languageIsoCode") == 0 && strcmp(languageIsoCode, "") == 0)
		strncpy(languageIsoCode, val, MAX_LANGUAGE_ISO_CODE - 2);
	else if (strcmp(key, "countryIsoCode") == 0 && strcmp(countryIsoCode, "") == 0)
		strncpy(countryIsoCode, val, MAX_COUNTRY_ISO_CODE - 2);
	else if (strcmp(key, "issueNumber") == 0 && strcmp(issueNumber, "") == 0)
		strncpy(issueNumber, val, MAX_ISSUE_NUMBER - 2);
	else if (strcmp(key, "inWork") == 0 && strcmp(inWork, "") == 0)
		strncpy(inWork, val, MAX_IN_WORK - 2);
	else if (strcmp(key, "securityClassification") == 0 && strcmp(securityClassification, "") == 0)
		strncpy(securityClassification, val, MAX_SECURITY_CLASSIFICATION - 2);
	else if (strcmp(key, "responsiblePartnerCompany") == 0 && strcmp(responsiblePartnerCompany_enterpriseName, "") == 0)
		strncpy(responsiblePartnerCompany_enterpriseName, val, MAX_ENTERPRISE_NAME - 2);
	else if (strcmp(key, "responsiblePartnerCompanyCode") == 0 && strcmp(responsiblePartnerCompany_enterpriseCode, "") == 0)
		strncpy(responsiblePartnerCompany_enterpriseCode, val, MAX_ENTERPRISE_CODE - 2);
	else if (strcmp(key, "originator") == 0 && strcmp(originator_enterpriseName, "") == 0)
		strncpy(originator_enterpriseName, val, MAX_ENTERPRISE_NAME - 2);
	else if (strcmp(key, "originatorCode") == 0 && strcmp(originator_enterpriseCode, "") == 0)
		strncpy(originator_enterpriseCode, val, MAX_ENTERPRISE_CODE - 2);
	else if (strcmp(key, "techName") == 0 && strcmp(techName_content, "") == 0)
		strncpy(techName_content, val, MAX_TECH_NAME - 2);
	else if (strcmp(key, "infoName") == 0 && strcmp(infoName_content, "") == 0)
		strncpy(infoName_content, val, MAX_INFO_CODE - 2);
	else if (strcmp(key, "schema") == 0 && strcmp(schema, "") == 0)
		strncpy(schema, val, 1023);
	else if (strcmp(key, "brex") == 0 && strcmp(brex_dmcode, "") == 0)
		strncpy(brex_dmcode, val, 255);
	else if (strcmp(key, "sns") == 0 && !sns_fname)
		sns_fname = strdup(val);
	else if (strcmp(key, "issue") == 0 && issue == NO_ISS)
		issue = get_issue(val);
	else if (strcmp(key, "omitIssueInfo") == 0 && !no_issue_set)
		no_issue = strcasecmp(val, "true") == 0;
	else if (strcmp(key, "remarks") == 0 && !remarks)
		remarks = xmlStrdup(BAD_CAST val);
	else if (strcmp(key, "templates") == 0 && !template_dir)
		template_dir = strdup(val);
	else if (strcmp(key, "maintainedSns") == 0 && !maint_sns)
		maint_sns = strdup(val);
	else if (strcmp(key, "includePrevSnsTitle") == 0 && !sns_prev_title_set)
		sns_prev_title = strcasecmp(val, "true") == 0;
}

xmlNodePtr firstXPathNode(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(doc ? doc : node->doc);
	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	first = xmlXPathNodeSetIsEmpty(obj->nodesetval) ? NULL : obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
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

	dmCode = firstXPathNode(doc, NULL, "//brexDmRef/dmRef/dmRefIdent/dmCode");

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

#define SNS_XPATH_1 "//snsSystem[snsCode='%s']/snsSubSystem[snsCode='%s']/snsSubSubSystem[snsCode='%s']/snsAssy[snsCode='%s']/snsTitle"
#define SNS_XPATH_2 "//snsSystem[snsCode='%s']/snsSubSystem[snsCode='%s']/snsSubSubSystem[snsCode='%s']/snsTitle"
#define SNS_XPATH_3 "//snsSystem[snsCode='%s']/snsSubSystem[snsCode='%s']/snsTitle"
#define SNS_XPATH_4 "//snsSystem[snsCode='%s']/snsTitle"

void set_sns_title(xmlNodePtr snsTitle)
{
	char *title;

	title = (char *) xmlNodeGetContent(snsTitle);

	strcpy(techName_content, "");

	if (sns_prev_title) {
		xmlNodePtr prev;
		if ((prev = firstXPathNode(NULL, snsTitle, "parent::*/parent::*/snsTitle"))) {
			char *p;
			if (strcmp((p = (char *) xmlNodeGetContent(prev)), title) != 0) {
				strcpy(techName_content, p);
				strcat(techName_content, " - ");
			}
			xmlFree(p);
		}
	}

	strcat(techName_content, title);

	xmlFree(title);
}

/* Find the filename of the latest version of a BREX DM in the current directory
 * by its code.
 */
bool find_brex_file(char *dst, const char *code)
{
	DIR *dir;
	struct dirent *cur;
	int n = strlen(code);
	bool found = false;

	dir = opendir(".");

	strcpy(dst, "");

	while ((cur = readdir(dir))) {
		if (strncmp(code, cur->d_name + 4, n) == 0 && strcmp(cur->d_name, dst) > 0) {
			strcpy(dst, cur->d_name);
			found = true;
		}
	}

	closedir(dir);

	return found;
}

struct inmem_xml {
	unsigned char *xml;
	unsigned int len;
};

/* Map maintained SNS title to XML template. */
struct inmem_xml maint_sns_xml(void)
{
	struct inmem_xml res;

	if (strcasecmp(maint_sns, "Generic") == 0) {
		res.xml = sns_DMC_S1000D_A_08_02_0100_00A_022A_D_EN_CA_XML;
		res.len = sns_DMC_S1000D_A_08_02_0100_00A_022A_D_EN_CA_XML_len;
	} else if (strcasecmp(maint_sns, "Support and training equipment") == 0) {
		res.xml = sns_DMC_S1000D_A_08_02_0200_00A_022A_D_EN_CA_XML;
		res.len = sns_DMC_S1000D_A_08_02_0200_00A_022A_D_EN_CA_XML_len;
	} else if (strcasecmp(maint_sns, "Ordnance") == 0) {
		res.xml = sns_DMC_S1000D_A_08_02_0300_00A_022A_D_EN_CA_XML;
		res.len = sns_DMC_S1000D_A_08_02_0300_00A_022A_D_EN_CA_XML_len;
	} else if (strcasecmp(maint_sns, "General communications") == 0) {
		res.xml = sns_DMC_S1000D_A_08_02_0400_00A_022A_D_EN_CA_XML;
		res.len = sns_DMC_S1000D_A_08_02_0400_00A_022A_D_EN_CA_XML_len;
	} else if (strcasecmp(maint_sns, "Air vehicle, engines and equipment") == 0) {
		res.xml = sns_DMC_S1000D_A_08_02_0500_00A_022A_D_EN_CA_XML;
		res.len = sns_DMC_S1000D_A_08_02_0500_00A_022A_D_EN_CA_XML_len;
	} else if (strcasecmp(maint_sns, "Tactical missiles") == 0) {
		res.xml = sns_DMC_S1000D_A_08_02_0600_00A_022A_D_EN_CA_XML;
		res.len = sns_DMC_S1000D_A_08_02_0600_00A_022A_D_EN_CA_XML_len;
	} else if (strcasecmp(maint_sns, "General surface vehicles") == 0) {
		res.xml = sns_DMC_S1000D_A_08_02_0700_00A_022A_D_EN_CA_XML;
		res.len = sns_DMC_S1000D_A_08_02_0700_00A_022A_D_EN_CA_XML_len;
	} else if (strcasecmp(maint_sns, "General sea vehicles") == 0) {
		res.xml = sns_DMC_S1000D_A_08_02_0800_00A_022A_D_EN_CA_XML;
		res.len = sns_DMC_S1000D_A_08_02_0800_00A_022A_D_EN_CA_XML_len;
	} else {
		fprintf(stderr, ERR_PREFIX "No maintained SNS: %s\n", maint_sns);
		res.xml = NULL;
		res.len = 0;
	}

	return res;
}


xmlDocPtr maint_sns_doc(void)
{
	struct inmem_xml xml;

	xml = maint_sns_xml();

	return xmlReadMemory((const char *) xml.xml, xml.len, NULL, NULL, 0);
}

xmlDocPtr set_tech_from_sns(void)
{
	xmlDocPtr brex = NULL;
	char xpath[256];
	xmlNodePtr snsTitle;
	char fname[PATH_MAX];

	if (sns_fname && find_brex_file(fname, sns_fname)) {
		brex = xmlReadFile(fname, NULL, 0);
	} else if (maint_sns) {
		brex = maint_sns_doc();
	}

	if (!brex) {
		return NULL;
	}

	sprintf(xpath, SNS_XPATH_1, systemCode, subSystemCode, subSubSystemCode, assyCode);
	if ((snsTitle = firstXPathNode(brex, NULL, xpath))) {
		set_sns_title(snsTitle);
		return brex;
	}

	sprintf(xpath, SNS_XPATH_2, systemCode, subSystemCode, subSubSystemCode);
	if ((snsTitle = firstXPathNode(brex, NULL, xpath))) {
		set_sns_title(snsTitle);
		return brex;
	}

	sprintf(xpath, SNS_XPATH_3, systemCode, subSystemCode);
	if ((snsTitle = firstXPathNode(brex, NULL, xpath))) {
		set_sns_title(snsTitle);
		return brex;
	}

	sprintf(xpath, SNS_XPATH_4, systemCode);
	if ((snsTitle = firstXPathNode(brex, NULL, xpath))) {
		set_sns_title(snsTitle);
		return brex;
	}

	return brex;
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

		sprintf(day_s, "%.2d", day);
		sprintf(month_s, "%.2d", month);
		sprintf(year_s, "%d", year);
	} else {
		if (sscanf(issue_date, "%4s-%2s-%2s", year_s, month_s, day_s) != 3) {
			fprintf(stderr, ERR_PREFIX "Bad issue date: %s\n", issue_date);
			exit(EXIT_BAD_DATE);
		}
	}

	xmlSetProp(issueDate, BAD_CAST "year", BAD_CAST year_s);
	xmlSetProp(issueDate, BAD_CAST "month", BAD_CAST month_s);
	xmlSetProp(issueDate, BAD_CAST "day", BAD_CAST day_s);
}

xmlDocPtr xml_skeleton(const char *dmtype, enum issue iss)
{
	unsigned char *xml = NULL;
	unsigned int len;

	if (strcmp(dmtype, "") == 0) {
		fprintf(stderr, ERR_PREFIX "No schema defined for info code %s%s.\n", infoCode, infoCodeVariant);
		exit(EXIT_UNKNOWN_DMTYPE);
	} else if (template_dir) {
		char src[PATH_MAX];
		sprintf(src, "%s/%s.xml", template_dir, dmtype);

		if (access(src, F_OK) == -1) {
			fprintf(stderr, ERR_PREFIX "No schema %s in template directory \"%s\".\n", dmtype, template_dir);
			exit(EXIT_UNKNOWN_DMTYPE);
		}

		return xmlReadFile(src, NULL, 0);
	} else if (strcmp(dmtype, "descript") == 0) {
		switch (iss) {
			case ISS_20:
			case ISS_21:
			case ISS_22:
			case ISS_23:
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_descript_xml;
				len = templates_42_descript_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "proced") == 0) {
		switch (iss) {
			case ISS_20:
			case ISS_21:
			case ISS_22:
			case ISS_23:
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_proced_xml;
				len = templates_42_proced_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "frontmatter") == 0) {
		switch (iss) {
			case ISS_41:
			case ISS_42:
				xml = templates_42_frontmatter_xml;
				len = templates_42_frontmatter_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "brex") == 0) {
		switch (iss) {
			case ISS_22:
			case ISS_23:
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				if (maint_sns) {
					struct inmem_xml res;
					res = maint_sns_xml();
					xml = res.xml;
					len = res.len;
				} else {
					xml = templates_42_brex_xml;
					len = templates_42_brex_xml_len;
				}
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "brdoc") == 0) {
		switch (iss) {
			case ISS_42:
				xml = templates_42_brdoc_xml;
				len = templates_42_brdoc_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "appliccrossreftable") == 0) {
		switch (iss) {
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_appliccrossreftable_xml;
				len = templates_42_appliccrossreftable_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "prdcrossreftable") == 0) {
		switch (iss) {
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_prdcrossreftable_xml;
				len = templates_42_prdcrossreftable_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "condcrossreftable") == 0) {
		switch (iss) {
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_condcrossreftable_xml;
				len = templates_42_condcrossreftable_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "comrep") == 0) {
		switch (iss) {
			case ISS_41:
			case ISS_42:
				xml = templates_42_comrep_xml;
				len = templates_42_comrep_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "process") == 0) {
		switch (iss) {
			case ISS_20:
			case ISS_21:
			case ISS_22:
			case ISS_23:
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_process_xml;
				len = templates_42_process_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "ipd") == 0) {
		switch (iss) {
			case ISS_20:
			case ISS_21:
			case ISS_22:
			case ISS_23:
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_ipd_xml;
				len = templates_42_ipd_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "fault") == 0) {
		switch (iss) {
			case ISS_20:
			case ISS_21:
			case ISS_22:
			case ISS_23:
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_fault_xml;
				len = templates_42_fault_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "checklist") == 0) {
		switch (iss) {
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_checklist_xml;
				len = templates_42_checklist_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "learning") == 0) {
		switch (iss) {
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_learning_xml;
				len = templates_42_learning_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "container") == 0) {
		switch (iss) {
			case ISS_23:
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_container_xml;
				len = templates_42_container_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "crew") == 0) {
		switch (iss) {
			case ISS_20:
			case ISS_21:
			case ISS_22:
			case ISS_23:
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_crew_xml;
				len = templates_42_crew_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "sb") == 0) {
		switch (iss) {
			case ISS_41:
			case ISS_42:
				xml = templates_42_sb_xml;
				len = templates_42_sb_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "schedul") == 0) {
		switch (iss) {
			case ISS_20:
			case ISS_21:
			case ISS_22:
			case ISS_23:
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_schedul_xml;
				len = templates_42_schedul_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "wrngdata") == 0) {
		switch (iss) {
			case ISS_20:
			case ISS_21:
			case ISS_22:
			case ISS_23:
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_wrngdata_xml;
				len = templates_42_wrngdata_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "wrngflds") == 0) {
		switch (iss) {
			case ISS_20:
			case ISS_21:
			case ISS_22:
			case ISS_23:
			case ISS_30:
			case ISS_40:
			case ISS_41:
			case ISS_42:
				xml = templates_42_wrngflds_xml;
				len = templates_42_wrngflds_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "scocontent") == 0) {
		switch (iss) {
			case ISS_41:
			case ISS_42:
				xml = templates_42_scocontent_xml;
				len = templates_42_scocontent_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "techrep") == 0) {
		switch (iss) {
			case ISS_23:
			case ISS_30:
			case ISS_40:
				xml = templates_42_techrep_xml;
				len = templates_42_techrep_xml_len;
				break;
			default:
				break;
		}
	} else {
		fprintf(stderr, ERR_PREFIX "Unknown schema %s\n", dmtype);
		exit(EXIT_UNKNOWN_DMTYPE);
	}

	if (!xml) {
		fprintf(stderr, ERR_PREFIX "No schema %s for issue %s\n", dmtype, issue_name(iss));
		exit(EXIT_UNKNOWN_DMTYPE);
	}

	return xmlReadMemory((const char *) xml, len, NULL, NULL, 0);
}

xmlDocPtr toissue(xmlDocPtr doc, enum issue iss)
{
	xsltStylesheetPtr style;
	xmlDocPtr styledoc, res, orig;
	xmlNodePtr old;
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
		case ISS_23:
			xml = ___common_42to23_xsl;
			len = ___common_42to23_xsl_len;
			break;
		case ISS_22:
			xml = ___common_42to22_xsl;
			len = ___common_42to22_xsl_len;
			break;
		case ISS_21:
			xml = ___common_42to21_xsl;
			len = ___common_42to21_xsl_len;
			break;
		case ISS_20:
			xml = ___common_42to20_xsl;
			len = ___common_42to20_xsl_len;
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

	old = xmlDocSetRootElement(orig, xmlCopyNode(xmlDocGetRootElement(res), 1));

	xmlFreeNode(old);
	xmlFreeDoc(res);

	return orig;
}

void process_dmtypes_xml(xmlDocPtr defaults_xml)
{
	xmlNodePtr cur;

	for (cur = xmlDocGetRootElement(defaults_xml)->children; cur; cur = cur->next) {
		char *def_key, *def_val, *infname;
		char code[4], variant[2];
		int p;

		if (cur->type != XML_ELEMENT_NODE) continue;
		if (!xmlHasProp(cur, BAD_CAST "infoCode")) continue;
		if (!xmlHasProp(cur, BAD_CAST "schema")) continue;

		def_key = (char *) xmlGetProp(cur, BAD_CAST "infoCode");
		def_val = (char *) xmlGetProp(cur, BAD_CAST "schema");
		infname = (char *) xmlGetProp(cur, BAD_CAST "infoName");

		p = sscanf(def_key, "%3s%1s", code, variant);

		if (strcmp(code, infoCode) == 0 &&
		    (p < 2 || strcmp(variant, infoCodeVariant) == 0) &&
		    strcmp(dmtype, "") == 0)
			strcpy(dmtype, def_val);

		if (infname && strcmp(code, infoCode) == 0 &&
		    (p < 2 || strcmp(variant, infoCodeVariant) == 0) &&
		    strcmp(infoName_content, "") == 0)
			strcpy(infoName_content, infname);

		xmlFree(def_key);
		xmlFree(def_val);
		xmlFree(infname);
	}
}

void set_remarks(xmlDocPtr doc, xmlChar *text)
{
	xmlNodePtr remarks;
	
	remarks = firstXPathNode(doc, NULL, "//remarks");

	if (text) {
		xmlNodePtr simplePara;
		simplePara = xmlNewChild(remarks, NULL, BAD_CAST "simplePara", NULL);
		xmlNodeSetContent(simplePara, text);
	} else {
		xmlUnlinkNode(remarks);
		xmlFreeNode(remarks);
	}
}

/* Dump the built-in dmtypes XML or text */
void print_dmtypes(void)
{
	printf("%.*s", dmtypes_xml_len, dmtypes_xml);
}
void print_dmtypes_txt(void)
{
	printf("%.*s", dmtypes_txt_len, dmtypes_txt);
}

/* Try reading the ISO language and country codes from the environment,
 * otherwise default to "und" (undetermined) for language and ZZ for
 * country.
 */
void set_env_lang(void)
{
	char *env, *lang, *lang_l, *lang_c;

	if (!(env = getenv("LANG"))) {
		if (strcmp(languageIsoCode, "") == 0) {
			strcpy(languageIsoCode, DEFAULT_LANGUAGE_ISO_CODE);
		}
		if (strcmp(countryIsoCode, "") == 0) {
			strcpy(countryIsoCode, DEFAULT_COUNTRY_ISO_CODE);
		}
		return;
	}

	lang = strdup(env);
	lang_l = strtok(lang, "_");
	lang_c = strtok(NULL, ".");

	if (strcmp(languageIsoCode, "") == 0) {
		if (lang_l) {
			strncpy(languageIsoCode, lang_l, 3);
		} else {
			strcpy(languageIsoCode, DEFAULT_LANGUAGE_ISO_CODE);
		}
	}
	if (strcmp(countryIsoCode, "") == 0) {
		if (lang_c) {
			strncpy(countryIsoCode, lang_c, 2);
		} else {
			strcpy(countryIsoCode, DEFAULT_COUNTRY_ISO_CODE);
		}
	}

	free(lang);
}

int main(int argc, char **argv)
{
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

	FILE *defaults;

	char defaults_fname[256] = "defaults";
	char dmtypes_fname[256] = "dmtypes";

	int i;
	int c;

	bool showprompts = false;
	char dmcode[256] = "";
	bool skipdmc = false;
	bool verbose = false;
	bool overwrite = false;
	char *out = NULL;
	bool tech_name_flag = false;
	bool no_overwrite_error = false;

	xmlDocPtr defaults_xml;

	while ((c = getopt(argc, argv, "pd:D:L:C:n:w:c:r:R:o:O:t:i:T:#:Ns:b:S:I:v$:@:fm:,.%:qM:Ph?")) != -1) {
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
			case 't': strcpy(techName_content, optarg); tech_name_flag = true; break;
			case 'i': strcpy(infoName_content, optarg); break;
			case 'T': strcpy(dmtype, optarg); break;
			case '#': strcpy(dmcode, optarg); skipdmc = true; break;
			case 'N': no_issue = true; no_issue_set = true; break;
			case 's': strcpy(schema, optarg); break;
			case 'b': strcpy(brex_dmcode, optarg); break;
			case 'S': sns_fname = strdup(optarg); break;
			case 'I': strcpy(issue_date, optarg); break;
			case 'v': verbose = true; break;
			case 'f': overwrite = true; break;
			case '$': issue = get_issue(optarg); break;
			case '@': out = strdup(optarg); break;
			case 'm': remarks = xmlStrdup(BAD_CAST optarg); break;
			case ',': print_dmtypes(); return 0;
			case '.': print_dmtypes_txt(); return 0;
			case '%': template_dir = strdup(optarg); break;
			case 'q': no_overwrite_error = true; break;
			case 'M': maint_sns = strdup(optarg); break;
			case 'P': sns_prev_title = true; sns_prev_title_set = true; break;
			case 'h':
			case '?': show_help(); return 0;
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
		char default_line[1024];

		while (fgets(default_line, 1024, defaults)) {
			char def_key[32], def_val[256];

			if (sscanf(default_line, "%31s %255[^\n]", def_key, def_val) != 2)
				continue;

			copy_default_value(def_key, def_val);
		}

		fclose(defaults);
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
			fprintf(stderr, ERR_PREFIX "Bad data module code: %s\n", dmcode);
			exit(EXIT_BAD_DMC);
		}
	}

	if (strcmp(dmtype, "") == 0 || strcmp(infoName_content, "") == 0) {
		if ((defaults_xml = xmlReadFile(dmtypes_fname, NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING))) {
			process_dmtypes_xml(defaults_xml);
			xmlFreeDoc(defaults_xml);
		} else if ((defaults = fopen(dmtypes_fname, "r"))) {
			char default_line[1024];

			while (fgets(default_line, 1024, defaults)) {
				char def_key[32], def_val[256], infname[256];
				int n;
				char code[4], variant[2];
				int p;

				n = sscanf(default_line, "%31s %255s %255[^\n]", def_key, def_val, infname);

				if (n < 2)
					continue;

				p = sscanf(def_key, "%3s%1s", code, variant);

				if (strcmp(code, infoCode) == 0 &&
				    (p < 2 || strcmp(variant, infoCodeVariant) == 0) &&
				    strcmp(dmtype, "") == 0)
					strcpy(dmtype, def_val);

				if (n == 3 && strcmp(code, infoCode) == 0 &&
				    (p < 2 || strcmp(variant, infoCodeVariant) == 0) &&
				    strcmp(infoName_content, "") == 0)
					strcpy(infoName_content, infname);
			}

			fclose(defaults);
		} else {
			defaults_xml = xmlReadMemory((const char *) dmtypes_xml, dmtypes_xml_len, NULL, NULL, 0);
			process_dmtypes_xml(defaults_xml);
			xmlFreeDoc(defaults_xml);
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

	if (strcmp(modelIdentCode, "") == 0 ||
	    strcmp(systemDiffCode, "") == 0 ||
	    strcmp(systemCode, "") == 0 ||
	    strcmp(subSystemCode, "") == 0 ||
	    strcmp(subSubSystemCode, "") == 0 ||
	    strcmp(assyCode, "") == 0 ||
	    strcmp(disassyCode, "") == 0 ||
	    strcmp(disassyCodeVariant, "") == 0 ||
	    strcmp(infoCode, "") == 0 ||
	    strcmp(infoCodeVariant, "") == 0 ||
	    strcmp(itemLocationCode, "") == 0) {

	    fprintf(stderr, ERR_PREFIX "Missing required DMC components: ");
	    fprintf(stderr, "DMC-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s\n",
		strcmp(modelIdentCode, "") == 0     ? "???" : modelIdentCode,
		strcmp(systemDiffCode, "") == 0     ? "???" : systemDiffCode,
		strcmp(systemCode, "") == 0         ? "???" : systemCode,
		strcmp(subSystemCode, "") == 0      ? "???" : subSystemCode,
		strcmp(subSubSystemCode, "") == 0   ? "???" : subSubSystemCode,
		strcmp(assyCode, "") == 0           ? "???" : assyCode,
		strcmp(disassyCode, "") == 0        ? "???" : disassyCode,
		strcmp(disassyCodeVariant, "") == 0 ? "???" : disassyCodeVariant,
		strcmp(infoCode, "") == 0           ? "???" : infoCode,
		strcmp(infoCodeVariant, "") == 0    ? "???" : infoCodeVariant,
		strcmp(itemLocationCode, "") == 0   ? "???" : itemLocationCode);

		exit(EXIT_BAD_DMC);
	}

	if (!tech_name_flag && (maint_sns || sns_fname)) {
		xmlDocPtr brex;
		brex = set_tech_from_sns();
		xmlFreeDoc(brex);
	}

	if (issue == NO_ISS) issue = DEFAULT_S1000D_ISSUE;
	if (strcmp(issueNumber, "") == 0) strcpy(issueNumber, "000");
	if (strcmp(inWork, "") == 0) strcpy(inWork, "01");
	if (strcmp(securityClassification, "") == 0) strcpy(securityClassification, "01");

	set_env_lang();
	for (i = 0; languageIsoCode[i]; ++i) {
		languageIsoCode[i] = tolower(languageIsoCode[i]);
	}
	for (i = 0; countryIsoCode[i]; ++i) {
		countryIsoCode[i] = toupper(countryIsoCode[i]);
	}

	dm = xml_skeleton(dmtype, issue);

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

	set_issue_date(issueDate);

	xmlSetProp(security, BAD_CAST "securityClassification", BAD_CAST securityClassification);

	xmlNodeSetContent(techName, BAD_CAST techName_content);

	if (strcmp(infoName_content, "") == 0) {
		xmlUnlinkNode(infoName);
		xmlFreeNode(infoName);
	} else {
		xmlNodeSetContent(infoName, BAD_CAST infoName_content);
	}

	responsiblePartnerCompany = find_child(dmStatus, "responsiblePartnerCompany");
	if (strcmp(responsiblePartnerCompany_enterpriseCode, "") != 0) {
		xmlSetProp(responsiblePartnerCompany, BAD_CAST "enterpriseCode", BAD_CAST responsiblePartnerCompany_enterpriseCode);
	}
	if (strcmp(responsiblePartnerCompany_enterpriseName, "") != 0) {
		xmlNewChild(responsiblePartnerCompany, NULL, BAD_CAST "enterpriseName", BAD_CAST responsiblePartnerCompany_enterpriseName);
	}

	originator = find_child(dmStatus, "originator");
	if (strcmp(originator_enterpriseCode, "") != 0) {
		xmlSetProp(originator, BAD_CAST "enterpriseCode", BAD_CAST originator_enterpriseCode);
	}
	if (strcmp(originator_enterpriseName, "") != 0) {
		xmlNewChild(originator, NULL, BAD_CAST "enterpriseName", BAD_CAST originator_enterpriseName);
	}

	set_remarks(dm, remarks);

	if (strcmp(brex_dmcode, "") != 0)
		set_brex(dm, brex_dmcode);

	for (i = 0; languageIsoCode[i]; ++i) languageIsoCode[i] = toupper(languageIsoCode[i]);

	if (strcmp(learnCode, "") != 0 && strcmp(learnEventCode, "") != 0) {
		sprintf(learn, "-%s%s", learnCode, learnEventCode);
	}

	if (!no_issue) {
		sprintf(iss, "_%s-%s", issueNumber, inWork);
	}

	if (issue < ISS_42) {
		if (strcmp(brex_dmcode, "") == 0) {
			switch (issue) {
				case ISS_22:
					set_brex(dm, ISS_22_DEFAULT_BREX);
					break;
				case ISS_23:
					set_brex(dm, ISS_23_DEFAULT_BREX);
					break;
				case ISS_30:
					set_brex(dm, ISS_30_DEFAULT_BREX);
					break;
				case ISS_40:
					set_brex(dm, ISS_40_DEFAULT_BREX);
					break;
				case ISS_41:
					set_brex(dm, ISS_41_DEFAULT_BREX);
					break;
				default:
					break;
			}
		}

		dm = toissue(dm, issue);
	}

	if (!out) {
		char dmc[MAX_DATAMODULE_CODE];

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

		out = strdup(dmc);
	}

	if (!overwrite && access(out, F_OK) != -1) {
		if (no_overwrite_error) return 0;
		fprintf(stderr, ERR_PREFIX "Data module already exists: %s\n", out);
		exit(EXIT_DM_EXISTS);
	}

	xmlSaveFile(out, dm);

	if (verbose)
		puts(out);

	free(out);
	free(template_dir);
	free(sns_fname);
	free(maint_sns);

	xmlFree(remarks);

	xmlFreeDoc(dm);

	xmlCleanupParser();
	xsltCleanupGlobals();

	return 0;
}
