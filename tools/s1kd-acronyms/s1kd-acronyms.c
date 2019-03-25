#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxml/debugXML.h>

#include "stylesheets.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-acronyms"
#define VERSION "1.6.0"

/* Paths to text nodes where acronyms may occur */
#define ACRO_MARKUP_XPATH BAD_CAST "//para/text()|//notePara/text()|//warningAndCautionPara/text()|//attentionListItemPara/text()|//title/text()|//listItemTerm/text()|//term/text()|//termTitle/text()|//emphasis/text()|//changeInline/text()|//change/text()"
xmlChar *acro_markup_xpath = NULL;

/* Characters that must occur before/after a set of characters in order for the
 * set to be considered a valid acronym. */
#define PRE_ACRONYM_DELIM BAD_CAST " (/\n"
#define POST_ACRONYM_DELIM BAD_CAST " .,)/\n"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define INF_PREFIX PROG_NAME ": INFO: "
#define E_NO_LIST ERR_PREFIX "Could not read acronyms list: %s\n"
#define E_NO_FILE ERR_PREFIX "Could not read file: %s\n"
#define E_BAD_LIST ERR_PREFIX "Could not read list file: %s\n"
#define I_FIND INF_PREFIX "Searching for acronyms in %s...\n"
#define I_MARKUP INF_PREFIX "Marking up acronyms in %s...\n"
#define I_DELETE INF_PREFIX "Deleting acronym markup in %s...\n"
#define EXIT_NO_LIST 1

bool prettyPrint = false;
int minimumSpaces = 2;
enum xmlFormat { BASIC, DEFLIST, TABLE } xmlFormat = BASIC;
bool interactive = false;
bool alwaysAsk = false;
bool deferChoice = false;
bool verbose = false;

xsltStylesheetPtr termStylesheet, idStylesheet;

void combineAcronymLists(xmlNodePtr dst, xmlNodePtr src)
{
	xmlNodePtr cur;

	for (cur = src->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, "acronym") == 0) {
			xmlAddChild(dst, xmlCopyNode(cur, 1));
		}
	}
}

void findAcronymsInFile(xmlNodePtr acronyms, const char *path)
{
	xmlDocPtr doc, styleDoc, result;
	xsltStylesheetPtr style;

	if (verbose) {
		fprintf(stderr, I_FIND, path);
	}

	if (!(doc = read_xml_doc(path))) {
		fprintf(stderr, E_NO_FILE, path);
		return;
	}

	styleDoc = read_xml_mem((const char *) stylesheets_acronyms_xsl, stylesheets_acronyms_xsl_len);

	style = xsltParseStylesheetDoc(styleDoc);

	result = xsltApplyStylesheet(style, doc, NULL);

	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	combineAcronymLists(acronyms, xmlDocGetRootElement(result));

	xmlFreeDoc(result);
}

xmlDocPtr removeNonUniqueAcronyms(xmlDocPtr doc)
{
	xmlDocPtr styleDoc, result;
	xsltStylesheetPtr style;

	styleDoc = read_xml_mem((const char *) stylesheets_unique_xsl, stylesheets_unique_xsl_len);
	style = xsltParseStylesheetDoc(styleDoc);
	result = xsltApplyStylesheet(style, doc, NULL);
	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	return result;
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

int longestAcronymTerm(xmlNodePtr acronyms)
{
	xmlNodePtr cur;
	int longest = 0;

	for (cur = acronyms->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, "acronym") == 0) {
			char *term;
			int len;

			term = (char *) xmlNodeGetContent(findChild(cur, "acronymTerm"));
			len = strlen(term);
			xmlFree(term);

			longest = len > longest ? len : longest;
		}
	}

	return longest;
}

