#include "libxpath2.h"
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlregexp.h>
#include <libxml/xmlstring.h>

static void xpath2AbsFunction(xmlXPathParserContextPtr ctx, int nargs)
{
	double arg1;

	if (nargs != 1) {
		xmlXPathSetArityError(ctx);
		return;
	}

	arg1 = xmlXPathPopNumber(ctx);

	xmlXPathReturnNumber(ctx, fabs(arg1));
}

static void xpath2CompareFunction(xmlXPathParserContextPtr ctx, int nargs)
{
	xmlChar *arg1, *arg2;
	int c;

	if (nargs != 2) {
		xmlXPathSetArityError(ctx);
		return;
	}

	arg2 = xmlXPathPopString(ctx);
	arg1 = xmlXPathPopString(ctx);

	c = xmlStrcmp(arg1, arg2);

	xmlFree(arg1);
	xmlFree(arg2);

	if (c < 0) {
		xmlXPathReturnNumber(ctx, -1);
	} else if (c > 0) {
		xmlXPathReturnNumber(ctx, 1);
	} else{
		xmlXPathReturnNumber(ctx, 0);
	}
}

static void xpath2EndsWithFunction(xmlXPathParserContextPtr ctx, int nargs)
{
	xmlChar *arg1, *arg2;
	int len1, len2;

	if (nargs != 2) {
		xmlXPathSetArityError(ctx);
		return;
	}

	arg2 = xmlXPathPopString(ctx);
	arg1 = xmlXPathPopString(ctx);

	len1 = xmlStrlen(arg1);
	len2 = xmlStrlen(arg2);

	if (len1 < len2) {
		xmlXPathReturnBoolean(ctx, false);
	} else {
		xmlXPathReturnBoolean(ctx, xmlStrcmp(arg1 + (len1 - len2), arg2) == 0);
	}
}

static void lowercase(xmlChar *s)
{
	int i;

	for (i = 0; s[i]; ++i) {
		s[i] = tolower(s[i]);
	}
}

static void uppercase(xmlChar *s)
{
	int i;

	for (i = 0; s[i]; ++i) {
		s[i] = toupper(s[i]);
	}
}

static void xpath2LowerCaseFunction(xmlXPathParserContextPtr ctx, int nargs)
{
	xmlChar *arg1;

	if (nargs != 1) {
		xmlXPathSetArityError(ctx);
		return;
	}

	arg1 = xmlXPathPopString(ctx);

	lowercase(arg1);

	xmlXPathReturnString(ctx, arg1);
}

static bool matches(const xmlChar *input, const xmlChar *pattern, const xmlChar *flags)
{
	xmlRegexpPtr regex;
	bool ret;
	xmlChar *i, *p;

	i = xmlStrdup(input);
	p = xmlStrdup(pattern);

	if (xmlStrchr(flags, 'i')) {
		lowercase(i);
		lowercase(p);
	}

	regex = xmlRegexpCompile(p);
	ret = xmlRegexpExec(regex, i);
	xmlRegFreeRegexp(regex);

	xmlFree(i);
	xmlFree(p);

	return ret;
}

static void xpath2MatchesFunction(xmlXPathParserContextPtr ctx, int nargs)
{
	xmlChar *arg1, *arg2, *arg3;

	if (nargs < 2 || nargs > 3) {
		xmlXPathSetArityError(ctx);
		return;
	}

	if (nargs > 2) {
		arg3 = xmlXPathPopString(ctx);
	} else {
		arg3 = xmlCharStrdup("");
	}

	arg2 = xmlXPathPopString(ctx);
	arg1 = xmlXPathPopString(ctx);

	xmlXPathReturnBoolean(ctx, matches(arg1, arg2, arg3));

	xmlFree(arg1);
	xmlFree(arg2);
	xmlFree(arg3);
}

static xmlNodePtr newTextNode(xmlXPathParserContextPtr ctx, const xmlChar *val)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	doc = ctx->context->doc;
	node = xmlNewDocRawNode(doc, NULL, BAD_CAST "token", val);
	xmlAddChild((xmlNodePtr) doc, node);
	return node;
}

static void xpath2TokenizeFunction(xmlXPathParserContextPtr ctx, int nargs)
{
	xmlChar *arg1, *arg2, *arg3;
	xmlXPathObjectPtr obj;
	int sep_start;
	xmlChar *cur;

	if (nargs < 1 || nargs > 3) {
		xmlXPathSetArityError(ctx);
		return;
	}

	if (nargs > 2) {
		arg3 = xmlXPathPopString(ctx);
	} else {
		arg3 = xmlCharStrdup("");
	}

	if (nargs > 1) {
		arg2 = xmlXPathPopString(ctx);
	} else {
		arg2 = xmlCharStrdup(" ");
	}

	arg1 = xmlXPathPopString(ctx);

	obj = xmlXPathNewNodeSet(NULL);

	cur = arg1;

	if (nargs < 2) {
		while (isspace(cur[0])) ++cur;
	}

	sep_start = 0;

	while (cur[sep_start] != '\0') {
		int sep_end = xmlStrlen(cur);

		while (sep_end > 0 && cur[sep_end - 1] != '\0') {
			xmlChar *sep   = xmlStrsub(cur, sep_start, sep_end - sep_start);
			xmlChar *token = xmlStrsub(cur, 0, sep_start);

			if (sep && matches(sep, arg2, arg3)) {
				xmlXPathNodeSetAddUnique(obj->nodesetval, newTextNode(ctx, token));

				cur = cur + sep_start + xmlStrlen(sep);
				sep_start = -1;

				xmlFree(sep);
				xmlFree(token);
				break;
			}

			--sep_end;

			xmlFree(sep);
			xmlFree(token);
		}

		++sep_start;
	}

	if (nargs < 2) {
		int i;
		xmlChar *sub;

		for (i = 0; cur[i] != '\0' && !isspace(cur[i]); ++i);

		if ((sub = xmlStrsub(cur, 0, i))) {
			xmlXPathNodeSetAddUnique(obj->nodesetval, newTextNode(ctx, sub));
		}

		xmlFree(sub);
	} else {
		xmlXPathNodeSetAddUnique(obj->nodesetval, newTextNode(ctx, cur));
	}

	xmlXPathReturnNodeSet(ctx, obj->nodesetval);

	xmlXPathFreeNodeSetList(obj);

	xmlFree(arg1);
	xmlFree(arg2);
	xmlFree(arg3);
}

static void xpath2UpperCaseFunction(xmlXPathParserContextPtr ctx, int nargs)
{
	xmlChar *arg1;

	if (nargs != 1) {
		xmlXPathSetArityError(ctx);
		return;
	}

	arg1 = xmlXPathPopString(ctx);

	uppercase(arg1);

	xmlXPathReturnString(ctx, arg1);
}

void xpath2RegisterFunctions(xmlXPathContextPtr ctx)
{
	xmlXPathRegisterFunc(ctx, BAD_CAST "abs", xpath2AbsFunction);
	xmlXPathRegisterFunc(ctx, BAD_CAST "compare", xpath2CompareFunction);
	xmlXPathRegisterFunc(ctx, BAD_CAST "ends-with", xpath2EndsWithFunction);
	xmlXPathRegisterFunc(ctx, BAD_CAST "lower-case", xpath2LowerCaseFunction);
	xmlXPathRegisterFunc(ctx, BAD_CAST "matches", xpath2MatchesFunction);
	xmlXPathRegisterFunc(ctx, BAD_CAST "upper-case", xpath2UpperCaseFunction);
	xmlXPathRegisterFunc(ctx, BAD_CAST "tokenize", xpath2TokenizeFunction);
}
