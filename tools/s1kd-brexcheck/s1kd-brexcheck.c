#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <libgen.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/debugXML.h>
#include <libxml/xmlregexp.h>

#include "brex.h"
#include "s1kd_tools.h"

#define STRUCT_OBJ_RULE_PATH \
	"//contextRules[not(@rulesContext) or @rulesContext='%s']//structureObjectRule|" \
	"//contextrules[not(@context) or @context='%s']//objrule"
#define BREX_REF_DMCODE_PATH BAD_CAST "//brexDmRef//dmCode|//brexref//avee"

#define XSI_URI BAD_CAST "http://www.w3.org/2001/XMLSchema-instance"

#define PROG_NAME "s1kd-brexcheck"
#define VERSION "1.4.2"

#define E_PREFIX PROG_NAME ": ERROR: "
#define F_PREFIX PROG_NAME ": FAILED: "
#define S_PREFIX PROG_NAME ": SUCCESS: "
#define I_PREFIX PROG_NAME ": INFO: "

#define E_NODMOD E_PREFIX "Data module %s not found.\n"
#define E_NODMOD_STDIN E_PREFIX "stdin does not contain valid XML.\n"
#define E_NOBREX E_PREFIX "No BREX data module found for %s.\n"
#define E_NOBREX_STDIN E_PREFIX "No BREX found for data module on stdin.\n"
#define E_MAXBREX E_PREFIX "Too many BREX data modules specified.\n"
#define E_MAXBREXPATH E_PREFIX "Too many BREX search paths specified.\n"
#define E_MAXDMOD E_PREFIX "Too many data modules specified.\n"
#define E_INVOBJPATH E_PREFIX "Invalid object path.\n"
#define E_MISSBREX E_PREFIX "Could not find BREX file \"%s\".\n"
#define E_NOBREX_LAYER E_PREFIX "No BREX data module found for BREX %s.\n"
#define E_INVALIDDOC F_PREFIX "%s failed to validate against BREX %s.\n"
#define E_VALIDDOC S_PREFIX "%s validated successfully against BREX %s.\n"
#define E_BAD_LIST E_PREFIX "Could not read list: %s\n"

#define I_FILE_FOUND I_PREFIX "Found file for BREX %s: %s\n"

#define ERR_MISSING_BREX_FILE -1
#define ERR_UNFINDABLE_BREX_FILE -2
#define ERR_MISSING_DMODULE -3
#define ERR_INVALID_OBJ_PATH -4
#define ERR_MAX_BREX -5
#define ERR_MAX_BREX_PATH -6
#define ERR_MAX_DMOD -7

#define BREX_MAX 1024
#define BREX_PATH_MAX 1024
#define DMOD_MAX 10240

#define PROGRESS_BAR_WIDTH 60

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

enum verbosity {SILENT, NORMAL, VERBOSE, DEBUG};

enum verbosity verbose = NORMAL;
bool shortmsg = false;

char *brsl_fname = NULL;
xmlDocPtr brsl;

bool check_sns = false;
bool strict_sns = false;
bool unstrict_sns = false;

bool check_notation = false;

bool check_values = false;

xmlNodePtr firstXPathNode(xmlDocPtr doc, xmlNodePtr context, const char *xpath)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;

	ctx = xmlXPathNewContext(doc ? doc : context->doc);
	ctx->node = context;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval))
		node = NULL;
	else
		node = obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return node;
}

xmlChar *firstXPathValue(xmlNodePtr node, const char *expr)
{
	return xmlNodeGetContent(firstXPathNode(NULL, node, expr));
}

/* Tests whether a value is in an S1000D range (a~c is equivalent to a|b|c) */
bool is_in_range(const char *value, const char *range)
{
	char *ran;
	char *first;
	char *last;
	bool ret;

	if (!strchr(range, '~')) {
		return strcmp(value, range) == 0;
	}

	ran = malloc(strlen(range) + 1);

	strcpy(ran, range);

	first = strtok(ran, "~");
	last = strtok(NULL, "~");

	ret = strcmp(value, first) >= 0 && strcmp(value, last) <= 0;

	free(ran);

	return ret;
}

/* Tests whether a value is in an S1000D set (a|b|c) */
bool is_in_set(const char *value, const char *set)
{
	char *s;
	char *val = NULL;
	bool ret = false;

	if (!strchr(set, '|')) {
		return is_in_range(value, set);
	}

	s = malloc(strlen(set) + 1);

	strcpy(s, set);

	while ((val = strtok(val ? NULL : s, "|"))) {
		if (is_in_range(value, val)) {
			ret = true;
			break;
		}
	}

	free(s);

	return ret;
}

bool match_pattern(const char *value, const char *pattern)
{
	xmlRegexpPtr regex;
	bool match;
	regex = xmlRegexpCompile(BAD_CAST pattern);
	match = xmlRegexpExec(regex, BAD_CAST value);
	xmlRegFreeRegexp(regex);
	return match;
}

bool check_node_values(xmlNodePtr node, xmlNodeSetPtr values)
{
	int i;
	bool ret = false;

	if (xmlXPathNodeSetIsEmpty(values))
		return true;

	for (i = 0; i < values->nodeNr; ++i) {
		char *allowed, *value, *form;

		allowed = (char *) firstXPathValue(values->nodeTab[i], "@valueAllowed|@val1");
		form    = (char *) firstXPathValue(values->nodeTab[i], "@valueForm|@valtype");
		value   = (char *) xmlNodeGetContent(node);

		if (form && strcmp(form, "range") == 0) {
			ret = ret || is_in_set(value, allowed);
		} else if (form && strcmp(form, "pattern") == 0) {
			ret = ret || match_pattern(value, allowed);
		} else {
			ret = ret || strcmp(value, allowed) == 0;
		}

		xmlFree(allowed);
		xmlFree(form);
		xmlFree(value);
	}

	return ret;
}

