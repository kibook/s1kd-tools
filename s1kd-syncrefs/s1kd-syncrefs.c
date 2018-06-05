#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

/* Order of references */
#define DM "0" /* dmRef */
#define PM "1" /* pmRef */
#define EP "2" /* externalPubRef */

#define PROG_NAME "s1kd-syncrefs"
#define VERSION "1.1.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_INVALID_DM 1

struct ref {
	char code[256];
	xmlNodePtr ref;
};

bool only_delete = false;

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

bool contains_code(struct ref refs[256], int n, const char *code)
{
	int i;

	for (i = 0; i < n; ++i) {
		if (strcmp(refs[i].code, code) == 0) {
			return true;
		}
	}

	return false;
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

bool is_ref(xmlNodePtr node)
{
	return node->type == XML_ELEMENT_NODE && (
		xmlStrcmp(node->name, BAD_CAST "dmRef") == 0 ||
		xmlStrcmp(node->name, BAD_CAST "refdm") == 0 ||
		xmlStrcmp(node->name, BAD_CAST "pmRef") == 0 ||
		xmlStrcmp(node->name, BAD_CAST "reftp") == 0 ||
		xmlStrcmp(node->name, BAD_CAST "externalPubRef") == 0);
}

void copy_code(char *dst, xmlNodePtr ref)
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

void find_refs(struct ref refs[256], int *n, xmlNodePtr node)
{
	xmlNodePtr cur;

	if (is_ref(node)) {
		char code[256];

		copy_code(code, node);

		if (!contains_code(refs, *n, code)) {
			strcpy(refs[*n].code, code);
			refs[*n].ref = node;
			++(*n);
		}
	} else {
		for (cur = node->children; cur; cur = cur->next) {
			find_refs(refs, n, cur);
		}
	}
}

int compare_refs(const void *a, const void *b)
{
	struct ref *ref1 = (struct ref *) a;
	struct ref *ref2 = (struct ref *) b;

	return strcmp(ref1->code, ref2->code);
}

void sync_refs(xmlNodePtr dmodule)
{
	struct ref refs[256];
	int n = 0, i;

	xmlNodePtr content, old_refs, new_refs, searchable, new_node, refgrp, refdms, reftp, rdandrt;

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
		fprintf(stderr, ERR_PREFIX "Invalid data module.\n");
		exit(EXIT_INVALID_DM);
	}

	find_refs(refs, &n, searchable);

	new_refs = xmlNewNode(NULL, BAD_CAST "refs");

	if (n > 0) {
		xmlAddPrevSibling(content->children, new_refs);
	}

	if (refgrp) {
		refdms  = xmlNewChild(new_refs, NULL, BAD_CAST "refdms", NULL);
		reftp   = xmlNewChild(new_refs, NULL, BAD_CAST "reftp", NULL);
		rdandrt = xmlNewChild(new_refs, NULL, BAD_CAST "rdandrt", NULL);
	}

	qsort(refs, n, sizeof(struct ref), compare_refs);

	for (i = 0; i < n; ++i) {
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

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-o <out>] <dms>");
	puts("");
	puts("Options:");
	puts("  -o <out>	Output to <out> instead of stdout");
	puts("  -f              Overwrite the data modules automatically");
	puts("  -d              Delete the references table");
	puts("  --version       Show version information");
	puts("  <dms>		Any number of data modules");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
}

int main(int argc, char *argv[])
{
	int i;

	xmlDocPtr dm;

	xmlNodePtr dmodule;
	
	char out[PATH_MAX] = "-";

	bool overwrite = false;

	const char *sopts = "o:dfh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 'o':
				strcpy(out, optarg);
				break;
			case 'd':
				only_delete = true;
				break;
			case 'f':
				overwrite = true;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			dm = xmlReadFile(argv[i], NULL, PARSE_OPTS);

			dmodule = xmlDocGetRootElement(dm);

			sync_refs(dmodule);

			if (overwrite) {
				xmlSaveFile(argv[i], dm);
			} else {
				xmlSaveFile(out, dm);
			}

			xmlFreeDoc(dm);
		}
	} else {
		dm = xmlReadFile("-", NULL, PARSE_OPTS);
		dmodule = xmlDocGetRootElement(dm);
		sync_refs(dmodule);
		xmlSaveFile(out, dm);
		xmlFreeDoc(dm);
	}

	xmlCleanupParser();

	return 0;
}
