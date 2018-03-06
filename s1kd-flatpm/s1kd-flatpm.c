#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <ctype.h>
#include <libxml/tree.h>

int xinclude = 0;
int no_issue = 0;

int use_pub_fmt = 0;
xmlDocPtr pub_doc = NULL;
xmlNodePtr pub;

void show_help(void)
{
	puts("Usage: s1kd-flatpm [-Npxh?] <pubmodule>");
	puts("");
	puts("Options:");
	puts("  -N     Assume issue/inwork numbers are omitted.");
	puts("  -x     Use XInclude references.");
	puts("  -p     Output a 'publication' XML file.");
	puts("  -h -?  Show help/usage message.");
}

xmlNodePtr find_child(xmlNodePtr parent, const char *child_name)
{
	xmlNodePtr cur;

	for (cur = parent->children; cur; cur = cur->next) {
		if (strcmp((char *) cur->name, child_name) == 0) {
			return cur;
		}
	}

	return NULL;
}

bool is_dm(const char *fname)
{
	return strncmp(fname, "DMC-", 4) == 0 && strncasecmp(fname + (strlen(fname) - 4), ".XML", 4) == 0;
}

char *filesystem_dm_fname(char *fs_dm_fname, const char *dm_fname)
{
	DIR *dir;
	struct dirent *cur;

	dir = opendir(".");

	while ((cur = readdir(dir))) {
		if (is_dm(cur->d_name) && strncmp(cur->d_name, dm_fname, strlen(dm_fname)) == 0) {
			strcpy(fs_dm_fname, cur->d_name);
			break;
		}
	}

	closedir(dir);

	return fs_dm_fname;
}

bool is_pm(const char *fname)
{
	return strncmp(fname, "PMC-", 4) == 0 && strncasecmp(fname + (strlen(fname) - 4), ".XML", 4) == 0;
}

char *filesystem_pm_fname(char *fs_pm_fname, const char *pm_fname)
{
	DIR *dir;
	struct dirent *cur;

	dir = opendir(".");

	while ((cur = readdir(dir))) {
		if (is_pm(cur->d_name) && strncmp(cur->d_name, pm_fname, strlen(pm_fname)) == 0) {
			strcpy(fs_pm_fname, cur->d_name);
			break;
		}
	}

	closedir(dir);

	return fs_pm_fname;
}

void flatten_pm_ref(xmlNodePtr pm_ref)
{
	xmlNodePtr pm_ref_ident;
	xmlNodePtr pm_code;
	xmlNodePtr issue_info;
	xmlNodePtr language;

	char *model_ident_code;
	char *pm_issuer;
	char *pm_number;
	char *pm_volume;
	char *issue_number = NULL;
	char *in_work = NULL;
	char *language_iso_code = NULL;
	char *country_iso_code = NULL;

	char pmc[256];
	char pm_fname[256];
	char pm_fname_temp[256];
	char fs_pm_fname[256] = "";

	xmlNodePtr xi;
	xmlDocPtr doc;
	xmlNodePtr pm;

	pm_ref_ident = find_child(pm_ref, "pmRefIdent");
	pm_code = find_child(pm_ref_ident, "pmCode");
	issue_info = find_child(pm_ref_ident, "issueInfo");
	language = find_child(pm_ref_ident, "language");

	model_ident_code = (char *) xmlGetProp(pm_code, BAD_CAST "modelIdentCode");
	pm_issuer        = (char *) xmlGetProp(pm_code, BAD_CAST "pmIssuer");
	pm_number        = (char *) xmlGetProp(pm_code, BAD_CAST "pmNumber");
	pm_volume        = (char *) xmlGetProp(pm_code, BAD_CAST "pmVolume");

	snprintf(pmc, 256, "%s-%s-%s-%s",
		model_ident_code,
		pm_issuer,
		pm_number,
		pm_volume);

	snprintf(pm_fname, 256, "PMC-%s", pmc);

	if (issue_info && !no_issue) {
		issue_number = (char *) xmlGetProp(issue_info, BAD_CAST "issueNumber");
		in_work = (char *) xmlGetProp(issue_info, BAD_CAST "inWork");
		strcpy(pm_fname_temp, pm_fname);
		snprintf(pm_fname, 256, "%s_%s-%s", pm_fname_temp, issue_number, in_work);
	}

	if (language) {
		int i;

		language_iso_code = (char *)  xmlGetProp(language, BAD_CAST "languageIsoCode");
		country_iso_code = (char *) xmlGetProp(language, BAD_CAST "countryIsoCode");
	
		for (i = 0; language_iso_code[i]; ++i)
			language_iso_code[i] = toupper(language_iso_code[i]);
		strcpy(pm_fname_temp, pm_fname);
		snprintf(pm_fname, 256, "%s_%s-%s", pm_fname_temp, language_iso_code, country_iso_code);
	}

	xmlFree(model_ident_code);
	xmlFree(pm_issuer);
	xmlFree(pm_number);
	xmlFree(pm_volume);
	if(issue_info && !no_issue) {
		xmlFree(issue_number);
		xmlFree(in_work);
	}
	if (language) {
		xmlFree(language_iso_code);
		xmlFree(country_iso_code);
	}

	if (strcmp(filesystem_pm_fname(fs_pm_fname, pm_fname), "") != 0) {
		if (xinclude ) {
			xi = xmlNewNode(NULL, BAD_CAST "xi:include");
			xmlSetProp(xi, BAD_CAST "href", BAD_CAST fs_pm_fname);

			if (use_pub_fmt) {
				xi = xmlAddChild(pub, xi);
			} else {
				xi = xmlAddPrevSibling(pm_ref, xi);
			}
		} else {
			doc = xmlReadFile(fs_pm_fname, NULL, XML_PARSE_NONET);
			pm = xmlDocGetRootElement(doc);
			xmlAddPrevSibling(pm_ref, xmlCopyNode(pm, 1));
			xmlFreeDoc(doc);
		}
	}

	xmlUnlinkNode(pm_ref);
	xmlFreeNode(pm_ref);
}

