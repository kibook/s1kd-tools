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

#include "s1kd_tools.h"

/* Initial maximum number of CSDB objects of each type. */
#define OBJECT_MAX 1
static unsigned DM_MAX  = OBJECT_MAX;
static unsigned PM_MAX  = OBJECT_MAX;
static unsigned COM_MAX = OBJECT_MAX;
static unsigned IMF_MAX = OBJECT_MAX;
static unsigned DDN_MAX = OBJECT_MAX;
static unsigned DML_MAX = OBJECT_MAX;
static unsigned ICN_MAX = OBJECT_MAX;
static unsigned SMC_MAX = OBJECT_MAX;
static unsigned UPF_MAX = OBJECT_MAX;

#define PROG_NAME "s1kd-ls"
#define VERSION "1.10.0"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define EXIT_OBJECT_MAX 1 /* Cannot allocate memory for more objects. */

#define E_MAX_OBJECT ERR_PREFIX "Maximum CSDB objects reached: %d\n"

/* Set of CSDB object types to list. */
#define SHOW_DM  0x001
#define SHOW_PM  0x002
#define SHOW_COM 0x004
#define SHOW_IMF 0x008
#define SHOW_DDN 0x010
#define SHOW_DML 0x020
#define SHOW_ICN 0x040
#define SHOW_SMC 0x080
#define SHOW_UPF 0x100

/* Lists of CSDB objects. */
static char (*dms)[PATH_MAX] = NULL;
static char (*pms)[PATH_MAX] = NULL;
static char (*smcs)[PATH_MAX] = NULL;
static char (*coms)[PATH_MAX] = NULL;
static char (*icns)[PATH_MAX] = NULL;
static char (*imfs)[PATH_MAX] = NULL;
static char (*ddns)[PATH_MAX] = NULL;
static char (*dmls)[PATH_MAX] = NULL;
static char (*upfs)[PATH_MAX] = NULL;
static int ndms = 0, npms = 0, ncoms = 0, nicns = 0, nimfs = 0, nddns = 0, ndmls = 0, nsmcs = 0, nupfs = 0;

/* Separator between printed CSDB objects. */
static char sep = '\n';

/* Whether the CSDB objects were created with the -N option. */
static int no_issue = 0;

static void printfiles(char (*files)[PATH_MAX], int n)
{
	int i;
	for (i = 0; i < n; ++i) {
		printf("%s%c", files[i], sep);
	}
}

/* Compare the base names of two files. */
static int compare(const void *a, const void *b)
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

/* Compare two ICN files, grouped by file extension. */
static int compare_icn(const void *a, const void *b)
{
	char *sa, *sb, *ba, *bb, *e, *ea, *eb;
	int d;

	sa = strdup((const char *) a);
	sb = strdup((const char *) b);
	ba = basename(sa);
	bb = basename(sb);

	/* Move the file extension to the front. */
	ea = malloc(strlen(ba) + 1);
	eb = malloc(strlen(bb) + 1);

	if ((e = strchr(ba, '.'))) {
		d = e - ba;
		sprintf(ea, "%s%.*s", e, d, ba);
	} else {
		strcpy(ea, ba);
	}

	if ((e = strchr(bb, '.'))) {
		d = e - bb;
		sprintf(eb, "%s%.*s", e, d, bb);
	} else {
		strcpy(eb, bb);
	}

	d = strcasecmp(ea, eb);

	free(ea);
	free(eb);
	free(sa);
	free(sb);

	return d;
}

