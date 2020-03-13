#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/transform.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "xsl.h"
#include "defaults.h"
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-defaults"
#define VERSION "2.6.1"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define S_DMTYPES_ERR ERR_PREFIX "Could not create " DEFAULT_DMTYPES_FNAME " file.\n"
#define S_FMTYPES_ERR ERR_PREFIX "Could not create " DEFAULT_FMTYPES_FNAME " file.\n"
#define S_NO_FILE_ERR ERR_PREFIX "Could not open file: %s\n"
#define S_MKDIR_FAILED ERR_PREFIX "Could not create directory %s: %s\n"
#define S_CHDIR_FAILED ERR_PREFIX "Could not change to directory %s: %s\n"

#define EXIT_NO_FILE 2
#define EXIT_OS_ERROR 3

enum format {TEXT, XML};
enum file {NONE, DEFAULTS, DMTYPES, FMTYPES};

/* Show the help/usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-Ddfisth?] [-b <BREX>] [-j <map>] [-n <name> -v <value> ...] [-o <dir>] [<file>...]");
	puts("");
	puts("Options:");
	puts("  -b, --brex <BREX>    Create from a BREX DM.");
	puts("  -D, --dmtypes        Convert a .dmtypes file.");
	puts("  -d, --defaults       Convert a .defaults file.");
	puts("  -F, --fmtypes        Convert a .fmtypes file.");
	puts("  -f, --overwrite      Overwrite an existing file.");
	puts("  -h, -?, --help       Show usage message.");
	puts("  -i, --init           Initialize a new CSDB.");
	puts("  -J, --dump-brexmap   Dump default .brexmap file.");
	puts("  -j, --brexmap <map>  Use a custom .brexmap file.");
	puts("  -n, --name <name>    Default to set a value for with -v.");
	puts("  -o, --dir <dir>      Use <dir> instead of current directory.");
	puts("  -s, --sort           Sort entries.");
	puts("  -t, --text           Output in the simple text format.");
	puts("  -v, --value <value>  Value for default specified with -n.");
	puts("  --version  Show version information.");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

static xmlDocPtr transform_doc_with(xmlDocPtr doc, xmlDocPtr styledoc)
{
	xsltStylesheetPtr style;
	xmlDocPtr res;
	style = xsltParseStylesheetDoc(styledoc);
	res = xsltApplyStylesheet(style, doc, NULL);
	xsltFreeStylesheet(style);
	return res;
}

/* Apply a built-in XSLT stylesheet to an XML document. */
static xmlDocPtr transform_doc(xmlDocPtr doc, unsigned char *xml, unsigned int len)
{
	xmlDocPtr styledoc, res;

	styledoc = read_xml_mem((const char *) xml, len);
	res = transform_doc_with(doc, styledoc);

	return res;
}

/* Sort entries in defaults/dmtypes files. */
static xmlDocPtr sort_entries(xmlDocPtr doc)
{
	return transform_doc(doc, xsl_sort_xsl, xsl_sort_xsl_len);
}

/* Convert XML defaults to the simple text version. */
static xmlDocPtr xml_defaults_to_text(xmlDocPtr doc)
{
	return transform_doc(doc, xsl_xml_defaults_to_text_xsl, xsl_xml_defaults_to_text_xsl_len);
}

/* Convert XML dmtypes to the simple text version. */
static xmlDocPtr xml_dmtypes_to_text(xmlDocPtr doc)
{
	return transform_doc(doc, xsl_xml_dmtypes_to_text_xsl, xsl_xml_dmtypes_to_text_xsl_len);
}

/* Convert XML fmtypes to the simple text version. */
static xmlDocPtr xml_fmtypes_to_text(xmlDocPtr doc)
{
	return transform_doc(doc, xsl_xml_fmtypes_to_text_xsl, xsl_xml_fmtypes_to_text_xsl_len);
}

