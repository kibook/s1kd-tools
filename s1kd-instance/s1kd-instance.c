/* Filters an S1000D data module on user-supplied applicability definitions,
 * producing a new data module instance with non-applicable elements and
 * (optionally) unused applicability statements removed.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "strings.h"

/* Prefix before errors printed to console */
#define ERR_PREFIX "s1kd-instance: ERROR: "

/* Error codes */
#define EXIT_MISSING_ARGS 1 /* Option or parameter missing */
#define EXIT_MISSING_FILE 2 /* File does not exist */
#define EXIT_BAD_APPLIC 4 /* Malformed applic definitions */
#define EXIT_NO_OVERWRITE 5 /* Did not overwrite existing out file */
#define EXIT_BAD_XML 6 /* Invalid XML/S1000D */

/* Convenient structure for all strings related to uniquely identifying a
 * data module.
 */
struct dmident {
	bool extended;
	char *extensionProducer;
	char *extensionCode;
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
};

xmlNodePtr applicability;

int napplics = 0;

void define_applic(char *ident, char *type, char *value)
{
	xmlNodePtr assert = NULL;
	xmlNodePtr cur;

	for (cur = applicability->children; cur; cur = cur->next) {
		char *cur_ident = (char *) xmlGetProp(cur, (xmlChar *) "applicPropertyIdent");
		char *cur_type  = (char *) xmlGetProp(cur, (xmlChar *) "applicPropertyType");
		char *cur_value = (char *) xmlGetProp(cur, (xmlChar *) "applicPropertyValues");

		if (strcmp(cur_ident, ident) == 0 && strcmp(cur_type, type) == 0) {
			assert = cur;
		}

		xmlFree(cur_ident);
		xmlFree(cur_type);
		xmlFree(cur_value);
	}

	if (!assert) {
		assert = xmlNewChild(applicability, NULL, (xmlChar *) "assert", NULL);
		xmlSetProp(assert, (xmlChar *) "applicPropertyIdent", (xmlChar *) ident);
		xmlSetProp(assert, (xmlChar *) "applicPropertyType",  (xmlChar *) type);
	}

	xmlSetProp(assert, (xmlChar *) "applicPropertyValues", (xmlChar *) value);
}

/* Find the first child element with a given name */
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

/* Same as above but throws a fatal error if the child is not found instead of returning NULL. */
xmlNodePtr find_req_child(xmlNodePtr parent, const char *name)
{
	xmlNodePtr child;

	if (!(child = find_child(parent, name))) {
		fprintf(stderr, ERR_PREFIX "Element %s missing child element %s.\n", (char *) parent->name, name);
		exit(EXIT_BAD_XML);
	}

	return child;
}

/* Copy strings related to uniquely identifying a data module. The strings are
 * dynamically allocated so they must be freed using free_ident. */
void init_ident(struct dmident *ident, xmlDocPtr dm)
{
	xmlNodePtr dmodule;
	xmlNodePtr identAndStatusSection;
	xmlNodePtr dmAddress;
	xmlNodePtr dmIdent;
	xmlNodePtr identExtension;
	xmlNodePtr dmCode;
	xmlNodePtr language;
	xmlNodePtr issueInfo;

	dmodule = xmlDocGetRootElement(dm);
	identAndStatusSection = find_req_child(dmodule, "identAndStatusSection");
	dmAddress = find_req_child(identAndStatusSection, "dmAddress");
	dmIdent = find_req_child(dmAddress, "dmIdent");
	identExtension = find_child(dmIdent, "identExtension");
	dmCode = find_req_child(dmIdent, "dmCode");
	language = find_req_child(dmIdent, "language");
	issueInfo = find_req_child(dmIdent, "issueInfo");

	ident->modelIdentCode     = (char *) xmlGetProp(dmCode, (xmlChar *) "modelIdentCode");
	ident->systemDiffCode     = (char *) xmlGetProp(dmCode, (xmlChar *) "systemDiffCode");
	ident->systemCode         = (char *) xmlGetProp(dmCode, (xmlChar *) "systemCode");
	ident->subSystemCode      = (char *) xmlGetProp(dmCode, (xmlChar *) "subSystemCode");
	ident->subSubSystemCode   = (char *) xmlGetProp(dmCode, (xmlChar *) "subSubSystemCode");
	ident->assyCode           = (char *) xmlGetProp(dmCode, (xmlChar *) "assyCode");
	ident->disassyCode        = (char *) xmlGetProp(dmCode, (xmlChar *) "disassyCode");
	ident->disassyCodeVariant = (char *) xmlGetProp(dmCode, (xmlChar *) "disassyCodeVariant");
	ident->infoCode           = (char *) xmlGetProp(dmCode, (xmlChar *) "infoCode");
	ident->infoCodeVariant    = (char *) xmlGetProp(dmCode, (xmlChar *) "infoCodeVariant");
	ident->itemLocationCode   = (char *) xmlGetProp(dmCode, (xmlChar *) "itemLocationCode");

	ident->issueNumber = (char *) xmlGetProp(issueInfo, (xmlChar *) "issueNumber");
	ident->inWork      = (char *) xmlGetProp(issueInfo, (xmlChar *) "inWork");

	ident->languageIsoCode = (char *) xmlGetProp(language, (xmlChar *) "languageIsoCode");
	ident->countryIsoCode  = (char *) xmlGetProp(language, (xmlChar *) "countryIsoCode");

	if (identExtension) {
		ident->extended = true;
		ident->extensionProducer = (char *) xmlGetProp(identExtension, (xmlChar *) "extensionProducer");
		ident->extensionCode     = (char *) xmlGetProp(identExtension, (xmlChar *) "extensionCode");
	} else {
		ident->extended = false;
	}
}

void free_ident(struct dmident *ident)
{
	if (ident->extended) {
		xmlFree(ident->extensionProducer);
		xmlFree(ident->extensionCode);
	}
	xmlFree(ident->modelIdentCode);
	xmlFree(ident->systemDiffCode);
	xmlFree(ident->systemCode);
	xmlFree(ident->subSystemCode);
	xmlFree(ident->subSubSystemCode);
	xmlFree(ident->assyCode);
	xmlFree(ident->disassyCode);
	xmlFree(ident->disassyCodeVariant);
	xmlFree(ident->infoCode);
	xmlFree(ident->infoCodeVariant);
	xmlFree(ident->itemLocationCode);
	xmlFree(ident->issueNumber);
	xmlFree(ident->inWork);
	xmlFree(ident->languageIsoCode);
	xmlFree(ident->countryIsoCode);
}

/* Evaluate an applic statement, returning whether it is valid or invalid given
 * the user-supplied applicability settings.
 *
 * If assume is true, undefined attributes and conditions are ignored. This is
 * primarily useful for determining which elements are not applicable in content
 * and may be removed.
 *
 * If assume is false, undefined attributes or conditions will cause an applic
 * statement to evaluate as invalid. This is primarily useful for determining
 * which applic statements and references are unambigously true (they do not
 * rely on any undefined attributes or conditions) and therefore may be removed.
 *
 * An undefined attribute/condition is a product attribute (ACT) or
 * condition (CCT) for which a value is asserted in the applic statement but
 * for which no value was supplied by the user.
 */
