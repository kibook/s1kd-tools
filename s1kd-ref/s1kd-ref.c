#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <libgen.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-ref"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_MISSING_FILE 1
#define EXIT_BAD_INPUT 2

#define OPT_TITLE (int) 0x01
#define OPT_ISSUE (int) 0x02
#define OPT_LANG  (int) 0x04
#define OPT_DATE  (int) 0x08

bool hasopt(int opts, int opt)
{
	return (opts & opt) == opt;
}

void lowercase(char *s)
{
	int i;
	for (i = 0; s[i]; ++i) s[i] = tolower(s[i]);
}

xmlNode *find_child(xmlNode *parent, char *name)
{
	xmlNode *cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, name) == 0) {
			return cur;
		}
	}

	return NULL;
}

xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(doc ? doc : node->doc);
	ctx->node = node;
	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	first = xmlXPathNodeSetIsEmpty(obj->nodesetval) ? NULL : obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

xmlNodePtr find_or_create_refs(xmlDocPtr doc)
{
	xmlNodePtr refs;

	refs = first_xpath_node(doc, NULL, "//content//refs");

	if (!refs) {
		xmlNodePtr content, child;
		content = first_xpath_node(doc, NULL, "//content");
		child = xmlFirstElementChild(content);
		refs = xmlNewNode(NULL, BAD_CAST "refs");
		if (child) {
			refs = xmlAddPrevSibling(child, refs);
		} else {
			refs = xmlAddChild(content ,refs);
		}
	}

	return refs;
}

void dump_node(xmlNodePtr node, const char *dst)
{
	xmlBufferPtr buf;
	buf = xmlBufferCreate();
	xmlNodeDump(buf, NULL, node, 0, 1);
	if (strcmp(dst, "-") == 0) {
		puts((char *) buf->content);
	} else {
		FILE *f;
		f = fopen(dst, "w");
		fputs((char *) buf->content, f);
		fclose(f);
	}
	xmlBufferFree(buf);
}

#define PME_FMT "PME-%255[^-]-%255[^-]-%14[^-]-%5s-%5s-%2s"
#define PMC_FMT "PMC-%14[^-]-%5s-%5s-%2s"