bool check_single_object_values(xmlNodePtr rule, xmlNodePtr node)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	bool ret;

	ctx = xmlXPathNewContext(rule->doc);
	ctx->node = rule;

	obj = xmlXPathEvalExpression(BAD_CAST "objectValue|objval", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		ret = check_node_values(node, obj->nodesetval);
	} else {
		ret = false;
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return ret;
}

bool check_objects_values(xmlNodePtr rule, xmlNodeSetPtr nodes)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	bool ret = true;

	if (xmlXPathNodeSetIsEmpty(nodes))
		return true;

	ctx = xmlXPathNewContext(rule->doc);
	ctx->node = rule;

	obj = xmlXPathEvalExpression(BAD_CAST "objectValue|objval", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < nodes->nodeNr; ++i) {
			if (!check_node_values(nodes->nodeTab[i], obj->nodesetval)) {
				ret = false;
				break;
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return ret;
}

bool is_invalid(xmlNodePtr rule, char *allowedObjectFlag, xmlXPathObjectPtr obj)
{
	bool invalid = false;

	if (allowedObjectFlag) {
		if (strcmp(allowedObjectFlag, "0") == 0) {
			if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
				invalid = obj->boolval;
			} else {
				invalid = true;
			}
		} else if (strcmp(allowedObjectFlag, "1") == 0) {
			if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
				invalid = !obj->boolval;
			} else {
				invalid = false;
			}
		}
	}

	if (!invalid && check_values)
		invalid = !check_objects_values(rule, obj->nodesetval);

	return invalid;
}

void dump_nodes_xml(xmlNodeSetPtr nodes, const char *fname, xmlNodePtr brexError, xmlNodePtr rule)
{
	int i;

	for (i = 0; i < nodes->nodeNr; ++i) {
		xmlNodePtr node = nodes->nodeTab[i];
		xmlNodePtr object;
		char line_s[16];
		xmlChar *xpath;

		if (check_values && check_single_object_values(rule, node)) {
			continue;
		}

		snprintf(line_s, 16, "%ld", xmlGetLineNo(node));
		object = xmlNewChild(brexError, NULL, BAD_CAST "object", NULL);
		xmlSetProp(object, BAD_CAST "line", BAD_CAST line_s);

		xpath = xpath_of(node);
		xmlSetProp(object, BAD_CAST "xpath", xpath);
		xmlFree(xpath);

		if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;

		xmlAddChild(object, xmlCopyNode(node, 2));
	}
}

bool is_xml_file(const char *fname)
{
	return strcasecmp(fname + (strlen(fname) - 4), ".XML") == 0;
}

/* Search for the BREX in a specified directory. */
bool search_brex_fname(char *fname, const char *dpath, const char *dmcode, int len)
{
	DIR *dir;
	struct dirent *cur;

	bool found = false;

	char tmp_fname[PATH_MAX] = "";

	dir = opendir(dpath);

	while ((cur = readdir(dir))) {
		if (strncmp(cur->d_name, dmcode, len) == 0) {

			snprintf(tmp_fname, PATH_MAX, "%s/%s", dpath, cur->d_name);

			if (is_xml_file(tmp_fname) && (!found || strcmp(tmp_fname, fname) > 0)) {
				strcpy(fname, tmp_fname);
			}
			
			found = true;
		}
	}

	closedir(dir);

	return found;
}

/* Search for the BREX in the list of CSDB objects to be checked. */
bool search_brex_fname_from_dmods(char *fname,
	char dmod_fnames[DMOD_MAX][PATH_MAX], int num_dmod_fnames,
	char *dmcode, int len)
{
	int i;
	bool found = false;

	for (i = 0; i < num_dmod_fnames; ++i) {
		char *name, *base;

		name = strdup(dmod_fnames[i]);
		base = basename(name);
		found = strncmp(base, dmcode, len) == 0;
		free(name);

		if (found) {
			strcpy(fname, dmod_fnames[i]);
			break;
		}
	}

	return found;
}

/* Search for the BREX in the built-in default BREX data modules. */
bool search_brex_fname_from_default_brex(char *fname, char *dmcode, int len)
{
	return
		(strcmp(dmcode, "DMC-S1000D-F-04-10-0301-00A-022A-D") == 0 ||
		 strcmp(dmcode, "DMC-S1000D-E-04-10-0301-00A-022A-D") == 0 ||
		 strcmp(dmcode, "DMC-S1000D-A-04-10-0301-00A-022A-D") == 0 ||
		 strcmp(dmcode, "DMC-AE-A-04-10-0301-00A-022A-D") == 0) &&
		strcpy(fname, dmcode);
}

