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
#include <libxml/xpathInternals.h>
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-refs"
#define VERSION "4.5.1"

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
static bool contentOnly = false;

/* Do not display errors. */
static bool quiet = false;

/* Assume objects were created with the -N option. */
static bool noIssue = false;

/* Show unmatched references instead of an error. */
static bool showUnmatched = false;

/* Show references which are matched in the filesystem. */
static bool showMatched = true;

/* Recurse in to child directories. */
static bool recursive = false;

/* Directory to start search in. */
static char *directory;

/* Ignore issue info when matching. */
static bool ignoreIss = false;

/* Include the source object as a reference. */
static bool listSrc = false;

/* List references in matched objects recursively. */
static bool listRecursively = false;

/* Update the address information of references. */
static bool updateRefs = false;

/* Update the ident and address info from the latest matched issue. */
static bool updateRefIdent = false;

/* Overwrite updated input objects. */
static bool overwriteUpdated = false;

/* Remove unmatched references from the input objects. */
static bool tagUnmatched = false;

static char *execStr = NULL;

/* When listing references recursively, keep track of files which have already
 * been listed to avoid loops.
 */
static char (*listedFiles)[PATH_MAX] = NULL;
static int numListedFiles = 0;
static long unsigned maxListedFiles = 1;

/* Possible objects to list references to. */
#define SHOW_COM 0x001 /* Comments */
#define SHOW_DMC 0x002 /* Data modules */
#define SHOW_ICN 0x004 /* ICNs */
#define SHOW_PMC 0x008 /* Publication modules */
#define SHOW_EPR 0x010 /* External publications */
#define SHOW_HOT 0x020 /* Hotspots */
#define SHOW_FRG 0x040 /* Fragments */
#define SHOW_DML 0x080 /* DMLs */
#define SHOW_SMC 0x100 /* SCORM content packages */

/* All possible objects. */
#define SHOW_ALL SHOW_COM | SHOW_DMC | SHOW_ICN | SHOW_PMC | SHOW_EPR | SHOW_HOT | SHOW_FRG | SHOW_DML | SHOW_SMC

/* All objects relevant to -w mode. */
#define SHOW_WHERE_USED SHOW_COM | SHOW_DMC | SHOW_PMC | SHOW_DML | SHOW_SMC | SHOW_ICN

/* Write valid CSDB objects to stdout. */
static bool outputTree = false;

/* Verbose output. */
static bool verbose = false;

/* External pub list. */
static xmlDocPtr externalPubs = NULL;

/* Allow matching of filenames which only start with the code.
 *
 * If this is false, then the filenames must match the exact code up to the
 * extension (last .)
 *
 * For example, with loose matching a code of ABC would match a file named
 * ABC_001.PDF, while without loose matching it will not.
 */
static bool looseMatch = true;

/* XPath for matching hotspots. */
#define DEFAULT_HOTSPOT_XPATH BAD_CAST "/X3D//*[@DEF=$id]|//*[@id=$id]"
static xmlChar *hotspotXPath = NULL;
static xmlNodePtr hotspotNs = NULL;

/* Delimiter for the format string. */
#define FMTSTR_DELIM '%'
/* Custom format for printed references. */
static char *printFormat = NULL;

/* Return the first node matching an XPath expression. */
static xmlNodePtr firstXPathNode(xmlDocPtr doc, xmlNodePtr root, const xmlChar *path)
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
static xmlChar *firstXPathValue(xmlDocPtr doc, xmlNodePtr root, const xmlChar *path)
{
	return xmlNodeGetContent(firstXPathNode(doc, root, path));
}

/* Process and print info based on a format string. */
static void processFormatStr(FILE *f, xmlNodePtr node, const char *src, const char *ref)
{
	int i;

	for (i = 0; printFormat[i]; ++i) {
		if (printFormat[i] == FMTSTR_DELIM) {
			if (printFormat[i + 1] == FMTSTR_DELIM) {
				fputc(FMTSTR_DELIM, f);
				++i;
			} else {
				const char *k, *e;
				int n;

				k = printFormat + i + 1;
				e = strchr(k, FMTSTR_DELIM);
				if (!e) break;
				n = e - k;

				if (strncmp(k, "src", n) == 0) {
					fprintf(f, "%s", src);
				} else if (strncmp(k, "ref", n) == 0) {
					fprintf(f, "%s", ref);
				} else if (strncmp(k, "line", n) == 0) {
					fprintf(f, "%ld", xmlGetLineNo(node));
				} else if (strncmp(k, "xpath", n) == 0) {
					xmlChar *xpath = xpath_of(node);
					fprintf(f, "%s", (char *) xpath);
					xmlFree(xpath);
				}

				i += n + 1;
			}
		} else if (printFormat[i] == '\\') {
			switch (printFormat[i + 1]) {
				case 'n': fputc('\n', f); i++; break;
				case 't': fputc('\t', f); i++; break;
				case '0': fputc('\0', f); i++; break;
				default:  fputc(printFormat[i], f);
			}
		} else {
			fputc(printFormat[i], f);
		}
	}

	fputc('\n', f);
}

