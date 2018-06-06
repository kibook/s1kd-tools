#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>

#include "template.h"

#define PROG_NAME "s1kd-newpm"
#define VERSION "1.0.0"

#define ERR_PREFIX "s1kd-newpm: ERROR: "

#define EXIT_BAD_PMC 1
#define EXIT_PM_EXISTS 2
#define EXIT_BAD_BREX_DMC 3
#define EXIT_BAD_DATE 4
#define EXIT_BAD_ISSUE 5
#define EXIT_BAD_TEMPLATE 6

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
char short_pm_title[256] = "";
char security_classification[4] = "";
char enterprise_name[256] = "";
char enterprise_code[7] = "";

char brex_dmcode[256] = "";

char issue_date[16] = "";

#define DEFAULT_S1000D_ISSUE ISS_42
#define ISS_22_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_23_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_30_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_40_DEFAULT_BREX "S1000D-A-04-10-0301-00A-022A-D"
#define ISS_41_DEFAULT_BREX "S1000D-E-04-10-0301-00A-022A-D"

#define DEFAULT_LANGUAGE_ISO_CODE "und"
#define DEFAULT_COUNTRY_ISO_CODE "ZZ"

enum issue { NO_ISS, ISS_20, ISS_21, ISS_22, ISS_23, ISS_30, ISS_40, ISS_41, ISS_42 } issue = NO_ISS;

char *template_dir = NULL;

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

xmlDocPtr xml_skeleton(void)
{
	if (template_dir) {
		char src[PATH_MAX];
		sprintf(src, "%s/pm.xml", template_dir);

		if (access(src, F_OK) == -1) {
			fprintf(stderr, ERR_PREFIX "No schema pm in template directory \"%s\".\n", template_dir);
			exit(EXIT_BAD_TEMPLATE);
		}

		return xmlReadFile(src, NULL, PARSE_OPTS);
	} else {
		return xmlReadMemory((const char *) pm_xml, pm_xml_len, NULL, NULL, 0);
	}
}

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

	xmlDocSetRootElement(orig, xmlCopyNode(xmlDocGetRootElement(res), 1));

	xmlFreeDoc(res);

	return orig;
}

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

void add_dm_ref(xmlNodePtr pmEntry, char *path, bool include_issue_info, bool include_language, bool include_title, bool include_date)
{
	xmlNodePtr ident_extension, dm_code, issue_info, language;
	xmlNodePtr dm_ref, dm_ref_ident, dm_ref_address_items;

	xmlDocPtr dmodule;
	xmlXPathContextPtr ctx;

	if (access(path, F_OK) == -1) {
		fprintf(stderr, ERR_PREFIX "Could not find referenced data module '%s'.\n", path);
		return;
	}

	dmodule = xmlReadFile(path, NULL, PARSE_OPTS);
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

	if (include_title || include_date) {
		dm_ref_address_items = xmlNewChild(dm_ref, NULL, BAD_CAST "dmRefAddressItems", NULL);

		if (include_title) {
			xmlAddChild(dm_ref_address_items, xmlCopyNode(first_xpath_node("//dmAddressItems/dmTitle", ctx), 1));
		}
		if (include_date) {
			xmlAddChild(dm_ref_address_items, xmlCopyNode(first_xpath_node("//dmAddressItems/issueDate", ctx), 1));
		}
	}

	xmlAddChild(pmEntry, dm_ref);
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

	xmlSetProp(issueDate, BAD_CAST "year", BAD_CAST year_s);
	xmlSetProp(issueDate, BAD_CAST "month", BAD_CAST month_s);
	xmlSetProp(issueDate, BAD_CAST "day", BAD_CAST day_s);
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<dmodule>...]");
	puts("");
	puts("Options:");
	puts("  -d         Specify the 'defaults' file name.");
	puts("  -p         Prompt the user for each value.");
	puts("  -q         Don't report an error if file exists.");
	puts("  -N         Omit issue/inwork from file name.");
	puts("  -v         Print file name of pub module.");
	puts("  -f         Overwrite existing file.");
	puts("  -$         Specify which S1000D issue to use.");
	puts("  -@         Output to specified file.");
	puts("  -%         Use template in specified directory.");
	puts("  -D         Include issue date in referenced data modules.");
	puts("  -i         Include issue info in referenced data modules.");
	puts("  -l         Include language info in referenced data modules.");
	puts("  -T         Include titles in referenced data modules.");
	puts("  --version  Show version information.");
	puts("");
	puts("In addition, the following pieces of meta data can be set:");
	puts("  -#         Publication module code");
	puts("  -L         Language ISO code");
	puts("  -C         Country ISO code");
	puts("  -n         Issue number");
	puts("  -w         Inwork issue");
	puts("  -c         Security classification");
	puts("  -r         Responsible partner company enterprise name");
	puts("  -t         Publication module title");
	puts("  -s         Short PM title");
	puts("  -b         BREX data module code");
	puts("  -I         Issue date");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
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
	else if (strcmp(key, "issue") == 0 && issue == NO_ISS)
		issue = get_issue(val);
	else if (strcmp(key, "templates") == 0 && !template_dir)
		template_dir = strdup(val);
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