void printAcronyms(xmlNodePtr acronyms, const char *path)
{
	xmlNodePtr cur;
	int longest = 0;
	FILE *out;

	if (strcmp(path, "-") == 0)
		out = stdout;
	else
		out = fopen(path, "w");
	
	if (prettyPrint)
		longest = longestAcronymTerm(acronyms);

	for (cur = acronyms->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, "acronym") == 0) {
			char *term = (char *) xmlNodeGetContent(findChild(cur, "acronymTerm"));
			char *defn = (char *) xmlNodeGetContent(findChild(cur, "acronymDefinition"));

			char *type = (char *) xmlGetProp(cur, BAD_CAST "acronymType");

			if (prettyPrint) {
				int len, nspaces, i;

				len = strlen(term);

				nspaces = longest - len + minimumSpaces;

				fprintf(out, "%s", term);
				for (i = 0; i < nspaces; ++i) {
					fputc(' ', out);
				}
				if (type) {
					fprintf(out, "%s", type);
				} else {
					fprintf(out, "    ");
				}
				for (i = 0; i < minimumSpaces; ++i) {
					fputc(' ', out);
				}
				fprintf(out, "%s\n", defn);
			} else {
				fprintf(out, "%s\t", term);
				if (type) {
					fprintf(out, "%s\t", type);
				} else {
					fprintf(out, "    \t");
				}
				fprintf(out, "%s\n", defn);
			}

			xmlFree(term);
			xmlFree(defn);
			xmlFree(type);
		}
	}

	if (out != stdout)
		fclose(out);
}

xmlDocPtr formatXmlAs(xmlDocPtr doc, unsigned char *src, unsigned int len)
{
	xmlDocPtr styleDoc, result;
	xsltStylesheetPtr style;

	styleDoc = read_xml_mem((const char *) src, len);

	style = xsltParseStylesheetDoc(styleDoc);

	result = xsltApplyStylesheet(style, doc, NULL);

	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	return result;
}

xmlDocPtr limitToTypes(xmlDocPtr doc, const char *types)
{
	xmlDocPtr styleDoc, result;
	xsltStylesheetPtr style;
	const char *params[3];
	char *typesParam;

	typesParam = malloc(strlen(types) + 3);
	sprintf(typesParam, "'%s'", types);

	params[0] = "types";
	params[1] = typesParam;
	params[2] = NULL;

	styleDoc = read_xml_mem((const char *) stylesheets_types_xsl, stylesheets_types_xsl_len);

	style = xsltParseStylesheetDoc(styleDoc);

	result = xsltApplyStylesheet(style, doc, params);

	free(typesParam);

	xmlFreeDoc(doc);
	xsltFreeStylesheet(style);

	return result;
}

xmlNodePtr firstXPathNode(char *xpath, xmlNodePtr from)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;

	ctx = xmlXPathNewContext(from->doc);
	ctx->node = from;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval))
		node = NULL;
	 else
	 	node = obj->nodesetval->nodeTab[0];
	
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return node;
}

xmlNodePtr chooseAcronym(xmlNodePtr acronym, xmlChar *term, xmlChar *content)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	xmlChar xpath[256];

	ctx = xmlXPathNewContext(acronym->doc);

	xmlStrPrintf(xpath, 256, "//acronym[acronymTerm = '%s']", term);

	obj = xmlXPathEvalExpression(xpath, ctx);

	if (deferChoice) {
		int i;

		acronym = xmlNewNode(NULL, BAD_CAST "chooseAcronym");

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlAddChild(acronym, xmlCopyNode(obj->nodesetval->nodeTab[i], 1));
		}
	} else if (alwaysAsk || obj->nodesetval->nodeNr > 1) {
		int i;

		printf("Found acronym term %s in the following context:\n\n", (char *) term);

		printf("%s\n\n", (char *) content);

		puts("Choose definition:");

		for (i = 0; i < obj->nodesetval->nodeNr && i < 9; ++i) {
			xmlNodePtr acronymDefinition = firstXPathNode("acronymDefinition", obj->nodesetval->nodeTab[i]);
			xmlChar *definition = xmlNodeGetContent(acronymDefinition);

			printf("%d) %s\n", i + 1, (char *) definition);

			xmlFree(definition);
		}

		puts("s) Ignore this one");

		fflush(stdout);

		i = getchar();

		if (i < '1' || i > '9') {
			acronym = NULL;
		} else {
			acronym = obj->nodesetval->nodeTab[i - 49];
		}

		while ((i = getchar()) != EOF && i != '\n');
		putchar('\n');
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return acronym;
}

bool isAcronymTerm(xmlChar *content, int contentLen, int i, xmlChar *term, int termLen)
{
	bool isTerm;
	xmlChar s, e;

	s = i == 0 ? ' ' : content[i - 1];
	e = i + termLen >= contentLen - 1 ? ' ' : content[i + termLen];

	isTerm = xmlStrchr(PRE_ACRONYM_DELIM, s) &&
	         xmlStrncmp(content + i, term, termLen) == 0 &&
		 xmlStrchr(POST_ACRONYM_DELIM, e);

	return isTerm;
}

