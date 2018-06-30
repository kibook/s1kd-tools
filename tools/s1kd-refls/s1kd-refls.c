#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-refls"
#define VERSION "1.2.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

bool contentOnly = false;
bool quiet = false;
bool noIssue = false;
bool showUnmatched = false;
bool inclSrcFname = false;

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

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

char *firstXPathValue(const char *path, xmlDocPtr doc, xmlNodePtr root)
{
	return (char *) xmlNodeGetContent(firstXPathNode(path, doc, root));
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

	identExtension = firstXPathNode("dmRefIdent/identExtension|dmcextension", NULL, dmRef);
	dmCode = firstXPathNode("dmRefIdent/dmCode|dmc/avee|avee", NULL, dmRef);
	issueInfo = firstXPathNode("dmRefIdent/issueInfo|issno", NULL, dmRef);
	language = firstXPathNode("dmRefIdent/language|language", NULL, dmRef);

	strcpy(dst, "");

	if (identExtension) {
		char *extensionProducer, *extensionCode;

		extensionProducer = firstXPathValue("@extensionProducer|dmeproducer", NULL, identExtension);
		extensionCode     = firstXPathValue("@extensionCode|dmecode", NULL, identExtension);

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

	modelIdentCode     = firstXPathValue("@modelIdentCode|modelic", NULL, dmCode);
	systemDiffCode     = firstXPathValue("@systemDiffCode|sdc", NULL, dmCode);
	systemCode         = firstXPathValue("@systemCode|chapnum", NULL, dmCode);
	subSystemCode      = firstXPathValue("@subSystemCode|section", NULL, dmCode);
	subSubSystemCode   = firstXPathValue("@subSubSystemCode|subsect", NULL, dmCode);
	assyCode           = firstXPathValue("@assyCode|subject", NULL, dmCode);
	disassyCode        = firstXPathValue("@disassyCode|discode", NULL, dmCode);
	disassyCodeVariant = firstXPathValue("@disassyCodeVariant|discodev", NULL, dmCode);
	infoCode           = firstXPathValue("@infoCode|incode", NULL, dmCode);
	infoCodeVariant    = firstXPathValue("@infoCodeVariant|incodev", NULL, dmCode);
	itemLocationCode   = firstXPathValue("@itemLocationCode|itemloc", NULL, dmCode);
	learnCode          = firstXPathValue("@learnCode", NULL, dmCode);
	learnEventCode     = firstXPathValue("@learnEventCode", NULL, dmCode);

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

		issueNumber = firstXPathValue("@issueNumber|@issno", NULL, issueInfo);
		inWork      = firstXPathValue("@inWork|@inwork", NULL, issueInfo);

		strcat(dst, "_");
		strcat(dst, issueNumber);
		strcat(dst, "-");
		strcat(dst, inWork);

		xmlFree(issueNumber);
		xmlFree(inWork);
	}

	if (language) {
		char *languageIsoCode, *countryIsoCode;

		languageIsoCode = firstXPathValue("@languageIsoCode|@language", NULL, language);
		countryIsoCode  = firstXPathValue("@countryIsoCode|@country", NULL, language);

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
	pmCode         = firstXPathNode("pmRefIdent/pmCode|pmc", NULL, pmRef);
	issueInfo      = firstXPathNode("pmRefIdent/issueInfo|issno", NULL, pmRef);
	language       = firstXPathNode("pmRefIdent/language|language", NULL, pmRef);

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

	modelIdentCode = firstXPathValue("@modelIdentCode|modelic", NULL, pmCode);
	pmIssuer       = firstXPathValue("@pmIssuer|pmissuer", NULL, pmCode);
	pmNumber       = firstXPathValue("@pmNumber|pmnumber", NULL, pmCode);
	pmVolume       = firstXPathValue("@pmVolume|pmvolume", NULL, pmCode);

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

		issueNumber = firstXPathValue("@issueNumber|@issno", NULL, issueInfo);
		inWork      = firstXPathValue("@inWork|@inwork", NULL, issueInfo);

		strcat(dst, "_");
		strcat(dst, issueNumber);
		strcat(dst, "-");
		strcat(dst, inWork);

		xmlFree(issueNumber);
		xmlFree(inWork);
	}

	if (language) {
		char *languageIsoCode, *countryIsoCode;

		languageIsoCode = firstXPathValue("@languageIsoCode|@language", NULL, language);
		countryIsoCode  = firstXPathValue("@countryIsoCode|@country", NULL, language);

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
			strcpy(dst, cur->d_name);
			found = true;
			break;
		}
	}

	closedir(dir);

	return found;
}

