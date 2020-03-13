#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "s1kd_tools.h"

/* Order of references */
#define DM "0" /* dmRef */
#define PM "1" /* pmRef */
#define EP "2" /* externalPubRef */

#define PROG_NAME "s1kd-syncrefs"
#define VERSION "1.8.1"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define INF_PREFIX PROG_NAME ": INFO: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"
#define E_MAX_REFS ERR_PREFIX "Maximum references reached: %d\n"
#define I_SYNCREFS INF_PREFIX "Synchronizing references in %s...\n"
#define I_DELREFS  INF_PREFIX "Deleting refs table in %s...\n"

#define EXIT_INVALID_DM 1
#define EXIT_MAX_REFS 2

static unsigned MAX_REFS = 1;

struct ref {
	char code[256];
	xmlNodePtr ref;
};

static bool only_delete = false;
static enum verbosity { QUIET, NORMAL, VERBOSE } verbosity = NORMAL;

static struct ref *refs;
static int nrefs;

static bool contains_code(const char *code)
{
	int i;

	for (i = 0; i < nrefs; ++i) {
		if (strcmp(refs[i].code, code) == 0) {
			return true;
		}
	}

	return false;
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

static bool is_ref(xmlNodePtr node)
{
	return node->type == XML_ELEMENT_NODE && (
		xmlStrcmp(node->name, BAD_CAST "dmRef") == 0 ||
		xmlStrcmp(node->name, BAD_CAST "refdm") == 0 ||
		xmlStrcmp(node->name, BAD_CAST "pmRef") == 0 ||
		xmlStrcmp(node->name, BAD_CAST "reftp") == 0 ||
		xmlStrcmp(node->name, BAD_CAST "externalPubRef") == 0);
}

static void copy_code(char *dst, xmlNodePtr ref)
{
	xmlNodePtr code;

	char *model_ident_code;

	if (xmlStrcmp(ref->name, BAD_CAST "dmRef") == 0 || xmlStrcmp(ref->name, BAD_CAST "refdm") == 0) {
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

		char learn[6] = "";

		code = first_xpath_node(NULL, ref, ".//dmCode|.//avee");

		model_ident_code     = first_xpath_string(NULL, code, "@modelIdentCode|modelic");
		system_diff_code     = first_xpath_string(NULL, code, "@systemDiffCode|sdc");
		system_code          = first_xpath_string(NULL, code, "@systemCode|chapnum");
		sub_system_code      = first_xpath_string(NULL, code, "@subSystemCode|section");
		sub_sub_system_code  = first_xpath_string(NULL, code, "@subSubSystemCode|subsect");
		assy_code            = first_xpath_string(NULL, code, "@assyCode|subject");
		disassy_code         = first_xpath_string(NULL, code, "@disassyCode|discode");
		disassy_code_variant = first_xpath_string(NULL, code, "@disassyCodeVariant|discodev");
		info_code            = first_xpath_string(NULL, code, "@infoCode|incode");
		info_code_variant    = first_xpath_string(NULL, code, "@infoCodeVariant|incodev");
		item_location_code   = first_xpath_string(NULL, code, "@itemLocationCode|itemloc");
		learn_code           = first_xpath_string(NULL, code, "@learnCode");
		learn_event_code     = first_xpath_string(NULL, code, "@learnEventCode");

		if (learn_code && learn_event_code)
			sprintf(learn, "-%s%s", learn_code, learn_event_code);

		sprintf(dst, DM"%s-%s-%s-%s%s-%s-%s%s-%s%s-%s%s",
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
			item_location_code,
			learn);

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
		xmlFree(learn_code);
		xmlFree(learn_event_code);
	} else if (xmlStrcmp(ref->name, BAD_CAST "pmRef") == 0 || xmlStrcmp(ref->name, BAD_CAST "reftp") == 0) {
		char *pm_issuer;
		char *pm_number;
		char *pm_volume;

		code = first_xpath_node(NULL, ref, ".//pmCode|.//pmc");

		model_ident_code = (char *) xmlGetProp(code, BAD_CAST "modelIdentCode");
		pm_issuer = (char *) xmlGetProp(code, BAD_CAST "pmIssuer");
		pm_number = (char *) xmlGetProp(code, BAD_CAST "pmNumber");
		pm_volume = (char *) xmlGetProp(code, BAD_CAST "pmVolume");

		sprintf(dst, PM"%s-%s-%s-%s",
			model_ident_code,
			pm_issuer,
			pm_number,
			pm_volume);

		xmlFree(model_ident_code);
		xmlFree(pm_issuer);
		xmlFree(pm_number);
		xmlFree(pm_volume);
	} else if (xmlStrcmp(ref->name, BAD_CAST "externalPubRef") == 0) {
		xmlNodePtr title;

		code  = first_xpath_node(NULL, ref, ".//externalPubCode");
		title = first_xpath_node(NULL, ref, ".//externalPubTitle");

		if (code) {
			char *code_content;

			code_content = (char *) xmlNodeGetContent(code);
			sprintf(dst, EP"%s", code_content);
			xmlFree(code_content);
		} else if (title) {
			char *title_content;

			title_content = (char *) xmlNodeGetContent(title);
			sprintf(dst, EP"%s", title_content);
			xmlFree(title_content);
		} 
	} else {
		strcpy(dst, "");
	}
}

static void resize(void)
{
	if (!(refs = realloc(refs, (MAX_REFS *= 2) * sizeof(struct ref)))) {
		if (verbosity >= NORMAL) {
			fprintf(stderr, E_MAX_REFS, nrefs);
		}
		exit(EXIT_MAX_REFS);
	}
}

static void find_refs(xmlNodePtr node)
{
	xmlNodePtr cur;

	if (is_ref(node)) {
		char code[256];

		copy_code(code, node);

		if (!contains_code(code)) {
			if (nrefs == MAX_REFS) {
				resize();
			}

			strcpy(refs[nrefs].code, code);
			refs[nrefs].ref = node;
			++nrefs;
		}
	} else {
		for (cur = node->children; cur; cur = cur->next) {
			find_refs(cur);
		}
	}
}

static int compare_refs(const void *a, const void *b)
{
	struct ref *ref1 = (struct ref *) a;
	struct ref *ref2 = (struct ref *) b;

	return strcmp(ref1->code, ref2->code);
}

static void sync_refs(xmlNodePtr dmodule)
{
	int i;

	xmlNodePtr content, old_refs, new_refs, searchable, new_node,
		refgrp = NULL, refdms = NULL, reftp = NULL, rdandrt = NULL;

	nrefs = 0;

	content = find_child(dmodule, "content");

	old_refs = find_child(content, "refs");

	if (old_refs) {
		refgrp = first_xpath_node(NULL, old_refs, "norefs|refdms|reftp|rdandrt");

		xmlUnlinkNode(old_refs);
		xmlFreeNode(old_refs);
	}

	if (only_delete) return;

	searchable = xmlLastElementChild(content);

	if (!searchable) {
		if (verbosity >= NORMAL) {
			fprintf(stderr, ERR_PREFIX "Invalid data module.\n");
		}
		exit(EXIT_INVALID_DM);
	}

	find_refs(searchable);

	if (nrefs < 1) {
		return;
	}

	new_refs = xmlNewNode(NULL, BAD_CAST "refs");

	xmlAddPrevSibling(content->children, new_refs);

	if (refgrp) {
		refdms  = xmlNewChild(new_refs, NULL, BAD_CAST "refdms", NULL);
		reftp   = xmlNewChild(new_refs, NULL, BAD_CAST "reftp", NULL);
		rdandrt = xmlNewChild(new_refs, NULL, BAD_CAST "rdandrt", NULL);
	}

	qsort(refs, nrefs, sizeof(struct ref), compare_refs);

	for (i = 0; i < nrefs; ++i) {
		if (refgrp) {
			if (xmlStrcmp(refs[i].ref->name, BAD_CAST "refdm") == 0) {
				new_node = xmlAddChild(refdms, xmlCopyNode(refs[i].ref, 1));
				xmlUnsetProp(new_node, BAD_CAST "id");
				new_node = xmlAddChild(rdandrt, xmlCopyNode(refs[i].ref, 1));
				xmlUnsetProp(new_node, BAD_CAST "id");
			} else if (xmlStrcmp(refs[i].ref->name, BAD_CAST "reftp") == 0) {
				new_node = xmlAddChild(reftp, xmlCopyNode(refs[i].ref, 1));
				xmlUnsetProp(new_node, BAD_CAST "id");
				new_node = xmlAddChild(reftp, xmlCopyNode(refs[i].ref, 1));
				xmlUnsetProp(new_node, BAD_CAST "id");
			}
		} else {
			new_node = xmlAddChild(new_refs, xmlCopyNode(refs[i].ref, 1));
			xmlUnsetProp(new_node, BAD_CAST "id");
		}
	}

	if (refgrp) {
		if (!refdms->children) {
			xmlUnlinkNode(refdms);
			xmlFreeNode(refdms);
			xmlUnlinkNode(rdandrt);
			xmlFreeNode(rdandrt);
		} else if (!reftp->children) {
			xmlUnlinkNode(reftp);
			xmlFreeNode(reftp);
			xmlUnlinkNode(rdandrt);
			xmlFreeNode(rdandrt);
		} else {
			xmlUnlinkNode(rdandrt);
			xmlFreeNode(rdandrt);
		}
	}
}

static void sync_refs_file(const char *path, const char *out, bool overwrite)
{
	xmlDocPtr dm;
	xmlNodePtr dmodule;

	if (verbosity >= VERBOSE) {
		if (only_delete) {
			fprintf(stderr, I_DELREFS, path);
		} else {
			fprintf(stderr, I_SYNCREFS, path);
		}
	}

	if (!(dm = read_xml_doc(path, false))) {
		return;
	}

	dmodule = xmlDocGetRootElement(dm);

	sync_refs(dmodule);

	if (overwrite) {
		save_xml_doc(dm, path);
	} else {
		save_xml_doc(dm, out);
	}

	xmlFreeDoc(dm);
}

static void sync_refs_list(const char *path, const char *out, bool overwrite)
{
	FILE *f;
	char line[PATH_MAX];

	if (path) {
		if (!(f = fopen(path, "r"))) {
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
		sync_refs_file(line, out, overwrite);
	}

	if (path) {
		fclose(f);
	}
}

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-dflqvh?] [-o <out>] [<dms>]");
	puts("");
	puts("Options:");
	puts("  -d, --delete     Delete the references table.");
	puts("  -f, --overwrite  Overwrite the data modules automatically.");
	puts("  -h, -?, --help   Show help/usage message.");
	puts("  -l, --list       Treat input as list of CSDB objects.");
	puts("  -o, --out <out>  Output to <out> instead of stdout.");
	puts("  -q, --quiet      Quiet mode.");
	puts("  -v, --verbose    Verbose output.");
	puts("  --version        Show version information.");
	puts("  <dms>            Any number of data modules. Otherwise, read from stdin.");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int main(int argc, char *argv[])
{
	int i;

	char out[PATH_MAX] = "-";

	bool overwrite = false;
	bool islist = false;

	const char *sopts = "dflo:qvh?";
	struct option lopts[] = {
		{"version"  , no_argument      , 0, 0},
		{"help"     , no_argument      , 0, 'h'},
		{"delete"   , no_argument      , 0, 'd'},
		{"overwrite", no_argument      , 0, 'f'},
		{"list"     , no_argument      , 0, 'l'},
		{"out"      , required_argument, 0, 'o'},
		{"quiet"    , no_argument      , 0, 'q'},
		{"verbose"  , no_argument      , 0, 'v'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	refs = malloc(MAX_REFS * sizeof(struct ref));

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'd':
				only_delete = true;
				break;
			case 'f':
				overwrite = true;
				break;
			case 'l':
				islist = true;
				break;
			case 'o':
				strcpy(out, optarg);
				break;
			case 'q':
				--verbosity;
				break;
			case 'v':
				++verbosity;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				sync_refs_list(argv[i], out, overwrite);
			} else {
				sync_refs_file(argv[i], out, overwrite);
			}
		}
	} else if (islist) {
		sync_refs_list(NULL, out, overwrite);
	} else {
		sync_refs_file("-", out, false);
	}

	free(refs);

	xmlCleanupParser();

	return 0;
}
