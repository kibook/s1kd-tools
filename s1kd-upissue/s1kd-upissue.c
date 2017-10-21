#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define PROG_NAME "s1kd-upissue"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_NO_FILE 1
#define EXIT_NO_OVERWRITE 2
#define EXIT_BAD_FILENAME 3

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-viN] DATAMODULE");
	putchar('\n');
	puts("Options:");
	puts("  -v	Print filename of upissued data module");
	puts("  -i	Create a new issue of the data module");
	puts("  -N	Omit issue/inwork numbers from filename");
	puts("  -r      Keep RFUs from old issue");
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
void del_rfu_attrs(xmlNodePtr rfu, xmlXPathContextPtr ctx)
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

/* Delete old RFUs */
void del_rfus(xmlDocPtr doc)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(doc);
	obj = xmlXPathEvalExpression(BAD_CAST "//reasonForUpdate", ctx);

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			del_rfu_attrs(obj->nodesetval->nodeTab[i], ctx);
			xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
			xmlFreeNode(obj->nodesetval->nodeTab[i]);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

int main(int argc, char **argv)
{
	char dmfile[256], cpfile[256];
	xmlDocPtr dmdoc;

	xmlNodePtr issueInfo;
	xmlNodePtr issueDate;
	xmlNodePtr dmStatus;

	char *issueNumber;
	char *inWork;

	char upissued_issueNumber[32];
	char upissued_inWork[32];

	int issueNumber_int;
	int inWork_int;

	int c;

	bool verbose = false;
	bool newissue = false;
	bool overwrite = false;

	int i;

	time_t now;
	struct tm *local;
	int year, month, day;
	char year_s[5], month_s[3], day_s[3];

	char status[32] = "changed";

	bool no_issue = false;

	bool keep_rfus = false;

	while ((c = getopt(argc, argv, "ivs:Nfrh?")) != -1) {
		switch (c) {
			case 'i':
				newissue = true;
				break;
			case 'v':
				verbose = true;
				break;
			case 's':
				strcpy(status, optarg);
				break;
			case 'N':
				no_issue = true;
				overwrite = true;
				break;
			case 'f':
				overwrite = true;
				break;
			case 'r':
				keep_rfus = true;
				break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	for (i = optind; i < argc; i++) {
		strcpy(dmfile, argv[i]);

		if (access(dmfile, F_OK) == -1) {
			fprintf(stderr, ERR_PREFIX "Could not read file %s.\n", dmfile);
			exit(EXIT_NO_FILE);
		}

		dmdoc = xmlReadFile(dmfile, NULL, XML_PARSE_NONET | XML_PARSE_NOERROR);

		if (dmdoc) {
			issueInfo = firstXPathNode("//issueInfo", dmdoc);
		} else {
			issueInfo = NULL;
		}

		if (!issueInfo && no_issue) {
			fprintf(stderr, ERR_PREFIX "Cannot use -N when file does not contain issue info metadata.\n");
			exit(EXIT_NO_OVERWRITE);
		}

		if (issueInfo) {
			issueNumber = (char *) xmlGetProp(issueInfo, (xmlChar *) "issueNumber");
			inWork = (char *) xmlGetProp(issueInfo, (xmlChar *) "inWork");
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

		xmlFree(issueNumber);
		xmlFree(inWork);
		
		if (newissue) {
			snprintf(upissued_issueNumber, 32, "%.3d", issueNumber_int + 1);
			snprintf(upissued_inWork, 32, "%.2d", 0);
		} else {
			snprintf(upissued_issueNumber, 32, "%.3d", issueNumber_int);
			snprintf(upissued_inWork, 32, "%.2d", inWork_int + 1);
		}

		if (issueInfo) {
			xmlSetProp(issueInfo, (xmlChar *) "issueNumber", (xmlChar *) upissued_issueNumber);
			xmlSetProp(issueInfo, (xmlChar *) "inWork",      (xmlChar *) upissued_inWork);

			if (!keep_rfus && strcmp(upissued_inWork, "01") == 0) {
				del_rfus(dmdoc);
			}

			if (newissue) {
				issueDate = firstXPathNode("//issueDate", dmdoc);

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

				/* Do not change issueType when upissuing from 000 -> 001 */
				if (issueNumber_int > 0) {
					if ((dmStatus = firstXPathNode("//dmStatus|//pmStatus", dmdoc)))
						xmlSetProp(dmStatus, (xmlChar *) "issueType", (xmlChar *) status);
				}
			}
		}

		if (!dmdoc) { /* Preserve non-XML filename for copying */
			strcpy(cpfile, dmfile);
		}

		if (!no_issue) {
			char *i;

			i = strchr(dmfile, '_') + 1;

			strncpy(i, upissued_issueNumber, 3);
			strncpy(i + 4, upissued_inWork, 2);
		}

		if (!overwrite && access(dmfile, F_OK) != -1) {
			fprintf(stderr, ERR_PREFIX "%s already exists.\n", dmfile);
			exit(EXIT_NO_OVERWRITE);
		}

		if (dmdoc) {
			xmlSaveFormatFile(dmfile, dmdoc, 1);
		} else { /* Copy non-XML file */
			copy(cpfile, dmfile);
		}

		if (verbose) {
			puts(dmfile);
		}

		if (dmdoc) {
			xmlFreeDoc(dmdoc);
		}
	}

	xmlCleanupParser();

	return 0;
}
