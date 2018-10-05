#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-refls"
#define VERSION "1.8.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

/* List only references found in the content section. */
bool contentOnly = false;
/* Do not display errors. */
bool quiet = false;
/* Assume objects were created with the -N option. */
bool noIssue = false;
/* Show unmatched references instead of an error. */
bool showUnmatched = false;
/* Include the name of the object where each reference is found. */
bool inclSrcFname = false;
/* Show references which are matched in the filesystem. */
bool showMatched = true;
/* Recurse in to child directories. */
bool recursive = false;
/* Directory to start search in. */
char *directory;
/* Only match against code, ignore language/issue info even if present. */
bool fullMatch = true;
/* Include line numbers where references occur. */
bool inclLineNum = false;

/* Possible objects to list references to. */
#define SHOW_COM 0x01
#define SHOW_DMC 0x02
#define SHOW_ICN 0x04
#define SHOW_PMC 0x08

/* Which types of object references will be listed. */
int showObjects = 0;

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

/* Return the first node matching an XPath expression. */
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

/* Return the value of the first node matching an XPath expression. */
char *firstXPathValue(const char *path, xmlDocPtr doc, xmlNodePtr root)
{
	return (char *) xmlNodeGetContent(firstXPathNode(path, doc, root));
}

/* Convert string to uppercase. */
void uppercase(char *s)
{
	int i;
	for (i = 0; s[i]; ++i) {
		s[i] = toupper(s[i]);
	}
}

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

	identExtension = firstXPathNode("dmRefIdent/identExtension|dmcextension", NULL, dmRef);
	dmCode = firstXPathNode("dmRefIdent/dmCode|dmc/avee|avee", NULL, dmRef);

	if (fullMatch) {
		issueInfo = firstXPathNode("dmRefIdent/issueInfo|issno", NULL, dmRef);
		language  = firstXPathNode("dmRefIdent/language|language", NULL, dmRef);
	} else {
		issueInfo = NULL;
		language = NULL;
	}

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

	if (fullMatch) {
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

	identExtension = firstXPathNode("pmRefIdent/identExtension", NULL, pmRef);
	pmCode         = firstXPathNode("pmRefIdent/pmCode|pmc", NULL, pmRef);

	if (fullMatch) {
		issueInfo = firstXPathNode("pmRefIdent/issueInfo|issno", NULL, pmRef);
		language  = firstXPathNode("pmRefIdent/language|language", NULL, pmRef);
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

	if (fullMatch) {
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
	icn = xmlNodeGetContent(ref);
	strcpy(dst, (char *) icn);
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

	commentCode = firstXPathNode("commentRefIdent/commentCode", NULL, ref);

	if (fullMatch) {
		language = firstXPathNode("commentRefIdent/language", NULL, ref);
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

/* Determine if path is a directory. */
bool isDir(const char *path)
{
	struct stat st;
	char s[PATH_MAX], *b;

	strcpy(s, path);
	b = basename(s);

	if (strcmp(b, ".") == 0 || strcmp(b, "..") == 0) {
		return 0;
	}

	stat(path, &st);
	return S_ISDIR(st.st_mode);
}

/* Compare the codes of two paths. */
int codecmp(const char *p1, const char *p2)
{
	char s1[PATH_MAX], s2[PATH_MAX], *b1, *b2;

	strcpy(s1, p1);
	strcpy(s2, p2);

	b1 = basename(s1);
	b2 = basename(s2);

	return strcmp(b1, b2);
}

/* Find the filename of a referenced object by its code. */
bool getFileName(char *dst, char *code, char *path)
{
	DIR *dir;
	struct dirent *cur;
	int n;
	bool found = false;
	int len = strlen(path);
	char fpath[PATH_MAX], cpath[PATH_MAX];

	n = strlen(code);

	if (strcmp(path, ".") == 0) {
		strcpy(fpath, "");
	} else if (path[len - 1] != '/') {
		strcpy(fpath, path);
		strcat(fpath, "/");
	} else {
		strcpy(fpath, path);
	}

	dir = opendir(path);

	while ((cur = readdir(dir))) {
		strcpy(cpath, fpath);
		strcat(cpath, cur->d_name);

		if (recursive && isDir(cpath)) {
			char tmp[PATH_MAX];

			if (getFileName(tmp, code, cpath) && (!found || codecmp(tmp, dst) > 0)) {
				strcpy(dst, tmp);
				found = true;
			}
		} else if (strncmp(code, cur->d_name, n) == 0) {
			if (!found || codecmp(cpath, dst) > 0) {
				strcpy(dst, cpath);
				found = true;
			}
		}
	}

	closedir(dir);

	return found;
}

/* Print a reference which is matched in the filesystem. */
void printMatched(const char *src, unsigned short line, const char *ref)
{
	if (inclSrcFname) {
		if (inclLineNum) {
			printf("%s (%d): %s\n", src, line, ref);
		} else {
			printf("%s: %s\n", src, ref);
		}
	} else {
		puts(ref);
	}
}

/* Print an error for references which are unmatched. */
void printUnmatched(const char *src, unsigned short line, const char *ref)
{
	if (inclSrcFname) {
		if (inclLineNum) {
			fprintf(stderr, ERR_PREFIX "%s (%d): Unmatched reference: %s\n", src, line, ref);
		} else {
			fprintf(stderr, ERR_PREFIX "%s: Unmatched reference: %s\n", src, ref);
		}
	} else {
		fprintf(stderr, ERR_PREFIX "Unmatched reference: %s\n", ref);
	}
}

/* Print a reference found in an object. */
void printReference(xmlNodePtr ref, const char *src)
{
	char code[256];
	char fname[PATH_MAX];
	unsigned short line;

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
	else
		return;

	/* If the ref is an attribute, the line number must come from its
	 * parent element.
	 */
	if (ref->type == XML_ATTRIBUTE_NODE) {
		line = ref->parent->line;
	} else {
		line = ref->line;
	}

	if (showUnmatched) {
		if (showMatched) {
			printMatched(src, line, code);
		} else if (!getFileName(fname, code, directory)) {
			printMatched(src, line, code);
		}
	} else if (getFileName(fname, code, directory)) {
		if (showMatched) {
			printMatched(src, line, fname);
		}
	} else if (!quiet) {
		printUnmatched(src, line, code);
	}
}

/* List all references in the given object. */
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

	obj = xmlXPathEvalExpression(BAD_CAST ".//dmRef|.//refdm|.//addresdm|.//pmRef|.//refpm|.//infoEntityRef|//@infoEntityIdent|//@boardno|.//commentRef", ctx);

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

/* Parse a list of filenames as input. */
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

/* Display the usage message. */
void showHelp(void)
{
	puts("Usage: s1kd-refls [-aCcDfGilNnPqruh?] [-d <dir>] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -a         Print unmatched codes.");
	puts("  -C         List commnent references.");
	puts("  -c         Only show references in content section.");
	puts("  -D         List data module references.");
	puts("  -d         Directory to search for matches in.");
	puts("  -f         Print the source filename for each reference.");
	puts("  -G         List ICN references.");
	puts("  -i         Ignore issue info/language when matching.");
	puts("  -l         Treat input as list of CSDB objects.");
	puts("  -N         Assume filenames omit issue info.");
	puts("  -n         Show line number of reference with source filename.");
	puts("  -P         List publication module references.");
	puts("  -q         Quiet mode.");
	puts("  -r         Search recursively for matches.");
	puts("  -u         Show only unmatched references.");
	puts("  -h -?      Show help/usage message.");
	puts("  --version  Show version information.");
	puts("  <object>   CSDB object to list references in.");
}

/* Display version information. */
void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int main(int argc, char **argv)
{
	int i;

	bool isList = false;

	const char *sopts = "qcNafluCDGPrd:inh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
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
			case 'r':
				recursive = true;
				break;
			case 'd':
				free(directory);
				directory = strdup(optarg);
				break;
			case 'i':
				fullMatch = false;
				break;
			case 'n':
				inclLineNum = true;
				break;
			case 'h':
			case '?':
				showHelp();
				return 0;
		}
	}

	/* If none of -CDGP are given, show all types of objects. */
	if (!showObjects) {
		showObjects = SHOW_COM | SHOW_DMC | SHOW_ICN | SHOW_PMC;
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

	free(directory);
	xmlCleanupParser();

	return 0;
}
