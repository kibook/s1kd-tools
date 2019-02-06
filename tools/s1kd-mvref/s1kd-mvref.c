#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-mvref"
#define VERSION "2.0.2"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define E_ENCODING_ERROR "Error encoding path name.\n"
#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

#define EXIT_ENCODING_ERROR 1
#define EXIT_NO_FILE 2

#define ADDR_PATH     "//dmAddress|//dmaddres|//pmAddress|//pmaddres"
#define REFS_PATH_CONTENT BAD_CAST "//content//dmRef|//content//refdm[*]|//content//pmRef|//content/refpm"
#define REFS_PATH BAD_CAST "//dmRef|//pmRef|//refdm[*]|//refpm"

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

bool verbose = false;

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

		snprintf(cat, 256, "%s-%s-", extensionProducer, extensionCode);

		xmlFree(extensionProducer);
		xmlFree(extensionCode);

		strcat(dst, cat);
	}

	modelIdentCode = firstXPathString("@modelIdentCode|modelic", NULL, pmCode);
	pmIssuer       = firstXPathString("@pmIssuer|pmissuer", NULL, pmCode);
	pmNumber       = firstXPathString("@pmNumber|pmnumber", NULL, pmCode);
	pmVolume       = firstXPathString("@pmVolume|pmvolume", NULL, pmCode);

	snprintf(cat, 256, "%s-%s-%s-%s", modelIdentCode, pmIssuer, pmNumber, pmVolume);

	xmlFree(modelIdentCode);
	xmlFree(pmIssuer);
	xmlFree(pmNumber);
	xmlFree(pmVolume);

	strcat(dst, cat);

	if (withIssue && issueInfo) {
		char *issueNumber, *inWork;

		issueNumber = firstXPathString("@issueNumber|@issno", NULL, issueInfo);
		inWork      = firstXPathString("@inWork|@inwork", NULL, issueInfo);

		snprintf(cat, 256, "_%s-%s", issueNumber, inWork ? inWork : "00");

		xmlFree(issueNumber);
		xmlFree(inWork);

		strcat(dst, cat);
	}

	if (withLang && language) {
		char *languageIsoCode, *countryIsoCode;

		languageIsoCode = firstXPathString("@languageIsoCode|@language", NULL, language);
		countryIsoCode  = firstXPathString("@countryIsoCode|@country", NULL, language);

		snprintf(cat, 256, "_%s-%s", languageIsoCode, countryIsoCode);

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

		snprintf(cat, 256, "%s-%s-", extensionProducer, extensionCode);

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

	snprintf(cat, 256, "%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
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
		snprintf(cat, 256, "-%s%s", learnCode, learnEventCode);
		strcat(dst, cat);
	}

	xmlFree(learnCode);
	xmlFree(learnEventCode);

	if (withIssue && issueInfo) {
		char *issueNumber, *inWork;

		issueNumber = firstXPathString("@issueNumber|@issno", NULL, issueInfo);
		inWork      = firstXPathString("@inWork|@inwork", NULL, issueInfo);

		snprintf(cat, 256, "_%s-%s", issueNumber, inWork ? inWork : "00");

		xmlFree(issueNumber);
		xmlFree(inWork);

		strcat(dst, cat);
	}

	if (withLang && language) {
		char *languageIsoCode, *countryIsoCode;

		languageIsoCode = firstXPathString("@languageIsoCode|@language", NULL, language);
		countryIsoCode  = firstXPathString("@countryIsoCode|@country", NULL, language);

		snprintf(cat, 256, "_%s-%s", languageIsoCode, countryIsoCode);

		xmlFree(languageIsoCode);
		xmlFree(countryIsoCode);

		strcat(dst, cat);
	}
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

	if (verbose && strcmp(refcode, addcode) == 0) {
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

	if (verbose && strcmp(refcode, addcode) == 0) {
		fprintf(stderr, "    Updating reference to pub module %s...\n", addcode);
	}

	return strcmp(refcode, addcode) == 0;
}

void updateRef(xmlNodePtr ref, xmlNodePtr addresses, const char *fname, xmlNodePtr recode)
{
	xmlNodePtr cur;

	for (cur = addresses->children; cur; cur = cur->next) {
		if (sameDm(ref, cur)) {
			xmlNodePtr dmAddressItems, issueDate, dmTitle,
			           dmRefAddressItems, dmRefIssueDate,
				   dmRefTitle, dmIdent, dmCode, dmRefIdent,
				   dmRefCode, issueInfo, refIssueInfo,
				   language, refLanguage;

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

			issueDate           = findChild(dmAddressItems, "issueDate");
			dmTitle             = firstXPathNode("dmTitle|dmtitle", NULL, dmAddressItems);

			dmRefAddressItems   = firstXPathNode("dmRefAddressItems|self::refdm", NULL, ref);
			dmRefIssueDate      = findChild(dmRefAddressItems, "issueDate");
			dmRefTitle          = firstXPathNode("dmTitle|dmtitle", NULL, dmRefAddressItems);

			if (dmRefIssueDate) replaceNode(dmRefIssueDate, issueDate);
			if (dmRefTitle)     replaceNode(dmRefTitle, dmTitle);
		} else if (samePm(ref, cur)) {
			xmlNodePtr pmAddressItems, issueDate, pmTitle,
			           pmRefAddressItems, pmRefIssueDate,
				   pmRefTitle, pmIdent, pmCode, pmRefIdent,
				   pmRefCode, issueInfo, refIssueInfo,
				   language, refLanguage;

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

			issueDate           = firstXPathNode("issueDate|issdate", NULL, pmAddressItems);
			pmTitle             = firstXPathNode("pmTitle|pmtitle", NULL, pmAddressItems);

			pmRefAddressItems   = firstXPathNode("pmRefAddressItems|self::refpm", NULL, ref);
			pmRefIssueDate      = firstXPathNode("issueDate|issdate", NULL, pmRefAddressItems);
			pmRefTitle          = firstXPathNode("pmTitle|pmtitle", NULL, pmRefAddressItems);

			if (pmRefIssueDate) replaceNode(pmRefIssueDate, issueDate);
			if (pmRefTitle)     replaceNode(pmRefTitle, pmTitle);
		}
	}
}

void updateRefs(xmlNodeSetPtr refs, xmlNodePtr addresses, const char *fname, xmlNodePtr recode)
{
	int i;

	for (i = 0; i < refs->nodeNr; ++i) {
		updateRef(refs->nodeTab[i], addresses, fname, recode);
	}
}

void showHelp(void)
{
	puts("Usage: " PROG_NAME " [-d <dir>] [-s <source>] [-t <target>] [-clvh?] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -h -?          Show help/usage message.");
	puts("  -c             Only move references in content section of targets.");
	puts("  -d <dir>       Update data modules in directory <dir>.");
	puts("  -f             Overwrite input objects.");
	puts("  -l             Input is a list of data module filenames.");
	puts("  -s <source>    Source object.");
	puts("  -t <target>    Change refs to <source> into refs to <target>.");
	puts("  -v             Verbose output.");
	puts("  --version      Show version information.");
	puts("  <object>...    Objects to change refs in.");
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
	xmlNodePtr address;

	doc = xmlReadFile(fname, NULL, PARSE_OPTS|XML_PARSE_NOWARNING|XML_PARSE_NOERROR);

	if (!doc)
		return;

	if (verbose)
		fprintf(stderr, "Registering %s...\n", fname);

	address = firstXPathNode(ADDR_PATH, doc, NULL);

	if (address) {
		xmlAddChild(addresses, xmlCopyNode(address, 1));
	}

	xmlFreeDoc(doc);
}

void updateRefsFile(const char *fname, xmlNodePtr addresses, bool contentOnly, const char *recode, bool overwrite)
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
		obj = xmlXPathEvalExpression(REFS_PATH_CONTENT, ctx);
	} else {
		obj = xmlXPathEvalExpression(REFS_PATH, ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval))
		updateRefs(obj->nodesetval, addresses, fname, recodeIdent);
	
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	if (overwrite) {
		xmlSaveFile(fname, doc);
	} else {
		xmlSaveFile("-", doc);
	}

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
			if (snprintf(fname, PATH_MAX, "%s/%s", path, cur->d_name) < 0) {
				fprintf(stderr, E_ENCODING_ERROR);
				exit(EXIT_ENCODING_ERROR);
			}
			addAddress(fname, addresses);
		}
	}

	closedir(dir);
}

