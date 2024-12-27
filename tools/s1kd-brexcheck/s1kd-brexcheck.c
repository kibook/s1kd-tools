#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/debugXML.h>
#include <libxslt/transform.h>
#include <libexslt/exslt.h>
#include "brex.h"
#include "s1kd_tools.h"

#define NONE 1
#define SAXON 2
#define XQILLA 3

#if XPATH2_ENGINE == 0
#undef XPATH2_ENGINE
#endif

#ifndef XPATH2_ENGINE
#define XPATH2_ENGINE NONE
#endif

#if XPATH2_ENGINE == SAXON
#include "saxon/saxon.h"
#elif XPATH2_ENGINE == XQILLA
#include "xqilla/xqilla.h"
#endif

/* Progress formats. */
#define PROGRESS_OFF 0
#define PROGRESS_CLI 1
#define PROGRESS_ZENITY 2

#define PROG_NAME "s1kd-brexcheck"
#define VERSION "5.1.0"

#define STRUCT_OBJ_RULE_PATH BAD_CAST \
	"//contextRules[not(@rulesContext) or @rulesContext=$schema]//structureObjectRule|" \
	"//contextrules[not(@context) or @context=$schema]//objrule"

/* Prefixes on console messages. */
#define E_PREFIX PROG_NAME ": ERROR: "
#define W_PREFIX PROG_NAME ": WARNING: "
#define F_PREFIX PROG_NAME ": FAILED: "
#define S_PREFIX PROG_NAME ": SUCCESS: "

/* Error messages. */
#define E_NODMOD E_PREFIX "Could not read file \"%s\".\n"
#define E_NODMOD_STDIN E_PREFIX "stdin does not contain valid XML.\n"
#define E_BAD_LIST E_PREFIX "Could not read list: %s\n"
#define E_MAXOBJS E_PREFIX "Out of memory\n"
#define E_NOBREX_LAYER E_PREFIX "No BREX data module found for BREX %s.\n"
#define E_BREX_NOT_FOUND E_PREFIX "Could not find BREX data module: %s\n"
#define E_NOBREX E_PREFIX "No BREX data module found for %s.\n"
#define E_NOBREX_STDIN E_PREFIX "No BREX data module found for object on stdin.\n"
#define E_BAD_XPATH_VERSION E_PREFIX "Unsupported XPath version: %s\n"

/* Warning messages. */
#define W_NOBREX W_PREFIX "%s does not reference a BREX data module.\n"
#define W_NOBREX_STDIN W_PREFIX "Object on stdin does not reference a BREX data module.\n"
#define W_INVOBJPATH W_PREFIX "Ignoring invalid object path in BREX %s (%ld): %s\n"

/* Failure messages. */
#define F_INVALIDDOC F_PREFIX "%s failed to validate against BREX %s.\n"

/* Success messages. */
#define S_VALIDDOC S_PREFIX "%s validated successfully against BREX %s.\n"

/* Exit status codes. */
#define EXIT_BREX_ERROR 1
#define EXIT_BAD_DMODULE 2
#define EXIT_BREX_NOT_FOUND 3
#define EXIT_BAD_XPATH_VERSION 4
#define EXIT_MAX_OBJS 5

/* URI for the XMLSchema-instance namespace. */
#define XSI_URI BAD_CAST "http://www.w3.org/2001/XMLSchema-instance"

/* Initial maximum numbers of CSDB objects/search paths. */
static unsigned BREX_MAX = 1;
static unsigned DMOD_MAX = 1;
static unsigned BREX_PATH_MAX = 1;

/* Verbosity of the tool's output. */
enum verbosity {SILENT, NORMAL, VERBOSE};

/* Whether to use short, single-line error messages. */
static bool shortmsg = false;

/* Business rules severity levels configuration file. */
static char *brsl_fname = NULL;
static xmlDocPtr brsl;

/* Print the filenames of invalid objects. */
static enum show_fnames { SHOW_NONE, SHOW_INVALID, SHOW_VALID } show_fnames = SHOW_NONE;

/* Search for BREX data modules recursively. */
static bool recursive_search = false;

/* Directory to start search for BREX data modules in. */
static char *search_dir = NULL;

/* Output XML tree if it passes the BREX check. */
static bool output_tree = false;

/* Ignore empty/non-XML files. */
static bool ignore_empty = false;

/* Remove elements marked as "delete" before check. */
static bool rem_delete = false;

/* Whether to do a deep copy of invalid nodes for the XML report. */
static bool deep_copy_nodes = false;

/* Version of XPath to use. */
enum xpath_version { DYNAMIC, XPATH_1, XPATH_2 };

struct opts {
	enum verbosity verbosity;

	/* Whether to check layered BREX DMs. */
	bool layered;

	/* Whether to check object values. */
	bool check_values;

	/* Whether to check the SNS of specified data modules against the SNS
	 * rules defined in the BREX data modules.
	 *
	 * In normal SNS check mode, optional levels that are omitted from the
	 * SNS rules only allow the value of '0' (or '00'/'0000' for the
	 * assyCode). Any other code is treated as invalid.
	 */
	bool check_sns;

	/* In strict SNS check mode, all levels of the SNS must be explicitly
	 * defined in the SNS rules, otherwise an error will be reported.
	 */
	bool strict_sns;

	/* In unstrict SNS check mode, if an optional level is omitted from the
	 * SNS rules, that is interpreted as allowing ANY code.
	 */
	bool unstrict_sns;

	/* Whether to check notation rules, that is, what NOTATIONs are allowed in
	 * the DTD.
	 */
	bool check_notations;

	/* Assume object filenames do not include issue info. */
	bool ignore_issue;

	/* Force a version of XPath to be used. */
	enum xpath_version xpath_version;
};

/* Return the first node in a set matching an XPath expression. */
static xmlNodePtr firstXPathNode(xmlDocPtr doc, xmlNodePtr context, const char *xpath)
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

/* Return the string value of the first node matching an XPath expression. */
static xmlChar *firstXPathValue(xmlNodePtr node, const char *expr)
{
	return xmlNodeGetContent(firstXPathNode(NULL, node, expr));
}

/* Check the values of objects against the patterns in the BREX rule. */
static bool check_node_values(xmlNodePtr node, xmlNodeSetPtr values)
{
	int i;
	bool ret = false;

	if (xmlXPathNodeSetIsEmpty(values))
		return true;

	for (i = 0; i < values->nodeNr; ++i) {
		xmlChar *allowed, *value, *form;

		allowed = firstXPathValue(values->nodeTab[i], "@valueAllowed|@val1");
		form    = firstXPathValue(values->nodeTab[i], "@valueForm|@valtype");
		value   = xmlNodeGetContent(node);

		if (form && xmlStrcmp(form, BAD_CAST "range") == 0) {
			ret = ret || is_in_set((char *) value, (char *) allowed);
		} else if (form && xmlStrcmp(form, BAD_CAST "pattern") == 0) {
			ret = ret || match_pattern(value, allowed);
		} else {
			ret = ret || xmlStrcmp(value, allowed) == 0;
		}

		xmlFree(allowed);
		xmlFree(form);
		xmlFree(value);
	}

	return ret;
}

/* Check an individual node's value against a rule. */
static bool check_single_object_values(xmlNodePtr rule, xmlNodePtr node)
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

/* Check the values of a set of nodes against a rule. */
static bool check_objects_values(xmlNodePtr rule, xmlNodeSetPtr nodes)
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

/* Determine whether a BREX context rule is violated. */
static bool is_invalid(xmlNodePtr rule, char *allowedObjectFlag, xmlXPathObjectPtr obj, struct opts *opts)
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

	if (!invalid && opts->check_values)
		invalid = !check_objects_values(rule, obj->nodesetval);

	return invalid;
}

/* Dump the XML branches that violate a given BREX context rule. */
static void dump_nodes_xml(xmlNodeSetPtr nodes, const char *fname, xmlNodePtr brexError, xmlNodePtr rule, struct opts *opts)
{
	int i;

	for (i = 0; i < nodes->nodeNr; ++i) {
		xmlNodePtr node = nodes->nodeTab[i];
		xmlNodePtr object;
		char line_s[16];
		xmlChar *xpath;

		if (opts->check_values && check_single_object_values(rule, node)) {
			continue;
		}

		snprintf(line_s, 16, "%ld", xmlGetLineNo(node));
		object = xmlNewChild(brexError, NULL, BAD_CAST "object", NULL);
		xmlSetProp(object, BAD_CAST "line", BAD_CAST line_s);

		xpath = xpath_of(node);
		xmlSetProp(object, BAD_CAST "xpath", xpath);
		xmlFree(xpath);

		if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;

		xmlAddChild(object, xmlCopyNode(node, deep_copy_nodes ? 1 : 2));
	}
}

/* Determine whether a file in the filesystem is an XML file by extension. */
static bool is_xml_file(const char *fname)
{
	return strcasecmp(fname + (strlen(fname) - 4), ".XML") == 0;
}

