#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/debugXML.h>
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-refs"
#define VERSION "2.9.1"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define SUCC_PREFIX PROG_NAME ": SUCCESS: "
#define FAIL_PREFIX PROG_NAME ": FAILURE: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"
#define E_OUT_OF_MEMORY ERR_PREFIX "Too many files in recursive listing.\n"
#define E_BAD_STDIN ERR_PREFIX "stdin does not contain valid XML.\n"

#define S_UNMATCHED SUCC_PREFIX "No unmatched references in %s\n"
#define F_UNMATCHED FAIL_PREFIX "Unmatched references in %s\n"

#define EXIT_UNMATCHED_REF 1
#define EXIT_OUT_OF_MEMORY 2
#define EXIT_BAD_STDIN 3

/* List only references found in the content section. */
bool contentOnly = false;

/* Do not display errors. */
bool quiet = false;

/* Assume objects were created with the -N option. */
bool noIssue = false;

/* Show unmatched references instead of an error. */
bool showUnmatched = false;

/* Show references which are matched in the filesystem. */
bool showMatched = true;

/* Recurse in to child directories. */
bool recursive = false;

/* Directory to start search in. */
char *directory;

/* Only match against code, ignore language/issue info even if present. */
bool fullMatch = true;

/* Include the source object as a reference. */
bool listSrc = false;

/* List references in matched objects recursively. */
bool listRecursively = false;

/* Update the address information of references. */
bool updateRefs = false;

/* Update the ident and address info from the latest matched issue. */
bool updateRefIdent = false;

/* Overwrite updated input objects. */
bool overwriteUpdated = false;

/* Remove unmatched references from the input objects. */
bool tagUnmatched = false;

/* When listing references recursively, keep track of files which have already 
 * been listed to avoid loops.
 */
char (*listedFiles)[PATH_MAX] = NULL;
int numListedFiles = 0;
long unsigned maxListedFiles = 1;

/* Possible objects to list references to. */
#define SHOW_COM 0x01 /* Comments */
#define SHOW_DMC 0x02 /* Data modules */
#define SHOW_ICN 0x04 /* ICNs */
#define SHOW_PMC 0x08 /* Publication modules */
#define SHOW_EPR 0x10 /* External publications */

/* Which types of object references will be listed. */
int showObjects = 0;

/* Write valid CSDB objects to stdout. */
bool outputTree = false;

/* Verbose output. */
bool verbose = false;

/* External pub list. */
xmlDocPtr externalPubs = NULL;

