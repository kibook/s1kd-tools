#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-checkrefs"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_VALIDITY_ERR 1

bool verbose = false;
bool validateOnly = true;
bool failOnInvalid = false;

xmlNodePtr firstXPathNode(const char *xpath, xmlDocPtr doc, xmlNodePtr node)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(doc);

	if (node)
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

xmlNodePtr findChild(xmlNodePtr parent, const char *name)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, name) == 0) {
			return cur;
		}
	}

	return NULL;
}

void replaceNode(xmlNodePtr a, xmlNodePtr b)
{
	xmlChar *name = xmlStrdup(a->name);

	b = xmlAddNextSibling(a, xmlCopyNode(b, 1));
	xmlUnlinkNode(a);
	xmlFreeNode(a);

	xmlFree(name);
}

void getPmCode(char *dst, xmlNodePtr ident, bool withIssue, bool withLang)
{
	xmlNodePtr identExtension, pmCode, issueInfo, language;

	char *modelIdentCode, *pmIssuer, *pmNumber, *pmVolume;

	char cat[256];

	identExtension = findChild(ident, "identExtension");
	pmCode         = findChild(ident, "pmCode");
	issueInfo      = findChild(ident, "issueInfo");
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

	modelIdentCode = (char *) xmlGetProp(pmCode, BAD_CAST "modelIdentCode");
	pmIssuer       = (char *) xmlGetProp(pmCode, BAD_CAST "pmIssuer");
	pmNumber       = (char *) xmlGetProp(pmCode, BAD_CAST "pmNumber");
	pmVolume       = (char *) xmlGetProp(pmCode, BAD_CAST "pmVolume");

	sprintf(cat, "%s-%s-%s-%s", modelIdentCode, pmIssuer, pmNumber, pmVolume);

	xmlFree(modelIdentCode);
	xmlFree(pmIssuer);
	xmlFree(pmNumber);
	xmlFree(pmVolume);

	strcat(dst, cat);

	if (withIssue && issueInfo) {
		char *issueNumber, *inWork;

		issueNumber = (char *) xmlGetProp(issueInfo, BAD_CAST "issueNumber");
		inWork      = (char *) xmlGetProp(issueInfo, BAD_CAST "inWork");

		sprintf(cat, "_%s-%s", issueNumber, inWork);

		xmlFree(issueNumber);
		xmlFree(inWork);

		strcat(dst, cat);
	}

	if (withLang && language) {
		char *languageIsoCode, *countryIsoCode;

		languageIsoCode = (char *) xmlGetProp(language, BAD_CAST "languageIsoCode");
		countryIsoCode  = (char *) xmlGetProp(language, BAD_CAST "countryIsoCode");

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

	identExtension = findChild(ident, "identExtension");
	dmCode = findChild(ident, "dmCode");
	issueInfo = findChild(ident, "issueInfo");
	language = findChild(ident, "language");

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
	learnCode          = (char *) xmlGetProp(dmCode, BAD_CAST "learnCode");
	learnEventCode     = (char *) xmlGetProp(dmCode, BAD_CAST "learnEventCode");

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

		issueNumber = (char *) xmlGetProp(issueInfo, BAD_CAST "issueNumber");
		inWork      = (char *) xmlGetProp(issueInfo, BAD_CAST "inWork");

		sprintf(cat, "_%s-%s", issueNumber, inWork);

		xmlFree(issueNumber);
		xmlFree(inWork);

		strcat(dst, cat);
	}

	if (withLang && language) {
		char *languageIsoCode, *countryIsoCode;

		languageIsoCode = (char *) xmlGetProp(language, BAD_CAST "languageIsoCode");
		countryIsoCode  = (char *) xmlGetProp(language, BAD_CAST "countryIsoCode");

		sprintf(cat, "_%s-%s", languageIsoCode, countryIsoCode);

		xmlFree(languageIsoCode);
		xmlFree(countryIsoCode);

		strcat(dst, cat);
	}
}

