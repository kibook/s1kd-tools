#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <time.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-upissue"
#define VERSION "1.4.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define E_BAD_LIST "Could not read list: %s\n"

#define EXIT_NO_FILE 1
#define EXIT_NO_OVERWRITE 2
#define EXIT_BAD_FILENAME 3

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-dfHIilNqRrv] [-1 <type>] [-2 <type>] [-c <reason>] [-s <status>] [-t <urt>] [<file>...]");
	putchar('\n');
	puts("Options:");
	puts("  -1 <type>    Set first verification type.");
	puts("  -2 <type>    Set second verification type.");
	puts("  -c <reason>  Add an RFU to the upissued object.");
	puts("  -d           Do not write anything, only print new filename.");
	puts("  -f           Overwrite existing upissued object.");
	puts("  -I           Do not change issue date.");
	puts("  -i           Create a new issue of the object.");
	puts("  -l           Treat input as list of objects.");
	puts("  -N           Omit issue/inwork numbers from filename.");
	puts("  -q           Keep quality assurance from old issue.");
	puts("  -R           Only delete change marks associated with an RFU.");
	puts("  -r           Keep RFUs from old issue.");
	puts("  -s <status>  Set change status type.");
	puts("  -t <urt>     Set the updateReasonType of the last RFU.");
	puts("  -H           Highlight the last RFU.");
	puts("  -v           Print filename of upissued objects.");
	puts("  --version    Show version information");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

xmlNodePtr firstXPathNode(const char *xpath, xmlDocPtr doc)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr node;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval))
		node = NULL;
	else
		node = obj->nodesetval->nodeTab[0];
	
	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return node;
}

void copy(const char *from, const char *to)
{

	FILE *f1, *f2;
	char buf[4096];
	size_t n;

	f1 = fopen(from, "rb");
	f2 = fopen(to, "wb");

	while ((n = fread(buf, 1, 4096, f1)) > 0) {
		fwrite(buf, 1, n, f2);
	}

	fclose(f1);
	fclose(f2);
}

/* Remove change markup attributes from elements referencing old RFUs */
void del_assoc_rfu_attrs(xmlNodePtr rfu, xmlXPathContextPtr ctx, bool iss30)
{
	char xpath[256];
	xmlXPathObjectPtr obj;
	char *id;

	id = (char *) xmlGetProp(rfu, BAD_CAST "id");

	sprintf(xpath, "//*[contains(@reasonForUpdateRefIds, '%s')]", id);

	xmlFree(id);

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "changeType");
			xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "changeMark");
			xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "reasonForUpdateRefIds");
		}
	}

	xmlXPathFreeObject(obj);
}

/* Remove all change markup attributes */
void del_rfu_attrs(xmlXPathContextPtr ctx, bool iss30)
{
	xmlXPathObjectPtr obj;
	const xmlChar *change, *mark, *rfc;

	if (iss30) {
		change = BAD_CAST "change";
		mark = BAD_CAST "mark";
		rfc = BAD_CAST "rfc";
		obj = xmlXPathEvalExpression(BAD_CAST "//*[@change or @mark or @rfc or @level]", ctx);
	} else {
		change = BAD_CAST "changeType";
		mark = BAD_CAST "changeMark";
		rfc = BAD_CAST "reasonForUpdateRefIds";
		obj = xmlXPathEvalExpression(BAD_CAST "//*[@changeType or @changeMark or @reasonForUpdateRefIds]", ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlChar *type;

			type = xmlGetProp(obj->nodesetval->nodeTab[i], change);

			if (xmlStrcmp(type, BAD_CAST "delete") == 0) {
				xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
				xmlFreeNode(obj->nodesetval->nodeTab[i]);
				obj->nodesetval->nodeTab[i] = NULL;
			} else {
				xmlUnsetProp(obj->nodesetval->nodeTab[i], change);
				xmlUnsetProp(obj->nodesetval->nodeTab[i], mark);
				xmlUnsetProp(obj->nodesetval->nodeTab[i], rfc);

				if (iss30) {
					xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "level");
				}
			}

			xmlFree(type);
		}
	}

	xmlXPathFreeObject(obj);
}

