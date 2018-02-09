#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <libxml/tree.h>
#include <libxml/xmlschemas.h>
#include <libxml/debugXML.h>

#define PROGNAME "s1kd-validate"

#define ERR_PREFIX PROGNAME ": ERROR: "
#define SUCCESS_PREFIX PROGNAME ": SUCCESS: "
#define FAILED_PREFIX PROGNAME ": FAILED: "

#define EXIT_MAX_SCHEMAS 1
#define EXIT_MISSING_SCHEMA 2
#define EXIT_BAD_IDREF 3

#define INVALID_ID_XPATH BAD_CAST \
	"//@internalRefId[not(//@id = .)]|" \
        "//@applicRefId[not(//@id = .)]" \
	"//@nextActionRefId[not(//@id = .)]"

enum verbosity_level {SILENT, NORMAL, VERBOSE, DEBUG} verbosity = NORMAL;

/* Cache schemas to prevent parsing them twice (mainly needed when accessing
 * the schema over a network)
 */
struct s1kd_schema_parser {
	char *url;
	xmlSchemaParserCtxtPtr ctxt;
	xmlSchemaPtr schema;
	xmlSchemaValidCtxtPtr valid_ctxt;
};

#define SCHEMA_PARSERS_MAX 32

struct s1kd_schema_parser schema_parsers[SCHEMA_PARSERS_MAX];

int schema_parser_count = 0;

struct s1kd_schema_parser *get_schema_parser(const char *url)
{
	int i;

	for (i = 0; i < schema_parser_count; ++i) {
		if (strcmp(schema_parsers[i].url, url) == 0) {
			return &(schema_parsers[i]);
		}
	}

	return NULL;
}

struct s1kd_schema_parser *add_schema_parser(char *url)
{
	struct s1kd_schema_parser *parser;

	xmlSchemaParserCtxtPtr ctxt;
	xmlSchemaPtr schema;
	xmlSchemaValidCtxtPtr valid_ctxt;

	ctxt = xmlSchemaNewParserCtxt(url);
	schema = xmlSchemaParse(ctxt);
	valid_ctxt = xmlSchemaNewValidCtxt(schema);

	xmlSchemaSetParserErrors(ctxt,
		(xmlSchemaValidityErrorFunc) fprintf,
		(xmlSchemaValidityWarningFunc) fprintf,
		stderr);
	xmlSchemaSetValidErrors(valid_ctxt,
		(xmlSchemaValidityErrorFunc) fprintf,
		(xmlSchemaValidityWarningFunc) fprintf,
		stderr);

	schema_parsers[schema_parser_count].url = url;
	schema_parsers[schema_parser_count].ctxt = ctxt;
	schema_parsers[schema_parser_count].schema = schema;
	schema_parsers[schema_parser_count].valid_ctxt = valid_ctxt;

	parser = &schema_parsers[schema_parser_count];

	++schema_parser_count;

	return parser;
}

void show_help(void)
{
	puts("Usage: " PROGNAME " [-d <dir>] [-X <URI>] [-vqD] <dms>");
	puts("");
	puts("Options:");
	puts("  -d <dir> Search for schemas in <dir> instead of using the URL.");
	puts("  -X <URI> Exclude namespace from validation by URI.");
	puts("  -v       Verbose output.");
	puts("  -q       Silent (not output).");
	puts("  -D       Debug output.");
	puts("  <dms>    Any number of data modules to validate.");
}

void add_ignore_ns(xmlNodePtr ignore_ns, const char *arg)
{
	xmlNewChild(ignore_ns, NULL, BAD_CAST "ignore", BAD_CAST arg);
}

void strip_ns(xmlDocPtr doc, xmlNodePtr ignore)
{
	xmlXPathContextPtr ctxt;
	xmlXPathObjectPtr results;

	char xpath[256];

	char *uri;
	int i;

	uri = (char *) xmlNodeGetContent(ignore);

	snprintf(xpath, 256, "//*[namespace-uri() = '%s']", uri);

	xmlFree(uri);

	ctxt = xmlXPathNewContext(doc);
	
	results = xmlXPathEvalExpression(BAD_CAST xpath, ctxt);

	for (i = results->nodesetval->nodeNr - 1; i >= 0; --i) {
		xmlUnlinkNode(results->nodesetval->nodeTab[i]);
		xmlFreeNode(results->nodesetval->nodeTab[i]);
	}

	xmlXPathFreeObject(results);
	xmlXPathFreeContext(ctxt);
}