/* Show usage message. */
static void show_help(void)
{
	puts("Usage: " PROG_NAME " [-0CDGIiLlMNoPRrSwX] [<object>|<dir> ...]");
	puts("");
	puts("Options:");
	puts("  -0, --null        Output null-delimited list.");
	puts("  -C, --com         List comments.");
	puts("  -D, --dm          List data modules.");
	puts("  -G, --icn         List ICN files.");
	puts("  -I, --inwork      Show only inwork issues.");
	puts("  -i, --official    Show only official issues.");
	puts("  -h, -?, --help    Show this help message.");
	puts("  -L, --dml         List DMLs.");
	puts("  -l, --latest      Show only latest official/inwork issue.");
	puts("  -M, --imf         List ICN metadata files.");
	puts("  -N, --omit-issue  Assume issue/inwork numbers are omitted.");
	puts("  -o, --old         Show only old official/inwork issues.");
	puts("  -P, --pm          List publication modules.");
	puts("  -R, --read-only   Show only non-writable object files.");
	puts("  -r, --recursive   Recursively search directories.");
	puts("  -S, --smc         List SCORM content packages.");
	puts("  -U, --upf         List data update files.");
	puts("  -w, --writable    Show only writable object files.");
	puts("  -X, --ddn         List DDNs.");
	puts("  --version         Show version information.");
	LIBXML2_PARSE_LONGOPT_HELP
}

/* Show version information. */
static void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
	printf("Using libxml %s\n", xmlParserVersion);
}

/* Resize CSDB object lists when it is full. */
static void resize(char (**list)[PATH_MAX], unsigned *max)
{
	if (!(*list = realloc(*list, (*max *= 2) * PATH_MAX))) {
		fprintf(stderr, E_MAX_OBJECT,
			ndms + npms + ncoms + nimfs + nicns + nddns + ndmls + nsmcs);
		exit(EXIT_OBJECT_MAX);
	}
}

/* Find CSDB objects in a given directory. */
static void list_dir(const char *path, int only_writable, int only_readonly, int recursive)
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

		if (access(cpath, R_OK) != 0) {
			continue;
		} else if (only_writable && access(cpath, W_OK) != 0) {
			continue;
		} else if (only_readonly && access(cpath, W_OK) == 0) {
			continue;
		} else if (dms && is_dm(cur->d_name)) {
			if (ndms == DM_MAX) {
				resize(&dms, &DM_MAX);
			}
			strcpy(dms[(ndms)++], cpath);
		} else if (pms && is_pm(cur->d_name)) {
			if (npms == PM_MAX) {
				resize(&pms, &PM_MAX);
			}
			strcpy(pms[(npms)++], cpath);
		} else if (coms && is_com(cur->d_name)) {
			if (ncoms == COM_MAX) {
				resize(&coms, &COM_MAX);
			}
			strcpy(coms[(ncoms)++], cpath);
		} else if (imfs && is_imf(cur->d_name)) {
			if (nimfs == IMF_MAX) {
				resize(&imfs, &IMF_MAX);
			}
			strcpy(imfs[(nimfs)++], cpath);
		} else if (icns && is_icn(cur->d_name)) {
			if (nicns == ICN_MAX) {
				resize(&icns, &ICN_MAX);
			}
			strcpy(icns[(nicns)++], cpath);
		} else if (ddns && is_ddn(cur->d_name)) {
			if (nddns == DDN_MAX) {
				resize(&ddns, &DDN_MAX);
			}
			strcpy(ddns[(nddns)++], cpath);
		} else if (dmls && is_dml(cur->d_name)) {
			if (ndmls == DML_MAX) {
				resize(&dmls, &DML_MAX);
			}
			strcpy(dmls[(ndmls)++], cpath);
		} else if (smcs && is_smc(cur->d_name)) {
			if (nsmcs == SMC_MAX) {
				resize(&smcs, &SMC_MAX);
			}
			strcpy(smcs[(nsmcs)++], cpath);
		} else if (upfs && is_upf(cur->d_name)) {
			if (nupfs == UPF_MAX) {
				resize(&upfs, &UPF_MAX);
			}
			strcpy(upfs[(nupfs)++], cpath);
		} else if (recursive && isdir(cpath, recursive)) {
			list_dir(cpath, only_writable, only_readonly, recursive);
		}
	}

	closedir(dir);
}