/* Delete old RFUs */
void del_rfus(xmlDocPtr doc, bool only_assoc, bool iss30)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(doc);
	if (iss30) {
		obj = xmlXPathEvalExpression(BAD_CAST "//rfu", ctx);
	} else {
		obj = xmlXPathEvalExpression(BAD_CAST "//reasonForUpdate", ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		if (only_assoc) {
			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				del_assoc_rfu_attrs(obj->nodesetval->nodeTab[i], ctx, iss30);
				xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
				xmlFreeNode(obj->nodesetval->nodeTab[i]);
				obj->nodesetval->nodeTab[i] = NULL;
			}
		} else {
			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
				xmlFreeNode(obj->nodesetval->nodeTab[i]);
				obj->nodesetval->nodeTab[i] = NULL;
			}

			del_rfu_attrs(ctx, iss30);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

void set_unverified(xmlDocPtr doc, bool iss30)
{
	xmlNodePtr qa, cur;

	qa = firstXPathNode(iss30 ? "//qa" : "//qualityAssurance", doc);

	if (!qa)
		return;

	cur = qa->children;
	while (cur) {
		xmlNodePtr next = cur->next;
		xmlUnlinkNode(cur);
		xmlFreeNode(cur);
		cur = next;
	}

	xmlNewChild(qa, NULL, BAD_CAST (iss30 ? "unverif" : "unverified"), NULL);
}

void set_qa(xmlDocPtr doc, char *firstver, char *secondver, bool iss30)
{
	xmlNodePtr qa, unverif;

	if (!(firstver || secondver))
		return;

	qa = firstXPathNode(iss30 ? "//qa" : "//qualityAssurance", doc);

	if (!qa)
		return;

	unverif = firstXPathNode(iss30 ? "//unverif" : "//unverified", doc);

	if (unverif) {
		xmlUnlinkNode(unverif);
		xmlFreeNode(unverif);
	}

	if (firstver) {
		xmlNodePtr ver1;
		ver1 = firstXPathNode(iss30 ? "//firstver" : "//firstVerification", doc);
		if (ver1) {
			xmlUnlinkNode(ver1);
			xmlFreeNode(ver1);
		}
		ver1 = xmlNewChild(qa, NULL, BAD_CAST (iss30 ? "firstver" : "firstVerification"), NULL);
		xmlSetProp(ver1, BAD_CAST (iss30 ? "type" : "verificationType"), BAD_CAST firstver);
	}

	if (secondver) {
		xmlNodePtr ver2;
		ver2 = firstXPathNode(iss30 ? "//secver" : "//secondVerification", doc);
		if (ver2) {
			xmlUnlinkNode(ver2);
			xmlFreeNode(ver2);
		}
		ver2 = xmlNewChild(qa, NULL, BAD_CAST (iss30 ? "secver" : "secondVerification"), NULL);
		xmlSetProp(ver2, BAD_CAST (iss30 ? "type" : "verificationType"), BAD_CAST secondver);
	}
}

/* Add RFUs to the upissued object. */
#define ISS_30_RFU_PATH "(//qa|//sbc|//fic|//ein|//skill|//rfu)[last()]"
#define ISS_4X_RFU_PATH "(//qualityAssurance|//systemBreakdownCode|//functionalItemCode|//functionalItemRef|//skillLevel|//reasonForUpdate)[last()]"

void add_rfus(xmlDocPtr doc, xmlNodePtr rfus, bool iss30)
{
	xmlNodePtr node, cur, next;

	node = firstXPathNode(iss30 ? ISS_30_RFU_PATH : ISS_4X_RFU_PATH, doc);

	if (!node) {
		return;
	}

	next = xmlNextElementSibling(node);

	while (next && (
		xmlStrcmp(next->name, BAD_CAST "rfu") == 0 ||
		xmlStrcmp(next->name, BAD_CAST "reasonForUpdate") == 0)) {
		node = next;
		next = xmlNextElementSibling(node);
	}

	for (cur = rfus->last; cur; cur = cur->prev) {
		xmlNodePtr rfu;

		rfu = xmlCopyNode(cur, 1);

		if (iss30) {
			xmlNodeSetName(rfu, BAD_CAST "rfu");
			xmlUnsetProp(rfu, BAD_CAST "updateReasonType");
			xmlUnsetProp(rfu, BAD_CAST "updateHighlight");
		} else {
			xmlChar *content;

			content = xmlNodeGetContent(rfu);
			xmlNodeSetContent(rfu, NULL);
			xmlNewChild(rfu, NULL, BAD_CAST "simplePara", content);
			xmlFree(content);
		}

		xmlAddNextSibling(node, rfu);
	}
}

/* Upissue options */
bool verbose = false;
bool newissue = false;
bool overwrite = false;
char *status = NULL;
bool no_issue = false;
bool keep_rfus = false;
bool set_date = true;
bool only_assoc_rfus = false;
bool set_unverif = true;
bool dry_run = false;
char *firstver = NULL;
char *secondver = NULL;
xmlNodePtr rfus = NULL;

void upissue(const char *path)
{
	char *issueNumber;
	char *inWork;
	int issueNumber_int;
	int inWork_int;
	char dmfile[PATH_MAX], cpfile[PATH_MAX];
	xmlDocPtr dmdoc;
	xmlNodePtr issueInfo;
	xmlNodePtr issueDate;
	xmlNodePtr dmStatus;
	xmlChar *issno_name, *inwork_name;
	bool iss30 = false;
	char upissued_issueNumber[32];
	char upissued_inWork[32];

	strcpy(dmfile, path);

	if (access(dmfile, F_OK) == -1 && strcmp(dmfile, "-") != 0) {
		fprintf(stderr, ERR_PREFIX "Could not read file %s.\n", dmfile);
		exit(EXIT_NO_FILE);
	}

	dmdoc = xmlReadFile(dmfile, NULL, PARSE_OPTS | XML_PARSE_NONET | XML_PARSE_NOERROR);

	if (dmdoc) {
		issueInfo = firstXPathNode("//issueInfo|//issno", dmdoc);
	} else {
		issueInfo = NULL;
	}

	if (!issueInfo && no_issue) {
		fprintf(stderr, ERR_PREFIX "Cannot use -N or read from stdin when file does not contain issue info metadata.\n");
		exit(EXIT_NO_OVERWRITE);
	}

	if (issueInfo) {
		iss30 = strcmp((char *) issueInfo->name, "issueInfo") != 0;

		if (iss30) {
			issno_name = BAD_CAST "issno";
			inwork_name = BAD_CAST "inwork";
		} else {
			issno_name = BAD_CAST "issueNumber";
			inwork_name = BAD_CAST "inWork";
		}

		issueNumber = (char *) xmlGetProp(issueInfo, issno_name);
		inWork = (char *) xmlGetProp(issueInfo, inwork_name);
	} else { /* Get issue/inwork from filename only */
		char *i;

		i = strchr(dmfile, '_') + 1;

		if (i < dmfile || i > dmfile + strlen(dmfile) - 6) {
			fprintf(stderr, ERR_PREFIX "Filename does not contain issue info and -N not specified.\n");
			exit(EXIT_BAD_FILENAME);
		}

		issueNumber = calloc(4, 1);
		inWork = calloc(3, 1);

		strncpy(issueNumber, i, 3);
		strncpy(inWork, i + 4, 2);
	}

	issueNumber_int = atoi(issueNumber);
	inWork_int = atoi(inWork);

	if (newissue) {
		snprintf(upissued_issueNumber, 32, "%.3d", issueNumber_int + 1);
		snprintf(upissued_inWork, 32, "%.2d", 0);
	} else {
		snprintf(upissued_issueNumber, 32, "%.3d", issueNumber_int);
		snprintf(upissued_inWork, 32, "%.2d", inWork_int + 1);
	}

	if (issueInfo) {
		xmlSetProp(issueInfo, issno_name, BAD_CAST upissued_issueNumber);
		xmlSetProp(issueInfo, inwork_name, BAD_CAST upissued_inWork);

		/* When upissuing an official module to first inwork issue... */
		if (strcmp(inWork, "00") == 0) {
				/* Delete RFUs */
				if (!keep_rfus) {
					del_rfus(dmdoc, only_assoc_rfus, iss30);
				}

				/* Set unverified */
				if (set_unverif) {
					set_unverified(dmdoc, iss30);
				}
		}

		if (set_date) {
			time_t now;
			struct tm *local;
			int year, month, day;
			char year_s[5], month_s[3], day_s[3];

			issueDate = firstXPathNode("//issueDate|//issdate", dmdoc);

			time(&now);
			local = localtime(&now);
			year = local->tm_year + 1900;
			month = local->tm_mon + 1;
			day = local->tm_mday;
			sprintf(year_s, "%d", year);
			sprintf(month_s, "%.2d", month);
			sprintf(day_s, "%.2d", day);

			xmlSetProp(issueDate, (xmlChar *) "year",  (xmlChar *) year_s);
			xmlSetProp(issueDate, (xmlChar *) "month", (xmlChar *) month_s);
			xmlSetProp(issueDate, (xmlChar *) "day",   (xmlChar *) day_s);
		}

		set_qa(dmdoc, firstver, secondver, iss30);
		add_rfus(dmdoc, rfus, iss30);

		/* Default status is "new" before issue 1, and "changed" after */
		if (issueNumber_int < 1 && !status) {
			status = strdup("new");
		} else if (!status) {
			status = strdup("changed");
		}

		if (iss30) {
			xmlSetProp(issueInfo, BAD_CAST "type", BAD_CAST status);
		} else {
			if ((dmStatus = firstXPathNode("//dmStatus|//pmStatus", dmdoc))) {
				xmlSetProp(dmStatus, BAD_CAST "issueType", BAD_CAST status);
			}
		}
	}

	xmlFree(issueNumber);
	xmlFree(inWork);

	if (!dmdoc) { /* Preserve non-XML filename for copying */
		strcpy(cpfile, dmfile);
	}

	if (!no_issue) {
		char *i;

		if ((i = strchr(dmfile, '_'))) {
			strncpy(i + 1, upissued_issueNumber, 3);
			strncpy(i + 5, upissued_inWork, 2);
		}
	}

	if (!dry_run) {
		if (!overwrite && access(dmfile, F_OK) != -1) {
			fprintf(stderr, ERR_PREFIX "%s already exists.\n", dmfile);
			exit(EXIT_NO_OVERWRITE);
		}

		if (dmdoc) {
			xmlSaveFormatFile(dmfile, dmdoc, 1);
		} else { /* Copy non-XML file */
			copy(cpfile, dmfile);
		}
	}

	if (verbose) {
		puts(dmfile);
	}

	if (dmdoc) {
		xmlFreeDoc(dmdoc);
	}
}

void upissue_list(const char *path)
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
		upissue(line);
	}

	if (path) {
		fclose(f);
	}
}

