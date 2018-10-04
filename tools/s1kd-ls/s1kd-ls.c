#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

/* Maximum number of CSDB objects of each type. */
#define OBJECT_MAX 102400

#define PROG_NAME "s1kd-ls"
#define VERSION "1.2.1"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_OBJECT_MAX 1
#define EXIT_BAD_XML 2

#define S_MAX_DM ERR_PREFIX "Maximum DMs reached (%d).\n"
#define S_MAX_PM ERR_PREFIX "Maximum PMs reached (%d).\n"
#define S_MAX_COM ERR_PREFIX "Maximum comments reached (%d).\n"
#define S_MAX_ICN ERR_PREFIX "Maximum ICNs reached (%d).\n"
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
#define SHOW_ICN 0x40

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

char sep = '\n';
int no_issue = 0;

void printfiles(char files[OBJECT_MAX][PATH_MAX], int n)
{
	int i;
	for (i = 0; i < n; ++i) {
		printf("%s%c", files[i], sep);
	}
}

int compare(const void *a, const void *b)
{
	char *sa, *sb, *ba, *bb;
	int d;

	sa = strdup((const char *) a);
	sb = strdup((const char *) b);
	ba = basename(sa);
	bb = basename(sb);

	d = strcasecmp(ba, bb);

	free(sa);
	free(sb);

	return d;
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

int isicn(const char *name)
{
	return strncmp(name, "ICN-", 4) == 0;
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-0CDGIiLlMNoPrwX] [<object>|<dir> ...]");
	puts("");
	puts("Options:");
	puts("  -0         Output null-delimited list");
	puts("  -C         List comments");
	puts("  -D         List data modules");
	puts("  -G         List ICN files");
	puts("  -I         Show only inwork issues");
	puts("  -i         Show only official issues");
	puts("  -L         List DMLs");
	puts("  -l         Show only latest official/inwork issue");
	puts("  -M         List ICN metadata files");
	puts("  -N         Assume issue/inwork numbers are omitted");
	puts("  -o         Show only old official/inwork issues");
	puts("  -P         List publication modules");
	puts("  -r         Recursively search directories");
	puts("  -w         Show only writable object files");
	puts("  -X         List DDNs");
	puts("  -h -?      Show this help message");
	puts("  --version  Show version information");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
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

int hasopt(int opts, int opt)
{
	return ((opts & opt) == opt);
}

/* Find CSDB objects in a given directory. */
void list_dir(const char *path,
              char dms[OBJECT_MAX][PATH_MAX], int *ndms,
              char pms[OBJECT_MAX][PATH_MAX], int *npms,
	      char coms[OBJECT_MAX][PATH_MAX], int *ncoms,
	      char icns[OBJECT_MAX][PATH_MAX], int *nicns,
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
		else if (dms && isdm(cur->d_name)) {
			if (*ndms == OBJECT_MAX) {
				fprintf(stderr, S_MAX_DM, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(dms[(*ndms)++], cpath);
		} else if (pms && ispm(cur->d_name)) {
			if (*npms == OBJECT_MAX) {
				fprintf(stderr, S_MAX_PM, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(pms[(*npms)++], cpath);
		} else if (coms && iscom(cur->d_name)) {
			if (*ncoms == OBJECT_MAX) {
				fprintf(stderr, S_MAX_COM, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(coms[(*ncoms)++], cpath);
		} else if (imfs && isimf(cur->d_name)) {
			if (*nimfs == OBJECT_MAX) {
				fprintf(stderr, S_MAX_IMF, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(imfs[(*nimfs)++], cpath);
		} else if (icns && isicn(cur->d_name)) {
			if (*nicns == OBJECT_MAX) {
				fprintf(stderr, S_MAX_ICN, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(icns[(*nicns)++], cpath);
		} else if (ddns && isddn(cur->d_name)) {
			if (*nddns == OBJECT_MAX) {
				fprintf(stderr, S_MAX_DDN, OBJECT_MAX);
				exit(EXIT_OBJECT_MAX);
			}
			strcpy(ddns[(*nddns)++], cpath);
		} else if (dmls && isdml(cur->d_name)) {
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
				icns, nicns,
				imfs, nimfs,
				ddns, nddns,
				dmls, ndmls,
				only_writable, recursive);
		}
	}

	closedir(dir);
}

/* Return the first node matching an XPath expression. */
xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	ctx = xmlXPathNewContext(doc ? doc : node->doc);
	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST xpath, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		first = NULL;
	} else {
		first = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

/* Return the content of the first node matching an XPath expression. */
xmlChar *first_xpath_value(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
{
	return xmlNodeGetContent(first_xpath_node(doc, node, xpath));
}

/* Checks if a CSDB object is in the official state (inwork = 00). */
int is_official_issue(const char *fname, const char *path)
{
	if (no_issue) {
		xmlDocPtr doc;
		xmlChar *inwork;
		int official;

		doc = xmlReadFile(path, NULL, PARSE_OPTS | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);

		if (!doc) {
			return 1;
		}

		inwork = first_xpath_value(doc, NULL, "//@inWork|//@inwork");

		if (!inwork) {
			return 1;
		}

		official = xmlStrcmp(inwork, BAD_CAST "00") == 0;

		xmlFree(inwork);
		xmlFreeDoc(doc);

		return official;
	} else {
		char inwork[3] = "";
		int n;
		n = sscanf(fname, "%*[^_]_%*3s-%2s", inwork);
		return n < 1 || strcmp(inwork, "00") == 0;
	}
}

/* Copy only the latest issues of CSDB objects. */
int extract_latest(char latest[OBJECT_MAX][PATH_MAX], char files[OBJECT_MAX][PATH_MAX], int nfiles)
{
	int i, nlatest = 0;
	for (i = 0; i < nfiles; ++i) {
		char *name1, *name2, *base1, *base2;

		name1 = strdup(files[i]);
		base1 = basename(name1);
		if (i > 0) {
			name2 = strdup(files[i - 1]);
			base2 = basename(name2);
		} else {
			name2 = NULL;
		}

		if (i == 0 || strncmp(base1, base2, strchr(base1, '_') - base1) != 0) {
			strcpy(latest[nlatest++], files[i]);
		} else {
			strcpy(latest[nlatest - 1], files[i]);
		}

		free(name1);
		free(name2);
	}
	return nlatest;
}

/* Copy only old issues of CSDB objects. */
int remove_latest(char latest[OBJECT_MAX][PATH_MAX], char files[OBJECT_MAX][PATH_MAX], int nfiles)
{
	int i, nlatest = 0;
	for (i = 0; i < nfiles; ++i) {
		char *name1, *name3, *base1, *base3, *s;

		name1 = strdup(files[i]);
		base1 = basename(name1);

		s = strchr(base1, '_');
		if (!s || !strchr(s + 1, '_')) {
			free(name1);
			continue;
		}

		if (i < nfiles - 1) {
			name3 = strdup(files[i + 1]);
			base3 = basename(name3);
		} else {
			name3 = NULL;
		}

		if (name3 && strncmp(base1, base3, s - base1) == 0) {
			strcpy(latest[nlatest++], files[i]);
		}

		free(name1);
		free(name3);
	}
	return nlatest;
}

/* Copy only official issues of CSDB objects. */
int extract_official(char official[OBJECT_MAX][PATH_MAX], char files[OBJECT_MAX][PATH_MAX], int nfiles)
{
	int i, nofficial = 0;
	for (i = 0; i < nfiles; ++i) {
		char *name = strdup(files[i]);
		char *base = basename(name);

		if (is_official_issue(base, files[i])) {
			strcpy(official[nofficial++], files[i]);
		}

		free(name);
	}
	return nofficial;
}

int remove_official(char official[OBJECT_MAX][PATH_MAX], char files[OBJECT_MAX][PATH_MAX], int nfiles)
{
	int i, nofficial = 0;
	for (i = 0; i < nfiles; ++i) {
		char *name = strdup(files[i]);
		char *base = basename(name);

		if (!is_official_issue(base, files[i])) {
			strcpy(official[nofficial++], files[i]);
		}

		free(name);
	}
	return nofficial;
}

int main(int argc, char **argv)
{
	DIR *dir = NULL;

	char (*dms)[PATH_MAX] = NULL;
	char (*pms)[PATH_MAX] = NULL;
	char (*coms)[PATH_MAX] = NULL;
	char (*icns)[PATH_MAX] = NULL;
	char (*imfs)[PATH_MAX] = NULL;
	char (*ddns)[PATH_MAX] = NULL;
	char (*dmls)[PATH_MAX] = NULL;
	int ndms = 0, npms = 0, ncoms = 0, nicns = 0, nimfs = 0, nddns = 0, ndmls = 0;

	char (*latest_dms)[PATH_MAX] = NULL;
	char (*latest_pms)[PATH_MAX] = NULL;
	char (*latest_imfs)[PATH_MAX] = NULL;
	char (*latest_dmls)[PATH_MAX] = NULL;
	int nlatest_dms = 0, nlatest_pms = 0, nlatest_imfs = 0, nlatest_dmls = 0;

	char (*issue_dms)[PATH_MAX] = NULL;
	char (*issue_pms)[PATH_MAX] = NULL;
	char (*issue_imfs)[PATH_MAX] = NULL;
	char (*issue_dmls)[PATH_MAX] = NULL;
	int nissue_dms = 0, nissue_pms = 0, nissue_imfs = 0, nissue_dmls = 0;

	int only_latest = 0;
	int only_official_issue = 0;
	int only_writable = 0;
	int only_old = 0;
	int only_inwork = 0;
	int recursive = 0;
	int show = 0;

	int i;

	const char *sopts = "0CDGiLlMPrwXoINh?";
	struct option lopts[] = {
		{"version", no_argument, 0, 0},
		{0, 0, 0, 0}
	};
	int loptind = 0;

	while ((i = getopt_long(argc, argv, sopts, lopts, &loptind)) != -1) {
		switch (i) {
			case 0:
				if (strcmp(lopts[loptind].name, "version") == 0) {
					show_version();
					return 0;
				}
				break;
			case '0': sep = '\0'; break;
			case 'C': show |= SHOW_COM; break;
			case 'D': show |= SHOW_DM; break;
			case 'G': show |= SHOW_ICN; break;
			case 'i': only_official_issue = 1; break;
			case 'L': show |= SHOW_DML; break;
			case 'l': only_latest = 1; break;
			case 'M': show |= SHOW_IMF; break;
			case 'P': show |= SHOW_PM; break;
			case 'r': recursive = 1; break;
			case 'w': only_writable = 1; break;
			case 'X': show |= SHOW_DDN; break;
			case 'o': only_old = 1; break;
			case 'I': only_inwork = 1; break;
			case 'N': no_issue = 1; break;
			case 'h':
			case '?': show_help();
				  return 0;
		}
	}

	if (!show) show = SHOW_DM | SHOW_PM | SHOW_COM | SHOW_ICN | SHOW_IMF | SHOW_DDN | SHOW_DML;

	if (hasopt(show, SHOW_DM)) {
		dms = malloc(OBJECT_MAX * PATH_MAX);
	}
	if (hasopt(show, SHOW_PM)) {
		pms = malloc(OBJECT_MAX * PATH_MAX);
	}
	if (hasopt(show, SHOW_COM)) {
		coms = malloc(OBJECT_MAX * PATH_MAX);
	}
	if (hasopt(show, SHOW_ICN)) {
		icns = malloc(OBJECT_MAX * PATH_MAX);
	}
	if (hasopt(show, SHOW_IMF)) {
		imfs = malloc(OBJECT_MAX * PATH_MAX);
	}
	if (hasopt(show, SHOW_DDN)) {
		ddns = malloc(OBJECT_MAX * PATH_MAX);
	}
	if (hasopt(show, SHOW_DML)) {
		dmls = malloc(OBJECT_MAX * PATH_MAX);
	}

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

			if (dms && isdm(base)) {
				if (ndms == OBJECT_MAX) {
					fprintf(stderr, S_MAX_DM, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(dms[ndms++], argv[i]);
			} else if (pms && ispm(base)) {
				if (npms == OBJECT_MAX) {
					fprintf(stderr, S_MAX_PM, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(pms[npms++], argv[i]);
			} else if (coms && iscom(base)) {
				if (ncoms == OBJECT_MAX) {
					fprintf(stderr, S_MAX_COM, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(coms[ncoms++], argv[i]);
			} else if (icns && isicn(base)) {
				if (nicns == OBJECT_MAX) {
					fprintf(stderr, S_MAX_ICN, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(icns[nicns++], argv[i]);
			} else if (imfs && isimf(base)) {
				if (nimfs == OBJECT_MAX) {
					fprintf(stderr, S_MAX_IMF, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(imfs[nimfs++], argv[i]);
			} else if (ddns && isddn(base)) {
				if (nddns == OBJECT_MAX) {
					fprintf(stderr, S_MAX_DDN, OBJECT_MAX);
					exit(EXIT_OBJECT_MAX);
				}
				strcpy(ddns[nddns++], argv[i]);
			} else if (dmls && isdml(base)) {
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
					icns, &nicns,
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
			icns, &nicns,
			imfs, &nimfs,
			ddns, &nddns,
			dmls, &ndmls,
			only_writable, recursive);
	}

	if (ndms) qsort(dms, ndms, PATH_MAX, compare);
	if (npms) qsort(pms, npms, PATH_MAX, compare);
	if (nimfs) qsort(imfs, nimfs, PATH_MAX, compare);
	if (ndmls) qsort(dmls, ndmls, PATH_MAX, compare);

	if (ndms) {
		if (only_latest || only_old) latest_dms = malloc(ndms * PATH_MAX);
		if (only_official_issue || only_inwork) issue_dms = malloc(ndms * PATH_MAX);
	}
	if (npms) {
		if (only_latest || only_old) latest_pms = malloc(npms * PATH_MAX);
		if (only_official_issue || only_inwork) issue_pms = malloc(npms * PATH_MAX);
	}
	if (nimfs) {
		if (only_latest || only_old) latest_imfs = malloc(nimfs * PATH_MAX);
		if (only_official_issue || only_inwork) issue_imfs = malloc(nimfs * PATH_MAX);
	}
	if (ndmls) {
		if (only_latest || only_old) latest_dmls = malloc(ndmls * PATH_MAX);
		if (only_official_issue || only_inwork) issue_dmls = malloc(ndmls * PATH_MAX);
	}

	if (only_official_issue || only_inwork) {
		if (only_old) {
			int (*f)(char (*)[PATH_MAX], char (*)[PATH_MAX], int);

			if (ndms) nissue_dms = remove_latest(issue_dms, dms, ndms);
			if (npms) nissue_pms = remove_latest(issue_pms, pms, npms);
			if (nimfs) nissue_imfs = remove_latest(issue_imfs, imfs, nimfs);
			if (ndmls) nissue_dmls = remove_latest(issue_dmls, dmls, ndmls);

			if (only_official_issue) {
				f = extract_official;
			} else {
				f = remove_official;
			}

			if (nissue_dms) nlatest_dms = f(latest_dms, issue_dms, nissue_dms);
			if (nissue_pms) nlatest_pms = f(latest_pms, issue_pms, nissue_pms);
			if (nissue_imfs) nlatest_imfs = f(latest_imfs, issue_imfs, nissue_imfs);
			if (nissue_dmls) nlatest_dmls = f(latest_dmls, issue_dmls, nissue_dmls);
		} else {
			int (*f)(char (*)[PATH_MAX], char (*)[PATH_MAX], int);

			if (only_official_issue) {
				f = extract_official;
			} else {
				f = remove_official;
			}

			if (ndms) nissue_dms = f(issue_dms, dms, ndms);
			if (npms) nissue_pms = f(issue_pms, pms, npms);
			if (nimfs) nissue_imfs = f(issue_imfs, imfs, nimfs);
			if (ndmls) nissue_dmls = f(issue_dmls, dmls, ndmls);

			if (only_latest) {
				if (nissue_dms) nlatest_dms = extract_latest(latest_dms, issue_dms, nissue_dms);
				if (nissue_pms) nlatest_pms = extract_latest(latest_pms, issue_pms, nissue_pms);
				if (nissue_imfs) nlatest_imfs = extract_latest(latest_imfs, issue_imfs, nissue_imfs);
				if (nissue_dmls) nlatest_dmls = extract_latest(latest_dmls, issue_dmls, nissue_dmls);
			}
		}
	} else if (only_latest || only_old) {
		int (*f)(char (*)[PATH_MAX], char (*)[PATH_MAX], int);

		if (only_latest) {
			f = extract_latest;
		} else {
			f = remove_latest;
		}
		if (ndms) nlatest_dms = f(latest_dms, dms, ndms);
		if (npms) nlatest_pms = f(latest_pms, pms, npms);
		if (nimfs) nlatest_imfs = f(latest_imfs, imfs, nimfs);
		if (ndmls) nlatest_dmls = f(latest_dmls, dmls, ndmls);
	}

	if (ndms) {
		if (only_latest || only_old) {
			printfiles(latest_dms, nlatest_dms);
		} else if (only_official_issue || only_inwork) {
			printfiles(issue_dms, nissue_dms);
		} else {
			printfiles(dms, ndms);
		}
	}

	if (npms) {
		if (only_latest || only_old) {
			printfiles(latest_pms, nlatest_pms);
		} else if (only_official_issue || only_inwork) {
			printfiles(issue_pms, nissue_pms);
		} else {
			printfiles(pms, npms);
		}
	}

	if (ncoms && !only_old) {
		printfiles(coms, ncoms);
	}

	if (nicns && !only_old) {
		printfiles(icns, nicns);
	}

	if (nimfs) {
		if (only_latest || only_old) {
			printfiles(latest_imfs, nlatest_imfs);
		} else if (only_official_issue || only_inwork) {
			printfiles(issue_imfs, nissue_imfs);
		} else {
			printfiles(imfs, nimfs);
		}
	}

	if (nddns && !only_old) {
		printfiles(ddns, nddns);
	}

	if (ndmls) {
		if (only_latest || only_old) {
			printfiles(latest_dmls, nlatest_dmls);
		} else if (only_official_issue || only_inwork) {
			printfiles(issue_dmls, nissue_dmls);
		} else {
			printfiles(dmls, ndmls);
		}
	}

	if (dir) {
		closedir(dir);
	}

	free(dms);
	free(latest_dms);
	free(issue_dms);
	free(pms);
	free(latest_pms);
	free(issue_pms);
	free(coms);
	free(icns);
	free(imfs);
	free(latest_imfs);
	free(issue_imfs);
	free(ddns);
	free(dmls);
	free(latest_dmls);
	free(issue_dmls);

	xmlCleanupParser();

	return 0;
}
