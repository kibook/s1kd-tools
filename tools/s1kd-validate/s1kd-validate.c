#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/xmlschemas.h>
#include <libxml/debugXML.h>
#include <libxslt/transform.h>
#include "s1kd_tools.h"
#include "stats.h"

#define PROG_NAME "s1kd-validate"
#define VERSION "4.3.3"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define SUCCESS_PREFIX PROG_NAME ": SUCCESS: "
#define FAILED_PREFIX PROG_NAME ": FAILED: "

#define S_BAD_IDREF "No matching ID for '%s'."

#define E_BAD_LIST ERR_PREFIX "Could not read list file: %s\n"
#define E_MAX_SCHEMA_PARSERS ERR_PREFIX "Maximum number of schemas reached: %d\n"
#define E_BAD_IDREF ERR_PREFIX "%s (%ld): " S_BAD_IDREF "\n"

#define EXIT_MAX_SCHEMAS 2
#define EXIT_MISSING_SCHEMA 3

/* FIXME:
 *
 * libxml2 XML schema validation does not currently check IDREF/IDREFS, so this
 * tool implements its own checks. If libxml2 ever expands its XML schema
 * validation to include IDREF/IDREFS checking, this can probably be removed.
 *
 * Because this tool is designed for use only with S1000D CSDB objects, the
 * check can be done more efficiently by hardcoding all attributes that are
 * type IDREF/IDREFS.
 *
 * xml-utils/xml-validate has a more general-purpose implementation which
 * parses the XML schema for ID, IDS, IDREF and IDREFS types, but it is
 * noticeably slower.
 */

/* XPath to match all invalid IDREF attributes. */
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
	"//@xrefid[not(//@id=.)]"

/* XPath to match all IDREFS attributes to check their validity. */
#define INVALID_IDS_XPATH BAD_CAST \
	"//@applicRefIds|//@cautionRefs|//@controlAuthorityRefs|//@reasonForUpdateRefIds|//@warningRefs"

/* URI for XML schema instance namespace. */
#define XSI_URI BAD_CAST "http://www.w3.org/2001/XMLSchema-instance"

static enum verbosity_level {SILENT, NORMAL, VERBOSE} verbosity = NORMAL;

enum show_fnames { SHOW_NONE, SHOW_INVALID, SHOW_VALID };

/* Cache schemas to prevent parsing them twice (mainly needed when accessing
 * the schema over a network)
 */
struct s1kd_schema_parser {
	char *url;
	xmlSchemaParserCtxtPtr ctxt;
	xmlSchemaPtr schema;
	xmlSchemaValidCtxtPtr valid_ctxt;
};

/* Initial max schema parsers. */
static unsigned SCHEMA_PARSERS_MAX = 1;

static struct s1kd_schema_parser *schema_parsers;

static int schema_parser_count = 0;

/* Root element of the XML report. */
static xmlDocPtr xml_report_doc = NULL;

static int deep_copy_nodes = 0;

/* Add an error to the XML report. */
static void add_xml_report_error(const xmlNodePtr report_node, xmlNodePtr node, const long lineno, const char *message)
{
	char line[16];

	snprintf(line, 16, "%ld", lineno);

	xmlNodePtr error = xmlNewChild(report_node, NULL, BAD_CAST "error", NULL);
	xmlNewChild(error, NULL, BAD_CAST "message", BAD_CAST message);
	xmlSetProp(error, BAD_CAST "line", BAD_CAST line);

	if (node) {
		xmlNodePtr object;
		xmlChar *xpath;

		object = xmlNewChild(error, NULL, BAD_CAST "object", NULL);

		xpath = xpath_of(node);
		xmlSetProp(object, BAD_CAST "xpath", xpath);

		if (node->type == XML_ATTRIBUTE_NODE) node = node->parent;

		xmlAddChild(object, xmlCopyNode(node, deep_copy_nodes ? 1 : 2));

		xmlFree(xpath);
	}
}

/* The signature of xmlStructuredErrorFunc is different from v2.12.0 and up. */
#if LIBXML_VERSION < 21200
static void print_error(void *userData, xmlErrorPtr error)
#else
static void print_error(void *userData, const xmlError *error)
#endif
{
	if (verbosity > SILENT) {
		if (error->file) {
			fprintf(stderr, ERR_PREFIX "%s (%d): %s", error->file, error->line, error->message);
		} else {
			fprintf(stderr, ERR_PREFIX "%s\n", error->message);
		}
	}

	/* If -x is specified, add error to XML report. */
	if (xml_report_doc) {
		error->message[strcspn(error->message, "\n")] = 0;
		add_xml_report_error((xmlNodePtr) userData, error->node, error->line, error->message);
	}
}