int main(int argc, char **argv)
{
	int i;
	bool islist = false;

	const char *sopts = "ivs:NfrRIq1:2:dlc:t:Hh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	rfus = xmlNewNode(NULL, BAD_CAST "rfus");

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case '1':
				firstver = strdup(optarg);
				break;
			case '2':
				secondver = strdup(optarg);
				break;
			case 'c':
				xmlNewChild(rfus, NULL, BAD_CAST "reasonForUpdate", BAD_CAST optarg);
				break;
			case 'd':
				dry_run = true;
				verbose = true;
				break;
			case 'f':
				overwrite = true;
				break;
			case 'I':
				set_date = false;
				break;
			case 'i':
				newissue = true;
				break;
			case 'l':
				islist = true;
				break;
			case 'N':
				no_issue = true;
				overwrite = true;
				break;
			case 'q':
				set_unverif = false;
				break;
			case 'R':
				only_assoc_rfus = true;
				break;
			case 'r':
				keep_rfus = true;
				break;
			case 's':
				status = strdup(optarg);
				break;
			case 't':
				xmlSetProp(rfus->last, BAD_CAST "updateReasonType", BAD_CAST optarg);
				break;
			case 'H':
				xmlSetProp(rfus->last, BAD_CAST "updateHighlight", BAD_CAST "1");
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

	if (optind < argc) {
		for (i = optind; i < argc; i++) {
			if (islist) {
				upissue_list(argv[i]);
			} else {
				upissue(argv[i]);
			}
		}
	} else if (islist) {
		upissue_list(NULL);
	} else {
		no_issue = true;
		overwrite = true;
		upissue("-");
	}

	free(firstver);
	free(secondver);
	free(status);
	xmlFreeNode(rfus);

	xmlCleanupParser();

	return 0;
}
