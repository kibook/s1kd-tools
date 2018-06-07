#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/debugXML.h>
#include <libxslt/transform.h>
#include "templates.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-newupf"
#define VERSION "1.1.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_UPF_EXISTS 1
#define EXIT_MISSING_ARGS 2
#define EXIT_INVALID_ARGS 3
#define EXIT_BAD_ISSUE 4
#define EXIT_BAD_TEMPLATE 5

enum issue { NO_ISS, ISS_41, ISS_42 } issue = NO_ISS;

#define DEFAULT_S1000D_ISSUE ISS_42

#define CIR_OBJECT_XPATH \
	"//accessPointSpec|" \
	"//applicSpec|" \
	"//cautionSpec|" \
	"//circuitBreakerSpec|" \
	"//controlIndicatorSpec|" \
	"//enterpriseSpec|" \
	"//functionalItemSpec|" \
	"//partSpec|" \
	"//supplySpec|" \
	"//toolSpec|" \
	"//warningSpec"

typedef enum {
	NON_REPOSITORY,
	ACCESS_POINT_REPOSITORY,
	APPLIC_REPOSITORY,
	CAUTION_REPOSITORY,
	CIRCUIT_BREAKER_REPOSITORY,
	CONTROL_INDICATOR_REPOSITORY,
	ENTERPRISE_REPOSITORY,
	FUNCTIONAL_ITEM_REPOSITORY,
	PART_REPOSITORY,
	SUPPLY_REPOSITORY,
	TOOL_REPOSITORY,
	WARNING_REPOSITORY
} cirType;

char *templateDir = NULL;

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

enum issue getIssue(const char *iss)
{
	if (strcmp(iss, "4.2") == 0) {
		return ISS_42;
	} else if (strcmp(iss, "4.1") == 0) {
		return ISS_41;
	}

	fprintf(stderr, ERR_PREFIX "Unsupported issue: %s\n", iss);
	exit(EXIT_BAD_ISSUE);

	return NO_ISS;
}

xmlNodePtr nodeExists(const char *xpath, xmlXPathContextPtr ctx)
{
	xmlXPathObjectPtr obj;
	xmlNodePtr node;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);
	
	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		node = NULL;
	} else {
		node = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);

	return node;
}

xmlNodePtr firstXPathNode(const char *xpath, xmlXPathContextPtr ctx)
{
	xmlXPathObjectPtr obj;
	xmlNodePtr node;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		node = NULL;
	} else {
		node = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);

	return node;
}

cirType typeOfCir(xmlXPathContextPtr ctx)
{
	xmlNodePtr type;
	
	type = firstXPathNode("//content/commonRepository/*", ctx);

	if (xmlStrcmp(type->name, BAD_CAST "accessPointRepository") == 0) {
		return ACCESS_POINT_REPOSITORY;
	} else if (xmlStrcmp(type->name, BAD_CAST "applicRepository") == 0) {
		return APPLIC_REPOSITORY;
	} else if (xmlStrcmp(type->name, BAD_CAST "cautionRepository") == 0) {
		return CAUTION_REPOSITORY;
	} else if (xmlStrcmp(type->name, BAD_CAST "circuitBreakerRepository") == 0) {
		return CIRCUIT_BREAKER_REPOSITORY;
	} else if (xmlStrcmp(type->name, BAD_CAST "controlIndicatorRepository") == 0) {
		return CONTROL_INDICATOR_REPOSITORY;
	} else if (xmlStrcmp(type->name, BAD_CAST "enterpriseRepository") == 0) {
		return ENTERPRISE_REPOSITORY;
	} else if (xmlStrcmp(type->name, BAD_CAST "functionalItemRepository") == 0) {
		return FUNCTIONAL_ITEM_REPOSITORY;
	} else if (xmlStrcmp(type->name, BAD_CAST "partRepository") == 0) {
		return PART_REPOSITORY;
	} else if (xmlStrcmp(type->name, BAD_CAST "supplyRepository") == 0) {
		return SUPPLY_REPOSITORY;
	} else if (xmlStrcmp(type->name, BAD_CAST "toolRepository") == 0) {
		return TOOL_REPOSITORY;
	} else if (xmlStrcmp(type->name, BAD_CAST "warningRepository") == 0) {
		return WARNING_REPOSITORY;
	} else {
		return NON_REPOSITORY;
	}
}