void flatten_dm_ref(xmlNodePtr dm_ref)
{
	xmlNodePtr dm_ref_ident;
	xmlNodePtr dm_code;
	xmlNodePtr issue_info;
	xmlNodePtr language;

	char *model_ident_code;
	char *system_diff_code;
	char *system_code;
	char *sub_system_code;
	char *sub_sub_system_code;
	char *assy_code;
	char *disassy_code;
	char *disassy_code_variant;
	char *info_code;
	char *info_code_variant;
	char *item_location_code;
	char *issue_number = NULL;
	char *in_work = NULL;
	char *language_iso_code;
	char *country_iso_code;

	char dmc[256];
	char dm_fname[256];
	char dm_fname_temp[256];
	char fs_dm_fname[256] = "";

	xmlNodePtr xi;
	xmlDocPtr doc;
	xmlNodePtr dmodule;

	dm_ref_ident = find_child(dm_ref, "dmRefIdent");
	dm_code = find_child(dm_ref_ident, "dmCode");
	issue_info = find_child(dm_ref_ident, "issueInfo");
	language = find_child(dm_ref_ident, "language");

	model_ident_code     = (char *) xmlGetProp(dm_code, BAD_CAST "modelIdentCode");
	system_diff_code     = (char *) xmlGetProp(dm_code, BAD_CAST "systemDiffCode");
	system_code          = (char *) xmlGetProp(dm_code, BAD_CAST "systemCode");
	sub_system_code      = (char *) xmlGetProp(dm_code, BAD_CAST "subSystemCode");
	sub_sub_system_code  = (char *) xmlGetProp(dm_code, BAD_CAST "subSubSystemCode");
	assy_code            = (char *) xmlGetProp(dm_code, BAD_CAST "assyCode");
	disassy_code         = (char *) xmlGetProp(dm_code, BAD_CAST "disassyCode");
	disassy_code_variant = (char *) xmlGetProp(dm_code, BAD_CAST "disassyCodeVariant");
	info_code            = (char *) xmlGetProp(dm_code, BAD_CAST "infoCode");
	info_code_variant    = (char *) xmlGetProp(dm_code, BAD_CAST "infoCodeVariant");
	item_location_code   = (char *) xmlGetProp(dm_code, BAD_CAST "itemLocationCode");

	snprintf(dmc, 256, "%s-%s-%s-%s%s-%s-%s%s-%s%s-%s",
		model_ident_code,
		system_diff_code,
		system_code,
		sub_system_code,
		sub_sub_system_code,
		assy_code,
		disassy_code,
		disassy_code_variant,
		info_code,
		info_code_variant,
		item_location_code);

	snprintf(dm_fname, 256, "DMC-%s", dmc);

	if (issue_info && !no_issue) {
		issue_number = (char *) xmlGetProp(issue_info, BAD_CAST "issueNumber");
		in_work = (char *) xmlGetProp(issue_info, BAD_CAST "inWork");
		strcpy(dm_fname_temp, dm_fname);
		snprintf(dm_fname, 256, "%s_%s-%s", dm_fname_temp, issue_number, in_work);
	}

	if (language) {
		int i;

		language_iso_code = (char *)  xmlGetProp(language, BAD_CAST "languageIsoCode");
		country_iso_code = (char *) xmlGetProp(language, BAD_CAST "countryIsoCode");
	
		for (i = 0; language_iso_code[i]; ++i)
			language_iso_code[i] = toupper(language_iso_code[i]);
		strcpy(dm_fname_temp, dm_fname);
		snprintf(dm_fname, 256, "%s_%s-%s", dm_fname_temp, language_iso_code, country_iso_code);
	}

	xmlFree(model_ident_code);
	xmlFree(system_diff_code);
	xmlFree(system_code);
	xmlFree(sub_system_code);
	xmlFree(sub_sub_system_code);
	xmlFree(assy_code);
	xmlFree(disassy_code);
	xmlFree(disassy_code_variant);
	xmlFree(info_code);
	xmlFree(info_code_variant);
	xmlFree(item_location_code);
	if (issue_info && !no_issue) {
		xmlFree(issue_number);
		xmlFree(in_work);
	}
	if (language) {
		xmlFree(language_iso_code);
		xmlFree(country_iso_code);
	}

	if (strcmp(filesystem_dm_fname(fs_dm_fname, dm_fname), "") != 0) {
		if (xinclude) {
			xi = xmlNewNode(NULL, BAD_CAST "xi:include");
			xmlSetProp(xi, BAD_CAST "href", BAD_CAST fs_dm_fname);

			if (use_pub_fmt) {
				xi = xmlAddChild(pub, xi);
			} else {
				xi = xmlAddPrevSibling(dm_ref, xi);
			}
		} else {
			doc = xmlReadFile(fs_dm_fname, NULL, XML_PARSE_NONET);
			dmodule = xmlDocGetRootElement(doc);
			xmlAddPrevSibling(dm_ref, xmlCopyNode(dmodule, 1));
			xmlFreeDoc(doc);
		}
	}

	xmlUnlinkNode(dm_ref);
	xmlFreeNode(dm_ref);
}