void markupAcronymInNode(xmlNodePtr node, xmlNodePtr acronym)
{
	xmlChar *content;
	xmlChar *term;
	int termLen, contentLen;
	int i;

	content = xmlNodeGetContent(node);
	contentLen = xmlStrlen(content);

	term = xmlNodeGetContent(firstXPathNode("acronymTerm", acronym));
	termLen = xmlStrlen(term);

	i = 0;
	while (i + termLen <= contentLen) {
		if (isAcronymTerm(content, contentLen, i, term, termLen)) {
			xmlChar *s1 = xmlStrndup(content, i);
			xmlChar *s2 = xmlStrsub(content, i + termLen, contentLen - (i + termLen));
			xmlNodePtr acr = acronym;

			if (interactive) {
				acr = chooseAcronym(acronym, term, content);
			}

			xmlFree(content);

			xmlNodeSetContent(node, s1);
			xmlFree(s1);

			if (acr) {
				acr = xmlAddNextSibling(node, xmlCopyNode(acr, 1));
			} else {
				acr = xmlAddNextSibling(node, xmlNewNode(NULL, BAD_CAST "ignoredAcronym"));
				xmlNodeSetContent(acr, term);
			}

			node = xmlAddNextSibling(acr, xmlNewText(s2));

			content = s2;
			contentLen = xmlStrlen(s2);
			i = 0;
		} else {
			++i;
		}
	}

	xmlFree(term);
	xmlFree(content);
}

void markupAcronyms(xmlDocPtr doc, xmlNodePtr acronyms)
{
	xmlNodePtr cur;

	for (cur = acronyms->children; cur; cur = cur->next) {
		if (xmlStrcmp(cur->name, BAD_CAST "acronym") == 0) {
			xmlXPathContextPtr ctx;
			xmlXPathObjectPtr obj;

			ctx = xmlXPathNewContext(doc);

			obj = xmlXPathEvalExpression(acro_markup_xpath, ctx);

			if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
				int i;

				for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
					markupAcronymInNode(obj->nodesetval->nodeTab[i], cur);
				}
			}

			xmlXPathFreeObject(obj);
			xmlXPathFreeContext(ctx);
		}
	}
}

xmlDocPtr matchAcronymTerms(xmlDocPtr doc)
{
	xmlDocPtr res, orig;
	xmlNodePtr old;

	orig = xmlCopyDoc(doc, 1);

	res = xsltApplyStylesheet(termStylesheet, doc, NULL);
	xmlFreeDoc(doc);
	doc = res;
	res = xsltApplyStylesheet(idStylesheet, doc, NULL);
	xmlFreeDoc(doc);

	old = xmlDocSetRootElement(orig, xmlCopyNode(xmlDocGetRootElement(res), 1));
	xmlFreeNode(old);
	xmlFreeDoc(res);

	return orig;
}

void transformDoc(xmlDocPtr doc, unsigned char *xsl, unsigned int len)
{
	xmlDocPtr styledoc, src, res;
	xsltStylesheetPtr style;
	xmlNodePtr old;

	src = xmlCopyDoc(doc, 1);

	styledoc = read_xml_mem((const char *) xsl, len);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, src, NULL);

	old = xmlDocSetRootElement(doc, xmlCopyNode(xmlDocGetRootElement(res), 1));
	xmlFreeNode(old);
	
	xmlFreeDoc(src);
	xmlFreeDoc(res);
	xsltFreeStylesheet(style);
}

void convertToIssue30(xmlDocPtr doc)
{
	transformDoc(doc, stylesheets_30_xsl, stylesheets_30_xsl_len);
}

void markupAcronymsInFile(const char *path, xmlNodePtr acronyms, const char *out)
{
	xmlDocPtr doc;

	if (verbose) {
		fprintf(stderr, I_MARKUP, path);
	}

	if (!(doc = read_xml_doc(path))) {
		fprintf(stderr, E_NO_FILE, path);
		return;
	}

	markupAcronyms(doc, acronyms);

	doc = matchAcronymTerms(doc);

	if (xmlStrcmp(xmlFirstElementChild(xmlDocGetRootElement(doc))->name, BAD_CAST "idstatus") == 0) {
		convertToIssue30(doc);
	}

	save_xml_doc(doc, out);

	xmlFreeDoc(doc);
}

