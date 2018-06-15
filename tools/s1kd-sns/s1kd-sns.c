#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define PROG_NAME "s1kd-sns"
#define VERSION "1.0.1"

#define ERR_PREFIX PROG_NAME ": ERROR: "

/* Bug in libxml < 2.9.2 where parameter entities are resolved even when
 * XML_PARSE_NOENT is not specified.
 */
#if LIBXML_VERSION < 20902
#define PARSE_OPTS XML_PARSE_NONET
#else
#define PARSE_OPTS 0
#endif

#define DEFAULT_SNS_DNAME "SNS"

int hlink(const char *path, const char *fname)
{
	#ifdef _WIN32
		return CreateHardLink(fname, path, 0);
	#else
		return link(path, fname);
	#endif
}

int slink(const char *path, const char *fname)
{
	#ifdef _WIN32
		return CreateSymbolicLink(fname, path, 0);
	#else
		return symlink(path, fname);
	#endif
}

/* The type of link to use, hard or soft (symbolic). */
int (*linkfn)(const char *path, const char *fname) = hlink;

/* Title SNS directories using only the SNS code, not including the SNS title. */
bool only_numb = false;

/* Indent printed SNS entry to a specified level. */
void indent(int level)
{
	int i;
	for (i = 0; i < level * 4; ++i) putchar(' ');
}

/* Print the SNS. */
void print_sns(xmlNodePtr node, int level)
{
	xmlNodePtr cur;

	char *content;

	for (cur = node->children; cur; cur = cur->next) {
		if (xmlStrcmp(cur->name, BAD_CAST "snsCode") == 0) {
			content = (char *) xmlNodeGetContent(cur);
			indent(level);
			printf("%s", content);
			xmlFree(content);
		} else if (xmlStrcmp(cur->name, BAD_CAST "snsTitle") == 0) {
			content = (char *) xmlNodeGetContent(cur);
			printf(" - %s\n", content);
			xmlFree(content);
		} else if (cur->type == XML_ELEMENT_NODE) {
			print_sns(cur, level + 1);
		}
	}
}

/* Replace characters that cannot be used in directory names. */
void cleanstr(char *s)
{
	int i;
	for (i = 0; s[i]; ++i) {
		switch(s[i]) {
			case '/':
			#ifdef _WIN32
			case '<':
			case '>':
			case ':':
			case '"':
			case '\\':
			case '|':
			case '?':
			case '*':
			#endif
				s[i] = ' ';
		}
	}
}

/* Create a directory if it does not exist. */
void makedir(const char *path)
{
	if (access(path, F_OK) == -1) {
		#ifdef _WIN32
			mkdir(path);
		#else
			mkdir(path, S_IRWXU);
		#endif
	}
}

/* Create the directory structure for the SNS. */
void setup_sns(xmlNodePtr node, const char *snsdname)
{
	xmlNodePtr cur;
	char *content;
	char code[256];

	if (xmlStrcmp(node->name, BAD_CAST "snsDescr") == 0) {
		makedir(snsdname);
		if (chdir(snsdname) != 0) exit(1);
	}

	for (cur = node->children; cur; cur = cur->next) {
		if (xmlStrcmp(cur->name, BAD_CAST "snsCode") == 0) {
			content = (char *) xmlNodeGetContent(cur);

			strcpy(code, content);

			xmlFree(content);

			makedir(code);

			if (chdir(code) != 0) exit(1);
		} else if (xmlStrcmp(cur->name, BAD_CAST "snsTitle") == 0) {
			char oldname[PATH_MAX], newname[PATH_MAX];

			if (only_numb) continue;

			sprintf(oldname, "../%s", code);

			content = (char *) xmlNodeGetContent(cur);
			cleanstr(content);

			sprintf(newname, "../%s - %s", code, content);

			xmlFree(content);

			rename(oldname, newname);
		} else if (cur->type == XML_ELEMENT_NODE) {
			setup_sns(cur, snsdname);
		}
	}

	if (chdir("..") != 0) exit(1);
}

/* Tests if the given file is a data module. */
int is_dmodule(const char *fname)
{
	return (strncmp(fname, "DMC-", 4) == 0 || strncmp(fname, "DME-", 4) == 0) &&
	       strncasecmp(fname + (strlen(fname) - 4), ".XML", 4) == 0;
}

/* Convenient structure for DM code properties. */
struct dm_code {
	char model_ident_code[15];
	char system_diff_code[5];
	char system_code[4];
	char sub_system_code[2];
	char sub_sub_system_code[2];
	char assy_code[5];
	char disassy_code[3];
	char disassy_code_variant[4];
	char info_code[4];
	char info_code_variant[2];
	char item_location_code[2];
	char learn_code[4];
	char learn_event_code[2];
};