/* Search for the BREX in the built-in default BREX data modules. */
static bool search_brex_fname_from_default_brex(char *fname, char *dmcode, int len)
{
	if (strncasecmp(dmcode, "DMC-S1000D-H-04-10-0301-00A-022A-D_001-00_EN-US", len) == 0 || strncasecmp(dmcode, "DMC-S1000D-H-04-10-0301-00A-022A-D_\?\?\?-\?\?_EN-US", len) == 0) return strcpy(fname, "DMC-S1000D-H-04-10-0301-00A-022A-D");
	if (strncasecmp(dmcode, "DMC-S1000D-G-04-10-0301-00A-022A-D_001-00_EN-US", len) == 0 || strncasecmp(dmcode, "DMC-S1000D-G-04-10-0301-00A-022A-D_\?\?\?-\?\?_EN-US", len) == 0) return strcpy(fname, "DMC-S1000D-G-04-10-0301-00A-022A-D");
	if (strncasecmp(dmcode, "DMC-S1000D-F-04-10-0301-00A-022A-D_001-00_EN-US", len) == 0 || strncasecmp(dmcode, "DMC-S1000D-F-04-10-0301-00A-022A-D_\?\?\?-\?\?_EN-US", len) == 0) return strcpy(fname, "DMC-S1000D-F-04-10-0301-00A-022A-D");
	if (strncasecmp(dmcode, "DMC-S1000D-E-04-10-0301-00A-022A-D_012-00_EN-US", len) == 0 || strncasecmp(dmcode, "DMC-S1000D-E-04-10-0301-00A-022A-D_\?\?\?-\?\?_EN-US", len) == 0) return strcpy(fname, "DMC-S1000D-E-04-10-0301-00A-022A-D");
	if (strncasecmp(dmcode, "DMC-S1000D-D-04-10-0301-00A-022A-D_006-00_EN-US", len) == 0 || strncasecmp(dmcode, "DMC-S1000D-D-04-10-0301-00A-022A-D_\?\?\?-\?\?_EN-US", len) == 0) return strcpy(fname, "DMC-S1000D-D-04-10-0301-00A-022A-D");
	if (strncasecmp(dmcode, "DMC-S1000D-A-04-10-0301-00A-022A-D_005-00_EN-US", len) == 0 || strncasecmp(dmcode, "DMC-S1000D-A-04-10-0301-00A-022A-D_\?\?\?-\?\?_EN-US", len) == 0) return strcpy(fname, "DMC-S1000D-A-04-10-0301-00A-022A-D");
	if (strncasecmp(dmcode, "DMC-AE-A-04-10-0301-00A-022A-D_003-00_EN-US", len) == 0 || strncasecmp(dmcode, "DMC-AE-A-04-10-0301-00A-022A-D_\?\?\?-\?\?_EN-US", len) == 0) return strcpy(fname, "DMC-AE-A-04-10-0301-00A-022A-D");
	return false;
}

/* Find the filename of a BREX data module referenced by a CSDB object.
 * -1  Object does not reference a BREX DM.
 *  0  Object references a BREX DM, and it was found.
 *  1  Object references a BREX DM, but it couldn't be found.
 */
static int find_brex_fname_from_doc(char *fname, xmlDocPtr doc, char (*spaths)[PATH_MAX],
	int nspaths, char (*dmod_fnames)[PATH_MAX], int num_dmod_fnames,
	struct opts *opts)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr object;

	xmlNodePtr brexDmRef, dmCode, issueInfo, language;

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

	object = xmlXPathEval(BAD_CAST "//brexDmRef|//brexref", context);
	if (xmlXPathNodeSetIsEmpty(object->nodesetval)) {
		xmlXPathFreeObject(object);
		xmlXPathFreeContext(context);
		return -1;
	} else {
		brexDmRef = object->nodesetval->nodeTab[0];
	}
	xmlXPathFreeObject(object);

	xmlXPathSetContextNode(brexDmRef, context);

	object = xmlXPathEval(BAD_CAST ".//dmCode|.//avee", context);
	if (xmlXPathNodeSetIsEmpty(object->nodesetval)) {
		dmCode = NULL;
	} else {
		dmCode = object->nodesetval->nodeTab[0];
	}
	xmlXPathFreeObject(object);

	object = xmlXPathEval(BAD_CAST ".//issueInfo|.//issno", context);
	if (xmlXPathNodeSetIsEmpty(object->nodesetval)) {
		issueInfo = NULL;
	} else {
		issueInfo = object->nodesetval->nodeTab[0];
	}
	xmlXPathFreeObject(object);

	object = xmlXPathEval(BAD_CAST ".//language", context);
	if (xmlXPathNodeSetIsEmpty(object->nodesetval)) {
		language = NULL;
	} else {
		language = object->nodesetval->nodeTab[0];
	}
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

	if (!opts->ignore_issue) {
		if (issueInfo) {
			char *issue_number, *in_work;
			char iss[8];

			issue_number = (char *) firstXPathValue(issueInfo, "@issueNumber|@issno");
			in_work      = (char *) firstXPathValue(issueInfo, "@inWork|@inwork");

			snprintf(iss, 8, "_%s-%s", issue_number, in_work ? in_work : "00");
			strcat(dmcode, iss);

			xmlFree(issue_number);
			xmlFree(in_work);
		} else if (language) {
			strcat(dmcode, "_\?\?\?-\?\?");
		}
	}

	if (language) {
		char *language_iso_code, *country_iso_code;
		char lang[8];

		language_iso_code = (char *) firstXPathValue(language, "@languageIsoCode|@language");
		country_iso_code  = (char *) firstXPathValue(language, "@countryIsoCode|@country");

		snprintf(lang, 8, "_%s-%s", language_iso_code, country_iso_code);
		strcat(dmcode, lang);

		xmlFree(language_iso_code);
		xmlFree(country_iso_code);
	}

	len = strlen(dmcode);

	/* Look for the BREX in the current directory. */
	found = find_csdb_object(fname, search_dir, dmcode, is_xml_file, recursive_search);

	/* Look for the BREX in any of the specified search paths. */
	if (!found) {
		int i;

		for (i = 0; i < nspaths; ++i) {
			found = find_csdb_object(fname, spaths[i], dmcode, is_xml_file, recursive_search);
		}
	}

	/* Look for the BREX in the list of objects to check. */
	if (!found) {
		found = find_csdb_object_in_list(fname, dmod_fnames, num_dmod_fnames, dmcode);
	}

	/* Look for the BREX in the built-in default BREX. */
	if (!found) {
		found = search_brex_fname_from_default_brex(fname, dmcode, len);
	}

	if (opts->verbosity > SILENT && !found) {
		fprintf(stderr, E_BREX_NOT_FOUND, dmcode);
	}

	return !found;
}

/* Determine whether a violated rule counts as a failure, based on its
 * business rule severity level.
 */
static bool is_failure(xmlChar *severity)
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

/* Extract the type name of a business rule severity level. */
static xmlChar *brsl_type(xmlChar *severity)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlChar *type;

	ctx = xmlXPathNewContext(brsl);
	xmlXPathRegisterVariable(ctx, BAD_CAST "severity", xmlXPathNewString(severity));
	obj = xmlXPathEvalExpression(BAD_CAST "//brSeverityLevel[@value=$severity]", ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		type = NULL;
	} else {
		type = xmlNodeGetContent(obj->nodesetval->nodeTab[0]);
	}
	
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return type;
}

/* Copy the allowed object values to the XML report. */
static void add_object_values(xmlNodePtr brexError, xmlNodePtr rule)
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

/* Print the XML report as plain text messages */
static void print_node(xmlNodePtr node)
{
	xmlNodePtr cur;

	if (strcmp((char *) node->name, "error") == 0) {
		char *dpath = (char *) xmlGetProp(node->parent->parent, BAD_CAST "path");
		char *bpath = (char *) xmlGetProp(node->parent, BAD_CAST "path");
		if (xmlStrcmp(node->parent->name, BAD_CAST "sns") == 0) {
			if (shortmsg) {
				fprintf(stderr, "SNS ERROR: %s: ", dpath);
			} else {
				fprintf(stderr, "SNS ERROR: %s\n", dpath);
			}
		} else if (xmlStrcmp(node->parent->name, BAD_CAST "notations") == 0) {
			if (shortmsg) {
				fprintf(stderr, "NOTATION ERROR: %s: ", dpath);
			} else {
				fprintf(stderr, "NOTATION ERROR: %s\n", dpath);
			}
		} else {
			if (shortmsg) {
				fprintf(stderr, "BREX ERROR: %s: ", dpath);
			} else {
				fprintf(stderr, "BREX ERROR: %s\n", dpath);
				fprintf(stderr, "  BREX: %s\n", bpath);
			}
		}
		xmlFree(dpath);
		xmlFree(bpath);
	} else if (strcmp((char *) node->name, "type") == 0 && !shortmsg) {
		char *type = (char *) xmlNodeGetContent(node);
		fprintf(stderr, "  TYPE: %s\n", type);
		xmlFree(type);
	} else if (xmlStrcmp(node->name, BAD_CAST "brDecisionRef") == 0) {
		xmlChar *brdp = xmlGetProp(node, BAD_CAST "brDecisionIdentNumber");
		if (shortmsg) {
			fprintf(stderr, "%s: ", (char *) brdp);
		} else {
			fprintf(stderr, "  %s\n", (char *) brdp);
		}
		xmlFree(brdp);
	} else if (strcmp((char *) node->name, "objectUse") == 0) {
		char *use = (char *) xmlNodeGetContent(node);
		if (shortmsg) {
			fprintf(stderr, "%s", use);
		} else {
			fprintf(stderr, "  %s\n", use);
		}
		xmlFree(use);
	} else if (strcmp((char *) node->name, "objectValue") == 0 && !shortmsg) {
		char *allowed = (char *) xmlGetProp(node, BAD_CAST "valueAllowed");
		char *content = (char *) xmlNodeGetContent(node);
		fprintf(stderr, "  VALUE ALLOWED:");
		if (allowed)
			fprintf(stderr, " %s", allowed);
		if (content && strcmp(content, "") != 0)
			fprintf(stderr, " (%s)", content);
		fputc('\n', stderr);
		xmlFree(content);
		xmlFree(allowed);
	} else if (strcmp((char *) node->name, "objval") == 0 && !shortmsg) {
		char *allowed = (char *) xmlGetProp(node, BAD_CAST "val1");
		char *content = (char *) xmlNodeGetContent(node);
		fprintf(stderr, "  VALUE ALLOWED:");
		if (allowed)
			fprintf(stderr, " %s", allowed);
		if (content && strcmp(content, "") != 0)
			fprintf(stderr, " (%s)", content);
		fputc('\n', stderr);
		xmlFree(content);
		xmlFree(allowed);
	} else if (strcmp((char *) node->name, "object") == 0 && !shortmsg) {
		char *line = (char *) xmlGetProp(node, BAD_CAST "line");
		char *path = (char *) xmlGetProp(node, BAD_CAST "xpath");
		fprintf(stderr, "  line %s (%s):\n", line, path);
		xmlDebugDumpOneNode(stderr, node->children, 2);
		xmlFree(line);
		xmlFree(path);
	} else if (strcmp((char *) node->name, "code") == 0) {
		char *code = (char *) xmlNodeGetContent(node);
		if (!shortmsg) fprintf(stderr, "  ");
		fprintf(stderr, "Value of %s does not conform to SNS: ", code);
		xmlFree(code);
	} else if (strcmp((char *) node->name, "invalidValue") == 0) {
		char *value = (char *) xmlNodeGetContent(node);
		if (shortmsg) {
			fprintf(stderr, "%s", value);
		} else {
			fprintf(stderr, "%s\n", value);
		}
		xmlFree(value);
	} else if (strcmp((char *) node->name, "invalidNotation") == 0) {
		char *value = (char *) xmlNodeGetContent(node);
		if (!shortmsg) fprintf(stderr, "  ");
		fprintf(stderr, "Notation %s is not allowed", value);
		if (shortmsg)
			fprintf(stderr, ": ");
		else
			fprintf(stderr, ".\n");
		xmlFree(value);
	}

	for (cur = node->children; cur; cur = cur->next) {
		print_node(cur);
	}

	if (shortmsg && xmlStrcmp(node->name, BAD_CAST "error") == 0) {
		fputc('\n', stderr);
	}
}

