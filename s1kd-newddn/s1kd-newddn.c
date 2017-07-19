#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <libgen.h>

#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "templates.h"

#define PROG_NAME "s1kd-newddn"

#define ERR_PREFIX PROG_NAME ": ERROR: "

#define MODEL_IDENT_CODE_MAX	14	+ 2
#define CAGE_MAX		5	+ 2
#define YEAR_MAX		4	+ 2
#define SEQ_NO_MAX		5	+ 2

#define EXIT_DDN_EXISTS 1
#define EXIT_MALFORMED_CODE 2

void prompt(const char *prompt, char *str, int n)
{
	if (strcmp(str, "") == 0) {
		printf("%s: ", prompt);

		while (!strchr(fgets(str, n, stdin), '\n')) {
			fprintf(stderr, ERR_PREFIX "Bad input for \"%s\".\n", prompt);
			while (getchar() != '\n');
			printf("%s: ", prompt);
		}

		*(strchr(str, '\n')) = '\0';
	} else {
		char temp[512] = "";

		printf("%s [%s]: ", prompt, str);

		while (!strchr(fgets(temp, n, stdin), '\n')) {
			fprintf(stderr, ERR_PREFIX "Bad input for \"%s\".\n", prompt);
			while (getchar() != '\n');
			printf("%s [%s]:  ", prompt, str);
		}

		*(strchr(temp, '\n')) = '\0';

		if (strcmp(temp, "") != 0) {
			strncpy(str, temp, n);
		}
	}
}

void show_help(void)
{
	puts("Usage: " PROG_NAME " [options] <files>...");
	puts("");
	puts("Options:");
	puts("  -d <defaults>    Specify the 'defaults' file name.");
	puts("  -p               Prompt user for values.");
	puts("In addition, the following metadata can be set:");
	puts("  -# <code>        The DDN code (MIC-SENDER-RECEIVER-YEAR-SEQ)");
	puts("  -o <sender>      Sender enterprise name");
	puts("  -r <receiver>    Receiver enterprise name");
	puts("  -t <city>        Sender city");
	puts("  -T <city>        Receiver city");
	puts("  -n <country>     Sender country");
	puts("  -N <country>     Receiver country");
	puts("  -a <auth>        Authorization");
}

int matches_key_and_not_set(const char *key, const char *match, const char *var)
{
	return strcmp(key, match) == 0 && strcmp(var, "") == 0;
}