void printMatched(const char *src, const char *ref)
{
	if (inclSrcFname) {
		printf("%s: %s\n", src, ref);
	} else {
		puts(ref);
	}
}

void printUnmatched(const char *src, const char *ref)
{
	if (inclSrcFname) {
		fprintf(stderr, ERR_PREFIX "%s: Unmatched reference: %s\n", src, ref);
	} else {
		fprintf(stderr, ERR_PREFIX "Unmatched reference: %s\n", ref);
	}
}

void printReference(xmlNodePtr ref, const char *src)
{
	char code[256];
	char fname[PATH_MAX];

	if (xmlStrcmp(ref->name, BAD_CAST "dmRef") == 0 ||
	    xmlStrcmp(ref->name, BAD_CAST "refdm") == 0 ||
	    xmlStrcmp(ref->name, BAD_CAST "addresdm") == 0)
		getDmCode(code, ref);
	else if (xmlStrcmp(ref->name, BAD_CAST "pmRef") == 0 ||
	         xmlStrcmp(ref->name, BAD_CAST "refpm") == 0)
		getPmCode(code, ref);
	else if (strcmp((char *) ref->name, "infoEntityRef") == 0)
		getICN(code, ref);
	else if (strcmp((char *) ref->name, "commentRef") == 0)
		getComCode(code, ref);
	else
		return;

	if (showUnmatched)
		printMatched(src, code);
	else if (getFileName(fname, code, "."))
		printMatched(src, fname);
	else if (!quiet)
		printUnmatched(src, code);
}

void listReferences(const char *path)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	doc = xmlReadFile(path, NULL, PARSE_OPTS);

	ctx = xmlXPathNewContext(doc);

	if (contentOnly)
		ctx->node = firstXPathNode("//content|//dmlContent|//dml", doc, NULL);
	else
		ctx->node = xmlDocGetRootElement(doc);

	obj = xmlXPathEvalExpression(BAD_CAST ".//dmRef|.//refdm|.//addresdm|.//pmRef|.//refpm|.//infoEntityRef|.//commentRef", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			printReference(obj->nodesetval->nodeTab[i], path);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
	xmlFreeDoc(doc);
}

void listReferencesInList(const char *path)
{
	FILE *f;
	char line[PATH_MAX];

	if (path) {
		if (!(f = fopen(path, "r"))) {
			fprintf(stderr, E_BAD_LIST, path);
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");
		listReferences(line);
	}

	if (path) {
		fclose(f);
	}
}

void showHelp(void)
{
	puts("Usage: s1kd-refls [-acflNqh?] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -a         Print unmatched codes");
	puts("  -c         Only show references in content section");
	puts("  -f         Print the source filename for each reference");
	puts("  -l         Treat input as list of CSDB objects");
	puts("  -N         Assume filenames omit issue info");
	puts("  -q         Quiet mode");
	puts("  -h -?      Show help/usage message");
	puts("  --version  Show version information");
	puts("  <object>   CSDB object to list references in");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
}

int main(int argc, char **argv)
{
	int i;

	bool isList = false;

	const char *sopts = "qcNaflh?";
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
			case 'f':
				inclSrcFname = true;
				break;
			case 'l':
				isList = true;
				break;
			case 'h':
			case '?':
				showHelp();
				return 0;
		}
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (isList) {
				listReferencesInList(argv[i]);
			} else {
				listReferences(argv[i]);
			}
		}
	} else if (isList) {
		listReferencesInList(NULL);
	} else {
		listReferences("-");
	}

	xmlCleanupParser();

	return 0;
}
