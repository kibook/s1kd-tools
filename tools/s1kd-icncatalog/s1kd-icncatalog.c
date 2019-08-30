#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include <regex.h>

#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "templates.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-icncatalog"
#define VERSION "3.0.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define INF_PREFIX PROG_NAME ": INFO: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"
#define E_REGEX_INVALID ERR_PREFIX "Invalid regular expression: %s\n"
#define E_REGEX_BADREF ERR_PREFIX "Undefined reference in URI template: \\%c\n"
#define I_RESOLVE INF_PREFIX "Resolving ICN references in %s...\n"

static bool verbose = false;

/* Add a notation by its reference in the catalog file. */
static void add_notation_ref(xmlDocPtr doc, xmlDocPtr icns, const xmlChar *notation)
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
static bool icn_is_used(xmlDocPtr doc, const xmlChar *ident)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	bool used;

	ctx = xmlXPathNewContext(doc);
	xmlXPathRegisterVariable(ctx, BAD_CAST "id", xmlXPathNewString(ident));
	obj = xmlXPathEvalExpression(BAD_CAST "//@*[.=$id]", ctx);

	used = !xmlXPathNodeSetIsEmpty(obj->nodesetval);

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return used;
}

/* Replace the SYSTEM URI of an entity, adding a notation if necessary. */
static void replace_entity(xmlDocPtr doc, xmlDocPtr icns, xmlEntityPtr e, const xmlChar *ident, const xmlChar *uri, const xmlChar *notation)
{
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

/* Fill in backreferences in a string from a set of regex matches. */
#define BUF_MAX 256
static xmlChar *regex_replace(const xmlChar *icn, const xmlChar *uri, size_t nmatch, regmatch_t pmatch[])
{
	int i, n = 0;
	xmlChar buf[BUF_MAX];
	xmlChar *s;

	s = xmlStrdup(BAD_CAST "");

	for (i = 0; uri[i]; ++i) {
		if (uri[i] == '\\') {
			int ref = uri[++i] - '0';

			if (ref >= 0 && ref < nmatch && pmatch[ref].rm_so != -1) {
				s = xmlStrncat(s, buf, n);
				n = 0;
				s = xmlStrncat(s, icn + pmatch[ref].rm_so, pmatch[ref].rm_eo - pmatch[ref].rm_so);
			} else {
				fprintf(stderr, E_REGEX_BADREF, ref + '0');
			}
		} else {
			buf[n++] = uri[i];

			if (n == BUF_MAX) {
				s = xmlStrncat(s, buf, n);
				n = 0;
			}
		}
	}

	s = xmlStrncat(s, buf, n);

	return s;
}

/* Resolve an ICN using regular expressions. */
static void resolve_icn_regex(xmlDocPtr doc, xmlDocPtr icns, const xmlChar *pattern, const xmlChar *icn, const xmlChar *uri, const xmlChar *notation)
{
	regex_t re;
	regmatch_t *pmatch;
	size_t nmatch;

	if (regcomp(&re, (char *) pattern, REG_EXTENDED) != 0) {
		fprintf(stderr, E_REGEX_INVALID, (char *) pattern);
		return;
	}

	nmatch = re.re_nsub + 1;

	pmatch = malloc(sizeof(regmatch_t) * nmatch);

	if (regexec(&re, (char *) icn, nmatch, pmatch, 0) == 0) {
		xmlChar *s;
		xmlEntityPtr e;

		s = regex_replace(icn, uri, nmatch, pmatch);

		e = xmlGetDocEntity(doc, icn);

		if (e) {
			replace_entity(doc, icns, e, icn, s, notation);
		} else if (notation) {
			add_notation_ref(doc, icns, notation);
			xmlAddDocEntity(doc, icn, XML_EXTERNAL_GENERAL_UNPARSED_ENTITY, NULL, s, notation);
		} else {
			add_icn(doc, (char *) s, true);
		}

		xmlFree(s);
	}

	free(pmatch);

	regfree(&re);
}

/* Resolve an ICN pattern rule from the catalog. */
static void resolve_pattern_icn(xmlDocPtr doc, xmlDocPtr icns, const xmlChar *pattern, const xmlChar *uri, const xmlChar *notation)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST "//@infoEntityIdent", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *icn;

			icn = xmlNodeGetContent(obj->nodesetval->nodeTab[i]);

			resolve_icn_regex(doc, icns, pattern, icn, uri, notation);

			xmlFree(icn);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Resolve the ICNs in a document against the ICN catalog. */
static void resolve_icn(xmlDocPtr doc, xmlDocPtr icns, const xmlChar *ident, const xmlChar *uri, const xmlChar *notation)
{
	xmlEntityPtr e;

	e = xmlGetDocEntity(doc, ident);

	if (e) {
		replace_entity(doc, icns, e, ident, uri, notation);
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
static void resolve_icns_in_file(const char *fname, xmlDocPtr icns, bool overwrite, const char *media)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlChar *xpath;

	if (verbose) {
		fprintf(stderr, I_RESOLVE, fname);
	}

	if (!(doc = read_xml_doc(fname))) {
		return;
	}

	ctx = xmlXPathNewContext(icns);

	if (media) {
		xmlXPathRegisterVariable(ctx, BAD_CAST "media", xmlXPathNewString(BAD_CAST media));
		xpath = BAD_CAST "/icnCatalog/media[@name=$media]/icn";
	} else {
		xpath = BAD_CAST "/icnCatalog/icn";
	}

	obj = xmlXPathEvalExpression(xpath, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *type, *ident, *uri, *notation;

			type     = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "type");
			ident    = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "infoEntityIdent");
			uri      = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "uri");
			notation = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "notation");

			if (xmlStrcmp(type, BAD_CAST "pattern") == 0) {
				resolve_pattern_icn(doc, icns, ident, uri, notation);
			} else {
				resolve_icn(doc, icns, ident, uri, notation);
			}

			xmlFree(type);
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
static void resolve_icns_in_list(const char *path, xmlDocPtr icns, bool overwrite, const char *media)
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
static void add_icns(xmlDocPtr icns, xmlNodePtr add, const char *media)
{
	xmlNodePtr root, cur;

	if (media) {
		xmlXPathContextPtr ctx;
		xmlXPathObjectPtr obj;

		ctx = xmlXPathNewContext(icns);
		xmlXPathRegisterVariable(ctx, BAD_CAST "media", xmlXPathNewString(BAD_CAST media));

		obj = xmlXPathEvalExpression(BAD_CAST "/icnCatalog/media[@name=$media]", ctx);

		if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			root = NULL;
		} else {
			root = obj->nodesetval->nodeTab[0];
		}

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);
	} else {
		root = xmlDocGetRootElement(icns);
	}

	for (cur = add->children; cur; cur = cur->next) {
		xmlAddChild(root, xmlCopyNode(cur, 1));
	}
}