/* Return the first node matching an XPath expression. */
static xmlNodePtr first_xpath_node(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
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
static xmlChar *first_xpath_value(xmlDocPtr doc, xmlNodePtr node, const char *xpath)
{
	return xmlNodeGetContent(first_xpath_node(doc, node, xpath));
}

/* Checks if a CSDB object is in the official state (inwork = 00). */
static int is_official_issue(const char *fname, const char *path)
{
	if (no_issue) {
		xmlDocPtr doc;
		xmlChar *inwork;
		int official;

		doc = read_xml_doc(path);

		if (!doc) {
			return 1;
		}

		inwork = first_xpath_value(doc, NULL, "//@inWork|//@inwork");

		official = !inwork || xmlStrcmp(inwork, BAD_CAST "00") == 0;

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
static int extract_latest(char (*latest)[PATH_MAX], char (*files)[PATH_MAX], int nfiles)
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
static int extract_latest_icns(char (*latest)[PATH_MAX], char (*files)[PATH_MAX], int nfiles)
{
	int i, nlatest = 0;
	for (i = 0; i < nfiles; ++i) {
		char *name1, *name2, *base1, *base2, *s;
		int n;

		name1 = strdup(files[i]);
		base1 = basename(name1);
		if (i > 0) {
			name2 = strdup(files[i - 1]);
			base2 = basename(name2);
		} else {
			name2 = NULL;
		}

		s = strrchr(base1, '-');
		n = s - base1;

		if (i == 0 || strncmp(base1, base2, n - 3) != 0 || strcmp(s, base2 + n) != 0) {
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
static int remove_latest(char (*latest)[PATH_MAX], char (*files)[PATH_MAX], int nfiles)
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
static int remove_latest_icns(char (*latest)[PATH_MAX], char (*files)[PATH_MAX], int nfiles)
{
	int i, nlatest = 0;
	for (i = 0; i < nfiles; ++i) {
		char *name1, *name3, *base1, *base3, *s;
		int n;

		name1 = strdup(files[i]);
		base1 = basename(name1);

		s = strrchr(base1, '-');
		if (!s) {
			free(name1);
			continue;
		}

		if (i < nfiles - 1) {
			name3 = strdup(files[i + 1]);
			base3 = basename(name3);
		} else {
			name3 = NULL;
		}

		n = s - base1;

		if (name3 && strncmp(base1, base3, n - 3) == 0 && strcmp(s, base3 + n) == 0) {
			strcpy(latest[nlatest++], files[i]);
		}

		free(name1);
		free(name3);
	}
	return nlatest;
}

/* Copy only official issues of CSDB objects. */
static int extract_official(char (*official)[PATH_MAX], char (*files)[PATH_MAX], int nfiles)
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

/* Copy a list, removing official CSDB objects. */
static int remove_official(char (*official)[PATH_MAX], char (*files)[PATH_MAX], int nfiles)
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

	char (*latest_dms)[PATH_MAX] = NULL;
	char (*latest_pms)[PATH_MAX] = NULL;
	char (*latest_smcs)[PATH_MAX] = NULL;
	char (*latest_imfs)[PATH_MAX] = NULL;
	char (*latest_dmls)[PATH_MAX] = NULL;
	char (*latest_icns)[PATH_MAX] = NULL;
	char (*latest_upfs)[PATH_MAX] = NULL;
	int nlatest_dms = 0, nlatest_pms = 0, nlatest_imfs = 0, nlatest_dmls = 0, nlatest_icns = 0, nlatest_smcs = 0, nlatest_upfs = 0;

	char (*issue_dms)[PATH_MAX] = NULL;
	char (*issue_pms)[PATH_MAX] = NULL;
	char (*issue_smcs)[PATH_MAX] = NULL;
	char (*issue_imfs)[PATH_MAX] = NULL;
	char (*issue_dmls)[PATH_MAX] = NULL;
	char (*issue_upfs)[PATH_MAX] = NULL;
	int nissue_dms = 0, nissue_pms = 0, nissue_imfs = 0, nissue_dmls = 0, nissue_smcs = 0, nissue_upfs = 0;

	int only_latest = 0;
	int only_official_issue = 0;
	int only_writable = 0;
	int only_readonly = 0;
	int only_old = 0;
	int only_inwork = 0;
	int recursive = 0;
	int show = 0;

	int i;

	const char *sopts = "0CDGiLlMPRrSwXoINUh?";
	struct option lopts[] = {
		{"version"   , no_argument, 0, 0},
		{"help"      , no_argument, 0, 'h'},
		{"null"      , no_argument, 0, '0'},
		{"com"       , no_argument, 0, 'C'},
		{"dm"        , no_argument, 0, 'D'},
		{"icn"       , no_argument, 0, 'G'},
		{"official"  , no_argument, 0, 'i'},
		{"dml"       , no_argument, 0, 'L'},
		{"latest"    , no_argument, 0, 'l'},
		{"imf"       , no_argument, 0, 'M'},
		{"pm"        , no_argument, 0, 'P'},
		{"read-only" , no_argument, 0, 'R'},
		{"recursive" , no_argument, 0, 'r'},
		{"smc"       , no_argument, 0, 'S'},
		{"writable"  , no_argument, 0, 'w'},
		{"ddn"       , no_argument, 0, 'X'},
		{"old"       , no_argument, 0, 'o'},
		{"inwork"    , no_argument, 0, 'I'},
		{"omit-issue", no_argument, 0, 'N'},
		{"upf"       , no_argument, 0, 'U'},
		LIBXML2_PARSE_LONGOPT_DEFS
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
				LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind)
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
			case 'R': only_readonly = 1; break;
			case 'r': recursive = 1; break;
			case 'S': show |= SHOW_SMC; break;
			case 'w': only_writable = 1; break;
			case 'X': show |= SHOW_DDN; break;
			case 'o': only_old = 1; break;
			case 'I': only_inwork = 1; break;
			case 'N': no_issue = 1; break;
			case 'U': show |= SHOW_UPF; break;
			case 'h':
			case '?': show_help();
				  return 0;
		}
	}

	if (!show) show = SHOW_DM | SHOW_PM | SHOW_COM | SHOW_ICN | SHOW_IMF | SHOW_DDN | SHOW_DML | SHOW_SMC | SHOW_UPF;

	if (optset(show, SHOW_DM)) {
		dms = malloc(DM_MAX * PATH_MAX);
	}
	if (optset(show, SHOW_PM)) {
		pms = malloc(PM_MAX * PATH_MAX);
	}
	if (optset(show, SHOW_COM)) {
		coms = malloc(COM_MAX * PATH_MAX);
	}
	if (optset(show, SHOW_ICN)) {
		icns = malloc(ICN_MAX * PATH_MAX);
	}
	if (optset(show, SHOW_IMF)) {
		imfs = malloc(IMF_MAX * PATH_MAX);
	}
	if (optset(show, SHOW_DDN)) {
		ddns = malloc(DDN_MAX * PATH_MAX);
	}
	if (optset(show, SHOW_DML)) {
		dmls = malloc(DML_MAX * PATH_MAX);
	}
	if (optset(show, SHOW_SMC)) {
		smcs = malloc(SMC_MAX * PATH_MAX);
	}
	if (optset(show, SHOW_UPF)) {
		upfs = malloc(UPF_MAX * PATH_MAX);
	}

	if (optind < argc) {
		/* Read dms to list from arguments */
		for (i = optind; i < argc; ++i) {
			char path[PATH_MAX], *base;

			strcpy(path, argv[i]);
			base = basename(path);

			if (access(argv[i], R_OK) != 0) {
				continue;
			} else if (only_writable && access(argv[i], W_OK) != 0) {
				continue;
			} else if (only_readonly && access(argv[i], W_OK) == 0) {
				continue;
			} else if (dms && is_dm(base)) {
				if (ndms == DM_MAX) {
					resize(&dms, &DM_MAX);
				}
				strcpy(dms[ndms++], argv[i]);
			} else if (pms && is_pm(base)) {
				if (npms == PM_MAX) {
					resize(&pms, &PM_MAX);
				}
				strcpy(pms[npms++], argv[i]);
			} else if (coms && is_com(base)) {
				if (ncoms == COM_MAX) {
					resize(&coms, &COM_MAX);
				}
				strcpy(coms[ncoms++], argv[i]);
			} else if (icns && is_icn(base)) {
				if (nicns == ICN_MAX) {
					resize(&icns, &ICN_MAX);
				}
				strcpy(icns[nicns++], argv[i]);
			} else if (imfs && is_imf(base)) {
				if (nimfs == IMF_MAX) {
					resize(&imfs, &IMF_MAX);
				}
				strcpy(imfs[nimfs++], argv[i]);
			} else if (ddns && is_ddn(base)) {
				if (nddns == DDN_MAX) {
					resize(&ddns, &DDN_MAX);
				}
				strcpy(ddns[nddns++], argv[i]);
			} else if (dmls && is_dml(base)) {
				if (ndmls == DML_MAX) {
					resize(&dmls, &DML_MAX);
				}
				strcpy(dmls[ndmls++], argv[i]);
			} else if (smcs && is_smc(base)) {
				if (nsmcs == SMC_MAX) {
					resize(&smcs, &SMC_MAX);
				}
				strcpy(smcs[nsmcs++], argv[i]);
			} else if (upfs && is_upf(base)) {
				if (nupfs == UPF_MAX) {
					resize(&upfs, &UPF_MAX);
				}
				strcpy(upfs[nupfs++], argv[i]);
			} else if (isdir(argv[i], 0)) {
				list_dir(argv[i], only_writable, only_readonly, recursive);
			}
		}
	} else {
		/* Read dms to list from current directory */
		list_dir(".", only_writable, only_readonly, recursive);
	}


	if (ndms) {
		qsort(dms, ndms, PATH_MAX, compare);
		if (only_latest || only_old) latest_dms = malloc(ndms * PATH_MAX);
		if (only_official_issue || only_inwork) issue_dms = malloc(ndms * PATH_MAX);
	} else {
		free(dms);
	}
	if (npms) {
		qsort(pms, npms, PATH_MAX, compare);
		if (only_latest || only_old) latest_pms = malloc(npms * PATH_MAX);
		if (only_official_issue || only_inwork) issue_pms = malloc(npms * PATH_MAX);
	} else {
		free(pms);
	}
	if (nsmcs) {
		qsort(smcs, nsmcs, PATH_MAX, compare);
		if (only_latest || only_old) latest_smcs = malloc(nsmcs * PATH_MAX);
		if (only_official_issue || only_inwork) issue_smcs = malloc(nsmcs * PATH_MAX);
	} else {
		free(smcs);
	}
	if (nupfs) {
		qsort(upfs, nupfs, PATH_MAX, compare);
		if (only_latest || only_old) latest_upfs = malloc(nupfs * PATH_MAX);
		if (only_official_issue || only_inwork) issue_upfs = malloc(nupfs * PATH_MAX);
	} else {
		free(upfs);
	}
	if (nimfs) {
		qsort(imfs, nimfs, PATH_MAX, compare);
		if (only_latest || only_old) latest_imfs = malloc(nimfs * PATH_MAX);
		if (only_official_issue || only_inwork) issue_imfs = malloc(nimfs * PATH_MAX);
	} else {
		free(imfs);
	}
	if (ndmls) {
		qsort(dmls, ndmls, PATH_MAX, compare);
		if (only_latest || only_old) latest_dmls = malloc(ndmls * PATH_MAX);
		if (only_official_issue || only_inwork) issue_dmls = malloc(ndmls * PATH_MAX);
	} else {
		free(dmls);
	}
	if (nicns) {
		qsort(icns, nicns, PATH_MAX, compare_icn);
		if (only_latest || only_old) latest_icns = malloc(nicns * PATH_MAX);
	} else {
		free(icns);
	}

	if (!ncoms) {
		free(coms);
	}
	if (!nddns) {
		free(ddns);
	}

	if (only_official_issue || only_inwork) {
		if (only_old) {
			int (*f)(char (*)[PATH_MAX], char (*)[PATH_MAX], int);

			if (ndms) {
				nissue_dms = remove_latest(issue_dms, dms, ndms);
				free(dms);
			}
			if (npms) {
				nissue_pms = remove_latest(issue_pms, pms, npms);
				free(pms);
			}
			if (nsmcs) {
				nissue_smcs = remove_latest(issue_smcs, smcs, nsmcs);
				free(smcs);
			}
			if (nupfs) {
				nissue_upfs = remove_latest(issue_upfs, upfs, nupfs);
				free(upfs);
			}
			if (nimfs) {
				nissue_imfs = remove_latest(issue_imfs, imfs, nimfs);
				free(imfs);
			}
			if (ndmls) {
				nissue_dmls = remove_latest(issue_dmls, dmls, ndmls);
				free(dmls);
			}
			if (nicns) {
				nlatest_icns = remove_latest_icns(latest_icns, icns, nicns);
				free(icns);
			}

			if (only_official_issue) {
				f = extract_official;
			} else {
				f = remove_official;
			}

			if (nissue_dms) {
				nlatest_dms = f(latest_dms, issue_dms, nissue_dms);
			}
			if (nissue_pms) {
				nlatest_pms = f(latest_pms, issue_pms, nissue_pms);
			}
			if (nissue_smcs) {
				nlatest_smcs = f(latest_smcs, issue_smcs, nissue_smcs);
			}
			if (nissue_upfs) {
				nlatest_upfs = f(latest_upfs, issue_upfs, nissue_upfs);
			}
			if (nissue_imfs) {
				nlatest_imfs = f(latest_imfs, issue_imfs, nissue_imfs);
			}
			if (nissue_dmls) {
				nlatest_dmls = f(latest_dmls, issue_dmls, nissue_dmls);
			}

			free(issue_dms);
			free(issue_pms);
			free(issue_smcs);
			free(issue_upfs);
			free(issue_imfs);
			free(issue_dmls);
		} else {
			int (*f)(char (*)[PATH_MAX], char (*)[PATH_MAX], int);

			if (only_official_issue) {
				f = extract_official;
			} else {
				f = remove_official;
			}

			if (ndms) {
				nissue_dms = f(issue_dms, dms, ndms);
				free(dms);
			}
			if (npms) {
				nissue_pms = f(issue_pms, pms, npms);
				free(pms);
			}
			if (nsmcs) {
				nissue_smcs = f(issue_smcs, smcs, nsmcs);
				free(smcs);
			}
			if (nupfs) {
				nissue_upfs = f(issue_upfs, upfs, nupfs);
				free(upfs);
			}
			if (nimfs) {
				nissue_imfs = f(issue_imfs, imfs, nimfs);
				free(imfs);
			}
			if (ndmls) {
				nissue_dmls = f(issue_dmls, dmls, ndmls);
				free(dmls);
			}

			if (only_latest) {
				if (nissue_dms) {
					nlatest_dms = extract_latest(latest_dms, issue_dms, nissue_dms);
				}
				if (nissue_pms) {
					nlatest_pms = extract_latest(latest_pms, issue_pms, nissue_pms);
				}
				if (nissue_smcs) {
					nlatest_smcs = extract_latest(latest_smcs, issue_smcs, nissue_smcs);
				}
				if (nissue_upfs) {
					nlatest_upfs = extract_latest(latest_upfs, issue_upfs, nissue_upfs);
				}
				if (nissue_imfs) {
					nlatest_imfs = extract_latest(latest_imfs, issue_imfs, nissue_imfs);
				}
				if (nissue_dmls) {
					nlatest_dmls = extract_latest(latest_dmls, issue_dmls, nissue_dmls);
				}
				if (nicns) {
					nlatest_icns = extract_latest_icns(latest_icns, icns, nicns);
				}

				free(issue_dms);
				free(issue_pms);
				free(issue_smcs);
				free(issue_upfs);
				free(issue_imfs);
				free(issue_dmls);
				free(icns);
			}
		}
	} else if (only_latest || only_old) {
		int (*f)(char (*)[PATH_MAX], char (*)[PATH_MAX], int);
		int (*icnf)(char (*)[PATH_MAX], char (*)[PATH_MAX], int);

		if (only_latest) {
			f = extract_latest;
			icnf = extract_latest_icns;
		} else {
			f = remove_latest;
			icnf = remove_latest_icns;
		}
		if (ndms) {
			nlatest_dms = f(latest_dms, dms, ndms);
			free(dms);
		}
		if (npms) {
			nlatest_pms = f(latest_pms, pms, npms);
			free(pms);
		}
		if (nsmcs) {
			nlatest_smcs = f(latest_smcs, smcs, nsmcs);
			free(smcs);
		}
		if (nupfs) {
			nlatest_upfs = f(latest_upfs, upfs, nupfs);
			free(upfs);
		}
		if (nimfs) {
			nlatest_imfs = f(latest_imfs, imfs, nimfs);
			free(imfs);
		}
		if (ndmls) {
			nlatest_dmls = f(latest_dmls, dmls, ndmls);
			free(dmls);
		}
		if (nicns) {
			nlatest_icns = icnf(latest_icns, icns, nicns);
			free(icns);
		}
	}

	if (ncoms) {
		if (!only_old) {
			printfiles(coms, ncoms);
		}
		free(coms);
	}

	if (nddns) {
		if (!only_old) {
			printfiles(ddns, nddns);
		}
		free(ddns);
	}

	if (ndms) {
		if (only_latest || only_old) {
			printfiles(latest_dms, nlatest_dms);
			free(latest_dms);
		} else if (only_official_issue || only_inwork) {
			printfiles(issue_dms, nissue_dms);
			free(issue_dms);
		} else {
			printfiles(dms, ndms);
			free(dms);
		}
	}

	if (ndmls) {
		if (only_latest || only_old) {
			printfiles(latest_dmls, nlatest_dmls);
			free(latest_dmls);
		} else if (only_official_issue || only_inwork) {
			printfiles(issue_dmls, nissue_dmls);
			free(issue_dmls);
		} else {
			printfiles(dmls, ndmls);
			free(dmls);
		}
	}

	if (nicns) {
		if (only_inwork) {
			if (only_latest || only_old) {
				free(latest_icns);
			} else {
				free(icns);
			}
		} else {
			if (only_latest || only_old) {
				printfiles(latest_icns, nlatest_icns);
				free(latest_icns);
			} else {
				printfiles(icns, nicns);
				free(icns);
			}
		}
	}

	if (nimfs) {
		if (only_latest || only_old) {
			printfiles(latest_imfs, nlatest_imfs);
			free(latest_imfs);
		} else if (only_official_issue || only_inwork) {
			printfiles(issue_imfs, nissue_imfs);
			free(issue_imfs);
		} else {
			printfiles(imfs, nimfs);
			free(imfs);
		}
	}

	if (npms) {
		if (only_latest || only_old) {
			printfiles(latest_pms, nlatest_pms);
			free(latest_pms);
		} else if (only_official_issue || only_inwork) {
			printfiles(issue_pms, nissue_pms);
			free(issue_pms);
		} else {
			printfiles(pms, npms);
			free(pms);
		}
	}

	if (nsmcs) {
		if (only_latest || only_old) {
			printfiles(latest_smcs, nlatest_smcs);
			free(latest_smcs);
		} else if (only_official_issue || only_inwork) {
			printfiles(issue_smcs, nissue_smcs);
			free(issue_smcs);
		} else {
			printfiles(smcs, nsmcs);
			free(smcs);
		}
	}

	if (nupfs) {
		if (only_latest || only_old) {
			printfiles(latest_upfs, nlatest_upfs);
			free(latest_upfs);
		} else if (only_official_issue || only_inwork) {
			printfiles(issue_upfs, nissue_upfs);
			free(issue_upfs);
		} else {
			printfiles(upfs, nupfs);
			free(upfs);
		}
	}

	if (dir) {
		closedir(dir);
	}

	xmlCleanupParser();

	return 0;
}
