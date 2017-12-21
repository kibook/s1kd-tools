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
	puts("Usage: " PROG_NAME " [-viNrRqI] [-s <status>] [-1 <type>] [-2 <type>] <datamodules>");
	putchar('\n');
	puts("Options:");
	puts("  -v           Print filename of upissued data module");
	puts("  -i           Create a new issue of the data module");
	puts("  -N           Omit issue/inwork numbers from filename");
	puts("  -s <status>  Set change status type");
	puts("  -r           Keep RFUs from old issue");
	puts("  -R           Only delete change marks associated with an RFU");
	puts("  -q           Keep quality assurance from old issue");
	puts("  -I           Do not change issue date");
	puts("  -1 <type>    Set first verification type");
	puts("  -2 <type>    Set second verification type");
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

	if (iss30) {
		obj = xmlXPathEvalExpression(BAD_CAST "//*[@change or @mark or @rfc or @level]", ctx);
	} else {
		obj = xmlXPathEvalExpression(BAD_CAST "//*[@changeType or @changeMark or @reasonForUpdateRefIds]", ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		if (iss30) {
			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "change");
				xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "mark");
				xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "rfc");
				xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "level");
			}
		} else {
			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "changeType");
				xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "changeMark");
				xmlUnsetProp(obj->nodesetval->nodeTab[i], BAD_CAST "reasonForUpdateRefIds");
			}
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
			}
		} else {
			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				xmlUnlinkNode(obj->nodesetval->nodeTab[i]);
				xmlFreeNode(obj->nodesetval->nodeTab[i]);
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
	char *status = NULL;
	bool no_issue = false;
	bool keep_rfus = false;
	bool set_date = true;
	bool only_assoc_rfus = false;
	bool set_unverif = true;

	xmlChar *issno_name, *inwork_name;
	bool iss30 = false;

	char *firstver = NULL;
	char *secondver = NULL;

	while ((c = getopt(argc, argv, "ivs:NfrRIq1:2:h?")) != -1) {
		switch (c) {
			case 'i':
				newissue = true;
				break;
			case 'v':
				verbose = true;
				break;
			case 's':
				status = strdup(optarg);
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
			case 'R':
				only_assoc_rfus = true;
				break;
			case 'I':
				set_date = false;
				break;
			case 'q':
				set_unverif = false;
				break;
			case '1':
				firstver = strdup(optarg);
				break;
			case '2':
				secondver = strdup(optarg);
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
			issueInfo = firstXPathNode("//issueInfo|//issno", dmdoc);
		} else {
			issueInfo = NULL;
		}

		if (!issueInfo && no_issue) {
			fprintf(stderr, ERR_PREFIX "Cannot use -N when file does not contain issue info metadata.\n");
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

			/* Default status is "new" before issue 1, and "changed" after */
			if (issueNumber_int < 1 && !status) {
				status = "new";
			} else if (!status) {
				status = "changed";
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

	free(firstver);
	free(secondver);

	xmlCleanupParser();

	return 0;
}
