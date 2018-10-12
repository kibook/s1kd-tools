#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-checkrefs"
#define VERSION "1.2.3"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

#define EXIT_VALIDITY_ERR 1
#define EXIT_NO_FILE 2

#define ADDR_PATH     "//dmAddress|//dmaddres|//pmAddress|//pmaddres"
#define ADDR_PATH_EXT "//dmAddress|//dmaddres|//pmAddress|//pmaddres|//externalPubAddress"

#define REFS_PATH_CONTENT BAD_CAST "//content//dmRef|//content//refdm[*]|//content//pmRef|//content/refpm"
#define REFS_PATH_CONTENT_EXT BAD_CAST "//content//dmRef|//content//refdm[*]|//content//pmRef|//content//refpm|//content//externalPubRef"
#define REFS_PATH BAD_CAST "//dmRef|//pmRef|//refdm[*]|//refpm"
#define REFS_PATH_EXT BAD_CAST "//dmRef|//refdm[*]|//pmRef|//refpm|//externalPubRef"

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

bool verbose = false;
bool validateOnly = true;
bool failOnInvalid = false;
bool checkExtPubRefs = false;
enum list {OFF, REFS, DMS} listInvalid = OFF;
bool inclFname = false;
bool quiet = false;

xmlNodePtr firstXPathNode(const char *xpath, xmlDocPtr doc, xmlNodePtr node)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	if (doc) {
		ctx = xmlXPathNewContext(doc);
	} else if (node) {
		ctx = xmlXPathNewContext(node->doc);
	} else {
		return NULL;
	}

	ctx->node = node;
	
	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval))
		first = NULL;
	else
		first = obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

char *firstXPathString(const char *xpath, xmlDocPtr doc, xmlNodePtr node)
{
	return (char *) xmlNodeGetContent(firstXPathNode(xpath, doc, node));
}

xmlNodePtr findChild(xmlNodePtr parent, const char *name)
{
	xmlNodePtr cur;

	if (!parent) return NULL;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, name) == 0) {
			return cur;
		}
	}

	return NULL;
}

void replaceNode(xmlNodePtr a, xmlNodePtr b)
{
	xmlNodePtr c;
	c = xmlCopyNode(b, 1);
	xmlNodeSetName(c, a->name);
	xmlAddNextSibling(a, c);
	xmlUnlinkNode(a);
	xmlFreeNode(a);
}

void getPmCode(char *dst, xmlNodePtr ident, bool withIssue, bool withLang)
{
	xmlNodePtr identExtension, pmCode, issueInfo, language;

	char *modelIdentCode, *pmIssuer, *pmNumber, *pmVolume;

	char cat[256];

	identExtension = findChild(ident, "identExtension");
	pmCode         = firstXPathNode("pmCode|pmc", NULL, ident);
	issueInfo      = firstXPathNode("issueInfo|issno", NULL, ident);
	language       = findChild(ident, "language");

	strcpy(dst, "");

	if (identExtension) {
		char *extensionProducer, *extensionCode;

		extensionProducer = (char *) xmlGetProp(identExtension, BAD_CAST "extensionProducer");
		extensionCode     = (char *) xmlGetProp(identExtension, BAD_CAST "extensionCode");

		sprintf(cat, "%s-%s-", extensionProducer, extensionCode);

		xmlFree(extensionProducer);
		xmlFree(extensionCode);

		strcat(dst, cat);
	}

	modelIdentCode = firstXPathString("@modelIdentCode|modelic", NULL, pmCode);
	pmIssuer       = firstXPathString("@pmIssuer|pmissuer", NULL, pmCode);
	pmNumber       = firstXPathString("@pmNumber|pmnumber", NULL, pmCode);
	pmVolume       = firstXPathString("@pmVolume|pmvolume", NULL, pmCode);

	sprintf(cat, "%s-%s-%s-%s", modelIdentCode, pmIssuer, pmNumber, pmVolume);

	xmlFree(modelIdentCode);
	xmlFree(pmIssuer);
	xmlFree(pmNumber);
	xmlFree(pmVolume);

	strcat(dst, cat);

	if (withIssue && issueInfo) {
		char *issueNumber, *inWork;

		issueNumber = firstXPathString("@issueNumber|@issno", NULL, issueInfo);
		inWork      = firstXPathString("@inWork|@inwork", NULL, issueInfo);

		sprintf(cat, "_%s-%s", issueNumber, inWork ? inWork : "00");

		xmlFree(issueNumber);
		xmlFree(inWork);

		strcat(dst, cat);
	}

	if (withLang && language) {
		char *languageIsoCode, *countryIsoCode;

		languageIsoCode = firstXPathString("@languageIsoCode|@language", NULL, language);
		countryIsoCode  = firstXPathString("@countryIsoCode|@country", NULL, language);

		sprintf(cat, "_%s-%s", languageIsoCode, countryIsoCode);

		xmlFree(languageIsoCode);
		xmlFree(countryIsoCode);

		strcat(dst, cat);
	}
}