bool eval_applic(xmlNodePtr node, bool assume);

/* Tests whether a value is in an S1000D range (a~c is equivalent to a|b|c) */
bool is_in_range(const char *value, const char *range)
{
	char *ran;
	char *first;
	char *last;
	bool ret;

	if (!strchr(range, '~')) {
		return strcmp(value, range) == 0;
	}

	ran = malloc(strlen(range) + 1);

	strcpy(ran, range);

	first = strtok(ran, "~");
	last = strtok(NULL, "~");

	ret = strcmp(value, first) >= 0 && strcmp(value, last) <= 0;

	free(ran);

	return ret;
}

/* Tests whether a value is in an S1000D set (a|b|c) */
bool is_in_set(const char *value, const char *set)
{
	char *s;
	char *val = NULL;
	bool ret = false;

	if (!strchr(set, '|')) {
		return is_in_range(value, set);
	}

	s = malloc(strlen(set) + 1);

	strcpy(s, set);

	while ((val = strtok(val ? NULL : s, "|"))) {
		if (is_in_range(value, val)) {
			ret = true;
			break;
		}
	}

	free(s);

	return ret;
}

/* Tests whether ident:type=value was defined by the user */
bool is_applic(const char *ident, const char *type, const char *value, bool assume)
{
	xmlNodePtr cur;

	bool result = assume;

	for (cur = applicability->children; cur; cur = cur->next) {
		char *cur_ident = (char *) xmlGetProp(cur, (xmlChar *) "applicPropertyIdent");
		char *cur_type  = (char *) xmlGetProp(cur, (xmlChar *) "applicPropertyType");
		char *cur_value = (char *) xmlGetProp(cur, (xmlChar *) "applicPropertyValues");

		bool match = strcmp(cur_ident, ident) == 0 && strcmp(cur_type, type) == 0;

		if (match) {
			result = is_in_set(cur_value, value);
		}

		xmlFree(cur_ident);
		xmlFree(cur_type);
		xmlFree(cur_value);

		if (match) {
			break;
		}

	}

	return result;
}

/* Tests whether an <assert> element is applicable */
bool eval_assert(xmlNodePtr assert, bool assume)
{
	char *ident, *type, *values;

	bool ret;

	ident  = (char *) xmlGetProp(assert, (xmlChar *) "applicPropertyIdent");
	type   = (char *) xmlGetProp(assert, (xmlChar *) "applicPropertyType");
	values = (char *) xmlGetProp(assert, (xmlChar *) "applicPropertyValues");

	ret = is_applic(ident, type, values, assume);

	xmlFree(ident);
	xmlFree(type);
	xmlFree(values);

	return ret;
}

/* Test whether an <evaluate> element is applicable. */
bool eval_evaluate(xmlNodePtr evaluate, bool assume)
{
	char *op;

	bool ret;

	xmlNodePtr cur;

	op = (char *) xmlGetProp(evaluate, (xmlChar *) "andOr");

	if (!op) {
		fprintf(stderr, ERR_PREFIX "Element evaluate missing required attribute andOr.");
		exit(EXIT_BAD_XML);
	}

	ret = strcmp(op, "and") == 0;

	for (cur = evaluate->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, "assert") == 0 || strcmp((char *) cur->name, "evaluate") == 0) {
			if (strcmp(op, "and") == 0) {
				ret = ret && eval_applic(cur, assume);
			} else if (strcmp(op, "or") == 0) {
				ret = ret || eval_applic(cur, assume);
			}
		}
	}

	xmlFree(op);

	return ret;
}

/* Generic test for either <assert> or <evaluate> */
bool eval_applic(xmlNodePtr node, bool assume)
{
	if (strcmp((char *) node->name, "assert") == 0) {
		return eval_assert(node, assume);
	} else if (strcmp((char *) node->name, "evaluate") == 0) {
		return eval_evaluate(node, assume);
	}

	return false;
}

/* Tests whether an <applic> element is true. */
bool eval_applic_stmt(xmlNodePtr applic, bool assume)
{
	xmlNodePtr stmt;

	stmt = find_child(applic, "assert");

	if (!stmt) {
		stmt = find_child(applic, "evaluate");
	}

	return eval_applic(stmt, assume);
}

/* Search recursively for a descendant element with the given id */
xmlNodePtr get_element_by_id(xmlNodePtr root, const char *id)
{
	xmlNodePtr cur;
	char *cid;

	for (cur = root->children; cur; cur = cur->next) {
		xmlNodePtr ch;
		bool match;

		cid = (char *) xmlGetProp(cur, (xmlChar *) "id");

		match = cid && strcmp(cid, id) == 0;

		xmlFree(cid);

		if (match) {
			return cur;
		} else if ((ch = get_element_by_id(cur, id))) {
			return ch;
		}
	}

	return NULL;
}

/* Remove non-applicable elements from content */
void strip_applic(xmlNodePtr referencedApplicGroup, xmlNodePtr node)
{
	xmlNodePtr cur, next;

	if (xmlHasProp(node, (xmlChar *) "applicRefId")) {
		char *applicRefId;
		xmlNodePtr applic;

		applicRefId = (char *) xmlGetProp(node, (xmlChar *) "applicRefId");
		applic = get_element_by_id(referencedApplicGroup, applicRefId);
		xmlFree(applicRefId);

		if (applic && !eval_applic_stmt(applic, true)) {
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			return;
		}
	}

	cur = node->children;
	while (cur) {
		next = cur->next;
		strip_applic(referencedApplicGroup, cur);
		cur = next;
	}
}

/* Remove applic references on content where all assertions are unambigously true */
void clean_applic(xmlNodePtr referencedApplicGroup, xmlNodePtr node)
{
	xmlNodePtr cur;

	if (xmlHasProp(node, (xmlChar *) "applicRefId")) {
		char *applicRefId;
		xmlNodePtr applic;

		applicRefId = (char *) xmlGetProp(node, (xmlChar *) "applicRefId");
		applic = get_element_by_id(referencedApplicGroup, applicRefId);
		xmlFree(applicRefId);

		if (applic && eval_applic_stmt(applic, false)) {
			xmlUnsetProp(node, (xmlChar *) "applicRefId");
		}
	}

	for (cur = node->children; cur; cur = cur->next) {
		clean_applic(referencedApplicGroup, cur);
	}
}

/* Remove applic statements or parts of applic statements where all assertions
 * are unambigously true */
