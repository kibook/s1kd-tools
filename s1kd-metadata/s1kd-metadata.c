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
#define EXIT_NO_FILE 7

#define KEY_COLUMN_WIDTH 31

#define FMTSTR_DELIM '%'

enum verbosity {SILENT, NORMAL} verbosity = NORMAL;

struct metadata {
	char *key;
	char *path;
	void (*show)(xmlNodePtr, int endl);
	int (*edit)(xmlNodePtr, const char *);
	int (*create)(xmlXPathContextPtr, const char *val);
	char *descr;
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

xmlNodePtr first_xpath_node_local(xmlNodePtr node, const char *expr)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(node->doc);
	ctx->node  = node;

	obj = xmlXPathEvalExpression(BAD_CAST expr, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		first = NULL;
	} else {
		first = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

void show_issue_date(xmlNodePtr issue_date, int endl)
{
	char *year, *month, *day;

	year  = (char *) xmlGetProp(issue_date, BAD_CAST "year");
	month = (char *) xmlGetProp(issue_date, BAD_CAST "month");
	day   = (char *) xmlGetProp(issue_date, BAD_CAST "day");

	printf("%s-%s-%s", year, month, day);
	if (endl > -1) putchar(endl);

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

void show_simple_node(xmlNodePtr node, int endl)
{
	char *content = (char *) xmlNodeGetContent(node);
	printf("%s", content);
	if (endl > -1) putchar(endl);
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

void show_simple_attr(xmlNodePtr node, const char *attr, int endl)
{
	char *text = (char *) xmlGetProp(node, BAD_CAST attr);
	printf("%s", text);
	if (endl > -1) putchar(endl);
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

void show_ent_code(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "enterpriseCode", endl);
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

void show_sec_class(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "securityClassification", endl);
}

int edit_sec_class(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "securityClassification", val);
}

void show_schema(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "noNamespaceSchemaLocation", endl);
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

void show_type(xmlNodePtr node, int endl)
{
	printf("%s", node->name);
	if (endl > -1) putchar(endl);
}

void show_dmcode(xmlNodePtr node, int endl)
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

	printf("%s-%s-%s-%s%s-%s-%s%s-%s%s-%s%s",
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
	if (endl > -1) putchar(endl);

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

void show_issue_type(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "issueType", endl);
}

int edit_issue_type(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "issueType", val);
}

void show_language_iso_code(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "languageIsoCode", endl);
}

int edit_language_iso_code(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "languageIsoCode", val);
}

void show_country_iso_code(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "countryIsoCode", endl);
}

int edit_country_iso_code(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "countryIsoCode", val);
}

void show_issue_number(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "issueNumber", endl);
}

int edit_issue_number(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "issueNumber", val);
}

void show_in_work(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "inWork", endl);
}

int edit_in_work(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "inWork", val);
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

void show_comment_code(xmlNodePtr node, int endl)
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

	printf("%s-%s-%s-%s-%s",
		model_ident_code,
		sender_ident,
		year_of_data_issue,
		seq_number,
		comment_type);
	if (endl > -1) putchar(endl);
	
	xmlFree(model_ident_code);
	xmlFree(sender_ident);
	xmlFree(year_of_data_issue);
	xmlFree(seq_number);
	xmlFree(comment_type);
}

void show_comment_priority(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "commentPriorityCode", endl);
}

int edit_comment_priority(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "commentPriorityCode", val);
}

void show_comment_response(xmlNodePtr node, int endl)
{
	show_simple_attr(node, "responseType", endl);
}

int edit_comment_response(xmlNodePtr node, const char *val)
{
	return edit_simple_attr(node, "responseType", val);
}

int create_ent_name(xmlNodePtr node, const char *val)
{
	return xmlNewChild(node, NULL, BAD_CAST "enterpriseName", BAD_CAST val) == NULL;
}

int create_rpc_name(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;
	node = first_xpath_node("//responsiblePartnerCompany", ctxt);
	if (!node) return EXIT_INVALID_CREATE;
	return create_ent_name(node, val);
}

int create_orig_name(xmlXPathContextPtr ctxt, const char *val)
{
	xmlNodePtr node;
	node = first_xpath_node("//originator", ctxt);
	if (!node) return EXIT_INVALID_CREATE;
	return create_ent_name(node, val);
}