xmlNodePtr first_xpath_node(const char *expr, xmlXPathContextPtr ctxt)
{
	xmlXPathObjectPtr results;
	xmlNodePtr ret;

	results = xmlXPathEvalExpression(BAD_CAST expr, ctxt);
	
	if (xmlXPathNodeSetIsEmpty(results->nodesetval)) {
		ret = NULL;
	} else {
		ret = results->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(results);

	return ret;
}

void populate_list(xmlNodePtr deliv, int argc, char **argv, int i)
{
	for (; i < argc; ++i) {
		xmlNodePtr item = xmlNewChild(deliv, NULL, BAD_CAST "deliveryListItem", NULL);
		xmlNewChild(item, NULL, BAD_CAST "dispatchFileName", BAD_CAST basename(argv[i]));
	}
}

int main(int argc, char **argv)
{
	int c;

	FILE *defaults;
	char defaults_fname[PATH_MAX] = "defaults";
	char default_line[1024];
	char *def_key, *def_val;

	char ddncode[256] = "";

	int showprompts = 0;
	int skipcode = 0;

	char model_ident_code[MODEL_IDENT_CODE_MAX] = "";
	char sender_ident[CAGE_MAX] = "";
	char receiver_ident[CAGE_MAX] = "";
	char year_of_data_issue[YEAR_MAX] = "";
	char seq_number[SEQ_NO_MAX] = "";
	char sender[256] = "";
	char receiver[256] = "";
	char sender_city[256] = "";
	char receiver_city[256] = "";
	char sender_country[256] = "";
	char receiver_country[256] = "";
	char security_classification[4] = "";
	char authorization[256] = "";

	xmlDocPtr ddn;
	xmlNodePtr ddn_code;
	xmlNodePtr issue_date;
	xmlNodePtr sender_ent_name;
	xmlNodePtr receiver_ent_name;
	xmlNodePtr send_city, recv_city;
	xmlNodePtr send_country, recv_country;
	xmlNodePtr security;
	xmlNodePtr auth;
	xmlNodePtr delivery_list;

	xmlXPathContextPtr ctxt;

	time_t now;
	struct tm *local;
	int year, month, day;
	char year_s[5], month_s[3], day_s[3];

	char outfile[PATH_MAX];

	while ((c = getopt(argc, argv, "pd:#:c:o:r:t:n:T:N:a:h?")) != -1) {
		switch (c) {
			case 'p': showprompts = 1; break;
			case 'd': strncpy(defaults_fname, optarg, PATH_MAX - 1); break;
			case '#': strncpy(ddncode, optarg, 255); skipcode = 1; break;
			case 'o': strncpy(sender, optarg, 255); break;
			case 'r': strncpy(receiver, optarg, 255); break;
			case 't': strncpy(sender_city, optarg, 255); break;
			case 'n': strncpy(sender_country, optarg, 255); break;
			case 'T': strncpy(receiver_city, optarg, 255); break;
			case 'N': strncpy(receiver_country, optarg, 255); break;
			case 'a': strncpy(authorization, optarg, 255); break;
			case 'h':
			case '?': show_help(); exit(0);
		}
	}

	if (strcmp(ddncode, "") != 0) {
		int n;

		n = sscanf(ddncode, "%[^-]-%[^-]-%[^-]-%[^-]-%[^-]",
			model_ident_code,
			sender_ident,
			receiver_ident,
			year_of_data_issue,
			seq_number);

		if (n != 5) {
			fprintf(stderr, ERR_PREFIX "Bad DDN code.\n");
			exit(EXIT_MALFORMED_CODE);
		}
	}

	defaults = fopen(defaults_fname, "r");
	if (defaults) {
		while (fgets(default_line, 1024, defaults)) {
			def_key = strtok(default_line, "\t ");
			def_val = strtok(NULL, "\t\n");

			if (matches_key_and_not_set(def_key, "modelIdentCode", model_ident_code))
				strcpy(model_ident_code, def_val);
			if (matches_key_and_not_set(def_key, "senderIdent", sender_ident))
				strcpy(sender_ident, def_val);
			if (matches_key_and_not_set(def_key, "receiverIdent", receiver_ident))
				strcpy(receiver_ident, def_val);
			if (matches_key_and_not_set(def_key, "yearOfDataIssue", year_of_data_issue))
				strcpy(year_of_data_issue, def_val);
			if (matches_key_and_not_set(def_key, "seqNumber", seq_number))
				strcpy(seq_number, def_val);
			if (matches_key_and_not_set(def_key, "originator", sender))
				strcpy(sender, def_val);
			if (matches_key_and_not_set(def_key, "receiver", receiver))
				strcpy(receiver, def_val);
			if (matches_key_and_not_set(def_key, "city", sender_city))
				strcpy(sender_city, def_val);
			if (matches_key_and_not_set(def_key, "receiverCity", receiver_city))
				strcpy(receiver_city, def_val);
			if (matches_key_and_not_set(def_key, "country", sender_country))
				strcpy(sender_country, def_val);
			if (matches_key_and_not_set(def_key, "receiverCountry", receiver_country))
				strcpy(receiver_country, def_val);
			if (matches_key_and_not_set(def_key, "securityClassification", security_classification))
				strcpy(security_classification, def_val);
			if (matches_key_and_not_set(def_key, "authorization", authorization))
				strcpy(authorization, def_val);
		}

		fclose(defaults);
	}

	if (showprompts) {
		if (!skipcode) {
			prompt("Model identification code", model_ident_code, MODEL_IDENT_CODE_MAX);
			prompt("Sender ident", sender_ident, CAGE_MAX);
			prompt("Receiver ident", receiver_ident, CAGE_MAX);
			prompt("Year of data issue", year_of_data_issue, YEAR_MAX);
			prompt("Sequence number", seq_number, SEQ_NO_MAX);
		}
		prompt("Sender enterprise name", sender, 256);
		prompt("Sender city", sender_city, 256);
		prompt("Sender country", sender_country, 256);
		prompt("Receiver enterprise name", receiver, 256);
		prompt("Receiver city", receiver_city, 256);
		prompt("Receiver country", receiver_country, 256);
		prompt("Security classification", security_classification, 4);
		prompt("Authorization", authorization, 256);
	}

	if (strcmp(security_classification, "") == 0) strcpy(security_classification, "01");

	ddn = xmlReadMemory((const char *) templates_ddn_xml, templates_ddn_xml_len, NULL, NULL, 0);

	ctxt = xmlXPathNewContext(ddn);

	ddn_code = first_xpath_node("//ddnIdent/ddnCode", ctxt);
	xmlSetProp(ddn_code, BAD_CAST "modelIdentCode", BAD_CAST model_ident_code);
	xmlSetProp(ddn_code, BAD_CAST "senderIdent", BAD_CAST sender_ident);
	xmlSetProp(ddn_code, BAD_CAST "receiverIdent", BAD_CAST receiver_ident);
	xmlSetProp(ddn_code, BAD_CAST "yearOfDataIssue", BAD_CAST year_of_data_issue);
	xmlSetProp(ddn_code, BAD_CAST "seqNumber", BAD_CAST seq_number);

	issue_date = first_xpath_node("//ddnAddressItems/issueDate", ctxt);
	time(&now);
	local = localtime(&now);
	year = local->tm_year + 1900;
	month = local->tm_mon + 1;
	day = local->tm_mday;
	sprintf(year_s, "%d", year);
	sprintf(month_s, "%.2d", month);
	sprintf(day_s, "%.2d", day);
	xmlSetProp(issue_date, BAD_CAST "year", BAD_CAST year_s);
	xmlSetProp(issue_date, BAD_CAST "month", BAD_CAST month_s);
	xmlSetProp(issue_date, BAD_CAST "day", BAD_CAST day_s);

	sender_ent_name = first_xpath_node("//ddnAddressItems/dispatchFrom/dispatchAddress/enterprise/enterpriseName", ctxt);
	xmlNodeSetContent(sender_ent_name, BAD_CAST sender);

	receiver_ent_name = first_xpath_node("//ddnAddressItems/dispatchTo/dispatchAddress/enterprise/enterpriseName", ctxt);
	xmlNodeSetContent(receiver_ent_name, BAD_CAST receiver);

	send_city = first_xpath_node("//ddnAddressItems/dispatchFrom/dispatchAddress/address/city", ctxt);
	xmlNodeSetContent(send_city, BAD_CAST sender_city);

	recv_city = first_xpath_node("//ddnAddressItems/dispatchTo/dispatchAddress/address/city", ctxt);
	xmlNodeSetContent(recv_city, BAD_CAST receiver_city);

	send_country = first_xpath_node("//ddnAddressItems/dispatchFrom/dispatchAddress/address/country", ctxt);
	xmlNodeSetContent(send_country, BAD_CAST sender_country);

	recv_country = first_xpath_node("//ddnAddressItems/dispatchTo/dispatchAddress/address/country", ctxt);
	xmlNodeSetContent(recv_country, BAD_CAST receiver_country);

	security = first_xpath_node("//ddnStatus/security", ctxt);
	xmlSetProp(security, BAD_CAST "securityClassification", BAD_CAST security_classification);

	auth = first_xpath_node("//ddnStatus/authorization", ctxt);
	xmlNodeSetContent(auth, BAD_CAST authorization);

	delivery_list = first_xpath_node("//ddnContent/deliveryList", ctxt);

	xmlXPathFreeContext(ctxt);

	populate_list(delivery_list, argc, argv, optind);

	snprintf(outfile, PATH_MAX, "DDN-%s-%s-%s-%s-%s.XML",
		model_ident_code,
		sender_ident,
		receiver_ident,
		year_of_data_issue,
		seq_number);

	if (access(outfile, F_OK) != -1) {
		fprintf(stderr, ERR_PREFIX "%s already exists.\n", outfile);
		exit(EXIT_DDN_EXISTS);
	}

	xmlSaveFile(outfile, ddn);

	xmlFreeDoc(ddn);

	return 0;
}