void simpl_applic(xmlNodePtr node)
{
	xmlNodePtr cur, next;

	if (strcmp((char *) node->name, "applic") == 0) {
		if (eval_applic_stmt(node, false) || !eval_applic_stmt(node, true)) {
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			return;
		}
	} else if (strcmp((char *) node->name, "evaluate") == 0) {
		if (eval_applic(node, false) || !eval_applic(node, true)) {
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			return;
		}
	} else if (strcmp((char *) node->name, "assert") == 0) {
		char *ident  = (char *) xmlGetProp(node, (xmlChar *) "applicPropertyIdent");
		char *type   = (char *) xmlGetProp(node, (xmlChar *) "applicPropertyType");
		char *values = (char *) xmlGetProp(node, (xmlChar *) "applicPropertyValues");

		if (is_applic(ident, type, values, false) || !is_applic(ident, type, values, true)) {
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			return;
		}
	}

	cur = node->children;
	while (cur) {
		next = cur->next;
		simpl_applic(cur);
		cur = next;
	}
}

/* If an <evaluate> contains only one (or no) child elements, remove it. */
void simpl_evaluate(xmlNodePtr evaluate)
{
	int nchild = 0;
	xmlNodePtr cur;

	for (cur = evaluate->children; cur; cur = cur->next) {
		if (cur->type == XML_ELEMENT_NODE) {
			++nchild;
		}
	}

	if (nchild < 2) {
		xmlNodePtr child;
		
		child = find_child(evaluate, "assert");
		if (!child) child = find_child(evaluate, "evaluate");
		xmlAddNextSibling(evaluate, child);
		xmlUnlinkNode(evaluate);
		xmlFreeNode(evaluate);
	}
}

/* Simplify <evaluate> elements recursively. */
void simpl_applic_evals(xmlNodePtr node)
{
	xmlNodePtr cur;

	for (cur = node->children; cur; cur = cur->next) {
		if (cur->type == XML_ELEMENT_NODE) {
			simpl_applic_evals(cur);
		}
	}

	if (strcmp((char *) node->name, "evaluate") == 0) {
		simpl_evaluate(node);
	}
}

/* Remove <referencedApplicGroup> if all applic statements are removed */
void simpl_applic_clean(xmlNode* referencedApplicGroup)
{
	bool has_applic = false;
	xmlNodePtr cur;

	if (!referencedApplicGroup) {
		return;
	}

	simpl_applic(referencedApplicGroup);
	simpl_applic_evals(referencedApplicGroup);

	for (cur = referencedApplicGroup->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, "applic") == 0) {
			has_applic = true;
		}
	}

	if (!has_applic) {
		xmlUnlinkNode(referencedApplicGroup);
		xmlFreeNode(referencedApplicGroup);
	}
}

/* Add metadata linking the data module instance with the master data module */
void add_source(xmlNodePtr dmodule)
{
	xmlNodePtr identAndStatusSection = find_req_child(dmodule, "identAndStatusSection");
	xmlNodePtr dmStatus = find_req_child(identAndStatusSection, "dmStatus");
	xmlNodePtr sourceDmIdent = find_child(dmStatus, "sourceDmIdent");
	xmlNodePtr security = find_req_child(dmStatus, "security");

	xmlNodePtr src_identExtension;
	xmlNodePtr src_dmCode;
	xmlNodePtr src_issueInfo;
	xmlNodePtr src_language;

	struct dmident ident;

	if (sourceDmIdent) {
		xmlUnlinkNode(sourceDmIdent);
		xmlFreeNode(sourceDmIdent);
	}

	sourceDmIdent = xmlNewNode(NULL, (xmlChar *) "sourceDmIdent");
	sourceDmIdent = xmlAddPrevSibling(security, sourceDmIdent);

	init_ident(&ident, dmodule->doc);

	if (ident.extended) {
		src_identExtension = xmlNewChild(sourceDmIdent, NULL, (xmlChar *) "identExtension", NULL);
		xmlSetProp(src_identExtension, (xmlChar *) "extensionProducer", (xmlChar *) ident.extensionProducer);
		xmlSetProp(src_identExtension, (xmlChar *) "extensionCode", (xmlChar *) ident.extensionCode);
	}

	src_dmCode = xmlNewChild(sourceDmIdent, NULL, (xmlChar *) "dmCode", NULL);
	src_language = xmlNewChild(sourceDmIdent, NULL, (xmlChar *) "language", NULL);
	src_issueInfo = xmlNewChild(sourceDmIdent, NULL, (xmlChar *) "issueInfo", NULL);

	xmlSetProp(src_dmCode, (xmlChar *) "modelIdentCode",     (xmlChar *) ident.modelIdentCode);
	xmlSetProp(src_dmCode, (xmlChar *) "systemDiffCode",     (xmlChar *) ident.systemDiffCode);
	xmlSetProp(src_dmCode, (xmlChar *) "systemCode",         (xmlChar *) ident.systemCode);
	xmlSetProp(src_dmCode, (xmlChar *) "subSystemCode",      (xmlChar *) ident.subSystemCode);
	xmlSetProp(src_dmCode, (xmlChar *) "subSubSystemCode",   (xmlChar *) ident.subSubSystemCode);
	xmlSetProp(src_dmCode, (xmlChar *) "assyCode",           (xmlChar *) ident.assyCode);
	xmlSetProp(src_dmCode, (xmlChar *) "disassyCode",        (xmlChar *) ident.disassyCode);
	xmlSetProp(src_dmCode, (xmlChar *) "disassyCodeVariant", (xmlChar *) ident.disassyCodeVariant);
	xmlSetProp(src_dmCode, (xmlChar *) "infoCode",           (xmlChar *) ident.infoCode);
	xmlSetProp(src_dmCode, (xmlChar *) "infoCodeVariant",    (xmlChar *) ident.infoCodeVariant);
	xmlSetProp(src_dmCode, (xmlChar *) "itemLocationCode",   (xmlChar *) ident.itemLocationCode);

	xmlSetProp(src_issueInfo, (xmlChar *) "issueNumber", (xmlChar *) ident.issueNumber);
	xmlSetProp(src_issueInfo, (xmlChar *) "inWork",      (xmlChar *) ident.inWork);

	xmlSetProp(src_language, (xmlChar *) "languageIsoCode", (xmlChar *) ident.languageIsoCode);
	xmlSetProp(src_language, (xmlChar *) "countryIsoCode",  (xmlChar *) ident.countryIsoCode);

	free_ident(&ident);
}

/* Add an extension to the data module code */
void set_dme(xmlNodePtr dmodule, char *extension)
{
	xmlNodePtr identAndStatusSection = find_req_child(dmodule, "identAndStatusSection");
	xmlNodePtr dmAddress = find_req_child(identAndStatusSection, "dmAddress");
	xmlNodePtr dmIdent = find_req_child(dmAddress, "dmIdent");
	xmlNodePtr dmCode = find_req_child(dmIdent, "dmCode");
	xmlNodePtr identExtension = find_child(dmIdent, "identExtension");

	char *extensionProducer;
	char *extensionCode;

	if (!identExtension) {
		identExtension = xmlNewNode(NULL, (xmlChar *) "identExtension");
		identExtension = xmlAddPrevSibling(dmCode, identExtension);
	}

	extensionProducer = strtok(extension, "-");
	extensionCode     = strtok(NULL, "-");

	xmlSetProp(identExtension, (xmlChar *) "extensionProducer", (xmlChar *) extensionProducer);
	xmlSetProp(identExtension, (xmlChar *) "extensionCode",     (xmlChar *) extensionCode);
}