/* Find the filename of a BREX data module referenced by a CSDB object. */
bool find_brex_fname_from_doc(char *fname, xmlDocPtr doc, char spaths[BREX_PATH_MAX][PATH_MAX],
	int nspaths, char dmod_fnames[DMOD_MAX][PATH_MAX], int num_dmod_fnames)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr object;

	xmlNodePtr dmCode;

	char *modelIdentCode;
	char *systemDiffCode;
	char *systemCode;
	char *subSystemCode;
	char *subSubSystemCode;
	char *assyCode;
	char *disassyCode;
	char *disassyCodeVariant;
	char *infoCode;
	char *infoCodeVariant;
	char *itemLocationCode;

	char dmcode[256];
	int len;

	bool found;

	context = xmlXPathNewContext(doc);

	object = xmlXPathEvalExpression(BREX_REF_DMCODE_PATH, context);

	if (xmlXPathNodeSetIsEmpty(object->nodesetval)) {
		xmlXPathFreeObject(object);
		xmlXPathFreeContext(context);
		return false;
	}

	dmCode = object->nodesetval->nodeTab[0];

	xmlXPathFreeObject(object);
	xmlXPathFreeContext(context);

	if (xmlStrcmp(dmCode->name, BAD_CAST "dmCode") == 0) {
		modelIdentCode     = (char *) xmlGetProp(dmCode, BAD_CAST "modelIdentCode");
		systemDiffCode     = (char *) xmlGetProp(dmCode, BAD_CAST "systemDiffCode");
		systemCode         = (char *) xmlGetProp(dmCode, BAD_CAST "systemCode");
		subSystemCode      = (char *) xmlGetProp(dmCode, BAD_CAST "subSystemCode");
		subSubSystemCode   = (char *) xmlGetProp(dmCode, BAD_CAST "subSubSystemCode");
		assyCode           = (char *) xmlGetProp(dmCode, BAD_CAST "assyCode");
		disassyCode        = (char *) xmlGetProp(dmCode, BAD_CAST "disassyCode");
		disassyCodeVariant = (char *) xmlGetProp(dmCode, BAD_CAST "disassyCodeVariant");
		infoCode           = (char *) xmlGetProp(dmCode, BAD_CAST "infoCode");
		infoCodeVariant    = (char *) xmlGetProp(dmCode, BAD_CAST "infoCodeVariant");
		itemLocationCode   = (char *) xmlGetProp(dmCode, BAD_CAST "itemLocationCode");
	} else {
		modelIdentCode     = (char *) firstXPathValue(dmCode, "modelic");
		systemDiffCode     = (char *) firstXPathValue(dmCode, "sdc");
		systemCode         = (char *) firstXPathValue(dmCode, "chapnum");
		subSystemCode      = (char *) firstXPathValue(dmCode, "section");
		subSubSystemCode   = (char *) firstXPathValue(dmCode, "subsect");
		assyCode           = (char *) firstXPathValue(dmCode, "subject");
		disassyCode        = (char *) firstXPathValue(dmCode, "discode");
		disassyCodeVariant = (char *) firstXPathValue(dmCode, "discodev");
		infoCode           = (char *) firstXPathValue(dmCode, "incode");
		infoCodeVariant    = (char *) firstXPathValue(dmCode, "incodev");
		itemLocationCode   = (char *) firstXPathValue(dmCode, "itemloc");
	}

	snprintf(dmcode, 256, "DMC-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
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
		itemLocationCode);

	xmlFree(modelIdentCode);
	xmlFree(systemDiffCode);
	xmlFree(systemCode);
	xmlFree(subSystemCode);
	xmlFree(subSubSystemCode);
	xmlFree(assyCode);
	xmlFree(disassyCode);
	xmlFree(disassyCodeVariant);
	xmlFree(infoCode);
	xmlFree(infoCodeVariant);
	xmlFree(itemLocationCode);

	len = strlen(dmcode);

	/* Look for the BREX in the current directory. */
	found = search_brex_fname(fname, ".", dmcode, len);

	/* Look for the BREX in any of the specified search paths. */
	if (!found) {
		int i;

		for (i = 0; i < nspaths; ++i) {
			found = search_brex_fname(fname, spaths[i], dmcode, len);
		}
	}

	/* Look for the BREX in the list of objects to check. */
	if (!found) {
		found = search_brex_fname_from_dmods(fname, dmod_fnames, num_dmod_fnames, dmcode, len);
	}

	/* Look for the BREX in the built-in default BREX. */
	if (!found) {
		found = search_brex_fname_from_default_brex(fname, dmcode, len);
	}

	if (verbose >= DEBUG && found) {
		fprintf(stderr, I_FILE_FOUND, dmcode, fname);
	}

	return found;
}

bool is_failure(xmlChar *severity)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	bool ret = true;

	ctx = xmlXPathNewContext(brsl);
	obj = xmlXPathEvalExpression(BAD_CAST "//brSeverityLevel", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *value;
			bool match;

			value = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "value");
			match = xmlStrcmp(value, severity) == 0;
			xmlFree(value);

			if (match) {
				xmlChar *fail;

				fail = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "fail");
				ret = xmlStrcmp(fail, BAD_CAST "no") != 0;
				xmlFree(fail);
				break;
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return ret;
}

xmlChar *brsl_type(xmlChar *severity)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	char xpath[256];
	xmlChar *type;

	sprintf(xpath, "//brSeverityLevel[@value = '%s']", (char *) severity);

	ctx = xmlXPathNewContext(brsl);
	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		type = NULL;
	} else {
		type = xmlNodeGetContent(obj->nodesetval->nodeTab[0]);
	}
	
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return type;
}