/* XPath for specs which are distinguished by a single unique attribute */
xmlNodePtr xpathForBasicSpec(char *xpath, xmlNodePtr object, const char *attr)
{
	xmlNodePtr ident;
	xmlChar *value;

	ident = xmlFirstElementChild(object);
	value = xmlGetProp(ident, BAD_CAST attr);

	snprintf(xpath, 256, "//%s[%s/@%s='%s']",
		(char *) object->name,
		(char *) ident->name,
		attr,
		(char *) value);

	xmlFree(value);

	return ident;
}

xmlNodePtr xpathForPartSpec(char *xpath, xmlNodePtr object)
{
	xmlNodePtr ident;
	xmlChar *partNumberValue, *manufacturerCodeValue;

	ident = xmlFirstElementChild(object);
	partNumberValue = xmlGetProp(ident, BAD_CAST "partNumberValue");
	manufacturerCodeValue = xmlGetProp(ident, BAD_CAST "manufacturerCodeValue");

	snprintf(xpath, 256, "//partSpec[partIdent/@partNumberValue='%s' and partIdent/@manufacturerCodeValue='%s']",
		(char *) partNumberValue,
		(char *) manufacturerCodeValue);

	xmlFree(partNumberValue);
	xmlFree(manufacturerCodeValue);

	return ident;
}

xmlNodePtr xpathForToolSpec(char *xpath, xmlNodePtr object)
{
	xmlNodePtr ident;
	xmlChar *toolNumber, *manufacturerCodeValue;

	ident = xmlFirstElementChild(object);
	toolNumber = xmlGetProp(ident, BAD_CAST "toolNumber");
	manufacturerCodeValue = xmlGetProp(ident, BAD_CAST "manufacturerCodeValue");

	snprintf(xpath, 256, "//toolSpec[toolIdent/@toolNumber='%s' and toolIdent/@manufacturerCodeValue='%s']",
		(char *) toolNumber,
		(char *) manufacturerCodeValue);

	xmlFree(toolNumber);
	xmlFree(manufacturerCodeValue);

	return ident;
}

xmlNodePtr xpathForControlIndicatorSpec(char *xpath, xmlNodePtr object)
{
	xmlChar *controlIndicatorNumber;
	xmlNodePtr ident;

	controlIndicatorNumber = xmlGetProp(object, BAD_CAST "controlIndicatorNumber");

	snprintf(xpath, 256, "//controlIndicatorSpec[@controlIndicatorNumber='%s']",
		(char *) controlIndicatorNumber);
	
	ident = xmlNewNode(NULL, BAD_CAST "controlIndicatorIdent");
	xmlAddNextSibling(object, ident);
	xmlSetProp(ident, BAD_CAST "controlIndicatorNumber", controlIndicatorNumber);

	xmlFree(controlIndicatorNumber);

	return ident;
}

xmlNodePtr getNodeXPath(char *xpath, xmlNodePtr object)
{
	xmlNodePtr ident;
	xmlChar *id;


	if ((id = xmlGetProp(object, BAD_CAST "id"))) {
		snprintf(xpath, 256, "//%s[@id='%s']", (char *) object->name, (char *) id);
		ident = xmlFirstElementChild(object);
	} else  if (xmlStrcmp(object->name, BAD_CAST "accessPointSpec") == 0) {
		ident = xpathForBasicSpec(xpath, object, "accessPointNumber");
	} else if (xmlStrcmp(object->name, BAD_CAST "applicSpec") == 0) {
		ident = xpathForBasicSpec(xpath, object, "applicIdentValue");
	} else if (xmlStrcmp(object->name, BAD_CAST "cautionSpec") == 0) {
		ident = xpathForBasicSpec(xpath, object, "cautionIdentNumber");
	} else if (xmlStrcmp(object->name, BAD_CAST "circuitBreakerSpec") == 0) {
		ident = xpathForBasicSpec(xpath, object, "circuitBreakerNumber");
	} else if (xmlStrcmp(object->name, BAD_CAST "controlIndicatorSpec") == 0) {
		ident = xpathForControlIndicatorSpec(xpath, object);
	} else if (xmlStrcmp(object->name, BAD_CAST "enterpriseSpec") == 0) {
		ident = xpathForBasicSpec(xpath, object, "manufacturerCodeValue");
	} else if (xmlStrcmp(object->name, BAD_CAST "functionalItemSpec") == 0) {
		ident = xpathForBasicSpec(xpath, object, "functionalItemNumber");
	} else if (xmlStrcmp(object->name, BAD_CAST "partSpec") == 0) {
		ident = xpathForPartSpec(xpath, object);
	} else if (xmlStrcmp(object->name, BAD_CAST "supplySpec") == 0) {
		ident = xpathForBasicSpec(xpath, object, "supplyNumber");
	} else if (xmlStrcmp(object->name, BAD_CAST "toolSpec") == 0) {
		ident = xpathForToolSpec(xpath, object);
	} else if (xmlStrcmp(object->name, BAD_CAST "warningSpec") == 0) {
		ident = xpathForBasicSpec(xpath, object, "warningIdentNumber");
	} else {
		ident = NULL;
	}

	return ident;
}