bool isDmRef(xmlNodePtr ref)
{
	return strcmp((char *) ref->name, "dmRef") == 0;
}

bool sameDm(xmlNodePtr ref, xmlNodePtr address)
{
	char refcode[256], addcode[256];
	bool withIssue, withLang;

	xmlNodePtr ref_dmIdent;
	xmlNodePtr add_dmIdent;

	if (!isDmRef(ref) || strcmp((char *) address->name, "dmAddress") != 0)
		return false;

	ref_dmIdent = findChild(ref, "dmRefIdent");
	add_dmIdent = findChild(address, "dmIdent");

	withIssue = findChild(ref_dmIdent, "issueInfo");
	withLang  = findChild(ref_dmIdent, "language");

	getDmCode(refcode, ref_dmIdent, withIssue, withLang);
	getDmCode(addcode, add_dmIdent, withIssue, withLang);

	if (verbose && !validateOnly && strcmp(refcode, addcode) == 0) {
		printf("    Updating reference to data module %s...\n", addcode);
	}

	return strcmp(refcode, addcode) == 0;
}

bool isPmRef(xmlNodePtr ref)
{
	return strcmp((char *) ref->name, "pmRef") == 0;
}

bool samePm(xmlNodePtr ref, xmlNodePtr address)
{
	char refcode[256], addcode[256];
	bool withIssue, withLang;

	xmlNodePtr ref_pmIdent;
	xmlNodePtr add_pmIdent;

	if (!isPmRef(ref) || strcmp((char *) address->name, "pmAddress") != 0)
		return false;

	ref_pmIdent = findChild(ref, "pmRefIdent");
	add_pmIdent = findChild(address, "pmIdent");

	withIssue = findChild(ref_pmIdent, "issueInfo");
	withLang  = findChild(ref_pmIdent, "language");

	getPmCode(refcode, ref_pmIdent, withIssue, withLang);
	getPmCode(addcode, add_pmIdent, withIssue, withLang);

	if (verbose && !validateOnly && strcmp(refcode, addcode) == 0) {
		printf("    Updating reference to pub module %s...\n", addcode);
	}

	return strcmp(refcode, addcode) == 0;
}

void validityError(xmlNodePtr ref, const char *fname)
{
	char *prefix = "";
	char code[256];

	if (isDmRef(ref)) {
		prefix = "data module";
		getDmCode(code, firstXPathNode("dmRefIdent", ref->doc, ref), false, false);
	} else if (isPmRef(ref)) {
		prefix = "pub module";
		getPmCode(code, firstXPathNode("pmRefIdent", ref->doc, ref), false, false);
	}

	fprintf(stderr, ERR_PREFIX "%s (Line %d): invalid reference to %s %s.\n", fname, ref->line, prefix, code);

	if (failOnInvalid)
		exit(EXIT_VALIDITY_ERR);
}

void updateRef(xmlNodePtr ref, xmlNodePtr addresses, const char *fname)
{
	xmlNodePtr cur;
	bool isValid = false;

	for (cur = addresses->children; cur; cur = cur->next) {
		if (sameDm(ref, cur)) {
			isValid = true;

			if (!validateOnly) {
				xmlNodePtr dmAddressItems      = findChild(cur, "dmAddressItems");
				xmlNodePtr issueDate           = findChild(dmAddressItems, "issueDate");
				xmlNodePtr dmTitle             = findChild(dmAddressItems, "dmTitle");
				xmlNodePtr dmRefAddressItems   = findChild(ref, "dmRefAddressItems");
				xmlNodePtr dmRefIssueDate      = dmRefAddressItems ? findChild(dmRefAddressItems, "issueDate") : NULL;
				xmlNodePtr dmRefTitle          = dmRefAddressItems ? findChild(dmRefAddressItems, "dmTitle") : NULL;

				if (dmRefIssueDate) replaceNode(dmRefIssueDate, issueDate);
				if (dmRefTitle)     replaceNode(dmRefTitle, dmTitle);
			}
		} else if (samePm(ref, cur)) {
			isValid = true;

			if (!validateOnly) {
				xmlNodePtr pmAddressItems      = findChild(cur, "pmAddressItems");
				xmlNodePtr issueDate           = findChild(pmAddressItems, "issueDate");
				xmlNodePtr pmTitle             = findChild(pmAddressItems, "pmTitle");
				xmlNodePtr pmRefAddressItems   = findChild(ref, "pmRefAddressItems");
				xmlNodePtr pmRefIssueDate      = pmRefAddressItems ? findChild(pmRefAddressItems, "issueDate") : NULL;
				xmlNodePtr pmRefTitle          = pmRefAddressItems ? findChild(pmRefAddressItems, "pmTitle") : NULL;

				if (pmRefIssueDate) replaceNode(pmRefIssueDate, issueDate);
				if (pmRefTitle)     replaceNode(pmRefTitle, pmTitle);
			}
		}
	}

	if (!isValid)
		validityError(ref, fname);
}

