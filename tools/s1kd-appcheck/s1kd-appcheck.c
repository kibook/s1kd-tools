#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <paths.h>
#include <fcntl.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/c14n.h>
#include <libxml/hash.h>
#include <libxml/xmlsave.h>
#include <libxslt/transform.h>
#include <libexslt/exslt.h>
#include "s1kd_tools.h"
#include "stylesheets.h"

/* Program name and version information. */
#define PROG_NAME "s1kd-appcheck"
#define VERSION "6.8.0"

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
#define I_NESTEDCHECK INF_PREFIX "Checking nested applicability in %s...\n"
#define I_PROPCHECK INF_PREFIX "Checking product attribute and condition definitions in %s...\n"
#define I_NUM_PRODS INF_PREFIX "Checking %s for %d configurations...\n"
#define I_THREADS INF_PREFIX "Creating %d threads...\n"
#define I_EXEC INF_PREFIX "Executing: %s\n"

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
#define E_NESTEDCHECK ERR_PREFIX "%s: %s on line %ld is applicable when %s %s = %s, which is not a subset of the applicability of the parent %s on line %ld\n"
#define E_NESTEDCHECK_WHOLE ERR_PREFIX "%s: %s on line %ld is applicable when %s %s = %s, which is not a subset of the applicability of the whole object.\n"
#define E_NESTEDCHECK_REDUNDANT ERR_PREFIX "%s: %s on line %ld has the same applicability as its parent %s on line %ld (%s)\n"
#define E_DUPLICATECHECK ERR_PREFIX "%s: Annotation on line %ld is a duplicate of annotation on line %ld.\n"
#define E_MAX_OBJECTS ERR_PREFIX "Out of memory\n"
#define E_PIPE ERR_PREFIX "Error executing %s: %s\n"

/* Warning messages. */
#define W_MISSING_REF_DM WRN_PREFIX "Could not read referenced object: %s\n"

/* Success messages. */
#define S_VALID SUC_PREFIX "%s passed the applicability check.\n"

/* Failure messages. */
#define F_INVALID FLD_PREFIX "%s failed the applicability check.\n"

/* Exit status codes. */
#define EXIT_BAD_OBJECT 2
#define EXIT_MAX_OBJECTS 3
#define EXIT_PIPE 4

/* Default commands used to filter and validate. */
#define DEFAULT_FILTER "s1kd-instance"
#define DEFAULT_VALIDATE "s1kd-validate"
#define DEFAULT_BREXCHECK "s1kd-brexcheck"

/* Namespace for special elements/attributes. */
#define S1KD_APPCHECK_NS BAD_CAST "urn:s1kd-tools:s1kd-appcheck"

/* Progress bar formats. */
#define PROGRESS_OFF 0
#define PROGRESS_CLI 1
#define PROGRESS_ZENITY 2

/* Initial maximum number of CSDB object paths. */
static int OBJECT_MAX = 1;
/* List of CSDB object paths. */
static char (*objects)[PATH_MAX];
static int nobjects = 0;

/* Search for ACT, CCT, PCT recursively. */
static bool recursive_search = false;

/* Assume issue/inwork numbers are omitted. */
static bool no_issue = false;

/* Directory to search for ACT, CCT, PCT in. */
static char *search_dir;

/* The verbosity of output. */
static enum verbosity { QUIET, NORMAL, VERBOSE, DEBUG } verbosity = NORMAL;

/* What type of check to perform. */
enum appcheckmode { CUSTOM, PCT, ALL, STANDALONE };

/* Which filenames to print. */
enum show_filenames { SHOW_NONE, SHOW_INVALID, SHOW_VALID };

/* Applicability properties to ignore when validating objects. */
xmlHashTablePtr ignored_properties = NULL;

/* Number of simultaneous threads to run in parallel when iterating over the object list. */
static int object_threads = 1;

/* Number of simultaneous threads to run in parallel when checking product instances. */
static int check_prod_threads = 1;

/* Applicability check options. */
struct appcheckopts {
	char *useract;
	char *usercct;
	char *userpct;
	char *filter;
	char *args;
	xmlNodePtr validators;
	bool output_tree;
	enum show_filenames filenames;
	bool brexcheck;
	bool add_deps;
	bool check_props;
	bool check_nested;
	bool strict_nested;
	bool check_redundant;
	bool check_duplicate;
	bool rem_delete;
	enum appcheckmode mode;
	bool include_errors;
};

/* Struct to pass data to object_thread. */
struct object_thread_args {
	struct appcheckopts *opts;
	int show_progress;
	int job;
};

/* Struct to pass the results of an object_thread back to the main thread. */
struct object_thread_result {
	int err;
	xmlNodePtr report;
};

/* The number of objects checked so far when using multiple threads. */
pthread_mutex_t object_thread_progress_mutex;
int object_thread_progress = 0;

/* Struct to pass data to a check_prod thread. */
struct check_prod_thread_args {
	xmlDocPtr doc;
	const char *path;
	xmlDocPtr all;
	const char *pctfname;
	xmlNodeSetPtr nodes;
	struct appcheckopts *opts;
	int job;
};

/* Struct to pass the results of check_prod back to the main thread. */
struct check_prod_thread_result {
	int err;
	xmlNodePtr report;
};

/* Show usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -A, --act <file>        User-specified ACT.");
	puts("  -a, --all               Validate against all property values.");
	puts("  -b, --brexcheck         Validate against BREX.");
	puts("  -C, --cct <file>        User-specified CCT.");
	puts("  -c, --custom            Perform a customized check.");
	puts("  -D, --duplicate         Check for duplicate applicability annotations.");
	puts("  -d, --dir <dir>         Search for ACT/CCT/PCT in <dir>.");
	puts("  -e, --exec <cmd>        Commands used to validate objects.");
	puts("  -F, --valid-filenames   List valid files.");
	puts("  -f, --filenames         List invalid files.");
	puts("  -h, -?, --help          Show help/usage message.");
	puts("  -i, --ignore <id:type>  Ignore an applicability property when validating.");
	puts("  -K, --filter <cmd>      Command used to create objects.");
	puts("  -k, --args <args>       Arguments used to create objects.");
	puts("  -l, --list              Treat input as list of CSDB objects.");
	puts("  -N, --omit-issue        Assume issue/inwork numbers are omitted.");
	puts("  -n, --nested            Check nested applicability annotations.");
	puts("  -o, --output-valid      Output valid CSDB objects to stdout.");
	puts("  -P, --pct <file>        User-specified PCT.");
	puts("  -p, --progress          Display a progress bar.");
	puts("  -q, --quiet             Quiet mode.");
	puts("  -R, --redundant         Check for redundant applicability annotations.");
	puts("  -r, --recursive         Search for ACT/CCT/PCT recursively.");
	puts("  -s, --strict            Check that all properties are defined.");
	puts("  -T, --summary           Print a summary of the check.");
	puts("  -t, --products          Validate against product instances.");
	puts("  -u, --unstrict-nested   Perform a nested check in unstrict mode.");
	puts("  -v, --verbose           Verbose output.");
	puts("  -x, --xml               Output an XML report.");
	puts("  -~, --dependencies      Check CCT dependencies.");
	puts("  -^, --remove-deleted    Validate with elements marked as \"delete\" removed.");
	puts("  -#, --threads <x[,y]>   Number of threads to use. x * y threads are created in total.");
	puts("  --version               Show version information.");
	puts("  --zenity-progress       Print progress information in the zenity --progress format.");
	puts("  <object>...             CSDB object(s) to check.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Show version information. */
