#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxslt/transform.h>

#include "xslt.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-index"
#define VERSION "1.7.1"

/* Path to text nodes where indexFlags may occur */
#define ELEMENTS_XPATH BAD_CAST "//para/text()"

#define PRE_TERM_DELIM BAD_CAST " "
#define POST_TERM_DELIM BAD_CAST " .,"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define INF_PREFIX PROG_NAME ": INFO: "
#define E_NO_LIST ERR_PREFIX "Could not read index flags from %s\n"
#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"
#define E_NO_FILE ERR_PREFIX "Could not read file: %s\n"
#define I_MARKUP INF_PREFIX "Adding index flags to %s...\n"
#define I_DELETE INF_PREFIX "Deleting index flags from %s...\n"
#define EXIT_NO_LIST 1

static bool verbose = false;

/* Help/usage message */
static void show_help(void)
{
	puts("Usage:");
	puts("  " PROG_NAME " -h?");
	puts("  " PROG_NAME " [-I <index>] [-filv] [<module>...]");
	puts("  " PROG_NAME " -D [-filv] [<module>...]");
	puts("");
	puts("Options:");
	puts("  -D, --delete              Delete current index flags.");
	puts("  -f, --overwrite           Overwrite input module(s).");
	puts("  -h, -?, --help            Show help/usage message.");
	puts("  -I, --indexflags <index>  Specify a custom .indexflags file");
	puts("  -i, --ignore-case         Ignore case when flagging terms.");
	puts("  -l, --list                Input is a list of file names.");
	puts("  -v, --verbose             Verbose output.");
	puts("  --version                 Show version information.");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

/* Return the lowest level in an indexFlag. This is matched against the text
 * to determine where to insert the flag.
 */
static xmlChar *last_level(xmlNodePtr flag)
{
	xmlChar *lvl;

	if ((lvl = xmlGetProp(flag, BAD_CAST "indexLevelFour"))) {
		return lvl;
	} else if ((lvl = xmlGetProp(flag, BAD_CAST "indexLevelThree"))) {
		return lvl;
	} else if ((lvl = xmlGetProp(flag, BAD_CAST "indexLevelTwo"))) {
		return lvl;
	} else if ((lvl = xmlGetProp(flag, BAD_CAST "indexLevelOne"))) {
		return lvl;
	}

	return NULL;
}

static bool is_term(xmlChar *content, int content_len, int i, xmlChar *term, int term_len, bool ignorecase)
{
	bool is;
	xmlChar s, e;

	s = i == 0 ? ' ' : content[i - 1];
	e = i + term_len >= content_len - 1 ? ' ' : content[i + term_len];

	is = xmlStrchr(PRE_TERM_DELIM, s) &&
	     (ignorecase ?
	     	xmlStrncasecmp(content + i, term, term_len) :
		xmlStrncmp(content + i, term, term_len)) == 0 &&
	     xmlStrchr(POST_TERM_DELIM, e);

	return is;
}

/* Insert indexFlag elements after matched terms. */
static void gen_index_node(xmlNodePtr node, xmlNodePtr flag, bool ignorecase)
{
	xmlChar *content;
	xmlChar *term;
	int term_len, content_len;
	int i;

	content = xmlNodeGetContent(node);
	content_len = xmlStrlen(content);

	term = last_level(flag);
	term_len = xmlStrlen(term);

	i = 0;
	while (i + term_len <= content_len) {
		if (is_term(content, content_len, i, term, term_len, ignorecase)) {
			xmlChar *s1 = xmlStrndup(content, i + term_len);
			xmlChar *s2 = xmlStrsub(content, i + term_len, content_len - (i + term_len));
			xmlNodePtr acr;

			xmlFree(content);

			xmlNodeSetContent(node, s1);
			xmlFree(s1);

			acr = xmlAddNextSibling(node, xmlCopyNode(flag, 1));
			node = xmlAddNextSibling(acr, xmlNewText(s2));

			content = s2;
			content_len = xmlStrlen(s2);
			i = 0;
		} else {
			++i;
		}
	}

	xmlFree(term);
	xmlFree(content);
}

/* Flag an individual term in all applicable elements in a module. */
static void gen_index_flag(xmlNodePtr flag, xmlXPathContextPtr ctx, bool ignorecase)
{
	xmlXPathObjectPtr obj;

	obj = xmlXPathEvalExpression(ELEMENTS_XPATH, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			gen_index_node(obj->nodesetval->nodeTab[i], flag, ignorecase);
		}
	}

	xmlXPathFreeObject(obj);
}

/* Insert indexFlags for each term included in the specified index file. */
static void gen_index_flags(xmlNodeSetPtr flags, xmlXPathContextPtr ctx, bool ignorecase)
{
	int i;

	for (i = 0; i < flags->nodeNr; ++i) {
		gen_index_flag(flags->nodeTab[i], ctx, ignorecase);
	}
}