void add_object_values(xmlNodePtr brexError, xmlNodePtr rule)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(rule->doc);
	ctx->node = rule;

	obj = xmlXPathEvalExpression(BAD_CAST "objectValue|objval", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlAddChild(brexError, xmlCopyNode(obj->nodesetval->nodeTab[i], 1));
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

int check_brex_rules(xmlDocPtr brex_doc, xmlNodeSetPtr rules, xmlDocPtr doc, const char *fname,
	const char *brexfname, xmlNodePtr brexCheck)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr object;

	xmlNodePtr objectPath, objectUse;
	xmlChar *defaultBrSeverityLevel;

	int i;

	int nerr = 0;
	
	xmlNodePtr brexError;

	context = xmlXPathNewContext(doc);
	xmlXPathRegisterNs(context, BAD_CAST "xsi", XSI_URI);

	defaultBrSeverityLevel = xmlGetProp(firstXPathNode(brex_doc, NULL, "//brex"), BAD_CAST "defaultBrSeverityLevel");

	for (i = 0; i < rules->nodeNr; ++i) {
		xmlChar *allowedObjectFlag, *path, *use;

		objectPath = firstXPathNode(brex_doc, rules->nodeTab[i], "objectPath|objpath");
		objectUse  = firstXPathNode(brex_doc, rules->nodeTab[i], "objectUse|objuse");

		allowedObjectFlag = firstXPathValue(objectPath, "@allowedObjectFlag|@objappl");
		path = xmlNodeGetContent(objectPath);
		use  = xmlNodeGetContent(objectUse);

		object = xmlXPathEvalExpression(BAD_CAST path, context);

		if (!object) {
			if (verbose > SILENT) {
				fprintf(stderr, E_INVOBJPATH);
			}
			exit(ERR_INVALID_OBJ_PATH);
		}

		if (is_invalid(rules->nodeTab[i], (char *) allowedObjectFlag, object)) {
			char rpath[PATH_MAX];
			xmlChar *severity;
			xmlNodePtr err_path;

			if (!(severity = xmlGetProp(rules->nodeTab[i], BAD_CAST "brSeverityLevel"))) {
				severity = xmlStrdup(defaultBrSeverityLevel);
			}

			brexError = xmlNewChild(brexCheck, NULL, BAD_CAST "brexError",
				NULL);

			xmlNewChild(brexError, NULL, BAD_CAST "document", BAD_CAST real_path(fname, rpath));
			xmlNewChild(brexError, NULL, BAD_CAST "brex", BAD_CAST real_path(brexfname, rpath));

			if (severity) {
				xmlNewChild(brexError, NULL, BAD_CAST "severity", severity);

				if (brsl_fname) {
					xmlChar *type = brsl_type(severity);
					xmlNewChild(brexError, NULL, BAD_CAST "type", type);
					xmlFree(type);
				}
			}

			err_path = xmlNewChild(brexError, NULL, BAD_CAST "objectPath", path);
			xmlSetProp(err_path, BAD_CAST "allowedObjectFlag", allowedObjectFlag);
			xmlNewChild(brexError, NULL, BAD_CAST "objectUse", use);

			add_object_values(brexError, rules->nodeTab[i]);

			if (!xmlXPathNodeSetIsEmpty(object->nodesetval)) {
				dump_nodes_xml(object->nodesetval, fname,
					brexError, rules->nodeTab[i]);
			}
			
			if (severity) {
				if (is_failure(severity)) {
					++nerr;
				}
			} else {
				++nerr;
			}

			xmlFree(severity);
		}

		xmlXPathFreeObject(object);
		xmlFree(allowedObjectFlag);
		xmlFree(path);
		xmlFree(use);
	}

	xmlFree(defaultBrSeverityLevel);
	xmlXPathFreeContext(context);

	return nerr;
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-b <brex>] [-I <path>] [-w <sev>] [-BcDfLlnpqS[tu]svxh?] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -h -?        Show this help message.");
	puts("  -B           Use the default BREX.");
	puts("  -b <brex>    Use <brex> as the BREX data module.");
	puts("  -c           Check object values.");
	puts("  -D           Debug mode.");
	puts("  -f           Output only filenames of invalid modules.");
	puts("  -I <path>    Add <path> to search path for BREX data module.");
	puts("  -L           Input is a list of data module filenames.");
	puts("  -l           Check BREX referenced by other BREX.");
	puts("  -n           Check notation rules.");
	puts("  -p           Display progress bar.");
	puts("  -q           Quiet mode.");
	puts("  -S[tu]       Check SNS rules (normal, strict, unstrict).");
	puts("  -s           Short messages.");
	puts("  -v           Verbose mode.");
	puts("  -w <sev>     List of severity levels.");
	puts("  -x           XML output.");
	puts("  --version    Show version information.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

xmlDocPtr load_brex(const char *name)
{
	if (access(name, F_OK) != -1) {
		return xmlReadFile(name, NULL, PARSE_OPTS);
	} else {
		unsigned char *xml = NULL;
		unsigned int len = 0;

		if (strcmp(name, "DMC-S1000D-F-04-10-0301-00A-022A-D") == 0) {
			xml = brex_DMC_S1000D_F_04_10_0301_00A_022A_D_001_00_EN_US_XML;
			len = brex_DMC_S1000D_F_04_10_0301_00A_022A_D_001_00_EN_US_XML_len;
		} else if (strcmp(name, "DMC-S1000D-E-04-10-0301-00A-022A-D") == 0) {
			xml = brex_DMC_S1000D_E_04_10_0301_00A_022A_D_009_00_EN_US_XML;
			len = brex_DMC_S1000D_E_04_10_0301_00A_022A_D_009_00_EN_US_XML_len;
		} else if (strcmp(name, "DMC-S1000D-A-04-10-0301-00A-022A-D") == 0) {
			xml = brex_DMC_S1000D_A_04_10_0301_00A_022A_D_004_00_EN_US_XML;
			len = brex_DMC_S1000D_A_04_10_0301_00A_022A_D_004_00_EN_US_XML_len;
		} else if (strcmp(name, "DMC-AE-A-04-10-0301-00A-022A-D") == 0) {
			xml = brex_DMC_AE_A_04_10_0301_00A_022A_D_003_00_XML;
			len = brex_DMC_AE_A_04_10_0301_00A_022A_D_003_00_XML_len;
		}

		return xmlReadMemory((const char *) xml, len, NULL, NULL, 0);
	}
}

bool should_check(xmlChar *code, char *path, xmlDocPtr snsRulesDoc, xmlNodePtr ctx)
{
	bool ret;

	if (strict_sns) return true;

	if (unstrict_sns)
		return firstXPathNode(snsRulesDoc, ctx, path);

	if (strcmp(path, ".//snsSubSystem") == 0 || strcmp(path, ".//snsSubSubSystem") == 0) {
		ret = xmlStrcmp(code, BAD_CAST "0") != 0;
	} else {
		ret = !(xmlStrcmp(code, BAD_CAST "00") == 0 || xmlStrcmp(code, BAD_CAST "0000") == 0);
	}

	return ret || firstXPathNode(snsRulesDoc, ctx, path);
}

bool check_brex_sns(char brex_fnames[BREX_MAX][PATH_MAX], int nbrex_fnames, xmlDocPtr dmod_doc,
	const char *dmod_fname, xmlNodePtr brexCheck)
{
	xmlNodePtr dmcode;
	xmlChar *systemCode, *subSystemCode, *subSubSystemCode, *assyCode;
	char xpath[256];
	xmlNodePtr ctx = NULL;
	xmlNodePtr snsError;
	char rpath[PATH_MAX];
	char value[256];
	int i;
	xmlDocPtr snsRulesDoc;
	xmlNodePtr snsRulesGroup;
	bool correct = true;

	/* Only check SNS in data modules. */
	if (xmlStrcmp(xmlDocGetRootElement(dmod_doc)->name, BAD_CAST "dmodule") != 0)
		return correct;

	/* The valid SNS is taken as a combination of the snsRules from all specified BREX data modules. */
	snsRulesDoc = xmlNewDoc(BAD_CAST "1.0");
	xmlDocSetRootElement(snsRulesDoc, xmlNewNode(NULL, BAD_CAST "snsRulesGroup"));
	snsRulesGroup = xmlDocGetRootElement(snsRulesDoc);

	for (i = 0; i < nbrex_fnames; ++i) {
		xmlDocPtr brex;

		brex = load_brex(brex_fnames[i]);

		xmlAddChild(snsRulesGroup, xmlCopyNode(firstXPathNode(brex, NULL, "//snsRules"), 1));

		xmlFreeDoc(brex);
	}

	dmcode = firstXPathNode(dmod_doc, NULL, "//dmIdent/dmCode");

	systemCode       = xmlGetProp(dmcode, BAD_CAST "systemCode");
	subSystemCode    = xmlGetProp(dmcode, BAD_CAST "subSystemCode");
	subSubSystemCode = xmlGetProp(dmcode, BAD_CAST "subSubSystemCode");
	assyCode         = xmlGetProp(dmcode, BAD_CAST "assyCode");

	snsError = xmlNewNode(NULL, BAD_CAST "snsError");
	xmlNewChild(snsError, NULL, BAD_CAST "document", BAD_CAST real_path(dmod_fname, rpath));

	/* Check the SNS of the data module against the SNS rules in descending order. */

	/* System code. */
	if (should_check(systemCode, "//snsSystem", snsRulesDoc, ctx)) {
		sprintf(xpath, "//snsSystem[snsCode = '%s']", (char *) systemCode);
		if (!(ctx = firstXPathNode(snsRulesDoc, ctx, xpath))) {
			xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "systemCode");
			xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", systemCode);
			xmlAddChild(brexCheck, snsError);
			correct = false;
		}
	}

	/* Subsystem code. */
	if (correct && should_check(subSystemCode, ".//snsSubSystem", snsRulesDoc, ctx)) {
		sprintf(xpath, ".//snsSubSystem[snsCode = '%s']", (char *) subSystemCode);
		if (!(ctx = firstXPathNode(snsRulesDoc, ctx, xpath))) {
			xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "subSystemCode");
			sprintf(value, "%s-%s", systemCode, subSystemCode);
			xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", BAD_CAST value);
			xmlAddChild(brexCheck, snsError);
			correct = false;
		}
	}

	/* Subsubsystem code. */
	if (correct && should_check(subSubSystemCode, ".//snsSubSubSystem", snsRulesDoc, ctx)) {
		sprintf(xpath, ".//snsSubSubSystem[snsCode = '%s']", (char *) subSubSystemCode);
		if (!(ctx = firstXPathNode(snsRulesDoc, ctx, xpath))) {
			xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "subSubSystemCode");
			sprintf(value, "%s-%s%s", systemCode, subSystemCode, subSubSystemCode);
			xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", BAD_CAST value);
			xmlAddChild(brexCheck, snsError);
			correct = false;
		}
	}

	/* Assembly code. */
	if (correct && should_check(assyCode, ".//snsAssy", snsRulesDoc, ctx)) {
		sprintf(xpath, ".//snsAssy[snsCode = '%s']", (char *) assyCode);
		if (!firstXPathNode(snsRulesDoc, ctx, xpath)) {
			xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "assyCode");
			sprintf(value, "%s-%s%s-%s", systemCode, subSystemCode, subSubSystemCode, assyCode);
			xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", BAD_CAST value);
			xmlAddChild(brexCheck, snsError);
			correct = false;
		}
	}

	if (correct) {
		xmlFreeNode(snsError);
	}

	xmlFree(systemCode);
	xmlFree(subSystemCode);
	xmlFree(subSubSystemCode);
	xmlFree(assyCode);
	xmlFreeDoc(snsRulesDoc);

	return correct;
}