xmlNodePtr new_pm_ref(const char *ref, const char *fname, int opts)
{
	char extension_producer[256] = "";
	char extension_code[256]     = "";
	char model_ident_code[15]    = "";
	char pm_issuer[6]            = "";
	char pm_number[6]            = "";
	char pm_volume[3]            = "";
	xmlNode *pm_ref;
	xmlNode *pm_ref_ident;
	xmlNode *pm_code;
	bool is_extended;
	int n;

	is_extended = strncmp(ref, "PME-", 4) == 0;

	if (is_extended) {
		n = sscanf(ref, PME_FMT,
			extension_producer,
			extension_code,
			model_ident_code,
			pm_issuer,
			pm_number,
			pm_volume);
		if (n != 6) {
			fprintf(stderr, ERR_PREFIX "Publication module extended code invalid: %s\n", ref);
			exit(EXIT_BAD_INPUT);
		}
	} else {
		n = sscanf(ref, PMC_FMT,
			model_ident_code,
			pm_issuer,
			pm_number,
			pm_volume);
		if (n != 4) {
			fprintf(stderr, ERR_PREFIX "Publication module code invalid: %s\n", ref);
			exit(EXIT_BAD_INPUT);
		}
	}

	pm_ref = xmlNewNode(NULL, BAD_CAST "pmRef");
	pm_ref_ident = xmlNewChild(pm_ref, NULL, BAD_CAST "pmRefIdent", NULL);

	if (is_extended) {
		xmlNode *ident_extension;
		ident_extension = xmlNewChild(pm_ref_ident, NULL, BAD_CAST "identExtension", NULL);
		xmlSetProp(ident_extension, BAD_CAST "extensionProducer", BAD_CAST extension_producer);
		xmlSetProp(ident_extension, BAD_CAST "extensionCode", BAD_CAST extension_code);
	}

	pm_code = xmlNewChild(pm_ref_ident, NULL, BAD_CAST "pmCode", NULL);

	xmlSetProp(pm_code, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
	xmlSetProp(pm_code, BAD_CAST "pmIssuer", BAD_CAST pm_issuer);
	xmlSetProp(pm_code, BAD_CAST "pmNumber", BAD_CAST pm_number);
	xmlSetProp(pm_code, BAD_CAST "pmVolume", BAD_CAST pm_volume);

	if (opts) {
		xmlDocPtr doc;
		xmlNodePtr ref_pm_address;
		xmlNodePtr ref_pm_ident;
		xmlNodePtr ref_pm_address_items;
		xmlNodePtr ref_pm_title;
		xmlNodePtr ref_pm_issue_date;

		if (!(doc = xmlReadFile(fname, NULL, 0))) {
			fprintf(stderr, ERR_PREFIX "Could not read publication module: %s\n", ref);
			exit(EXIT_MISSING_FILE);
		}

		ref_pm_address = first_xpath_node(doc, NULL, "//pmAddress");
		ref_pm_ident = find_child(ref_pm_address, "pmIdent");
		ref_pm_address_items = find_child(ref_pm_address, "pmAddressItems");
		ref_pm_title = find_child(ref_pm_address_items, "pmTitle");
		ref_pm_issue_date = find_child(ref_pm_address_items, "issueDate");

		if (hasopt(opts, OPT_ISSUE)) {
			xmlAddChild(pm_ref_ident, xmlCopyNode(find_child(ref_pm_ident, "issueInfo"), 1));
		}

		if (hasopt(opts, OPT_LANG)) {
			xmlAddChild(pm_ref_ident, xmlCopyNode(find_child(ref_pm_ident, "language"), 1));
		}

		if (hasopt(opts, OPT_TITLE) || hasopt(opts, OPT_DATE)) {
			xmlNodePtr pm_ref_address_items;
			pm_ref_address_items = xmlNewChild(pm_ref, NULL, BAD_CAST "pmRefAddressItems", NULL);

			if (hasopt(opts, OPT_TITLE)) {
				xmlAddChild(pm_ref_address_items, xmlCopyNode(ref_pm_title, 1));
			}
			if (hasopt(opts, OPT_DATE)) {
				xmlAddChild(pm_ref_address_items, xmlCopyNode(ref_pm_issue_date, 1));
			}
		}

		xmlFreeDoc(doc);
	}

	return pm_ref;
}

#define DME_FMT "DME-%255[^-]-%255[^-]-%14[^-]-%4[^-]-%3[^-]-%1s%1s-%4[^-]-%2s%3[^-]-%3s%1s-%1s-%3s%1s"
#define DMC_FMT "DMC-%14[^-]-%4[^-]-%3[^-]-%1s%1s-%4[^-]-%2s%3[^-]-%3s%1s-%1s-%3s%1s"

xmlNodePtr new_dm_ref(const char *ref, const char *fname, int opts)
{
	char extension_producer[256] = "";
	char extension_code[256]     = "";
	char model_ident_code[15]    = "";
	char system_diff_code[5]     = "";
	char system_code[4]          = "";
	char assy_code[5]            = "";
	char item_location_code[2]   = "";
	char learn_code[4]           = "";
	char learn_event_code[2]     = "";
	char sub_system_code[2]      = "";
	char sub_sub_system_code[2]  = "";
	char disassy_code[3]         = "";
	char disassy_code_variant[4] = "";
	char info_code[4]            = "";
	char info_code_variant[2]    = "";
	xmlNode *dm_ref;
	xmlNode *dm_ref_ident;
	xmlNode *dm_code;
	bool is_extended;
	bool has_learn;
	int n;

	is_extended = strncmp(ref, "DME-", 4) == 0;

	if (is_extended) {
		n = sscanf(ref, DME_FMT,
			extension_producer,
			extension_code,
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
			learn_code,
			learn_event_code);
		if (n != 15 && n != 13) {
			fprintf(stderr, ERR_PREFIX "Data module extended code invalid: %s\n", ref);
			exit(EXIT_BAD_INPUT);
		}
		has_learn = n == 15;
	} else {
		n = sscanf(ref, DMC_FMT,
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
			learn_code,
			learn_event_code);
		if (n != 13 && n != 11) {
			fprintf(stderr, ERR_PREFIX "Data module code invalid: %s\n", ref);
			exit(EXIT_BAD_INPUT);
		}
		has_learn = n == 13;
	}

	dm_ref = xmlNewNode(NULL, BAD_CAST "dmRef");
	dm_ref_ident = xmlNewChild(dm_ref, NULL, BAD_CAST "dmRefIdent", NULL);

	if (is_extended) {
		xmlNode *ident_extension;
		ident_extension = xmlNewChild(dm_ref_ident, NULL, BAD_CAST "identExtension", NULL);
		xmlSetProp(ident_extension, BAD_CAST "extensionProducer", BAD_CAST extension_producer);
		xmlSetProp(ident_extension, BAD_CAST "extensionCode", BAD_CAST extension_code);
	}

	dm_code = xmlNewChild(dm_ref_ident, NULL, BAD_CAST "dmCode", NULL);

	xmlSetProp(dm_code, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
	xmlSetProp(dm_code, BAD_CAST "systemDiffCode", BAD_CAST system_diff_code);
	xmlSetProp(dm_code, BAD_CAST "systemCode", BAD_CAST system_code);
	xmlSetProp(dm_code, BAD_CAST "subSystemCode", BAD_CAST sub_system_code);
	xmlSetProp(dm_code, BAD_CAST "subSubSystemCode", BAD_CAST sub_sub_system_code);
	xmlSetProp(dm_code, BAD_CAST "assyCode", BAD_CAST assy_code);
	xmlSetProp(dm_code, BAD_CAST "disassyCode", BAD_CAST disassy_code);
	xmlSetProp(dm_code, BAD_CAST "disassyCodeVariant", BAD_CAST disassy_code_variant);
	xmlSetProp(dm_code, BAD_CAST "infoCode", BAD_CAST info_code);
	xmlSetProp(dm_code, BAD_CAST "infoCodeVariant", BAD_CAST info_code_variant);
	xmlSetProp(dm_code, BAD_CAST "itemLocationCode", BAD_CAST item_location_code);
	if (has_learn) {
		xmlSetProp(dm_code, BAD_CAST "learnCode", BAD_CAST learn_code);
		xmlSetProp(dm_code, BAD_CAST "learnEventCode", BAD_CAST learn_event_code);
	}

	if (opts) {
		xmlDocPtr doc;
		xmlNodePtr ref_dm_address;
		xmlNodePtr ref_dm_ident;
		xmlNodePtr ref_dm_address_items;
		xmlNodePtr ref_dm_title;
		xmlNodePtr ref_dm_issue_date;

		if (!(doc = xmlReadFile(fname, NULL, 0))) {
			fprintf(stderr, ERR_PREFIX "Could not read data module: %s\n", ref);
			exit(EXIT_MISSING_FILE);
		}

		ref_dm_address = first_xpath_node(doc, NULL, "//dmAddress");
		ref_dm_ident = find_child(ref_dm_address, "dmIdent");
		ref_dm_address_items = find_child(ref_dm_address, "dmAddressItems");
		ref_dm_title = find_child(ref_dm_address_items, "dmTitle");
		ref_dm_issue_date = find_child(ref_dm_address_items, "issueDate");

		if (hasopt(opts, OPT_ISSUE)) {
			xmlAddChild(dm_ref_ident, xmlCopyNode(find_child(ref_dm_ident, "issueInfo"), 1));
		}

		if (hasopt(opts, OPT_LANG)) {
			xmlAddChild(dm_ref_ident, xmlCopyNode(find_child(ref_dm_ident, "language"), 1));
		}

		if (hasopt(opts, OPT_TITLE) || hasopt(opts, OPT_DATE)) {
			xmlNodePtr dm_ref_address_items;
			dm_ref_address_items = xmlNewChild(dm_ref, NULL, BAD_CAST "dmRefAddressItems", NULL);

			if (hasopt(opts, OPT_TITLE)) {
				xmlAddChild(dm_ref_address_items, xmlCopyNode(ref_dm_title, 1));
			}
			if (hasopt(opts, OPT_DATE)) {
				xmlAddChild(dm_ref_address_items, xmlCopyNode(ref_dm_issue_date, 1));
			}
		}

		xmlFreeDoc(doc);
	}

	return dm_ref;
}

#define COM_FMT "COM-%14[^-]-%5s-%4s-%5s-%1s"

xmlNodePtr new_com_ref(const char *ref, const char *fname, int opts)
{
	char model_ident_code[15]  = "";
	char sender_ident[6]       = "";
	char year_of_data_issue[5] = "";
	char seq_number[6]         = "";
	char comment_type[2]       = "";

	int n;

	xmlNodePtr comment_ref, comment_ref_ident, comment_code;

	n = sscanf(ref, COM_FMT,
		model_ident_code,
		sender_ident,
		year_of_data_issue,
		seq_number,
		comment_type);
	if (n != 5) {
		fprintf(stderr, ERR_PREFIX "Comment code invalid: %s\n", ref);
		exit(EXIT_BAD_INPUT);
	}

	lowercase(comment_type);

	comment_ref = xmlNewNode(NULL, BAD_CAST "commentRef");
	comment_ref_ident = xmlNewChild(comment_ref, NULL, BAD_CAST "commentRefIdent", NULL);
	comment_code = xmlNewChild(comment_ref_ident, NULL, BAD_CAST "commentCode", NULL);

	xmlSetProp(comment_code, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
	xmlSetProp(comment_code, BAD_CAST "senderIdent", BAD_CAST sender_ident);
	xmlSetProp(comment_code, BAD_CAST "yearOfDataIssue", BAD_CAST year_of_data_issue);
	xmlSetProp(comment_code, BAD_CAST "seqNumber", BAD_CAST seq_number);
	xmlSetProp(comment_code, BAD_CAST "commentType", BAD_CAST comment_type);

	if (opts) {
		xmlDocPtr doc;
		xmlNodePtr ref_comment_address;
		xmlNodePtr ref_comment_ident;

		if (!(doc = xmlReadFile(fname, NULL, 0))) {
			fprintf(stderr, ERR_PREFIX "Could not read comment: %s\n", ref);
			exit(EXIT_MISSING_FILE);
		}

		ref_comment_address = first_xpath_node(doc, NULL, "//commentAddress");
		ref_comment_ident = find_child(ref_comment_address, "commentIdent");

		if (hasopt(opts, OPT_LANG)) {
			xmlAddChild(comment_ref_ident, xmlCopyNode(find_child(ref_comment_ident, "language"), 1));
		}

		xmlFreeDoc(doc);
	}

	return comment_ref;
}

xmlNodePtr new_icn_ref(const char *ref, const char *fname, int opts)
{
	xmlNodePtr info_entity_ref;

	info_entity_ref = xmlNewNode(NULL, BAD_CAST "infoEntityRef");

	xmlSetProp(info_entity_ref, BAD_CAST "infoEntityRefIdent", BAD_CAST ref);

	return info_entity_ref;
}

bool is_pm(const char *ref)
{
	return strncmp(ref, "PMC-", 4) == 0 || strncmp(ref, "PME-", 4) == 0;
}

bool is_dm(const char *ref)
{
	return strncmp(ref, "DMC-", 4) == 0 || strncmp(ref, "DME-", 4) == 0;
}

bool is_com(const char *ref)
{
	return strncmp(ref, "COM-", 4) == 0;
}

bool is_icn(const char *ref)
{
	return strncmp(ref, "ICN-", 4) == 0;
}

void add_ref(const char *src, const char *dst, xmlNodePtr ref)
{
	xmlDocPtr doc;
	xmlNodePtr refs;

	if (!(doc = xmlReadFile(src, NULL, 0))) {
		fprintf(stderr, ERR_PREFIX "Could not read source data module: %s\n", src);
		exit(EXIT_MISSING_FILE);
	}

	refs = find_or_create_refs(doc);
	xmlAddChild(refs, xmlCopyNode(ref, 1));

	xmlSaveFile(dst, doc);
}

void print_ref(const char *src, const char *dst, const char *ref, const char *fname, int opts, bool insert_refs, bool overwrite)
{
	xmlNodePtr node;
	xmlNodePtr (*f)(const char *, const char *, int);

	if (is_dm(ref)) {
		f = new_dm_ref;
	} else if (is_pm(ref)) {
		f = new_pm_ref;
	} else if (is_com(ref)) {
		f = new_com_ref;
	} else if (is_icn(ref)) {
		f = new_icn_ref;
	} else {
		fprintf(stderr, ERR_PREFIX "Unknown reference type: %s\n", ref);
		exit(EXIT_BAD_INPUT);
	}

	node = f(ref, fname, opts);

	if (insert_refs) {
		if (overwrite) {
			add_ref(src, src, node);
		} else {
			add_ref(src, dst, node);
		}
	} else {
		dump_node(node, dst);
	}

	xmlFreeNode(node);
}

char *trim(char *str)
{
	char *end;

	while (isspace(*str)) str++;

	if (*str == 0) return str;

	end = str + strlen(str) - 1;
	while (end > str && isspace(*end)) end--;

	*(end + 1) = 0;

	return str;
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-filrth?] [-s <src>] [-o <dst>] [<code>|<file>]");
	puts("");
	puts("Options:");
	puts("  -f        Overwrite source data module instead of writing to stdout.");
	puts("  -i        Include issue info (target must be file)");
	puts("  -l        Include language (target must be file)");
	puts("  -o <dst>  Output to <dst> instead of stdout.");
	puts("  -r        Add reference to data module's <refs> table.");
	puts("  -s <src>  Source data module to add references to.");
	puts("  -t        Include title (target must be file)");
	puts("  -d        Include issue date (target must be file)");
	puts("  -h -?     Show this help message.");
	puts("  <code>    The code of the reference (must include prefix DMC/PMC/etc.).");
	puts("  <file>    A file to reference.");
	puts("            -t/-i/-l can then be used to include the title, issue, and language.");
}

int main(int argc, char **argv)
{
	char scratch[PATH_MAX];
	int i;
	int opts = 0;
	bool insert_refs = false;
	char src[PATH_MAX] = "-";
	char dst[PATH_MAX] = "-";
	bool overwrite = false;

	while ((i = getopt(argc, argv, "filo:rs:tdh?")) != -1) {
		switch (i) {
			case 'f': overwrite = true; break;
			case 'i': opts |= OPT_ISSUE; break;
			case 'l': opts |= OPT_LANG; break;
			case 'o': strcpy(dst, optarg); break;
			case 'r': insert_refs = true; break;
			case 's': strcpy(src, optarg); break;
			case 't': opts |= OPT_TITLE; break;
			case 'd': opts |= OPT_DATE; break;
			case '?':
			case 'h': show_help(); exit(EXIT_SUCCESS);
		}
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			char fname[PATH_MAX];
			char *base;

			strcpy(fname, argv[i]);
			strcpy(scratch, fname);
			base = basename(scratch);

			print_ref(src, dst, base, fname, opts, insert_refs, overwrite);
		}
	} else {
		while (fgets(scratch, PATH_MAX, stdin)) {
			print_ref(src, dst, trim(scratch), NULL, opts, insert_refs, overwrite);
		}
	}

	xmlCleanupParser();

	return 0;
}
