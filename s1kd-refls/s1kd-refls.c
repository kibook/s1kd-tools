#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-refls"

#define ERR_PREFIX PROG_NAME ": ERROR: "

bool contentOnly = false;
bool quiet = false;
bool noIssue = false;
bool showUnmatched = false;

xmlNodePtr firstXPathNode(const char *path, xmlDocPtr doc, xmlNodePtr root)
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

	identExtension = firstXPathNode("dmRefIdent/identExtension", NULL, dmRef);
	dmCode = firstXPathNode("dmRefIdent/dmCode", NULL, dmRef);
	issueInfo = firstXPathNode("dmRefIdent/issueInfo", NULL, dmRef);
	language = firstXPathNode("dmRefIdent/language", NULL, dmRef);

	strcpy(dst, "");

	if (identExtension) {
		char *extensionProducer, *extensionCode;

		extensionProducer = (char *) xmlGetProp(identExtension, BAD_CAST "extensionProducer");
		extensionCode     = (char *) xmlGetProp(identExtension, BAD_CAST "extensionCode");

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

	if (issueInfo && !noIssue) {
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

	if (language) {
		char *languageIsoCode, *countryIsoCode;

		languageIsoCode = (char *) xmlGetProp(language, BAD_CAST "languageIsoCode");
		countryIsoCode  = (char *) xmlGetProp(language, BAD_CAST "countryIsoCode");

		strcat(dst, "_");
		strcat(dst, languageIsoCode);
		strcat(dst, "-");
		strcat(dst, countryIsoCode);

		xmlFree(languageIsoCode);
		xmlFree(countryIsoCode);
	}
}

void getPmCode(char *dst, xmlNodePtr pmRef)
{
	xmlNodePtr identExtension, pmCode, issueInfo, language;

	char *modelIdentCode;
	char *pmIssuer;
	char *pmNumber;
	char *pmVolume;

	identExtension = firstXPathNode("pmRefIdent/identExtension", NULL, pmRef);
	pmCode         = firstXPathNode("pmRefIdent/pmCode", NULL, pmRef);
	issueInfo      = firstXPathNode("pmRefIdent/issueInfo", NULL, pmRef);
	language       = firstXPathNode("pmRefIdent/language", NULL, pmRef);

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

	modelIdentCode = (char *) xmlGetProp(pmCode, BAD_CAST "modelIdentCode");
	pmIssuer       = (char *) xmlGetProp(pmCode, BAD_CAST "pmIssuer");
	pmNumber       = (char *) xmlGetProp(pmCode, BAD_CAST "pmNumber");
	pmVolume       = (char *) xmlGetProp(pmCode, BAD_CAST "pmVolume");

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

	if (issueInfo && !noIssue) {
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

	if (language) {
		char *languageIsoCode, *countryIsoCode;

		languageIsoCode = (char *) xmlGetProp(language, BAD_CAST "languageIsoCode");
		countryIsoCode  = (char *) xmlGetProp(language, BAD_CAST "countryIsoCode");

		strcat(dst, "_");
		strcat(dst, languageIsoCode);
		strcat(dst, "-");
		strcat(dst, countryIsoCode);

		xmlFree(languageIsoCode);
		xmlFree(countryIsoCode);
	}
}

void getICN(char *dst, xmlNodePtr ref)
{
	char *icn;

	icn = (char *) xmlGetProp(ref, BAD_CAST "infoEntityRefIdent");

	strcpy(dst, icn);

	xmlFree(icn);
}

void getComCode(char *dst, xmlNodePtr ref)
{
	xmlNodePtr commentCode, language;

	char *modelIdentCode;
	char *senderIdent;
	char *yearOfDataIssue;
	char *seqNumber;
	char *commentType;

	commentCode = firstXPathNode("commentRefIdent/commentCode", NULL, ref);
	language    = firstXPathNode("commentRefIdent/language", NULL, ref);

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

		strcat(dst, "_");
		strcat(dst, languageIsoCode);
		strcat(dst, "-");
		strcat(dst, countryIsoCode);

		xmlFree(languageIsoCode);
		xmlFree(countryIsoCode);
	}
}

bool getFileName(char *dst, char *code, char *path)
{
	DIR *dir;
	struct dirent *cur;
	int n;
	bool found = false;

	n = strlen(code);

	dir = opendir(path);

	while ((cur = readdir(dir))) {
		if (strncasecmp(code, cur->d_name, n) == 0) {
			/*strcpy(dst, path);
			strcat(dst, "/");
			strcat(dst, cur->d_name);*/
			strcpy(dst, cur->d_name);
			found = true;
			break;
		}
	}

	closedir(dir);

	return found;
}

void printReference(xmlNodePtr ref)
{
	char code[256];
	char fname[PATH_MAX];

	if (strcmp((char *) ref->name, "dmRef") == 0)
		getDmCode(code, ref);
	else if (strcmp((char *) ref->name, "pmRef") == 0)
		getPmCode(code, ref);
	else if (strcmp((char *) ref->name, "infoEntityRef") == 0)
		getICN(code, ref);
	else if (strcmp((char *) ref->name, "commentRef") == 0)
		getComCode(code, ref);

	if (showUnmatched)
		puts(code);
	else if (getFileName(fname, code, "."))
		puts(fname);
	else if (!quiet)
		fprintf(stderr, ERR_PREFIX "Unmatched reference: %s\n", code);
}

void listReferences(const char *path)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	doc = xmlReadFile(path, NULL, 0);

	ctx = xmlXPathNewContext(doc);

	if (contentOnly)
		ctx->node = firstXPathNode("//content|//dmlContent", doc, NULL);
	else
		ctx->node = xmlDocGetRootElement(doc);

	obj = xmlXPathEvalExpression(BAD_CAST ".//dmRef|.//pmRef|.//infoEntityRef|.//commentRef", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			printReference(obj->nodesetval->nodeTab[i]);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
	xmlFreeDoc(doc);
}

void showHelp(void)
{
	puts("Usage: s1kd-refls [-qcNah?] <objects>...");
	puts("");
	puts("Options:");
	puts("  -q       Quiet mode");
	puts("  -c       Only show references in content section");
	puts("  -N       Assume filenames omit issue info");
	puts("  -a       Print unmatched codes");
	puts("  -h -?    Show help/usage message");
}

int main(int argc, char **argv)
{
	int i;

	while ((i = getopt(argc, argv, "qcNah?")) != -1) {
		switch (i) {
			case 'q':
				quiet = true;
				break;
			case 'c':
				contentOnly = true;
				break;
			case 'N':
				noIssue = true;
				break;
			case 'a':
				showUnmatched = true;
				break;
			case 'h':
			case '?':
				showHelp();
				exit(0);
		}
	}

	for (i = optind; i < argc; ++i) {
		listReferences(argv[i]);
	}

	return 0;
}
