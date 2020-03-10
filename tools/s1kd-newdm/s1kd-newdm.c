#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>

#include "templates.h"
#include "dmtypes.h"
#include "sns.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-newdm"
#define VERSION "3.1.1"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define E_BREX_NOT_FOUND ERR_PREFIX "Could not find BREX: %s\n"
#define E_BAD_TEMPL_DIR ERR_PREFIX "Cannot dump templates in directory: %s\n"
#define E_ENCODING_ERROR ERR_PREFIX "Error encoding path name.\n"
#define E_NO_SCHEMA ERR_PREFIX "No schema defined for information type %s%s-%s\n"
#define E_NO_SCHEMA_LEARN ERR_PREFIX "No schema defined for information type %s%s-%s-%s%s\n"

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
#define MAX_ISSUE_NUMBER		 5	+ 2
#define MAX_IN_WORK			 2	+ 2
#define MAX_SECURITY_CLASSIFICATION	 2	+ 2

#define MAX_DATAMODULE_CODE 256

#define MAX_ENTERPRISE_NAME 256
#define MAX_ENTERPRISE_CODE 7

#define MAX_TECH_NAME 256
#define MAX_INFO_NAME 256

#define EXIT_DM_EXISTS 1
#define EXIT_UNKNOWN_DMTYPE 2
#define EXIT_BAD_DMC 3
#define EXIT_BAD_BREX_DMC 4
#define EXIT_BAD_DATE 5
#define EXIT_BAD_ISSUE 6
#define EXIT_BAD_TEMPL_DIR 7
#define EXIT_ENCODING_ERROR 8
#define EXIT_OS_ERROR 9

static char modelIdentCode[MAX_MODEL_IDENT_CODE] = "";
static char systemDiffCode[MAX_SYSTEM_DIFF_CODE] = "";
static char systemCode[MAX_SYSTEM_CODE] = "";
static char subSystemCode[MAX_SUB_SYSTEM_CODE] = "";
static char subSubSystemCode[MAX_SUB_SUB_SYSTEM_CODE] = "";
static char assyCode[MAX_ASSY_CODE] = "";
static char disassyCode[MAX_DISASSY_CODE] = "";
static char disassyCodeVariant[MAX_DISASSY_CODE_VARIANT] = "";
static char infoCode[MAX_INFO_CODE] = "";
static char infoCodeVariant[MAX_INFO_CODE_VARIANT] = "";
static char itemLocationCode[MAX_ITEM_LOCATION_CODE] = "";
static char learnCode[MAX_LEARN_CODE] = "";
static char learnEventCode[MAX_LEARN_EVENT_CODE] = "";

static char languageIsoCode[MAX_LANGUAGE_ISO_CODE] = "";
static char countryIsoCode[MAX_COUNTRY_ISO_CODE] = "";

static char securityClassification[MAX_SECURITY_CLASSIFICATION] = "";

static char issueNumber[MAX_ISSUE_NUMBER] = "";
static char inWork[MAX_IN_WORK] = "";

static char responsiblePartnerCompany_enterpriseName[MAX_ENTERPRISE_NAME] = "";
static char originator_enterpriseName[MAX_ENTERPRISE_NAME] = "";

static char responsiblePartnerCompany_enterpriseCode[MAX_ENTERPRISE_CODE] = "";
static char originator_enterpriseCode[MAX_ENTERPRISE_CODE] = "";

static char techName_content[MAX_TECH_NAME] = "";
static char infoName_content[MAX_INFO_NAME] = "";
static xmlChar *info_name_variant = NULL;

static char dmtype[32] = "";

static char schema[PATH_MAX] = "";
static char brex_dmcode[PATH_MAX] = "";
static char *sns_fname = NULL;
static char *maint_sns = NULL;
static char issue_date[16] = "";
static xmlChar *issue_type = NULL;

static xmlChar *remarks = NULL;
static xmlChar *skill_level_code = NULL;

/* Omit the issue information from the object filename. */
static bool no_issue = false;
static bool no_issue_set = false;

static enum issue { NO_ISS, ISS_20, ISS_21, ISS_22, ISS_23, ISS_30, ISS_40, ISS_41, ISS_42, ISS_50 } issue = NO_ISS;

#define DEFAULT_S1000D_ISSUE ISS_50

#define ISS_22_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_23_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_30_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_40_DEFAULT_BREX "S1000D-A-04-10-0301-00A-022A-D"
#define ISS_41_DEFAULT_BREX "S1000D-E-04-10-0301-00A-022A-D"
#define ISS_42_DEFAULT_BREX "S1000D-F-04-10-0301-00A-022A-D"

/* ISO language and country codes if none can be determined. */
#define DEFAULT_LANGUAGE_ISO_CODE "und"
#define DEFAULT_COUNTRY_ISO_CODE "ZZ"

static char *template_dir = NULL;

/* Include the previous level of SNS title in a tech name. */
static bool sns_prev_title = false;
static bool sns_prev_title_set = false;

static bool no_info_name = false;

static char *act_dmcode = NULL;

#define BREX_INFOCODE_USE BAD_CAST "The information code used is not in the allowed set."

static enum issue get_issue(const char *iss)
{
	if (strcmp(iss, "5.0") == 0)
		return ISS_50;
	else if (strcmp(iss, "4.2") == 0)
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

static const char *issue_name(enum issue iss)
{
	switch (iss) {
		case ISS_50: return "5.0";
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

static void prompt(const char *prompt, char *str, int n)
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
			memcpy(str, temp, n - 1);
		}
	}
}