int check_entity(xmlEntityPtr entity, xmlDocPtr notationRuleDoc,
	xmlNodePtr brexCheck, const char *docname)
{	
	char xpath[256];
	xmlNodePtr rule;
	xmlNodePtr notationError;

	sprintf(xpath, "//notationRule[notationName='%s' and notationName/@allowedNotationFlag!='0']",
		(char *) entity->content);

	if ((rule = firstXPathNode(notationRuleDoc, NULL, xpath)))
		return 0;

	sprintf(xpath, "(//notationRule[notationName='%s']|//notationRule)[1]", (char *) entity->content);
	rule = firstXPathNode(notationRuleDoc, NULL, xpath);

	notationError = xmlNewChild(brexCheck, NULL, BAD_CAST "notationError", NULL);
	xmlNewChild(notationError, NULL, BAD_CAST "document", BAD_CAST docname);
	xmlNewChild(notationError, NULL, BAD_CAST "invalidNotation", entity->content);
	xmlAddChild(notationError, xmlCopyNode(firstXPathNode(notationRuleDoc, rule, "objectUse"), 1));

	return 1;
}

int check_brex_notations(char brex_fnames[BREX_MAX][PATH_MAX], int nbrex_fnames, xmlDocPtr dmod_doc,
	const char *dmod_fname, xmlNodePtr brexCheck)
{
	xmlDocPtr notationRuleDoc;
	xmlNodePtr notationRuleGroup;
	int i;
	xmlDtdPtr dtd;
	xmlNodePtr cur;
	int invalid = 0;

	if (!(dtd = dmod_doc->intSubset))
		return 0;

	notationRuleDoc = xmlNewDoc(BAD_CAST "1.0");
	xmlDocSetRootElement(notationRuleDoc, xmlNewNode(NULL, BAD_CAST "notationRuleGroup"));
	notationRuleGroup = xmlDocGetRootElement(notationRuleDoc);

	for (i = 0; i < nbrex_fnames; ++i) {
		xmlDocPtr brex;

		brex = load_brex(brex_fnames[i]);

		xmlAddChild(notationRuleGroup, xmlCopyNode(firstXPathNode(brex, NULL, "//notationRuleList"), 1));

		xmlFreeDoc(brex);
	}

	for (cur = dtd->children; cur; cur = cur->next) {
		if (cur->type == XML_ENTITY_DECL && ((xmlEntityPtr) cur)->etype == 3) {
			invalid += check_entity((xmlEntityPtr) cur, notationRuleDoc,
				brexCheck, dmod_fname);
		}
	}

	xmlFreeDoc(notationRuleDoc);

	return invalid;
}