static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

/* Return the first node matching an XPath expression. */
static xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const xmlChar *path)
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
static xmlChar *first_xpath_value(xmlDocPtr doc, xmlNodePtr node, const xmlChar *path)
{
	return xmlNodeGetContent(first_xpath_node(doc, node, path));
}

/* Find a data module filename in the current directory based on the dmRefIdent
 * element. */
static bool find_dmod_fname(char *dst, xmlNodePtr dmRefIdent)
{
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

	if (!no_issue) {
		if (issueInfo) {
			char *issue_number;
			char *in_work;
			char iss[8];

			issue_number = (char *) first_xpath_value(NULL, issueInfo, BAD_CAST "@issno|@issueNumber");
			in_work      = (char *) first_xpath_value(NULL, issueInfo, BAD_CAST "@inwork|@inWork");

			snprintf(iss, 8, "_%s-%s", issue_number, in_work ? in_work : "00");
			strcat(code, iss);

			xmlFree(issue_number);
			xmlFree(in_work);
		} else if (language) {
			strcat(code, "_\?\?\?-\?\?");
		}
	}

	if (language) {
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

	/* Look for DM in the directory hierarchy. */
	if (find_csdb_object(dst, search_dir, code, is_dm, recursive_search)) {
		return true;
	}

	/* Look for DM in the list of objects to check. */
	if (find_csdb_object_in_list(dst, objects, nobjects, code)) {
		return true;
	}

	fprintf(stderr, W_MISSING_REF_DM, code);
	return false;
}

/* Find the filename of a referenced ACT data module. */
static bool find_act_fname(char *dst, const char *useract, xmlDocPtr doc)
{
	if (useract) {
		strcpy(dst, useract);
		return true;
	} else if (doc) {
		xmlNodePtr actref;
		actref = first_xpath_node(doc, NULL, BAD_CAST "//applicCrossRefTableRef/dmRef/dmRefIdent|//actref/refdm");
		return actref && find_dmod_fname(dst, actref);
	}

	return false;
}

/* Find the filename of a referenced CCT data module. */
static bool find_cct_fname(char *dst, const char *usercct, xmlDocPtr act)
{
	if (usercct) {
		strcpy(dst, usercct);
		return true;
	} else if (act) {
		xmlNodePtr cctref;
		cctref = first_xpath_node(act, NULL, BAD_CAST "//condCrossRefTableRef/dmRef/dmRefIdent|//cctref/refdm");
		return cctref && find_dmod_fname(dst, cctref);
	}

	return false;
}

/* Find the filename of a referenced PCT data module via the ACT. */
static bool find_pct_fname(char *dst, const char *userpct, xmlDocPtr act)
{
	if (userpct) {
		strcpy(dst, userpct);
		return true;
	} else if (act) {
		xmlNodePtr pctref;
		pctref = first_xpath_node(act, NULL, BAD_CAST "//productCrossRefTableRef/dmRef/dmRefIdent|//pctref/refdm");
		return pctref && find_dmod_fname(dst, pctref);
	}

	return false;
}

/* Add a nested applic error to the report. */
static xmlNodePtr add_nested_error(xmlNodePtr report, xmlNodePtr node, xmlNodePtr parent, const xmlChar *id, const xmlChar *type, const xmlChar *val, const char *path)
{
	xmlNodePtr und;
	long int cline, pline;
	xmlChar line_s[16], *xpath;

	cline = xmlGetLineNo(node);
	pline = xmlGetLineNo(parent);

	if (verbosity >= NORMAL) {
		if (parent) {
			fprintf(stderr, E_NESTEDCHECK,
				path,
				(char *) node->name,
				cline,
				(char *) type,
				(char *) id,
				(char *) val,
				(char *) parent->name,
				pline);
		} else {
			fprintf(stderr, E_NESTEDCHECK_WHOLE,
				path,
				(char *) node->name,
				cline,
				(char *) type,
				(char *) id,
				(char *) val);
		}
	}

	und = xmlNewChild(report, NULL, BAD_CAST "nestedApplicError", NULL);

	xmlSetProp(und, BAD_CAST "applicPropertyIdent", id);
	xmlSetProp(und, BAD_CAST "applicPropertyType", type);
	xmlSetProp(und, BAD_CAST "applicPropertyValue", val);

	xmlStrPrintf(line_s, 16, "%ld", cline);
	xmlSetProp(und, BAD_CAST "line", line_s);

	xpath = xpath_of(node);
	xmlSetProp(und, BAD_CAST "xpath", xpath);
	xmlFree(xpath);

	if (parent) {
		xmlStrPrintf(line_s, 16, "%ld", pline);
		xmlSetProp(und, BAD_CAST "parentLine", line_s);

		xpath = xpath_of(parent);
		xmlSetProp(und, BAD_CAST "parentXpath", xpath);
		xmlFree(xpath);
	}

	return und;
}

/* Add a redundant applicability error to the report. */
static xmlNodePtr add_redundant_error(xmlNodePtr report, xmlNodePtr node, xmlNodePtr parent, const xmlChar *id, const char *path)
{
	xmlNodePtr und;
	long int cline, pline;
	xmlChar line_s[16], *xpath;

	cline = xmlGetLineNo(node);
	pline = xmlGetLineNo(parent);

	if (verbosity >= NORMAL) {
		fprintf(stderr, E_NESTEDCHECK_REDUNDANT,
			path,
			(char *) node->name,
			cline,
			(char *) parent->name,
			pline,
			(char *) id);
	}

	und = xmlNewChild(report, NULL, BAD_CAST "redundantApplicError", NULL);

	xmlStrPrintf(line_s, 16, "%ld", cline);
	xmlSetProp(und, BAD_CAST "line", line_s);

	xpath = xpath_of(node);
	xmlSetProp(und, BAD_CAST "xpath", xpath);
	xmlFree(xpath);

	if (parent) {
		xmlStrPrintf(line_s, 16, "%ld", pline);
		xmlSetProp(und, BAD_CAST "parentLine", line_s);

		xpath = xpath_of(parent);
		xmlSetProp(und, BAD_CAST "parentXpath", xpath);
		xmlFree(xpath);
	}

	return und;
}

/* Define a value for a product attribute or condition. */
static void define_applic(xmlNodePtr defs, const xmlChar *ident, const xmlChar *type, const xmlChar *value)
{
	xmlNodePtr assert = NULL;
	xmlNodePtr cur;

	if (!(ident && type && value)) {
		return;
	}

	/* Check if an assert has already been created for this property. */
	for (cur = defs->children; cur; cur = cur->next) {
		xmlChar *cur_ident = xmlGetProp(cur, BAD_CAST "applicPropertyIdent");
		xmlChar *cur_type  = xmlGetProp(cur, BAD_CAST "applicPropertyType");

		if (xmlStrcmp(cur_ident, ident) == 0 && xmlStrcmp(cur_type, type) == 0) {
			assert = cur;
		}

		xmlFree(cur_ident);
		xmlFree(cur_type);
	}

	/* If no assert exists, add a new one. */
	if (!assert) {
		assert = xmlNewChild(defs, NULL, BAD_CAST "assert", NULL);
		xmlSetProp(assert, BAD_CAST "applicPropertyIdent", ident);
		xmlSetProp(assert, BAD_CAST "applicPropertyType",  type);
		xmlSetProp(assert, BAD_CAST "applicPropertyValues", value);
	/* Or, if an assert already exists... */
	} else {
		/* Check for duplicate value in a single-assert. */
		if (xmlHasProp(assert, BAD_CAST "applicPropertyValues")) {
			xmlChar *first_value;

			first_value = xmlGetProp(assert, BAD_CAST "applicPropertyValues");

			/* If the value is not a duplicate, and the assertion
			 * may be modified, convert to a multi-assert and add
			 * the original and new values. */
			if (xmlStrcmp(first_value, BAD_CAST value) != 0) {
				xmlNewChild(assert, NULL, BAD_CAST "value", first_value);
				xmlNewChild(assert, NULL, BAD_CAST "value", value);
				xmlUnsetProp(assert, BAD_CAST "applicPropertyValues");
			}

			xmlFree(first_value);
		/* Check for duplicate value in a multi-assert. */
		} else {
			bool dup = false;

			for (cur = assert->children; cur && !dup; cur = cur->next) {
				xmlChar *cur_value;
				cur_value = xmlNodeGetContent(cur);
				dup = xmlStrcmp(cur_value, value) == 0;
				xmlFree(cur_value);
			}

			/* If the value is not a duplicate, and the assertion
			 * may be modified, add the new value to the
			 * multi-assert. */
			if (!dup) {
				xmlNewChild(assert, NULL, BAD_CAST "value", value);
			}
		}
	}
}

/* Check that an assertion in a nested applicability annotation is a subset of
 * its parent.
 */
static int check_nested_applic_assert(xmlNodePtr node, xmlNodePtr parent, xmlNodePtr assert, xmlNodePtr parent_app_node, const char *path, xmlNodePtr report)
{
	xmlNodePtr defs;
	int err;

	defs = xmlNewNode(NULL, BAD_CAST "applic");

	xmlChar *id   = xpath_first_value(NULL, assert, BAD_CAST "@applicPropertyIdent|@actidref");
	xmlChar *type = xpath_first_value(NULL, assert, BAD_CAST "@applicPropertyType|@actreftype");
	xmlChar *vals = xpath_first_value(NULL, assert, BAD_CAST "@applicPropertyValues|@actvalues");

	define_applic(defs, id, type, vals);

	err = !eval_applic(defs, parent_app_node, true);

	xmlFreeNode(defs);

	if (err) {
		add_nested_error(report, node, parent, id, type, vals, path);
	}

	xmlFree(id);
	xmlFree(type);
	xmlFree(vals);

	return err;
}

/* Check that an applicability annotation is a subset of a given parent annotation. */
static int check_nested_applic_props(xmlDocPtr doc, const char *path, xmlNodePtr node, xmlNodePtr parent, xmlNodePtr app, xmlNodePtr parent_app, struct appcheckopts *opts, xmlNodePtr report)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr parent_app_node;
	int err = 0;

	parent_app_node = xpath_first_node(parent_app->doc, parent_app, BAD_CAST "assert|evaluate");

	if (parent_app_node == NULL) {
		return 0;
	}

	ctx = xmlXPathNewContext(doc);
	ctx->node = app;

	obj = xmlXPathEvalExpression(BAD_CAST ".//assert", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		if (opts->strict_nested) {
			/* Check individual asserts regardless of whether the whole annotation is applicable to its parent. */
			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				err += check_nested_applic_assert(node, parent, obj->nodesetval->nodeTab[i], parent_app_node, path, report);
			}
		} else {
			/* Only check individual asserts if the whole annotation is not applicable to its parent. */
			xmlNodePtr defs = xmlNewNode(NULL, BAD_CAST "applic");

			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				xmlChar *id, *type, *vals;

				id   = xpath_first_value(NULL, obj->nodesetval->nodeTab[i], BAD_CAST "@applicPropertyIdent|@actidref");
				type = xpath_first_value(NULL, obj->nodesetval->nodeTab[i], BAD_CAST "@applicPropertyType|@actreftype");
				vals = xpath_first_value(NULL, obj->nodesetval->nodeTab[i], BAD_CAST "@applicPropertyValues|@actvalues");

				define_applic(defs, id, type, vals);

				xmlFree(id);
				xmlFree(type);
				xmlFree(vals);
			}

			if (!eval_applic(defs, parent_app_node, true)) {
				for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
					err += check_nested_applic_assert(node, parent, obj->nodesetval->nodeTab[i], parent_app_node, path, report);
				}
			}

			xmlFreeNode(defs);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return err;
}

