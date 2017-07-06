#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#define COL_ISSDATE	0x001
#define COL_RPC		0x002
#define COL_ORIG	0x004
#define COL_TECH	0x008
#define COL_INFO	0x010
#define COL_APPLIC	0x020
#define COL_STITLE	0x040
#define COL_FNAME	0x080
#define COL_CODE	0x100
#define COL_ISSUE	0x200
#define COL_LANG	0x400

#define COL_TITLE	(COL_TECH | COL_INFO)
#define COL_ALL		(COL_ISSDATE | COL_RPC | COL_ORIG | COL_TECH | COL_INFO | COL_APPLIC | COL_FNAME | COL_CODE | COL_ISSUE | COL_LANG)

#define DM_MAX 5120

#define ERR_PREFIX "s1kd-dmls: ERROR: "

#define EXIT_DM_MAX 1
#define EXIT_BAD_XML 2

#define SHOW_DM 0x1
#define SHOW_PM 0x2

int clean_unprintable = 1;

struct dmident {
	bool extended;
	char *extensionProducer;
	char *extensionCode;
	char *modelIdentCode;
	char *systemDiffCode;
	char *systemCode;
	char *subSystemCode;
	char *subSubSystemCode;
	char *assyCode;
	char *disassyCode;
	char *disassyCodeVariant;
	char *infoCode;
	char *infoCodeVariant;
	char *itemLocationCode;
	char *learnCode;
	char *learnEventCode;
	char *issueNumber;
	char *inWork;
	char *languageIsoCode;
	char *countryIsoCode;
};

xmlNodePtr find_child(xmlNodePtr parent, const char *childname)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, childname) == 0) {
			return cur;
		}
	}

	return NULL;
}

xmlNodePtr find_req_child(xmlNodePtr parent, const char *name, const char *fname)
{
	xmlNodePtr child;

	if (!(child = find_child(parent, name))) {
		fprintf(stderr, ERR_PREFIX "Element %s missing child element %s (%s).\n", (char *) parent->name, name, fname);
	}

	return child;
}

bool init_ident(struct dmident *ident, xmlDocPtr dm, const char *fname)
{
	xmlNodePtr dmodule;
	xmlNodePtr identAndStatusSection;
	xmlNodePtr dmAddress;
	xmlNodePtr dmIdent;
	xmlNodePtr identExtension;
	xmlNodePtr dmCode;
	xmlNodePtr language;
	xmlNodePtr issueInfo;

	dmodule = xmlDocGetRootElement(dm);
	if (!(identAndStatusSection = find_req_child(dmodule, "identAndStatusSection", fname))) return false;
	if (!(dmAddress = find_req_child(identAndStatusSection, "dmAddress", fname))) return false;
	if (!(dmIdent = find_req_child(dmAddress, "dmIdent", fname))) return false;
	identExtension = find_child(dmIdent, "identExtension");
	if (!(dmCode = find_req_child(dmIdent, "dmCode", fname))) return false;
	if (!(language = find_req_child(dmIdent, "language", fname))) return false;
	if (!(issueInfo = find_req_child(dmIdent, "issueInfo", fname))) return false;

	ident->modelIdentCode     = (char *) xmlGetProp(dmCode, BAD_CAST "modelIdentCode");
	ident->systemDiffCode     = (char *) xmlGetProp(dmCode, BAD_CAST "systemDiffCode");
	ident->systemCode         = (char *) xmlGetProp(dmCode, BAD_CAST "systemCode");
	ident->subSystemCode      = (char *) xmlGetProp(dmCode, BAD_CAST "subSystemCode");
	ident->subSubSystemCode   = (char *) xmlGetProp(dmCode, BAD_CAST "subSubSystemCode");
	ident->assyCode           = (char *) xmlGetProp(dmCode, BAD_CAST "assyCode");
	ident->disassyCode        = (char *) xmlGetProp(dmCode, BAD_CAST "disassyCode");
	ident->disassyCodeVariant = (char *) xmlGetProp(dmCode, BAD_CAST "disassyCodeVariant");
	ident->infoCode           = (char *) xmlGetProp(dmCode, BAD_CAST "infoCode");
	ident->infoCodeVariant    = (char *) xmlGetProp(dmCode, BAD_CAST "infoCodeVariant");
	ident->itemLocationCode   = (char *) xmlGetProp(dmCode, BAD_CAST "itemLocationCode");
	ident->learnCode          = (char *) xmlGetProp(dmCode, BAD_CAST "learnCode");
	ident->learnEventCode     = (char *) xmlGetProp(dmCode, BAD_CAST "learnEventCode");

	ident->issueNumber = (char *) xmlGetProp(issueInfo, BAD_CAST "issueNumber");
	ident->inWork      = (char *) xmlGetProp(issueInfo, BAD_CAST "inWork");

	ident->languageIsoCode = (char *) xmlGetProp(language, BAD_CAST "languageIsoCode");
	ident->countryIsoCode  = (char *) xmlGetProp(language, BAD_CAST "countryIsoCode");

	if (identExtension) {
		ident->extended = true;
		ident->extensionProducer = (char *) xmlGetProp(identExtension, BAD_CAST "extensionProducer");
		ident->extensionCode     = (char *) xmlGetProp(identExtension, BAD_CAST "extensionCode");
	} else {
		ident->extended = false;
	}

	return true;
}