/* Register extra XPath functions in a new XPath context. */
static void register_functions(xmlXPathContextPtr ctx)
{
	exsltDateXpathCtxtRegister(ctx, BAD_CAST "date");
	exsltMathXpathCtxtRegister(ctx, BAD_CAST "math");
	exsltSetsXpathCtxtRegister(ctx, BAD_CAST "set");
	exsltStrXpathCtxtRegister(ctx, BAD_CAST "str");
}

/* Register all namespaces applicable to a node in a new XPath context. */
#if XPATH2_ENGINE == SAXON
static void register_namespaces(xmlXPathContextPtr ctx, void *saxon_xpath, xmlNodePtr node)
#elif XPATH2_ENGINE == XQILLA
static void register_namespaces(xmlXPathContextPtr ctx, void *xqilla_ns_resolver, xmlNodePtr node)
#else
static void register_namespaces(xmlXPathContextPtr ctx, xmlNodePtr node)
#endif
{
	xmlNodePtr cur;

	for (cur = node; cur; cur = cur->parent) {
		if (cur->nsDef) {
			xmlNsPtr cur_ns;
			for (cur_ns = cur->nsDef; cur_ns; cur_ns = cur_ns->next) {
				xmlXPathRegisterNs(ctx, cur_ns->prefix, cur_ns->href);

#if XPATH2_ENGINE == SAXON
				if (saxon_xpath) {
					saxon_register_namespace(saxon_xpath, cur_ns->prefix, cur_ns->href);
				}
#elif XPATH2_ENGINE == XQILLA
				if (xqilla_ns_resolver) {
					xqilla_register_namespace(xqilla_ns_resolver, cur_ns->prefix, cur_ns->href);
				}
#endif

			}
		}
	}
}

/* Check the context rules of a BREX DM against a CSDB object. */
#if XPATH2_ENGINE == SAXON
static int check_brex_rules(xmlDocPtr brex_doc, xmlNodeSetPtr rules, xmlDocPtr doc, const char *fname, const char *brexfname, xmlNodePtr documentNode, struct opts *opts, void *saxon_processor, void *saxon_node)
#elif XPATH2_ENGINE == XQILLA
static int check_brex_rules(xmlDocPtr brex_doc, xmlNodeSetPtr rules, xmlDocPtr doc, const char *fname, const char *brexfname, xmlNodePtr documentNode, struct opts *opts, void *xqilla_doc)
#else
static int check_brex_rules(xmlDocPtr brex_doc, xmlNodeSetPtr rules, xmlDocPtr doc, const char *fname, const char *brexfname, xmlNodePtr documentNode, struct opts *opts)
#endif
{
	xmlChar *defaultBrSeverityLevel;
	int nerr = 0;
	xmlNodePtr brexNode;

	defaultBrSeverityLevel = xmlGetProp(firstXPathNode(brex_doc, NULL, "//brex"), BAD_CAST "defaultBrSeverityLevel");

	brexNode = xmlNewChild(documentNode, NULL, BAD_CAST "brex", NULL);
	xmlSetProp(brexNode, BAD_CAST "path", BAD_CAST brexfname);

	if (!xmlXPathNodeSetIsEmpty(rules)) {
		int i;

		for (i = 0; i < rules->nodeNr; ++i) {
			xmlXPathContextPtr context;
			xmlXPathObjectPtr object;
			xmlNodePtr brDecisionRef, objectPath, objectUse;
			xmlChar *allowedObjectFlag, *path, *use, *brdp;

#if XPATH2_ENGINE == SAXON
			void *xpath_processor;

			if (saxon_processor) {
				xpath_processor = saxon_new_xpath_processor(saxon_processor);
			} else {
				xpath_processor = NULL;
			}
#elif XPATH2_ENGINE == XQILLA
			void *xqilla_ns_resolver;

			if (xqilla_doc) {
				xqilla_ns_resolver = xqilla_create_ns_resolver(xqilla_doc);
			} else {
				xqilla_ns_resolver = NULL;
			}
#endif

			brDecisionRef = firstXPathNode(brex_doc, rules->nodeTab[i], "brDecisionRef");
			objectPath = firstXPathNode(brex_doc, rules->nodeTab[i], "objectPath|objpath");
			objectUse  = firstXPathNode(brex_doc, rules->nodeTab[i], "objectUse|objuse");

			brdp = xmlGetProp(brDecisionRef, BAD_CAST "brDecisionIdentNumber");
			allowedObjectFlag = firstXPathValue(objectPath, "@allowedObjectFlag|@objappl");
			path = xmlNodeGetContent(objectPath);
			use  = xmlNodeGetContent(objectUse);

			context = xmlXPathNewContext(doc);
			register_functions(context);

#if XPATH2_ENGINE == SAXON
			register_namespaces(context, xpath_processor, objectPath);

			if (xpath_processor) {
				object = saxon_eval_xpath(saxon_processor, xpath_processor, saxon_node, path, context);
			} else {
				object = xmlXPathEval(path, context);
			}
#elif XPATH2_ENGINE == XQILLA
			register_namespaces(context, xqilla_ns_resolver, objectPath);

			if (xqilla_ns_resolver) {
				object = xqilla_eval_xpath(xqilla_doc, xqilla_ns_resolver, path, context);
			} else {
				object = xmlXPathEval(path, context);
			}
#else
			register_namespaces(context, objectPath);

			object = xmlXPathEvalExpression(path, context);
#endif

			if (object != NULL) {
				if (is_invalid(rules->nodeTab[i], (char *) allowedObjectFlag, object, opts)) {
					xmlChar *severity;
					xmlNodePtr brexError, err_path;

					if (!(severity = xmlGetProp(rules->nodeTab[i], BAD_CAST "brSeverityLevel"))) {
						severity = xmlStrdup(defaultBrSeverityLevel);
					}

					brexError = xmlNewChild(brexNode, NULL, BAD_CAST "error", NULL);

					if (severity) {
						xmlSetProp(brexError, BAD_CAST "brSeverityLevel", severity);

						if (brsl_fname) {
							xmlChar *type = brsl_type(severity);
							xmlNewChild(brexError, NULL, BAD_CAST "type", type);
							xmlFree(type);
						}
					} else {
						xmlSetProp(brexError, BAD_CAST "fail", BAD_CAST "yes");
					}

					if (brDecisionRef) {
						xmlAddChild(brexError, xmlCopyNode(brDecisionRef, 1));
					}

					err_path = xmlNewChild(brexError, NULL, BAD_CAST "objectPath", path);
					xmlSetProp(err_path, BAD_CAST "allowedObjectFlag", allowedObjectFlag);
					xmlNewChild(brexError, NULL, BAD_CAST "objectUse", use);

					add_object_values(brexError, rules->nodeTab[i]);

					if (!xmlXPathNodeSetIsEmpty(object->nodesetval)) {
						dump_nodes_xml(object->nodesetval, fname,
							brexError, rules->nodeTab[i],
							opts);
					}

					if (severity) {
						if (is_failure(severity)) {
							++nerr;
						} else {
							xmlSetProp(brexError, BAD_CAST "fail", BAD_CAST "no");
						}
					} else {
						++nerr;
					}

					xmlFree(severity);

					if (opts->verbosity > SILENT) {
						print_node(brexError);
					}
				}
			} else {
				long int line;
				xmlChar line_s[16];
				xmlChar *xpath;
				xmlNodePtr xpath_err;

				line = xmlGetLineNo(objectPath);
				xmlStrPrintf(line_s, 16, "%ld", line);

				xpath = xpath_of(objectPath);

				xpath_err = xmlNewChild(brexNode, brexNode->ns, BAD_CAST "xpathError", path);
				xmlSetProp(xpath_err, BAD_CAST "line", line_s);
				xmlSetProp(xpath_err, BAD_CAST "xpath", xpath);

				xmlFree(xpath);

				if (opts->verbosity > SILENT) {
					fprintf(stderr, W_INVOBJPATH, brexfname, line, path);
				}
			}

#if XPATH2_ENGINE == SAXON
			saxon_free_xpath_processor(xpath_processor);
#endif
			/* FIXME: If the XPath expression was invalid, xmlXPathFreeObject doesn't
			 *        seem to free everything, so there will be a memory leak. */
			xmlXPathFreeObject(object);
			xmlXPathFreeContext(context);
			xmlFree(brdp);
			xmlFree(allowedObjectFlag);
			xmlFree(path);
			xmlFree(use);
		}
	}

	if (!brexNode->children) {
		xmlNewChild(brexNode, NULL, BAD_CAST "noErrors", NULL);
	}

	xmlFree(defaultBrSeverityLevel);

	return nerr;
}

