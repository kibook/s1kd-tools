/* Usage:
 * 	upissue [-vI] [-f] DATAMODULE
 *
 * 	-v	Print filename of upissued datamodule
 * 	-I	Create a new issue of the datamodule
 * 	-f	specify the datamodule file to upissue
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <libxml/tree.h>

void show_help(void)
{
	puts("Usage: upissue [-vI] DATAMODULE");
	putchar('\n');
	puts("Options:");
	puts("  -v	Print filename of upissued data module");
	puts("  -I	Create a new issue of the data module");
	puts("  -N	Omit issue/inwork numbers from filename");
}

xmlNode *find_child(xmlNode *parent, const char *name)
{
	xmlNode *cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, name) == 0) {
			return cur;
		}
	}

	return NULL;
}

int main(int argc, char **argv)
{
	char dmfile[256];
	int dmfilelen;
	xmlDocPtr dmdoc;

	xmlNode *dmodule;
	xmlNode *identAndStatusSection;
	xmlNode *dmAddress;
	xmlNode *dmIdent;
	xmlNode *issueInfo;
	xmlNode *dmAddressItems;
	xmlNode *issueDate;
	xmlNode *dmStatus;

	char *issueNumber;
	char *inWork;

	char upissued_issueNumber[32];
	char upissued_inWork[32];

	int issueNumber_int;
	int inWork_int;

	int c;

	bool verbose;
	bool newissue;

	int i;

	time_t now;
	struct tm *local;
	int year, month, day;
	char year_s[5], month_s[3], day_s[3];

	dmfilelen = 0;
	verbose = false;
	newissue = false;

	char status[32] = "changed";

	bool no_issue = false;

	while ((c = getopt(argc, argv, "Ivs:Nh?")) != -1) {
		switch (c) {
			case 'I':
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
				break;
			case 'h':
			case '?':
				show_help();
				exit(0);
		}
	}

	for (i = optind; i < argc; i++) {
		strcpy(dmfile, argv[i]);
		dmfilelen = strlen(dmfile);

		if (!dmfilelen) {
			fputs("ERROR: No file specified\n", stderr);
			exit(1);
		}

		dmdoc = xmlReadFile(dmfile, NULL, XML_PARSE_NONET);

		dmodule = xmlDocGetRootElement(dmdoc);
		identAndStatusSection = find_child(dmodule, "identAndStatusSection");
		dmAddress = find_child(identAndStatusSection, "dmAddress");
		dmIdent = find_child(dmAddress, "dmIdent");
		issueInfo = find_child(dmIdent, "issueInfo");

		issueNumber = (char *) xmlGetProp(issueInfo, (xmlChar *) "issueNumber");
		inWork = (char *) xmlGetProp(issueInfo, (xmlChar *) "inWork");

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

		xmlSetProp(issueInfo, (xmlChar *) "issueNumber", (xmlChar *) upissued_issueNumber);
		xmlSetProp(issueInfo, (xmlChar *) "inWork",      (xmlChar *) upissued_inWork);

		if (newissue) {
			dmAddressItems = find_child(dmAddress, "dmAddressItems");
			issueDate = find_child(dmAddressItems, "issueDate");

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

			dmStatus = find_child(identAndStatusSection, "dmStatus");
			xmlSetProp(dmStatus, (xmlChar *) "issueType", (xmlChar *) status);
		}

		if (!no_issue) {
			strncpy(dmfile + (dmfilelen - 16), upissued_issueNumber, 3);
			strncpy(dmfile + (dmfilelen - 12), upissued_inWork, 2);
		}

		xmlSaveFormatFile(dmfile, dmdoc, 1);

		if (verbose) {
			puts(dmfile);
		}

		xmlFreeDoc(dmdoc);
	}

	xmlCleanupParser();

	return 0;
}