void getDmCode(char *dst, xmlNodePtr ident, bool withIssue, bool withLang)
{
	xmlNodePtr identExtension, dmCode, issueInfo, language;
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
	
	char cat[256];

	identExtension = firstXPathNode("identExtension|dmcextension", NULL, ident);
	dmCode         = firstXPathNode("dmCode|.//avee", NULL, ident);
	issueInfo      = firstXPathNode("issueInfo|issno", NULL, ident);
	language       = findChild(ident, "language");

	strcpy(dst, "");

	if (identExtension) {
		char *extensionProducer, *extensionCode;

		extensionProducer = firstXPathString("@extensionProducer|dmeproducer", NULL, identExtension);
		extensionCode     = firstXPathString("@extensionCode|dmecode", NULL, identExtension);

		sprintf(cat, "%s-%s-", extensionProducer, extensionCode);

		xmlFree(extensionProducer);
		xmlFree(extensionCode);

		strcat(dst, cat);
	}

	modelIdentCode     = firstXPathString("@modelIdentCode|modelic", NULL, dmCode);
	systemDiffCode     = firstXPathString("@systemDiffCode|sdc", NULL, dmCode);
	systemCode         = firstXPathString("@systemCode|chapnum", NULL, dmCode);
	subSystemCode      = firstXPathString("@subSystemCode|section", NULL, dmCode);
	subSubSystemCode   = firstXPathString("@subSubSystemCode|subsect", NULL, dmCode);
	assyCode           = firstXPathString("@assyCode|subject", NULL, dmCode);
	disassyCode        = firstXPathString("@disassyCode|discode", NULL, dmCode);
	disassyCodeVariant = firstXPathString("@disassyCodeVariant|discodev", NULL, dmCode);
	infoCode           = firstXPathString("@infoCode|incode", NULL, dmCode);
	infoCodeVariant    = firstXPathString("@infoCodeVariant|incodev", NULL, dmCode);
	itemLocationCode   = firstXPathString("@itemLocationCode|itemloc", NULL, dmCode);
	learnCode          = firstXPathString("@learnCode", NULL, dmCode);
	learnEventCode     = firstXPathString("@learnEventCode", NULL, dmCode);

	sprintf(cat, "%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
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

	strcat(dst, cat);

	if (learnCode && learnEventCode) {
		sprintf(cat, "-%s%s", learnCode, learnEventCode);
		strcat(dst, cat);
	}

	xmlFree(learnCode);
	xmlFree(learnEventCode);

	if (withIssue && issueInfo) {
		char *issueNumber, *inWork;

		issueNumber = firstXPathString("@issueNumber|@issno", NULL, issueInfo);
		inWork      = firstXPathString("@inWork|@inwork", NULL, issueInfo);

		sprintf(cat, "_%s-%s", issueNumber, inWork ? inWork : "00");

		xmlFree(issueNumber);
		xmlFree(inWork);

		strcat(dst, cat);
	}

	if (withLang && language) {
		char *languageIsoCode, *countryIsoCode;

		languageIsoCode = firstXPathString("@languageIsoCode|@language", NULL, language);
		countryIsoCode  = firstXPathString("@countryIsoCode|@country", NULL, language);

		sprintf(cat, "_%s-%s", languageIsoCode, countryIsoCode);

		xmlFree(languageIsoCode);
		xmlFree(countryIsoCode);

		strcat(dst, cat);
	}
}

void getExtPubCode(char *dst, xmlNodePtr ident, bool withIssue, bool withLang)
{
	xmlNodePtr externalPubCode;
	char *scheme;
	char *code;

	externalPubCode = findChild(ident, "externalPubCode");

	if (!externalPubCode) {
		externalPubCode = findChild(ident, "externalPubTitle");
	}

	scheme = (char *) xmlGetProp(externalPubCode, BAD_CAST "pubCodingScheme");
	code = (char *) xmlNodeGetContent(externalPubCode);

	if (scheme) {
		strcpy(dst, scheme);
		strcat(dst, " ");
		strcat(dst, code);
	} else {
		strcpy(dst, code);
	}

	xmlFree(scheme);
	xmlFree(code);
}

bool isDmRef(xmlNodePtr ref)
{
	return xmlStrcmp(ref->name, BAD_CAST "dmRef") == 0 || xmlStrcmp(ref->name, BAD_CAST "refdm") == 0;
}

bool isDmAddress(xmlNodePtr address)
{
	return xmlStrcmp(address->name, BAD_CAST "dmAddress") == 0 || xmlStrcmp(address->name, BAD_CAST "dmaddres") == 0;
}

bool sameDm(xmlNodePtr ref, xmlNodePtr address)
{
	char refcode[256], addcode[256];
	bool withIssue, withLang;

	xmlNodePtr ref_dmIdent;
	xmlNodePtr add_dmIdent;

	if (!isDmRef(ref) || !isDmAddress(address))
		return false;

	ref_dmIdent = firstXPathNode("dmRefIdent|self::refdm", NULL, ref);
	add_dmIdent = firstXPathNode("dmIdent|self::dmaddres", NULL, address);

	withIssue = firstXPathNode(".//issueInfo|.//issno", NULL, ref);
	withLang  = firstXPathNode(".//language", NULL, ref);

	getDmCode(refcode, ref_dmIdent, withIssue, withLang);
	getDmCode(addcode, add_dmIdent, withIssue, withLang);

	if (verbose && !validateOnly && strcmp(refcode, addcode) == 0) {
		fprintf(stderr, "    Updating reference to data module %s...\n", addcode);
	}

	return strcmp(refcode, addcode) == 0;
}

bool isPmRef(xmlNodePtr ref)
{
	return xmlStrcmp(ref->name, BAD_CAST "pmRef") == 0 || xmlStrcmp(ref->name, BAD_CAST "refpm") == 0;
}

bool isPmAddress(xmlNodePtr address)
{
	return xmlStrcmp(address->name, BAD_CAST "pmAddress") == 0 || xmlStrcmp(address->name, BAD_CAST "pmaddres") == 0;
}

bool samePm(xmlNodePtr ref, xmlNodePtr address)
{
	char refcode[256], addcode[256];
	bool withIssue, withLang;

	xmlNodePtr ref_pmIdent;
	xmlNodePtr add_pmIdent;

	if (!isPmRef(ref) || !isPmAddress(address))
		return false;

	ref_pmIdent = firstXPathNode("pmRefIdent|self::refpm", NULL, ref);
	add_pmIdent = firstXPathNode("pmIdent|self::pmaddres", NULL, address);

	withIssue = firstXPathNode("issueInfo|issno", NULL, ref_pmIdent);
	withLang  = findChild(ref_pmIdent, "language");

	getPmCode(refcode, ref_pmIdent, withIssue, withLang);
	getPmCode(addcode, add_pmIdent, withIssue, withLang);

	if (verbose && !validateOnly && strcmp(refcode, addcode) == 0) {
		fprintf(stderr, "    Updating reference to pub module %s...\n", addcode);
	}

	return strcmp(refcode, addcode) == 0;
}

bool isExtPubRef(xmlNodePtr ref)
{
	return strcmp((char *) ref->name, "externalPubRef") == 0;
}

bool sameExtPubRef(xmlNodePtr ref, xmlNodePtr address)
{
	char refcode[256], addcode[256];
	bool withIssue;

	xmlNodePtr ref_extPubIdent;
	xmlNodePtr add_extPubIdent;

	if (!isExtPubRef(ref) || strcmp((char *) address->name, "externalPubAddress") != 0)
		return false;

	ref_extPubIdent = findChild(ref, "externalPubRefIdent");
	add_extPubIdent = findChild(address, "externalPubIdent");

	withIssue = findChild(ref_extPubIdent, "externalPubIssueInfo");

	getExtPubCode(refcode, ref_extPubIdent, withIssue, false);
	getExtPubCode(addcode, add_extPubIdent, withIssue, false);

	if (verbose && !validateOnly && strcmp(refcode, addcode) == 0) {
		fprintf(stderr, "    Updating reference to external pub %s...\n", addcode);
	}

	return strcmp(refcode, addcode) == 0;
}

void validityError(xmlNodePtr ref, const char *fname)
{
	char *prefix = "";
	char code[256];

	if (isDmRef(ref)) {
		prefix = "data module";
		getDmCode(code, firstXPathNode("dmRefIdent|self::refdm", ref->doc, ref), false, false);
	} else if (isPmRef(ref)) {
		prefix = "pub module";
		getPmCode(code, firstXPathNode("pmRefIdent|self::refpm", ref->doc, ref), false, false);
	} else if (isExtPubRef(ref)) {
		prefix = "external pub";
		getExtPubCode(code, firstXPathNode("externalPubRefIdent", ref->doc, ref), false, false);
	}

	if (listInvalid == REFS) {
		if (inclFname) {
			printf("%s: %s\n", fname, code);
		} else {
			printf("%s\n", code);
		}
	} else if (listInvalid == DMS) {
		printf("%s\n", fname);
	} else if (!quiet) {
		fprintf(stderr, ERR_PREFIX "%s (Line %d): invalid reference to %s %s.\n", fname, ref->line, prefix, code);
	}

	if (failOnInvalid)
		exit(EXIT_VALIDITY_ERR);
}

bool updateRef(xmlNodePtr ref, xmlNodePtr addresses, const char *fname, xmlNodePtr recode)
{
	xmlNodePtr cur;
	bool isValid = false;

	for (cur = addresses->children; cur; cur = cur->next) {
		if (sameDm(ref, cur)) {
			isValid = true;

			if (!validateOnly) {
				xmlNodePtr dmAddressItems, issueDate, dmTitle, dmRefAddressItems, dmRefIssueDate, dmRefTitle;

				if (recode) {
					xmlNodePtr dmIdent, dmCode, dmRefIdent, dmRefCode, issueInfo, refIssueInfo, language, refLanguage;

					dmIdent = firstXPathNode("dmIdent|self::dmaddres", NULL, recode);
					dmCode = firstXPathNode("dmCode|.//avee", NULL, dmIdent);
					issueInfo = firstXPathNode("issueInfo|issno", NULL, dmIdent);
					language = findChild(dmIdent, "language");

					dmRefIdent = firstXPathNode("dmRefIdent|self::refdm", NULL, ref);
					dmRefCode  = firstXPathNode("dmCode|.//avee", NULL, dmRefIdent);
					refIssueInfo = firstXPathNode("issueInfo|issno", NULL, dmRefIdent);
					refLanguage = findChild(dmRefIdent, "language");

					if (verbose) {
						char code[256];
						getDmCode(code, dmIdent, refIssueInfo, refLanguage);
						fprintf(stderr, "      Recoding to %s...\n", code);
					}

					replaceNode(dmRefCode, dmCode);
					if (refIssueInfo) replaceNode(refIssueInfo, issueInfo);
					if (refLanguage) replaceNode(refLanguage, language);

					dmAddressItems = firstXPathNode("dmAddressItems|self::dmaddres", NULL, recode);
				} else {
					dmAddressItems = firstXPathNode("dmAddressItems|self::dmaddres", NULL, cur);
				}

				issueDate           = findChild(dmAddressItems, "issueDate");
				dmTitle             = firstXPathNode("dmTitle|dmtitle", NULL, dmAddressItems);

				dmRefAddressItems   = firstXPathNode("dmRefAddressItems|self::refdm", NULL, ref);
				dmRefIssueDate      = findChild(dmRefAddressItems, "issueDate");
				dmRefTitle          = firstXPathNode("dmTitle|dmtitle", NULL, dmRefAddressItems);

				if (dmRefIssueDate) replaceNode(dmRefIssueDate, issueDate);
				if (dmRefTitle)     replaceNode(dmRefTitle, dmTitle);
			}
		} else if (samePm(ref, cur)) {
			isValid = true;

			if (!validateOnly) {
				xmlNodePtr pmAddressItems, issueDate, pmTitle, pmRefAddressItems, pmRefIssueDate, pmRefTitle;

				if (recode) {
					xmlNodePtr pmIdent, pmCode, pmRefIdent, pmRefCode, issueInfo, refIssueInfo, language, refLanguage;

					pmIdent = firstXPathNode("pmIdent|self::pmaddres", NULL, recode);
					pmCode  = firstXPathNode("pmCode|pmc", NULL, pmIdent);
					issueInfo = firstXPathNode("issueInfo|issno", NULL, pmIdent);
					language = findChild(pmIdent, "language");

					pmRefIdent = firstXPathNode("pmRefIdent|self::refpm", NULL, ref);
					pmRefCode  = firstXPathNode("pmCode|pmc", NULL, pmRefIdent);
					refIssueInfo = firstXPathNode("issueInfo|issno", NULL, pmRefIdent);
					refLanguage = findChild(pmRefIdent, "language");

					if (verbose) {
						char code[256];
						getPmCode(code, pmIdent, refIssueInfo, refLanguage);
						fprintf(stderr, "      Recoding to %s...\n", code);
					}

					replaceNode(pmRefCode, pmCode);
					if (refIssueInfo) replaceNode(refIssueInfo, issueInfo);
					if (refLanguage) replaceNode(refLanguage, language);

					pmAddressItems = firstXPathNode("pmAddressItems|self::pmaddres", NULL, recode);
				} else {
					pmAddressItems = firstXPathNode("pmAddressItems|self::pmaddres", NULL, recode);
				}

				issueDate           = firstXPathNode("issueDate|issdate", NULL, pmAddressItems);
				pmTitle             = firstXPathNode("pmTitle|pmtitle", NULL, pmAddressItems);

				pmRefAddressItems   = firstXPathNode("pmRefAddressItems|self::refpm", NULL, ref);
				pmRefIssueDate      = firstXPathNode("issueDate|issdate", NULL, pmRefAddressItems);
				pmRefTitle          = firstXPathNode("pmTitle|pmtitle", NULL, pmRefAddressItems);

				if (pmRefIssueDate) replaceNode(pmRefIssueDate, issueDate);
				if (pmRefTitle)     replaceNode(pmRefTitle, pmTitle);
			}
		} else if (checkExtPubRefs && sameExtPubRef(ref, cur)) {
			isValid = true;

			if (!validateOnly) {
				xmlNodePtr extPubIdent = findChild(cur, "externalPubIdent");
				xmlNodePtr extPubTitle = findChild(extPubIdent, "externalPubTitle");
				xmlNodePtr extPubAddressItems = findChild(cur, "externalPubAddressItems");
				xmlNodePtr extPubIssueDate = findChild(extPubAddressItems, "externalPubIssueDate");
				xmlNodePtr extPubSecurity = findChild(extPubAddressItems, "security");
				xmlNodePtr extPubRPC = findChild(extPubAddressItems, "responsiblePartnerCompany");
				xmlNodePtr extPubShortTitle = findChild(extPubAddressItems, "shortExternalPubTitle");
				xmlNodePtr extPubRefIdent = findChild(ref, "externalPubRefIdent");
				xmlNodePtr extPubRefTitle = findChild(extPubRefIdent, "externalPubTitle");
				xmlNodePtr extPubRefAddressItems = findChild(ref, "externalPubRefAddressItems");
				xmlNodePtr extPubRefIssueDate = findChild(extPubRefAddressItems, "externalPubIssueDate");
				xmlNodePtr extPubRefSecurity = findChild(extPubRefAddressItems, "security");
				xmlNodePtr extPubRefRPC = findChild(extPubRefAddressItems, "responsiblePartnerCompany");
				xmlNodePtr extPubRefShortTitle = findChild(extPubRefAddressItems, "shortExternalPubTitle");

				if (extPubRefIssueDate)  replaceNode(extPubRefIssueDate, extPubIssueDate);
				if (extPubRefTitle)      replaceNode(extPubRefTitle, extPubTitle);
				if (extPubRefSecurity)   replaceNode(extPubRefSecurity, extPubSecurity);
				if (extPubRefRPC)        replaceNode(extPubRefRPC, extPubRPC);
				if (extPubRefShortTitle) replaceNode(extPubRefShortTitle, extPubShortTitle);
			}

		}
	}

	if (!isValid)
		validityError(ref, fname);

	return isValid;
}

void updateRefs(xmlNodeSetPtr refs, xmlNodePtr addresses, const char *fname, xmlNodePtr recode)
{
	int i;

	for (i = 0; i < refs->nodeNr; ++i) {
		if (!updateRef(refs->nodeTab[i], addresses, fname, recode) && listInvalid == DMS) {
			break;
		}
	}
}

void showHelp(void)
{
	puts("Usage: " PROG_NAME " [-d <dir>] [-m <object>] [-s <source>] [-t <target>] [-ceFLlnquvh?]");
	puts("");
	puts("Options:");
	puts("  -h -?          Show help/usage message.");
	puts("  -c             Only check references in content section of targets.");
	puts("  -d <dir>       Check data modules in directory <dir>.");
	puts("  -e             Check externalPubRefs.");
	puts("  -F             Fail on first invalid reference, returning error code.");
	puts("  -f             List files containing invalid references.");
	puts("  -L             Input is a list of data module filenames.");
	puts("  -l             List invalid references.");
	puts("  -m <object>    Change refs to <source> into refs to <object>.");
	puts("  -n             Include filename in list of invalid references.");
	puts("  -q             Do not show an error on invalid references.");
	puts("  -s <source>    Use only <source> as source.");
	puts("  -t <target>    Only check references in <target>.");
	puts("  -u             Update address items of references.");
	puts("  -v             Verbose output.");
	puts("  --version      Show version information.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

bool isS1000D(const char *fname)
{
	return (strncmp(fname, "DMC-", 4) == 0 ||
		strncmp(fname, "DME-", 4) == 0 ||
		strncmp(fname, "PMC-", 4) == 0 ||
		strncmp(fname, "PME-", 4) == 0 ||
		strncmp(fname, "COM-", 4) == 0 ||
		strncmp(fname, "IMF-", 4) == 0 ||
		strncmp(fname, "DDN-", 4) == 0 ||
		strncmp(fname, "DML-", 4) == 0 ||
		strncmp(fname, "UPF-", 4) == 0 ||
		strncmp(fname, "UPE-", 4) == 0 ||
		strncmp(fname, "SMC-", 4) == 0 ||
		strncmp(fname, "SME-", 4) == 0) && strncasecmp(fname + strlen(fname) - 4, ".XML", 4) == 0;
}

void addAddress(const char *fname, xmlNodePtr addresses)
{
	xmlDocPtr doc;
	xmlNodePtr root;

	doc = xmlReadFile(fname, NULL, PARSE_OPTS|XML_PARSE_NOWARNING|XML_PARSE_NOERROR);

	if (!doc)
		return;

	root = xmlDocGetRootElement(doc);

	if (verbose)
		fprintf(stderr, "Registering %s...\n", fname);

	if (checkExtPubRefs && strcmp((char *) root->name, "externalPubs") == 0) {
		xmlXPathContextPtr ctx;
		xmlXPathObjectPtr obj;

		ctx = xmlXPathNewContext(doc);
		obj = xmlXPathEvalExpression(BAD_CAST "//externalPubAddress", ctx);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			int i;

			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				xmlAddChild(addresses, xmlCopyNode(obj->nodesetval->nodeTab[i], 1));
			}
		}
	} else {
		xmlNodePtr address = firstXPathNode(ADDR_PATH, doc, NULL);

		if (address) {
			xmlAddChild(addresses, xmlCopyNode(address, 1));
		}
	}

	xmlFreeDoc(doc);
}

void updateRefsFile(const char *fname, xmlNodePtr addresses, bool contentOnly, const char *recode)
{
	xmlDocPtr doc, recodeDoc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr recodeIdent;

	if (!(doc = xmlReadFile(fname, NULL, PARSE_OPTS|XML_PARSE_NOWARNING|XML_PARSE_NOERROR))) {
		return;
	}

	if (recode) {
		recodeDoc = xmlReadFile(recode, NULL, PARSE_OPTS);
		recodeIdent = firstXPathNode(ADDR_PATH, recodeDoc, NULL);
	} else {
		recodeIdent = NULL;
	}

	if (verbose) {
		fprintf(stderr, "Checking refs in %s...\n", fname);
	}

	ctx = xmlXPathNewContext(doc);

	if (contentOnly) {
		if (checkExtPubRefs) {
			obj = xmlXPathEvalExpression(REFS_PATH_CONTENT_EXT, ctx);
		} else {
			obj = xmlXPathEvalExpression(REFS_PATH_CONTENT, ctx);
		}
	} else {
		if (checkExtPubRefs) {
			obj = xmlXPathEvalExpression(REFS_PATH_EXT, ctx);
		} else {
			obj = xmlXPathEvalExpression(REFS_PATH, ctx);
		}
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval))
		updateRefs(obj->nodesetval, addresses, fname, recodeIdent);
	
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	if (!validateOnly)
		xmlSaveFile(fname, doc);

	xmlFreeDoc(doc);
}