/* Load a BREX DM from the filesystem or from in-memory. */
static xmlDocPtr load_brex(const char *name, xmlDocPtr dmod_doc)
{
	/* If the BREX name is -, this means the DM on stdin is a BREX DM.
	 * BREX DMs are checked against themselves, so return a copy of the
	 * same document.
	 */
	if (strcmp(name, "-") == 0) {
		return xmlCopyDoc(dmod_doc, 1);
	/* If the BREX name is an existing filename, read from that. */
	} else if (access(name, F_OK) != -1) {
		return read_xml_doc(name);
	/* If the BREX name is one of the standard Default BREX codes, read
	 * it from memory.
	 */
	} else {
		unsigned char *xml = NULL;
		unsigned int len = 0;

		if (strcmp(name, "DMC-S1000D-H-04-10-0301-00A-022A-D") == 0) {
			xml = brex_DMC_S1000D_H_04_10_0301_00A_022A_D_001_00_EN_US_XML;
			len = brex_DMC_S1000D_H_04_10_0301_00A_022A_D_001_00_EN_US_XML_len;
		} else if (strcmp(name, "DMC-S1000D-G-04-10-0301-00A-022A-D") == 0) {
			xml = brex_DMC_S1000D_G_04_10_0301_00A_022A_D_001_00_EN_US_XML;
			len = brex_DMC_S1000D_G_04_10_0301_00A_022A_D_001_00_EN_US_XML_len;
		} else if (strcmp(name, "DMC-S1000D-F-04-10-0301-00A-022A-D") == 0) {
			xml = brex_DMC_S1000D_F_04_10_0301_00A_022A_D_001_00_EN_US_XML;
			len = brex_DMC_S1000D_F_04_10_0301_00A_022A_D_001_00_EN_US_XML_len;
		} else if (strcmp(name, "DMC-S1000D-E-04-10-0301-00A-022A-D") == 0) {
			xml = brex_DMC_S1000D_E_04_10_0301_00A_022A_D_012_00_EN_US_XML;
			len = brex_DMC_S1000D_E_04_10_0301_00A_022A_D_012_00_EN_US_XML_len;
		} else if (strcmp(name, "DMC-S1000D-D-04-10-0301-00A-022A-D") == 0) {
			xml = brex_DMC_S1000D_D_04_10_0301_00A_022A_D_006_00_EN_US_XML;
			len = brex_DMC_S1000D_D_04_10_0301_00A_022A_D_006_00_EN_US_XML_len;
		} else if (strcmp(name, "DMC-S1000D-A-04-10-0301-00A-022A-D") == 0) {
			xml = brex_DMC_S1000D_A_04_10_0301_00A_022A_D_005_00_EN_US_XML;
			len = brex_DMC_S1000D_A_04_10_0301_00A_022A_D_005_00_EN_US_XML_len;
		} else if (strcmp(name, "DMC-AE-A-04-10-0301-00A-022A-D") == 0) {
			xml = brex_DMC_AE_A_04_10_0301_00A_022A_D_003_00_XML;
			len = brex_DMC_AE_A_04_10_0301_00A_022A_D_003_00_XML_len;
		}

		return read_xml_mem((const char *) xml, len);
	}
}

/* Determine which parts of the SNS rules to check. */
static bool should_check(xmlChar *code, char *path, xmlDocPtr snsRulesDoc, xmlNodePtr ctx, struct opts *opts)
{
	bool ret;

	if (opts->strict_sns) return true;

	if (opts->unstrict_sns)
		return firstXPathNode(snsRulesDoc, ctx, path);

	if (strcmp(path, ".//snsSubSystem") == 0 || strcmp(path, ".//snsSubSubSystem") == 0) {
		ret = xmlStrcmp(code, BAD_CAST "0") != 0;
	} else {
		ret = !(xmlStrcmp(code, BAD_CAST "00") == 0 || xmlStrcmp(code, BAD_CAST "0000") == 0);
	}

	return ret || firstXPathNode(snsRulesDoc, ctx, path);
}

/* Check SNS rules against a CSDB object. */
static bool check_brex_sns_rules(xmlDocPtr snsRulesDoc, xmlNodePtr snsRulesGroup, xmlDocPtr dmod_doc, xmlNodePtr documentNode, struct opts *opts)
{
	xmlNodePtr dmcode, snsCheck, snsError;
	xmlChar *systemCode, *subSystemCode, *subSubSystemCode, *assyCode;
	char value[256];
	char xpath[256];
	xmlNodePtr ctx = NULL;
	bool correct = true;

	/* Only check SNS in data modules. */
	if (xmlStrcmp(xmlDocGetRootElement(dmod_doc)->name, BAD_CAST "dmodule") != 0)
		return correct;

	dmcode = firstXPathNode(dmod_doc, NULL, "//dmIdent/dmCode");

	systemCode       = xmlGetProp(dmcode, BAD_CAST "systemCode");
	subSystemCode    = xmlGetProp(dmcode, BAD_CAST "subSystemCode");
	subSubSystemCode = xmlGetProp(dmcode, BAD_CAST "subSubSystemCode");
	assyCode         = xmlGetProp(dmcode, BAD_CAST "assyCode");

	snsCheck = xmlNewChild(documentNode, NULL, BAD_CAST "sns", NULL);
	snsError = xmlNewNode(NULL, BAD_CAST "error");

	/* Check the SNS of the data module against the SNS rules in descending order. */

	/* System code. */
	if (should_check(systemCode, "//snsSystem", snsRulesDoc, ctx, opts)) {
		sprintf(xpath, "//snsSystem[snsCode = '%s']", (char *) systemCode);
		if (!(ctx = firstXPathNode(snsRulesDoc, ctx, xpath))) {
			xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "systemCode");
			xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", systemCode);
			xmlAddChild(snsCheck, snsError);
			correct = false;
		}
	}

	/* Subsystem code. */
	if (correct && should_check(subSystemCode, ".//snsSubSystem", snsRulesDoc, ctx, opts)) {
		sprintf(xpath, ".//snsSubSystem[snsCode = '%s']", (char *) subSystemCode);
		if (!(ctx = firstXPathNode(snsRulesDoc, ctx, xpath))) {
			xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "subSystemCode");
			sprintf(value, "%s-%s", systemCode, subSystemCode);
			xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", BAD_CAST value);
			xmlAddChild(snsCheck, snsError);
			correct = false;
		}
	}

	/* Subsubsystem code. */
	if (correct && should_check(subSubSystemCode, ".//snsSubSubSystem", snsRulesDoc, ctx, opts)) {
		sprintf(xpath, ".//snsSubSubSystem[snsCode = '%s']", (char *) subSubSystemCode);
		if (!(ctx = firstXPathNode(snsRulesDoc, ctx, xpath))) {
			xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "subSubSystemCode");
			sprintf(value, "%s-%s%s", systemCode, subSystemCode, subSubSystemCode);
			xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", BAD_CAST value);
			xmlAddChild(snsCheck, snsError);
			correct = false;
		}
	}

	/* Assembly code. */
	if (correct && should_check(assyCode, ".//snsAssy", snsRulesDoc, ctx, opts)) {
		sprintf(xpath, ".//snsAssy[snsCode = '%s']", (char *) assyCode);
		if (!firstXPathNode(snsRulesDoc, ctx, xpath)) {
			xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "assyCode");
			sprintf(value, "%s-%s%s-%s", systemCode, subSystemCode, subSubSystemCode, assyCode);
			xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", BAD_CAST value);
			xmlAddChild(snsCheck, snsError);
			correct = false;
		}
	}

	if (correct) {
		xmlFreeNode(snsError);
		xmlNewChild(snsCheck, NULL, BAD_CAST "noErrors", NULL);
	} else if (opts->verbosity > SILENT) {
		print_node(snsError);
	}

	xmlFree(systemCode);
	xmlFree(subSystemCode);
	xmlFree(subSubSystemCode);
	xmlFree(assyCode);

	return correct;
}

/* Check the SNS rules of BREX DMs against a CSDB object. */
static bool check_brex_sns(char (*brex_fnames)[PATH_MAX], int nbrex_fnames,
	xmlDocPtr dmod_doc,xmlNodePtr documentNode, struct opts *opts)
{
	int i;
	xmlDocPtr snsRulesDoc;
	xmlNodePtr snsRulesGroup;
	bool correct;

	/* The valid SNS is taken as a combination of the snsRules from all specified BREX data modules. */
	snsRulesDoc = xmlNewDoc(BAD_CAST "1.0");
	xmlDocSetRootElement(snsRulesDoc, xmlNewNode(NULL, BAD_CAST "snsRulesGroup"));
	snsRulesGroup = xmlDocGetRootElement(snsRulesDoc);

	for (i = 0; i < nbrex_fnames; ++i) {
		xmlDocPtr brex;

		brex = load_brex(brex_fnames[i], dmod_doc);

		xmlAddChild(snsRulesGroup, xmlCopyNode(firstXPathNode(brex, NULL, "//snsRules"), 1));

		xmlFreeDoc(brex);
	}

	correct = check_brex_sns_rules(snsRulesDoc, snsRulesGroup, dmod_doc, documentNode, opts);

	xmlFreeDoc(snsRulesDoc);

	return correct;
}

/* Check the notation used by an entity against the notation rules. */
static int check_entity(xmlEntityPtr entity, xmlDocPtr notationRuleDoc,
	xmlNodePtr notationCheck, struct opts *opts)
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

	notationError = xmlNewChild(notationCheck, NULL, BAD_CAST "error", NULL);
	xmlNewChild(notationError, NULL, BAD_CAST "invalidNotation", entity->content);
	xmlAddChild(notationError, xmlCopyNode(firstXPathNode(notationRuleDoc, rule, "objectUse"), 1));

	if (opts->verbosity > SILENT) {
		print_node(notationError);
	}

	return 1;
}

