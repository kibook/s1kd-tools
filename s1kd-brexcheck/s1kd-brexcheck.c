#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/debugXML.h>

#define STRUCT_OBJ_RULE_PATH "/dmodule/content/brex/contextRules[not(@rulesContext) or @rulesContext='%s']/structureObjectRuleGroup/structureObjectRule"
#define BREX_REF_DMCODE_PATH BAD_CAST "//brexDmRef/dmRef/dmRefIdent/dmCode"

#define XSI_URI BAD_CAST "http://www.w3.org/2001/XMLSchema-instance"

#define PROG_NAME "s1kd-brexcheck"

#define E_PREFIX PROG_NAME ": ERROR: "
#define F_PREFIX PROG_NAME ": FAILED: "
#define S_PREFIX PROG_NAME ": SUCCESS: "
#define I_PREFIX PROG_NAME ": INFO: "

#define E_NODMOD E_PREFIX "Data module %s not found.\n"
#define E_NODMOD_STDIN E_PREFIX "stdin does not contain valid XML.\n"
#define E_NOBREX E_PREFIX "No BREX data module found for %s.\n"
#define E_NOBREX_STDIN E_PREFIX "No BREX found for data module on stdin.\n"
#define E_MAXBREX E_PREFIX "Too many BREX data modules specified.\n"
#define E_MAXBREXPATH E_PREFIX "Too many BREX search paths specified.\n"
#define E_MAXDMOD E_PREFIX "Too many data modules specified.\n"
#define E_INVOBJPATH E_PREFIX "Invalid object path.\n"
#define E_MISSBREX E_PREFIX "Could not find BREX file \"%s\".\n"
#define E_NOBREX_LAYER E_PREFIX "No BREX data module found for BREX %s.\n"

#define E_INVALIDDOC F_PREFIX "%s failed to validate against BREX %s.\n"
#define E_VALIDDOC S_PREFIX "%s validated successfully against BREX %s.\n"

#define I_FILE_FOUND I_PREFIX "Found file for BREX %s: %s\n"

#define ERR_MISSING_BREX_FILE -1
#define ERR_UNFINDABLE_BREX_FILE -2
#define ERR_MISSING_DMODULE -3
#define ERR_INVALID_OBJ_PATH -4
#define ERR_MAX_BREX -5
#define ERR_MAX_BREX_PATH -6
#define ERR_MAX_DMOD -7

#define BREX_MAX 1024
#define BREX_PATH_MAX 1024
#define DMOD_MAX 10240

#define PROGRESS_BAR_WIDTH 60

enum verbosity {SILENT, NORMAL, MESSAGE, INFO, DEBUG};

enum verbosity verbose = NORMAL;
bool shortmsg = false;

char *brsl_fname = NULL;
xmlDocPtr brsl;

bool check_sns = false;
bool strict_sns = false;
bool unstrict_sns = false;

bool check_notation = false;

xmlNodePtr find_child(xmlNodePtr parent, const char *name)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, name) == 0) {
			return cur;
		}
	}

	return NULL;
}

void dump_nodes_xml(xmlNodeSetPtr nodes, const char *fname, xmlNodePtr brexError)
{
	int i;

	for (i = 0; i < nodes->nodeNr; ++i) {
		xmlNodePtr node = nodes->nodeTab[i];
		xmlNodePtr object;
		char line_s[256];

		if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;

		snprintf(line_s, 256, "%d", node->line);
		object = xmlNewChild(brexError, NULL, BAD_CAST "object", NULL);
		xmlSetProp(object, BAD_CAST "line", BAD_CAST line_s);

		xmlAddChild(object, xmlCopyNode(node, 2));
	}
}

bool is_xml_file(const char *fname)
{
	return strcasecmp(fname + (strlen(fname) - 4), ".XML") == 0;
}

bool search_brex_fname(char *fname, const char *dpath, const char *dmcode)
{
	DIR *dir;
	struct dirent *cur;

	bool found = false;

	char tmp_fname[256] = "";

	dir = opendir(dpath);

	while ((cur = readdir(dir))) {
		if (strncmp(cur->d_name, dmcode, strlen(dmcode)) == 0) {

			sprintf(tmp_fname, "%s/%s", dpath, cur->d_name);

			if (is_xml_file(tmp_fname) && (!found || strcmp(tmp_fname, fname) > 0)) {
				strcpy(fname, tmp_fname);
			}
			
			found = true;
		}
	}

	closedir(dir);

	return found;
}