xmlNodePtr deleteObjects(xmlXPathContextPtr src, xmlXPathContextPtr tgt)
{
	xmlNodePtr group;
	xmlXPathObjectPtr results;
	xmlNodeSetPtr objects;

	group = xmlNewNode(NULL, BAD_CAST "deleteObjectGroup");

	results = xmlXPathEvalExpression(BAD_CAST CIR_OBJECT_XPATH, src);
	objects = results->nodesetval;

	if (!xmlXPathNodeSetIsEmpty(objects)) {
		int i;

		for (i = 0; i < objects->nodeNr; ++i) {
			char xpath[256];
			xmlNodePtr ident;

			ident = getNodeXPath(xpath, objects->nodeTab[i]);

			if (!nodeExists(xpath, tgt)) {
				xmlNodePtr deleteObject;

				deleteObject = xmlNewChild(group, NULL, BAD_CAST "deleteObject", NULL);
				xmlAddChild(deleteObject, xmlCopyNode(ident, 1));
			}
		}
	}

	if (group->children) {
		return group;
	}

	xmlFreeNode(group);
	return NULL;
}

xmlNodePtr insertObjects(xmlXPathContextPtr src, xmlXPathContextPtr tgt)
{
	xmlNodePtr group;
	xmlXPathObjectPtr results;
	xmlNodeSetPtr objects;

	group = xmlNewNode(NULL, BAD_CAST "insertObjectGroup");

	results = xmlXPathEvalExpression(BAD_CAST CIR_OBJECT_XPATH, tgt);
	objects = results->nodesetval;

	if (!xmlXPathNodeSetIsEmpty(objects)) {
		int i;

		for (i = 0; i < objects->nodeNr; ++i) {
			char xpath[256];

			getNodeXPath(xpath, objects->nodeTab[i]);

			if (!nodeExists(xpath, src)) {
				xmlNodePtr insertObject, before, after;

				insertObject = xmlNewChild(group, NULL, BAD_CAST "insertObject", NULL);

				before = xmlPreviousElementSibling(objects->nodeTab[i]);
				after  = xmlNextElementSibling(objects->nodeTab[i]);

				if (before) {
					getNodeXPath(xpath, before);
					xmlSetProp(insertObject, BAD_CAST "targetPath", BAD_CAST xpath);
					xmlSetProp(insertObject, BAD_CAST "insertionOrder", BAD_CAST "after");
				} else if (after) {
					getNodeXPath(xpath, after);
					xmlSetProp(insertObject, BAD_CAST "targetPath", BAD_CAST xpath);
					xmlSetProp(insertObject, BAD_CAST "insertionOrder", BAD_CAST "before");
				}

				xmlAddChild(insertObject, xmlCopyNode(objects->nodeTab[i], 1));
			}
		}
	}

	if (group->children) {
		return group;
	}

	xmlFreeNode(group);
	return NULL;
}

bool sameNodes(xmlNodePtr a, xmlNodePtr b)
{
	xmlBufferPtr bufA, bufB;
	bool equal;

	bufA = xmlBufferCreate();
	bufB = xmlBufferCreate();

	xmlNodeDump(bufA, a->doc, a, 0, 0);
	xmlNodeDump(bufB, b->doc, b, 0, 0);

	equal = xmlStrcmp(bufA->content, bufB->content) == 0;

	xmlBufferFree(bufA);
	xmlBufferFree(bufB);

	return equal;
}

