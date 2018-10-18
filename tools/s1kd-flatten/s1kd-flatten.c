#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-flatten"
#define VERSION "2.0.0"

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define E_BAD_PM ERR_PREFIX "Bad publication module: %s\n"
#define EXIT_BAD_PM 1
#define EXIT_BAD_CODE 2

int xinclude = 0;
int no_issue = 0;

int use_pub_fmt = 0;
xmlDocPtr pub_doc = NULL;
xmlNodePtr pub;

xmlNodePtr search_paths;
char *search_dir;

int flatten_ref = 1;
int flatten_container = 0;
int recursive = 0;
int recursive_search = 0;

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-d <dir>] [-I <path>] [-cDfNpRrxh?] <pubmodule> [<dmodule>...]");
	puts("");
	puts("Options:");
	puts("  -c         Flatten referenced container data modules.");
	puts("  -D         Remove unresolved refs without flattening.");
	puts("  -d <dir>   Directory to start search in.");
	puts("  -f         Overwrite publication module.");
	puts("  -I <path>  Search <path> for referenced objects.");
	puts("  -N         Assume issue/inwork numbers are omitted.");
	puts("  -p         Output a 'publication' XML file.");
	puts("  -R         Recursively flatten referenced PMs.");
	puts("  -r         Search directories recursively.");
	puts("  -x         Use XInclude references.");
	puts("  -h -?      Show help/usage message.");
	puts("  --version  Show version information.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

xmlNodePtr find_child(xmlNodePtr parent, const char *child_name)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, child_name) == 0) {
			return cur;
		}
	}

	return NULL;
}

xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const char *expr)
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

char *first_xpath_string(xmlDocPtr doc, xmlNodePtr node, const char *expr)
{
	return (char *) xmlNodeGetContent(first_xpath_node(doc, node, expr));
}

bool is_dm(const char *fname)
{
	return strncmp(fname, "DMC-", 4) == 0 && strncasecmp(fname + (strlen(fname) - 4), ".XML", 4) == 0;
}

bool is_pm(const char *fname)
{
	return strncmp(fname, "PMC-", 4) == 0 && strncasecmp(fname + (strlen(fname) - 4), ".XML", 4) == 0;
}

bool is_dir(const char *path)
{
	struct stat st;
	char s[PATH_MAX], *b;

	strcpy(s, path);
	b = basename(s);

	if (strcmp(b, ".") == 0 || strcmp(b, "..") == 0) {
		return 0;
	}

	stat(path, &st);
	return S_ISDIR(st.st_mode);
}

int codecmp(const char *p1, const char *p2)
{
	char s1[PATH_MAX], s2[PATH_MAX], *b1, *b2;

	strcpy(s1, p1);
	strcpy(s2, p2);

	b1 = basename(s1);
	b2 = basename(s2);

	return strcasecmp(b1, b2);
}

bool filesystem_fname(char *fs_fname, const char *fname, const char *path, bool (*is)(const char *))
{
	DIR *dir;
	struct dirent *cur;
	int fname_len;
	bool found = false;
	int len = strlen(path);
	char fpath[PATH_MAX], cpath[PATH_MAX];

	if (!is_dir(path)) {
		return false;
	}

	if (strcmp(path, ".") == 0) {
		strcpy(fpath, "");
	} else if (path[len - 1] != '/') {
		strcpy(fpath, path);
		strcat(fpath, "/");
	} else {
		strcpy(fpath, path);
	}

	fname_len = strlen(fname);

	if (!(dir = opendir(path))) {
		return false;
	}

	while ((cur = readdir(dir))) {
		strcpy(cpath, fpath);
		strcat(cpath, cur->d_name);

		if (recursive_search && is_dir(cpath)) {
			char tmp[PATH_MAX];

			if (filesystem_fname(tmp, fname, cpath, is) && (!found || codecmp(tmp, fs_fname) > 0)) {
				strcpy(fs_fname, tmp);
				found = true;
			}
		} else if (is(cur->d_name) && strncmp(cur->d_name, fname, fname_len) == 0) {
			if (!found || codecmp(cpath, fs_fname) > 0) {
				strcpy(fs_fname, cpath);
				found = true;
			}
		}
	}

	closedir(dir);

	return found;
}

void flatten_pm_entry(xmlNodePtr pm_entry);

