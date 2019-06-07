#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/transform.h>
#include "s1kd_tools.h"
#include "stylesheets.h"

/* Program name and version information. */
#define PROG_NAME "s1kd-appcheck"
#define VERSION "3.0.0"

/* Message prefixes. */
#define ERR_PREFIX PROG_NAME ": ERROR: "
#define WRN_PREFIX PROG_NAME ": WARNING: "
#define INF_PREFIX PROG_NAME ": INFO: "
#define SUC_PREFIX PROG_NAME ": SUCCESS: "
#define FLD_PREFIX PROG_NAME ": FAILED: "

/* Info messages. */
#define I_CHECK_PROD INF_PREFIX "Checking %s for product %s in %s...\n"
#define I_CHECK_PROD_LINENO INF_PREFIX "Checking %s for product on line %ld of %s...\n"
#define I_CHECK_ALL_START INF_PREFIX "Checking %s for:\n"
#define I_CHECK_ALL_PROP INF_PREFIX "  %s %s = %s\n"

/* Error messages. */
#define E_CHECK_FAIL_PROD ERR_PREFIX "%s is invalid for product %s (line %ld of %s)\n"
#define E_CHECK_FAIL_PROD_LINENO ERR_PREFIX "%s is invalid for product on line %ld of %s\n"
#define E_CHECK_FAIL_ALL_START ERR_PREFIX "%s is invalid when:\n"
#define E_CHECK_FAIL_ALL_PROP ERR_PREFIX "  %s %s = %s\n"
#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"
#define E_NO_ACT ERR_PREFIX "%s uses computable applicability, but no ACT could be found.\n"
#define E_NO_CCT ERR_PREFIX "%s uses conditions, but no CCT could be found.\n"
#define E_BAD_OBJECT ERR_PREFIX "Could not read object: %s\n"
#define E_PROPCHECK ERR_PREFIX "%s: %s %s is not defined (line %ld)\n"
#define E_PROPCHECK_VAL ERR_PREFIX "%s: %s is not a defined value of %s %s (line %ld)\n"

/* Warning messages. */
#define W_MISSING_REF_DM WRN_PREFIX "Could not read referenced object: %s\n"

/* Success messages. */
#define S_VALID SUC_PREFIX "%s passed the applicability check.\n"

/* Failure messages. */
#define F_INVALID FLD_PREFIX "%s failed the applicability check.\n"

/* Exit status codes. */
#define EXIT_BAD_OBJECT 2

/* Search for ACT, CCT, PCT recursively. */
bool recursive_search = false;

/* Assume issue/inwork numbers are omitted. */
bool no_issue = false;

/* Directory to search for ACT, CCT, PCT in. */
char *search_dir;

/* The verbosity of output. */
enum verbosity { QUIET, NORMAL, VERBOSE, DEBUG } verbosity = NORMAL;

/* What type of check to perform. */
enum appcheckmode { BASIC, PCT, ALL, STANDALONE };

/* Applicability check options. */
struct appcheckopts {
	char *useract;
	char *usercct;
	char *userpct;
	char *args;
	xmlNodePtr validators;
	bool output_tree;
	bool filenames;
	bool brexcheck;
	bool add_deps;
	bool check_props;
	enum appcheckmode mode;
};