xmlNodePtr replaceObjects(xmlXPathContextPtr src, xmlXPathContextPtr tgt)
{
	xmlNodePtr group;
	xmlXPathObjectPtr results;
	xmlNodeSetPtr objects;

	group = xmlNewNode(NULL, BAD_CAST "replaceObjectGroup");

	results = xmlXPathEvalExpression(BAD_CAST CIR_OBJECT_XPATH, src);
	objects = results->nodesetval;

	if (!xmlXPathNodeSetIsEmpty(objects)) {
		int i;

		for (i = 0; i < objects->nodeNr; ++i) {
			char xpath[256];
			xmlNodePtr node;

			getNodeXPath(xpath, objects->nodeTab[i]);

			if ((node = nodeExists(xpath, tgt)) && !sameNodes(objects->nodeTab[i], node)) {
				xmlNodePtr replaceObject;

				replaceObject = xmlNewChild(group, NULL, BAD_CAST "replaceObject", NULL);
				xmlAddChild(replaceObject, xmlCopyNode(node, 1));
			}
		}
	}

	if (group->children) {
		return group;
	}

	return NULL;
}

/* Copy metadata from source and target CIRs */
void setMetadata(xmlXPathContextPtr update, xmlXPathContextPtr source, xmlXPathContextPtr target)
{
	xmlNodePtr updateAddress, updateIdent, updateCode, sourceDmIdent,
		updateStatus, targetDmIssueInfo, updateIdentAndStatusSection,
		targetDmStatus;

	updateAddress = firstXPathNode("//updateAddress", update);

	updateIdent = xmlAddChild(updateAddress, xmlCopyNode(firstXPathNode("//dmIdent", source), 1));
	xmlNodeSetName(updateIdent, BAD_CAST "updateIdent");

	updateCode = firstXPathNode("//updateIdent/dmCode", update);
	xmlNodeSetName(updateCode, BAD_CAST "updateCode");
	xmlSetProp(updateCode, BAD_CAST "objectIdentCode", BAD_CAST "UPF");

	updateStatus = firstXPathNode("//updateStatus", update);

	xmlAddChild(updateAddress, xmlCopyNode(firstXPathNode("//dmAddressItems/issueDate", target), 1));

	sourceDmIdent = xmlAddChild(updateStatus, xmlCopyNode(firstXPathNode("//dmIdent", source), 1));
	xmlNodeSetName(sourceDmIdent, BAD_CAST "sourceDmIdent");

	targetDmIssueInfo = xmlAddChild(updateStatus, xmlCopyNode(firstXPathNode("//dmIdent/issueInfo", target), 1));
	xmlNodeSetName(targetDmIssueInfo, BAD_CAST "targetDmIssueInfo");

	xmlAddChild(updateStatus, xmlCopyNode(firstXPathNode("//dmStatus/responsiblePartnerCompany", target), 1));
	xmlAddChild(updateStatus, xmlCopyNode(firstXPathNode("//dmStatus/originator", target), 1));
	xmlAddChild(updateStatus, xmlCopyNode(firstXPathNode("//dmStatus/brexDmRef", target), 1));
	xmlAddChild(updateStatus, xmlCopyNode(firstXPathNode("//dmStatus/qualityAssurance", target), 1));

	updateIdentAndStatusSection = firstXPathNode("//updateIdentAndStatusSection", update);
	targetDmStatus = xmlAddChild(updateIdentAndStatusSection, xmlCopyNode(firstXPathNode("//dmStatus", target), 1));
	xmlNodeSetName(targetDmStatus, BAD_CAST "targetDmStatus");
}