/* Apply a built-in XSLT transform to a doc in place. */
static void transform_doc(xmlDocPtr doc, unsigned char *xsl, unsigned int len)
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

/* Convert index flags for older issues. */
static void convert_to_iss_30(xmlDocPtr doc)
{
	transform_doc(doc, iss30_xsl, iss30_xsl_len);
}

static void delete_index_flags(const char *path, bool overwrite)
{
	xmlDocPtr doc;

	if (verbose) {
		fprintf(stderr, I_DELETE, path);
	}

	doc = read_xml_doc(path);

	transform_doc(doc, delete_xsl, delete_xsl_len);

	if (overwrite) {
		save_xml_doc(doc, path);
	} else {
		save_xml_doc(doc, "-");
	}
}

/* Insert indexFlag elements after matched terms in a document. */
static void gen_index(const char *path, xmlDocPtr index_doc, bool overwrite, bool ignorecase)
{
	xmlDocPtr doc;
	xmlXPathContextPtr doc_ctx, index_ctx;
	xmlXPathObjectPtr index_obj;
	xmlNodeSetPtr flags;

	if (verbose) {
		fprintf(stderr, I_MARKUP, path);
	}

	if (!(doc = read_xml_doc(path))) {
		fprintf(stderr, E_NO_FILE, path);
		return;
	}

	index_ctx = xmlXPathNewContext(index_doc);
	index_obj = xmlXPathEvalExpression(BAD_CAST "//indexFlag", index_ctx);
	flags = index_obj->nodesetval;

	doc_ctx = xmlXPathNewContext(doc);

	if (!xmlXPathNodeSetIsEmpty(flags)) {
		gen_index_flags(flags, doc_ctx, ignorecase);
	}

	xmlXPathFreeContext(doc_ctx);
	xmlXPathFreeObject(index_obj);
	xmlXPathFreeContext(index_ctx);

	if (xmlStrcmp(xmlFirstElementChild(xmlDocGetRootElement(doc))->name, BAD_CAST "idstatus") == 0) {
		convert_to_iss_30(doc);
	}

	if (overwrite) {
		save_xml_doc(doc, path);
	} else {
		save_xml_doc(doc, "-");
	}

	xmlFreeDoc(doc);
}

static xmlDocPtr read_index_flags(const char *fname)
{
	xmlDocPtr index_doc;

	if (!(index_doc = read_xml_doc(fname))) {
		fprintf(stderr, E_NO_LIST, fname);
		exit(EXIT_NO_LIST);
	}

	return index_doc;
}

static void handle_list(const char *path, bool delflags, xmlDocPtr index_doc, bool overwrite, bool ignorecase)
{
	FILE *f;
	char line[PATH_MAX];

	if (path) {
		f = fopen(path, "r");
	} else {
		f = stdin;
	}

	if (!f) {
		fprintf(stderr, E_BAD_LIST, path);
		return;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");

		if (delflags) {
			delete_index_flags(line, overwrite);
		} else {
			gen_index(line, index_doc, overwrite, ignorecase);
		}
	}

	fclose(f);
}

int main(int argc, char **argv)
{
	int i;
	bool overwrite = false;
	bool ignorecase = false;
	bool delflags = false;
	bool list = false;

	xmlDocPtr index_doc = NULL;

	const char *sopts = "DfI:livh?";
	struct option lopts[] = {
		{"version"    , no_argument      , 0, 0},
		{"help"       , no_argument      , 0, 'h'},
		{"delete"     , no_argument      , 0, 'D'},
		{"overwrite"  , no_argument      , 0, 'f'},
		{"indexflags" , required_argument, 0, 'I'},
		{"ignore-case", no_argument      , 0, 'i'},
		{"list"       , no_argument      , 0, 'l'},
		{"verbose"    , no_argument      , 0, 'v'},
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
			case 'D':
				delflags = true;
				break;
			case 'f':
				overwrite = true;
				break;
			case 'I':
				if (!index_doc) {
					index_doc = read_index_flags(optarg);
				}
				break;
			case 'i':
				ignorecase = true;
				break;
			case 'l':
				list = true;
				break;
			case 'v':
				verbose = true;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (!index_doc && !delflags) {
		char fname[PATH_MAX];
		find_config(fname, DEFAULT_INDEXFLAGS_FNAME);
		index_doc = read_index_flags(fname);
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (list) {
				handle_list(argv[i], delflags, index_doc, overwrite, ignorecase);
			} else if (delflags) {
				delete_index_flags(argv[i], overwrite);
			} else {
				gen_index(argv[i], index_doc, overwrite, ignorecase);
			}
		}
	} else if (list) {
		handle_list(NULL, delflags, index_doc, overwrite, ignorecase);
	} else if (delflags) {
		delete_index_flags("-", false);
	} else {
		gen_index("-", index_doc, false, ignorecase);
	}

	xmlFreeDoc(index_doc);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
