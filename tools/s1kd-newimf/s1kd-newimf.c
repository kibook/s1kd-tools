#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "template.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-newimf"
#define VERSION "1.8.2"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_IMF_EXISTS 1
#define EXIT_BAD_BREX_DMC 2
#define EXIT_BAD_DATE 3
#define EXIT_BAD_TEMPLATE 4
#define EXIT_BAD_TEMPL_DIR 5
#define EXIT_ENCODING_ERROR 6
#define EXIT_OS_ERROR 7

#define E_BAD_TEMPL_DIR ERR_PREFIX "Cannot dump template in directory: %s\n"
#define E_ENCODING_ERROR ERR_PREFIX "Error encoding path name.\n"

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

static char issue_number[5] = "";
static char in_work[4] = "";
static char security_classification[4] = "";
static char responsible_partner_company[256] = "";
static char responsible_partner_company_code[7] = "";
static char originator[256] = "";
static char originator_code[7] = "";
static char icn_title[256] = "";

static char brex_dmcode[256] = "";

static char issue_date[16] = "";

static char *template_dir = NULL;

static xmlChar *remarks = NULL;

static xmlDocPtr xml_skeleton(void)
{
	if (template_dir) {
		char src[PATH_MAX];
		snprintf(src, PATH_MAX, "%s/icnmetadata.xml", template_dir);

		if (access(src, F_OK) == -1) {
			fprintf(stderr, ERR_PREFIX "No schema icnmetadata in template directory \"%s\".\n", template_dir);
			exit(EXIT_BAD_TEMPLATE);
		}

		return read_xml_doc(src);
	} else {
		return read_xml_mem((const char *) icnmetadata_xml, icnmetadata_xml_len);
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
			strncpy(str, temp, n - 1);
		}
	}
}

static void copy_def_val(char *dst, const char *target, const char *key, const char *val)
{
	if (strcmp(target, key) == 0 && strcmp(dst, "") == 0) {
		strcpy(dst, val);
	}
}

static xmlNodePtr first_xpath_node(const char *xpath, xmlXPathContextPtr ctx)
{
	xmlXPathObjectPtr obj;
	xmlNodePtr result;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		result = NULL;
	} else {
		result = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);

	return result;
}

static void dump_template(const char *path)
{
	FILE *f;

	if (access(path, W_OK) == -1 || chdir(path)) {
		fprintf(stderr, E_BAD_TEMPL_DIR, path);
		exit(EXIT_BAD_TEMPL_DIR);
	}

	f = fopen("icnmetadata.xml", "w");
	fprintf(f, "%.*s", icnmetadata_xml_len, icnmetadata_xml);
	fclose(f);
}

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] <icns>...");
	puts("");
	puts("Options:");
	puts("  -@, --out <path>            Output to specified file or directory.");
	puts("  -%, --templates <dir>       Use template in specified directory.");
	puts("  -~, --dump-templates <dir>  Dump built-in template to directory.");
	puts("  -d, --defaults <file>       Specify .defaults file path.");
	puts("  -f, --overwrite             Overwrite existing file.");
	puts("  -N, --omit-issue            Omit issue/inwork numbers from filename.");
	puts("  -p, --prompt                Show prompts.");
	puts("  -q, --quiet                 Don't report an error if file exists.");
	puts("  -v, --verbose               Print file name of IMF.");
	puts("  --version                   Show version information.");
	puts("  <icns>                      1 or more ICNs to generate a metadata file for.");
	puts("");
	puts("In addition, the following metadata can be set:");
	puts("  -b, --brex <BREX>           BREX data module code");
	puts("  -c, --security <sec>        Security classification");
	puts("  -I, --date <date>           Issue date");
	puts("  -m, --remarks <remarks>     Remarks");
	puts("  -n, --issno <iss>           Issue number");
	puts("  -O, --origcode <CAGE>       Originator CAGE code");
	puts("  -o, --origname <orig>       Originator");
	puts("  -R, --rpccode <CAGE>        Responsible partner company CAGE code");
	puts("  -r, --rpcname <RPC>         Responsible partner company");
	puts("  -t, --title <title>         ICN title");
	puts("  -w, --inwork <inwork>       Inwork issue");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