static xmlNodePtr find_child(xmlNodePtr parent, const char *name)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, name) == 0) {
			return cur;
		}
	}

	return NULL;
}

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [options]");
	puts("");
	puts("Options:");
	puts("  -$, --issue <issue>               Specify which S1000D issue to use.");
	puts("  -@, --out <path>                  Output to specified file or directory.");
	puts("  -%, --templates <dir>             Use templates in specified directory.");
	puts("  -~, --dump-templates <dir>        Dump default templates to a directory.");
	puts("  -,, --dump-dmtypes-xml            Dump default dmtypes XML.");
	puts("  -., --dump-dmtypes                Dump default dmtypes text file.");
	puts("  -!, --no-infoname                 Do not include an info name.");
	puts("  -B, --generate-brex-rules         Generate BREX rules from .defaults file.");
	puts("  -D, --dmtypes <dmtypes>           Specify .dmtypes file name.");
	puts("  -d, --defaults <defaults>         Specify .defaults file name.");
	puts("  -f, --overwrite                   Overwrite existing file.");
	puts("  -j, --brexmap <map>               Use a custom .brexmap file.");
	puts("  -M, --maintained-sns <SNS>        Use one of the maintained SNS.");
	puts("  -N, --omit-issue                  Omit issue/inwork from filename.");
	puts("  -P, --two-sns-levels              Include previous level of SNS in tech name.");
	puts("  -p, --prompt                      Prompt the user for each value.");
	puts("  -q, --quiet                       Don't report an error if file exists.");
	puts("  -S, --sns <BREX>                  Get tech name from BREX SNS.");
	puts("  -v, --verbose                     Print file name of new data module.");
	puts("  --version                         Show version information.");
	puts("");
	puts("In addition, the following pieces of meta data can be set:");
	puts("  -#, --code <code>                 Data module code");
	puts("  -a, --act <ACT>                   ACT data module code");
	puts("  -b, --brex <BREX>                 BREX data module code");
	puts("  -C, --country <country>           Country ISO code");
	puts("  -c, --security <sec>              Security classification");
	puts("  -I, --date <date>                 Issue date");
	puts("  -i, --infoname <info>             Info name");
	puts("  -k, --skill <skill>               Skill level");
	puts("  -L, --language <lang>             Language ISO code");
	puts("  -m, --remarks <remarks>           Remarks");
	puts("  -n, --issno <iss>                 Issue number");
	puts("  -O, --origcode <CAGE>             Originator CAGE code.");
	puts("  -o, --origname <orig>             Originator enterprise name");
	puts("  -R, --rpccode <CAGE>              Responsible partner company CAGE code.");
	puts("  -r, --rpcname <RPC>               Responsible partner company enterprise name");
	puts("  -s, --schema <schema>             Schema");
	puts("  -T, --type <type>                 DM type (descript, proced, frontmatter, etc.)");
	puts("  -t, --techname <tech>             Tech name");
	puts("  -V, --infoname-variant <variant>  Info name variant");
	puts("  -w, --inwork <inwork>             Inwork issue");
	puts("  -z, --issue-type <type>           Issue type");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

static void copy_default_value(const char *key, const char *val)
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
	else if (strcmp(key, "infoName") == 0 && strcmp(infoName_content, "") == 0 && !no_info_name)
		strncpy(infoName_content, val, MAX_INFO_CODE - 2);
	else if (strcmp(key, "infoNameVariant") == 0 && !info_name_variant)
		info_name_variant = xmlStrdup(BAD_CAST val);
	else if (strcmp(key, "schema") == 0 && strcmp(schema, "") == 0)
		strncpy(schema, val, PATH_MAX - 1);
	else if (strcmp(key, "brex") == 0 && strcmp(brex_dmcode, "") == 0)
		strncpy(brex_dmcode, val, 255);
	else if (strcmp(key, "sns") == 0 && !sns_fname)
		sns_fname = strdup(val);
	else if (strcmp(key, "issue") == 0 && issue == NO_ISS)
		issue = get_issue(val);
	else if (strcmp(key, "remarks") == 0 && !remarks)
		remarks = xmlStrdup(BAD_CAST val);
	else if (strcmp(key, "templates") == 0 && !template_dir)
		template_dir = strdup(val);
	else if (strcmp(key, "maintainedSns") == 0 && !maint_sns)
		maint_sns = strdup(val);
	else if (strcmp(key, "includePrevSnsTitle") == 0 && !sns_prev_title_set)
		sns_prev_title = strcasecmp(val, "true") == 0;
	else if (strcmp(key, "skillLevelCode") == 0 && !skill_level_code)
		skill_level_code = xmlStrdup(BAD_CAST val);
	else if (strcmp(key, "act") == 0 && !act_dmcode)
		act_dmcode = strdup(val);
	else if (strcmp(key, "issueType") == 0 && !issue_type)
		issue_type = xmlStrdup(BAD_CAST val);
}

