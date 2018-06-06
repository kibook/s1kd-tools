#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>
#include <libxml/debugXML.h>
#include <libxml/xinclude.h>

#define PROG_NAME "s1kd-validate"
#define VERSION "1.0.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define SUCCESS_PREFIX PROG_NAME ": SUCCESS: "
#define FAILED_PREFIX PROG_NAME ": FAILED: "

#define EXIT_MAX_SCHEMAS 1
#define EXIT_MISSING_SCHEMA 2
#define EXIT_BAD_IDREF 3

#define S_BAD_IDREF ERR_PREFIX "No matching ID for '%s' (%s line %u).\n"

#define INVALID_ID_XPATH BAD_CAST \
	"//@applicMapRefId[not(//@id=.)]|" \
        "//@applicRefId[not(//@id=.)]|" \
	"//@condRefId[not(//@id=.)]|" \
	"//@condTypeRefId[not(//@id=.)]|" \
	"//@conditionidref[not(//@id=.)]|" \
	"//@condtyperef[not(//@id=.)]|" \
	"//@dependencyTest[not(//@id=.)]|" \
	"//@derivativeClassificationRefId[not(//@id = .)]|" \
	"//@internalRefId[not(//@id=.)]|" \
	"//@nextActionRefId[not(//@id=.)]|" \
	"//@refapplic[not(//@id=.)]|" \
	"//@refid[not(//@id=.)]|" \
	"//@xrefid[not(//id=.)]"

#define INVALID_IDS_XPATH BAD_CAST \
	"//@reasonForUpdateRefIds|//@warningRefs|//@cautionRefs"

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

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

xmlSchemaValidityErrorFunc schema_efunc = (xmlSchemaValidityErrorFunc) fprintf;
xmlSchemaValidityWarningFunc schema_wfunc = (xmlSchemaValidityWarningFunc) fprintf;

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

void suppress_err(void *ctx, const char *msg)
{
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

	xmlSchemaSetParserErrors(ctxt, schema_efunc, schema_wfunc, stderr);
	xmlSchemaSetValidErrors(valid_ctxt, schema_efunc, schema_wfunc, stderr);

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
	puts("Usage: " PROG_NAME " [-d <dir>] [-X <URI>] [-Dflqvx] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -D         Debug output.");
	puts("  -d <dir>   Search for schemas in <dir> instead of using the URL.");
	puts("  -f         List invalid files.");
	puts("  -l         Treat input as list of filenames.");
	puts("  -q         Silent (not output).");
	puts("  -v         Verbose output.");
	puts("  -X <URI>   Exclude namespace from validation by URI.");
	puts("  -x         Do XInclude processing before validation.");
	puts("  --version  Show version information.");
	puts("  <object>   Any number of CSDB objects to validate.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
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

/* Check that certain attributes of type xs:IDREF and xs:IDREFS have a matching
 * xs:ID attribute.
 */
int check_idrefs(xmlDocPtr doc, const char *fname)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	ctx = xmlXPathNewContext(doc);

	/* Check xs:IDREF */
	obj = xmlXPathEvalExpression(INVALID_ID_XPATH, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		if (verbosity > SILENT) {
			int i;
			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				xmlChar *id = xmlNodeGetContent(obj->nodesetval->nodeTab[i]);
				fprintf(stderr,
					S_BAD_IDREF,
					(char *) id,
					fname,
					obj->nodesetval->nodeTab[i]->parent->line);
				xmlFree(id);
			}
		}

		err = EXIT_BAD_IDREF;
	}

	xmlXPathFreeObject(obj);

	/* Check xs:IDREFS */
	obj = xmlXPathEvalExpression(INVALID_IDS_XPATH, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;
		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			char *ids, *id = NULL;

			ids = (char *) xmlNodeGetContent(obj->nodesetval->nodeTab[i]);

			while ((id = strtok(id ? NULL : ids, " "))) {
				xmlChar xpath[256];
				xmlXPathObjectPtr res;

				xmlStrPrintf(xpath, 256, "//*[@id='%s']", id);

				res = xmlXPathEvalExpression(xpath, ctx);

				if (xmlXPathNodeSetIsEmpty(res->nodesetval)) {
					if (verbosity > SILENT) {
						fprintf(stderr,
							S_BAD_IDREF,
							id,
							fname,
							obj->nodesetval->nodeTab[i]->parent->line);
					}
					err = EXIT_BAD_IDREF;
				}
			}

			xmlFree(ids);
		}
	}

	xmlXPathFreeObject(obj);

	xmlXPathFreeContext(ctx);

	return err;
}

int validate_file(const char *fname, const char *schema_dir, xmlNodePtr ignore_ns, int list, int xinclude)
{
	xmlDocPtr doc;
	xmlNodePtr dmodule;
	char *url;
	struct s1kd_schema_parser *parser;
	int err = 0;

	doc = xmlReadFile(fname, NULL, PARSE_OPTS);

	if (xinclude) {
		xmlXIncludeProcess(doc);
	}

	if (ignore_ns->children) {
		xmlNodePtr cur;

		for (cur = ignore_ns->children; cur; cur = cur->next) {
			strip_ns(doc, cur);
		}
	}

	/* This shouldn't be needed because the xs:ID, xs:IDREF and xs:IDREFS
	 * are defined in the schema, but at this time libxml2 does not check
	 * these when validating.
	 */
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

	if (list && err) {
		printf("%s\n", fname);
	}

	xmlFreeDoc(doc);

	return err;
}

int validate_file_list(const char *fname, char *schema_dir, xmlNodePtr ignore_ns, int list_invalid, int xinclude)
{
	FILE *f;
	char path[PATH_MAX];
	int err;

	if (fname) {
		f = fopen(fname, "r");
	} else {
		f = stdin;
	}

	err = 0;

	while (fgets(path, PATH_MAX, f)) {
		strtok(path, "\t\r\n");
		err += validate_file(path, schema_dir, ignore_ns, list_invalid, xinclude);
	}

	if (fname) {
		fclose(f);
	}

	return err;
}

int main(int argc, char *argv[])
{
	int c, i;
	char schema_dir[256] = "";
	int err = 0;
	int list_invalid = 0;
	int is_list = 0;
	int xinclude = 0;

	xmlNodePtr ignore_ns;

	const char *sopts = "vqDd:X:xflh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	ignore_ns = xmlNewNode(NULL, BAD_CAST "ignorens");

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 'q': verbosity = SILENT; break;
			case 'v': verbosity = VERBOSE; break;
			case 'D': verbosity = DEBUG; break;
			case 'd': strcpy(schema_dir, optarg); break;
			case 'X': add_ignore_ns(ignore_ns, optarg); break;
			case 'x': xinclude = 1; break;
			case 'f': list_invalid = 1; break;
			case 'l': is_list = 1; break;
			case 'h': 
			case '?': show_help(); return 0;
		}
	}

	if (verbosity == SILENT) {
		schema_efunc = (xmlSchemaValidityErrorFunc) suppress_err;
		schema_wfunc = (xmlSchemaValidityWarningFunc) suppress_err;
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (is_list) {
				err += validate_file_list(argv[i], schema_dir, ignore_ns, list_invalid, xinclude);
			} else {
				err += validate_file(argv[i], schema_dir, ignore_ns, list_invalid, xinclude);
			}
		}
	} else if (is_list) {
		err = validate_file_list(NULL, schema_dir, ignore_ns, list_invalid, xinclude);
	} else {
		err = validate_file("-", schema_dir, ignore_ns, list_invalid, xinclude);
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