/* Convert simple text defaults to the XML version. */
static xmlDocPtr text_defaults_to_xml(const char *path)
{
	FILE *f;
	char line[1024];
	xmlDocPtr doc;
	xmlNodePtr defaults;

	if (!path) {
		return NULL;
	}

	if ((doc = read_xml_doc(path, false))) {
		return doc;
	}

	if (strcmp(path, "-") == 0) {
		f = stdin;
	} else if (!(f = fopen(path, "r"))) {
		return NULL;
	}

	doc = xmlNewDoc(BAD_CAST "1.0");
	defaults = xmlNewNode(NULL, BAD_CAST "defaults");
	xmlDocSetRootElement(doc, defaults);

	while (fgets(line, 1024, f)) {
		char key[32], val[256];
		xmlNodePtr def;

		if (sscanf(line, "%31s %255[^\n]", key, val) != 2)
			continue;

		def = xmlNewChild(defaults, NULL, BAD_CAST "default", NULL);
		xmlSetProp(def, BAD_CAST "ident", BAD_CAST key);
		xmlSetProp(def, BAD_CAST "value", BAD_CAST val);
	}

	fclose(f);

	return doc;
}

/* Convert simple text dmtypes to the XML version. */
static xmlDocPtr text_dmtypes_to_xml(const char *path)
{
	FILE *f;
	char line[1024];
	xmlDocPtr doc;
	xmlNodePtr dmtypes;

	if ((doc = read_xml_doc(path, false))) {
		return doc;
	}

	if (strcmp(path, "-") == 0) {
		f = stdin;
	} else if (!(f = fopen(path, "r"))) {
		return NULL;
	}

	doc = xmlNewDoc(BAD_CAST "1.0");
	dmtypes = xmlNewNode(NULL, BAD_CAST "dmtypes");
	xmlDocSetRootElement(doc, dmtypes);

	while (fgets(line, 1024, f)) {
		char code[6], schema[64], infname[256];
		int n;
		xmlNodePtr type;

		n = sscanf(line, "%5s %63s %255[^\n]", code, schema, infname);

		if (n < 2) {
			continue;
		}

		type = xmlNewChild(dmtypes, NULL, BAD_CAST "type", NULL);
		xmlSetProp(type, BAD_CAST "infoCode", BAD_CAST code);
		xmlSetProp(type, BAD_CAST "schema", BAD_CAST schema);
		if (n > 2) {
			xmlSetProp(type, BAD_CAST "infoName", BAD_CAST infname);
		}
	}

	fclose(f);

	return doc;
}

/* Convert simple text fmtypes to the XML version. */
static xmlDocPtr text_fmtypes_to_xml(const char *path)
{
	FILE *f;
	char line[1024];
	xmlDocPtr doc;
	xmlNodePtr fmtypes;

	if ((doc = read_xml_doc(path, false))) {
		return doc;
	}

	if (strcmp(path, "-") == 0) {
		f = stdin;
	} else if (!(f = fopen(path, "r"))) {
		return NULL;
	}

	doc = xmlNewDoc(BAD_CAST "1.0");
	fmtypes = xmlNewNode(NULL, BAD_CAST "fmtypes");
	xmlDocSetRootElement(doc, fmtypes);

	while (fgets(line, 1024, f)) {
		char code[5] = "", type[32] = "", xsl[1024] = "";
		int n;
		xmlNodePtr fm;

		n = sscanf(line, "%4s %31s %1023[^\n]", code, type, xsl);

		if (n < 2) {
			continue;
		}

		fm = xmlNewChild(fmtypes, NULL, BAD_CAST "fm", NULL);
		xmlSetProp(fm, BAD_CAST "infoCode", BAD_CAST code);
		xmlSetProp(fm, BAD_CAST "type", BAD_CAST type);
		if (n == 3) {
			xmlSetProp(fm, BAD_CAST "xsl", BAD_CAST xsl);
		}
	}

	fclose(f);

	return doc;
}

/* Return the first node matching an XPath expression. */
static xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(doc ? doc : node->doc);
	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);
	first = xmlXPathNodeSetIsEmpty(obj->nodesetval) ? NULL : obj->nodesetval->nodeTab[0];

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

