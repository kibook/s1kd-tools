#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <libgen.h>
#include <errno.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>

#include "templates.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-newdml"
#define VERSION "2.1.1"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_DML_EXISTS 1
#define EXIT_BAD_INPUT 2
#define EXIT_BAD_CODE 3
#define EXIT_BAD_BREX_DMC 4
#define EXIT_BAD_DATE 5
#define EXIT_BAD_ISSUE 6
#define EXIT_BAD_TEMPLATE 7
#define EXIT_BAD_TEMPL_DIR 8
#define EXIT_OS_ERROR 9
#define EXIT_BAD_SNS 10

#define E_BAD_TEMPL_DIR ERR_PREFIX "Cannot dump template in directory: %s\n"
#define E_BAD_SNS ERR_PREFIX "Specified BREX DM could not be read: %s\n"
#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

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

#define MAX_ISSUE_NUMBER		 5	+ 2
#define MAX_IN_WORK			 2	+ 2

static char model_ident_code[MAX_MODEL_IDENT_CODE] = "";
static char sender_ident[7] = "";
static char dml_type[3] = "";
static char year_of_data_issue[6] = "";
static char seq_number[7] = "";
static char security_classification[4] = "";
static char issue_number[MAX_ISSUE_NUMBER] = "";
static char in_work[MAX_IN_WORK] = "";

static char brex_dmcode[256] = "";

static char issue_date[16] = "";
static xmlChar *issue_type = NULL;

static xmlChar *remarks = NULL;

#define DEFAULT_S1000D_ISSUE ISS_50
#define ISS_22_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_23_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_30_DEFAULT_BREX "AE-A-04-10-0301-00A-022A-D"
#define ISS_40_DEFAULT_BREX "S1000D-A-04-10-0301-00A-022A-D"
#define ISS_41_DEFAULT_BREX "S1000D-E-04-10-0301-00A-022A-D"
#define ISS_42_DEFAULT_BREX "S1000D-F-04-10-0301-00A-022A-D"

static enum issue { NO_ISS, ISS_20, ISS_21, ISS_22, ISS_23, ISS_30, ISS_40, ISS_41, ISS_42, ISS_50 } issue = NO_ISS;

static char *defaultRpcName = NULL;
static char *defaultRpcCode = NULL;

static char *template_dir = NULL;

static xmlDocPtr xml_skeleton(void)
{
	if (template_dir) {
		char src[PATH_MAX];
		sprintf(src, "%s/dml.xml", template_dir);

		if (access(src, F_OK) == -1) {
			fprintf(stderr, ERR_PREFIX "No schema dml found in template directory \"%s\".\n", template_dir);
			exit(EXIT_BAD_TEMPLATE);
		}

		return read_xml_doc(src);
	} else {
		return read_xml_mem((const char *) dml_xml, dml_xml_len);
	}
}

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

static void transform_doc(xmlDocPtr doc, unsigned char *xsl, unsigned int len, const char **params)
{
	xmlDocPtr styledoc, src, res;
	xsltStylesheetPtr style;
	xmlNodePtr old;

	src = xmlCopyDoc(doc, 1);

	styledoc = read_xml_mem((const char *) xsl, len);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, src, params);

	old = xmlDocSetRootElement(doc, xmlCopyNode(xmlDocGetRootElement(res), 1));
	xmlFreeNode(old);

	xmlFreeDoc(src);
	xmlFreeDoc(res);
	xsltFreeStylesheet(style);
}