/* Check notation rules against a CSDB object. */
static int check_brex_notation_rules(xmlDocPtr notationRuleDoc, xmlNodePtr notationRuleGroup, xmlDocPtr dmod_doc, xmlNodePtr documentNode, struct opts *opts)
{
	xmlDtdPtr dtd;
	xmlNodePtr notationCheck, cur;
	int invalid = 0;

	if (!(dtd = dmod_doc->intSubset))
		return 0;

	notationCheck = xmlNewChild(documentNode, NULL, BAD_CAST "notations", NULL);

	for (cur = dtd->children; cur; cur = cur->next) {
		if (cur->type == XML_ENTITY_DECL && ((xmlEntityPtr) cur)->etype == 3) {
			invalid += check_entity((xmlEntityPtr) cur, notationRuleDoc,
				notationCheck, opts);
		}
	}

	if (!notationCheck->children) {
		xmlNewChild(notationCheck, NULL, BAD_CAST "noErrors", NULL);
	}

	return invalid;
}

/* Check the notation rules of BREX DMs against a CSDB object. */
static int check_brex_notations(char (*brex_fnames)[PATH_MAX], int nbrex_fnames,
	xmlDocPtr dmod_doc, xmlNodePtr documentNode, struct opts *opts)
{
	xmlDocPtr notationRuleDoc;
	xmlNodePtr notationRuleGroup;
	int i, invalid;

	notationRuleDoc = xmlNewDoc(BAD_CAST "1.0");
	xmlDocSetRootElement(notationRuleDoc, xmlNewNode(NULL, BAD_CAST "notationRuleGroup"));
	notationRuleGroup = xmlDocGetRootElement(notationRuleDoc);

	for (i = 0; i < nbrex_fnames; ++i) {
		xmlDocPtr brex;

		brex = load_brex(brex_fnames[i], dmod_doc);

		xmlAddChild(notationRuleGroup, xmlCopyNode(firstXPathNode(brex, NULL, "//notationRuleList"), 1));

		xmlFreeDoc(brex);
	}

	invalid = check_brex_notation_rules(notationRuleDoc, notationRuleGroup, dmod_doc, documentNode, opts);

	xmlFreeDoc(notationRuleDoc);

	return invalid;
}

/* Print the filenames of CSDB objects with BREX errors. */
static void print_fnames(xmlNodePtr node)
{
	if (xmlStrcmp(node->name, BAD_CAST "document") == 0 && firstXPathNode(NULL, node, "brex/error")) {
		xmlChar *fname;
		fname = xmlGetProp(node, BAD_CAST "path");
		puts((char *) fname);
		xmlFree(fname);
	} else {
		xmlNodePtr cur;

		for (cur = node->children; cur; cur = cur->next) {
			print_fnames(cur);
		}
	}
}

/* Print the filenames of CSDB objects with no BREX errors. */
static void print_valid_fnames(xmlNodePtr node)
{
	if (xmlStrcmp(node->name, BAD_CAST "document") == 0 && !firstXPathNode(NULL, node, "brex/error")) {
		xmlChar *fname;
		fname = xmlGetProp(node, BAD_CAST "path");
		puts((char *) fname);
		xmlFree(fname);
	} else {
		xmlNodePtr cur;

		for (cur = node->children; cur; cur = cur->next) {
			print_fnames(cur);
		}
	}
}

#if XPATH2_ENGINE != NONE
/* Determine whether the S1000D issue of a BREX DM means it may require
 * XPath 2.0 to evaluate its rules.
 */
static bool brex_requires_xpath2(xmlDocPtr brex, struct opts *opts)
{
	xmlChar *issue;
	bool requires_xpath2;

	switch (opts->xpath_version) {
		case XPATH_1: return false;
		case XPATH_2: return true;
		case DYNAMIC: break;
	}

	issue = xmlGetNsProp(xmlDocGetRootElement(brex),
		BAD_CAST "noNamespaceSchemaLocation",
		BAD_CAST "http://www.w3.org/2001/XMLSchema-instance");

	/* These issues of S1000D only required XPath 1.0. */
	     if (xmlStrncmp(issue, BAD_CAST "http://www.s1000d.org/S1000D_2-0", 32) == 0) requires_xpath2 = false;
	else if (xmlStrncmp(issue, BAD_CAST "http://www.s1000d.org/S1000D_2-1", 32) == 0) requires_xpath2 = false;
	else if (xmlStrncmp(issue, BAD_CAST "http://www.s1000d.org/S1000D_2-2", 32) == 0) requires_xpath2 = false;
	else if (xmlStrncmp(issue, BAD_CAST "http://www.s1000d.org/S1000D_2-3", 32) == 0) requires_xpath2 = false;
	else if (xmlStrncmp(issue, BAD_CAST "http://www.s1000d.org/S1000D_3-0", 32) == 0) requires_xpath2 = false;
	else requires_xpath2 = true;

	xmlFree(issue);

	return requires_xpath2;
}
#endif

/* Check context, SNS, and notation rules of BREX DMs against a CSDB object. */
#if XPATH2_ENGINE == SAXON
static int check_brex(xmlDocPtr dmod_doc, const char *docname, char (*brex_fnames)[PATH_MAX], int num_brex_fnames, xmlNodePtr brexCheck, struct opts *opts, void *saxon_processor)
#elif XPATH2_ENGINE == XQILLA
static int check_brex(xmlDocPtr dmod_doc, const char *docname, char (*brex_fnames)[PATH_MAX], int num_brex_fnames, xmlNodePtr brexCheck, struct opts *opts, void *xqilla_impl)
#else
static int check_brex(xmlDocPtr dmod_doc, const char *docname, char (*brex_fnames)[PATH_MAX], int num_brex_fnames, xmlNodePtr brexCheck, struct opts *opts)
#endif
{
	xmlDocPtr brex_doc;
	xmlNodePtr documentNode;

	int i;
	int total = 0;
	bool valid_sns = true;
	int invalid_notations = 0;

	xmlChar *schema;

	xmlDocPtr validtree = NULL;

#if XPATH2_ENGINE == SAXON
	void *saxon_node;
#elif XPATH2_ENGINE == XQILLA
	void *xqilla_parser;
	void *xqilla_doc;
#endif

	/* Make a copy of the original XML tree before performing extra
	 * processing on it. */
	if (output_tree) {
		validtree = xmlCopyDoc(dmod_doc, 1);
	}

	/* Remove "delete" elements. */
	if (rem_delete) {
		rem_delete_elems(dmod_doc);
	}

#if XPATH2_ENGINE == SAXON
	saxon_node = saxon_new_node(saxon_processor, dmod_doc);
#elif XPATH2_ENGINE == XQILLA
	xqilla_parser = xqilla_create_parser(xqilla_impl);
	xqilla_doc = xqilla_create_doc(xqilla_impl, xqilla_parser, dmod_doc);
#endif

	schema = xmlGetNsProp(xmlDocGetRootElement(dmod_doc), BAD_CAST "noNamespaceSchemaLocation", XSI_URI);

	documentNode = xmlNewChild(brexCheck, NULL, BAD_CAST "document", NULL);
	xmlSetProp(documentNode, BAD_CAST "path", BAD_CAST docname);

	if (opts->check_sns &&
	    !(valid_sns = check_brex_sns(brex_fnames, num_brex_fnames, dmod_doc,
			                 documentNode, opts)))
	{
		++total;
	}

	if (opts->check_notations) {
		invalid_notations = check_brex_notations(brex_fnames, num_brex_fnames, dmod_doc, documentNode, opts);
		total += invalid_notations;
	}

	for (i = 0; i < num_brex_fnames; ++i) {
		xmlXPathContextPtr context;
		xmlXPathObjectPtr result;
		int status;
#if XPATH2_ENGINE != NONE
		bool use_xpath2;
#endif

		brex_doc = load_brex(brex_fnames[i], dmod_doc);

		if (!brex_doc) {
			if (opts->verbosity > SILENT) {
				fprintf(stderr, E_NODMOD, brex_fnames[i]);
			}
			exit(EXIT_BAD_DMODULE);
		}

		context = xmlXPathNewContext(brex_doc);
		xmlXPathRegisterVariable(context, BAD_CAST "schema", xmlXPathNewString(schema));

		result = xmlXPathEvalExpression(STRUCT_OBJ_RULE_PATH, context);

#if XPATH2_ENGINE != NONE
		/* Determine if the BREX rules should be evaluated with XPath 2.0. */
		use_xpath2 = brex_requires_xpath2(brex_doc, opts);
#endif

#if XPATH2_ENGINE == SAXON
		if (use_xpath2) {
			status = check_brex_rules(brex_doc, result->nodesetval, dmod_doc, docname, brex_fnames[i], documentNode, opts, saxon_processor, saxon_node);
		} else {
			status = check_brex_rules(brex_doc, result->nodesetval, dmod_doc, docname, brex_fnames[i], documentNode, opts, NULL, NULL);
		}
#elif XPATH2_ENGINE == XQILLA
		if (use_xpath2) {
			status = check_brex_rules(brex_doc, result->nodesetval, dmod_doc, docname, brex_fnames[i], documentNode, opts, xqilla_doc);
		} else {
			status = check_brex_rules(brex_doc, result->nodesetval, dmod_doc, docname, brex_fnames[i], documentNode, opts, NULL);
		}
#else
		status = check_brex_rules(brex_doc, result->nodesetval, dmod_doc, docname, brex_fnames[i], documentNode, opts);
#endif

		if (opts->verbosity >= VERBOSE) {
			fprintf(stderr,
				status || !valid_sns || invalid_notations ?
				F_INVALIDDOC :
				S_VALIDDOC, docname, brex_fnames[i]);
		}

		total += status;

		xmlXPathFreeObject(result);
		xmlXPathFreeContext(context);
		xmlFreeDoc(brex_doc);
	}

	xmlFree(schema);

	switch (show_fnames) {
		case SHOW_NONE: break;
		case SHOW_INVALID: print_fnames(documentNode); break;
		case SHOW_VALID: print_valid_fnames(documentNode); break;
	}

	if (output_tree) {
		if (total == 0) {
			save_xml_doc(validtree, "-");
		}
		xmlFreeDoc(validtree);
	}

#if XPATH2_ENGINE == SAXON
	saxon_free_node(saxon_node);
#elif XPATH2_ENGINE == XQILLA
	xqilla_free_parser(xqilla_parser);
#endif

	return total;
}

