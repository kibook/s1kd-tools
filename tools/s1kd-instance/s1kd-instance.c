#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libexslt/exslt.h>
#include "s1kd_tools.h"
#include "strings.h"
#include "xsl.h"

#define PROG_NAME "s1kd-instance"
#define VERSION "1.11.0"

/* Prefixes before errors/warnings printed to console */
#define ERR_PREFIX PROG_NAME ": ERROR: "
#define WRN_PREFIX PROG_NAME ": WARNING: "

/* Error codes */
#define EXIT_MISSING_ARGS 1 /* Option or parameter missing */
#define EXIT_MISSING_FILE 2 /* File does not exist */
#define EXIT_BAD_APPLIC 4 /* Malformed applic definitions */
#define EXIT_BAD_XML 6 /* Invalid XML/S1000D */
#define EXIT_BAD_ARG 7 /* Malformed argument */
#define EXIT_BAD_DATE 8 /* Malformed issue date */

/* Error messages */
#define S_MISSING_OBJECT ERR_PREFIX "Could not read source object: %s\n"
#define S_MISSING_LIST ERR_PREFIX "Could not read list file: %s\n"
#define S_BAD_TYPE ERR_PREFIX "Cannot automatically name unsupported object types.\n"
#define S_BAD_XML ERR_PREFIX "%s does not contain valid XML. If it is a list, specify the -L option.\n"
#define S_MISSING_ANDOR ERR_PREFIX "Element evaluate missing required attribute andOr.\n"
#define S_BAD_CODE ERR_PREFIX "Bad %s code: %s.\n"
#define S_NO_XSLT ERR_PREFIX "No built-in XSLT for CIR type: %s\n"
#define S_INVALID_CIR ERR_PREFIX "%s is not a valid CIR data module.\n"
#define S_INVALID_ISSFMT ERR_PREFIX "Invalid format for issue/in-work number.\n"
#define S_BAD_DATE ERR_PREFIX "Bad issue date: %s\n"
#define S_NO_PRODUCT ERR_PREFIX "No product matching '%s' in PCT '%s'.\n"
#define S_BAD_ASSIGN ERR_PREFIX "Malformed applicability definition: %s.\n"
#define S_MISSING_REF_DM ERR_PREFIX "Could not read referenced ACT/PCT: %s\n"
#define S_MISSING_PCT ERR_PREFIX "PCT '%s' not found.\n"
#define S_MISSING_CIR ERR_PREFIX "Could not find CIR %s."
#define S_MKDIR_FAILED ERR_PREFIX "Could not create directory %s\n"

/* Warning messages */
#define S_FILE_EXISTS WRN_PREFIX "%s already exists. Use -f to overwrite.\n"

/* When using the -g option, these are set as the values for the
 * originator.
 */
#define DEFAULT_ORIG_CODE "S1KDI"
#define DEFAULT_ORIG_NAME "s1kd-instance tool"

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

/* Convenient structure for all strings related to uniquely identifying a
 * CSDB object.
 */
enum object_type { DM, PM, DML, COM, DDN, IMF, UPF };
enum issue { ISS_30, ISS_4X };
struct ident {
	bool extended;
	enum object_type type;
	enum issue issue;
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
	char *learnCode;
	char *learnEventCode;
	char *senderIdent;
	char *pmNumber;
	char *pmVolume;
	char *issueNumber;
	char *inWork;
	char *languageIsoCode;
	char *countryIsoCode;
	char *dmlCommentType;
	char *seqNumber;
	char *yearOfDataIssue;
	char *receiverIdent;
	char *imfIdentIcn;
};

/* User-defined applicability */
xmlNodePtr applicability;
int napplics = 0;

