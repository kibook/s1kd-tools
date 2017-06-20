#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define COL_ISSDATE	0x01
#define COL_RPC		0x02
#define COL_ORIG	0x04
#define COL_TECH	0x08
#define COL_INFO	0x10
#define COL_APPLIC	0x20

#define COL_TITLE	(COL_TECH | COL_INFO)
#define COL_ALL		(COL_ISSDATE | COL_RPC | COL_ORIG | COL_TECH | COL_INFO | COL_APPLIC)

xmlNodePtr getElementByName(xmlNodePtr root, const char *name)
{
	xmlNodePtr cur;

	for (cur = root->children; cur; cur = cur->next) {
		xmlNodePtr elem;

		if (strcmp((char *) cur->name, name) == 0) {
			return cur;
		} else if ((elem = getElementByName(cur, name))) {
			return elem;
		}
	}

	return NULL;
}

xmlNodePtr findChild(xmlNodePtr parent, const char *childname)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, childname) == 0) {
			return cur;
		}
	}

	return NULL;
}

void printdms(char dms[1024][256], int n, int columns, int header)
{
	int i;

	xmlDocPtr dm;
	xmlNodePtr dmodule;
	xmlNodePtr identAndStatusSection;
	xmlNodePtr dmAddress;
	xmlNodePtr dmAddressItems;
	xmlNodePtr dmTitle;
	xmlNodePtr techName;
	xmlNodePtr infoName;
	xmlNodePtr issueDate;
	xmlNodePtr dmStatus;
	xmlNodePtr responsiblePartnerCompany;
	xmlNodePtr originator;
	xmlNodePtr enterpriseName;
	xmlNodePtr applic;
	xmlNodePtr displayText;
	xmlNodePtr simplePara;

	char *year;
	char *month;
	char *day;

	char *name;

	char *display;

	if (header) {
		printf("FILENAME");
		if ((columns & COL_ISSDATE) == COL_ISSDATE) printf("	DATE");
		if ((columns & COL_TECH) == COL_TECH) printf("	TECH NAME");
		if ((columns & COL_INFO) == COL_INFO) printf("	INFO NAME");
		if ((columns & COL_RPC) == COL_RPC) printf("	RPC");
		if ((columns & COL_ORIG) == COL_ORIG) printf("	ORIG");
		if ((columns & COL_APPLIC) == COL_APPLIC) printf("	APPLIC");
		printf("\n");
	}

	for (i = 0; i < n; ++i) {
		char *tech, *info;
		char real[PATH_MAX];

		if (readlink(dms[i], real, PATH_MAX) != -1) {
			dm = xmlReadFile(real, NULL, 0);
		} else {
			dm = xmlReadFile(dms[i], NULL, 0);
		}

		dmodule = xmlDocGetRootElement(dm);
		identAndStatusSection = findChild(dmodule, "identAndStatusSection");
		dmAddress = findChild(identAndStatusSection, "dmAddress");
		dmAddressItems = findChild(dmAddress, "dmAddressItems");
		dmTitle = findChild(dmAddressItems, "dmTitle");
		techName = findChild(dmTitle, "techName");
		infoName = findChild(dmTitle, "infoName");

		dmStatus = findChild(identAndStatusSection, "dmStatus");

		tech = (char *) xmlNodeGetContent(techName);
		info = (char *) xmlNodeGetContent(infoName);

		printf("%s", dms[i]);

		if ((columns & COL_ISSDATE) == COL_ISSDATE) {
			issueDate = findChild(dmAddressItems, "issueDate");
			year  = (char *) xmlGetProp(issueDate, (xmlChar *) "year");
			month = (char *) xmlGetProp(issueDate, (xmlChar *) "month");
			day   = (char *) xmlGetProp(issueDate, (xmlChar *) "day");

			printf("	%s-%s-%s", year, month, day);

			xmlFree(year);
			xmlFree(month);
			xmlFree(day);
		}

		if ((columns & COL_ISSDATE) == COL_ISSDATE) {
		}
		if ((columns & COL_TECH) == COL_TECH) {
			printf("	%s", tech);
		}
		if ((columns & COL_INFO) == COL_INFO) {
			printf("	%s", infoName ? info : "");
		}

		if ((columns & COL_RPC) == COL_RPC) {
			responsiblePartnerCompany = findChild(dmStatus, "responsiblePartnerCompany");
			enterpriseName = findChild(responsiblePartnerCompany, "enterpriseName");

			name = (char *) xmlNodeGetContent(enterpriseName);

			printf("	%s", name);

			xmlFree(name);
		}

		if ((columns & COL_ORIG) == COL_ORIG) {
			originator = findChild(dmStatus, "originator");
			enterpriseName = findChild(originator, "enterpriseName");

			name = (char *) xmlNodeGetContent(enterpriseName);

			printf("	%s", name);

			xmlFree(name);
		}

		if ((columns & COL_APPLIC) == COL_APPLIC) {
			applic = findChild(dmStatus, "applic");
			displayText = findChild(applic, "displayText");
			simplePara = findChild(displayText, "simplePara");

			display = (char *) xmlNodeGetContent(simplePara);

			printf("	%s", display);

			xmlFree(display);
		}

		printf("\n");
		
		xmlFree(tech);
		xmlFree(info);

		xmlFreeDoc(dm);
	}
}

