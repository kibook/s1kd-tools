#include <stdbool.h>
#include <libxml/tree.h>
#include <s1kd/brexcheck.h>
#include <s1kd/instance.h>
#include <s1kd/metadata.h>

void test_brexcheck(void)
{
	int err;
	xmlDocPtr doc = xmlReadFile("test.xml", NULL, 0);
	xmlDocPtr brex;
	char *report;
	int size;

	err = s1kdDocCheckDefaultBREX(doc, NULL);

	printf("Default BREX: %s\n", err ? "FAIL" : "PASS");

	brex = xmlReadFile("brex.xml", NULL, 0);

	err = s1kdDocCheckBREX(doc, brex, NULL);

	printf("Custom BREX: %s\n", err ? "FAIL" : "PASS");

	xmlFreeDoc(brex);
	xmlFreeDoc(doc);
}

void test_brexcheck_2(void)
{
	char *report;
	int size;

	s1kdCheckDefaultBREX("<root/>", 7, &report, &size);
	puts(report);
	free(report);

	s1kdCheckBREX("<root/>", 7, "<root/>", 7, &report, &size);
	puts(report);
	free(report);
}

void test_metadata(void)
{
	xmlDocPtr doc = xmlReadFile("test.xml", NULL, 0);
	xmlChar *date;

	date = s1kdGetMetadata(doc, BAD_CAST "issueDate");
	printf("DATE: %s\n", (char *) date);
	xmlFree(date);

	s1kdSetMetadata(doc, BAD_CAST "issueDate", BAD_CAST "1970-01-01");

	date = s1kdGetMetadata(doc, BAD_CAST "issueDate");
	printf("DATE: %s\n", (char *) date);
	xmlFree(date);

	xmlFreeDoc(doc);
}

void test_instance(void)
{
	xmlDocPtr doc = xmlReadFile("test.xml", NULL, 0);
	xmlDocPtr out;
	s1kdApplicDefs defs = s1kdNewApplicDefs();
	char *result;
	int size;

	s1kdAssign(defs, BAD_CAST "version", BAD_CAST "prodattr", BAD_CAST "A");

	out = s1kdDocFilter(doc, defs, S1KD_FILTER_REDUCE);

	xmlSaveFile("-", out);

	s1kdFreeApplicDefs(defs);
	xmlFreeDoc(doc);
	xmlFreeDoc(out);
}

void test_instance_2(void)
{
	s1kdApplicDefs defs = s1kdNewApplicDefs();
	char *result;
	int size;

	s1kdAssign(defs, BAD_CAST "version", BAD_CAST "prodattr", BAD_CAST "A");

	s1kdFilter("<root/>", 7, defs, S1KD_FILTER_DEFAULT, &result, &size);

	puts(result);

	free(result);
	s1kdFreeApplicDefs(defs);
}

int main()
{
	test_brexcheck();
	test_brexcheck_2();
	test_metadata();
	test_instance();
	test_instance_2();

	xmlCleanupParser();

	return 0;
}