#if LIBXML_VERSION < 21200
static void print_non_parser_error(void *userData, xmlErrorPtr error)
#else
static void print_non_parser_error(void *userData, const xmlError *error)
#endif
{
	if (error->domain == XML_FROM_PARSER) {
		return;
	}

	print_error(userData, error);
}

xmlStructuredErrorFunc schema_errfunc = print_error;

/* Print the XML tree to stdout if it is valid. */
static int output_tree = 0;

static struct s1kd_schema_parser *get_schema_parser(const char *url)
{
	int i;

	for (i = 0; i < schema_parser_count; ++i) {
		if (strcmp(schema_parsers[i].url, url) == 0) {
			return &(schema_parsers[i]);
		}
	}

	return NULL;
}

static struct s1kd_schema_parser *add_schema_parser(char *url)
{
	struct s1kd_schema_parser *parser;

	xmlSchemaParserCtxtPtr ctxt;
	xmlSchemaPtr schema;
	xmlSchemaValidCtxtPtr valid_ctxt;

	ctxt = xmlSchemaNewParserCtxt(url);
	schema = xmlSchemaParse(ctxt);
	valid_ctxt = xmlSchemaNewValidCtxt(schema);

	schema_parsers[schema_parser_count].url = url;
	schema_parsers[schema_parser_count].ctxt = ctxt;
	schema_parsers[schema_parser_count].schema = schema;
	schema_parsers[schema_parser_count].valid_ctxt = valid_ctxt;

	parser = &schema_parsers[schema_parser_count];

	++schema_parser_count;

	return parser;
}

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-s <path>] [-X <URI>] [-F|-f] [-o|-x] [-elqTv8^h?] [<object>...]");
	puts("");
	puts("Options:");
	puts("  -e, --ignore-empty     Ignore empty/non-XML documents.");
	puts("  -F, --valid-filenames  List valid files.");
	puts("  -f, --filenames        List invalid files.");
	puts("  -h, -?, --help         Show help/usage message.");
	puts("  -l, --list             Treat input as list of filenames.");
	puts("  -o, --output-valid     Output valid CSDB objects to stdout.");
	puts("  -q, --quiet            Silent (no output).");
	puts("  -s, --schema <path>    Validate against the given schema.");
	puts("  -T, --summary          Print a summary of the check.");
	puts("  -v, --verbose          Verbose output.");
	puts("  -X, --exclude <URI>    Exclude namespace from validation by URI.");
	puts("  -x, --xml              Output an XML report.");
	puts("  -8, --deep-copy-nodes  The XML report will include a deep copy of invalid nodes.");
	puts("  -^, --remove-deleted   Validate with elements marked as \"delete\" removed.");
	puts("  --version              Show version information.");
	puts("  <object>               Any number of CSDB objects to validate.");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

static void add_ignore_ns(xmlNodePtr ignore_ns, const char *arg)
{
	xmlNewChild(ignore_ns, NULL, BAD_CAST "ignore", BAD_CAST arg);
}

static void strip_ns(xmlDocPtr doc, xmlNodePtr ignore)
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
static int check_idrefs(xmlDocPtr doc, const char *fname, const xmlNodePtr report_node)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int err = 0;

	ctx = xmlXPathNewContext(doc);

	/* Check xs:IDREF */
	obj = xmlXPathEvalExpression(INVALID_ID_XPATH, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *id = xmlNodeGetContent(obj->nodesetval->nodeTab[i]);
			long line = xmlGetLineNo(obj->nodesetval->nodeTab[i]->parent);

			if (verbosity > SILENT) {
				fprintf(stderr,
					E_BAD_IDREF,
					fname,
					line,
					(char *) id);
			}

			/* If -x is specified, add error to XML report. */
			if (xml_report_doc) {
				char message[256];
				snprintf(message, 256, S_BAD_IDREF, (char *) id);
				add_xml_report_error(report_node, obj->nodesetval->nodeTab[i], line, message);
			}

			xmlFree(id);
		}

		++err;
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
					long line = xmlGetLineNo(obj->nodesetval->nodeTab[i]->parent);

					if (verbosity > SILENT) {
						fprintf(stderr,
							E_BAD_IDREF,
							fname,
							xmlGetLineNo(obj->nodesetval->nodeTab[i]->parent),
							(char *) id);
					}

					/* If -x is specified, add error to XML report. */
					if (xml_report_doc) {
						char message[256];
						snprintf(message, 256, S_BAD_IDREF, (char *) id);
						add_xml_report_error(report_node, obj->nodesetval->nodeTab[i], line, message);
					}

					++err;
				}

				xmlXPathFreeObject(res);
			}

			xmlFree(ids);
		}
	}

	xmlXPathFreeObject(obj);

	xmlXPathFreeContext(ctx);

	return err;
}