bool find_brex_fname_from_doc(char *fname, xmlDocPtr doc, char spaths[BREX_PATH_MAX][PATH_MAX],
	int nspaths)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr object;

	xmlNodePtr dmCode;

	char *modelIdentCode;
	char *systemDiffCode;
	char *systemCode;
	char *subSystemCode;
	char *subSubSystemCode;
	char *assyCode;
	char *disassyCode;
	char *disassyCodeVariant;
	char *infoCode;
	char *infoCodeVariant;
	char *itemLocationCode;

	char dmcode[256];

	bool found;

	context = xmlXPathNewContext(doc);

	object = xmlXPathEvalExpression(BREX_REF_DMCODE_PATH, context);

	if (xmlXPathNodeSetIsEmpty(object->nodesetval))
		return false;

	dmCode = object->nodesetval->nodeTab[0];

	xmlXPathFreeObject(object);
	xmlXPathFreeContext(context);

	modelIdentCode     = (char *) xmlGetProp(dmCode, BAD_CAST "modelIdentCode");
	systemDiffCode     = (char *) xmlGetProp(dmCode, BAD_CAST "systemDiffCode");
	systemCode         = (char *) xmlGetProp(dmCode, BAD_CAST "systemCode");
	subSystemCode      = (char *) xmlGetProp(dmCode, BAD_CAST "subSystemCode");
	subSubSystemCode   = (char *) xmlGetProp(dmCode, BAD_CAST "subSubSystemCode");
	assyCode           = (char *) xmlGetProp(dmCode, BAD_CAST "assyCode");
	disassyCode        = (char *) xmlGetProp(dmCode, BAD_CAST "disassyCode");
	disassyCodeVariant = (char *) xmlGetProp(dmCode, BAD_CAST "disassyCodeVariant");
	infoCode           = (char *) xmlGetProp(dmCode, BAD_CAST "infoCode");
	infoCodeVariant    = (char *) xmlGetProp(dmCode, BAD_CAST "infoCodeVariant");
	itemLocationCode   = (char *) xmlGetProp(dmCode, BAD_CAST "itemLocationCode");

	snprintf(dmcode, 256, "DMC-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
		modelIdentCode,
		systemDiffCode,
		systemCode,
		subSystemCode,
		subSubSystemCode,
		assyCode,
		disassyCode,
		disassyCodeVariant,
		infoCode,
		infoCodeVariant,
		itemLocationCode);

	xmlFree(modelIdentCode);
	xmlFree(systemDiffCode);
	xmlFree(systemCode);
	xmlFree(subSystemCode);
	xmlFree(subSubSystemCode);
	xmlFree(assyCode);
	xmlFree(disassyCode);
	xmlFree(disassyCodeVariant);
	xmlFree(infoCode);
	xmlFree(infoCodeVariant);
	xmlFree(itemLocationCode);

	found = search_brex_fname(fname, ".", dmcode);

	if (!found) {
		int i;

		for (i = 0; i < nspaths; ++i) {
			found = search_brex_fname(fname, spaths[i], dmcode);
		}
	}

	if (verbose >= INFO && found) {
		fprintf(stderr, I_FILE_FOUND, dmcode, fname);
	}

	return found;
}

bool is_invalid(char *allowedObjectFlag, xmlNodeSetPtr nodesetval)
{
	return (strcmp(allowedObjectFlag, "0") == 0 &&
	        !xmlXPathNodeSetIsEmpty(nodesetval)) ||
	       (strcmp(allowedObjectFlag, "1") == 0 &&
	        xmlXPathNodeSetIsEmpty(nodesetval));
}

bool is_failure(xmlChar *severity)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	bool ret = true;

	ctx = xmlXPathNewContext(brsl);
	obj = xmlXPathEvalExpression(BAD_CAST "//brSeverityLevel", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *value;
			bool match;

			value = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "value");
			match = xmlStrcmp(value, severity) == 0;
			xmlFree(value);

			if (match) {
				xmlChar *fail;

				fail = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "fail");
				ret = xmlStrcmp(fail, BAD_CAST "no") != 0;
				xmlFree(fail);
				break;
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return ret;
}