/* Show usage message. */
void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -A, --act <file>    User-specified ACT.");
	puts("  -a, --all           Validate against all property values.");
	puts("  -b, --brexcheck     Validate against BREX.");
	puts("  -C, --cct <file>    User-specified CCT.");
	puts("  -c, --basic         Only check that all properties are defined.");
	puts("  -d, --dir <dir>     Search for ACT/CCT/PCT in <dir>.");
	puts("  -e, --exec <cmd>    Commands used to validate objects.");
	puts("  -f, --filenames     List invalid files.");
	puts("  -h, -?, --help      Show help/usage message.");
	puts("  -k, --args <args>   Arguments used to create objects.");
	puts("  -l, --list          Treat input as list of CSDB objects.");
	puts("  -N, --omit-issue    Assume issue/inwork numbers are omitted.");
	puts("  -o, --output-valid  Output valid CSDB objects to stdout.");
	puts("  -P, --pct <file>    User-specified PCT.");
	puts("  -p, --products      Validate against product instances.");
	puts("  -q, --quiet         Quiet mode.");
	puts("  -r, --recursive     Search for ACT/CCT/PCT recursively.");
	puts("  -s, --strict        Check that all properties are defined.");
	puts("  -T, --summary       Print a summary of the check.");
	puts("  -v, --verbose       Verbose output.");
	puts("  -x, --xml           Output XML report.");
	puts("  -~, --dependencies  Check CCT dependencies.");
	puts("  --version           Show version information.");
	puts("  <object>...         CSDB object(s) to check.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Show version information. */
void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

/* Return the first node matching an XPath expression. */
xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const xmlChar *path)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	if (doc) {
		ctx = xmlXPathNewContext(doc);
	} else {
		ctx = xmlXPathNewContext(node->doc);
	}

	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST path, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		first = NULL;
	} else {
		first = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

/* Return the value of the first node matching an XPath expression. */
xmlChar *first_xpath_value(xmlDocPtr doc, xmlNodePtr node, const xmlChar *path)
{
	return xmlNodeGetContent(first_xpath_node(doc, node, path));
}

/* Determine if the file is a data module. */
bool is_dm(const char *name)
{
	return strncmp(name, "DMC-", 4) == 0 && strncasecmp(name + strlen(name) - 4, ".XML", 4) == 0;
}

/* Find a data module filename in the current directory based on the dmRefIdent
 * element. */
bool find_dmod_fname(char *dst, xmlNodePtr dmRefIdent, bool ignore_iss)
{
	bool found = false;
	char *model_ident_code;
	char *system_diff_code;
	char *system_code;
	char *sub_system_code;
	char *sub_sub_system_code;
	char *assy_code;
	char *disassy_code;
	char *disassy_code_variant;
	char *info_code;
	char *info_code_variant;
	char *item_location_code;
	char *learn_code;
	char *learn_event_code;
	char code[64];
	xmlNodePtr dmCode, issueInfo, language;

	dmCode = first_xpath_node(NULL, dmRefIdent, BAD_CAST "dmCode|avee");
	issueInfo = first_xpath_node(NULL, dmRefIdent, BAD_CAST "issueInfo|issno");
	language = first_xpath_node(NULL, dmRefIdent, BAD_CAST "language");

	model_ident_code     = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "modelic|@modelIdentCode");
	system_diff_code     = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "sdc|@systemDiffCode");
	system_code          = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "chapnum|@systemCode");
	sub_system_code      = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "section|@subSystemCode");
	sub_sub_system_code  = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "subsect|@subSubSystemCode");
	assy_code            = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "subject|@assyCode");
	disassy_code         = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "discode|@disassyCode");
	disassy_code_variant = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "discodev|@disassyCodeVariant");
	info_code            = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "incode|@infoCode");
	info_code_variant    = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "incodev|@infoCodeVariant");
	item_location_code   = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "itemloc|@itemLocationCode");
	learn_code           = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "@learnCode");
	learn_event_code     = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "@learnEventCode");

	snprintf(code, 64, "DMC-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
		model_ident_code,
		system_diff_code,
		system_code,
		sub_system_code,
		sub_sub_system_code,
		assy_code,
		disassy_code,
		disassy_code_variant,
		info_code,
		info_code_variant,
		item_location_code);

	xmlFree(model_ident_code);
	xmlFree(system_diff_code);
	xmlFree(system_code);
	xmlFree(sub_system_code);
	xmlFree(sub_sub_system_code);
	xmlFree(assy_code);
	xmlFree(disassy_code);
	xmlFree(disassy_code_variant);
	xmlFree(info_code);
	xmlFree(info_code_variant);
	xmlFree(item_location_code);

	if (learn_code) {
		char learn[8];
		snprintf(learn, 8, "-%s%s", learn_code, learn_event_code);
		strcat(code, learn);
	}

	xmlFree(learn_code);
	xmlFree(learn_event_code);

	if (issueInfo && !(no_issue || ignore_iss)) {
		char *issue_number;
		char *in_work;
		char iss[8];

		issue_number = (char *) first_xpath_value(NULL, issueInfo, BAD_CAST "@issno|@issueNumber");
		in_work      = (char *) first_xpath_value(NULL, issueInfo, BAD_CAST "@inwork|@inWork");

		snprintf(iss, 8, "_%s-%s", issue_number, in_work ? in_work : "00");
		strcat(code, iss);

		xmlFree(issue_number);
		xmlFree(in_work);
	}

	if (language && !ignore_iss) {
		char *language_iso_code;
		char *country_iso_code;
		char lang[8];

		language_iso_code = (char *) first_xpath_value(NULL, language, BAD_CAST "@language|@languageIsoCode");
		country_iso_code  = (char *) first_xpath_value(NULL, language, BAD_CAST "@country|@countryIsoCode");

		snprintf(lang, 8, "_%s-%s", language_iso_code, country_iso_code);
		strcat(code, lang);

		xmlFree(language_iso_code);
		xmlFree(country_iso_code);
	}

	found = find_csdb_object(dst, search_dir, code, is_dm, recursive_search);

	if (!found) {
		fprintf(stderr, W_MISSING_REF_DM, code);
	}

	return found;
}

/* Find the filename of a referenced ACT data module. */
bool find_act_fname(char *dst, const char *useract, xmlDocPtr doc)
{
	if (useract) {
		strcpy(dst, useract);
		return true;
	} else if (doc) {
		xmlNodePtr actref;
		actref = first_xpath_node(doc, NULL, BAD_CAST "//applicCrossRefTableRef/dmRef/dmRefIdent|//actref/refdm");
		return actref && find_dmod_fname(dst, actref, false);
	}

	return false;
}

/* Find the filename of a referenced CCT data module. */
bool find_cct_fname(char *dst, const char *usercct, xmlDocPtr act)
{
	if (usercct) {
		strcpy(dst, usercct);
		return true;
	} else if (act) {
		xmlNodePtr cctref;
		cctref = first_xpath_node(act, NULL, BAD_CAST "//condCrossRefTableRef/dmRef/dmRefIdent|//cctref/refdm");
		return cctref && find_dmod_fname(dst, cctref, false);
	}

	return false;
}

/* Find the filename of a referenced PCT data module via the ACT. */
bool find_pct_fname(char *dst, const char *userpct, xmlDocPtr act)
{
	if (userpct) {
		strcpy(dst, userpct);
		return true;
	} else if (act) {
		xmlNodePtr pctref;
		pctref = first_xpath_node(act, NULL, BAD_CAST "//productCrossRefTableRef/dmRef/dmRefIdent|//pctref/refdm");
		return pctref && find_dmod_fname(dst, pctref, false);
	}

	return false;
}

/* Test whether an object value matches a regex pattern. */
bool match_pattern(const xmlChar *value, const xmlChar *pattern)
{
	xmlRegexpPtr regex;
	bool match;
	regex = xmlRegexpCompile(BAD_CAST pattern);
	match = xmlRegexpExec(regex, BAD_CAST value);
	xmlRegFreeRegexp(regex);
	return match;
}