/* Read a DM code from a given string. */
int parse_dmcode(struct dm_code *code, const char *str)
{
	int c;

	c = sscanf(str, "%*[^-]-%14[^-]-%4[^-]-%3[^-]-%1s%1s-%4[^-]-%2s%3[^-]-%3s%1s-%1s-%3s%1s",
		code->model_ident_code,
		code->system_diff_code,
		code->system_code,
		code->sub_system_code,
		code->sub_sub_system_code,
		code->assy_code,
		code->disassy_code,
		code->disassy_code_variant,
		code->info_code,
		code->info_code_variant,
		code->item_location_code,
		code->learn_code,
		code->learn_event_code);

	if (c != 11 && c != 13)
		return 1;
		
	return 0;
}

/* Test if the SNS directory exists for a specified code. */
int sns_exists(const char *code, char *dname)
{
	DIR *dir;
	struct dirent *cur;
	int exists = 0;

	dir = opendir(".");

	while ((cur = readdir(dir))) {
		if (strncmp(code, cur->d_name, strlen(code)) == 0) {
			exists = 1;
			strcpy(dname, cur->d_name);
			break;
		}
	}

	closedir(dir);

	return exists;
}

/* Place a link to a DM file in to the proper place in the SNS directory hierarchy. */
void placedm(const char *fname, struct dm_code *code, const char *snsdname)
{
	char path[PATH_MAX] = "../", orig[PATH_MAX], dname[256];

	if (chdir(snsdname) != 0) exit(1);

	if (sns_exists(code->system_code, dname)) {
		strcat(path, "../");
		if (chdir(dname) != 0) exit(1);

		if (sns_exists(code->sub_system_code, dname)) {
			strcat(path, "../");
			if (chdir(dname) != 0) exit(1);

			if (sns_exists(code->sub_sub_system_code, dname)) {
				strcat(path, "../");
				if (chdir(dname) != 0) exit(1);

				if (sns_exists(code->assy_code, dname)) {
					strcat(path, "../");
					if (chdir(dname) != 0) exit(1);
				}
			}
		}
	}
	
	strcpy(orig, path);

	strcat(path, fname);

	if (access(fname, F_OK) != -1) {
		unlink(fname);
	}

	if (linkfn(path, fname) != 0) exit(1);

	if (chdir(orig) != 0) exit(1);
}

/* Resort DMs in to the SNS directory hierarchy. */
void sort_sns(const char *snsdname)
{
	DIR *dir;
	struct dirent *cur;

	dir = opendir(".");

	while ((cur = readdir(dir))) {
		if (is_dmodule(cur->d_name)) {
			struct dm_code code;

			if (parse_dmcode(&code, cur->d_name) == 0) {
				placedm(cur->d_name, &code, snsdname);
			}
		}

	}

	closedir(dir);
}

/* Print or setup the SNS directory structure for a given BREX containing SNS rules. */
void print_or_setup_sns(const char *brex_fname, bool printsns, const char *snsdname)
{
	xmlDocPtr brex;
	xmlXPathContextPtr ctxt;
	xmlXPathObjectPtr results;
	xmlNodePtr sns_descr;

	if (!(brex = xmlReadFile(brex_fname, NULL, PARSE_OPTS))) {
		fprintf(stderr, ERR_PREFIX "Could not read BREX data module: %s\n", brex_fname);
		exit(1);
	}

	ctxt = xmlXPathNewContext(brex);

	results = xmlXPathEvalExpression(BAD_CAST "//snsRules/snsDescr", ctxt);

	sns_descr = results->nodesetval->nodeTab[0];

	if (printsns) {
		print_sns(sns_descr, -1);
	} else if (access(snsdname, F_OK) == -1) {
		setup_sns(sns_descr, snsdname);
	}

	xmlXPathFreeObject(results);
	xmlXPathFreeContext(ctxt);

	xmlFreeDoc(brex);

	if (!printsns)
		sort_sns(snsdname);
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [-d <dir>] [-npsh?] [<BREX> ...]");
	puts("");
	puts("Options:");
	puts("  -h -?      Show usage message.");
	puts("  -d <dir>   Directory to organize DMs in to. Default is '" DEFAULT_SNS_DNAME "'.");
	puts("  -n         Only use the SNS code to name directories.");
	puts("  -p         Print SNS instead of organizing.");
	puts("  -s         Use symbolic links.");
	puts("  --version  Show version information.");
	puts("  <BREX>     BREX data module to read SNS structure from.");
}

void show_version(void)
{
	printf("%s (s1kd-tools) %s\n", PROG_NAME, VERSION);
}

int main(int argc, char **argv)
{
	int i;
	bool printsns = false;
	char *snsdname = NULL;

	const char *sopts = "d:npsh?";
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
			case 'd': snsdname = strdup(optarg); break;
			case 's': linkfn = slink; break;
			case 'n': only_numb = true; break;
			case 'p': printsns = true; break;
			case 'h':
			case '?': show_help(); return 0;
		}
	}

	if (!snsdname) {
		snsdname = strdup(DEFAULT_SNS_DNAME);
	}

	if (optind < argc) {
		for (i = optind; i < argc; ++i) {
			print_or_setup_sns(argv[i], printsns, snsdname);
		}
	} else {
		print_or_setup_sns("-", printsns, snsdname);
	}

	free(snsdname);

	return 0;
}
