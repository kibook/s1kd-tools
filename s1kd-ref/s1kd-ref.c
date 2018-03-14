#include <stdio.h>
#include <string.h>
#include <libxml/tree.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <libgen.h>

#define PROG_NAME "s1kd-ref"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_MISSING_FILE 1
#define EXIT_BAD_INPUT 2

#define OPT_TITLE (int) 0x01
#define OPT_ISSUE (int) 0x02
#define OPT_LANG  (int) 0x04

bool hasopt(int opts, int opt)
{
	return (opts & opt) == opt;
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

void dump_node(xmlNodePtr node)
{
	xmlBufferPtr buf;
	buf = xmlBufferCreate();
	xmlNodeDump(buf, NULL, node, 0, 1);
	puts((char *) buf->content);
	xmlBufferFree(buf);
}

#define PME_FMT "PME-%255[^-]-%255[^-]-%14[^-]-%5s-%5s-%2s"
#define PMC_FMT "PMC-%14[^-]-%5s-%5s-%2s"

void print_pm_ref(const char *ref, const char *fname, int opts)
{
	char full_code[1024];
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

	strcpy(full_code, ref);

	is_extended = strncmp(full_code, "PME-", 4) == 0;

	if (is_extended) {
		n = sscanf(full_code, PME_FMT,
			extension_producer,
			extension_code,
			model_ident_code,
			pm_issuer,
			pm_number,
			pm_volume);
		if (n != 6) {
			fprintf(stderr, ERR_PREFIX "Publication module extended code invalid: %s\n", full_code);
			exit(EXIT_BAD_INPUT);
		}
	} else {
		n = sscanf(full_code, PMC_FMT,
			model_ident_code,
			pm_issuer,
			pm_number,
			pm_volume);
		if (n != 4) {
			fprintf(stderr, ERR_PREFIX "Publication module code invalid: %s\n", full_code);
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
		xmlNodePtr ref_pm;
		xmlNodePtr ref_ident_and_status_section;
		xmlNodePtr ref_pm_address;
		xmlNodePtr ref_pm_ident;
		xmlNodePtr ref_pm_address_items;
		xmlNodePtr ref_pm_title;

		if (!(doc = xmlReadFile(fname, NULL, 0))) {
			fprintf(stderr, ERR_PREFIX "Could not read file: %s\n", ref);
			exit(EXIT_MISSING_FILE);
		}

		ref_pm = xmlDocGetRootElement(doc);
		ref_ident_and_status_section = find_child(ref_pm, "identAndStatusSection");
		ref_pm_address = find_child(ref_ident_and_status_section, "pmAddress");
		ref_pm_ident = find_child(ref_pm_address, "pmIdent");
		ref_pm_address_items = find_child(ref_pm_address, "pmAddressItems");
		ref_pm_title = find_child(ref_pm_address_items, "pmTitle");

		if (hasopt(opts, OPT_ISSUE)) {
			xmlAddChild(pm_ref_ident, xmlCopyNode(find_child(ref_pm_ident, "issueInfo"), 1));
		}

		if (hasopt(opts, OPT_LANG)) {
			xmlAddChild(pm_ref_ident, xmlCopyNode(find_child(ref_pm_ident, "language"), 1));
		}

		if (hasopt(opts, OPT_TITLE)) {
			xmlNodePtr pm_ref_address_items;
			pm_ref_address_items = xmlNewChild(pm_ref, NULL, BAD_CAST "pmRefAddressItems", NULL);
			xmlAddChild(pm_ref_address_items, xmlCopyNode(ref_pm_title, 1));
		}

		xmlFreeDoc(doc);
	}

	dump_node(pm_ref);

	xmlFreeNode(pm_ref);
}

#define DME_FMT "DME-%255[^-]-%255[^-]-%14[^-]-%4[^-]-%3[^-]-%1s%1s-%4[^-]-%2s%3[^-]-%3s%1s-%1s-%3s%1s"
#define DMC_FMT "DMC-%14[^-]-%4[^-]-%3[^-]-%1s%1s-%4[^-]-%2s%3[^-]-%3s%1s-%1s-%3s%1s"

void print_dm_ref(const char *ref, const char *fname, int opts)
{
	char full_code[1024];
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

	strcpy(full_code, ref);

	is_extended = strncmp(full_code, "DME-", 4) == 0;

	if (is_extended) {
		n = sscanf(full_code, DME_FMT,
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
			fprintf(stderr, ERR_PREFIX "Data module extended code invalid: %s\n", full_code);
			exit(EXIT_BAD_INPUT);
		}
		has_learn = n == 15;
	} else {
		n = sscanf(full_code, DMC_FMT,
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
			fprintf(stderr, ERR_PREFIX "Data module code invalid: %s\n", full_code);
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
		xmlNodePtr ref_dmodule;
		xmlNodePtr ref_ident_and_status_section;
		xmlNodePtr ref_dm_address;
		xmlNodePtr ref_dm_ident;
		xmlNodePtr ref_dm_address_items;
		xmlNodePtr ref_dm_title;

		if (!(doc = xmlReadFile(fname, NULL, 0))) {
			fprintf(stderr, ERR_PREFIX "Could not read file: %s\n", ref);
			exit(EXIT_MISSING_FILE);
		}

		ref_dmodule = xmlDocGetRootElement(doc);
		ref_ident_and_status_section = find_child(ref_dmodule, "identAndStatusSection");
		ref_dm_address = find_child(ref_ident_and_status_section, "dmAddress");
		ref_dm_ident = find_child(ref_dm_address, "dmIdent");
		ref_dm_address_items = find_child(ref_dm_address, "dmAddressItems");
		ref_dm_title = find_child(ref_dm_address_items, "dmTitle");

		if (hasopt(opts, OPT_ISSUE)) {
			xmlAddChild(dm_ref_ident, xmlCopyNode(find_child(ref_dm_ident, "issueInfo"), 1));
		}

		if (hasopt(opts, OPT_LANG)) {
			xmlAddChild(dm_ref_ident, xmlCopyNode(find_child(ref_dm_ident, "language"), 1));
		}

		if (hasopt(opts, OPT_TITLE)) {
			xmlNodePtr dm_ref_address_items;
			dm_ref_address_items = xmlNewChild(dm_ref, NULL, BAD_CAST "dmRefAddressItems", NULL);
			xmlAddChild(dm_ref_address_items, xmlCopyNode(ref_dm_title, 1));
		}

		xmlFreeDoc(doc);
	}

	dump_node(dm_ref);

	xmlFreeNode(dm_ref);
}

bool is_pm(const char *ref)
{
	return strncmp(ref, "PMC-", 4) == 0 || strncmp(ref, "PME-", 4) == 0;
}

bool is_dm(const char *ref)
{
	return strncmp(ref, "DMC-", 4) == 0 || strncmp(ref, "DME-", 4) == 0;
}

void printref(const char *ref, const char *fname, int opts)
{
	if (is_dm(ref)) {
		print_dm_ref(ref, fname, opts);
	} else if (is_pm(ref)) {
		print_pm_ref(ref, fname, opts);
	}
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
	puts("Usage: " PROG_NAME " [-tlih?] [<code>|<file>]");
	puts("");
	puts("Options:");
	puts("  -t      Include title (target must be file)");
	puts("  -l      Include language (target must be file)");
	puts("  -i      Include issue info (target must be file)");
	puts("  -h -?   Show this help message.");
	puts("  <code>  The code of the reference (must include prefix DMC/PMC/etc.).");
	puts("  <file>  A file to reference.");
	puts("          -t/-i/-l can then be used to include the title, issue, and language.");
}

int main(int argc, char **argv)
{
	char scratch[PATH_MAX];
	int i;
	int opts = 0;

	while ((i = getopt(argc, argv, "tilh?")) != -1) {
		switch (i) {
			case 't': opts |= OPT_TITLE; break;
			case 'i': opts |= OPT_ISSUE; break;
			case 'l': opts |= OPT_LANG; break;
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
			printref(base, fname, opts);
		}
	} else {
		while (fgets(scratch, PATH_MAX, stdin)) {
			printref(trim(scratch), NULL, opts);
		}
	}

	xmlCleanupParser();

	return 0;
}