/* Check for redundant applicability.
 *
 * FIXME: This is a very rudimentary implementation that only checks if the
 *        EXACT annotation is repeated on child elements. In the future, this
 *        should also check if logic between parent and child annotations is
 *        redundant.
 */
static int check_redundant_applic(const char *path, xmlNodePtr node, const xmlChar *id, xmlNodePtr parent, const xmlChar *parent_id, xmlNodePtr report)
{
	if (xmlStrcmp(id, parent_id) == 0) {
		add_redundant_error(report, node, parent, id, path);
		return 1;
	}

	return 0;
}

/* Check that an applicability annotation is a subset of any parent annotation. */
static int check_nested_applic(xmlDocPtr doc, xmlNodePtr node, const char *path, struct appcheckopts *opts, xmlNodePtr report)
{
	xmlNodePtr app, parent_app, parent;
	xmlChar *id, *xpath;
	int n, err = 0;

	/* Get annotation of the current node. */
	id = first_xpath_value(doc, node, BAD_CAST "@applicRefId|@refapplic");
	n = xmlStrlen(id) + 17;
	xpath = malloc(n * sizeof(xmlChar));
	xmlStrPrintf(xpath, n, "//applic[@id='%s']", id);
	app = first_xpath_node(doc, NULL, xpath);
	xmlFree(xpath);

	/* Check against the applicability of each parent. */
	parent = node->parent;
	while (parent && parent->type == XML_ELEMENT_NODE) {
		xmlChar *parent_id;

		parent_id = first_xpath_value(doc, parent, BAD_CAST "@applicRefId|@refapplic");

		if (parent_id != NULL) {
			/* Get annotation of the parent node. */
			n = xmlStrlen(parent_id) + 100;
			xpath = malloc(n * sizeof(xmlChar));
			xmlStrPrintf(xpath, n, "//applic[@id='%s']", parent_id);
			parent_app = first_xpath_node(doc, NULL, xpath);
			xmlFree(xpath);

			/* Check for incompatible annotations. */
			if (opts->check_nested && check_nested_applic_props(doc, path, node, parent, app, parent_app, opts, report) != 0) {
				err = 1;
			}

			/* Check for redundant annotations. */
			if (opts->check_redundant && check_redundant_applic(path, node, id, parent, parent_id, report) != 0) {
				err = 1;
			}
		}

		xmlFree(parent_id);

		parent = parent->parent;
	}

	xmlFree(id);

	/* Check against the whole object applicability. */
	if ((parent_app = first_xpath_node(doc, node, BAD_CAST "//applic"))) {
		/* Check for incompatible annotations. */
		if (opts->check_nested && check_nested_applic_props(doc, path, node, NULL, app, parent_app, opts, report) != 0) {
			err = 1;
		}
	}

	return err != 0;
}