/* Return the first node matching an XPath expression. */
xmlNodePtr firstXPathNode(xmlDocPtr doc, xmlNodePtr root, const xmlChar *path)
{
	xmlNodePtr node;

	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	if (!doc && root)
		doc = root->doc;
	
	if (!doc)
		return NULL;
	
	ctx = xmlXPathNewContext(doc);

	if (root)
		ctx->node = root;

	obj = xmlXPathEvalExpression(BAD_CAST path, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		node = NULL;
	} else {
		node = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return node;
}

/* Return the value of the first node matching an XPath expression. */
xmlChar *firstXPathValue(xmlDocPtr doc, xmlNodePtr root, const xmlChar *path)
{
	return xmlNodeGetContent(firstXPathNode(doc, root, path));
}

/* Print a reference which is matched in the filesystem. */
void printMatched(xmlNodePtr node, const char *src, const char *ref)
{
	puts(ref);
}
void printMatchedSrc(xmlNodePtr node, const char *src, const char *ref)
{
	printf("%s: %s\n", src, ref);
}
void printMatchedSrcLine(xmlNodePtr node, const char *src, const char *ref)
{
	printf("%s (%ld): %s\n", src, xmlGetLineNo(node), ref);
}
void printMatchedXml(xmlNodePtr node, const char *src, const char *ref)
{
	xmlChar *s, *r, *xpath;
	xmlDocPtr doc;

	if (!node) {
		return;
	}

	s = xmlEncodeEntitiesReentrant(node->doc, BAD_CAST src);
	r = xmlEncodeEntitiesReentrant(node->doc, BAD_CAST ref);
	xpath = xpath_of(node);

	printf("<found>");

	printf("<ref>");
	doc = xmlNewDoc(BAD_CAST "1.0");
	if (node->type == XML_ATTRIBUTE_NODE) {
		xmlDocSetRootElement(doc, xmlCopyNode(node->parent, 1));
	} else {
		xmlDocSetRootElement(doc, xmlCopyNode(node, 1));
	}
	xmlShellPrintNode(xmlDocGetRootElement(doc));
	xmlFreeDoc(doc);
	printf("</ref>");

	printf("<source line=\"%ld\" xpath=\"%s\">%s</source>", xmlGetLineNo(node), xpath, s);
	printf("<filename>%s</filename>", r);

	printf("</found>");

	xmlFree(s);
	xmlFree(r);
	xmlFree(xpath);
}

/* Print an error for references which are unmatched. */
void printUnmatched(xmlNodePtr node, const char *src, const char *ref)
{
	fprintf(stderr, ERR_PREFIX "Unmatched reference: %s\n", ref);
}
void printUnmatchedSrc(xmlNodePtr node, const char *src, const char *ref)
{
	fprintf(stderr, ERR_PREFIX "%s: Unmatched reference: %s\n", src, ref);
}
void printUnmatchedSrcLine(xmlNodePtr node, const char *src, const char *ref)
{
	fprintf(stderr, ERR_PREFIX "%s (%ld): Unmatched reference: %s\n", src, xmlGetLineNo(node), ref);
}
void printUnmatchedXml(xmlNodePtr node, const char *src, const char *ref)
{
	xmlChar *s, *r, *xpath;
	xmlDocPtr doc;

	s = xmlEncodeEntitiesReentrant(node->doc, BAD_CAST src);
	r = xmlEncodeEntitiesReentrant(node->doc, BAD_CAST ref);
	xpath = xpath_of(node);

	printf("<missing>");

	printf("<ref>");
	doc = xmlNewDoc(BAD_CAST "1.0");
	if (node->type == XML_ATTRIBUTE_NODE) {
		xmlDocSetRootElement(doc, xmlCopyNode(node->parent, 1));
	} else {
		xmlDocSetRootElement(doc, xmlCopyNode(node, 1));
	}
	xmlShellPrintNode(xmlDocGetRootElement(doc));
	xmlFreeDoc(doc);
	printf("</ref>");

	printf("<source line=\"%ld\" xpath=\"%s\">%s</source>", xmlGetLineNo(node), xpath, s);
	printf("<code>%s</code>", r);

	printf("</missing>");

	xmlFree(s);
	xmlFree(r);
	xmlFree(xpath);
}

void (*printMatchedFn)(xmlNodePtr, const char *, const char *) = printMatched;
void (*printUnmatchedFn)(xmlNodePtr, const char *, const char*) = printUnmatched;

/* Get the DMC as a string from a dmRef. */
void getDmCode(char *dst, xmlNodePtr dmRef)
{
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

	xmlNodePtr identExtension, dmCode, issueInfo, language;

	identExtension = firstXPathNode(NULL, dmRef, BAD_CAST "dmRefIdent/identExtension|dmcextension");
	dmCode = firstXPathNode(NULL, dmRef, BAD_CAST "dmRefIdent/dmCode|dmc/avee|avee");

	if (fullMatch) {
		issueInfo = firstXPathNode(NULL, dmRef, BAD_CAST "dmRefIdent/issueInfo|issno");
		language  = firstXPathNode(NULL, dmRef, BAD_CAST "dmRefIdent/language|language");
	} else {
		issueInfo = NULL;
		language = NULL;
	}

	strcpy(dst, "");

	if (identExtension) {
		char *extensionProducer, *extensionCode;

		extensionProducer = (char *) firstXPathValue(NULL, identExtension, BAD_CAST "@extensionProducer|dmeproducer");
		extensionCode     = (char *) firstXPathValue(NULL, identExtension, BAD_CAST "@extensionCode|dmecode");

		strcat(dst, "DME-");

		strcat(dst, extensionProducer);
		strcat(dst, "-");
		strcat(dst, extensionCode);
		strcat(dst, "-");

		xmlFree(extensionProducer);
		xmlFree(extensionCode);
	} else {
		strcat(dst, "DMC-");
	}

	modelIdentCode     = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@modelIdentCode|modelic");
	systemDiffCode     = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@systemDiffCode|sdc");
	systemCode         = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@systemCode|chapnum");
	subSystemCode      = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@subSystemCode|section");
	subSubSystemCode   = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@subSubSystemCode|subsect");
	assyCode           = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@assyCode|subject");
	disassyCode        = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@disassyCode|discode");
	disassyCodeVariant = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@disassyCodeVariant|discodev");
	infoCode           = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@infoCode|incode");
	infoCodeVariant    = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@infoCodeVariant|incodev");
	itemLocationCode   = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@itemLocationCode|itemloc");
	learnCode          = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@learnCode");
	learnEventCode     = (char *) firstXPathValue(NULL, dmCode, BAD_CAST "@learnEventCode");

	if (modelIdentCode) {
		strcat(dst, modelIdentCode);
		strcat(dst, "-");
		strcat(dst, systemDiffCode);
		strcat(dst, "-");
		strcat(dst, systemCode);
		strcat(dst, "-");
		strcat(dst, subSystemCode);
		strcat(dst, subSubSystemCode);
		strcat(dst, "-");
		strcat(dst, assyCode);
		strcat(dst, "-");
		strcat(dst, disassyCode);
		strcat(dst, disassyCodeVariant);
		strcat(dst, "-");
		strcat(dst, infoCode);
		strcat(dst, infoCodeVariant);
		strcat(dst, "-");
		strcat(dst, itemLocationCode);

		if (learnCode) {
			strcat(dst, "-");
			strcat(dst, learnCode);
			strcat(dst, learnEventCode);
		}
	}

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
	xmlFree(learnCode);
	xmlFree(learnEventCode);

	if (fullMatch) {
		if (!noIssue) {
			if (issueInfo) {
				char *issueNumber, *inWork;

				issueNumber = (char *) firstXPathValue(NULL, issueInfo, BAD_CAST "@issueNumber|@issno");
				inWork      = (char *) firstXPathValue(NULL, issueInfo, BAD_CAST "@inWork|@inwork");

				if (!inWork) {
					inWork = strdup("00");
				}

				strcat(dst, "_");
				strcat(dst, issueNumber);
				strcat(dst, "-");
				strcat(dst, inWork);

				xmlFree(issueNumber);
				xmlFree(inWork);
			} else if (language) {
				strcat(dst, "_\?\?\?-\?\?");
			}
		}

		if (language) {
			char *languageIsoCode, *countryIsoCode;

			languageIsoCode = (char *) firstXPathValue(NULL, language, BAD_CAST "@languageIsoCode|@language");
			countryIsoCode  = (char *) firstXPathValue(NULL, language, BAD_CAST "@countryIsoCode|@country");

			uppercase(languageIsoCode);

			strcat(dst, "_");
			strcat(dst, languageIsoCode);
			strcat(dst, "-");
			strcat(dst, countryIsoCode);

			xmlFree(languageIsoCode);
			xmlFree(countryIsoCode);
		}
	}
}

/* Get the PMC as a string from a pmRef. */
void getPmCode(char *dst, xmlNodePtr pmRef)
{
	xmlNodePtr identExtension, pmCode, issueInfo, language;

	char *modelIdentCode;
	char *pmIssuer;
	char *pmNumber;
	char *pmVolume;

	identExtension = firstXPathNode(NULL, pmRef, BAD_CAST "pmRefIdent/identExtension");
	pmCode         = firstXPathNode(NULL, pmRef, BAD_CAST "pmRefIdent/pmCode|pmc");

	if (fullMatch) {
		issueInfo = firstXPathNode(NULL, pmRef, BAD_CAST "pmRefIdent/issueInfo|issno");
		language  = firstXPathNode(NULL, pmRef, BAD_CAST "pmRefIdent/language|language");
	} else {
		issueInfo = NULL;
		language = NULL;
	}

	strcpy(dst, "");

	if (identExtension) {
		char *extensionProducer, *extensionCode;

		extensionProducer = (char *) xmlGetProp(identExtension, BAD_CAST "extensionProducer");
		extensionCode     = (char *) xmlGetProp(identExtension, BAD_CAST "extensionCode");

		strcat(dst, "PME-");

		strcat(dst, extensionProducer);
		strcat(dst, "-");
		strcat(dst, extensionCode);
		strcat(dst, "-");

		xmlFree(extensionProducer);
		xmlFree(extensionCode);
	} else {
		strcat(dst, "PMC-");
	}

	modelIdentCode = (char *) firstXPathValue(NULL, pmCode, BAD_CAST "@modelIdentCode|modelic");
	pmIssuer       = (char *) firstXPathValue(NULL, pmCode, BAD_CAST "@pmIssuer|pmissuer");
	pmNumber       = (char *) firstXPathValue(NULL, pmCode, BAD_CAST "@pmNumber|pmnumber");
	pmVolume       = (char *) firstXPathValue(NULL, pmCode, BAD_CAST "@pmVolume|pmvolume");

	strcat(dst, modelIdentCode);
	strcat(dst, "-");
	strcat(dst, pmIssuer);
	strcat(dst, "-");
	strcat(dst, pmNumber);
	strcat(dst, "-");
	strcat(dst, pmVolume);

	xmlFree(modelIdentCode);
	xmlFree(pmIssuer);
	xmlFree(pmNumber);
	xmlFree(pmVolume);

	if (fullMatch) {
		if (!noIssue) {
			if (issueInfo) {
				char *issueNumber, *inWork;

				issueNumber = (char *) firstXPathValue(NULL, issueInfo, BAD_CAST "@issueNumber|@issno");
				inWork      = (char *) firstXPathValue(NULL, issueInfo, BAD_CAST "@inWork|@inwork");

				strcat(dst, "_");
				strcat(dst, issueNumber);
				strcat(dst, "-");
				strcat(dst, inWork);

				xmlFree(issueNumber);
				xmlFree(inWork);
			} else if (language) {
				strcat(dst, "_\?\?\?-\?\?");
			}
		}

		if (language) {
			char *languageIsoCode, *countryIsoCode;

			languageIsoCode = (char *) firstXPathValue(NULL, language, BAD_CAST "@languageIsoCode|@language");
			countryIsoCode  = (char *) firstXPathValue(NULL, language, BAD_CAST "@countryIsoCode|@country");

			uppercase(languageIsoCode);

			strcat(dst, "_");
			strcat(dst, languageIsoCode);
			strcat(dst, "-");
			strcat(dst, countryIsoCode);

			xmlFree(languageIsoCode);
			xmlFree(countryIsoCode);
		}
	}
}

/* Get the ICN as a string from an ICN reference. */
void getICN(char *dst, xmlNodePtr ref)
{
	char *icn;
	icn = (char *) xmlGetProp(ref, BAD_CAST "infoEntityRefIdent");
	strcpy(dst, icn);
	xmlFree(icn);
}

/* Get the ICN as a string from an ICN entity reference. */
void getICNAttr(char *dst, xmlNodePtr ref)
{
	xmlChar *icn;
	xmlEntityPtr ent;
	icn = xmlNodeGetContent(ref);
	if ((ent = xmlGetDocEntity(ref->doc, icn)) && ent->URI) {
		char uri[PATH_MAX], *base;
		strcpy(uri, (char *) ent->URI);
		base = basename(uri);
		strcpy(dst, base);
	} else {
		strcpy(dst, (char *) icn);
	}

	/* Remove issue number when not doing a full match. */
	if (!fullMatch) {
		*(strrchr(dst, '-') - 3) = 0;
	}

	xmlFree(icn);
}

/* Get the comment code as a string from a commentRef. */
void getComCode(char *dst, xmlNodePtr ref)
{
	xmlNodePtr commentCode, language;

	char *modelIdentCode;
	char *senderIdent;
	char *yearOfDataIssue;
	char *seqNumber;
	char *commentType;

	commentCode = firstXPathNode(NULL, ref, BAD_CAST "commentRefIdent/commentCode");

	if (fullMatch) {
		language = firstXPathNode(NULL, ref, BAD_CAST "commentRefIdent/language");
	} else {
		language = NULL;
	}

	modelIdentCode  = (char *) xmlGetProp(commentCode, BAD_CAST "modelIdentCode");
	senderIdent     = (char *) xmlGetProp(commentCode, BAD_CAST "senderIdent");
	yearOfDataIssue = (char *) xmlGetProp(commentCode, BAD_CAST "yearOfDataIssue");
	seqNumber       = (char *) xmlGetProp(commentCode, BAD_CAST "seqNumber");
	commentType     = (char *) xmlGetProp(commentCode, BAD_CAST "commentType");

	strcpy(dst, "COM-");
	strcat(dst, modelIdentCode);
	strcat(dst, "-");
	strcat(dst, senderIdent);
	strcat(dst, "-");
	strcat(dst, yearOfDataIssue);
	strcat(dst, "-");
	strcat(dst, seqNumber);
	strcat(dst, "-");
	strcat(dst, commentType);

	xmlFree(modelIdentCode);
	xmlFree(senderIdent);
	xmlFree(yearOfDataIssue);
	xmlFree(seqNumber);
	xmlFree(commentType);

	if (fullMatch) {
		if (language) {
			char *languageIsoCode, *countryIsoCode;

			languageIsoCode = (char *) xmlGetProp(language, BAD_CAST "languageIsoCode");
			countryIsoCode  = (char *) xmlGetProp(language, BAD_CAST "countryIsoCode");

			uppercase(languageIsoCode);

			strcat(dst, "_");
			strcat(dst, languageIsoCode);
			strcat(dst, "-");
			strcat(dst, countryIsoCode);

			xmlFree(languageIsoCode);
			xmlFree(countryIsoCode);
		}
	}
}

/* Get the external pub code as a string from an externalPubRef. */
void getExternalPubCode(char *dst, xmlNodePtr ref)
{
	xmlNodePtr externalPubCode;
	char *code;

	externalPubCode = firstXPathNode(NULL, ref,
		BAD_CAST "externalPubRefIdent/externalPubCode|externalPubRefIdent/externalPubTitle|pubcode");

	if (externalPubCode) {
		code = (char *) xmlNodeGetContent(externalPubCode);
	} else {
		code = (char *) xmlNodeGetContent(ref);
	}

	strcpy(dst, code);

	xmlFree(code);
}

/* Get filename from DDN item. */
void getDispatchFileName(char *dst, xmlNodePtr ref)
{
	char *fname;
	fname = (char *) xmlNodeGetContent(ref);
	strcpy(dst, fname);
	xmlFree(fname);
}

/* Update address items using the matched referenced object. */
void updateRef(xmlNodePtr *refptr, const char *src, const char *code, const char *fname)
{
	xmlNodePtr ref = *refptr;

	if (xmlStrcmp(ref->name, BAD_CAST "dmRef") == 0) {
		xmlDocPtr doc;
		xmlNodePtr dmRefAddressItems, dmTitle;
		xmlChar *techName, *infoName;

		if (!(doc = read_xml_doc(fname))) {
			return;
		}

		if (updateRefIdent) {
			xmlNodePtr dmRefIdent, refIssueInfo, refLanguage, issueInfo, language;

			dmRefIdent   = firstXPathNode(NULL, ref, BAD_CAST "dmRefIdent");
			refIssueInfo = firstXPathNode(NULL, dmRefIdent, BAD_CAST "issueInfo");
			refLanguage  = firstXPathNode(NULL, dmRefIdent, BAD_CAST "language");

			issueInfo = xmlCopyNode(firstXPathNode(doc, NULL, BAD_CAST "//issueInfo|//issno"), 1);
			language  = xmlCopyNode(firstXPathNode(doc, NULL, BAD_CAST "//language"), 1);

			/* 4.x references a 3.0 DM */
			if (xmlStrcmp(issueInfo->name, BAD_CAST "issno") == 0) {
				xmlNodeSetName(issueInfo, BAD_CAST "issueInfo");
				xmlNodeSetName((xmlNodePtr) xmlHasProp(issueInfo,
					BAD_CAST "issno"),
					BAD_CAST "issueNumber");
				if (xmlHasProp(issueInfo, BAD_CAST "inwork")) {
					xmlNodeSetName((xmlNodePtr) xmlHasProp(issueInfo,
						BAD_CAST "inwork"),
						BAD_CAST "inWork");
				} else {
					xmlSetProp(issueInfo, BAD_CAST "inWork", BAD_CAST "00");
				}
				xmlUnsetProp(issueInfo, BAD_CAST "type");
				xmlNodeSetName((xmlNodePtr) xmlHasProp(language,
					BAD_CAST "language"),
					BAD_CAST "languageIsoCode");
				xmlNodeSetName((xmlNodePtr) xmlHasProp(language,
					BAD_CAST "country"),
					BAD_CAST "countryIsoCode");
			}

			if (refIssueInfo) {
				xmlUnlinkNode(refIssueInfo);
				xmlFreeNode(refIssueInfo);
			}
			if (refLanguage) {
				xmlUnlinkNode(refLanguage);
				xmlFreeNode(refLanguage);
			}

			xmlAddChild(dmRefIdent, issueInfo);
			xmlAddChild(dmRefIdent, language);
		}

		if ((dmRefAddressItems = firstXPathNode(NULL, ref, BAD_CAST "dmRefAddressItems"))) {
			xmlUnlinkNode(dmRefAddressItems);
			xmlFreeNode(dmRefAddressItems);
		}
		dmRefAddressItems = xmlNewChild(ref, NULL, BAD_CAST "dmRefAddressItems", NULL);

		techName = firstXPathValue(doc, NULL, BAD_CAST "//techName|//techname");
		infoName = firstXPathValue(doc, NULL, BAD_CAST "//infoName|//infoname");

		dmTitle = xmlNewChild(dmRefAddressItems, NULL, BAD_CAST "dmTitle", NULL);
		xmlNewTextChild(dmTitle, NULL, BAD_CAST "techName", techName);
		if (infoName) {
			xmlNewTextChild(dmTitle, NULL, BAD_CAST "infoName", infoName);
		}

		xmlFree(techName);
		xmlFree(infoName);

		if (updateRefIdent) {
			xmlNodePtr issueDate;

			issueDate = xmlCopyNode(firstXPathNode(doc, NULL, BAD_CAST "//issueDate|//issdate"), 1);

			if (xmlStrcmp(issueDate->name, BAD_CAST "issdate")) {
				xmlNodeSetName(issueDate, BAD_CAST "issueDate");
			}

			xmlAddChild(dmRefAddressItems, issueDate);
		}

		xmlFreeDoc(doc);
	} else if (xmlStrcmp(ref->name, BAD_CAST "pmRef") == 0) {
		xmlDocPtr doc;
		xmlNodePtr pmRefAddressItems;
		xmlChar *pmTitle;

		if (!(doc = read_xml_doc(fname))) {
			return;
		}

		if (updateRefIdent) {
			xmlNodePtr pmRefIdent, refIssueInfo, refLanguage, issueInfo, language;

			pmRefIdent   = firstXPathNode(NULL, ref, BAD_CAST "pmRefIdent");
			refIssueInfo = firstXPathNode(NULL, pmRefIdent, BAD_CAST "issueInfo");
			refLanguage  = firstXPathNode(NULL, pmRefIdent, BAD_CAST "language");
			issueInfo    = xmlCopyNode(firstXPathNode(doc, NULL, BAD_CAST "//issueInfo|//issno"), 1);
			language     = xmlCopyNode(firstXPathNode(doc, NULL, BAD_CAST "//language"), 1);

			/* 4.x references a 3.0 DM */
			if (xmlStrcmp(issueInfo->name, BAD_CAST "issno") == 0) {
				xmlNodeSetName(issueInfo, BAD_CAST "issueInfo");
				xmlNodeSetName((xmlNodePtr) xmlHasProp(issueInfo,
					BAD_CAST "issno"),
					BAD_CAST "issueNumber");
				if (xmlHasProp(issueInfo, BAD_CAST "inwork")) {
					xmlNodeSetName((xmlNodePtr) xmlHasProp(issueInfo,
						BAD_CAST "inwork"),
						BAD_CAST "inWork");
				} else {
					xmlSetProp(issueInfo, BAD_CAST "inWork", BAD_CAST "00");
				}
				xmlUnsetProp(issueInfo, BAD_CAST "type");
				xmlNodeSetName((xmlNodePtr) xmlHasProp(language,
					BAD_CAST "language"),
					BAD_CAST "languageIsoCode");
				xmlNodeSetName((xmlNodePtr) xmlHasProp(language,
					BAD_CAST "country"),
					BAD_CAST "countryIsoCode");
			}

			if (refIssueInfo) {
				xmlUnlinkNode(refIssueInfo);
				xmlFreeNode(refIssueInfo);
			}
			if (refLanguage) {
				xmlUnlinkNode(refLanguage);
				xmlFreeNode(refLanguage);
			}

			xmlAddChild(pmRefIdent, issueInfo);
			xmlAddChild(pmRefIdent, language);
		}

		if ((pmRefAddressItems = firstXPathNode(NULL, ref, BAD_CAST "pmRefAddressItems"))) {
			xmlUnlinkNode(pmRefAddressItems);
			xmlFreeNode(pmRefAddressItems);
		}
		pmRefAddressItems = xmlNewChild(ref, NULL, BAD_CAST "pmRefAddressItems", NULL);

		pmTitle = firstXPathValue(doc, NULL, BAD_CAST "//pmTitle|//pmtitle");

		xmlNewTextChild(pmRefAddressItems, NULL, BAD_CAST "pmTitle", pmTitle);

		xmlFree(pmTitle);

		if (updateRefIdent) {
			xmlNodePtr issueDate;

			issueDate = xmlCopyNode(firstXPathNode(doc, NULL, BAD_CAST "//issueDate|//issdate"), 1);

			if (xmlStrcmp(issueDate->name, BAD_CAST "issdate")) {
				xmlNodeSetName(issueDate, BAD_CAST "issueDate");
			}

			xmlAddChild(pmRefAddressItems, issueDate);
		}

		xmlFreeDoc(doc);
	} else if (xmlStrcmp(ref->name, BAD_CAST "refdm") == 0) {
		xmlDocPtr doc;
		xmlNodePtr oldtitle, newtitle;
		xmlChar *techname, *infoname;

		if (!(doc = read_xml_doc(fname))) {
			return;
		}

		if (updateRefIdent) {
			xmlNodePtr oldissno, newissno, oldlanguage, newlanguage;

			newissno    = xmlCopyNode(firstXPathNode(doc, NULL, BAD_CAST "//issueInfo|//issno"), 1);
			newlanguage = xmlCopyNode(firstXPathNode(doc, NULL, BAD_CAST "//language"), 1);

			/* 3.0 references a 4.x DM */
			if (xmlStrcmp(newissno->name, BAD_CAST "issueInfo") == 0) {
				xmlNodeSetName(newissno, BAD_CAST "issno");
				xmlNodeSetName((xmlNodePtr) xmlHasProp(newissno,
					BAD_CAST "issueNumber"),
					BAD_CAST "issno");
				xmlNodeSetName((xmlNodePtr) xmlHasProp(newissno,
					BAD_CAST "inWork"),
					BAD_CAST "inwork");
				xmlNodeSetName((xmlNodePtr) xmlHasProp(newlanguage,
					BAD_CAST "languageIsoCode"),
					BAD_CAST "language");
				xmlNodeSetName((xmlNodePtr) xmlHasProp(newlanguage,
					BAD_CAST "countryIsoCode"),
					BAD_CAST "country");
			}

			if ((oldissno = firstXPathNode(NULL, ref, BAD_CAST "issno"))) {
				newissno = xmlAddNextSibling(oldissno, newissno);
			} else {
				newissno = xmlAddNextSibling(firstXPathNode(NULL, ref, BAD_CAST "avee"), newissno);
			}
			xmlUnlinkNode(oldissno);
			xmlFreeNode(oldissno);

			if ((oldlanguage = firstXPathNode(NULL, ref, BAD_CAST "language"))) {
				newlanguage = xmlAddNextSibling(oldlanguage, newlanguage);
			} else {
				newlanguage = xmlAddNextSibling(firstXPathNode(NULL, ref, BAD_CAST "issno"), newlanguage);
			}
			xmlUnlinkNode(oldlanguage);
			xmlFreeNode(oldlanguage);
		}

		techname = firstXPathValue(doc, NULL, BAD_CAST "//techName|//techname");
		infoname = firstXPathValue(doc, NULL, BAD_CAST "//infoName|//infoname");

		newtitle = xmlNewNode(NULL, BAD_CAST "dmtitle");

		if ((oldtitle = firstXPathNode(NULL, ref, BAD_CAST "dmtitle"))) {
			newtitle = xmlAddNextSibling(oldtitle, newtitle);
		} else {
			newtitle = xmlAddNextSibling(firstXPathNode(NULL, ref, BAD_CAST "(avee|issno)[last()]"), newtitle);
		}

		xmlNewTextChild(newtitle, NULL, BAD_CAST "techname", techname);
		if (infoname) {
			xmlNewTextChild(newtitle, NULL, BAD_CAST "infoname", infoname);
		}

		xmlFree(techname);
		xmlFree(infoname);

		xmlUnlinkNode(oldtitle);
		xmlFreeNode(oldtitle);
		xmlFreeDoc(doc);
	} else if (xmlStrcmp(ref->name, BAD_CAST "infoEntityIdent") == 0) {
		xmlChar *icn;
		xmlEntityPtr e;

		/* Remove old ICN entity. */
		icn = xmlNodeGetContent(ref);
		if ((e = xmlGetDocEntity(ref->doc, icn))) {
			xmlUnlinkNode((xmlNodePtr) e);
			xmlFreeEntity(e);
		}
		xmlFree(icn);

		/* Add new ICN entity. */
		e = add_icn(ref->doc, fname, false);
		xmlNodeSetContent(ref, e->name);
	} else if (xmlStrcmp(ref->name, BAD_CAST "externalPubRef") == 0) {
		xmlNodePtr new;
		xmlChar xpath[512];

		xmlStrPrintf(xpath, 512, "//externalPubRef[externalPubRefIdent/externalPubCode='%s']", code);

		if (!(new = firstXPathNode(externalPubs, NULL, xpath))) {
			return;
		}

		xmlAddNextSibling(ref, xmlCopyNode(new, 1));

		xmlUnlinkNode(ref);
		xmlFreeNode(ref);
		*refptr = NULL;
	}
}

/* Tag unmatched references in the source object. */
void tagUnmatchedRef(xmlNodePtr ref)
{
	add_first_child(ref, xmlNewPI(BAD_CAST "unmatched", NULL));
}

int listReferences(const char *path);

/* Print a reference found in an object. */
int printReference(xmlNodePtr *refptr, const char *src)
{
	char code[PATH_MAX];
	char fname[PATH_MAX];
	xmlNodePtr ref = *refptr;

	if ((showObjects & SHOW_DMC) == SHOW_DMC &&
	    (xmlStrcmp(ref->name, BAD_CAST "dmRef") == 0 ||
	     xmlStrcmp(ref->name, BAD_CAST "refdm") == 0 ||
	     xmlStrcmp(ref->name, BAD_CAST "addresdm") == 0))
		getDmCode(code, ref);
	else if ((showObjects & SHOW_PMC) == SHOW_PMC &&
		 (xmlStrcmp(ref->name, BAD_CAST "pmRef") == 0 ||
	          xmlStrcmp(ref->name, BAD_CAST "refpm") == 0))
		getPmCode(code, ref);
	else if ((showObjects & SHOW_ICN) == SHOW_ICN &&
	         (xmlStrcmp(ref->name, BAD_CAST "infoEntityRef") == 0))
		getICN(code, ref);
	else if ((showObjects & SHOW_COM) == SHOW_COM && (xmlStrcmp(ref->name, BAD_CAST "commentRef") == 0))
		getComCode(code, ref);
	else if ((showObjects & SHOW_ICN) == SHOW_ICN &&
		 (xmlStrcmp(ref->name, BAD_CAST "infoEntityIdent") == 0 ||
	          xmlStrcmp(ref->name, BAD_CAST "boardno") == 0))
		getICNAttr(code, ref);
	else if ((showObjects & SHOW_EPR) == SHOW_EPR &&
	         (xmlStrcmp(ref->name, BAD_CAST "externalPubRef") == 0 ||
		  xmlStrcmp(ref->name, BAD_CAST "reftp") == 0))
		getExternalPubCode(code, ref);
	else if (xmlStrcmp(ref->name, BAD_CAST "dispatchFileName") == 0 ||
	         xmlStrcmp(ref->name, BAD_CAST "ddnfilen") == 0)
		getDispatchFileName(code, ref);
	else
		return 0;

	if (find_csdb_object(fname, directory, code, NULL, recursive)) {
		if (updateRefs) {
			updateRef(refptr, src, code, fname);
		} else if (!tagUnmatched) {
			if (showMatched) {
				printMatchedFn(ref, src, fname);
			}

			if (listRecursively) {
				listReferences(fname);
			}
		}
		return 0;
	} else if (tagUnmatched) {
		tagUnmatchedRef(ref);
	} else if (showUnmatched) {
		printMatchedFn(ref, src, code);
	} else if (!quiet) {
		printUnmatchedFn(ref, src, code);
	}

	/* Update metadata for unmatched external pubs. */
	if (updateRefs && externalPubs && xmlStrcmp(ref->name, BAD_CAST "externalPubRef") == 0) {
		updateRef(refptr, src, code, fname);
	}

	return 1;
}

/* Check if a file has already been listed when listing recursively. */
bool listedFile(const char *path)
{
	int i;
	for (i = 0; i < numListedFiles; ++i) {
		if (strcmp(listedFiles[i], path) == 0) {
			return true;
		}
	}
	return false;
}

/* Add a file to the list of files already checked. */
void addFile(const char *path)
{
	if (!listedFiles || numListedFiles == maxListedFiles) {
		if (!(listedFiles = realloc(listedFiles, (maxListedFiles *= 2) * PATH_MAX))) {
			fprintf(stderr, E_OUT_OF_MEMORY);
			exit(EXIT_OUT_OF_MEMORY);
		}
	}

	strcpy(listedFiles[numListedFiles++], path);
}

/* List all references in the given object. */
int listReferences(const char *path)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int unmatched = 0;

	if (listRecursively) {
		if (listedFile(path)) {
			return 0;
		}

		addFile(path);
	}

	if (listSrc) {
		printMatched(NULL, path, path);
	}

	if (!(doc = read_xml_doc(path))) {
		if (strcmp(path, "-") == 0) {
			fprintf(stderr, E_BAD_STDIN);
			exit(EXIT_BAD_STDIN);
		}

		return 0;
	}

	ctx = xmlXPathNewContext(doc);

	if (contentOnly)
		ctx->node = firstXPathNode(doc, NULL,
			BAD_CAST "//content|//dmlContent|//dml|//ddnContent|//delivlst");
	else
		ctx->node = xmlDocGetRootElement(doc);

	obj = xmlXPathEvalExpression(BAD_CAST ".//dmRef|.//refdm|.//addresdm|.//pmRef|.//refpm|.//infoEntityRef|//@infoEntityIdent|//@boardno|.//commentRef|.//externalPubRef|.//reftp|.//dispatchFileName|.//ddnfilen", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			unmatched += printReference(&(obj->nodesetval->nodeTab[i]), path);
		}
	}

	/* Write valid CSDB object to stdout. */
	if (outputTree && !unmatched) {
		save_xml_doc(doc, "-");
	}

	/* If the given object was modified by updating matched refs or
	 * tagging unmatched refs, write the changes.
	 */
	if (updateRefs || tagUnmatched) {
		if (overwriteUpdated) {
			save_xml_doc(doc, path);
		} else {
			save_xml_doc(doc, "-");
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
	xmlFreeDoc(doc);

	if (verbose) {
		fprintf(stderr, unmatched ? F_UNMATCHED : S_UNMATCHED, path);
	}

	return unmatched;
}

/* Parse a list of filenames as input. */
int listReferencesInList(const char *path)
{
	FILE *f;
	char line[PATH_MAX];
	int unmatched = 0;

	if (path) {
		if (!(f = fopen(path, "r"))) {
			fprintf(stderr, E_BAD_LIST, path);
			return 0;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");
		unmatched += listReferences(line);
	}

	if (path) {
		fclose(f);
	}

	return unmatched;
}

/* Display the usage message. */
void showHelp(void)
{
	puts("Usage: s1kd-refs [-aCcDEFfGIilNnoPqrsUuvXxh?] [-d <dir>] [-e <file>] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -a, --all                  Print unmatched codes.");
	puts("  -C, --com                  List commnent references.");
	puts("  -c, --content              Only show references in content section.");
	puts("  -D, --dm                   List data module references.");
	puts("  -d, --dir                  Directory to search for matches in.");
	puts("  -E, --epr                  List external pub refs.");
	puts("  -e, --externalpubs <file>  Use custom .externalpubs file.");
	puts("  -F, --overwrite            Overwrite updated (-U) or tagged (-X) objects.");
	puts("  -f, --filename             Print the source filename for each reference.");
	puts("  -G, --icn                  List ICN references.");
	puts("  -h, -?, --help             Show help/usage message.");
	puts("  -I, --update-issue         Update references to point to the latest matched object.");
	puts("  -i, --ignore-issue         Ignore issue info/language when matching.");
	puts("  -l, --list                 Treat input as list of CSDB objects.");
	puts("  -N, --omit-issue           Assume filenames omit issue info.");
	puts("  -n, --lineno               Print the source filename and line number for each reference.");
	puts("  -o, --output-valid         Output valid CSDB objects to stdout.");
	puts("  -P, --pm                   List publication module references.");
	puts("  -q, --quiet                Quiet mode.");
	puts("  -R, --recursively          List references in matched objects recursively.");
	puts("  -r, --recursive            Search for matches in directories recursively.");
	puts("  -s, --include-src          Include the source object as a reference.");
	puts("  -U, --update               Update address items in matched references.");
	puts("  -u, --unmatched            Show only unmatched references.");
	puts("  -v, --verbose              Verbose output.");
	puts("  -X, --tag-unmatched        Tag unmatched references.");
	puts("  -x, --xml                  Output XML report.");
	puts("  --version                  Show version information.");
	puts("  <object>                   CSDB object to list references in.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Display version information. */
void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int main(int argc, char **argv)
{
	int i, unmatched;

	bool isList = false;
	bool xmlOutput = false;
	bool inclSrcFname = false;
	bool inclLineNum = false;
	char extpubsFname[PATH_MAX] = "";

	const char *sopts = "qcNaFflUuCDGPRrd:IinEXxsove:h?";
	struct option lopts[] = {
		{"version"      , no_argument      , 0, 0},
		{"help"         , no_argument      , 0, 'h'},
		{"quiet"        , no_argument      , 0, 'q'},
		{"content"      , no_argument      , 0, 'c'},
		{"externalpubs" , required_argument, 0, 'e'},
		{"omit-issue"   , no_argument      , 0, 'N'},
		{"all"          , no_argument      , 0, 'a'},
		{"overwrite"    , no_argument      , 0, 'F'},
		{"filename"     , no_argument      , 0, 'f'},
		{"list"         , no_argument      , 0, 'l'},
		{"update"       , no_argument      , 0, 'U'},
		{"unmatched"    , no_argument      , 0, 'u'},
		{"com"          , no_argument      , 0, 'C'},
		{"dm"           , no_argument      , 0, 'D'},
		{"icn"          , no_argument      , 0, 'G'},
		{"pm"           , no_argument      , 0, 'P'},
		{"recursively"  , no_argument      , 0, 'R'},
		{"recursive"    , no_argument      , 0, 'r'},
		{"dir"          , required_argument, 0, 'd'},
		{"update-issue" , no_argument      , 0, 'I'},
		{"ignore-issue" , no_argument      , 0, 'i'},
		{"lineno"       , no_argument      , 0, 'n'},
		{"epr"          , no_argument      , 0, 'E'},
		{"tag-unmatched", no_argument      , 0, 'X'},
		{"xml"          , no_argument      , 0, 'x'},
		{"include-src"  , no_argument      , 0, 's'},
		{"output-valid" , no_argument      , 0, 'o'},
		{"verbose"      , no_argument      , 0, 'v'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	directory = strdup(".");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'q':
				quiet = true;
				break;
			case 'c':
				contentOnly = true;
				break;
			case 'e':
				strncpy(extpubsFname, optarg, PATH_MAX - 1);
				break;
			case 'N':
				noIssue = true;
				break;
			case 'a':
				showUnmatched = true;
				break;
			case 'F':
				overwriteUpdated = true;
				break;
			case 'f':
				inclSrcFname = true;
				break;
			case 'l':
				isList = true;
				break;
			case 'U':
				updateRefs = true;
				break;
			case 'u':
				showMatched = false;
				break;
			case 'C':
				showObjects |= SHOW_COM;
				break;
			case 'D':
				showObjects |= SHOW_DMC;
				break;
			case 'G':
				showObjects |= SHOW_ICN;
				break;
			case 'P':
				showObjects |= SHOW_PMC;
				break;
			case 'R':
				listRecursively = true;
				break;
			case 'r':
				recursive = true;
				break;
			case 'd':
				free(directory);
				directory = strdup(optarg);
				break;
			case 'I':
				updateRefs = true;
				fullMatch = false;
				updateRefIdent = true;
				break;
			case 'i':
				fullMatch = false;
				break;
			case 'n':
				inclSrcFname = true;
				inclLineNum = true;
				break;
			case 'E':
				showObjects |= SHOW_EPR;
				break;
			case 'X':
				tagUnmatched = true;
				break;
			case 'x':
				xmlOutput = true;
				break;
			case 's':
				listSrc = true;
				break;
			case 'o':
				outputTree = true;
				break;
			case 'v':
				verbose = true;
				break;
			case 'h':
			case '?':
				showHelp();
				return 0;
		}
	}

	/* If none of -CDEGP are given, show all types of objects. */
	if (!showObjects) {
		showObjects = SHOW_COM | SHOW_DMC | SHOW_ICN | SHOW_PMC | SHOW_EPR;
	}

	/* Load .externalpubs config file. */
	if (strcmp(extpubsFname, "") != 0 || find_config(extpubsFname, DEFAULT_EXTPUBS_FNAME)) {
		externalPubs = read_xml_doc(extpubsFname);
	}

	/* Set the functions for printing matched/unmatched refs. */
	if (xmlOutput) {
		printMatchedFn = printMatchedXml;
		printUnmatchedFn = printUnmatchedXml;
		puts("<?xml version=\"1.0\"?>");
		printf("<results>");
	} else if (inclSrcFname) {
		if (inclLineNum) {
			printMatchedFn = printMatchedSrcLine;
			printUnmatchedFn = printUnmatchedSrcLine;
		} else {
			printMatchedFn = printMatchedSrc;
			printUnmatchedFn = printUnmatchedSrc;
		}
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (isList) {
				unmatched = listReferencesInList(argv[i]);
			} else {
				unmatched = listReferences(argv[i]);
			}
		}
	} else if (isList) {
		unmatched = listReferencesInList(NULL);
	} else {
		unmatched = listReferences("-");
	}

	if (xmlOutput) {
		printf("</results>\n");
	}

	free(directory);
	free(listedFiles);
	xmlFreeDoc(externalPubs);
	xmlCleanupParser();

	return unmatched > 0 ? EXIT_UNMATCHED_REF : EXIT_SUCCESS;
}
