#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include "s1kd_tools.h"
#include "uom.h"

#define PROG_NAME "s1kd-uom"
#define VERSION "1.4.1"

#define ERR_PREFIX PROG_NAME ": ERROR: "
#define WRN_PREFIX PROG_NAME ": WARNING: "
#define INF_PREFIX PROG_NAME ": INFO: "
#define E_NO_UOM ERR_PREFIX "%s: Unit conversions must be specified as: -u <uom> -t <uom> [-e <expr>] [-F <fmt>]\n"
#define W_BAD_LIST WRN_PREFIX "Could not read list: %s\n"
#define W_NO_CONV WRN_PREFIX "No conversion defined for %s -> %s.\n"
#define W_NO_CONV_TO WRN_PREFIX "No target UOM given for %s.\n"
#define I_CONVERT INF_PREFIX "Converting units in %s...\n"
#define EXIT_NO_CONV 1
#define EXIT_NO_UOM 2

bool verbose = false;

/* Show usage message. */
void show_help(void)
{
	puts("Usage: " PROG_NAME " [-F <fmt>] [-u <uom> -t <uom> [-e <expr>] [-F <fmt>] ...] [-p <fmt> [-P <path>]] [-U <path>] [-lv,h?] [<object>...]");
	puts("");
	puts("  -e <expr>  Specify formula for a conversion.");
	puts("  -F <fmt>   Number format for converted values.");
	puts("  -h -?      Show help/usage message.");
	puts("  -l         Treat input as list of CSDB objects.");
	puts("  -P <path>  Use custom UOM display file.");
	puts("  -p <fmt>   Preformat quantity data.");
	puts("  -t <uom>   UOM to convert to.");
	puts("  -U <path>  Use custom .uom file.");
	puts("  -u <uom>   UOM to convert from.");
	puts("  -v         Verbose output.");
	puts("  -,         Dump default .uom file.");
	puts("  -.         Dump default UOM preformatting file.");
	puts("  --version  Show version information.");
	puts("  <object>   CSDB object to convert quantities in.");
}

/* Show version information. */
void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s and libxslt %s\n", xmlParserVersion, xsltEngineVersion);
}

/* Select specified UOM to convert. */
void select_uoms(xmlNodePtr uom, xmlNodePtr conversions)
{
	xmlNodePtr cur;

	cur = uom->children;

	while (cur) {
		xmlNodePtr next;

		next = cur->next;

		if (cur->type == XML_ELEMENT_NODE) {
			xmlChar *uom_from, *uom_to;
			xmlNodePtr c;
			bool match = false;

			uom_from = xmlGetProp(cur, BAD_CAST "from");
			uom_to   = xmlGetProp(cur, BAD_CAST "to");

			c = conversions->children;
			while (c) {
				xmlNodePtr n;
				xmlChar *convert_from, *convert_to, *formula, *format;
				bool m;

				n = c->next;

				convert_from = xmlGetProp(c, BAD_CAST "from");
				convert_to   = xmlGetProp(c, BAD_CAST "to");
				formula      = xmlGetProp(c, BAD_CAST "formula");
				format       = xmlGetProp(c, BAD_CAST "format");

				if (!convert_to) {
					convert_to = xmlStrdup(convert_from);
				}

				m = convert_from &&
				    xmlStrcmp(uom_from, convert_from) == 0 &&
				    xmlStrcmp(uom_to, convert_to) == 0;
				
				xmlFree(convert_from);
				xmlFree(convert_to);

				if (m) {
					if (formula) {
						xmlSetProp(cur, BAD_CAST "formula", formula);
					}
					if (format) {
						xmlSetProp(cur, BAD_CAST "format", format);
					}

					xmlUnlinkNode(c);
					xmlFreeNode(c);

					match = true;
					break;
				}

				c = n;
			}

			xmlFree(uom_from);
			xmlFree(uom_to);

			if (!match) {
				xmlUnlinkNode(cur);
				xmlFreeNode(cur);
			}
		} else {
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
		}

		cur = next;
	}

	cur = conversions->children;
	while (cur) {
		xmlNodePtr next;

		next = cur->next;

		if (xmlHasProp(cur, BAD_CAST "formula")) {
			xmlAddChild(uom, xmlCopyNode(cur, 1));
		} else {
			xmlChar *from, *to;

			from = xmlGetProp(cur, BAD_CAST "from");
			to   = xmlGetProp(cur, BAD_CAST "to");

			if (to) {
				fprintf(stderr, W_NO_CONV, (char *) from, (char *) to);
			} else {
				fprintf(stderr, W_NO_CONV_TO, (char *) from);
			}

			xmlFree(from);
			xmlFree(to);
		}

		xmlUnlinkNode(cur);
		xmlFreeNode(cur);

		cur = next;
	}
}

