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
#define VERSION "0.4.0"

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

/* Verbosity of messages. */
enum verbosity { QUIET, NORMAL, VERBOSE, DEBUG };

/* List of CSDB objects. */
struct objects {
	char (*paths)[PATH_MAX];
	unsigned count;
	unsigned max;
};

/* Program options. */
struct opts {
	enum verbosity verbosity;
	bool show_filenames;
	char *search_dir;
	bool recursive;
	bool no_issue;
	bool search_all_objs;
	bool output_valid;
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
static void add_ref_to_report(xmlNodePtr rpt, xmlNodePtr ref, long int lineno, const char *cir, struct opts *opts)
{
	xmlNodePtr node;
	xmlChar line_s[16], *xpath;

	node = xmlAddChild(rpt, xmlCopyNode(ref, 1));
	xmlUnsetProp(node, BAD_CAST "name");
	xmlUnsetProp(node, BAD_CAST "test");

	xmlStrPrintf(line_s, 16, "%ld", lineno);
	xmlSetProp(node, BAD_CAST "line", BAD_CAST line_s);

	xpath = xpath_of(ref);
	xmlSetProp(node, BAD_CAST "xpath", xpath);
	xmlFree(xpath);

	if (cir) {
		xmlSetProp(node, BAD_CAST "cir", BAD_CAST cir);
	}
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

/* Check a specific CIR reference. */
static int check_cir_ref(xmlNodePtr ref, const char *path, xmlNodePtr rpt, struct opts *opts)
{
	int i, err = 0;
	xmlChar *ident, *xpath;
	long int lineno;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	lineno = xmlGetLineNo(ref);

	ident = xmlGetProp(ref, BAD_CAST "repcheck_name");
	xpath = xmlGetProp(ref, BAD_CAST "repcheck_test");

	/* Check if there is an explicit CIR reference. */
	ctx = xmlXPathNewContext(ref->doc);
	xmlXPathSetContextNode(ref, ctx);
	obj = xmlXPathEvalExpression(BAD_CAST "refs/dmRef/dmRefIdent", ctx);

	/* If there is not, use any of the specified/found CIRs. */
	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		/* Search in all CIRs. */
		for (i = 0; i < opts->cirs.count; ++i) {
			if (find_ref_in_cir(ref, ident, xpath, opts->cirs.paths[i], opts)) {
				add_ref_to_report(rpt, ref, lineno, opts->cirs.paths[i], opts);
				goto done;
			}
		}

		/* Search in all other specified objects, if allowed. */
		if (opts->search_all_objs) {
			for (i = 0; i < opts->objects.count; ++i) {
				if (find_ref_in_cir(ref, ident, xpath, opts->objects.paths[i], opts)) {
					add_ref_to_report(rpt, ref, lineno, opts->objects.paths[i], opts);
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
					add_ref_to_report(rpt, ref, lineno, fname, opts);
					goto done;
				}
			}
		}
	}

	if (opts->verbosity >= NORMAL) {
		fprintf(stderr, E_NOT_FOUND, path, lineno, ident);
	}
	add_ref_to_report(rpt, ref, lineno, NULL, opts);
	err = 1;

done:
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
	xmlFree(ident);
	xmlFree(xpath);

	return err;
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
	rpt = xmlNewChild(opts->report, NULL, BAD_CAST "object", NULL);
	xmlSetProp(rpt, BAD_CAST "path", BAD_CAST path);

	styledoc = read_xml_mem((const char *) xsl_cirrefs_xsl, xsl_cirrefs_xsl_len);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);