static void resize_schema_parsers(void)
{
	if (!(schema_parsers = realloc(schema_parsers, (SCHEMA_PARSERS_MAX *= 2) * sizeof(struct s1kd_schema_parser)))) {
		fprintf(stderr, E_MAX_SCHEMA_PARSERS, schema_parser_count);
		exit(EXIT_MAX_SCHEMAS);
	}
}

static int validate_file(const char *fname, const char *schema, xmlNodePtr ignore_ns, enum show_fnames show_fnames, int ignore_empty, int rem_del)
{
	xmlDocPtr doc;
	xmlDocPtr validtree = NULL;
	xmlNodePtr dmodule;
	char *url;
	struct s1kd_schema_parser *parser;
	int err = 0;
	xmlNodePtr report_node = NULL;

	if (!(doc = read_xml_doc(fname))) {
		return !ignore_empty;
	}

	/* Make a copy of the original XML tree before performing extra
	 * processing on it. */
	if (output_tree) {
		validtree = xmlCopyDoc(doc, 1);
	}

	if (ignore_ns->children) {
		xmlNodePtr cur;

		for (cur = ignore_ns->children; cur; cur = cur->next) {
			strip_ns(doc, cur);
		}
	}

	/* Remove elements marked as "delete". */
	if (rem_del) {
		rem_delete_elems(doc);
	}

	/* Add a node to the XML report (if enabled) for the current file. */
	if (xml_report_doc) {
		report_node = xmlNewChild(xmlDocGetRootElement(xml_report_doc), NULL, BAD_CAST "document", NULL);
		xmlSetProp(report_node, BAD_CAST "path", BAD_CAST fname);
	}

	/* This shouldn't be needed because the xs:ID, xs:IDREF and xs:IDREFS
	 * are defined in the schema, but at this time libxml2 does not check
	 * these when validating.
	 */
	err += check_idrefs(doc, fname, report_node);

	dmodule = xmlDocGetRootElement(doc);

	if (schema) {
		url = strdup(schema);
	} else {
		url = (char *) xmlGetNsProp(dmodule, BAD_CAST "noNamespaceSchemaLocation", XSI_URI);
	}

	if (!url) {
		if (verbosity > SILENT) {
			fprintf(stderr, ERR_PREFIX "%s has no schema.\n", fname);
		}
		return 1;
	}

	if ((parser = get_schema_parser(url))) {
		xmlFree(url);
	} else {
		if (schema_parser_count == SCHEMA_PARSERS_MAX) {
			resize_schema_parsers();
		}

		parser = add_schema_parser(url);
	}

	xmlSchemaSetParserStructuredErrors(parser->ctxt, schema_errfunc, report_node);
	xmlSchemaSetValidStructuredErrors(parser->valid_ctxt, schema_errfunc, report_node);

	if (xmlSchemaValidateDoc(parser->valid_ctxt, doc)) {
		++err;
	}

	/* Write the original XML tree to stdout if determined to be valid. */
	if (output_tree) {
		if (err == 0) {
			save_xml_doc(validtree, "-");
		}
		xmlFreeDoc(validtree);
	}

	if (verbosity >= VERBOSE) {
		if (err) {
			fprintf(stderr, FAILED_PREFIX "%s fails to validate against schema %s\n", fname, parser->url);
		} else {
			fprintf(stderr, SUCCESS_PREFIX "%s validates against schema %s\n", fname, parser->url);
		}
	}

	if ((show_fnames == SHOW_INVALID && err != 0) || (show_fnames == SHOW_VALID && err == 0)) {
		printf("%s\n", fname);
	}

	xmlFreeDoc(doc);

	return err;
}

static int validate_file_list(const char *fname, const char *schema, xmlNodePtr ignore_ns, enum show_fnames show_fnames, int ignore_empty, int rem_del)
{
	FILE *f;
	char path[PATH_MAX];
	int err;

	if (fname) {
		if (!(f = fopen(fname, "r"))) {
			fprintf(stderr, E_BAD_LIST, fname);
			return 0;
		}
	} else {
		f = stdin;
	}

	err = 0;

	while (fgets(path, PATH_MAX, f)) {
		strtok(path, "\t\r\n");
		err += validate_file(path, schema, ignore_ns, show_fnames, ignore_empty, rem_del);
	}

	if (fname) {
		fclose(f);
	}

	return err;
}