xmlChar *brsl_type(xmlChar *severity)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	char xpath[256];
	xmlChar *type;

	sprintf(xpath, "//brSeverityLevel[@value = '%s']", (char *) severity);

	ctx = xmlXPathNewContext(brsl);
	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		type = NULL;
	} else {
		type = xmlNodeGetContent(obj->nodesetval->nodeTab[0]);
	}
	
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return type;
}

char *real_path(const char *path, char *real)
{
	#ifdef _WIN32
	GetFullPathName(path, PATH_MAX, real, NULL);
	return real;
	#else
	return realpath(path, real);
	#endif
}

void add_object_values(xmlNodePtr brexError, xmlNodePtr rule)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(rule->doc);
	ctx->node = rule;

	obj = xmlXPathEvalExpression(BAD_CAST "objectValue", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlAddChild(brexError, xmlCopyNode(obj->nodesetval->nodeTab[i], 1));
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

int check_brex_rules(xmlNodeSetPtr rules, xmlDocPtr doc, const char *fname,
	const char *brexfname, xmlNodePtr brexCheck)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr object;

	xmlNodePtr objectPath;
	xmlNodePtr objectUse;

	int i;

	int nerr = 0;
	
	xmlNodePtr brexError;

	context = xmlXPathNewContext(doc);
	xmlXPathRegisterNs(context, BAD_CAST "xsi", XSI_URI);

	for (i = 0; i < rules->nodeNr; ++i) {
		char *allowedObjectFlag;
		char *path;
		char *use;

		objectPath = find_child(rules->nodeTab[i], "objectPath");
		objectUse = find_child(rules->nodeTab[i], "objectUse");

		allowedObjectFlag = (char *) xmlGetProp(objectPath, BAD_CAST "allowedObjectFlag");
		path = (char *) xmlNodeGetContent(objectPath);
		use  = (char *) xmlNodeGetContent(objectUse);

		object = xmlXPathEvalExpression(BAD_CAST path, context);

		if (!object) {
			if (verbose > SILENT) {
				fprintf(stderr, E_INVOBJPATH);
			}
			exit(ERR_INVALID_OBJ_PATH);
		}

		if (is_invalid(allowedObjectFlag, object->nodesetval)) {
			char rpath[PATH_MAX];
			xmlChar *severity;

			severity = xmlGetProp(rules->nodeTab[i], BAD_CAST "brSeverityLevel");

			brexError = xmlNewChild(brexCheck, NULL, BAD_CAST "brexError",
				NULL);

			xmlNewChild(brexError, NULL, BAD_CAST "document", BAD_CAST real_path(fname, rpath));
			xmlNewChild(brexError, NULL, BAD_CAST "brex", BAD_CAST real_path(brexfname, rpath));

			if (severity) {
				xmlNewChild(brexError, NULL, BAD_CAST "severity", severity);

				if (brsl_fname) {
					xmlChar *type = brsl_type(severity);
					xmlNewChild(brexError, NULL, BAD_CAST "type", type);
					xmlFree(type);
				}
			}

			xmlNewChild(brexError, NULL, BAD_CAST "objectPath", BAD_CAST path);
			xmlNewChild(brexError, NULL, BAD_CAST "objectUse", BAD_CAST use);

			add_object_values(brexError, rules->nodeTab[i]);

			if (!xmlXPathNodeSetIsEmpty(object->nodesetval)) {
				dump_nodes_xml(object->nodesetval, fname,
					brexError);
			}
			
			if (severity) {
				if (is_failure(severity)) {
					++nerr;
				}
			} else {
				++nerr;
			}

			xmlFree(severity);
		}

		xmlXPathFreeObject(object);
		xmlFree(allowedObjectFlag);
		xmlFree(path);
		xmlFree(use);
	}

	xmlXPathFreeContext(context);

	return nerr;
}

void show_help(void)
{
	puts("Usage: s1kd-brexcheck [-b <brex>] [-I <path>] [-vVqsxlStuph?] <datamodules>");
	puts("");
	puts("Options:");
	puts("  -b <brex>    Use <brex> as the BREX data module");
	puts("  -I <path>    Add <path> to search path for BREX data module.");
	puts("  -v -V -q -D  Message verbosity.");
	puts("  -s           Short messages.");
	puts("  -x           XML output.");
	puts("  -l           Check BREX referenced by other BREX.");
	puts("  -w <sev>     List of severity levels.");
	puts("  -S[tu]       Check SNS rules (normal, strict, unstrict)");
	puts("  -n           Check notation rules.");
	puts("  -p           Display progress bar.");
	puts("  -f           Output only filenames of invalid modules.");
	puts("  -h -?        Show this help message.");
}