/* Define a value for a product attribute or condition. */
void define_applic(char *ident, char *type, char *value, bool pct)
{
	xmlNodePtr assert = NULL;
	xmlNodePtr cur;

	/* Check if an assert has already been created for this property. */
	for (cur = applicability->children; cur; cur = cur->next) {
		char *cur_ident = (char *) xmlGetProp(cur, BAD_CAST "applicPropertyIdent");
		char *cur_type  = (char *) xmlGetProp(cur, BAD_CAST "applicPropertyType");

		if (strcmp(cur_ident, ident) == 0 && strcmp(cur_type, type) == 0) {
			assert = cur;
		}

		xmlFree(cur_ident);
		xmlFree(cur_type);
	}

	/* If no assert exists, add a new one. */
	if (!assert) {
		assert = xmlNewChild(applicability, NULL, BAD_CAST "assert", NULL);
		xmlSetProp(assert, BAD_CAST "applicPropertyIdent", BAD_CAST ident);
		xmlSetProp(assert, BAD_CAST "applicPropertyType",  BAD_CAST type);
		xmlSetProp(assert, BAD_CAST "applicPropertyValues", BAD_CAST value);
		++napplics;
	/* Or, if an assert already exists... */
	} else {
		/* Check for duplicate value in a single-assert. */
		if (xmlHasProp(assert, BAD_CAST "applicPropertyValues")) {
			xmlChar *first_value;

			first_value = xmlGetProp(assert, BAD_CAST "applicPropertyValues");

			/* If not a duplicate, convert to a multi-assert and
			 * add the original and new values. */
			if (xmlStrcmp(first_value, BAD_CAST value) != 0) {
				xmlNewChild(assert, NULL, BAD_CAST "value", first_value);
				xmlNewChild(assert, NULL, BAD_CAST "value", BAD_CAST value);
				xmlUnsetProp(assert, BAD_CAST "applicPropertyValues");
			}

			xmlFree(first_value);
		/* Check for duplicate value in a multi-assert. */
		} else {
			bool dup = false;

			for (cur = assert->children; cur && !dup; cur = cur->next) {
				xmlChar *cur_value;
				cur_value = xmlNodeGetContent(cur);
				dup = xmlStrcmp(cur_value, BAD_CAST value) == 0;
				xmlFree(cur_value);
			}

			/* If not a duplicate, add the new value to the
			 * multi-assert. */
			if (!dup) {
				xmlNewChild(assert, NULL, BAD_CAST "value", BAD_CAST value);
			}
		}
	}

	if (pct) {
		xmlSetProp(assert, BAD_CAST "fromPct", BAD_CAST "true");
	}
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

xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const char *path)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	if (doc) {
		ctx = xmlXPathNewContext(doc);
	} else {
		ctx = xmlXPathNewContext(node->doc);
	}

	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST path, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		first = NULL;
	} else {
		first = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

xmlChar *first_xpath_value(xmlDocPtr doc, xmlNodePtr node, const char *path)
{
	return xmlNodeGetContent(first_xpath_node(doc, node, path));
}

void lowercase(char *s)
{
	int i;
	for (i = 0; s[i]; ++i) {
		s[i] = tolower(s[i]);
	}
}

void uppercase(char *s)
{
	int i;
	for (i = 0; s[i]; ++i) {
		s[i] = toupper(s[i]);
	}
}

/* Copy strings related to uniquely identifying a CSDB object. The strings are
 * dynamically allocated so they must be freed using free_ident. */
#define IDENT_XPATH \
	"//dmIdent|//dmaddres|" \
	"//pmIdent|//pmaddres|" \
	"//dmlIdent|//dml[dmlc]|" \
	"//commentIdent|//cstatus|" \
	"//ddnIdent|//ddn|" \
	"//imfIdent|" \
	"//updateIdent"
#define EXTENSION_XPATH \
	"//dmIdent/identExtension|//dmaddres/dmcextension|" \
	"//pmIdent/identExtension|" \
	"//updateIdent/identExtension"
#define CODE_XPATH \
	"//dmIdent/dmCode|//dmaddres/dmc/avee|" \
	"//pmIdent/pmCode|//pmaddres/pmc|" \
	"//dmlIdent/dmlCode|//dml/dmlc|" \
	"//commentIdent/commentCode|//cstatus/ccode|" \
	"//ddnIdent/ddnCode|//ddn/ddnc|" \
	"//imfIdent/imfCode|" \
	"//updateIdent/updateCode"
#define LANGUAGE_XPATH \
	"//dmIdent/language|//dmaddres/language|" \
	"//pmIdent/language|//pmaddres/language|" \
	"//commentIdent/language|//cstatus/language|" \
	"//updateIdent/language"
#define ISSUE_INFO_XPATH \
	"//dmIdent/issueInfo|//dmaddres/issno|" \
	"//pmIdent/issueInfo|//pmaddres/issno|" \
	"//dmlIdent/issueInfo|//dml/issno|" \
	"//imfIdent/issueInfo|" \
	"//updateIdent/issueInfo"
bool init_ident(struct ident *ident, xmlDocPtr doc)
{
	xmlNodePtr moduleIdent, identExtension, code, language, issueInfo;

	moduleIdent = first_xpath_node(doc, NULL, IDENT_XPATH);

	if (!moduleIdent) {
		return false;
	}

	if (xmlStrcmp(moduleIdent->name, BAD_CAST "pmIdent") == 0) {
		ident->type = PM;
		ident->issue = ISS_4X;
	} else if (xmlStrcmp(moduleIdent->name, BAD_CAST "pmaddres") == 0) {
		ident->type = PM;
		ident->issue = ISS_30;
	} else if (xmlStrcmp(moduleIdent->name, BAD_CAST "dmlIdent") == 0) {
		ident->type = DML;
		ident->issue = ISS_4X;
	} else if (xmlStrcmp(moduleIdent->name, BAD_CAST "dml") == 0) {
		ident->type = DML;
		ident->issue = ISS_30;
	} else if (xmlStrcmp(moduleIdent->name, BAD_CAST "commentIdent") == 0) {
		ident->type = COM;
		ident->issue = ISS_4X;
	} else if (xmlStrcmp(moduleIdent->name, BAD_CAST "cstatus") == 0) {
		ident->type = COM;
		ident->issue = ISS_30;
	} else if (xmlStrcmp(moduleIdent->name, BAD_CAST "dmIdent") == 0) {
		ident->type = DM;
		ident->issue = ISS_4X;
	} else if (xmlStrcmp(moduleIdent->name, BAD_CAST "dmaddres") == 0) {
		ident->type = DM;
		ident->issue = ISS_30;
	} else if (xmlStrcmp(moduleIdent->name, BAD_CAST "ddnIdent") == 0) {
		ident->type = DDN;
		ident->issue = ISS_4X;
	} else if (xmlStrcmp(moduleIdent->name, BAD_CAST "ddn") == 0) {
		ident->type = DDN;
		ident->issue = ISS_30;
	} else if (xmlStrcmp(moduleIdent->name, BAD_CAST "imfIdent") == 0) {
		ident->type = IMF;
		ident->issue = ISS_4X;
	} else if (xmlStrcmp(moduleIdent->name, BAD_CAST "updateIdent") == 0) {
		ident->type = UPF;
		ident->issue = ISS_4X;
	}

	identExtension = first_xpath_node(doc, NULL, EXTENSION_XPATH);
	code = first_xpath_node(doc, NULL, CODE_XPATH);
	language = first_xpath_node(doc, NULL, LANGUAGE_XPATH);
	issueInfo = first_xpath_node(doc, NULL, ISSUE_INFO_XPATH);

	if (!code) {
		return false;
	}

	if (ident->issue == ISS_30) {
		ident->modelIdentCode = (char *) xmlNodeGetContent(find_child(code, "modelic"));
	} else {
		ident->modelIdentCode = (char *) xmlGetProp(code, BAD_CAST "modelIdentCode");
	}

	if (ident->type == PM) {
		if (ident->issue == ISS_30) {
			ident->senderIdent = (char *) xmlNodeGetContent(find_child(code, "pmissuer"));
			ident->pmNumber    = (char *) xmlNodeGetContent(find_child(code, "pmnumber"));
			ident->pmVolume    = (char *) xmlNodeGetContent(find_child(code, "pmvolume"));
		} else {
			ident->senderIdent = (char *) xmlGetProp(code, BAD_CAST "pmIssuer");
			ident->pmNumber    = (char *) xmlGetProp(code, BAD_CAST "pmNumber");
			ident->pmVolume    = (char *) xmlGetProp(code, BAD_CAST "pmVolume");
		}
	} else if (ident->type == DML || ident->type == COM) {
		if (ident->issue == ISS_30) {
			ident->senderIdent = (char *) xmlNodeGetContent(find_child(code, "sendid"));
			ident->yearOfDataIssue = (char *) xmlNodeGetContent(find_child(code, "diyear"));
			ident->seqNumber = (char *) xmlNodeGetContent(find_child(code, "seqnum"));
		} else {
			ident->senderIdent        = (char *) xmlGetProp(code, BAD_CAST "senderIdent");
			ident->yearOfDataIssue    = (char *) xmlGetProp(code, BAD_CAST "yearOfDataIssue");
			ident->seqNumber          = (char *) xmlGetProp(code, BAD_CAST "seqNumber");
		}

		if (ident->type == DML) {
			if (ident->issue == ISS_30) {
				ident->dmlCommentType = (char *) xmlGetProp(find_child(code, "dmltype"), BAD_CAST "type");
			} else {
				ident->dmlCommentType = (char *) xmlGetProp(code, BAD_CAST "dmlType");
			}
		} else {
			if (ident->issue == ISS_30) {
				ident->dmlCommentType = (char *) xmlGetProp(find_child(code, "ctype"), BAD_CAST "type");
			} else {
				ident->dmlCommentType = (char *) xmlGetProp(code, BAD_CAST "commentType");
			}
		}
	} else if (ident->type == DDN) {
		if (ident->issue == ISS_30) {
			ident->senderIdent     = (char *) xmlNodeGetContent(find_child(code, "sendid"));
			ident->receiverIdent   = (char *) xmlNodeGetContent(find_child(code, "recvid"));
			ident->yearOfDataIssue = (char *) xmlNodeGetContent(find_child(code, "diyear"));
			ident->seqNumber       = (char *) xmlNodeGetContent(find_child(code, "seqnum"));
		} else {
			ident->senderIdent     = (char *) xmlGetProp(code, BAD_CAST "senderIdent");
			ident->receiverIdent   = (char *) xmlGetProp(code, BAD_CAST "receiverIdent");
			ident->yearOfDataIssue = (char *) xmlGetProp(code, BAD_CAST "yearOfDataIssue");
			ident->seqNumber       = (char *) xmlGetProp(code, BAD_CAST "seqNumber");
		}
	} else if (ident->type == DM || ident->type == UPF) {
		if (ident->issue == ISS_30) {
			ident->systemDiffCode     = (char *) xmlNodeGetContent(find_child(code, "sdc"));
			ident->systemCode         = (char *) xmlNodeGetContent(find_child(code, "chapnum"));
			ident->subSystemCode      = (char *) xmlNodeGetContent(find_child(code, "section"));
			ident->subSubSystemCode   = (char *) xmlNodeGetContent(find_child(code, "subsect"));
			ident->assyCode           = (char *) xmlNodeGetContent(find_child(code, "subject"));
			ident->disassyCode        = (char *) xmlNodeGetContent(find_child(code, "discode"));
			ident->disassyCodeVariant = (char *) xmlNodeGetContent(find_child(code, "discodev"));
			ident->infoCode           = (char *) xmlNodeGetContent(find_child(code, "incode"));
			ident->infoCodeVariant    = (char *) xmlNodeGetContent(find_child(code, "incodev"));
			ident->itemLocationCode   = (char *) xmlNodeGetContent(find_child(code, "itemloc"));
			ident->learnCode = NULL;
			ident->learnEventCode = NULL;
		} else {
			ident->systemDiffCode     = (char *) xmlGetProp(code, BAD_CAST "systemDiffCode");
			ident->systemCode         = (char *) xmlGetProp(code, BAD_CAST "systemCode");
			ident->subSystemCode      = (char *) xmlGetProp(code, BAD_CAST "subSystemCode");
			ident->subSubSystemCode   = (char *) xmlGetProp(code, BAD_CAST "subSubSystemCode");
			ident->assyCode           = (char *) xmlGetProp(code, BAD_CAST "assyCode");
			ident->disassyCode        = (char *) xmlGetProp(code, BAD_CAST "disassyCode");
			ident->disassyCodeVariant = (char *) xmlGetProp(code, BAD_CAST "disassyCodeVariant");
			ident->infoCode           = (char *) xmlGetProp(code, BAD_CAST "infoCode");
			ident->infoCodeVariant    = (char *) xmlGetProp(code, BAD_CAST "infoCodeVariant");
			ident->itemLocationCode   = (char *) xmlGetProp(code, BAD_CAST "itemLocationCode");
			ident->learnCode          = (char *) xmlGetProp(code, BAD_CAST "learnCode");
			ident->learnEventCode     = (char *) xmlGetProp(code, BAD_CAST "learnEventCode");
		}
	} else if (ident->type == IMF) {
		ident->imfIdentIcn = (char *) xmlGetProp(code, BAD_CAST "imfIdentIcn");
	}

	if (ident->type == DM || ident->type == PM || ident->type == DML || ident->type == IMF || ident->type == UPF) {
		const char *issueNumberName, *inWorkName;

		if (!issueInfo) return false;

		issueNumberName = ident->issue == ISS_30 ? "issno" : "issueNumber";
		inWorkName      = ident->issue == ISS_30 ? "inwork" : "inWork";

		ident->issueNumber = (char *) xmlGetProp(issueInfo, BAD_CAST issueNumberName);
		ident->inWork      = (char *) xmlGetProp(issueInfo, BAD_CAST inWorkName);

		if (!ident->inWork) {
			ident->inWork = strdup("00");
		}
	}

	if (ident->type == DM || ident->type == PM || ident->type == COM || ident->type == UPF) {
		const char *languageIsoCodeName, *countryIsoCodeName;

		if (!language) return false;

		languageIsoCodeName = ident->issue == ISS_30 ? "language" : "languageIsoCode";
		countryIsoCodeName  = ident->issue == ISS_30 ? "country" : "countryIsoCode";

		ident->languageIsoCode = (char *) xmlGetProp(language, BAD_CAST languageIsoCodeName);
		ident->countryIsoCode  = (char *) xmlGetProp(language, BAD_CAST countryIsoCodeName);
	}

	if (identExtension) {
		ident->extended = true;

		if (ident->issue == ISS_30) {
			ident->extensionProducer = (char *) xmlNodeGetContent(find_child(identExtension, "dmeproducer"));
			ident->extensionCode     = (char *) xmlNodeGetContent(find_child(identExtension, "dmecode"));
		} else {
			ident->extensionProducer = (char *) xmlGetProp(identExtension, BAD_CAST "extensionProducer");
			ident->extensionCode     = (char *) xmlGetProp(identExtension, BAD_CAST "extensionCode");
		}
	} else {
		ident->extended = false;
	}

	return true;
}

void free_ident(struct ident *ident)
{
	if (ident->extended) {
		xmlFree(ident->extensionProducer);
		xmlFree(ident->extensionCode);
	}

	xmlFree(ident->modelIdentCode);

	if (ident->type == PM) {
		xmlFree(ident->senderIdent);
		xmlFree(ident->pmNumber);
		xmlFree(ident->pmVolume);
	} else if (ident->type == DML || ident->type == COM) {
		xmlFree(ident->senderIdent);
		xmlFree(ident->yearOfDataIssue);
		xmlFree(ident->seqNumber);
		xmlFree(ident->dmlCommentType);
	} else if (ident->type == DM || ident->type == UPF) {
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
		xmlFree(ident->learnCode);
		xmlFree(ident->learnEventCode);
	} else if (ident->type == DDN) {
		xmlFree(ident->senderIdent);
		xmlFree(ident->receiverIdent);
		xmlFree(ident->yearOfDataIssue);
		xmlFree(ident->seqNumber);
	}

	if (ident->type == DM || ident->type == PM || ident->type == DML) {
		xmlFree(ident->issueNumber);
		xmlFree(ident->inWork);
	}

	if (ident->type == DM || ident->type == PM || ident->type == COM) {
		xmlFree(ident->languageIsoCode);
		xmlFree(ident->countryIsoCode);
	}
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

/* Evaluate multiple values for a property */
bool eval_multi(xmlNodePtr multi, const char *ident, const char *type, const char *value)
{
	xmlNodePtr cur;
	bool result = false;

	for (cur = multi->children; cur; cur = cur->next) {
		xmlChar *cur_value;
		bool in_set;

		cur_value = xmlNodeGetContent(cur);
		in_set = is_in_set((char *) cur_value, value);
		xmlFree(cur_value);

		if (in_set) {
			result = true;
			break;
		}
	}

	return result;
}

/* Tests whether ident:type=value was defined by the user */
bool is_applic(const char *ident, const char *type, const char *value, bool assume)
{
	xmlNodePtr cur;

	bool result = assume;

	if (!(ident || type || value)) {
		return assume;
	}

	for (cur = applicability->children; cur; cur = cur->next) {
		char *cur_ident = (char *) xmlGetProp(cur, BAD_CAST "applicPropertyIdent");
		char *cur_type  = (char *) xmlGetProp(cur, BAD_CAST "applicPropertyType");
		char *cur_value = (char *) xmlGetProp(cur, BAD_CAST "applicPropertyValues");

		bool match = strcmp(cur_ident, ident) == 0 && strcmp(cur_type, type) == 0;

		if (match) {
			if (cur_value) {
				result = is_in_set(cur_value, value);
			} else {
				result = result && eval_multi(cur, ident, type, value);
			}
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
	xmlNodePtr ident_attr, type_attr, values_attr;
	char *ident, *type, *values;

	bool ret;

	ident_attr  = first_xpath_node(NULL, assert, "@applicPropertyIdent|@actidref");
	type_attr   = first_xpath_node(NULL, assert, "@applicPropertyType|@actreftype");
	values_attr = first_xpath_node(NULL, assert, "@applicPropertyValues|@actvalues");

	ident  = (char *) xmlNodeGetContent(ident_attr);
	type   = (char *) xmlNodeGetContent(type_attr);
	values = (char *) xmlNodeGetContent(values_attr);

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

	op = (char *) xmlGetProp(evaluate, BAD_CAST "andOr");

	if (!op) {
		fprintf(stderr, S_MISSING_ANDOR);
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

	if (!stmt) {
		return assume;
	}

	return eval_applic(stmt, assume);
}

/* Search recursively for a descendant element with the given id */
xmlNodePtr get_element_by_id(xmlNodePtr root, const char *id)
{
	xmlNodePtr cur;
	char *cid;

	if (!root) {
		return NULL;
	}

	for (cur = root->children; cur; cur = cur->next) {
		xmlNodePtr ch;
		bool match;

		cid = (char *) xmlGetProp(cur, BAD_CAST "id");

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
	xmlNodePtr attr;

	attr = first_xpath_node(NULL, node, "@applicRefId|@refapplic");

	if (attr) {
		xmlChar *applicRefId;
		xmlNodePtr applic;

		applicRefId = xmlNodeGetContent(attr);
		applic = get_element_by_id(referencedApplicGroup, (char *) applicRefId);
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

/* Remove unambigously true or false applic statements. */
void clean_applic_stmts(xmlNodePtr referencedApplicGroup)
{
	xmlNodePtr cur;

	cur = referencedApplicGroup->children;

	while (cur) {
		xmlNodePtr next = cur->next;

		if (cur->type == XML_ELEMENT_NODE && (eval_applic_stmt(cur, false) || !eval_applic_stmt(cur, true))) {
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
		}

		cur = next;
	}
}

/* Remove applic references on content where the applic statement was removed by clean_applic_stmts. */
void clean_applic(xmlNodePtr referencedApplicGroup, xmlNodePtr node)
{
	xmlNodePtr cur;

	if (xmlHasProp(node, BAD_CAST "applicRefId")) {
		char *applicRefId;
		xmlNodePtr applic;

		applicRefId = (char *) xmlGetProp(node, BAD_CAST "applicRefId");
		applic = get_element_by_id(referencedApplicGroup, applicRefId);
		xmlFree(applicRefId);

		if (!applic) {
			xmlUnsetProp(node, BAD_CAST "applicRefId");
		}
	}

	for (cur = node->children; cur; cur = cur->next) {
		clean_applic(referencedApplicGroup, cur);
	}
}

/* Remove applic statements or parts of applic statements where all assertions
 * are unambigously true or false */
bool simpl_applic(xmlNodePtr node)
{
	xmlNodePtr cur, next;

	if (strcmp((char *) node->name, "applic") == 0) {
		if (eval_applic_stmt(node, false) || !eval_applic_stmt(node, true)) {
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			return true;
		}
	} else if (strcmp((char *) node->name, "evaluate") == 0) {
		if (eval_applic(node, false) || !eval_applic(node, true)) {
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			return false;
		}
	} else if (strcmp((char *) node->name, "assert") == 0) {
		xmlNodePtr ident_attr  = first_xpath_node(NULL, node, "@applicPropertyIdent|@actidref");
		xmlNodePtr type_attr   = first_xpath_node(NULL, node, "@applicPropertyType|@actreftype");
		xmlNodePtr values_attr = first_xpath_node(NULL, node, "@applicPropertyValues|@actvalues");

		char *ident  = (char *) xmlNodeGetContent(ident_attr);
		char *type   = (char *) xmlNodeGetContent(type_attr);
		char *values = (char *) xmlNodeGetContent(values_attr);

		bool uneeded = is_applic(ident, type, values, false) || !is_applic(ident, type, values, true);

		xmlFree(ident);
		xmlFree(type);
		xmlFree(values);

		if (uneeded) {
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			return false;
		}
	}

	cur = node->children;
	while (cur) {
		next = cur->next;
		simpl_applic(cur);
		cur = next;
	}

	return false;
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
	xmlNodePtr cur, next;

	if (!node) {
		return;
	}

	cur = node->children;
	while (cur) {
		next = cur->next;
		if (cur->type == XML_ELEMENT_NODE) {
			simpl_applic_evals(cur);
		}
		cur = next;
	}

	if (xmlStrcmp(node->name, BAD_CAST "evaluate") == 0) {
		simpl_evaluate(node);
	}
}

/* Remove <referencedApplicGroup> if all applic statements are removed */
void simpl_applic_clean(xmlNodePtr referencedApplicGroup)
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

xmlNodePtr simpl_whole_applic(xmlDocPtr doc)
{
	xmlNodePtr applic, orig;

	orig = first_xpath_node(doc, NULL, "//dmStatus/applic|//pmStatus/applic");

	if (!orig) {
		return NULL;
	}

	applic = xmlCopyNode(orig, 1);

	if (simpl_applic(applic)) {
		xmlNodePtr disptext;
		applic = xmlNewNode(NULL, BAD_CAST "applic");
		disptext = xmlNewChild(applic, NULL, BAD_CAST "displayText", NULL);
		xmlNewChild(disptext, NULL, BAD_CAST "simplePara", BAD_CAST "All");
	} else {
		simpl_applic_evals(applic);
	}

	xmlAddNextSibling(orig, applic);
	xmlUnlinkNode(orig);
	xmlFreeNode(orig);

	return applic;
}

/* Add metadata linking the data module instance with the master data module */
void add_source(xmlDocPtr doc)
{
	xmlNodePtr ident, sourceIdent, node, cur;
	const xmlChar *type;

	ident       = first_xpath_node(doc, NULL, "//dmIdent|//pmIdent|//dmaddres");
	sourceIdent = first_xpath_node(doc, NULL, "//dmStatus/sourceDmIdent|//pmStatus/sourcePmIdent|//status/srcdmaddres");
	node        = first_xpath_node(doc, NULL, "(//dmStatus/repositorySourceDmIdent|//dmStatus/security|//pmStatus/security|//status/security)[1]");

	if (!node) {
		return;
	}

	if (sourceIdent) {
		xmlUnlinkNode(sourceIdent);
		xmlFreeNode(sourceIdent);
	}

	type = ident->name;

	if (xmlStrcmp(type, BAD_CAST "dmIdent") == 0) {
		sourceIdent = xmlNewNode(NULL, BAD_CAST "sourceDmIdent");
	} else if (xmlStrcmp(type, BAD_CAST "pmIdent") == 0) {
		sourceIdent = xmlNewNode(NULL, BAD_CAST "sourcePmIdent");
	} else if (xmlStrcmp(type, BAD_CAST "dmaddres") == 0) {
		sourceIdent = xmlNewNode(NULL, BAD_CAST "srcdmaddres");
	} else {
		return;
	}

	sourceIdent = xmlAddPrevSibling(node, sourceIdent);

	for (cur = ident->children; cur; cur = cur->next) {
		xmlAddChild(sourceIdent, xmlCopyNode(cur, 1));
	}
}

/* Add an extension to the data module code */
void set_extd(xmlDocPtr doc, const char *extension)
{
	xmlNodePtr identExtension, code;
	char *ext, *extensionProducer, *extensionCode;
	enum issue issue;

	identExtension = first_xpath_node(doc, NULL, "//dmIdent/identExtension|//pmIdent/identExtension|//dmaddres/dmcextension");
	code = first_xpath_node(doc, NULL, "//dmIdent/dmCode|//pmIdent/pmCode|//dmaddres/dmc");

	if (xmlStrcmp(code->name, BAD_CAST "dmCode") == 0) {
		issue = ISS_4X;
	} else if (xmlStrcmp(code->name, BAD_CAST "pmCode") == 0) {
		issue = ISS_4X;
	} else if (xmlStrcmp(code->name, BAD_CAST "dmc") == 0) {
		issue = ISS_30;
	} else {
		return;
	}

	ext = strdup(extension);

	if (!identExtension) {
		identExtension = xmlNewNode(NULL, BAD_CAST (issue == ISS_30 ? "dmcextension" : "identExtension"));
		identExtension = xmlAddPrevSibling(code, identExtension);
	}

	extensionProducer = strtok(ext, "-");
	extensionCode     = strtok(NULL, "-");

	if (issue == ISS_30) {
		xmlNewChild(identExtension, NULL, BAD_CAST "dmeproducer", BAD_CAST extensionProducer);
		xmlNewChild(identExtension, NULL, BAD_CAST "dmecode", BAD_CAST extensionCode);
	} else {
		xmlSetProp(identExtension, BAD_CAST "extensionProducer", BAD_CAST extensionProducer);
		xmlSetProp(identExtension, BAD_CAST "extensionCode",     BAD_CAST extensionCode);
	}

	free(ext);
}

void set_dm_code(xmlNodePtr code, enum issue iss, const char *s)
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

	n = sscanf(s,
		"%14[^-]-%4[^-]-%3[^-]-%1s%1s-%4[^-]-%2s%3[^-]-%3s%1s-%1s-%3s%1s",
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
		fprintf(stderr, S_BAD_CODE, "data module", s);
		exit(EXIT_BAD_ARG);
	}

	if (iss == ISS_4X) {
		xmlSetProp(code, BAD_CAST "modelIdentCode",     BAD_CAST model_ident_code);
		xmlSetProp(code, BAD_CAST "systemDiffCode",     BAD_CAST system_diff_code);
		xmlSetProp(code, BAD_CAST "systemCode",         BAD_CAST system_code);
		xmlSetProp(code, BAD_CAST "subSystemCode",      BAD_CAST sub_system_code);
		xmlSetProp(code, BAD_CAST "subSubSystemCode",   BAD_CAST sub_sub_system_code);
		xmlSetProp(code, BAD_CAST "assyCode",           BAD_CAST assy_code);
		xmlSetProp(code, BAD_CAST "disassyCode",        BAD_CAST disassy_code);
		xmlSetProp(code, BAD_CAST "disassyCodeVariant", BAD_CAST disassy_code_variant);
		xmlSetProp(code, BAD_CAST "infoCode",           BAD_CAST info_code);
		xmlSetProp(code, BAD_CAST "infoCodeVariant",    BAD_CAST info_code_variant);
		xmlSetProp(code, BAD_CAST "itemLocationCode",   BAD_CAST item_location_code);

		if (n == 13) {
			xmlSetProp(code, BAD_CAST "learnCode", BAD_CAST learn_code);
			xmlSetProp(code, BAD_CAST "learnEventCode", BAD_CAST learn_event_code);
		}
	} else if (iss == ISS_30) {
		xmlNodeSetContent(find_child(code, "modelic"), BAD_CAST model_ident_code);
		xmlNodeSetContent(find_child(code, "sdc"), BAD_CAST system_diff_code);
		xmlNodeSetContent(find_child(code, "chapnum"), BAD_CAST system_code);
		xmlNodeSetContent(find_child(code, "section"), BAD_CAST sub_system_code);
		xmlNodeSetContent(find_child(code, "subsect"), BAD_CAST sub_sub_system_code);
		xmlNodeSetContent(find_child(code, "subject"), BAD_CAST assy_code);
		xmlNodeSetContent(find_child(code, "discode"), BAD_CAST disassy_code);
		xmlNodeSetContent(find_child(code, "discodev"), BAD_CAST disassy_code_variant);
		xmlNodeSetContent(find_child(code, "incode"), BAD_CAST info_code);
		xmlNodeSetContent(find_child(code, "incodev"), BAD_CAST info_code_variant);
		xmlNodeSetContent(find_child(code, "ilc"), BAD_CAST item_location_code);
	}
}

void set_pm_code(xmlNodePtr code, enum issue iss, const char *s)
{
	char model_ident_code[15];
	char pm_issuer[6];
	char pm_number[6];
	char pm_volume[3];
	int n;

	n = sscanf(s,
		"%14[^-]-%5[^-]-%5[^-]-%2s",
		model_ident_code,
		pm_issuer,
		pm_number,
		pm_volume);

	if (n != 4) {
		fprintf(stderr, S_BAD_CODE, "publication module", s);
		exit(EXIT_BAD_ARG);
	}

	if (iss == ISS_4X) {
		xmlSetProp(code, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
		xmlSetProp(code, BAD_CAST "pmIssuer", BAD_CAST pm_issuer);
		xmlSetProp(code, BAD_CAST "pmNumber", BAD_CAST pm_number);
		xmlSetProp(code, BAD_CAST "pmVolume", BAD_CAST pm_volume);
	} else if (iss == ISS_30) {
		xmlNodeSetContent(find_child(code, "modelic"), BAD_CAST model_ident_code);
		xmlNodeSetContent(find_child(code, "pmissuer"), BAD_CAST pm_issuer);
		xmlNodeSetContent(find_child(code, "pmnumber"), BAD_CAST pm_number);
		xmlNodeSetContent(find_child(code, "pmvolume"), BAD_CAST pm_volume);
	}
}

void set_com_code(xmlNodePtr code, enum issue iss, const char *s)
{
	char model_ident_code[15];
	char sender_ident[6];
	char year_of_data_issue[5];
	char seq_number[6];
	char comment_type[2];
	int n;

	n = sscanf(s,
		"%14[^-]-%5s-%4s-%5s-%1s",
		model_ident_code,
		sender_ident,
		year_of_data_issue,
		seq_number,
		comment_type);

	if (n != 5) {
		fprintf(stderr, S_BAD_CODE, "comment", s);
		exit(EXIT_BAD_ARG);
	}

	lowercase(comment_type);

	if (iss == ISS_4X) {
		xmlSetProp(code, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
		xmlSetProp(code, BAD_CAST "senderIdent", BAD_CAST sender_ident);
		xmlSetProp(code, BAD_CAST "yearOfDataIssue", BAD_CAST year_of_data_issue);
		xmlSetProp(code, BAD_CAST "seqNumber", BAD_CAST seq_number);
		xmlSetProp(code, BAD_CAST "commentType", BAD_CAST comment_type);
	} else if (iss == ISS_30) {
		xmlNodeSetContent(find_child(code, "modelic"), BAD_CAST model_ident_code);
		xmlNodeSetContent(find_child(code, "sendid"), BAD_CAST sender_ident);
		xmlNodeSetContent(find_child(code, "diyear"), BAD_CAST year_of_data_issue);
		xmlNodeSetContent(find_child(code, "seqnum"), BAD_CAST seq_number);
	}
}

void set_dml_code(xmlNodePtr code, enum issue iss, const char *s)
{
	char model_ident_code[15];
	char sender_ident[6];
	char dml_type[2];
	char year_of_data_issue[5];
	char seq_number[6];
	int n;

	n = sscanf(s,
		"%14[^-]-%5s-%1s-%4s-%5s",
		model_ident_code,
		sender_ident,
		dml_type,
		year_of_data_issue,
		seq_number);

	if (n != 5) {
		fprintf(stderr, S_BAD_CODE, "data management list", s);
		exit(EXIT_BAD_ARG);
	}

	lowercase(dml_type);

	if (iss == ISS_4X) {
		xmlSetProp(code, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
		xmlSetProp(code, BAD_CAST "senderIdent", BAD_CAST sender_ident);
		xmlSetProp(code, BAD_CAST "dmlType", BAD_CAST dml_type);
		xmlSetProp(code, BAD_CAST "yearOfDataIssue", BAD_CAST year_of_data_issue);
		xmlSetProp(code, BAD_CAST "seqNumber", BAD_CAST seq_number);
	} else if (iss == ISS_30) {
		xmlNodeSetContent(find_child(code, "modelic"), BAD_CAST model_ident_code);
		xmlNodeSetContent(find_child(code, "sendid"), BAD_CAST sender_ident);
		xmlSetProp(find_child(code, "dmltype"), BAD_CAST "type", BAD_CAST dml_type);
		xmlNodeSetContent(find_child(code, "diyear"), BAD_CAST year_of_data_issue);
		xmlNodeSetContent(find_child(code, "seqnum"), BAD_CAST seq_number);
	}
}

void set_code(xmlDocPtr doc, const char *new_code)
{
	xmlNodePtr code;

	code = first_xpath_node(doc, NULL,
		"//dmIdent/dmCode|"
		"//pmIdent/pmCode|"
		"//commentIdent/commentCode|"
		"//dmlIdent/dmlCode|"
		"//dmaddres/dmc/avee|"
		"//pmaddres/pmc|"
		"//cstatus/ccode|"
		"//dml/dmlc");

	if (!code) {
		return;
	}

	if (xmlStrcmp(code->name, BAD_CAST "dmCode") == 0) {
		set_dm_code(code, ISS_4X, new_code);
	} else if (xmlStrcmp(code->name, BAD_CAST "avee") == 0) {
		set_dm_code(code, ISS_30, new_code);
	} else if (xmlStrcmp(code->name, BAD_CAST "pmCode") == 0) {
		set_pm_code(code, ISS_4X, new_code);
	} else if (xmlStrcmp(code->name, BAD_CAST "pmc") == 0) {
		set_pm_code(code, ISS_30, new_code);
	} else if (xmlStrcmp(code->name, BAD_CAST "commentCode") == 0) {
		set_com_code(code, ISS_4X, new_code);
	} else if (xmlStrcmp(code->name, BAD_CAST "ccode") == 0) {
		set_com_code(code, ISS_30, new_code);
	} else if (xmlStrcmp(code->name, BAD_CAST "dmlCode") == 0) {
		set_dml_code(code, ISS_4X, new_code);
	} else if (xmlStrcmp(code->name, BAD_CAST "dmlc") == 0) {
		set_dml_code(code, ISS_30, new_code);
	}
}

/* Set the techName and/or infoName of the data module instance */
void set_title(xmlDocPtr doc, char *tech, char *info)
{
	xmlNodePtr dmTitle, techName, infoName;
	enum issue iss;
	
	dmTitle  = first_xpath_node(doc, NULL,
		"//dmAddressItems/dmTitle|"
		"//dmaddres/dmtitle");
	techName = first_xpath_node(doc, NULL,
		"//dmAddressItems/dmTitle/techName|"
		"//pmAddressItems/pmTitle|"
		"//commentAddressItems/commentTitle|"
		"//dmaddres/dmtitle/techname|"
		"//pmaddres/pmtitle|"
		"//cstatus/ctitle");
	infoName = first_xpath_node(doc, NULL,
		"//dmAddressItems/dmTitle/infoName|"
		"//dmaddres/dmtitle/infoname");

	if (!techName) {
		return;
	}

	if (xmlStrcmp(techName->name, BAD_CAST "techName") == 0) {
		iss = ISS_4X;
	} else if (xmlStrcmp(techName->name, BAD_CAST "pmTitle") == 0) {
		iss = ISS_4X;
	} else if (xmlStrcmp(techName->name, BAD_CAST "commentTitle") == 0) {
		iss = ISS_4X;
	} else {
		iss = ISS_30;
	}

	if (strcmp(tech, "") != 0) {
		xmlNodeSetContent(techName, BAD_CAST tech);
	}

	if (strcmp(info, "") != 0) {
		if (!infoName) {
			infoName = xmlNewChild(dmTitle, NULL, BAD_CAST (iss == ISS_30 ? "infoname" : "infoName"), NULL);
		}
		xmlNodeSetContent(infoName, BAD_CAST info);
	}	
}

xmlNodePtr create_assert(xmlChar *ident, xmlChar *type, xmlChar *values, enum issue iss)
{
	xmlNodePtr assert;

	assert = xmlNewNode(NULL, BAD_CAST "assert");

	xmlSetProp(assert, BAD_CAST (iss == ISS_30 ? "actidref" : "applicPropertyIdent"), ident);
	xmlSetProp(assert, BAD_CAST (iss == ISS_30 ? "actreftype" : "applicPropertyType"), type);
	xmlSetProp(assert, BAD_CAST (iss == ISS_30 ? "actvalues" : "applicPropertyValues"), values);

	return assert;
}

xmlNodePtr create_or(xmlChar *ident, xmlChar *type, xmlNodePtr values, enum issue iss)
{
	xmlNodePtr or, cur;

	or = xmlNewNode(NULL, BAD_CAST "evaluate");
	xmlSetProp(or, BAD_CAST (iss == ISS_30 ? "operator" : "andOr"), BAD_CAST "or");

	for (cur = values->children; cur; cur = cur->next) {
		xmlChar *value;

		value = xmlNodeGetContent(cur);
		xmlAddChild(or, create_assert(ident, type, value, iss));
		xmlFree(value);
	}

	return or;
}

/* Set the applicability for the whole data module instance */
void set_applic(xmlDocPtr doc, char *new_text, bool combine)
{
	xmlNodePtr new_applic, new_displayText, new_simplePara, new_evaluate, cur, applic;
	enum issue iss;

	applic = first_xpath_node(doc, NULL, "//dmStatus/applic|//pmStatus/applic|//status/applic|//pmstatus/applic");

	if (!applic) {
		return;
	} else if (xmlStrcmp(applic->parent->name, BAD_CAST "dmStatus") == 0 || xmlStrcmp(applic->parent->name, BAD_CAST "pmStatus") == 0) {
		iss = ISS_4X;
	} else if (xmlStrcmp(applic->parent->name, BAD_CAST "status") == 0 || xmlStrcmp(applic->parent->name, BAD_CAST "pmstatus") == 0) {
		iss = ISS_30;
	} else {
		return;
	}

	new_applic = xmlNewNode(NULL, BAD_CAST "applic");
	xmlAddNextSibling(applic, new_applic);

	if (strcmp(new_text, "") != 0) {
		new_displayText = xmlNewChild(new_applic, NULL, BAD_CAST (iss == ISS_30 ? "displaytext" : "displayText"), NULL);
		new_simplePara = xmlNewChild(new_displayText, NULL, BAD_CAST (iss == ISS_30 ? "p" : "simplePara"), NULL);
		xmlNodeSetContent(new_simplePara, BAD_CAST new_text);
	}

	if (combine && first_xpath_node(doc, applic, "assert|evaluate|expression")) {
		new_applic = xmlNewChild(new_applic, NULL, BAD_CAST "evaluate", NULL);
		xmlSetProp(new_applic, BAD_CAST (iss == ISS_30 ? "operator" : "andOr"), BAD_CAST "and");
		for (cur = applic->children; cur; cur = cur->next) {
			if (cur->type != XML_ELEMENT_NODE || xmlStrcmp(cur->name, BAD_CAST "displayText") == 0 || xmlStrcmp(cur->name, BAD_CAST "displaytext") == 0) {
				continue;
			}
			xmlAddChild(new_applic, xmlCopyNode(cur, 1));
		}
	}

	if (napplics > 1) {
		new_evaluate = xmlNewChild(new_applic, NULL, BAD_CAST "evaluate", NULL);
		xmlSetProp(new_evaluate, BAD_CAST (iss == ISS_30 ? "operator" : "andOr"), BAD_CAST "and");
	} else {
		new_evaluate = new_applic;
	}

	for (cur = applicability->children; cur; cur = cur->next) {
		xmlChar *cur_ident, *cur_type, *cur_value;

		cur_ident = xmlGetProp(cur, BAD_CAST "applicPropertyIdent");
		cur_type  = xmlGetProp(cur, BAD_CAST "applicPropertyType");
		cur_value = xmlGetProp(cur, BAD_CAST "applicPropertyValues");

		if (cur_value) {
			xmlAddChild(new_evaluate, create_assert(cur_ident, cur_type, cur_value, iss));
		} else {
			xmlAddChild(new_evaluate, create_or(cur_ident, cur_type, cur, iss));
		}

		xmlFree(cur_ident);
		xmlFree(cur_type);
		xmlFree(cur_value);
	}

	xmlUnlinkNode(applic);
	xmlFreeNode(applic);
}

/* Set the language/country for the data module instance */
void set_lang(xmlDocPtr doc, char *lang)
{
	xmlNodePtr language;
	char *language_iso_code;
	char *country_iso_code;
	enum issue iss;

	language = first_xpath_node(doc, NULL, LANGUAGE_XPATH);

	if (!language) {
		return;
	} else if (xmlStrcmp(language->parent->name, BAD_CAST "dmIdent") == 0 ||
	           xmlStrcmp(language->parent->name, BAD_CAST "pmIdent") == 0) {
		iss = ISS_4X;
	} else if (xmlStrcmp(language->parent->name, BAD_CAST "dmaddres") == 0 ||
	           xmlStrcmp(language->parent->name, BAD_CAST "pmaddres") == 0) {
		iss = ISS_30;
	} else {
		return;
	}

	language_iso_code = strtok(lang, "-");
	country_iso_code = strtok(NULL, "");

	lowercase(language_iso_code);
	uppercase(country_iso_code);

	xmlSetProp(language, BAD_CAST (iss == ISS_30 ? "language" : "languageIsoCode"), BAD_CAST language_iso_code);
	xmlSetProp(language, BAD_CAST (iss == ISS_30 ? "country" : "countryIsoCode"), BAD_CAST country_iso_code);
}

bool auto_name(char *out, char *src, xmlDocPtr dm, const char *dir, bool noiss)
{
	struct ident ident;
	char iss[8] = "";

	if (!init_ident(&ident, dm)) {
		char *base;
		base = basename(src);
		snprintf(out, PATH_MAX, "%s/%s", dir, base);
		return true;
	}

	if (ident.type == DM || ident.type == PM || ident.type == COM || ident.type == UPF) {
		int i;
		for (i = 0; ident.languageIsoCode[i]; ++i) {
			ident.languageIsoCode[i] = toupper(ident.languageIsoCode[i]);
		}
	}

	if (ident.type == DML || ident.type == COM) {
		ident.dmlCommentType[0] = toupper(ident.dmlCommentType[0]);
	}	

	if (!noiss && (ident.type == DM || ident.type == PM || ident.type == DML || ident.type == IMF || ident.type == UPF)) {
		sprintf(iss, "_%s-%s", ident.issueNumber, ident.inWork);
	}

	if (ident.type == PM) {
		if (ident.extended) {
			sprintf(out, "%s/PME-%s-%s-%s-%s-%s-%s%s_%s-%s.XML",
				dir,
				ident.extensionProducer,
				ident.extensionCode,
				ident.modelIdentCode,
				ident.senderIdent,
				ident.pmNumber,
				ident.pmVolume,
				iss,
				ident.languageIsoCode,
				ident.countryIsoCode);
		} else {
			sprintf(out, "%s/PMC-%s-%s-%s-%s%s_%s-%s.XML",
				dir,
				ident.modelIdentCode,
				ident.senderIdent,
				ident.pmNumber,
				ident.pmVolume,
				iss,
				ident.languageIsoCode,
				ident.countryIsoCode);
		}
	} else if (ident.type == DML) {
		sprintf(out, "%s/DML-%s-%s-%s-%s-%s%s.XML",
			dir,
			ident.modelIdentCode,
			ident.senderIdent,
			ident.dmlCommentType,
			ident.yearOfDataIssue,
			ident.seqNumber,
			iss);
	} else if (ident.type == DM || ident.type == UPF) {
		char learn[6] = "";

		if (ident.learnCode && ident.learnEventCode) {
			sprintf(learn, "-%s%s", ident.learnCode, ident.learnEventCode);
		}

		if (ident.extended) {
			sprintf(out, "%s/%s-%s-%s-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s%s%s_%s-%s.XML",
				dir,
				ident.type == DM ? "DME" : "UPE",
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
				learn,
				iss,
				ident.languageIsoCode,
				ident.countryIsoCode);
		} else {
			sprintf(out, "%s/%s-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s%s%s_%s-%s.XML",
				dir,
				ident.type == DM ? "DMC" : "UPF",
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
				learn,
				iss,
				ident.languageIsoCode,
				ident.countryIsoCode);
		}
	} else if (ident.type == COM) {
		sprintf(out, "%s/COM-%s-%s-%s-%s-%s_%s-%s.XML",
			dir,
			ident.modelIdentCode,
			ident.senderIdent,
			ident.yearOfDataIssue,
			ident.seqNumber,
			ident.dmlCommentType,
			ident.languageIsoCode,
			ident.countryIsoCode);
	} else if (ident.type == DDN) {
		sprintf(out, "%s/DDN-%s-%s-%s-%s-%s.XML",
			dir,
			ident.modelIdentCode,
			ident.senderIdent,
			ident.receiverIdent,
			ident.yearOfDataIssue,
			ident.seqNumber);
	} else if (ident.type == IMF) {
		sprintf(out, "%s/IMF-%s%s.XML",
			dir,
			ident.imfIdentIcn,
			iss);
	} else {
		return false;
	}

	free_ident(&ident);

	return true;
}

/* Add an "identity" template to an XSL stylesheet */
void add_identity(xmlDocPtr style)
{
	xmlDocPtr identity;
	xmlNodePtr stylesheet, first, template;

	identity = xmlReadMemory((const char *) ___common_identity_xsl, ___common_identity_xsl_len, NULL, NULL, 0);
	template = xmlFirstElementChild(xmlDocGetRootElement(identity));

	stylesheet = xmlDocGetRootElement(style);

	first = xmlFirstElementChild(stylesheet);

	if (first) {
		xmlAddPrevSibling(first, xmlCopyNode(template, 1));
	} else {
		xmlAddChild(stylesheet, xmlCopyNode(template, 1));
	}

	xmlFreeDoc(identity);
}

/* Get the appropriate built-in CIR repository XSLT by name */
bool get_cir_xsl(const char *cirtype, unsigned char **xsl, unsigned int *len)
{
	if (strcmp(cirtype, "accessPointRepository") == 0) {
		*xsl = cirxsl_accessPointRepository_xsl;
		*len = cirxsl_accessPointRepository_xsl_len;
	} else if (strcmp(cirtype, "applicRepository") == 0) {
		*xsl = cirxsl_applicRepository_xsl;
		*len = cirxsl_applicRepository_xsl_len;
	} else if (strcmp(cirtype, "cautionRepository") == 0) {
		*xsl = cirxsl_cautionRepository_xsl;
		*len = cirxsl_cautionRepository_xsl_len;
	} else if (strcmp(cirtype, "circuitBreakerRepository") == 0) {
		*xsl = cirxsl_circuitBreakerRepository_xsl;
		*len = cirxsl_circuitBreakerRepository_xsl_len;
	} else if (strcmp(cirtype, "controlIndicatorRepository") == 0) {
		*xsl = cirxsl_controlIndicatorRepository_xsl;
		*len = cirxsl_controlIndicatorRepository_xsl_len;
	} else if (strcmp(cirtype, "enterpriseRepository") == 0) {
		*xsl = cirxsl_enterpriseRepository_xsl;
		*len = cirxsl_enterpriseRepository_xsl_len;
	} else if (strcmp(cirtype, "functionalItemRepository") == 0) {
		*xsl = cirxsl_functionalItemRepository_xsl;
		*len = cirxsl_functionalItemRepository_xsl_len;
	} else if (strcmp(cirtype, "einlist") == 0) {
		*xsl = cirxsl_einlist_xsl;
		*len = cirxsl_einlist_xsl_len;
	} else if (strcmp(cirtype, "partRepository") == 0) {
		*xsl = cirxsl_partRepository_xsl;
		*len = cirxsl_partRepository_xsl_len;
	} else if (strcmp(cirtype, "illustratedPartsCatalog") == 0) {
		*xsl = cirxsl_illustratedPartsCatalog_xsl;
		*len = cirxsl_illustratedPartsCatalog_xsl_len;
	} else if (strcmp(cirtype, "supplyRepository") == 0) {
		*xsl = cirxsl_supplyRepository_xsl;
		*len = cirxsl_supplyRepository_xsl_len;
	} else if (strcmp(cirtype, "toolRepository") == 0) {
		*xsl = cirxsl_toolRepository_xsl;
		*len = cirxsl_toolRepository_xsl_len;
	} else if (strcmp(cirtype, "warningRepository") == 0) {
		*xsl = cirxsl_warningRepository_xsl;
		*len = cirxsl_warningRepository_xsl_len;
	} else if (strcmp(cirtype, "zoneRepository") == 0) {
		*xsl = cirxsl_zoneRepository_xsl;
		*len = cirxsl_zoneRepository_xsl_len;
	} else {
		fprintf(stderr, S_NO_XSLT, cirtype);
		return false;
	}

	return true;
}

/* Dump built-in XSLT for resolving CIR repository dependencies */
void dump_cir_xsl(const char *repo)
{
	unsigned char *xsl;
	unsigned int len;

	if (get_cir_xsl(repo, &xsl, &len)) {
		printf("%.*s", len, xsl);
	} else {
		exit(EXIT_BAD_ARG);
	}
}

/* Use user-supplied XSL script to resolve CIR references. */
void undepend_cir_xsl(xmlDocPtr dm, xmlDocPtr cir, xsltStylesheetPtr style)
{
	xmlDocPtr res, muxdoc;
	xmlNodePtr mux;

	muxdoc = xmlNewDoc(BAD_CAST "1.0");
	mux = xmlNewNode(NULL, BAD_CAST "mux");
	xmlDocSetRootElement(muxdoc, mux);
	xmlAddChild(mux, xmlCopyNode(xmlDocGetRootElement(dm), 1));
	xmlAddChild(mux, xmlCopyNode(xmlDocGetRootElement(cir), 1));

	res = xsltApplyStylesheet(style, muxdoc, NULL);

	xmlDocSetRootElement(dm, xmlCopyNode(first_xpath_node(res, NULL, "/mux/dmodule[1]"), 1));
	xmlFreeDoc(res);
	xmlFreeDoc(muxdoc);
}

/* Apply the user-defined applicability to the CIR data module, then call the
 * appropriate function for the specific type of CIR. */
void undepend_cir(xmlDocPtr dm, const char *cirdocfname, bool add_src, const char *cir_xsl)
{
	xmlDocPtr cir;
	xmlXPathContextPtr ctxt;
	xmlXPathObjectPtr results;

	xmlNodePtr cirnode;
	xmlNodePtr content;
	xmlNodePtr referencedApplicGroup;

	char *cirtype;

	xmlDocPtr styledoc = NULL;

	cir = xmlReadFile(cirdocfname, NULL, PARSE_OPTS);

	if (!cir) {
		fprintf(stderr, S_INVALID_CIR, cirdocfname);
		exit(EXIT_BAD_XML);
	}

	ctxt = xmlXPathNewContext(cir);

	results = xmlXPathEvalExpression(BAD_CAST "//content", ctxt);
	content = results->nodesetval->nodeTab[0];
	xmlXPathFreeObject(results);

	results = xmlXPathEvalExpression(BAD_CAST "//referencedApplicGroup", ctxt);

	if (!xmlXPathNodeSetIsEmpty(results->nodesetval)) {
		referencedApplicGroup = results->nodesetval->nodeTab[0];
		strip_applic(referencedApplicGroup, content);
	}

	xmlXPathFreeObject(results);

	results = xmlXPathEvalExpression(BAD_CAST
		"//content/commonRepository/*[position()=last()]|"
		"//content/techRepository/*[position()=last()]|"
		"//content/techrep/*[position()=last()]|"
		"//content/illustratedPartsCatalog",
		ctxt);

	if (xmlXPathNodeSetIsEmpty(results->nodesetval)) {
		fprintf(stderr, S_INVALID_CIR, cirdocfname);
		exit(EXIT_BAD_XML);
	}

	cirnode = results->nodesetval->nodeTab[0];
	xmlXPathFreeObject(results);

	cirtype = (char *) cirnode->name;

	if (cir_xsl) {
		styledoc = xmlReadFile(cir_xsl, NULL, PARSE_OPTS);
	} else {
		unsigned char *xsl = NULL;
		unsigned int len = 0;

		if (!get_cir_xsl(cirtype, &xsl, &len)) {
			add_src = false;
		}

		styledoc = xmlReadMemory((const char *) xsl, len, NULL, NULL, 0);
	}

	if (styledoc) {
		xsltStylesheetPtr style;
		add_identity(styledoc);
		style = xsltParseStylesheetDoc(styledoc);
		undepend_cir_xsl(dm, cir, style);
		xsltFreeStylesheet(style);
	}

	xmlXPathFreeContext(ctxt);

	if (first_xpath_node(dm, NULL, "//idstatus")) {
		add_src = false;
	}

	if (add_src) {
		xmlNodePtr security, dmIdent, repositorySourceDmIdent, cur;

		ctxt = xmlXPathNewContext(dm);
		results = xmlXPathEvalExpression(BAD_CAST "//security", ctxt);
		security = results->nodesetval->nodeTab[0];
		xmlXPathFreeObject(results);
		xmlXPathFreeContext(ctxt);

		repositorySourceDmIdent = xmlNewNode(NULL, BAD_CAST "repositorySourceDmIdent");
		repositorySourceDmIdent = xmlAddPrevSibling(security, repositorySourceDmIdent);

		ctxt = xmlXPathNewContext(cir);
		results = xmlXPathEvalExpression(BAD_CAST "//dmIdent", ctxt);
		dmIdent = results->nodesetval->nodeTab[0];
		xmlXPathFreeObject(results);
		xmlXPathFreeContext(ctxt);

		for (cur = dmIdent->children; cur; cur = cur->next) {
			xmlAddChild(repositorySourceDmIdent, xmlCopyNode(cur, 1));
		}
	}

	xmlFreeDoc(cir);
}

/* Set the issue and inwork numbers of the instance. */
void set_issue(xmlDocPtr dm, char *issinfo)
{
	char issue[4], inwork[3];
	xmlNodePtr issueInfo;
	enum issue iss;

	if (sscanf(issinfo, "%3s-%2s", issue, inwork) != 2) {
		fprintf(stderr, ERR_PREFIX S_INVALID_ISSFMT);
		exit(EXIT_MISSING_ARGS);
	}

	issueInfo = first_xpath_node(dm, NULL, ISSUE_INFO_XPATH);

	if (!issueInfo) {
		return;
	}

	if (xmlStrcmp(issueInfo->name, BAD_CAST "issueInfo") == 0) {
		iss = ISS_4X;
	} else {
		iss = ISS_30;
	}

	if (iss == ISS_30) {
		xmlSetProp(issueInfo, BAD_CAST "issno", BAD_CAST issue);
		xmlSetProp(issueInfo, BAD_CAST "inwork", BAD_CAST inwork);
	} else {
		xmlSetProp(issueInfo, BAD_CAST "issueNumber", BAD_CAST issue);
		xmlSetProp(issueInfo, BAD_CAST "inWork", BAD_CAST inwork);
	}
}

/* Set the issue date of the instance. */
void set_issue_date(xmlDocPtr doc, const char *year, const char *month, const char *day)
{
	xmlNodePtr issueDate;

	issueDate = first_xpath_node(doc, NULL, "//issueDate|//issdate");

	xmlSetProp(issueDate, BAD_CAST "year", BAD_CAST year);
	xmlSetProp(issueDate, BAD_CAST "month", BAD_CAST month);
	xmlSetProp(issueDate, BAD_CAST "day", BAD_CAST day);
}

/* Set the securty classification of the instance. */
void set_security(xmlDocPtr dm, char *sec)
{
	xmlNodePtr security;
	enum issue iss;

	security = first_xpath_node(dm, NULL, "//security");

	if (!security) {
		return;
	} else if (xmlStrcmp(security->parent->name, BAD_CAST "dmStatus") == 0 || xmlStrcmp(security->parent->name, BAD_CAST "pmStatus") == 0) {
		iss = ISS_4X;
	} else {
		iss = ISS_30;
	}

	xmlSetProp(security, BAD_CAST (iss == ISS_30 ? "class" : "securityClassification"), BAD_CAST sec);
}

/* Get the originator of the master. If it has no originator
 * (e.g. pub modules do not require one) then create one in the
 * instance.
 */
xmlNodePtr find_or_create_orig(xmlDocPtr doc)
{
	xmlNodePtr orig, rpc;
	orig = first_xpath_node(doc, NULL, "//originator|//orig");
	if (!orig) {
		rpc = first_xpath_node(doc, NULL, "//responsiblePartnerCompany|//rpc");
		orig = xmlNewNode(NULL, BAD_CAST (xmlStrcmp(rpc->name, BAD_CAST "rpc") == 0 ? "orig" : "originator"));
		orig = xmlAddNextSibling(rpc, orig);
	}
	return orig;
}

/* Set the originator of the instance.
 *
 * When origspec == NULL, a default code and name are used to identify this
 * tool as the originator.
 *
 * Otherwise, origspec is a string in the form of "CODE/NAME", where CODE is
 * the NCAGE code and NAME is the enterprise name.
 */
void set_orig(xmlDocPtr doc, char *origspec)
{
	xmlNodePtr originator;
	const char *code, *name;
	enum issue iss;

	if (origspec) {
		code = strtok(origspec, "/");
		name = strtok(NULL, "");
	} else {
		code = DEFAULT_ORIG_CODE;
		name = DEFAULT_ORIG_NAME;
	}

	originator = find_or_create_orig(doc);

	if (xmlStrcmp(originator->name, BAD_CAST "orig") == 0) {
		iss = ISS_30;
	} else {
		iss = ISS_4X;
	}

	if (code) {
		if (iss == ISS_30) {
			xmlNodeSetContent(originator, BAD_CAST code);
		} else {
			xmlSetProp(originator, BAD_CAST "enterpriseCode", BAD_CAST code);
		}
	}

	if (name) {
		if (iss == ISS_30) {
			xmlSetProp(originator, BAD_CAST "origname", BAD_CAST name);
		} else {
			xmlNodePtr enterpriseName;
			enterpriseName = find_child(originator, "enterpriseName");	
			if (enterpriseName) {
				xmlNodeSetContent(enterpriseName, BAD_CAST name);
			} else {
				xmlNewChild(originator, NULL, BAD_CAST "enterpriseName", BAD_CAST name);
			}
		}
	}
}

/* Determine if the whole data module is applicable. */
bool check_wholedm_applic(xmlDocPtr dm)
{
	xmlNodePtr applic;

	applic = first_xpath_node(dm, NULL, "//dmStatus/applic|//pmStatus/applic");

	return eval_applic_stmt(applic, true);
}

/* Read applicability definitions from the <assign> elements of a
 * product instance in the specified PCT data module.\
 */
void load_applic_from_pct(const char *pctfname, const char *product)
{
	xmlDocPtr pct;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	char xpath[512];
	int i;

	pct = xmlReadFile(pctfname, NULL, PARSE_OPTS);
	ctx = xmlXPathNewContext(pct);

	/* If the product is in the form of IDENT:TYPE:VALUE, it identifies the
	 * primary key of a product instance.
	 *
	 * Otherwise, it is simply the XML ID of a product instance.
	 */
	if (strchr(product, ':')) {
		char *prod, *ident, *type, *value;

		prod  = strdup(product);

		ident = strtok(prod, ":");
		type  = strtok(NULL, "=");
		value = strtok(NULL, "");

		if (!(ident && type && value)) {
			fprintf(stderr, S_BAD_ASSIGN, product);
			exit(EXIT_BAD_APPLIC);
		}

		snprintf(xpath, 512, "//product[assign[@applicPropertyIdent='%s' and @applicPropertyType='%s' and @applicPropertyValue='%s']]/assign", ident, type, value);

		free(prod);
	} else {
		snprintf(xpath, 512, "//product[@id='%s']/assign", product);
	}

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		fprintf(stderr, S_NO_PRODUCT, product, pctfname);
		exit(EXIT_BAD_APPLIC);
	}

	for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
		char *ident, *type, *value;

		ident = (char *) xmlGetProp(obj->nodesetval->nodeTab[i],
			BAD_CAST "applicPropertyIdent");
		type  = (char *) xmlGetProp(obj->nodesetval->nodeTab[i],
			BAD_CAST "applicPropertyType");
		value = (char *) xmlGetProp(obj->nodesetval->nodeTab[i],
			BAD_CAST "applicPropertyValue");

		define_applic(ident, type, value, true);

		xmlFree(ident);
		xmlFree(type);
		xmlFree(value);
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
	xmlFreeDoc(pct);
}

/* Remove the extended identification from the instance. */
void strip_extension(xmlDocPtr doc)
{
	xmlNodePtr ext;

	ext = first_xpath_node(doc, NULL, "//identExtension");

	xmlUnlinkNode(ext);
	xmlFreeNode(ext);
}

/* General XSLT transformation with embedded stylesheet, preserving the DTD. */
void transform_doc(xmlDocPtr doc, unsigned char *xml, unsigned int len, const char **params)
{
	xmlDocPtr styledoc, res, src;
	xsltStylesheetPtr style;
	xmlNodePtr old;

	styledoc = xmlReadMemory((const char *) xml, len, NULL, NULL, 0);
	add_identity(styledoc);
	style = xsltParseStylesheetDoc(styledoc);

	src = xmlCopyDoc(doc, 1);
	res = xsltApplyStylesheet(style, src, params);
	xmlFreeDoc(src);

	old = xmlDocSetRootElement(doc, xmlCopyNode(xmlDocGetRootElement(res), 1));
	xmlFreeNode(old);

	xmlFreeDoc(res);
	xsltFreeStylesheet(style);
}

/* Flatten alts elements. */
void flatten_alts(xmlDocPtr doc)
{
	transform_doc(doc, xsl_flatten_alts_xsl, xsl_flatten_alts_xsl_len, NULL);
}

/* Removes invalid empty sections in a PM after all references have
 * been filtered out.
 */
void remove_empty_pmentries(xmlDocPtr doc)
{
	transform_doc(doc, xsl_remove_empty_pmentries_xsl, xsl_remove_empty_pmentries_xsl_len, NULL);
}

/* Insert a custom comment. */
void insert_comment(xmlDocPtr doc, const char *text, const char *path)
{
	xmlNodePtr comment, pos;

	comment = xmlNewComment(BAD_CAST text);
	pos = first_xpath_node(doc, NULL, path);

	if (!pos)
		return;

	if (pos->children) {
		xmlAddPrevSibling(pos->children, comment);
	} else {
		xmlAddChild(pos, comment);
	}
}

/* Read an applicability assign in the form of ident:type=value */
void read_applic(char *s)
{

	char *ident, *type, *value;

	if (!strchr(s, ':') || !strchr(s, '=')) {
		fprintf(stderr, S_BAD_ASSIGN, s);
		exit(EXIT_BAD_APPLIC);
	}

	ident = strtok(s, ":");
	type  = strtok(NULL, "=");
	value = strtok(NULL, "");

	define_applic(ident, type, value, false);
}

/* Set the remarks for the object */
void set_remarks(xmlDocPtr doc, const char *s)
{
	xmlNodePtr status, remarks;

	status = first_xpath_node(doc, NULL,
		"//dmStatus|"
		"//pmStatus|"
		"//commentStatus|"
		"//dmlStatus|"
		"//status|"
		"//pmstatus|"
		"//cstatus|"
		"//dml[dmlc]");

	if (!status) {
		return;
	}

	remarks = first_xpath_node(doc, status, "//remarks");

	if (remarks) {
		xmlNodePtr cur, next;
		cur = remarks->children;
		while (cur) {
			next = cur->next;
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
			cur = next;
		}
	} else {
		remarks = xmlNewChild(status, NULL, BAD_CAST "remarks", NULL);
	}

	if (xmlStrcmp(status->parent->name, BAD_CAST "identAndStatusSection") == 0) {
		xmlNewChild(remarks, NULL, BAD_CAST "simplePara", BAD_CAST s);
	} else {
		xmlNewChild(remarks, NULL, BAD_CAST "p", BAD_CAST s);
	}
}

/* Return whether objects with the given classification code should be
 * instantiated. */
bool valid_object(xmlDocPtr doc, const char *path, const char *codes)
{
	xmlNodePtr obj;
	xmlChar *code;
	bool has;
	if (!(obj = first_xpath_node(doc, NULL, path))) {
		return true;
	}
	code = xmlNodeGetContent(obj);
	has = strstr(codes, (char *) code);
	xmlFree(code);
	return has;
}

/* Remove elements that have an attribute whose value does not match a given
 * set of valid values. */
void filter_elements_by_att(xmlDocPtr doc, const char *att, const char *codes)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlChar xpath[256];

	ctx = xmlXPathNewContext(doc);

	xmlStrPrintf(xpath, 256, "//content//*[@%s]", att);
	obj = xmlXPathEvalExpression(xpath, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *val;

			val = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST att);

			if (!strstr(codes, (char *) val)) {
				xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
				xmlFreeNode(obj->nodesetval->nodeTab[i]);
				obj->nodesetval->nodeTab[i] = NULL;
			}

			xmlFree(val);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Determine whether or not to create an instance based on the object's:
 * - Applicability
 * - Skill level
 * - Security classification
 */
bool create_instance(xmlDocPtr doc, const char *skills, const char *securities)
{
	if (!check_wholedm_applic(doc)) {
		return false;
	}

	if (securities && !valid_object(doc, "//dmStatus/security/@securityClassification", securities)) {
		return false;
	}

	if (skills && !valid_object(doc, "//dmStatus/skillLevel/@skillLevelCode", skills)) {
		return false;
	}

	return true;
}

/* Set the skill level code of the instance. */
void set_skill(xmlDocPtr doc, const char *skill)
{
	xmlNodePtr skill_level;

	if (xmlStrcmp(xmlDocGetRootElement(doc)->name, BAD_CAST "dmodule") != 0) {
		return;
	}

	skill_level = first_xpath_node(doc, NULL, "//skillLevel");

	if (!skill_level) {
		xmlNodePtr node;
		node = first_xpath_node(doc, NULL,
			"("
			"//qualityAssurance|"
			"//systemBreakdownCode|"
			"//functionalItemCode|"
			"//dmStatus/functionalItemRef"
			")[last()]");
		skill_level = xmlNewNode(NULL, BAD_CAST "skillLevel");
		xmlAddNextSibling(node, skill_level);
	}

	xmlSetProp(skill_level, BAD_CAST "skillLevelCode", BAD_CAST skill);
}

/* Determine if the file is a data module. */
bool is_dm(const char *name)
{
	return strncmp(name, "DMC-", 4) == 0 && strncasecmp(name + strlen(name) - 4, ".XML", 4) == 0;
}

/* Find a data module filename in the current directory based on the dmRefIdent
 * element. */
bool find_dmod_fname(char *dst, xmlNodePtr dmRefIdent)
{
	DIR *dir;
	struct dirent *cur;
	int len;
	bool found = false;
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
	char *learn_code;
	char *learn_event_code;
	char code[64];
	xmlNodePtr dmCode, issueInfo, language;

	dmCode = first_xpath_node(NULL, dmRefIdent, "dmCode|avee");
	issueInfo = first_xpath_node(NULL, dmRefIdent, "issueInfo|issno");
	language = first_xpath_node(NULL, dmRefIdent, "language");

	model_ident_code     = (char *) first_xpath_value(NULL, dmCode, "modelic|@modelIdentCode");
	system_diff_code     = (char *) first_xpath_value(NULL, dmCode, "sdc|@systemDiffCode");
	system_code          = (char *) first_xpath_value(NULL, dmCode, "chapnum|@systemCode");
	sub_system_code      = (char *) first_xpath_value(NULL, dmCode, "section|@subSystemCode");
	sub_sub_system_code  = (char *) first_xpath_value(NULL, dmCode, "subsect|@subSubSystemCode");
	assy_code            = (char *) first_xpath_value(NULL, dmCode, "subject|@assyCode");
	disassy_code         = (char *) first_xpath_value(NULL, dmCode, "discode|@disassyCode");
	disassy_code_variant = (char *) first_xpath_value(NULL, dmCode, "discodev|@disassyCodeVariant");
	info_code            = (char *) first_xpath_value(NULL, dmCode, "incode|@infoCode");
	info_code_variant    = (char *) first_xpath_value(NULL, dmCode, "incodev|@infoCodeVariant");
	item_location_code   = (char *) first_xpath_value(NULL, dmCode, "itemloc|@itemLocationCode");
	learn_code           = (char *) first_xpath_value(NULL, dmCode, "@learnCode");
	learn_event_code     = (char *) first_xpath_value(NULL, dmCode, "@learnEventCode");

	snprintf(code, 64, "DMC-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
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

	if (learn_code) {
		char learn[8];
		snprintf(learn, 8, "-%s%s", learn_code, learn_event_code);
		strcat(code, learn);
	}

	xmlFree(learn_code);
	xmlFree(learn_event_code);

	if (issueInfo) {
		char *issue_number;
		char *in_work;
		char iss[8];

		issue_number = (char *) first_xpath_value(NULL, issueInfo, "@issno|@issueNumber");
		in_work      = (char *) first_xpath_value(NULL, issueInfo, "@inwork|@inWork");

		snprintf(iss, 8, "_%s-%s", issue_number, in_work ? in_work : "00");
		strcat(code, iss);

		xmlFree(issue_number);
		xmlFree(in_work);
	}

	if (language) {
		char *language_iso_code;
		char *country_iso_code;
		char lang[8];

		language_iso_code = (char *) first_xpath_value(NULL, language, "@language|@languageIsoCode");
		country_iso_code  = (char *) first_xpath_value(NULL, language, "@country|@countryIsoCode");

		snprintf(lang, 8, "_%s-%s", language_iso_code, country_iso_code);
		strcat(code, lang);

		xmlFree(language_iso_code);
		xmlFree(country_iso_code);
	}

	len = strlen(code);

	dir = opendir(".");

	while ((cur = readdir(dir))) {
		if (strncmp(cur->d_name, code, len) == 0) {
			if (is_dm(cur->d_name) && (!found || strcmp(cur->d_name, dst) > 0)) {
				strcpy(dst, cur->d_name);
			}

			found = true;
		}
	}

	closedir(dir);

	if (!found) {
		fprintf(stderr, S_MISSING_REF_DM, code);
	}

	return found;
}

/* Find the filename of a referenced ACT data module. */
bool find_act_fname(char *dst, xmlDocPtr doc)
{
	xmlNodePtr actref;
	actref = first_xpath_node(doc, NULL, "//applicCrossRefTableRef/dmRef/dmRefIdent|//actref/refdm");
	return actref && find_dmod_fname(dst, actref);
}

/* Find the filename of a referenced PCT data module via the ACT. */
bool find_pct_fname(char *dst, xmlDocPtr doc)
{
	char act_fname[PATH_MAX];
	xmlDocPtr act;
	xmlNodePtr pctref;
	bool found;

	if (!find_act_fname(act_fname, doc)) {
		return false;
	}

	if (!(act = xmlReadFile(act_fname, NULL, PARSE_OPTS))) {
		return false;
	}

	pctref = first_xpath_node(act, NULL, "//productCrossRefTableRef/dmRef/dmRefIdent|//pctref/refdm");
	found = pctref && find_dmod_fname(dst, pctref);
	xmlFreeDoc(act);

	return found;
}

/* Unset all applicability assigned by a PCT. */
void clear_pct_applic(void)
{
	xmlNodePtr cur;
	cur = applicability->children;
	while (cur) {
		xmlNodePtr next;
		next = cur->next;
		if (xmlHasProp(cur, BAD_CAST "fromPct")) {
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
			--napplics;
		}
		cur = next;
	}
}

/* Print a usage message */
void show_help(void)
{
	printf("%.*s", help_msg_len, help_msg);
}

/* Print version information */
void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s, libxslt %s and libexslt %s\n",
		xmlParserVersion, xsltEngineVersion, exsltLibraryVersion);
}

int main(int argc, char **argv)
{
	xmlDocPtr doc;
	xmlNodePtr root;
	xmlNodePtr referencedApplicGroup;

	int i;
	int c;

	char code[256] = "";
	char out[PATH_MAX] = "-";
	bool clean = false;
	bool simpl = false;
	char tech[256] = "";
	char info[256] = "";
	bool autoname = false;
	char dir[PATH_MAX] = "";
	bool new_applic = false;
	char new_display_text[256] = "";
	char comment_text[256] = "";
	char comment_path[256] = "/";
	char extension[256] = "";
	char language[256] = "";
	bool add_source_ident = true;
	bool force_overwrite = false;
	bool use_stdin = false;
	char issinfo[8] = "";
	char secu[4] = "";
	bool wholedm = false;
	bool no_issue = false;
	char pctfname[PATH_MAX] = "";
	char product[64] = "";
	bool load_pct_per_dm = false;
	bool dmlist = false;
	FILE *list = NULL;
	char issdate[16] = "";
	char issdate_year[5];
	char issdate_month[3];
	char issdate_day[3];
	bool stripext = false;
	bool verbose = false;
	bool setorig = false;
	char *origspec = NULL;
	bool flat_alts = false;
	char *remarks = NULL;
	char *skill_codes = NULL;
	char *sec_classes = NULL;
	char *skill = NULL;
	bool combine_applic = true;

	xmlNodePtr cirs, cir;

	const char *sopts = "AaC:c:Ee:FfG:gh?I:i:K:k:Ll:m:Nn:O:o:P:p:R:r:Ss:t:U:u:vWwX:x:Y:y";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	exsltRegisterAll();

	opterr = 1;

	cirs = xmlNewNode(NULL, BAD_CAST "cirs");

	applicability = xmlNewNode(NULL, BAD_CAST "applic");

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return EXIT_SUCCESS;
				}
				break;
			case 'a': clean = true; break;
			case 'A': simpl = true; break;
			case 'c': strncpy(code, optarg, 255); break;
			case 'C': strncpy(comment_text, optarg, 255); break;
			case 'E': stripext = true; break;
			case 'e': strncpy(extension, optarg, 255); break;
			case 'F': flat_alts = true; break;
			case 'f': force_overwrite = true; break;
			case 'g': setorig = true; break;
			case 'G': setorig = true; origspec = strdup(optarg); break;
			case 'i': strncpy(info, optarg, 255); break;
			case 'I': strncpy(issdate, optarg, 15); break;
			case 'K': skill_codes = strdup(optarg); break;
			case 'k': skill = strdup(optarg); break;
			case 'L': dmlist = true; break;
			case 'l': strncpy(language, optarg, 255); break;
			case 'm': remarks = strdup(optarg); break;
			case 'N': no_issue = true; break;
			case 'n': strncpy(issinfo, optarg, 6); break;
			case 'O': autoname = true; strncpy(dir, optarg, PATH_MAX); break;
			case 'o': strncpy(out, optarg, PATH_MAX - 1); break;
			case 'P': strncpy(pctfname, optarg, PATH_MAX - 1); break;
			case 'p': strncpy(product, optarg, 63); break;
			case 'R': xmlNewChild(cirs, NULL, BAD_CAST "cir", BAD_CAST optarg); break;
			case 'r': xmlSetProp(cirs->last, BAD_CAST "xsl", BAD_CAST optarg); break;
			case 'S': add_source_ident = false; break;
			case 's': read_applic(optarg); break;
			case 't': strncpy(tech, optarg, 255); break;
			case 'U': sec_classes = strdup(optarg); break;
			case 'u': strncpy(secu, optarg, 2); break;
			case 'v': verbose = true; break;
			case 'W': new_applic = true; combine_applic = false; break;
			case 'w': wholedm = true; break;
			case 'x': dump_cir_xsl(optarg); exit(0);
			case 'X': strncpy(comment_path, optarg, 255); break;
			case 'y': new_applic = true; break;
			case 'Y': new_applic = true; strncpy(new_display_text, optarg, 255); break;
			case 'h':
			case '?':
				show_help();
				exit(EXIT_SUCCESS);
		}
	}

	if (optind >= argc) {
		use_stdin = true;
		list = stdin;
	}

	if (autoname && access(dir, F_OK) == -1) {
		int err;

		#ifdef _WIN32
			err = mkdir(dir);
		#else
			err = mkdir(dir, S_IRWXU);
		#endif

		if (err) {
			fprintf(stderr, S_MKDIR_FAILED, dir);
			exit(EXIT_BAD_ARG);
		}
	}

	if (strcmp(product, "") != 0) {
		/* If a PCT filename is specified with -P, use that for all data
		 * modules and ignore their individual ACT->PCT refs. */
		if (strcmp(pctfname, "") != 0) {
			if (access(pctfname, F_OK) == -1) {
				fprintf(stderr, S_MISSING_PCT, pctfname);
				exit(EXIT_MISSING_FILE);
			} else {
				load_applic_from_pct(pctfname, product);
			}
		/* Otherwise the PCT must be loaded separately for each data
		 * module, since they may reference different ones. */
		} else {
			load_pct_per_dm = true;
		}
	}

	/* Determine the issue date from the -I option. */
	if (strcmp(issdate, "-") == 0) {
		time_t now;
		struct tm *local;

		time(&now);
		local = localtime(&now);

		sprintf(issdate_year, "%d", local->tm_year + 1900);
		sprintf(issdate_month, "%.2d", local->tm_mon + 1);
		sprintf(issdate_day, "%.2d", local->tm_mday);
	} else if (strcmp(issdate, "") != 0) {
		if (sscanf(issdate, "%4s-%2s-%2s", issdate_year, issdate_month, issdate_day) != 3) {
			fprintf(stderr, S_BAD_DATE, issdate);
			exit(EXIT_BAD_DATE);
		}
	}

	i = optind;

	while (1) {
		bool ispm;
		char src[PATH_MAX] = "";

		if (dmlist) {
			if (!list && !(list = fopen(argv[i++], "r"))) {
				fprintf(stderr, S_MISSING_LIST, argv[i - 1]);
				exit(EXIT_MISSING_FILE);
			}

			if (!fgets(src, PATH_MAX - 1, list)) {
				fclose(list);
				list = NULL;

				if (i < argc) {
					continue;
				} else {
					break;
				}
			}

			strtok(src, "\t\r\n");
		} else if (i < argc) {
			strcpy(src, argv[i++]);
		} else if (use_stdin) {
			strcpy(src, "-");
		}

		if (!use_stdin && access(src, F_OK) == -1) {
			fprintf(stderr, S_MISSING_OBJECT, src);
			exit(EXIT_MISSING_FILE);
		}

		doc = xmlReadFile(src, NULL, PARSE_OPTS | XML_PARSE_NOWARNING | XML_PARSE_NOERROR);

		if (doc) {
			root = xmlDocGetRootElement(doc);
			ispm = xmlStrcmp(root->name, BAD_CAST "pm") == 0;

			/* Load the applic assigns from the PCT data module referenced
			 * by the ACT data module referenced by this data module. */
			if (load_pct_per_dm) {
				char pct_fname[PATH_MAX];

				if (find_pct_fname(pct_fname, doc)) {
					load_applic_from_pct(pct_fname, product);
				}
			}

			if (!wholedm || create_instance(doc, skill_codes, sec_classes)) {
				if (add_source_ident) {
					add_source(doc);
				}

				for (cir = cirs->children; cir; cir = cir->next) {
					char *cirdocfname = (char *) xmlNodeGetContent(cir);
					char *cirxsl = (char *) xmlGetProp(cir, BAD_CAST "xsl");

					if (access(cirdocfname, F_OK) == -1) {
						fprintf(stderr, S_MISSING_CIR, cirdocfname);
						continue;
					}

					if (ispm) {
						undepend_cir(doc, cirdocfname, false, cirxsl);
					} else {
						undepend_cir(doc, cirdocfname, add_source_ident, cirxsl);
					}

					xmlFree(cirdocfname);
					xmlFree(cirxsl);
				}

				referencedApplicGroup = first_xpath_node(doc, NULL, "//referencedApplicGroup|//inlineapplics");

				if (referencedApplicGroup) {
					strip_applic(referencedApplicGroup, root);

					if (clean || simpl) {
						clean_applic_stmts(referencedApplicGroup);

						if (xmlChildElementCount(referencedApplicGroup) == 0) {
							xmlUnlinkNode(referencedApplicGroup);
							xmlFreeNode(referencedApplicGroup);
							referencedApplicGroup = NULL;
						}

						clean_applic(referencedApplicGroup, root);
					}

					if (simpl) {
						simpl_applic_clean(referencedApplicGroup);
					}
				}

				/* Remove elements whose securityClassification is not
				 * in the given list. */
				if (sec_classes) {
					filter_elements_by_att(doc, "securityClassification", sec_classes);
				}
				/* Remove elements whose skillLevelCode is not in the
				 * given list. */
				if (skill_codes) {
					filter_elements_by_att(doc, "skillLevelCode", skill_codes);
				}

				if (strcmp(extension, "") != 0) {
					set_extd(doc, extension);
				}

				if (stripext) {
					strip_extension(doc);
				}

				if (strcmp(code, "") != 0) {
					set_code(doc, code);
				}

				set_title(doc, tech, info);

				if (strcmp(language, "") != 0) {
					set_lang(doc, language);
				}

				if (new_applic && napplics > 0) {
					/* Simplify the whole object applic before
					 * adding the user-defined applicability, to
					 * remove duplicate information.
					 *
					 * If overwriting the applic instead of merging
					 * it, there's no need to do this.
					 */
					if (combine_applic) {
						simpl_whole_applic(doc);
					}

					set_applic(doc, new_display_text, combine_applic);
				}

				if (strcmp(issinfo, "") != 0) {
					set_issue(doc, issinfo);
				}

				if (strcmp(issdate, "") != 0) {
					set_issue_date(doc, issdate_year, issdate_month, issdate_day);
				}

				if (strcmp(secu, "") != 0) {
					set_security(doc, secu);
				}

				if (setorig) {
					set_orig(doc, origspec);
				}

				if (skill) {
					set_skill(doc, skill);
				}

				if (remarks) {
					set_remarks(doc, remarks);
				}

				if (strcmp(comment_text, "") != 0) {
					insert_comment(doc, comment_text, comment_path);
				}

				if (ispm) {
					remove_empty_pmentries(doc);
				}

				if (flat_alts) {
					flatten_alts(doc);
				}

				if (autoname && !auto_name(out, src, doc, dir, no_issue)) {
					fprintf(stderr, S_BAD_TYPE);
					exit(EXIT_BAD_XML);
				}

				if (access(out, F_OK) == 0 && !force_overwrite) {
					fprintf(stderr, S_FILE_EXISTS, out);
				} else {
					xmlSaveFile(out, doc);

					if (verbose) {
						puts(out);
					}
				}
			}

			/* The ACT/PCT may be different for the next DM, so these
			 * assigns must be cleared. Those directly set with -s will
			 * carry over. */
			if (load_pct_per_dm) {
				clear_pct_applic();
			}

			xmlFreeDoc(doc);
		} else if (autoname) { /* Copy the non-XML object to the directory. */
			char *base;

			base = basename(src);
			snprintf(out, PATH_MAX, "%s/%s", dir, base);

			if (access(out, F_OK) == 0 && !force_overwrite) {
				fprintf(stderr, S_FILE_EXISTS, out);
			} else {
				copy(src, out);

				if (verbose) {
					puts(out);
				}
			}
		} else {
			fprintf(stderr, S_BAD_XML, use_stdin ? "stdin" : src);
			exit(EXIT_BAD_XML);
		}

		if (!dmlist && (use_stdin || i >= argc)) {
			break;
		}
	}

	free(origspec);
	free(remarks);
	free(skill);
	free(skill_codes);
	free(sec_classes);
	xmlFreeNode(cirs);
	xmlFreeNode(applicability);
	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