/* Try reading the ISO language and country codes from the environment,
 * otherwise default to "und" (undetermined) for language and ZZ for
 * country.
 */
void set_env_lang(void)
{
	char *env, *lang, *lang_l, *lang_c;

	if (!(env = getenv("LANG"))) {
		if (strcmp(language_iso_code, "") == 0) {
			strcpy(language_iso_code, DEFAULT_LANGUAGE_ISO_CODE);
		}
		if (strcmp(country_iso_code, "") == 0) {
			strcpy(country_iso_code, DEFAULT_COUNTRY_ISO_CODE);
		}
		return;
	}

	lang = strdup(env);
	lang_l = strtok(lang, "_");
	lang_c = strtok(NULL, ".");

	if (strcmp(language_iso_code, "") == 0) {
		if (lang_l) {
			strncpy(language_iso_code, lang_l, 3);
		} else {
			strcpy(language_iso_code, DEFAULT_LANGUAGE_ISO_CODE);
		}
	}
	if (strcmp(country_iso_code, "") == 0) {
		if (lang_c) {
			strncpy(country_iso_code, lang_c, 2);
		} else {
			strcpy(country_iso_code, DEFAULT_COUNTRY_ISO_CODE);
		}
	}

	free(lang);
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

	int c;
	int i;

	char pmcode[256] = "";
	bool showprompts = false;
	bool skippmc = false;
	char defaults_fname[256] = "defaults";
	bool no_issue = false;
	char iss[8] = "";
	bool include_issue_info = false;
	bool include_language = false;
	bool include_title = false;
	bool include_date = false;
	bool verbose = false;
	bool overwrite = false;
	bool no_overwrite_error = false;
	xmlDocPtr defaults_xml;

	char *out = NULL;

	const char *sopts = "pDd:#:L:C:n:w:c:r:R:t:NilTb:I:vf$:@:%:s:qh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 'p': showprompts = true; break;
			case 'D': include_date = true; break;
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
			case 'T': include_title = true; break;
			case 'b': strcpy(brex_dmcode, optarg); break;
			case 'I': strcpy(issue_date, optarg); break;
			case 'v': verbose = true; break;
			case 'f': overwrite = true; break;
			case '$': issue = get_issue(optarg); break;
			case '@': out = strdup(optarg); break;
			case '%': template_dir = strdup(optarg); break;
			case 's': strcpy(short_pm_title, optarg); break;
			case 'q': no_overwrite_error = true; break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	defaults_xml = xmlReadFile(defaults_fname, NULL, PARSE_OPTS | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);

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
				char def_key[32];
				char def_val[256];

				if (sscanf(default_line, "%31s %255[^\n]", def_key, def_val) != 2)
					continue;

				copy_default_value(def_key, def_val);
			}

			fclose(defaults);
		}
	}

	pm_doc = xml_skeleton();

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
		prompt("PM title", pm_title, 256);
		prompt("Short PM title", short_pm_title, 256);
		prompt("Security classification", security_classification, 4);
		prompt("Responsible partner company", enterprise_name, 256);
	}

	if (strcmp(model_ident_code, "") == 0 ||
	    strcmp(pm_issuer, "") == 0 ||
	    strcmp(pm_number, "") == 0 ||
	    strcmp(pm_volume, "") == 0) {

		fprintf(stderr, ERR_PREFIX "Missing required PMC components: ");
		fprintf(stderr, "PMC-%s-%s-%s-%s\n",
			strcmp(model_ident_code, "") == 0 ? "???" : model_ident_code,
			strcmp(pm_issuer, "") == 0        ? "???" : pm_issuer,
			strcmp(pm_number, "") == 0        ? "???" : pm_number,
			strcmp(pm_volume, "") == 0        ? "???" : pm_volume);

		exit(EXIT_BAD_PMC);
	}

	if (issue == NO_ISS) issue = DEFAULT_S1000D_ISSUE;
	if (strcmp(issue_number, "") == 0) strcpy(issue_number, "000");
	if (strcmp(in_work, "") == 0) strcpy(in_work, "01");
	if (strcmp(security_classification, "") == 0) strcpy(security_classification, "01");

	set_env_lang();
	for (i = 0; language_iso_code[i]; ++i) {
		language_iso_code[i] = tolower(language_iso_code[i]);
	}
	for (i = 0; country_iso_code[i]; ++i) {
		country_iso_code[i] = toupper(country_iso_code[i]);
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

	xmlSetProp(pmCode, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
	xmlSetProp(pmCode, BAD_CAST "pmIssuer",       BAD_CAST pm_issuer);
	xmlSetProp(pmCode, BAD_CAST "pmNumber",       BAD_CAST pm_number);
	xmlSetProp(pmCode, BAD_CAST "pmVolume",       BAD_CAST pm_volume);

	xmlSetProp(language, BAD_CAST "languageIsoCode", BAD_CAST language_iso_code);
	xmlSetProp(language, BAD_CAST "countryIsoCode",  BAD_CAST country_iso_code);

	xmlSetProp(issueInfo, BAD_CAST "issueNumber", BAD_CAST issue_number);
	xmlSetProp(issueInfo, BAD_CAST "inWork",      BAD_CAST in_work);

	set_issue_date(issueDate);

	xmlNodeSetContent(pmTitle, BAD_CAST pm_title);

	if (strcmp(short_pm_title, "") != 0) {
		xmlNewChild(pmAddressItems, NULL, BAD_CAST "shortPmTitle", BAD_CAST short_pm_title);
	}

	xmlSetProp(security, BAD_CAST "securityClassification", BAD_CAST security_classification);

	if (strcmp(enterprise_name, "") != 0)
		xmlNewChild(responsiblePartnerCompany, NULL, BAD_CAST "enterpriseName", BAD_CAST enterprise_name);

	if (strcmp(enterprise_code, "") != 0)
		xmlSetProp(responsiblePartnerCompany, BAD_CAST "enterpriseCode", BAD_CAST enterprise_code);

	if (strcmp(brex_dmcode, "") != 0)
		set_brex(pm_doc, brex_dmcode);

	for (i = optind; i < argc; ++i) {
		add_dm_ref(pmEntry, argv[i], include_issue_info, include_language, include_title, include_date);
	}

	for (i = 0; language_iso_code[i]; ++i) {
		language_iso_code[i] = toupper(language_iso_code[i]);
	}

	if (!no_issue) {
		sprintf(iss, "_%s-%s", issue_number, in_work);
	}

	if (issue < ISS_42) {
		switch (issue) {
			case ISS_22:
				set_brex(pm_doc, ISS_22_DEFAULT_BREX);
				break;
			case ISS_23:
				set_brex(pm_doc, ISS_23_DEFAULT_BREX);
				break;
			case ISS_30:
				set_brex(pm_doc, ISS_30_DEFAULT_BREX);
				break;
			case ISS_40:
				set_brex(pm_doc, ISS_40_DEFAULT_BREX);
				break;
			case ISS_41:
				set_brex(pm_doc, ISS_41_DEFAULT_BREX);
				break;
			default:
				break;
		}

		pm_doc = toissue(pm_doc, issue);
	}

	if (!out) {
		char pm_filename[256];

		snprintf(pm_filename, 256, "PMC-%s-%s-%s-%s%s_%s-%s.XML",
			model_ident_code,
			pm_issuer,
			pm_number,
			pm_volume,
			iss,
			language_iso_code,
			country_iso_code);

		out = strdup(pm_filename);
	}

	if (!overwrite && access(out, F_OK) != -1) {
		if (no_overwrite_error) return 0;
		fprintf(stderr, ERR_PREFIX "%s already exists.\n", out);
		exit(EXIT_PM_EXISTS);
	}

	xmlSaveFormatFile(out, pm_doc, 1);

	if (verbose)
		puts(out);

	free(out);
	free(template_dir);
	xmlFreeDoc(pm_doc);

	xmlCleanupParser();

	return 0;
}