void show_url(xmlNodePtr node, int endl)
{
	printf("%s", node->doc->URL);
	if (endl > -1) putchar(endl);
}

void show_title(xmlNodePtr node, int endl)
{
	if (xmlStrcmp(node->name, BAD_CAST "dmTitle") == 0) {
		xmlNodePtr tech, info;
		xmlChar *tech_content;
		tech = first_xpath_node_local(node, "techName");
		info = first_xpath_node_local(node, "infoName");
		tech_content = xmlNodeGetContent(tech);
		printf("%s", (char *) tech_content);
		xmlFree(tech_content);
		if (info) {
			xmlChar *info_content;
			info_content = xmlNodeGetContent(info);
			printf(" - %s", info_content);
			xmlFree(info_content);
		}
		if (endl > -1) putchar(endl);
	} else {
		show_simple_node(node, endl);
	}
}

struct metadata metadata[] = {
	{"act",
		"//applicCrossRefTableRef/dmRef/dmRefIdent/dmCode",
		show_dmcode,
		edit_dmcode,
		create_act_ref,
		"ACT data module code"},
	{"applic",
		"//applic/displayText/simplePara",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Whole data module applicability"},
	{"authorization",
		"//ddnStatus/authorization",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Authorization for a DDN"},
	{"brex",
		"//brexDmRef/dmRef/dmRefIdent/dmCode",
		show_dmcode,
		edit_dmcode,
		NULL,
		"BREX data module code"},
	{"commentCode",
		"//commentIdent/commentCode",
		show_comment_code,
		NULL,
		NULL,
		"Comment code"},
	{"commentPriority",
		"//commentStatus/commentPriority/@commentPriorityCode",
		show_comment_priority,
		edit_comment_priority,
		NULL,
		"Priority code of a comment"},
	{"commentResponse",
		"//commentStatus/commentResponse/@responseType",
		show_comment_response,
		edit_comment_response,
		NULL,
		"Response type of a comment"},
	{"commentTitle",
		"//commentAddressItems/commentTitle",
		show_simple_node,
		edit_simple_node,
		create_comment_title,
		"Title of a comment"},
	{"countryIsoCode",
		"//language/@countryIsoCode",
		show_country_iso_code,
		edit_country_iso_code,
		NULL,
		"Country ISO code (CA, US, GB...)"},
	{"dmCode",
		"//dmIdent/dmCode",
		show_dmcode,
		NULL,
		NULL,
		"Data module code"},
	{"url",
		"/",
		show_url,
		NULL,
		NULL,
		"URL of the document"},
	{"icnTitle",
		"//imfAddressItems/icnTitle",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Title of an IMF"},
	{"infoName",
		"//dmAddressItems/dmTitle/infoName",
		show_simple_node,
		edit_info_name,
		create_info_name,
		"Information name of a data module"},
	{"issueDate",
	 	"//issueDate",
		show_issue_date,
		edit_issue_date,
		NULL,
		"Issue date in ISO 8601 format (YYYY-MM-DD)"},
	{"issueNumber",
		"//issueInfo/@issueNumber",
		show_issue_number,
		edit_issue_number,
		NULL,
		"Issue number (NNN)"},
	{"inWork",
		"//issueInfo/@inWork",
		show_in_work,
		edit_in_work,
		NULL,
		"Inwork issue number (NN)"},
	{"issueType",
		"//dmStatus/@issueType|//pmStatus/@issueType",
		show_issue_type,
		edit_issue_type,
		NULL,
		"Issue type (new, changed, deleted...)"},
	{"languageIsoCode",
		"//language/@languageIsoCode",
		show_language_iso_code,
		edit_language_iso_code,
		NULL,
		"Language ISO code (en, fr, es...)"},
	{"originator",
		"//originator/enterpriseName",
		show_simple_node,
		edit_simple_node,
		create_orig_name,
		"Name of the originator"},
	{"originatorCode",
		"//originator/@enterpriseCode",
		show_ent_code,
		edit_ent_code,
		create_orig_ent_code,
		"NCAGE code of the originator"},
	{"path",
		"false()",
		NULL,
		NULL,
		NULL,
		"Filesystem path of object"},
	{"pmTitle",
		"//pmAddressItems/pmTitle",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Title of a publication module"},
	{"responsiblePartnerCompany",
		"//responsiblePartnerCompany/enterpriseName",
		show_simple_node,
		edit_simple_node,
		create_rpc_name,
		"Name of the RPC"},
	{"responsiblePartnerCompanyCode",
		"//responsiblePartnerCompany/@enterpriseCode",
		show_ent_code,
		edit_ent_code,
		create_rpc_ent_code,
		"NCAGE code of the RPC"},
	{"schema",
		"/*",
		show_schema,
		edit_schema,
		NULL,
		"XML schema URI"},
	{"securityClassification",
		"//security",
		show_sec_class,
		edit_sec_class,
		NULL,
		"Security classification (01, 02...)"},
	{"shortPmTitle",
		"//pmAddressItems/shortPmTitle",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Short title of a publication module"},
	{"techName",
		"//dmAddressItems/dmTitle/techName",
		show_simple_node,
		edit_simple_node,
		NULL,
		"Technical name of a data module"},
	{"title",
		"//dmAddressItems/dmTitle|//pmAddressItems/pmTitle|"
		"//commentAddressItems/commentTitle|//imfAddressItems/icnTitle",
		show_title,
		NULL,
		NULL,
		"Title of a CSDB object"},
	{"type",
		"/*",
		show_type,
		NULL,
		NULL,
		"Name of the root element of the document"},
	{NULL}
};

int show_metadata(xmlXPathContextPtr ctxt, const char *key, int endl)
{
	int i;

	for (i = 0; metadata[i].key; ++i) {
		if (strcmp(key, metadata[i].key) == 0) {
			xmlNodePtr node;
			if (!(node = first_xpath_node(metadata[i].path, ctxt))) {
				putchar(endl);
				return EXIT_MISSING_METADATA;
			}
			if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;
			metadata[i].show(node, endl);
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

int show_all_metadata(xmlXPathContextPtr ctxt, int formatall, int endl, int only_editable)
{
	int i;

	for (i = 0; metadata[i].key; ++i) {
		xmlNodePtr node;

		if (only_editable && !metadata[i].edit) continue;

		if ((node = first_xpath_node(metadata[i].path, ctxt))) {
			if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;

			if (endl == '\n') {
				printf("%s", metadata[i].key);

				if (formatall) {
					int n = KEY_COLUMN_WIDTH - strlen(metadata[i].key);
					int j;
					for (j = 0; j < n; ++j) putchar(' ');
				} else {
					putchar('\t');
				}
			}

			metadata[i].show(node, endl);
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

void list_metadata_key(const char *key, const char *descr, int formatall)
{
	int n = KEY_COLUMN_WIDTH - strlen(key);
	printf("%s", key);
	if (formatall) {
		int j;
		for (j = 0; j < n; ++j) putchar(' ');
	} else {
		putchar('\t');
	}
	printf("%s", descr);
	putchar('\n');
}

int has_key(xmlNodePtr keys, const char *key)
{
	if (keys->children) {
		xmlNodePtr cur;
		for (cur = keys->children; cur; cur = cur->next) {
			xmlChar *name;
			int match;

			name = xmlGetProp(cur, BAD_CAST "name");
			match = xmlStrcmp(name, BAD_CAST key) == 0;
			xmlFree(name);

			if (match) return 1;
		}

		return 0;
	}

	return 1;
}

void list_metadata_keys(xmlNodePtr keys, int formatall)
{
	int i;
	for (i = 0; metadata[i].key; ++i) {
		if (has_key(keys, metadata[i].key)) {
			list_metadata_key(metadata[i].key, metadata[i].descr, formatall);
		}
	}
}

void show_help(void)
{
	puts("Usage:");
	puts("  " PROG_NAME " [-h?]");
	puts("  " PROG_NAME " -H [-n <name>]...");
	puts("  " PROG_NAME " -c <file> [-flq] [<module>...]");
	puts("  " PROG_NAME " [-0flqTt] [-n <name> [-v <value>]]... [<module>]");
	puts("  " PROG_NAME " -F <fmt> [-lq] [<module>...]");
	puts("");
	puts("Options:");
	puts("  -0           Use null-delimited fields.");
	puts("  -c <file>    Set metadata using definitions in <file> (- for stdin).");
	puts("  -e           Include only editable metadata when showing all.");
	puts("  -f           Overwrite modules when editing metadata.");
	puts("  -H           List information on available metadata.");
	puts("  -l           Input is a list of filenames.");
	puts("  -n <name>    Specific metadata name to view/edit.");
	puts("  -q           Quiet mode, do not show non-fatal errors.");
	puts("  -T           Do not format columns in output.");
	puts("  -t           Use tab-delimited fields.");
	puts("  -v <value>   Edit the value of the metadata specified.");
	puts("  <module>     S1000D module to view/edit metadata on.");
}

void show_err(int err, const char *key, const char *val, const char *fname)
{
	if (verbosity < NORMAL) return;

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
}

int show_path(const char *fname, int endl)
{
	printf("%s", fname);
	if (endl > -1) putchar(endl);
	return 0;
}

int show_metadata_fmtstr_key(xmlXPathContextPtr ctx, const char *k, int n)
{
	int i;
	char *key;

	key = malloc(n + 1);
	sprintf(key, "%.*s", n, k);

	for (i = 0; metadata[i].key; ++i) {
		if (strcmp(metadata[i].key, key) == 0) {
			xmlNodePtr node;
			if (!(node = first_xpath_node(metadata[i].path, ctx))) {
				show_err(EXIT_MISSING_METADATA, key, NULL, NULL);
				return EXIT_MISSING_METADATA;
			}
			if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;
			metadata[i].show(node, -1);
			return EXIT_SUCCESS;
		}
	}

	show_err(EXIT_INVALID_METADATA, key, NULL, NULL);
	return EXIT_INVALID_METADATA;
}

int show_metadata_fmtstr(const char *fname, xmlXPathContextPtr ctx, const char *fmt)
{
	int i;
	for (i = 0; fmt[i]; ++i) {
		if (fmt[i] == FMTSTR_DELIM) {
			if (fmt[i + 1] == FMTSTR_DELIM) {
				putchar(FMTSTR_DELIM);
				++i;
			} else {
				const char *k, *e;
				int n;
				k = fmt + i + 1;
				e = strchr(k, FMTSTR_DELIM);
				if (!e) break;
				n = e - k;

				if (strncmp(k, "path", n) == 0) {
					show_path(fname, -1);
				} else {
					show_metadata_fmtstr_key(ctx, k, n);
				}

				i += n + 1;
			}
		} else if (fmt[i] == '\\') {
			switch (fmt[i + 1]) {
				case 'n': putchar('\n'); ++i; break;
				case 't': putchar('\t'); ++i; break;
				case '0': putchar('\0'); ++i; break;
				default: putchar(fmt[i]);
			}
		} else {
			putchar(fmt[i]);
		}
	}
	putchar('\n');
	return 0;
}

int show_or_edit_metadata(const char *fname, const char *metadata_fname,
	xmlNodePtr keys, int formatall, int overwrite, int endl,
	int only_editable, const char *fmtstr)
{
	int err;
	xmlDocPtr doc;
	xmlXPathContextPtr ctxt;
	int edit = 0;

	doc = xmlReadFile(fname, NULL, 0);

	ctxt = xmlXPathNewContext(doc);

	if (fmtstr) {
		err = show_metadata_fmtstr(fname, ctxt, fmtstr);
	} else if (keys->children) {
		xmlNodePtr cur;
		for (cur = keys->children; cur; cur = cur->next) {
			char *key = NULL, *val = NULL;

			key = (char *) xmlGetProp(cur, BAD_CAST "name");
			val = (char *) xmlGetProp(cur, BAD_CAST "value");

			if (val) {
				edit = 1;
				err = edit_metadata(ctxt, key, val);
			} else if (strcmp(key, "path") == 0) {
				err = show_path(fname, endl);
			} else {
				err = show_metadata(ctxt, key, endl);
			}

			show_err(err, key, val, fname);

			xmlFree(key);
			xmlFree(val);
		}
	} else if (metadata_fname) {
			FILE *input;

			edit = 1;

			if (strcmp(metadata_fname, "-") == 0) {
				input = stdin;
			} else {
				input = fopen(metadata_fname, "r");
			}

			err = edit_all_metadata(input, ctxt);

			fclose(input);
	} else {
			err = show_all_metadata(ctxt, formatall, endl, only_editable);
	}

	xmlXPathFreeContext(ctxt);

	if (endl != '\n') {
		putchar('\n');
	}

	if (edit) {
		if (overwrite) {
			if (access(fname, W_OK) != -1) {
				xmlSaveFile(fname, doc);
			} else {
				fprintf(stderr, ERR_PREFIX "%s does not have write permission.\n", fname);
				exit(EXIT_NO_WRITE);
			}
		} else {
			xmlSaveFile("-", doc);
		}
	}

	xmlFreeDoc(doc);

	return err;
}

void add_key(xmlNodePtr keys, const char *name)
{
	xmlNodePtr key;
	key = xmlNewChild(keys, NULL, BAD_CAST "key", NULL);
	xmlSetProp(key, BAD_CAST "name", BAD_CAST name);
}

void add_val(xmlNodePtr keys, const char *val)
{
	xmlNodePtr key;
	key = keys->last;
	xmlSetProp(key, BAD_CAST "value", BAD_CAST val);
}

int show_or_edit_metadata_list(const char *fname, const char *metadata_fname,
	xmlNodePtr keys, int formatall, int overwrite, int endl,
	int only_editable, const char *fmtstr)
{
	FILE *f;
	char path[PATH_MAX];
	int err = 0, i = 0;

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			fprintf(stderr, ERR_PREFIX "Could not read list file '%s'.\n", fname);
			exit(EXIT_NO_FILE);
		}
	} else {
		f = stdin;
	}

	while (fgets(path, PATH_MAX, f)) {
		strtok(path, "\t\n");

		if (i > 0) {
			putchar('\n');
		}

		err += show_or_edit_metadata(path, metadata_fname, keys,
			formatall, overwrite, endl, only_editable, fmtstr);
		++i;
	}

	if (fname) {
		fclose(f);
	}

	return err;
}

int main(int argc, char **argv)
{
	xmlNodePtr keys;
	int err = 0;

	int i;
	char *metadata_fname = NULL;
	int formatall = 1;
	int overwrite = 0;
	int endl = '\n';
	int list_keys = 0;
	int islist = 0;
	int only_editable = 0;
	char *fmtstr = NULL;

	keys = xmlNewNode(NULL, BAD_CAST "keys");

	while ((i = getopt(argc, argv, "0c:eF:fHln:Ttv:qh?")) != -1) {
		switch (i) {
			case '0': endl = '\0'; break;
			case 'c': metadata_fname = strdup(optarg); break;
			case 'e': only_editable = 1; break;
			case 'F': fmtstr = strdup(optarg); break;
			case 'f': overwrite = 1; break;
			case 'H': list_keys = 1; break;
			case 'l': islist = 1; break;
			case 'n': add_key(keys, optarg); break;
			case 'T': formatall = 0; break;
			case 't': endl = '\t'; break;
			case 'v': add_val(keys, optarg); break;
			case 'q': verbosity = SILENT; break;
			case 'h':
			case '?': show_help(); exit(0);
		}
	}

	if (list_keys) {
		list_metadata_keys(keys, formatall);
	} else if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (i > optind) {
				putchar('\n');
			}

			if (islist) {
				err += show_or_edit_metadata_list(argv[i],
					metadata_fname, keys, formatall,
					overwrite, endl, only_editable, fmtstr);
			} else {
				err += show_or_edit_metadata(argv[i],
					metadata_fname, keys, formatall,
					overwrite, endl, only_editable, fmtstr);
			}
		}
	} else if (islist) {
		err = show_or_edit_metadata_list(NULL, metadata_fname, keys, formatall,
			overwrite, endl, only_editable, fmtstr);
	} else {
		err = show_or_edit_metadata("-", metadata_fname, keys, formatall,
			overwrite, endl, only_editable, fmtstr);
	}

	free(metadata_fname);
	xmlFreeNode(keys);

	xmlCleanupParser();

	return err;
}