xmlDocPtr sortAcronyms(xmlDocPtr doc)
{
	xmlDocPtr sortdoc;
	xsltStylesheetPtr sort;
	xmlDocPtr sorted;

	sortdoc = read_xml_mem((const char *) stylesheets_sort_xsl, stylesheets_sort_xsl_len);
	sort = xsltParseStylesheetDoc(sortdoc);
	sorted = xsltApplyStylesheet(sort, doc, NULL);
	xmlFreeDoc(doc);
	xsltFreeStylesheet(sort);
	return sorted;
}

void markupAcronymsInList(const char *fname, xmlNodePtr acronyms, const char *out, bool overwrite)
{
	FILE *f;
	char line[PATH_MAX];

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			fprintf(stderr, E_BAD_LIST, fname);
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");

		if (overwrite) {
			markupAcronymsInFile(line, acronyms, line);
		} else {
			markupAcronymsInFile(line, acronyms, out);
		}
	}

	fclose(f);
}

void findAcronymsInList(xmlNodePtr acronyms, const char *fname)
{
	FILE *f;
	char line[PATH_MAX];

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			fprintf(stderr, E_BAD_LIST, fname);
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");
		findAcronymsInFile(acronyms, line);
	}

	fclose(f);
}

void deleteAcronyms(xmlDocPtr doc)
{
	transformDoc(doc, stylesheets_delete_xsl, stylesheets_delete_xsl_len);
}

void deleteAcronymsInFile(const char *fname, const char *out)
{
	xmlDocPtr doc;

	if (verbose) {
		fprintf(stderr, I_DELETE, fname);
	}

	doc = read_xml_doc(fname);
	
	deleteAcronyms(doc);

	save_xml_doc(doc, out);

	xmlFreeDoc(doc);
}

void deleteAcronymsInList(const char *fname, const char *out, bool overwrite)
{
	FILE *f;
	char line[PATH_MAX];

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			fprintf(stderr, E_BAD_LIST, fname);
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");
		
		if (overwrite) {
			deleteAcronymsInFile(line, line);
		} else {
			deleteAcronymsInFile(line, out);
		}
	}

	fclose(f);
}