/* Obtain some defaults values from the user as arguments. */
static void set_user_defaults(xmlDocPtr doc, xmlNodePtr user_defs)
{
	xmlNodePtr root, cur;

	root = xmlDocGetRootElement(doc);

	for (cur = user_defs->children; cur; cur = cur->next) {
		xmlXPathContextPtr ctx;
		xmlXPathObjectPtr obj;
		xmlChar *ident, *value;

		ident = xmlGetProp(cur, BAD_CAST "ident");
		value = xmlGetProp(cur, BAD_CAST "value");

		ctx = xmlXPathNewContext(doc);
		xmlXPathSetContextNode(root, ctx);
		xmlXPathRegisterVariable(ctx, BAD_CAST "ident", xmlXPathNewString(ident));

		obj = xmlXPathEvalExpression(BAD_CAST "default[@ident=$ident]", ctx);

		if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			xmlAddChild(root, xmlCopyNode(cur, 1));
		} else {
			xmlSetProp(obj->nodesetval->nodeTab[0], BAD_CAST "value", value);
		}

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);

		xmlFree(ident);
		xmlFree(value);
	}
}

/* Obtain some defaults values from the environment. */
static void set_defaults(xmlDocPtr doc)
{
	char *env;

	if ((env = getenv("LANG"))) {
		char *lang, *lang_l, *lang_c;
		xmlNodePtr liso, ciso;

		lang = strdup(env);
		lang_l = strtok(lang, "_");
		lang_c = strtok(NULL, ".");

		liso = first_xpath_node(doc, NULL, "//default[@ident = 'languageIsoCode']");
		ciso = first_xpath_node(doc, NULL, "//default[@ident = 'countryIsoCode']");

		if (lang_l) {
			xmlSetProp(liso, BAD_CAST "value", BAD_CAST lang_l);
		}
		if (lang_c) {
			xmlSetProp(ciso, BAD_CAST "value", BAD_CAST lang_c);
		}

		free(lang);
	}
}

/* Use the .brexmap to create the XSL to transform a BREX DM to a .defaults
 * file.
 */
static xmlDocPtr make_brex2defaults_xsl(xmlDocPtr brexmap)
{
	xmlDocPtr res;
	res = transform_doc(brexmap, xsl_brexmap_defaults_xsl, xsl_brexmap_defaults_xsl_len);
	return res;
}

/* Use the .brexmap to create the XSL to transform a BREX DM to a .dmtypes
 * file.
 */
static xmlDocPtr make_brex2dmtypes_xsl(xmlDocPtr brexmap)
{
	xmlDocPtr res;
	res = transform_doc(brexmap, xsl_brexmap_dmtypes_xsl, xsl_brexmap_dmtypes_xsl_len);
	return res;
}

/* Create a .defaults file from a BREX DM. */
static xmlDocPtr new_defaults_from_brex(xmlDocPtr brex, xmlDocPtr brexmap)
{
	xmlDocPtr styledoc, res, sorted;

	styledoc = make_brex2defaults_xsl(brexmap);
	res = transform_doc_with(brex, styledoc);
	sorted = sort_entries(res);
	xmlFreeDoc(res);

	return sorted;
}

/* Create a .dmtypes file from a BREX DM. */
static xmlDocPtr new_dmtypes_from_brex(xmlDocPtr brex, xmlDocPtr brexmap)
{
	xmlDocPtr styledoc, res, sorted;

	styledoc = make_brex2dmtypes_xsl(brexmap);
	res = transform_doc_with(brex, styledoc);
	sorted = sort_entries(res);
	xmlFreeDoc(res);

	return sorted;
}