/* Determine if a BREX exists in the given search paths. */
static bool brex_exists(char fname[PATH_MAX], char (*fnames)[PATH_MAX], int nfnames, char (*spaths)[PATH_MAX], int nspaths)
{
	int i;

	for (i = 0; i < nfnames; ++i) {
		if (strcmp(fname, fnames[i]) == 0) {
			return true;
		}
	}

	return false;
}

/* Add a path to a list of paths, extending its size if necessary. */
static void add_path(char (**list)[PATH_MAX], int *n, unsigned *max, const char *s, struct opts *opts)
{
	if ((*n) == (*max)) {
		if (!(*list = realloc(*list, (*max *= 2) * PATH_MAX))) {
			if (opts->verbosity > SILENT) {
				fprintf(stderr, E_MAXOBJS);
			}
			exit(EXIT_MAX_OBJS);
		}
	}

	strcpy((*list)[(*n)++], s);
}

/* Add the BREX referenced by another BREX DM in layered mode (-l).*/
static int add_layered_brex(char (**fnames)[PATH_MAX], int nfnames, char (*spaths)[PATH_MAX], int nspaths, char (*dmod_fnames)[PATH_MAX], int num_dmod_fnames, xmlDocPtr dmod_doc, struct opts *opts)
{
	int i;
	int total = nfnames;

	for (i = 0; i < nfnames; ++i) {
		xmlDocPtr doc;
		char fname[PATH_MAX];
		int err;

		doc = load_brex((*fnames)[i], dmod_doc);

		err = find_brex_fname_from_doc(fname, doc, spaths, nspaths, dmod_fnames, num_dmod_fnames, opts);

		if (err) {
			fprintf(stderr, E_NOBREX_LAYER, (*fnames)[i]);
			exit(EXIT_BREX_NOT_FOUND);
		} else if (!brex_exists(fname, (*fnames), nfnames, spaths, nspaths)) {
			add_path(fnames, &total, &BREX_MAX, fname, opts);
			total = add_layered_brex(fnames, total, spaths, nspaths, dmod_fnames, num_dmod_fnames, dmod_doc, opts);
		}

		xmlFreeDoc(doc);
	}

	return total;
}

/* Add CSDB objects to check from a list of filenames. */
static void add_dmod_list(const char *fname, char (**dmod_fnames)[PATH_MAX], int *num_dmod_fnames, struct opts *opts)
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
		add_path(dmod_fnames, num_dmod_fnames, &DMOD_MAX, path, opts);
	}

	if (fname) {
		fclose(f);
	}
}

/* Return the default BREX DMC for a given issue of the spec. */
static const char *default_brex_dmc(xmlDocPtr doc)
{
	xmlChar *schema;
	const char *code;

	schema = xmlGetNsProp(xmlDocGetRootElement(doc), BAD_CAST "noNamespaceSchemaLocation", XSI_URI);

	if (schema == NULL || xmlStrstr(schema, BAD_CAST "S1000D_6")) {
		code = "DMC-S1000D-H-04-10-0301-00A-022A-D";
	} else if (xmlStrstr(schema, BAD_CAST "S1000D_5-0")) {
		code = "DMC-S1000D-G-04-10-0301-00A-022A-D";
	} else if (xmlStrstr(schema, BAD_CAST "S1000D_4-2")) {
		code = "DMC-S1000D-F-04-10-0301-00A-022A-D";
	} else if (xmlStrstr(schema, BAD_CAST "S1000D_4-1")) {
		code = "DMC-S1000D-E-04-10-0301-00A-022A-D";
	} else if (xmlStrstr(schema, BAD_CAST "S1000D_4-0")) {
		code = "DMC-S1000D-D-04-10-0301-00A-022A-D";
	} else {
		code = "DMC-AE-A-04-10-0301-00A-022A-D";
	}

	xmlFree(schema);

	return code;
}

static void print_stats(xmlDocPtr doc)
{
	xmlDocPtr styledoc;
	xsltStylesheetPtr style;
	xmlDocPtr res;

	styledoc = read_xml_mem((const char *) stats_xsl, stats_xsl_len);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);

	fprintf(stderr, "%s", (char *) res->children->content);

	xmlFreeDoc(res);
	xsltFreeStylesheet(style);
}

/* Add configuration information to report. */
static void add_config_to_report(xmlNodePtr brexCheck, struct opts *opts)
{
	if (opts->layered) {
		xmlSetProp(brexCheck, BAD_CAST "layered", BAD_CAST "yes");
	} else {
		xmlSetProp(brexCheck, BAD_CAST "layered", BAD_CAST "no");
	}

	if (opts->check_values) {
		xmlSetProp(brexCheck, BAD_CAST "checkObjectValues", BAD_CAST "yes");
	} else {
		xmlSetProp(brexCheck, BAD_CAST "checkObjectValues", BAD_CAST "no");
	}

	if (opts->check_sns) {
		if (opts->strict_sns) {
			xmlSetProp(brexCheck, BAD_CAST "snsCheck", BAD_CAST "strict");
		} else if (opts->unstrict_sns) {
			xmlSetProp(brexCheck, BAD_CAST "snsCheck", BAD_CAST "unstrict");
		} else {
			xmlSetProp(brexCheck, BAD_CAST "snsCheck", BAD_CAST "normal");
		}
	} else {
		xmlSetProp(brexCheck, BAD_CAST "snsCheck", BAD_CAST "no");
	}

	if (opts->check_notations) {
		xmlSetProp(brexCheck, BAD_CAST "notationCheck", BAD_CAST "yes");
	} else {
		xmlSetProp(brexCheck, BAD_CAST "notationCheck", BAD_CAST "no");
	}
}

/* Determine XPath version of user-supplied string. */
static void set_xpath_version(struct opts *opts, const char *str)
{
	if (strcmp(str, "1.0") == 0) {
		opts->xpath_version = XPATH_1;
#if XPATH2_ENGINE != NONE
	} else if (strcmp(str, "2.0") == 0) {
		opts->xpath_version = XPATH_2;
#endif
	} else {
		if (opts->verbosity > SILENT) {
			fprintf(stderr, E_BAD_XPATH_VERSION, str);
		}

		exit(EXIT_BAD_XPATH_VERSION);
	}
}

#ifdef LIBS1KD
typedef enum {
	S1KD_BREXCHECK_VALUES = 1,
	S1KD_BREXCHECK_SNS = 2,
	S1KD_BREXCHECK_STRICT_SNS = 4,
	S1KD_BREXCHECK_UNSTRICT_SNS = 8,
	S1KD_BREXCHECK_NOTATIONS = 16,
	S1KD_BREXCHECK_NORMAL_LOG = 32,
	S1KD_BREXCHECK_VERBOSE_LOG = 64
} s1kdBREXCheckOption;

static void init_opts(struct opts *opts, int options)
{
	if (optset(options, S1KD_BREXCHECK_NORMAL_LOG)) {
		opts->verbosity = NORMAL;
	} else if (optset(options, S1KD_BREXCHECK_VERBOSE_LOG)) {
		opts->verbosity = VERBOSE;
	} else {
		opts->verbosity = SILENT;
	}

	opts->layered         = false;
	opts->check_values    = optset(options, S1KD_BREXCHECK_VALUES);
	opts->check_sns       = optset(options, S1KD_BREXCHECK_SNS);
	opts->strict_sns      = optset(options, S1KD_BREXCHECK_STRICT_SNS);
	opts->unstrict_sns    = optset(options, S1KD_BREXCHECK_UNSTRICT_SNS);
	opts->check_notations = optset(options, S1KD_BREXCHECK_NOTATIONS);
	opts->ignore_issue    = false;
	opts->xpath_version   = DYNAMIC;
}

static void free_opts(struct opts *opts)
{
}

int s1kdDocCheckDefaultBREX(xmlDocPtr doc, int options, xmlDocPtr *report)
{
	int err;
	xmlDocPtr brex;
	xmlDocPtr rep;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;
	const char *brex_dmc;
	struct opts opts;
#if XPATH2_ENGINE == SAXON
	void *saxon_processor, *saxon_node;
#elif XPATH2_ENGINE == XQILLA
	void *xqilla_impl, *xqilla_parser, *xqilla_doc;
#endif
#if XPATH2_ENGINE != NONE
	bool use_xpath2;
#endif

	init_opts(&opts, options);

#if XPATH2_ENGINE != NONE
	use_xpath2 = brex_requires_xpath2(brex, &opts);
#endif

	rep = xmlNewDoc(BAD_CAST "1.0");
	node = xmlNewNode(NULL, BAD_CAST "brexCheck");
	xmlDocSetRootElement(rep, node);
	add_config_to_report(node, &opts);

	node = xmlNewChild(node, NULL, BAD_CAST "document", NULL);
	xmlSetProp(node, BAD_CAST "path", doc->URL);

	brex_dmc = default_brex_dmc(doc);
	brex = load_brex(brex_dmc, doc);

	ctx = xmlXPathNewContext(brex);
	obj = xmlXPathEvalExpression(BAD_CAST "//structureObjectRule", ctx);

#if XPATH2_ENGINE == SAXON
	if (use_xpath2) {
		saxon_processor = saxon_new_processor();
		saxon_node = saxon_new_node(saxon_processor, doc);

		err = check_brex_rules(brex, obj->nodesetval, doc, doc->URL, brex_dmc, node, &opts, saxon_processor, saxon_node);

		saxon_free_node(saxon_node);
		saxon_free_processor(saxon_processor);
	} else {
		err = check_brex_rules(brex, obj->nodesetval, doc, doc->URL, brex_dmc, node, &opts, NULL, NULL);
	}
#elif XPATH2_ENGINE == XQILLA
	if (use_xpath2) {
		xqilla_impl = xqilla_initialize();
		xqilla_parser = xqilla_create_parser(xqilla_impl);
		xqilla_doc = xqilla_create_doc(xqilla_impl, xqilla_parser, doc);

		err = check_brex_rules(brex, obj->nodesetval, doc, doc->URL, brex_dmc, node, &opts, xqilla_doc);

		xqilla_free_parser(xqilla_parser);
		xqilla_terminate();
	} else {
		err = check_brex_rules(brex, obj->nodesetval, doc, doc->URL, brex_dmc, node, &opts, NULL);
	}
#else
	err = check_brex_rules(brex, obj->nodesetval, doc, doc->URL, brex_dmc, node, &opts);
#endif

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
	xmlFreeDoc(brex);

	if (report) {
		*report = rep;
	} else {
		xmlFreeDoc(rep);
	}

	free_opts(&opts);

	return err;
}