int check_brex(xmlDocPtr dmod_doc, const char *docname,
	char brex_fnames[BREX_MAX][PATH_MAX], int num_brex_fnames, xmlNodePtr brexCheck)
{
	xmlDocPtr brex_doc;

	int i;
	int status;
	int total = 0;
	bool valid_sns = true;
	int invalid_notations = 0;

	char *schema;
	char xpath[512];

	schema = (char *) xmlGetProp(xmlDocGetRootElement(dmod_doc), BAD_CAST "noNamespaceSchemaLocation");
	sprintf(xpath, STRUCT_OBJ_RULE_PATH, schema, schema);
	xmlFree(schema);

	if (check_sns && !(valid_sns = check_brex_sns(brex_fnames, num_brex_fnames, dmod_doc, docname, brexCheck)))
		++total;

	if (check_notation) {
		invalid_notations = check_brex_notations(brex_fnames, num_brex_fnames, dmod_doc, docname, brexCheck);
		total += invalid_notations;
	}

	for (i = 0; i < num_brex_fnames; ++i) {
		xmlXPathContextPtr context;
		xmlXPathObjectPtr result;

		brex_doc = load_brex(brex_fnames[i]);

		if (!brex_doc) {
			if (verbose > SILENT) {
				fprintf(stderr, E_MISSBREX, brex_fnames[i]);
			}
			exit(ERR_MISSING_BREX_FILE);
		}

		context = xmlXPathNewContext(brex_doc);

		result = xmlXPathEvalExpression(BAD_CAST xpath, context);

		if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
			status = check_brex_rules(brex_doc, result->nodesetval, dmod_doc, docname,
				brex_fnames[i], brexCheck);

			if (verbose >= VERBOSE) {
				fprintf(stderr, status || !valid_sns || invalid_notations ? E_INVALIDDOC : E_VALIDDOC, docname, brex_fnames[i]);
			}

			total += status;
		} else if (verbose >= VERBOSE) {
			fprintf(stderr, valid_sns && !invalid_notations ? E_VALIDDOC : E_INVALIDDOC, docname, brex_fnames[i]);
		}

		xmlXPathFreeObject(result);

		xmlXPathFreeContext(context);

		xmlFreeDoc(brex_doc);
	}

	return total;
}

void print_node(xmlNodePtr node)
{

	xmlNodePtr cur;

	if (strcmp((char *) node->name, "brexError") == 0) {
		printf("BREX ERROR: ");
	} else if (strcmp((char *) node->name, "type") == 0 && !shortmsg) {
		char *type = (char *) xmlNodeGetContent(node);
		printf("  TYPE: %s\n", type);
		xmlFree(type);
	} else if (strcmp((char *) node->name, "brex") == 0 && !shortmsg) {
		char *brex = (char *) xmlNodeGetContent(node);
		if (strcmp(brex, "") != 0) {
			printf("  BREX: %s\n", brex);
		}
		xmlFree(brex);
	} else if (strcmp((char *) node->name, "document") == 0) {
		char *doc = (char *) xmlNodeGetContent(node);
		if (shortmsg) {
			printf("%s: ", doc);
		} else {
			printf("%s\n", doc);
		}
		xmlFree(doc);
	} else if (strcmp((char *) node->name, "objectUse") == 0) {
		char *use = (char *) xmlNodeGetContent(node);
		if (shortmsg) {
			printf("%s", use);
		} else {
			printf("  %s\n", use);
		}
		xmlFree(use);
	} else if (strcmp((char *) node->name, "objectValue") == 0 && !shortmsg) {
		char *allowed = (char *) xmlGetProp(node, BAD_CAST "valueAllowed");
		char *content = (char *) xmlNodeGetContent(node);
		printf("  VALUE ALLOWED:");
		if (allowed)
			printf(" %s", allowed);
		if (content && strcmp(content, "") != 0)
			printf(" (%s)", content);
		putchar('\n');
		xmlFree(content);
		xmlFree(allowed);
	} else if (strcmp((char *) node->name, "objval") == 0 && !shortmsg) {
		char *allowed = (char *) xmlGetProp(node, BAD_CAST "val1");
		char *content = (char *) xmlNodeGetContent(node);
		printf("  VALUE ALLOWED:");
		if (allowed)
			printf(" %s", allowed);
		if (content && strcmp(content, "") != 0)
			printf(" (%s)", content);
		putchar('\n');
		xmlFree(content);
		xmlFree(allowed);
	} else if (strcmp((char *) node->name, "object") == 0) {
		char *line = (char *) xmlGetProp(node, BAD_CAST "line");
		char *path = (char *) xmlGetProp(node, BAD_CAST "xpath");
		if (shortmsg) {
			printf(" (line %s)", line);
		} else {
			printf("  line %s (%s):\n", line, path);
			xmlDebugDumpOneNode(stdout, node->children, 2);
		}
		xmlFree(line);
		xmlFree(path);
	} else if (strcmp((char *) node->name, "snsError") == 0) {
		printf("SNS ERROR: ");
	} else if (strcmp((char *) node->name, "code") == 0) {
		char *code = (char *) xmlNodeGetContent(node);
		if (!shortmsg) printf("  ");
		printf("Value of %s does not conform to SNS: ", code);
		xmlFree(code);
	} else if (strcmp((char *) node->name, "invalidValue") == 0) {
		char *value = (char *) xmlNodeGetContent(node);
		printf("%s\n", value);
		xmlFree(value);
	} else if (strcmp((char *) node->name, "notationError") == 0) {
		printf("NOTATION ERROR: ");
	} else if (strcmp((char *) node->name, "invalidNotation") == 0) {
		char *value = (char *) xmlNodeGetContent(node);
		if (!shortmsg) printf("  ");
		printf("Notation %s is not allowed", value);
		if (shortmsg)
			printf(": ");
		else
			printf(".\n");
		xmlFree(value);
	}

	for (cur = node->children; cur; cur = cur->next) {
		print_node(cur);
	}

	if (shortmsg && xmlStrcmp(node->name, BAD_CAST "brexError") == 0) {
		putchar('\n');
	}
}

