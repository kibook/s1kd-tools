#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/debugXML.h>

#define STRUCT_OBJ_RULE_PATH "/dmodule/content/brex/contextRules[not(@rulesContext) or @rulesContext='%s']/structureObjectRuleGroup/structureObjectRule"
#define BREX_REF_DMCODE_PATH (xmlChar *) "//brexDmRef/dmRef/dmRefIdent/dmCode"

#define XSI_URI (xmlChar *) "http://www.w3.org/2001/XMLSchema-instance"

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

enum verbosity {SILENT, NORMAL, MESSAGE, INFO, DEBUG};

enum verbosity verbose = NORMAL;
bool shortmsg = false;

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
		object = xmlNewChild(brexError, NULL, (xmlChar *) "object", NULL);
		xmlSetProp(object, (xmlChar *) "line", (xmlChar *) line_s);

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

	modelIdentCode     = (char *) xmlGetProp(dmCode, (xmlChar *) "modelIdentCode");
	systemDiffCode     = (char *) xmlGetProp(dmCode, (xmlChar *) "systemDiffCode");
	systemCode         = (char *) xmlGetProp(dmCode, (xmlChar *) "systemCode");
	subSystemCode      = (char *) xmlGetProp(dmCode, (xmlChar *) "subSystemCode");
	subSubSystemCode   = (char *) xmlGetProp(dmCode, (xmlChar *) "subSubSystemCode");
	assyCode           = (char *) xmlGetProp(dmCode, (xmlChar *) "assyCode");
	disassyCode        = (char *) xmlGetProp(dmCode, (xmlChar *) "disassyCode");
	disassyCodeVariant = (char *) xmlGetProp(dmCode, (xmlChar *) "disassyCodeVariant");
	infoCode           = (char *) xmlGetProp(dmCode, (xmlChar *) "infoCode");
	infoCodeVariant    = (char *) xmlGetProp(dmCode, (xmlChar *) "infoCodeVariant");
	itemLocationCode   = (char *) xmlGetProp(dmCode, (xmlChar *) "itemLocationCode");

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
	xmlXPathRegisterNs(context, (xmlChar *) "xsi", XSI_URI);

	for (i = 0; i < rules->nodeNr; ++i) {
		char *allowedObjectFlag;
		char *path;
		char *use;

		objectPath = find_child(rules->nodeTab[i], "objectPath");
		objectUse = find_child(rules->nodeTab[i], "objectUse");

		allowedObjectFlag = (char *) xmlGetProp(objectPath, (xmlChar *) "allowedObjectFlag");
		path = (char *) xmlNodeGetContent(objectPath);
		use  = (char *) xmlNodeGetContent(objectUse);

		object = xmlXPathEvalExpression((xmlChar *) path, context);

		if (!object) {
			if (verbose > SILENT) {
				fprintf(stderr, E_INVOBJPATH);
			}
			exit(ERR_INVALID_OBJ_PATH);
		}

		if (is_invalid(allowedObjectFlag, object->nodesetval)) {
			char rpath[PATH_MAX];

			brexError = xmlNewChild(brexCheck, NULL, (xmlChar *) "brexError",
				NULL);
			xmlNewChild(brexError, NULL, (xmlChar *) "document", (xmlChar *) realpath(fname, rpath));
			xmlNewChild(brexError, NULL, (xmlChar *) "brex", (xmlChar *) realpath(brexfname, rpath));
			xmlNewChild(brexError, NULL, (xmlChar *) "objectPath", (xmlChar *) path);
			xmlNewChild(brexError, NULL, (xmlChar *) "objectUse", (xmlChar *) use);

			if (!xmlXPathNodeSetIsEmpty(object->nodesetval)) {
				dump_nodes_xml(object->nodesetval, fname,
					brexError);
			}

			++nerr;
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
	puts("Usage: s1kd-brexcheck [-b <brex>] [-I <path>] [-vVqsxlh?] <datamodules>");
	puts("");
	puts("Options:");
	puts("	-b <brex>    Use <brex> as the BREX data module");
	puts("	-I <path>    Add <path> to search path for BREX data module.");
	puts("	-v -V -q -D  Message verbosity.");
	puts("	-s           Short messages.");
	puts("	-x           XML output.");
	puts("  -l           Check BREX referenced by other BREX.");
	puts("	-h -?        Show this help message.");
}

int check_brex(xmlDocPtr dmod_doc, const char *docname,
	char brex_fnames[BREX_MAX][PATH_MAX], int num_brex_fnames, xmlNodePtr brexCheck)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	xmlDocPtr brex_doc;

	int i;
	int status;
	int total = 0;

	char *schema;
	char xpath[512];

	schema = (char *) xmlGetProp(xmlDocGetRootElement(dmod_doc), BAD_CAST "noNamespaceSchemaLocation");
	sprintf(xpath, STRUCT_OBJ_RULE_PATH, schema);
	xmlFree(schema);

	for (i = 0; i < num_brex_fnames; ++i) {
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
				printf(status ? E_INVALIDDOC : E_VALIDDOC, docname, brex_fnames[i]);
			}

			total += status;
		} else if (verbose >= MESSAGE) {
			printf(E_VALIDDOC, docname, brex_fnames[i]);
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
	} else if (strcmp((char *) node->name, "object") == 0 && !shortmsg) {
		char *line = (char *) xmlGetProp(node, (xmlChar *) "line");
		printf("  line %s:\n", line);
		xmlFree(line);

		xmlDebugDumpOneNode(stdout, node->children, 2);
	}

	for (cur = node->children; cur; cur = cur->next) {
		print_node(cur);
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

	xmlDocPtr outdoc;
	xmlNodePtr brexCheck;

	while ((c = getopt(argc, argv, "b:I:xvVDqslh?")) != -1) {
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

	outdoc = xmlNewDoc((xmlChar *) "1.0");
	brexCheck = xmlNewNode(NULL, (xmlChar *) "brexCheck");
	xmlDocSetRootElement(outdoc, brexCheck);

	for (i = 0; i < num_dmod_fnames; ++i) {
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
		}

		if (layered) {
			num_brex_fnames = add_layered_brex(brex_fnames,
				num_brex_fnames, brex_search_paths,
				num_brex_search_paths);
		}

		status += check_brex(dmod_doc, dmod_fnames[i],
			brex_fnames, num_brex_fnames, brexCheck);

		xmlFreeDoc(dmod_doc);
	}

	if (status == 0) {
		xmlNewChild(brexCheck, NULL, (xmlChar *) "noErrors", NULL);
	}

	if (!xmlout) {
		if (verbose > SILENT) {
			print_node(brexCheck);
		}
	} else {
		xmlSaveFormatFile("-", outdoc, 1);
	}

	xmlFreeDoc(outdoc);

	xmlCleanupParser();

	free(brex_fnames);
	free(brex_search_paths);
	free(dmod_fnames);

	return status;
}