	ctx = xmlXPathNewContext(res);
	obj = xmlXPathEvalExpression(BAD_CAST "//*[@repcheck_test]", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			if (check_cir_ref(obj->nodesetval->nodeTab[i], path, rpt, opts) != 0) {
				err = 1;
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	xmlFreeDoc(res);
	xsltFreeStylesheet(style);

	if (err) {
		xmlSetProp(rpt, BAD_CAST "valid", BAD_CAST "no");
	} else {
		xmlSetProp(rpt, BAD_CAST "valid", BAD_CAST "yes");
	}

	return err;
}

/* Check all CIR references in the specified CSDB object. */
static int check_cir_refs_in_file(const char *path, struct opts *opts)
{
	xmlDocPtr doc;
	int err = 0;

	if (opts->verbosity >= DEBUG) {
		fprintf(stderr, I_CHECK, path);
	}

	if (!(doc = read_xml_doc(path))) {
		return 1;
	}

	err = check_cir_refs(doc, path, opts);

	if (opts->verbosity >= VERBOSE) {
		if (err) {
			fprintf(stderr, F_INVALID, path);
		} else {
			fprintf(stderr, S_VALID, path);
		}
	}

	if (err) {
		if (opts->show_filenames) {
			puts(path);
		}
	} else {
		if (opts->output_valid) {
			save_xml_doc(doc, "-");
		}
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

/* Determine if a CSDB object is a CIR. */
static bool is_cir(const char *path)
{
	xmlDocPtr doc;
	bool is;

	if (!(doc = read_xml_doc(path))) {
		return false;
	}

	is = xpath_first_node(doc, NULL, BAD_CAST "//commonRepository") != NULL;

	xmlFreeDoc(doc);

	return is;
}

/* Find CIRs in directories and add them to the list. */
static void find_cirs(struct objects *cirs, char *search_dir, struct opts *opts)
{
	DIR *dir;
	struct dirent *cur;
	char fpath[PATH_MAX], cpath[PATH_MAX];
	struct objects latest;

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
		} else if (is_dm(cur->d_name) && is_cir(cpath)) {
			if (opts->verbosity >= DEBUG) {
				fprintf(stderr, I_FIND_CIR_FOUND, cpath);
			}
			add_object(cirs, cpath, opts);
		}
	}

	closedir(dir);

	/* Use only the latest issue of a CIR. */
	qsort(cirs->paths, cirs->count, PATH_MAX, compare_basename);
	latest.paths = malloc(cirs->count * PATH_MAX);
	latest.count = extract_latest_csdb_objects(latest.paths, cirs->paths, cirs->count);
	free(cirs->paths);
	cirs->paths = latest.paths;
	cirs->count = latest.count;

	/* Print the final CIR list in DEBUG mode. */
	if (opts->verbosity >= DEBUG) {
		int i;
		for (i = 0; i < cirs->count; ++i) {
			fprintf(stderr, I_FIND_CIR_ADD, cirs->paths[i]);
		}
	}
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
	puts("  -a, --all           Resolve against CIRs specified as objects to check.");
	puts("  -d, --dir <dir>     Search for CIRs in <dir>.");
	puts("  -f, --filenames     List invalid files.");
	puts("  -h, -?, --help      Show help/usage message.");
	puts("  -l, --list          Treat input as list of CSDB objects.");
	puts("  -N, --omit-issue    Assume issue/inwork numbers are omitted.");
	puts("  -o, --output-valid  Output valid CSDB objects to stdout.");
	puts("  -p, --progress      Display a progress bar.");
	puts("  -q, --quiet         Quiet mode.");
	puts("  -R, --cir <CIR>     Check references against the given CIR.");
	puts("  -r, --recursive     Search for CIRs recursively.");
	puts("  -T, --summary       Print a summary of the check.");
	puts("  -v, --verbose       Verbose output.");
	puts("  -x, --xml           Output XML report.");
	puts("  --version           Show version information.");
	puts("  <object>            CSDB object(s) to check.");
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

	const char *sopts = "ad:flNopqR:rTvxh?";
	struct option lopts[] = {
		{"version"     , no_argument      , 0, 0},
		{"help"        , no_argument      , 0, 'h'},
		{"all"         , no_argument      , 0, 'a'},
		{"dir"         , required_argument, 0, 'd'},
		{"filenames"   , no_argument      , 0, 'f'},
		{"list"        , no_argument      , 0, 'l'},
		{"omit-issue"  , no_argument      , 0, 'N'},
		{"output-valid", no_argument      , 0, 'o'},
		{"progress"    , no_argument      , 0, 'p'},
		{"quiet"       , no_argument      , 0, 'q'},
		{"cir"         , required_argument, 0, 'R'},
		{"recursive"   , no_argument      , 0, 'r'},
		{"summary"     , no_argument      , 0, 'T'},
		{"verbose"     , no_argument      , 0, 'v'},
		{"xml"         , no_argument      , 0, 'x'},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	struct opts opts;
	bool is_list = false;
	bool show_progress = false;
	bool find_cir = false;
	bool xml_report = false;
	bool show_stats = false;

	xmlDocPtr report_doc;

	/* Initialize program options. */
	opts.verbosity = NORMAL;
	opts.show_filenames = false;
	opts.recursive = false;
	opts.no_issue = false;
	opts.search_all_objs = false;
	opts.output_valid = false;

	init_objects(&opts.objects);
	init_objects(&opts.cirs);

	report_doc = xmlNewDoc(BAD_CAST "1.0");
	opts.report = xmlNewNode(NULL, BAD_CAST "repCheck");
	xmlDocSetRootElement(report_doc, opts.report);

	opts.search_dir = strdup(".");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					goto cleanup;
				}
				break;
			case 'a':
				opts.search_all_objs = true;
				break;
			case 'd':
				free(opts.search_dir);
				opts.search_dir = strdup(optarg);
				break;
			case 'f':
				opts.show_filenames = true;
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
				show_progress = true;
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
			case 'v':
				++opts.verbosity;
				break;
			case 'x':
				xml_report = true;
				break;
			case 'h':
			case '?':
				show_help();
				goto cleanup;
		}
	}

	if (find_cir) {
		if (opts.verbosity >= DEBUG) {
			fprintf(stderr, I_FIND_CIR, opts.search_dir);
		}
		find_cirs(&opts.cirs, opts.search_dir, &opts);
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

		if (show_progress) {
			print_progress_bar(i, opts.objects.count);
		}
	}

	if (show_progress && opts.objects.count > 0) {
		print_progress_bar(i, opts.objects.count);
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
	xmlFreeDoc(report_doc);

	xmlCleanupParser();
	xsltCleanupGlobals();

	return err;
}