void addDirectory(const char *path, xmlNodePtr addresses)
{
	DIR *dir;
	struct dirent *cur;

	dir = opendir(path);

	if (!dir) {
		fprintf(stderr, ERR_PREFIX "Directory %s does not exist.\n", path);
		exit(EXIT_NO_FILE);
	}

	while ((cur = readdir(dir))) {
		if (isS1000D(cur->d_name)) {
			char fname[PATH_MAX];
			sprintf(fname, "%s/%s", path, cur->d_name);
			addAddress(fname, addresses);
		}
	}

	closedir(dir);
}

void updateRefsDirectory(const char *path, xmlNodePtr addresses, bool contentOnly, const char *recode)
{
	DIR *dir;
	struct dirent *cur;

	dir = opendir(path);

	while ((cur = readdir(dir))) {
		if (isS1000D(cur->d_name)) {
			char fname[PATH_MAX];
			sprintf(fname, "%s/%s", path, cur->d_name);
			updateRefsFile(fname, addresses, contentOnly, recode);
		}
	}

	closedir(dir);
}

xmlNodePtr addAddressList(const char *fname, xmlNodePtr addresses, xmlNodePtr paths)
{
	FILE *f;
	char path[PATH_MAX];

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			fprintf(stderr, E_BAD_LIST, fname);
			return paths;
		}
	} else {
		f = stdin;
	}

	while (fgets(path, PATH_MAX, f)) {
		strtok(path, "\t\r\n");
		addAddress(path, addresses);
		xmlNewChild(paths, NULL, BAD_CAST "path", BAD_CAST path);
	}

	if (fname) {
		fclose(f);
	}

	return paths;
}