/* Remove ICNs from a catalog. */
static void del_icns(xmlDocPtr icns, xmlNodePtr del, const char *media)
{
	xmlNodePtr cur;
	xmlXPathContextPtr ctx;

	ctx = xmlXPathNewContext(icns);

	for (cur = del->children; cur; cur = cur->next) {
		xmlChar *ident, *xpath;
		xmlXPathObjectPtr obj;

		ident = xmlGetProp(cur, BAD_CAST "infoEntityIdent");
		xmlXPathRegisterVariable(ctx, BAD_CAST "id", xmlXPathNewString(ident));
		xmlFree(ident);

		if (media) {
			xmlXPathRegisterVariable(ctx, BAD_CAST "media", xmlXPathNewString(BAD_CAST media));
			xpath = BAD_CAST "/icnCatalog/media[@name=$media]/icn[@infoEntityIdent=$id]";
		} else {
			xpath = BAD_CAST "/icnCatalog/icn[@infoEntityIdent=$id]";
		}

		obj = xmlXPathEvalExpression(xpath, ctx);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			xmlUnlinkNode(obj->nodesetval->nodeTab[0]);
			xmlFreeNode(obj->nodesetval->nodeTab[0]);
			obj->nodesetval->nodeTab[0] = NULL;
		}

		xmlXPathFreeObject(obj);
	}

	xmlXPathFreeContext(ctx);
}

/* Help/usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -a, --add <icn>         Add an ICN to the catalog.");
	puts("  -C, --create            Create a new ICN catalog.");
	puts("  -c, --catalog <catalog> Use <catalog> as the ICN catalog.");
	puts("  -d, --del <icn>         Delete an ICN from the catalog.");
	puts("  -f, --overwrite         Overwrite input objects.");
	puts("  -h, -?, --help          Show help/usage message.");
	puts("  -l, --list              Treat input as list of objects.");
	puts("  -m, --media <media>     Specify intended output media.");
	puts("  -n, --ndata <notation>  Set the notation of the new ICN.");
	puts("  -t, --type <type>       Set the type of the new catalog entry.");
	puts("  -u, --uri <uri>         Set the URI of the new ICN.");
	puts("  -v, --verbose           Verbose output.");
	puts("  --version               Show version information.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Show version information. */
static void show_version(void)
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

	const char *sopts = "a:Cc:d:flm:n:t:u:vxh?";
	struct option lopts[] = {
		{"version"  , no_argument      , 0, 0},
		{"help"     , no_argument      , 0, 'h'},
		{"add"      , required_argument, 0, 'a'},
		{"create"   , no_argument      , 0, 'C'},
		{"catalog"  , required_argument, 0, 'c'},
		{"del"      , required_argument, 0, 'd'},
		{"overwrite", no_argument      , 0, 'f'},
		{"list"     , no_argument      , 0, 'l'},
		{"media"    , required_argument, 0, 'm'},
		{"ndata"    , required_argument, 0, 'n'},
		{"type"     , required_argument, 0, 't'},
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
			case 'C':
				createnew = true;
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
				if (cur) {
					xmlSetProp(cur, BAD_CAST "type", BAD_CAST optarg);
				}
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