void printpms(char pms[1024][256], int n, int columns)
{
	int i;
	xmlDocPtr pm_doc;
	xmlNodePtr pm;
	xmlNodePtr identAndStatusSection;
	xmlNodePtr pmAddress;
	xmlNodePtr pmAddressItems;
	xmlNodePtr pmTitle;

	for (i = 0; i < n; ++i) {
		char *title;

		pm_doc = xmlReadFile(pms[i], NULL, 0);

		pm = xmlDocGetRootElement(pm_doc);
		identAndStatusSection = findChild(pm, "identAndStatusSection");
		pmAddress = findChild(identAndStatusSection, "pmAddress");
		pmAddressItems = findChild(pmAddress, "pmAddressItems");
		pmTitle = findChild(pmAddressItems, "pmTitle");

		title = (char *) xmlNodeGetContent(pmTitle);

		printf("%s", pms[i]);
		if (columns & COL_TITLE) {
			printf("	%s", title);
		}
		printf("\n");

		xmlFree(title);
	}
}

int compare(const void *a, const void *b)
{
	return strcasecmp((const char *) a, (const char *) b);
}

int isdm(const char *name)
{
	return (strncmp(name, "DMC-", 4) == 0 || strncmp(name, "DME-", 4) == 0) && strncasecmp(name + strlen(name) - 4, ".XML", 4) == 0;
}

int ispm(const char *name)
{
	return (strncmp(name, "PMC-", 4) == 0 || strncmp(name, "PME-", 4) == 0) && strncasecmp(name + strlen(name) - 4, ".XML", 4) == 0;
}

void show_help(void)
{
	puts("Usage: s1kd-dmls [-aHhilor]");
	puts("");
	puts("Options:");
	puts("  -l	Show only latest issue/inwork version");
	puts("  -I      Show only official issues");
	puts("  -i	Include issue date column");
	puts("  -r	Include responsible partner company column");
	puts("  -o	Include originator column");
	puts("  -a	Include applicability column");
	puts("  -H	Show headers on columns");
	puts("  -w	Show only writable data module files.");
	puts("  -h	Show this help message");
}

int is_directory(const char *path)
{
	struct stat st;

	char scratch[PATH_MAX];
	char *base;

	strcpy(scratch, path);
	base = basename(scratch);

	/* Do not recurse in to current or parent directory */
	if (strcmp(base, ".") == 0 || strcmp(base, "..") == 0)
		return 0;

	stat(path, &st);
	return S_ISDIR(st.st_mode);
}

void list_dir(const char *path, char dms[1024][256], int *ndms, char pms[1024][256], int *npms, int only_writable, int recursive)
{
	DIR *dir;
	struct dirent *cur;

	int len = strlen(path);
	char fpath[PATH_MAX];
	char cpath[PATH_MAX];

	if (strcmp(path, ".") == 0) {
		strcpy(fpath, "");
	} else if (path[len - 1] != '/') {
		strcpy(fpath, path);
		strcat(fpath, "/");
	} else {
		strcpy(fpath, path);
	}

	dir = opendir(path);

	while ((cur = readdir(dir))) {
		if (access(cur->d_name, R_OK) != 0)
			continue;
		if (only_writable && access(cur->d_name, W_OK) != 0)
			continue;
		if (isdm(cur->d_name)) {
			strcpy(dms[*ndms], fpath);
			strcat(dms[*ndms], cur->d_name);
			(*ndms)++;
		} else if (ispm(cur->d_name)) {
			strcpy(pms[*npms], fpath);
			strcat(pms[*npms], cur->d_name);
			(*npms)++;
		} else if (recursive) {
			strcpy(cpath, fpath);
			strcat(cpath, cur->d_name);

			if (is_directory(cpath)) {
				list_dir(cpath, dms, ndms, pms, npms, only_writable, recursive);
			}
		}
	}
}