/* Print a reference which is matched in the filesystem. */
static void printMatched(xmlNodePtr node, const char *src, const char *ref)
{
	puts(ref);
}
static void printMatchedSrc(xmlNodePtr node, const char *src, const char *ref)
{
	printf("%s: %s\n", src, ref);
}
static void printMatchedSrcLine(xmlNodePtr node, const char *src, const char *ref)
{
	printf("%s (%ld): %s\n", src, xmlGetLineNo(node), ref);
}
static void printMatchedXml(xmlNodePtr node, const char *src, const char *ref)
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
static void printMatchedWhereUsed(xmlNodePtr node, const char *src, const char *ref)
{
	printf("%s\n", src);
}
static void printMatchedCustom(xmlNodePtr node, const char *src, const char *ref)
{
	processFormatStr(stdout, node, src, ref);
}

static void execMatched(xmlNodePtr node, const char *src, const char *ref)
{
	execfile(execStr, ref);
}

/* Print an error for references which are unmatched. */
static void printUnmatched(xmlNodePtr node, const char *src, const char *ref)
{
	fprintf(stderr, ERR_PREFIX "Unmatched reference: %s\n", ref);
}
static void printUnmatchedSrc(xmlNodePtr node, const char *src, const char *ref)
{
	fprintf(stderr, ERR_PREFIX "%s: Unmatched reference: %s\n", src, ref);
}
static void printUnmatchedSrcLine(xmlNodePtr node, const char *src, const char *ref)
{
	fprintf(stderr, ERR_PREFIX "%s (%ld): Unmatched reference: %s\n", src, xmlGetLineNo(node), ref);
}
static void printUnmatchedXml(xmlNodePtr node, const char *src, const char *ref)
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
static void printUnmatchedCustom(xmlNodePtr node, const char *src, const char *ref)
{
	fputs(ERR_PREFIX "Unmatched reference: ", stderr);
	processFormatStr(stderr, node, src, ref);
}

static void (*printMatchedFn)(xmlNodePtr, const char *, const char *) = printMatched;
static void (*printUnmatchedFn)(xmlNodePtr, const char *, const char*) = printUnmatched;

static bool exact_match(char *dst, const char *code)
{
	char *s, *base;
	bool match;

	s = strdup(dst);
	base = basename(s);

	match = strrchr(base, '.') - base == strlen(code);

	free(s);

	return match;
}

/* Match a code to a file name. */
static bool find_object_fname(char *dst, const char *dir, const char *code, bool recursive)
{
	return find_csdb_object(dst, dir, code, NULL, recursive) && (looseMatch || exact_match(dst, code));
}

/* Tag unmatched references in the source object. */
static void tagUnmatchedRef(xmlNodePtr ref)
{
	add_first_child(ref, xmlNewPI(BAD_CAST "unmatched", NULL));
}

/* Get the DMC as a string from a dmRef. */
static void getDmCode(char *dst, xmlNodePtr dmRef)
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

	if (ignoreIss) {
		issueInfo = NULL;
	} else {
		issueInfo = firstXPathNode(NULL, dmRef, BAD_CAST "dmRefIdent/issueInfo|issno");
	}

	language = firstXPathNode(NULL, dmRef, BAD_CAST "dmRefIdent/language|language");

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

