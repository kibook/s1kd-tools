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

	err = s1kdCheckDefaultBREX(doc);

	printf("Default BREX: %s\n", err ? "FAIL" : "PASS");

	brex = xmlReadFile("brex.xml", NULL, 0);

	err = s1kdCheckBREX(doc, brex);

	printf("Custom BREX: %s\n", err ? "FAIL" : "PASS");

	xmlFreeDoc(brex);
	xmlFreeDoc(doc);
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
	xmlNodePtr defs = xmlNewNode(NULL, BAD_CAST "applic");
	xmlNodePtr a;

	a = xmlNewChild(defs, NULL, BAD_CAST "assert", NULL);
	xmlSetProp(a, BAD_CAST "applicPropertyIdent", BAD_CAST "version");
	xmlSetProp(a, BAD_CAST "applicPropertyType", BAD_CAST "prodattr");
	xmlSetProp(a, BAD_CAST "applicPropertyValues", BAD_CAST "A");

	out = s1kdFilter(doc, defs, true);

	xmlSaveFile("-", out);

	xmlFreeNode(defs);
	xmlFreeDoc(doc);
	xmlFreeDoc(out);
}

int main()
{
	test_brexcheck();
	test_metadata();
	test_instance();

	xmlCleanupParser();
}