/* Dump the built-in defaults in the XML format. */
static void dump_defaults_xml(const char *fname, bool overwrite, xmlDocPtr brex, xmlDocPtr brexmap, xmlNodePtr user_defs)
{
	xmlDocPtr doc;

	if (brex) {
		doc = new_defaults_from_brex(brex, brexmap);
	} else {
		doc = read_xml_mem((const char *) defaults_xml, defaults_xml_len);
		set_defaults(doc);
		set_user_defaults(doc, user_defs);
	}

	if (overwrite) {
		save_xml_doc(doc, fname);
	} else {
		save_xml_doc(doc, "-");
	}

	xmlFreeDoc(doc);
}

/* Dump the built-in defaults in the simple text format. */
static void dump_defaults_text(const char *fname, bool overwrite, xmlDocPtr brex, xmlDocPtr brexmap, xmlNodePtr user_defs)
{
	xmlDocPtr doc, res;
	FILE *f;

	if (brex) {
		doc = new_defaults_from_brex(brex, brexmap);
	} else {
		doc = read_xml_mem((const char *) defaults_xml, defaults_xml_len);
		set_defaults(doc);
		set_user_defaults(doc, user_defs);
	}

	res = transform_doc(doc, xsl_xml_defaults_to_text_xsl, xsl_xml_defaults_to_text_xsl_len);

	if (overwrite) {
		f = fopen(fname, "w");
	} else {
		f = stdout;
	}

	fprintf(f, "%s", res->children->content);

	if (overwrite) {
		fclose(f);
	}

	xmlFreeDoc(res);
	xmlFreeDoc(doc);
}

static xmlDocPtr simple_text_to_xml(const char *path, enum file f, bool sort, xmlDocPtr brex, xmlDocPtr brexmap)
{
	xmlDocPtr doc = NULL;

	switch (f) {
		case NONE:
		case DEFAULTS:
			doc = brex ? new_defaults_from_brex(brex, brexmap) : text_defaults_to_xml(path);
			break;
		case DMTYPES:
			doc = brex ? new_dmtypes_from_brex(brex, brexmap) : text_dmtypes_to_xml(path);
			break;
		case FMTYPES:
			doc = text_fmtypes_to_xml(path);
			break;
	}

	if (sort) {
		xmlDocPtr res;
		res = sort_entries(doc);
		xmlFreeDoc(doc);
		doc = res;
	}

	return doc;
}

/* Convert an XML defaults/dmtypes file to the simple text version. */
static void xml_to_text(const char *path, enum file f, bool overwrite, bool sort, xmlDocPtr brex, xmlDocPtr brexmap, xmlNodePtr user_defs)
{
	xmlDocPtr doc, res = NULL;

	if (!(doc = read_xml_doc(path, false))) {
		doc = simple_text_to_xml(path, f, sort, brex, brexmap);
	}

	if (f == DEFAULTS) {
		set_user_defaults(doc, user_defs);
	}

	switch (f) {
		case NONE:
		case DEFAULTS:
			res = xml_defaults_to_text(doc);
			break;
		case DMTYPES:
			res = xml_dmtypes_to_text(doc);
			break;
		case FMTYPES:
			res = xml_fmtypes_to_text(doc);
			break;
	}

	if (res->children) {
		FILE *f;

		if (overwrite) {
			f = fopen(path, "w");
		} else {
			f = stdout;
		}
		fprintf(f, "%s", (char *) res->children->content);
		if (overwrite) {
			fclose(f);
		}
	}

	xmlFreeDoc(res);
	xmlFreeDoc(doc);
}

/* Convert a simple text defaults/dmtypes file to the XML version. */
static void text_to_xml(const char *path, enum file f, bool overwrite, bool sort, xmlDocPtr brex, xmlDocPtr brexmap, xmlNodePtr user_defs)
{
	xmlDocPtr doc;

	doc = simple_text_to_xml(path, f, sort, brex, brexmap);

	if (f == DEFAULTS) {
		set_user_defaults(doc, user_defs);
	}

	if (sort) {
		xmlDocPtr res;
		res = sort_entries(doc);
		xmlFreeDoc(doc);
		doc = res;
	}

	if (overwrite) {
		save_xml_doc(doc, path);
	} else {
		save_xml_doc(doc, "-");
	}

	xmlFreeDoc(doc);
}