xmlNodePtr firstXPathNode(xmlDocPtr doc, xmlNodePtr context, const char *xpath)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;

	ctx = xmlXPathNewContext(doc);
	ctx->node = context;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval))
		node = NULL;
	else
		node = obj->nodesetval->nodeTab[0];
	
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return node;
}

bool should_check(xmlChar *code, char *path, xmlDocPtr snsRulesDoc, xmlNodePtr ctx)
{
	bool ret;

	if (strict_sns) return true;

	if (unstrict_sns)
		return firstXPathNode(snsRulesDoc, ctx, path);

	if (strcmp(path, ".//snsSubSystem") == 0 || strcmp(path, ".//snsSubSubSystem") == 0) {
		ret = xmlStrcmp(code, BAD_CAST "0") != 0;
	} else {
		ret = !(xmlStrcmp(code, BAD_CAST "00") == 0 || xmlStrcmp(code, BAD_CAST "0000") == 0);
	}

	return ret || firstXPathNode(snsRulesDoc, ctx, path);
}

bool check_brex_sns(char brex_fnames[BREX_MAX][PATH_MAX], int nbrex_fnames, xmlDocPtr dmod_doc,
	const char *dmod_fname, xmlNodePtr brexCheck)
{
	xmlNodePtr dmcode;
	xmlChar *systemCode, *subSystemCode, *subSubSystemCode, *assyCode;
	char xpath[256];
	xmlNodePtr ctx = NULL;
	xmlNodePtr snsError;
	char rpath[PATH_MAX];
	char value[256];
	int i;
	xmlDocPtr snsRulesDoc;
	xmlNodePtr snsRulesGroup;

	/* Only check SNS in data modules. */
	if (xmlStrcmp(xmlDocGetRootElement(dmod_doc)->name, BAD_CAST "dmodule") != 0)
		return true;

	/* The valid SNS is taken as a combination of the snsRules from all specified BREX data modules. */
	snsRulesDoc = xmlNewDoc(BAD_CAST "1.0");
	xmlDocSetRootElement(snsRulesDoc, xmlNewNode(NULL, BAD_CAST "snsRulesGroup"));
	snsRulesGroup = xmlDocGetRootElement(snsRulesDoc);

	for (i = 0; i < nbrex_fnames; ++i) {
		xmlDocPtr brex;

		brex = xmlReadFile(brex_fnames[i], NULL, 0);

		xmlAddChild(snsRulesGroup, xmlCopyNode(firstXPathNode(brex, NULL, "//snsRules"), 1));

		xmlFreeDoc(brex);
	}

	dmcode = firstXPathNode(dmod_doc, NULL, "//dmIdent/dmCode");

	systemCode       = xmlGetProp(dmcode, BAD_CAST "systemCode");
	subSystemCode    = xmlGetProp(dmcode, BAD_CAST "subSystemCode");
	subSubSystemCode = xmlGetProp(dmcode, BAD_CAST "subSubSystemCode");
	assyCode         = xmlGetProp(dmcode, BAD_CAST "assyCode");

	snsError = xmlNewNode(NULL, BAD_CAST "snsError");
	xmlNewChild(snsError, NULL, BAD_CAST "document", BAD_CAST real_path(dmod_fname, rpath));

	sprintf(xpath, "//snsSystem[snsCode = '%s']", (char *) systemCode);
	if (!(ctx = firstXPathNode(snsRulesDoc, ctx, xpath))) {
		xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "systemCode");
		xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", systemCode);
		xmlAddChild(brexCheck, snsError);

		xmlFreeDoc(snsRulesDoc);
		return false;
	}

	if (should_check(subSystemCode, ".//snsSubSystem", snsRulesDoc, ctx)) {
		sprintf(xpath, ".//snsSubSystem[snsCode = '%s']", (char *) subSystemCode);
		if (!(ctx = firstXPathNode(snsRulesDoc, ctx, xpath))) {
			xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "subSystemCode");

			sprintf(value, "%s-%s", systemCode, subSystemCode);
			xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", BAD_CAST value);

			xmlAddChild(brexCheck, snsError);

			xmlFreeDoc(snsRulesDoc);
			return false;
		}
	}

	if (should_check(subSubSystemCode, ".//snsSubSubSystem", snsRulesDoc, ctx)) {
		sprintf(xpath, ".//snsSubSubSystem[snsCode = '%s']", (char *) subSubSystemCode);
		if (!(ctx = firstXPathNode(snsRulesDoc, ctx, xpath))) {
			xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "subSubSystemCode");

			sprintf(value, "%s-%s%s", systemCode, subSystemCode, subSubSystemCode);
			xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", BAD_CAST value);

			xmlAddChild(brexCheck, snsError);

			xmlFree(snsRulesDoc);
			return false;
		}
	}

	if (should_check(assyCode, ".//snsAssy", snsRulesDoc, ctx)) {
		sprintf(xpath, ".//snsAssy[snsCode = '%s']", (char *) assyCode);
		if (!firstXPathNode(snsRulesDoc, ctx, xpath)) {
			xmlNewChild(snsError, NULL, BAD_CAST "code", BAD_CAST "assyCode");

			sprintf(value, "%s-%s%s-%s", systemCode, subSystemCode, subSubSystemCode, assyCode);
			xmlNewChild(snsError, NULL, BAD_CAST "invalidValue", BAD_CAST value);

			xmlAddChild(brexCheck, snsError);

			xmlFreeDoc(snsRulesDoc);
			return false;
		}
	}

	xmlFreeNode(snsError);

	xmlFreeDoc(snsRulesDoc);
	return true;
}