void updateRefsDirectory(const char *path, xmlNodePtr addresses, bool contentOnly, const char *recode, bool overwrite)
{
	DIR *dir;
	struct dirent *cur;

	dir = opendir(path);

	while ((cur = readdir(dir))) {
		if (isS1000D(cur->d_name)) {
			char fname[PATH_MAX];
			if (snprintf(fname, PATH_MAX, "%s/%s", path, cur->d_name) < 0) {
				fprintf(stderr, E_ENCODING_ERROR);
				exit(EXIT_ENCODING_ERROR);
			}
			updateRefsFile(fname, addresses, contentOnly, recode, overwrite);
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

void updateRefsList(xmlNodePtr addresses, xmlNodePtr paths, bool contentOnly, const char *recode, bool overwrite)
{
	xmlNodePtr cur;
	for (cur = paths->children; cur; cur = cur->next) {
		char *path;
		path = (char *) xmlNodeGetContent(cur);
		updateRefsFile(path, addresses, contentOnly, recode, overwrite);
		xmlFree(path);
	}
}

int main(int argc, char **argv)
{
	xmlNodePtr addresses, paths;

	int i;

	bool contentOnly = false;
	char *source = NULL;
	char *directory = NULL;
	bool isList = false;
	char *recode = NULL;
	bool overwrite = false;

	const char *sopts = "s:cfvd:lt:qh?";
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
			case 'c':
				contentOnly = true;
				break;
			case 'f':
				overwrite = true;
				break;
			case 'v':
				verbose = true;
				break;
			case 'd':
				if (!directory) directory = strdup(optarg);
				break;
			case 'l':
				isList = true;
				break;
			case 't':
				if (!recode) recode = strdup(optarg);
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

	if (directory) {
		updateRefsDirectory(directory, addresses, contentOnly, recode, overwrite);
	} else if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (isList) {
				updateRefsList(addresses, paths, contentOnly, recode, overwrite);
			} else {
				updateRefsFile(argv[i], addresses, contentOnly, recode, overwrite);
			}
		}
	} else if (isList) {
		updateRefsList(addresses, paths, contentOnly, recode, overwrite);
	}

	free(source);
	free(directory);
	free(recode);

	xmlFreeNode(addresses);
	xmlFreeNode(paths);

	xmlCleanupParser();

	return 0;
}