void autoName(char *dst, xmlXPathContextPtr update)
{
	xmlNodePtr updateCode, issueInfo, language;
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
	char *issueNumber;
	char *inWork;
	char *languageIsoCode;
	char *countryIsoCode;
	int i;

	updateCode = firstXPathNode("//updateIdent/updateCode", update);
	issueInfo  = firstXPathNode("//updateIdent/issueInfo", update);
	language   = firstXPathNode("//updateIdent/language", update);

	modelIdentCode     = (char *) xmlGetProp(updateCode, BAD_CAST "modelIdentCode");
	systemDiffCode     = (char *) xmlGetProp(updateCode, BAD_CAST "systemDiffCode");
	systemCode         = (char *) xmlGetProp(updateCode, BAD_CAST "systemCode");
	subSystemCode      = (char *) xmlGetProp(updateCode, BAD_CAST "subSystemCode");
	subSubSystemCode   = (char *) xmlGetProp(updateCode, BAD_CAST "subSubSystemCode");
	assyCode           = (char *) xmlGetProp(updateCode, BAD_CAST "assyCode");
	disassyCode        = (char *) xmlGetProp(updateCode, BAD_CAST "disassyCode");
	disassyCodeVariant = (char *) xmlGetProp(updateCode, BAD_CAST "disassyCodeVariant");
	infoCode           = (char *) xmlGetProp(updateCode, BAD_CAST "infoCode");
	infoCodeVariant    = (char *) xmlGetProp(updateCode, BAD_CAST "infoCodeVariant");
	itemLocationCode   = (char *) xmlGetProp(updateCode, BAD_CAST "itemLocationCode");

	issueNumber = (char *) xmlGetProp(issueInfo, BAD_CAST "issueNumber");
	inWork      = (char *) xmlGetProp(issueInfo, BAD_CAST "inWork");

	languageIsoCode = (char *) xmlGetProp(language, BAD_CAST "languageIsoCode");
	countryIsoCode  = (char *) xmlGetProp(language, BAD_CAST "countryIsoCode");

	for (i = 0; languageIsoCode[i]; ++i) languageIsoCode[i] = toupper(languageIsoCode[i]);

	sprintf(dst, "UPF-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s_%s-%s_%s-%s.XML",
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
		itemLocationCode,
		issueNumber,
		inWork,
		languageIsoCode,
		countryIsoCode);
	
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

	xmlFree(issueNumber);
	xmlFree(inWork);

	xmlFree(languageIsoCode);
	xmlFree(countryIsoCode);
}

xmlDocPtr toIssue(xmlDocPtr doc, enum issue iss)
{
	xsltStylesheetPtr style;
	xmlDocPtr styledoc, res, orig;
	unsigned char *xml = NULL;
	unsigned int len;

	switch (iss) {
		case ISS_41:
			xml = ___common_42to41_xsl;
			len = ___common_42to41_xsl_len;
			break;
		default:
			return NULL;
	}

	orig = xmlCopyDoc(doc, 1);

	styledoc = xmlReadMemory((const char *) xml, len, NULL, NULL, 0);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, doc, NULL);

	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	xmlDocSetRootElement(orig, xmlCopyNode(xmlDocGetRootElement(res), 1));

	xmlFreeDoc(res);

	return orig;
}

xmlDocPtr xmlSkeleton(const char *templateDir)
{
	if (templateDir) {
		char src[PATH_MAX];
		sprintf(src, "%s/update.xml", templateDir);

		if (access(src, F_OK) == -1) {
			fprintf(stderr, ERR_PREFIX "No schema update in template directory \"%s\".\n", templateDir);
			exit(EXIT_BAD_TEMPLATE);
		}

		return xmlReadFile(src, NULL, PARSE_OPTS);
	} else {
		return xmlReadMemory((const char *) update_xml, update_xml_len, NULL, NULL, 0);
	}
}

void copyDefaultValue(const char *key, const char *val)
{
	if (strcmp(key, "issue") == 0 && issue == NO_ISS) {
		issue = getIssue(val);
	} else if (strcmp(key, "templates") == 0 && !templateDir) {
		templateDir = strdup(val);
	}
}