/* Set the DMC of the produced data module of the instance */
void set_dmc(xmlNodePtr dmodule, char *dmc)
{
	xmlNodePtr identAndStatusSection = find_req_child(dmodule, "identAndStatusSection");
	xmlNodePtr dmAddress = find_req_child(identAndStatusSection, "dmAddress");
	xmlNodePtr dmIdent = find_req_child(dmAddress, "dmIdent");
	xmlNodePtr dmCode = find_req_child(dmIdent, "dmCode");

	char *modelIdentCode;
	char *systemDiffCode;      
	char *systemCode;      
	char *subAndSubSub;
	char *assyCode;
	char *disassyCodeAndVariant;
	char *infoCodeAndVariant;
	char *itemLocationCode;

	char subSystemCode[2] = "";
	char subSubSystemCode[2] = "";
	char disassyCode[3] = "";
	char disassyCodeVariant[2] = "";
	char infoCode[4] = "";
	char infoCodeVariant[2] = "";

	modelIdentCode        = strtok(dmc, "-");
	systemDiffCode        = strtok(NULL, "-");
	systemCode            = strtok(NULL, "-");
	subAndSubSub          = strtok(NULL, "-");
	assyCode              = strtok(NULL, "-");
	disassyCodeAndVariant = strtok(NULL, "-");
	infoCodeAndVariant    = strtok(NULL, "-");
	itemLocationCode      = strtok(NULL, "_");

	strncpy(subSystemCode, subAndSubSub, 1);
	strncpy(subSubSystemCode, subAndSubSub + 1, 1);
	strncpy(disassyCode, disassyCodeAndVariant, 2);
	strncpy(disassyCodeVariant, disassyCodeAndVariant + 2, 1);
	strncpy(infoCode, infoCodeAndVariant, 3);
	strncpy(infoCodeVariant, infoCodeAndVariant + 3, 1);

	xmlSetProp(dmCode, (xmlChar *) "modelIdentCode",     (xmlChar *) modelIdentCode);
	xmlSetProp(dmCode, (xmlChar *) "systemDiffCode",     (xmlChar *) systemDiffCode);
	xmlSetProp(dmCode, (xmlChar *) "systemCode",         (xmlChar *) systemCode);
	xmlSetProp(dmCode, (xmlChar *) "subSystemCode",      (xmlChar *) subSystemCode);
	xmlSetProp(dmCode, (xmlChar *) "subSubSystemCode",   (xmlChar *) subSubSystemCode);
	xmlSetProp(dmCode, (xmlChar *) "assyCode",           (xmlChar *) assyCode);
	xmlSetProp(dmCode, (xmlChar *) "disassyCode",        (xmlChar *) disassyCode);
	xmlSetProp(dmCode, (xmlChar *) "disassyCodeVariant", (xmlChar *) disassyCodeVariant);
	xmlSetProp(dmCode, (xmlChar *) "infoCode",           (xmlChar *) infoCode);
	xmlSetProp(dmCode, (xmlChar *) "infoCodeVariant",    (xmlChar *) infoCodeVariant);
	xmlSetProp(dmCode, (xmlChar *) "itemLocationCode",   (xmlChar *) itemLocationCode);
}

/* Set the techName and/or infoName of the data module instance */
void set_title(xmlNodePtr dmodule, char *tech, char *info)
{
	xmlNodePtr identAndStatusSection = find_req_child(dmodule, "identAndStatusSection");
	xmlNodePtr dmAddress = find_req_child(identAndStatusSection, "dmAddress");
	xmlNodePtr dmAddressItems = find_req_child(dmAddress, "dmAddressItems");
	xmlNodePtr dmTitle = find_req_child(dmAddressItems, "dmTitle");
	xmlNodePtr techName = find_req_child(dmTitle, "techName");
	xmlNodePtr infoName = find_child(dmTitle, "infoName");

	if (strcmp(tech, "") != 0) {
		xmlNodeSetContent(techName, (xmlChar *) tech);
	}

	if (strcmp(info, "") != 0) {
		if (!infoName) {
			infoName = xmlNewChild(dmTitle, NULL, (xmlChar *) "infoName", NULL);
		}
		xmlNodeSetContent(infoName, (xmlChar *) info);
	}	
}

/* Set the applicability for the whole datamodule instance */
void set_applic(xmlNodePtr dmodule, char *new_text)
{
	xmlNodePtr identAndStatusSection = find_child(dmodule, "identAndStatusSection");
	xmlNodePtr dmStatus = find_child(identAndStatusSection, "dmStatus");
	xmlNodePtr applic = find_child(dmStatus, "applic");

	xmlNodePtr new_applic;
	xmlNodePtr new_displayText;
	xmlNodePtr new_simplePara;
	xmlNodePtr new_evaluate;

	xmlNodePtr cur;

	new_applic = xmlNewNode(NULL, (xmlChar *) "applic");
	xmlAddNextSibling(applic, new_applic);

	new_displayText = xmlNewChild(new_applic, NULL, (xmlChar *) "displayText", NULL);
	new_simplePara = xmlNewChild(new_displayText, NULL, (xmlChar *) "simplePara", NULL);
	xmlNodeSetContent(new_simplePara, (xmlChar *) new_text);

	if (napplics > 1) {
		new_evaluate = xmlNewChild(new_applic, NULL, (xmlChar *) "evaluate", NULL);
		xmlSetProp(new_evaluate, (xmlChar *) "andOr", (xmlChar *) "and");
	} else {
		new_evaluate = new_applic;
	}

	for (cur = applicability->children; cur; cur = cur->next) {
		char *cur_ident = (char *) xmlGetProp(cur, (xmlChar *) "applicPropertyIdent");
		char *cur_type  = (char *) xmlGetProp(cur, (xmlChar *) "applicPropertyType");
		char *cur_value = (char *) xmlGetProp(cur, (xmlChar *) "applicPropertyValues");

		xmlNodePtr new_assert = xmlNewChild(new_evaluate, NULL, (xmlChar *) "assert", NULL);
		xmlSetProp(new_assert, (xmlChar *) "applicPropertyIdent",  (xmlChar *) cur_ident);
		xmlSetProp(new_assert, (xmlChar *) "applicPropertyType",   (xmlChar *) cur_type);
		xmlSetProp(new_assert, (xmlChar *) "applicPropertyValues", (xmlChar *) cur_value);

		xmlFree(cur_ident);
		xmlFree(cur_type);
		xmlFree(cur_value);
	}

	xmlUnlinkNode(applic);
	xmlFreeNode(applic);
}