static void copy_default_value(const char *def_key, const char *def_val)
{
	copy_def_val(issue_number, "issueNumber", def_key, def_val);
	copy_def_val(in_work, "inWork", def_key, def_val);
	copy_def_val(security_classification, "securityClassification", def_key, def_val);
	copy_def_val(responsible_partner_company, "responsiblePartnerCompany", def_key, def_val);
	copy_def_val(responsible_partner_company_code, "responsiblePartnerCompanyCode", def_key, def_val);
	copy_def_val(originator, "originator", def_key, def_val);
	copy_def_val(originator_code, "originatorCode", def_key, def_val);
	copy_def_val(brex_dmcode, "brex", def_key, def_val);
	
	if (strcmp(def_key, "templates") == 0 && !template_dir) {
		template_dir = strdup(def_val);
	}
	if (strcmp(def_key, "remarks") == 0 && !remarks) {
		remarks = xmlStrdup(BAD_CAST def_val);
	}
}

static void set_brex(xmlDocPtr doc, const char *code)
{
	xmlNodePtr dmCode;
	xmlXPathContextPtr ctx;
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

	ctx = xmlXPathNewContext(doc);

	dmCode = first_xpath_node("//brexDmRef/dmRef/dmRefIdent/dmCode", ctx);

	xmlXPathFreeContext(ctx);

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

int main(int argc, char **argv)
{
	int i;

	bool show_prompts = false;
	bool no_issue = false;
	bool verbose = false;
	bool overwrite = false;
	bool no_overwrite_error = false;

	FILE *defaults;
	char defaults_fname[PATH_MAX];
	bool custom_defaults = false;

	char *out = NULL;
	char *outdir = NULL;

	xmlDocPtr defaults_xml;

	const char *sopts = "pd:n:w:c:r:R:o:O:Nt:b:I:vf%:qm:~:@:h?";
	struct option lopts[] = {
		{"version"       , no_argument      , 0, 0},
		{"help"          , no_argument      , 0, 'h'},
		{"prompts"       , no_argument      , 0, 'p'},
		{"defaults"      , required_argument, 0, 'd'},
		{"issno"         , required_argument, 0, 'n'},
		{"inwork"        , required_argument, 0, 'w'},
		{"security"      , required_argument, 0, 'c'},
		{"rpcname"       , required_argument, 0, 'r'},
		{"rpccode"       , required_argument, 0, 'R'},
		{"origname"      , required_argument, 0, 'o'},
		{"origcode"      , required_argument, 0, 'O'},
		{"omit-issue"    , no_argument      , 0, 'N'},
		{"title"         , required_argument, 0, 't'},
		{"brex"          , required_argument, 0, 'b'},
		{"date"          , required_argument, 0, 'I'},
		{"verbose"       , no_argument      , 0, 'v'},
		{"overwrite"     , no_argument      , 0, 'f'},
		{"templates"     , required_argument, 0, '%'},
		{"quiet"         , no_argument      , 0, 'q'},
		{"remarks"       , required_argument, 0, 'm'},
		{"dump-templates", required_argument, 0, '~'},
		{"out"           , required_argument, 0, '@'},
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
			case 'p': show_prompts = true; break;
			case 'd': strncpy(defaults_fname, optarg, PATH_MAX - 1); custom_defaults = true; break;
			case 'n': strncpy(issue_number, optarg, 3); break;
			case 'w': strncpy(in_work, optarg, 2); break;
			case 'c': strncpy(security_classification, optarg, 2); break;
			case 'r': strncpy(responsible_partner_company, optarg, 255); break;
			case 'R': strncpy(responsible_partner_company_code, optarg, 5); break;
			case 'o': strncpy(originator, optarg, 255); break;
			case 'O': strncpy(originator_code, optarg, 5); break;
			case 'N': no_issue = true; break;
			case 't': strncpy(icn_title, optarg, 255); break;
			case 'b': strncpy(brex_dmcode, optarg, 255); break;
			case 'I': strncpy(issue_date, optarg, 15); break;
			case 'v': verbose = true; break;
			case 'f': overwrite = true; break;
			case '%': template_dir = strdup(optarg); break;
			case 'q': no_overwrite_error = true; break;
			case 'm': remarks = xmlStrdup(BAD_CAST optarg); break;
			case '~': dump_template(optarg); return 0;
			case '@': out = strdup(optarg); break;
			case 'h':
			case '?': show_help(); return 0;
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
	} else if ((defaults = fopen(defaults_fname, "r"))) {
		char defaults_line[1024];

		while (fgets(defaults_line, 1024, defaults)) {
			char def_key[32], def_val[256];

			if (sscanf(defaults_line, "%31s %255[^\n]", def_key, def_val) != 2)
				continue;

			copy_default_value(def_key, def_val);
		}

		fclose(defaults);
	}

	if (show_prompts) {
		prompt("Issue number", issue_number, 5);
		prompt("In-work issue", in_work, 4);
		prompt("Security classification", security_classification, 4);
		prompt("Responsible partner company", responsible_partner_company, 256);
		prompt("Originator", originator, 256);
		prompt("ICN title", icn_title, 256);
	}

	if (strcmp(issue_number, "") == 0) strcpy(issue_number, "000");
	if (strcmp(in_work, "") == 0) strcpy(in_work, "01");
	if (strcmp(security_classification, "") == 0) strcpy(security_classification, "01");

	if (out && isdir(out, false)) {
		outdir = out;
		out = NULL;
	}

	if (outdir) {
		if (chdir(outdir) != 0) {
			fprintf(stderr, ERR_PREFIX "Could not change to directory %s: %s\n", outdir, strerror(errno));
			exit(EXIT_OS_ERROR);
		}
	}

	for (i = optind; i < argc; ++i) {
		int n;
		char icn[256];
		char fname[PATH_MAX];
		xmlDocPtr template;
		xmlNodePtr node;
		xmlXPathContextPtr ctx;

		n = sscanf(argv[i], "ICN-%255[^.].%*s", icn);

		if (n != 1) continue;

		template = xml_skeleton();

		ctx = xmlXPathNewContext(template);

		node = first_xpath_node("//imfIdent/imfCode", ctx);
		xmlSetProp(node, BAD_CAST "imfIdentIcn", BAD_CAST icn);

		node = first_xpath_node("//imfIdent/issueInfo", ctx);
		xmlSetProp(node, BAD_CAST "issueNumber", BAD_CAST issue_number);
		xmlSetProp(node, BAD_CAST "inWork", BAD_CAST in_work);

		node = first_xpath_node("//imfAddressItems/icnTitle", ctx);
		xmlNodeSetContent(node, BAD_CAST icn_title);

		node = first_xpath_node("//imfAddressItems/issueDate", ctx);
		set_issue_date(node);

		node = first_xpath_node("//imfStatus/security", ctx);
		xmlSetProp(node, BAD_CAST "securityClassification", BAD_CAST security_classification);

		if (strcmp(responsible_partner_company_code, "") != 0) {
			node = first_xpath_node("//imfStatus/responsiblePartnerCompany", ctx);
			xmlSetProp(node, BAD_CAST "enterpriseCode", BAD_CAST responsible_partner_company_code);
		}

		if (strcmp(responsible_partner_company, "") != 0) {
			node = first_xpath_node("//imfStatus/responsiblePartnerCompany", ctx);
			xmlNewChild(node, NULL, BAD_CAST "enterpriseName", BAD_CAST responsible_partner_company);
		}
		
		if (strcmp(originator_code, "") != 0) {
			node = first_xpath_node("//imfStatus/originator", ctx);
			xmlSetProp(node, BAD_CAST "enterpriseCode", BAD_CAST originator_code);
		}

		if (strcmp(originator, "") != 0) {
			node = first_xpath_node("//imfStatus/originator", ctx);
			xmlNewChild(node, NULL, BAD_CAST "enterpriseName", BAD_CAST originator);
		}

		if (strcmp(brex_dmcode, "") != 0)
			set_brex(template, brex_dmcode);

		set_remarks(template, remarks);

		if (out) {
			strcpy(fname, out);
		} else if (no_issue) {
			if (snprintf(fname, PATH_MAX, "IMF-%s.XML", icn) < 0) {
				fprintf(stderr, E_ENCODING_ERROR);
				exit(EXIT_ENCODING_ERROR);
			}
		} else {
			if (snprintf(fname, PATH_MAX, "IMF-%s_%s-%s.XML", icn, issue_number, in_work) < 0) {
				fprintf(stderr, E_ENCODING_ERROR);
				exit(EXIT_ENCODING_ERROR);
			}
		}

		if (!overwrite && access(fname, F_OK) != -1) {
			if (no_overwrite_error) return 0;
			if (outdir) {
				fprintf(stderr, ERR_PREFIX "%s/%s already exists.\n", outdir, fname);
			} else {
				fprintf(stderr, ERR_PREFIX "%s already exists.\n", fname);
			}
			exit(EXIT_IMF_EXISTS);
		}

		save_xml_doc(template, fname);

		if (verbose) {
			if (outdir) {
				printf("%s/%s\n", outdir, fname);
			} else {
				puts(fname);
			}
		}

		xmlXPathFreeContext(ctx);

		xmlFreeDoc(template);
	}

	free(out);
	free(outdir);
	free(template_dir);
	xmlFree(remarks);

	xmlCleanupParser();

	return 0;
}
