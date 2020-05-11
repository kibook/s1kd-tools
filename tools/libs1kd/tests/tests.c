#include <stdbool.h>
#include <string.h>
#include <libxml/tree.h>
#include <s1kd/brexcheck.h>
#include <s1kd/instance.h>
#include <s1kd/metadata.h>

void test_brexcheck(void)
{
	int err;
	xmlDocPtr doc = xmlReadFile("test.xml", NULL, 0);
	xmlDocPtr brex;
	xmlDocPtr report;
	int opts;

	opts = S1KD_BREXCHECK_VALUES | S1KD_BREXCHECK_SNS | S1KD_BREXCHECK_STRICT_SNS | S1KD_BREXCHECK_NOTATIONS;

	err = s1kdDocCheckDefaultBREX(doc, opts, &report);
	printf("Default BREX: %s\n", err ? "FAIL" : "PASS");
	xmlSaveFile("-", report);
	xmlFreeDoc(report);

	brex = xmlReadFile("brex.xml", NULL, 0);
	err = s1kdDocCheckBREX(doc, brex, opts, &report);
	printf("Custom BREX: %s\n", err ? "FAIL" : "PASS");
	xmlSaveFile("-", report);
	xmlFreeDoc(report);
	xmlFreeDoc(brex);

	xmlFreeDoc(doc);
}

void test_brexcheck_2(void)
{
	char *report;
	int size;
	int opts;

	opts = S1KD_BREXCHECK_VALUES;

	s1kdCheckDefaultBREX("<root/>", 7, opts, &report, &size);
	puts(report);
	free(report);

	s1kdCheckBREX("<root/>", 7, "<root/>", 7, opts, &report, &size);
	puts(report);
	free(report);
}

void test_metadata(void)
{
	xmlDocPtr doc = xmlReadFile("test.xml", NULL, 0);
	xmlChar *date;

	date = s1kdDocGetMetadata(doc, BAD_CAST "issueDate");
	printf("DATE: %s\n", (char *) date);
	xmlFree(date);

	s1kdDocSetMetadata(doc, BAD_CAST "issueDate", BAD_CAST "1970-01-01");

	date = s1kdDocGetMetadata(doc, BAD_CAST "issueDate");
	printf("DATE: %s\n", (char *) date);
	xmlFree(date);

	xmlFreeDoc(doc);
}

void test_metadata_2(void)
{
	char *xml;
	char *date;
	char *result;
	int size;

	xml = "<issueDate year=\"2020\" month=\"05\" day=\"01\"/>";

	date = s1kdGetMetadata(xml, strlen(xml), "issueDate");
	printf("DATE: %s\n", (char *) date);
	free(date);

	s1kdSetMetadata(xml, strlen(xml), "issueDate", "1970-01-01", &result, &size);

	date = s1kdGetMetadata(result, size, "issueDate");
	printf("DATE: %s\n", (char *) date);
	free(date);

	free(result);
}

void test_instance(void)
{
	xmlDocPtr doc = xmlReadFile("test.xml", NULL, 0);
	xmlDocPtr out;
	s1kdApplicability app = s1kdNewApplicability();
	char *result;
	int size;

	s1kdAssign(app, BAD_CAST "version", BAD_CAST "prodattr", BAD_CAST "A");

	out = s1kdDocFilter(doc, app, S1KD_FILTER_DEFAULT);
	xmlSaveFile("-", out);
	xmlFreeDoc(out);

	out = s1kdDocFilter(doc, app, S1KD_FILTER_REDUCE);
	xmlSaveFile("-", out);
	xmlFreeDoc(out);

	out = s1kdDocFilter(doc, app, S1KD_FILTER_SIMPLIFY);
	xmlSaveFile("-", out);
	xmlFreeDoc(out);

	out = s1kdDocFilter(doc, app, S1KD_FILTER_PRUNE);
	xmlSaveFile("-", out);
	xmlFreeDoc(out);

	s1kdFreeApplicability(app);
	xmlFreeDoc(doc);
}

void test_instance_2(void)
{
	s1kdApplicability app = s1kdNewApplicability();
	char *result;
	int size;

	s1kdAssign(app, BAD_CAST "version", BAD_CAST "prodattr", BAD_CAST "A");

	s1kdFilter("<root/>", 7, app, S1KD_FILTER_DEFAULT, &result, &size);

	puts(result);

	free(result);
	s1kdFreeApplicability(app);
}

int main()
{
	test_brexcheck();
	test_brexcheck_2();
	test_metadata();
	test_metadata_2();
	test_instance();
	test_instance_2();

	xmlCleanupParser();

	return 0;
}
