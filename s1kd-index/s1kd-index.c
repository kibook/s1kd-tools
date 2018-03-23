#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-index"

/* Path to text nodes where indexFlags may occur */
#define ELEMENTS_XPATH BAD_CAST "//para/text()"

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

/* Insert indexFlag elements after matched terms. */
void gen_index_node(xmlNodePtr node, xmlNodePtr flag, bool ignorecase)
{
	xmlChar *flagtext, *s1;
	const xmlChar *s2;

	flagtext = last_level(flag);
	s1 = xmlNodeGetContent(node);

	if (ignorecase) {
		s2 = xmlStrcasestr(s1, flagtext);
	} else {
		s2 = xmlStrstr(s1, flagtext);
	}

	if (s2) {
		xmlChar *s3;

		s2 = s2 + xmlStrlen(flagtext);
		s3 = xmlStrndup(s1, s2 - s1);

		xmlNodeSetContent(node, s3);
		flag = xmlAddNextSibling(node, xmlCopyNode(flag, 1));
		node = xmlAddNextSibling(flag, xmlNewText(s2));

		xmlFree(s3);
	}

	xmlFree(s1);
	xmlFree(flagtext);
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

	doc = xmlReadFile(path, NULL, 0);
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
					index_doc = xmlReadFile(optarg, NULL, 0);
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