static void convert_or_dump(enum format fmt, enum file f, const char *fname, bool overwrite, bool sort, xmlDocPtr brex, xmlDocPtr brexmap, xmlNodePtr user_defs)
{
	if (f != NONE && (!brex || f == FMTYPES) && access(fname, F_OK) == -1) {
		fprintf(stderr, S_NO_FILE_ERR, fname);
		exit(EXIT_NO_FILE);
	}

	if (fmt == TEXT) {
		if (f == NONE) {
			dump_defaults_text(fname, overwrite, brex, brexmap, user_defs);
		} else {
			xml_to_text(fname, f, overwrite, sort, brex, brexmap, user_defs);
		}
	} else if (fmt == XML) {
		if (f == NONE) {
			dump_defaults_xml(fname, overwrite, brex, brexmap, user_defs);
		} else {
			text_to_xml(fname, f, overwrite, sort, brex, brexmap, user_defs);
		}
	}
}

static xmlDocPtr read_default_brexmap(void)
{
	char fname[PATH_MAX];

	if (find_config(fname, DEFAULT_BREXMAP_FNAME)) {
		return read_xml_doc(fname, false);
	} else {
		return read_xml_mem((const char *) ___common_brexmap_xml, ___common_brexmap_xml_len);
	}
}

static void dump_brexmap(void)
{
	printf("%.*s", ___common_brexmap_xml_len, ___common_brexmap_xml);
}