/* Set the language/country for the data module instance */
void set_lang(xmlNodePtr dmodule, char *lang)
{
	xmlNodePtr identAndStatusSection = find_child(dmodule, "identAndStatusSection");
	xmlNodePtr dmAddress = find_child(identAndStatusSection, "dmAddress");
	xmlNodePtr dmIdent = find_child(dmAddress, "dmIdent");
	xmlNodePtr language = find_child(dmIdent, "language");

	char *language_iso_code;
	char *country_iso_code;

	int i;

	language_iso_code = strtok(lang, "-");
	country_iso_code = strtok(NULL, "");

	for (i = 0; language_iso_code[i]; ++i) {
		language_iso_code[i] = tolower(language_iso_code[i]);
	}
	for (i = 0; country_iso_code[i]; ++i) {
		country_iso_code[i] = toupper(country_iso_code[i]);
	}

	xmlSetProp(language, (xmlChar *) "languageIsoCode", (xmlChar *) language_iso_code);
	xmlSetProp(language, (xmlChar *) "countryIsoCode", (xmlChar *) country_iso_code);
}

void auto_name(char *out, xmlDocPtr dm, const char *dir)
{
	struct dmident ident;
	int i;

	init_ident(&ident, dm);

	for (i = 0; ident.languageIsoCode[i]; ++i) {
		ident.languageIsoCode[i] = toupper(ident.languageIsoCode[i]);
	}

	if (ident.extended) {
		sprintf(out, "%s/DME-%s-%s-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s_%s-%s_%s-%s.XML",
			dir,
			ident.extensionProducer,
			ident.extensionCode,
			ident.modelIdentCode,
			ident.systemDiffCode,
			ident.systemCode,
			ident.subSystemCode,
			ident.subSubSystemCode,
			ident.assyCode,
			ident.disassyCode,
			ident.disassyCodeVariant,
			ident.infoCode,
			ident.infoCodeVariant,
			ident.itemLocationCode,
			ident.issueNumber,
			ident.inWork,
			ident.languageIsoCode,
			ident.countryIsoCode);
	} else {
		sprintf(out, "%s/DMC-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s_%s-%s_%s-%s.XML",
			dir,
			ident.modelIdentCode,
			ident.systemDiffCode,
			ident.systemCode,
			ident.subSystemCode,
			ident.subSubSystemCode,
			ident.assyCode,
			ident.disassyCode,
			ident.disassyCodeVariant,
			ident.infoCode,
			ident.infoCodeVariant,
			ident.itemLocationCode,
			ident.issueNumber,
			ident.inWork,
			ident.languageIsoCode,
			ident.countryIsoCode);
	}

	free_ident(&ident);
}


/* These functions make a CIR-dependant data module standalone given specified
 * CIR data modules. See mainly Chapter 4.13.1 in the Issue 4.2 spec.*/

/* Functional item repository (00E) */
bool is_funcitemref_child(char *name)
{
	return strcmp(name, "name") == 0 || strcmp(name, "shortName") == 0 || strcmp(name, "refs") == 0;
}

