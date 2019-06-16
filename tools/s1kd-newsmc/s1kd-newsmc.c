#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>

#include "template.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-newsmc"
#define VERSION "1.6.2"

#define ERR_PREFIX PROG_NAME " ERROR: "

#define EXIT_BAD_SMC 1
#define EXIT_SMC_EXISTS 2
#define EXIT_BAD_BREX_DMC 3
#define EXIT_BAD_DATE 4
#define EXIT_BAD_ISSUE 5
#define EXIT_BAD_TEMPLATE 6
#define EXIT_BAD_TEMPL_DIR 7
#define EXIT_OS_ERROR 8

#define E_BAD_TEMPL_DIR ERR_PREFIX "Cannot dump template to directory: %s\n"

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

static char model_ident_code[16] = "";
static char smc_issuer[7] = "";
static char smc_number[7] = "";
static char smc_volume[4] = "";
static char language_iso_code[5] = "";
static char country_iso_code[4] = "";
static char issue_number[5] = "";
static char in_work[4] = "";
static char smc_title[256] = "";
static char security_classification[4] = "";
static char enterprise_name[256] = "";
static char enterprise_code[7] = "";

static char brex_dmcode[256] = "";

static char issue_date[16] = "";
static xmlChar *issue_type = NULL;

static xmlChar *remarks = NULL;
static xmlChar *skill_level_code = NULL;

#define DEFAULT_S1000D_ISSUE ISS_42
#define ISS_41_DEFAULT_BREX "S1000D-E-04-10-0301-00A-022A-D"

#define DEFAULT_LANGUAGE_ISO_CODE "und"
#define DEFAULT_COUNTRY_ISO_CODE "ZZ"

static enum issue { NO_ISS, ISS_41, ISS_42 } issue = NO_ISS;

static char *template_dir = NULL;

static char *act_dmcode = NULL;

static xmlDocPtr xml_skeleton(void)
{
	if (template_dir) {
		char src[PATH_MAX];
		sprintf(src, "%s/scormcontentpackage.xml", template_dir);

		if (access(src, F_OK) == -1) {
			fprintf(stderr, ERR_PREFIX "No schema scormcontentpackage in template directory \"%s\".\n", template_dir);
			exit(EXIT_BAD_TEMPLATE);
		}

		return read_xml_doc(src);
	} else {
		return read_xml_mem((const char *) scormcontentpackage_xml, scormcontentpackage_xml_len);
	}
}

static enum issue get_issue(const char *iss)
{
	if (strcmp(iss, "4.2") == 0)
		return ISS_42;
	else if (strcmp(iss, "4.1") == 0)
		return ISS_41;
	
	fprintf(stderr, ERR_PREFIX "Unsupported issue: %s\n", iss);
	exit(EXIT_BAD_ISSUE);

	return NO_ISS;
}

static xmlDocPtr toissue(xmlDocPtr doc, enum issue iss)
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
		default:
			return NULL;
	}

	orig = xmlCopyDoc(doc, 1);
			
	styledoc = read_xml_mem((const char *) xml, len);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);

	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	xmlDocSetRootElement(orig, xmlCopyNode(xmlDocGetRootElement(res), 1));

	xmlFreeDoc(res);

	return orig;
}

static xmlNodePtr find_child(xmlNodePtr parent, char *name)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, name) == 0) {
			return cur;
		}
	}

	return NULL;
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
			strncpy(str, temp, n - 1);
		}
	}
}

static xmlNodePtr first_xpath_node(const char *xpath, xmlXPathContextPtr ctx)
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

