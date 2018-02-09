#include <stdio.h>
#include <string.h>
#include <libxml/tree.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <libgen.h>

#define ERR_PREFIX "s1kd-dmref: ERROR: "

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

void printref(const char *ref, const char *fname, int opts)
{
	char dmc[256];

	char extension_producer[256] = "";
	char extension_code[256] = "";

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

	xmlNode *dmRef;
	xmlNode *dmRefIdent;
	xmlNode *dmCode;

	
	xmlDocPtr doc;

	xmlBufferPtr buf;

	bool is_dme;

	int n;
	char *code;

	strcpy(dmc, ref);

	is_dme = strncmp(dmc, "DME-", 4) == 0;

	if (is_dme) {
		if (sscanf(dmc, "DME-%255[^-]-%255[^-]-%*s", extension_producer, extension_code) != 2) {
			fprintf(stderr, ERR_PREFIX "Data module extended code invalid: %s\n", dmc);
			exit(EXIT_BAD_INPUT);
		}

		strtok(dmc, "-");
		strtok(NULL, "-");
		strtok(NULL, "-");
		code = strtok(NULL, "");
	} else {
		strtok(dmc, "-");
		code = strtok(NULL, "");
	}

	n = sscanf(code, "%14[^-]-%4[^-]-%3[^-]-%1s%1s-%4[^-]-%2s%3[^-]-%3s%1s-%1s-%3s%1s",
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

	if (n != 11 && n != 13) {
		fprintf(stderr, ERR_PREFIX "Data module code invalid: %s\n", code);
		exit(EXIT_BAD_INPUT);
	}

	dmRef = xmlNewNode(NULL, BAD_CAST "dmRef");
	dmRefIdent = xmlNewChild(dmRef, NULL, BAD_CAST "dmRefIdent", NULL);

	if (is_dme) {
		xmlNode *identExtension;
		identExtension = xmlNewChild(dmRefIdent, NULL, BAD_CAST "identExtension", NULL);
		xmlSetProp(identExtension, BAD_CAST "extensionProducer", BAD_CAST extension_producer);
		xmlSetProp(identExtension, BAD_CAST "extensionCode", BAD_CAST extension_code);
	}

	dmCode = xmlNewChild(dmRefIdent, NULL, BAD_CAST "dmCode", NULL);

	xmlSetProp(dmCode, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
	xmlSetProp(dmCode, BAD_CAST "systemDiffCode", BAD_CAST system_diff_code);
	xmlSetProp(dmCode, BAD_CAST "systemCode", BAD_CAST system_code);
	xmlSetProp(dmCode, BAD_CAST "subSystemCode", BAD_CAST sub_system_code);
	xmlSetProp(dmCode, BAD_CAST "subSubSystemCode", BAD_CAST sub_sub_system_code);
	xmlSetProp(dmCode, BAD_CAST "assyCode", BAD_CAST assy_code);
	xmlSetProp(dmCode, BAD_CAST "disassyCode", BAD_CAST disassy_code);
	xmlSetProp(dmCode, BAD_CAST "disassyCodeVariant", BAD_CAST disassy_code_variant);
	xmlSetProp(dmCode, BAD_CAST "infoCode", BAD_CAST info_code);
	xmlSetProp(dmCode, BAD_CAST "infoCodeVariant", BAD_CAST info_code_variant);
	xmlSetProp(dmCode, BAD_CAST "itemLocationCode", BAD_CAST item_location_code);

	if (strcmp(learn_code, "") != 0) xmlSetProp(dmCode, BAD_CAST "learnCode", BAD_CAST learn_code);
	if (strcmp(learn_event_code, "") != 0) xmlSetProp(dmCode, BAD_CAST "learnEventCode", BAD_CAST learn_event_code);

	if (opts) {
		xmlNode* ref_dmodule;
		xmlNode *ref_identAndStatusSection;
		xmlNode *ref_dmAddress;
		xmlNode *ref_dmIdent;
		xmlNode *ref_dmAddressItems;
		xmlNode *ref_dmTitle;

		doc = xmlReadFile(fname, NULL, 0);

		if (!doc) {
			fprintf(stderr, ERR_PREFIX "Could not read file: %s\n", ref);
			exit(EXIT_MISSING_FILE);
		}

		ref_dmodule = xmlDocGetRootElement(doc);
		ref_identAndStatusSection = find_child(ref_dmodule, "identAndStatusSection");
		ref_dmAddress = find_child(ref_identAndStatusSection, "dmAddress");
		ref_dmIdent = find_child(ref_dmAddress, "dmIdent");
		ref_dmAddressItems = find_child(ref_dmAddress, "dmAddressItems");
		ref_dmTitle = find_child(ref_dmAddressItems, "dmTitle");

		if (hasopt(opts, OPT_ISSUE)) {
			xmlNode *ref_issueInfo;
			ref_issueInfo = find_child(ref_dmIdent, "issueInfo");
			xmlAddChild(dmRefIdent, xmlCopyNode(ref_issueInfo, 1));
		}

		if (hasopt(opts, OPT_LANG)) {
			xmlNode *ref_language;
			ref_language = find_child(ref_dmIdent, "language");
			xmlAddChild(dmRefIdent, xmlCopyNode(ref_language, 1));
		}

		if (hasopt(opts, OPT_TITLE)) {
			xmlNode *dmRefAddressItems;
			dmRefAddressItems = xmlNewChild(dmRef, NULL, BAD_CAST "dmRefAddressItems", NULL);
			xmlAddChild(dmRefAddressItems, xmlCopyNode(ref_dmTitle, 1));
		}

		xmlFreeDoc(doc);
	}

	buf = xmlBufferCreate();

	xmlNodeDump(buf, NULL, dmRef, 0, 1);

	puts((char *) buf->content);

	xmlBufferFree(buf);
	xmlFreeNode(dmRef);
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
	puts("Usage: s1kd-dmref [-tlih?] [<code>|<file>]");
	puts("");
	puts("Options:");
	puts("  -t      Include title (target must be file)");
	puts("  -l      Include language (target must be file)");
	puts("  -i      Include issue info (target must be file)");
	puts("  -h -?   Show this help message.");
	puts("  <code>  The data module code of the reference (must include prefix DMC-/DME-).");
	puts("  <file>  A data module file to reference.");
	puts("          -t/-i/-l can then be used to include the title, issue, and language.");
}

int main(int argc, char **argv)
{
	char scratch[PATH_MAX];
	int c;

	int opts = 0;

	while ((c = getopt(argc, argv, "tilh?")) != -1) {
		switch (c) {
			case 't': opts |= OPT_TITLE; break;
			case 'i': opts |= OPT_ISSUE; break;
			case 'l': opts |= OPT_LANG; break;
			case '?':
			case 'h': show_help(); exit(EXIT_SUCCESS);
		}
	}

	if (optind < argc) {
		int i;

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
