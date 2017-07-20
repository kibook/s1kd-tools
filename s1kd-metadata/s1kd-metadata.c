#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-metadata"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_INVALID_METADATA 1
#define EXIT_INVALID_VALUE 2
#define EXIT_NO_WRITE 3
#define EXIT_MISSING_METADATA 4
#define EXIT_NO_EDIT 5
#define EXIT_INVALID_CREATE 6

#define KEY_COLUMN_WIDTH 31

struct metadata {
	char *key;
	char *path;
	void (*show)(xmlNodePtr);
	int (*edit)(xmlNodePtr, const char *);
	int (*create)(xmlXPathContextPtr, const char *val);
};

xmlNodePtr first_xpath_node(char *expr, xmlXPathContextPtr ctxt)
{
	xmlXPathObjectPtr results;
	xmlNodePtr node;

	results = xmlXPathEvalExpression(BAD_CAST expr, ctxt);

	if (xmlXPathNodeSetIsEmpty(results->nodesetval)) {
		node = NULL;
	} else {
		node = results->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(results);

	return node;
}

void show_issue_date(xmlNodePtr issue_date)
{
	char *year, *month, *day;

	year  = (char *) xmlGetProp(issue_date, BAD_CAST "year");
	month = (char *) xmlGetProp(issue_date, BAD_CAST "month");
	day   = (char *) xmlGetProp(issue_date, BAD_CAST "day");

	printf("%s-%s-%s\n", year, month, day);

	xmlFree(year);
	xmlFree(month);
	xmlFree(day);
}

int edit_issue_date(xmlNodePtr issue_date, const char *val)
{
	char year[5], month[3], day[3];

	if (sscanf(val, "%4s-%2s-%2s", year, month, day) != 3) {
		return EXIT_INVALID_VALUE;
	}

	xmlSetProp(issue_date, BAD_CAST "year", BAD_CAST year);
	xmlSetProp(issue_date, BAD_CAST "month", BAD_CAST month);
	xmlSetProp(issue_date, BAD_CAST "day", BAD_CAST day);

	return 0;
}

void show_simple_node(xmlNodePtr node)
{
	char *content = (char *) xmlNodeGetContent(node);
	printf("%s\n", content);
	xmlFree(content);
}

int edit_simple_node(xmlNodePtr node, const char *val)
{
	xmlNodeSetContent(node, BAD_CAST val);
	return 0;
}

int create_info_name(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr tech_name = first_xpath_node("//dmAddressItems/dmTitle/techName", ctxt);
	xmlNodePtr info_name = xmlNewNode(NULL, BAD_CAST "infoName");
	info_name = xmlAddNextSibling(tech_name, info_name);
	xmlNodeSetContent(info_name, BAD_CAST val);

	return 0;
}

void show_simple_attr(xmlNodePtr node, const char *attr)
{
	char *text = (char *) xmlGetProp(node, BAD_CAST attr);
	printf("%s\n", text);
	xmlFree(text);
}

int edit_simple_attr(xmlNodePtr node, const char *attr, const char *val)
{
	xmlSetProp(node, BAD_CAST attr, BAD_CAST val);
	return 0;
}

int create_simple_attr(xmlXPathContextPtr ctxt, const char *nodepath, const char *attr, const char *val)
{
	xmlXPathObjectPtr results;
	xmlNodePtr node;

	results = xmlXPathEvalExpression(BAD_CAST nodepath, ctxt);

	if (!xmlXPathNodeSetIsEmpty(results->nodesetval)) {
		node = results->nodesetval->nodeTab[0];
		xmlSetProp(node, BAD_CAST attr, BAD_CAST val);
	}

	xmlXPathFreeObject(results);

	return 0;
}

void show_ent_code(xmlNodePtr node)
{
	show_simple_attr(node, "enterpriseCode");
}

int edit_ent_code(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "enterpriseCode", val);
}