void showHelp(void)
{
	puts("Usage:");
	puts("  " PROG_NAME " -h?");
	puts("  " PROG_NAME " [-dlptvx] [-n <#>] [-o <file>] [-T <types>] [<dmodules>]");
	puts("  " PROG_NAME " [-m|-M <list>] [-i|-I|-!] [-flv] [-o <file>] [-X <xpath>] [<dmodules>]");
	puts("  " PROG_NAME " -D [-flv] [-o <file>] [<dmodules>]");
	puts("");
	puts("Options:");
	puts("  -D          Remove acronym markup.");
	puts("  -d          Format XML output as definitionList.");
	puts("  -f          Overwrite data modules when marking up acronyms.");
	puts("  -i -I -!    Markup acronyms in interactive modes.");
	puts("  -l          Input is a list of file names.");
	puts("  -M <list>   Markup acronyms from specified list.");
	puts("  -m          Markup acronyms from .acronyms file.");
	puts("  -n <#>      Minimum spaces after term in pretty printed output.");
	puts("  -o <file>   Output to <file> instead of stdout.");
	puts("  -p          Pretty print text/XML output.");
	puts("  -T <types>  Only search for acronyms of these types.");
	puts("  -t          Format XML output as table.");
	puts("  -v          Verbose output.");
	puts("  -X <xpath>  Use custom XPath to markup elements.");
	puts("  -x          Output XML instead of text.");
	puts("  -h -?       Show usage message.");
	puts("  --version   Show version information.");
	LIBXML2_PARSE_LONGOPT_HELP
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

int main(int argc, char **argv)
{
	int i;

	xmlDocPtr doc = NULL;
	xmlNodePtr acronyms;

	bool xmlOut = false;
	char *types = NULL;
	char *out = strdup("-");
	char *markup = NULL;
	bool overwrite = false;
	bool list = false;
	bool delete = false;

	const char *sopts = "pn:xDdtT:o:M:miIfl!X:vh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		LIBXML2_PARSE_LONGOPT_DEFS
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
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'p':
				prettyPrint = true;
				break;
			case 'n':
				minimumSpaces = atoi(optarg);
				break;
			case 'x':
				xmlOut = true;
				break;
			case 'D':
				delete = true;
				break;
			case 'd':
				xmlFormat = DEFLIST;
				break;
			case 't':
				xmlFormat = TABLE;
				break;
			case 'T':
				types = strdup(optarg);
				break;
			case 'o':
				free(out);
				out = strdup(optarg);
				break;
			case 'm':
				markup = malloc(PATH_MAX);
				find_config(markup, DEFAULT_ACRONYMS_FNAME);
				break;
			case 'M':
				markup = strdup(optarg);
				break;
			case 'i':
				interactive = true;
				break;
			case 'I':
				interactive = true;
				alwaysAsk = true;
				break;
			case 'f':
				overwrite = true;
				break;
			case 'l':
				list = true;
				break;
			case '!':
				interactive = true;
				deferChoice = true;
				break;
			case 'X':
				if (!acro_markup_xpath) {
					acro_markup_xpath = xmlStrdup(BAD_CAST optarg);
				}
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

	if (!acro_markup_xpath) {
		acro_markup_xpath = xmlStrdup(ACRO_MARKUP_XPATH);
	}

	if (delete) {
		if (optind >= argc) {
			if (list) {
				deleteAcronymsInList(NULL, out, overwrite);
			} else {
				deleteAcronymsInFile("-", out);
			}
		}

		for (i = optind; i < argc; ++i) {
			if (list) {
				deleteAcronymsInList(argv[i], out, overwrite);
			} else if (overwrite) {
				deleteAcronymsInFile(argv[i], argv[i]);
			} else {
				deleteAcronymsInFile(argv[i], out);
			}
		}
	} else if (markup) {
		xmlDocPtr termStylesheetDoc, idStylesheetDoc;

		if (!(doc = read_xml_doc(markup))) {
			fprintf(stderr, E_NO_LIST, markup);
			exit(EXIT_NO_LIST);
		}

		doc = sortAcronyms(doc);
		acronyms = xmlDocGetRootElement(doc);

		termStylesheetDoc = read_xml_mem((const char *) stylesheets_term_xsl,
			stylesheets_term_xsl_len);
		idStylesheetDoc = read_xml_mem((const char *) stylesheets_id_xsl,
			stylesheets_id_xsl_len);

		termStylesheet = xsltParseStylesheetDoc(termStylesheetDoc);
		idStylesheet = xsltParseStylesheetDoc(idStylesheetDoc);

		if (optind >= argc) {
			if (list) {
				markupAcronymsInList(NULL, acronyms, out, overwrite);
			} else {
				markupAcronymsInFile("-", acronyms, out);
			}
		}

		for (i = optind; i < argc; ++i) {
			if (list) {
				markupAcronymsInList(argv[i], acronyms, out, overwrite);
			} else if (overwrite) {
				markupAcronymsInFile(argv[i], acronyms, argv[i]);
			} else {
				markupAcronymsInFile(argv[i], acronyms, out);
			}
		}

		xsltFreeStylesheet(termStylesheet);
		xsltFreeStylesheet(idStylesheet);
	} else {
		doc = xmlNewDoc(BAD_CAST "1.0");
		acronyms = xmlNewNode(NULL, BAD_CAST "acronyms");
		xmlDocSetRootElement(doc, acronyms);

		if (optind >= argc) {
			if (list) {
				findAcronymsInList(acronyms, NULL);
			} else {
				findAcronymsInFile(acronyms, "-");
			}
		}

		for (i = optind; i < argc; ++i) {
			if (list) {
				findAcronymsInList(acronyms, argv[i]);
			} else {
				findAcronymsInFile(acronyms, argv[i]);
			}
		}

		doc = removeNonUniqueAcronyms(doc);

		if (types)
			doc = limitToTypes(doc, types);

		if (xmlOut) {
			switch (xmlFormat) {
				case DEFLIST:
					doc = formatXmlAs(doc, stylesheets_list_xsl, stylesheets_list_xsl_len);
					break;
				case TABLE:
					doc = formatXmlAs(doc, stylesheets_table_xsl, stylesheets_table_xsl_len);
					break;
				default:
					break;
			}

			if (prettyPrint) {
				xmlSaveFormatFile(out, doc, 1);
			} else {
				save_xml_doc(doc, out);
			}
		} else {
			printAcronyms(xmlDocGetRootElement(doc), out);
		}
	}

	free(types);
	free(out);
	free(markup);

	xmlFreeDoc(doc);

	xmlFree(acro_markup_xpath);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