void free_ident(struct dmident *ident)
{
	if (ident->extended) {
		xmlFree(ident->extensionProducer);
		xmlFree(ident->extensionCode);
	}
	xmlFree(ident->modelIdentCode);
	xmlFree(ident->systemDiffCode);
	xmlFree(ident->systemCode);
	xmlFree(ident->subSystemCode);
	xmlFree(ident->subSubSystemCode);
	xmlFree(ident->assyCode);
	xmlFree(ident->disassyCode);
	xmlFree(ident->disassyCodeVariant);
	xmlFree(ident->infoCode);
	xmlFree(ident->infoCodeVariant);
	xmlFree(ident->itemLocationCode);
	xmlFree(ident->learnCode);
	xmlFree(ident->learnEventCode);
	xmlFree(ident->issueNumber);
	xmlFree(ident->inWork);
	xmlFree(ident->languageIsoCode);
	xmlFree(ident->countryIsoCode);
}

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

void clean(char *s)
{
	int i;

	for (i = 0; s[i]; ++i) {
		switch (s[i]) {
			case '\n':
			case '\t':
				s[i] = ' ';
		}
	}
}

void printdms(char dms[DM_MAX][256], int n, int columns)
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

	struct dmident ident;

	for (i = 0; i < n; ++i) {
		char *tech, *info;

		#ifndef _WIN32
		char real[PATH_MAX];
		#endif

		#ifdef _WIN32
		dm = xmlReadFile(dms[i], NULL, 0);
		#else
		if (readlink(dms[i], real, PATH_MAX) != -1) {
			dm = xmlReadFile(real, NULL, 0);
		} else {
			dm = xmlReadFile(dms[i], NULL, 0);
		}
		#endif

		if (!dm) continue;

		if (!init_ident(&ident, dm, dms[i])) continue;

		dmodule = xmlDocGetRootElement(dm);
		if (!(identAndStatusSection = find_req_child(dmodule, "identAndStatusSection", dms[i]))) continue;
		if (!(dmAddress = find_req_child(identAndStatusSection, "dmAddress", dms[i]))) continue;
		if (!(dmAddressItems = find_req_child(dmAddress, "dmAddressItems", dms[i]))) continue;
		if (!(dmTitle = find_req_child(dmAddressItems, "dmTitle", dms[i]))) continue;
		if (!(techName = find_req_child(dmTitle, "techName", dms[i]))) continue;
		infoName = find_child(dmTitle, "infoName");

		if (!(dmStatus = find_req_child(identAndStatusSection, "dmStatus", dms[i]))) continue;

		tech = (char *) xmlNodeGetContent(techName);
		info = (char *) xmlNodeGetContent(infoName);

		if (clean_unprintable) clean(tech);
		if (clean_unprintable && info) clean(info);

		if ((columns & COL_FNAME) == COL_FNAME) {
			printf("%s	", dms[i]);
		}

		if ((columns & COL_CODE) == COL_CODE) {
			if (ident.extended) {
				printf("%s-%s-", ident.extensionProducer, ident.extensionCode);
			}

			printf("%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
				ident.modelIdentCode,
				ident.systemDiffCode,
				ident.systemCode,
				ident.subSystemCode, ident.subSubSystemCode,
				ident.assyCode,
				ident.disassyCode, ident.disassyCodeVariant,
				ident.infoCode, ident.infoCodeVariant,
				ident.itemLocationCode);

			if (ident.learnCode && ident.learnEventCode) {
				printf("-%s%s", ident.learnCode, ident.learnEventCode);
			}

			printf("	");
		}

		if ((columns & COL_LANG) == COL_LANG) {
			printf("%s-%s	", ident.languageIsoCode, ident.countryIsoCode);
		}

		if ((columns & COL_ISSUE) == COL_ISSUE) {
			printf("%s-%s	", ident.issueNumber, ident.inWork);
		}

		if ((columns & COL_ISSDATE) == COL_ISSDATE) {
			issueDate = find_child(dmAddressItems, "issueDate");
			year  = (char *) xmlGetProp(issueDate, (xmlChar *) "year");
			month = (char *) xmlGetProp(issueDate, (xmlChar *) "month");
			day   = (char *) xmlGetProp(issueDate, (xmlChar *) "day");

			printf("%s-%s-%s	", year, month, day);

			xmlFree(year);
			xmlFree(month);
			xmlFree(day);
		}

		if ((columns & COL_STITLE) == COL_STITLE) {
			printf("%s", tech);

			if (infoName) {
				printf(" - %s", info);
			}

			putchar('\t');
		} else {
			if ((columns & COL_TECH) == COL_TECH) {
				printf("%s	", tech);
			}
			if ((columns & COL_INFO) == COL_INFO) {
				printf("%s	", infoName ? info : " ");
			}
		}

		if ((columns & COL_RPC) == COL_RPC) {
			responsiblePartnerCompany = find_child(dmStatus, "responsiblePartnerCompany");
			enterpriseName = find_child(responsiblePartnerCompany, "enterpriseName");

			name = (char *) xmlNodeGetContent(enterpriseName);

			printf("%s	", name);

			xmlFree(name);
		}

		if ((columns & COL_ORIG) == COL_ORIG) {
			originator = find_child(dmStatus, "originator");
			enterpriseName = find_child(originator, "enterpriseName");

			name = (char *) xmlNodeGetContent(enterpriseName);

			printf("%s	", name);

			xmlFree(name);
		}

		if ((columns & COL_APPLIC) == COL_APPLIC) {
			applic = find_child(dmStatus, "applic");
			displayText = find_child(applic, "displayText");
			simplePara = find_child(displayText, "simplePara");

			display = (char *) xmlNodeGetContent(simplePara);

			printf("%s	", display);

			xmlFree(display);
		}

		printf("\n");

		free_ident(&ident);
		
		xmlFree(tech);
		xmlFree(info);

		xmlFreeDoc(dm);
	}
}

