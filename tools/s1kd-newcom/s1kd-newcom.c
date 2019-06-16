#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>

#include "template.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-newcom"
#define VERSION "1.9.3"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_BAD_CODE 1
#define EXIT_COMMENT_EXISTS 2
#define EXIT_BAD_BREX_DMC 3
#define EXIT_BAD_DATE 4
#define EXIT_BAD_ISSUE 5
#define EXIT_BAD_TEMPLATE 6
#define EXIT_BAD_TEMPL_DIR 7
#define EXIT_OS_ERROR 8

#define E_BAD_TEMPL_DIR "Cannot dump template in directory: %s\n"

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

static char modelIdentCode[MAX_MODEL_IDENT_CODE] = "";
static char senderIdent[7] = "";
static char yearOfDataIssue[6] = "";
static char seqNumber[7] = "";
static char commentType[3] = "";
static char languageIsoCode[5] = "";
static char countryIsoCode[4] = "";
static char enterprise_name[256] = "";
static char address_city[256] = "";
static char address_country[256] = "";
static char securityClassification[4] = "";
static char commentPriorityCode[6] = "";
static char responseType[6] = "";

static char brex_dmcode[256] = "";

static char issue_date[16] = "";
static xmlChar *issue_type = NULL;

static xmlChar *remarks = NULL;

#define DEFAULT_S1000D_ISSUE ISS_42
#define ISS_22_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_23_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_30_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_40_DEFAULT_BREX "S1000D-A-04-10-0301-00A-022A-D"
#define ISS_41_DEFAULT_BREX "S1000D-E-04-10-0301-00A-022A-D"

#define DEFAULT_LANGUAGE_ISO_CODE "und"
#define DEFAULT_COUNTRY_ISO_CODE "ZZ"

static enum issue { NO_ISS, ISS_20, ISS_21, ISS_22, ISS_23, ISS_30, ISS_40, ISS_41, ISS_42 } issue = NO_ISS;

static char *template_dir = NULL;

static xmlDocPtr xml_skeleton(void)
{
	if (template_dir) {
		char src[PATH_MAX];
		sprintf(src, "%s/comment.xml", template_dir);

		if (access(src, F_OK) == -1) {
			fprintf(stderr, ERR_PREFIX "No schema comment in template directory \"%s\".\n", template_dir);
			exit(EXIT_BAD_TEMPLATE);
		}

		return read_xml_doc(src);
	} else {
		return read_xml_mem((const char *) comment_xml, comment_xml_len);
	}
}

static enum issue get_issue(const char *iss)
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
			
	styledoc = read_xml_mem((const char *) xml, len);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);

	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	xmlDocSetRootElement(orig, xmlCopyNode(xmlDocGetRootElement(res), 1));

	xmlFreeDoc(res);

	return orig;
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

	xmlSetProp(issueDate, BAD_CAST "year",  BAD_CAST year_s);
	xmlSetProp(issueDate, BAD_CAST "month", BAD_CAST month_s);
	xmlSetProp(issueDate, BAD_CAST "day",   BAD_CAST day_s);

}