void print_fnames(xmlNodePtr node)
{
	xmlNodePtr cur;

	if (xmlStrcmp(node->name, BAD_CAST "document") == 0) {
		xmlChar *fname;
		fname = xmlNodeGetContent(node);
		puts((char *) fname);
		xmlFree(fname);
	}

	for (cur = node->children; cur; cur = cur->next) {
		print_fnames(cur);
	}
}

bool brex_exists(char fname[PATH_MAX], char fnames[BREX_MAX][PATH_MAX], int nfnames, char spaths[BREX_PATH_MAX][PATH_MAX], int nspaths)
{
	int i;

	for (i = 0; i < nfnames; ++i) {
		if (strcmp(fname, fnames[i]) == 0) {
			return true;
		}
	}

	return false;
}

int add_layered_brex(char fnames[BREX_MAX][PATH_MAX], int nfnames, char spaths[BREX_PATH_MAX][PATH_MAX], int nspaths, char dmod_fnames[DMOD_MAX][PATH_MAX], int num_dmod_fnames)
{
	int i;
	int total = nfnames;

	for (i = 0; i < nfnames; ++i) {
		xmlDocPtr doc;
		char fname[PATH_MAX];
		bool found;

		doc = load_brex(fnames[i]);

		found = find_brex_fname_from_doc(fname, doc, spaths, nspaths, dmod_fnames, num_dmod_fnames);

		if (!found) {
			fprintf(stderr, E_NOBREX_LAYER, fnames[i]);
		} else if (!brex_exists(fname, fnames, nfnames, spaths, nspaths)) {
			strcpy(fnames[total++], fname);

			total = add_layered_brex(fnames, total, spaths, nspaths, dmod_fnames, num_dmod_fnames);
		}

		xmlFreeDoc(doc);
	}

	return total;
}

void show_progress(float cur, float total)
{
	float p;
	int i, b;

	p = cur / total;
	b = PROGRESS_BAR_WIDTH * p;

	fprintf(stderr, "\r[");
	for (i = 0; i < PROGRESS_BAR_WIDTH; ++i) {
		if (i < b)
			fputc('=', stderr);
		else
			fputc(' ', stderr);
	}
	fprintf(stderr, "] %d%% (%d/%d) ", (int)(p * 100.0), (int) cur, (int) total);
	if (cur == total)
		fputc('\n', stderr);
	fflush(stderr);
}

void add_dmod_list(const char *fname, char dmod_fnames[DMOD_MAX][PATH_MAX], int *num_dmod_fnames)
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

		if (*num_dmod_fnames == DMOD_MAX) {
			if (verbose > SILENT) fprintf(stderr, E_MAXDMOD);
			exit(ERR_MAX_DMOD);
		}

		strcpy(dmod_fnames[(*num_dmod_fnames)++], path);
	}

	if (fname) {
		fclose(f);
	}
}

const char *default_brex_dmc(xmlDocPtr doc)
{
	char *schema;
	const char *code;

	schema = (char *) xmlGetProp(xmlDocGetRootElement(doc), BAD_CAST "noNamespaceSchemaLocation");

	if (!schema || strstr(schema, "S1000D_4-2")) {
		code = "DMC-S1000D-F-04-10-0301-00A-022A-D";
	} else if (strstr(schema, "S1000D_4-1")) {
		code = "DMC-S1000D-E-04-10-0301-00A-022A-D";
	} else if (strstr(schema, "S1000D_4-0")) {
		code = "DMC-S1000D-A-04-10-0301-00A-022A-D";
	} else {
		code = "DMC-AE-A-04-10-0301-00A-022A-D";
	}

	xmlFree(schema);

	return code;
}