static xmlDocPtr toissue(xmlDocPtr doc, enum issue iss)
{
	xmlDocPtr orig;
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
	transform_doc(orig, xml, len, NULL);
	xmlFreeDoc(doc);

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

static xmlNodePtr firstXPathNode(const char *xpath, xmlDocPtr doc)
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

static bool isdm(xmlDocPtr doc)
{
	return xmlStrcmp(xmlDocGetRootElement(doc)->name, BAD_CAST "dmodule") == 0;
}

static bool ispm(xmlDocPtr doc)
{
	return xmlStrcmp(xmlDocGetRootElement(doc)->name, BAD_CAST "pm") == 0;
}

static bool isicn(const char *name)
{
	return strncmp(name, "ICN-", 4) == 0;
}

static bool isimf(xmlDocPtr doc)
{
	return xmlStrcmp(xmlDocGetRootElement(doc)->name, BAD_CAST "icnMetadataFile") == 0;
}

static bool iscom(xmlDocPtr doc)
{
	return xmlStrcmp(xmlDocGetRootElement(doc)->name, BAD_CAST "comment") == 0;
}

static bool isdml(xmlDocPtr doc)
{
	return xmlStrcmp(xmlDocGetRootElement(doc)->name, BAD_CAST "dml") == 0;
}

static void addDmRef(xmlDocPtr dm, xmlNodePtr dmlContent, bool csl)
{
	xmlNodePtr dmRef, dmRefIdent, dmRefAddressItems, dmlEntry;

	dmlEntry = xmlNewChild(dmlContent, NULL, BAD_CAST "dmlEntry", NULL);

	if (csl) {
		xmlChar *issueType;

		issueType = xmlGetProp(firstXPathNode("//dmStatus", dm), BAD_CAST "issueType");

		if (issueType) {
			xmlSetProp(dmlEntry, BAD_CAST "issueType", issueType);
		}

		xmlFree(issueType);
	}

	dmRef = xmlNewChild(dmlEntry, NULL, BAD_CAST "dmRef", NULL);
	dmRefIdent = xmlNewChild(dmRef, NULL, BAD_CAST "dmRefIdent", NULL);
	xmlAddChild(dmRefIdent, xmlCopyNode(firstXPathNode("//dmIdent/identExtension", dm), 1));
	xmlAddChild(dmRefIdent, xmlCopyNode(firstXPathNode("//dmIdent/dmCode", dm), 1));

	if (csl) {
		xmlAddChild(dmRefIdent, xmlCopyNode(firstXPathNode("//dmIdent/issueInfo", dm), 1));
	}

	xmlAddChild(dmRefIdent, xmlCopyNode(firstXPathNode("//dmIdent/language", dm), 1));

	dmRefAddressItems = xmlNewChild(dmRef, NULL, BAD_CAST "dmRefAddressItems", NULL);
	xmlAddChild(dmRefAddressItems, xmlCopyNode(firstXPathNode("//dmAddressItems/dmTitle", dm), 1));

	if (csl) {
		xmlAddChild(dmRefAddressItems, xmlCopyNode(firstXPathNode("//dmAddressItems/issueDate", dm), 1));
	}

	xmlAddChild(dmlEntry, xmlCopyNode(firstXPathNode("//dmStatus/security", dm), 1));
	xmlAddChild(dmlEntry, xmlCopyNode(firstXPathNode("//dmStatus/responsiblePartnerCompany", dm), 1));
}

static void addPmRef(xmlDocPtr pm, xmlNodePtr dmlContent, bool csl)
{
	xmlNodePtr pmRef, pmRefIdent, pmRefAddressItems, dmlEntry;

	dmlEntry = xmlNewChild(dmlContent, NULL, BAD_CAST "dmlEntry", NULL);

	if (csl) {
		xmlChar *issueType;

		issueType = xmlGetProp(firstXPathNode("//pmStatus", pm), BAD_CAST "issueType");

		if (issueType) {
			xmlSetProp(dmlEntry, BAD_CAST "issueType", issueType);
		}

		xmlFree(issueType);
	}

	pmRef = xmlNewChild(dmlEntry, NULL, BAD_CAST "pmRef", NULL);
	pmRefIdent = xmlNewChild(pmRef, NULL, BAD_CAST "pmRefIdent", NULL);
	xmlAddChild(pmRefIdent, xmlCopyNode(firstXPathNode("//pmIdent/identExtension", pm), 1));
	xmlAddChild(pmRefIdent, xmlCopyNode(firstXPathNode("//pmIdent/pmCode", pm), 1));

	if (csl) {
		xmlAddChild(pmRefIdent, xmlCopyNode(firstXPathNode("//pmIdent/issueInfo", pm), 1));
	}

	xmlAddChild(pmRefIdent, xmlCopyNode(firstXPathNode("//pmIdent/language", pm), 1));

	pmRefAddressItems = xmlNewChild(pmRef, NULL, BAD_CAST "pmRefAddressItems", NULL);
	xmlAddChild(pmRefAddressItems, xmlCopyNode(firstXPathNode("//pmAddressItems/pmTitle", pm), 1));

	if (csl) {
		xmlAddChild(pmRefAddressItems, xmlCopyNode(firstXPathNode("//pmAddressItems/issueDate", pm), 1));
	}

	xmlAddChild(pmRefAddressItems, xmlCopyNode(firstXPathNode("//pmAddressItems/shortPmTitle", pm), 1));

	xmlAddChild(dmlEntry, xmlCopyNode(firstXPathNode("//pmStatus/security", pm), 1));
	xmlAddChild(dmlEntry, xmlCopyNode(firstXPathNode("//pmStatus/responsiblePartnerCompany", pm), 1));
}

static void addIcnRef(const char *str, xmlNodePtr dmlContent)
{
	xmlNodePtr dmlEntry, infoEntityRef, responsiblePartnerCompany, security;
	char *icn;
	char *sec;

	dmlEntry = xmlNewChild(dmlContent, NULL, BAD_CAST "dmlEntry", NULL);
	infoEntityRef = xmlNewChild(dmlEntry, NULL, BAD_CAST "infoEntityRef", NULL);

	icn = strdup(str);
	strtok(icn, ".");

	sec = strrchr(icn, '-') + 1;

	xmlSetProp(infoEntityRef, BAD_CAST "infoEntityRefIdent", BAD_CAST icn);

	security = xmlNewChild(dmlEntry, NULL, BAD_CAST "security", NULL);
	xmlSetProp(security, BAD_CAST "securityClassification", BAD_CAST sec);

	responsiblePartnerCompany = xmlNewChild(dmlEntry, NULL, BAD_CAST "responsiblePartnerCompany", NULL);
	if (defaultRpcCode) {
		xmlSetProp(responsiblePartnerCompany, BAD_CAST "enterpriseCode", BAD_CAST defaultRpcCode);
	}
	if (defaultRpcName) {
		xmlNewChild(responsiblePartnerCompany, NULL, BAD_CAST "enterpriseName", BAD_CAST defaultRpcName);
	}

	free(icn);
}

static void addImfRef(xmlDocPtr imf, xmlNodePtr dmlContent)
{
	xmlChar *imfIdentIcn;

	char *icn;

	imfIdentIcn = xmlGetProp(firstXPathNode("//imfIdent/imfCode", imf), BAD_CAST "imfIdentIcn");

	icn = malloc(xmlStrlen(imfIdentIcn) + 5);

	sprintf(icn, "ICN-%s", imfIdentIcn);

	addIcnRef(icn, dmlContent);

	free(icn);
	xmlFree(imfIdentIcn);
}

static void addComRef(xmlDocPtr com, xmlNodePtr dmlContent)
{
	xmlNodePtr dmlEntry, commentRef, commentRefIdent, responsiblePartnerCompany;

	dmlEntry = xmlNewChild(dmlContent, NULL, BAD_CAST "dmlEntry", NULL);
	commentRef = xmlNewChild(dmlEntry, NULL, BAD_CAST "commentRef", NULL);
	commentRefIdent = xmlNewChild(commentRef, NULL, BAD_CAST "commentRefIdent", NULL);

	xmlAddChild(commentRefIdent, xmlCopyNode(firstXPathNode("//commentIdent/commentCode", com), 1));
	xmlAddChild(commentRefIdent, xmlCopyNode(firstXPathNode("//commentIdent/language", com), 1));

	xmlAddChild(dmlEntry, xmlCopyNode(firstXPathNode("//commentStatus/security", com), 1));

	responsiblePartnerCompany = xmlNewChild(dmlEntry, NULL, BAD_CAST "responsiblePartnerCompany", NULL);
	if (defaultRpcCode) {
		xmlSetProp(responsiblePartnerCompany, BAD_CAST "enterpriseCode", BAD_CAST defaultRpcCode);
	}
	if (defaultRpcName) {
		xmlNewChild(responsiblePartnerCompany, NULL, BAD_CAST "enterpriseName", BAD_CAST defaultRpcName);
	}
}

static void addDmlRef(xmlDocPtr dml, xmlNodePtr dmlContent, bool csl)
{
	xmlNodePtr dmlEntry, dmlRef, dmlRefIdent, responsiblePartnerCompany;

	dmlEntry = xmlNewChild(dmlContent, NULL, BAD_CAST "dmlEntry", NULL);
	dmlRef = xmlNewChild(dmlEntry, NULL, BAD_CAST "dmlRef", NULL);
	dmlRefIdent = xmlNewChild(dmlRef, NULL, BAD_CAST "dmlRefIdent", NULL);

	xmlAddChild(dmlRefIdent, xmlCopyNode(firstXPathNode("//dmlIdent/dmlCode", dml), 1));

	if (csl) {
		xmlAddChild(dmlRefIdent, xmlCopyNode(firstXPathNode("//dmlIdent/issueInfo", dml), 1));
	}

	xmlAddChild(dmlEntry, xmlCopyNode(firstXPathNode("//dmlStatus/security", dml), 1));

	responsiblePartnerCompany = xmlNewChild(dmlEntry, NULL, BAD_CAST "responsiblePartnerCompany", NULL);
	if (defaultRpcCode) {
		xmlSetProp(responsiblePartnerCompany, BAD_CAST "enterpriseCode", BAD_CAST defaultRpcCode);
	}
	if (defaultRpcName) {
		xmlNewChild(responsiblePartnerCompany, NULL, BAD_CAST "enterpriseName", BAD_CAST defaultRpcName);
	}
}

static void copy_default_value(const char *def_key, const char *def_val)
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
	else if (strcmp(def_key, "responsiblePartnerCompany") == 0 && !defaultRpcName)
		defaultRpcName = strdup(def_val);
	else if (strcmp(def_key, "responsiblePartnerCompanyCode") == 0 && !defaultRpcCode)
		defaultRpcCode = strdup(def_val);
	else if (strcmp(def_key, "templates") == 0 && !template_dir)
		template_dir = strdup(def_val);
	else if (strcmp(def_key, "remarks") == 0 && !remarks)
		remarks = xmlStrdup(BAD_CAST def_val);
	else if (strcmp(def_key, "issue") == 0 && issue == NO_ISS)
		issue = get_issue(def_val);
	else if (strcmp(def_key, "issueType") == 0 && !issue_type)
		issue_type = xmlStrdup(BAD_CAST def_val);
}