int s1kdCheckDefaultBREX(const char *object_xml, int object_size, int options, char **report_xml, int *report_size)
{
	xmlDocPtr doc, rep;
	int err;

	doc = read_xml_mem(object_xml, object_size);
	err = s1kdDocCheckDefaultBREX(doc, options, &rep);
	xmlFreeDoc(doc);

	if (report_xml && report_size) {
		xmlDocDumpMemory(rep, (xmlChar **) report_xml, report_size);
		xmlFreeDoc(rep);
	}

	return err;
}

int s1kdDocCheckBREX(xmlDocPtr doc, xmlDocPtr brex, int options, xmlDocPtr *report)
{
	int err = 0;
	xmlDocPtr rep;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;
	struct opts opts;
#if XPATH2_ENGINE == SAXON
	void *saxon_processor, *saxon_node;
#elif XPATH2_ENGINE == XQILLA
	void *xqilla_impl, *xqilla_parser, *xqilla_doc;
#endif
#if XPATH2_ENGINE != NONE
	bool use_xpath2;
#endif

	init_opts(&opts, options);

#if XPATH2_ENGINE != NONE
	use_xpath2 = brex_requires_xpath2(brex, &opts);
#endif

	rep = xmlNewDoc(BAD_CAST "1.0");
	node = xmlNewNode(NULL, BAD_CAST "brexCheck");
	xmlDocSetRootElement(rep, node);
	add_config_to_report(node, &opts);

	node = xmlNewChild(node, NULL, BAD_CAST "document", NULL);
	xmlSetProp(node, BAD_CAST "path", doc->URL);

	ctx = xmlXPathNewContext(brex);
	obj = xmlXPathEvalExpression(BAD_CAST "//structureObjectRule", ctx);

	if (opts.check_sns) {
		xmlDocPtr snsRulesDoc = xmlNewDoc(BAD_CAST "1.0");
		xmlNodePtr snsRulesGroup = xmlNewNode(NULL, BAD_CAST "snsRules");

		xmlDocSetRootElement(snsRulesDoc, snsRulesGroup);
		xmlAddChild(snsRulesGroup, xmlCopyNode(firstXPathNode(brex, NULL, "//snsRules"), 1));

		err += check_brex_sns_rules(snsRulesDoc, snsRulesGroup, doc, node, &opts);

		xmlFreeDoc(snsRulesDoc);
	}

	if (opts.check_notations) {
		xmlDocPtr notationRulesDoc = xmlNewDoc(BAD_CAST "1.0");
		xmlNodePtr notationRulesGroup = xmlNewNode(NULL, BAD_CAST "notationRules");

		xmlDocSetRootElement(notationRulesDoc, notationRulesGroup);
		xmlAddChild(notationRulesGroup, xmlCopyNode(firstXPathNode(brex, NULL, "//notationRuleList"), 1));

		err += check_brex_notation_rules(notationRulesDoc, notationRulesGroup, doc, node, &opts);

		xmlFreeDoc(notationRulesDoc);
	}

#if XPATH2_ENGINE == SAXON
	if (use_xpath2) {
		saxon_processor = saxon_new_processor();
		saxon_node = saxon_new_node(saxon_processor, doc);

		err += check_brex_rules(brex, obj->nodesetval, doc, doc->URL, brex->URL, node, &opts, saxon_processor, saxon_node);

		saxon_free_node(saxon_node);
		saxon_free_processor(saxon_processor);
	} else {
		err += check_brex_rules(brex, obj->nodesetval, doc, doc->URL, brex->URL, node, &opts, NULL, NULL);
	}
#elif XPATH2_ENGINE == XQILLA
	if (use_xpath2) {
		xqilla_impl = xqilla_initialize();
		xqilla_parser = xqilla_create_parser(xqilla_impl);
		xqilla_doc = xqilla_create_doc(xqilla_impl, xqilla_parser, doc);

		err += check_brex_rules(brex, obj->nodesetval, doc, doc->URL, brex->URL, node, &opts, xqilla_doc);

		xqilla_free_parser(xqilla_parser);
		xqilla_terminate();
	} else {
		err += check_brex_rules(brex, obj->nodesetval, doc, doc->URL, brex->URL, node, &opts, NULL);
	}
#else
	err += check_brex_rules(brex, obj->nodesetval, doc, doc->URL, brex->URL, node, &opts);
#endif

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	if (report) {
		*report = rep;
	} else {
		xmlFreeDoc(rep);
	}

	free_opts(&opts);

	return err;
}

int s1kdCheckBREX(const char *object_xml, int object_size, const char *brex_xml, int brex_size, int options, char **report_xml, int *report_size)
{
	xmlDocPtr doc, brex, rep;
	int err;

	doc = read_xml_mem(object_xml, object_size);
	brex = read_xml_mem(brex_xml, brex_size);
	err = s1kdDocCheckBREX(doc, brex, options, &rep);
	xmlFreeDoc(doc);
	xmlFreeDoc(brex);

	if (report_xml && report_size) {
		xmlDocDumpMemory(rep, (xmlChar **) report_xml, report_size);
		xmlFreeDoc(rep);
	}

	return err;
}
#else
/* Show usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-b <brex>] [-d <dir>] [-I <path>] [-w <file>] [-X <version>] [-F|-f] [-BceLlNnopqrS[tu]sTvx8^h?] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -B, --default-brex                   Use the default BREX.");
	puts("  -b, --brex <brex>                    Use <brex> as the BREX data module.");
	puts("  -c, --values                         Check object values.");
	puts("  -d, --dir <dir>                      Directory to start search for BREX in.");
	puts("  -e, --ignore-empty                   Ignore empty/non-XML files.");
	puts("  -F, --valid-filenames                Print the filenames of valid objects.");
	puts("  -f, --filenames                      Print the filenames of invalid objects.");
	puts("  -h, -?, --help                       Show this help message.");
	puts("  -I, --include <path>                 Add <path> to search path for BREX data module.");
	puts("  -L, --list                           Input is a list of data module filenames.");
	puts("  -l, --layered                        Check BREX referenced by other BREX.");
	puts("  -N, --omit-issue                     Assume issue/inwork numbers are omitted.");
	puts("  -n, --notations                      Check notation rules.");
	puts("  -o, --output-valid                   Output valid CSDB objects to stdout.");
	puts("  -p, --progress                       Display progress bar.");
	puts("  -q, --quiet                          Quiet mode. Do not print errors.");
	puts("  -r, --recursive                      Search for BREX recursively.");
	puts("  -S[tu], --sns [--strict|--unstrict]  Check SNS rules.");
	puts("  -s, --short                          Short messages.");
	puts("  -T, --summary                        Print a summary of the check.");
	puts("  -v, --verbose                        Verbose mode.");
	puts("  -w, --severity-levels <file>         List of severity levels.");
	puts("  -X, --xpath-version <version>        Force the version of XPath that will be used.");
	puts("  -x, --xml                            XML output.");
	puts("  -8, --deep-copy-nodes                The XML report will contain a deep copy of invalid nodes.");
	puts("  -^, --remove-deleted                 Check with elements marked as \"delete\" removed.");
	puts("  --version                            Show version information.");
	puts("  --zenity-progress                    Prints progress information in the zenity --progress format.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Show version information. */
#if XPATH2_ENGINE == SAXON
static void show_version(void *saxon_processor)
#else
static void show_version(void)
#endif
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);

#if XPATH2_ENGINE == SAXON
	printf("Using libxml %s, libxslt %s, libexslt %s and %s\n", xmlParserVersion, xsltEngineVersion, exsltLibraryVersion, saxon_version(saxon_processor));
	puts("XPath support: 1.0 (libxml), 2.0 (Saxon)");
#elif XPATH2_ENGINE == XQILLA
	printf("Using libxml %s, libxslt %s, libexslt %s and Xerces-C %s + XQilla\n", xmlParserVersion, xsltEngineVersion, exsltLibraryVersion, xqilla_version());
	puts("XPath support: 1.0 (libxml), 2.0 (XQilla)");
#else
	printf("Using libxml %s, libxslt %s and libexslt %s\n", xmlParserVersion, xsltEngineVersion, exsltLibraryVersion);
	puts("XPath support: 1.0 (libxml)");
#endif
}

