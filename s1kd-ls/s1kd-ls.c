#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/stat.h>

#define DM_MAX 10240

#define PROG_NAME "s1kd-ls"
#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_DM_MAX 1
#define EXIT_BAD_XML 2

#define SHOW_DM 0x1
#define SHOW_PM 0x2
#define SHOW_COM 0x4
#define SHOW_IMF 0x8

void printfiles(char files[DM_MAX][PATH_MAX], int n)
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

int isdm(const char *name)
{
	return (strncmp(name, "DMC-", 4) == 0 || strncmp(name, "DME-", 4) == 0) && strncasecmp(name + strlen(name) - 4, ".XML", 4) == 0;
}

int ispm(const char *name)
{
	return (strncmp(name, "PMC-", 4) == 0 || strncmp(name, "PME-", 4) == 0) && strncasecmp(name + strlen(name) - 4, ".XML", 4) == 0;
}

int iscom(const char *name)
{
	return strncmp(name, "COM-", 4) == 0 && strncasecmp(name + strlen(name) - 4, ".XML", 4) == 0;
}

int isimf(const char *name)
{
	return strncmp(name, "IMF-", 4) == 0 && strncasecmp(name + strlen(name) - 4, ".XML", 4) == 0;
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-CDilMPrw]");
	puts("");
	puts("Options:");
	puts("  -l      Show only latest issue/inwork version");
	puts("  -i      Show only official issues");
	puts("  -w      Show only writable data module files");
	puts("  -r      Recursively search directories");
	puts("  -D      List data modules");
	puts("  -P      List publication modules");
	puts("  -C      List comments");
	puts("  -M      List ICN metadata files");
	puts("  -h      Show this help message");
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

void list_dir(const char *path,
              char dms[DM_MAX][PATH_MAX], int *ndms,
              char pms[DM_MAX][PATH_MAX], int *npms,
	      char coms[DM_MAX][PATH_MAX], int *ncoms,
	      char imfs[DM_MAX][PATH_MAX], int *nimfs,
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
		} else if (iscom(cur->d_name)) {
			if (*ncoms == DM_MAX) {
				fprintf(stderr, ERR_PREFIX "Maximum comments reached (%d).\n", DM_MAX);
				exit(EXIT_DM_MAX);
			}
			strcpy(coms[(*ncoms)++], cpath);
		} else if (isimf(cur->d_name)) {
			if (*nimfs == DM_MAX) {
				fprintf(stderr, ERR_PREFIX "Maximum IMFs reached (%d).\n", DM_MAX);
				exit(EXIT_DM_MAX);
			}
			strcpy(imfs[(*nimfs)++], cpath);
		} else if (recursive && is_directory(cpath, recursive)) {
			list_dir(cpath,
				dms, ndms,
				pms, npms,
				coms, ncoms,
				imfs, nimfs,
				only_writable, recursive);
		}
	}

	closedir(dir);
}

int is_official_issue(const char *fname)
{
	char inwork[3] = "";
	sscanf(fname, "%*[^_]_%*[^-]-%2s", inwork);
	return strcmp(inwork, "00") == 0;
}