/* Get the PMC as a string from a pmRef. */
static void getPmCode(char *dst, xmlNodePtr pmRef)
{
	xmlNodePtr identExtension, pmCode, issueInfo, language;

	char *modelIdentCode;
	char *pmIssuer;
	char *pmNumber;
	char *pmVolume;

	identExtension = firstXPathNode(NULL, pmRef, BAD_CAST "pmRefIdent/identExtension");
	pmCode         = firstXPathNode(NULL, pmRef, BAD_CAST "pmRefIdent/pmCode|pmc");

	if (ignoreIss) {
		issueInfo = NULL;
	} else {
		issueInfo = firstXPathNode(NULL, pmRef, BAD_CAST "pmRefIdent/issueInfo|issno");
	}

	language = firstXPathNode(NULL, pmRef, BAD_CAST "pmRefIdent/language|language");

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

/* Get the SMC as a string from a scormContentPackageRef. */
static void getSmcCode(char *dst, xmlNodePtr smcRef)
{
	xmlNodePtr identExtension, smcCode, issueInfo, language;

	char *modelIdentCode;
	char *smcIssuer;
	char *smcNumber;
	char *smcVolume;

	identExtension = firstXPathNode(NULL, smcRef, BAD_CAST "scormContentPackageRefIdent/identExtension");
	smcCode        = firstXPathNode(NULL, smcRef, BAD_CAST "scormContentPackageRefIdent/scormContentPackageCode");

	if (ignoreIss) {
		issueInfo = NULL;
	} else {
		issueInfo = firstXPathNode(NULL, smcRef, BAD_CAST "scormContentPackageRefIdent/issueInfo");
	}

	language = firstXPathNode(NULL, smcRef, BAD_CAST "scormContentPackageRefIdent/language");

	strcpy(dst, "");

	if (identExtension) {
		char *extensionProducer, *extensionCode;

		extensionProducer = (char *) xmlGetProp(identExtension, BAD_CAST "extensionProducer");
		extensionCode     = (char *) xmlGetProp(identExtension, BAD_CAST "extensionCode");

		strcat(dst, "SME-");

		if (extensionProducer && extensionCode) {
			strcat(dst, extensionProducer);
			strcat(dst, "-");
			strcat(dst, extensionCode);
			strcat(dst, "-");
		}

		xmlFree(extensionProducer);
		xmlFree(extensionCode);
	} else {
		strcat(dst, "SMC-");
	}

	modelIdentCode = (char *) xmlGetProp(smcCode, BAD_CAST "modelIdentCode");
	smcIssuer      = (char *) xmlGetProp(smcCode, BAD_CAST "scormContentPackageIssuer");
	smcNumber      = (char *) xmlGetProp(smcCode, BAD_CAST "scormContentPackageNumber");
	smcVolume      = (char *) xmlGetProp(smcCode, BAD_CAST "scormContentPackageVolume");

	if (modelIdentCode && smcIssuer && smcNumber && smcVolume) {
		strcat(dst, modelIdentCode);
		strcat(dst, "-");
		strcat(dst, smcIssuer);
		strcat(dst, "-");
		strcat(dst, smcNumber);
		strcat(dst, "-");
		strcat(dst, smcVolume);
	}

	xmlFree(modelIdentCode);
	xmlFree(smcIssuer);
	xmlFree(smcNumber);
	xmlFree(smcVolume);

	if (!noIssue) {
		if (issueInfo) {
			char *issueNumber, *inWork;

			issueNumber = (char *) xmlGetProp(issueInfo, BAD_CAST "issueNumber");
			inWork      = (char *) xmlGetProp(issueInfo, BAD_CAST "inWork");

			if (issueNumber && inWork) {
				strcat(dst, "_");
				strcat(dst, issueNumber);
				strcat(dst, "-");
				strcat(dst, inWork);
			}

			xmlFree(issueNumber);
			xmlFree(inWork);
		} else if (language) {
			strcat(dst, "_\?\?\?-\?\?");
		}
	}

	if (language) {
		char *languageIsoCode, *countryIsoCode;

		languageIsoCode = (char *) xmlGetProp(language, BAD_CAST "languageIsoCode");
		countryIsoCode  = (char *) xmlGetProp(language, BAD_CAST "countryIsoCode");

		if (languageIsoCode && countryIsoCode) {
			uppercase(languageIsoCode);

			strcat(dst, "_");
			strcat(dst, languageIsoCode);
			strcat(dst, "-");
			strcat(dst, countryIsoCode);
		}

		xmlFree(languageIsoCode);
		xmlFree(countryIsoCode);
	}
}

/* Get the ICN as a string from an ICN reference. */
static void getICN(char *dst, xmlNodePtr ref)
{
	char *icn;
	icn = (char *) xmlGetProp(ref, BAD_CAST "infoEntityRefIdent");
	strcpy(dst, icn);
	xmlFree(icn);
}

/* Get the ICN as a string from an ICN entity reference. */
static void getICNAttr(char *dst, xmlNodePtr ref)
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
	if (ignoreIss) {
		char *e = strrchr(dst, '-');
		char *s = e - 3;

		if (e && s >= dst) {
			*s = 0;
		}
	}

	xmlFree(icn);
}

/* Match each hotspot against the ICN. */
static int matchHotspot(xmlNodePtr ref, xmlDocPtr doc, const char *code, const char *fname, const char *src)
{
	xmlChar *apsid;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;
	char *s;
	int err = doc == NULL;

	apsid = xmlNodeGetContent(ref);

	if (doc) {
		xmlNodePtr cur;

		ctx = xmlXPathNewContext(doc);

		/* Register namespaces for the hotspot XPath. */
		for (cur = hotspotNs->children; cur; cur = cur->next) {
			xmlChar *prefix, *uri;

			prefix = xmlGetProp(cur, BAD_CAST "prefix");
			uri = xmlGetProp(cur, BAD_CAST "uri");

			xmlXPathRegisterNs(ctx, prefix, uri);

			xmlFree(prefix);
			xmlFree(uri);
		}

		xmlXPathRegisterVariable(ctx, BAD_CAST "id", xmlXPathNewString(apsid));
		obj = xmlXPathEvalExpression(hotspotXPath, ctx);

		if (!obj || xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			node = NULL;
		} else {
			node = obj->nodesetval->nodeTab[0];
		}

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);

		if (node) {
			if (showMatched && !tagUnmatched) {
				s = malloc(strlen(fname) + strlen((char *) apsid) + 2);
				strcpy(s, fname);
				strcat(s, "#");
				strcat(s, (char *) apsid);
				printMatchedFn(ref, src, s);
				free(s);
			}
		} else {
			++err;
		}
	}

	if (err) {
		s = malloc(strlen(code) + strlen((char *) apsid) + 2);
		strcpy(s, code);
		strcat(s, "#");
		strcat(s, (char *) apsid);

		if (tagUnmatched) {
			tagUnmatchedRef(ref);
		} else if (showUnmatched) {
			printMatchedFn(ref, src, s);
		} else if (!quiet) {
			printUnmatchedFn(ref, src, s);
		}

		free(s);
	}

	xmlFree(apsid);
	return err;
}

