#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>

/* Maximum number of CSDB objects of each type. */
#define OBJECT_MAX 10240

#define PROG_NAME "s1kd-ls"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_OBJECT_MAX 1
#define EXIT_BAD_XML 2

#define S_MAX_DM ERR_PREFIX "Maximum DMs reached (%d).\n"
#define S_MAX_PM ERR_PREFIX "Maximum PMs reached (%d).\n"
#define S_MAX_COM ERR_PREFIX "Maximum comments reached (%d).\n"
#define S_MAX_IMF ERR_PREFIX "Maximum IMFs reached (%d).\n"
#define S_MAX_DDN ERR_PREFIX "Maximum DDNs reached (%d).\n"
#define S_MAX_DML ERR_PREFIX "Maximum DMLs reached (%d).\n"

/* Set of CSDB object types to list. */
#define SHOW_DM  0x01
#define SHOW_PM  0x02
#define SHOW_COM 0x04
#define SHOW_IMF 0x08
#define SHOW_DDN 0x10
#define SHOW_DML 0x20

void printfiles(char files[OBJECT_MAX][PATH_MAX], int n)
{
	int i;
	for (i = 0; i < n; ++i) {
		printf("%s\n", files[i]);
	}
}

int compare(const void *a, const void *b)
{
	return strcasecmp((const char *) a, (const char *) b);
}

int isxml(const char *name)
{
	return strncasecmp(name + strlen(name) - 4, ".XML", 4) == 0;
}

int isdm(const char *name)
{
	return (strncmp(name, "DMC-", 4) == 0 || strncmp(name, "DME-", 4) == 0) && isxml(name);
}

int ispm(const char *name)
{
	return (strncmp(name, "PMC-", 4) == 0 || strncmp(name, "PME-", 4) == 0) && isxml(name);
}

int iscom(const char *name)
{
	return strncmp(name, "COM-", 4) == 0 && isxml(name);
}

int isimf(const char *name)
{
	return strncmp(name, "IMF-", 4) == 0 && isxml(name);
}

int isddn(const char *name)
{
	return strncmp(name, "DDN-", 4) == 0 && isxml(name);
}

int isdml(const char *name)
{
	return strncmp(name, "DML-", 4) == 0 && isxml(name);
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-CDiLlMPrwX]");
	puts("");
	puts("Options:");
	puts("  -C     List comments");
	puts("  -D     List data modules");
	puts("  -i     Show only official issues");
	puts("  -L     List DMLs");
	puts("  -l     Show only latest issue/inwork version");
	puts("  -M     List ICN metadata files");
	puts("  -P     List publication modules");
	puts("  -r     Recursively search directories");
	puts("  -w     Show only writable data module files");
	puts("  -X     List DDNs");
	puts("  -h -?  Show this help message");
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

