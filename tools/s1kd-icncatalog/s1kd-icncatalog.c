#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>

#include <libxml/tree.h>
#include <libxml/xinclude.h>
#include <libxml/valid.h>
#include <libxml/xpath.h>

#include "templates.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-icncatalog"
#define VERSION "1.2.4"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

/* Not exposed by the libxml API */
static void xmlFreeEntity(xmlEntityPtr entity)
{
    xmlDictPtr dict = NULL;

    if (entity == NULL)
        return;

    if (entity->doc != NULL)
        dict = entity->doc->dict;


    if ((entity->children) && (entity->owner == 1) &&
        (entity == (xmlEntityPtr) entity->children->parent))
        xmlFreeNodeList(entity->children);
    if (dict != NULL) {
        if ((entity->name != NULL) && (!xmlDictOwns(dict, entity->name)))
            xmlFree((char *) entity->name);
        if ((entity->ExternalID != NULL) &&
	    (!xmlDictOwns(dict, entity->ExternalID)))
            xmlFree((char *) entity->ExternalID);
        if ((entity->SystemID != NULL) &&
	    (!xmlDictOwns(dict, entity->SystemID)))
            xmlFree((char *) entity->SystemID);
        if ((entity->URI != NULL) && (!xmlDictOwns(dict, entity->URI)))
            xmlFree((char *) entity->URI);
        if ((entity->content != NULL)
            && (!xmlDictOwns(dict, entity->content)))
            xmlFree((char *) entity->content);
        if ((entity->orig != NULL) && (!xmlDictOwns(dict, entity->orig)))
            xmlFree((char *) entity->orig);
    } else {
        if (entity->name != NULL)
            xmlFree((char *) entity->name);
        if (entity->ExternalID != NULL)
            xmlFree((char *) entity->ExternalID);
        if (entity->SystemID != NULL)
            xmlFree((char *) entity->SystemID);
        if (entity->URI != NULL)
            xmlFree((char *) entity->URI);
        if (entity->content != NULL)
            xmlFree((char *) entity->content);
        if (entity->orig != NULL)
            xmlFree((char *) entity->orig);
    }
    xmlFree(entity);
}

/* Add a NOTATION */
void add_notation(xmlDocPtr doc, const xmlChar *name, const xmlChar *pubId, const xmlChar *sysId)
{
	xmlValidCtxtPtr valid;

	if (!doc->intSubset) {
		return;
	}

	if (!xmlHashLookup(doc->intSubset->notations, BAD_CAST name)) {
		valid = xmlNewValidCtxt();
		xmlAddNotationDecl(valid, doc->intSubset, name, pubId, sysId);
		xmlFreeValidCtxt(valid);
	}
}

/* Add a notation by its reference in the catalog file. */
void add_notation_ref(xmlDocPtr doc, xmlDocPtr icns, const xmlChar *notation)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(icns);
	obj = xmlXPathEvalExpression(BAD_CAST "/icnCatalog/notation", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		bool found = false;

		for (i = 0; i < obj->nodesetval->nodeNr && !found; ++i) {
			xmlChar *name;

			name = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "name");

			if (xmlStrcmp(name, notation) == 0) {
				xmlChar *pubId, *sysId;

				pubId = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "publicId");
				sysId = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "systemId");

				add_notation(doc, name, pubId, sysId);

				xmlFree(pubId);
				xmlFree(sysId);

				found = true;
			}

			xmlFree(name);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Resolve the ICNs in a document against the ICN catalog. */
void resolve_icn(xmlDocPtr doc, xmlDocPtr icns, xmlChar *ident, xmlChar *uri, xmlChar *notation)
{
	xmlEntityPtr e;

	e = xmlGetDocEntity(doc, ident);

	if (e) {
		xmlChar *ndata;

		if (!notation) {
			ndata = xmlStrdup(e->content);
		}

		xmlUnlinkNode((xmlNodePtr) e);
		xmlFreeEntity(e);

		if (notation) {
			xmlAddDocEntity(doc, ident, XML_EXTERNAL_GENERAL_UNPARSED_ENTITY, NULL, uri, notation);
		} else {
			xmlAddDocEntity(doc, ident, XML_EXTERNAL_GENERAL_UNPARSED_ENTITY, NULL, uri, ndata);
		}

		if (notation) {
			add_notation_ref(doc, icns, notation);
		} else {
			xmlFree(ndata);
		}
	}
}

/* Resolve ICNs in a file against the ICN catalog. */
void resolve_icns_in_file(const char *fname, xmlDocPtr icns, bool overwrite, bool xinclude, const char *media)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlChar xpath[256];

	doc = xmlReadFile(fname, NULL, PARSE_OPTS);

	if (xinclude) {
		xmlXIncludeProcess(doc);
	}

	ctx = xmlXPathNewContext(icns);

	if (media) {
		xmlStrPrintf(xpath, 256, "/icnCatalog/media[@name='%s']/icn", media);
	} else {
		xmlStrPrintf(xpath, 256, "/icnCatalog/icn");
	}

	obj = xmlXPathEvalExpression(xpath, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *ident, *uri, *notation;

			ident    = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "infoEntityIdent");
			uri      = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "uri");
			notation = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "notation");

			resolve_icn(doc, icns, ident, uri, notation);

			xmlFree(ident);
			xmlFree(uri);
			xmlFree(notation);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	if (overwrite) {
		xmlSaveFile(fname, doc);
	} else {
		xmlSaveFile("-", doc);
	}

	xmlFreeDoc(doc);
}

