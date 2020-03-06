#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <time.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "s1kd_tools.h"

#define PROG_NAME "s1kd-upissue"
#define VERSION "1.16.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define E_BAD_LIST ERR_PREFIX "Could not read list: %s\n"
#define E_ICN_INWORK ERR_PREFIX "ICNs cannot have inwork issues.\n"
#define E_BAD_FILENAME ERR_PREFIX "Filename does not contain issue info and -N not specified.\n"
#define E_ISSUE_TOO_LARGE ERR_PREFIX "%s is at the max issue number.\n"
#define E_INWORK_TOO_LARGE ERR_PREFIX "%s is at the max inwork number.\n"
#define E_NON_XML_STDIN ERR_PREFIX "Cannot use -m, -N or read from stdin when file does not contain issue info metadata.\n"

#define EXIT_NO_FILE 1
#define EXIT_NO_OVERWRITE 2
#define EXIT_BAD_FILENAME 3
#define EXIT_BAD_DATE 4
#define EXIT_ICN_INWORK 5
#define EXIT_ISSUE_TOO_LARGE 6

static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-4DdefHIilmNqRruvw] [-1 <type>] [-2 <type>] [-c <reason>] [-s <status>] [-t <urt>] [-z <date>] [<file>...]");
	putchar('\n');
	puts("Options:");
	puts("  -1, --first-ver <type>       Set first verification type.");
	puts("  -2, --second-ver <type>      Set second verification type.");
	puts("  -4, --remove-marks           Remove change marks (but not RFUs).");
	puts("  -c, --reason <reason>        Add an RFU to the upissued object.");
	puts("  -D, --remove-deleted         Remove \"delete\"d elements.");
	puts("  -d, --dry-run                Do not write anything, only print new filename.");
	puts("  -e, --erase                  Remove old issue.");
	puts("  -f, --overwrite              Overwrite existing upissued object.");
	puts("  -I, --(keep|change)-date     Do not change issue date. In -m mode, change issue date.");
	puts("  -i, --official               Increase issue number instead of inwork.");
	puts("  -l, --list                   Treat input as list of objects.");
	puts("  -m, --modify                 Modify metadata without upissuing.");
	puts("  -N, --omit-issue             Omit issue/inwork numbers from filename.");
	puts("  -q, --(keep|reset)-qa        Keep quality assurance from old issue. In -m mode, set to unverified.");
	puts("  -R, --keep-unassoc-marks     Only delete change marks associated with an RFU.");
	puts("  -r, --(keep|remove)-changes  Keep RFUs and change marks from old issue. In -m mode, remove them.");
	puts("  -s, --status <status>        Set change status type.");
	puts("  -t, --type <urt>             Set the updateReasonType of the last RFU.");
	puts("  -u, --clean-rfus             Remove unassociated RFUs.");
	puts("  -H, --highlight              Highlight the last RFU.");
	puts("  -v, --verbose                Print filename of upissued objects.");
	puts("  -w, --lock                   Make old and official issues read-only.");
	puts("  -z, --date <date>            The issue date to use for the upissued objects.");
	puts("  --version                    Show version information");
	LIBXML2_PARSE_LONGOPT_HELP
}

static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

static xmlNodePtr firstXPathNode(const char *xpath, xmlDocPtr doc)
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

/* Remove change markup attributes from elements referencing old RFUs */
static void del_assoc_rfu_attrs(xmlNodePtr rfu, xmlXPathContextPtr ctx)
{
	xmlXPathObjectPtr obj;
	char *rfuid;

	rfuid = (char *) xmlGetProp(rfu, BAD_CAST "id");

	obj = xmlXPathEvalExpression(BAD_CAST "//*[@reasonForUpdateRefIds]", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			char *ids, *id = NULL;
			bool used = false;

			ids = (char *) xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "reasonForUpdateRefIds");

			while ((id = strtok(id ? NULL : ids, " "))) {
				if (strcmp(id, rfuid) == 0) {
					used = true;
					break;
				}
			}

			xmlFree(ids);

			if (used) {
				xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "changeType");
				xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "changeMark");
				xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "reasonForUpdateRefIds");
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlFree(rfuid);
}