static void add_dm_ref(xmlNodePtr scoEntry, char *path, bool include_issue_info, bool include_language, bool include_title, bool include_date)
{
	xmlNodePtr ident_extension, dm_code, issue_info, language;
	xmlNodePtr dm_ref, dm_ref_ident, dm_ref_address_items, sco_entry_item;

	xmlDocPtr dmodule;
	xmlXPathContextPtr ctx;

	if (access(path, F_OK) == -1) {
		fprintf(stderr, ERR_PREFIX "Could not find referenced data module '%s'.\n", path);
		return;
	}

	dmodule = read_xml_doc(path);
	ctx = xmlXPathNewContext(dmodule);

	ident_extension = first_xpath_node("//dmIdent/identExtension", ctx);
	dm_code = first_xpath_node("//dmIdent/dmCode", ctx);
	issue_info = first_xpath_node("//dmIdent/issueInfo", ctx);
	language = first_xpath_node("//dmIdent/language", ctx);

	sco_entry_item = xmlNewNode(NULL, BAD_CAST "scoEntryItem");
	dm_ref = xmlNewChild(sco_entry_item, NULL, BAD_CAST "dmRef", NULL);
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

	xmlAddChild(scoEntry, sco_entry_item);
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

		if (snprintf(year_s, 5, "%u", year) < 0 ||
		    snprintf(month_s, 3, "%.2u", month) < 0 ||
		    snprintf(day_s, 3, "%.2u", day) < 0)
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

static void dump_template(const char *path)
{
	FILE *f;

	if (access(path, W_OK) == -1 || chdir(path)) {
		fprintf(stderr, E_BAD_TEMPL_DIR, path);
		exit(EXIT_BAD_TEMPL_DIR);
	}

	f = fopen("scormcontentpackage.xml", "w");
	fprintf(f, "%.*s", scormcontentpackage_xml_len, scormcontentpackage_xml);
	fclose(f);
}

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<dmodule>...]");
	puts("");
	puts("Options:");
	puts("  -$, --issue <issue>         Specify which S1000D issue to use.");
	puts("  -@, --out <path>            Output to specified file or directory.");
	puts("  -%, --templates <dir>       Use template in specified directory.");
	puts("  -~, --dump-templates <dir>  Dump built-in template to directory.");
	puts("  -D, --include-date          Include issue date in referenced data modules.");
	puts("  -d, --defaults <file>       Specify the .defaults file name.");
	puts("  -f, --overwrite             Overwrite existing file.");
	puts("  -i, --include-issue         Include issue info in referenced data modules.");
	puts("  -l, --include-lang          Include language info in referenced data modules.");
	puts("  -N, --omit-issue            Omit issue/inwork from file name.");
	puts("  -p, --prompt                Prompt the user for each value.");
	puts("  -q, --quiet                 Don't report an error if file exists.");
	puts("  -T, --include-title         Include titles in referenced data modules.");
	puts("  -v, --verbose               Print file name of SMC.");
	puts("  --version                   Show version information.");
	puts("  <dmodule>...                Data modules to include in new SMC.");
	puts("");
	puts("In addition, the following pieces of meta data can be set:");
	puts("  -#, --code <code>           SCORM content package code");
	puts("  -a, --act <ACT>             ACT data module code");
	puts("  -b, --brex <BREX>           BREX data module code");
	puts("  -C, --country <country>     Country ISO code");
	puts("  -c, --security <sec>        Security classification");
	puts("  -I, --date <date>           Issue date");
	puts("  -k, --skill <skill>         Skill level");
	puts("  -L, --language <lang>       Language ISO code");
	puts("  -m, --remarks <remarks>     Remarks");
	puts("  -n, --issno <iss>           Issue number");
	puts("  -R, --rpccode <CAGE>        Responsible partner company code");
	puts("  -r, --rpcname <RPC>         Responsible partner company enterprise name");
	puts("  -t, --title <title>         SCORM content package title");
	puts("  -w, --inwork <inwork>       Inwork issue");
	puts("  -z, --issue-type <type>     Issue type");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

static void copy_default_value(const char *key, const char *val)
{
	if (strcmp(key, "modelIdentCode") == 0 && strcmp(model_ident_code, "") == 0)
		strcpy(model_ident_code, val);
	else if (strcmp(key, "scormContentPackageIssuer") == 0 && strcmp(smc_issuer, "") == 0)
		strcpy(smc_issuer, val);
	else if (strcmp(key, "scormContentPackageNumber") == 0 && strcmp(smc_number, "") == 0)
		strcpy(smc_number, val);
	else if (strcmp(key, "scormContentPackageVolume") == 0 && strcmp(smc_volume, "") == 0)
		strcpy(smc_volume, val);
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
	else if (strcmp(key, "remarks") == 0 && !remarks)
		remarks = xmlStrdup(BAD_CAST val);
	else if (strcmp(key, "skillLevelCode") == 0 && !skill_level_code)
		skill_level_code = xmlStrdup(BAD_CAST val);
	else if (strcmp(key, "issueType") == 0 && !issue_type)
		issue_type = xmlStrdup(BAD_CAST val);
	else if (strcmp(key, "act") == 0 && !act_dmcode)
		act_dmcode = strdup(val);
}

static xmlNodePtr firstXPathNode(xmlDocPtr doc, const char *xpath)
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
	dmCode = firstXPathNode(doc, "//brexDmRef/dmRef/dmRefIdent/dmCode");
	set_dmcode(dmCode, fname);
}