static void add_sns(xmlNodePtr content, const char *path, const char *incode)
{
	xmlDocPtr doc;
	xmlNodePtr new_content, cur;
	const char *params[9];
	char infocode[4], variant[2], itemloc[2];
	int n, i = 0;
	char *is = NULL, *vs = NULL, *ls = NULL, *rc = NULL, *rn = NULL;

	if (!(doc = read_xml_doc(path))) {
		fprintf(stderr, E_BAD_SNS, path);
		exit(EXIT_BAD_SNS);
	}

	params[i++] = "infoCode";

	if ((n = sscanf(incode, "%3s%1s-%1s", infocode, variant, itemloc)) < 1) {
		fprintf(stderr, ERR_PREFIX "Bad info code: %s\n", incode);
		exit(EXIT_BAD_INPUT);
	}

	is = malloc(7);
	sprintf(is, "\"%s\"", infocode);
	params[i++] = is;

	if (n > 1) {
		params[i++] = "infoCodeVariant";

		vs = malloc(4);
		sprintf(vs, "\"%s\"", variant);
		params[i++] = vs;

		if (n > 2) {
			params[i++] = "itemLocationCode";

			ls = malloc(4);
			sprintf(ls, "\"%s\"", itemloc);
			params[i++] = ls;
		}
	}

	if (defaultRpcCode) {
		params[i++] = "RPCcode";
		rc = malloc(strlen(defaultRpcCode) + 3);
		sprintf(rc, "\"%s\"", defaultRpcCode);
		params[i++] = rc;
	}
	if (defaultRpcName) {
		params[i++] = "RPCname";
		rn = malloc(strlen(defaultRpcName) + 3);
		sprintf(rn, "\"%s\"", defaultRpcName);
		params[i++] = rn;
	}

	params[i++] = NULL;

	transform_doc(doc, sns2dmrl_xsl, sns2dmrl_xsl_len, params);

	free(is);
	free(vs);
	free(ls);
	free(rc);
	free(rn);

	new_content = xmlDocGetRootElement(doc);

	for (cur = new_content->children; cur; cur = cur->next) {
		if (cur->type != XML_ELEMENT_NODE) {
			continue;
		}

		xmlAddChild(content, xmlCopyNode(cur, 1));
	}

	xmlFreeDoc(doc);
}