int check_entity(xmlEntityPtr entity, xmlDocPtr notationRuleDoc,
	xmlNodePtr brexCheck, const char *docname)
{	
	char xpath[256];
	xmlNodePtr rule;
	xmlNodePtr notationError;

	sprintf(xpath, "//notationRule[notationName='%s' and notationName/@allowedNotationFlag!='0']",
		(char *) entity->content);

	if ((rule = firstXPathNode(notationRuleDoc, NULL, xpath)))
		return 0;

	sprintf(xpath, "(//notationRule[notationName='%s']|//notationRule)[1]", (char *) entity->content);
	rule = firstXPathNode(notationRuleDoc, NULL, xpath);

	notationError = xmlNewChild(brexCheck, NULL, BAD_CAST "notationError", NULL);
	xmlNewChild(notationError, NULL, BAD_CAST "document", BAD_CAST docname);
	xmlNewChild(notationError, NULL, BAD_CAST "invalidNotation", entity->content);
	xmlAddChild(notationError, xmlCopyNode(firstXPathNode(notationRuleDoc, rule, "objectUse"), 1));

	return 1;
}

int check_brex_notations(char brex_fnames[BREX_MAX][PATH_MAX], int nbrex_fnames, xmlDocPtr dmod_doc,
	const char *dmod_fname, xmlNodePtr brexCheck)
{
	xmlDocPtr notationRuleDoc;
	xmlNodePtr notationRuleGroup;
	int i;
	xmlDtdPtr dtd;
	xmlNodePtr cur;
	int invalid = 0;

	if (!(dtd = dmod_doc->intSubset))
		return 0;

	notationRuleDoc = xmlNewDoc(BAD_CAST "1.0");
	xmlDocSetRootElement(notationRuleDoc, xmlNewNode(NULL, BAD_CAST "notationRuleGroup"));
	notationRuleGroup = xmlDocGetRootElement(notationRuleDoc);

	for (i = 0; i < nbrex_fnames; ++i) {
		xmlDocPtr brex;

		brex = xmlReadFile(brex_fnames[i], NULL, 0);

		xmlAddChild(notationRuleGroup, xmlCopyNode(firstXPathNode(brex, NULL, "//notationRuleList"), 1));

		xmlFreeDoc(brex);
	}

	for (cur = dtd->children; cur; cur = cur->next) {
		if (cur->type == XML_ENTITY_DECL && ((xmlEntityPtr) cur)->etype == 3) {
			invalid += check_entity((xmlEntityPtr) cur, notationRuleDoc,
				brexCheck, dmod_fname);
		}
	}

	xmlFreeDoc(notationRuleDoc);

	return invalid;
}