static void set_act(xmlDocPtr doc, const char *fname)
{
	xmlNodePtr dmCode;
	dmCode = firstXPathNode(doc, "//applicCrossRefTableRef/dmRef/dmRefIdent/dmCode");
	set_dmcode(dmCode, fname);
}

static void unset_act(xmlDocPtr doc)
{
	xmlNodePtr dmCode;
	dmCode = firstXPathNode(doc, "//applicCrossRefTableRef");
	xmlUnlinkNode(dmCode);
	xmlFreeNode(dmCode);
}

/* Try reading the ISO language and country codes from the environment,
 * otherwise default to "und" (undetermined) for language and ZZ for
 * country.
 */
static void set_env_lang(void)
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

static void set_remarks(xmlDocPtr doc, xmlChar *text)
{
	xmlXPathContextPtr ctx;
	xmlNodePtr remarks;

	ctx = xmlXPathNewContext(doc);

	remarks = first_xpath_node("//remarks", ctx);

	xmlXPathFreeContext(ctx);

	if (text) {
		xmlNewChild(remarks, NULL, BAD_CAST "simplePara", text);
	} else {
		xmlUnlinkNode(remarks);
		xmlFreeNode(remarks);
	}
}

static void set_skill_level(xmlDocPtr doc, xmlChar *code)
{
	xmlNodePtr skill_level;

	skill_level = firstXPathNode(doc, "//personSkill");

	if (code) {
		xmlSetProp(skill_level, BAD_CAST "skillLevelCode", code);
	} else {
		xmlUnlinkNode(skill_level);
		xmlFreeNode(skill_level);
	}
}