void replace_funcitem_ref(xmlNodePtr ref, xmlNodePtr spec)
{
	xmlXPathContextPtr ctxt;
	xmlXPathObjectPtr results;
	xmlNodePtr cur;

	ctxt = xmlXPathNewContext(spec->doc);
	ctxt->node = spec;

	results = xmlXPathEvalExpression((xmlChar *) "functionalItemAlts/functionalItem", ctxt);

	if (!xmlXPathNodeSetIsEmpty(results->nodesetval) && results->nodesetval->nodeNr == 1) {
		spec = results->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(results);
	xmlXPathFreeContext(ctxt);

	while ((cur = ref->children)) {
		xmlUnlinkNode(cur);
		xmlFreeNode(cur);
	}

	for (cur = spec->children; cur; cur = cur->next) {
		if (is_funcitemref_child((char *) cur->name)) {
			xmlAddChild(ref, xmlCopyNode(cur, 1));
		}
	}
}

bool funcitem_match(xmlNodePtr ref, xmlNodePtr spc)
{
	xmlChar *refnum, *spcnum;
	xmlChar *reffit, *spcfit;
	xmlChar *refins, *spcins;
	xmlChar *refctx, *spcctx;
	xmlChar *refman, *spcman;
	xmlChar *reforg, *spcorg;

	bool match;

	refnum = xmlGetProp(ref, (xmlChar *) "functionalItemNumber");
	spcnum = xmlGetProp(spc, (xmlChar *) "functionalItemNumber");

	match = refnum == NULL;
	if (refnum && spcnum) match = xmlStrcmp(refnum, spcnum) == 0;

	xmlFree(refnum);
	xmlFree(spcnum);

	if (!match) return false;

	reffit = xmlGetProp(ref, (xmlChar *) "functionalItemType");
	spcfit = xmlGetProp(spc, (xmlChar *) "functionalItemType");

	match = reffit == NULL;
	if (reffit && spcfit) match = xmlStrcmp(reffit, spcfit) == 0;

	xmlFree(reffit);
	xmlFree(spcfit);

	if (!match) return false;

	refins = xmlGetProp(ref, (xmlChar *) "installationIdent");
	spcins = xmlGetProp(spc, (xmlChar *) "installationIdent");

	match = refins == NULL;
	if (refins && spcins) match = xmlStrcmp(refins, spcins) == 0;

	xmlFree(refins);
	xmlFree(spcins);

	if (!match) return false;

	refctx = xmlGetProp(ref, (xmlChar *) "contextIdent");
	spcctx = xmlGetProp(spc, (xmlChar *) "contextIdent");

	match = refctx == NULL;
	if (refctx && spcctx) match = xmlStrcmp(refctx, spcctx) == 0;

	xmlFree(refctx);
	xmlFree(spcctx);

	if (!match) return false;

	refman = xmlGetProp(ref, (xmlChar *) "manufacturerCodeValue");
	spcman = xmlGetProp(spc, (xmlChar *) "manufacturerCodeValue");

	match = refman == NULL;
	if (refman && spcman) match = xmlStrcmp(refman, spcman) == 0;

	xmlFree(refman);
	xmlFree(spcman);

	if (!match) return false;

	reforg = xmlGetProp(ref, (xmlChar *) "itemOriginator");
	spcorg = xmlGetProp(spc, (xmlChar *) "itemOriginator");

	match = reforg == NULL;
	if (reforg && spcorg) match = xmlStrcmp(reforg, spcorg) == 0;

	xmlFree(reforg);
	xmlFree(spcorg);

	if (!match) return false;

	return true;
}

void undepend_funcitem_cir(xmlDocPtr dm, xmlDocPtr cir)
{
	xmlXPathContextPtr ctxt1;
	xmlXPathContextPtr ctxt2;
	xmlXPathObjectPtr results1;
	xmlXPathObjectPtr results2;

	xmlNodePtr functionalItemRef;
	xmlNodePtr functionalItemSpec;
	xmlNodePtr functionalItemIdent;

	int i, j;

	ctxt1 = xmlXPathNewContext(cir);
	ctxt2 = xmlXPathNewContext(dm);

	results1 = xmlXPathEvalExpression((xmlChar *) "//functionalItemSpec", ctxt1);
	results2 = xmlXPathEvalExpression((xmlChar *) "//functionalItemRef", ctxt2);

	for (i = 0; i < results2->nodesetval->nodeNr; ++i) {
		functionalItemRef = results2->nodesetval->nodeTab[i];

		for (j = 0; j < results1->nodesetval->nodeNr; ++j) {
			functionalItemSpec = results1->nodesetval->nodeTab[j];

			functionalItemIdent = find_req_child(functionalItemSpec, "functionalItemIdent");

			if (funcitem_match(functionalItemRef, functionalItemIdent)) {
				replace_funcitem_ref(functionalItemRef, functionalItemSpec);
				break;
			}
		}
	}

	xmlXPathFreeObject(results1);
	xmlXPathFreeObject(results2);

	xmlXPathFreeContext(ctxt1);
	xmlXPathFreeContext(ctxt2);
}

/* Warnings and cautions repositories (0A4, 0A5) */
bool warncautref_match(xmlNodePtr ref, xmlNodePtr spec)
{
	char *refname, *specname, *refident, *specident;

	xmlNodePtr ident;

	bool match;

	refname   = (char *) ref->name;
	specname  = (char *) spec->name;

	if (strcmp(refname, "warningRef") == 0 && strcmp(specname, "warningSpec") != 0) return false;
	if (strcmp(refname, "cautionRef") == 0 && strcmp(specname, "cautionSpec") != 0) return false;

	if (strcmp(refname, "warningRef") == 0) {
		refident = (char *) xmlGetProp(ref, (xmlChar *) "warningIdentNumber");
	} else {
		refident = (char *) xmlGetProp(ref, (xmlChar *) "cautionIdentNumber");
	}

	if (strcmp(specname, "warningSpec") == 0) {
		ident = find_req_child(spec, "warningIdent");
		specident = (char *) xmlGetProp(ident, (xmlChar *) "warningIdentNumber");
	} else {
		ident = find_req_child(spec, "cautionIdent");
		specident = (char *) xmlGetProp(ident, (xmlChar *) "cautionIdentNumber");
	}

	match = strcmp(refident, specident) == 0;

	xmlFree(refident);
	xmlFree(specident);

	return match;
}

bool is_warncautref_child(char *name)
{
	return strcmp(name, "warningAndCautionPara") == 0;
}

void replace_warncaut_ref(xmlNodePtr ref, xmlNodePtr spec)
{
	xmlNodePtr cur, new;
	xmlChar *id;

	if (strcmp((char *) ref->name, "warningRef") == 0) {
		new = xmlNewNode(NULL, (xmlChar *) "warning");
	} else {
		new = xmlNewNode(NULL, (xmlChar *) "caution");
	}

	id = xmlGetProp(ref, (xmlChar *) "id");
	xmlSetProp(new, (xmlChar *) "id", id);
	xmlFree(id);

	for (cur = spec->children; cur; cur = cur->next) {
		if (is_warncautref_child((char *) cur->name)) {
			xmlAddChild(new, xmlCopyNode(cur, 1));
		}
	}

	new = xmlAddNextSibling(ref, new);

	xmlUnlinkNode(ref);
	xmlFreeNode(ref);
}

void undepend_warncaut_cir(xmlDocPtr dm, xmlDocPtr cir)
{
	xmlXPathContextPtr ctxt1, ctxt2;
	xmlXPathObjectPtr results1, results2;

	xmlNodePtr warningsAndCautionsRef;
	xmlNodePtr spec;
	xmlNodePtr cur, next;

	int i;

	ctxt1 = xmlXPathNewContext(cir);

	results1 = xmlXPathEvalExpression((xmlChar *) "//warningSpec|//cautionSpec", ctxt1);

	ctxt2 = xmlXPathNewContext(dm);
	results2 = xmlXPathEvalExpression((xmlChar *) "//warningsAndCautionsRef", ctxt2);
	warningsAndCautionsRef = results2->nodesetval->nodeTab[0];
	xmlXPathFreeObject(results2);
	xmlXPathFreeContext(ctxt2);

	cur = warningsAndCautionsRef->children;
	while (cur) {
		next = cur->next;

		if (cur->type == XML_ELEMENT_NODE) {
			for (i = 0; i < results1->nodesetval->nodeNr; ++i) {
				spec = results1->nodesetval->nodeTab[i];

				if (warncautref_match(cur, spec)) {
					replace_warncaut_ref(cur, spec);
				}
			}
		}

		cur = next;
	}

	xmlXPathFreeObject(results1);
	xmlXPathFreeContext(ctxt1);
}

/* Applicability repository (0A2) */

void replace_applic_ref(xmlNodePtr ref, xmlNodePtr applic)
{
	xmlChar *id;
	xmlNodePtr a;

	a = xmlAddNextSibling(ref, xmlCopyNode(applic, 1));
	id = xmlGetProp(ref, (xmlChar *) "id");
	xmlSetProp(a, (xmlChar *) "id", id);
	xmlFree(id);
}

void replace_applic_refs(xmlDocPtr dm, xmlNodePtr applicSpecIdent, xmlNodePtr applic)
{
	char xpath[256], *applicIdentValue;
	xmlXPathContextPtr ctxt;
	xmlXPathObjectPtr results;

	ctxt = xmlXPathNewContext(dm);

	applicIdentValue = (char *) xmlGetProp(applicSpecIdent, (xmlChar *) "applicIdentValue");
	snprintf(xpath, 256, "//applicRef[@applicIdentValue='%s']", applicIdentValue);
	xmlFree(applicIdentValue);

	results = xmlXPathEvalExpression((xmlChar *) xpath, ctxt);

	if (!xmlXPathNodeSetIsEmpty(results->nodesetval)) {
		int i;

		for (i = 0; i < results->nodesetval->nodeNr; ++i) {
			replace_applic_ref(results->nodesetval->nodeTab[i], applic);
		}
	}

	xmlXPathFreeObject(results);
	xmlXPathFreeContext(ctxt);
}

void replace_referenced_applic_group_ref(xmlDocPtr dm)
{
	xmlXPathContextPtr ctxt;
	xmlXPathObjectPtr result;
	xmlNodePtr referencedApplicGroupRef, referencedApplicGroup, cur;

	ctxt = xmlXPathNewContext(dm);
	result = xmlXPathEvalExpression((xmlChar *) "//referencedApplicGroupRef", ctxt);

	referencedApplicGroupRef = result->nodesetval->nodeTab[0];

	referencedApplicGroup = xmlNewNode(NULL, (xmlChar *) "referencedApplicGroup");
	referencedApplicGroup = xmlAddNextSibling(referencedApplicGroupRef, referencedApplicGroup);

	for (cur = referencedApplicGroupRef->children; cur; cur = cur->next)
		if (strcmp((char *) cur->name, "applic") == 0)
			xmlAddChild(referencedApplicGroup, xmlCopyNode(cur, 1));

	xmlXPathFreeObject(result);
	xmlXPathFreeContext(ctxt);

	xmlUnlinkNode(referencedApplicGroupRef);
	xmlFreeNode(referencedApplicGroupRef);
}

void undepend_applic_cir(xmlDocPtr dm, xmlDocPtr cir)
{
	xmlXPathContextPtr ctxt;
	xmlXPathObjectPtr results1;

	ctxt = xmlXPathNewContext(cir);

	results1 = xmlXPathEvalExpression((xmlChar *) "//applicSpec", ctxt);

	if (!xmlXPathNodeSetIsEmpty(results1->nodesetval)) {
		int i;

		for (i = 0; i < results1->nodesetval->nodeNr; ++i) {
			char *applicMapRefId, xpath[256];
			xmlNodePtr applicSpec, applicSpecIdent, applic;
			xmlXPathObjectPtr results2;
			
			applicSpec = results1->nodesetval->nodeTab[i];

			ctxt->node = applicSpec;
			results2 = xmlXPathEvalExpression((xmlChar *) "applicSpecIdent", ctxt);
			applicSpecIdent = results2->nodesetval->nodeTab[0];
			xmlXPathFreeObject(results2);

			applicMapRefId = (char *) xmlGetProp(applicSpec, (xmlChar *) "applicMapRefId");
			snprintf(xpath, 256, "//referencedApplicGroup/applic[@id='%s']", applicMapRefId);
			xmlFree(applicMapRefId);
			results2 = xmlXPathEvalExpression((xmlChar *) xpath, ctxt);

			if (!xmlXPathNodeSetIsEmpty(results2->nodesetval)) {
				applic = results2->nodesetval->nodeTab[0];
				replace_applic_refs(dm, applicSpecIdent, applic);
			}

			xmlXPathFreeObject(results2);
		}

		replace_referenced_applic_group_ref(dm);
	}

	xmlXPathFreeObject(results1);
	xmlXPathFreeContext(ctxt);
}

/* Apply the user-defined applicability to the CIR data module, then call the
 * appropriate function for the specific type of CIR. */

void undepend_cir(xmlDocPtr dm, xmlDocPtr cir, bool add_src)
{
	xmlXPathContextPtr ctxt;
	xmlXPathObjectPtr results;

	xmlNodePtr cirnode;
	xmlNodePtr content;
	xmlNodePtr referencedApplicGroup;

	char *cirtype;

	ctxt = xmlXPathNewContext(cir);

	results = xmlXPathEvalExpression((xmlChar *) "//content", ctxt);
	content = results->nodesetval->nodeTab[0];
	xmlXPathFreeObject(results);

	results = xmlXPathEvalExpression((xmlChar *) "//referencedApplicGroup", ctxt);

	if (!xmlXPathNodeSetIsEmpty(results->nodesetval)) {
		referencedApplicGroup = results->nodesetval->nodeTab[0];
		strip_applic(referencedApplicGroup, content);
	}

	xmlXPathFreeObject(results);

	results = xmlXPathEvalExpression((xmlChar *) "//content/commonRepository/*", ctxt);
	cirnode = results->nodesetval->nodeTab[0];
	xmlXPathFreeObject(results);

	cirtype = (char *) cirnode->name;

	if (strcmp(cirtype, "functionalItemRepository") == 0) {
		undepend_funcitem_cir(dm, cir);
	} else if (strcmp(cirtype, "warningRepository") == 0 || strcmp(cirtype, "cautionRepository") == 0) {
		undepend_warncaut_cir(dm, cir);
	} else if (strcmp(cirtype, "applicRepository") == 0) {
		undepend_applic_cir(dm, cir);
	} else {
		fprintf(stderr, ERR_PREFIX "Unsupported CIR type: %s\n", cirtype);
		add_src = false;
	}

	xmlXPathFreeContext(ctxt);

	if (add_src) {
		xmlNodePtr security, dmIdent, repositorySourceDmIdent, cur;

		ctxt = xmlXPathNewContext(dm);
		results = xmlXPathEvalExpression((xmlChar *) "//dmStatus/security", ctxt);
		security = results->nodesetval->nodeTab[0];
		xmlXPathFreeObject(results);
		xmlXPathFreeContext(ctxt);

		repositorySourceDmIdent = xmlNewNode(NULL, (xmlChar *) "repositorySourceDmIdent");
		repositorySourceDmIdent = xmlAddPrevSibling(security, repositorySourceDmIdent);

		ctxt = xmlXPathNewContext(cir);
		results = xmlXPathEvalExpression((xmlChar *) "//dmIdent", ctxt);
		dmIdent = results->nodesetval->nodeTab[0];
		xmlXPathFreeObject(results);
		xmlXPathFreeContext(ctxt);

		for (cur = dmIdent->children; cur; cur = cur->next) {
			xmlAddChild(repositorySourceDmIdent, xmlCopyNode(cur, 1));
		}
	}
}

xmlNodePtr first_xpath_node(xmlDocPtr doc, char *path)
{
	xmlXPathContextPtr ctxt = xmlXPathNewContext(doc);
	xmlXPathObjectPtr result = xmlXPathEvalExpression((xmlChar *) path, ctxt);
	xmlNodePtr ret;
	
	if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
		ret = NULL;
	} else {
		ret = result->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(result);
	xmlXPathFreeContext(ctxt);

	return ret;
}

void set_issue(xmlDocPtr dm, char *issinfo)
{
	char issue[4], inwork[3];
	xmlNodePtr issueInfo;

	if (sscanf(issinfo, "%3s-%2s", issue, inwork) != 2) {
		fprintf(stderr, ERR_PREFIX "Invalid format for issue/in-work number.\n");
		exit(EXIT_MISSING_ARGS);
	}

	issueInfo = first_xpath_node(dm, "//dmIdent/issueInfo");

	xmlSetProp(issueInfo, BAD_CAST "issueNumber", BAD_CAST issue);
	xmlSetProp(issueInfo, BAD_CAST "inWork", BAD_CAST inwork);
}

void set_security(xmlDocPtr dm, char *sec)
{
	xmlNodePtr security = first_xpath_node(dm, "//dmStatus/security");
	xmlSetProp(security, BAD_CAST "securityClassification", BAD_CAST sec);
}

/* Print a usage message */
void show_help(void)
{
	printf("%.*s", help_msg_len, help_msg);
}

int main(int argc, char **argv)
{
	xmlDocPtr dm;
	xmlNodePtr dmodule;
	xmlNodePtr content;
	xmlNodePtr identAndStatusSection;
	xmlNodePtr referencedApplicGroup;
	xmlNodePtr comment;

	int i;
	int c;

	char src[256] = "";
	char dmc[256] = "";
	char out[256] = "-";
	bool clean = false;
	bool simpl = false;
	char tech[256] = "";
	char info[256] = "";
	bool autoname = false;
	char dir[256] = "";
	char new_display_text[256] = "";
	char comment_text[256] = "";
	char extension[256] = "";
	char language[256] = "";
	bool add_source_ident = true;
	bool force_overwrite = false;
	bool use_stdin = false;
	char issinfo[8] = "";
	char secu[4] = "";

	int parseopts = 0;

	xmlNodePtr cirs, cir;
	xmlDocPtr cirdoc;

	opterr = 1;

	cirs = xmlNewNode(NULL, (xmlChar *) "cirs");

	while ((c = getopt(argc, argv, "s:Se:c:o:O:faAt:i:Y:C:l:R:I:u:h?")) != -1) {
		switch (c) {
			case 's': strncpy(src, optarg, 255); break;
			case 'S': add_source_ident = false; break;
			case 'e': strncpy(extension, optarg, 255); break;
			case 'c': strncpy(dmc, optarg, 255); break;
			case 'o': strncpy(out, optarg, 255); break;
			case 'O': autoname = true; strncpy(dir, optarg, 255); break;
			case 'f': force_overwrite = true; break;
			case 'a': clean = true; break;
			case 'A': simpl = true; break;
			case 't': strncpy(tech, optarg, 255); break;
			case 'i': strncpy(info, optarg, 255); break;
			case 'Y': strncpy(new_display_text, optarg, 255); break;
			case 'C': strncpy(comment_text, optarg, 255); break;
			case 'l': strncpy(language, optarg, 255); break;
			case 'R': xmlNewChild(cirs, NULL, (xmlChar *) "cir", (xmlChar *) optarg); break;
			case 'I': strncpy(issinfo, optarg, 6); break;
			case 'u': strncpy(secu, optarg, 2); break;
			case 'h':
			case '?':
				show_help();
				exit(EXIT_SUCCESS);
		}
	}

	if (strcmp(src, "") == 0) {
		strcpy(src, "-");
		use_stdin = true;
	}

	/* Bug in libxml < 20902 where entities in DTD are substituted even
	 * when XML_PARSE_NOENT is specified (default). Denying network access
	 * prevents it from substituting the %ISOEntities; parameter in the DTD
	 */
	if (LIBXML_VERSION < 20902) {
		parseopts |= XML_PARSE_NONET;
	}

	if (!use_stdin && access(src, F_OK) == -1) {
		fprintf(stderr, ERR_PREFIX "Could not find source data module \"%s\".\n", src);
		exit(EXIT_MISSING_FILE);
	}

	dm = xmlReadFile(src, NULL, parseopts);

	if (!dm) {
		fprintf(stderr, ERR_PREFIX "%s does not contain valid XML.\n",
			use_stdin ? "stdin" : src);
		exit(EXIT_BAD_XML);
	}

	if (autoname) {
		if (strcmp(dir, "") == 0) {
			fprintf(stderr, ERR_PREFIX "No directory specified with -O.\n");
			exit(EXIT_MISSING_ARGS);
		}
	}

	dmodule = xmlDocGetRootElement(dm);

	if (!dmodule) {
		fprintf(stderr, ERR_PREFIX "Source XML is not a data module.\n");
		exit(EXIT_BAD_XML);
	}

	content = find_req_child(dmodule, "content");

	applicability = xmlNewNode(NULL, (xmlChar *) "applic");

	/* All remaining arguments are treated as applic defs and copied to the
	 * global applicability list. */
	for (i = optind; i < argc; ++i) {
		char *ident, *type, *value;

		if (!strchr(argv[i], ':') || !strchr(argv[i], '=')) {
			fprintf(stderr, ERR_PREFIX "Malformed applicability definition: %s.\n", argv[i]);
			exit(EXIT_BAD_APPLIC);
		}

		ident = strtok(argv[i], ":");
		type  = strtok(NULL, "=");
		value = strtok(NULL, "");

		define_applic(ident, type, value);

		++napplics;
	}

	if (add_source_ident) {
		add_source(dmodule);
	}

	for (cir = cirs->children; cir; cir = cir->next) {
		char *cirdocfname = (char *) xmlNodeGetContent(cir);

		if (access(cirdocfname, F_OK) == -1) {
			fprintf(stderr, ERR_PREFIX "Could not find CIR %s.", cirdocfname);
			continue;
		}

		cirdoc = xmlReadFile(cirdocfname, NULL, 0);

		if (!cirdoc) {
			fprintf(stderr, ERR_PREFIX "CIR %s is invalid.", cirdocfname);
			continue;
		}

		undepend_cir(dm, cirdoc, add_source_ident);

		xmlFreeDoc(cirdoc);
		xmlFree(cirdocfname);
	}

	referencedApplicGroup = find_child(content, "referencedApplicGroup");

	if (referencedApplicGroup) {
		strip_applic(referencedApplicGroup, content);

		if (clean || simpl) {
			clean_applic(referencedApplicGroup, content);
		}

		if (simpl) {
			simpl_applic_clean(referencedApplicGroup);
		}
	}

	xmlFreeNode(cirs);

	if (strcmp(extension, "") != 0) {
		set_dme(dmodule, extension);
	}

	if (strcmp(dmc, "") != 0) {
		set_dmc(dmodule, dmc);
	}

	set_title(dmodule, tech, info);

	if (strcmp(language, "") != 0) {
		set_lang(dmodule, language);
	}

	if (strcmp(new_display_text, "") != 0) {
		set_applic(dmodule, new_display_text);
	}

	if (strcmp(issinfo, "") != 0) {
		set_issue(dm, issinfo);
	}

	if (strcmp(secu, "") != 0) {
		set_security(dm, secu);
	}

	if (strcmp(comment_text, "") != 0) {
		comment = xmlNewComment((xmlChar *) comment_text);
		identAndStatusSection = find_child(dmodule, "identAndStatusSection");

		if (!identAndStatusSection) {
			fprintf(stderr, ERR_PREFIX "Data module missing child identAndStatusSection.");
			exit(EXIT_BAD_XML);
		}

		xmlAddPrevSibling(identAndStatusSection, comment);
	}

	if (autoname) {
		auto_name(out, dm, dir);

		if (access(out, F_OK) == 0 && !force_overwrite) {
			fprintf(stderr, ERR_PREFIX "%s already exists. Use -f to overwrite.\n", out);
			exit(EXIT_NO_OVERWRITE);
		}
	}

	xmlSaveFile(out, dm);

	xmlFreeNode(applicability);
	xmlFreeDoc(dm);
	xmlCleanupParser();

	return 0;
}