int create_rpc_ent_code(xmlXPathContextPtr ctxt, const char *val)
{
	create_simple_attr(ctxt, "//dmStatus/responsiblePartnerCompany", "enterpriseCode", val);
	return 0;
}

int create_orig_ent_code(xmlXPathContextPtr ctxt, const char *val)
{
	create_simple_attr(ctxt, "//dmStatus/originator", "enterpriseCode", val);
	return 0;
}

void show_sec_class(xmlNodePtr node)
{
	show_simple_attr(node, "securityClassification");
}

int edit_sec_class(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "securityClassification", val);
}

void show_schema(xmlNodePtr node)
{
	show_simple_attr(node, "noNamespaceSchemaLocation");
}

int edit_schema(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "xsi:noNamespaceSchemaLocation", val);
}

int edit_info_name(xmlNodePtr node, const char *val)
{
	if (strcmp(val, "") == 0) {
		xmlUnlinkNode(node);
		xmlFreeNode(node);
		return 0;
	} else {
		return edit_simple_node(node, val);
	}
}

void show_type(xmlNodePtr node)
{
	printf("%s\n", node->name);
}

void show_dmcode(xmlNodePtr node)
{
	char *model_ident_code      = (char *) xmlGetProp(node, BAD_CAST "modelIdentCode");
	char *system_diff_code      = (char *) xmlGetProp(node, BAD_CAST "systemDiffCode");
	char *system_code           = (char *) xmlGetProp(node, BAD_CAST "systemCode");
	char *sub_system_code       = (char *) xmlGetProp(node, BAD_CAST "subSystemCode");
	char *sub_sub_system_code   = (char *) xmlGetProp(node, BAD_CAST "subSubSystemCode");
	char *assy_code             = (char *) xmlGetProp(node, BAD_CAST "assyCode");
	char *disassy_code          = (char *) xmlGetProp(node, BAD_CAST "disassyCode");
	char *disassy_code_variant  = (char *) xmlGetProp(node, BAD_CAST "disassyCodeVariant");
	char *info_code             = (char *) xmlGetProp(node, BAD_CAST "infoCode");
	char *info_code_variant     = (char *) xmlGetProp(node, BAD_CAST "infoCodeVariant");
	char *item_location_code    = (char *) xmlGetProp(node, BAD_CAST "itemLocationCode");
	char *learn_code            = (char *) xmlGetProp(node, BAD_CAST "learnCode");
	char *learn_event_code      = (char *) xmlGetProp(node, BAD_CAST "learnEventCode");

	char learn[6] = "";

	if (learn_code && learn_event_code) sprintf(learn, "-%s%s", learn_code, learn_event_code);

	printf("%s-%s-%s-%s%s-%s-%s%s-%s%s-%s%s\n",
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
}

int edit_dmcode(xmlNodePtr node, const char *val)
{
	char model_ident_code[15];
	char system_diff_code[5];
	char system_code[4];
	char sub_system_code[2];
	char sub_sub_system_code[2];
	char assy_code[5];
	char disassy_code[3];
	char disassy_code_variant[4];
	char info_code[4];
	char info_code_variant[2];
	char item_location_code[2];
	char learn_code[4];
	char learn_event_code[2];
	int n;

	n = sscanf(val, "%14[^-]-%4[^-]-%3[^-]-%1s%1s-%4[^-]-%2s%3[^-]-%3s%1s-%1s-%3s%1s",
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
		return EXIT_INVALID_VALUE;
	}

	edit_simple_attr(node, "modelIdentCode", model_ident_code);
	edit_simple_attr(node, "systemDiffCode", system_diff_code);
	edit_simple_attr(node, "systemCode", system_code);
	edit_simple_attr(node, "subSystemCode", sub_system_code);
	edit_simple_attr(node, "subSubSystemCode", sub_sub_system_code);
	edit_simple_attr(node, "assyCode", assy_code);
	edit_simple_attr(node, "disassyCode", disassy_code);
	edit_simple_attr(node, "disassyCodeVariant", disassy_code_variant);
	edit_simple_attr(node, "infoCode", info_code);
	edit_simple_attr(node, "infoCodeVariant", info_code_variant);
	edit_simple_attr(node, "itemLocationCode", item_location_code);

	if (n == 13) {
		edit_simple_attr(node, "learnCode", learn_code);
		edit_simple_attr(node, "learnEventCode", learn_event_code);
	}

	return 0;
}

void show_issue_type(xmlNodePtr node)
{
	show_simple_attr(node, "issueType");
}

int edit_issue_type(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "issueType", val);
}