/* Find CSDB objects in a given directory. */
void list_dir(const char *path,
              char dms[OBJECT_MAX][PATH_MAX], int *ndms,
              char pms[OBJECT_MAX][PATH_MAX], int *npms,
	      char coms[OBJECT_MAX][PATH_MAX], int *ncoms,
	      char imfs[OBJECT_MAX][PATH_MAX], int *nimfs,
	      char ddns[OBJECT_MAX][PATH_MAX], int *nddns,
	      char dmls[OBJECT_MAX][PATH_MAX], int *ndmls,
	      int only_writable, int recursive)
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
			if (*ndms == OBJECT_MAX) {
				fprintf(stderr, S_MAX_DM, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(dms[(*ndms)++], cpath);
		} else if (ispm(cur->d_name)) {
			if (*npms == OBJECT_MAX) {
				fprintf(stderr, S_MAX_PM, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(pms[(*npms)++], cpath);
		} else if (iscom(cur->d_name)) {
			if (*ncoms == OBJECT_MAX) {
				fprintf(stderr, S_MAX_COM, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(coms[(*ncoms)++], cpath);
		} else if (isimf(cur->d_name)) {
			if (*nimfs == OBJECT_MAX) {
				fprintf(stderr, S_MAX_IMF, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(imfs[(*nimfs)++], cpath);
		} else if (isddn(cur->d_name)) {
			if (*nddns == OBJECT_MAX) {
				fprintf(stderr, S_MAX_DDN, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(ddns[(*nddns)++], cpath);
		} else if (isdml(cur->d_name)) {
			if (*ndmls == OBJECT_MAX) {
				fprintf(stderr, S_MAX_DML, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(dmls[(*ndmls)++], cpath);
		} else if (recursive && is_directory(cpath, recursive)) {
			list_dir(cpath,
				dms, ndms,
				pms, npms,
				coms, ncoms,
				imfs, nimfs,
				ddns, nddns,
				dmls, ndmls,
				only_writable, recursive);
		}
	}

	closedir(dir);
}

/* Checks if a CSDB object is in the official state (inwork = 00). */
int is_official_issue(const char *fname)
{
	char inwork[3] = "";
	sscanf(fname, "%*[^_]_%*[^-]-%2s", inwork);
	return strcmp(inwork, "00") == 0;
}

/* Copy only the latest issues of CSDB objects. */
int extract_latest(char latest[OBJECT_MAX][PATH_MAX], char files[OBJECT_MAX][PATH_MAX], int nfiles)
{
	int i, nlatest = 0;
	for (i = 0; i < nfiles; ++i) {
		if (i == 0 || strncmp(files[i], files[i - 1], strchr(files[i], '_') - files[i]) != 0) {
			strcpy(latest[nlatest++], files[i]);
		} else {
			strcpy(latest[nlatest - 1], files[i]);
		}
	}
	return nlatest;
}

/* Copy only official issues of CSDB objects. */
int extract_official(char official[OBJECT_MAX][PATH_MAX], char files[OBJECT_MAX][PATH_MAX], int nfiles)
{
	int i, nofficial = 0;
	for (i = 0; i < nfiles; ++i) {
		if (is_official_issue(files[i])) {
			strcpy(official[nofficial++], files[i]);
		}
	}
	return nofficial;
}

int main(int argc, char **argv)
{
	DIR *dir = NULL;

	char (*dms)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	char (*pms)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	char (*coms)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	char (*imfs)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	char (*ddns)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	char (*dmls)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	int ndms;
	int npms;
	int ncoms;
	int nimfs;
	int nddns;
	int ndmls;

	int i;

	int c;
	int only_latest = 0;
	int only_official_issue = 0;
	int only_writable = 0;

	char (*latest_dms)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	int nlatest_dms;

	char (*latest_pms)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	int nlatest_pms;

	char (*latest_imfs)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	int nlatest_imfs;

	char (*latest_dmls)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	int nlatest_dmls;

	char (*issue_dms)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	int nissue_dms;

	char (*issue_pms)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	int nissue_pms;

	char (*issue_imfs)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	int nissue_imfs;

	char (*issue_dmls)[PATH_MAX] = malloc(OBJECT_MAX * PATH_MAX);
	int nissue_dmls;

	int recursive = 0;
	int show = 0;

	while ((c = getopt(argc, argv, "CDiLlMPrwXh?")) != -1) {
		switch (c) {
			case 'C': show |= SHOW_COM; break;
			case 'D': show |= SHOW_DM; break;
			case 'i': only_official_issue = 1; break;
			case 'L': show |= SHOW_DML; break;
			case 'l': only_latest = 1; break;
			case 'M': show |= SHOW_IMF; break;
			case 'P': show |= SHOW_PM; break;
			case 'r': recursive = 1; break;
			case 'w': only_writable = 1; break;
			case 'X': show |= SHOW_DDN; break;
			case 'h':
			case '?': show_help();
				  exit(0);
		}
	}

	if (!show) show = SHOW_DM | SHOW_PM | SHOW_COM | SHOW_IMF | SHOW_DDN | SHOW_DML;

	ndms = 0;
	nlatest_dms = 0;
	nissue_dms = 0;

	npms = 0;
	nlatest_pms = 0;
	nissue_pms = 0;

	ncoms = 0;

	nimfs = 0;
	nlatest_imfs = 0;
	nissue_imfs = 0;

	nddns = 0;

	ndmls = 0;
	nlatest_dmls = 0;
	nissue_dmls = 0;

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
				if (ndms == OBJECT_MAX) {
					fprintf(stderr, S_MAX_DM, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(dms[ndms++], argv[i]);
			} else if (ispm(base)) {
				if (npms == OBJECT_MAX) {
					fprintf(stderr, S_MAX_PM, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(pms[npms++], argv[i]);
			} else if (iscom(base)) {
				if (ncoms == OBJECT_MAX) {
					fprintf(stderr, S_MAX_COM, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(coms[ncoms++], argv[i]);
			} else if (isimf(base)) {
				if (nimfs == OBJECT_MAX) {
					fprintf(stderr, S_MAX_IMF, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(imfs[nimfs++], argv[i]);
			} else if (isddn(base)) {
				if (nddns == OBJECT_MAX) {
					fprintf(stderr, S_MAX_DDN, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(ddns[nddns++], argv[i]);
			} else if (isdml(base)) {
				if (ndmls == OBJECT_MAX) {
					fprintf(stderr, S_MAX_DML, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(dmls[ndmls++], argv[i]);
			} else if (is_directory(argv[i], 0)) {
				list_dir(argv[i],
					dms, &ndms,
					pms, &npms,
					coms, &ncoms,
					imfs, &nimfs,
					ddns, &nddns,
					dmls, &ndmls,
					only_writable, recursive);
			}
		}
	} else {
		/* Read dms to list from current directory */
		list_dir(".",
			dms, &ndms,
			pms, &npms,
			coms, &ncoms,
			imfs, &nimfs,
			ddns, &nddns,
			dmls, &ndmls,
			only_writable, recursive);
	}

	qsort(dms, ndms, PATH_MAX, compare);
	qsort(pms, npms, PATH_MAX, compare);
	qsort(imfs, nimfs, PATH_MAX, compare);
	qsort(dmls, ndmls, PATH_MAX, compare);

	if (only_official_issue) {
		nissue_dms = extract_official(issue_dms, dms, ndms);
		nissue_pms = extract_official(issue_pms, pms, npms);
		nissue_imfs = extract_official(issue_imfs, imfs, nimfs);
		nissue_dmls = extract_official(issue_dmls, dmls, ndmls);

		if (only_latest) {
			nlatest_dms = extract_latest(latest_dms, issue_dms, nissue_dms);
			nlatest_pms = extract_latest(latest_pms, issue_pms, nissue_pms);
			nlatest_imfs = extract_latest(latest_imfs, issue_imfs, nissue_imfs);
			nlatest_dmls = extract_latest(latest_dmls, issue_dmls, nissue_dmls);
		}
	} else if (only_latest) {
		nlatest_dms = extract_latest(latest_dms, dms, ndms);
		nlatest_pms = extract_latest(latest_pms, pms, npms);
		nlatest_imfs = extract_latest(latest_imfs, imfs, nimfs);
		nlatest_dmls = extract_latest(latest_dmls, dmls, ndmls);
	}

	if ((show & SHOW_DM) == SHOW_DM) {
		if (only_latest) {
			printfiles(latest_dms, nlatest_dms);
		} else if (only_official_issue) {
			printfiles(issue_dms, nissue_dms);
		} else {
			printfiles(dms, ndms);
		}
	}

	if ((show & SHOW_PM) == SHOW_PM) {
		if (only_latest) {
			printfiles(latest_pms, nlatest_pms);
		} else if (only_official_issue) {
			printfiles(issue_pms, nissue_pms);
		} else {
			printfiles(pms, npms);
		}
	}

	if ((show & SHOW_COM) == SHOW_COM) {
		printfiles(coms, ncoms);
	}

	if ((show & SHOW_IMF) == SHOW_IMF) {
		if (only_latest) {
			printfiles(latest_imfs, nlatest_imfs);
		} else if (only_official_issue) {
			printfiles(issue_imfs, nissue_imfs);
		} else {
			printfiles(imfs, nimfs);
		}
	}

	if ((show & SHOW_DDN) == SHOW_DDN) {
		printfiles(ddns, nddns);
	}

	if ((show & SHOW_DML) == SHOW_DML) {
		if (only_latest) {
			printfiles(latest_dmls, nlatest_dmls);
		} else if (only_official_issue) {
			printfiles(issue_dmls, nissue_dmls);
		} else {
			printfiles(dmls, ndmls);
		}
	}

	if (dir) {
		closedir(dir);

	}

	free(dms);
	free(pms);
	free(coms);
	free(imfs);
	free(latest_dms);
	free(latest_pms);
	free(latest_imfs);
	free(issue_dms);
	free(issue_pms);
	free(issue_imfs);

	return 0;
}