void printpms(char pms[DM_MAX][256], int n, int columns)
{
	int i;
	xmlDocPtr pm_doc;
	xmlNodePtr pm;
	xmlNodePtr identAndStatusSection;
	xmlNodePtr pmAddress;
	xmlNodePtr pmIdent;
	xmlNodePtr pmAddressItems;
	xmlNodePtr pmTitle;
	xmlNodePtr pmCode;
	xmlNodePtr language;
	xmlNodePtr issueInfo;
	xmlNodePtr issueDate;

	for (i = 0; i < n; ++i) {
		char *title;

		pm_doc = xmlReadFile(pms[i], NULL, 0);

		pm = xmlDocGetRootElement(pm_doc);
		if (!(identAndStatusSection = find_req_child(pm, "identAndStatusSection", pms[i]))) continue;
		if (!(pmAddress = find_req_child(identAndStatusSection, "pmAddress", pms[i]))) continue;
		if (!(pmIdent = find_req_child(pmAddress, "pmIdent", pms[i]))) continue;
		if (!(pmAddressItems = find_req_child(pmAddress, "pmAddressItems", pms[i]))) continue;
		if (!(pmTitle = find_req_child(pmAddressItems, "pmTitle", pms[i]))) continue;
		if (!(pmCode = find_req_child(pmIdent, "pmCode", pms[i]))) continue;
		if (!(language = find_req_child(pmIdent, "language", pms[i]))) continue;
		if (!(issueInfo = find_req_child(pmIdent, "issueInfo", pms[i]))) continue;
		if (!(issueDate = find_req_child(pmAddressItems, "issueDate", pms[i]))) continue;

		title = (char *) xmlNodeGetContent(pmTitle);

		if (clean_unprintable) clean(title);

		if ((columns & COL_FNAME) == COL_FNAME) {
			printf("%s	", pms[i]);
		}

		if ((columns & COL_CODE) == COL_CODE) {
			char *modelIdentCode;
			char *pmIssuer;
			char *pmNumber;
			char *pmVolume;

			modelIdentCode = (char *) xmlGetProp(pmCode, BAD_CAST "modelIdentCode");
			pmIssuer       = (char *) xmlGetProp(pmCode, BAD_CAST "pmIssuer");
			pmNumber       = (char *) xmlGetProp(pmCode, BAD_CAST "pmNumber");
			pmVolume       = (char *) xmlGetProp(pmCode, BAD_CAST "pmVolume");

			printf("%s-%s-%s-%s	", modelIdentCode, pmIssuer, pmNumber, pmVolume);

			xmlFree(modelIdentCode);
			xmlFree(pmIssuer);
			xmlFree(pmNumber);
			xmlFree(pmVolume);
		}

		if ((columns & COL_LANG) == COL_LANG) {
			char *languageIsoCode;
			char *countryIsoCode;

			languageIsoCode = (char *) xmlGetProp(language, BAD_CAST "languageIsoCode");
			countryIsoCode  = (char *) xmlGetProp(language, BAD_CAST "countryIsoCode");

			printf("%s-%s	", languageIsoCode, countryIsoCode);

			xmlFree(languageIsoCode);
			xmlFree(countryIsoCode);
		}

		if ((columns & COL_ISSUE) == COL_ISSUE) {
			char *issueNumber;
			char *inWork;

			issueNumber = (char *) xmlGetProp(issueInfo, BAD_CAST "issueNumber");
			inWork      = (char *) xmlGetProp(issueInfo, BAD_CAST "inWork");

			printf("%s-%s	", issueNumber, inWork);

			xmlFree(issueNumber);
			xmlFree(inWork);
		}

		if ((columns & COL_ISSDATE) == COL_ISSDATE) {
			char *year, *month, *day;

			year  = (char *) xmlGetProp(issueDate, BAD_CAST "year");
			month = (char *) xmlGetProp(issueDate, BAD_CAST "month");
			day   = (char *) xmlGetProp(issueDate, BAD_CAST "day");

			printf("%s-%s-%s	", year, month, day);

			xmlFree(year);
			xmlFree(month);
			xmlFree(day);
		}

		if (((columns & COL_TITLE) == COL_TITLE) || ((columns & COL_STITLE) == COL_STITLE)) {
			printf("%s	", title);
		}

		printf("\n");

		xmlFree(title);

		xmlFreeDoc(pm_doc);
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
	puts("Usage: s1kd-dmls [-acfHhilorRTt]");
	puts("");
	puts("Options:");
	puts("  -l	Show only latest issue/inwork version");
	puts("  -I	Show only official issues");
	puts("  -f	Do not show filename");
	puts("  -c	Show data module code");
	puts("  -n	Show issue info");
	puts("  -L	Show language info");
	puts("  -t	Show tech and info name columns");
	puts("  -T	Show single title column");
	puts("  -i	Include issue date column");
	puts("  -r	Include responsible partner company column");
	puts("  -o	Include originator column");
	puts("  -a	Include applicability column");
	puts("  -H	Show headers on columns");
	puts("  -w	Show only writable data module files");
	puts("  -R	Recursively search directories");
	puts("  -p	Print control characters");
	puts("  -D	List data modules only");
	puts("  -P	List pub modules only");
	puts("  -h	Show this help message");
}

int is_directory(const char *path, int recursive)
{
	struct stat st;

	char scratch[PATH_MAX];
	char *base;

	strcpy(scratch, path);
	base = basename(scratch);

	/* Do not recurse in to current or parent directory */
	if (recursive && (strcmp(base, ".") == 0 || strcmp(base, "..") == 0))
		return 0;

	stat(path, &st);
	return S_ISDIR(st.st_mode);
}

void list_dir(const char *path, char dms[DM_MAX][256], int *ndms, char pms[DM_MAX][256], int *npms, int only_writable, int recursive)
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
		strcpy(cpath, fpath);
		strcat(cpath, cur->d_name);

		if (access(cpath, R_OK) != 0)
			continue;
		else if (only_writable && access(cur->d_name, W_OK) != 0)
			continue;
		else if (isdm(cur->d_name)) {
			if (*ndms == DM_MAX) {
				fprintf(stderr, ERR_PREFIX "Maximum data modules reached (%d).\n", DM_MAX);
				exit(EXIT_DM_MAX);
			}
			strcpy(dms[(*ndms)++], cpath);
		} else if (ispm(cur->d_name)) {
			if (*npms == DM_MAX) {
				fprintf(stderr, ERR_PREFIX "Maximum pub modules reached (%d).\n", DM_MAX);
				exit(EXIT_DM_MAX);
			}
			strcpy(pms[(*npms)++], cpath);
		} else if (recursive && is_directory(cpath, recursive)) {
			list_dir(cpath, dms, ndms, pms, npms, only_writable, recursive);
		}
	}

	closedir(dir);
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

	char (*dms)[256] = malloc(DM_MAX * 256);
	char (*pms)[256] = malloc(DM_MAX * 256);
	int ndms;
	int npms;

	int i;

	int c;
	int only_latest = 0;
	int only_official_issue = 0;
	int only_writable = 0;

	char (*latest_dms)[256] = malloc(DM_MAX * 256);
	int nlatest_dms;

	char (*issue_dms)[256] = malloc(DM_MAX * 256);
	int nissue_dms;

	int recursive = 0;
	int show = 0;

	int columns = COL_FNAME;
	int header = 0;

	while ((c = getopt(argc, argv, "fclItTiroaAHwRnLpDPh?")) != -1) {
		switch (c) {
			case 'l': only_latest = 1; break;
			case 'I': only_official_issue = 1; break;
			case 'f': columns &= ~COL_FNAME; break;
			case 'c': columns |= COL_CODE; break;
			case 'n': columns |= COL_ISSUE; break;
			case 'L': columns |= COL_LANG; break;
			case 't': columns |= COL_TITLE; break;
			case 'T': columns |= COL_STITLE; break;
			case 'p': clean_unprintable = 0; break;
			case 'i': columns |= COL_ISSDATE; break;
			case 'r': columns |= COL_RPC; break;
			case 'o': columns |= COL_ORIG; break;
			case 'a': columns |= COL_APPLIC; break;
			case 'A': columns |= COL_ALL; break;
			case 'H': header = 1; break;
			case 'w': only_writable = 1; break;
			case 'R': recursive = 1; break;
			case 'D': show = SHOW_DM; break;
			case 'P': show = SHOW_PM; break;
			case 'h':
			case '?': show_help();
				  exit(0);
		}
	}

	if (!columns) exit(0);

	if (!show) show = SHOW_DM | SHOW_PM;

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
				if (ndms == DM_MAX) {
					fprintf(stderr, ERR_PREFIX "Maximum data modules reached (%d).\n", DM_MAX);
					exit(EXIT_DM_MAX);
				}
				strcpy(dms[ndms++], argv[i]);
			} else if (ispm(base)) {
				if (npms == DM_MAX) {
					fprintf(stderr, ERR_PREFIX "Maximum pub modules reached (%d).\n", DM_MAX);
					exit(EXIT_DM_MAX);
				}
				strcpy(pms[npms++], argv[i]);
			} else if (is_directory(argv[i], recursive)) {
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

	if (header) {
		if ((columns & COL_FNAME) == COL_FNAME) printf("FILENAME	");
		if ((columns & COL_CODE) == COL_CODE) printf("CODE	");
		if ((columns & COL_LANG) == COL_LANG) printf("LANG	");
		if ((columns & COL_ISSUE) == COL_ISSUE) printf("ISSUE	");
		if ((columns & COL_ISSDATE) == COL_ISSDATE) printf("DATE	");
		if ((columns & COL_STITLE) == COL_STITLE) {
			printf("TITLE	");
		} else {
			if ((columns & COL_TECH) == COL_TECH) printf("TECH NAME/TITLE	");
			if ((columns & COL_INFO) == COL_INFO) printf("INFO NAME	");
		}
		if ((columns & COL_RPC) == COL_RPC) printf("RPC	");
		if ((columns & COL_ORIG) == COL_ORIG) printf("ORIG	");
		if ((columns & COL_APPLIC) == COL_APPLIC) printf("APPLIC	");
		printf("\n");
	}

	if ((show & SHOW_DM) == SHOW_DM) {
		if (only_latest) {
			printdms(latest_dms, nlatest_dms, columns);
		} else if (only_official_issue) {
			printdms(issue_dms, nissue_dms, columns);
		} else {
			printdms(dms, ndms, columns);
		}
	}

	if ((show & SHOW_PM) == SHOW_PM) {
		printpms(pms, npms, columns);
	}

	if (dir) {
		closedir(dir);

	}

	free(dms);
	free(pms);
	free(latest_dms);
	free(issue_dms);

	xmlCleanupParser();

	return 0;
}