void show_language(xmlNodePtr node)
{
	char *language_iso_code = (char *) xmlGetProp(node, BAD_CAST "languageIsoCode");
	char *country_iso_code  = (char *) xmlGetProp(node, BAD_CAST "countryIsoCode");

	printf("%s-%s\n", language_iso_code, country_iso_code);

	xmlFree(language_iso_code);
	xmlFree(country_iso_code);
}

void show_issue_info(xmlNodePtr node)
{
	char *issue_number = (char *) xmlGetProp(node, BAD_CAST "issueNumber");
	char *in_work      = (char *) xmlGetProp(node, BAD_CAST "inWork");

	printf("%s-%s\n", issue_number, in_work);
	
	xmlFree(issue_number);
	xmlFree(in_work);
}

int create_act_ref(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;

	node = first_xpath_node("//dmStatus/originator", ctxt);

	node = xmlAddNextSibling(node, xmlNewNode(NULL, BAD_CAST "applicCrossRefTableRef"));
	node = xmlNewChild(node, NULL, BAD_CAST "dmRef", NULL);
	node = xmlNewChild(node, NULL, BAD_CAST "dmRefIdent", NULL);
	node = xmlNewChild(node, NULL, BAD_CAST "dmCode", NULL);

	return edit_dmcode(node, val);
}

int create_comment_title(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;

	node = first_xpath_node("//commentAddressItems/issueDate", ctxt);
	
	if (!node) return EXIT_INVALID_CREATE;

	node = xmlAddNextSibling(node, xmlNewNode(NULL, BAD_CAST "commentTitle"));

	return edit_simple_node(node, val);
}

void show_comment_code(xmlNodePtr node)
{
	char *model_ident_code;
	char *sender_ident;
	char *year_of_data_issue;
	char *seq_number;
	char *comment_type;

	model_ident_code   = (char *) xmlGetProp(node, BAD_CAST "modelIdentCode");
	sender_ident       = (char *) xmlGetProp(node, BAD_CAST "senderIdent");
	year_of_data_issue = (char *) xmlGetProp(node, BAD_CAST "yearOfDataIssue");
	seq_number         = (char *) xmlGetProp(node, BAD_CAST "seqNumber");
	comment_type       = (char *) xmlGetProp(node, BAD_CAST "commentType");

	printf("%s-%s-%s-%s-%s\n",
		model_ident_code,
		sender_ident,
		year_of_data_issue,
		seq_number,
		comment_type);
	
	xmlFree(model_ident_code);
	xmlFree(sender_ident);
	xmlFree(year_of_data_issue);
	xmlFree(seq_number);
	xmlFree(comment_type);
}

void show_comment_priority(xmlNodePtr node)
{
	show_simple_attr(node, "commentPriorityCode");
}

int edit_comment_priority(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "commentPriorityCode", val);
}

void show_comment_response(xmlNodePtr node)
{
	show_simple_attr(node, "responseType");
}

int edit_comment_response(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "responseType", val);
}