int check_brex(xmlDocPtr dmod_doc, const char *docname,
	char brex_fnames[BREX_MAX][PATH_MAX], int num_brex_fnames, xmlNodePtr brexCheck)
{
	xmlDocPtr brex_doc;

	int i;
	int status;
	int total = 0;
	bool valid_sns = true;
	int invalid_notations = 0;

	char *schema;
	char xpath[512];

	schema = (char *) xmlGetProp(xmlDocGetRootElement(dmod_doc), BAD_CAST "noNamespaceSchemaLocation");
	sprintf(xpath, STRUCT_OBJ_RULE_PATH, schema);
	xmlFree(schema);

	if (check_sns && !(valid_sns = check_brex_sns(brex_fnames, num_brex_fnames, dmod_doc, docname, brexCheck)))
		++total;

	if (check_notation) {
		invalid_notations = check_brex_notations(brex_fnames, num_brex_fnames, dmod_doc, docname, brexCheck);
		total += invalid_notations;
	}

	for (i = 0; i < num_brex_fnames; ++i) {
		xmlXPathContextPtr context;
		xmlXPathObjectPtr result;

		brex_doc = xmlReadFile(brex_fnames[i], NULL, 0);

		if (!brex_doc) {
			if (verbose > SILENT) {
				fprintf(stderr, E_MISSBREX, brex_fnames[i]);
			}
			exit(ERR_MISSING_BREX_FILE);
		}

		context = xmlXPathNewContext(brex_doc);

		result = xmlXPathEvalExpression(BAD_CAST xpath, context);

		if (!xmlXPathNodeSetIsEmpty(result->nodesetval)) {
			status = check_brex_rules(result->nodesetval, dmod_doc, docname,
				brex_fnames[i], brexCheck);

			if (verbose >= MESSAGE) {
				fprintf(stderr, status || !valid_sns || invalid_notations ? E_INVALIDDOC : E_VALIDDOC, docname, brex_fnames[i]);
			}

			total += status;
		} else if (verbose >= MESSAGE) {
			fprintf(stderr, valid_sns && !invalid_notations ? E_VALIDDOC : E_INVALIDDOC, docname, brex_fnames[i]);
		}

		xmlXPathFreeObject(result);

		xmlXPathFreeContext(context);

		xmlFreeDoc(brex_doc);
	}

	return total;
}

void print_node(xmlNodePtr node)
{

	xmlNodePtr cur;

	if (strcmp((char *) node->name, "brexError") == 0) {
		printf("BREX ERROR: ");
	} else if (strcmp((char *) node->name, "type") == 0 && !shortmsg) {
		char *type = (char *) xmlNodeGetContent(node);
		printf("  TYPE: %s\n", type);
		xmlFree(type);
	} else if (strcmp((char *) node->name, "brex") == 0 && !shortmsg) {
		char *brex = (char *) xmlNodeGetContent(node);
		printf("  BREX: %s\n", brex);
		xmlFree(brex);
	} else if (strcmp((char *) node->name, "document") == 0) {
		char *doc = (char *) xmlNodeGetContent(node);
		if (shortmsg) {
			printf("%s: ", doc);
		} else {
			printf("%s\n", doc);
		}
		xmlFree(doc);
	} else if (strcmp((char *) node->name, "objectUse") == 0) {
		char *use = (char *) xmlNodeGetContent(node);
		if (!shortmsg) {
			printf("  ");
		}
		printf("%s\n", use);
		xmlFree(use);
	} else if (strcmp((char *) node->name, "objectValue") == 0 && !shortmsg) {
		char *allowed = (char *) xmlGetProp(node, BAD_CAST "valueAllowed");
		char *content = (char *) xmlNodeGetContent(node);
		printf("  VALUE ALLOWED: %s", content);
		if (allowed)
			printf(" (%s)", allowed);
		putchar('\n');
		xmlFree(content);
		xmlFree(allowed);
	} else if (strcmp((char *) node->name, "object") == 0 && !shortmsg) {
		char *line = (char *) xmlGetProp(node, BAD_CAST "line");
		printf("  line %s:\n", line);
		xmlFree(line);
		xmlDebugDumpOneNode(stdout, node->children, 2);
	} else if (strcmp((char *) node->name, "snsError") == 0) {
		printf("SNS ERROR: ");
	} else if (strcmp((char *) node->name, "code") == 0) {
		char *code = (char *) xmlNodeGetContent(node);
		if (!shortmsg) printf("  ");
		printf("Value of %s does not conform to SNS: ", code);
		xmlFree(code);
	} else if (strcmp((char *) node->name, "invalidValue") == 0) {
		char *value = (char *) xmlNodeGetContent(node);
		printf("%s\n", value);
		xmlFree(value);
	} else if (strcmp((char *) node->name, "notationError") == 0) {
		printf("NOTATION ERROR: ");
	} else if (strcmp((char *) node->name, "invalidNotation") == 0) {
		char *value = (char *) xmlNodeGetContent(node);
		if (!shortmsg) printf("  ");
		printf("Notation %s is not allowed", value);
		if (shortmsg)
			printf(": ");
		else
			printf(".\n");
		xmlFree(value);
	}

	for (cur = node->children; cur; cur = cur->next) {
		print_node(cur);
	}
}