int main(int argc, char **argv)
{
	DIR *dir = NULL;

	char (*dms)[PATH_MAX] = malloc(DM_MAX * PATH_MAX);
	char (*pms)[PATH_MAX] = malloc(DM_MAX * PATH_MAX);
	char (*coms)[PATH_MAX] = malloc(DM_MAX * PATH_MAX);
	char (*imfs)[PATH_MAX] = malloc(DM_MAX * PATH_MAX);
	int ndms;
	int npms;
	int ncoms;
	int nimfs;

	int i;

	int c;
	int only_latest = 0;
	int only_official_issue = 0;
	int only_writable = 0;

	char (*latest_dms)[PATH_MAX] = malloc(DM_MAX * PATH_MAX);
	int nlatest_dms;

	char (*latest_pms)[PATH_MAX] = malloc(DM_MAX * PATH_MAX);
	int nlatest_pms;

	char (*latest_imfs)[PATH_MAX] = malloc(DM_MAX * PATH_MAX);
	int nlatest_imfs;

	char (*issue_dms)[PATH_MAX] = malloc(DM_MAX * PATH_MAX);
	int nissue_dms;

	char (*issue_pms)[PATH_MAX] = malloc(DM_MAX * PATH_MAX);
	int nissue_pms;

	char (*issue_imfs)[PATH_MAX] = malloc(DM_MAX * PATH_MAX);
	int nissue_imfs;

	int recursive = 0;
	int show = 0;

	while ((c = getopt(argc, argv, "CDilMPrwh?")) != -1) {
		switch (c) {
			case 'C': show = show | SHOW_COM; break;
			case 'D': show = show | SHOW_DM; break;
			case 'i': only_official_issue = 1; break;
			case 'l': only_latest = 1; break;
			case 'M': show = show | SHOW_IMF; break;
			case 'P': show = show | SHOW_PM; break;
			case 'r': recursive = 1; break;
			case 'w': only_writable = 1; break;
			case 'h':
			case '?': show_help();
				  exit(0);
		}
	}

	if (!show) show = SHOW_DM | SHOW_PM | SHOW_COM | SHOW_IMF;

	ndms = 0;
	npms = 0;
	ncoms = 0;
	nimfs = 0;
	nlatest_dms = 0;
	nlatest_pms = 0;
	nlatest_imfs = 0;
	nissue_dms = 0;
	nissue_pms = 0;
	nissue_imfs = 0;

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
			} else if (iscom(base)) {
				if (ncoms == DM_MAX) {
					fprintf(stderr, ERR_PREFIX "Maximum comments reached (%d).\n", DM_MAX);
					exit(EXIT_DM_MAX);
				}
				strcpy(coms[ncoms++], argv[i]);
			} else if (isimf(base)) {
				if (nimfs == DM_MAX) {
					fprintf(stderr, ERR_PREFIX "Maximum IMFs reached (%d).\n", DM_MAX);
					exit(EXIT_DM_MAX);
				}
				strcpy(imfs[nimfs++], argv[i]);
			} else if (is_directory(argv[i], 0)) {
				list_dir(argv[i],
					dms, &ndms,
					pms, &npms,
					coms, &ncoms,
					imfs, &nimfs,
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
			only_writable, recursive);
	}

	qsort(dms, ndms, PATH_MAX, compare);
	qsort(pms, npms, PATH_MAX, compare);
	qsort(imfs, nimfs, PATH_MAX, compare);

	if (only_official_issue) {
		for (i = 0; i < ndms; ++i) {
			if (is_official_issue(dms[i])) {
				strcpy(issue_dms[nissue_dms++], dms[i]);
			}
		}

		for (i = 0; i < npms; ++i) {
			if (is_official_issue(pms[i])) {
				strcpy(issue_pms[nissue_pms++], pms[i]);
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

			for (i = 0; i < nissue_pms; ++i) {
				if (i == 0 || strncpy(issue_pms[i], issue_pms[i - 1], strchr(issue_pms[i], '_') - issue_pms[i]) != 0) {
					strcpy(latest_pms[nlatest_pms++], issue_pms[i]);
				} else {
					strcpy(latest_pms[nlatest_pms - 1], issue_pms[i]);
				}
			}

			for (i = 0; i < nissue_imfs; ++i) {
				if (i == 0 || strncpy(issue_imfs[i], issue_imfs[i - 1], strchr(issue_imfs[i], '_') - issue_imfs[i]) != 0) {
					strcpy(latest_imfs[nlatest_imfs++], issue_imfs[i]);
				} else {
					strcpy(latest_imfs[nlatest_imfs - 1], issue_imfs[i]);
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

		for (i = 0; i < npms; ++i) {
			if (i == 0 || strncmp(pms[i], pms[i - 1], strchr(pms[i], '_') - pms[i]) != 0) {
				strcpy(latest_pms[nlatest_pms++], pms[i]);
			} else {
				strcpy(latest_pms[nlatest_pms - 1], pms[i]);
			}
		}

		for (i = 0; i < nimfs; ++i) {
			if (i == 0 || strncmp(imfs[i], imfs[i - 1], strchr(imfs[i], '_') - imfs[i]) != 0) {
				strcpy(latest_imfs[nlatest_imfs++], imfs[i]);
			} else {
				strcpy(latest_imfs[nlatest_imfs - 1], imfs[i]);
			}
		}
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