/* Determine if an RFU is ever referenced */
static bool rfu_used(xmlNodePtr rfu, xmlXPathContextPtr ctx)
{
	xmlXPathObjectPtr obj;
	bool ret = false;
	char *rfuid;

	rfuid = (char *) xmlGetProp(rfu, BAD_CAST "id");

	obj = xmlXPathEvalExpression(BAD_CAST "//@reasonForUpdateRefIds", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			char *ids, *id = NULL;

			ids = (char *) xmlNodeGetContent(obj->nodesetval->nodeTab[i]);

			while ((id = strtok(id ? NULL : ids, " "))) {
				if (strcmp(id, rfuid) == 0) {
					ret = true;
					break;
				}
			}

			xmlFree(ids);

			if (ret) {
				break;
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlFree(rfuid);

	return ret;
}

/* Remove RFUs which are never referenced */
static void rem_unassoc_rfus(xmlDocPtr doc)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST "//reasonForUpdate", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			if (!rfu_used(obj->nodesetval->nodeTab[i], ctx)) {
				xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
				xmlFreeNode(obj->nodesetval->nodeTab[i]);
				obj->nodesetval->nodeTab[i] = NULL;
			}
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Remove all change markup attributes */
static void del_rfu_attrs(xmlXPathContextPtr ctx, bool iss30)
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

/* Remove elements marked as "delete". */
static void rem_delete(xmlDocPtr doc, bool iss30)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(doc);

	if (iss30) {
		obj = xmlXPathEvalExpression(BAD_CAST "//*[@change='delete']", ctx);
	} else {
		obj = xmlXPathEvalExpression(BAD_CAST "//*[@changeType='delete']", ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
			xmlFreeNode(obj->nodesetval->nodeTab[i]);
			obj->nodesetval->nodeTab[i] = NULL;
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Delete old RFUs */
static void del_rfus(xmlDocPtr doc, bool only_assoc, bool iss30)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST "//reasonForUpdate", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		if (only_assoc) {
			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				del_assoc_rfu_attrs(obj->nodesetval->nodeTab[i], ctx);
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

static void del_marks(xmlDocPtr doc, bool iss30)
{
	xmlXPathContextPtr ctx;
	ctx = xmlXPathNewContext(doc);
	del_rfu_attrs(ctx, iss30);
	xmlXPathFreeContext(ctx);
}

static void set_unverified(xmlDocPtr doc, bool iss30)
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

static void set_qa(xmlDocPtr doc, char *firstver, char *secondver, bool iss30)
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

static void add_rfus(xmlDocPtr doc, xmlNodePtr rfus, bool iss30)
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

static void set_iss_date(xmlDocPtr dmdoc, const char *issdate)
{
	char year_s[5], month_s[3], day_s[3];
	xmlNodePtr issueDate;

	issueDate = firstXPathNode("//issueDate|//issdate", dmdoc);

	if (issdate) {
		sscanf(issdate, "%4s-%2s-%2s", year_s, month_s, day_s);
	} else {
		time_t now;
		struct tm *local;
		unsigned short year, month, day;

		time(&now);
		local = localtime(&now);

		year = local->tm_year + 1900;
		month = local->tm_mon + 1;
		day = local->tm_mday;

		if (snprintf(year_s, 5, "%u", year) < 0 ||
		    snprintf(month_s, 3, "%.2u", month) < 0 ||
		    snprintf(day_s, 3, "%.2u", day) < 0)
			exit(EXIT_BAD_DATE);
	}

	xmlSetProp(issueDate, (xmlChar *) "year",  (xmlChar *) year_s);
	xmlSetProp(issueDate, (xmlChar *) "month", (xmlChar *) month_s);
	xmlSetProp(issueDate, (xmlChar *) "day",   (xmlChar *) day_s);
}

static void set_status(xmlDocPtr dmdoc, const char *status, bool iss30, xmlNodePtr issueInfo)
{
	xmlNodePtr dmStatus;

	if (iss30) {
		xmlSetProp(issueInfo, BAD_CAST "type", BAD_CAST status);
	} else {
		if ((dmStatus = firstXPathNode("//dmStatus|//pmStatus", dmdoc))) {
			xmlSetProp(dmStatus, BAD_CAST "issueType", BAD_CAST status);
		}
	}
}

/* Upissue options */
static bool verbose = false;
static bool newissue = false;
static bool overwrite = false;
static char *status = NULL;
static bool no_issue = false;
static bool keep_rfus = false;
static bool set_date = true;
static bool only_assoc_rfus = false;
static bool set_unverif = true;
static bool dry_run = false;
static char *firstver = NULL;
static char *secondver = NULL;
static xmlNodePtr rfus = NULL;
static bool lock = false;
static bool remdel= false;
static bool remold = false;
static bool only_mod = false;
static bool clean_rfus = false;
static char *issdate = NULL;
static bool remove_marks = false;

static void upissue(const char *path)
{
	char *issueNumber = NULL;
	char *inWork = NULL;
	int issueNumber_int;
	int inWork_int;
	char dmfile[PATH_MAX], cpfile[PATH_MAX];
	xmlDocPtr dmdoc;
	xmlNodePtr issueInfo;
	xmlChar *issno_name, *inwork_name;
	bool iss30 = false;
	char upissued_issueNumber[32];
	char upissued_inWork[32];
	char *p, *i = NULL;

	strcpy(dmfile, path);

	if (access(dmfile, F_OK) == -1 && strcmp(dmfile, "-") != 0) {
		fprintf(stderr, ERR_PREFIX "Could not read file %s.\n", dmfile);
		exit(EXIT_NO_FILE);
	}

	dmdoc = read_xml_doc(dmfile);

	if (dmdoc) {
		issueInfo = firstXPathNode("//issueInfo|//issno", dmdoc);
	} else {
		issueInfo = NULL;
	}

	if (!issueInfo && no_issue) {
		fprintf(stderr, E_NON_XML_STDIN);
		exit(EXIT_NO_OVERWRITE);
	}

	/* Apply modifications without actually upissuing. */
	if (only_mod) {
		iss30 = xmlStrcmp(issueInfo->name, BAD_CAST "issueInfo") != 0;

		if (clean_rfus && !iss30) {
			rem_unassoc_rfus(dmdoc);
		}

		if (remdel) {
			rem_delete(dmdoc, iss30);
		}

		add_rfus(dmdoc, rfus, iss30);
		set_qa(dmdoc, firstver, secondver, iss30);
		if (status) {
			set_status(dmdoc, status, iss30, issueInfo);
		}

		/* The following options have the opposite effect in -m mode:
		 * -I sets the date
		 * -r removes RFUs
		 * -q sets the QA status to unverified
		 */
		if (!set_date) {
			set_iss_date(dmdoc, issdate);
		}
		if (keep_rfus) {
			del_rfus(dmdoc, only_assoc_rfus, iss30);
		} else if (remove_marks) {
			del_marks(dmdoc, iss30);
		}
		if (!set_unverif) {
			set_unverified(dmdoc, iss30);
		}

		if (overwrite) {
			save_xml_doc(dmdoc, dmfile);
		} else {
			save_xml_doc(dmdoc, "-");
		}

		xmlFreeDoc(dmdoc);

		return;
	}

	p = strchr(dmfile, '_');

	/* Issue info from XML */
	if (issueInfo) {
		i = p + 1;

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

		if (!inWork) {
			inWork = strdup("00");
		}
	/* Get issue/inwork from filename only */
	} else if (p) {
		i = p + 1;

		if (i > dmfile + strlen(dmfile) - 6) {
			fprintf(stderr, E_BAD_FILENAME);
			exit(EXIT_BAD_FILENAME);
		}

		issueNumber = calloc(4, 1);
		inWork = calloc(3, 1);

		strncpy(issueNumber, i, 3);
		strncpy(inWork, i + 4, 2);
	/* Get issue from ICN */
	} else if ((p = strchr(dmfile, '-'))) {
		int n, c = 0;
		int l;

		if (!newissue) {
			fprintf(stderr, E_ICN_INWORK);
			exit(EXIT_ICN_INWORK);
		}

		l = strlen(dmfile);

		/* Get second-to-last '-' */
		for (n = l; n >= 0; --n) {
			if (dmfile[n] == '-') {
				if (c == 1) {
					break;
				} else {
					++c;
				}
			}
		}

		i = dmfile + n + 1;

		if (n == -1 || i > dmfile + l - 6) {
			fprintf(stderr, E_BAD_FILENAME);
			exit(EXIT_BAD_FILENAME);
		}

		issueNumber = calloc(4, 1);
		strncpy(issueNumber, i, 3);
	} else {
		fprintf(stderr, E_BAD_FILENAME);
		exit(EXIT_BAD_FILENAME);
	}

	if ((issueNumber_int = atoi(issueNumber)) >= 999) {
		fprintf(stderr, E_ISSUE_TOO_LARGE, dmfile);
		exit(EXIT_ISSUE_TOO_LARGE);
	}
	if (inWork) {
		if ((inWork_int = atoi(inWork)) >= 99) {
			fprintf(stderr, E_INWORK_TOO_LARGE, dmfile);
			exit(EXIT_ISSUE_TOO_LARGE);
		}
	}

	if (newissue) {
		snprintf(upissued_issueNumber, 32, "%.3d", issueNumber_int + 1);
		if (inWork) {
			snprintf(upissued_inWork, 32, "%.2d", 0);
		}
	} else {
		snprintf(upissued_issueNumber, 32, "%.3d", issueNumber_int);
		if (inWork) {
			snprintf(upissued_inWork, 32, "%.2d", inWork_int + 1);
		}
	}

	if (issueInfo) {
		xmlSetProp(issueInfo, issno_name, BAD_CAST upissued_issueNumber);
		xmlSetProp(issueInfo, inwork_name, BAD_CAST upissued_inWork);

		/* Optionally cleanup unused RFUs */
		if (clean_rfus && !iss30) {
			rem_unassoc_rfus(dmdoc);
		}

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
		/* Or, remove "delete"d elements any time. */
		} else if (remdel) {
			rem_delete(dmdoc, iss30);
		}

		if (set_date) {
			set_iss_date(dmdoc, issdate);
		}

		set_qa(dmdoc, firstver, secondver, iss30);
		add_rfus(dmdoc, rfus, iss30);

		/* Default status is "new" before issue 1, and "changed" after */
		if (issueNumber_int < 1 && !status) {
			status = strdup("new");
		} else if (!status) {
			status = strdup("changed");
		}

		set_status(dmdoc, status, iss30, issueInfo);

		if (remove_marks) {
			del_marks(dmdoc, iss30);
		}
	}

	xmlFree(issueNumber);
	xmlFree(inWork);

	if (!dmdoc) { /* Preserve non-XML filename for copying */
		strcpy(cpfile, dmfile);
	}

	if (!no_issue) {
		if (!dry_run) {
			if (remold && dmdoc) { /* Delete previous issue (XML file) */
				remove(dmfile);
			} else if (lock) { /* Remove write permission from previous issue. */
				mkreadonly(dmfile);
			}
		}

		if (p) {
			memcpy(i, upissued_issueNumber, 3);
			if (inWork) {
				memcpy(i + 4, upissued_inWork, 2);
			}
		}
	}

	if (!dry_run) {
		if (!overwrite && access(dmfile, F_OK) != -1) {
			fprintf(stderr, ERR_PREFIX "%s already exists.\n", dmfile);
			exit(EXIT_NO_OVERWRITE);
		}

		if (dmdoc) {
			save_xml_doc(dmdoc, dmfile);
		} else { /* Copy non-XML file */
			copy(cpfile, dmfile);

			if (remold) { /* Delete previous issue (non-XML file) */
				remove(cpfile);
			}
		}

		/* Lock official issues. */
		if (lock && newissue) {
			mkreadonly(dmfile);
		}
	}

	if (verbose) {
		puts(dmfile);
	}

	if (dmdoc) {
		xmlFreeDoc(dmdoc);
	}
}

static void upissue_list(const char *path)
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

	const char *sopts = "ivs:NfrRIq1:2:4Ddelc:t:Hwmuz:h?";
	struct option lopts[] = {
		{"version"           , no_argument      , 0, 0},
		{"help"              , no_argument      , 0, 'h'},
		{"first-ver"         , required_argument, 0, '1'},
		{"second-ver"        , required_argument, 0, '2'},
		{"remove-marks"      , no_argument      , 0, '4'},
		{"reason"            , required_argument, 0, 'c'},
		{"remove-deleted"    , no_argument      , 0, 'D'},
		{"dry-run"           , no_argument      , 0, 'd'},
		{"erase"             , no_argument      , 0, 'e'},
		{"overwrite"         , no_argument      , 0, 'f'},
		{"keep-date"         , no_argument      , 0, 'I'},
		{"change-date"       , no_argument      , 0, 'I'},
		{"official"          , no_argument      , 0, 'i'},
		{"list"              , no_argument      , 0, 'l'},
		{"modify"            , no_argument      , 0, 'm'},
		{"omit-issue"        , no_argument      , 0, 'N'},
		{"keep-qa"           , no_argument      , 0, 'q'},
		{"reset-qa"          , no_argument      , 0, 'q'},
		{"keep-unassoc-marks", no_argument      , 0, 'R'},
		{"keep-changes"      , no_argument      , 0, 'r'},
		{"remove-changes"    , no_argument      , 0, 'r'},
		{"status"            , required_argument, 0, 's'},
		{"type"              , required_argument, 0, 't'},
		{"clean-rfus"        , no_argument      , 0, 'u'},
		{"highlight"         , no_argument      , 0, 'H'},
		{"verbose"           , no_argument      , 0, 'v'},
		{"lock"              , no_argument      , 0, 'w'},
		{"date"              , required_argument, 0, 'z'},
		LIBXML2_PARSE_LONGOPT_DEFS
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
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
				break;
			case '1':
				firstver = strdup(optarg);
				break;
			case '2':
				secondver = strdup(optarg);
				break;
			case '4':
				remove_marks = true;
				break;
			case 'c':
				xmlNewChild(rfus, NULL, BAD_CAST "reasonForUpdate", BAD_CAST optarg);
				break;
			case 'D':
				remdel = true;
				break;
			case 'd':
				dry_run = true;
				verbose = true;
				break;
			case 'e':
				remold = true;
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
			case 'm':
				only_mod = true;
				no_issue = true;
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
				if (!(strcmp(status, "changed") == 0 || strcmp(status, "rinstate-changed") == 0)) {
					remove_marks = true;
				}
				break;
			case 't':
				xmlSetProp(rfus->last, BAD_CAST "updateReasonType", BAD_CAST optarg);
				break;
			case 'u':
				clean_rfus = true;
				break;
			case 'H':
				xmlSetProp(rfus->last, BAD_CAST "updateHighlight", BAD_CAST "1");
				break;
			case 'v':
				verbose = true;
				break;
			case 'w':
				lock = true;
				break;
			case 'z':
				issdate = strdup(optarg);
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
	free(issdate);
	xmlFreeNode(rfus);

	xmlCleanupParser();

	return 0;
}