struct metadata metadata[] = {
	{"issueDate",
	 	"//issueDate",
		show_issue_date,
		edit_issue_date,
		NULL},
	{"techName",
		"//dmAddressItems/dmTitle/techName",
		show_simple_node,
		edit_simple_node,
		NULL},
	{"infoName",
		"//dmAddressItems/dmTitle/infoName",
		show_simple_node,
		edit_info_name,
		create_info_name},
	{"responsiblePartnerCompany",
		"//responsiblePartnerCompany/enterpriseName",
		show_simple_node,
		edit_simple_node,
		NULL},
	{"originator",
		"//originator/enterpriseName",
		show_simple_node,
		edit_simple_node,
		NULL},
	{"responsiblePartnerCompanyCode",
		"//responsiblePartnerCompany/@enterpriseCode",
		show_ent_code,
		edit_ent_code,
		create_rpc_ent_code},
	{"originatorCode",
		"//originator/@enterpriseCode",
		show_ent_code,
		edit_ent_code,
		create_orig_ent_code},
	{"securityClassification",
		"//security",
		show_sec_class,
		edit_sec_class,
		NULL},
	{"schema",
		"/*",
		show_schema,
		edit_schema,
		NULL},
	{"type",
		"/*",
		show_type,
		NULL,
		NULL},
	{"applic",
		"//applic/displayText/simplePara",
		show_simple_node,
		edit_simple_node,
		NULL},
	{"brex",
		"//brexDmRef/dmRef/dmRefIdent/dmCode",
		show_dmcode,
		edit_dmcode,
		NULL},
	{"act",
		"//applicCrossRefTableRef/dmRef/dmRefIdent/dmCode",
		show_dmcode,
		edit_dmcode,
		create_act_ref},
	{"issueType",
		"//dmStatus|//pmStatus",
		show_issue_type,
		edit_issue_type,
		NULL},
	{"language",
		"//language",
		show_language,
		NULL,
		NULL},
	{"issueInfo",
		"//issueInfo",
		show_issue_info,
		NULL,
		NULL},
	{"authorization",
		"//ddnStatus/authorization",
		show_simple_node,
		edit_simple_node,
		NULL},
	{"dmCode",
		"//dmIdent/dmCode",
		show_dmcode,
		NULL,
		NULL},
	{"commentTitle",
		"//commentAddressItems/commentTitle",
		show_simple_node,
		edit_simple_node,
		create_comment_title},
	{"commentCode",
		"//commentIdent/commentCode",
		show_comment_code,
		NULL,
		NULL},
	{"commentPriority",
		"//commentStatus/commentPriority/@commentPriorityCode",
		show_comment_priority,
		edit_comment_priority,
		NULL},
	{"commentResponse",
		"//commentStatus/commentResponse/@responseType",
		show_comment_response,
		edit_comment_response,
		NULL},
	{"icnTitle",
		"//imfAddressItems/icnTitle",
		show_simple_node,
		edit_simple_node,
		NULL},
	{NULL}
};

int show_metadata(xmlXPathContextPtr ctxt, const char *key)
{
	int i;

	for (i = 0; metadata[i].key; ++i) {
		if (strcmp(key, metadata[i].key) == 0) {
			xmlNodePtr node;
			if (!(node = first_xpath_node(metadata[i].path, ctxt))) {
				return EXIT_MISSING_METADATA;
			}
			if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;
			metadata[i].show(node);
			return EXIT_SUCCESS;
		}
	}

	return EXIT_INVALID_METADATA;
}

int edit_metadata(xmlXPathContextPtr ctxt, const char *key, const char *val)
{
	int i;

	for (i = 0; metadata[i].key; ++i) {
		if (strcmp(key, metadata[i].key) == 0) {
			xmlNodePtr node;
			if (!(node = first_xpath_node(metadata[i].path, ctxt))) {
				if (metadata[i].create) {
					return metadata[i].create(ctxt, val);
				} else {
					return EXIT_NO_EDIT;
				}
			} else {
				if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;
				if (metadata[i].edit) {
					return metadata[i].edit(node, val);
				} else {
					return EXIT_NO_EDIT;
				}
			}
		}
	}

	return EXIT_INVALID_METADATA;
}