/* Transform a document with a pre-parsed stylesheet. */
void transform_doc_with(xmlDocPtr doc, xmlDocPtr styledoc, const char **params)
{
	xmlDocPtr src, res, sdoc;
	xsltStylesheetPtr style;
	xmlNodePtr old;

	src = xmlCopyDoc(doc, 1);
	sdoc = xmlCopyDoc(styledoc, 1);

	style = xsltParseStylesheetDoc(sdoc);
	res = xsltApplyStylesheet(style, src, params);

	old = xmlDocSetRootElement(doc, xmlCopyNode(xmlDocGetRootElement(res), 1));
	xmlFreeNode(old);

	xmlFreeDoc(src);
	xmlFreeDoc(res);
	xsltFreeStylesheet(style);
}

/* Transform a document with an in-memory stylesheet. */
void transform_doc(xmlDocPtr doc, unsigned char *xsl, unsigned int len, const char **params)
{
	xmlDocPtr styledoc;
	styledoc = read_xml_mem((const char *) xsl, len);
	transform_doc_with(doc, styledoc, params);
	xmlFreeDoc(styledoc);
}

/* Convert UOM for a single data module. */
void convert_uoms(const char *path, xmlDocPtr uom, const char *format, xmlDocPtr uomdisp, const char *dispfmt, bool overwrite)
{
	xmlDocPtr doc;
	char *params[3];

	if (verbose) {
		fprintf(stderr, I_CONVERT, path ? path : "-");
	}

	if (path) {
		doc = read_xml_doc(path);
	} else {
		doc = read_xml_doc("-");
	}

	if (!doc) {
		return;
	}

	if (format) {
		params[0] = "user-format";
		params[1] = malloc(strlen(format) + 3);
		sprintf(params[1], "\"%s\"", format);
		params[2] = NULL;
	} else {
		params[0] = NULL;
	}

	transform_doc(uom, uom_xsl, uom_xsl_len, (const char **) params);
	transform_doc_with(doc, uom, NULL);

	if (format) {
		free(params[1]);
	}

	if (dispfmt) {
		params[0] = "format";
		params[1] = malloc(strlen(dispfmt) + 3);
		sprintf(params[1], "\"%s\"", dispfmt);
		params[2] = NULL;

		transform_doc(uomdisp, uomdisplay_xsl, uomdisplay_xsl_len, (const char **) params);
		transform_doc_with(doc, uomdisp, NULL);

		free(params[1]);
	}

	if (overwrite) {
		save_xml_doc(doc, path);
	} else {
		save_xml_doc(doc, "-");
	}

	xmlFreeDoc(doc);
}

/* Convert UOM for data modules given in a list. */
void convert_uoms_list(const char *path, xmlDocPtr uom, const char *format, xmlDocPtr uomdisp, const char *dispfmt, bool overwrite)
{
	FILE *f;
	char line[PATH_MAX];

	if (path) {
		if (!(f = fopen(path, "r"))) {
			fprintf(stderr, W_BAD_LIST, path);
			return;
		}
	} else {
		f = stdin;
	}

	while (fgets(line, PATH_MAX, f)) {
		strtok(line, "\t\r\n");
		convert_uoms(line, uom, format, uomdisp, dispfmt, overwrite);
	}

	if (path) {
		fclose(f);
	}
}

