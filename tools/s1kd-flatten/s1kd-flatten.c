#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/transform.h>

#include "s1kd_tools.h"

#include "xsl.h"

#define PROG_NAME "s1kd-flatten"
#define VERSION "3.5.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define WRN_PREFIX PROG_NAME ": WARNING: "
#define INF_PREFIX PROG_NAME ": INFO: "
#define E_BAD_PM ERR_PREFIX "Bad publication module: %s\n"
#define E_ENCODING_ERROR ERR_PREFIX "An encoding error occurred: %s (%d)\n"
#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"
#define W_MISSING_REF WRN_PREFIX "Could not read referenced object: %s\n"
#define I_INCLUDE INF_PREFIX "Including %s...\n"
#define I_FOUND INF_PREFIX "Found %s\n"
#define I_REMOVE INF_PREFIX "Removing %s...\n"
#define I_SEARCH INF_PREFIX "Searching for %s in '%s' ...\n"
#define I_REMDUPS INF_PREFIX "Removing duplicate references...\n"
#define EXIT_BAD_PM 1
#define EXIT_ENCODING_ERROR 2

#define ENCODING_ERROR {\
	fprintf(stderr, "An encoding error occurred: %s (%d)\n", __FILE__, __LINE__);\
	exit(EXIT_ENCODING_ERROR);\
}

static int xinclude = 0;
static int no_issue = 0;
static int ignore_iss = 0;

static int use_pub_fmt = 0;
static xmlDocPtr pub_doc = NULL;
static xmlNodePtr pub;

static xmlNodePtr search_paths;
static char *search_dir;

static int flatten_ref = 1;
static int flatten_container = 0;
static int recursive = 0;
static int recursive_search = 0;
static int remove_unresolved = 0;
static int add_pmentry = 0;

static int only_pm_refs = 0;

static enum verbosity { QUIET, NORMAL, VERBOSE, DEBUG } verbosity = NORMAL;

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-d <dir>] [-I <path>] [-acDfilmNPpqRruvxh?] <pubmodule> [<dmodule>...]");
	puts("");
	puts("Options:");
	puts("  -a, --recursively-add-entries   Recursively flatten referenced PMs, adding a new PM entry for each.");
	puts("  -c, --containers                Flatten referenced container data modules.");
	puts("  -D, --remove                    Remove unresolved references.");
	puts("  -d, --dir <dir>                 Directory to start search in.");
	puts("  -f, --overwrite                 Overwrite publication module.");
	puts("  -h, -?, --help                  Show help/usage message.");
	puts("  -I, --include <path>            Search <path> for referenced objects.");
	puts("  -i, --ignore-issue              Always match the latest issue of an object found.");
	puts("  -l, --list                      Treat input as a list of objects.");
	puts("  -m, --modify                    Modiy references without flattening them.");
	puts("  -N, --omit-issue                Assume issue/inwork numbers are omitted.");
	puts("  -P, --only-pm-refs              Only flatten PM refs.");
	puts("  -p, --simple                    Output a simple, flat XML file.");
	puts("  -q, --quiet                     Quiet mode.");
	puts("  -R, --recursively               Recursively flatten referenced PMs.");
	puts("  -r, --recursive                 Search directories recursively.");
	puts("  -u, --unique                    Remove duplicate references.");
	puts("  -v, --verbose                   Verbose output.");
	puts("  -x, --use-xinclude              Use XInclude references.");
	puts("  --version  Show version information.");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

static xmlNodePtr find_child(xmlNodePtr parent, const char *child_name)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, child_name) == 0) {
			return cur;
		}
	}

	return NULL;
}

static xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const char *expr)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(doc ? doc : node->doc);
	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST expr, ctx);

	first = xmlXPathNodeSetIsEmpty(obj->nodesetval) ? NULL : obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

static char *first_xpath_string(xmlDocPtr doc, xmlNodePtr node, const char *expr)
{
	return (char *) xmlNodeGetContent(first_xpath_node(doc, node, expr));
}

static void flatten_pm_entry(xmlNodePtr pm_entry, xmlNsPtr xiNs);