static void sort_entries(xmlDocPtr doc)
{
	transform_doc(doc, sort_xsl, sort_xsl_len, NULL);
}

static void dump_template(const char *path)
{
	FILE *f;

	if (access(path, W_OK) == -1 || chdir(path)) {
		fprintf(stderr, E_BAD_TEMPL_DIR, path);
		exit(EXIT_BAD_TEMPL_DIR);
	}

	f = fopen("dml.xml", "w");
	fprintf(f, "%.*s", dml_xml_len, dml_xml);
	fclose(f);
}

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -$, --issue <issue>         Specify which S1000d issue to use.");
	puts("  -@, --out <path>            Output to specified file or directory.");
	puts("  -%, --templates <dir>       Use template in specified directory.");
	puts("  -~, --dump-templates <dir>  Dump built-in template to directory.");
	puts("  -d, --defaults <file>       Specify .defaults file name.");
	puts("  -f, --overwrite             Overwrite existing file.");
	puts("  -h, -?, --help              Show usage message.");
	puts("  -i, --info-code <code>      Specify info code for SNS-generated DMRL.");
	puts("  -l, --list                  Treat input as a list of objects to add to the new list.");
	puts("  -N, --omit-issue            Omit issue/inwork from filename.");
	puts("  -p, --prompt                Prompt the user for each value.");
	puts("  -q, --quiet                 Don't report an error if file exists.");
	puts("  -S, --sns <BREX>            Create a DMRL from SNS rules.");
	puts("  -v, --verbose               Print file name of DML.");
	puts("  --version                   Show version information.");
	puts("  <object>...                 CSDB object(s) to add to the new list.");
	puts("");
	puts("In addition, the following pieces of metadata can be set:");
	puts("  -#, --code <code>           DML code");
	puts("  -b, --brex <BREX>           BREX data module code");
	puts("  -c, --security <sec>        Security classification");
	puts("  -I, --date <date>           Issue date");
	puts("  -m, --remarks <remarks>     Remarks");
	puts("  -n, --issno <iss>           Issue number");
	puts("  -R, --rpccode <CAGE>        Default RPC code");
	puts("  -r, --rpcname <RPC>         Default RPC name");
	puts("  -w, --inwork <inwork>       Inwork issue");
	puts("  -z, --issue-type <type>     Issue type");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
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
	xmlNodePtr remarks;

	remarks = firstXPathNode("//remarks", doc);

	if (text) {
		xmlNewChild(remarks, NULL, BAD_CAST "simplePara", text);
	} else {
		xmlUnlinkNode(remarks);
		xmlFreeNode(remarks);
	}
}

