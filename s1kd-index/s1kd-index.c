#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-index"

/* Path to text nodes where indexFlags may occur */
#define ELEMENTS_XPATH BAD_CAST "//para/text()"

#define PRE_TERM_DELIM BAD_CAST " "
#define POST_TERM_DELIM BAD_CAST " .,"

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

/* Help/usage message */
void show_help(void)
{
	puts("Usage: " PROG_NAME " [-I <index>] [-fih?] [<module>...]");
	puts("");
	puts("Options:");
	puts("  -f          Overwrite input module(s).");
	puts("  -I <index>  XML file containing index flags.");
	puts("  -i          Ignore case when flagging terms.");
	puts("  -h -?       Show help/usage message.");
}

/* Return the lowest level in an indexFlag. This is matched against the text
 * to determine where to insert the flag.
 */
xmlChar *last_level(xmlNodePtr flag)
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

bool is_term(xmlChar *content, int content_len, int i, xmlChar *term, int term_len, bool ignorecase)
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
void gen_index_node(xmlNodePtr node, xmlNodePtr flag, bool ignorecase)
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
void gen_index_flag(xmlNodePtr flag, xmlXPathContextPtr ctx, bool ignorecase)
{
	xmlXPathObjectPtr obj;

	obj = xmlXPathEvalExpression(ELEMENTS_XPATH, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			gen_index_node(obj->nodesetval->nodeTab[i], flag, ignorecase);
		}
	}
}

/* Insert indexFlags for each term included in the specified index file. */
void gen_index_flags(xmlNodeSetPtr flags, xmlXPathContextPtr ctx, bool ignorecase)
{
	int i;

	for (i = 0; i < flags->nodeNr; ++i) {
		gen_index_flag(flags->nodeTab[i], ctx, ignorecase);
	}
}

/* Insert indexFlag elements after matched terms in a document. */
void gen_index(const char *path, xmlDocPtr index_doc, bool overwrite, bool ignorecase)
{
	xmlDocPtr doc;
	xmlXPathContextPtr doc_ctx, index_ctx;
	xmlXPathObjectPtr index_obj;
	xmlNodeSetPtr flags;

	index_ctx = xmlXPathNewContext(index_doc);
	index_obj = xmlXPathEvalExpression(BAD_CAST "//indexFlag", index_ctx);
	flags = index_obj->nodesetval;

	doc = xmlReadFile(path, NULL, PARSE_OPTS);
	doc_ctx = xmlXPathNewContext(doc);

	if (!xmlXPathNodeSetIsEmpty(flags)) {
		gen_index_flags(flags, doc_ctx, ignorecase);
	}

	xmlXPathFreeContext(doc_ctx);
	xmlXPathFreeObject(index_obj);
	xmlXPathFreeContext(index_ctx);

	if (overwrite) {
		xmlSaveFile(path, doc);
	} else {
		xmlSaveFile("-", doc);
	}

	xmlFreeDoc(doc);
}


int main(int argc, char **argv)
{
	int i;
	bool overwrite = false;
	bool ignorecase = false;

	xmlDocPtr index_doc = NULL;

	while ((i = getopt(argc, argv, "fI:ih?")) != -1) {
		switch (i) {
			case 'f':
				overwrite = true;
				break;
			case 'I':
				if (!index_doc) {
					index_doc = xmlReadFile(optarg, NULL, PARSE_OPTS);
				}
				break;
			case 'i':
				ignorecase = true;
				break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			gen_index(argv[i], index_doc, overwrite, ignorecase);
		}
	} else {
		gen_index("-", index_doc, false, ignorecase);
	}

	xmlFreeDoc(index_doc);

	return 0;
}
