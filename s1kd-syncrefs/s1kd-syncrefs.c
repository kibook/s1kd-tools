#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <libxml/tree.h>

/* Order of references */
#define DM "0" /* dmRef */
#define PM "1" /* pmRef */
#define EP "2" /* externalPubRef */

#define ERR_PREFIX "syncrefs: ERROR: "

#define EXIT_INVALID_DM 1

struct ref {
	char code[256];
	xmlNode *ref;
};

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

xmlNode *find_child(xmlNode *parent, const char *child_name)
{
	xmlNode *cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, child_name) == 0) {
			return cur;
		}
	}

	return NULL;
}

bool is_ref(xmlNode *node)
{
	return node->type == XML_ELEMENT_NODE && (
		strcmp((char *) node->name, "dmRef") == 0 ||
		strcmp((char *) node->name, "pmRef") == 0 ||
		strcmp((char *) node->name, "externalPubRef") == 0);
}

void copy_code(char *dst, xmlNode *ref)
{
	xmlNode *ref_ident;
	xmlNode *code;
	xmlNode *title;

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

	char *pm_issuer;
	char *pm_number;
	char *pm_volume;

	char *code_content;
	char *title_content;

	if (strcmp((char *) ref->name, "dmRef") == 0) {
		ref_ident = find_child(ref, "dmRefIdent");
		code = find_child(ref_ident, "dmCode");

		model_ident_code = (char *) xmlGetProp(code, (xmlChar *) "modelIdentCode");
		system_diff_code = (char *) xmlGetProp(code, (xmlChar *) "systemDiffCode");
		system_code = (char *) xmlGetProp(code, (xmlChar *) "systemCode");
		sub_system_code = (char *) xmlGetProp(code, (xmlChar *) "subSystemCode");
		sub_sub_system_code = (char *) xmlGetProp(code, (xmlChar *) "subSubSystemCode");
		assy_code = (char *) xmlGetProp(code, (xmlChar *) "assyCode");
		disassy_code = (char *) xmlGetProp(code, (xmlChar *) "disassyCode");
		disassy_code_variant = (char *) xmlGetProp(code, (xmlChar *) "disassyCodeVariant");
		info_code = (char *) xmlGetProp(code, (xmlChar *) "infoCode");
		info_code_variant = (char *) xmlGetProp(code, (xmlChar *) "infoCodeVariant");
		item_location_code = (char *) xmlGetProp(code, (xmlChar *) "itemLocationCode");

		sprintf(dst, DM"%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
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
	} else if (strcmp((char *) ref->name, "pmRef") == 0) {
		ref_ident = find_child(ref, "pmRefIdent");
		code = find_child(ref_ident, "pmCode");

		model_ident_code = (char *) xmlGetProp(code, (xmlChar *) "modelIdentCode");
		pm_issuer = (char *) xmlGetProp(code, (xmlChar *) "pmIssuer");
		pm_number = (char *) xmlGetProp(code, (xmlChar *) "pmNumber");
		pm_volume = (char *) xmlGetProp(code, (xmlChar *) "pmVolume");

		sprintf(dst, PM"%s-%s-%s-%s",
			model_ident_code,
			pm_issuer,
			pm_number,
			pm_volume);

		xmlFree(model_ident_code);
		xmlFree(pm_issuer);
		xmlFree(pm_number);
		xmlFree(pm_volume);
	} else if (strcmp((char *) ref->name, "externalPubRef") == 0) {
		ref_ident = find_child(ref, "externalPubRefIdent");
		code = find_child(ref_ident, "externalPubCode");
		title = find_child(ref_ident, "externalPubTitle");

		if (code) {
			code_content = (char *) xmlNodeGetContent(code);
			sprintf(dst, EP"%s", code_content);
			xmlFree(code_content);
		} else if (title) {
			title_content = (char *) xmlNodeGetContent(title);
			sprintf(dst, EP"%s", title_content);
			xmlFree(title_content);
		} 
	} else {
		strcpy(dst, "");
	}
}

void find_refs(struct ref refs[256], int *n, xmlNode *node)
{
	xmlNode *cur;

	char code[256];

	if (is_ref(node)) {
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

void sync_refs(xmlNode *dmodule)
{
	struct ref refs[256];
	int n = 0, i;

	xmlNodePtr content, old_refs, new_refs, searchable, new_node;

	content = find_child(dmodule, "content");

	old_refs = find_child(content, "refs");
	if (old_refs) {
		xmlUnlinkNode(old_refs);
		xmlFreeNode(old_refs);
	}

	new_refs = xmlNewNode(NULL, (xmlChar *) "refs");

	xmlAddPrevSibling(content->children, new_refs);

	searchable = find_child(content, "description");
	if (!searchable) searchable = find_child(content, "procedure");
	if (!searchable) searchable = find_child(content, "brDoc");

	if (!searchable) {
		fprintf(stderr, ERR_PREFIX "Invalid data module.\n");
		exit(EXIT_INVALID_DM);
	}

	find_refs(refs, &n, searchable);

	qsort(refs, n, sizeof(struct ref), compare_refs);

	for (i = 0; i < n; ++i) {
		new_node = xmlAddChild(new_refs, xmlCopyNode(refs[i].ref, 1));
		xmlUnsetProp(new_node, (xmlChar *) "id");
	}
}

void show_help(void)
{
	puts("Usage: syncrefs [-o <out>] <dms>");
	puts("");
	puts("Options:");
	puts("  -o <out>	Output to <out> instead of overwriting (- for stdout)");
	puts("  <dms>		Any number of data modules");
}

int main(int argc, char *argv[])
{
	int c;
	int i;

	xmlDocPtr dm;

	xmlNode *dmodule;
	
	char out[256] = "";

	while ((c = getopt(argc, argv, "o:h?")) != -1) {
		switch (c) {
			case 'o':
				strcpy(out, optarg);
				break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	for (i = optind; i < argc; ++i) {
		dm = xmlReadFile(argv[i], NULL, XML_PARSE_NONET);

		dmodule = xmlDocGetRootElement(dm);

		sync_refs(dmodule);

		if (strcmp(out, "") != 0) {
			xmlSaveFormatFile(out, dm, 1);
		} else {
			xmlSaveFormatFile(argv[i], dm, 1);
		}

		xmlFreeDoc(dm);
	}

	xmlCleanupParser();

	return 0;
}