void updateRefs(xmlNodeSetPtr refs, xmlNodePtr addresses, const char *fname)
{
	int i;

	for (i = 0; i < refs->nodeNr; ++i)
		updateRef(refs->nodeTab[i], addresses, fname);
}

void showHelp(void)
{
	puts("Usage: " PROG_NAME " [-h?]");
	puts("");
	puts("Options:");
	puts("  -s <source>    Use only <source> as source.");
	puts("  -t <target>    Only check references in <target>.");
	puts("  -c             Only check references in content section of targets.");
	puts("  -u             Update address items of references.");
	puts("  -F             Fail on first invalid reference, returning error code.");
	puts("  -v             Verbose output.");
	puts("  -h -?          Show help/usage message.");
}

void addAddress(const char *fname, xmlNodePtr addresses)
{
	xmlDocPtr doc;
	xmlNodePtr address;

	doc = xmlReadFile(fname, NULL, 0);

	if (!doc)
		return;
	
	if (verbose)
		printf("Registering %s...\n", fname);
	
	address = firstXPathNode("//dmAddress|//pmAddress", doc, NULL);

	if (address) {
		xmlAddChild(addresses, xmlCopyNode(address, 1));
	}

	xmlFreeDoc(doc);
}

void updateRefsFile(const char *fname, xmlNodePtr addresses, bool contentOnly)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	doc = xmlReadFile(fname, NULL, 0);

	if (!doc)
		return;

	if (verbose)
		printf("Checking refs in %s...\n", fname);

	ctx = xmlXPathNewContext(doc);

	if (contentOnly) {
		obj = xmlXPathEvalExpression(BAD_CAST "//content//dmRef|//content//pmRef", ctx);
	} else {
		obj = xmlXPathEvalExpression(BAD_CAST "//dmRef|//pmRef", ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval))
		updateRefs(obj->nodesetval, addresses, fname);
	
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	if (!validateOnly)
		xmlSaveFile(fname, doc);

	xmlFreeDoc(doc);
}


int main(int argc, char **argv)
{
	xmlNodePtr addresses;

	int i;

	bool contentOnly = false;
	char *source = NULL;
	char *target = NULL;

	while ((i = getopt(argc, argv, "s:t:cuFvh?")) != -1) {
		switch (i) {
			case 's':
				source = strdup(optarg);
				break;
			case 't':
				target = strdup(optarg);
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
			case 'v':
				verbose = true;
				break;
			case 'h':
			case '?':
				showHelp();
				exit(0);
		}
	}

	addresses = xmlNewNode(NULL, BAD_CAST "addresses");

	if (source) {
		addAddress(source, addresses);
	} else {
		for (i = optind; i < argc; ++i) {
			addAddress(argv[i], addresses);
		}
	}

	if (target) {
		updateRefsFile(target, addresses, contentOnly);
	} else {
		for (i = optind; i < argc; ++i) {
			updateRefsFile(argv[i], addresses, contentOnly);
		}
	}

	if (source)
		free(source);
	if (target)
		free(target);

	xmlFreeNode(addresses);

	xmlCleanupParser();

	return 0;
}