int check_idrefs(xmlDocPtr doc, const char *fname)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	ctx = xmlXPathNewContext(doc);

	obj = xmlXPathEvalExpression(INVALID_ID_XPATH, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		if (verbosity > SILENT) {
			int i;
			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				xmlChar *id = xmlNodeGetContent(obj->nodesetval->nodeTab[i]);
				fprintf(stderr, ERR_PREFIX "No matching ID for '%s' (%s line %u).\n", (char *) id, fname, obj->nodesetval->nodeTab[i]->parent->line);
				xmlFree(id);
			}
		}

		err = EXIT_BAD_IDREF;
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return err;
}

int validate_file(const char *fname, const char *schema_dir, xmlNodePtr ignore_ns)
{
	xmlDocPtr doc;
	xmlNodePtr dmodule;
	char *url;
	struct s1kd_schema_parser *parser;
	int err = 0;

	doc = xmlReadFile(fname, NULL, 0);

	if (ignore_ns->children) {
		xmlNodePtr cur;

		for (cur = ignore_ns->children; cur; cur = cur->next) {
			strip_ns(doc, cur);
		}
	}

	err += check_idrefs(doc, fname);

	dmodule = xmlDocGetRootElement(doc);

	url = (char *) xmlGetProp(dmodule, (xmlChar *) "noNamespaceSchemaLocation");

	if (!url) {
		if (verbosity > SILENT) {
			fprintf(stderr, ERR_PREFIX "%s has no schema.\n", fname);
		}
		return 1;
	}

	if (strcmp(schema_dir, "") != 0) {
		char *last_slash, *slash1, *slash2, *schema_name, schema_file[256];

		/* Check if directory is in multi-spec format */
		slash1 = strrchr(url, '/');
		slash1[0] = 0;
		slash2 = strrchr(url, '/');
		slash2[0] = 0;
		last_slash = strrchr(url, '/');
		slash1[0] = slash2[0] = '/';
		schema_name = url + (last_slash - url) + 1;
		snprintf(schema_file, 256, "%s/%s", schema_dir, schema_name);

		/* Otherwise, try single-spec format */
		if (access(schema_file, F_OK) == -1) {
			last_slash = strrchr(url, '/');
			schema_name = url + (last_slash - url) + 1;
			snprintf(schema_file, 256, "%s/%s", schema_dir, schema_name);
		}

		if (access(schema_file, F_OK) == -1) {
			if (verbosity > SILENT) {
				fprintf(stderr, ERR_PREFIX "Schema %s not found in %s.\n", schema_name, schema_dir);
			}
			exit(EXIT_MISSING_SCHEMA);
		}

		xmlFree(url);
		url = (char *) xmlStrdup((xmlChar *) schema_file);
	}

	if (!(parser = get_schema_parser(url))) {
		if (schema_parser_count == SCHEMA_PARSERS_MAX) {
			if (verbosity > SILENT) {
				fprintf(stderr, ERR_PREFIX "Maximum number of schemas reached.\n");
			}
			exit(EXIT_MAX_SCHEMAS);
		}

		parser = add_schema_parser(url);
	}

	err += xmlSchemaValidateDoc(parser->valid_ctxt, doc);

	if (verbosity >= VERBOSE) {
		if (err) {
			printf(FAILED_PREFIX "%s fails to validate\n", fname);
		} else {
			printf(SUCCESS_PREFIX "%s validates\n", fname);
		}
	}

	xmlFreeDoc(doc);

	return err;
}

int main(int argc, char *argv[])
{
	int c, i;
	char schema_dir[256] = "";
	int err = 0;

	xmlNodePtr ignore_ns;

	ignore_ns = xmlNewNode(NULL, BAD_CAST "ignorens");

	while ((c = getopt(argc, argv, "vqDd:X:h?")) != -1) {
		switch (c) {
			case 'q': verbosity = SILENT; break;
			case 'v': verbosity = VERBOSE; break;
			case 'D': verbosity = DEBUG; break;
			case 'd': strcpy(schema_dir, optarg); break;
			case 'X': add_ignore_ns(ignore_ns, optarg); break;
			case 'h': 
			case '?': show_help(); exit(0);
		}
	}

	if (optind >= argc) {
		err = validate_file("-", schema_dir, ignore_ns);
	} else {
		for (i = optind; i < argc; ++i) {
			err += validate_file(argv[i], schema_dir, ignore_ns);
		}
	}

	for (i = 0; i < schema_parser_count; ++i) {
		xmlFree(schema_parsers[i].url);
		xmlSchemaFreeValidCtxt(schema_parsers[i].valid_ctxt);
		xmlSchemaFree(schema_parsers[i].schema);
		xmlSchemaFreeParserCtxt(schema_parsers[i].ctxt);
	}

	xmlFreeNode(ignore_ns);

	xmlCleanupParser();

	return err;
}