void flatten_pm_ref(xmlNodePtr pm_ref)
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

	pm_code = first_xpath_node(NULL, pm_ref, ".//pmCode|.//pmc");
	issue_info = first_xpath_node(NULL, pm_ref, ".//issueInfo|.//issno");
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

	if (issue_info && !no_issue) {
		issue_number = first_xpath_string(NULL, issue_info, "@issueNumber|@issno");
		in_work      = first_xpath_string(NULL, issue_info, "@inWork|@inwork");
		strcpy(pm_fname_temp, pm_fname);

		if (snprintf(pm_fname, PATH_MAX, "%s_%s-%s", pm_fname_temp, issue_number, in_work ? in_work : "00") < 0) {
			exit(EXIT_BAD_CODE);
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
			exit(EXIT_BAD_CODE);
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

		if (filesystem_fname(fs_pm_fname, pm_fname, path, is_pm)) {
			found = true;

			if (recursive) {
				xmlDocPtr subpm;
				xmlNodePtr content;

				subpm = xmlReadFile(fs_pm_fname, NULL, PARSE_OPTS);
				content = first_xpath_node(subpm, NULL, "//content");

				if (content) {
					xmlNodePtr c;

					flatten_pm_entry(content);

					for (c = content->children; c; c = c->next) {
						if (xmlStrcmp(c->name, BAD_CAST "pmEntry") != 0) {
							continue;
						}
						xmlAddNextSibling(pm_ref, xmlCopyNode(c, 1));
					}
				}

				xmlFreeDoc(subpm);
			} else if (flatten_ref) {
				if (xinclude) {
					xi = xmlNewNode(NULL, BAD_CAST "xi:include");
					xmlSetProp(xi, BAD_CAST "href", BAD_CAST fs_pm_fname);

					if (use_pub_fmt) {
						xi = xmlAddChild(pub, xi);
					} else {
						xi = xmlAddPrevSibling(pm_ref, xi);
					}
				} else {
					doc = xmlReadFile(fs_pm_fname, NULL, PARSE_OPTS);
					pm = xmlDocGetRootElement(doc);
					xmlAddPrevSibling(pm_ref, xmlCopyNode(pm, 1));
					xmlFreeDoc(doc);
				}
			}
		}

		xmlFree(path);
	}

	if (flatten_ref || recursive || !found) {
		xmlUnlinkNode(pm_ref);
		xmlFreeNode(pm_ref);
	}
}

void flatten_dm_ref(xmlNodePtr dm_ref)
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

	dm_code    = first_xpath_node(NULL, dm_ref, ".//dmCode|.//avee");
	issue_info = first_xpath_node(NULL, dm_ref, ".//issueInfo|.//issno");
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

	if (issue_info && !no_issue) {
		issue_number = first_xpath_string(NULL, issue_info, "@issueNumber|@issno");
		in_work      = first_xpath_string(NULL, issue_info, "@inWork|@inwork");
		strcpy(dm_fname_temp, dm_fname);

		if (snprintf(dm_fname, PATH_MAX, "%s_%s-%s", dm_fname_temp, issue_number, in_work ? in_work : "00") < 0) {
			exit(EXIT_BAD_CODE);
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
			exit(EXIT_BAD_CODE);
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

		if (filesystem_fname(fs_dm_fname, dm_fname, path, is_dm)) {
			found = true;

			/* Flatten a container data module by copying the
			 * dmRefs inside the container directly in to the
			 * publication module.
			 */
			if (flatten_container) {
				xmlDocPtr doc;
				xmlNodePtr refs;

				doc = xmlReadFile(fs_dm_fname, NULL, PARSE_OPTS);
				refs = first_xpath_node(doc, NULL, "//container/refs");

				if (refs) {
					xmlNodePtr c;

					/* First, flatten the dmRefs in the
					 * container itself. */
					flatten_pm_entry(refs);

					/* Copy each dmRef from the container
					 * into the PM. */
					for (c = refs->children; c; c = c->next) {
						if (c->type != XML_ELEMENT_NODE) {
							continue;
						}
						xmlAddNextSibling(dm_ref, xmlCopyNode(c, 1));
					}
				}

				xmlFreeDoc(doc);
			}

			if (flatten_ref) {
				if (xinclude) {
					xi = xmlNewNode(NULL, BAD_CAST "xi:include");
					xmlSetProp(xi, BAD_CAST "href", BAD_CAST fs_dm_fname);

					if (use_pub_fmt) {
						xi = xmlAddChild(pub, xi);
					} else {
						xi = xmlAddPrevSibling(dm_ref, xi);
					}
				} else {
					xmlChar *app;
					doc = xmlReadFile(fs_dm_fname, NULL, PARSE_OPTS);
					dmodule = xmlDocGetRootElement(doc);
					if ((app = xmlGetProp(dm_ref, BAD_CAST "applicRefId"))) {
						xmlSetProp(dmodule, BAD_CAST "applicRefId", app);
					}
					xmlFree(app);
					xmlAddPrevSibling(dm_ref, xmlCopyNode(dmodule, 1));
					xmlFreeDoc(doc);
				}
			}
		}

		xmlFree(path);
	}

	if (flatten_ref || !found) {
		xmlUnlinkNode(dm_ref);
		xmlFreeNode(dm_ref);
	}
}