int main(int argc, char *argv[])
{
	int c;
	int i;

	char (*brex_fnames)[PATH_MAX] = malloc(BREX_MAX * PATH_MAX);
	int num_brex_fnames = 0;

	char (*brex_search_paths)[PATH_MAX] = malloc(BREX_PATH_MAX * PATH_MAX);
	int num_brex_search_paths = 0;

	char (*dmod_fnames)[PATH_MAX] = malloc(DMOD_MAX * PATH_MAX);
	int num_dmod_fnames = 0;

	int status = 0;

	bool use_stdin = false;
	bool xmlout = false;
	int show_progress = PROGRESS_OFF;
	bool is_list = false;
	bool use_default_brex = false;
	bool show_stats = false;

	xmlDocPtr outdoc;
	xmlNodePtr brexCheck;

	struct opts opts = {
		/* verbosity */ NORMAL,
		/* layered */ false,
		/* check_values */ false,
		/* check_sns */ false,
		/* strict_sns */ false,
		/* unstrict_sns */ false,
		/* check_notations */ false,
		/* ignore_issue */ false,
		/* xpath_version */ DYNAMIC
	};

	const char *sopts = "Bb:eI:xvqslw:StupFfNncLTrd:oX:8^h?";
	struct option lopts[] = {
		{"version"        , no_argument      , 0, 0},
		{"help"           , no_argument      , 0, 'h'},
		{"default-brex"   , no_argument      , 0, 'B'},
		{"brex"           , required_argument, 0, 'b'},
		{"dir"            , required_argument, 0, 'd'},
		{"ignore-empty"   , no_argument      , 0, 'e'},
		{"include"        , required_argument, 0, 'I'},
		{"xml"            , no_argument      , 0, 'x'},
		{"quiet"          , no_argument      , 0, 'q'},
		{"verbose"        , no_argument      , 0, 'v'},
		{"short"          , no_argument      , 0, 's'},
		{"layered"        , no_argument      , 0, 'l'},
		{"severity-levels", required_argument, 0, 'w'},
		{"sns"            , no_argument      , 0, 'S'},
		{"strict"         , no_argument      , 0, 't'},
		{"unstrict"       , no_argument      , 0, 'u'},
		{"progress"       , no_argument      , 0, 'p'},
		{"valid-filenames", no_argument      , 0, 'F'},
		{"filenames"      , no_argument      , 0, 'f'},
		{"omit-issue"     , no_argument      , 0, 'N'},
		{"notations"      , no_argument      , 0, 'n'},
		{"values"         , no_argument      , 0, 'c'},
		{"list"           , no_argument      , 0, 'L'},
		{"summary"        , no_argument      , 0, 'T'},
		{"recursive"      , no_argument      , 0, 'r'},
		{"output-valid"   , no_argument      , 0, 'o'},
		{"xpath-version"  , required_argument, 0, 'X'},
		{"deep-copy-nodes", no_argument      , 0, '8'},
		{"remove-deleted" , no_argument      , 0, '^'},
		{"zenity-progress", no_argument      , 0, 0},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

#if XPATH2_ENGINE == SAXON
	void *saxon_processor;

	saxon_processor = saxon_new_processor();
#elif XPATH2_ENGINE == XQILLA
	void *xqilla_impl;

	xqilla_impl = xqilla_initialize();

#endif

	search_dir = strdup(".");

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
#if XPATH2_ENGINE == SAXON
					show_version(saxon_processor);
#else
					show_version();
#endif
					goto cleanup;
				} else if (strcmp(lopts[loptind].name, "zenity-progress") == 0) {
					show_progress = PROGRESS_ZENITY;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg)
				break;
			case 'B':
				use_default_brex = true;
				break;
			case 'b':
				add_path(&brex_fnames, &num_brex_fnames, &BREX_MAX, optarg, &opts);
				break;
			case 'd':
				free(search_dir);
				search_dir = strdup(optarg);
				break;
			case 'I':
				add_path(&brex_search_paths, &num_brex_search_paths, &BREX_PATH_MAX, optarg, &opts);
				break;
			case '8': deep_copy_nodes = true;
			case 'x': xmlout = true; break;
			case 'q': opts.verbosity = SILENT; break;
			case 'v': opts.verbosity = VERBOSE; break;
			case 's': shortmsg = true; break;
			case 'l': opts.layered = true; break;
			case 'w': brsl_fname = strdup(optarg); break;
			case 'S': opts.check_sns = true; break;
			case 't': opts.strict_sns = true; break;
			case 'u': opts.unstrict_sns = true; break;
			case 'p': show_progress = PROGRESS_CLI; break;
			case 'F': show_fnames = SHOW_VALID; break;
			case 'f': show_fnames = SHOW_INVALID; break;
			case 'N': opts.ignore_issue = true; break;
			case 'n': opts.check_notations = true; break;
			case 'c': opts.check_values = true; break;
			case 'L': is_list = true; break;
			case 'T': show_stats = true; break;
			case 'r': recursive_search = true; break;
			case 'o': output_tree = true; break;
			case 'e': ignore_empty = true; break;
			case 'X': set_xpath_version(&opts, optarg); break;
			case '^':
				rem_delete = true;
				break;
			case 'h':
			case '?':
				show_help();
				goto cleanup;
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
				add_dmod_list(argv[i], &dmod_fnames, &num_dmod_fnames, &opts);
			} else {
				add_path(&dmod_fnames, &num_dmod_fnames, &DMOD_MAX, argv[i], &opts);
			}
		}
	} else if (is_list) {
		add_dmod_list(NULL, &dmod_fnames, &num_dmod_fnames, &opts);
	} else {
		strcpy(dmod_fnames[num_dmod_fnames++], "-");
		use_stdin = true;
	}

	if (brsl_fname) {
		brsl = read_xml_doc(brsl_fname);
	}

	outdoc = xmlNewDoc(BAD_CAST "1.0");
	brexCheck = xmlNewNode(NULL, BAD_CAST "brexCheck");
	xmlDocSetRootElement(outdoc, brexCheck);

	/* Add configuration info to XML report. */
	add_config_to_report(brexCheck, &opts);

	for (i = 0; i < num_dmod_fnames; ++i) {
		/* Indicates if a referenced BREX data module is used as
		 * opposed to one specified on the command line.
		 *
		 * The practical difference is that those specified on the
		 * command line are meant to apply to ALL data modules
		 * specified, while a referenced BREX only applies to the data
		 * module which referenced it. */
		bool ref_brex = false;

		xmlDocPtr dmod_doc = read_xml_doc(dmod_fnames[i]);

		if (!dmod_doc) {
			if (ignore_empty) {
				continue;
			} else if (use_stdin) {
				if (opts.verbosity > SILENT) fprintf(stderr, E_NODMOD_STDIN);
			} else {
				if (opts.verbosity > SILENT) fprintf(stderr, E_NODMOD, dmod_fnames[i]);
			}
			exit(EXIT_BAD_DMODULE);
		}

		if (num_brex_fnames == 0) {
			int err;

			strcpy(brex_fnames[0], "");

			/* Override the referenced BREX with a default BREX
			 * based on which issue of the specification a data
			 * module is written to.
			 */
			if (use_default_brex) {
				strcpy(brex_fnames[0], default_brex_dmc(dmod_doc));
			/* Find BREX file from the brexDmRef and store it in the
			 * list of BREX.
			 *
			 * If the object has no brexDmRef or the BREX is not
			 * found, skip it.
			 *
			 * Indicate a BREX error in the exit status code if the
			 * object references a BREX but it couldn't be located.
			 */
			} else if ((err = find_brex_fname_from_doc(
					brex_fnames[0], dmod_doc,
					brex_search_paths,
					num_brex_search_paths,
					dmod_fnames,
					num_dmod_fnames,
					&opts))) {
				if (use_stdin) {
					if (opts.verbosity > SILENT) fprintf(stderr, err == 1 ? E_NOBREX_STDIN : W_NOBREX_STDIN);
				} else {
					if (opts.verbosity > SILENT) fprintf(stderr, err == 1 ? E_NOBREX : W_NOBREX, dmod_fnames[i]);
				}

				/* BREX DM was referenced but not found. */
				if (err == 1) {
					exit(EXIT_BREX_NOT_FOUND);
				}

				xmlFreeDoc(dmod_doc);
				continue;
			}

			num_brex_fnames = 1;
			ref_brex = true;

			/* When using brexDmRef, if the data module is itself a
			 * BREX data module, include it as a BREX. */
			if (strcmp(brex_fnames[0], dmod_fnames[i]) != 0 && firstXPathNode(dmod_doc, NULL, "//brex")) {
				add_path(&brex_fnames, &num_brex_fnames, &BREX_MAX, dmod_fnames[i], &opts);
			}
		}

		if (opts.layered) {
			num_brex_fnames = add_layered_brex(&brex_fnames,
				num_brex_fnames, brex_search_paths,
				num_brex_search_paths,
				dmod_fnames, num_dmod_fnames, dmod_doc, &opts);
		}

#if XPATH2_ENGINE == SAXON
		status += check_brex(dmod_doc, dmod_fnames[i], brex_fnames, num_brex_fnames, brexCheck, &opts, saxon_processor);
#elif XPATH2_ENGINE == XQILLA
		status += check_brex(dmod_doc, dmod_fnames[i], brex_fnames, num_brex_fnames, brexCheck, &opts, xqilla_impl);
#else
		status += check_brex(dmod_doc, dmod_fnames[i], brex_fnames, num_brex_fnames, brexCheck, &opts);
#endif

		xmlFreeDoc(dmod_doc);

		switch (show_progress) {
			case PROGRESS_OFF:
				break;
			case PROGRESS_CLI:
				print_progress_bar(i, num_dmod_fnames);
				break;
			case PROGRESS_ZENITY:
				print_zenity_progress("Performing BREX check...", i, num_dmod_fnames);
				break;
		}

		/* If the referenced BREX was used, reset the BREX data module
		 * list, as each data module may reference a different BREX or
		 * set of BREX. */
		if (ref_brex)
			num_brex_fnames = 0;
	}

	if (num_dmod_fnames) {
		switch (show_progress) {
			case PROGRESS_OFF:
				break;
			case PROGRESS_CLI:
				print_progress_bar(i, num_dmod_fnames);
				break;
			case PROGRESS_ZENITY:
				print_zenity_progress("BREX check complete.", i, num_dmod_fnames);
				break;
		}
	}

	if (xmlout) {
		save_xml_doc(outdoc, "-");
	}

	if (show_stats) {
		print_stats(outdoc);
	}

	xmlFreeDoc(outdoc);

	if (brsl_fname) {
		xmlFreeDoc(brsl);
		free(brsl_fname);
	}

cleanup:
	xsltCleanupGlobals();
	xmlCleanupParser();

	free(brex_fnames);
	free(brex_search_paths);
	free(dmod_fnames);
	free(search_dir);

#if XPATH2_ENGINE == SAXON
	saxon_free_processor(saxon_processor);
	saxon_cleanup();
#elif XPATH2_ENGINE == XQILLA
	xqilla_terminate();
#endif

	if (status > 0) {
		return EXIT_BREX_ERROR;
	} else {
		return EXIT_SUCCESS;
	}
}
#endif