/* Add an undefined property node to the report. */
xmlNodePtr add_undef_node(xmlNodePtr report, xmlNodePtr assert, const xmlChar *id, const xmlChar *type, const xmlChar *val, long int line)
{
	xmlNodePtr und;
	xmlChar line_s[16], *xpath;

	und = xmlNewChild(report, NULL, BAD_CAST "undefined", NULL);

	xmlSetProp(und, BAD_CAST "applicPropertyIdent", id);
	xmlSetProp(und, BAD_CAST "applicPropertyType", type);

	if (val) {
		xmlSetProp(und, BAD_CAST "applicPropertyValue", val);
	}

	xmlStrPrintf(line_s, 16, "%ld", line);
	xmlSetProp(und, BAD_CAST "line", line_s);

	xpath = xpath_of(assert);
	xmlSetProp(und, BAD_CAST "xpath", xpath);
	xmlFree(xpath);

	return und;
}

/* Check whether a property value is defined in the ACT/CCT. */
int check_val_against_prop(xmlNodePtr assert, const xmlChar *id, const xmlChar *type, const xmlChar *val, xmlNodePtr prop, const char *path, xmlNodePtr report)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	bool match;
	xmlChar *pattern;

	pattern = first_xpath_value(NULL, prop, BAD_CAST "@valuePattern|@pattern");

	if (pattern) {
		match = match_pattern(val, pattern);
		xmlFree(pattern);
	} else {
		match = false;

		ctx = xmlXPathNewContext(prop->doc);
		ctx->node = prop;

		obj = xmlXPathEvalExpression(BAD_CAST "enumeration|enum", ctx);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			int i;

			for (i = 0; !match && i < obj->nodesetval->nodeNr; ++i) {
				xmlChar *vals, *v = NULL;
				char *end = NULL;

				vals = first_xpath_value(NULL, obj->nodesetval->nodeTab[i],
					BAD_CAST "@applicPropertyValues|@actvalues");

				while ((v = BAD_CAST strtok_r(v ? NULL : (char *) vals, "|", &end))) {
					if (is_in_range((char *) val, (char *) v)) {
						match = true;
						break;
					}
				}

				xmlFree(vals);
			}
		}

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);
	}

	if (match) {
		return 0;
	} else {
		long int line;

		line = xmlGetLineNo(assert);

		if (verbosity >= NORMAL) {
			fprintf(stderr, E_PROPCHECK_VAL, path, (char *) val, (char *) type, (char *) id, line);
		}

		add_undef_node(report, assert, id, type, val, line);
	}

	return 1;
}