int show_all_metadata(xmlXPathContextPtr ctxt, int formatall)
{
	int i;

	for (i = 0; metadata[i].key; ++i) {
		xmlNodePtr node;
		if ((node = first_xpath_node(metadata[i].path, ctxt))) {
			if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;
			printf("%s", metadata[i].key);
			if (formatall) {
				int n = KEY_COLUMN_WIDTH - strlen(metadata[i].key);
				int j;
				for (j = 0; j < n; ++j) putchar(' ');
			} else {
				putchar('\t');
			}
			metadata[i].show(node);
		}
	}

	return 0;
}

int edit_all_metadata(FILE *input, xmlXPathContextPtr ctxt)
{
	char key[256], val[256];

	while (fscanf(input, "%255s %255[^\n]", key, val) == 2) {
		edit_metadata(ctxt, key, val);
	}

	return 0;
}

void show_help(void)
{
	puts("Usage: s1kd-metadata [-c <file>] [-t] <module> [<name> [<value>]]");
	puts("");
	puts("Options:");
	puts("  -c <file>    Set metadata using definitions in <file> (- for stdin).");
	puts("  -t           Do not format columns in output.");
	puts("  <module>     S1000D data module to view/edit metadata on.");
	puts("  <name>       Specific metadata name to view/edit.");
	puts("  <value>      Edit the value of the metadata specified.");
}

int main(int argc, char **argv)
{
	const char *fname, *key, *val;
	int err;

	xmlDocPtr doc;
	xmlXPathContextPtr ctxt;

	int c;
	char *metadata_fname = NULL;
	int formatall = 1;

	while ((c = getopt(argc, argv, "c:th?")) != -1) {
		switch (c) {
			case 'c':
				metadata_fname = malloc(strlen(optarg));
				strcpy(metadata_fname, optarg);
				break;
			case 't': formatall = 0; break;
			case 'h':
			case '?': show_help(); exit(0);
		}
	}

	fname = argv[optind];
	key = argc > optind + 1 ? argv[optind + 1] : NULL;
	val = argc > optind + 2 ? argv[optind + 2] : NULL;

	doc = xmlReadFile(fname, NULL, 0);

	ctxt = xmlXPathNewContext(doc);

	if (key) {
		if (val) {
			err = edit_metadata(ctxt, key, val);
		} else {
			err = show_metadata(ctxt, key);
		}
	} else {
		if (metadata_fname) {
			FILE *input;

			if (strcmp(metadata_fname, "-") == 0) {
				input = stdin;
			} else {
				input = fopen(metadata_fname, "r");
			}

			err = edit_all_metadata(input, ctxt);

			fclose(input);
		} else {
			err = show_all_metadata(ctxt, formatall);
		}
	}

	xmlXPathFreeContext(ctxt);

	if (val || metadata_fname) {
		if (access(fname, W_OK) != -1) {
			xmlSaveFile(fname, doc);
		} else {
			fprintf(stderr, ERR_PREFIX "%s does not have write permission.\n", fname);
			exit(EXIT_NO_WRITE);
		}
	}

	if (metadata_fname) free(metadata_fname);

	xmlFreeDoc(doc);

	switch (err) {
		case EXIT_INVALID_METADATA:
			if (val) {
				fprintf(stderr, ERR_PREFIX "Cannot edit metadata: %s\n", key);
			} else {
				fprintf(stderr, ERR_PREFIX "Invalid metadata name: %s\n", key);
			}
			break;
		case EXIT_INVALID_VALUE:
			fprintf(stderr, ERR_PREFIX "Invalid value for %s: %s\n", key, val);
			break;
		case EXIT_MISSING_METADATA:
			fprintf(stderr, ERR_PREFIX "Data has no metadata: %s\n", key);
			break;
		case EXIT_NO_EDIT:
			fprintf(stderr, ERR_PREFIX "Cannot edit metadata: %s\n", key);
			break;
		case EXIT_INVALID_CREATE:
			fprintf(stderr, ERR_PREFIX "%s is not valid metadata for %s\n", key, fname);
			break;
	}

	return err;
}