/* Match the hotspots for an XML-based ICN. */
static int getHotspots(xmlNodePtr ref, const char *src)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	ctx = xmlXPathNewContext(ref->doc);
	xmlXPathSetContextNode(ref, ctx);

	/* Select all hotspots that have an APS ID, meaning they point to some
	 * object in the ICN (vs. using coordinates).
	 */
	obj = xmlXPathEvalExpression(BAD_CAST ".//hotspot/@applicationStructureIdent|.//hotspot/@apsid", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		xmlNodePtr icn;
		char code[PATH_MAX], fname[PATH_MAX];
		int i;
		xmlDocPtr doc;

		icn = firstXPathNode(ref->doc, ref, BAD_CAST "@infoEntityIdent|@boardno");

		getICNAttr(code, icn);

		if (find_object_fname(fname, directory, code, recursive)) {
			doc = read_xml_doc(fname);
		} else {
			doc = NULL;
		}

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			err += matchHotspot(obj->nodesetval->nodeTab[i], doc, code, doc ? fname : code, src);
		}

		xmlFreeDoc(doc);
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return err;
}

/* Match a single referred fragment in another DM. */
static int matchFragment(xmlDocPtr doc, xmlNodePtr ref, const char *code, const char *fname, const char *src)
{
	xmlChar *id;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;
	char *s;
	int err = doc == NULL;

	id = xmlNodeGetContent(ref);

	if (doc) {
		ctx = xmlXPathNewContext(doc);
		xmlXPathRegisterVariable(ctx, BAD_CAST "id", xmlXPathNewString(id));
		obj = xmlXPathEvalExpression(BAD_CAST "//*[@id=$id]", ctx);

		if (!obj || xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			node = NULL;
		} else {
			node = obj->nodesetval->nodeTab[0];
		}

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);

		if (node) {
			if (showMatched && !tagUnmatched) {
				s = malloc(strlen(fname) + strlen((char *) id) + 2);
				strcpy(s, fname);
				strcat(s, "#");
				strcat(s, (char *) id);
				printMatchedFn(ref, src, s);
				free(s);
			}
		} else {
			++err;
		}
	}

	if (err) {
		s = malloc(strlen(code) + strlen((char *) id) + 2);
		strcpy(s, code);
		strcat(s, "#");
		strcat(s, (char *) id);

		if (tagUnmatched) {
			tagUnmatchedRef(ref);
		} else if (showUnmatched) {
			printMatchedFn(ref, src, s);
		} else if (!quiet) {
			printUnmatchedFn(ref, src, s);
		}

		free(s);
	}

	xmlFree(id);
	return err;

}

/* Match the referred fragments in another DM. */
static int getFragment(xmlNodePtr ref, const char *src)
{
	xmlNodePtr dmref;
	char code[PATH_MAX], fname[PATH_MAX];
	xmlDocPtr doc;
	int err;

	dmref = firstXPathNode(ref->doc, ref, BAD_CAST "ancestor::dmRef");

	getDmCode(code, dmref);

	if (find_object_fname(fname, directory, code, recursive)) {
		doc = read_xml_doc(fname);
	} else {
		doc = NULL;
	}

	err = matchFragment(doc, ref, code, doc ? fname : code, src);

	xmlFreeDoc(doc);

	return err;
}

