#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <dirent.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/transform.h>
#include "s1kd_tools.h"
#include "xsl.h"

/* Program information. */
#define PROG_NAME "s1kd-repcheck"
#define VERSION "1.10.0"

/* Message prefixes. */
#define ERR_PREFIX PROG_NAME ": ERROR: "
#define WRN_PREFIX PROG_NAME ": WARNING: "
#define INF_PREFIX PROG_NAME ": INFO: "
#define SUC_PREFIX PROG_NAME ": SUCCESS: "
#define FLD_PREFIX PROG_NAME ": FAILED: "

/* Error messages. */
#define E_MAX_OBJECTS ERR_PREFIX "Out of memory\n"
#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"
#define E_NOT_FOUND ERR_PREFIX "%s (%ld): %s not found.\n"
#define E_UNHANDLED_REF ERR_PREFIX "Unhandled CIR ref type: %s\n"

/* Warning messages. */
#define W_MISSING_REF_DM WRN_PREFIX "Could not read referenced object: %s\n"

/* Informational messages. */
#define I_CHECK INF_PREFIX "Checking CIR references in %s...\n"
#define I_SEARCH_PART INF_PREFIX "Searching for %s in CIR %s...\n"
#define I_FOUND INF_PREFIX "Found %s in CIR %s\n"
#define I_NOT_FOUND INF_PREFIX "Not found in CIR %s\n"
#define I_FIND_CIR INF_PREFIX "Searching for CIRs in \"%s\"...\n"
#define I_FIND_CIR_FOUND INF_PREFIX "Found CIR %s...\n"
#define I_FIND_CIR_ADD INF_PREFIX "Added CIR %s\n"

/* Success messages. */
#define S_VALID SUC_PREFIX "All CIR references were resolved in %s.\n"

/* Failure messages. */
#define F_INVALID FLD_PREFIX "Could not resolve some CIR references in %s.\n"

/* Exit status codes. */
#define EXIT_MAX_OBJECTS 2

/* Progress formats. */
#define PROGRESS_OFF 0
#define PROGRESS_CLI 1
#define PROGRESS_ZENITY 2

/* Namespace for special attributes used to extract CIR references. */
#define S1KD_REPCHECK_NS BAD_CAST "urn:s1kd-tools:s1kd-repcheck"

/* Verbosity of messages. */
enum verbosity { QUIET, NORMAL, VERBOSE, DEBUG };

/* List of CSDB objects. */
struct objects {
	char (*paths)[PATH_MAX];
	unsigned count;
	unsigned max;
};

enum show_filenames { SHOW_NONE, SHOW_INVALID, SHOW_VALID };

/* Program options. */
struct opts {
	enum verbosity verbosity;
	enum show_filenames show_filenames;
	char *search_dir;
	bool recursive;
	bool no_issue;
	bool search_all_objs;
	bool output_valid;
	bool list_refs;
	bool rem_delete;
	xmlDocPtr cir_refs_xsl;
	char *type;
	struct objects objects;
	struct objects cirs;
	xmlNodePtr report;
};