static void copy_default_value(const char *def_key, const char *def_val)
{
	if (strcmp(def_key, "modelIdentCode") == 0 && strcmp(modelIdentCode, "") == 0)
		strcpy(modelIdentCode, def_val);
	else if (strcmp(def_key, "senderIdent") == 0 && strcmp(senderIdent, "") == 0)
		strcpy(senderIdent, def_val);
	else if (strcmp(def_key, "yearOfDataIssue") == 0 && strcmp(yearOfDataIssue, "") == 0)
		strcpy(yearOfDataIssue, def_val);
	else if (strcmp(def_key, "seqNumber") == 0 && strcmp(seqNumber, "") == 0)
		strcpy(seqNumber, def_val);
	else if (strcmp(def_key, "commentType") == 0 && strcmp(commentType, "") == 0)
		strcpy(commentType, def_val);
	else if (strcmp(def_key, "languageIsoCode") == 0 && strcmp(languageIsoCode, "") == 0)
		strcpy(languageIsoCode, def_val);
	else if (strcmp(def_key, "countryIsoCode") == 0 && strcmp(countryIsoCode, "") == 0)
		strcpy(countryIsoCode, def_val);
	else if (strcmp(def_key, "originator") == 0 && strcmp(enterprise_name, "") == 0)
		strcpy(enterprise_name, def_val);
	else if (strcmp(def_key, "city") == 0 && strcmp(address_city, "") == 0)
		strcpy(address_city, def_val);
	else if (strcmp(def_key, "country") == 0 && strcmp(address_country, "") == 0)
		strcpy(address_country, def_val);
	else if (strcmp(def_key, "securityClassification") == 0 && strcmp(securityClassification, "") == 0)
		strcpy(securityClassification, def_val);
	else if (strcmp(def_key, "commentPriorityCode") == 0 && strcmp(commentPriorityCode, "") == 0)
		strcpy(commentPriorityCode, def_val);
	else if (strcmp(def_key, "brex") == 0 && strcmp(brex_dmcode, "") == 0)
		strcpy(brex_dmcode, def_val);
	else if (strcmp(def_key, "templates") == 0 && !template_dir)
		template_dir = strdup(def_val);
	else if (strcmp(def_key, "remarks") == 0 && !remarks)
		remarks = xmlStrdup(BAD_CAST def_val);
	else if (strcmp(def_key, "issue") == 0 && issue == NO_ISS)
		issue = get_issue(def_val);
	else if (strcmp(def_key, "issueType") == 0 && !issue_type)
		issue_type = xmlStrdup(BAD_CAST def_val);
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

static void set_brex(xmlDocPtr doc, const char *code)
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

static void set_remarks(xmlDocPtr doc, xmlChar *text)
{
	xmlNodePtr remarks;

	remarks = firstXPathNode(doc, "//remarks");

	if (text) {
		xmlNewChild(remarks, NULL, BAD_CAST "simplePara", text);
	} else {
		xmlUnlinkNode(remarks);
		xmlFreeNode(remarks);
	}
}

static void dump_template(const char *path)
{
	FILE *f;

	if (access(path, W_OK) == -1 || chdir(path)) {
		fprintf(stderr, E_BAD_TEMPL_DIR, path);
		exit(EXIT_BAD_TEMPL_DIR);
	}

	f = fopen("comment.xml", "w");
	fprintf(f, "%.*s", comment_xml_len, comment_xml);
	fclose(f);
}

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [options]");
	puts("");
	puts("Options:");
	puts("  -$, --issue <issue>         Specify which S1000D issue to use.");
	puts("  -@, --out <path>            Output to specified file or directory.");
	puts("  -%, --templates <dir>       Use templates in specified directory.");
	puts("  -~, --dump-templates <dir>  Dump built-in XML template to directory.");
	puts("  -d, --defaults <file>       Specify the .defaults file name.");
	puts("  -f, --overwrite             Overwrite existing file.");
	puts("  -p, --prompt                Prompt the user for each value.");
	puts("  -q, --quiet                 Don't report an error if file exists.");
	puts("  -v, --verbose               Print file name of comment.");
	puts("  --version                   Show version information.");
	puts("");
	puts("In addition, the following pieces of meta data can be set:");
	puts("  -#, --code <code>           Comment code");
	puts("  -b, --brex <BREX>           BREX data module code");
	puts("  -C, --country <country>     Country ISO code");
	puts("  -c, --security <sec>        Security classification");
	puts("  -I, --date <date>           Issue date");
	puts("  -m, --remarks <remarks>     Remarks");
	puts("  -L, --language <lang>       Language ISO code");
	puts("  -o, --origname <orig>       Originator");
	puts("  -P, --priority <code>       Priority code");
	puts("  -r, --response <type>       Response type");
	puts("  -t, --title <title>         Comment title");
	puts("  -z, --issue-type <type>     Issue type");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
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

	char language_fname[4];

	char code[256] = "";
	char defaults_fname[PATH_MAX];
	bool custom_defaults = false;
	bool show_prompts = false;
	bool skip_code = false;
	char commentTitle[256] = "";

	bool verbose = false;
	bool overwrite = false;
	bool no_overwrite_error = false;

	char *out = NULL;
	char *outdir = NULL;

	xmlDocPtr defaults_xml;

	int i;

	const char *sopts = "d:p#:o:c:L:C:P:t:r:b:I:vf$:@:%:qm:~:z:h?";
	struct option lopts[] = {
		{"version"       , no_argument      , 0, 0},
		{"help"          , no_argument      , 0, 'h'},
		{"defaults"      , required_argument, 0, 'd'},
		{"prompt"        , no_argument      , 0, 'p'},
		{"code"          , required_argument, 0, '#'},
		{"origname"      , required_argument, 0, 'o'},
		{"security"      , required_argument, 0, 'c'},
		{"language"      , required_argument, 0, 'L'},
		{"country"       , required_argument, 0, 'C'},
		{"priority"      , required_argument, 0, 'P'},
		{"title"         , required_argument, 0, 't'},
		{"response"      , required_argument, 0, 'r'},
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
		{"issue-type"    , required_argument, 0, 'z'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'd':
				strncpy(defaults_fname, optarg, PATH_MAX - 1);
				custom_defaults = true;
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
			case 'I':
				strncpy(issue_date, optarg, 15);
				break;
			case 'v':
				verbose = true;
				break;
			case 'f':
				overwrite = true;
				break;
			case '$':
				issue = get_issue(optarg);
				break;
			case '@':
				out = strdup(optarg);
				break;
			case '%':
				template_dir = strdup(optarg);
				break;
			case 'q':
				no_overwrite_error = true;
				break;
			case 'm':
				remarks = xmlStrdup(BAD_CAST optarg);
				break;
			case '~':
				dump_template(optarg);
				return 0;
			case 'z':
				issue_type = xmlStrdup(BAD_CAST optarg);
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (!custom_defaults) {
		find_config(defaults_fname, DEFAULT_DEFAULTS_FNAME);
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
				char def_key[32], def_val[256];

				if (sscanf(default_line, "%31s %255[^\n]", def_key, def_val) != 2)
					continue;

				copy_default_value(def_key, def_val);
			}

			fclose(defaults);
		}
	}

	comment_doc = xml_skeleton();

	if (strcmp(code, "") != 0) {
		int n, offset;

		offset = strncmp(code, "COM-", 4) == 0 ? 4 : 0;

		n = sscanf(code + offset, "%14[^-]-%5s-%4s-%5s-%1s",
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

	if (strcmp(modelIdentCode, "") == 0 ||
		strcmp(senderIdent, "") == 0 ||
		strcmp(yearOfDataIssue, "") == 0 ||
		strcmp(seqNumber, "") == 0 ||
		strcmp(commentType, "") == 0) {
		fprintf(stderr, ERR_PREFIX "Missing required comment code components: ");
		fprintf(stderr, "COM-%s-%s-%s-%s-%s\n",
			strcmp(modelIdentCode, "") == 0  ? "???" : modelIdentCode,
			strcmp(senderIdent, "") == 0     ? "???" : senderIdent,
			strcmp(yearOfDataIssue, "") == 0 ? "???" : yearOfDataIssue,
			strcmp(seqNumber, "") == 0       ? "???" : seqNumber,
			strcmp(commentType, "") == 0     ? "???" : commentType);

		exit(EXIT_BAD_CODE);
	}

	if (issue == NO_ISS) issue = DEFAULT_S1000D_ISSUE;
	if (strcmp(securityClassification, "") == 0) strcpy(securityClassification, "01");
	if (strcmp(responseType, "") == 0) strcpy(responseType, "rt02");
	if (strcmp(commentPriorityCode, "") == 0) strcpy(commentPriorityCode, "cp01");

	set_env_lang();
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

	set_issue_date(issueDate);

	if (issue_type) xmlSetProp(commentStatus, BAD_CAST "issueType", issue_type);

	xmlNodeSetContent(enterpriseName, BAD_CAST enterprise_name);
	xmlNodeSetContent(city, BAD_CAST address_city);
	xmlNodeSetContent(country, BAD_CAST address_country);

	xmlSetProp(security,        BAD_CAST "securityClassification", BAD_CAST securityClassification);
	xmlSetProp(commentPriority, BAD_CAST "commentPriorityCode",    BAD_CAST commentPriorityCode);
	xmlSetProp(commentResponse, BAD_CAST "responseType", BAD_CAST responseType);

	if (strcmp(brex_dmcode, "") != 0)
		set_brex(comment_doc, brex_dmcode);

	set_remarks(comment_doc, remarks);

	for (i = 0; languageIsoCode[i]; ++i) {
		language_fname[i] = toupper(languageIsoCode[i]);
	}
	language_fname[i] = '\0';

	for (i = 0; commentType[i]; ++i) commentType[i] = toupper(commentType[i]);

	if (issue < ISS_42) {
		if (strcmp(brex_dmcode, "") == 0) {
			switch (issue) {
				case ISS_22:
					set_brex(comment_doc, ISS_22_DEFAULT_BREX);
					break;
				case ISS_23:
					set_brex(comment_doc, ISS_23_DEFAULT_BREX);
					break;
				case ISS_30:
					set_brex(comment_doc, ISS_30_DEFAULT_BREX);
					break;
				case ISS_40:
					set_brex(comment_doc, ISS_40_DEFAULT_BREX);
					break;
				case ISS_41:
					set_brex(comment_doc, ISS_41_DEFAULT_BREX);
					break;
				default:
					break;
			}
		}

		comment_doc = toissue(comment_doc, issue);
	}

	if (out && isdir(out, false)) {
		outdir = out;
		out = NULL;
	}

	if (!out) {
		char comment_fname[256];

		snprintf(comment_fname, 256, "COM-%s-%s-%s-%s-%s_%s-%s.XML",
			modelIdentCode,
			senderIdent,
			yearOfDataIssue,
			seqNumber,
			commentType,
			language_fname,
			countryIsoCode);

		out = strdup(comment_fname);
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
		exit(EXIT_COMMENT_EXISTS);
	}

	save_xml_doc(comment_doc, out);

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
	xmlFree(remarks);
	xmlFree(issue_type);
	xmlFreeDoc(comment_doc);

	xmlCleanupParser();

	return 0;
}