int main(int argc, char **argv)
{
	int i;

	const char *sopts = "e:F:flP:p:t:U:u:v,.h?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	bool list = false;
	bool overwrite = false;
	bool dump_uom = false;

	xmlDocPtr uom = NULL;
	xmlNodePtr conversions, cur = NULL;

	char *format = NULL;
	char uom_fname[PATH_MAX] = "";

	xmlDocPtr uomdisp = NULL;
	char uomdisp_fname[PATH_MAX] = "";
	char *dispfmt = NULL;
	bool dump_uomdisp = false;

	conversions = xmlNewNode(NULL, BAD_CAST "conversions");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case 'e':
				if (!cur) {
					fprintf(stderr, E_NO_UOM, "-e");
					exit(EXIT_NO_UOM);
				}
				xmlSetProp(cur, BAD_CAST "formula", BAD_CAST optarg);
				break;
			case 'F':
				if (cur) {
					xmlSetProp(cur, BAD_CAST "format", BAD_CAST optarg);
				} else {
					format = strdup(optarg);
				}
				break;
			case 'f':
				overwrite = true;
				break;
			case 'l':
				list = true;
				break;
			case 'P':
				strncpy(uomdisp_fname, optarg, PATH_MAX - 1);
				break;
			case 'p':
				dispfmt = strdup(optarg);
				break;
			case 't':
				if (!cur) {
					fprintf(stderr, E_NO_UOM, "-t");
					exit(EXIT_NO_UOM);
				}
				xmlSetProp(cur, BAD_CAST "to", BAD_CAST optarg);
				break;
			case 'U':
				strncpy(uom_fname, optarg, PATH_MAX - 1);
				break;
			case 'u':
				cur = xmlNewChild(conversions, NULL, BAD_CAST "convert", NULL);
				xmlSetProp(cur, BAD_CAST "from", BAD_CAST optarg);
				break;
			case 'v':
				verbose = true;
				break;
			case ',':
				dump_uom = true;
				break;
			case '.':
				dump_uomdisp = true;
				break;
			case 'h':
			case '?':
				show_help();
				return 0;
		}
	}

	/* Load .uom configuration file (or built-in copy). */
	if (!dump_uom && (strcmp(uom_fname, "") != 0 || find_config(uom_fname, DEFAULT_UOM_FNAME))) {
		uom = read_xml_doc(uom_fname);
	}
	if (!uom) {
		uom = read_xml_mem((const char *) uom_xml, uom_xml_len);
	}

	/* Load .uomdisplay configuration file (or built-in copy). */
	if (!dump_uomdisp && (strcmp(uomdisp_fname, "") != 0 || find_config(uomdisp_fname, DEFAULT_UOMDISP_FNAME))) {
		uomdisp = read_xml_doc(uomdisp_fname);
	}
	if (!uomdisp) {
		uomdisp = read_xml_mem((const char *) uomdisplay_xml, uomdisplay_xml_len);
	}

	if (conversions->children) {
		select_uoms(xmlDocGetRootElement(uom), conversions);
	}

	if (dump_uom) {
		save_xml_doc(uom, "-");
	} else if (dump_uomdisp) {
		save_xml_doc(uomdisp, "-");
	} else if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			if (list) {
				convert_uoms_list(argv[i], uom, format, uomdisp, dispfmt, overwrite);
			} else {
				convert_uoms(argv[i], uom, format, uomdisp, dispfmt, overwrite);
			}
		}
	} else if (list) {
		convert_uoms_list(NULL, uom, format, uomdisp, dispfmt, overwrite);
	} else {
		convert_uoms(NULL, uom, format, uomdisp, dispfmt, false);
	}
	
	free(format);
	free(dispfmt);

	xmlFreeDoc(uom);
	xmlFreeNode(conversions);
	xmlFreeDoc(uomdisp);

	xsltCleanupGlobals();
	xmlCleanupParser();

	return 0;
}