void flatten_pm_entry(xmlNodePtr pm_entry)
{
	xmlNodePtr cur, next;

	cur = pm_entry->children;

	while (cur) {
		next = cur->next;

		if (strcmp((char *) cur->name, "dmRef") == 0) {
			flatten_dm_ref(cur);
		} else if (strcmp((char *) cur->name, "pmRef") == 0) {
			flatten_pm_ref(cur);
		} else if (strcmp((char *) cur->name, "pmEntry") == 0) {
			flatten_pm_entry(cur);
		}

		cur = next;
	}
}

int main(int argc, char **argv)
{
	int c;

	char *pm_fname;

	xmlDocPtr pm_doc;

	xmlNodePtr pm;
	xmlNodePtr content;

	xmlNodePtr cur;

	while ((c = getopt(argc, argv, "xNph?")) != -1) {
		switch (c) {
			case 'x': xinclude = 1; break;
			case 'N': no_issue = 1; break;
			case 'p': use_pub_fmt = 1; xinclude = 1; break;
			case 'h':
			case '?': show_help(); exit(0);
		}
	}

	pm_fname = argv[optind];

	pm_doc = xmlReadFile(pm_fname, NULL, 0);

	pm = xmlDocGetRootElement(pm_doc);

	if (xinclude && !use_pub_fmt) {
		xmlSetProp(pm, BAD_CAST "xmlns:xi", BAD_CAST "http://www.w3.org/2001/XInclude");
	}

	content = find_child(pm, "content");

	if (use_pub_fmt) {
		xmlNodePtr xi;

		pub_doc = xmlNewDoc(BAD_CAST "1.0");
		pub = xmlNewNode(NULL, BAD_CAST "publication");
		xmlDocSetRootElement(pub_doc, pub);
		xmlSetProp(pub, BAD_CAST "xmlns:xi", BAD_CAST "http://www.w3.org/2001/XInclude");
		xi = xmlNewChild(pub, NULL, BAD_CAST "xi:include", NULL);
		xmlSetProp(xi, BAD_CAST "href", BAD_CAST pm_fname);
	}

	if (optind == argc - 1 || !use_pub_fmt) {
		for (cur = content->children; cur; cur = cur->next) {
			if (strcmp((char *) cur->name, "pmEntry") == 0) {
				flatten_pm_entry(cur);
			}
		}
	} else if (use_pub_fmt) {
		int i;

		for (i = optind + 1; i < argc; ++i) {
			xmlNodePtr xi;

			xi = xmlNewChild(pub, NULL, BAD_CAST "xi:include", NULL);
			xmlSetProp(xi, BAD_CAST "href", BAD_CAST argv[i]);
		}
	}

	xmlSaveFormatFile("-", use_pub_fmt ? pub_doc : pm_doc, 1);

	xmlFreeDoc(pm_doc);

	return 0;
}