/* Check whether a property is defined in the ACT/CCT. */
int check_prop_against_ct(xmlNodePtr assert, xmlDocPtr act, xmlDocPtr cct, const char *path, xmlNodePtr report)
{
	xmlChar *id, *type, *vals, *xpath = NULL;
	int n, err = 0;
	xmlNodePtr prop;

	if (!(id = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyIdent|@actidref"))) {
		return 0;
	}
	if (!(type = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyType|@actreftype"))) {
		return 0;
	}
	if (!(vals = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyValues|@applicPropertyValue|@actvalues|@actvalue"))) {
		return 0;
	}

	if (xmlStrcmp(type, BAD_CAST "condition") == 0 && cct) {
		/* For conditions, first get the condition itself. */
		n = xmlStrlen(id) + 29;
		xpath = malloc(n * sizeof(xmlChar));
		xmlStrPrintf(xpath, n, "(//cond|//condition)[@id='%s']", id);
		prop = first_xpath_node(cct, NULL, xpath);

		/* Then get the condition type. */
		if (prop) {
			xmlChar *condtype;

			condtype = first_xpath_value(cct, prop, BAD_CAST "@condTypeRefId|@condtyperef");

			if (condtype) {
				xmlFree(xpath);
				n = xmlStrlen(condtype) + 37;
				xpath = malloc(n * sizeof(xmlChar));
				xmlStrPrintf(xpath, n, "(//condType|//conditiontype)[@id='%s']", condtype);
				prop = first_xpath_node(cct, NULL, xpath);
			}

			xmlFree(condtype);
		}
	} else if (xmlStrcmp(type, BAD_CAST "prodattr") == 0 && act) {
		n = xmlStrlen(id) + 40;
		xpath = malloc(n * sizeof(xmlChar));
		xmlStrPrintf(xpath, n, "(//productAttribute|//prodattr)[@id='%s']", id);
		prop = first_xpath_node(act, NULL, xpath);
	} else {
		prop = NULL;
	}

	xmlFree(xpath);

	if (prop) {
		xmlChar *v = NULL;
		char *end = NULL;

		while ((v = BAD_CAST strtok_r(v ? NULL : (char *) vals, "|~", &end))) {
			err += check_val_against_prop(assert, id, type, v, prop, path, report);
		}
	} else {
		long int line;

		line = xmlGetLineNo(assert);

		if (verbosity >= NORMAL) {
			fprintf(stderr, E_PROPCHECK, path, (char *) type, (char *) id, line);
		}

		add_undef_node(report, assert, id, type, NULL, line);

		++err;
	}

	xmlFree(id);
	xmlFree(type);
	xmlFree(vals);

	return err;
}

/* Check whether all properties in an object are defined in the ACT/CCT. */
int check_props_against_cts(xmlDocPtr doc, const char *path, xmlDocPtr act, xmlDocPtr cct, xmlNodePtr report)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST "//assert", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			err += check_prop_against_ct(obj->nodesetval->nodeTab[i], act, cct, path, report);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return err;
}

/* Add an assignment to a set of assertions. */
void add_assign(xmlNodePtr asserts, xmlNodePtr assert)
{
	xmlChar *i, *t, *v;
	xmlNodePtr new;

	i = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyIdent|@actidref");
	t = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyType|@actreftype");
	v = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyValue|@actvalue");

	new = xmlNewNode(NULL, BAD_CAST "assign");
	xmlSetProp(new, BAD_CAST "applicPropertyIdent", i);
	xmlSetProp(new, BAD_CAST "applicPropertyType", t);
	xmlSetProp(new, BAD_CAST "applicPropertyValue", v);

	xmlAddChild(asserts, new);

	xmlFree(i);
	xmlFree(t);
	xmlFree(v);
}

/* Extract the assignments in a PCT instance. */
void extract_assigns(xmlNodePtr asserts, xmlNodePtr product)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(product->doc);
	ctx->node = product;
	obj = xmlXPathEvalExpression(BAD_CAST ".//assign", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			add_assign(asserts, obj->nodesetval->nodeTab[i]);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Check if an object is valid for a set of assertions. */
int check_assigns(xmlDocPtr doc, const char *path, xmlNodePtr asserts, xmlNodePtr product, const xmlChar *id, const char *pctfname, struct appcheckopts *opts)
{
	xmlNodePtr cur;
	int err = 0, e = 0;
	char filter_cmd[1024] = "s1kd-instance";
	char cmd[4096];
	FILE *p;

	if (verbosity >= DEBUG) {
		if (opts->mode >= ALL) {
			fprintf(stderr, I_CHECK_ALL_START, path);
		} else if (id) {
			fprintf(stderr, I_CHECK_PROD, path, id, pctfname);
		} else {
			fprintf(stderr, I_CHECK_PROD_LINENO, path, xmlGetLineNo(product), pctfname);
		}
	}

	if (opts->args) {
		strcat(filter_cmd, " ");
		strncat(filter_cmd, opts->args, 1023 - strlen(filter_cmd));
	} else {
		strcat(filter_cmd, " -w");
	}

	for (cur = asserts->children; cur; cur = cur->next) {
		char *i, *t, *v;
		char *c;

		i = (char *) first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyIdent");
		t = (char *) first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyType");
		v = (char *) first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyValue");

		if (opts->mode >= ALL && verbosity >= DEBUG) {
			fprintf(stderr, I_CHECK_ALL_PROP, t, i, v);
		}

		c = malloc(strlen(i) + strlen(t) + strlen(v) + 9);
		sprintf(c, " -s \"%s:%s=%s\"", i, t, v);
		strcat(filter_cmd, c);
		free(c);

		xmlFree(i);
		xmlFree(t);
		xmlFree(v);
	}

	/* Custom validators. */
	if (opts->validators) {
		for (cur = opts->validators->children; cur; cur = cur->next) {
			xmlChar *c;

			strcpy(cmd, filter_cmd);
			strcat(cmd, "|");

			c = xmlNodeGetContent(cur);

			strncat(cmd, (char *) c, 4095 - strlen(cmd));

			p = popen(cmd, "w");
			xmlDocDump(p, doc);
			e += pclose(p);

			xmlFree(c);
		}
	/* Default validators. */
	} else {
		strcpy(cmd, filter_cmd);

		/* Schema validation */
		strcat(cmd, "|s1kd-validate -e");

		switch (verbosity) {
			case QUIET:
			case NORMAL:
				strcat(cmd, " -q");
				break;
			case VERBOSE:
				break;
			case DEBUG:
				strcat(cmd, " -v");
				break;
		}

		p = popen(cmd, "w");
		xmlDocDump(p, doc);
		e += pclose(p);

		/* BREX validation */
		if (opts->brexcheck) {
			strcpy(cmd, filter_cmd);
			strcat(cmd, "|s1kd-brexcheck -cel");

			strcat(cmd, " -d '");
			strcat(cmd, search_dir);
			strcat(cmd, "'");

			if (recursive_search) {
				strcat(cmd, " -r");
			}

			switch (verbosity) {
				case QUIET:
				case NORMAL:
					strcat(cmd, " -q");
					break;
				case VERBOSE:
					break;
				case DEBUG:
					strcat(cmd, " -v");
					break;
			}

			p = popen(cmd, "w");
			xmlDocDump(p, doc);
			e += pclose(p);
		}
	}

	if (e) {
		if (verbosity >= NORMAL) {
			if (opts->mode >= ALL) {
				fprintf(stderr, E_CHECK_FAIL_ALL_START, path);
				for (cur = asserts->children; cur; cur = cur->next) {
					char *i, *t, *v;

					i = (char *) first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyIdent");
					t = (char *) first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyType");
					v = (char *) first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyValue");

					fprintf(stderr, E_CHECK_FAIL_ALL_PROP, t, i, v);

					xmlFree(i);
					xmlFree(t);
					xmlFree(v);
				}
			} else if (id) {
				fprintf(stderr, E_CHECK_FAIL_PROD, path, id, xmlGetLineNo(product), pctfname);
			} else {
				fprintf(stderr, E_CHECK_FAIL_PROD_LINENO, path, xmlGetLineNo(product), pctfname);
			}
		}

		xmlSetProp(asserts, BAD_CAST "valid", BAD_CAST "no");
		++err;
	} else {
		xmlSetProp(asserts, BAD_CAST "valid", BAD_CAST "yes");
	}

	return err ? 1 : 0;
}

/* Extract assertions from ACT/CCT enumerations. */
void extract_enumvals(xmlNodePtr asserts, xmlNodePtr prop, const xmlChar *id, bool cct)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(prop->doc);
	ctx->node = prop;
	obj = xmlXPathEvalExpression(BAD_CAST ".//enumeration/@applicPropertyValues|.//enum/@actvalues", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *v;
			xmlChar *c = NULL;

			v = xmlNodeGetContent(obj->nodesetval->nodeTab[i]);

			while ((c = BAD_CAST strtok(c ? NULL : (char *) v, "|~"))) {
				xmlNodePtr assert;

				assert = xmlNewNode(NULL, BAD_CAST "assign");
				xmlSetProp(assert, BAD_CAST "applicPropertyIdent", id);
				xmlSetProp(assert, BAD_CAST "applicPropertyType", BAD_CAST (cct ? "condition" : "prodattr"));
				xmlSetProp(assert, BAD_CAST "applicPropertyValue", c);

				xmlAddChild(asserts, assert);
			}

			xmlFree(v);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* General XSLT transformation with embedded stylesheet, preserving the DTD. */
void transform_doc(xmlDocPtr doc, unsigned char *xml, unsigned int len, const char **params)
{
	xmlDocPtr styledoc, res, src;
	xsltStylesheetPtr style;
	xmlNodePtr old;

	styledoc = read_xml_mem((const char *) xml, len);
	style = xsltParseStylesheetDoc(styledoc);

	src = xmlCopyDoc(doc, 1);
	res = xsltApplyStylesheet(style, src, params);
	xmlFreeDoc(src);

	old = xmlDocSetRootElement(doc, xmlCopyNode(xmlDocGetRootElement(res), 1));
	xmlFreeNode(old);

	xmlFreeDoc(res);
	xsltFreeStylesheet(style);
}

/* Add a node containing the path of an object to the report. */
xmlNodePtr add_object_node(xmlNodePtr parent, const char *name, const char *path)
{
	xmlNodePtr node;
	node = xmlNewChild(parent, NULL, BAD_CAST name, NULL);
	xmlSetProp(node, BAD_CAST "path", BAD_CAST path);
	return node;
}

int check_props_only(xmlDocPtr doc, const char *path, struct appcheckopts *opts, xmlNodePtr report)
{
	char actfname[PATH_MAX];
	char cctfname[PATH_MAX];
	xmlDocPtr act = NULL;
	xmlDocPtr cct = NULL;
	int err;

	if (find_act_fname(actfname, opts->useract, doc)) {
		if ((act = read_xml_doc(actfname))) {
			add_object_node(report, "act", actfname);
		}
	}

	if (find_cct_fname(cctfname, opts->usercct, act)) {
		if ((cct = read_xml_doc(cctfname))) {
			add_object_node(report, "cct", cctfname);
			add_cct_depends(doc, cct, NULL);
		}
	}

	err = check_props_against_cts(doc, path, act, cct, report);

	xmlFreeDoc(cct);
	xmlFreeDoc(act);

	return err;
}

/* Check that an object is valid for all defined product instances. */
int check_prods(xmlDocPtr doc, const char *path, xmlDocPtr all, xmlDocPtr act, struct appcheckopts *opts, xmlNodePtr report)
{
	char pctfname[PATH_MAX];
	int err = 0;
	xmlDocPtr pct;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	if (all) {
		pct = all;
	} else {
		if (!find_pct_fname(pctfname, opts->userpct, act)) {
			return 0;
		}

		pct = read_xml_doc(pctfname);

		add_object_node(report, "pct", pctfname);
	}

	ctx = xmlXPathNewContext(pct);
	obj = xmlXPathEvalExpression(BAD_CAST "//product", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlNodePtr asserts;
			xmlChar *id;

			asserts = xmlNewNode(NULL, BAD_CAST "asserts");

			if (all) {
				id = NULL;
			} else {
				xmlChar line_s[16], *xpath;

				id = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "id");

				if (id) {
					xmlSetProp(asserts, BAD_CAST "id", id);
				}

				xmlStrPrintf(line_s, 16, "%ld", xmlGetLineNo(obj->nodesetval->nodeTab[i]));
				xmlSetProp(asserts, BAD_CAST "line", line_s);

				xpath = xpath_of(obj->nodesetval->nodeTab[i]);
				xmlSetProp(asserts, BAD_CAST "xpath", xpath);
				xmlFree(xpath);
			}

			extract_assigns(asserts, obj->nodesetval->nodeTab[i]);

			err += check_assigns(doc, path, asserts, obj->nodesetval->nodeTab[i], id, pctfname, opts);

			xmlAddChild(report, xmlCopyNode(asserts, 1));

			xmlFreeNode(asserts);

			xmlFree(id);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	if (!all) {
		xmlFreeDoc(pct);
	}

	return err;
}

/* Add an assertion from an object to a set of assertions. */
void add_assert(xmlNodePtr asserts, xmlNodePtr assert)
{
	xmlChar *i, *t, *v, *c = NULL;

	i = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyIdent|@actidref");
	t = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyType|@actreftype");
	v = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyValues|@actvalues");

	while ((c = BAD_CAST strtok(c ? NULL : (char *) v, "|~"))) {
		xmlNodePtr cur;
		bool exists = false;

		for (cur = asserts->children; cur && !exists; cur = cur->next) {
			xmlChar *ci, *ct, *cv;

			ci = first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyIdent");
			ct = first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyType");
			cv = first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyValue");

			exists = xmlStrcmp(i, ci) == 0 && xmlStrcmp(t, ct) == 0 && xmlStrcmp(c, cv) == 0;

			xmlFree(ci);
			xmlFree(ct);
			xmlFree(cv);
		}

		if (!exists) {
			xmlNodePtr new;

			new = xmlNewNode(NULL, BAD_CAST "assign");
			xmlSetProp(new, BAD_CAST "applicPropertyIdent", i);
			xmlSetProp(new, BAD_CAST "applicPropertyType", t);
			xmlSetProp(new, BAD_CAST "applicPropertyValue", c);

			xmlAddChild(asserts, new);
		}
	}

	xmlFree(i);
	xmlFree(t);
	xmlFree(v);
}

/* Find a property in a set of properties. */
xmlNodePtr set_has_prop(xmlNodePtr set, const xmlChar *name)
{
	xmlNodePtr cur, assert = NULL;

	for (cur = set->children; cur && !assert; cur = cur->next) {
		xmlChar *i;

		i = first_xpath_value(NULL, cur->children, BAD_CAST "@applicPropertyIdent|@actidref");

		if (xmlStrcmp(i, name) == 0) {
			assert = cur;
		}

		xmlFree(i);
	}

	return assert;
}

/* Check the applicability within an object without using an ACT, CCT or PCT. */
int check_object_props(xmlDocPtr doc, const char *path, struct appcheckopts *opts, xmlNodePtr report)
{
	xmlDocPtr psdoc;
	xmlNodePtr propsets;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	psdoc = xmlNewDoc(BAD_CAST "1.0");
	propsets = xmlNewNode(NULL, BAD_CAST "propsets");
	xmlDocSetRootElement(psdoc, propsets);

	/* Add CCT dependencies so they are counted as part of the object's applicability. */
	if (opts->add_deps || opts->check_props) {
		char actfname[PATH_MAX];
		char cctfname[PATH_MAX];
		xmlDocPtr act = NULL;
		xmlDocPtr cct = NULL;

		if (find_act_fname(actfname, opts->useract, doc)) {
			if ((act = read_xml_doc(actfname))) {
				add_object_node(report, "act", actfname);
			}
		}

		if (find_cct_fname(cctfname, opts->usercct, act)) {
			if ((cct = read_xml_doc(cctfname))) {
				add_object_node(report, "cct", cctfname);

				if (opts->add_deps) {
					add_cct_depends(doc, cct, NULL);
				}
			}
		}

		if (opts->check_props) {
			err += check_props_against_cts(doc, path, act, cct, report);
		}

		xmlFreeDoc(cct);
		xmlFreeDoc(act);
	}

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST "//assert", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlNodePtr asserts;
			xmlChar *name;

			name = first_xpath_value(doc, obj->nodesetval->nodeTab[i], BAD_CAST "@applicPropertyIdent|@actidref");

			if (!(asserts = set_has_prop(propsets, name))) {
				asserts = xmlNewChild(propsets, NULL, BAD_CAST "asserts", NULL);
			}

			xmlFree(name);

			add_assert(asserts, obj->nodesetval->nodeTab[i]);
		}
	}

	transform_doc(psdoc, combos_xsl, combos_xsl_len, NULL);
	err += check_prods(doc, path, psdoc, NULL, opts, report);

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	xmlFreeDoc(psdoc);

	return err;
}

/* Determine whether a property is used in an object. */
bool prop_is_used(const xmlChar *id, xmlDocPtr doc)
{
	xmlChar *xpath;
	int n;
	xmlNodePtr node;

	n = xmlStrlen(id) * 2 + 50;
	xpath = malloc(n * sizeof(xmlChar));
	xmlStrPrintf(xpath, n, "//assert[@applicPropertyIdent='%s' or @actidref='%s']", id, id);
	node = first_xpath_node(doc, NULL, xpath);
	xmlFree(xpath);

	return node != NULL;
}

/* Determine whether an object uses any conditions. */
bool has_conds(xmlDocPtr doc)
{
	return first_xpath_node(doc, NULL, BAD_CAST "//assert[@applicPropertyType='condition' or @actreftype='condition']");
}

/* Check all possible combinations of applicability property values which may
 * affect the object.
 */
int check_all_props(xmlDocPtr doc, const char *path, xmlDocPtr act, struct appcheckopts *opts, xmlNodePtr report)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;
	char cctfname[PATH_MAX];
	xmlDocPtr cct = NULL;
	int i;
	xmlDocPtr psdoc;
	xmlNodePtr propsets;

	psdoc = xmlNewDoc(BAD_CAST "1.0");
	propsets = xmlNewNode(NULL, BAD_CAST "propsets");
	xmlDocSetRootElement(psdoc, propsets);

	if (find_cct_fname(cctfname, opts->usercct, act)) {
		if ((cct = read_xml_doc(cctfname))) {
			add_object_node(report, "cct", cctfname);

			if (opts->add_deps) {
				add_cct_depends(doc, cct, NULL);
			}
		}
	}

	if (opts->check_props) {
		err += check_props_against_cts(doc, path, act, cct, report);
	}

	ctx = xmlXPathNewContext(act);
	obj = xmlXPathEvalExpression(BAD_CAST "//productAttribute|//prodattr", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *id;
			xmlNodePtr asserts;

			id = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "id");

			if (prop_is_used(id, doc)) {
				asserts = xmlNewNode(NULL, BAD_CAST "asserts");
				extract_enumvals(asserts, obj->nodesetval->nodeTab[i], id, false);
				xmlAddChild(propsets, asserts);
			}

			xmlFree(id);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	if (cct) {
		ctx = xmlXPathNewContext(cct);
		obj = xmlXPathEvalExpression(BAD_CAST "//cond|//condition", ctx);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				xmlChar *id;
				xmlNodePtr type;
				xmlNodePtr asserts;

				id = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "id");

				if (prop_is_used(id, doc)) {
					xmlChar *typerefid;
					xmlChar *xpath;
					int n;

					asserts = xmlNewNode(NULL, BAD_CAST "asserts");

					typerefid = first_xpath_value(cct,
						obj->nodesetval->nodeTab[i],
						BAD_CAST "@condTypeRefId|@condtyperef");

					n = xmlStrlen(typerefid) + 12;
					xpath = malloc(n * sizeof(xmlChar));
					xmlStrPrintf(xpath, n, "//*[@id='%s']", typerefid);
					xmlFree(typerefid);
					type = first_xpath_node(cct, NULL, xpath);
					xmlFree(xpath);

					extract_enumvals(asserts, type, id, true);

					xmlAddChild(propsets, asserts);
				}

				xmlFree(id);
			}
		}

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);
		xmlFreeDoc(cct);
	} else if (has_conds(doc)) {
		fprintf(stderr, E_NO_CCT, path);
		xmlNewChild(report, NULL, BAD_CAST "cctNotFound", NULL);
		++err;
	}

	if (!err) {
		transform_doc(psdoc, combos_xsl, combos_xsl_len, NULL);
		err += check_prods(doc, path, psdoc, NULL, opts, report);
	}

	xmlFreeDoc(psdoc);

	return err;
}