int main(int argc, char **argv)
{
	xmlDocPtr smc_doc;

	xmlNodePtr scormContentPackage;
	xmlNodePtr identAndStatusSection;
	xmlNodePtr scormContentPackageAddress;
	xmlNodePtr scormContentPackageIdent;
	xmlNodePtr scormContentPackageCode;
	xmlNodePtr language;
	xmlNodePtr issueInfo;
	xmlNodePtr scormContentPackageAddressItems;
	xmlNodePtr issueDate;
	xmlNodePtr scormContentPackageTitle;
	xmlNodePtr scormContentPackageStatus;
	xmlNodePtr security;
	xmlNodePtr responsiblePartnerCompany;
	xmlNodePtr scoEntry;

	int c;
	int i;

	char smcode[256] = "";
	bool showprompts = false;
	bool skipcode = false;
	char defaults_fname[PATH_MAX];
	bool custom_defaults = false;
	bool no_issue = false;
	char iss[16] = "";
	bool include_issue_info = false;
	bool include_language = false;
	bool include_title = false;
	bool include_date = false;
	bool verbose = false;
	bool overwrite = false;
	bool no_overwrite_error = false;
	xmlDocPtr defaults_xml;

	char *out = NULL;
	char *outdir = NULL;

	const char *sopts = "pDd:#:L:C:n:w:c:r:R:t:NilTb:I:vf$:@:%:qm:~:k:z:a:h?";
	struct option lopts[] = {
		{"version"       , no_argument      , 0, 0},
		{"help"          , no_argument      , 0, 'h'},
		{"prompt"        , no_argument      , 0, 'p'},
		{"include-date"  , no_argument      , 0, 'D'},
		{"defaults"      , required_argument, 0, 'd'},
		{"code"          , required_argument, 0, '#'},
		{"language"      , required_argument, 0, 'L'},
		{"country"       , required_argument, 0, 'C'},
		{"issno"         , required_argument, 0, 'n'},
		{"inwork"        , required_argument, 0, 'w'},
		{"security"      , required_argument, 0, 'c'},
		{"rpcname"       , required_argument, 0, 'r'},
		{"rpccode"       , required_argument, 0, 'R'},
		{"title"         , required_argument, 0, 't'},
		{"omit-issue"    , no_argument      , 0, 'N'},
		{"include-issue" , no_argument      , 0, 'i'},
		{"include-lang"  , no_argument      , 0, 'l'},
		{"include-title" , no_argument      , 0, 'T'},
		{"brex"          , required_argument, 0, 'b'},
		{"date"          , required_argument, 0, 'I'},
		{"verbose"       , no_argument      , 0, 'v'},
		{"overwrite"     , no_argument      , 0, 'f'},
		{"issue"         , required_argument, 0, '$'},
		{"out"           , required_argument, 0, '@'},
		{"templates"     , required_argument, 0, '%'},
		{"quiet"         , no_argument      , 0, 'q'},
		{"remarks"       , required_argument, 0, 'm'},
		{"dump-templates", required_argument, 0, '~'},
		{"skill"         , required_argument, 0, 'k'},
		{"issue-type"    , required_argument, 0, 'z'},
		{"act"           , required_argument, 0, 'a'},
		LIBXML2_PARSE_LONGOPT_DEFS
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
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'a': act_dmcode = strdup(optarg); break;
			case 'p': showprompts = true; break;
			case 'D': include_date = true; break;
			case 'd': strncpy(defaults_fname, optarg, PATH_MAX - 1); custom_defaults = true; break;
			case '#': strcpy(smcode, optarg); skipcode = true; break;
			case 'L': strcpy(language_iso_code, optarg); break;
			case 'C': strcpy(country_iso_code, optarg); break;
			case 'n': strcpy(issue_number, optarg); break;
			case 'w': strcpy(in_work, optarg); break;
			case 'c': strcpy(security_classification, optarg); break;
			case 'r': strcpy(enterprise_name, optarg); break;
			case 'R': strcpy(enterprise_code, optarg); break;
			case 't': strcpy(smc_title, optarg); break;
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
			case 'q': no_overwrite_error = true; break;
			case 'm': remarks = xmlStrdup(BAD_CAST optarg); break;
			case '~': dump_template(optarg); return 0;
			case 'k': skill_level_code = xmlStrdup(BAD_CAST optarg); break;
			case 'z': issue_type = xmlStrdup(BAD_CAST optarg); break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (!custom_defaults) {
		find_config(defaults_fname, DEFAULT_DEFAULTS_FNAME);
	}

	defaults_xml = read_xml_doc(defaults_fname);

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

	smc_doc = xml_skeleton();

	if (strcmp(smcode, "") != 0) {
		int n, offset;

		offset = strncmp(smcode, "SMC-", 4) == 0 ? 4 : 0;

		n = sscanf(smcode + offset, "%14[^-]-%5s-%5s-%2s",
			model_ident_code,
			smc_issuer,
			smc_number,
			smc_volume);

		if (n != 4) {
			fprintf(stderr, ERR_PREFIX "Bad SCORM content package code.\n");
			exit(EXIT_BAD_SMC);
		}
	}

	if (showprompts) {
		if (!skipcode) {
			prompt("Model ident code", model_ident_code, 16);
			prompt("SMC Issuer", smc_issuer, 7);
			prompt("SMC Number", smc_number, 7);
			prompt("SMC Volume", smc_volume, 4);
		}
		prompt("Language ISO code", language_iso_code, 5);
		prompt("Country ISO code", country_iso_code, 4);
		prompt("Issue number", issue_number, 5);
		prompt("In work", in_work, 4);
		prompt("SMC title", smc_title, 256);
		prompt("Security classification", security_classification, 4);
		prompt("Responsible partner company", enterprise_name, 256);
	}

	if (strcmp(model_ident_code, "") == 0 ||
	    strcmp(smc_issuer, "") == 0 ||
	    strcmp(smc_number, "") == 0 ||
	    strcmp(smc_volume, "") == 0) {

		fprintf(stderr, ERR_PREFIX "Missing required SMC components: ");
		fprintf(stderr, "SMC-%s-%s-%s-%s\n",
			strcmp(model_ident_code, "") == 0 ? "???" : model_ident_code,
			strcmp(smc_issuer, "") == 0        ? "???" : smc_issuer,
			strcmp(smc_number, "") == 0        ? "???" : smc_number,
			strcmp(smc_volume, "") == 0        ? "???" : smc_volume);

		exit(EXIT_BAD_SMC);
	}

	if (issue == NO_ISS) issue = DEFAULT_S1000D_ISSUE;
	if (strcmp(issue_number, "") == 0) strcpy(issue_number, "000");
	if (strcmp(in_work, "") == 0) strcpy(in_work, "01");
	if (strcmp(security_classification, "") == 0) strcpy(security_classification, "01");
	if (!skill_level_code) skill_level_code = xmlStrdup(BAD_CAST "sk01");

	set_env_lang();
	for (i = 0; language_iso_code[i]; ++i) {
		language_iso_code[i] = tolower(language_iso_code[i]);
	}
	for (i = 0; country_iso_code[i]; ++i) {
		country_iso_code[i] = toupper(country_iso_code[i]);
	}

	scormContentPackage = xmlDocGetRootElement(smc_doc);
	identAndStatusSection = find_child(scormContentPackage, "identAndStatusSection");
	scormContentPackageAddress = find_child(identAndStatusSection, "scormContentPackageAddress");
	scormContentPackageIdent = find_child(scormContentPackageAddress, "scormContentPackageIdent");
	scormContentPackageCode = find_child(scormContentPackageIdent, "scormContentPackageCode");
	language = find_child(scormContentPackageIdent, "language");
	issueInfo = find_child(scormContentPackageIdent, "issueInfo");
	scormContentPackageAddressItems = find_child(scormContentPackageAddress, "scormContentPackageAddressItems");
	issueDate = find_child(scormContentPackageAddressItems, "issueDate");
	scormContentPackageTitle = find_child(scormContentPackageAddressItems, "scormContentPackageTitle");
	scormContentPackageStatus = find_child(identAndStatusSection, "scormContentPackageStatus");
	security = find_child(scormContentPackageStatus, "security");
	responsiblePartnerCompany = find_child(scormContentPackageStatus, "responsiblePartnerCompany");
	scoEntry = find_child(find_child(scormContentPackage, "content"), "scoEntry");

	xmlSetProp(scormContentPackageCode, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
	xmlSetProp(scormContentPackageCode, BAD_CAST "scormContentPackageIssuer",       BAD_CAST smc_issuer);
	xmlSetProp(scormContentPackageCode, BAD_CAST "scormContentPackageNumber",       BAD_CAST smc_number);
	xmlSetProp(scormContentPackageCode, BAD_CAST "scormContentPackageVolume",       BAD_CAST smc_volume);

	xmlSetProp(language, BAD_CAST "languageIsoCode", BAD_CAST language_iso_code);
	xmlSetProp(language, BAD_CAST "countryIsoCode",  BAD_CAST country_iso_code);

	xmlSetProp(issueInfo, BAD_CAST "issueNumber", BAD_CAST issue_number);
	xmlSetProp(issueInfo, BAD_CAST "inWork",      BAD_CAST in_work);

	set_issue_date(issueDate);

	if (issue_type) xmlSetProp(scormContentPackageStatus, BAD_CAST "issueType", issue_type);

	xmlNodeSetContent(scormContentPackageTitle, BAD_CAST smc_title);

	xmlSetProp(security, BAD_CAST "securityClassification", BAD_CAST security_classification);

	if (strcmp(enterprise_name, "") != 0)
		xmlNewChild(responsiblePartnerCompany, NULL, BAD_CAST "enterpriseName", BAD_CAST enterprise_name);

	if (strcmp(enterprise_code, "") != 0)
		xmlSetProp(responsiblePartnerCompany, BAD_CAST "enterpriseCode", BAD_CAST enterprise_code);

	if (act_dmcode) {
		set_act(smc_doc, act_dmcode);
	} else {
		unset_act(smc_doc);
	}

	if (strcmp(brex_dmcode, "") != 0)
		set_brex(smc_doc, brex_dmcode);

	set_skill_level(smc_doc, skill_level_code);

	set_remarks(smc_doc, remarks);

	for (i = optind; i < argc; ++i) {
		add_dm_ref(scoEntry, argv[i], include_issue_info, include_language, include_title, include_date);
	}

	for (i = 0; language_iso_code[i]; ++i) {
		language_iso_code[i] = toupper(language_iso_code[i]);
	}

	if (!no_issue) {
		snprintf(iss, 16, "_%s-%s", issue_number, in_work);
	}

	if (issue < ISS_42) {
		switch (issue) {
			case ISS_41:
				set_brex(smc_doc, ISS_41_DEFAULT_BREX);
				break;
			default:
				break;
		}

		smc_doc = toissue(smc_doc, issue);
	}

	if (out && isdir(out, false)) {
		outdir = out;
		out = NULL;
	}

	if (!out) {
		char smc_filename[256];

		snprintf(smc_filename, 256, "SMC-%s-%s-%s-%s%s_%s-%s.XML",
			model_ident_code,
			smc_issuer,
			smc_number,
			smc_volume,
			iss,
			language_iso_code,
			country_iso_code);

		out = strdup(smc_filename);
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
			fprintf(stderr, ERR_PREFIX "%s/%s already exists.\n", outdir, out);
		} else {
			fprintf(stderr, ERR_PREFIX "%s already exists.\n", out);
		}
		exit(EXIT_SMC_EXISTS);
	}

	save_xml_doc(smc_doc, out);

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
	xmlFree(remarks);
	xmlFree(skill_level_code);
	xmlFree(issue_type);
	xmlFreeDoc(smc_doc);

	xmlCleanupParser();

	return 0;
}
