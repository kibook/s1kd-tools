#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libexslt/exslt.h>
#include "s1kd_tools.h"
#include "xsl.h"

#define PROG_NAME "s1kd-instance"
#define VERSION "12.2.1"

/* Prefixes before messages printed to console */
#define ERR_PREFIX PROG_NAME ": ERROR: "
#define WRN_PREFIX PROG_NAME ": WARNING: "
#define INF_PREFIX PROG_NAME ": INFO: "

/* Error codes */
#define EXIT_MISSING_ARGS 1	/* Option or parameter missing */
#define EXIT_MISSING_FILE 2	/* File does not exist */
#define EXIT_MISSING_SOURCE 3	/* Source object could not be found */
#define EXIT_BAD_APPLIC 4	/* Malformed applic definitions */
#define EXIT_BAD_XML 6		/* Invalid XML/S1000D */
#define EXIT_BAD_ARG 7		/* Malformed argument */
#define EXIT_BAD_DATE 8		/* Malformed issue date */
#define EXIT_MAX_OBJECTS 9      /* Out of memory */

/* Error messages */
#define S_MISSING_OBJECT ERR_PREFIX "Could not read source object: %s\n"
#define S_MISSING_LIST ERR_PREFIX "Could not read list file: %s\n"
#define S_BAD_TYPE ERR_PREFIX "Cannot automatically name unsupported object types.\n"
#define S_BAD_XML ERR_PREFIX "%s does not contain valid XML. If it is a list, specify the -L option.\n"
#define S_MISSING_ANDOR ERR_PREFIX "Evaluate has no operator.\n"
#define S_BAD_CODE ERR_PREFIX "Bad %s code: %s.\n"
#define S_INVALID_CIR ERR_PREFIX "%s is not a valid CIR data module.\n"
#define S_INVALID_ISSFMT ERR_PREFIX "Invalid format for issue/in-work number.\n"
#define S_BAD_DATE ERR_PREFIX "Bad issue date: %s\n"
#define S_BAD_ASSIGN ERR_PREFIX "Malformed applicability definition: \"%s\". Definitions must be in the form of \"<ident>:<type>=<value>\".\n"
#define S_MISSING_ACT ERR_PREFIX "Could not read ACT %s\n"
#define S_MISSING_CCT ERR_PREFIX "Could not read CCT %s\n"
#define S_MISSING_PCT ERR_PREFIX "Could not read PCT %s\n"
#define S_MKDIR_FAILED ERR_PREFIX "Could not create directory %s\n"
#define S_MISSING_SOURCE ERR_PREFIX "Could not find source object for instance %s\n"
#define S_NOT_DIR ERR_PREFIX "%s is not a directory.\n"
#define E_MAX_OBJECTS ERR_PREFIX "Out of memory\n"

/* Warning messages */
#define S_FILE_EXISTS WRN_PREFIX "%s already exists. Use -f to overwrite.\n"
#define S_NO_PRODUCT WRN_PREFIX "No product matching '%s' in PCT '%s'.\n"
#define S_NO_XSLT WRN_PREFIX "No built-in XSLT for CIR type: %s\n"
#define S_MISSING_REF_DM WRN_PREFIX "Could not read referenced object: %s\n"
#define S_MISSING_CIR WRN_PREFIX "Could not find CIR %s.\n"
#define S_RESOLVE_CONTAINER WRN_PREFIX "Could not resolve container %s\n"
#define S_NO_CT WRN_PREFIX "%s is a %s, but no %s was found.\n"
#define S_NO_CT_PROP WRN_PREFIX "Could not find definition of %s %s\n"

/* Info messages */
#define I_UPDATE_INST INF_PREFIX "Updating instance %s from source %s...\n"
#define I_CUSTOMIZE INF_PREFIX "Customizing %s...\n"
#define I_CUSTOMIZE_DIR INF_PREFIX "Customizing %s -> %s ...\n"
#define I_COPY INF_PREFIX "Copying %s -> %s ...\n"
#define I_FIND_CIR INF_PREFIX "Searching for CIRs in \"%s\"...\n"
#define I_FIND_CIR_FOUND INF_PREFIX "Found CIR %s...\n"
#define I_FIND_CIR_ADD INF_PREFIX "Added CIR %s\n"
#define I_NON_APPLIC INF_PREFIX "Ignoring non-applicable object: %s\n"

/* When using the -g option, these are set as the values for the
 * originator.
 */
#define DEFAULT_ORIG_CODE "S1KDI"
#define DEFAULT_ORIG_NAME "s1kd-instance tool"

/* Text of the default RFU added when a "new" master produces non-new
 * instances. */
#define DEFAULT_RFU BAD_CAST "New master"

/* Search for ACT/PCT recursively. */
static bool recursive_search = false;

/* Directory to start searching for ACT/PCT in. */
static char *search_dir;

/* Tag non-applicable elements instead of deleting them. */
static bool tag_non_applic = false;

/* Remove display text from annotations which are modified in -A mode. */
static bool clean_disp_text = false;

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

/* Assume objects were created with -N. */
static bool no_issue = false;

/* Verbosity level */
static enum verbosity { QUIET, NORMAL, VERBOSE, DEBUG } verbosity = NORMAL;

/* Method for listing properties. */
enum listprops { STANDALONE, APPLIC, ALL };

/* Determine whether an applicability definition may be modified.
 * User-definitions may only be modified by other user-definitions.
 * User-definitions may modify non-user-definitions.
 */
static bool allow_def_modify(bool userdefined, const xmlChar *attr)
{
	return userdefined || xmlStrcmp(attr, BAD_CAST "false") == 0;
}

/* Define a value for a product attribute or condition. */
static void define_applic(xmlNodePtr defs, int *napplics, const xmlChar *ident, const xmlChar *type, const xmlChar *value, bool perdm, bool userdefined)
{
	xmlNodePtr assert = NULL;
	xmlNodePtr cur;

	if (!(ident && type && value)) {
		return;
	}

	/* Check if an assert has already been created for this property. */
	for (cur = defs->children; cur; cur = cur->next) {
		xmlChar *cur_ident = xmlGetProp(cur, BAD_CAST "applicPropertyIdent");
		xmlChar *cur_type  = xmlGetProp(cur, BAD_CAST "applicPropertyType");

		if (xmlStrcmp(cur_ident, ident) == 0 && xmlStrcmp(cur_type, type) == 0) {
			assert = cur;
		}

		xmlFree(cur_ident);
		xmlFree(cur_type);
	}

	/* If no assert exists, add a new one. */
	if (!assert) {
		assert = xmlNewChild(defs, NULL, BAD_CAST "assert", NULL);
		xmlSetProp(assert, BAD_CAST "applicPropertyIdent", ident);
		xmlSetProp(assert, BAD_CAST "applicPropertyType",  type);
		xmlSetProp(assert, BAD_CAST "applicPropertyValues", value);
		xmlSetProp(assert, BAD_CAST "userDefined", BAD_CAST (userdefined ? "true" : "false"));

		if (userdefined) {
			++(*napplics);
		}
	/* Or, if an assert already exists... */
	} else {
		xmlChar *user_defined_attr;

		user_defined_attr = xmlGetProp(assert, BAD_CAST "userDefined");

		/* Check for duplicate value in a single-assert. */
		if (xmlHasProp(assert, BAD_CAST "applicPropertyValues")) {
			xmlChar *first_value;

			first_value = xmlGetProp(assert, BAD_CAST "applicPropertyValues");

			/* If the value is not a duplicate, and the assertion
			 * may be modified, convert to a multi-assert and add
			 * the original and new values. */
			if (xmlStrcmp(first_value, BAD_CAST value) != 0 && allow_def_modify(userdefined, user_defined_attr)) {
				xmlNewChild(assert, NULL, BAD_CAST "value", first_value);
				xmlNewChild(assert, NULL, BAD_CAST "value", value);
				xmlUnsetProp(assert, BAD_CAST "applicPropertyValues");
			}

			xmlFree(first_value);
		/* Check for duplicate value in a multi-assert. */
		} else {
			bool dup = false;

			for (cur = assert->children; cur && !dup; cur = cur->next) {
				xmlChar *cur_value;
				cur_value = xmlNodeGetContent(cur);
				dup = xmlStrcmp(cur_value, value) == 0;
				xmlFree(cur_value);
			}

			/* If the value is not a duplicate, and the assertion
			 * may be modified, add the new value to the
			 * multi-assert. */
			if (!dup && allow_def_modify(userdefined, user_defined_attr)) {
				xmlNewChild(assert, NULL, BAD_CAST "value", value);
			}
		}

		xmlFree(user_defined_attr);
	}

	/* Tag asserts that may only be true for individual DMs. */
	if (perdm) {
		xmlSetProp(assert, BAD_CAST "perDm", BAD_CAST "true");
	}
}

/* Find the first child element with a given name */
static xmlNodePtr find_child(xmlNodePtr parent, const char *name)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, name) == 0) {
			return cur;
		}
	}

	return NULL;
}

static xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const xmlChar *path)
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

static xmlChar *first_xpath_value(xmlDocPtr doc, xmlNodePtr node, const xmlChar *path)
{
	return xmlNodeGetContent(first_xpath_node(doc, node, path));
}

/* Copy strings related to uniquely identifying a CSDB object. The strings are
 * dynamically allocated so they must be freed using free_ident. */
#define IDENT_XPATH BAD_CAST \
	"//dmIdent|//dmaddres|" \
	"//pmIdent|//pmaddres|" \
	"//dmlIdent|//dml[dmlc]|" \
	"//commentIdent|//cstatus|" \
	"//ddnIdent|//ddn|" \
	"//imfIdent|" \
	"//updateIdent"
#define EXTENSION_XPATH BAD_CAST \
	"//dmIdent/identExtension|//dmaddres/dmcextension|" \
	"//pmIdent/identExtension|" \
	"//updateIdent/identExtension"
#define CODE_XPATH BAD_CAST \
	"//dmIdent/dmCode|//dmaddres/dmc/avee|" \
	"//pmIdent/pmCode|//pmaddres/pmc|" \
	"//dmlIdent/dmlCode|//dml/dmlc|" \
	"//commentIdent/commentCode|//cstatus/ccode|" \
	"//ddnIdent/ddnCode|//ddn/ddnc|" \
	"//imfIdent/imfCode|" \
	"//updateIdent/updateCode"
#define LANGUAGE_XPATH BAD_CAST \
	"//dmIdent/language|//dmaddres/language|" \
	"//pmIdent/language|//pmaddres/language|" \
	"//commentIdent/language|//cstatus/language|" \
	"//updateIdent/language"
#define ISSUE_INFO_XPATH BAD_CAST \
	"//dmIdent/issueInfo|//dmaddres/issno|" \
	"//pmIdent/issueInfo|//pmaddres/issno|" \
	"//dmlIdent/issueInfo|//dml/issno|" \
	"//imfIdent/issueInfo|" \
	"//updateIdent/issueInfo"
static bool init_ident(struct ident *ident, xmlDocPtr doc)
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

static void free_ident(struct ident *ident)
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
 * which applic statements and references are unambiguously true (they do not
 * rely on any undefined attributes or conditions) and therefore may be removed.
 *
 * An undefined attribute/condition is a product attribute (ACT) or
 * condition (CCT) for which a value is asserted in the applic statement but
 * for which no value was supplied by the user.
 */
static bool eval_applic(xmlNodePtr defs, xmlNodePtr node, bool assume);