/* Match a CIR ref to a CIR spec in a given CIR data module. */
static xmlNodePtr find_ref_in_cir(xmlNodePtr ref, const xmlChar *ident, const xmlChar *xpath, const char *cirpath, struct opts *opts)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node = NULL;

	if (opts->verbosity >= DEBUG) {
		fprintf(stderr, I_SEARCH_PART, ident, cirpath);
	}

	if (!(doc = read_xml_doc(cirpath))) {
		return NULL;
	}

	if (opts->rem_delete) {
		rem_delete_elems(doc);
	}

	ctx = xmlXPathNewContext(doc);

	if ((obj = xmlXPathEvalExpression(xpath, ctx))) {
		if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			if (opts->verbosity >= DEBUG) {
				fprintf(stderr, I_NOT_FOUND, cirpath);
			}
		} else {
			node = obj->nodesetval->nodeTab[0];

			if (opts->verbosity >= DEBUG) {
				fprintf(stderr, I_FOUND, ident, cirpath);
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	xmlFreeDoc(doc);

	return node;
}

/* Add a reference to the XML report. */
static void add_ref_to_report(xmlNodePtr rpt, xmlNodePtr ref, const xmlChar *type, const xmlChar *ident, long int lineno, const char *cir, struct opts *opts)
{
	xmlNodePtr node;
	xmlChar line_s[16], *xpath;

	/* Check if XML report is enabled. */
	if (!rpt) {
		return;
	}

	node = xmlNewChild(rpt, NULL, BAD_CAST "ref", NULL);

	xmlSetProp(node, BAD_CAST "type", type);
	xmlSetProp(node, BAD_CAST "name", ident);

	xmlStrPrintf(line_s, 16, "%ld", lineno);
	xmlSetProp(node, BAD_CAST "line", BAD_CAST line_s);

	xpath = xpath_of(ref);
	xmlSetProp(node, BAD_CAST "xpath", xpath);
	xmlFree(xpath);

	if (cir) {
		xmlSetProp(node, BAD_CAST "cir", BAD_CAST cir);
	}

	node = xmlAddChild(node, xmlCopyNode(ref, 1));
}

/* Find a data module filename in the current directory based on the dmRefIdent
 * element. */
static bool find_dmod_fname(char *dst, xmlNodePtr dmRefIdent, struct opts *opts)
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

	dmCode = xpath_first_node(NULL, dmRefIdent, BAD_CAST "dmCode|avee");
	issueInfo = xpath_first_node(NULL, dmRefIdent, BAD_CAST "issueInfo|issno");
	language = xpath_first_node(NULL, dmRefIdent, BAD_CAST "language");

	model_ident_code     = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "modelic|@modelIdentCode");
	system_diff_code     = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "sdc|@systemDiffCode");
	system_code          = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "chapnum|@systemCode");
	sub_system_code      = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "section|@subSystemCode");
	sub_sub_system_code  = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "subsect|@subSubSystemCode");
	assy_code            = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "subject|@assyCode");
	disassy_code         = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "discode|@disassyCode");
	disassy_code_variant = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "discodev|@disassyCodeVariant");
	info_code            = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "incode|@infoCode");
	info_code_variant    = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "incodev|@infoCodeVariant");
	item_location_code   = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "itemloc|@itemLocationCode");
	learn_code           = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "@learnCode");
	learn_event_code     = (char *) xpath_first_value(NULL, dmCode, BAD_CAST "@learnEventCode");

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

	if (!opts->no_issue) {
		if (issueInfo) {
			char *issue_number;
			char *in_work;
			char iss[8];

			issue_number = (char *) xpath_first_value(NULL, issueInfo, BAD_CAST "@issno|@issueNumber");
			in_work      = (char *) xpath_first_value(NULL, issueInfo, BAD_CAST "@inwork|@inWork");

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

		language_iso_code = (char *) xpath_first_value(NULL, language, BAD_CAST "@language|@languageIsoCode");
		country_iso_code  = (char *) xpath_first_value(NULL, language, BAD_CAST "@country|@countryIsoCode");

		snprintf(lang, 8, "_%s-%s", language_iso_code, country_iso_code);
		strcat(code, lang);

		xmlFree(language_iso_code);
		xmlFree(country_iso_code);
	}

	/* Look for DM in the directory hierarchy. */
	if (find_csdb_object(dst, opts->search_dir, code, is_dm, opts->recursive)) {
		return true;
	}

	/* Look for DM in the list of CIRs. */
	if (find_csdb_object_in_list(dst, opts->cirs.paths, opts->cirs.count, code)) {
		return true;
	}

	/* Look for DM in the list of objects to check. */
	if (find_csdb_object_in_list(dst, opts->objects.paths, opts->objects.count, code)) {
		return true;
	}

	fprintf(stderr, W_MISSING_REF_DM, code);
	return false;
}

