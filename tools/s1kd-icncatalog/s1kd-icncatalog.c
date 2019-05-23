#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>

#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/xpath.h>

#include "templates.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-icncatalog"
#define VERSION "2.0.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define INF_PREFIX PROG_NAME ": INFO: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"
#define I_RESOLVE INF_PREFIX "Resolving ICN references in %s...\n"

bool verbose = false;

/* Return the first node matching an XPath expression. */
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

/* Check whether an ICN is used in an object. */
bool icn_is_used(xmlDocPtr doc, const xmlChar *ident)
{
	xmlChar xpath[256];
	xmlStrPrintf(xpath, 256, "//@*[.='%s']", ident);
	return first_xpath_node(doc, NULL, xpath) != NULL;
}

/* Resolve the ICNs in a document against the ICN catalog. */
void resolve_icn(xmlDocPtr doc, xmlDocPtr icns, const xmlChar *ident, const xmlChar *uri, const xmlChar *notation)
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
	} else if (icn_is_used(doc, ident)) {
		if (notation) {
			add_notation_ref(doc, icns, notation);
			xmlAddDocEntity(doc, ident, XML_EXTERNAL_GENERAL_UNPARSED_ENTITY, NULL, uri, notation);
		} else {
			add_icn(doc, (char *) uri, true);
		}
	}
}

/* Resolve ICNs in a file against the ICN catalog. */
void resolve_icns_in_file(const char *fname, xmlDocPtr icns, bool overwrite, const char *media)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlChar xpath[256];

	if (verbose) {
		fprintf(stderr, I_RESOLVE, fname);
	}

	if (!(doc = read_xml_doc(fname))) {
		return;
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
		save_xml_doc(doc, fname);
	} else {
		save_xml_doc(doc, "-");
	}

	xmlFreeDoc(doc);
}

/* Resolve ICNs in objects in a list of file names. */
void resolve_icns_in_list(const char *path, xmlDocPtr icns, bool overwrite, const char *media)
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
		resolve_icns_in_file(line, icns, overwrite, media);
	}

	if (path) {
		fclose(f);
	}
}

/* Add ICNs to a catalog. */
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

/* Remove ICNs from a catalog. */
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
	puts("  -a, --add <icn>         Add an ICN to the catalog.");
	puts("  -c, --catalog <catalog> Use <catalog> as the ICN catalog.");
	puts("  -d, --del <icn>         Delete an ICN from the catalog.");
	puts("  -f, --overwrite         Overwrite input objects.");
	puts("  -h, -?, --help          Show help/usage message.");
	puts("  -l, --list              Treat input as list of objects.");
	puts("  -m, --media <media>     Specify intended output media.");
	puts("  -n, --ndata <notation>  Set the notation of the new ICN.");
	puts("  -t, --new               Create new ICN catalog.");
	puts("  -u, --uri <uri>         Set the URI of the new ICN.");
	puts("  -v, --verbose           Verbose output.");
	puts("  --version               Show version information.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Show version information. */
void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

int main(int argc, char **argv)
{
	int i;
	bool overwrite = false;
	char *icns_fname = NULL;
	bool createnew = false;
	char *media = NULL;
	xmlDocPtr icns;
	xmlNodePtr add, del, cur = NULL;
	bool islist = false;

	const char *sopts = "a:c:d:flm:n:tu:vxh?";
	struct option lopts[] = {
		{"version"  , no_argument      , 0, 0},
		{"help"     , no_argument      , 0, 'h'},
		{"add"      , required_argument, 0, 'a'},
		{"catalog"  , required_argument, 0, 'c'},
		{"del"      , required_argument, 0, 'd'},
		{"overwrite", no_argument      , 0, 'f'},
		{"list"     , no_argument      , 0, 'l'},
		{"media"    , required_argument, 0, 'm'},
		{"ndata"    , required_argument, 0, 'n'},
		{"new"      , no_argument      , 0, 't'},
		{"uri"      , required_argument, 0, 'u'},
		{"verbose"  , no_argument      , 0, 'v'},
		LIBXML2_PARSE_LONGOPT_DEFS
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
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
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
			case 'v':
				verbose = true;
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
		icns = read_xml_mem((const char *) icncatalog_xml, icncatalog_xml_len);
	} else {
		icns = read_xml_doc(icns_fname);
	}

	if (add->children || del->children) {
		if (add->children) {
			add_icns(icns, add, media);
		}
		if (del->children) {
			del_icns(icns, del, media);
		}
		if (overwrite) {
			save_xml_doc(icns, icns_fname);
		} else {
			save_xml_doc(icns, "-");
		}
	} else if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (islist) {
				resolve_icns_in_list(argv[i], icns, overwrite, media);
			} else {
				resolve_icns_in_file(argv[i], icns, overwrite, media);
			}
		}
	} else if (createnew) {
		if (overwrite) {
			save_xml_doc(icns, icns_fname);
		} else {
			save_xml_doc(icns, "-");
		}
	} else if (islist) {
		resolve_icns_in_list(NULL, icns, overwrite, media);
	} else {
		resolve_icns_in_file("-", icns, false, media);
	}

	free(icns_fname);
	free(media);
	xmlFreeNode(add);
	xmlFreeNode(del);
	xmlFreeDoc(icns);
	xmlCleanupParser();

	return 0;
}