/* Check product instances read from the PCT. */
int check_pct_instances(xmlDocPtr doc, const char *path, xmlDocPtr act, struct appcheckopts *opts, xmlNodePtr report)
{
	int err = 0;

	/* Add CCT dependencies. */
	if (opts->add_deps || opts->check_props) {
		char cctfname[PATH_MAX];
		xmlDocPtr cct = NULL;

		/* The ACT may or may not have already been read. */
		if (act) {
			if (find_cct_fname(cctfname, opts->usercct, act)) {
				if ((cct = read_xml_doc(cctfname))) {
					add_object_node(report, "cct", cctfname);

					if (opts->add_deps) {
						add_cct_depends(doc, cct, NULL);
					}
				}
			}

			if (opts->check_props) {
				err += check_props_against_cts(doc, path, act, cct, report);
			}

			xmlFreeDoc(cct);
		} else {
			char actfname[PATH_MAX];

			if (find_act_fname(actfname, opts->useract, doc)) {
				if ((act = read_xml_doc(actfname))) {
					add_object_node(report, "act", cctfname);
				}
			}

			if (find_cct_fname(cctfname, opts->usercct, act)) {
				if ((cct = read_xml_doc(cctfname))) {
					add_object_node(report, "cct", cctfname);

					if (opts->add_deps) {
						add_cct_depends(doc, cct, NULL);
					}
				}
			}

			if (opts->check_props) {
				err += check_props_against_cts(doc, path, act, cct, report);
			}

			xmlFreeDoc(cct);
			xmlFreeDoc(act);
			act = NULL;
		}
	}

	err += check_prods(doc, path, NULL, act, opts, report);

	return err;
}