/* Get the comment code as a string from a commentRef. */
static void getComCode(char *dst, xmlNodePtr ref)
{
	xmlNodePtr commentCode, language;

	char *modelIdentCode;
	char *senderIdent;
	char *yearOfDataIssue;
	char *seqNumber;
	char *commentType;

	commentCode = firstXPathNode(NULL, ref, BAD_CAST "commentRefIdent/commentCode");

	language = firstXPathNode(NULL, ref, BAD_CAST "commentRefIdent/language");

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

/* Get the DML code as a string from a dmlRef. */
static void getDmlCode(char *dst, xmlNodePtr ref)
{
	xmlNodePtr dmlCode, issueInfo;

	char *modelIdentCode;
	char *senderIdent;
	char *dmlType;
	char *yearOfDataIssue;
	char *seqNumber;

	dmlCode   = firstXPathNode(NULL, ref, BAD_CAST "dmlRefIdent/dmlCode");
	issueInfo = firstXPathNode(NULL, ref, BAD_CAST "dmlRefIdent/issueInfo");

	modelIdentCode  = (char *) xmlGetProp(dmlCode, BAD_CAST "modelIdentCode");
	senderIdent     = (char *) xmlGetProp(dmlCode, BAD_CAST "senderIdent");
	dmlType         = (char *) xmlGetProp(dmlCode, BAD_CAST "dmlType");
	yearOfDataIssue = (char *) xmlGetProp(dmlCode, BAD_CAST "yearOfDataIssue");
	seqNumber       = (char *) xmlGetProp(dmlCode, BAD_CAST "seqNumber");

	uppercase(dmlType);

	strcpy(dst, "DML-");
	strcat(dst, modelIdentCode);
	strcat(dst, "-");
	strcat(dst, senderIdent);
	strcat(dst, "-");
	strcat(dst, dmlType);
	strcat(dst, "-");
	strcat(dst, yearOfDataIssue);
	strcat(dst, "-");
	strcat(dst, seqNumber);

	xmlFree(modelIdentCode);
	xmlFree(senderIdent);
	xmlFree(dmlType);
	xmlFree(yearOfDataIssue);
	xmlFree(seqNumber);

	if (issueInfo) {
		char *issueNumber, *inWork;

		issueNumber = (char *) xmlGetProp(issueInfo, BAD_CAST "issueNumber");
		inWork      = (char *) xmlGetProp(issueInfo, BAD_CAST "inWork");

		strcat(dst, "_");
		strcat(dst, issueNumber);
		strcat(dst, "-");
		strcat(dst, inWork);

		xmlFree(issueNumber);
		xmlFree(inWork);
	}
}

/* Get the external pub code as a string from an externalPubRef. */
static void getExternalPubCode(char *dst, xmlNodePtr ref)
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
static void getDispatchFileName(char *dst, xmlNodePtr ref)
{
	char *fname;
	fname = (char *) xmlNodeGetContent(ref);
	strcpy(dst, fname);
	xmlFree(fname);
}

/* Update address items using the matched referenced object. */
static void updateRef(xmlNodePtr *refptr, const char *src, const char *code, const char *fname)
{
	xmlNodePtr ref = *refptr;

	if (xmlStrcmp(ref->name, BAD_CAST "dmRef") == 0) {
		xmlDocPtr doc;
		xmlNodePtr dmRefAddressItems, dmTitle;
		xmlChar *techName, *infoName, *infoNameVariant;

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
		infoNameVariant = firstXPathValue(doc, NULL, BAD_CAST "//infoNameVariant");

		dmTitle = xmlNewChild(dmRefAddressItems, NULL, BAD_CAST "dmTitle", NULL);
		xmlNewTextChild(dmTitle, NULL, BAD_CAST "techName", techName);
		if (infoName) {
			xmlNewTextChild(dmTitle, NULL, BAD_CAST "infoName", infoName);
		}
		if (infoNameVariant) {
			xmlNewTextChild(dmTitle, NULL, BAD_CAST "infoNameVariant", infoNameVariant);
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

static int listReferences(const char *path, int show, const char *targetRef, int targetShow);
static int listWhereUsed(const char *path, int show);

/* Print a reference found in an object. */
static int printReference(xmlNodePtr *refptr, const char *src, int show, const char *targetRef, int targetShow)
{
	char code[PATH_MAX];
	char fname[PATH_MAX];
	xmlNodePtr ref = *refptr;

	if ((show & SHOW_DMC) == SHOW_DMC &&
	    (xmlStrcmp(ref->name, BAD_CAST "dmRef") == 0 ||
	     xmlStrcmp(ref->name, BAD_CAST "refdm") == 0 ||
	     xmlStrcmp(ref->name, BAD_CAST "addresdm") == 0))
		getDmCode(code, ref);
	else if ((show & SHOW_PMC) == SHOW_PMC &&
		 (xmlStrcmp(ref->name, BAD_CAST "pmRef") == 0 ||
	          xmlStrcmp(ref->name, BAD_CAST "refpm") == 0))
		getPmCode(code, ref);
	else if ((show & SHOW_SMC) == SHOW_SMC && xmlStrcmp(ref->name, BAD_CAST "scormContentPackageRef") == 0)
		getSmcCode(code, ref);
	else if ((show & SHOW_ICN) == SHOW_ICN &&
	         (xmlStrcmp(ref->name, BAD_CAST "infoEntityRef") == 0))
		getICN(code, ref);
	else if ((show & SHOW_COM) == SHOW_COM && (xmlStrcmp(ref->name, BAD_CAST "commentRef") == 0))
		getComCode(code, ref);
	else if ((show & SHOW_DML) == SHOW_DML && (xmlStrcmp(ref->name, BAD_CAST "dmlRef") == 0))
		getDmlCode(code, ref);
	else if ((show & SHOW_ICN) == SHOW_ICN &&
		 (xmlStrcmp(ref->name, BAD_CAST "infoEntityIdent") == 0 ||
	          xmlStrcmp(ref->name, BAD_CAST "boardno") == 0))
		getICNAttr(code, ref);
	else if ((show & SHOW_EPR) == SHOW_EPR &&
	         (xmlStrcmp(ref->name, BAD_CAST "externalPubRef") == 0 ||
		  xmlStrcmp(ref->name, BAD_CAST "reftp") == 0))
		getExternalPubCode(code, ref);
	else if (xmlStrcmp(ref->name, BAD_CAST "dispatchFileName") == 0 ||
	         xmlStrcmp(ref->name, BAD_CAST "ddnfilen") == 0)
		getDispatchFileName(code, ref);
	else if ((show & SHOW_HOT) == SHOW_HOT &&
		 xmlStrcmp(ref->name, BAD_CAST "graphic") == 0)
		return getHotspots(ref, src);
	else if ((show & SHOW_FRG) == SHOW_FRG &&
		 (xmlStrcmp(ref->name, BAD_CAST "referredFragment") == 0 ||
		  xmlStrcmp(ref->name, BAD_CAST "target") == 0))
		return getFragment(ref, src);
	else
		return 0;

	if (targetRef) {
		/* If looking for a particular ref in -w mode, skip any others. */
		if (!strnmatch(targetRef, code, strlen(code))) {
			return 0;
		}

		/* Replace the code with the target ref so as to match that
		 * specific object rather than the latest object with the same
		 * code. */
		strcpy(code, targetRef);
	}

	if (find_object_fname(fname, directory, code, recursive)) {
		if (updateRefs) {
			updateRef(refptr, src, code, fname);
		} else if (!tagUnmatched) {
			if (showMatched) {
				printMatchedFn(ref, src, fname);
			}

			if (listRecursively) {
				if (targetRef) {
					listWhereUsed(src, targetShow);
				} else {
					listReferences(fname, show, NULL, 0);
				}
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
static bool listedFile(const char *path)
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
static void addFile(const char *path)
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
static int listReferences(const char *path, int show, const char *targetRef, int targetShow)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int unmatched = 0;

	/* In recursive mode, keep a record of which files have been listed
	 * to avoid infinite loops.
	 *
	 * If this is invoked in -w mode (targetRef != NULL), don't update the
	 * record, as that is handled by listWhereUsed.
	 */
	if (listRecursively && targetRef == NULL) {
		if (listedFile(path)) {
			return 0;
		}

		addFile(path);
	}

	if (listSrc) {
		printMatchedFn(NULL, path, path);
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

	obj = xmlXPathEvalExpression(BAD_CAST ".//dmRef|.//refdm|.//addresdm|.//pmRef|.//refpm|.//infoEntityRef|//@infoEntityIdent|//@boardno|.//commentRef|.//dmlRef|.//externalPubRef|.//reftp|.//dispatchFileName|.//ddnfilen|.//graphic[hotspot]|.//dmRef/@referredFragment|.//refdm/@target|.//scormContentPackageRef", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			unmatched += printReference(&(obj->nodesetval->nodeTab[i]), path, show, targetRef, targetShow);
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
static int listReferencesInList(const char *path, int show)
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
		unmatched += listReferences(line, show, NULL, 0);
	}

	if (path) {
		fclose(f);
	}

	return unmatched;
}

/* Register a NS for the hotspot XPath expression. */
static void addHotspotNs(char *s)
{
	char *prefix, *uri;
	xmlNodePtr node;

	prefix = strtok(s, "=");
	uri = strtok(NULL, "");

	node = xmlNewChild(hotspotNs, NULL, BAD_CAST "ns", NULL);
	xmlSetProp(node, BAD_CAST "prefix", BAD_CAST prefix);
	xmlSetProp(node, BAD_CAST "uri", BAD_CAST uri);
}

/* Determine if an object is a type that may contain references to other
 * objects. */
static bool isUsedTarget(const char *name, int show)
{
	return
		(optset(show, SHOW_COM) && is_com(name)) ||
		(optset(show, SHOW_DMC) && is_dm(name))  ||
		(optset(show, SHOW_DML) && is_dml(name)) ||
		(optset(show, SHOW_PMC) && is_pm(name))  ||
		(optset(show, SHOW_SMC) && is_smc(name));
}

/* Search objects in a given directory for references to a target object. */
static int findWhereUsed(const char *dpath, const char *ref, int show)
{
	DIR *dir;
	struct dirent *cur;
	char fpath[PATH_MAX], cpath[PATH_MAX];
	int unmatched = 0;

	if (!(dir = opendir(dpath))) {
		return 1;
	}

	if (strcmp(dpath, ".") == 0) {
		strcpy(fpath, "");
	} else if (dpath[strlen(dpath) - 1] != '/') {
		strcpy(fpath, dpath);
		strcat(fpath, "/");
	} else {
		strcpy(fpath, dpath);
	}

	while ((cur = readdir(dir))) {
		strcpy(cpath, fpath);
		strcat(cpath, cur->d_name);

		if (recursive && isdir(cpath, true)) {
			unmatched += findWhereUsed(cpath, ref, show);
		} else if (isUsedTarget(cur->d_name, show)) {
			unmatched += listReferences(cpath, SHOW_WHERE_USED, ref, show);
		}
	}

	closedir(dir);

	return unmatched;
}

/* List objects that reference a target object. */
static int listWhereUsed(const char *path, int show)
{
	char code[PATH_MAX] = "";
	xmlDocPtr doc;

	/* In recursive mode, keep a record of which objects have been listed
	 * to avoid infinite loops. */
	if (listRecursively) {
		if (listedFile(path)) {
			return 0;
		}

		addFile(path);
	}

	/* If the target object is an ICN, get the ICN from the file name. */
	if (is_icn(path)) {
		strcpy(code, path);
		strtok(code, ".");
	/* If the target object is an XML file, read the object and get the
	 * appropriate code from the IDSTATUS section. */
	} else if ((doc = read_xml_doc(path))) {
		xmlDocPtr tmp;
		xmlNodePtr ident, node;

		ident = firstXPathNode(doc, NULL, BAD_CAST "//dmIdent|//pmIdent|//commentIdent|//dmlIdent|//scormContentPackageIdent");
		node  = xmlNewNode(NULL, BAD_CAST "ref");
		ident = xmlAddChild(node, xmlCopyNode(ident, 1));
		tmp   = xmlNewDoc(BAD_CAST "1.0");
		xmlDocSetRootElement(tmp, node);

		if (xmlStrcmp(ident->name, BAD_CAST "commentIdent") == 0) {
			xmlNodeSetName(ident, BAD_CAST "commentRefIdent");
			xmlNodeSetName(ident, BAD_CAST "commentRef");
			getComCode(code, node);
		} else if (xmlStrcmp(ident->name, BAD_CAST "dmIdent") == 0) {
			xmlNodeSetName(ident, BAD_CAST "dmRefIdent");
			xmlNodeSetName(node , BAD_CAST "dmRef");
			getDmCode(code, node);
		} else if (xmlStrcmp(ident->name, BAD_CAST "dmlIdent") == 0) {
			xmlNodeSetName(ident, BAD_CAST "dmlRefIdent");
			xmlNodeSetName(node , BAD_CAST "dmlRef");
			getDmlCode(code, node);
		} else if (xmlStrcmp(ident->name, BAD_CAST "pmIdent") == 0) {
			xmlNodeSetName(ident, BAD_CAST "pmRefIdent");
			xmlNodeSetName(node , BAD_CAST "pmRef");
			getPmCode(code, node);
		} else if (xmlStrcmp(ident->name, BAD_CAST "scormContentPackageIdent") == 0) {
			xmlNodeSetName(ident, BAD_CAST "scormContentPackageRefIdent");
			xmlNodeSetName(node , BAD_CAST "scormContentPackageRef");
			getSmcCode(code, node);
		}

		xmlFreeDoc(tmp);
		xmlFreeDoc(doc);
	/* Otherwise, interpret the path as a literal code. */
	} else {
		strcpy(code, path);
	}

	/* If no code could be determined, give up. */
	if (strcmp(code, "") == 0) {
		return 1;
	}

	return findWhereUsed(directory, code, show);
}

/* List objects that reference any of a list of target objects. */
static int listWhereUsedList(const char *path, int show)
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
		unmatched += listWhereUsed(line, show);
	}

	if (path) {
		fclose(f);
	}

	return unmatched;
}

/* Display the usage message. */
static void show_help(void)
{
	puts("Usage: s1kd-refs [-aCcDEFfGHIiLlmNnoPqrSsTUuvwXxh?] [-d <dir>] [-e <cmd>] [-J <ns=URL> ...] [-j <xpath>] [-t <fmt>] [-3 <file>] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -a, --all                    Print unmatched codes.");
	puts("  -C, --com                    List commnent references.");
	puts("  -c, --content                Only show references in content section.");
	puts("  -D, --dm                     List data module references.");
	puts("  -d, --dir                    Directory to search for matches in.");
	puts("  -E, --epr                    List external pub refs.");
	puts("  -e, --exec <cmd>             Execute <cmd> for each CSDB object matched.");
	puts("  -F, --overwrite              Overwrite updated (-U) or tagged (-X) objects.");
	puts("  -f, --filename               Print the source filename for each reference.");
	puts("  -G, --icn                    List ICN references.");
	puts("  -H, --hotspot                List hotspot matches in ICNs.");
	puts("  -h, -?, --help               Show help/usage message.");
	puts("  -I, --update-issue           Update references to point to the latest matched object.");
	puts("  -i, --ignore-issue           Ignore issue info when matching.");
	puts("  -J, --namespace <ns=URL>     Register a namespace for the hotspot XPath.");
	puts("  -j, --hotspot-xpath <xpath>  XPath to use for matching hotspots (-H).");
	puts("  -L, --dml                    List DML references.");
	puts("  -l, --list                   Treat input as list of CSDB objects.");
	puts("  -m, --strict-match           Be more strict when matching filenames of objects.");
	puts("  -N, --omit-issue             Assume filenames omit issue info.");
	puts("  -n, --lineno                 Print the source filename and line number for each reference.");
	puts("  -o, --output-valid           Output valid CSDB objects to stdout.");
	puts("  -P, --pm                     List publication module references.");
	puts("  -q, --quiet                  Quiet mode.");
	puts("  -R, --recursively            List references in matched objects recursively.");
	puts("  -r, --recursive              Search for matches in directories recursively.");
	puts("  -S, --smc                    List SCORM content package references.");
	puts("  -s, --include-src            Include the source object as a reference.");
	puts("  -T, --fragment               List referred fragments in other DMs.");
	puts("  -t, --format <fmt>           The format to use when printing references.");
	puts("  -U, --update                 Update address items in matched references.");
	puts("  -u, --unmatched              Show only unmatched references.");
	puts("  -v, --verbose                Verbose output.");
	puts("  -w, --where-used             List places where an object is referenced.");
	puts("  -X, --tag-unmatched          Tag unmatched references.");
	puts("  -x, --xml                    Output XML report.");
	puts("  -3, --externalpubs <file>    Use custom .externalpubs file.");
	puts("  --version                    Show version information.");
	puts("  <object>                     CSDB object to list references in.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Display version information. */
static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int main(int argc, char **argv)
{
	int i, unmatched = 0;

	bool isList = false;
	bool xmlOutput = false;
	bool inclSrcFname = false;
	bool inclLineNum = false;
	char extpubsFname[PATH_MAX] = "";
	bool findUsed = false;

	/* Which types of object references will be listed. */
	int showObjects = 0;

	const char *sopts = "qcNaFfLlUuCDGPRrd:IinEXxSsove:mHj:J:Tt:3:wh?";
	struct option lopts[] = {
		{"version"      , no_argument      , 0, 0},
		{"help"         , no_argument      , 0, 'h'},
		{"quiet"        , no_argument      , 0, 'q'},
		{"content"      , no_argument      , 0, 'c'},
		{"externalpubs" , required_argument, 0, '3'},
		{"omit-issue"   , no_argument      , 0, 'N'},
		{"all"          , no_argument      , 0, 'a'},
		{"overwrite"    , no_argument      , 0, 'F'},
		{"filename"     , no_argument      , 0, 'f'},
		{"dml"          , no_argument      , 0, 'L'},
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
		{"exec"         , required_argument, 0, 'e'},
		{"tag-unmatched", no_argument      , 0, 'X'},
		{"xml"          , no_argument      , 0, 'x'},
		{"smc"          , no_argument      , 0, 'S'},
		{"include-src"  , no_argument      , 0, 's'},
		{"output-valid" , no_argument      , 0, 'o'},
		{"verbose"      , no_argument      , 0, 'v'},
		{"strict-match" , no_argument      , 0, 'm'},
		{"hotspot"      , no_argument      , 0, 'H'},
		{"hotspot-xpath", required_argument, 0, 'j'},
		{"namespace"    , required_argument, 0, 'J'},
		{"fragment"     , no_argument      , 0, 'T'},
		{"format"       , required_argument, 0, 't'},
		{"where-used"   , no_argument      , 0, 'w'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	directory = strdup(".");
	hotspotXPath = xmlStrdup(DEFAULT_HOTSPOT_XPATH);
	hotspotNs = xmlNewNode(NULL, BAD_CAST "hotspotNs");

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
			case '3':
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
			case 'H':
				showObjects |= SHOW_HOT;
				break;
			case 'L':
				showObjects |= SHOW_DML;
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
				ignoreIss = true;
				updateRefIdent = true;
				break;
			case 'i':
				ignoreIss = true;
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
			case 'S':
				showObjects |= SHOW_SMC;
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
			case 'm':
				looseMatch = false;
				break;
			case 'j':
				xmlFree(hotspotXPath);
				hotspotXPath = xmlStrdup(BAD_CAST optarg);
				break;
			case 'J':
				addHotspotNs(optarg);
				break;
			case 'T':
				showObjects |= SHOW_FRG;
				break;
			case 't':
				printFormat = strdup(optarg);
				break;
			case 'e':
				execStr = strdup(optarg);
				break;
			case 'w':
				findUsed = true;
				printMatchedFn = printMatchedWhereUsed;
				printUnmatchedFn = printUnmatchedSrc;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	/* If none of -CDEGHLPST are given, show all types of objects. */
	if (!showObjects) {
		showObjects = SHOW_ALL;
	}

	/* Load .externalpubs config file. */
	if (strcmp(extpubsFname, "") != 0 || find_config(extpubsFname, DEFAULT_EXTPUBS_FNAME)) {
		externalPubs = read_xml_doc(extpubsFname);
	}

	/* Print opening of XML report. */
	if (xmlOutput) {
		puts("<?xml version=\"1.0\"?>");
		printf("<results>");
	}

	/* Set the functions for printing matched/unmatched refs. */
	if (execStr) {
		printMatchedFn = execMatched;
	} else if (printFormat) {
		printMatchedFn   = printMatchedCustom;
		printUnmatchedFn = printUnmatchedCustom;
	} else if (xmlOutput) {
		printMatchedFn = printMatchedXml;
		printUnmatchedFn = printUnmatchedXml;
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
				if (findUsed) {
					unmatched += listWhereUsedList(argv[i], showObjects);
				} else {
					unmatched += listReferencesInList(argv[i], showObjects);
				}
			} else {
				if (findUsed) {
					unmatched += listWhereUsed(argv[i], showObjects);
				} else {
					unmatched += listReferences(argv[i], showObjects, NULL, 0);
				}
			}
		}
	} else if (isList) {
		if (findUsed) {
			unmatched += listWhereUsedList(NULL, showObjects);
		} else {
			unmatched += listReferencesInList(NULL, showObjects);
		}
	} else {
		if (findUsed) {
			unmatched += listWhereUsed("-", showObjects);
		} else {
			unmatched += listReferences("-", showObjects, NULL, 0);
		}
	}

	if (xmlOutput) {
		printf("</results>\n");
	}

	free(directory);
	xmlFree(hotspotXPath);
	xmlFreeNode(hotspotNs);
	free(listedFiles);
	free(execStr);
	free(printFormat);
	xmlFreeDoc(externalPubs);
	xmlCleanupParser();

	return unmatched > 0 ? EXIT_UNMATCHED_REF : EXIT_SUCCESS;
}