int is_official_issue(const char *fname)
{
	xmlDocPtr doc = xmlReadFile(fname, NULL, 0);
	xmlXPathContextPtr ctxt = xmlXPathNewContext(doc);
	xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST "string(//dmIdent/issueInfo/@inWork)", ctxt);
	int ret = strcmp((char *) result->stringval, "00") == 0;

	xmlXPathFreeObject(result);
	xmlXPathFreeContext(ctxt);
	xmlFreeDoc(doc);

	return ret;
}

int main(int argc, char **argv)
{
	DIR *dir = NULL;

	char dms[1024][256];
	char pms[1024][256];
	int ndms;
	int npms;
	int i;

	int c;
	int only_latest = 0;
	int only_official_issue = 0;
	int only_writable = 0;
	char latest_dms[1024][256];
	int nlatest_dms;
	char issue_dms[1024][256];
	int nissue_dms;
	int recursive = 0;

	int columns = 0;
	int header = 0;

	while ((c = getopt(argc, argv, "lItiroaAHwRh?")) != -1) {
		switch (c) {
			case 'l': only_latest = 1; break;
			case 'I': only_official_issue = 1; break;
			case 't': columns |= COL_TITLE; break;
			case 'i': columns |= COL_ISSDATE; break;
			case 'r': columns |= COL_RPC; break;
			case 'o': columns |= COL_ORIG; break;
			case 'a': columns |= COL_APPLIC; break;
			case 'A': columns |= COL_ALL; break;
			case 'H': header = 1; break;
			case 'w': only_writable = 1; break;
			case 'R': recursive = 1; break;
			case 'h':
			case '?': show_help();
				  exit(0);
		}
	}

	ndms = 0;
	npms = 0;
	nlatest_dms = 0;
	nissue_dms = 0;

	if (optind < argc) {
		/* Read dms to list from arguments */
		for (i = optind; i < argc; ++i) {
			char path[PATH_MAX], *base;

			strcpy(path, argv[i]);
			base = basename(path);

			if (access(argv[i], R_OK) != 0)
				continue;

			if (only_writable && access(argv[i], W_OK) != 0)
				continue;

			if (isdm(base)) {
				strcpy(dms[ndms++], argv[i]);
			} else if (ispm(base)) {
				strcpy(pms[npms++], argv[i]);
			} else if (recursive && is_directory(argv[i])) {
				list_dir(argv[i], dms, &ndms, pms, &npms, only_writable, recursive);
			}
		}
	} else {
		/* Read dms to list from current directory */
		list_dir(".", dms, &ndms, pms, &npms, only_writable, recursive);
	}

	qsort(dms, ndms, 256, compare);
	qsort(pms, npms, 256, compare);

	if (only_official_issue) {
		for (i = 0; i < ndms; ++i) {
			if (is_official_issue(dms[i])) {
				strcpy(issue_dms[nissue_dms++], dms[i]);
			}
		}

		if (only_latest) {
			for (i = 0; i < nissue_dms; ++i) {
				if (i == 0 || strncmp(issue_dms[i], issue_dms[i - 1], strchr(issue_dms[i], '_') - issue_dms[i]) != 0) {
					strcpy(latest_dms[nlatest_dms++], issue_dms[i]);
				} else {
					strcpy(latest_dms[nlatest_dms - 1], issue_dms[i]);
				}
			}
		}
	} else if (only_latest) {
		for (i = 0; i < ndms; ++i) {
			if (i == 0 || strncmp(dms[i], dms[i - 1], strchr(dms[i], '_') - dms[i]) != 0) {
				strcpy(latest_dms[nlatest_dms++], dms[i]);
			} else {
				strcpy(latest_dms[nlatest_dms - 1], dms[i]);
			}
		}
	}

	if (only_latest) {
		printdms(latest_dms, nlatest_dms, columns, header);
	} else if (only_official_issue) {
		printdms(issue_dms, nissue_dms, columns, header);
	} else {
		printdms(dms, ndms, columns, header);
	}

	printpms(pms, npms, columns);

	if (dir) {
		closedir(dir);

	}

	xmlCleanupParser();

	return 0;
}