int main(int argc, char *argv[])
{
	int c;
	int i;

	xmlDocPtr dmod_doc;

	char (*brex_fnames)[PATH_MAX] = malloc(BREX_MAX * PATH_MAX);
	int num_brex_fnames = 0;

	char (*brex_search_paths)[PATH_MAX] = malloc(BREX_PATH_MAX * PATH_MAX);
	int num_brex_search_paths = 0;

	char (*dmod_fnames)[PATH_MAX] = malloc(DMOD_MAX * PATH_MAX);
	int num_dmod_fnames = 0;

	int status = 0;

	bool use_stdin = false;
	bool xmlout = false;
	bool layered = false;
	bool progress = false;
	bool only_fnames = false;
	bool is_list = false;
	bool use_default_brex = false;

	xmlDocPtr outdoc;
	xmlNodePtr brexCheck;

	const char *sopts = "Bb:DI:xvqslw:StupfncLh?";
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
			case 'B':
				use_default_brex = true;
				break;
			case 'b':
				if (num_brex_fnames == BREX_MAX) {
					if (verbose > SILENT) {
						fprintf(stderr, E_MAXBREX);
					}
					exit(ERR_MAX_BREX);
				} else {
					strcpy(brex_fnames[num_brex_fnames++], optarg);
				}
				break;
			case 'D': verbose = DEBUG; break;
			case 'I':
				if (num_brex_search_paths == BREX_PATH_MAX) {
					if (verbose > SILENT) {
						fprintf(stderr, E_MAXBREXPATH);
					}
					exit(ERR_MAX_BREX_PATH);
				} else {
					strcpy(brex_search_paths[num_brex_search_paths],
						optarg);
				}
				++num_brex_search_paths;
				break;
			case 'x': xmlout = true; break;
			case 'q': verbose = SILENT; break;
			case 'v': verbose = VERBOSE; break;
			case 's': shortmsg = true; break;
			case 'l': layered = true; break;
			case 'w': brsl_fname = strdup(optarg); break;
			case 'S': check_sns = true; break;
			case 't': strict_sns = true; break;
			case 'u': unstrict_sns = true; break;
			case 'p': progress = true; break;
			case 'f': only_fnames = true; break;
			case 'n': check_notation = true; break;
			case 'c': check_values = true; break;
			case 'L': is_list = true; break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (!brsl_fname) {
		char fname[PATH_MAX];
		if (find_config(fname, DEFAULT_BRSL_FNAME)) {
			brsl_fname = strdup(fname);
		}
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (is_list) {
				add_dmod_list(argv[i], dmod_fnames, &num_dmod_fnames);
			} else {
				if (num_dmod_fnames == DMOD_MAX) {
					if (verbose > SILENT) fprintf(stderr, E_MAXDMOD);
					exit(ERR_MAX_DMOD);
				}

				strcpy(dmod_fnames[num_dmod_fnames++], argv[i]);
			}
		}
	} else if (is_list) {
		add_dmod_list(NULL, dmod_fnames, &num_dmod_fnames);
	} else {
		strcpy(dmod_fnames[num_dmod_fnames++], "-");
		use_stdin = true;
	}

	if (brsl_fname) {
		brsl = xmlReadFile(brsl_fname, NULL, PARSE_OPTS);
	}

	outdoc = xmlNewDoc(BAD_CAST "1.0");
	brexCheck = xmlNewNode(NULL, BAD_CAST "brexCheck");
	xmlDocSetRootElement(outdoc, brexCheck);

	for (i = 0; i < num_dmod_fnames; ++i) {
		/* Indicates if a referenced BREX data module is used as
		 * opposed to one specified on the command line.
		 *
		 * The practical difference is that those specified on the
		 * command line are meant to apply to ALL data modules
		 * specified, while a referenced BREX only applies to the data
		 * module which referenced it. */
		bool ref_brex = false;

		dmod_doc = xmlReadFile(dmod_fnames[i], NULL, PARSE_OPTS);

		if (!dmod_doc) {
			if (use_stdin) {
				if (verbose > SILENT) fprintf(stderr, E_NODMOD_STDIN);
			} else {
				if (verbose > SILENT) fprintf(stderr, E_NODMOD, dmod_fnames[i]);
			}
			exit(ERR_MISSING_DMODULE);
		}

		if (num_brex_fnames == 0) {
			strcpy(brex_fnames[0], "");

			/* Override the referenced BREX with a default BREX
			 * based on which issue of the specification a data
			 * module is written to.
			 */
			if (use_default_brex) {
				strcpy(brex_fnames[0], default_brex_dmc(dmod_doc));
			} else if (!find_brex_fname_from_doc(brex_fnames[0], dmod_doc,
				brex_search_paths, num_brex_search_paths, dmod_fnames, num_dmod_fnames)) {
				if (use_stdin) {
					if (verbose > SILENT) fprintf(stderr, E_NOBREX_STDIN);
				} else {
					if (verbose > SILENT) fprintf(stderr, E_NOBREX,
						dmod_fnames[i]);
				}
				continue;
			}

			num_brex_fnames = 1;
			ref_brex = true;

			/* When using brexDmRef, if the data module is itself a
			 * BREX data module, include it as a BREX. */
			if (strcmp(brex_fnames[0], dmod_fnames[i]) != 0 && firstXPathNode(dmod_doc, NULL, "//brex"))
				strcpy(brex_fnames[num_brex_fnames++], dmod_fnames[i]);
		}

		if (layered) {
			num_brex_fnames = add_layered_brex(brex_fnames,
				num_brex_fnames, brex_search_paths,
				num_brex_search_paths,
				dmod_fnames, num_dmod_fnames);
		}

		status += check_brex(dmod_doc, dmod_fnames[i],
			brex_fnames, num_brex_fnames, brexCheck);

		xmlFreeDoc(dmod_doc);

		if (progress) 
			show_progress(i, num_dmod_fnames);

		/* If the referenced BREX was used, reset the BREX data module
		 * list, as each data module may reference a different BREX or
		 * set of BREX. */
		if (ref_brex)
			num_brex_fnames = 0;
	}

	if (progress)
		show_progress(i, num_dmod_fnames);

	if (!brexCheck->children) {
		xmlNewChild(brexCheck, NULL, BAD_CAST "noErrors", NULL);
	}

	if (!xmlout) {
		if (verbose > SILENT) {
			if (only_fnames) {
				print_fnames(brexCheck);
			} else {
				print_node(brexCheck);
			}
		}
	} else {
		xmlSaveFormatFile("-", outdoc, 1);
	}

	xmlFreeDoc(outdoc);

	if (brsl_fname) {
		xmlFreeDoc(brsl);
		free(brsl_fname);
	}

	xmlCleanupParser();

	free(brex_fnames);
	free(brex_search_paths);
	free(dmod_fnames);

	return status;
}