static xmlNodePtr firstXPathNode(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
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

static void set_dmcode(xmlNodePtr dmCode, const char *fname)
{
	int n, offset;
	char *path, *code;

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

	path = strdup(fname);
	code = basename(path);

	offset = strncmp(code, "DMC-", 4) == 0 ? 4 : 0;

	n = sscanf(code + offset, "%14[^-]-%4[^-]-%3[^-]-%c%c-%4[^-]-%2s%3[^-]-%3s%c-%c-%3s%1s",
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

	free(path);
}

static void set_brex(xmlDocPtr doc, const char *fname)
{
	xmlNodePtr dmCode;
	dmCode = firstXPathNode(doc, NULL, "//brexDmRef/dmRef/dmRefIdent/dmCode");
	set_dmcode(dmCode, fname);
}

static void set_act(xmlDocPtr doc, const char *fname)
{
	xmlNodePtr dmCode;
	dmCode = firstXPathNode(doc, NULL, "//applicCrossRefTableRef/dmRef/dmRefIdent/dmCode");
	set_dmcode(dmCode, fname);
}

static void unset_act(xmlDocPtr doc)
{
	xmlNodePtr dmCode;
	dmCode = firstXPathNode(doc, NULL, "//applicCrossRefTableRef");
	xmlUnlinkNode(dmCode);
	xmlFreeNode(dmCode);
}

#define SNS_XPATH_1 "//snsSystem[snsCode='%s']/snsSubSystem[snsCode='%s']/snsSubSubSystem[snsCode='%s']/snsAssy[snsCode='%s']/snsTitle"
#define SNS_XPATH_2 "//snsSystem[snsCode='%s']/snsSubSystem[snsCode='%s']/snsSubSubSystem[snsCode='%s']/snsTitle"
#define SNS_XPATH_3 "//snsSystem[snsCode='%s']/snsSubSystem[snsCode='%s']/snsTitle"
#define SNS_XPATH_4 "//snsSystem[snsCode='%s']/snsTitle"

static void set_sns_title(xmlNodePtr snsTitle)
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

/* Find the filename of the latest version of a BREX DM by its code. */
static bool find_brex_file(char *dst, const char *dir, const char *code)
{
	char s[PATH_MAX];

	if (strncmp(code, "DMC-", 4) == 0) {
		if (snprintf(s, PATH_MAX, "%s", code) < 0) {
			fprintf(stderr, E_ENCODING_ERROR);
			exit(EXIT_ENCODING_ERROR);
		}
	} else {
		if (snprintf(s, PATH_MAX, "DMC-%s", code) < 0) {
			fprintf(stderr, E_ENCODING_ERROR);
			exit(EXIT_ENCODING_ERROR);
		}
	}

	return find_csdb_object(dst, dir, s, NULL, true);
}

struct inmem_xml {
	unsigned char *xml;
	unsigned int len;
};

/* Map maintained SNS title to XML template. */
static struct inmem_xml maint_sns_xml(void)
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


static xmlDocPtr maint_sns_doc(void)
{
	struct inmem_xml xml;

	xml = maint_sns_xml();

	return read_xml_mem((const char *) xml.xml, xml.len);
}

static xmlDocPtr set_tech_from_sns(const char *dir)
{
	xmlDocPtr brex = NULL;
	char xpath[256];
	xmlNodePtr snsTitle;
	char fname[PATH_MAX];

	if (maint_sns) {
		brex = maint_sns_doc();
	} else if (sns_fname && find_brex_file(fname, dir, sns_fname)) {
		brex = read_xml_doc(fname);
	} else if (strcmp(brex_dmcode, "") != 0 && find_brex_file(fname, dir, brex_dmcode)) {
		brex = read_xml_doc(fname);
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

static void set_issue_date(xmlNodePtr issueDate)
{
	char year_s[5], month_s[3], day_s[3];

	if (strcmp(issue_date, "") == 0) {
		time_t now;
		struct tm *local;
		unsigned short year, month, day;

		time(&now);
		local = localtime(&now);

		year = local->tm_year + 1900;
		month = local->tm_mon + 1;
		day = local->tm_mday;

		if (snprintf(day_s, 3, "%.2u", day) < 0 ||
		    snprintf(month_s, 3, "%.2u", month) < 0 ||
		    snprintf(year_s, 5, "%u", year) < 0)
			exit(EXIT_BAD_DATE);
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

static xmlDocPtr xml_skeleton(const char *dmtype, enum issue iss)
{
	unsigned char *xml = NULL;
	unsigned int len;

	if (strcmp(dmtype, "") == 0) {
		if (strcmp(learnCode, "") == 0) {
			fprintf(stderr, E_NO_SCHEMA, infoCode, infoCodeVariant,
				itemLocationCode);
		} else {
			fprintf(stderr, E_NO_SCHEMA_LEARN, infoCode, infoCodeVariant,
				itemLocationCode, learnCode, learnEventCode);
		}
		exit(EXIT_UNKNOWN_DMTYPE);
	} else if (template_dir) {
		char src[PATH_MAX];
		sprintf(src, "%s/%s.xml", template_dir, dmtype);

		if (access(src, F_OK) == -1) {
			fprintf(stderr, ERR_PREFIX "No schema %s in template directory \"%s\".\n", dmtype, template_dir);
			exit(EXIT_UNKNOWN_DMTYPE);
		}

		return read_xml_doc(src);
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
			case ISS_50:
				xml = templates_descript_xml;
				len = templates_descript_xml_len;
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
			case ISS_50:
				xml = templates_proced_xml;
				len = templates_proced_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "frontmatter") == 0) {
		switch (iss) {
			case ISS_41:
			case ISS_42:
			case ISS_50:
				xml = templates_frontmatter_xml;
				len = templates_frontmatter_xml_len;
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
			case ISS_50:
				if (maint_sns) {
					struct inmem_xml res;
					res = maint_sns_xml();
					xml = res.xml;
					len = res.len;
				} else {
					xml = templates_brex_xml;
					len = templates_brex_xml_len;
				}
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "brdoc") == 0) {
		switch (iss) {
			case ISS_42:
			case ISS_50:
				xml = templates_brdoc_xml;
				len = templates_brdoc_xml_len;
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
			case ISS_50:
				xml = templates_appliccrossreftable_xml;
				len = templates_appliccrossreftable_xml_len;
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
			case ISS_50:
				xml = templates_prdcrossreftable_xml;
				len = templates_prdcrossreftable_xml_len;
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
			case ISS_50:
				xml = templates_condcrossreftable_xml;
				len = templates_condcrossreftable_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "comrep") == 0) {
		switch (iss) {
			case ISS_41:
			case ISS_42:
			case ISS_50:
				xml = templates_comrep_xml;
				len = templates_comrep_xml_len;
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
			case ISS_50:
				xml = templates_process_xml;
				len = templates_process_xml_len;
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
			case ISS_50:
				xml = templates_ipd_xml;
				len = templates_ipd_xml_len;
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
			case ISS_50:
				xml = templates_fault_xml;
				len = templates_fault_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "checklist") == 0) {
		switch (iss) {
			case ISS_40:
			case ISS_41:
			case ISS_42:
			case ISS_50:
				xml = templates_checklist_xml;
				len = templates_checklist_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "learning") == 0) {
		switch (iss) {
			case ISS_40:
			case ISS_41:
			case ISS_42:
			case ISS_50:
				xml = templates_learning_xml;
				len = templates_learning_xml_len;
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
			case ISS_50:
				xml = templates_container_xml;
				len = templates_container_xml_len;
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
			case ISS_50:
				xml = templates_crew_xml;
				len = templates_crew_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "sb") == 0) {
		switch (iss) {
			case ISS_41:
			case ISS_42:
			case ISS_50:
				xml = templates_sb_xml;
				len = templates_sb_xml_len;
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
			case ISS_50:
				xml = templates_schedul_xml;
				len = templates_schedul_xml_len;
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
			case ISS_50:
				xml = templates_wrngdata_xml;
				len = templates_wrngdata_xml_len;
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
			case ISS_50:
				xml = templates_wrngflds_xml;
				len = templates_wrngflds_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "scocontent") == 0) {
		switch (iss) {
			case ISS_41:
			case ISS_42:
			case ISS_50:
				xml = templates_scocontent_xml;
				len = templates_scocontent_xml_len;
				break;
			default:
				break;
		}
	} else if (strcmp(dmtype, "techrep") == 0) {
		switch (iss) {
			case ISS_23:
			case ISS_30:
			case ISS_40:
				xml = templates_techrep_xml;
				len = templates_techrep_xml_len;
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

	return read_xml_mem((const char *) xml, len);
}

static xmlDocPtr toissue(xmlDocPtr doc, enum issue iss)
{
	xsltStylesheetPtr style;
	xmlDocPtr styledoc, res, orig;
	xmlNodePtr old;
	unsigned char *xml = NULL;
	unsigned int len;

	switch (iss) {
		case ISS_42:
			xml = ___common_to42_xsl;
			len = ___common_to42_xsl_len;
			break;
		case ISS_41:
			xml = ___common_to41_xsl;
			len = ___common_to41_xsl_len;
			break;
		case ISS_40:
			xml = ___common_to40_xsl;
			len = ___common_to40_xsl_len;
			break;
		case ISS_30:
			xml = ___common_to30_xsl;
			len = ___common_to30_xsl_len;
			break;
		case ISS_23:
			xml = ___common_to23_xsl;
			len = ___common_to23_xsl_len;
			break;
		case ISS_22:
			xml = ___common_to22_xsl;
			len = ___common_to22_xsl_len;
			break;
		case ISS_21:
			xml = ___common_to21_xsl;
			len = ___common_to21_xsl_len;
			break;
		case ISS_20:
			xml = ___common_to20_xsl;
			len = ___common_to20_xsl_len;
			break;
		default:
			return NULL;
	}

	orig = xmlCopyDoc(doc, 1);
			
	styledoc = read_xml_mem((const char *) xml, len);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);

	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	old = xmlDocSetRootElement(orig, xmlCopyNode(xmlDocGetRootElement(res), 1));

	xmlFreeNode(old);
	xmlFreeDoc(res);

	return orig;
}

static void add_dmtypes_brex_val(xmlNodePtr rules, const char *key, const char *val)
{
	xmlNodePtr objval;
	xmlChar *name;
	objval = xmlNewChild(rules->children, NULL, BAD_CAST "objectValue", NULL);
	xmlSetProp(objval, BAD_CAST "valueAllowed", BAD_CAST key);
	name = xmlEncodeEntitiesReentrant(NULL, BAD_CAST val);
	xmlNodeSetContent(objval, name);
	xmlFree(name);
}

static void process_dmtypes_xml(xmlDocPtr defaults_xml, xmlNodePtr brex_rules)
{
	xmlNodePtr cur;

	for (cur = xmlDocGetRootElement(defaults_xml)->children; cur; cur = cur->next) {
		char *def_key, *def_val, *infname;
		xmlChar *infnamev;
		char code[4], variant[2], itemloc[2], learn[4], levent[2];
		int p;

		if (cur->type != XML_ELEMENT_NODE) continue;
		if (!xmlHasProp(cur, BAD_CAST "infoCode")) continue;
		if (!xmlHasProp(cur, BAD_CAST "schema")) continue;

		def_key  = (char *) xmlGetProp(cur, BAD_CAST "infoCode");
		def_val  = (char *) xmlGetProp(cur, BAD_CAST "schema");
		infname  = (char *) xmlGetProp(cur, BAD_CAST "infoName");
		infnamev = xmlGetProp(cur, BAD_CAST "infoNameVariant");

		p = sscanf(def_key, "%3s%1s-%1s-%3s%1s", code, variant, itemloc, learn, levent);

		/* Get schema */
		if (strcmp(dmtype, "") == 0 &&
		    strcmp(code, infoCode) == 0 &&
		    (p < 2 || strcmp(variant, "*") == 0 || strcmp(variant, infoCodeVariant) == 0) &&
		    (p < 3 || strcmp(itemloc, "*") == 0 || strcmp(itemloc, itemLocationCode) == 0) &&
		    (p < 4 || strcmp(learn, "***") == 0   || strcmp(learn, learnCode) == 0) &&
		    (p < 5 || strcmp(levent, "*") == 0  || strcmp(levent, learnEventCode) == 0)) {
			strcpy(dmtype, def_val);
		}

		/* Get info name */
		if (infname &&
		    strcmp(infoName_content, "") == 0 &&
		    !no_info_name &&
		    strcmp(code, infoCode) == 0 &&
		    (p < 2 || strcmp(variant, "*") == 0 || strcmp(variant, infoCodeVariant) == 0) &&
		    (p < 3 || strcmp(itemloc, "*") == 0 || strcmp(itemloc, itemLocationCode) == 0) &&
		    (p < 4 || strcmp(learn, "***") == 0   || strcmp(learn, learnCode) == 0) &&
		    (p < 5 || strcmp(levent, "*") == 0  || strcmp(levent, learnEventCode) == 0)) {
			strcpy(infoName_content, infname);

			if (infnamev && !info_name_variant) {
				info_name_variant = xmlStrdup(infnamev);
			}
		}

		if (brex_rules) {
			add_dmtypes_brex_val(brex_rules, def_key, infname);
		}

		xmlFree(def_key);
		xmlFree(def_val);
		xmlFree(infname);
		xmlFree(infnamev);
	}
}

static void set_remarks(xmlDocPtr doc, xmlChar *text)
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

static void set_skill_level(xmlDocPtr doc, xmlChar *code)
{
	xmlNodePtr skill_level;

	skill_level = firstXPathNode(doc, NULL, "//skillLevel");

	if (code) {
		xmlSetProp(skill_level, BAD_CAST "skillLevelCode", code);
	} else {
		xmlUnlinkNode(skill_level);
		xmlFreeNode(skill_level);
	}
}

/* Dump the built-in dmtypes XML or text */
static void print_dmtypes(void)
{
	printf("%.*s", dmtypes_xml_len, dmtypes_xml);
}
static void print_dmtypes_txt(void)
{
	printf("%.*s", dmtypes_txt_len, dmtypes_txt);
}

/* Try reading the ISO language and country codes from the environment,
 * otherwise default to "und" (undetermined) for language and ZZ for
 * country.
 */
static void set_env_lang(void)
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

static void add_brex_rule(xmlNodePtr rules, xmlDocPtr brexmap, const char *key, const char *val)
{
	xmlNodePtr rule, objpath, objval;
	char *path;
	xmlChar use[256];

	/* Read the object path from the .brexmap file. */
	xmlStrPrintf(use, 256, "//default[@ident='%s']/@path", key);
	path = (char *) xmlNodeGetContent(firstXPathNode(brexmap, NULL, (char *) use));
	if (!path) {
		return;
	}

	xmlStrPrintf(use, 256, "%s must be %s", key, val);

	rule = xmlNewChild(rules, NULL, BAD_CAST "structureObjectRule", NULL);
	objpath = xmlNewChild(rule, NULL, BAD_CAST "objectPath", BAD_CAST path);
	xmlNewChild(rule, NULL, BAD_CAST "objectUse", use);
	objval = xmlNewChild(rule, NULL, BAD_CAST "objectValue", NULL);

	xmlSetProp(objpath, BAD_CAST "allowedObjectFlag", BAD_CAST "2");
	xmlSetProp(objval, BAD_CAST "valueAllowed", BAD_CAST val);

	xmlFree(path);
}

static xmlDocPtr read_default_brexmap(void)
{
	char fname[PATH_MAX];

	if (find_config(fname, DEFAULT_BREXMAP_FNAME)) {
		return read_xml_doc(fname);
	} else {
		return read_xml_mem((const char *) ___common_brexmap_xml, ___common_brexmap_xml_len);
	}
}

static void dump_templ(const char *fname, const unsigned char *xml, const unsigned int len)
{
	FILE *f;
	f = fopen(fname, "w");
	fprintf(f, "%.*s", len, xml);
	fclose(f);
}

static void dump_templates(const char *path)
{
	if (access(path, W_OK) == -1 || chdir(path)) {
		fprintf(stderr, E_BAD_TEMPL_DIR, path);
		exit(EXIT_BAD_TEMPL_DIR);
	}

	dump_templ("appliccrossreftable.xml",
		templates_appliccrossreftable_xml,
		templates_appliccrossreftable_xml_len);
	dump_templ("brdoc.xml",
		templates_brdoc_xml,
		templates_brdoc_xml_len);
	dump_templ("brex.xml",
		templates_brex_xml,
		templates_brex_xml_len);
	dump_templ("checklist.xml",
		templates_checklist_xml,
		templates_checklist_xml_len);
	dump_templ("comrep.xml",
		templates_comrep_xml,
		templates_comrep_xml_len);
	dump_templ("condcrossreftable.xml",
		templates_condcrossreftable_xml,
		templates_condcrossreftable_xml_len);
	dump_templ("container.xml",
		templates_container_xml,
		templates_container_xml_len);
	dump_templ("crew.xml",
		templates_crew_xml,
		templates_crew_xml_len);
	dump_templ("descript.xml",
		templates_descript_xml,
		templates_descript_xml_len);
	dump_templ("fault.xml",
		templates_fault_xml,
		templates_fault_xml_len);
	dump_templ("frontmatter.xml",
		templates_frontmatter_xml,
		templates_frontmatter_xml_len);
	dump_templ("ipd.xml",
		templates_ipd_xml,
		templates_ipd_xml_len);
	dump_templ("learning.xml",
		templates_learning_xml,
		templates_learning_xml_len);
	dump_templ("prdcrossreftable.xml",
		templates_prdcrossreftable_xml,
		templates_prdcrossreftable_xml_len);
	dump_templ("proced.xml",
		templates_proced_xml,
		templates_proced_xml_len);
	dump_templ("process.xml",
		templates_process_xml,
		templates_process_xml_len);
	dump_templ("sb.xml",
		templates_sb_xml,
		templates_sb_xml_len);
	dump_templ("schedul.xml",
		templates_schedul_xml,
		templates_schedul_xml_len);
	dump_templ("scocontent.xml",
		templates_scocontent_xml,
		templates_scocontent_xml_len);
	dump_templ("techrep.xml",
		templates_techrep_xml,
		templates_techrep_xml_len);
	dump_templ("wrngdata.xml",
		templates_wrngdata_xml,
		templates_wrngdata_xml_len);
	dump_templ("wrngflds.xml",
		templates_wrngflds_xml,
		templates_wrngflds_xml_len);
}

/* Generate a random code. */
static void random_code(char *dst, size_t n, const char *modelid)
{
	static const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVXWYZ0123456789";

	if (strcmp(modelid, "") != 0) {
		snprintf(dst, n, "%s-%c%c%c%c-%c%c-%c%c-%c%c%c%c-%c%c%c%c%c-000A-D",
			modelid,
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)]);
	} else {
		snprintf(dst, n, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c-%c%c%c%c-%c%c-%c%c-%c%c%c%c-%c%c%c%c%c-000A-D",
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)],
			alphanum[rand() % (sizeof(alphanum) - 1)]);
	}
}

int main(int argc, char **argv)
{
	char learn[8] = "";
	char iss[16] = "";

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
	xmlNode *infoNameVariant;
	xmlNode *responsiblePartnerCompany;
	xmlNode *originator;

	FILE *defaults;

	char defaults_fname[PATH_MAX];
	char dmtypes_fname[PATH_MAX];
	bool custom_defaults = false;
	bool custom_dmtypes = false;

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
	xmlNodePtr brex_rules = NULL;
	xmlDocPtr brexmap = NULL;

	char *defaults_dir_str;
	char *defaults_dir;

	char *outdir = NULL;

	const char *sopts = "a:pd:D:L:C:n:w:c:r:R:o:O:t:i:T:#:Ns:Bb:S:I:v$:@:fm:,.%:qM:P!k:j:~:z:V:h?";
	struct option lopts[] = {
		{"version"               , no_argument      , 0, 0},
		{"help"                  , no_argument      , 0, 'h'},
		{"act"                   , required_argument, 0, 'a'},
		{"prompt"                , no_argument      , 0, 'p'},
		{"defaults"              , required_argument, 0, 'd'},
		{"dmtypes"               , required_argument, 0, 'D'},
		{"language"              , required_argument, 0, 'L'},
		{"country"               , required_argument, 0, 'C'},
		{"issno"                 , required_argument, 0, 'n'},
		{"inwork"                , required_argument, 0, 'w'},
		{"security"              , required_argument, 0, 'c'},
		{"rpcname"               , required_argument, 0, 'r'},
		{"rpccode"               , required_argument, 0, 'R'},
		{"orgname"               , required_argument, 0, 'o'},
		{"orgcode"               , required_argument, 0, 'O'},
		{"techname"              , required_argument, 0, 't'},
		{"infoname"              , required_argument, 0, 'i'},
		{"type"                  , required_argument, 0, 'T'},
		{"code"                  , required_argument, 0, '#'},
		{"omit-issue"            , no_argument      , 0, 'N'},
		{"schema"                , required_argument, 0, 's'},
		{"generate-brex-rules"   , no_argument      , 0, 'B'},
		{"brex"                  , required_argument, 0, 'b'},
		{"sns"                   , required_argument, 0, 'S'},
		{"date"                  , required_argument, 0, 'I'},
		{"verbose"               , no_argument      , 0, 'v'},
		{"overwrite"             , no_argument      , 0, 'f'},
		{"issue"                 , required_argument, 0, '$'},
		{"out"                   , required_argument, 0, '@'},
		{"remarks"               , required_argument, 0, 'm'},
		{"dump-dmtypes"          , no_argument      , 0, '.'},
		{"dump-dmtypes-xml"      , no_argument      , 0, ','},
		{"templates"             , required_argument, 0, '%'},
		{"quiet"                 , no_argument      , 0, 'q'},
		{"maintained-sns"        , required_argument, 0, 'M'},
		{"two-sns-levels"        , no_argument      , 0, 'P'},
		{"no-infoname"           , no_argument      , 0, '!'},
		{"skill"                 , required_argument, 0, 'k'},
		{"brexmap"               , required_argument, 0, 'j'},
		{"dump-templates"        , required_argument, 0, '~'},
		{"issue-type"            , required_argument, 0, 'z'},
		{"infoname-variant"      , required_argument, 0, 'V'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	srand(time(NULL) + getpid());

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'a': act_dmcode = strdup(optarg); break;
			case 'p': showprompts = true; break;
			case 'd': strcpy(defaults_fname, optarg); custom_defaults = true; break;
			case 'D': strcpy(dmtypes_fname, optarg); custom_dmtypes = true; break;
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
			case '#': if (strchr(optarg, '-')) {
					  strncpy(dmcode, optarg, 255);
				  } else {
					  random_code(dmcode, 256, optarg);
				  }
				  skipdmc = true;
				  break;
			case 'N': no_issue = true; no_issue_set = true; break;
			case 's': strcpy(schema, optarg); break;
			case 'B': if (!brex_rules) brex_rules = xmlNewNode(NULL, BAD_CAST "structureObjectRuleGroup"); break;
			case 'b': strcpy(brex_dmcode, optarg); break;
			case 'S': sns_fname = strdup(optarg); break;
			case 'I': strcpy(issue_date, optarg); break;
			case 'V': info_name_variant = xmlStrdup(BAD_CAST optarg); break;
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
			case '!': no_info_name = true; break;
			case 'k': skill_level_code = xmlStrdup(BAD_CAST optarg); break;
			case 'j': if (!brexmap) brexmap = read_xml_doc(optarg); break;
			case '~': dump_templates(optarg); return 0;
			case 'z': issue_type = xmlStrdup(BAD_CAST optarg); break;
			case 'h':
			case '?': show_help(); return 0;
		}
	}

	if (!custom_defaults) {
		find_config(defaults_fname, DEFAULT_DEFAULTS_FNAME);
	}
	if (!custom_dmtypes) {
		find_config(dmtypes_fname, DEFAULT_DMTYPES_FNAME);
	}

	defaults_dir_str = strdup(defaults_fname);
	defaults_dir = dirname(defaults_dir_str);

	if (!brexmap) {
		brexmap = read_default_brexmap();
	}

	if (brex_rules) {
		xmlNodePtr dmtypes_brex_rule, objpath;
		xmlChar *path;

		path = xmlNodeGetContent(firstXPathNode(brexmap, NULL, "//dmtypes/@path"));

		dmtypes_brex_rule = xmlNewChild(brex_rules, NULL, BAD_CAST "structureObjectRule", NULL);

		objpath = xmlNewChild(dmtypes_brex_rule, NULL, BAD_CAST "objectPath", path);
		xmlSetProp(objpath, BAD_CAST "allowedObjectFlag", BAD_CAST "2");

		xmlNewChild(dmtypes_brex_rule, NULL, BAD_CAST "objectUse", BREX_INFOCODE_USE);

		xmlFree(path);
	}

	if ((defaults_xml = read_xml_doc(defaults_fname))) {
		xmlNodePtr cur;

		for (cur = xmlDocGetRootElement(defaults_xml)->children; cur; cur = cur->next) {
			char *def_key, *def_val;

			if (cur->type != XML_ELEMENT_NODE) continue;
			if (!xmlHasProp(cur, BAD_CAST "ident")) continue;
			if (!xmlHasProp(cur, BAD_CAST "value")) continue;

			def_key = (char *) xmlGetProp(cur, BAD_CAST "ident");
			def_val = (char *) xmlGetProp(cur, BAD_CAST "value");

			copy_default_value(def_key, def_val);

			if (brex_rules) {
				add_brex_rule(brex_rules, brexmap, def_key, def_val);
			}

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

			if (brex_rules) {
				add_brex_rule(brex_rules, brexmap, def_key, def_val);
			}
		}

		fclose(defaults);
	}

	if (strcmp(dmcode, "-") == 0) {
		random_code(dmcode, 256, modelIdentCode);
	}

	if (strcmp(dmcode, "") != 0) {
		int n, offset;

		offset = strncmp(dmcode, "DMC-", 4) == 0 ? 4 : 0;

		n = sscanf(dmcode + offset, "%14[^-]-%4[^-]-%3[^-]-%c%c-%4[^-]-%2s%3[^-]-%3s%c-%c-%3s%1s",
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

	if (strcmp(dmtype, "") == 0 || (strcmp(infoName_content, "") == 0 && !no_info_name)) {
		if ((defaults_xml = read_xml_doc(dmtypes_fname))) {
			process_dmtypes_xml(defaults_xml, brex_rules);
			xmlFreeDoc(defaults_xml);
		} else if ((defaults = fopen(dmtypes_fname, "r"))) {
			char default_line[1024];

			while (fgets(default_line, 1024, defaults)) {
				char def_key[32], def_val[256], infname[256];
				int n;
				char code[4], variant[2], itemloc[2], learn[4], levent[2];
				int p;

				n = sscanf(default_line, "%31s %255s %255[^\n]", def_key, def_val, infname);

				if (n < 2)
					continue;

				p = sscanf(def_key, "%3s%1s-%1s-%3s%1s", code, variant, itemloc, learn, levent);

				/* Get schema */
				if (strcmp(dmtype, "") == 0 &&
				    strcmp(code, infoCode) == 0 &&
				    (p < 2 || strcmp(variant, "*") == 0 || strcmp(variant, infoCodeVariant) == 0) &&
				    (p < 3 || strcmp(itemloc, "*") == 0 || strcmp(itemloc, itemLocationCode) == 0) &&
				    (p < 4 || strcmp(learn, "***") == 0   || strcmp(learn, learnCode) == 0) &&
				    (p < 5 || strcmp(levent, "*") == 0  || strcmp(levent, learnEventCode) == 0)) {
					strcpy(dmtype, def_val);
				}

				/* Get info name */
				if (n == 3 &&
				    strcmp(infoName_content, "") == 0 &&
				    !no_info_name &&
				    strcmp(code, infoCode) == 0 &&
				    (p < 2 || strcmp(variant, "*") == 0 || strcmp(variant, infoCodeVariant) == 0) &&
				    (p < 3 || strcmp(itemloc, "*") == 0 || strcmp(itemloc, itemLocationCode) == 0) &&
				    (p < 4 || strcmp(learn, "***") == 0   || strcmp(learn, learnCode) == 0) &&
				    (p < 5 || strcmp(levent, "*") == 0  || strcmp(levent, learnEventCode) == 0)) {
					strcpy(infoName_content, infname);
				}

				if (brex_rules) {
					add_dmtypes_brex_val(brex_rules, def_key, n == 3 ? infname : NULL);
				}
			}

			fclose(defaults);
		} else {
			defaults_xml = read_xml_mem((const char *) dmtypes_xml, dmtypes_xml_len);
			process_dmtypes_xml(defaults_xml, brex_rules);
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

	if (!tech_name_flag && (maint_sns || sns_fname || strcmp(brex_dmcode, "") != 0)) {
		xmlDocPtr brex;
		brex = set_tech_from_sns(defaults_dir);
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
	infoNameVariant = find_child(dmTitle, "infoNameVariant");

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

	if (issue_type) xmlSetProp(dmStatus, BAD_CAST "issueType", issue_type);

	/* SB DMs also contain an "original issue date" */
	if (strcmp(dmtype, "sb") == 0) {
		xmlNodePtr sbissdate;
		sbissdate = firstXPathNode(dm, NULL, "//sbOriginalIssueDate/issueDate");
		if (sbissdate) set_issue_date(sbissdate);
	}

	xmlSetProp(security, BAD_CAST "securityClassification", BAD_CAST securityClassification);

	xmlNodeSetContent(techName, BAD_CAST techName_content);

	if (strcmp(infoName_content, "") == 0) {
		xmlUnlinkNode(infoName);
		xmlFreeNode(infoName);
	} else {
		xmlChar *s;
		s = xmlEncodeEntitiesReentrant(dm, BAD_CAST infoName_content);
		xmlNodeSetContent(infoName, s);
		xmlFree(s);
	}

	if (info_name_variant) {
		xmlChar *s;
		s = xmlEncodeEntitiesReentrant(dm, info_name_variant);
		xmlNodeSetContent(infoNameVariant, s);
		xmlFree(s);
	} else {
		xmlUnlinkNode(infoNameVariant);
		xmlFreeNode(infoNameVariant);
	}

	responsiblePartnerCompany = find_child(dmStatus, "responsiblePartnerCompany");
	if (strcmp(responsiblePartnerCompany_enterpriseCode, "") != 0) {
		xmlSetProp(responsiblePartnerCompany, BAD_CAST "enterpriseCode", BAD_CAST responsiblePartnerCompany_enterpriseCode);
	}
	if (strcmp(responsiblePartnerCompany_enterpriseName, "") != 0) {
		xmlNodePtr node;
		if ((node = firstXPathNode(dm, NULL, "//responsiblePartnerCompany/enterpriseName"))) {
			xmlNodeSetContent(node, BAD_CAST responsiblePartnerCompany_enterpriseName);
		} else {
			xmlNewChild(responsiblePartnerCompany, NULL, BAD_CAST "enterpriseName", BAD_CAST responsiblePartnerCompany_enterpriseName);
		}
	}

	originator = find_child(dmStatus, "originator");
	if (strcmp(originator_enterpriseCode, "") != 0) {
		xmlSetProp(originator, BAD_CAST "enterpriseCode", BAD_CAST originator_enterpriseCode);
	}
	if (strcmp(originator_enterpriseName, "") != 0) {
		xmlNodePtr node;
		if ((node = firstXPathNode(dm, NULL, "//originator/enterpriseName"))) {
			xmlNodeSetContent(node, BAD_CAST originator_enterpriseName);
		} else {
			xmlNewChild(originator, NULL, BAD_CAST "enterpriseName", BAD_CAST originator_enterpriseName);
		}
	}

	set_skill_level(dm, skill_level_code);

	set_remarks(dm, remarks);

	if (act_dmcode) {
		set_act(dm, act_dmcode);
	} else {
		unset_act(dm);
	}

	if (strcmp(brex_dmcode, "") != 0)
		set_brex(dm, brex_dmcode);

	if (brex_rules) {
		xmlNodePtr context_rules;
		if ((context_rules = firstXPathNode(dm, NULL, "//contextRules"))) {
			xmlAddChild(context_rules, brex_rules);
		} else {
			xmlFreeNode(brex_rules);
		}
	}

	for (i = 0; languageIsoCode[i]; ++i) languageIsoCode[i] = toupper(languageIsoCode[i]);

	if (strcmp(learnCode, "") != 0 && strcmp(learnEventCode, "") != 0) {
		snprintf(learn, 8, "-%s%s", learnCode, learnEventCode);
	}

	if (!no_issue) {
		snprintf(iss, 16, "_%s-%s", issueNumber, inWork);
	}

	if (issue < ISS_50) {
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
				case ISS_42:
					set_brex(dm, ISS_42_DEFAULT_BREX);
					break;
				default:
					break;
			}
		}

		dm = toissue(dm, issue);
	}

	if (out && isdir(out, false)) {
		outdir = out;
		out = NULL;
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

	if (outdir) {
		if (chdir(outdir) != 0) {
			fprintf(stderr, ERR_PREFIX "Could not change to directory %s: %s\n", outdir, strerror(errno));
			exit(EXIT_OS_ERROR);
		}
	}

	if (!overwrite && access(out, F_OK) != -1) {
		if (no_overwrite_error) return 0;
		if (outdir) {
			fprintf(stderr, ERR_PREFIX "%s/%s already exists. Use -f to overwrite.\n", outdir, out);
		} else {
			fprintf(stderr, ERR_PREFIX "%s already exists. Use -f to overwrite.\n", out);
		}
		exit(EXIT_DM_EXISTS);
	}

	save_xml_doc(dm, out);

	if (verbose) {
		if (outdir) {
			printf("%s/%s\n", outdir, out);
		} else {
			puts(out);
		}
	}

	free(out);
	free(outdir);
	free(template_dir);
	free(act_dmcode);
	free(sns_fname);
	free(maint_sns);
	free(skill_level_code);
	free(defaults_dir_str);

	xmlFree(issue_type);
	xmlFree(remarks);
	xmlFree(info_name_variant);

	xmlFreeDoc(brexmap);
	xmlFreeDoc(dm);

	xmlCleanupParser();
	xsltCleanupGlobals();

	return 0;
}