/* Evaluate multiple values for a property */
static bool eval_multi(xmlNodePtr multi, const char *ident, const char *type, const char *value)
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
static bool is_applic(xmlNodePtr defs, const char *ident, const char *type, const char *value, bool assume)
{
	xmlNodePtr cur;

	bool result = assume;

	if (!(ident && type && value)) {
		return assume;
	}

	for (cur = defs->children; cur; cur = cur->next) {
		char *cur_ident = (char *) xmlGetProp(cur, BAD_CAST "applicPropertyIdent");
		char *cur_type  = (char *) xmlGetProp(cur, BAD_CAST "applicPropertyType");
		char *cur_value = (char *) xmlGetProp(cur, BAD_CAST "applicPropertyValues");

		bool match = strcmp(cur_ident, ident) == 0 && strcmp(cur_type, type) == 0;

		if (match) {
			if (cur_value) {
				result = is_in_set(cur_value, value);
			} else if (assume) {
				result = eval_multi(cur, ident, type, value);
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
static bool eval_assert(xmlNodePtr defs, xmlNodePtr assert, bool assume)
{
	xmlNodePtr ident_attr, type_attr, values_attr;
	char *ident, *type, *values;

	bool ret;

	ident_attr  = first_xpath_node(NULL, assert, BAD_CAST "@applicPropertyIdent|@actidref");
	type_attr   = first_xpath_node(NULL, assert, BAD_CAST "@applicPropertyType|@actreftype");
	values_attr = first_xpath_node(NULL, assert, BAD_CAST "@applicPropertyValues|@actvalues");

	ident  = (char *) xmlNodeGetContent(ident_attr);
	type   = (char *) xmlNodeGetContent(type_attr);
	values = (char *) xmlNodeGetContent(values_attr);

	ret = is_applic(defs, ident, type, values, assume);

	xmlFree(ident);
	xmlFree(type);
	xmlFree(values);

	return ret;
}

enum operator { AND, OR };

/* Test whether an <evaluate> element is applicable. */
static bool eval_evaluate(xmlNodePtr defs, xmlNodePtr evaluate, bool assume)
{
	xmlChar *andOr;
	enum operator op;
	bool ret = assume;
	xmlNodePtr cur;

	andOr = first_xpath_value(NULL, evaluate, BAD_CAST "@andOr|@operator");

	if (!andOr) {
		if (verbosity > QUIET) {
			fprintf(stderr, S_MISSING_ANDOR);
		}
		exit(EXIT_BAD_XML);
	}

	if (xmlStrcmp(andOr, BAD_CAST "and") == 0) {
		op = AND;
	} else {
		op = OR;
	}

	xmlFree(andOr);

	for (cur = evaluate->children; cur; cur = cur->next) {
		if (xmlStrcmp(cur->name, BAD_CAST "assert") == 0 || xmlStrcmp(cur->name, BAD_CAST "evaluate") == 0) {
			ret = eval_applic(defs, cur, assume);

			if ((op == AND && !ret) || (op == OR && ret)) {
				break;
			}
		}
	}

	return ret;
}

/* Generic test for either <assert> or <evaluate> */
static bool eval_applic(xmlNodePtr defs, xmlNodePtr node, bool assume)
{
	if (strcmp((char *) node->name, "assert") == 0) {
		return eval_assert(defs, node, assume);
	} else if (strcmp((char *) node->name, "evaluate") == 0) {
		return eval_evaluate(defs, node, assume);
	}

	return false;
}

/* Tests whether an <applic> element is true. */
static bool eval_applic_stmt(xmlNodePtr defs, xmlNodePtr applic, bool assume)
{
	xmlNodePtr stmt;

	stmt = find_child(applic, "assert");

	if (!stmt) {
		stmt = find_child(applic, "evaluate");
	}

	if (!stmt) {
		return assume;
	}

	return eval_applic(defs, stmt, assume);
}

/* Search recursively for a descendant element with the given id */
static xmlNodePtr get_element_by_id(xmlNodePtr root, const char *id)
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
static void strip_applic(xmlNodePtr defs, xmlNodePtr referencedApplicGroup, xmlNodePtr node)
{
	xmlNodePtr cur, next;
	xmlNodePtr attr;

	attr = first_xpath_node(NULL, node, BAD_CAST "@applicRefId|@refapplic");

	if (attr) {
		xmlChar *applicRefId;
		xmlNodePtr applic;

		applicRefId = xmlNodeGetContent(attr);
		applic = get_element_by_id(referencedApplicGroup, (char *) applicRefId);
		xmlFree(applicRefId);

		if (applic && !eval_applic_stmt(defs, applic, true)) {
			if (tag_non_applic) {
				add_first_child(node, xmlNewPI(BAD_CAST "notApplicable", NULL));
			} else {
				xmlUnlinkNode(node);
				xmlFreeNode(node);
			}
			return;
		}
	}

	cur = node->children;
	while (cur) {
		next = cur->next;
		strip_applic(defs, referencedApplicGroup, cur);
		cur = next;
	}
}

/* Remove unambiguously true or false applic statements. */
static void clean_applic_stmts(xmlNodePtr defs, xmlNodePtr referencedApplicGroup, bool remtrue)
{
	xmlNodePtr cur;

	cur = referencedApplicGroup->children;

	while (cur) {
		xmlNodePtr next = cur->next;

		if (cur->type == XML_ELEMENT_NODE && ((remtrue && eval_applic_stmt(defs, cur, false)) || !eval_applic_stmt(defs, cur, true))) {
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
		}

		cur = next;
	}
}

/* Remove applic references on content where the applic statement was removed by clean_applic_stmts. */
static void clean_applic(xmlNodePtr referencedApplicGroup, xmlNodePtr node)
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

/* Remove unused applicability annotations. */
static xmlNodePtr rem_unused_annotations(xmlDocPtr doc, xmlNodePtr referencedApplicGroup)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(doc);
	xmlXPathSetContextNode(referencedApplicGroup, ctx);

	if (xmlStrcmp(referencedApplicGroup->name, BAD_CAST "referencedApplicGroup") == 0) {
		obj = xmlXPathEval(BAD_CAST "applic[not(@id=//@applicRefId)]", ctx);
	} else {
		obj = xmlXPathEval(BAD_CAST "applic[not(@id=//@refapplic)]", ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
			xmlFreeNode(obj->nodesetval->nodeTab[i]);
			obj->nodesetval->nodeTab[i] = NULL;
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	if (xmlChildElementCount(referencedApplicGroup) == 0) {
		xmlUnlinkNode(referencedApplicGroup);
		xmlFreeNode(referencedApplicGroup);
		return NULL;
	}

	return referencedApplicGroup;
}

/* Remove display text from the containing annotation. */
static void rem_disp_text(xmlNodePtr node)
{
	xmlNodePtr disptext;

	disptext = first_xpath_node(NULL, node, BAD_CAST "ancestor::applic/*[self::displayText or self::displaytext]");

	if (disptext) {
		xmlUnlinkNode(disptext);
		xmlFreeNode(disptext);
	}
}

/* Remove applic statements or parts of applic statements where all assertions
 * are unambiguously true or false.
 *
 * Returns true if the whole annotation is removed, or false if only parts of
 * it are removed.
 */
static bool simpl_applic(xmlNodePtr defs, xmlNodePtr node, bool remtrue)
{
	xmlNodePtr cur, next;

	if (xmlStrcmp(node->name, BAD_CAST "applic") == 0) {
		if ((remtrue && eval_applic_stmt(defs, node, false)) || !eval_applic_stmt(defs, node, true)) {
			xmlUnlinkNode(node);
			xmlFreeNode(node);
			return true;
		}
	} else if (xmlStrcmp(node->name, BAD_CAST "evaluate") == 0) {
		if ((remtrue && eval_applic(defs, node, false)) || !eval_applic(defs, node, true)) {
			if (clean_disp_text) {
				rem_disp_text(node);
			}

			xmlUnlinkNode(node);
			xmlFreeNode(node);

			return false;
		}
	} else if (xmlStrcmp(node->name, BAD_CAST "assert") == 0) {
		if ((remtrue && eval_assert(defs, node, false)) || !eval_assert(defs, node, true)) {
			if (clean_disp_text) {
				rem_disp_text(node);
			}

			xmlUnlinkNode(node);
			xmlFreeNode(node);

			return false;
		}
	}

	cur = node->children;
	while (cur) {
		next = cur->next;
		simpl_applic(defs, cur, remtrue);
		cur = next;
	}

	return false;
}

/* If an <evaluate> contains only one (or no) child elements, remove it. */
static void simpl_evaluate(xmlNodePtr evaluate)
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
static void simpl_applic_evals(xmlNodePtr node)
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
static xmlNodePtr simpl_applic_clean(xmlNodePtr defs, xmlNodePtr referencedApplicGroup, bool remtrue)
{
	bool has_applic = false;
	xmlNodePtr cur;

	if (!referencedApplicGroup) {
		return NULL;
	}

	simpl_applic(defs, referencedApplicGroup, remtrue);
	simpl_applic_evals(referencedApplicGroup);

	for (cur = referencedApplicGroup->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, "applic") == 0) {
			has_applic = true;
		}
	}

	if (!has_applic) {
		xmlUnlinkNode(referencedApplicGroup);
		xmlFreeNode(referencedApplicGroup);
		return NULL;
	}

	return referencedApplicGroup;
}

/* Copy applic defs without non-user-definitions. */
static xmlNodePtr remove_non_user_defs(xmlNodePtr defs)
{
	xmlNodePtr cur, userdefs;

	userdefs = xmlCopyNode(defs, 1);
	cur = userdefs->children;

	while (cur) {
		xmlNodePtr next;
		xmlChar *userdefined;

		next = cur->next;

		userdefined = xmlGetProp(cur, BAD_CAST "userDefined");

		if (xmlStrcmp(userdefined, BAD_CAST "false") == 0) {
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
		}

		xmlFree(userdefined);

		cur = next;
	}

	return userdefs;
}

/* Simplify the applicability of the whole object. */
static xmlNodePtr simpl_whole_applic(xmlNodePtr defs, xmlDocPtr doc, bool remtrue)
{
	xmlNodePtr applic, orig, userdefs;

	/* Remove non-user-definitions. */
	userdefs = remove_non_user_defs(defs);

	orig = first_xpath_node(doc, NULL, BAD_CAST "//dmStatus/applic|//pmStatus/applic");

	if (!orig) {
		return NULL;
	}

	applic = xmlCopyNode(orig, 1);

	if (simpl_applic(userdefs, applic, remtrue)) {
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
	xmlFreeNode(userdefs);

	return applic;
}

/* Add metadata linking the data module instance with the source data module */
static void add_source(xmlDocPtr source)
{
	xmlNodePtr ident, sourceIdent, node, cur;
	const xmlChar *type;

	ident       = first_xpath_node(source, NULL, BAD_CAST "//dmIdent|//pmIdent|//dmaddres");
	sourceIdent = first_xpath_node(source, NULL, BAD_CAST "//dmStatus/sourceDmIdent|//pmStatus/sourcePmIdent|//status/srcdmaddres");
	node        = first_xpath_node(source, NULL, BAD_CAST "(//dmStatus/repositorySourceDmIdent|//dmStatus/security|//pmStatus/security|//status/security)[1]");

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
static void set_extd(xmlDocPtr doc, const char *extension)
{
	xmlNodePtr identExtension, code;
	char *ext, *extensionProducer, *extensionCode;
	enum issue issue;

	identExtension = first_xpath_node(doc, NULL, BAD_CAST "//dmIdent/identExtension|//pmIdent/identExtension|//dmaddres/dmcextension");
	code = first_xpath_node(doc, NULL, BAD_CAST "//dmIdent/dmCode|//pmIdent/pmCode|//dmaddres/dmc");

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

static void set_dm_code(xmlNodePtr code, enum issue iss, const char *s)
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
		if (verbosity > QUIET) {
			fprintf(stderr, S_BAD_CODE, "data module", s);
		}
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

static void set_pm_code(xmlNodePtr code, enum issue iss, const char *s)
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
		if (verbosity > QUIET) {
			fprintf(stderr, S_BAD_CODE, "publication module", s);
		}
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

static void set_com_code(xmlNodePtr code, enum issue iss, const char *s)
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
		if (verbosity > QUIET) {
			fprintf(stderr, S_BAD_CODE, "comment", s);
		}
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

static void set_dml_code(xmlNodePtr code, enum issue iss, const char *s)
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
		if (verbosity > QUIET) {
			fprintf(stderr, S_BAD_CODE, "data management list", s);
		}
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

static void set_code(xmlDocPtr doc, const char *new_code)
{
	xmlNodePtr code;

	code = first_xpath_node(doc, NULL, BAD_CAST
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
static void set_title(xmlDocPtr doc, const char *tech, const char *info, const xmlChar *info_name_variant, bool no_info_name)
{
	xmlNodePtr dmTitle, techName, infoName, infoNameVariant;
	enum issue iss;
	
	dmTitle  = first_xpath_node(doc, NULL, BAD_CAST
		"//dmAddressItems/dmTitle|"
		"//dmaddres/dmtitle");
	techName = first_xpath_node(doc, NULL, BAD_CAST
		"//dmAddressItems/dmTitle/techName|"
		"//pmAddressItems/pmTitle|"
		"//commentAddressItems/commentTitle|"
		"//dmaddres/dmtitle/techname|"
		"//pmaddres/pmtitle|"
		"//cstatus/ctitle");
	infoName = first_xpath_node(doc, NULL, BAD_CAST
		"//dmAddressItems/dmTitle/infoName|"
		"//dmaddres/dmtitle/infoname");
	infoNameVariant = first_xpath_node(doc, NULL, BAD_CAST
		"//dmAddressItems/dmTitle/infoNameVariant");

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

	if (tech) {
		xmlNodeSetContent(techName, BAD_CAST tech);
	}

	if (info) {
		if (!infoName) {
			infoName = xmlNewChild(dmTitle, NULL, BAD_CAST (iss == ISS_30 ? "infoname" : "infoName"), NULL);
		}
		xmlNodeSetContent(infoName, BAD_CAST info);
	} else if (no_info_name && infoName) {
		xmlUnlinkNode(infoName);
		xmlFreeNode(infoName);
	}

	if (info_name_variant) {
		if (infoNameVariant) {
			xmlChar *s;
			s = xmlEncodeEntitiesReentrant(doc, info_name_variant);
			xmlNodeSetContent(infoNameVariant, s);
			xmlFree(s);
		} else {
			infoNameVariant = xmlNewTextChild(dmTitle, NULL, BAD_CAST "infoNameVariant", info_name_variant);
		}
	} else if (no_info_name && infoNameVariant) {
		xmlUnlinkNode(infoNameVariant);
		xmlFreeNode(infoNameVariant);
	}
}

static xmlNodePtr create_assert(xmlChar *ident, xmlChar *type, xmlChar *values, enum issue iss)
{
	xmlNodePtr assert;

	assert = xmlNewNode(NULL, BAD_CAST "assert");

	xmlSetProp(assert, BAD_CAST (iss == ISS_30 ? "actidref" : "applicPropertyIdent"), ident);
	xmlSetProp(assert, BAD_CAST (iss == ISS_30 ? "actreftype" : "applicPropertyType"), type);
	xmlSetProp(assert, BAD_CAST (iss == ISS_30 ? "actvalues" : "applicPropertyValues"), values);

	return assert;
}

static xmlNodePtr create_or(xmlChar *ident, xmlChar *type, xmlNodePtr values, enum issue iss)
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
static void set_applic(xmlDocPtr doc, xmlNodePtr defs, int napplics, char *new_text, bool combine)
{
	xmlNodePtr new_applic, new_displayText, new_simplePara, new_evaluate, cur, applic;
	enum issue iss;

	applic = first_xpath_node(doc, NULL, BAD_CAST "//dmStatus/applic|//pmStatus/applic|//status/applic|//pmstatus/applic");

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

	if (combine && first_xpath_node(doc, applic, BAD_CAST "assert|evaluate|expression")) {
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

	for (cur = defs->children; cur; cur = cur->next) {
		xmlChar *user_def;

		user_def = xmlGetProp(cur, BAD_CAST "userDefined");

		if (xmlStrcmp(user_def, BAD_CAST "true") == 0) {
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

		xmlFree(user_def);
	}

	xmlUnlinkNode(applic);
	xmlFreeNode(applic);
}

/* Set the language/country for the data module instance */
static void set_lang(xmlDocPtr doc, const char *lang)
{
	xmlNodePtr language;
	char *l;
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

	l = strdup(lang);
	language_iso_code = strtok(l, "-");
	country_iso_code = strtok(NULL, "");

	lowercase(language_iso_code);
	uppercase(country_iso_code);

	xmlSetProp(language, BAD_CAST (iss == ISS_30 ? "language" : "languageIsoCode"), BAD_CAST language_iso_code);
	xmlSetProp(language, BAD_CAST (iss == ISS_30 ? "country" : "countryIsoCode"), BAD_CAST country_iso_code);

	free(l);
}

static bool auto_name(char *out, char *src, xmlDocPtr dm, const char *dir, bool noiss)
{
	struct ident ident = {0};
	char iss[8] = "";
	const char *dname, *sep;

	if (strcmp(dir, ".") == 0) {
		dname = "";
		sep = "";
	} else {
		dname = dir;
		sep = "/";
	}

	if (!init_ident(&ident, dm)) {
		char *base;
		base = basename(src);
		snprintf(out, PATH_MAX, "%s%s%s", dname, sep, base);
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
			sprintf(out, "%s%sPME-%s-%s-%s-%s-%s-%s%s_%s-%s.XML",
				dname,
				sep,
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
			sprintf(out, "%s%sPMC-%s-%s-%s-%s%s_%s-%s.XML",
				dname,
				sep,
				ident.modelIdentCode,
				ident.senderIdent,
				ident.pmNumber,
				ident.pmVolume,
				iss,
				ident.languageIsoCode,
				ident.countryIsoCode);
		}
	} else if (ident.type == DML) {
		sprintf(out, "%s%sDML-%s-%s-%s-%s-%s%s.XML",
			dname,
			sep,
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
			sprintf(out, "%s%s%s-%s-%s-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s%s%s_%s-%s.XML",
				dname,
				sep,
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
			sprintf(out, "%s%s%s-%s-%s-%s-%s%s-%s-%s%s-%s%s-%s%s%s_%s-%s.XML",
				dname,
				sep,
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
		sprintf(out, "%s%sCOM-%s-%s-%s-%s-%s_%s-%s.XML",
			dname,
			sep,
			ident.modelIdentCode,
			ident.senderIdent,
			ident.yearOfDataIssue,
			ident.seqNumber,
			ident.dmlCommentType,
			ident.languageIsoCode,
			ident.countryIsoCode);
	} else if (ident.type == DDN) {
		sprintf(out, "%s%sDDN-%s-%s-%s-%s-%s.XML",
			dname,
			sep,
			ident.modelIdentCode,
			ident.senderIdent,
			ident.receiverIdent,
			ident.yearOfDataIssue,
			ident.seqNumber);
	} else if (ident.type == IMF) {
		sprintf(out, "%s%sIMF-%s%s.XML",
			dname,
			sep,
			ident.imfIdentIcn,
			iss);
	} else {
		return false;
	}

	free_ident(&ident);

	return true;
}

/* Get the appropriate built-in CIR repository XSLT by name */
static bool get_cir_xsl(const char *cirtype, unsigned char **xsl, unsigned int *len)
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
		if (verbosity > QUIET) {
			fprintf(stderr, S_NO_XSLT, cirtype);
		}
		return false;
	}

	return true;
}

/* Dump built-in XSLT for resolving CIR repository dependencies */
static void dump_cir_xsl(const char *repo)
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
static void undepend_cir_xsl(xmlDocPtr dm, xmlDocPtr cir, xsltStylesheetPtr style)
{
	xmlDocPtr res, muxdoc;
	xmlNodePtr mux, old;

	muxdoc = xmlNewDoc(BAD_CAST "1.0");
	mux = xmlNewNode(NULL, BAD_CAST "mux");
	xmlDocSetRootElement(muxdoc, mux);
	xmlAddChild(mux, xmlCopyNode(xmlDocGetRootElement(dm), 1));
	xmlAddChild(mux, xmlCopyNode(xmlDocGetRootElement(cir), 1));

	res = xsltApplyStylesheet(style, muxdoc, NULL);

	old = xmlDocSetRootElement(dm, xmlCopyNode(first_xpath_node(res, NULL, BAD_CAST "/mux/*[1]"), 1));
	xmlFreeNode(old);

	xmlFreeDoc(res);
	xmlFreeDoc(muxdoc);
}

/* Apply the user-defined applicability to the CIR data module, then call the
 * appropriate function for the specific type of CIR. */
static xmlNodePtr undepend_cir(xmlDocPtr dm, xmlNodePtr defs, const char *cirdocfname, bool add_src, const char *cir_xsl, xmlDocPtr def_cir_xsl)
{
	xmlDocPtr cir;
	xmlXPathContextPtr ctxt;
	xmlXPathObjectPtr results;

	xmlNodePtr cirnode;
	xmlNodePtr content;
	xmlNodePtr referencedApplicGroup;

	char *cirtype;

	xmlDocPtr styledoc = NULL;

	cir = read_xml_doc(cirdocfname);

	if (!cir) {
		if (verbosity > QUIET) {
			fprintf(stderr, S_INVALID_CIR, cirdocfname);
		}
		exit(EXIT_BAD_XML);
	}

	ctxt = xmlXPathNewContext(cir);

	results = xmlXPathEvalExpression(BAD_CAST "//content", ctxt);

	if (xmlXPathNodeSetIsEmpty(results->nodesetval)) {
		cirnode = xmlDocGetRootElement(cir);
	} else {
		content = results->nodesetval->nodeTab[0];
		xmlXPathFreeObject(results);

		results = xmlXPathEvalExpression(BAD_CAST "//referencedApplicGroup", ctxt);

		if (!xmlXPathNodeSetIsEmpty(results->nodesetval)) {
			referencedApplicGroup = results->nodesetval->nodeTab[0];
			strip_applic(defs, referencedApplicGroup, content);
		}

		xmlXPathFreeObject(results);

		results = xmlXPathEvalExpression(BAD_CAST
			"//content/commonRepository/*[position()=last()]|"
			"//content/techRepository/*[position()=last()]|"
			"//content/techrep/*[position()=last()]|"
			"//content/illustratedPartsCatalog",
			ctxt);

		if (xmlXPathNodeSetIsEmpty(results->nodesetval)) {
			cirnode = xmlDocGetRootElement(cir);
		} else {
			cirnode = results->nodesetval->nodeTab[0];
		}
	}

	xmlXPathFreeObject(results);

	cirtype = (char *) cirnode->name;

	if (cir_xsl) {
		styledoc = read_xml_doc(cir_xsl);
	} else if (def_cir_xsl) {
		styledoc = xmlCopyDoc(def_cir_xsl, 1);
	} else {
		unsigned char *xsl = NULL;
		unsigned int len = 0;

		if (!get_cir_xsl(cirtype, &xsl, &len)) {
			add_src = false;
		}

		styledoc = read_xml_mem((const char *) xsl, len);
	}

	if (styledoc) {
		xsltStylesheetPtr style;
		style = xsltParseStylesheetDoc(styledoc);
		undepend_cir_xsl(dm, cir, style);
		xsltFreeStylesheet(style);
	}

	xmlXPathFreeContext(ctxt);

	if (first_xpath_node(dm, NULL, BAD_CAST "//idstatus")) {
		add_src = false;
	}

	if (add_src) {
		xmlNodePtr dmIdent;

		dmIdent = xpath_first_node(cir, NULL, BAD_CAST "//dmIdent");

		if (dmIdent) {
			xmlNodePtr security, repositorySourceDmIdent, cur;

			security = xpath_first_node(dm, NULL, BAD_CAST "//security");

			repositorySourceDmIdent = xmlNewNode(NULL, BAD_CAST "repositorySourceDmIdent");
			repositorySourceDmIdent = xmlAddPrevSibling(security, repositorySourceDmIdent);

			for (cur = dmIdent->children; cur; cur = cur->next) {
				xmlAddChild(repositorySourceDmIdent, xmlCopyNode(cur, 1));
			}
		}
	}

	xmlFreeDoc(cir);

	return xmlDocGetRootElement(dm);
}

/* General XSLT transformation with embedded stylesheet, preserving the DTD. */
static void transform_doc(xmlDocPtr doc, unsigned char *xml, unsigned int len, const char **params)
{
	xmlDocPtr styledoc, res, src;
	xsltStylesheetPtr style;
	xmlNodePtr old;

	styledoc = read_xml_mem((const char *) xml, len);
	style = xsltParseStylesheetDoc(styledoc);

	src = xmlCopyDoc(doc, 1);
	res = xsltApplyStylesheet(style, src, params);
	xmlFreeDoc(src);

	old = xmlDocSetRootElement(doc, xmlCopyNode(xmlDocGetRootElement(res), 1));
	xmlFreeNode(old);

	xmlFreeDoc(res);
	xsltFreeStylesheet(style);
}

/* Remove all change markup from the instance. */
static void remove_change_markup(xmlDocPtr doc)
{
	transform_doc(doc, xsl_remove_change_markup_xsl, xsl_remove_change_markup_xsl_len, NULL);
}

/* Set the issue type of the instance. */
static void set_issue_type(xmlDocPtr doc, const char *type)
{
	xmlNodePtr status;

	status = first_xpath_node(doc, NULL, BAD_CAST "//dmStatus|//pmStatus|//commentStatus|//dmlStatus|//scormContentPackageStatus|//issno");

	if (xmlStrcmp(status->name, BAD_CAST "issno") == 0) {
		xmlSetProp(status, BAD_CAST "type", BAD_CAST type);
	} else {
		xmlSetProp(status, BAD_CAST "issueType", BAD_CAST type);
	}
}

/* Add a default RFU when a "new" master produces non-new instances. */
static void add_default_rfu(xmlDocPtr dm)
{
	xmlNodePtr node, rfu;
	bool iss30;

	/* Issue 4.2+ allows "new" DMs to have RFUs, so use this instead if
	 * present. */
	if (first_xpath_node(dm, NULL, BAD_CAST "//rfu|//reasonForUpdate")) {
		return;
	}

	node = first_xpath_node(dm, NULL, BAD_CAST
		"("
		"//dmStatus/*|//status/*|"
		"//pmStatus/*|//pmstatus/*|"
		"//commentStatus/*|"
		"//ddnStatus/*|"
		"//dmlStatus/*|"
		"//scormContentPackageStatus/*"
		")[not(self::productSafety or self::remarks)][last()]");

	if (!node) {
		return;
	}

	iss30 = xmlStrcmp(node->parent->name, BAD_CAST "status") == 0 || xmlStrcmp(node->parent->name, BAD_CAST "pmstatus") == 0;

	rfu = xmlNewNode(node->ns, BAD_CAST (iss30 ? "rfu" : "reasonForUpdate"));
	xmlAddNextSibling(node, rfu);
	xmlNewTextChild(rfu, rfu->ns, BAD_CAST (iss30 ? "p" : "simplePara"), DEFAULT_RFU);
}

/* Set the issue and inwork numbers of the instance. */
static void set_issue(xmlDocPtr dm, char *issinfo, bool incr_iss)
{
	char issue[32], inwork[32];
	xmlNodePtr issueInfo;

	if (!(issueInfo = first_xpath_node(dm, NULL, ISSUE_INFO_XPATH))) {
		return;
	}

	if (incr_iss) {
		xmlChar *issue_s, *inwork_s;
		int inwork_i;

		issue_s  = first_xpath_value(dm, issueInfo, BAD_CAST "@issueNumber|@issno");
		inwork_s = first_xpath_value(dm, issueInfo, BAD_CAST "@inWork|@inwork");

		strcpy(issue, (char *) issue_s);
		if (inwork_s) {
			strcpy(inwork, (char *) inwork_s);
		} else {
			strcpy(inwork, "00");
		}

		xmlFree(issue_s);
		xmlFree(inwork_s);

		inwork_i = atoi(inwork);

		snprintf(inwork, 32, "%.2d", inwork_i + 1);
	} else if (sscanf(issinfo, "%3s-%2s", issue, inwork) != 2) {
		if (verbosity > QUIET) {
			fprintf(stderr, ERR_PREFIX S_INVALID_ISSFMT);
		}
		exit(EXIT_MISSING_ARGS);
	}

	/* If the issue is set below 001-01, there cannot be change marks, and issue type is "new" */
	if (strcmp(issue, "000") == 0 || (strcmp(issue, "001") == 0 && strcmp(inwork, "00") == 0)) {
		remove_change_markup(dm);
		set_issue_type(dm, "new");
	/* Otherwise, try to default to the issue type of the master. */
	} else {
		xmlChar *type;

		type = first_xpath_value(dm, NULL, BAD_CAST "//@issueType|//issno/@type");

		/* If the master is "new" but the target issue cannot be,
		 * default to "status" as their should be no change marks. */
		if (xmlStrcmp(type, BAD_CAST "new") == 0) {
			set_issue_type(dm, "status");
			add_default_rfu(dm);
		/* Otherwise, use the master's issue type. */
		} else {
			set_issue_type(dm, (char *) type);
		}

		xmlFree(type);
	}

	issueInfo = first_xpath_node(dm, NULL, ISSUE_INFO_XPATH);

	if (xmlStrcmp(issueInfo->name, BAD_CAST "issueInfo") == 0) {
		xmlSetProp(issueInfo, BAD_CAST "issueNumber", BAD_CAST issue);
		xmlSetProp(issueInfo, BAD_CAST "inWork", BAD_CAST inwork);
	} else {
		xmlSetProp(issueInfo, BAD_CAST "issno", BAD_CAST issue);
		xmlSetProp(issueInfo, BAD_CAST "inwork", BAD_CAST inwork);
	}
}

/* Set the issue date of the instance. */
static void set_issue_date(xmlDocPtr doc, const char *year, const char *month, const char *day)
{
	xmlNodePtr issueDate;

	issueDate = first_xpath_node(doc, NULL, BAD_CAST "//issueDate|//issdate");

	xmlSetProp(issueDate, BAD_CAST "year", BAD_CAST year);
	xmlSetProp(issueDate, BAD_CAST "month", BAD_CAST month);
	xmlSetProp(issueDate, BAD_CAST "day", BAD_CAST day);
}

/* Set the securty classification of the instance. */
static void set_security(xmlDocPtr dm, char *sec)
{
	xmlNodePtr security;
	enum issue iss;

	security = first_xpath_node(dm, NULL, BAD_CAST "//security");

	if (!security) {
		return;
	} else if (xmlStrcmp(security->parent->name, BAD_CAST "dmStatus") == 0 || xmlStrcmp(security->parent->name, BAD_CAST "pmStatus") == 0) {
		iss = ISS_4X;
	} else {
		iss = ISS_30;
	}

	xmlSetProp(security, BAD_CAST (iss == ISS_30 ? "class" : "securityClassification"), BAD_CAST sec);
}

/* Get the originator of the source. If it has no originator
 * (e.g. pub modules do not require one) then create one in the
 * instance.
 */
static xmlNodePtr find_or_create_orig(xmlDocPtr doc)
{
	xmlNodePtr orig, rpc;
	orig = first_xpath_node(doc, NULL, BAD_CAST "//originator|//orig");
	if (!orig) {
		rpc = first_xpath_node(doc, NULL, BAD_CAST "//responsiblePartnerCompany|//rpc");
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
static void set_orig(xmlDocPtr doc, const char *origspec)
{
	xmlNodePtr originator;
	const char *code, *name;
	enum issue iss;
	char *s;

	if (origspec) {
		s = strdup(origspec);
		code = strtok(s, "/");
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

	if (code && strcmp(code, "-") != 0) {
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

	if (origspec) {
		free(s);
	}
}

/* Determine if the whole object is applicable. */
static bool check_wholedm_applic(xmlDocPtr dm, xmlNodePtr defs)
{
	xmlNodePtr applic;

	applic = first_xpath_node(dm, NULL, BAD_CAST "//dmStatus/applic|//pmStatus/applic");

	if (!applic) {
		return true;
	}

	return eval_applic_stmt(defs, applic, true);
}

/* Read applicability definitions from the <assign> elements of a
 * product instance in the specified PCT data module.\
 */
static void load_applic_from_pct(xmlNodePtr defs, int *napplics, xmlDocPtr pct, const char *pctfname, const char *product)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlChar *xpath;

	ctx = xmlXPathNewContext(pct);

	/* If the product is in the form of IDENT:TYPE=VALUE, it identifies the
	 * primary key of a product instance.
	 *
	 * Otherwise, it is simply the XML ID of a product instance.
	 */
	if (match_pattern(BAD_CAST product, BAD_CAST "[^:]+:(prodattr|condition)=[^|~]+")) {
		char *prod, *ident, *type, *value;

		prod  = strdup(product);

		ident = strtok(prod, ":");
		type  = strtok(NULL, "=");
		value = strtok(NULL, "");

		xmlXPathRegisterVariable(ctx, BAD_CAST "ident", xmlXPathNewCString(ident));
		xmlXPathRegisterVariable(ctx, BAD_CAST "type" , xmlXPathNewCString(type));
		xmlXPathRegisterVariable(ctx, BAD_CAST "value", xmlXPathNewCString(value));
		xpath = BAD_CAST "//product[assign[@applicPropertyIdent=$ident and @applicPropertyType=$type and @applicPropertyValue=$value]]/assign";

		free(prod);
	} else {
		xmlXPathRegisterVariable(ctx, BAD_CAST "id", xmlXPathNewCString(product));
		xpath = BAD_CAST "//product[@id=$id]/assign";
	}

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		if (verbosity > QUIET) {
			fprintf(stderr, S_NO_PRODUCT, product, pctfname);
		}
	} else {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *ident, *type, *value;

			ident = xmlGetProp(obj->nodesetval->nodeTab[i],
				BAD_CAST "applicPropertyIdent");
			type  = xmlGetProp(obj->nodesetval->nodeTab[i],
				BAD_CAST "applicPropertyType");
			value = xmlGetProp(obj->nodesetval->nodeTab[i],
				BAD_CAST "applicPropertyValue");

			define_applic(defs, napplics, ident, type, value, true, true);

			xmlFree(ident);
			xmlFree(type);
			xmlFree(value);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Remove the extended identification from the instance. */
static void strip_extension(xmlDocPtr doc)
{
	xmlNodePtr ext;

	ext = first_xpath_node(doc, NULL, BAD_CAST "//identExtension");

	xmlUnlinkNode(ext);
	xmlFreeNode(ext);
}

/* Flatten alts elements. */
static void flatten_alts(xmlDocPtr doc, bool fix_alts_refs)
{
	char *params[3];

	params[0] = "fix-alts-refs";
	params[1] = fix_alts_refs ? "true()" : "false()";
	params[2] = NULL;

	transform_doc(doc, xsl_flatten_alts_xsl, xsl_flatten_alts_xsl_len, (const char **) params);
}

/* Removes invalid empty sections in a PM after all references have
 * been filtered out.
 */
static void remove_empty_pmentries(xmlDocPtr doc)
{
	transform_doc(doc, ___common_remove_empty_pmentries_xsl, ___common_remove_empty_pmentries_xsl_len, NULL);
}

/* Fix certain elements automatically after filtering. */
static void autocomplete(xmlDocPtr doc)
{
	transform_doc(doc, xsl_autocomplete_xsl, xsl_autocomplete_xsl_len, NULL);
}

/* Insert a custom comment. */
static void insert_comment(xmlDocPtr doc, const char *text, const char *path)
{
	xmlNodePtr comment, pos;

	comment = xmlNewComment(BAD_CAST text);
	pos = first_xpath_node(doc, NULL, BAD_CAST path);

	if (!pos)
		return;

	if (pos->children) {
		xmlAddPrevSibling(pos->children, comment);
	} else {
		xmlAddChild(pos, comment);
	}
}

/* Read an applicability assign in the form of ident:type=value */
static void read_applic(xmlNodePtr defs, int *napplics, char *s)
{
	char *ident, *type, *value;

	if (!match_pattern(BAD_CAST s, BAD_CAST "[^:]+:(prodattr|condition)=[^|~]+")) {
		if (verbosity > QUIET) {
			fprintf(stderr, S_BAD_ASSIGN, s);
		}
		exit(EXIT_BAD_APPLIC);
	}

	ident = strtok(s, ":");
	type  = strtok(NULL, "=");
	value = strtok(NULL, "");

	define_applic(defs, napplics, BAD_CAST ident, BAD_CAST type, BAD_CAST value, false, true);
}

/* Set the remarks for the object */
static void set_remarks(xmlDocPtr doc, const char *s)
{
	xmlNodePtr status, remarks;

	status = first_xpath_node(doc, NULL, BAD_CAST
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

	remarks = first_xpath_node(doc, status, BAD_CAST "//remarks");

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
static bool valid_object(xmlDocPtr doc, const char *path, const char *codes)
{
	xmlNodePtr obj;
	xmlChar *code;
	bool has;
	if (!(obj = first_xpath_node(doc, NULL, BAD_CAST path))) {
		return true;
	}
	code = xmlNodeGetContent(obj);
	has = strstr(codes, (char *) code);
	xmlFree(code);
	return has;
}

/* Remove elements that have an attribute whose value does not match a given
 * set of valid values. */
static void filter_elements_by_att(xmlDocPtr doc, const char *att, const char *codes)
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

/* Determine if an object is "deleted". */
static bool is_deleted(xmlDocPtr doc)
{
	xmlChar *isstype;
	bool is;

	isstype = first_xpath_value(doc, NULL, BAD_CAST "//dmStatus/@issueType|//dmaddres/issno/@type|//pmStatus/@issueType|//pmaddres/issno/@type|//commentStatus/@issueType|//dmlStatus/@issueType|//scormContentPackageStatus/@issueType");
	is = xmlStrcmp(isstype, BAD_CAST "deleted") == 0;
	xmlFree(isstype);

	return is;
}

/* Determine whether or not to create an instance based on the object's:
 * - Applicability
 * - Skill level
 * - Security classification
 * - Issue type
 */
static bool create_instance(xmlDocPtr doc, xmlNodePtr defs, const char *skills, const char *securities, bool delete)
{
	if (!check_wholedm_applic(doc, defs)) {
		return false;
	}

	if (securities && !valid_object(doc, "//dmStatus/security/@securityClassification", securities)) {
		return false;
	}

	if (skills && !valid_object(doc, "//dmStatus/skillLevel/@skillLevelCode", skills)) {
		return false;
	}

	if (delete && is_deleted(doc)) {
		return false;
	}

	return true;
}

/* Set the skill level code of the instance. */
static void set_skill(xmlDocPtr doc, const char *skill)
{
	xmlNodePtr skill_level;

	if (xmlStrcmp(xmlDocGetRootElement(doc)->name, BAD_CAST "dmodule") != 0) {
		return;
	}

	skill_level = first_xpath_node(doc, NULL, BAD_CAST "//skillLevel");

	if (!skill_level) {
		xmlNodePtr node;
		node = first_xpath_node(doc, NULL, BAD_CAST
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

/* Find a data module filename in the current directory based on the dmRefIdent
 * element. */
static bool find_dmod_fname(char *dst, xmlNodePtr dmRefIdent, bool ignore_iss)
{
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

	dmCode = first_xpath_node(NULL, dmRefIdent, BAD_CAST "dmCode|avee");
	issueInfo = first_xpath_node(NULL, dmRefIdent, BAD_CAST "issueInfo|issno");
	language = first_xpath_node(NULL, dmRefIdent, BAD_CAST "language");

	model_ident_code     = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "modelic|@modelIdentCode");
	system_diff_code     = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "sdc|@systemDiffCode");
	system_code          = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "chapnum|@systemCode");
	sub_system_code      = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "section|@subSystemCode");
	sub_sub_system_code  = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "subsect|@subSubSystemCode");
	assy_code            = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "subject|@assyCode");
	disassy_code         = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "discode|@disassyCode");
	disassy_code_variant = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "discodev|@disassyCodeVariant");
	info_code            = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "incode|@infoCode");
	info_code_variant    = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "incodev|@infoCodeVariant");
	item_location_code   = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "itemloc|@itemLocationCode");
	learn_code           = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "@learnCode");
	learn_event_code     = (char *) first_xpath_value(NULL, dmCode, BAD_CAST "@learnEventCode");

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

	if (!no_issue) {
		if (!ignore_iss && issueInfo) {
			char *issue_number;
			char *in_work;
			char iss[8];

			issue_number = (char *) first_xpath_value(NULL, issueInfo, BAD_CAST "@issno|@issueNumber");
			in_work      = (char *) first_xpath_value(NULL, issueInfo, BAD_CAST "@inwork|@inWork");

			snprintf(iss, 8, "_%s-%s", issue_number, in_work ? in_work : "00");
			strcat(code, iss);

			xmlFree(issue_number);
			xmlFree(in_work);
		} else if (language) {
			strcat(code, "_\?\?\?-\?\?");
		}
	}

	if (language) {
		char *language_iso_code;
		char *country_iso_code;
		char lang[8];

		language_iso_code = (char *) first_xpath_value(NULL, language, BAD_CAST "@language|@languageIsoCode");
		country_iso_code  = (char *) first_xpath_value(NULL, language, BAD_CAST "@country|@countryIsoCode");

		snprintf(lang, 8, "_%s-%s", language_iso_code, country_iso_code);
		strcat(code, lang);

		xmlFree(language_iso_code);
		xmlFree(country_iso_code);
	}

	if (find_csdb_object(dst, search_dir, code, is_dm, recursive_search)) {
		return true;
	}

	if (verbosity > QUIET) {
		fprintf(stderr, S_MISSING_REF_DM, code);
	}
	return false;
}

/* Find a PM filename in the current directory based on the pmRefIdent
 * element. */
static bool find_pm_fname(char *dst, xmlNodePtr pmRefIdent, bool ignore_iss)
{
	char *model_ident_code;
	char *pm_issuer;
	char *pm_number;
	char *pm_volume;
	char code[64];
	xmlNodePtr pmCode, issueInfo, language;

	pmCode = first_xpath_node(NULL, pmRefIdent, BAD_CAST "pmCode|pmc");
	issueInfo = first_xpath_node(NULL, pmRefIdent, BAD_CAST "issueInfo|issno");
	language = first_xpath_node(NULL, pmRefIdent, BAD_CAST "language");

	model_ident_code = (char *) first_xpath_value(NULL, pmCode, BAD_CAST "modelic|@modelIdentCode");
	pm_issuer        = (char *) first_xpath_value(NULL, pmCode, BAD_CAST "pmissuer|@pmIssuer");
	pm_number        = (char *) first_xpath_value(NULL, pmCode, BAD_CAST "pmnumber|@pmNumber");
	pm_volume        = (char *) first_xpath_value(NULL, pmCode, BAD_CAST "pmvolume|@pmVolume");

	snprintf(code, 64, "PMC-%s-%s-%s-%s",
		model_ident_code,
		pm_issuer,
		pm_number,
		pm_volume);

	xmlFree(model_ident_code);
	xmlFree(pm_issuer);
	xmlFree(pm_number);
	xmlFree(pm_volume);

	if (!no_issue) {
		if (!ignore_iss && issueInfo) {
			char *issue_number;
			char *in_work;
			char iss[8];

			issue_number = (char *) first_xpath_value(NULL, issueInfo, BAD_CAST "@issno|@issueNumber");
			in_work      = (char *) first_xpath_value(NULL, issueInfo, BAD_CAST "@inwork|@inWork");

			snprintf(iss, 8, "_%s-%s", issue_number, in_work ? in_work : "00");
			strcat(code, iss);

			xmlFree(issue_number);
			xmlFree(in_work);
		} else if (language) {
			strcat(code, "_\?\?\?-\?\?");
		}
	}

	if (language) {
		char *language_iso_code;
		char *country_iso_code;
		char lang[8];

		language_iso_code = (char *) first_xpath_value(NULL, language, BAD_CAST "@language|@languageIsoCode");
		country_iso_code  = (char *) first_xpath_value(NULL, language, BAD_CAST "@country|@countryIsoCode");

		snprintf(lang, 8, "_%s-%s", language_iso_code, country_iso_code);
		strcat(code, lang);

		xmlFree(language_iso_code);
		xmlFree(country_iso_code);
	}

	if (find_csdb_object(dst, search_dir, code, is_pm, recursive_search)) {
		return true;
	}

	if (verbosity > QUIET) {
		fprintf(stderr, S_MISSING_REF_DM, code);
	}
	return false;
}

/* Find the filename of a referenced ACT data module. */
static bool find_act_fname(char *dst, xmlDocPtr doc)
{
	xmlNodePtr actref;
	actref = first_xpath_node(doc, NULL, BAD_CAST "//applicCrossRefTableRef/dmRef/dmRefIdent|//actref/refdm");
	return actref && find_dmod_fname(dst, actref, false);
}

/* Find the filename of a referenced CCT data module. */
static bool find_cct_fname(char *dst, xmlDocPtr act)
{
	xmlNodePtr cctref;
	cctref = first_xpath_node(act, NULL, BAD_CAST "//condCrossRefTableRef/dmRef/dmRefIdent|//cctref/refdm");
	return cctref && find_dmod_fname(dst, cctref, false);
}

/* Find the filename of a referenced PCT data module via the ACT. */
static bool find_pct_fname(char *dst, xmlDocPtr act)
{
	xmlNodePtr pctref;
	pctref = first_xpath_node(act, NULL, BAD_CAST "//productCrossRefTableRef/dmRef/dmRefIdent|//pctref/refdm");
	return pctref && find_dmod_fname(dst, pctref, false);
}

/* Unset all applicability assigned on a per-DM basis. */
static void clear_perdm_applic(xmlNodePtr defs, int *napplics)
{
	xmlNodePtr cur;
	cur = defs->children;
	while (cur) {
		xmlNodePtr next;
		next = cur->next;
		if (xmlHasProp(cur, BAD_CAST "perDm")) {
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
			--(*napplics);
		}
		cur = next;
	}
}

/* Callback for clean_entities.
 *
 * The name parameter of the xmlHashScanner type was changed to be const in
 * newer versions of libxml2.
 *
 * Older versions, and the publically documented API, have it as non-const.
 */
#if LIBXML_VERSION < 20907
static void clean_entities_callback(void *payload, void *data, xmlChar *name)
#else
static void clean_entities_callback(void *payload, void *data, const xmlChar *name)
#endif
{
	xmlEntityPtr e = (xmlEntityPtr) payload;
	xmlChar *xpath;

	xpath = xmlStrdup(BAD_CAST "//@*[.='");
	xpath = xmlStrcat(xpath, e->name);
	xpath = xmlStrcat(xpath, BAD_CAST "']");

	if (e->etype == XML_EXTERNAL_GENERAL_UNPARSED_ENTITY && !first_xpath_node(e->doc, NULL, xpath)) {
		xmlUnlinkNode((xmlNodePtr) e);
		xmlFreeEntity(e);
	}
}

/* Remove unused external entities after filtering. */
static void clean_entities(xmlDocPtr doc)
{
	if (doc->intSubset) {
		xmlHashScan(doc->intSubset->entities, clean_entities_callback, NULL);
	}
}

/* Find the source object for an instance.
 * -1  Object does not identify a source.
 *  0  Object identifies a source and it was found.
 *  1  Object identifies a source but it couldn't be found.
 */
static int find_source(char *src, xmlDocPtr *doc)
{
	xmlNodePtr sdi;
	bool found = false;

	if (!(*doc = read_xml_doc(src))) {
		return -1;
	}

	if (!(sdi = first_xpath_node(*doc, NULL, BAD_CAST "//sourceDmIdent|//sourcePmIdent|//srcdmaddres"))) {
		return -1;
	}

	if (xmlStrcmp(sdi->name, BAD_CAST "sourceDmIdent") == 0 || xmlStrcmp(sdi->name, BAD_CAST "srcdmaddres") == 0) {
		found = find_dmod_fname(src, sdi, true);
	} else if (xmlStrcmp(sdi->name, BAD_CAST "sourcePmIdent") == 0) {
		found = find_pm_fname(src, sdi, true);
	}

	return !found;
}

static void load_applic_from_inst(xmlNodePtr defs, xmlDocPtr doc)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST "//identAndStatusSection//applic[1]//assert", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *ident, *type, *value;

			ident = first_xpath_value(doc, obj->nodesetval->nodeTab[i], BAD_CAST "@applicPropertyIdent");
			type  = first_xpath_value(doc, obj->nodesetval->nodeTab[i], BAD_CAST "@applicPropertyType");
			value = first_xpath_value(doc, obj->nodesetval->nodeTab[i], BAD_CAST "@applicPropertyValues");

			/* userdefined = false so that user-defined assertions override those in the instance. */
			define_applic(defs, NULL, ident, type, value, true, false);

			xmlFree(ident);
			xmlFree(type);
			xmlFree(value);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

static void load_skill_from_inst(xmlDocPtr doc, char **skill_codes)
{
	char *skill;

	if ((skill = (char *) first_xpath_value(doc, NULL, BAD_CAST "//skillLevel/@skillLevelCode|//skill/@skill"))) {
		free(*skill_codes);
		*skill_codes = skill;
	}
}

static void load_sec_from_inst(xmlDocPtr doc, char **sec_classes)
{
	char *sec;

	if ((sec = (char *) first_xpath_value(doc, NULL, BAD_CAST "//security/@securityClassification|//security/@class"))) {
		free(*sec_classes);
		*sec_classes = sec;
	}
}

static void add_cirs_from_inst(xmlDocPtr doc, xmlNodePtr cirs)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST "//repositorySourceDmIdent", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			char path[PATH_MAX];

			if (find_dmod_fname(path, obj->nodesetval->nodeTab[i], true)) {
				xmlNewChild(cirs, NULL, BAD_CAST "cir", BAD_CAST path);
			}

			xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
			xmlFreeNode(obj->nodesetval->nodeTab[i]);
			obj->nodesetval->nodeTab[i] = NULL;
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Load all other metadata settings from the instance. */
static void load_metadata_from_inst(xmlDocPtr doc,
	char *extension,
	char *code,
	char *lang,
	char *issinfo,
	bool incr_iss,
	char **techname,
	char **infoname,
	xmlChar **infonamevar,
	bool *no_infoname,
	char **isstype,
	char *security,
	char **orig,
	bool *setorig,
	char **skill,
	char **remarks,
	bool *new_applic,
	char *new_display_text)
{
	xmlNodePtr node;

	if ((node = first_xpath_node(doc, NULL, EXTENSION_XPATH))) {
		xmlChar *p, *c;
		p = first_xpath_value(doc, node, BAD_CAST "@extensionProducer|dmeproducer");
		c = first_xpath_value(doc, node, BAD_CAST "@extensionCode|dmecode");
		sprintf(extension, "%s-%s", (char *) p, (char *) c);
	}

	if ((node = first_xpath_node(doc, NULL, CODE_XPATH))) {
		if (xmlStrcmp(node->name, BAD_CAST "dmCode") == 0 || xmlStrcmp(node->name, BAD_CAST "avee") == 0) {
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

			model_ident_code     = (char *) first_xpath_value(NULL, node, BAD_CAST "modelic|@modelIdentCode");
			system_diff_code     = (char *) first_xpath_value(NULL, node, BAD_CAST "sdc|@systemDiffCode");
			system_code          = (char *) first_xpath_value(NULL, node, BAD_CAST "chapnum|@systemCode");
			sub_system_code      = (char *) first_xpath_value(NULL, node, BAD_CAST "section|@subSystemCode");
			sub_sub_system_code  = (char *) first_xpath_value(NULL, node, BAD_CAST "subsect|@subSubSystemCode");
			assy_code            = (char *) first_xpath_value(NULL, node, BAD_CAST "subject|@assyCode");
			disassy_code         = (char *) first_xpath_value(NULL, node, BAD_CAST "discode|@disassyCode");
			disassy_code_variant = (char *) first_xpath_value(NULL, node, BAD_CAST "discodev|@disassyCodeVariant");
			info_code            = (char *) first_xpath_value(NULL, node, BAD_CAST "incode|@infoCode");
			info_code_variant    = (char *) first_xpath_value(NULL, node, BAD_CAST "incodev|@infoCodeVariant");
			item_location_code   = (char *) first_xpath_value(NULL, node, BAD_CAST "itemloc|@itemLocationCode");
			learn_code           = (char *) first_xpath_value(NULL, node, BAD_CAST "@learnCode");
			learn_event_code     = (char *) first_xpath_value(NULL, node, BAD_CAST "@learnEventCode");

			sprintf(code, "%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
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
		} else if (xmlStrcmp(node->name, BAD_CAST "pmCode") == 0 || xmlStrcmp(node->name, BAD_CAST "pmc") == 0) {
			char *model_ident_code;
			char *pm_issuer;
			char *pm_number;
			char *pm_volume;

			model_ident_code = (char *) first_xpath_value(NULL, node, BAD_CAST "modelic|@modelIdentCode");
			pm_issuer        = (char *) first_xpath_value(NULL, node, BAD_CAST "pmissuer|@pmIssuer");
			pm_number        = (char *) first_xpath_value(NULL, node, BAD_CAST "pmnumber|@pmNumber");
			pm_volume        = (char *) first_xpath_value(NULL, node, BAD_CAST "pmvolume|@pmVolume");

			sprintf(code, "%s-%s-%s-%s",
				model_ident_code,
				pm_issuer,
				pm_number,
				pm_volume);

			xmlFree(model_ident_code);
			xmlFree(pm_issuer);
			xmlFree(pm_number);
			xmlFree(pm_volume);
		}
	}

	if ((node = first_xpath_node(doc, NULL, LANGUAGE_XPATH))) {
		xmlChar *l, *c;
		l = first_xpath_value(doc, node, BAD_CAST "@languageIsoCode|@language");
		c = first_xpath_value(doc, node, BAD_CAST "@countryIsoCode|@country");
		sprintf(lang, "%s-%s", (char *) l, (char *) c);
		xmlFree(l);
		xmlFree(c);
	}

	if ((node = first_xpath_node(doc, NULL, ISSUE_INFO_XPATH))) {
		char *i, *w;
		i = (char *) first_xpath_value(doc, node, BAD_CAST "@issueNumber|@issno");
		w = (char *) first_xpath_value(doc, node, BAD_CAST "@inWork|@inwork");

		if (!w) {
			w = strdup("00");
		}

		if (incr_iss) {
			int inwork_i;
			inwork_i = atoi(w);
			free(w);
			w = malloc(32);
			snprintf(w, 32, "%.2d", inwork_i + 1);
		}

		sprintf(issinfo, "%s-%s", i, w);

		xmlFree(i);
		xmlFree(w);
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//dmAddressItems/dmTitle/techName|//dmaddres/dmtitle/techname|//pmAddressItems/pmTitle|//pmaddres/pmtitle"))) {
		free(*techname);
		*techname = (char *) xmlNodeGetContent(node);
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//dmAddressItems/dmTitle/infoName|//dmaddres/dmtitle/infoname"))) {
		free(*infoname);
		*infoname = (char *) xmlNodeGetContent(node);
		*no_infoname = false;
	} else {
		*no_infoname = true;
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//dmAddressItems/dmTitle/infoNameVariant"))) {
		*infonamevar = xmlNodeGetContent(node);
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//dmStatus/@issueType|//dmaddres/issno/@type|//pmStatus/@issueType|//pmaddres/issno/@type"))) {
		xmlChar *t;
		t = xmlNodeGetContent(node);
		free(*isstype);
		*isstype = (char *) t;
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//dmStatus/security|//status/security|//pmStatus/security|//pmstatus/security"))) {
		xmlChar *s;
		s = first_xpath_value(doc, node, BAD_CAST "@securityClassification|@class");
		strcpy(security, (char *) s);
		xmlFree(s);
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//dmStatus/originator|//pmStatus/originator"))) {
		char *c, *n;
		*setorig = true;
		c = (char *) first_xpath_value(doc, node, BAD_CAST "@enterpriseCode");
		n = (char *) first_xpath_value(doc, node, BAD_CAST "enterpriseName");
		free(*orig);
		if (c && n) {
			*orig = malloc(strlen(c) + strlen(n) + 2);
			sprintf(*orig, "%s/%s", c, n);
		} else if (c) {
			*orig = malloc(strlen(c) + 1);
			sprintf(*orig, "%s", c);
		} else if (n) {
			*orig = malloc(strlen(n) + 3);
			sprintf(*orig, "-/%s", n);
		} else {
			*setorig = false;
		}
		xmlFree(c);
		xmlFree(n);
	} else {
		*setorig = false;
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//dmStatus/skillLevel"))) {
		xmlChar *c;
		c = first_xpath_value(doc, node, BAD_CAST "@skillLevelCode");
		free(*skill);
		*skill = (char *) c;
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//dmStatus/remarks|//pmStatus/remarks"))) {
		xmlChar *p;
		p = first_xpath_value(doc, node, BAD_CAST "simplePara");
		free(*remarks);
		*remarks = (char *) p;
	}

	if ((node = first_xpath_node(doc, NULL, BAD_CAST "//dmStatus/applic/displayText|//pmStatus/applic/displayText"))) {
		xmlChar *d;
		d = first_xpath_value(doc, node, BAD_CAST "simplePara");
		strcpy(new_display_text, (char *) d);
		xmlFree(d);
	}
}

/* Create an applicability annotation in a container. */
static xmlNodePtr add_container_applic(xmlNodePtr rag, xmlDocPtr doc, const xmlChar *id)
{
	xmlNodePtr app;

	if (!(app = first_xpath_node(doc, NULL, BAD_CAST "//applic"))) {
		return NULL;
	}

	xmlSetProp(app, BAD_CAST "id", id);

	return xmlAddChild(rag, xmlCopyNode(app, 1));
}

/* Copy the applicability of the referenced DMs of a container into the
 * container itself as inline annotations.
 */
static xmlNodePtr add_container_applics(xmlDocPtr doc, xmlNodePtr content, xmlNodePtr container)
{
	xmlNodePtr rag, refs;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	rag = xmlNewNode(NULL, BAD_CAST "referencedApplicGroup");

	/* Insert the referencedApplicGroup element appropriately. */
	if ((refs = first_xpath_node(doc, content, BAD_CAST "refs"))) {
		xmlAddNextSibling(refs, rag);
	} else {
		add_first_child(content, rag);
	}

	ctx = xmlXPathNewContext(doc);
	xmlXPathSetContextNode(container, ctx);
	obj = xmlXPathEvalExpression(BAD_CAST "refs/dmRef/dmRefIdent", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i, n = 1;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			char path[PATH_MAX];

			if (find_dmod_fname(path, obj->nodesetval->nodeTab[i], false)) {
				xmlDocPtr doc;
				xmlChar id[32];

				if (!(doc = read_xml_doc(path))) {
					continue;
				}

				/* Give each new annotation a sequential ID. */
				xmlStrPrintf(id, 32, "app-%.4d", n);

				if (add_container_applic(rag, doc, id)) {
					xmlSetProp(obj->nodesetval->nodeTab[i]->parent, BAD_CAST "applicRefId", id);
					++n;
				}

				xmlFreeDoc(doc);
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return rag;
}

/* Replace a reference to a container with the appropriate reference within
 * the container for the given applicability. */
static xmlNodePtr resolve_container_ref(xmlNodePtr refident, xmlDocPtr doc, const char *path, xmlNodePtr defs)
{
	xmlNodePtr root, content, container, rag, ref;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	root = xmlDocGetRootElement(doc);

	if (!(content = first_xpath_node(doc, root, BAD_CAST "//content"))) {
		return NULL;
	}

	/* Referenced DM is not a container. */
	if (!(container = first_xpath_node(doc, content, BAD_CAST "container"))) {
		return NULL;
	}

	/* If container does not contain inline annotations, copy them from
	 * the referenced DMs. */
	if (!(rag = first_xpath_node(doc, content, BAD_CAST "//referencedApplicGroup"))) {
		if (!(rag = add_container_applics(doc, content, container))) {
			return NULL;
		}
	}

	/* Filter the container. */
	strip_applic(defs, rag, root);

	ctx = xmlXPathNewContext(doc);
	xmlXPathSetContextNode(container, ctx);

	obj = xmlXPathEvalExpression(BAD_CAST "refs/dmRef", ctx);

	/* If the container does not have exactly one ref after filtering, it
	 * should not be resolved. */
	if (xmlXPathNodeSetIsEmpty(obj->nodesetval) || obj->nodesetval->nodeNr > 1) {
		ref = NULL;
	} else {
		ref = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	/* Replace the ref to the container in the source. */
	if (ref) {
		xmlNodePtr old, new;

		old = refident->parent;
		new = xmlCopyNode(ref, 1);

		xmlUnsetProp(new, BAD_CAST "applicRefId");

		xmlAddNextSibling(old, new);

		xmlUnlinkNode(old);
		xmlFreeNode(old);
	} else if (verbosity > QUIET) {
		fprintf(stderr, S_RESOLVE_CONTAINER, path);
	}

	return ref;
}

/* Resolve references to containers, replacing them with the appropriate
 * reference from within the container for the given applicability.
 */
static void resolve_containers(xmlDocPtr doc, xmlNodePtr defs)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST "//dmRef/dmRefIdent", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			char path[PATH_MAX];

			if (find_dmod_fname(path, obj->nodesetval->nodeTab[i], false)) {
				xmlDocPtr doc;

				if (!(doc = read_xml_doc(path))) {
					continue;
				}

				if (resolve_container_ref(obj->nodesetval->nodeTab[i], doc, path, defs)) {
					obj->nodesetval->nodeTab[i] = NULL;
				}

				xmlFreeDoc(doc);
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

static void add_ct_prop_vals(xmlDocPtr act, xmlDocPtr cct, const xmlChar *id, const xmlChar *type, xmlNodePtr p, xmlNodePtr defs)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr prop_desc = NULL;
	xmlNodePtr prop_vals = NULL;

	if (act && xmlStrcmp(type, BAD_CAST "prodattr") == 0) {
		ctx = xmlXPathNewContext(act);
		xmlXPathRegisterVariable(ctx, BAD_CAST "id", xmlXPathNewString(id));
		obj = xmlXPathEvalExpression(BAD_CAST "//productAttribute[@id=$id]|//prodattr[@id=$id]", ctx);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			prop_desc = obj->nodesetval->nodeTab[0];
			prop_vals = prop_desc;
		}
	} else if (cct && xmlStrcmp(type, BAD_CAST "condition") == 0) {
		xmlChar *condtype;

		ctx = xmlXPathNewContext(cct);
		xmlXPathRegisterVariable(ctx, BAD_CAST "id", xmlXPathNewString(id));
		obj = xmlXPathEvalExpression(BAD_CAST "//cond[@id=$id]/@condTypeRefId|//condition[@id=$id]/@condtyperef", ctx);

		if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			fprintf(stderr, S_NO_CT_PROP, (char *) type, (char *) id);
			return;
		}

		prop_desc = obj->nodesetval->nodeTab[0]->parent;

		condtype = xmlNodeGetContent(obj->nodesetval->nodeTab[0]);
		xmlXPathFreeObject(obj);
		xmlXPathRegisterVariable(ctx, BAD_CAST "id", xmlXPathNewString(condtype));
		xmlFree(condtype);

		obj = xmlXPathEvalExpression(BAD_CAST "//condType[@id=$id]|//conditiontype[@id=$id]", ctx);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			prop_vals = obj->nodesetval->nodeTab[0];
		}
	} else {
		fprintf(stderr, S_NO_CT, (char *) id, (char *) type, xmlStrcmp(type, BAD_CAST "prodattr") == 0 ? "ACT" : "CCT");
		return;
	}

	xmlXPathFreeObject(obj);

	if (prop_desc && prop_vals) {
		xmlChar *s;

		/* Copy information about the property from ACT/CCT. */
		if ((s = first_xpath_value(NULL, prop_vals, BAD_CAST "@valuePattern|@pattern"))) {
			xmlSetProp(p, BAD_CAST "pattern", s);
			xmlFree(s);
		}

		if ((s = first_xpath_value(NULL, prop_desc, BAD_CAST "name"))) {
			xmlNewTextChild(p, p->ns, BAD_CAST "name", s);
			xmlFree(s);
		}

		if ((s = first_xpath_value(NULL, prop_desc, BAD_CAST "descr|description"))) {
			xmlNewTextChild(p, p->ns, BAD_CAST "descr", s);
			xmlFree(s);
		}

		if ((s = first_xpath_value(NULL, prop_desc, BAD_CAST "displayName|displayname"))) {
			xmlNewTextChild(p, p->ns, BAD_CAST "displayName", s);
			xmlFree(s);
		}

		xmlXPathSetContextNode(prop_vals, ctx);
		obj = xmlXPathEvalExpression(BAD_CAST "enumeration|enum", ctx);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			int i;

			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				xmlChar *v, *l, *c = NULL;

				v = first_xpath_value(NULL, obj->nodesetval->nodeTab[i],
					BAD_CAST "@applicPropertyValues|@actvalues");
				l = first_xpath_value(NULL, obj->nodesetval->nodeTab[i],
					BAD_CAST "@enumerationLabel");

				while ((c = BAD_CAST strtok(c ? NULL : (char *) v, "|"))) {
					xmlNodePtr cur;
					bool add = true;

					for (cur = p->children; cur && add; cur = cur->next) {
						xmlChar *cv;
						cv = xmlNodeGetContent(cur);
						add = xmlStrcmp(c, cv) != 0;
						xmlFree(cv);
					}

					if (defs) {
						add &= is_applic(defs, (char *) id, (char *) type, (char *) c, true);
					}

					if (add) {
						xmlNodePtr n;

						n = xmlNewTextChild(p, NULL, BAD_CAST "value", c);

						if (l) {
							xmlSetProp(n, BAD_CAST "label", l);
						}
					}
				}

				xmlFree(v);
				xmlFree(l);
			}
		}

		xmlXPathFreeObject(obj);
	} else {
		fprintf(stderr, S_NO_CT_PROP, (char *) type, (char *) id);
	}

	xmlXPathFreeContext(ctx);
}

/* Add a property used in an object to the properties report. */
static void add_prop(xmlNodePtr object, xmlNodePtr assert, enum listprops listprops, xmlDocPtr act, xmlDocPtr cct, xmlNodePtr defs)
{
	xmlChar *i, *t;
	xmlNodePtr p = NULL, cur;

	i = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyIdent|@actidref");
	t = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyType|@actreftype");

	for (cur = object->children; cur && !p; cur = cur->next) {
		xmlChar *ci, *ct;

		ci = first_xpath_value(NULL, cur, BAD_CAST "@ident");
		ct = first_xpath_value(NULL, cur, BAD_CAST "@type");

		if (xmlStrcmp(i, ci) == 0 && xmlStrcmp(t, ct) == 0) {
			p = cur;
		}

		xmlFree(ci);
		xmlFree(ct);
	}

	if (!p) {
		p = xmlNewChild(object, NULL, BAD_CAST "property", NULL);
		xmlSetProp(p, BAD_CAST "ident", i);
		xmlSetProp(p, BAD_CAST "type", t);

		if (listprops != STANDALONE) {
			add_ct_prop_vals(act, cct, i, t, p, defs);
		}
	}

	if (listprops == STANDALONE) {
		xmlChar *v, *c = NULL;

		v = first_xpath_value(NULL, assert, BAD_CAST "@applicPropertyValues|@actvalues");

		while ((c = BAD_CAST strtok(c ? NULL : (char *) v, "|"))) {
			xmlNodePtr cur;
			bool add = true;

			for (cur = p->children; cur && add; cur = cur->next) {
				xmlChar *cv;

				cv = xmlNodeGetContent(cur);

				add = xmlStrcmp(c, cv) != 0;

				xmlFree(cv);
			}

			if (add) {
				xmlNewTextChild(p, NULL, BAD_CAST "value", c);
			}
		}

		xmlFree(v);
	}

	xmlFree(i);
	xmlFree(t);
}

/* Determine whether a product instance is applicable to an object. */
static bool product_is_applic(xmlNodePtr product, xmlNodePtr defs)
{
	xmlNodePtr evaluate, cur;
	bool is;

	evaluate = xmlNewNode(NULL, BAD_CAST "evaluate");
	xmlSetProp(evaluate, BAD_CAST "andOr", BAD_CAST "and");

	for (cur = product->children; cur; cur = cur->next) {
		xmlChar *i, *t, *v;
		xmlNodePtr a;

		if (cur->type != XML_ELEMENT_NODE) {
			continue;
		}

		i = first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyIdent|@actidref");
		t = first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyType|@actreftype");
		v = first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyValue|@actvalue");

		a = xmlNewNode(evaluate->ns, BAD_CAST "assert");
		xmlSetProp(a, BAD_CAST "applicPropertyIdent", i);
		xmlSetProp(a, BAD_CAST "applicPropertyType", t);
		xmlSetProp(a, BAD_CAST "applicPropertyValues", v);

		xmlAddChild(evaluate, a);

		xmlFree(i);
		xmlFree(t);
		xmlFree(v);
	}

	is = eval_evaluate(defs, evaluate, true);

	xmlFreeNode(evaluate);

	return is;
}

/* Add a PCT product instance to the properties report. */
static void add_product(xmlNodePtr object, xmlNodePtr product, xmlNodePtr defs)
{
	xmlNodePtr p, cur;
	xmlChar *id;

	if (defs && !product_is_applic(product, defs)) {
		return;
	}

	p = xmlNewNode(NULL, BAD_CAST "product");

	if ((id = xmlGetProp(product, BAD_CAST "id"))) {
		xmlSetProp(p, BAD_CAST "ident", id);
		xmlFree(id);
	}

	for (cur = product->children; cur; cur = cur->next) {
		xmlChar *i, *t, *v;
		xmlNodePtr a;

		if (cur->type != XML_ELEMENT_NODE) {
			continue;
		}

		i = first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyIdent|@actidref");
		t = first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyType|@actreftype");
		v = first_xpath_value(NULL, cur, BAD_CAST "@applicPropertyValue|@actvalue");

		a = xmlNewNode(p->ns, BAD_CAST "assign");
		xmlSetProp(a, BAD_CAST "applicPropertyIdent", i);
		xmlSetProp(a, BAD_CAST "applicPropertyType", t);
		xmlSetProp(a, BAD_CAST "applicPropertyValue", v);

		xmlAddChild(p, a);

		xmlFree(i);
		xmlFree(t);
		xmlFree(v);
	}

	xmlAddChild(object, p);
}

/* Add the properties used in an object to the properties report. */
static void add_props(xmlNodePtr report, const char *path, enum listprops listprops, const char *useract, const char *usercct, const char *userpct)
{
	xmlDocPtr doc, act = NULL, cct = NULL, pct = NULL;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr object, defs;

	if (!(doc = read_xml_doc(path))) {
		return;
	}

	if (listprops == APPLIC) {
		defs = xmlNewNode(NULL, BAD_CAST "applic");
		load_applic_from_inst(defs, doc);
	} else {
		defs = NULL;
	}

	object = xmlNewChild(report, NULL, BAD_CAST "object", NULL);
	xmlSetProp(object, BAD_CAST "path", BAD_CAST path);

	if (listprops != STANDALONE) {
		char fname[PATH_MAX];

		if (useract) {
			if ((act = read_xml_doc(useract))) {
				xmlSetProp(object, BAD_CAST "act", BAD_CAST useract);
			} else if (verbosity > QUIET) {
				fprintf(stderr, S_MISSING_ACT, useract);
			}
		} else if (find_act_fname(fname, doc)) {
			if ((act = read_xml_doc(fname))) {
				xmlSetProp(object, BAD_CAST "act", BAD_CAST fname);
			} else if (verbosity > QUIET) {
				fprintf(stderr, S_MISSING_ACT, fname);
			}
		}

		if (usercct) {
			if ((cct = read_xml_doc(usercct))) {
				xmlSetProp(object, BAD_CAST "cct", BAD_CAST usercct);
			} else if (verbosity > QUIET) {
				fprintf(stderr, S_MISSING_CCT, usercct);
			}
		} else if (act && find_cct_fname(fname, act)) {
			if ((cct = read_xml_doc(fname))) {
				xmlSetProp(object, BAD_CAST "cct", BAD_CAST fname);
			} else if (verbosity > QUIET) {
				fprintf(stderr, S_MISSING_CCT, fname);
			}
		}

		if (userpct) {
			if ((pct = read_xml_doc(userpct))) {
				xmlSetProp(object, BAD_CAST "pct", BAD_CAST userpct);
			} else if (verbosity > QUIET) {
				fprintf(stderr, S_MISSING_PCT, userpct);
			}
		} else if (act && find_pct_fname(fname, act)) {
			if ((pct = read_xml_doc(fname))) {
				xmlSetProp(object, BAD_CAST "pct", BAD_CAST fname);
			} else if (verbosity > QUIET) {
				fprintf(stderr, S_MISSING_PCT, fname);
			}
		}
	}

	/* Add properties from DM, ACT and/or CCT. */
	ctx = xmlXPathNewContext(doc);

	/* Use assertions from the whole object applic in standalone mode. */
	if (listprops == STANDALONE) {
		obj = xmlXPathEvalExpression(BAD_CAST "//assert", ctx);
	/* Otherwise, only use assertions from inline annotations. */
	} else {
		obj = xmlXPathEvalExpression(BAD_CAST "(//content|//inlineapplics)//assert", ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			add_prop(object, obj->nodesetval->nodeTab[i], listprops, act, cct, defs);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	/* Add products from PCT. */
	if (pct) {
		ctx = xmlXPathNewContext(pct);
		obj = xmlXPathEval(BAD_CAST "//product", ctx);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			int i;

			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				add_product(object, obj->nodesetval->nodeTab[i], defs);
			}
		}

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);
	}

	xmlFreeNode(defs);

	xmlFreeDoc(act);
	xmlFreeDoc(cct);
	xmlFreeDoc(pct);
	xmlFreeDoc(doc);
}

/* Add the properties used in objects in a list to the properties report. */
static void add_props_list(xmlNodePtr report, const char *fname, enum listprops listprops, const char *useract, const char *usercct, const char *userpct)
{
	FILE *f;
	char path[PATH_MAX];

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			if (verbosity >= NORMAL) {
				fprintf(stderr, S_MISSING_LIST, fname);
			}
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(path, PATH_MAX, f)) {
		strtok(path, "\t\r\n");
		add_props(report, path, listprops, useract, usercct, userpct);
	}

	if (fname) {
		fclose(f);
	}
}

/* Remove an applicability definition from a set. */
static void undefine_applic(xmlNodePtr def, const xmlChar *vals)
{
	xmlChar *cur_val;

	cur_val = xmlGetProp(def, BAD_CAST "applicPropertyValues");

	if (cur_val) {
		if (is_in_set((char *) cur_val, (char *) vals)) {
			xmlUnlinkNode(def);
			xmlFreeNode(def);
		}
	} else {
		xmlNodePtr cur;
		bool match = false;

		for (cur = def->children; cur && !match; cur = cur->next) {
			xmlChar *v;

			v = xmlNodeGetContent(cur);

			if (is_in_set((char *) v, (char *) vals)) {
				xmlUnlinkNode(cur);
				xmlFreeNode(cur);
				match = true;
			}

			xmlFree(v);
		}

		if (!def->children) {
			xmlUnlinkNode(def);
			xmlFreeNode(def);
		}
	}

	xmlFree(cur_val);
}

/* Find an applicability definition in a set. */
static xmlNodePtr get_applic_def(xmlNodePtr defs, const xmlChar *id, const xmlChar *type)
{
	xmlNodePtr cur, node = NULL;

	for (cur = defs->children; cur && !node; cur = cur->next) {
		xmlChar *cur_id, *cur_type;

		cur_id   = xmlGetProp(cur, BAD_CAST "applicPropertyIdent");
		cur_type = xmlGetProp(cur, BAD_CAST "applicPropertyType");

		if (xmlStrcmp(cur_id, id) == 0 && xmlStrcmp(cur_type, type) == 0) {
			node = cur;
		}

		xmlFree(cur_id);
		xmlFree(cur_type);
	}

	return node;
}

/* Determine whether an annotation is a superset of the user-defined applicability. */
static bool annotation_is_superset(xmlNodePtr defs, xmlNodePtr applic, bool simpl)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr defscopy, app;
	bool result;

	defscopy = xmlCopyNode(defs, 1);
	app = xmlCopyNode(applic, 1);

	if (simpl) {
		simpl_applic(defscopy, app, true);
		simpl_applic_evals(app);
	}

	ctx = xmlXPathNewContext(NULL);
	xmlXPathSetContextNode(app, ctx);

	obj = xmlXPathEvalExpression(BAD_CAST ".//assert", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlNodePtr a;
			xmlChar *id, *type;

			id   = first_xpath_value(NULL, obj->nodesetval->nodeTab[i], BAD_CAST "@applicPropertyIdent|@actidref");
			type = first_xpath_value(NULL, obj->nodesetval->nodeTab[i], BAD_CAST "@applicPropertyType|@actreftype");

			if ((a = get_applic_def(defscopy, id, type)) && eval_assert(defscopy, obj->nodesetval->nodeTab[i], true)) {
				xmlChar *vals, *op;

				vals = first_xpath_value(NULL, obj->nodesetval->nodeTab[i],
					BAD_CAST "@applicPropertyValues|@actvalues");
				op   = first_xpath_value(NULL, obj->nodesetval->nodeTab[i],
					BAD_CAST "parent::evaluate/@andOr|parent::evaluate/@operator");

				/* Do not remove assertions from AND evaluations,
				 * unless they are unambiguously true.
				 */
				if (xmlStrcmp(op, BAD_CAST "and") != 0 || eval_assert(defscopy, obj->nodesetval->nodeTab[i], false)) {
					xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
					xmlFreeNode(obj->nodesetval->nodeTab[i]);
					obj->nodesetval->nodeTab[i] = NULL;

					undefine_applic(a, vals);
				}

				xmlFree(vals);
				xmlFree(op);
			}

			xmlFree(id);
			xmlFree(type);
		}
	}

	xmlXPathFreeObject(obj);

	obj = xmlXPathEvalExpression(BAD_CAST ".//assert", ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		result = eval_applic_stmt(defscopy, applic, true);
	} else {
		result = eval_applic_stmt(defscopy, app, false);
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	xmlFreeNode(app);
	xmlFreeNode(defscopy);

	return result;
}

/* Remove annotations which are supersets of the user-defined applicability. */
static xmlNodePtr rem_supersets(xmlNodePtr defs, xmlNodePtr referencedApplicGroup, xmlNodePtr root, bool simpl)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(referencedApplicGroup->doc);
	xmlXPathSetContextNode(referencedApplicGroup, ctx);

	obj = xmlXPathEvalExpression(BAD_CAST "applic", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			if (annotation_is_superset(defs, obj->nodesetval->nodeTab[i], simpl)) {
				xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
				xmlFreeNode(obj->nodesetval->nodeTab[i]);
				obj->nodesetval->nodeTab[i] = NULL;
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	clean_applic(referencedApplicGroup, root);

	if (xmlChildElementCount(referencedApplicGroup) == 0) {
		xmlUnlinkNode(referencedApplicGroup);
		xmlFreeNode(referencedApplicGroup);
		return NULL;
	}

	return referencedApplicGroup;
}

/* List of CSDB objects. */
struct objects {
	char (*paths)[PATH_MAX];
	unsigned count;
	unsigned max;
};

/* Initialize a list of CSDB objects. */
static void init_objects(struct objects *objects)
{
	objects->paths = malloc(PATH_MAX);
	objects->max = 1;
	objects->count = 0;
}

/* Free a list of CSDB objects. */
static void free_objects(struct objects *objects)
{
	free(objects->paths);
}

/* Add a CSDB object to a list. */
static void add_object(struct objects *objects, const char *path)
{
	if (objects->count == objects->max) {
		if (!(objects->paths = realloc(objects->paths, (objects->max *= 2) * PATH_MAX))) {
			if (verbosity > QUIET) {
				fprintf(stderr, E_MAX_OBJECTS);
			}
			exit(EXIT_MAX_OBJECTS);
		}
	}

	strcpy(objects->paths[(objects->count)++], path);
}

/* Find CIRs in directories and add them to the list. */
static void find_cirs(struct objects *cirs, const char *spath)
{
	DIR *dir;
	struct dirent *cur;
	char fpath[PATH_MAX], cpath[PATH_MAX];

	if (verbosity >= DEBUG) {
		fprintf(stderr, I_FIND_CIR, spath);
	}

	if (!(dir = opendir(spath))) {
		return;
	}

	/* Clean up the directory string. */
	if (strcmp(spath, ".") == 0) {
		strcpy(fpath, "");
	} else if (spath[strlen(spath) - 1] != '/') {
		strcpy(fpath, spath);
		strcat(fpath, "/");
	} else {
		strcpy(fpath, spath);
	}

	/* Search for CIRs. */
	while ((cur = readdir(dir))) {
		strcpy(cpath, fpath);
		strcat(cpath, cur->d_name);

		if (recursive_search && isdir(cpath, true)) {
			find_cirs(cirs, cpath);
		} else if (is_dm(cur->d_name) && is_cir(cpath, false)) {
			if (verbosity >= DEBUG) {
				fprintf(stderr, I_FIND_CIR_FOUND, cpath);
			}
			add_object(cirs, cpath);
		}
	}

	closedir(dir);
}

/* Use only the latest issue of a CIR. */
static void extract_latest_cirs(struct objects *cirs)
{
	struct objects latest;

	qsort(cirs->paths, cirs->count, PATH_MAX, compare_basename);

	latest.paths = malloc(cirs->count * PATH_MAX);
	latest.count = extract_latest_csdb_objects(latest.paths, cirs->paths, cirs->count);

	free(cirs->paths);
	cirs->paths = latest.paths;
	cirs->count = latest.count;
}

static void auto_add_cirs(xmlNodePtr cirs)
{
	struct objects files;
	int i;

	init_objects(&files);

	find_cirs(&files, search_dir);
	extract_latest_cirs(&files);

	for (i = 0; i < files.count; ++i) {
		xmlNewChild(cirs, NULL, BAD_CAST "cir", BAD_CAST files.paths[i]);

		if (verbosity >= DEBUG) {
			fprintf(stderr, I_FIND_CIR_ADD, files.paths[i]);
		}
	}

	free_objects(&files);
}

#ifdef LIBS1KD
#define s1kdApplicability xmlNodePtr
typedef enum {
	S1KD_FILTER_DEFAULT,
	S1KD_FILTER_REDUCE,
	S1KD_FILTER_SIMPLIFY,
	S1KD_FILTER_PRUNE
} s1kdFilterMode;

s1kdApplicability s1kdNewApplicability(void)
{
	return xmlNewNode(NULL, BAD_CAST "applic");
}

void s1kdFreeApplicability(s1kdApplicability app)
{
	xmlFreeNode(app);
}

void s1kdAssign(s1kdApplicability app, const xmlChar *ident, const xmlChar *type, const xmlChar *value)
{
	xmlNodePtr a;
	a = xmlNewChild(app, NULL, BAD_CAST "assert", NULL);
	xmlSetProp(a, BAD_CAST "applicPropertyIdent", ident);
	xmlSetProp(a, BAD_CAST "applicPropertyType", type);
	xmlSetProp(a, BAD_CAST "applicPropertyValues", value);
}

xmlDocPtr s1kdDocFilter(const xmlDocPtr doc, s1kdApplicability app, s1kdFilterMode mode)
{
	xmlDocPtr out;
	xmlNodePtr root, referencedApplicGroup;

	out = xmlCopyDoc(doc, 1);

	if (app == NULL || xmlChildElementCount(app) == 0) {
		return out;
	}

	root = xmlDocGetRootElement(out);
	referencedApplicGroup = first_xpath_node(out, NULL, BAD_CAST "//referencedApplicGroup");

	if (xmlChildElementCount(referencedApplicGroup) == 0) {
		return out;
	}

	strip_applic(app, referencedApplicGroup, root);

	if (mode >= S1KD_FILTER_REDUCE) {
		clean_applic_stmts(app, referencedApplicGroup, mode < S1KD_FILTER_PRUNE);

		if (xmlChildElementCount(referencedApplicGroup) == 0) {
			xmlUnlinkNode(referencedApplicGroup);
			xmlFreeNode(referencedApplicGroup);
			referencedApplicGroup = NULL;
		}

		clean_applic(referencedApplicGroup, root);

		if (mode >= S1KD_FILTER_SIMPLIFY && referencedApplicGroup) {
			referencedApplicGroup = simpl_applic_clean(app, referencedApplicGroup, mode == S1KD_FILTER_PRUNE);
		}

		if (mode != S1KD_FILTER_PRUNE && referencedApplicGroup) {
			referencedApplicGroup = rem_supersets(app, referencedApplicGroup, root, mode < S1KD_FILTER_SIMPLIFY);
		}
	}

	return out;
}

int s1kdFilter(const char *object_xml, int object_size, s1kdApplicability app, s1kdFilterMode mode, char **result_xml, int *result_size)
{
	xmlDocPtr doc, res;

	if ((doc = read_xml_mem(object_xml, object_size)) == NULL) {
		return 1;
	}
	if ((res = s1kdDocFilter(doc, app, mode)) == NULL) {
		return 1;
	}
	xmlFreeDoc(doc);

	if (result_xml && result_size) {
		xmlDocDumpMemory(res, (xmlChar **) result_xml, result_size);
	}

	xmlFreeDoc(res);

	return 0;
}
#else
/* Print a usage message */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -A, --simplify                    Simplify and reduce applicability annotations.");
	puts("  -a, --reduce                      Remove applicability annotations which are unambiguously valid or invalid.");
	puts("  -C, --comment <comment>           Add an XML comment to the top of the instance.");
	puts("  -c, --code <DMC>                  The new code of the instance.");
	puts("  -D, --dump <CIR>                  Dump default XSLT for resolving CIR references.");
	puts("  -d, --dir <dir>                   Directory to start searching for referenced data modules in.");
	puts("  -E, --no-extension                Remove extension from instance.");
	puts("  -e, --extension <ext>             Specify an extension on the instance code (DME/PME).");
	puts("  -F, --flatten-alts                Flatten alts elements.");
	puts("  -f, --overwrite                   Force overwriting of files.");
	puts("  -G, --custom-orig <NCAGE/name>    Use custom NCAGE/name for originator.");
	puts("  -g, --set-orig                    Set originator of the instance to identify this tool.");
	puts("  -H, --list-properties <method>    List the applicability properties used in objects.");
	puts("  -h, -?, --help                    Show this help/usage message.");
	puts("  -I, --date <date>                 Set the issue date of the instance (- for current date).");
	puts("  -i, --infoname <infoName>         Give the data module instance a different infoName.");
	puts("  -J, --clean-display-text          Remove display text from simplified annotations (-A).");
	puts("  -j, --clean-ents                  Remove unused external entities (such as ICNs)");
	puts("  -K, --skill-levels <levels>       Filter on the specified skill levels.");
	puts("  -k, --skill <level>               Set the skill level of the instance.");
	puts("  -L, --list                        Treat input as a list of objects.");
	puts("  -l, --language <lang>             Specify the language of the instance.");
	puts("  -m, --remarks <remarks>           Set the remarks for the instance.");
	puts("  -N, --omit-issue                  Omit issue/inwork numbers from automatic filename.");
	puts("  -n, --issue <iss>                 Set the issue and inwork numbers of the instance.");
	puts("  -O, --outdir <dir>                Output instance in dir, automatically named.");
	puts("  -o, --out <file>                  Output instance to file instead of stdout.");
	puts("  -P, --pct <PCT>                   PCT file to read products from.");
	puts("  -p, --product <product>           ID/primary key of a product in the PCT to filter on.");
	puts("  -Q, --resolve-containers          Resolve references to containers.");
	puts("  -q, --quiet                       Quiet mode.");
	puts("  -R, --cir <CIR>                   Resolve externalized items using the given CIR.");
	puts("  -r, --recursive                   Search for referenced data modules recursively.");
	puts("  -S, --no-source-ident             Do not include <sourceDmIdent>/<sourcePmIdent>.");
	puts("  -s, --assign <applic>             An assign in the form of <ident>:<type>=<value>");
	puts("  -T, --tag                         Tag non-applicable elements instead of removing them.");
	puts("  -t, --techname <techName>         Give the instance a different techName/pmTitle.");
	puts("  -U, --security-classes <classes>  Filter on the specified security classes.");
	puts("  -u, --security <sec>              Set the security classification of the instance.");
	puts("  -V, --infoname-variant <variant>  Give the instance a different info name variant.");
	puts("  -v, --verbose                     Verbose output.");
	puts("  -W, --set-applic                  Overwrite whole object applicability.");
	puts("  -w, --whole-objects               Check the status of the whole object.");
	puts("  -X, --comment-xpath <xpath>       XPath where the -C comment will be inserted.");
	puts("  -x, --xsl <XSL>                   Use custom XSLT to resolve CIR references.");
	puts("  -Y, --applic <text>               Set applic for DM with text as the display text.");
	puts("  -y, --update-applic               Set applic for DM based on the user-defined defs.");
	puts("  -Z, --add-required                Fix certain elements automatically after filtering.");
	puts("  -z, --issue-type <type>           Set the issue type of the instance.");
	puts("  -1, --act <file>                  Specify custom ACT.");
	puts("  -2, --cct <file>                  Specify custom CCT.");
	puts("  -3, --no-repository-ident         Do not include <repositorySourceDmIdent>.");
	puts("  -4, --flatten-alts-refs           Flatten alts elements and adjust cross-references to them.");
	puts("  -5, --print                       Print the file name of the instance when -O is used.");
	puts("  -6, --clean-annotations           Remove unused applicability annotations.");
	puts("  -7, --dry-run                     Do not write anything out.");
	puts("  -8, --reapply                     Reapply the source object's applicability.");
	puts("  -9, --prune                       Simplify by removing only false assertions.");
	puts("  -0, --print-non-applic            Print the file names of objects which are not applicable.");
	puts("  -@, --update-instances            Update existing instance objects from their source.");
	puts("  -%, --read-only                   Make instances read-only.");
	puts("  -!, --no-infoname                 Do not include an infoName for the instance.");
	puts("  -~, --dependencies                Add CCT dependencies to annotations.");
	puts("  -^, --remove-deleted              Remove deleted objects/elements.");
	puts("  --version                         Show version information.");
	puts("  <object>...                       Source CSDB object(s)");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Print version information */
static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s, libxslt %s and libexslt %s\n",
		xmlParserVersion, xsltEngineVersion, exsltLibraryVersion);
}

int main(int argc, char **argv)
{
	xmlNodePtr referencedApplicGroup;

	int i, c;

	/* User-defined applicability */
	xmlNodePtr applicability;

	/* Number of user-defined applicability definitions. */
	int napplics = 0;

	char code[256] = "";
	char out[PATH_MAX] = "-";
	bool clean = false;
	bool simpl = false;
	char *tech = NULL;
	char *info = NULL;
	xmlChar *info_name_variant = NULL;
	bool autoname = false;
	char dir[PATH_MAX] = "";
	bool new_applic = false;
	char new_display_text[256] = "";
	char comment_text[256] = "";
	char comment_path[256] = "/";
	char extension[256] = "";
	char language[256] = "";
	bool add_source_ident = true;
	bool add_rep_ident = true;
	bool force_overwrite = false;
	bool use_stdin = false;
	char issinfo[16] = "";
	bool incr_iss = false;
	char secu[4] = "";
	bool wholedm = false;
	char *useract = NULL;
	char *usercct = NULL;
	char *userpct = NULL;
	xmlDocPtr act = NULL;
	xmlDocPtr cct = NULL;
	xmlDocPtr pct = NULL;
	char product[64] = "";
	bool load_applic_per_dm = false;
	bool dmlist = false;
	FILE *list = NULL;
	char issdate[16] = "";
	char issdate_year[5];
	char issdate_month[3];
	char issdate_day[3];
	char *isstype = NULL;
	bool stripext = false;
	bool setorig = false;
	char *origspec = NULL;
	bool flat_alts = false;
	bool fix_alts_refs = false;
	char *remarks = NULL;
	char *skill_codes = NULL;
	char *sec_classes = NULL;
	char *skill = NULL;
	bool combine_applic = true;
	bool clean_ents = false;
	bool autocomp = false;
	bool use_stdout = true;
	bool update_inst = false;
	bool lock = false;
	bool no_info_name = false;
	bool add_deps = false;
	bool print_fnames = false;
	bool rslvcntrs = false;
	bool rem_unused = false;
	bool re_applic = false;
	bool remtrue = true;
	bool find_cir = false;
	bool write_files = true;
	bool print_non_applic = false;
	bool delete = false;

	xmlNodePtr cirs, cir;
	xmlDocPtr def_cir_xsl = NULL;

	xmlDocPtr props_report = NULL;
	enum listprops listprops = STANDALONE;

	const char *sopts = "AaC:c:D:d:Ee:FfG:gh?I:i:JjK:k:Ll:m:Nn:O:o:P:p:QqR:rSs:Tt:U:u:V:vWwX:x:Y:yZz:@%!1:2:34567890~H:^";
	struct option lopts[] = {
		{"version"            , no_argument      , 0, 0},
		{"help"               , no_argument      , 0, 'h'},
		{"reduce"             , no_argument      , 0, 'a'},
		{"simplify"           , no_argument      , 0, 'A'},
		{"code"               , required_argument, 0, 'c'},
		{"comment"            , required_argument, 0, 'C'},
		{"dump"               , required_argument, 0, 'D'},
		{"dir"                , required_argument, 0, 'd'},
		{"no-extension"       , no_argument      , 0, 'E'},
		{"extension"          , required_argument, 0, 'e'},
		{"flatten-alts"       , no_argument      , 0, 'F'},
		{"overwrite"          , no_argument      , 0, 'f'},
		{"set-orig"           , no_argument      , 0, 'g'},
		{"custom-orig"        , required_argument, 0, 'G'},
		{"infoname"           , required_argument, 0, 'i'},
		{"date"               , required_argument, 0, 'I'},
		{"clean-display-text" , no_argument      , 0, 'J'},
		{"clean-ents"         , no_argument      , 0, 'j'},
		{"skill-levels"       , required_argument, 0, 'K'},
		{"skill"              , required_argument, 0, 'k'},
		{"list"               , no_argument      , 0, 'L'},
		{"language"           , required_argument, 0, 'l'},
		{"remarks"            , required_argument, 0, 'm'},
		{"omit-issue"         , no_argument      , 0, 'N'},
		{"issue"              , required_argument, 0, 'n'},
		{"outdir"             , required_argument, 0, 'O'},
		{"out"                , required_argument, 0, 'o'},
		{"pct"                , required_argument, 0, 'P'},
		{"product"            , required_argument, 0, 'p'},
		{"quiet"              , no_argument      , 0, 'q'},
		{"cir"                , required_argument, 0, 'R'},
		{"recursive"          , no_argument      , 0, 'r'},
		{"no-source-ident"    , no_argument      , 0, 'S'},
		{"assign"             , required_argument, 0, 's'},
		{"tag"                , no_argument      , 0, 'T'},
		{"techname"           , required_argument, 0, 't'},
		{"security-classes"   , required_argument, 0, 'U'},
		{"security"           , required_argument, 0, 'u'},
		{"print"              , no_argument      , 0, '5'},
		{"verbose"            , no_argument      , 0, 'v'},
		{"set-applic"         , no_argument      , 0, 'W'},
		{"whole-objects"      , no_argument      , 0, 'w'},
		{"comment-xpath"      , required_argument, 0, 'X'},
		{"xsl"                , required_argument, 0, 'x'},
		{"applic"             , required_argument, 0, 'Y'},
		{"update-applic"      , no_argument      , 0 ,'y'},
		{"add-required"       , no_argument      , 0, 'Z'},
		{"issue-type"         , required_argument, 0, 'z'},
		{"update-instances"   , no_argument      , 0, '@'},
		{"read-only"          , no_argument      , 0, '%'},
		{"no-infoname"        , no_argument      , 0, '!'},
		{"act"                , required_argument, 0, '1'},
		{"cct"                , required_argument, 0, '2'},
		{"dependencies"       , no_argument      , 0, '~'},
		{"resolve-containers" , no_argument      , 0, 'Q'},
		{"no-repository-ident", no_argument      , 0, '3'},
		{"flatten-alts-refs"  , no_argument      , 0, '4'},
		{"list-properties"    , required_argument, 0, 'H'},
		{"infoname-variant"   , required_argument, 0, 'V'},
		{"clean-annotations"  , no_argument      , 0, '6'},
		{"dry-run"            , no_argument      , 0, '7'},
		{"reapply"            , no_argument      , 0, '8'},
		{"prune"              , no_argument      , 0, '9'},
		{"print-non-applic"   , no_argument      , 0, '0'},
		{"remove-deleted"     , no_argument      , 0, '^'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	int err = EXIT_SUCCESS;

	exsltRegisterAll();

	opterr = 1;

	cirs = xmlNewNode(NULL, BAD_CAST "cirs");

	applicability = xmlNewNode(NULL, BAD_CAST "applic");

	search_dir = strdup(".");

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					goto cleanup;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg)
				break;
			case 'a':
				clean = true;
				break;
			case 'A':
				simpl = true;
				break;
			case 'c':
				strncpy(code, optarg, 255);
				break;
			case 'C':
				strncpy(comment_text, optarg, 255);
				break;
			case 'D':
				dump_cir_xsl(optarg);
				goto cleanup;
			case 'd':
				free(search_dir); search_dir = strdup(optarg);
				break;
			case 'E':
				stripext = true;
				break;
			case 'e':
				strncpy(extension, optarg, 255);
				break;
			case 'F':
				flat_alts = true;
				break;
			case 'f':
				force_overwrite = true;
				break;
			case 'g':
				setorig = true;
				break;
			case 'G':
				setorig = true; origspec = strdup(optarg);
				break;
			case 'i':
				info = strdup(optarg);
				break;
			case 'I':
				strncpy(issdate, optarg, 15);
				break;
			case 'J':
				clean_disp_text = true;
				break;
			case 'j':
				clean_ents = true;
				break;
			case 'K':
				skill_codes = strdup(optarg);
				break;
			case 'k':
				skill = strdup(optarg);
				break;
			case 'L':
				dmlist = true;
				break;
			case 'l':
				strncpy(language, optarg, 255);
				break;
			case 'm':
				free(remarks);
				remarks = strdup(optarg);
				break;
			case 'N':
				no_issue = true;
				break;
			case 'n':
				if (strcmp(optarg, "+") == 0) {
					incr_iss = true;
				} else {
					strncpy(issinfo, optarg, 15);
				}
				break;
			case 'O':
				autoname = true;
				strncpy(dir, optarg, PATH_MAX - 1);
				use_stdout = false;
				break;
			case 'o':
				strncpy(out, optarg, PATH_MAX - 1);
				use_stdout = false;
				break;
			case '1':
				useract = strdup(optarg);
				break;
			case '2':
				usercct = strdup(optarg);
				break;
			case 'P':
				userpct = strdup(optarg);
				break;
			case 'p':
				strncpy(product, optarg, 63);
				break;
			case 'Q':
				rslvcntrs = true;
				break;
			case 'q':
				--verbosity;
				break;
			case 'R':
				if (strcmp(optarg, "*") == 0) {
					find_cir = true;
				} else {
					xmlNewChild(cirs, NULL, BAD_CAST "cir", BAD_CAST optarg);
				}
				break;
			case 'r':
				recursive_search = true;
				break;
			case 'S':
				add_source_ident = false;
				break;
			case 's':
				read_applic(applicability, &napplics, optarg);
				break;
			case 'T':
				tag_non_applic = true;
				break;
			case 't':
				tech = strdup(optarg);
				break;
			case 'U':
				sec_classes = strdup(optarg);
				break;
			case 'u':
				strncpy(secu, optarg, 2);
				break;
			case 'V':
				info_name_variant = xmlStrdup(BAD_CAST optarg);
				break;
			case 'v':
				++verbosity;
				break;
			case 'W':
				new_applic = true; combine_applic = false;
				break;
			case 'w':
				wholedm = true;
				break;
			case 'X':
				strncpy(comment_path, optarg, 255);
				break;
			case 'x':
				if (cirs->last) {
					xmlSetProp(cirs->last, BAD_CAST "xsl", BAD_CAST optarg);
				} else if (!def_cir_xsl) {
					def_cir_xsl = read_xml_doc(optarg);
				}
				break;
			case 'y':
				new_applic = true;
				break;
			case 'Y':
				new_applic = true; strncpy(new_display_text, optarg, 255);
				break;
			case 'Z':
				autocomp = true;
				break;
			case 'z':
				isstype = strdup(optarg);
				break;
			case '@':
				update_inst = true;
				load_applic_per_dm = true;
				new_applic = true;
				break;
			case '%':
				lock = true;
				break;
			case '!':
				no_info_name = true;
				break;
			case '~':
				add_deps = true;
				break;
			case '3':
				add_rep_ident = false;
				break;
			case '4':
				flat_alts = true;
				fix_alts_refs = true;
				break;
			case '5':
				print_fnames = true;
				break;
			case '6':
				rem_unused = true;
				break;
			case '7':
				write_files = false;
				force_overwrite = true;
				break;
			case '8':
				re_applic = true;
				break;
			case '9':
				simpl = true;
				remtrue = false;
				break;
			case '0':
				print_non_applic = true;
				wholedm = true;
				break;
			case 'H':
				if (!props_report) {
					props_report = xmlNewDoc(BAD_CAST "1.0");

					if (strcmp(optarg, "all") == 0) {
						listprops = ALL;
					} else if (strcmp(optarg, "applic") == 0) {
						listprops = APPLIC;
					}
				}
				break;
			case '^':
				delete = true;
				break;
			case 'h':
			case '?':
				show_help();
				goto cleanup;
		}
	}

	/* Create a report of all applicability properties. */
	if (props_report) {
		xmlNodePtr properties;

		properties = xmlNewNode(NULL, BAD_CAST "properties");
		xmlDocSetRootElement(props_report, properties);

		switch (listprops) {
			case STANDALONE:
				xmlSetProp(properties, BAD_CAST "method", BAD_CAST "standalone");
				break;
			case ALL:
				xmlSetProp(properties, BAD_CAST "method", BAD_CAST "all");
				break;
			case APPLIC:
				xmlSetProp(properties, BAD_CAST "method", BAD_CAST "applic");
				break;
		}

		if (optind < argc) {
			int i;

			for (i = optind; i < argc; ++i) {
				if (dmlist) {
					add_props_list(properties, argv[i], listprops, useract, usercct, userpct);
				} else {
					add_props(properties, argv[i], listprops, useract, usercct, userpct);
				}
			}
		} else if (dmlist) {
			add_props_list(properties, NULL, listprops, useract, usercct, userpct);
		} else {
			add_props(properties, "-", listprops, useract, usercct, userpct);
		}

		transform_doc(props_report, xsl_sort_props_xsl, xsl_sort_props_xsl_len, NULL);

		save_xml_doc(props_report, "-");

		goto cleanup;
	}

	/* Except when in update mode, only add a source ident by default if
	 * any of the following are changed in the instance:
	 *
	 * - extension
	 * - code
	 * - issue info
	 * - language
	 */
	if (!update_inst && strcmp(extension, "") == 0 && strcmp(code, "") == 0 && strcmp(issinfo, "") == 0 && strcmp(language, "") == 0) {
		add_source_ident = false;
	}

	if (find_cir) {
		auto_add_cirs(cirs);
	}

	if (optind >= argc) {
		if (dmlist) {
			list = stdin;
		} else {
			use_stdin = true;
		}
	}

	/* If the -O option is given, create the directory if it does not
	 * exist.
	 *
	 * Fail if an existing non-directory file is specified.
	 *
	 * Ignore if the -7 option is given and no files will be written.
	 */
	if (autoname && write_files) {
		if (access(dir, F_OK) == -1) {
			int err;

			#ifdef _WIN32
				err = mkdir(dir);
			#else
				err = mkdir(dir, S_IRWXU);
			#endif

			if (err) {
				if (verbosity > QUIET) {
					fprintf(stderr, S_MKDIR_FAILED, dir);
				}
				exit(EXIT_BAD_ARG);
			}
		} else if (!isdir(dir, false)) {
			if (verbosity > QUIET) {
				fprintf(stderr, S_NOT_DIR, dir);
			}
			exit(EXIT_BAD_ARG);
		}
	}

	if (useract) {
		if (!(act = read_xml_doc(useract))) {
			if (verbosity > QUIET) {
				fprintf(stderr, S_MISSING_ACT, useract);
			}
			exit(EXIT_MISSING_FILE);
		}
	}
	if (usercct) {
		if (!(cct = read_xml_doc(usercct))) {
			if (verbosity > QUIET) {
				fprintf(stderr, S_MISSING_CCT, usercct);
			}
			exit(EXIT_MISSING_FILE);
		}
	}
	if (userpct) {
		if (!(pct = read_xml_doc(userpct))) {
			if (verbosity > QUIET) {
				fprintf(stderr, S_MISSING_PCT, userpct);
			}
			exit(EXIT_MISSING_FILE);
		}
	}

	if (strcmp(product, "") != 0) {
		/* If a PCT filename is specified with -P, use that for all data
		 * modules and ignore their individual ACT->PCT refs. */
		if (pct) {
			load_applic_from_pct(applicability, &napplics, pct, userpct, product);
		/* Otherwise the PCT must be loaded separately for each data
		 * module, since they may reference different ones. */
		} else {
			load_applic_per_dm = true;
		}
	}

	/* Determine the issue date from the -I option. */
	if (strcmp(issdate, "-") == 0) {
		time_t now;
		struct tm *local;

		time(&now);
		local = localtime(&now);

		if (snprintf(issdate_year, 5, "%d", local->tm_year + 1900) < 0 ||
		    snprintf(issdate_month, 3, "%.2d", local->tm_mon + 1) < 0 ||
		    snprintf(issdate_day, 3, "%.2d", local->tm_mday) < 0)
			exit(EXIT_BAD_DATE);
	} else if (strcmp(issdate, "") != 0) {
		if (sscanf(issdate, "%4s-%2s-%2s", issdate_year, issdate_month, issdate_day) != 3) {
			if (verbosity > QUIET) {
				fprintf(stderr, S_BAD_DATE, issdate);
			}
			exit(EXIT_BAD_DATE);
		}
	}

	i = optind;

	while (1) {
		xmlDocPtr doc;
		char src[PATH_MAX] = "";
		char *inst_src = NULL;

		if (dmlist) {
			if (!list && !(list = fopen(argv[i++], "r"))) {
				if (verbosity > QUIET) {
					fprintf(stderr, S_MISSING_LIST, argv[i - 1]);
				}
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
		} else {
			break;
		}

		if (!use_stdin && access(src, F_OK) == -1) {
			if (verbosity > QUIET) {
				fprintf(stderr, S_MISSING_OBJECT, src);
			}
			exit(EXIT_MISSING_FILE);
		}

		/* Get the source from the sourceDmIdent/sourcePmIdent of the object. */
		if (update_inst) {
			int e;
			xmlDocPtr inst = NULL;

			inst_src = strdup(src);

			if ((e = find_source(src, &inst))) {
				if (e == 1) {
					if (verbosity > QUIET) {
						fprintf(stderr, S_MISSING_SOURCE, src);
					}
					err = EXIT_MISSING_SOURCE;
				}
				xmlFreeDoc(inst);
				free(inst_src);

				if (use_stdin) {
					break;
				} else {
					continue;
				}
			}

			if (verbosity >= VERBOSE) {
				fprintf(stderr, I_UPDATE_INST, inst_src, src);
			}

			load_applic_from_inst(applicability, inst);
			load_skill_from_inst(inst, &skill_codes);
			load_sec_from_inst(inst, &sec_classes);
			load_metadata_from_inst(inst,
				extension,
				code,
				language,
				issinfo,
				incr_iss,
				&tech,
				&info,
				&info_name_variant,
				&no_info_name,
				&isstype,
				secu,
				&origspec,
				&setorig,
				&skill,
				&remarks,
				&new_applic,
				new_display_text);
			add_cirs_from_inst(inst, cirs);

			xmlFreeDoc(inst);
		}

		if ((doc = read_xml_doc(src))) {
			if (verbosity >= VERBOSE && !update_inst) {
				if (autoname) {
					fprintf(stderr, I_CUSTOMIZE_DIR, src, dir);
				} else {
					fprintf(stderr, I_CUSTOMIZE, src);
				}
			}

			/* Load the ACT to find the CCT and/or PCT. */
			if (!useract && ((add_deps && !usercct) || (strcmp(product, "") != 0 && !userpct))) {
				char fname[PATH_MAX];
				if (find_act_fname(fname, doc)) {
					act = read_xml_doc(fname);
				}
			}

			/* Add dependency tests from the CCT. */
			if (add_deps) {
				if (usercct) {
					add_cct_depends(doc, cct, NULL);
				} else if (act) {
					char fname[PATH_MAX];
					if (find_cct_fname(fname, act)) {
						if ((cct = read_xml_doc(fname))) {
							add_cct_depends(doc, cct, NULL);
							xmlFreeDoc(cct);
						}
					}
				}
			}

			/* Load the applic assigns from the PCT data module referenced
			 * by the ACT data module referenced by this data module.
			 */
			if (!userpct && act && strcmp(product, "") != 0) {
				char fname[PATH_MAX];
				if (find_pct_fname(fname, act)) {
					if ((pct = read_xml_doc(fname))) {
						load_applic_from_pct(applicability, &napplics, pct, fname, product);
						xmlFreeDoc(pct);
					}
				}
			}

			if (act && !useract) {
				xmlFreeDoc(act);
				act = NULL;
			}

			if (re_applic) {
				load_applic_from_inst(applicability, doc);
			}

			if (!wholedm || create_instance(doc, applicability, skill_codes, sec_classes, delete)) {
				bool ispm;
				xmlNodePtr root;

				if (add_source_ident) {
					add_source(doc);
				}

				/* Updating an instance, so reset the source
				 * to the instance in case we want to
				 * overwrite it.
				 */
				if (update_inst) {
					strcpy(src, inst_src);
					free(inst_src);
				}

				root = xmlDocGetRootElement(doc);
				ispm = xmlStrcmp(root->name, BAD_CAST "pm") == 0;

				for (cir = cirs->children; cir; cir = cir->next) {
					char *cirdocfname = (char *) xmlNodeGetContent(cir);
					char *cirxsl = (char *) xmlGetProp(cir, BAD_CAST "xsl");

					if (access(cirdocfname, F_OK) == -1) {
						if (verbosity > QUIET) {
							fprintf(stderr, S_MISSING_CIR, cirdocfname);
						}
						continue;
					}

					if (ispm) {
						root = undepend_cir(doc, applicability, cirdocfname, false, cirxsl, def_cir_xsl);
					} else {
						root = undepend_cir(doc, applicability, cirdocfname, add_rep_ident, cirxsl, def_cir_xsl);
					}

					xmlFree(cirdocfname);
					xmlFree(cirxsl);
				}

				referencedApplicGroup = first_xpath_node(doc, NULL,
					BAD_CAST "//referencedApplicGroup|//inlineapplics");

				/* In -Q mode, add inline applicability to container DMs. */
				if (rslvcntrs && !referencedApplicGroup) {
					xmlNodePtr content;
					if ((content = first_xpath_node(doc, root, BAD_CAST "//content"))) {
						xmlNodePtr container;
						if ((container = first_xpath_node(doc, content, BAD_CAST "container"))) {
							referencedApplicGroup = add_container_applics(doc, content, container);
						}
					}
				}

				if (referencedApplicGroup) {
					if (applicability->children) {
						strip_applic(applicability, referencedApplicGroup, root);

						if (clean || simpl) {
							clean_applic_stmts(applicability, referencedApplicGroup, remtrue);

							if (xmlChildElementCount(referencedApplicGroup) == 0) {
								xmlUnlinkNode(referencedApplicGroup);
								xmlFreeNode(referencedApplicGroup);
								referencedApplicGroup = NULL;
							}

							clean_applic(referencedApplicGroup, root);

							if (simpl && referencedApplicGroup) {
								referencedApplicGroup = simpl_applic_clean(applicability, referencedApplicGroup, remtrue);
							}

							if (remtrue && referencedApplicGroup) {
								referencedApplicGroup = rem_supersets(applicability, referencedApplicGroup, root, !simpl);
							}
						}
					}

					if (rem_unused && referencedApplicGroup) {
						referencedApplicGroup = rem_unused_annotations(doc, referencedApplicGroup);
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
				/* Remove elements marked as "delete". */
				if (delete) {
					rem_delete_elems(doc);
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

				set_title(doc, tech, info, info_name_variant, no_info_name);

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
						simpl_whole_applic(applicability, doc, remtrue);
					}

					set_applic(doc, applicability, napplics, new_display_text, combine_applic);
				}

				if (strcmp(issinfo, "") != 0) {
					set_issue(doc, issinfo, incr_iss);
				}

				if (strcmp(issdate, "") != 0) {
					set_issue_date(doc, issdate_year, issdate_month, issdate_day);
				}

				if (isstype) {
					set_issue_type(doc, isstype);
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
					flatten_alts(doc, fix_alts_refs);
				}

				if (clean_ents) {
					clean_entities(doc);
				}

				if (autocomp) {
					autocomplete(doc);
				}

				/* Resolve references to containers. */
				if (rslvcntrs) {
					resolve_containers(doc, applicability);
				}

				if (use_stdout && force_overwrite) {
					strcpy(out, src);
				} else if (autoname && !auto_name(out, src, doc, dir, no_issue)) {
					if (verbosity > QUIET) {
						fprintf(stderr, S_BAD_TYPE);
					}
					exit(EXIT_BAD_XML);
				}

				if (!use_stdout && access(out, F_OK) == 0 && !force_overwrite) {
					if (verbosity > QUIET) {
						fprintf(stderr, S_FILE_EXISTS, out);
					}
				} else {
					if (write_files) {
						save_xml_doc(doc, out);

						if (lock) {
							mkreadonly(out);
						}
					}

					if (print_fnames) {
						puts(out);
					}
				}
			} else {
				if (verbosity >= VERBOSE) {
					fprintf(stderr, I_NON_APPLIC, src);
				}

				if (print_non_applic) {
					puts(src);
				}
			}

			/* The ACT/PCT may be different for the next DM, so these
			 * assigns must be cleared. Those directly set with -s will
			 * carry over. */
			if (load_applic_per_dm) {
				clear_perdm_applic(applicability, &napplics);
			}

			xmlFreeDoc(doc);
		} else if (autoname) { /* Copy the non-XML object to the directory. */
			char *base;

			if (verbosity >= VERBOSE) {
				fprintf(stderr, I_COPY, src, dir);
			}

			base = basename(src);
			if (snprintf(out, PATH_MAX, "%s/%s", dir, base) < 0) {
				exit(EXIT_BAD_ARG);
			}

			if (access(out, F_OK) == 0 && !force_overwrite) {
				if (verbosity > QUIET) {
					fprintf(stderr, S_FILE_EXISTS, out);
				}
			} else {
				if (write_files) {
					copy(src, out);

					if (lock) {
						mkreadonly(out);
					}
				}

				if (print_fnames) {
					puts(out);
				}
			}
		} else {
			if (verbosity > QUIET) {
				fprintf(stderr, S_BAD_XML, use_stdin ? "stdin" : src);
			}
			exit(EXIT_BAD_XML);
		}

		if (use_stdin) {
			break;
		}
	}

cleanup:
	if (useract) {
		xmlFreeDoc(act);
		free(useract);
	}
	if (usercct) {
		xmlFreeDoc(cct);
		free(usercct);
	}
	if (userpct) {
		xmlFreeDoc(pct);
		free(userpct);
	}

	free(origspec);
	free(remarks);
	free(skill);
	free(skill_codes);
	free(sec_classes);
	free(search_dir);
	free(tech);
	free(info);
	free(isstype);
	xmlFree(info_name_variant);
	xmlFreeNode(cirs);
	xmlFreeDoc(def_cir_xsl);
	xmlFreeNode(applicability);
	xmlFreeDoc(props_report);
	xsltCleanupGlobals();
	xmlCleanupParser();

	return err;
}
#endif