static void flatten_pm_ref(xmlNodePtr pm_ref, xmlNsPtr xiNs)
{
	xmlNodePtr pm_code;
	xmlNodePtr issue_info;
	xmlNodePtr language;

	char *model_ident_code;
	char *pm_issuer;
	char *pm_number;
	char *pm_volume;
	char *issue_number = NULL;
	char *in_work = NULL;
	char *language_iso_code = NULL;
	char *country_iso_code = NULL;

	char pmc[256];
	char pm_fname[PATH_MAX];
	char pm_fname_temp[PATH_MAX];
	char fs_pm_fname[PATH_MAX] = "";

	xmlNodePtr xi;
	xmlDocPtr doc;
	xmlNodePtr pm;

	bool found = false;
	xmlNodePtr cur;

	/* Skip PM refs if they do not need to be processed. */
	if (!(flatten_ref || remove_unresolved || recursive)) {
		return;
	}

	pm_code = first_xpath_node(NULL, pm_ref, ".//pmCode|.//pmc");
	issue_info = ignore_iss ? NULL : first_xpath_node(NULL, pm_ref, ".//issueInfo|.//issno");
	language = first_xpath_node(NULL, pm_ref, ".//language");

	model_ident_code = first_xpath_string(NULL, pm_code, "@modelIdentCode|modelic");
	pm_issuer        = first_xpath_string(NULL, pm_code, "@pmIssuer|pmissuer");
	pm_number        = first_xpath_string(NULL, pm_code, "@pmNumber|pmnumber");
	pm_volume        = first_xpath_string(NULL, pm_code, "@pmVolume|pmvolume");

	snprintf(pmc, 256, "%s-%s-%s-%s",
		model_ident_code,
		pm_issuer,
		pm_number,
		pm_volume);

	snprintf(pm_fname, PATH_MAX, "PMC-%s", pmc);

	if (!no_issue) {
		strcpy(pm_fname_temp, pm_fname);

		if (issue_info) {
			issue_number = first_xpath_string(NULL, issue_info, "@issueNumber|@issno");
			in_work      = first_xpath_string(NULL, issue_info, "@inWork|@inwork");

			if (snprintf(pm_fname, PATH_MAX, "%s_%s-%s", pm_fname_temp, issue_number, in_work ? in_work : "00") < 0) {
				ENCODING_ERROR
			}
		} else if (language) {
			if (snprintf(pm_fname, PATH_MAX, "%s_\?\?\?-\?\?", pm_fname_temp) < 0) {
				ENCODING_ERROR
			}
		}
	}

	if (language) {
		int i;

		language_iso_code = first_xpath_string(NULL, language, "@languageIsoCode|@language");
		country_iso_code  = first_xpath_string(NULL, language, "@countryIsoCode|@country");

		for (i = 0; language_iso_code[i]; ++i)
			language_iso_code[i] = toupper(language_iso_code[i]);
		strcpy(pm_fname_temp, pm_fname);

		if (snprintf(pm_fname, PATH_MAX, "%s_%s-%s", pm_fname_temp, language_iso_code, country_iso_code) < 0) {
			ENCODING_ERROR
		}
	}

	xmlFree(model_ident_code);
	xmlFree(pm_issuer);
	xmlFree(pm_number);
	xmlFree(pm_volume);
	xmlFree(issue_number);
	xmlFree(in_work);
	xmlFree(language_iso_code);
	xmlFree(country_iso_code);

	for (cur = search_paths->children; cur && !found; cur = cur->next) {
		char *path;

		path = (char *) xmlNodeGetContent(cur);

		if (verbosity >= DEBUG) {
			fprintf(stderr, I_SEARCH, pm_fname, path);
		}

		if (find_csdb_object(fs_pm_fname, path, pm_fname, is_pm, recursive_search)) {
			if (verbosity >= DEBUG) {
				fprintf(stderr, I_FOUND, fs_pm_fname);
			}

			found = true;

			if (recursive) {
				xmlDocPtr subpm;
				xmlNodePtr pmentry = NULL;
				xmlNodePtr content;

				if (verbosity >= VERBOSE) {
					fprintf(stderr, I_INCLUDE, fs_pm_fname);
				}

				subpm = read_xml_doc(fs_pm_fname);

				/* Create a new PM entry to contain the contents of the referenced PM. */
				if (add_pmentry) {
					xmlChar *title;

					pmentry = xmlNewNode(NULL, BAD_CAST "pmEntry");

					title = xpath_first_value(subpm, NULL, BAD_CAST "//pmTitle");
					xmlNewChild(pmentry, NULL, BAD_CAST "pmEntryTitle", title);
					xmlFree(title);

					pmentry = xmlAddNextSibling(pm_ref, pmentry);
				}

				content = first_xpath_node(subpm, NULL, "//content");

				if (content) {
					xmlNodePtr c;

					flatten_pm_entry(content, xiNs);

					/* Add the contents to the new PM entry. */
					if (add_pmentry) {
						for (c = content->children; c; c = c->next) {
							if (xmlStrcmp(c->name, BAD_CAST "pmEntry") != 0) {
								continue;
							}

							xmlAddChild(pmentry, xmlCopyNode(c, 1));
						}
					/* Or, add the contents directly to the main PM. */
					} else {
						for (c = content->last; c; c = c->prev) {
							if (xmlStrcmp(c->name, BAD_CAST "pmEntry") != 0) {
								continue;
							}

							xmlAddNextSibling(pm_ref, xmlCopyNode(c, 1));
						}
					}
				}

				xmlFreeDoc(subpm);
			} else if (flatten_ref) {
				if (verbosity >= VERBOSE) {
					fprintf(stderr, I_INCLUDE, fs_pm_fname);
				}

				if (xinclude) {
					xi = xmlNewNode(xiNs, BAD_CAST "include");
					xmlSetProp(xi, BAD_CAST "href", BAD_CAST fs_pm_fname);

					if (use_pub_fmt) {
						xi = xmlAddChild(pub, xi);
					} else {
						xi = xmlAddPrevSibling(pm_ref, xi);
					}
				} else {
					doc = read_xml_doc(fs_pm_fname);
					pm = xmlDocGetRootElement(doc);
					xmlAddPrevSibling(pm_ref, xmlCopyNode(pm, 1));
					xmlFreeDoc(doc);
				}
			}
		} else if (remove_unresolved) {
			if (verbosity >= VERBOSE) {
				fprintf(stderr, I_REMOVE, pm_fname);
			}
		} else {
			if (verbosity >= NORMAL) {
				fprintf(stderr, W_MISSING_REF, pm_fname);
			}
		}

		xmlFree(path);
	}

	if ((found && (flatten_ref || recursive)) || (!found && remove_unresolved)) {
		xmlUnlinkNode(pm_ref);
		xmlFreeNode(pm_ref);
	}
}