static void add_ref(xmlNodePtr dmlContent, char *path)
{
	xmlDocPtr doc = read_xml_doc(path);

	if (doc) {
		if (isdm(doc)) {
			addDmRef(doc, dmlContent, strcmp(dml_type, "S") == 0);
		} else if (ispm(doc)) {
			addPmRef(doc, dmlContent, strcmp(dml_type, "S") == 0);
		} else if (isimf(doc)) {
			addImfRef(doc, dmlContent);
		} else if (iscom(doc)) {
			addComRef(doc, dmlContent);
		} else if (isdml(doc)) {
			addDmlRef(doc, dmlContent, strcmp(dml_type, "S") == 0);
		}

		xmlFreeDoc(doc);
	} else {
		char *base = basename(path);

		if (isicn(base)) {
			addIcnRef(base, dmlContent);
		}
	}
}

static void add_ref_list(xmlNodePtr dmlContent, const char *fname)
{
	FILE *f;
	char path[PATH_MAX];

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			fprintf(stderr, E_BAD_LIST, fname);
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(path, PATH_MAX, f)) {
		strtok(path, "\t\r\n");
		add_ref(dmlContent, path);
	}

	if (fname) {
		fclose(f);
	}
}

int main(int argc, char **argv)
{
	xmlDocPtr dml_doc;

	xmlNodePtr dmlCode, issueInfo, security, issueDate, dmlContent, dmlStatus, sns_incodes;
	
	xmlXPathContextPtr ctxt;
	xmlXPathObjectPtr results;

	char defaults_fname[PATH_MAX];
	bool custom_defaults = false;

	bool showprompts = false;
	char code[256] = "";
	bool skipcode = false;
	bool noissue = false;
	bool verbose = false;
	bool overwrite = false;
	bool no_overwrite_error = false;
	char *sns = NULL;
	bool islist = false;

	int c;

	xmlDocPtr defaults_xml;

	char *out = NULL;
	char *outdir = NULL;

	const char *sopts = "pd:#:n:w:c:Nb:I:vf$:@:r:R:%:qS:i:m:~:z:lh?";
	struct option lopts[] = {
		{"version"       , no_argument      , 0, 0},
		{"help"          , no_argument      , 0, 'h'},
		{"prompt"        , no_argument      , 0, 'p'},
		{"defaults"      , required_argument, 0, 'd'},
		{"code"          , required_argument, 0, '#'},
		{"issno"         , required_argument, 0, 'n'},
		{"inwork"        , required_argument, 0, 'w'},
		{"security"      , required_argument, 0, 'c'},
		{"omit-issue"    , no_argument      , 0, 'N'},
		{"brex"          , required_argument, 0, 'b'},
		{"date"          , required_argument, 0, 'I'},
		{"verbose"       , no_argument      , 0, 'v'},
		{"overwrite"     , no_argument      , 0, 'f'},
		{"issue"         , required_argument, 0, '$'},
		{"out"           , required_argument, 0, '@'},
		{"rpcname"       , required_argument, 0, 'r'},
		{"rpccode"       , required_argument, 0, 'R'},
		{"templates"     , required_argument, 0, '%'},
		{"quiet"         , no_argument      , 0, 'q'},
		{"sns"           , required_argument, 0, 'S'},
		{"info-code"     , required_argument, 0, 'i'},
		{"remarks"       , required_argument, 0, 'm'},
		{"dump-templates", required_argument, 0, '~'},
		{"issue-type"    , required_argument, 0, 'z'},
		{"list"          , no_argument      , 0, 'l'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	sns_incodes = xmlNewNode(NULL, BAD_CAST "incodes");

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'p': showprompts = true; break;
			case 'd': strcpy(defaults_fname, optarg); custom_defaults = true; break;
			case '#': strcpy(code, optarg); skipcode = true; break;
			case 'n': strcpy(issue_number, optarg); break;
			case 'w': strcpy(in_work, optarg); break;
			case 'c': strcpy(security_classification, optarg); break;
			case 'N': noissue = true; break;
			case 'b': strncpy(brex_dmcode, optarg, 255); break;
			case 'I': strncpy(issue_date, optarg, 15); break;
			case 'v': verbose = true; break;
			case 'f': overwrite = true; break;
			case '$': issue = get_issue(optarg); break;
			case '@': out = strdup(optarg); break;
			case 'r': defaultRpcName = strdup(optarg); break;
			case 'R': defaultRpcCode = strdup(optarg); break;
			case '%': template_dir = strdup(optarg); break;
			case 'q': no_overwrite_error = true; break;
			case 'S': sns = strdup(optarg); break;
			case 'i': xmlNewChild(sns_incodes, NULL, BAD_CAST "incode", BAD_CAST optarg); break;
			case 'm': remarks = xmlStrdup(BAD_CAST optarg); break;
			case '~': dump_template(optarg); return 0;
			case 'z': issue_type = xmlStrdup(BAD_CAST optarg); break;
			case 'l': islist = true; break;
			case 'h':
			case '?': show_help(); return 0;
		}
	}

	if (!custom_defaults) {
		find_config(defaults_fname, DEFAULT_DEFAULTS_FNAME);
	}

	if (!sns_incodes->children) {
		xmlNewChild(sns_incodes, NULL, BAD_CAST "incode", BAD_CAST "000");
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

	if (strcmp(code, "") != 0) {
		int n, offset;

		offset = strncmp(code, "DML-", 4) == 0 ? 4 : 0;

		n = sscanf(code + offset, "%14[^-]-%5[^-]-%c-%4[^-]-%5[^-]",
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

	if (strcmp(model_ident_code, "") == 0 ||
	    strcmp(sender_ident, "") == 0 ||
	    strcmp(dml_type, "") == 0 ||
	    strcmp(year_of_data_issue, "") == 0 ||
	    strcmp(seq_number, "") == 0) {

		fprintf(stderr, ERR_PREFIX "Missing required DML code components: ");
		fprintf(stderr, "DML-%s-%s-%s-%s-%s\n",
			strcmp(model_ident_code, "") == 0   ? "???" : model_ident_code,
			strcmp(sender_ident, "") == 0       ? "???" : sender_ident,
			strcmp(dml_type, "") == 0           ? "???" : dml_type,
			strcmp(year_of_data_issue, "") == 0 ? "???" : year_of_data_issue,
			strcmp(seq_number, "") == 0         ? "???" : seq_number);

		exit(EXIT_BAD_CODE);
	}

	if (issue == NO_ISS) issue = DEFAULT_S1000D_ISSUE;
	if (strcmp(issue_number, "") == 0) strcpy(issue_number, "000");
	if (strcmp(in_work, "") == 0) strcpy(in_work, "01");
	if (strcmp(security_classification, "") == 0) strcpy(security_classification, "01");

	dml_doc = xml_skeleton();

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
	xmlXPathFreeObject(results);

	dml_type[0] = tolower(dml_type[0]);

	xmlSetProp(dmlCode, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
	xmlSetProp(dmlCode, BAD_CAST "senderIdent", BAD_CAST sender_ident);
	xmlSetProp(dmlCode, BAD_CAST "dmlType", BAD_CAST dml_type);
	xmlSetProp(dmlCode, BAD_CAST "yearOfDataIssue", BAD_CAST year_of_data_issue);
	xmlSetProp(dmlCode, BAD_CAST "seqNumber", BAD_CAST seq_number);

	xmlSetProp(issueInfo, BAD_CAST "issueNumber", BAD_CAST issue_number);
	xmlSetProp(issueInfo, BAD_CAST "inWork", BAD_CAST in_work);

	xmlSetProp(security, BAD_CAST "securityClassification", BAD_CAST security_classification);
	
	set_issue_date(issueDate);

	dmlStatus = firstXPathNode("//dmlStatus", dml_doc);
	if (issue_type) xmlSetProp(dmlStatus, BAD_CAST "issueType", issue_type);

	xmlXPathFreeContext(ctxt);

	if (strcmp(brex_dmcode, "") != 0)
		set_brex(dml_doc, brex_dmcode);

	set_remarks(dml_doc, remarks);

	dml_type[0] = toupper(dml_type[0]);

	dmlContent = firstXPathNode("//dmlContent", dml_doc);

	if (sns) {
		xmlNodePtr incode;
		for (incode = sns_incodes->children; incode; incode = incode->next) {
			char *inc;
			inc = (char *) xmlNodeGetContent(incode);
			add_sns(dmlContent, sns, inc);
			xmlFree(inc);
		}
	}

	if (optind < argc) {
		for (c = optind; c < argc; ++c) {
			if (islist) {
				add_ref_list(dmlContent, argv[c]);
			} else {
				add_ref(dmlContent, argv[c]);
			}
		}
	} else if (islist) {
		add_ref_list(dmlContent, NULL);
	}

	sort_entries(dml_doc);

	if (issue < ISS_50) {
		if (strcmp(brex_dmcode, "") == 0) {
			switch (issue) {
				case ISS_22:
					set_brex(dml_doc, ISS_22_DEFAULT_BREX);
					break;
				case ISS_23:
					set_brex(dml_doc, ISS_23_DEFAULT_BREX);
					break;
				case ISS_30:
					set_brex(dml_doc, ISS_30_DEFAULT_BREX);
					break;
				case ISS_40:
					set_brex(dml_doc, ISS_40_DEFAULT_BREX);
					break;
				case ISS_41:
					set_brex(dml_doc, ISS_41_DEFAULT_BREX);
					break;
				case ISS_42:
					set_brex(dml_doc, ISS_42_DEFAULT_BREX);
					break;
				default:
					break;
			}
		}

		dml_doc = toissue(dml_doc, issue);
	}

	if (out && isdir(out, false)) {
		outdir = out;
		out = NULL;
	}

	if (!out) {
		char dml_fname[PATH_MAX];

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

		out = strdup(dml_fname);
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
		exit(EXIT_DML_EXISTS);
	}

	save_xml_doc(dml_doc, out);

	if (verbose) {
		if (outdir) {
			printf("%s/%s\n", outdir, out);
		} else {
			puts(out);
		}
	}

	free(out);
	free(outdir);
	free(defaultRpcName);
	free(defaultRpcCode);
	free(template_dir);
	xmlFree(remarks);
	xmlFree(issue_type);
	free(sns);
	xmlFreeDoc(dml_doc);
	xmlFreeNode(sns_incodes);

	xmlCleanupParser();
	xsltCleanupGlobals();

	return 0;
}