/* Remove namespace declaration added by tool. */
static void remove_repcheck_ns(xmlNodePtr node)
{
	xmlNsPtr cur, prev;

	cur  = node->nsDef;
	prev = NULL;

	while (cur) {
		xmlNsPtr next;

		next = cur->next;

		if (xmlStrcmp(cur->href, S1KD_REPCHECK_NS) == 0) {
			if (prev == NULL) {
				node->nsDef = next;
			} else {
				prev->next = next;
			}

			xmlFreeNode((xmlNodePtr) cur);
		} else {
			prev = cur;
		}

		cur = next;
	}
}

/* Remove attributes added by tool. */
static void remove_repcheck_attrs(xmlNodePtr ref, xmlNsPtr ns)
{
	xmlUnsetNsProp(ref, ns, BAD_CAST "type");
	xmlUnsetNsProp(ref, ns, BAD_CAST "name");
	xmlUnsetNsProp(ref, ns, BAD_CAST "test");
	remove_repcheck_ns(ref);
}

/* Check a specific CIR reference. */
static int check_cir_ref(xmlNodePtr ref, const char *path, xmlNodePtr rpt, struct opts *opts)
{
	int i, err = 0;
	xmlAttrPtr type_attr, ident_attr, xpath_attr;
	xmlChar *type, *ident, *xpath;
	long int lineno;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	lineno = xmlGetLineNo(ref);

	type_attr  = xmlHasNsProp(ref, BAD_CAST "type", S1KD_REPCHECK_NS);
	ident_attr = xmlHasNsProp(ref, BAD_CAST "name", S1KD_REPCHECK_NS);
	xpath_attr = xmlHasNsProp(ref, BAD_CAST "test", S1KD_REPCHECK_NS);

	type  = xmlNodeGetContent((xmlNodePtr) type_attr);
	ident = xmlNodeGetContent((xmlNodePtr) ident_attr);
	xpath = xmlNodeGetContent((xmlNodePtr) xpath_attr);

	remove_repcheck_attrs(ref, ident_attr->ns);

	/* Check if there is an explicit CIR reference. */
	ctx = xmlXPathNewContext(ref->doc);
	xmlXPathSetContextNode(ref, ctx);
	obj = xmlXPathEvalExpression(BAD_CAST "refs/dmRef/dmRefIdent|refs/refdm", ctx);

	/* If there is not, use any of the specified/found CIRs. */
	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		/* Search in all CIRs. */
		for (i = 0; i < opts->cirs.count; ++i) {
			if (find_ref_in_cir(ref, ident, xpath, opts->cirs.paths[i], opts)) {
				add_ref_to_report(rpt, ref, type, ident, lineno, opts->cirs.paths[i], opts);
				goto done;
			}
		}

		/* Search in all other specified objects, if allowed. */
		if (opts->search_all_objs) {
			for (i = 0; i < opts->objects.count; ++i) {
				if (find_ref_in_cir(ref, ident, xpath, opts->objects.paths[i], opts)) {
					add_ref_to_report(rpt, ref, type, ident, lineno, opts->objects.paths[i], opts);
					goto done;
				}
			}
		}
	/* If there is an explicit reference, only check against that. */
	} else {
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			char fname[PATH_MAX];

			if (find_dmod_fname(fname, obj->nodesetval->nodeTab[i], opts)) {
				if (find_ref_in_cir(ref, ident, xpath, fname, opts)) {
					add_ref_to_report(rpt, ref, type, ident, lineno, fname, opts);
					goto done;
				}
			}
		}
	}

	if (opts->verbosity >= NORMAL) {
		fprintf(stderr, E_NOT_FOUND, path, lineno, ident);
	}
	add_ref_to_report(rpt, ref, type, ident, lineno, NULL, opts);
	err = 1;

done:
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
	xmlFree(type);
	xmlFree(ident);
	xmlFree(xpath);

	return err;
}