int main(int argc, char **argv)
{
	int i;
	enum format fmt = XML;
	enum file f = NONE;
	char *fname = NULL;
	bool overwrite = false;
	bool initialize = false;
	bool sort = false;
	xmlDocPtr brex = NULL;
	xmlDocPtr brexmap = NULL;
	char *dir = NULL;
	xmlNodePtr user_defs, cur = NULL;

	const char *sopts = "b:DdFfiJj:n:o:stv:h?";
	struct option lopts[] = {
		{"version"     , no_argument      , 0, 0},
		{"help"        , no_argument      , 0, 'h'},
		{"brex"        , required_argument, 0, 'b'},
		{"dmtypes"     , no_argument      , 0, 'D'},
		{"defaults"    , no_argument      , 0, 'd'},
		{"fmtypes"     , no_argument      , 0, 'F'},
		{"overwrite"   , no_argument      , 0, 'f'},
		{"init"        , no_argument      , 0, 'i'},
		{"name"        , required_argument, 0, 'n'},
		{"dir"         , required_argument, 0, 'o'},
		{"dump-brexmap", no_argument      , 0, 'J'},
		{"brexmap"     , required_argument, 0, 'j'},
		{"sort"        , no_argument      , 0, 's'},
		{"text"        , no_argument      , 0, 't'},
		{"value"       , required_argument, 0, 'v'},
		LIBXML2_PARSE_LONGOPT_DEFS
		{0, 0, 0, 0}
	};
	int loptind = 0;

	user_defs = xmlNewNode(NULL, BAD_CAST "defs");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case 'b':
				if (!brex) brex = read_xml_doc(optarg, false);
				break;
			case 'D':
				f = DMTYPES;
				if (!fname) fname = strdup(DEFAULT_DMTYPES_FNAME);
				break;
			case 'd':
				f = DEFAULTS;
				if (!fname) fname = strdup(DEFAULT_DEFAULTS_FNAME);
				break;
			case 'F':
				f = FMTYPES;
				if (!fname) fname = strdup(DEFAULT_FMTYPES_FNAME);
				break;
			case 'f':
				overwrite = true;
				break;
			case 'i':
				initialize = true;
				break;
			case 'J':
				dump_brexmap();
				return 0;
			case 'j':
				if (!brexmap) brexmap = read_xml_doc(optarg, false);
				break;
			case 'n':
				cur = xmlNewChild(user_defs, NULL, BAD_CAST "default", NULL);
				xmlSetProp(cur, BAD_CAST "ident", BAD_CAST optarg);
				break;
			case 'o':
				free(dir);
				dir = strdup(optarg);
				break;
			case 's':
				sort = true;
				break;
			case 't':
				fmt = TEXT;
				break;
			case 'v':
				if (cur) {
					xmlSetProp(cur, BAD_CAST "value", BAD_CAST optarg);
				}
				break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	if (!fname) {
		fname = strdup(DEFAULT_DEFAULTS_FNAME);
	}

	if (!brexmap) {
		brexmap = read_default_brexmap();
	}

	if (dir) {
		if (access(dir, F_OK) == -1) {
			int err;

			#ifdef _WIN32
			err = mkdir(dir);
			#else
			err = mkdir(dir, S_IRWXU);
			#endif

			if (err) {
				fprintf(stderr, S_MKDIR_FAILED, dir, strerror(errno));
				exit(EXIT_OS_ERROR);
			}
		}

		if (chdir(dir) != 0) {
			fprintf(stderr, S_CHDIR_FAILED, dir, strerror(errno));
			exit(EXIT_OS_ERROR);
		}
	}

	if (initialize) {
		char sys[256];
		const char *opt;
		void (*fn)(const char *, bool, xmlDocPtr, xmlDocPtr, xmlNodePtr);

		if (fmt == TEXT) {
			opt = "-.";
			fn = dump_defaults_text;
		} else {
			opt = "-,";
			fn = dump_defaults_xml;
		}


		if (overwrite || access(DEFAULT_DEFAULTS_FNAME, F_OK) == -1) {
			fn(DEFAULT_DEFAULTS_FNAME, true, brex, brexmap, user_defs);

			#ifdef _WIN32
			SetFileAttributes(DEFAULT_DEFAULTS_FNAME, FILE_ATTRIBUTE_HIDDEN);
			#endif
		}

		if (overwrite || access(DEFAULT_DMTYPES_FNAME, F_OK) == -1) {
			if (brex) {
				xmlDocPtr dmtypes;

				dmtypes = new_dmtypes_from_brex(brex, brexmap);

				if (fmt == TEXT) {
					xmlDocPtr res;
					FILE *f;
					res = xml_dmtypes_to_text(dmtypes);
					f = fopen(DEFAULT_DMTYPES_FNAME, "w");
					fprintf(f, "%s", (char *) res->children->content);
					fclose(f);
					xmlFreeDoc(res);
				} else {
					save_xml_doc(dmtypes, DEFAULT_DMTYPES_FNAME);
				}

				xmlFreeDoc(dmtypes);
			} else {
				snprintf(sys, 256, "s1kd-newdm %s > %s", opt, DEFAULT_DMTYPES_FNAME);

				if (system(sys) != 0) {
					fprintf(stderr, S_DMTYPES_ERR);
				}
			}

			#ifdef _WIN32
			SetFileAttributes(DEFAULT_DMTYPES_FNAME, FILE_ATTRIBUTE_HIDDEN);
			#endif
		}

		if (overwrite || access(DEFAULT_FMTYPES_FNAME, F_OK) == -1) {
			snprintf(sys, 256, "s1kd-fmgen %s > %s", opt, DEFAULT_FMTYPES_FNAME);

			if (system(sys) != 0) {
				fprintf(stderr, S_FMTYPES_ERR);
			}

			#ifdef _WIN32
			SetFileAttributes(DEFAULT_FMTYPES_FNAME, FILE_ATTRIBUTE_HIDDEN);
			#endif
		}
	} else if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			convert_or_dump(fmt, f, argv[i], overwrite, sort, brex, brexmap, user_defs);
		}
	} else {
		convert_or_dump(fmt, f, fname, overwrite, sort, brex, brexmap, user_defs);
	}

	free(fname);
	free(dir);
	xmlFreeDoc(brex);
	xmlFreeDoc(brexmap);
	xmlFreeNode(user_defs);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
