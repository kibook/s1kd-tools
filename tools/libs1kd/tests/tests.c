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

	err = s1kdCheckDefaultBREX("<root/>", 7, &report, &size);
	puts(report);
	free(report);

	err = s1kdCheckBREX("<root/>", 7, "<root/>", 7, &report, &size);
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

	s1kdAssign(defs, BAD_CAST "version", BAD_CAST "prodattr", BAD_CAST "A");

	out = s1kdFilter(doc, defs, true);

	xmlSaveFile("-", out);

	s1kdFreeApplicDefs(defs);
	xmlFreeDoc(doc);
	xmlFreeDoc(out);
}

int main()
{
	test_brexcheck();
	test_metadata();
	test_instance();

	xmlCleanupParser();

	return 0;
}