/* Determine whether an object uses any computable applicability. */
bool has_applic(xmlDocPtr doc)
{
	return first_xpath_node(doc, NULL, BAD_CAST "//assert") != NULL;
}

/* Check the applicability in an object. */
int check_applic_file(const char *path, struct appcheckopts *opts, xmlNodePtr report)
{
	xmlDocPtr doc;
	int err = 0;
	char actfname[PATH_MAX];
	xmlNodePtr report_node = NULL;

	if (!(doc = read_xml_doc(path))) {
		if (verbosity > QUIET) {
			fprintf(stderr, E_BAD_OBJECT, path);
		}
		exit(EXIT_BAD_OBJECT);
	}

	if (report) {
		report_node = add_object_node(report, "object", path);
	}

	/* Add the type of check to the report. */
	switch (opts->mode) {
		case BASIC:
			xmlSetProp(report, BAD_CAST "type", BAD_CAST "basic");
			break;
		case PCT:
			xmlSetProp(report, BAD_CAST "type", BAD_CAST "pct");
			break;
		case ALL:
			xmlSetProp(report, BAD_CAST "type", BAD_CAST "all");
			break;
		case STANDALONE:
			xmlSetProp(report, BAD_CAST "type", BAD_CAST "standalone");
			break;
	}

	if (opts->check_props) {
		xmlSetProp(report, BAD_CAST "strict", BAD_CAST "yes");
	} else {
		xmlSetProp(report, BAD_CAST "strict", BAD_CAST "no");
	}

	if (opts->mode == BASIC) {
		err += check_props_only(doc, path, opts, report_node);
	} else if (opts->mode == STANDALONE) {
		err += check_object_props(doc, path, opts, report_node);
	} else if (opts->mode == PCT && opts->userpct) {
		err += check_pct_instances(doc, path, NULL, opts, report_node);
	} else if (find_act_fname(actfname, opts->useract, doc)) {
		xmlDocPtr act;

		add_object_node(report_node, "act", actfname);

		act = read_xml_doc(actfname);

		if (opts->mode == ALL) {
			err += check_all_props(doc, path, act, opts, report_node);
		} else {
			err += check_pct_instances(doc, path, act, opts, report_node);
		}

		xmlFreeDoc(act);
	} else if (has_applic(doc)) {
		fprintf(stderr, E_NO_ACT, path);
		xmlNewChild(report_node, NULL, BAD_CAST "actNotFound", NULL);
		++err;
	}

	if (err) {
		xmlSetProp(report_node, BAD_CAST "valid", BAD_CAST "no");

		if (opts->filenames) {
			puts(path);
		}
	} else {
		xmlSetProp(report_node, BAD_CAST "valid", BAD_CAST "yes");

		if (opts->output_tree) {
			save_xml_doc(doc, "-");
		}
	}

	if (verbosity >= VERBOSE) {
		fprintf(stderr, err ? F_INVALID : S_VALID, path);
	}

	xmlFreeDoc(doc);

	return err;
}