/* Check that all applicability annotations are subsets of their parent annotations. */
static int check_nested_applics(xmlDocPtr doc, const char *path, struct appcheckopts *opts, xmlNodePtr report)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	if (verbosity >= DEBUG) {
		fprintf(stderr, I_NESTEDCHECK, path);
	}

	ctx = xmlXPathNewContext(doc);

	obj = xmlXPathEvalExpression(BAD_CAST "//*[@applicRefId or @refapplic]", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			err += check_nested_applic(doc, obj->nodesetval->nodeTab[i], path, opts, report);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return err;
}

/* Add an undefined property node to the report. */
static xmlNodePtr add_undef_node(xmlNodePtr report, xmlNodePtr assert, const xmlChar *id, const xmlChar *type, const xmlChar *val, long int line)
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
static int check_val_against_prop(xmlNodePtr assert, const xmlChar *id, const xmlChar *type, const xmlChar *val, xmlNodePtr prop, const char *path, xmlNodePtr report)
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
				char *saveptr = NULL;

				vals = first_xpath_value(NULL, obj->nodesetval->nodeTab[i],
					BAD_CAST "@applicPropertyValues|@actvalues");

				if (vals) {
					while ((v = BAD_CAST strtok_r(v ? NULL : (char *) vals, "|", &saveptr))) {
						if (is_in_range((char *) val, (char *) v)) {
							match = true;
							break;
						}
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
static int check_prop_against_ct(xmlNodePtr assert, xmlDocPtr act, xmlDocPtr cct, const char *path, xmlNodePtr report)
{
	xmlChar *id, *type, *vals;
	int err = 0;

	id = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyIdent|@actidref");
	type = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyType|@actreftype");
	vals = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyValues|@applicPropertyValue|@actvalues|@actvalue");

	if (id && type && vals) {
		xmlChar *xpath = NULL;
		int n;
		xmlNodePtr prop;

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
			char *saveptr = NULL;

			while ((v = BAD_CAST strtok_r(v ? NULL : (char *) vals, "|~", &saveptr))) {
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
	}

	xmlFree(id);
	xmlFree(type);
	xmlFree(vals);

	return err;
}

/* Check whether all properties in an object are defined in the ACT/CCT. */
static int check_props_against_cts(xmlDocPtr doc, const char *path, xmlDocPtr act, xmlDocPtr cct, xmlNodePtr report)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	if (verbosity >= DEBUG) {
		fprintf(stderr, I_PROPCHECK, path);
	}

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
static void add_assign(xmlNodePtr asserts, xmlNodePtr assert)
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
static void extract_assigns(xmlNodePtr asserts, xmlNodePtr product)
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

/* Send an XML document to an external command's stdin, and return the exit status code. */
static int execute_command_on_doc(xmlDocPtr doc, const char *cmd)
{
	FILE *p;
	p = popen(cmd, "w");
	if (p == NULL) {
		fprintf(stderr, E_PIPE, cmd, strerror(errno));
	}
	xmlDocDump(p, doc);
	return pclose(p);
}

/* Send an XML document to an external command's stdin, and read an XML document from the command's stdout. */
static int execute_command_on_doc_and_parse_output(xmlDocPtr doc, const char *cmd, xmlDocPtr *out)
{
	/* Pipe to write to the stdin of the command. */
	int stdin_pipe[2];

	/* Pipe to read the stdout from the command. */
	int stdout_pipe[2];

	/* The PID of the child process. */
	int pid;

	/* Create a pipe for stdin. Abort on error. */
	if (pipe2(stdin_pipe, O_CLOEXEC) < 0) {
		goto pipe_error;
	}

	/* Create a pipe for stdout. Abort on error. */
	if (pipe2(stdout_pipe, O_CLOEXEC) < 0) {
		goto pipe_error;
	}

	/* Create a child process to execute the command. */
	pid = fork();

	/* If there is an error forking, abort. */
	if (pid == -1) {
		goto pipe_error;
	}

	/* The child process executes the given command with stdin and stdout directed to the created pipes. */
	if (pid == 0) {
		/* Close the ends of the pipes we don't use in the child process. */
		if (close(stdin_pipe[1]) == -1 || close(stdout_pipe[0]) == -1) {
			goto child_pipe_error;
		}

		/* Direct stdin and stdout to the created pipes, and close the dup'd pipe ends. */
		if (stdin_pipe[0] != STDIN_FILENO) {
			if (dup2(stdin_pipe[0], STDIN_FILENO) == -1 || close(stdin_pipe[0]) == -1) {
				goto child_pipe_error;
			}
		}
		if (stdout_pipe[1] != STDOUT_FILENO) {
			if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1 || close(stdout_pipe[1]) == -1) {
				goto child_pipe_error;
			}
		}

		/* Print an info message with the command in DEBUG verbosity. */
		if (verbosity >= DEBUG) {
			fprintf(stderr, I_EXEC, cmd);
		}

		/* Replace the child process with the execution of the command. */
		execl(_PATH_BSHELL, "sh", "-c", cmd, (char *) NULL);

		/* If for some reason exec fails, abort. */
child_pipe_error:
		fprintf(stderr, E_PIPE, cmd, strerror(errno));
		_exit(EXIT_FAILURE);
	}

	/* Parent process continues here. */

	/* Close the ends of the pipes we don't use in the parent process. */
	if (close(stdin_pipe[0]) == -1 || close(stdout_pipe[1]) == -1) {
		goto pipe_error;
	}

	/* Write the XML document to the stdin of the child process. */
	xmlSaveCtxtPtr ctxt = xmlSaveToFd(stdin_pipe[1], NULL, 0);
	xmlSaveDoc(ctxt, doc);
	xmlSaveClose(ctxt);

	/* Close stdin of the child process. */
	if (close(stdin_pipe[1]) == -1) {
		goto pipe_error;
	}

	/* Read the stdout of the child process into a new XML document. */
	*out = xmlReadFd(stdout_pipe[0], NULL, NULL, XML_PARSE_NOERROR);

	/* Close the stdout of the child process. */
	if (close(stdout_pipe[0]) == -1) {
		goto pipe_error;
	}

	/* Wait for the child process to finish to get its exit status. */
	int status;
	pid_t wait_pid;
	do {
		wait_pid = waitpid(pid, &status, 0);
	} while (wait_pid == -1 && errno == EINTR);

	/* If the child process returned normally, return the exit status. Otherwise, abort. */
	if (WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}

	/* Abort if there are any errors creating/executing the child process. */
pipe_error:
	fprintf(stderr, E_PIPE, cmd, strerror(errno));
	exit(EXIT_PIPE);
}

/* Add error elements from reports to the collection of errors. */
static void add_errors_from_report(const xmlNodePtr errors, const xmlDocPtr report, const xmlChar *cmd)
{
	xmlNodePtr cmd_node;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	cmd_node = xmlNewChild(errors, NULL, BAD_CAST "validator", NULL);
	xmlSetProp(cmd_node, BAD_CAST "command", cmd);

	ctx = xmlXPathNewContext(report);
	obj = xmlXPathEvalExpression(BAD_CAST "//error", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlAddChild(cmd_node, xmlCopyNode(obj->nodesetval->nodeTab[i], 1));
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Check if an object is valid for a set of assertions. */
static int check_assigns(xmlDocPtr doc, const char *path, xmlNodePtr asserts, xmlNodePtr product, const xmlChar *id, const char *pctfname, struct appcheckopts *opts)
{
	xmlNodePtr cur;
	int err = 0, e = 0;
	char filter_cmd[1024] = "";
	xmlDocPtr filtered_doc = NULL;
	xmlNodePtr errors = NULL;

	if (verbosity >= DEBUG) {
		if (opts->mode >= ALL) {
			fprintf(stderr, I_CHECK_ALL_START, path);
		} else if (id) {
			fprintf(stderr, I_CHECK_PROD, path, id, pctfname);
		} else {
			fprintf(stderr, I_CHECK_PROD_LINENO, path, xmlGetLineNo(product), pctfname);
		}
	}

	if (opts->filter) {
		strncpy(filter_cmd, opts->filter, 1023);
	} else {
		strncpy(filter_cmd, DEFAULT_FILTER, 1023);
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

	execute_command_on_doc_and_parse_output(doc, filter_cmd, &filtered_doc);

	/* If the filtered doc is empty, the doc was not applicable to the given assigns. */
	if (filtered_doc == NULL) {
		return 0;
	}

	if (opts->include_errors) {
		errors = xmlNewNode(NULL, BAD_CAST "errors");
	}

	/* Custom validators. */
	if (opts->validators) {
		for (cur = opts->validators->children; cur; cur = cur->next) {
			xmlChar *cmd;

			cmd = xmlNodeGetContent(cur);

			if (opts->include_errors) {
				xmlDocPtr report = NULL;

				e += execute_command_on_doc_and_parse_output(filtered_doc, (char *) cmd, &report);

				if (report) {
					add_errors_from_report(errors, report, cmd);
				}

				xmlFreeDoc(report);
			} else {
				e += execute_command_on_doc(filtered_doc, (char *) cmd);
			}

			xmlFree(cmd);
		}
	/* Default validators. */
	} else {
		char cmd[4096];

		/* Schema validation */
		if (opts->include_errors) {
			strcpy(cmd, DEFAULT_VALIDATE " -x");
		} else {
			strcpy(cmd, DEFAULT_VALIDATE);
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

		if (opts->include_errors) {
			xmlDocPtr validate_report = NULL;

			e += execute_command_on_doc_and_parse_output(filtered_doc, cmd, &validate_report);

			if (validate_report) {
				add_errors_from_report(errors, validate_report, BAD_CAST cmd);
			}

			xmlFreeDoc(validate_report);
		} else {
			e += execute_command_on_doc(filtered_doc, cmd);
		}

		/* BREX validation */
		if (opts->brexcheck) {
			if (opts->include_errors) {
				strcpy(cmd, DEFAULT_BREXCHECK " -clx");
			} else {
				strcpy(cmd, DEFAULT_BREXCHECK " -cl");
			}

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

			if (opts->include_errors) {
				xmlDocPtr brexcheck_report = NULL;

				e += execute_command_on_doc_and_parse_output(filtered_doc, cmd, &brexcheck_report);

				if (brexcheck_report) {
					add_errors_from_report(errors, brexcheck_report, BAD_CAST cmd);
				}

				xmlFreeDoc(brexcheck_report);
			} else {
				e += execute_command_on_doc(filtered_doc, cmd);
			}
		}
	}

	xmlFreeDoc(filtered_doc);

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

		if (opts->include_errors) {
			xmlAddChild(asserts, xmlCopyNode(errors, 1));
		}

		++err;
	} else {
		xmlSetProp(asserts, BAD_CAST "valid", BAD_CAST "yes");
	}

	if (opts->include_errors) {
		xmlFreeNode(errors);
	}

	return err ? 1 : 0;
}

/* Extract assertions from ACT/CCT enumerations. */
static void extract_enumvals(xmlNodePtr asserts, xmlNodePtr prop, const xmlChar *id, bool cct)
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
			char *saveptr = NULL;

			v = xmlNodeGetContent(obj->nodesetval->nodeTab[i]);

			while ((c = BAD_CAST strtok_r(c ? NULL : (char *) v, "|~", &saveptr))) {
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
static void transform_doc(xmlDocPtr doc, unsigned char *xml, unsigned int len, const char **params)
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
static xmlNodePtr add_object_node(xmlNodePtr parent, const char *name, const char *path)
{
	xmlNodePtr node;
	node = xmlNewChild(parent, NULL, BAD_CAST name, NULL);
	xmlSetProp(node, BAD_CAST "path", BAD_CAST path);
	return node;
}

/* Add a string to each annotation that uniquely identifies it. */
static void add_unique_applic_strings(xmlDocPtr doc)
{
	transform_doc(doc, duplicate_xsl, duplicate_xsl_len, NULL);
}

/* Remove the unique identifier strings from annotations. */
static void rem_unique_applic_strings(xmlDocPtr doc)
{
	transform_doc(doc, clean_duplicate_xsl, clean_duplicate_xsl_len, NULL);
}

/* Add a duplicate applicability error to the report. */
static xmlNodePtr add_duplicate_error(xmlNodePtr report, xmlNodePtr node1, xmlNodePtr node2, const char *path)
{
	xmlNodePtr error;
	long int line1, line2;
	xmlChar line_s[16], *xpath;

	line1 = xmlGetLineNo(node1);
	line2 = xmlGetLineNo(node2);

	if (verbosity >= NORMAL) {
		fprintf(stderr, E_DUPLICATECHECK,
			path,
			line2,
			line1);
	}

	error = xmlNewChild(report, NULL, BAD_CAST "duplicateApplicError", NULL);

	xmlStrPrintf(line_s, 16, "%ld", line2);
	xmlSetProp(error, BAD_CAST "line", line_s);

	xpath = xpath_of(node2);
	xmlSetProp(error, BAD_CAST "xpath", xpath);
	xmlFree(xpath);

	xmlStrPrintf(line_s, 16, "%ld", line1);
	xmlSetProp(error, BAD_CAST "duplicateOfLine", line_s);

	xpath = xpath_of(node1);
	xmlSetProp(error, BAD_CAST "duplicateOfXPath", xpath);
	xmlFree(xpath);

	return error;
}

static bool same_annotation(xmlNodePtr app1, xmlNodePtr app2)
{
	xmlDocPtr d1, d2;
	xmlChar *s1, *s2;
	bool same;

	/* Compare c14n representation of XML to
	 * determine if the annotations are duplicates.
	 */
	d1 = xmlNewDoc(BAD_CAST "1.0");
	d2 = xmlNewDoc(BAD_CAST "1.0");

	xmlDocSetRootElement(d1, xmlCopyNode(app1, 1));
	xmlDocSetRootElement(d2, xmlCopyNode(app2, 1));

	xmlC14NDocDumpMemory(d1, NULL, XML_C14N_1_0, NULL, 0, &s1);
	xmlC14NDocDumpMemory(d2, NULL, XML_C14N_1_0, NULL, 0, &s2);

	same = xmlStrcmp(s1, s2) == 0;

	xmlFree(s1);
	xmlFree(s2);

	xmlFreeDoc(d1);
	xmlFreeDoc(d2);

	return same;
}

/* Check for duplicate annotations. */
static int check_duplicate_applic(xmlDocPtr doc, const char *path, struct appcheckopts *opts, xmlNodePtr report)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	add_unique_applic_strings(doc);

	ctx = xmlXPathNewContext(doc);
	xmlXPathRegisterNs(ctx, BAD_CAST "s1kd-appcheck", S1KD_APPCHECK_NS);
	obj = xmlXPathEval(BAD_CAST "//applic/s1kd-appcheck:annotation", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			int j;

			for (j = i + 1; j < obj->nodesetval->nodeNr; ++j) {
				if (same_annotation(obj->nodesetval->nodeTab[i], obj->nodesetval->nodeTab[j])) {
					add_duplicate_error(report, obj->nodesetval->nodeTab[i]->parent, obj->nodesetval->nodeTab[j]->parent, path);
					err = 1;
				}
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	rem_unique_applic_strings(doc);

	return err;
}

static int custom_check(xmlDocPtr doc, const char *path, struct appcheckopts *opts, xmlNodePtr report)
{
	xmlDocPtr act = NULL;
	xmlDocPtr cct = NULL;
	int err = 0;

	if (opts->add_deps || opts->check_props) {
		char actfname[PATH_MAX];
		char cctfname[PATH_MAX];

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
	}

	if (opts->check_duplicate) {
		err += check_duplicate_applic(doc, path, opts, report);
	}

	if (opts->check_nested || opts->check_redundant) {
		err += check_nested_applics(doc, path, opts, report);
	}

	xmlFreeDoc(cct);
	xmlFreeDoc(act);

	return err;
}

/* Check that an object is valid for a single product instances. */
static int check_prod(xmlDocPtr doc, const char *path, xmlDocPtr all, const char *pctfname, xmlNodePtr prod, struct appcheckopts *opts, xmlNodePtr report)
{
	xmlNodePtr asserts;
	xmlChar *id;
	int err = 0;

	asserts = xmlNewNode(NULL, BAD_CAST "asserts");

	if (all) {
		id = NULL;
	} else {
		xmlChar line_s[16], *xpath;

		id = xmlGetProp(prod, BAD_CAST "id");

		if (id) {
			xmlSetProp(asserts, BAD_CAST "product", id);
		}

		xmlStrPrintf(line_s, 16, "%ld", xmlGetLineNo(prod));
		xmlSetProp(asserts, BAD_CAST "line", line_s);

		xpath = xpath_of(prod);
		xmlSetProp(asserts, BAD_CAST "xpath", xpath);
		xmlFree(xpath);
	}

	extract_assigns(asserts, prod);

	err += check_assigns(doc, path, asserts, prod, id, pctfname, opts);

	xmlAddChild(report, xmlCopyNode(asserts, 1));

	xmlFreeNode(asserts);

	xmlFree(id);

	return err;
}

/* Function used to run check_prod in parallel. */
static void *check_prod_thread(void *data)
{
	int i;

	struct check_prod_thread_args *args = (struct check_prod_thread_args *) data;

	struct check_prod_thread_result *result;

	result = malloc(sizeof(struct check_prod_thread_result));
	result->err = 0;
	result->report = xmlNewNode(NULL, BAD_CAST "report");

	for (i = args->job; i < args->nodes->nodeNr; i += check_prod_threads) {
		result->err += check_prod(args->doc, args->path, args->all, args->pctfname, args->nodes->nodeTab[i], args->opts, result->report);
	}

	free(args);

	pthread_exit(result);

	return NULL;
}

/* Check that an object is valid for all defined product instances. */
static int check_prods(xmlDocPtr doc, const char *path, xmlDocPtr all, xmlDocPtr act, struct appcheckopts *opts, xmlNodePtr report)
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

		if (verbosity >= DEBUG) {
			fprintf(stderr, I_NUM_PRODS, path, obj->nodesetval->nodeNr);
		}

		/* If check_prod_threads > 1, check objects in parallel. */
		if (check_prod_threads > 1) {
			pthread_t threads[check_prod_threads];

			if (verbosity >= DEBUG) {
				fprintf(stderr, I_THREADS, check_prod_threads);
			}

			/* Create threads. */
			for (i = 0; i < check_prod_threads; ++i) {
				struct check_prod_thread_args *args;

				args = malloc(sizeof(struct check_prod_thread_args));
				args->doc = doc;
				args->path = path;
				args->all = all;
				args->pctfname = pctfname;
				args->nodes = obj->nodesetval;
				args->opts = opts;
				args->job = i;

				pthread_create(&threads[i], NULL, check_prod_thread, args);
			}

			/* Wait for all threads to finish and combine results into main report. */
			for (i = 0; i < check_prod_threads; ++i) {
				struct check_prod_thread_result *result;
				xmlNodePtr child;

				pthread_join(threads[i], (void **) &result);

				err += result->err;

				for (child = result->report->children; child; child = child->next) {
					xmlAddChild(report, xmlCopyNode(child, 1));
				}

				xmlFreeNode(result->report);
				free(result);
			}
		/* Otherwise, check all objects in the main thread. */
		} else {
			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				err += check_prod(doc, path, all, pctfname, obj->nodesetval->nodeTab[i], opts, report);
			}
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
static void add_assert(xmlNodePtr asserts, xmlNodePtr assert)
{
	xmlChar *i, *t, *v, *c = NULL;
	char *saveptr = NULL;

	i = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyIdent|@actidref");
	t = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyType|@actreftype");
	v = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyValues|@actvalues");

	if (i && t && v) {
		while ((c = BAD_CAST strtok_r(c ? NULL : (char *) v, "|~", &saveptr))) {
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
	}

	xmlFree(i);
	xmlFree(t);
	xmlFree(v);
}

/* Find a property in a set of properties. */
static xmlNodePtr set_has_prop(xmlNodePtr set, const xmlChar *name)
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
static int check_object_props(xmlDocPtr doc, const char *path, struct appcheckopts *opts, xmlNodePtr report)
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

	if (opts->check_duplicate) {
		err += check_duplicate_applic(doc, path, opts, report);
	}

	if (opts->check_nested || opts->check_redundant) {
		err += check_nested_applics(doc, path, opts, report);
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

/* Check if a property has been added to the ignord properties table. */
static bool prop_is_ignored(const xmlChar *id, const xmlChar *type)
{
	xmlChar *specifier;

	if (ignored_properties == NULL)
	{
		return false;
	}

	specifier = xmlStrdup(id);
	specifier = xmlStrcat(specifier, BAD_CAST ":");
	specifier = xmlStrcat(specifier, type);

	bool is_ignored = xmlHashLookup(ignored_properties, specifier) != NULL;

	xmlFree(specifier);

	return is_ignored;
}

/* Determine whether a property is used in the inline annotations of an object. */
static bool prop_is_used(const xmlChar *id, const xmlChar *type, xmlDocPtr doc)
{
	xmlChar *xpath;
	int n;
	xmlNodePtr node;

	if (prop_is_ignored(id, type)) {
		return false;
	}

	n = xmlStrlen(id) * 2 + xmlStrlen(type) * 2 + 126;
	xpath = malloc(n * sizeof(xmlChar));
	xmlStrPrintf(xpath, n, "(//content|//inlineapplics)//assert[(@applicPropertyIdent='%s' or @actidref='%s') and (@applicPropertyType='%s' or @actreftype='%s')]", id, id, type, type);
	node = first_xpath_node(doc, NULL, xpath);
	xmlFree(xpath);

	return node != NULL;
}

/* Determine whether an object uses any conditions. */
static bool has_conds(xmlDocPtr doc)
{
	return first_xpath_node(doc, NULL, BAD_CAST "//assert[@applicPropertyType='condition' or @actreftype='condition']");
}

/* Check all possible combinations of applicability property values which may
 * affect the object.
 */
static int check_all_props(xmlDocPtr doc, const char *path, xmlDocPtr act, struct appcheckopts *opts, xmlNodePtr report)
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

	if (opts->check_duplicate) {
		err += check_duplicate_applic(doc, path, opts, report);
	}

	if (opts->check_nested || opts->check_redundant) {
		err += check_nested_applics(doc, path, opts, report);
	}

	ctx = xmlXPathNewContext(act);
	obj = xmlXPathEvalExpression(BAD_CAST "//productAttribute|//prodattr", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *id;
			xmlNodePtr asserts;

			id = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "id");

			if (prop_is_used(id, BAD_CAST "prodattr", doc)) {
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

				if (prop_is_used(id, BAD_CAST "condition", doc)) {
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

	transform_doc(psdoc, combos_xsl, combos_xsl_len, NULL);
	err += check_prods(doc, path, psdoc, NULL, opts, report);

	xmlFreeDoc(psdoc);

	return err;
}

/* Check product instances read from the PCT. */
static int check_pct_instances(xmlDocPtr doc, const char *path, xmlDocPtr act, struct appcheckopts *opts, xmlNodePtr report)
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
			act = NULL;
		}
	}

	if (opts->check_duplicate) {
		err += check_duplicate_applic(doc, path, opts, report);
	}

	if (opts->check_nested || opts->check_redundant) {
		err += check_nested_applics(doc, path, opts, report);
	}

	err += check_prods(doc, path, NULL, act, opts, report);

	return err;
}

/* Determine whether an object uses any computable applicability. */
static bool has_applic(xmlDocPtr doc)
{
	return first_xpath_node(doc, NULL, BAD_CAST "//assert") != NULL;
}

/* Check the applicability in an object. */
static int check_applic_file(const char *path, struct appcheckopts *opts, xmlNodePtr report)
{
	xmlDocPtr doc;
	int err = 0;
	char actfname[PATH_MAX];
	xmlNodePtr report_node = NULL;
	xmlDocPtr validtree = NULL;

	if (!(doc = read_xml_doc(path))) {
		if (verbosity > QUIET) {
			fprintf(stderr, E_BAD_OBJECT, path);
		}
		exit(EXIT_BAD_OBJECT);
	}

	/* Make a copy of the XML tree before performing extra
	 * processing on it. */
	if (opts->output_tree) {
		validtree = xmlCopyDoc(doc, 1);
	}

	/* Remove elements marked as "delete". */
	if (opts->rem_delete) {
		rem_delete_elems(doc);
	}

	if (report) {
		report_node = add_object_node(report, "object", path);
	}

	/* Add the type of check to the report. */
	switch (opts->mode) {
		case CUSTOM:
			xmlSetProp(report, BAD_CAST "type", BAD_CAST "custom");
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

	if (opts->mode == CUSTOM) {
		err += custom_check(doc, path, opts, report_node);
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

		if (opts->filenames == SHOW_INVALID) {
			puts(path);
		}
	} else {
		xmlSetProp(report_node, BAD_CAST "valid", BAD_CAST "yes");

		if (opts->filenames == SHOW_VALID) {
			puts(path);
		}
	}

	if (opts->output_tree) {
		if (err == 0) {
			save_xml_doc(validtree, "-");
		}
		xmlFreeDoc(validtree);
	}

	if (verbosity >= VERBOSE) {
		fprintf(stderr, err ? F_INVALID : S_VALID, path);
	}

	xmlFreeDoc(doc);

	return err;
}

/* Add a CSDB object path to check. */
static void add_object(const char *path)
{
	if (nobjects == OBJECT_MAX) {
		if (!(objects = realloc(objects, (OBJECT_MAX *= 2) * PATH_MAX))) {
			if (verbosity > QUIET) {
				fprintf(stderr, E_MAX_OBJECTS);
			}
			exit(EXIT_MAX_OBJECTS);
		}
	}

	strcpy(objects[nobjects++], path);
}

/* Add a list of CSDB object paths to check. */
static void add_object_list(const char *fname)
{
	FILE *f;
	char path[PATH_MAX];

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			if (verbosity >= NORMAL) {
				fprintf(stderr, E_BAD_LIST, fname);
			}
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(path, PATH_MAX, f)) {
		strtok(path, "\t\r\n");
		add_object(path);
	}

	if (fname) {
		fclose(f);
	}
}

/* Show a summary of the check. */
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

/* Add a property to the list of ones to ignore while validating. */
static void add_ignored_property(char *specifier)
{
	if (ignored_properties == NULL) {
		ignored_properties = xmlHashCreate(0);
	}

	xmlHashAddEntry(ignored_properties, BAD_CAST specifier, ignored_properties);
}

/* Function used to check object files in parallel. */
static void *object_thread(void *data)
{
	int i;
	struct object_thread_args *args = (struct object_thread_args *) data;
	struct object_thread_result *result;

	result = malloc(sizeof(struct object_thread_result));
	result->err = 0;
	result->report = xmlNewNode(NULL, BAD_CAST "report");

	for (i = args->job; i < nobjects; i += object_threads) {
		result->err += check_applic_file(objects[i], args->opts, result->report);

		/* Use mutex lock to ensure only one thread is writing the progress at a time. */
		if (args->show_progress != PROGRESS_OFF) {
			pthread_mutex_lock(&object_thread_progress_mutex);

			switch (args->show_progress) {
				case PROGRESS_CLI:
					print_progress_bar(object_thread_progress, nobjects);
					break;
				case PROGRESS_ZENITY:
					print_zenity_progress("Performing applicability check...", object_thread_progress, nobjects);
					break;
			}

			++object_thread_progress;

			pthread_mutex_unlock(&object_thread_progress_mutex);
		}
	}

	free(args);

	pthread_exit(result);

	return NULL;
}

/* Parse the number of threads to use, in the form of <x[,y]>. */
static void parse_thread_arg(char *optarg)
{
	char *x, *y, *saveptr;

	x = strtok_r(optarg, ",", &saveptr);
	y = strtok_r(NULL, ",", &saveptr);

	object_threads = atoi(x);

	if (y) {
		check_prod_threads = atoi(y);
	}
}

int main(int argc, char **argv)
{
	int i;

	const char *sopts = "A:abC:cDd:e:Ffi:NnK:k:loP:pqRrsTtuvXx~#:h?";
	struct option lopts[] = {
		{"version"        , no_argument      , 0, 0},
		{"help"           , no_argument      , 0, 'h'},
		{"act"            , required_argument, 0, 'A'},
		{"all"            , no_argument      , 0, 'a'},
		{"brexcheck"      , no_argument      , 0, 'b'},
		{"cct"            , required_argument, 0, 'C'},
		{"custom"         , no_argument      , 0, 'c'},
		{"duplicate"      , no_argument      , 0, 'D'},
		{"dir"            , required_argument, 0, 'd'},
		{"exec"           , required_argument, 0, 'e'},
		{"valid-filenames", no_argument      , 0, 'F'},
		{"filenames"      , no_argument      , 0, 'f'},
		{"ignore"         , required_argument, 0, 'i'},
		{"filter"         , required_argument, 0, 'K'},
		{"args"           , required_argument, 0, 'k'},
		{"list"           , no_argument      , 0, 'l'},
		{"omit-issue"     , no_argument      , 0, 'N'},
		{"nested"         , no_argument      , 0, 'n'},
		{"output-valid"   , no_argument      , 0, 'o'},
		{"pct"            , required_argument, 0, 'P'},
		{"progress"       , required_argument, 0, 'p'},
		{"quiet"          , no_argument      , 0, 'q'},
		{"redundant"      , no_argument      , 0, 'R'},
		{"recursive"      , no_argument      , 0, 'r'},
		{"strict"         , no_argument      , 0, 's'},
		{"summary"        , no_argument      , 0, 'T'},
		{"products"       , no_argument      , 0, 't'},
		{"unstrict-nested", no_argument      , 0, 'u'},
		{"verbose"        , no_argument      , 0, 'v'},
		{"xml-with-errors", no_argument      , 0, 'X'},
		{"xml"            , no_argument      , 0, 'x'},
		{"dependencies"   , no_argument      , 0, '~'},
		{"threads"        , required_argument, 0, '#'},
		{"remove-deleted" , no_argument      , 0, '^'},
		{"zenity-progress", no_argument      , 0, 0},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	bool islist = false;
	bool xmlout = false;
	bool show_stats = false;
	int show_progress = PROGRESS_OFF;

	struct appcheckopts opts = {
		/* useract */         NULL,
		/* usercct */         NULL,
		/* userpct */         NULL,
		/* args */            NULL,
		/* filter */          NULL,
		/* validators */      NULL,
		/* output_tree */     false,
		/* filenames */       SHOW_NONE,
		/* brexcheck */       false,
		/* add_deps */        false,
		/* check_props */     false,
		/* check_nested */    false,
		/* strict_nested */   true,
		/* check_redundant */ false,
		/* check_duplicate */ false,
		/* rem_delete */      false,
		/* mode */            STANDALONE,
		/* include_errors */  false
	};

	int err = 0;

	xmlDocPtr report;
	xmlNodePtr appcheck;

	exsltRegisterAll();

	objects = malloc(OBJECT_MAX * PATH_MAX);

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
				} else if (strcmp(lopts[loptind].name, "zenity-progress") == 0) {
					show_progress = PROGRESS_ZENITY;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg)
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
				opts.mode = CUSTOM;
				break;
			case 'D':
				opts.check_duplicate = true;
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
			case 'F':
				opts.filenames = SHOW_VALID;
				break;
			case 'f':
				opts.filenames = SHOW_INVALID;
				break;
			case 'i':
				add_ignored_property(optarg);
				break;
			case 'K':
				opts.filter = strdup(optarg);
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
			case 'n':
				opts.check_nested = true;
				break;
			case 'o':
				opts.output_tree = true;
				break;
			case 'P':
				opts.userpct = strdup(optarg);
				break;
			case 'p':
				show_progress = PROGRESS_CLI;
				break;
			case 'R':
				opts.check_redundant = true;
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
			case 't':
				opts.mode = PCT;
				break;
			case 'u':
				opts.check_nested = true;
				opts.strict_nested = false;
				break;
			case 'q':
				--verbosity;
				break;
			case 'v':
				++verbosity;
				break;
			case 'X':
				opts.include_errors = true;
			case 'x':
				xmlout = true;
				break;
			case '~':
				opts.add_deps = true;
				break;
			case '^':
				opts.rem_delete = true;
				break;
			case '#':
				parse_thread_arg(optarg);
				break;
			case 'h':
			case '?':
				show_help();
				goto cleanup;
		}
	}

	/* Record options in the XML report. */
	if (opts.check_props) {
		xmlSetProp(appcheck, BAD_CAST "strict", BAD_CAST "yes");
	} else {
		xmlSetProp(appcheck, BAD_CAST "strict", BAD_CAST "no");
	}

	if (opts.check_nested) {
		xmlSetProp(appcheck, BAD_CAST "checkNestedApplic", BAD_CAST "yes");
	} else {
		xmlSetProp(appcheck, BAD_CAST "checkNestedApplic", BAD_CAST "no");
	}

	if (opts.check_redundant) {
		xmlSetProp(appcheck, BAD_CAST "checkRedundantApplic", BAD_CAST "yes");
	} else {
		xmlSetProp(appcheck, BAD_CAST "checkRedundantApplic", BAD_CAST "no");
	}

	if (opts.check_duplicate) {
		xmlSetProp(appcheck, BAD_CAST "checkDuplicateApplic", BAD_CAST "yes");
	} else {
		xmlSetProp(appcheck, BAD_CAST "checkDuplicateApplic", BAD_CAST "no");
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				add_object_list(argv[i]);
			} else {
				add_object(argv[i]);
			}
		}
	} else if (islist) {
		add_object_list(NULL);
	} else {
		add_object("-");
	}

	if (object_threads > 1) {
		pthread_t threads[object_threads];

		for (i = 0; i < object_threads; ++i) {
			struct object_thread_args *args;

			args = malloc(sizeof(struct object_thread_args));
			args->opts = &opts;
			args->show_progress = show_progress;
			args->job = i;

			pthread_create(&threads[i], NULL, object_thread, args);
		}

		for (i = 0; i < object_threads; ++i) {
			struct object_thread_result *result;
			xmlNodePtr child;

			pthread_join(threads[i], (void **) &result);

			err += result-> err;

			for (child = result->report->children; child; child = child->next) {
				xmlAddChild(appcheck, xmlCopyNode(child, 1));
			}

			xmlFreeNode(result->report);
			free(result);
		}
	} else {
		for (i = 0; i < nobjects; ++i) {
			err += check_applic_file(objects[i], &opts, appcheck);

			switch (show_progress) {
				case PROGRESS_OFF:
					break;
				case PROGRESS_CLI:
					print_progress_bar(i, nobjects);
					break;
				case PROGRESS_ZENITY:
					print_zenity_progress("Performing applicability check...", i, nobjects);
					break;
			}
		}
	}

	if (nobjects) {
		switch (show_progress) {
			case PROGRESS_OFF:
				break;
			case PROGRESS_CLI:
				print_progress_bar(nobjects, nobjects);
				break;
			case PROGRESS_ZENITY:
				print_zenity_progress("Applicability check complete", nobjects, nobjects);
				break;
		}
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
	free(opts.filter);
	free(opts.args);
	xmlFreeNode(opts.validators);
	free(search_dir);
	free(objects);

	if (ignored_properties != NULL) xmlHashFree(ignored_properties, NULL);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}
