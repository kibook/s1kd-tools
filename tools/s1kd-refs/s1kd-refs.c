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
#define VERSION "5.2.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define SUCC_PREFIX PROG_NAME ": SUCCESS: "
#define FAIL_PREFIX PROG_NAME ": FAILURE: "
#define INF_PREFIX PROG_NAME ": INFO: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"
#define E_OUT_OF_MEMORY ERR_PREFIX "Too many files in recursive listing.\n"
#define E_BAD_STDIN ERR_PREFIX "stdin does not contain valid XML.\n"
#define E_BAD_CSN_CODE ERR_PREFIX "Invalid non-chapterized IPD SNS: %s\n"

#define S_UNMATCHED SUCC_PREFIX "No unmatched references in %s\n"
#define F_UNMATCHED FAIL_PREFIX "Unmatched references in %s\n"

#define I_WHEREUSED INF_PREFIX "Searching for references to %s...\n"
#define I_UPDATE_REF INF_PREFIX "%s: Updating reference %s to match %s...\n"

#define EXIT_UNMATCHED_REF 1
#define EXIT_OUT_OF_MEMORY 2
#define EXIT_BAD_STDIN 3
#define EXIT_BAD_CSN_CODE 4

/* List only references found in the content section. */
static bool contentOnly = false;

static enum verbosity { QUIET, NORMAL, VERBOSE, DEBUG } verbosity = NORMAL;

/* Whether to perform reference matching or not. */
static bool matchReferences = true;

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

/* Command string to execute with the -e option. */
static char *execStr = NULL;

/* Non-chapterized IPD SNS. */
static bool nonChapIpdSns = false;
static char nonChapIpdSystemCode[4] = "";
static char nonChapIpdSubSystemCode[2] = "";
static char nonChapIpdSubSubSystemCode[2] = "";
static char nonChapIpdAssyCode[5] = "";

/* Figure number variant format string. */
static xmlChar *figNumVarFormat;

/* When listing references recursively, keep track of files which have already
 * been listed to avoid loops.
 */
static char (*listedFiles)[PATH_MAX] = NULL;
static int numListedFiles = 0;
static long unsigned maxListedFiles = 1;

/* Possible objects to list references to. */
#define SHOW_COM 0x0001 /* Comments */
#define SHOW_DMC 0x0002 /* Data modules */
#define SHOW_ICN 0x0004 /* ICNs */
#define SHOW_PMC 0x0008 /* Publication modules */
#define SHOW_EPR 0x0010 /* External publications */
#define SHOW_HOT 0x0020 /* Hotspots */
#define SHOW_FRG 0x0040 /* Fragments */
#define SHOW_DML 0x0080 /* DMLs */
#define SHOW_SMC 0x0100 /* SCORM content packages */
#define SHOW_SRC 0x0200 /* Source ident */
#define SHOW_REP 0x0400 /* Repository source ident */
#define SHOW_IPD 0x0800 /* IPD data modules */
#define SHOW_CSN 0x1000 /* CSN items */

/* All possible objects. */
#define SHOW_ALL \
	SHOW_COM | \
	SHOW_DMC | \
	SHOW_ICN | \
	SHOW_PMC | \
	SHOW_EPR | \
	SHOW_HOT | \
	SHOW_FRG | \
	SHOW_DML | \
	SHOW_SMC | \
	SHOW_SRC | \
	SHOW_REP | \
	SHOW_IPD | \
	SHOW_CSN

/* All objects relevant to -w mode. */
#define SHOW_WHERE_USED \
	SHOW_COM | \
	SHOW_DMC | \
	SHOW_PMC | \
	SHOW_DML | \
	SHOW_SMC | \
	SHOW_ICN | \
	SHOW_SRC | \
	SHOW_REP | \
	SHOW_IPD | \
        SHOW_EPR

/* Write valid CSDB objects to stdout. */
static bool outputTree = false;

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

/* Remove elements marked as "delete". */
static bool remDelete = false;