static void flatten_dm_ref(xmlNodePtr dm_ref, xmlNsPtr xiNs)
{
	xmlNodePtr dm_code;
	xmlNodePtr issue_info;
	xmlNodePtr language;

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
	char *issue_number = NULL;
	char *in_work = NULL;
	char *language_iso_code = NULL;
	char *country_iso_code = NULL;

	char dmc[256];
	char dm_fname[PATH_MAX];
	char dm_fname_temp[PATH_MAX];
	char fs_dm_fname[PATH_MAX] = "";

	xmlNodePtr xi;
	xmlDocPtr doc;
	xmlNodePtr dmodule;

	bool found = false;
	xmlNodePtr cur;

	/* Skip DM refs if they do not need to be processed. */
	if (only_pm_refs || !(flatten_ref || remove_unresolved || flatten_container)) {
		return;
	}

	dm_code    = first_xpath_node(NULL, dm_ref, ".//dmCode|.//avee");
	issue_info = ignore_iss ? NULL : first_xpath_node(NULL, dm_ref, ".//issueInfo|.//issno");
	language   = first_xpath_node(NULL, dm_ref, ".//language");

	model_ident_code     = first_xpath_string(NULL, dm_code, "@modelIdentCode|modelic");
	system_diff_code     = first_xpath_string(NULL, dm_code, "@systemDiffCode|sdc");
	system_code          = first_xpath_string(NULL, dm_code, "@systemCode|chapnum");
	sub_system_code      = first_xpath_string(NULL, dm_code, "@subSystemCode|section");
	sub_sub_system_code  = first_xpath_string(NULL, dm_code, "@subSubSystemCode|subsect");
	assy_code            = first_xpath_string(NULL, dm_code, "@assyCode|subject");
	disassy_code         = first_xpath_string(NULL, dm_code, "@disassyCode|discode");
	disassy_code_variant = first_xpath_string(NULL, dm_code, "@disassyCodeVariant|discodev");
	info_code            = first_xpath_string(NULL, dm_code, "@infoCode|incode");
	info_code_variant    = first_xpath_string(NULL, dm_code, "@infoCodeVariant|incodev");
	item_location_code   = first_xpath_string(NULL, dm_code, "@itemLocationCode|itemloc");

	snprintf(dmc, 256, "%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
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

	snprintf(dm_fname, PATH_MAX, "DMC-%s", dmc);

	if (!no_issue) {
		strcpy(dm_fname_temp, dm_fname);

		if (issue_info) {
			issue_number = first_xpath_string(NULL, issue_info, "@issueNumber|@issno");
			in_work      = first_xpath_string(NULL, issue_info, "@inWork|@inwork");

			if (snprintf(dm_fname, PATH_MAX, "%s_%s-%s", dm_fname_temp, issue_number, in_work ? in_work : "00") < 0) {
				ENCODING_ERROR
			}
		} else if (language) {
			if (snprintf(dm_fname, PATH_MAX, "%s_\?\?\?-\?\?", dm_fname_temp) < 0) {
				ENCODING_ERROR
			}
		}
	}

	if (language) {
		int i;

		language_iso_code = first_xpath_string(NULL, language, "@languageIsoCode|@language");
		country_iso_code  = first_xpath_string(NULL, language, "@countryIsoCode|@country");
	
		for (i = 0; language_iso_code[i]; ++i)
			language_iso_code[i] = toupper(language_iso_code[i]);
		strcpy(dm_fname_temp, dm_fname);

		if (snprintf(dm_fname, PATH_MAX, "%s_%s-%s", dm_fname_temp, language_iso_code, country_iso_code) < 0) {
			ENCODING_ERROR
		}
	}

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
	xmlFree(issue_number);
	xmlFree(in_work);
	xmlFree(language_iso_code);
	xmlFree(country_iso_code);

	for (cur = search_paths->children; cur && !found; cur = cur->next) {
		char *path;

		path = (char *) xmlNodeGetContent(cur);

		if (verbosity >= DEBUG) {
			fprintf(stderr, I_SEARCH, dm_fname, path);
		}

		if (find_csdb_object(fs_dm_fname, path, dm_fname, is_dm, recursive_search)) {
			if (verbosity >= DEBUG) {
				fprintf(stderr, I_FOUND, fs_dm_fname);
			}

			found = true;

			/* Flatten a container data module by copying the
			 * dmRefs inside the container directly in to the
			 * publication module.
			 */
			if (flatten_container) {
				xmlDocPtr doc;
				xmlNodePtr refs;

				doc = read_xml_doc(fs_dm_fname);
				refs = first_xpath_node(doc, NULL, "//container/refs");

				if (refs) {
					xmlNodePtr c;

					/* First, flatten the dmRefs in the
					 * container itself. */
					flatten_pm_entry(refs, xiNs);

					/* Copy each dmRef from the container
					 * into the PM. */
					for (c = refs->last; c; c = c->prev) {
						if (c->type != XML_ELEMENT_NODE) {
							continue;
						}
						xmlAddNextSibling(dm_ref, xmlCopyNode(c, 1));
					}
				}

				xmlFreeDoc(doc);
			}

			if (flatten_ref) {
				if (verbosity >= VERBOSE) {
					fprintf(stderr, I_INCLUDE, fs_dm_fname);
				}

				if (xinclude) {
					xi = xmlNewNode(xiNs, BAD_CAST "include");
					xmlSetProp(xi, BAD_CAST "href", BAD_CAST fs_dm_fname);

					if (use_pub_fmt) {
						xi = xmlAddChild(pub, xi);
					} else {
						xi = xmlAddPrevSibling(dm_ref, xi);
					}
				} else {
					xmlChar *app;
					doc = read_xml_doc(fs_dm_fname);
					dmodule = xmlDocGetRootElement(doc);
					if ((app = xmlGetProp(dm_ref, BAD_CAST "applicRefId"))) {
						xmlSetProp(dmodule, BAD_CAST "applicRefId", app);
					}
					xmlFree(app);
					xmlAddPrevSibling(dm_ref, xmlCopyNode(dmodule, 1));
					xmlFreeDoc(doc);
				}
			}
		} else if (remove_unresolved) {
			if (verbosity >= VERBOSE) {
				fprintf(stderr, I_REMOVE, dm_fname);
			}
		} else {
			if (verbosity >= NORMAL) {
				fprintf(stderr, W_MISSING_REF, dm_fname);
			}
		}

		xmlFree(path);
	}

	if ((found && flatten_ref) || (!found && remove_unresolved)) {
		xmlUnlinkNode(dm_ref);
		xmlFreeNode(dm_ref);
	}
}

static void flatten_pm_entry(xmlNodePtr pm_entry, xmlNsPtr xiNs)
{
	xmlNodePtr cur, next;

	cur = pm_entry->children;

	while (cur) {
		next = cur->next;

		if (xmlStrcmp(cur->name, BAD_CAST "dmRef") == 0 || xmlStrcmp(cur->name, BAD_CAST "refdm") == 0) {
			flatten_dm_ref(cur, xiNs);
		} else if (xmlStrcmp(cur->name, BAD_CAST "pmRef") == 0 || xmlStrcmp(cur->name, BAD_CAST "refpm") == 0) {
			flatten_pm_ref(cur, xiNs);
		} else if (xmlStrcmp(cur->name, BAD_CAST "pmEntry") == 0 || xmlStrcmp(cur->name, BAD_CAST "pmentry") == 0) {
			flatten_pm_entry(cur, xiNs);
		}

		cur = next;
	}

	if (xmlChildElementCount(pm_entry) == 0 ||
	    xmlStrcmp((cur = xmlLastElementChild(pm_entry))->name, BAD_CAST "pmEntryTitle") == 0 ||
	    xmlStrcmp(cur->name, BAD_CAST "title") == 0) {
		xmlUnlinkNode(pm_entry);
		xmlFreeNode(pm_entry);
	}
}

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

static void remove_dup_refs(xmlDocPtr pm)
{
	const char *params[5];

	params[0] = "INF_PREFIX";
	params[1] = "\"" INF_PREFIX "\"";
	params[2] = "verbosity";
	switch (verbosity) {
		case QUIET:	params[3] = "0"; break;
		case NORMAL:	params[3] = "1"; break;
		case VERBOSE:	params[3] = "2"; break;
		case DEBUG:	params[3] = "3"; break;
	}
	params[4] = NULL;

	if (verbosity >= VERBOSE) {
		fprintf(stderr, I_REMDUPS);
	}

	transform_doc(pm, xsl_remdups1_xsl, xsl_remdups1_xsl_len, NULL);
	transform_doc(pm, xsl_remdups2_xsl, xsl_remdups2_xsl_len, params);
	transform_doc(pm, ___common_remove_empty_pmentries_xsl, ___common_remove_empty_pmentries_xsl_len, NULL);
}

static void flatten_file(xmlNodePtr pub, xmlNsPtr xiNs, const char *fname)
{
	if (xinclude) {
		xmlNodePtr xi;
		xi = xmlNewChild(pub, xiNs, BAD_CAST "include", NULL);
		xmlSetProp(xi, BAD_CAST "href", BAD_CAST fname);
	} else {
		xmlDocPtr doc;
		doc = read_xml_doc(fname);
		xmlAddChild(pub, xmlCopyNode(xmlDocGetRootElement(doc), 1));
		xmlFreeDoc(doc);
	}
}

static void flatten_list(const char *path, xmlNodePtr pub, xmlNsPtr xiNs)
{
	FILE *f = NULL;
	char line[PATH_MAX];

	if (path) {
		if (!(fopen(path, "r"))) {
			if (verbosity >= NORMAL) {
				fprintf(stderr, E_BAD_LIST, path);
			}

			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");
		flatten_file(pub, xiNs, line);
	}

	if (path) {
		fclose(f);
	}
}

int main(int argc, char **argv)
{
	int c;

	char *pm_fname = NULL;

	xmlNodePtr pm;
	xmlNodePtr content;

	xmlNodePtr cur;

	const char *sopts = "acDd:fxmNPpqRruvI:ilh?";
	struct option lopts[] = {
		{"version"                , no_argument      , 0, 0},
		{"recursively-add-entries", no_argument      , 0, 'a'},
		{"help"                   , no_argument      , 0, 'h'},
		{"containers"             , no_argument      , 0, 'c'},
		{"remove"                 , no_argument      , 0, 'D'},
		{"dir"                    , required_argument, 0, 'd'},
		{"overwrite"              , no_argument      , 0, 'f'},
		{"use-xinclude"           , no_argument      , 0, 'x'},
		{"modify"                 , no_argument      , 0, 'm'},
		{"omit-issue"             , no_argument      , 0, 'N'},
		{"only-pm-refs"           , no_argument      , 0, 'P'},
		{"simple"                 , no_argument      , 0, 'p'},
		{"quiet"                  , no_argument      , 0, 'q'},
		{"recursively"            , no_argument      , 0, 'R'},
		{"recursive"              , no_argument      , 0, 'r'},
		{"unique"                 , no_argument      , 0, 'u'},
		{"verbose"                , no_argument      , 0, 'v'},
		{"include"                , required_argument, 0, 'I'},
		{"ignore-issue"           , no_argument      , 0, 'i'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	int overwrite = 0;
	int remove_dups = 0;
	bool is_list = false;

	xmlNsPtr xiNs = NULL;

	search_paths = xmlNewNode(NULL, BAD_CAST "searchPaths");
	search_dir = strdup(".");

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg)
				break;
			case 'a': recursive = 1; add_pmentry = 1; break;
			case 'c': flatten_container = 1; break;
			case 'D': remove_unresolved = 1; break;
			case 'd': free(search_dir); search_dir = strdup(optarg); break;
			case 'f': overwrite = 1; break;
			case 'x': xinclude = 1; break;
			case 'm': flatten_ref = 0; break;
			case 'N': no_issue = 1; break;
			case 'P': only_pm_refs = 1; break;
			case 'p': use_pub_fmt = 1; break;
			case 'q': --verbosity; break;
			case 'R': recursive = 1; break;
			case 'r': recursive_search = 1; break;
			case 'u': remove_dups = 1; break;
			case 'v': ++verbosity; break;
			case 'I': xmlNewChild(search_paths, NULL, BAD_CAST "path", BAD_CAST optarg); break;
			case 'i': ignore_iss = 1; break;
			case 'l': is_list = true; use_pub_fmt = 1; break;
			case 'h':
			case '?': show_help(); exit(0);
		}
	}

	xmlNewChild(search_paths, NULL, BAD_CAST "path", BAD_CAST search_dir);
	free(search_dir);

	if (use_pub_fmt) {
		pub_doc = xmlNewDoc(BAD_CAST "1.0");
		pub = xmlNewNode(NULL, BAD_CAST "publication");
		xmlDocSetRootElement(pub_doc, pub);

		if (optind < argc) {
			int i;

			for (i = optind; i < argc; ++i) {
				if (is_list) {
					flatten_list(argv[i], pub, xiNs);
				} else {
					flatten_file(pub, xiNs, argv[i]);
				}
			}
		} else if (is_list) {
			flatten_list(NULL, pub, xiNs);
		} else {
			flatten_file(pub, xiNs, "-");
		}
	} else {
		if (optind < argc) {
			pm_fname = argv[optind];
		} else {
			pm_fname = "-";
		}

		if (!(pub_doc = read_xml_doc(pm_fname))) {
			fprintf(stderr, E_BAD_PM, pm_fname);
			exit(EXIT_BAD_PM);
		}

		pm = xmlDocGetRootElement(pub_doc);
		content = find_child(pm, "content");

		if (content) {
			if (xinclude) {
				xiNs = xmlNewNs(pm, BAD_CAST "http://www.w3.org/2001/XInclude", BAD_CAST "xi");
			}
		} else {
			fprintf(stderr, E_BAD_PM, pm_fname);
			exit(EXIT_BAD_PM);
		}

		cur = content->children;

		while (cur) {
			xmlNodePtr next;

			next = cur->next;

			if (xmlStrcmp(cur->name, BAD_CAST "pmEntry") == 0 || xmlStrcmp(cur->name, BAD_CAST "pmentry") == 0) {
				flatten_pm_entry(cur, xiNs);
			}

			cur = next;
		}
	}

	/* Remove duplicate entries from the flattened PM. */
	if (remove_dups) {
		remove_dup_refs(pub_doc);
	}

	save_xml_doc(pub_doc, (overwrite && !use_pub_fmt) ? pm_fname : "-");

	xmlFreeNode(search_paths);
	xmlFreeDoc(pub_doc);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