/* Check the applicability in a list of objects. */
int check_applic_list(const char *fname, struct appcheckopts *opts, xmlNodePtr report)
{
	FILE *f;
	char path[PATH_MAX];
	int err = 0;

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			if (verbosity >= NORMAL) {
				fprintf(stderr, E_BAD_LIST, fname);
			}
			return err;
		}
	} else {
		f = stdin;
	}

	while (fgets(path, PATH_MAX, f)) {
		strtok(path, "\t\r\n");
		err += check_applic_file(path, opts, report);
	}

	if (fname) {
		fclose(f);
	}

	return err;
}

/* Show a summary of the check. */
void print_stats(xmlDocPtr doc)
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

int main(int argc, char **argv)
{
	int i;

	const char *sopts = "A:abC:cd:e:fNk:loP:pqrsTvx~h?";
	struct option lopts[] = {
		{"version"     , no_argument      , 0, 0},
		{"help"        , no_argument      , 0, 'h'},
		{"act"         , required_argument, 0, 'A'},
		{"all"         , no_argument      , 0, 'a'},
		{"brexcheck"   , no_argument      , 0, 'b'},
		{"cct"         , required_argument, 0, 'C'},
		{"basic"       , no_argument      , 0, 'c'},
		{"dir"         , required_argument, 0, 'd'},
		{"exec"        , required_argument, 0, 'e'},
		{"filenames"   , no_argument      , 0, 'f'},
		{"args"        , required_argument, 0, 'k'},
		{"list"        , no_argument      , 0, 'l'},
		{"omit-issue"  , no_argument      , 0, 'N'},
		{"output-valid", no_argument      , 0, 'o'},
		{"pct"         , required_argument, 0, 'P'},
		{"products"    , no_argument      , 0, 'p'},
		{"quiet"       , no_argument      , 0, 'q'},
		{"recursive"   , no_argument      , 0, 'r'},
		{"strict"      , no_argument      , 0, 's'},
		{"summary"     , no_argument      , 0, 'T'},
		{"verbose"     , no_argument      , 0, 'v'},
		{"xml"         , no_argument      , 0, 'x'},
		{"dependencies", no_argument      , 0, '~'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	bool islist = false;
	bool xmlout = false;
	bool show_stats = false;

	struct appcheckopts opts = {
		/* useract */     NULL,
		/* usercct */     NULL,
		/* userpct */     NULL,
		/* args */        NULL,
		/* validators */  NULL,
		/* output_tree */ false,
		/* filenames */   false,
		/* brexcheck */   false,
		/* add_deps */    false,
		/* check_props */ false,
		/* mode */        STANDALONE
	};

	int err = 0;

	xmlDocPtr report;
	xmlNodePtr appcheck;

	search_dir = strdup(".");

	report = xmlNewDoc(BAD_CAST "1.0");
	appcheck = xmlNewNode(NULL, BAD_CAST "appCheck");
	xmlDocSetRootElement(report, appcheck);

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					goto cleanup;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'A':
				opts.useract = strdup(optarg);
				break;
			case 'a':
				opts.mode = ALL;
				break;
			case 'b':
				opts.brexcheck = true;
				break;
			case 'C':
				opts.usercct = strdup(optarg);
				break;
			case 'c':
				opts.mode = BASIC;
				break;
			case 'd':
				free(search_dir);
				search_dir = strdup(optarg);
				break;
			case 'e':
				if (!opts.validators) {
					opts.validators = xmlNewNode(NULL, BAD_CAST "validators");
				}
				xmlNewChild(opts.validators, NULL, BAD_CAST "cmd", BAD_CAST optarg);
				break;
			case 'f':
				opts.filenames = true;
				break;
			case 'k':
				opts.args = strdup(optarg);
				break;
			case 'l':
				islist = true;
				break;
			case 'N':
				no_issue = true;
				break;
			case 'o':
				opts.output_tree = true;
				break;
			case 'P':
				opts.userpct = strdup(optarg);
				break;
			case 'p':
				opts.mode = PCT;
				break;
			case 'r':
				recursive_search = true;
				break;
			case 's':
				opts.check_props = true;
				break;
			case 'T':
				show_stats = true;
				break;
			case 'q':
				--verbosity;
				break;
			case 'v':
				++verbosity;
				break;
			case 'x':
				xmlout = true;
				break;
			case '~':
				opts.add_deps = true;
				break;
			case 'h':
			case '?':
				show_help();
				goto cleanup;
		}
	}

	if (opts.mode == BASIC) {
		opts.check_props = true;
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				err += check_applic_list(argv[i], &opts, appcheck);
			} else {
				err += check_applic_file(argv[i], &opts, appcheck);
			}
		}
	} else if (islist) {
		err += check_applic_list(NULL, &opts, appcheck);
	} else {
		err += check_applic_file("-", &opts, appcheck);
	}

	if (xmlout) {
		save_xml_doc(report, "-");
	}

	if (show_stats) {
		print_stats(report);
	}

cleanup:
	xmlFreeDoc(report);
	free(opts.userpct);
	free(opts.useract);
	free(opts.usercct);
	free(opts.args);
	xmlFreeNode(opts.validators);
	free(search_dir);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}