static void print_stats()
{
	xmlDocPtr styledoc;
	xsltStylesheetPtr style;
	xmlDocPtr res;

	styledoc = read_xml_mem((const char *) stats_xsl, stats_xsl_len);
	style = xsltParseStylesheetDoc(styledoc);

	res = xsltApplyStylesheet(style, xml_report_doc, NULL);

	fprintf(stderr, "%s", (char *) res->children->content);

	xmlFreeDoc(res);
	xsltFreeStylesheet(style);
}

int main(int argc, char *argv[])
{
	int c, i;
	int err = 0;
	enum show_fnames show_fnames = SHOW_NONE;
	int is_list = 0;
	int ignore_empty = 0;
	int rem_del = 0;
	char *schema = NULL;
	int xml = 0;
	int show_stats = 0;

	xmlNodePtr ignore_ns;

	const char *sopts = "vqX:xFfloes:T8^h?";
	struct option lopts[] = {
		{"version"        , no_argument      , 0, 0},
		{"help"           , no_argument      , 0, 'h'},
		{"valid-filenames", no_argument      , 0, 'F'},
		{"filenames"      , no_argument      , 0, 'f'},
		{"list"           , no_argument      , 0, 'l'},
		{"output-valid"   , no_argument      , 0, 'o'},
		{"quiet"          , no_argument      , 0, 'q'},
		{"verbose"        , no_argument      , 0, 'v'},
		{"exclude"        , required_argument, 0, 'X'},
		{"ignore-empty"   , no_argument      , 0, 'e'},
		{"schema"         , required_argument, 0, 's'},
		{"summary"        , no_argument      , 0, 'T'},
		{"xml"            , no_argument      , 0, 'x'},
		{"deep-copy-nodes", no_argument      , 0, '8'},
		{"remove-deleted" , no_argument      , 0, '^'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	schema_parsers = malloc(SCHEMA_PARSERS_MAX * sizeof(struct s1kd_schema_parser));

	ignore_ns = xmlNewNode(NULL, BAD_CAST "ignorens");

	while ((c = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (c) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return EXIT_SUCCESS;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind, optarg)
				break;
			case 'q': verbosity = SILENT; break;
			case 'v': verbosity = VERBOSE; break;
			case 'X': add_ignore_ns(ignore_ns, optarg); break;
			case 'F': show_fnames = SHOW_VALID; break;
			case 'f': show_fnames = SHOW_INVALID; break;
			case 'l': is_list = 1; break;
			case 'o': output_tree = 1; break;
			case 'e': ignore_empty = 1; break;
			case 's': schema = strdup(optarg); break;
			case '^': rem_del = 1; break;
			case 'T': show_stats = 1; break;
			case '8': deep_copy_nodes = 1;
			case 'x': xml = 1; break;
			case 'h': 
			case '?': show_help(); return EXIT_SUCCESS;
		}
	}

	LIBXML2_PARSE_INIT

	if (ignore_empty) {
		schema_errfunc = print_non_parser_error;
	}

	/* FIXME: This function is deprecated, is it still needed here when we set per-context error handlers as well? */
	xmlSetStructuredErrorFunc(NULL, schema_errfunc);

	if (xml || show_stats) {
		xml_report_doc = xmlNewDoc(BAD_CAST "1.0");
		xmlDocSetRootElement(xml_report_doc, xmlNewNode(NULL, BAD_CAST "s1kdValidateReport"));
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (is_list) {
				err += validate_file_list(argv[i], schema, ignore_ns, show_fnames, ignore_empty, rem_del);
			} else {
				err += validate_file(argv[i], schema, ignore_ns, show_fnames, ignore_empty, rem_del);
			}
		}
	} else if (is_list) {
		err = validate_file_list(NULL, schema, ignore_ns, show_fnames, ignore_empty, rem_del);
	} else {
		err = validate_file("-", schema, ignore_ns, show_fnames, ignore_empty, rem_del);
	}

	for (i = 0; i < schema_parser_count; ++i) {
		xmlFree(schema_parsers[i].url);
		xmlSchemaFreeValidCtxt(schema_parsers[i].valid_ctxt);
		xmlSchemaFree(schema_parsers[i].schema);
		xmlSchemaFreeParserCtxt(schema_parsers[i].ctxt);
	}

	free(schema_parsers);
	xmlFreeNode(ignore_ns);
	free(schema);

	if (xml) {
		save_xml_doc(xml_report_doc, "-");
	}

	if (show_stats) {
		print_stats();
	}

	xmlFreeDoc(xml_report_doc);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return err ? EXIT_FAILURE : EXIT_SUCCESS;
}