void print_fnames(xmlNodePtr node)
{
	xmlNodePtr cur;

	if (xmlStrcmp(node->name, BAD_CAST "document") == 0) {
		xmlChar *fname;
		fname = xmlNodeGetContent(node);
		puts((char *) fname);
		xmlFree(fname);
	}

	for (cur = node->children; cur; cur = cur->next) {
		print_fnames(cur);
	}
}

bool brex_exists(char fname[PATH_MAX], char fnames[BREX_MAX][PATH_MAX], int nfnames, char spaths[BREX_PATH_MAX][PATH_MAX], int nspaths)
{
	int i;

	for (i = 0; i < nfnames; ++i) {
		if (strcmp(fname, fnames[i]) == 0) {
			return true;
		}
	}

	return false;
}

int add_layered_brex(char fnames[BREX_MAX][PATH_MAX], int nfnames, char spaths[BREX_PATH_MAX][PATH_MAX], int nspaths)
{
	int i;
	int total = nfnames;

	for (i = 0; i < nfnames; ++i) {
		xmlDocPtr doc;
		char fname[PATH_MAX];
		bool found;

		doc = xmlReadFile(fnames[i], NULL, 0);

		found = find_brex_fname_from_doc(fname, doc, spaths, nspaths);

		if (!found) {
			fprintf(stderr, E_NOBREX_LAYER, fnames[i]);
		} else if (!brex_exists(fname, fnames, nfnames, spaths, nspaths)) {
			strcpy(fnames[total++], fname);

			total = add_layered_brex(fnames, total, spaths, nspaths);
		}

		xmlFreeDoc(doc);
	}

	return total;
}

void show_progress(float cur, float total)
{
	float p;
	int i, b;

	p = cur / total;
	b = PROGRESS_BAR_WIDTH * p;

	fprintf(stderr, "\r[");
	for (i = 0; i < PROGRESS_BAR_WIDTH; ++i) {
		if (i < b)
			fputc('=', stderr);
		else
			fputc(' ', stderr);
	}
	fprintf(stderr, "] %d%% (%d/%d) ", (int)(p * 100.0), (int) cur, (int) total);
	if (cur == total)
		fputc('\n', stderr);
	fflush(stderr);
}