/* List a CIR reference without validating it. */
static void list_cir_ref(const xmlNodePtr ref, const char *path, xmlNodePtr rpt, struct opts *opts)
{
	xmlAttrPtr type_attr, ident_attr;
	xmlChar *type, *ident;
	long int lineno;

	lineno = xmlGetLineNo(ref);

	type_attr  = xmlHasNsProp(ref, BAD_CAST "type", S1KD_REPCHECK_NS);
	ident_attr = xmlHasNsProp(ref, BAD_CAST "name", S1KD_REPCHECK_NS);

	type  = xmlNodeGetContent((xmlNodePtr) type_attr);
	ident = xmlNodeGetContent((xmlNodePtr) ident_attr);

	remove_repcheck_attrs(ref, ident_attr->ns);

	if (rpt) {
		add_ref_to_report(rpt, ref, type, ident, lineno, NULL, opts);
	} else {
		printf("%s:%ld:%s\n", path, lineno, (char *) ident);
	}

	xmlFree(type);
	xmlFree(ident);
}

/* Check all CIR references in a document. */
static int check_cir_refs(xmlDocPtr doc, const char *path, struct opts *opts)
{
	int err = 0;
	xmlDocPtr styledoc, res;
	xsltStylesheetPtr style;
	xmlNodePtr rpt;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	/* Add object to report. */
	if (opts->report) {
		rpt = xmlNewChild(opts->report, NULL, BAD_CAST "object", NULL);
		xmlSetProp(rpt, BAD_CAST "path", BAD_CAST path);
	} else {
		rpt = NULL;
	}

	styledoc = xmlCopyDoc(opts->cir_refs_xsl, 1);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);

	ctx = xmlXPathNewContext(res);
	xmlXPathRegisterNs(ctx, BAD_CAST "s1kd-repcheck", S1KD_REPCHECK_NS);
	if (opts->type) {
		xmlXPathRegisterVariable(ctx, BAD_CAST "type", xmlXPathNewString(BAD_CAST opts->type));
		obj = xmlXPathEval(BAD_CAST "//*[@s1kd-repcheck:test and @s1kd-repcheck:type=$type]", ctx);
	} else {
		obj = xmlXPathEval(BAD_CAST "//*[@s1kd-repcheck:test]", ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			if (opts->list_refs) {
				list_cir_ref(obj->nodesetval->nodeTab[i], path, rpt, opts);
			} else if (check_cir_ref(obj->nodesetval->nodeTab[i], path, rpt, opts) != 0) {
				err = 1;
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	xmlFreeDoc(res);
	xsltFreeStylesheet(style);

	if (!opts->list_refs) {
		if (err) {
			xmlSetProp(rpt, BAD_CAST "valid", BAD_CAST "no");
		} else {
			xmlSetProp(rpt, BAD_CAST "valid", BAD_CAST "yes");
		}
	}

	return err;
}

/* Check all CIR references in the specified CSDB object. */
static int check_cir_refs_in_file(const char *path, struct opts *opts)
{
	xmlDocPtr doc;
	int err = 0;
	xmlDocPtr validtree = NULL;

	if (opts->verbosity >= DEBUG) {
		fprintf(stderr, I_CHECK, path);
	}

	if (!(doc = read_xml_doc(path))) {
		return 1;
	}

	/* Make a copy of the XML tree before performing additional
	 * processing on it. */
	if (opts->output_valid) {
		validtree = xmlCopyDoc(doc, 1);
	}

	if (opts->rem_delete) {
		rem_delete_elems(doc);
	}

	err = check_cir_refs(doc, path, opts);

	if (opts->verbosity >= VERBOSE) {
		if (err) {
			fprintf(stderr, F_INVALID, path);
		} else {
			fprintf(stderr, S_VALID, path);
		}
	}

	if ((err && opts->show_filenames == SHOW_INVALID) || (!err && opts->show_filenames == SHOW_VALID)) {
			puts(path);
	}

	if (opts->output_valid) {
		if (err == 0) {
			save_xml_doc(validtree, "-");
		}
		xmlFreeDoc(validtree);
	}

	xmlFreeDoc(doc);

	return err;
}

/* Add a CSDB object to a list. */
static void add_object(struct objects *objects, const char *path, struct opts *opts)
{
	if (objects->count == objects->max) {
		if (!(objects->paths = realloc(objects->paths, (objects->max *= 2) * PATH_MAX))) {
			if (opts->verbosity > QUIET) {
				fprintf(stderr, E_MAX_OBJECTS);
			}
			exit(EXIT_MAX_OBJECTS);
		}
	}

	strcpy(objects->paths[(objects->count)++], path);
}

/* Add a list of CSDB objects to a list. */
static void add_object_list(struct objects *objects, const char *list, struct opts *opts)
{
	FILE *f;
	char path[PATH_MAX];

	if (list) {
		if (!(f = fopen(list, "r"))) {
			if (opts->verbosity >= NORMAL) {
				fprintf(stderr, E_BAD_LIST, list);
			}
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(path, PATH_MAX, f)) {
		strtok(path, "\t\r\n");
		add_object(objects, path, opts);
	}

	if (list) {
		fclose(f);
	}
}

/* Initialize a list of CSDB objects. */
static void init_objects(struct objects *objects)
{
	objects->paths = malloc(PATH_MAX);
	objects->max = 1;
	objects->count = 0;
}

/* Free a list of CSDB objects. */
static void free_objects(struct objects *objects)
{
	free(objects->paths);
}

/* Find CIRs in directories and add them to the list. */
static void find_cirs(struct objects *cirs, char *search_dir, struct opts *opts)
{
	DIR *dir;
	struct dirent *cur;
	char fpath[PATH_MAX], cpath[PATH_MAX];

	if (opts->verbosity >= DEBUG) {
		fprintf(stderr, I_FIND_CIR, search_dir);
	}

	if (!(dir = opendir(search_dir))) {
		return;
	}

	/* Clean up the directory string. */
	if (strcmp(search_dir, ".") == 0) {
		strcpy(fpath, "");
	} else if (search_dir[strlen(search_dir) - 1] != '/') {
		strcpy(fpath, search_dir);
		strcat(fpath, "/");
	} else {
		strcpy(fpath, search_dir);
	}

	/* Search for CIRs. */
	while ((cur = readdir(dir))) {
		strcpy(cpath, fpath);
		strcat(cpath, cur->d_name);

		if (opts->recursive && isdir(cpath, true)) {
			find_cirs(cirs, cpath, opts);
		} else if (is_dm(cur->d_name) && is_cir(cpath, opts->rem_delete)) {
			if (opts->verbosity >= DEBUG) {
				fprintf(stderr, I_FIND_CIR_FOUND, cpath);
			}
			add_object(cirs, cpath, opts);
		}
	}

	closedir(dir);
}

/* Use only the latest issue of a CIR. */
static void extract_latest_cirs(struct objects *cirs)
{
	struct objects latest;

	qsort(cirs->paths, cirs->count, PATH_MAX, compare_basename);

	latest.paths = malloc(cirs->count * PATH_MAX);
	latest.count = extract_latest_csdb_objects(latest.paths, cirs->paths, cirs->count);

	free(cirs->paths);
	cirs->paths = latest.paths;
	cirs->count = latest.count;
}

/* Show a summary of the check. */
static void print_stats(xmlDocPtr doc)
{
	xmlDocPtr styledoc;
	xsltStylesheetPtr style;
	xmlDocPtr res;

	styledoc = read_xml_mem((const char *) xsl_stats_xsl, xsl_stats_xsl_len);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);

	fprintf(stderr, "%s", (char *) res->children->content);

	xmlFreeDoc(res);
	xsltFreeStylesheet(style);
}

/* Show usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -A, --all-refs         Validate indirect CIR references.");
	puts("  -a, --all              Resolve against CIRs specified as objects to check.");
	puts("  -d, --dir <dir>        Search for CIRs in <dir>.");
	puts("  -F, --valid-filenames  List valid files.");
	puts("  -f, --filenames        List invalid files.");
	puts("  -h, -?, --help         Show help/usage message.");
	puts("  -L, --list-refs        List CIR refs instead of validating them.");
	puts("  -l, --list             Treat input as list of CSDB objects.");
	puts("  -N, --omit-issue       Assume issue/inwork numbers are omitted.");
	puts("  -o, --output-valid     Output valid CSDB objects to stdout.");
	puts("  -p, --progress         Display a progress bar.");
	puts("  -q, --quiet            Quiet mode.");
	puts("  -R, --cir <CIR>        Check references against the given CIR.");
	puts("  -r, --recursive        Search for CIRs recursively.");
	puts("  -T, --summary          Print a summary of the check.");
	puts("  -t, --type <type>      Type of CIR references to check.");
	puts("  -v, --verbose          Verbose output.");
	puts("  -X, --xsl <file>       Custom XSLT for extracting CIR references.");
	puts("  -x, --xml              Output XML report.");
	puts("  -^, --remove-deleted   Validate with elements marked as \"delete\" removed.");
	puts("  --version              Show version information.");
	puts("  --zenity-progress      Print progress information in the zenity --progress format.");
	puts("  <object>               CSDB object(s) to check.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Show version information. */
static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

int main(int argc, char **argv)
{
	int i, err = 0;

	const char *sopts = "AaDd:FfLlNopqR:rTt:vX:x^h?";
	struct option lopts[] = {
		{"version"        , no_argument      , 0, 0},
		{"help"           , no_argument      , 0, 'h'},
		{"all-refs"       , no_argument      , 0, 'A'},
		{"all"            , no_argument      , 0, 'a'},
		{"dump-xsl"       , no_argument      , 0, 'D'},
		{"dir"            , required_argument, 0, 'd'},
		{"valid-filenames", no_argument      , 0, 'F'},
		{"filenames"      , no_argument      , 0, 'f'},
		{"list-refs"      , no_argument      , 0, 'L'},
		{"list"           , no_argument      , 0, 'l'},
		{"omit-issue"     , no_argument      , 0, 'N'},
		{"output-valid"   , no_argument      , 0, 'o'},
		{"progress"       , no_argument      , 0, 'p'},
		{"quiet"          , no_argument      , 0, 'q'},
		{"cir"            , required_argument, 0, 'R'},
		{"recursive"      , no_argument      , 0, 'r'},
		{"summary"        , no_argument      , 0, 'T'},
		{"type"           , required_argument, 0, 't'},
		{"verbose"        , no_argument      , 0, 'v'},
		{"xsl"            , required_argument, 0, 'X'},
		{"xml"            , no_argument      , 0, 'x'},
		{"remove-deleted" , no_argument      , 0, '^'},
		{"zenity-progress", no_argument      , 0, 0},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	struct opts opts;
	bool is_list = false;
	int show_progress = PROGRESS_OFF;
	bool find_cir = false;
	bool show_stats = false;
	bool xml_report = false;
	bool all_refs = false;
	bool dump_xsl = false;

	xmlDocPtr report_doc = NULL;

	/* Initialize program options. */
	opts.verbosity = NORMAL;
	opts.show_filenames = SHOW_NONE;
	opts.recursive = false;
	opts.no_issue = false;
	opts.search_all_objs = false;
	opts.output_valid = false;
	opts.list_refs = false;
	opts.rem_delete = false;
	opts.cir_refs_xsl = NULL;
	opts.type = NULL;

	init_objects(&opts.objects);
	init_objects(&opts.cirs);

	opts.search_dir = strdup(".");

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
				all_refs = true;
				break;
			case 'a':
				opts.search_all_objs = true;
				break;
			case 'D':
				dump_xsl = true;
				break;
			case 'd':
				free(opts.search_dir);
				opts.search_dir = strdup(optarg);
				break;
			case 'F':
				opts.show_filenames = SHOW_VALID;
				break;
			case 'f':
				opts.show_filenames = SHOW_INVALID;
				break;
			case 'L':
				opts.list_refs = true;
				break;
			case 'l':
				is_list = true;
				break;
			case 'N':
				opts.no_issue = true;
				break;
			case 'o':
				opts.output_valid = true;
				break;
			case 'p':
				show_progress = PROGRESS_CLI;
				break;
			case 'q':
				--opts.verbosity;
				break;
			case 'R':
				if (strcmp(optarg, "*") == 0) {
					find_cir = true;
				} else {
					add_object(&opts.cirs, optarg, &opts);
				}
				break;
			case 'r':
				opts.recursive = true;
				break;
			case 'T':
				show_stats = true;
				break;
			case 't':
				free(opts.type);
				opts.type = strdup(optarg);
				break;
			case 'v':
				++opts.verbosity;
				break;
			case 'X':
				free(opts.cir_refs_xsl);
				opts.cir_refs_xsl = read_xml_doc(optarg);
				break;
			case 'x':
				xml_report = true;
				break;
			case '^':
				opts.rem_delete = true;
				break;
			case 'h':
			case '?':
				show_help();
				goto cleanup;
		}
	}

	/* Load XSLT to extract CIR refs. */
	if (opts.cir_refs_xsl == NULL) {
		if (all_refs) {
			opts.cir_refs_xsl = read_xml_mem((const char *) xsl_cirrefsall_xsl, xsl_cirrefsall_xsl_len);
		} else {
			opts.cir_refs_xsl = read_xml_mem((const char *) xsl_cirrefs_xsl, xsl_cirrefs_xsl_len);
		}
	}

	/* Dump built-in XSLT if the -D option is specified. */
	if (dump_xsl) {
		save_xml_doc(opts.cir_refs_xsl, "-");
		goto cleanup;
	}

	/* Initialize the XML report if the -x option is specified. */
	if (xml_report || show_stats) {
		report_doc = xmlNewDoc(BAD_CAST "1.0");
		opts.report = xmlNewNode(NULL, BAD_CAST "repCheck");
		xmlDocSetRootElement(report_doc, opts.report);
	} else {
		opts.report = NULL;
	}

	/* Search for CIRs when -R* is specified. */
	if (find_cir) {
		find_cirs(&opts.cirs, opts.search_dir, &opts);

		extract_latest_cirs(&opts.cirs);

		/* Print the final CIR list in DEBUG mode. */
		if (opts.verbosity >= DEBUG) {
			int i;
			for (i = 0; i < opts.cirs.count; ++i) {
				fprintf(stderr, I_FIND_CIR_ADD, opts.cirs.paths[i]);
			}
		}
	}

	/* Read specified objects into a list in memory. */
	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (is_list) {
				add_object_list(&opts.objects, argv[i], &opts);
			} else {
				add_object(&opts.objects, argv[i], &opts);
			}
		}
	} else if (is_list) {
		add_object_list(&opts.objects, NULL, &opts);
	} else {
		add_object(&opts.objects, "-", &opts);
	}

	/* Check CIR references in the objects in the list. */
	for (i = 0; i < opts.objects.count; ++i) {
		if (check_cir_refs_in_file(opts.objects.paths[i], &opts) != 0) {
			err = 1;
		}

		switch (show_progress) {
			case PROGRESS_OFF:
				break;
			case PROGRESS_CLI:
				print_progress_bar(i, opts.objects.count);
				break;
			case PROGRESS_ZENITY:
				print_zenity_progress("Performing repository check...", i, opts.objects.count);
				break;
		}
	}

	if (opts.objects.count > 0) {
		switch (show_progress) {
			case PROGRESS_OFF:
				break;
			case PROGRESS_CLI:
				print_progress_bar(i, opts.objects.count);
				break;
			case PROGRESS_ZENITY:
				print_zenity_progress("Performing repository check...", i, opts.objects.count);
				break;
		}
	}

	if (xml_report) {
		save_xml_doc(report_doc, "-");
	}

	if (show_stats) {
		print_stats(report_doc);
	}

cleanup:
	free_objects(&opts.objects);
	free_objects(&opts.cirs);
	free(opts.search_dir);
	xmlFreeDoc(opts.cir_refs_xsl);
	free(opts.type);
	xmlFreeDoc(report_doc);

	xmlCleanupParser();
	xsltCleanupGlobals();

	return err;
}