void resolve_icns_in_list(const char *path, xmlDocPtr icns, bool overwrite, bool xinclude, const char *media)
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
		resolve_icns_in_file(line, icns, overwrite, xinclude, media);
	}

	if (path) {
		fclose(f);
	}
}

xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const xmlChar *expr)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(doc ? doc : node->doc);
	ctx->node = node;

	obj = xmlXPathEvalExpression(expr, ctx);

	first = xmlXPathNodeSetIsEmpty(obj->nodesetval) ? NULL : obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

void add_icns(xmlDocPtr icns, xmlNodePtr add, const char *media)
{
	xmlNodePtr root, cur;

	if (media) {
		xmlChar xpath[256];
		xmlStrPrintf(xpath, 256, "/icnCatalog/media[@name='%s']", media);
		root = first_xpath_node(icns, NULL, xpath);
	} else {
		root = xmlDocGetRootElement(icns);
	}


	for (cur = add->children; cur; cur = cur->next) {
		xmlAddChild(root, xmlCopyNode(cur, 1));
	}
}

void del_icns(xmlDocPtr icns, xmlNodePtr del, const char *media)
{
	xmlNodePtr cur;

	for (cur = del->children; cur; cur = cur->next) {
		xmlChar xpath[256];
		xmlChar *ident;
		xmlNodePtr icn;

		ident = xmlGetProp(cur, BAD_CAST "infoEntityIdent");
		if (media) {
			xmlStrPrintf(xpath, 256, "/icnCatalog/media[@name='%s']/icn[@infoEntityIdent='%s']", media, ident);
		} else {
			xmlStrPrintf(xpath, 256, "/icnCatalog/icn[@infoEntityIdent='%s']", ident);
		}
		xmlFree(ident);

		if ((icn = first_xpath_node(icns, NULL, xpath))) {
			xmlUnlinkNode(icn);
			xmlFreeNode(icn);
		}
	}
}

/* Help/usage message. */
void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -h -?  Show help/usage message.");
	puts("  -a <icn>       Add an ICN to the catalog.");
	puts("  -c <catalog>   Use <catalog> as the ICN catalog.");
	puts("  -d <icn>       Delete an ICN from the catalog.");
	puts("  -f             Overwrite input objects.");
	puts("  -l             Treat input as list of objects.");
	puts("  -m <media>     Specify intended output media.");
	puts("  -n <notation>  Set the notation of the new ICN.");
	puts("  -t             Create new ICN catalog.");
	puts("  -u <uri>       Set the URI of the new ICN.");
	puts("  -x             Process XInclude elements.");
	puts("  --version      Show version information.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int main(int argc, char **argv)
{
	int i;
	bool overwrite = false;
	bool xinclude = false;
	char *icns_fname = NULL;
	bool createnew = false;
	char *media = NULL;
	xmlDocPtr icns;
	xmlNodePtr add, del, cur = NULL;
	bool islist = false;

	const char *sopts = "a:c:d:flm:n:tu:xh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	add = xmlNewNode(NULL, BAD_CAST "add");
	del = xmlNewNode(NULL, BAD_CAST "del");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 'a':
				cur = xmlNewChild(add, NULL, BAD_CAST "icn", NULL);
				xmlSetProp(cur, BAD_CAST "infoEntityIdent", BAD_CAST optarg);
				break;
			case 'c':
				if (!icns_fname) {
					icns_fname = strdup(optarg);
				}
				break;
			case 'd':
				cur = xmlNewChild(del, NULL, BAD_CAST "icn", NULL);
				xmlSetProp(cur, BAD_CAST "infoEntityIdent", BAD_CAST optarg);
				break;
			case 'f':
				overwrite = true;
				break;
			case 'l':
				islist = true;
				break;
			case 'm':
				if (!media) {
					media = strdup(optarg);
				}
				break;
			case 'n':
				if (cur) {
					xmlSetProp(cur, BAD_CAST "notation", BAD_CAST optarg);
				}
				break;
			case 't':
				createnew = true;
				break;
			case 'u':
				if (cur) {
					xmlSetProp(cur, BAD_CAST "uri", BAD_CAST optarg);
				}
				break;
			case 'x':
				xinclude = true;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	if (!icns_fname) {
		icns_fname = malloc(PATH_MAX);
		find_config(icns_fname, DEFAULT_ICNCATALOG_FNAME);
	}

	if (createnew || access(icns_fname, F_OK) == -1) {
		icns = xmlReadMemory((const char *) icncatalog_xml, icncatalog_xml_len, NULL, NULL, 0);
	} else {
		icns = xmlReadFile(icns_fname, NULL, PARSE_OPTS);
	}

	if (add->children || del->children) {
		if (add->children) {
			add_icns(icns, add, media);
		}
		if (del->children) {
			del_icns(icns, del, media);
		}
		if (overwrite) {
			xmlSaveFile(icns_fname, icns);
		} else {
			xmlSaveFile("-", icns);
		}
	} else if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				resolve_icns_in_list(argv[i], icns, overwrite, xinclude, media);
			} else {
				resolve_icns_in_file(argv[i], icns, overwrite, xinclude, media);
			}
		}
	} else if (createnew) {
		if (overwrite) {
			xmlSaveFile(icns_fname, icns);
		} else {
			xmlSaveFile("-", icns);
		}
	} else if (islist) {
		resolve_icns_in_list(NULL, icns, overwrite, xinclude, media);
	} else {
		resolve_icns_in_file("-", icns, false, xinclude, media);
	}

	free(icns_fname);
	free(media);
	xmlFreeNode(add);
	xmlFreeNode(del);
	xmlFreeDoc(icns);
	xmlCleanupParser();

	return 0;
}