int main(int argc, char *argv[])
{
	int c;
	int i;

	xmlDocPtr dmod_doc;

	char (*brex_fnames)[PATH_MAX] = malloc(BREX_MAX * PATH_MAX);
	int num_brex_fnames = 0;

	char (*brex_search_paths)[PATH_MAX] = malloc(BREX_PATH_MAX * PATH_MAX);
	int num_brex_search_paths = 0;

	char (*dmod_fnames)[PATH_MAX] = malloc(DMOD_MAX * PATH_MAX);
	int num_dmod_fnames = 0;

	int status = 0;

	bool use_stdin = false;
	bool xmlout = false;
	bool layered = false;
	bool progress = false;
	bool only_fnames = false;

	xmlDocPtr outdoc;
	xmlNodePtr brexCheck;

	while ((c = getopt(argc, argv, "b:I:xvVDqslw:Stupfnh?")) != -1) {
		switch (c) {
			case 'b':
				if (num_brex_fnames == BREX_MAX) {
					if (verbose > SILENT) {
						fprintf(stderr, E_MAXBREX);
					}
					exit(ERR_MAX_BREX);
				} else {
					strcpy(brex_fnames[num_brex_fnames++], optarg);
				}
				break;
			case 'I':
				if (num_brex_search_paths == BREX_PATH_MAX) {
					if (verbose > SILENT) {
						fprintf(stderr, E_MAXBREXPATH);
					}
					exit(ERR_MAX_BREX_PATH);
				} else {
					strcpy(brex_search_paths[num_brex_search_paths],
						optarg);
				}
				++num_brex_search_paths;
				break;
			case 'x': xmlout = true; break;
			case 'q': verbose = SILENT; break;
			case 'v': verbose = MESSAGE; break;
			case 'V': verbose = INFO; break;
			case 'D': verbose = DEBUG; break;
			case 's': shortmsg = true; break;
			case 'l': layered = true; break;
			case 'w': brsl_fname = strdup(optarg); break;
			case 'S': check_sns = true; break;
			case 't': strict_sns = true; break;
			case 'u': unstrict_sns = true; break;
			case 'p': progress = true; break;
			case 'f': only_fnames = true; break;
			case 'n': check_notation = true; break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (i == DMOD_MAX) {
				if (verbose > SILENT) fprintf(stderr, E_MAXDMOD);
				exit(ERR_MAX_DMOD);
			} else {
				strcpy(dmod_fnames[num_dmod_fnames++], argv[i]);
			}
		}
	} else {
		strcpy(dmod_fnames[num_dmod_fnames++], "-");
		use_stdin = true;
	}

	if (brsl_fname) {
		brsl = xmlReadFile(brsl_fname, NULL, 0);
	}

	outdoc = xmlNewDoc(BAD_CAST "1.0");
	brexCheck = xmlNewNode(NULL, BAD_CAST "brexCheck");
	xmlDocSetRootElement(outdoc, brexCheck);

	for (i = 0; i < num_dmod_fnames; ++i) {
		/* Indicates if a referenced BREX data module is used as
		 * opposed to one specified on the command line.
		 *
		 * The practical difference is that those specified on the
		 * command line are meant to apply to ALL data modules
		 * specified, while a referenced BREX only applies to the data
		 * module which referenced it. */
		bool ref_brex = false;

		dmod_doc = xmlReadFile(dmod_fnames[i], NULL, 0);

		if (!dmod_doc) {
			if (use_stdin) {
				if (verbose > SILENT) fprintf(stderr, E_NODMOD_STDIN);
			} else {
				if (verbose > SILENT) fprintf(stderr, E_NODMOD, dmod_fnames[i]);
			}
			exit(ERR_MISSING_DMODULE);
		}

		if (num_brex_fnames == 0) {
			if (!find_brex_fname_from_doc(brex_fnames[0], dmod_doc,
				brex_search_paths, num_brex_search_paths)) {
				if (use_stdin) {
					if (verbose > SILENT) fprintf(stderr, E_NOBREX_STDIN);
				} else {
					if (verbose > SILENT) fprintf(stderr, E_NOBREX,
						dmod_fnames[i]);
				}
				continue;
			}

			num_brex_fnames = 1;
			ref_brex = true;

			/* When using brexDmRef, if the data module is itself a
			 * BREX data module, include it as a BREX. */
			if (strcmp(brex_fnames[0], dmod_fnames[i]) != 0 && firstXPathNode(dmod_doc, NULL, "//brex"))
				strcpy(brex_fnames[num_brex_fnames++], dmod_fnames[i]);
		}

		if (layered) {
			num_brex_fnames = add_layered_brex(brex_fnames,
				num_brex_fnames, brex_search_paths,
				num_brex_search_paths);
		}

		status += check_brex(dmod_doc, dmod_fnames[i],
			brex_fnames, num_brex_fnames, brexCheck);

		xmlFreeDoc(dmod_doc);

		if (progress) 
			show_progress(i, num_dmod_fnames);

		/* If the referenced BREX was used, reset the BREX data module
		 * list, as each data module may reference a different BREX or
		 * set of BREX. */
		if (ref_brex)
			num_brex_fnames = 0;
	}

	if (progress)
		show_progress(i, num_dmod_fnames);

	if (!brexCheck->children) {
		xmlNewChild(brexCheck, NULL, BAD_CAST "noErrors", NULL);
	}

	if (!xmlout) {
		if (verbose > SILENT) {
			if (only_fnames) {
				print_fnames(brexCheck);
			} else {
				print_node(brexCheck);
			}
		}
	} else {
		xmlSaveFormatFile("-", outdoc, 1);
	}

	xmlFreeDoc(outdoc);

	if (brsl_fname) {
		xmlFreeDoc(brsl);
	}

	xmlCleanupParser();

	free(brex_fnames);
	free(brex_search_paths);
	free(dmod_fnames);

	return status;
}