void showHelp(void)
{
	puts("Usage: " PROG_NAME " [options] <SOURCE> <TARGET>");
	puts("");
	puts("Options:");
	puts("  -h -?       Show help/usage message.");
	puts("  -@ <file>   Output to <file>.");
	puts("  -$ <issue>  Specify which S1000D issue to use.");
	puts("  -% <dir>    Use templates in specified directory.");
	puts("  -d <file>   Specify the .defaults file name.");
	puts("  -f          Overwrite existing file.");
	puts("  -q          Don't report an error if file exists.");
	puts("  --version   Show version information.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
}

int main(int argc, char **argv)
{
	const char *source, *target;
	xmlDocPtr sourceDoc, targetDoc, updateFile;
	xmlXPathContextPtr sourceContext, targetContext, updateFileContext;
	xmlNodePtr update;
	xmlNodePtr deleteObjectGroup, insertObjectGroup, replaceObjectGroup;
	int c;
	bool overwrite = false;
	bool no_overwrite_error = false;
	char *out = NULL;
	char defaultsFname[PATH_MAX] = DEFAULT_DEFAULTS_FNAME;
	xmlDocPtr defaultsXml;

	const char *sopts = "@:$:%:d:fqh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case '@':
				out = strdup(optarg);
				break;
			case '$':
				issue = getIssue(optarg);
				break;
			case '%':
				templateDir = strdup(optarg);
				break;
			case 'd':
				strncpy(defaultsFname, optarg, PATH_MAX - 1);
				break;
			case 'f':
				overwrite = true;
				break;
			case 'q':
				no_overwrite_error = true;
				break;
			case 'h':
			case '?':
				showHelp();
				return 0;
		}
	}

	if (argc - optind < 2) {
		exit(EXIT_MISSING_ARGS);
	}

	source = argv[optind];
	target = argv[optind + 1];

	sourceDoc = xmlReadFile(source, NULL, PARSE_OPTS);
	targetDoc = xmlReadFile(target, NULL, PARSE_OPTS);

	if ((defaultsXml = xmlReadFile(defaultsFname, NULL, PARSE_OPTS | XML_PARSE_NOERROR | XML_PARSE_NOWARNING))) {
		xmlNodePtr cur;

		for (cur = xmlDocGetRootElement(defaultsXml)->children; cur; cur = cur->next) {
			char *key, *val;

			if (cur->type != XML_ELEMENT_NODE) continue;
			if (!xmlHasProp(cur, BAD_CAST "ident")) continue;
			if (!xmlHasProp(cur, BAD_CAST "value")) continue;

			key = (char *) xmlGetProp(cur, BAD_CAST "ident");
			val = (char *) xmlGetProp(cur, BAD_CAST "value");

			copyDefaultValue(key, val);

			xmlFree(key);
			xmlFree(val);
		}

		xmlFreeDoc(defaultsXml);
	} else {
		FILE *defaults;

		if ((defaults = fopen(defaultsFname, "r"))) {
			char line[1024];
			while (fgets(line, 1024, defaults)) {
				char key[32];
				char val[256];

				if (sscanf(line, "%31s %255[^\n]", key, val) != 2)
					continue;

				copyDefaultValue(key, val);
			}

			fclose(defaults);
		}
	}

	if (issue == NO_ISS) issue = DEFAULT_S1000D_ISSUE;

	updateFile = xmlSkeleton(templateDir);

	sourceContext = xmlXPathNewContext(sourceDoc);
	targetContext = xmlXPathNewContext(targetDoc);
	updateFileContext = xmlXPathNewContext(updateFile);

	if (typeOfCir(sourceContext) != typeOfCir(targetContext)) {
		fprintf(stderr, ERR_PREFIX "Source CIR and target CIR are of different types.\n");
		exit(EXIT_INVALID_ARGS);
	}

	setMetadata(updateFileContext, sourceContext, targetContext);

	update = firstXPathNode("//content/update", updateFileContext);

	if ((deleteObjectGroup = deleteObjects(sourceContext, targetContext))) {
		xmlAddChild(update, deleteObjectGroup);
	}

	if ((insertObjectGroup = insertObjects(sourceContext, targetContext))) {
		xmlAddChild(update, insertObjectGroup);
	}

	if ((replaceObjectGroup = replaceObjects(sourceContext, targetContext))) {
		xmlAddChild(update, replaceObjectGroup);
	}

	if (issue < ISS_42) {
		xmlXPathFreeContext(updateFileContext);
		updateFile = toIssue(updateFile, issue);
		updateFileContext = xmlXPathNewContext(updateFile);
	}

	if (out) {
		xmlSaveFile(out, updateFile);
	} else {
		char upfname[PATH_MAX];

		autoName(upfname, updateFileContext);

		if (!overwrite && access(upfname, F_OK) != -1) {
			if (no_overwrite_error) return 0;
			fprintf(stderr, ERR_PREFIX "'%s' already exists.\n", upfname);
			exit(EXIT_UPF_EXISTS);
		} else {
			xmlSaveFile(upfname, updateFile);
		}
	}

	free(out);
	free(templateDir);

	xmlXPathFreeContext(sourceContext);
	xmlXPathFreeContext(targetContext);

	xmlFreeDoc(sourceDoc);
	xmlFreeDoc(targetDoc);
	xmlFreeDoc(updateFile);

	xmlCleanupParser();

	return 0;
}