void updateRefsList(xmlNodePtr addresses, xmlNodePtr paths, bool contentOnly, const char *recode)
{
	xmlNodePtr cur;
	for (cur = paths->children; cur; cur = cur->next) {
		char *path;
		path = (char *) xmlNodeGetContent(cur);
		updateRefsFile(path, addresses, contentOnly, recode);
		xmlFree(path);
	}
}

int main(int argc, char **argv)
{
	xmlNodePtr addresses, paths;

	int i;

	bool contentOnly = false;
	char *source = NULL;
	char *target = NULL;
	char *directory = NULL;
	bool isList = false;
	char *recode = NULL;

	const char *sopts = "s:t:cuFfveld:Lm:nqh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 's':
				if (!source) source = strdup(optarg);
				break;
			case 't':
				if (!target) target = strdup(optarg);
				break;
			case 'c':
				contentOnly = true;
				break;
			case 'u':
				validateOnly = false;
				break;
			case 'F':
				failOnInvalid = true;
				break;
			case 'f':
				listInvalid = DMS;
				break;
			case 'v':
				verbose = true;
				break;
			case 'e':
				checkExtPubRefs = true;
				break;
			case 'l':
				listInvalid = REFS;
				break;
			case 'd':
				if (!directory) directory = strdup(optarg);
				break;
			case 'L':
				isList = true;
				break;
			case 'm':
				if (!recode) recode = strdup(optarg);
				validateOnly = false;
				break;
			case 'n':
				inclFname = true;
				break;
			case 'q':
				quiet = true;
				break;
			case 'h':
			case '?':
				showHelp();
				exit(0);
		}
	}

	if (recode && !source) {
		fprintf(stderr, ERR_PREFIX "Source object must be specified with -s to be moved with -m.\n");
		exit(EXIT_NO_FILE);
	}

	addresses = xmlNewNode(NULL, BAD_CAST "addresses");
	paths = xmlNewNode(NULL, BAD_CAST "paths");

	if (directory) {
		addDirectory(directory, addresses);
	}

	if (source) {
		addAddress(source, addresses);
	} else if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (isList) {
				addAddressList(argv[i], addresses, paths);
			} else {
				addAddress(argv[i], addresses);
			}
		}
	} else if (isList) {
		addAddressList(NULL, addresses, paths);
	}

	if (target) {
		updateRefsFile(target, addresses, contentOnly, recode);
	} else if (directory) {
		updateRefsDirectory(directory, addresses, contentOnly, recode);
	} else if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (isList) {
				updateRefsList(addresses, paths, contentOnly, recode);
			} else {
				updateRefsFile(argv[i], addresses, contentOnly, recode);
			}
		}
	} else if (isList) {
		updateRefsList(addresses, paths, contentOnly, recode);
	}

	free(source);
	free(target);
	free(directory);
	free(recode);

	xmlFreeNode(addresses);
	xmlFreeNode(paths);

	xmlCleanupParser();

	return 0;
}