/* Separator for fragments. */
#define FRAGMENT_SEP "#"

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
static void processFormatStr(FILE *f, xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
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
					if (frag) {
						fprintf(f, "%s" FRAGMENT_SEP "%s", fname ? fname : code, frag);
					} else {
						fprintf(f, "%s", fname ? fname : code);
					}
				} else if (strncmp(k, "code", n) == 0) {
					fprintf(f, "%s", code);
				} else if (strncmp(k, "fragment", n) == 0) {
					fprintf(f, "%s", frag);
				} else if (strncmp(k, "file", n) == 0 && fname) {
					fprintf(f, "%s", fname);
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
static void printMatched(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	if (frag) {
		printf("%s" FRAGMENT_SEP "%s\n", fname ? fname : code, frag);
	} else {
		puts(fname ? fname : code);
	}
}
static void printMatchedSrc(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	if (frag) {
		printf("%s: %s" FRAGMENT_SEP "%s\n", src, fname ? fname : code, frag);
	} else {
		printf("%s: %s\n", src, fname ? fname : code);
	}
}
static void printMatchedSrcLine(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	if (frag) {
		printf("%s (%ld): %s" FRAGMENT_SEP "%s\n", src, xmlGetLineNo(node), fname ? fname : code, frag);
	} else {
		printf("%s (%ld): %s\n", src, xmlGetLineNo(node), fname ? fname : code);
	}
}
static void printMatchedXml(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	xmlChar *s, *r, *f, *xpath;
	xmlDocPtr doc;

	if (!node) {
		return;
	}

	s = xmlEncodeEntitiesReentrant(node->doc, BAD_CAST src);
	r = xmlEncodeEntitiesReentrant(node->doc, BAD_CAST code);
	f = xmlEncodeEntitiesReentrant(node->doc, BAD_CAST fname);
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
	printf("<code>%s</code>", r);
	if (frag) {
		printf("<fragment>%s</fragment>", frag);
	}
	if (f) {
		printf("<filename>%s</filename>", f);
	}

	printf("</found>");

	xmlFree(s);
	xmlFree(r);
	xmlFree(xpath);
}
static void printMatchedWhereUsed(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	printf("%s\n", src);
}
static void printMatchedCustom(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	processFormatStr(stdout, node, src, code, frag, fname);
}

static void execMatched(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	execfile(execStr, fname);
}

/* Print an error for references which are unmatched. */
static void printUnmatched(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	if (frag) {
		fprintf(stderr, ERR_PREFIX "Unmatched reference: %s" FRAGMENT_SEP "%s\n", code, frag);
	} else {
		fprintf(stderr, ERR_PREFIX "Unmatched reference: %s\n", code);
	}
}
static void printUnmatchedSrc(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	if (frag) {
		fprintf(stderr, ERR_PREFIX "%s: Unmatched reference: %s" FRAGMENT_SEP "%s\n", src, code, frag);
	} else {
		fprintf(stderr, ERR_PREFIX "%s: Unmatched reference: %s\n", src, code);
	}
}
static void printUnmatchedSrcLine(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	if (frag) {
		fprintf(stderr, ERR_PREFIX "%s (%ld): Unmatched reference: %s" FRAGMENT_SEP "%s\n", src, xmlGetLineNo(node), code, frag);
	} else {
		fprintf(stderr, ERR_PREFIX "%s (%ld): Unmatched reference: %s\n", src, xmlGetLineNo(node), code);
	}
}
static void printUnmatchedXml(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	xmlChar *s, *r, *f, *xpath;
	xmlDocPtr doc;

	s = xmlEncodeEntitiesReentrant(node->doc, BAD_CAST src);
	r = xmlEncodeEntitiesReentrant(node->doc, BAD_CAST code);
	f = xmlEncodeEntitiesReentrant(node->doc, BAD_CAST fname);
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
	if (frag) {
		printf("<fragment>%s</fragment>", frag);
	}
	if (f) {
		printf("<filename>%s</filename>", f);
	}

	printf("</missing>");

	xmlFree(s);
	xmlFree(r);
	xmlFree(xpath);
}
static void printUnmatchedCustom(xmlNodePtr node, const char *src, const char *code, const xmlChar *frag, const char *fname)
{
	fputs(ERR_PREFIX "Unmatched reference: ", stderr);
	processFormatStr(stderr, node, src, code, frag, fname);
}

static void (*printMatchedFn)(xmlNodePtr, const char *, const char *, const xmlChar *, const char *) = printMatched;
static void (*printUnmatchedFn)(xmlNodePtr, const char *, const char*, const xmlChar *, const char *) = printUnmatched;

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
	return matchReferences && find_csdb_object(dst, dir, code, NULL, recursive) && (looseMatch || exact_match(dst, code));
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

/* Get the code of the source DM or PM. */
static void getSourceIdent(char *dst, xmlNodePtr sourceIdent)
{
	xmlDocPtr refdoc;
	xmlNodePtr ref, ident;

	refdoc = xmlNewDoc(BAD_CAST "1.0");
	ident = xmlCopyNode(sourceIdent, 1);

	if (xmlStrcmp(sourceIdent->name, BAD_CAST "sourcePmIdent") == 0) {
		ref = xmlNewNode(NULL, BAD_CAST "pmRef");
		xmlDocSetRootElement(refdoc, ref);
		xmlNodeSetName(ident, BAD_CAST "pmRefIdent");
		ident = xmlAddChild(ref, ident);

		getPmCode(dst, ref);
	} else {
		ref = xmlNewNode(NULL, BAD_CAST "dmRef");
		xmlDocSetRootElement(refdoc, ref);
		xmlNodeSetName(ident, BAD_CAST "dmRefIdent");
		ident = xmlAddChild(ref, ident);

		getDmCode(dst, ref);
	}

	xmlFreeDoc(refdoc);
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
				printMatchedFn(ref, src, code, apsid, fname);
			}
		} else {
			++err;
		}
	}

	if (err) {
		if (tagUnmatched) {
			tagUnmatchedRef(ref);
		} else if (showUnmatched) {
			printMatchedFn(ref, src, code, apsid, fname);
		} else if (verbosity >= NORMAL) {
			printUnmatchedFn(ref, src, code, apsid, fname);
		}
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

		if (remDelete) {
			rem_delete_elems(doc);
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
				printMatchedFn(ref, src, code, id, fname);
			}
		} else {
			++err;
		}
	}

	if (err && showUnmatched) {
		printMatchedFn(ref, src, code, id, doc ? fname : NULL);
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

	if (remDelete) {
		rem_delete_elems(doc);
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

/* Get the disassembly code variant pattern using the figure number variant and
 * the specified format string. */
static xmlChar *formatFigNumVar(const xmlChar *figureNumberVariant)
{
	int i;
	xmlChar *disassyCodeVariant;

	disassyCodeVariant = xmlStrdup(figNumVarFormat);

	for (i = 0; disassyCodeVariant[i]; ++i) {
		switch (disassyCodeVariant[i]) {
			case '%':
				disassyCodeVariant[i] = figureNumberVariant[0];
				break;
			default:
				break;
		}
	}

	return disassyCodeVariant;
}

/* Parse an old (< 4.1) style CSN reference.
 *
 * refcsn (2.0-3.0)/catalogSeqNumberValue (4.0) is a 13-16 digit code:
 *
 * 13:  YY|Y|Y|  YY|YY|Y|NNN|Y (2-character system, 2-character assembly)
 * 14: YYY|Y|Y|  YY|YY|Y|NNN|Y (3-character system, 2-character assembly)
 * 15:  YY|Y|Y|YYYY|YY|Y|NNN|Y (2-character system, 4-character assembly)
 * 16: YYY|Y|Y|YYYY|YY|Y|NNN|Y (3-character system, 4-character assembly)
 *
 * Y = [A-Z0-9 ] (alphanumeric + space)
 * N = [0-9]     (numeric)
 */
#define CSN_VALUE_PATTERN_16 "%3[A-Z0-9 ]%1[A-Z0-9 ]%1[A-Z0-9 ]%4[A-Z0-9 ]%2[A-Z0-9]%1[A-Z0-9 ]%3[0-9]%1[A-Z0-9 ]"
#define CSN_VALUE_PATTERN_15 "%2[A-Z0-9 ]%1[A-Z0-9 ]%1[A-Z0-9 ]%4[A-Z0-9 ]%2[A-Z0-9]%1[A-Z0-9 ]%3[0-9]%1[A-Z0-9 ]"
#define CSN_VALUE_PATTERN_14 "%3[A-Z0-9 ]%1[A-Z0-9 ]%1[A-Z0-9 ]%2[A-Z0-9 ]%2[A-Z0-9]%1[A-Z0-9 ]%3[0-9]%1[A-Z0-9 ]"
#define CSN_VALUE_PATTERN_13 "%2[A-Z0-9 ]%1[A-Z0-9 ]%1[A-Z0-9 ]%2[A-Z0-9 ]%2[A-Z0-9]%1[A-Z0-9 ]%3[0-9]%1[A-Z0-9 ]"

static int str_is_blank(const char *s) {
	int i;
	for (i = 0; s[i]; ++i) {
		if (!isspace((unsigned char) s[i])) {
			return 0;
		}
	}
	return 1;
}

static void parseCsnValue(const xmlChar *csnValue,
	xmlChar **systemCode,
	xmlChar **subSystemCode,
	xmlChar **subSubSystemCode,
	xmlChar **assyCode,
	xmlChar **figureNumber,
	xmlChar **figureNumberVariant,
	xmlChar **item,
	xmlChar **itemVariant)
{
	char system[4];
	char subsys[2];
	char subsub[2];
	char assemb[5];
	char fignum[3];
	char figvar[2];
	char itemno[4];
	char itemva[2];
	const char *pattern;

	switch (xmlStrlen(csnValue)) {
		case 16: pattern = CSN_VALUE_PATTERN_16; break;
		case 15: pattern = CSN_VALUE_PATTERN_15; break;
		case 14: pattern = CSN_VALUE_PATTERN_14; break;
		case 13: pattern = CSN_VALUE_PATTERN_13; break;
		default: pattern = NULL; break;
	}

	if (pattern && sscanf((char *) csnValue, pattern,
			system,
			subsys,
			subsub,
			assemb,
			fignum,
			figvar,
			itemno,
			itemva) == 8) {
		*systemCode          = str_is_blank(system) ? NULL : xmlCharStrdup(system);
		*subSystemCode       = str_is_blank(subsys) ? NULL : xmlCharStrdup(subsys);
		*subSubSystemCode    = str_is_blank(subsub) ? NULL : xmlCharStrdup(subsub);
		*assyCode            = str_is_blank(assemb) ? NULL : xmlCharStrdup(assemb);
		*figureNumber        = xmlCharStrdup(fignum);
		*figureNumberVariant = str_is_blank(figvar) ? NULL : xmlCharStrdup(figvar);
		*item                = xmlCharStrdup(itemno);
		*itemVariant         = str_is_blank(itemva) ? NULL : xmlCharStrdup(itemva);
	} else {
		*systemCode          = NULL;
		*subSystemCode       = NULL;
		*subSubSystemCode    = NULL;
		*assyCode            = NULL;
		*figureNumber        = NULL;
		*figureNumberVariant = NULL;
		*item                = NULL;
		*itemVariant         = NULL;
	}
}

/* Get the code of a CSN ref, including IPD data module code, CSN and item number. */
static void getCsnCode(char *dst, xmlNodePtr ref, xmlChar **csnValue, xmlChar **item, xmlChar **itemVariant)
{
	xmlChar *modelIdentCode;
	xmlChar *systemDiffCode;
	xmlChar *systemCode;
	xmlChar *subSystemCode;
	xmlChar *subSubSystemCode;
	xmlChar *assyCode;
	xmlChar *figureNumber;
	xmlChar *figureNumberVariant;
	xmlChar *itemLocationCode;

	*csnValue = firstXPathValue(NULL, ref, BAD_CAST "@catalogSeqNumberValue|@refcsn");

	if (*csnValue) {
		modelIdentCode = NULL;
		systemDiffCode = NULL;
		itemLocationCode = NULL;

		parseCsnValue(*csnValue,
			&systemCode,
			&subSystemCode,
			&subSubSystemCode,
			&assyCode,
			&figureNumber,
			&figureNumberVariant,
			item,
			itemVariant);
	} else {
		modelIdentCode      = xmlGetProp(ref, BAD_CAST "modelIdentCode");
		systemDiffCode      = xmlGetProp(ref, BAD_CAST "systemDiffCode");
		systemCode          = xmlGetProp(ref, BAD_CAST "systemCode");
		subSystemCode       = xmlGetProp(ref, BAD_CAST "subSystemCode");
		subSubSystemCode    = xmlGetProp(ref, BAD_CAST "subSubSystemCode");
		assyCode            = xmlGetProp(ref, BAD_CAST "assyCode");
		figureNumber        = xmlGetProp(ref, BAD_CAST "figureNumber");
		figureNumberVariant = xmlGetProp(ref, BAD_CAST "figureNumberVariant");
		itemLocationCode    = xmlGetProp(ref, BAD_CAST "itemLocationCode");

		*item               = xmlGetProp(ref, BAD_CAST "item");
		*itemVariant        = xmlGetProp(ref, BAD_CAST "itemVariant");
	}

	/* Apply attributes to non-chapterized or old style CSN refs. */
	if (nonChapIpdSns || *csnValue) {
		xmlNodePtr dmCode = firstXPathNode(NULL, ref, BAD_CAST "ancestor::dmodule/identAndStatusSection/dmAddress/dmIdent/dmCode|ancestor::dmodule/idstatus/dmaddres/dmc/avee");

		if (dmCode) {
			/* These attributes are always interpreted as relative to the current DM. */
			if (!modelIdentCode) {
				modelIdentCode = firstXPathValue(NULL, dmCode, BAD_CAST "@modelIdentCode|modelic");
			}
			if (!systemDiffCode) {
				systemDiffCode = firstXPathValue(NULL, dmCode, BAD_CAST "@systemDiffCode|sdc");
			}

			/* Use wildcard for itemLocationCode if not given. */
			if (!itemLocationCode) {
				itemLocationCode = xmlCharStrdup("?");
			}

			/* If a non-chapterized IPD SNS is given, apply it. */
			if (nonChapIpdSns) {
				/* "-" indicates the SNS is also relative to the current DM. */
				if (strcmp(nonChapIpdSystemCode, "-") == 0) {
					if (!systemCode) {
						systemCode = firstXPathValue(NULL, dmCode,
							BAD_CAST "@systemCode|chapnum");
					}
					if (!subSystemCode) {
						subSystemCode = firstXPathValue(NULL, dmCode,
							BAD_CAST "@subSystemCode|section");
					}
					if (!subSubSystemCode) {
						subSubSystemCode = firstXPathValue(NULL, dmCode,
							BAD_CAST "@subSubSystemCode|subsect");
					}
					if (!assyCode) {
						assyCode = firstXPathValue(NULL, dmCode,
							BAD_CAST "@assyCode|subject");
					}
				/* Otherwise, construct the SNS from the given code. */
				} else {
					if (!systemCode) {
						systemCode = xmlCharStrdup(nonChapIpdSystemCode);
					}
					if (!subSystemCode) {
						subSystemCode = xmlCharStrdup(nonChapIpdSubSystemCode);
					}
					if (!subSubSystemCode) {
						subSubSystemCode = xmlCharStrdup(nonChapIpdSubSubSystemCode);
					}
					if (!assyCode) {
						assyCode = xmlCharStrdup(nonChapIpdAssyCode);
					}
				}
			}
		}
	}

	/* If CSN is chapterized, attempt to match it to a DMC. */
	if (modelIdentCode && systemDiffCode && systemCode && subSystemCode && subSubSystemCode && assyCode && figureNumber) {
		xmlDocPtr tmp;
		xmlNodePtr dmRef, dmRefIdent, dmCode;
		xmlChar *disassyCodeVariant;

		tmp = xmlNewDoc(BAD_CAST "1.0");

		dmRef = xmlNewNode(NULL, BAD_CAST "dmRef");
		dmRefIdent = xmlNewChild(dmRef, NULL, BAD_CAST "dmRefIdent", NULL);
		dmCode = xmlNewChild(dmRefIdent, NULL, BAD_CAST "dmCode", NULL);

		xmlDocSetRootElement(tmp, dmRef);

		if (!figureNumberVariant) {
			figureNumberVariant = xmlCharStrdup("0");
		}

		/* The figure number variant alone cannot fully determine the
		 * disassembly code variant of the IPD data module (for example,
		 * in projects where the disassembly code variant is more than 1
		 * character). Therefore, the figNumVarFormat pattern is used to
		 * construct the full disassemby code variant. */
		disassyCodeVariant = formatFigNumVar(figureNumberVariant);

		if (!itemLocationCode) {
			itemLocationCode = xmlCharStrdup("?");
		}

		xmlSetProp(dmCode, BAD_CAST "modelIdentCode", modelIdentCode);
		xmlSetProp(dmCode, BAD_CAST "systemDiffCode", systemDiffCode);
		xmlSetProp(dmCode, BAD_CAST "systemCode", systemCode);
		xmlSetProp(dmCode, BAD_CAST "subSystemCode", subSystemCode);
		xmlSetProp(dmCode, BAD_CAST "subSubSystemCode", subSubSystemCode);
		xmlSetProp(dmCode, BAD_CAST "assyCode", assyCode);
		xmlSetProp(dmCode, BAD_CAST "disassyCode", figureNumber);
		xmlSetProp(dmCode, BAD_CAST "disassyCodeVariant", disassyCodeVariant);
		xmlSetProp(dmCode, BAD_CAST "infoCode", BAD_CAST "941");
		xmlSetProp(dmCode, BAD_CAST "infoCodeVariant", BAD_CAST "A");
		xmlSetProp(dmCode, BAD_CAST "itemLocationCode", itemLocationCode);

		getDmCode(dst, dmRef);

		xmlFree(disassyCodeVariant);
		xmlFreeDoc(tmp);
	/* Otherwise, just return a generic IPD figure name. */
	} else {
		strcpy(dst, "Fig ");
		if (figureNumber) {
			strcat(dst, (char *) figureNumber);
		} else {
			strcat(dst, "??");
		}
		if (figureNumberVariant) {
			strcat(dst, (char *) figureNumberVariant);
		}
	}

	xmlFree(modelIdentCode);
	xmlFree(systemDiffCode);
	xmlFree(systemCode);
	xmlFree(subSystemCode);
	xmlFree(subSubSystemCode);
	xmlFree(assyCode);
	xmlFree(figureNumber);
	xmlFree(figureNumberVariant);
	xmlFree(itemLocationCode);
}

/* Get the code of an IPD data module only, discarding item number. */
static void getIpdCode(char *dst, xmlNodePtr ref)
{
	xmlChar *csn;
	xmlChar *item;
	xmlChar *itemVariant;

	getCsnCode(dst, ref, &csn, &item, &itemVariant);

	xmlFree(csn);
	xmlFree(item);
	xmlFree(itemVariant);
}

/* Match a CSN item in an IPD. */
static int matchCsnItem(xmlDocPtr doc, xmlNodePtr ref, xmlChar *csn,
	xmlChar *item, xmlChar *itemVariant, const char *code,
	const char *fname, const char *src)
{
	xmlChar *itemSeqNumberValue, *id;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;
	int err = doc == NULL;

	itemSeqNumberValue = firstXPathValue(NULL, ref, BAD_CAST "@itemSeqNumberValue|@refisn");

	id = xmlCharStrdup("Item ");
	id = xmlStrcat(id, item);
	id = xmlStrcat(id, itemVariant);
	if (itemSeqNumberValue) {
		id = xmlStrcat(id, BAD_CAST " ISN ");
		id = xmlStrcat(id, itemSeqNumberValue);
	}

	if (doc) {
		ctx = xmlXPathNewContext(doc);
		xmlXPathRegisterVariable(ctx, BAD_CAST "item", xmlXPathNewString(item));
		xmlXPathRegisterVariable(ctx, BAD_CAST "itemVariant", xmlXPathNewString(itemVariant));
		xmlXPathRegisterVariable(ctx, BAD_CAST "csn", xmlXPathNewString(csn));

		if (itemSeqNumberValue) {
			xmlXPathRegisterVariable(ctx, BAD_CAST "isn", xmlXPathNewString(itemSeqNumberValue));
			obj = xmlXPathEvalExpression(BAD_CAST
				/* 4.1+ */ "//catalogSeqNumber[@item=$item and (not(@itemVariant) or not($itemVariant) or @itemVariant=$itemVariant)]/itemSeqNumber[@itemSeqNumberValue=$isn]|"
				/* 4.0  */ "//catalogSeqNumber[@catalogSeqNumberValue=$csn]/itemSequenceNumber[@itemSeqNumberValue=$isn]|"
				/* 3.0- */ "//csn[@csn=$csn]/isn[@isn=$isn]",
				ctx);
		} else {
			obj = xmlXPathEvalExpression(BAD_CAST
				/* 4.1+ */ "//catalogSeqNumber[@item=$item and (not(@itemVariant) or not($itemVariant) or @itemVariant=$itemVariant)]|"
				/* 4.0  */ "//catalogSeqNumber[@catalogSeqNumberValue=$csn]|"
				/* 3.0- */ "//csn[@csn=$csn]",
				ctx);
		}

		if (!obj || xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			node = NULL;
		} else {
			node = obj->nodesetval->nodeTab[0];
		}

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);

		if (node) {
			if (showMatched && !tagUnmatched) {
				printMatchedFn(ref, src, code, id, fname);
			}
		} else {
			++err;
		}
	}

	if (err) {
		if (tagUnmatched) {
			tagUnmatchedRef(ref);
		} else if (showUnmatched) {
			printMatchedFn(ref, src, code, id, doc ? fname : NULL);
		} else if (verbosity >= NORMAL) {
			printUnmatchedFn(ref, src, code, id, doc ? fname : NULL);
		}
	}

	xmlFree(itemSeqNumberValue);
	xmlFree(id);

	return err;

}

/* Match the CSN items in another DM. */
static int getCsnItem(xmlNodePtr ref, const char *src)
{
	xmlNodePtr csnref;
	char code[PATH_MAX], fname[PATH_MAX];
	xmlDocPtr doc;
	int err;
	xmlChar *csn;
	xmlChar *item;
	xmlChar *itemVariant;

	csnref = ref->parent;

	getCsnCode(code, csnref, &csn, &item, &itemVariant);

	if (find_object_fname(fname, directory, code, recursive)) {
		doc = read_xml_doc(fname);
	} else {
		doc = NULL;
	}

	if (remDelete) {
		rem_delete_elems(doc);
	}

	err = matchCsnItem(doc, csnref, csn, item, itemVariant, code, doc ? fname : code, src);

	xmlFree(csn);
	xmlFree(item);
	xmlFree(itemVariant);

	xmlFreeDoc(doc);

	return err;
}

/* Update address items using the matched referenced object. */
static void updateRef(xmlNodePtr *refptr, const char *src, const char *code, const char *fname)
{
	xmlNodePtr ref = *refptr;

	if (verbosity >= DEBUG) {
		fprintf(stderr, I_UPDATE_REF, src, code, fname);
	}

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
	else if ((show & SHOW_SRC) == SHOW_SRC &&
		(xmlStrcmp(ref->name, BAD_CAST "sourceDmIdent") == 0 ||
		 xmlStrcmp(ref->name, BAD_CAST "sourcePmIdent") == 0))
		getSourceIdent(code, ref);
	else if ((show & SHOW_REP) == SHOW_REP &&
		xmlStrcmp(ref->name, BAD_CAST "repositorySourceDmIdent") == 0)
		getSourceIdent(code, ref);
	else if ((show & SHOW_HOT) == SHOW_HOT &&
		 xmlStrcmp(ref->name, BAD_CAST "graphic") == 0)
		return getHotspots(ref, src);
	else if ((show & SHOW_FRG) == SHOW_FRG &&
		 (xmlStrcmp(ref->name, BAD_CAST "referredFragment") == 0 ||
		  xmlStrcmp(ref->name, BAD_CAST "target") == 0))
		return getFragment(ref, src);
	else if ((show & SHOW_IPD) == SHOW_IPD &&
		 (xmlStrcmp(ref->name, BAD_CAST "catalogSeqNumberRef") == 0 ||
		  xmlStrcmp(ref->name, BAD_CAST "csnref") == 0))
		getIpdCode(code, ref);
	else if ((show & SHOW_CSN) == SHOW_CSN &&
		 (xmlStrcmp(ref->name, BAD_CAST "item") == 0 ||
		  xmlStrcmp(ref->name, BAD_CAST "catalogSeqNumberValue") == 0 ||
		  xmlStrcmp(ref->name, BAD_CAST "refcsn") == 0))
		return getCsnItem(ref, src);
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
				printMatchedFn(ref, src, code, NULL, fname);
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
		printMatchedFn(ref, src, code, NULL, NULL);
	} else if (verbosity >= NORMAL) {
		printUnmatchedFn(ref, src, code, NULL, NULL);
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

/* XPath to select all possible types of references. */
#define REFS_XPATH BAD_CAST \
	".//dmRef|.//refdm|.//addresdm|" \
	".//pmRef|.//refpm|" \
	".//infoEntityRef|//@infoEntityIdent|//@boardno|" \
	".//commentRef|" \
	".//dmlRef|" \
	".//externalPubRef|.//reftp|" \
	".//dispatchFileName|.//ddnfilen|" \
	".//graphic[hotspot]|" \
	".//dmRef/@referredFragment|.//refdm/@target|" \
	".//scormContentPackageRef|" \
	".//sourceDmIdent|.//sourcePmIdent|.//repositorySourceDmIdent|" \
	".//catalogSeqNumberRef|.//csnref|" \
	".//catalogSeqNumberRef/@item|.//catalogSeqNumberRef/@catalogSeqNumberValue|.//@refcsn"

/* List all references in the given object. */
static int listReferences(const char *path, int show, const char *targetRef, int targetShow)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int unmatched = 0;
	xmlDocPtr validTree = NULL;

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
		printMatchedFn(NULL, path, path, NULL, path);
	}

	if (!(doc = read_xml_doc(path))) {
		if (strcmp(path, "-") == 0) {
			fprintf(stderr, E_BAD_STDIN);
			exit(EXIT_BAD_STDIN);
		}

		return 0;
	}

	/* Make a copy of the XML tree before performing extra
	 * processing on it. */
	if (outputTree) {
		validTree = xmlCopyDoc(doc, 1);
	}

	/* Remove elements marked as "delete". */
	if (remDelete) {
		rem_delete_elems(doc);
	}

	ctx = xmlXPathNewContext(doc);

	if (contentOnly)
		ctx->node = firstXPathNode(doc, NULL,
			BAD_CAST "//content|//dmlContent|//dml|//ddnContent|//delivlst");
	else
		ctx->node = xmlDocGetRootElement(doc);

	obj = xmlXPathEvalExpression(REFS_XPATH, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			unmatched += printReference(&(obj->nodesetval->nodeTab[i]), path, show, targetRef, targetShow);
		}
	}

	/* Write valid CSDB object to stdout. */
	if (outputTree) {
		if (unmatched == 0) {
			save_xml_doc(validTree, "-");
		}
		xmlFreeDoc(validTree);
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

	if (verbosity >= VERBOSE && !targetRef) {
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

	if (verbosity >= VERBOSE) {
		fprintf(stderr, I_WHEREUSED, path);
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

		if (remDelete) {
			rem_delete_elems(doc);
		}

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
		} else if (xmlStrcmp(ident->name, BAD_CAST "sourceDmIdent") == 0 ||
			   xmlStrcmp(ident->name, BAD_CAST "sourcePmIdent") == 0 ||
			   xmlStrcmp(ident->name, BAD_CAST "repositorySourceDmIdent") == 0) {
			getSourceIdent(code, ident);
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

/* Read a non-chapterized IPD SNS code. */
static void readnonChapIpdSns(const char *s)
{
	if (strcmp(s, "-") == 0) {
		strcpy(nonChapIpdSystemCode, s);
	} else {
		int n;

		n = sscanf(s, "%3[0-9A-Z]-%1[0-9A-Z]%1[0-9A-Z]-%4[0-9A-Z]",
			nonChapIpdSystemCode,
			nonChapIpdSubSystemCode,
			nonChapIpdSubSubSystemCode,
			nonChapIpdAssyCode);

		if (n != 4) {
			fprintf(stderr, E_BAD_CSN_CODE, s);
			exit(EXIT_BAD_CSN_CODE);
		}
	}
}

/* Display the usage message. */
static void show_help(void)
{
	puts("Usage: s1kd-refs [-aBCcDEFfGHIiKLlMmNnoPqrSsTUuvwXxYZ^h?] [-b <SNS>] [-d <dir>] [-e <cmd>] [-J <ns=URL> ...] [-j <xpath>] [-k <pattern>] [-t <fmt>] [-3 <file>] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -a, --all                    Print unmatched codes.");
	puts("  -B, --ipd                    List IPD references.");
	puts("  -b, --ipd-sns <SNS>          The SNS for non-chapterized IPDs.");
	puts("  -C, --com                    List comment references.");
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
	puts("  -K, --csn                    List CSN references.");
	puts("  -k, --ipd-dcv <pattern>      Pattern for IPD disassembly code variant.");
	puts("  -L, --dml                    List DML references.");
	puts("  -l, --list                   Treat input as list of CSDB objects.");
	puts("  -M, --no-match               Do not attempt to match references to CSDB objects.");
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
	puts("  -Y, --repository             List repository source DMs.");
	puts("  -Z, --source                 List source DM or PM.");
	puts("  -3, --externalpubs <file>    Use custom .externalpubs file.");
	puts("  -^, --remove-deleted         List refs with elements marked as \"delete\" removed.");
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

	const char *sopts = "qcNaFfLlUuCDGPRrd:IinEXxSsove:MmHj:J:Tt:3:wYZBKb:k:^h?";
	struct option lopts[] = {
		{"version"       , no_argument      , 0, 0},
		{"help"          , no_argument      , 0, 'h'},
		{"quiet"         , no_argument      , 0, 'q'},
		{"content"       , no_argument      , 0, 'c'},
		{"externalpubs"  , required_argument, 0, '3'},
		{"omit-issue"    , no_argument      , 0, 'N'},
		{"all"           , no_argument      , 0, 'a'},
		{"overwrite"     , no_argument      , 0, 'F'},
		{"filename"      , no_argument      , 0, 'f'},
		{"dml"           , no_argument      , 0, 'L'},
		{"list"          , no_argument      , 0, 'l'},
		{"update"        , no_argument      , 0, 'U'},
		{"unmatched"     , no_argument      , 0, 'u'},
		{"com"           , no_argument      , 0, 'C'},
		{"dm"            , no_argument      , 0, 'D'},
		{"icn"           , no_argument      , 0, 'G'},
		{"pm"            , no_argument      , 0, 'P'},
		{"recursively"   , no_argument      , 0, 'R'},
		{"recursive"     , no_argument      , 0, 'r'},
		{"dir"           , required_argument, 0, 'd'},
		{"update-issue"  , no_argument      , 0, 'I'},
		{"ignore-issue"  , no_argument      , 0, 'i'},
		{"lineno"        , no_argument      , 0, 'n'},
		{"epr"           , no_argument      , 0, 'E'},
		{"exec"          , required_argument, 0, 'e'},
		{"tag-unmatched" , no_argument      , 0, 'X'},
		{"xml"           , no_argument      , 0, 'x'},
		{"smc"           , no_argument      , 0, 'S'},
		{"include-src"   , no_argument      , 0, 's'},
		{"output-valid"  , no_argument      , 0, 'o'},
		{"verbose"       , no_argument      , 0, 'v'},
		{"no-match"      , no_argument      , 0, 'M'},
		{"strict-match"  , no_argument      , 0, 'm'},
		{"hotspot"       , no_argument      , 0, 'H'},
		{"hotspot-xpath" , required_argument, 0, 'j'},
		{"namespace"     , required_argument, 0, 'J'},
		{"fragment"      , no_argument      , 0, 'T'},
		{"format"        , required_argument, 0, 't'},
		{"where-used"    , no_argument      , 0, 'w'},
		{"repository"    , no_argument      , 0, 'Y'},
		{"source"        , no_argument      , 0, 'Z'},
		{"ipd"           , no_argument      , 0, 'B'},
		{"csn"           , no_argument      , 0, 'K'},
		{"ipd-sns"       , required_argument, 0, 'b'},
		{"ipd-dcv"       , required_argument, 0, 'k'},
		{"remove-deleted", no_argument      , 0, '^'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	directory = strdup(".");
	hotspotXPath = xmlStrdup(DEFAULT_HOTSPOT_XPATH);
	hotspotNs = xmlNewNode(NULL, BAD_CAST "hotspotNs");

	figNumVarFormat = xmlCharStrdup("%");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg)
				break;
			case 'q':
				--verbosity;
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
			case 'M':
				matchReferences = false;
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
				++verbosity;
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
			case 'Y':
				showObjects |= SHOW_REP;
				break;
			case 'Z':
				showObjects |= SHOW_SRC;
				break;
			case 'B':
				showObjects |= SHOW_IPD;
				break;
			case 'K':
				showObjects |= SHOW_CSN;
				break;
			case 'b':
				readnonChapIpdSns(optarg);
				nonChapIpdSns = true;
				break;
			case 'k':
				xmlFree(figNumVarFormat);
				figNumVarFormat = xmlCharStrdup(optarg);
				break;
			case '^':
				remDelete = true;
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
	xmlFree(figNumVarFormat);
	xmlFreeDoc(externalPubs);
	xmlCleanupParser();

	return unmatched > 0 ? EXIT_UNMATCHED_REF : EXIT_SUCCESS;
}