void flatten_pm_entry(xmlNodePtr pm_entry)
{
	xmlNodePtr cur, next;

	cur = pm_entry->children;

	while (cur) {
		next = cur->next;

		if (xmlStrcmp(cur->name, BAD_CAST "dmRef") == 0 || xmlStrcmp(cur->name, BAD_CAST "refdm") == 0) {
			flatten_dm_ref(cur);
		} else if (xmlStrcmp(cur->name, BAD_CAST "pmRef") == 0 || xmlStrcmp(cur->name, BAD_CAST "refpm") == 0) {
			flatten_pm_ref(cur);
		} else if (xmlStrcmp(cur->name, BAD_CAST "pmEntry") == 0 || xmlStrcmp(cur->name, BAD_CAST "pmentry") == 0) {
			flatten_pm_entry(cur);
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

int main(int argc, char **argv)
{
	int c;

	char *pm_fname;

	xmlDocPtr pm_doc;

	xmlNodePtr pm;
	xmlNodePtr content;

	xmlNodePtr cur;

	const char *sopts = "cDd:fxNpRrI:h?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	int overwrite = 0;

	search_paths = xmlNewNode(NULL, BAD_CAST "searchPaths");
	search_dir = strdup(".");

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 'c': flatten_container = 1; break;
			case 'D': flatten_ref = 0; break;
			case 'd': search_dir = strdup(optarg); break;
			case 'f': overwrite = 1; break;
			case 'x': xinclude = 1; break;
			case 'N': no_issue = 1; break;
			case 'p': use_pub_fmt = 1; xinclude = 1; break;
			case 'R': recursive = 1; break;
			case 'r': recursive_search = 1; break;
			case 'I': xmlNewChild(search_paths, NULL, BAD_CAST "path", BAD_CAST optarg); break;
			case 'h':
			case '?': show_help(); exit(0);
		}
	}

	xmlNewChild(search_paths, NULL, BAD_CAST "path", BAD_CAST search_dir);
	free(search_dir);

	if (optind < argc) {
		pm_fname = argv[optind];
	} else {
		pm_fname = "-";
	}

	pm_doc = xmlReadFile(pm_fname, NULL, PARSE_OPTS);

	pm = xmlDocGetRootElement(pm_doc);

	if (xinclude && !use_pub_fmt) {
		xmlSetProp(pm, BAD_CAST "xmlns:xi", BAD_CAST "http://www.w3.org/2001/XInclude");
	}

	content = find_child(pm, "content");

	if (use_pub_fmt) {
		xmlNodePtr xi;

		pub_doc = xmlNewDoc(BAD_CAST "1.0");
		pub = xmlNewNode(NULL, BAD_CAST "publication");
		xmlDocSetRootElement(pub_doc, pub);
		xmlSetProp(pub, BAD_CAST "xmlns:xi", BAD_CAST "http://www.w3.org/2001/XInclude");
		xi = xmlNewChild(pub, NULL, BAD_CAST "xi:include", NULL);
		xmlSetProp(xi, BAD_CAST "href", BAD_CAST pm_fname);
	} else if (!content) {
		fprintf(stderr, E_BAD_PM, pm_fname);
		exit(EXIT_BAD_PM);
	}

	if (optind == argc - 1 || !use_pub_fmt) {
		cur = content->children;

		while (cur) {
			xmlNodePtr next;

			next = cur->next;

			if (xmlStrcmp(cur->name, BAD_CAST "pmEntry") == 0 || xmlStrcmp(cur->name, BAD_CAST "pmentry") == 0) {
				flatten_pm_entry(cur);
			}

			cur = next;
		}
	} else if (use_pub_fmt) {
		int i;

		for (i = optind + 1; i < argc; ++i) {
			xmlNodePtr xi;

			xi = xmlNewChild(pub, NULL, BAD_CAST "xi:include", NULL);
			xmlSetProp(xi, BAD_CAST "href", BAD_CAST argv[i]);
		}
	}

	xmlSaveFile(overwrite ? pm_fname : "-", use_pub_fmt ? pub_doc : pm_doc);

	xmlFreeDoc(pm_doc);
	xmlFreeNode(search_paths);
	xmlFreeDoc(pub_doc);

	xmlCleanupParser();

	return 0;
}
