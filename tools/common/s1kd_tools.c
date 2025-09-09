#include "s1kd_tools.h"

#define PROGRESS_BAR_WIDTH 60

/* Default global XML parsing options.
 *
 * XML_PARSE_NOERROR / XML_PARSE_NOWARNING
 *   Suppress the error logging built-in to the parser. The tools will handle
 *   reporting errors.
 *
 * XML_PARSE_NONET
 *   Disable network access as a safety precaution.
 */
int DEFAULT_PARSE_OPTS = XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET;

/* Return the full path name from a relative path. */
char *real_path(const char *path, char *real)
{
	#ifdef _WIN32
	if (!GetFullPathName(path, PATH_MAX, real, NULL)) {
	#else
	if (!realpath(path, real)) {
	#endif
		strcpy(real, path);
	}
	return real;
}

/* Search up the directory tree to find a configuration file. */
bool find_config(char *dst, const char *name)
{
	char cwd[PATH_MAX], prev[PATH_MAX];
	bool found = true;

	real_path(".", cwd);
	strcpy(prev, cwd);

	while (access(name, F_OK) == -1) {
		char cur[PATH_MAX];

		if (chdir("..") || strcmp(real_path(".", cur), prev) == 0) {
			found = false;
			break;
		}

		strcpy(prev, cur);
	}

	if (found) {
		real_path(name, dst);
	} else {
		strcpy(dst, name);
	}

	return chdir(cwd) == 0 && found;
}

/* Generate an XPath expression for a node. */
xmlChar *xpath_of(xmlNodePtr node)
{
	xmlNodePtr path, cur;
	xmlChar *dst = NULL;

	path = xmlNewNode(NULL, BAD_CAST "path");

	/* Build XPath expression node by traversing up the tree. */
	while (node && node->type != XML_DOCUMENT_NODE) {
		xmlNodePtr e;
		const xmlChar *name;

		e = xmlNewChild(path, NULL, BAD_CAST "node", NULL);

		switch (node->type) {
			case XML_COMMENT_NODE:
				name = BAD_CAST "comment()";
				break;
			case XML_PI_NODE:
				name = BAD_CAST "processing-instruction()";
				break;
			case XML_TEXT_NODE:
				name = BAD_CAST "text()";
				break;
			default:
				name = node->name;
				break;
		}

		if (node->ns != NULL) {
			xmlSetProp(e, BAD_CAST "ns", node->ns->prefix);
		}
		xmlSetProp(e, BAD_CAST "name", name);

		/* Locate the node's position within its parent. */
		if (node->type != XML_ATTRIBUTE_NODE) {
			int n = 1;
			xmlChar pos[16];

			for (cur = node->parent->children; cur; cur = cur->next) {
				if (cur == node) {
					break;
				} else if (cur->type == node->type && (node->type != XML_ELEMENT_NODE || xmlStrcmp(cur->name, node->name) == 0)) {
					++n;
				}
			}

			xmlStrPrintf(pos, 16, "%d", n);
			xmlSetProp(e, BAD_CAST "pos", pos);
		}

		node = node->parent;
	}

	/* Convert XPath expression node to string. */
	for (cur = path->last; cur; cur = cur->prev) {
		xmlChar *ns, *name, *pos;

		ns   = xmlGetProp(cur, BAD_CAST "ns");
		name = xmlGetProp(cur, BAD_CAST "name");
		pos  = xmlGetProp(cur, BAD_CAST "pos");

		dst = xmlStrcat(dst, BAD_CAST "/");
		if (!pos) {
			dst = xmlStrcat(dst, BAD_CAST "@");
		}
		if (ns) {
			dst = xmlStrcat(dst, ns);
			dst = xmlStrcat(dst, BAD_CAST ":");
		}
		dst = xmlStrcat(dst, name);
		if (pos) {
			dst = xmlStrcat(dst, BAD_CAST "[");
			dst = xmlStrcat(dst, pos);
			dst = xmlStrcat(dst, BAD_CAST "]");
		}

		xmlFree(ns);
		xmlFree(name);
		xmlFree(pos);
	}

	xmlFreeNode(path);

	return dst;
}

/* Make a copy of a file. */
int copy(const char *from, const char *to)
{

	FILE *f1, *f2;
	char buf[4096];
	size_t n;
	struct stat sf, st;

	if (stat(from, &sf) == -1) {
		return 1;
	}
	if (stat(to, &st) == 0 && sf.st_dev == st.st_dev && sf.st_ino == st.st_ino) {
		return 1;
	}

	f1 = fopen(from, "rb");
	f2 = fopen(to, "wb");

	while ((n = fread(buf, 1, 4096, f1)) > 0) {
		fwrite(buf, 1, n, f2);
	}

	fclose(f1);
	fclose(f2);

	return 0;
}

/* Determine if path is a directory. */
bool isdir(const char *path, bool recursive)
{
	struct stat st;
	char s[PATH_MAX], *b;

	strcpy(s, path);
	b = basename(s);

	if (recursive && (strcmp(b, ".") == 0 || strcmp(b, "..") == 0)) {
		return false;
	}

	if (stat(path, &st) != 0) {
		return false;
	}

	return S_ISDIR(st.st_mode);
}

/* Not exposed by the libxml API < 2.12.0 */
#if LIBXML_VERSION < 21200
void xmlFreeEntity(xmlEntityPtr entity)
{
    xmlDictPtr dict = NULL;

    if (entity == NULL)
        return;

    if (entity->doc != NULL)
        dict = entity->doc->dict;


    if ((entity->children) && (entity->owner == 1) &&
        (entity == (xmlEntityPtr) entity->children->parent))
        xmlFreeNodeList(entity->children);
    if (dict != NULL) {
        if ((entity->name != NULL) && (!xmlDictOwns(dict, entity->name)))
            xmlFree((char *) entity->name);
        if ((entity->ExternalID != NULL) &&
	    (!xmlDictOwns(dict, entity->ExternalID)))
            xmlFree((char *) entity->ExternalID);
        if ((entity->SystemID != NULL) &&
	    (!xmlDictOwns(dict, entity->SystemID)))
            xmlFree((char *) entity->SystemID);
        if ((entity->URI != NULL) && (!xmlDictOwns(dict, entity->URI)))
            xmlFree((char *) entity->URI);
        if ((entity->content != NULL)
            && (!xmlDictOwns(dict, entity->content)))
            xmlFree((char *) entity->content);
        if ((entity->orig != NULL) && (!xmlDictOwns(dict, entity->orig)))
            xmlFree((char *) entity->orig);
    } else {
        if (entity->name != NULL)
            xmlFree((char *) entity->name);
        if (entity->ExternalID != NULL)
            xmlFree((char *) entity->ExternalID);
        if (entity->SystemID != NULL)
            xmlFree((char *) entity->SystemID);
        if (entity->URI != NULL)
            xmlFree((char *) entity->URI);
        if (entity->content != NULL)
            xmlFree((char *) entity->content);
        if (entity->orig != NULL)
            xmlFree((char *) entity->orig);
    }
    xmlFree(entity);
}
#endif

/* Compare the codes of two CSDB objects. */
static int codecmp(const char *p1, const char *p2)
{
	char s1[PATH_MAX], s2[PATH_MAX], *b1, *b2;

	strcpy(s1, p1);
	strcpy(s2, p2);

	b1 = basename(s1);
	b2 = basename(s2);

	return strcasecmp(b1, b2);
}

/* Match a string with a pattern case-insensitively, using ? as a wildcard. */
bool strmatch(const char *p, const char *s)
{
	const unsigned char *cp = (const unsigned char *) p;
	const unsigned char *cs = (const unsigned char *) s;

	while (*cp) {
		if (tolower(*cp) != tolower(*cs) && *cp != '?') {
			return false;
		}
		++cp;
		++cs;
	}

	return true;
}

/* Match a string with a pattern case-insensitvely, using ? as a wildcard, up to a certain length. */
bool strnmatch(const char *p, const char *s, int n)
{
	const unsigned char *cp = (const unsigned char *) p;
	const unsigned char *cs = (const unsigned char *) s;
	int i = 0;

	while (*cp && i < n) {
		if (tolower(*cp) != tolower(*cs) && *cp != '?') {
			return false;
		}
		++cp;
		++cs;
		++i;
	}

	return true;
}

/* Find a CSDB object in a directory hierarchy based on its code. */
bool find_csdb_object(char *dst, const char *path, const char *code, bool (*is)(const char *), bool recursive)
{
	DIR *dir;
	struct dirent *cur;
	bool found = false;
	int len = strlen(path);
	char fpath[PATH_MAX], cpath[PATH_MAX];

	if (!isdir(path, false)) {
		return false;
	}

	if (!(dir = opendir(path))) {
		return false;
	}

	if (strcmp(path, ".") == 0) {
		strcpy(fpath, "");
	} else if (path[len - 1] != '/') {
		strcpy(fpath, path);
		strcat(fpath, "/");
	} else {
		strcpy(fpath, path);
	}

	while ((cur = readdir(dir))) {
		strcpy(cpath, fpath);
		strcat(cpath, cur->d_name);

		if (recursive && isdir(cpath, true)) {
			char tmp[PATH_MAX];

			if (find_csdb_object(tmp, cpath, code, is, true) && (!found || codecmp(tmp, dst) > 0)) {
				strcpy(dst, tmp);
				found = true;
			}
		} else if ((!is || is(cur->d_name)) && strmatch(code, cur->d_name)) {
			if (!found || codecmp(cpath, dst) > 0) {
				strcpy(dst, cpath);
				found = true;
			}
		}
	}

	closedir(dir);

	return found;
}

/* Find a CSDB object in a list of paths. */
bool find_csdb_object_in_list(char *dst, char (*objects)[PATH_MAX], int n, const char *code)
{
	int i;
	bool found = false;

	for (i = 0; i < n; ++i) {
		char *name, *base;

		name = strdup(objects[i]);
		base = basename(name);
		found = strmatch(code, base);
		free(name);

		if (found) {
			strcpy(dst, objects[i]);
			break;
		}
	}

	return found;
}

/* Convert string to double. Returns true if the string contained only a
 * correct double value or false if it contained extra content. */
static bool strtodbl(double *d, const char *s)
{
	char *e;
	*d = strtod(s, &e);
	return e != s && *e == 0;
}

/* Tests whether a value is in an S1000D range (a~c is equivalent to a|b|c) */
bool is_in_range(const char *value, const char *range)
{
	char *ran, *first, *last;
	bool ret;
	double f, l, v;

	if (!strchr(range, '~')) {
		return strcmp(value, range) == 0;
	}

	ran = malloc(strlen(range) + 1);

	strcpy(ran, range);

	first = strtok(ran, "~");
	last = strtok(NULL, "~");

	/* Attempt to compare the values numerically. If any of the values are
	 * non-numeric, fall back to lexicographic comparison.
	 *
	 * For example, if the range is 20~100, 30 falls in this range
	 * numerically but not lexicographically.
	 */
	if (strtodbl(&f, first) && strtodbl(&l, last) && strtodbl(&v, value)) {
		ret = v - f >= 0 && v - l <= 0;
	} else {
		ret = strcmp(value, first) >= 0 && strcmp(value, last) <= 0;
	}

	free(ran);

	return ret;
}

/* Tests whether a value is in an S1000D set (a|b|c) */
bool is_in_set(const char *value, const char *set)
{
	char *s, *val = NULL;
	bool ret = false;

	if (!strchr(set, '|')) {
		return is_in_range(value, set);
	}

	s = malloc(strlen(set) + 1);

	strcpy(s, set);

	while ((val = strtok(val ? NULL : s, "|"))) {
		if (is_in_range(value, val)) {
			ret = true;
			break;
		}
	}

	free(s);

	return ret;
}

/* Add a NOTATION to the DTD. */
void add_notation(xmlDocPtr doc, const xmlChar *name, const xmlChar *pubId, const xmlChar *sysId)
{
	if (!doc->intSubset) {
		xmlCreateIntSubset(doc, xmlDocGetRootElement(doc)->name, NULL, NULL);
	}

	if (!xmlHashLookup(doc->intSubset->notations, BAD_CAST name)) {
/* FIXME:
 *
 * Needed for libxml >= 2.14.0 and < 2.14.6
 *
 * See: https://gitlab.gnome.org/GNOME/libxml2/-/commit/c4b278ec
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		xmlAddNotationDecl(NULL, doc->intSubset, name, pubId, sysId);
#pragma GCC diagnostic pop
	}
}

/* Add an ICN entity from a file path. */
xmlEntityPtr add_icn(xmlDocPtr doc, const char *path, bool fullpath)
{
	char *full, *base, *name;
	char *infoEntityIdent;
	char *notation;
	xmlEntityPtr e;

	full = strdup(path);
	base = basename(full);
	name = strdup(base);

	infoEntityIdent = strtok(name, ".");
	notation = strtok(NULL, "");

	add_notation(doc, BAD_CAST notation, NULL, BAD_CAST notation);
	e = xmlAddDocEntity(doc, BAD_CAST infoEntityIdent,
		XML_EXTERNAL_GENERAL_UNPARSED_ENTITY, NULL,
		BAD_CAST (fullpath ? path : base), BAD_CAST notation);

	free(name);
	free(full);

	return e;
}

/* Make a file read-only. */
void mkreadonly(const char *path)
{
	#ifdef _WIN32
	SetFileAttributesA(path, FILE_ATTRIBUTE_READONLY);
	#else
	struct stat st;
	stat(path, &st);
	chmod(path, (st.st_mode & 07777) & ~(S_IWUSR | S_IWGRP | S_IWOTH));
	#endif
}

/* Insert a child node instead of appending one. */
xmlNodePtr add_first_child(xmlNodePtr parent, xmlNodePtr child)
{
	if (parent->children) {
		return xmlAddPrevSibling(parent->children, child);
	} else {
		return xmlAddChild(parent, child);
	}
}

/* Convert string to lowercase. */
void lowercase(char *s)
{
	int i;
	for (i = 0; s[i]; ++i) {
		s[i] = tolower(s[i]);
	}
}

/* Convert string to uppercase. */
void uppercase(char *s)
{
	int i;
	for (i = 0; s[i]; ++i) {
		s[i] = toupper(s[i]);
	}
}

/* Return whether a bitset contains an option. */
bool optset(int opts, int opt)
{
	return ((opts & opt) == opt);
}

/* Read an XML document from a file. */
xmlDocPtr read_xml_doc(const char *path)
{
	xmlDocPtr doc;

	doc = xmlReadFile(path, NULL, DEFAULT_PARSE_OPTS);

	if (optset(DEFAULT_PARSE_OPTS, XML_PARSE_XINCLUDE)) {
		xmlXIncludeProcessFlags(doc, DEFAULT_PARSE_OPTS);
	}

	return doc;
}

/* Read an XML document from memory. */
xmlDocPtr read_xml_mem(const char *buffer, int size)
{
	xmlDocPtr doc;

	doc = xmlReadMemory(buffer, size, NULL, NULL, DEFAULT_PARSE_OPTS);

	if (optset(DEFAULT_PARSE_OPTS, XML_PARSE_XINCLUDE)) {
		xmlXIncludeProcessFlags(doc, DEFAULT_PARSE_OPTS);
	}

	return doc;
}

/* Save an XML document to a file. */
int save_xml_doc(xmlDocPtr doc, const char *path)
{
	return xmlSaveFile(path, doc);
}

/* Return the first node matching an XPath expression. */
xmlNodePtr xpath_first_node(xmlDocPtr doc, xmlNodePtr node, const xmlChar *path)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	xmlNodePtr first;

	if (doc) {
		ctx = xmlXPathNewContext(doc);
	} else {
		ctx = xmlXPathNewContext(node->doc);
	}

	ctx->node = node;

	obj = xmlXPathEvalExpression(BAD_CAST path, ctx);

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		first = NULL;
	} else {
		first = obj->nodesetval->nodeTab[0];
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return first;
}

/* Return the value of the first node matching an XPath expression. */
xmlChar *xpath_first_value(xmlDocPtr doc, xmlNodePtr node, const xmlChar *path)
{
	return xmlNodeGetContent(xpath_first_node(doc, node, path));
}

/* Add a dependency test to an assertion if it contains any of the dependent
 * values.
 *
 * If the assertion uses a set (|), the values will be split up in order to
 * only add the dependency to the appropriate values.
 */
static void add_cct_depend_to_assert(xmlNodePtr *assert, const xmlChar *id, const xmlChar *forval, xmlNodePtr applic)
{
	xmlChar *vals, *v = NULL;
	bool match = false;
	xmlNodePtr eval;
	char *end = NULL;

	vals = xmlGetProp(*assert, BAD_CAST "applicPropertyValues");

	eval = xmlNewNode(NULL, BAD_CAST "evaluate");
	xmlSetProp(eval, BAD_CAST "andOr", BAD_CAST "or");

	/* Split any sets (|) */
	while ((v = BAD_CAST strtok_r(v ? NULL : (char *) vals, "|", &end))) {
		xmlNodePtr a;

		a = xmlNewNode(NULL, BAD_CAST "assert");
		xmlSetProp(a, BAD_CAST "applicPropertyIdent", id);
		xmlSetProp(a, BAD_CAST "applicPropertyType", BAD_CAST "condition");
		xmlSetProp(a, BAD_CAST "applicPropertyValues", v);

		/* If the current value has a dependency, add the depedency
		 * test to it with an AND evaluate.
		 */
		if (xmlStrcmp(v, forval) == 0) {
			xmlNodePtr cur, e;

			match = true;

			e = xmlNewChild(eval, NULL, BAD_CAST "evaluate", NULL);
			xmlSetProp(e, BAD_CAST "andOr", BAD_CAST "and");

			for (cur = applic->children; cur; cur = cur->next) {
				if (xmlStrcmp(cur->name, BAD_CAST "assert") == 0 || xmlStrcmp(cur->name, BAD_CAST "evaluate") == 0) {
					xmlAddChild(e, xmlCopyNode(cur, 1));
				}
			}

			xmlAddChild(e, a);
		} else {
			xmlAddChild(eval, a);
		}
	}

	xmlFree(vals);

	/* If any of the values in the set had a dependency, replace the assert
	 * with the new evaluation.
	 */
	if (match) {
		xmlChar *op;

		op = xmlGetProp((*assert)->parent, BAD_CAST "andOr");

		/* If the dependency test is being added to an OR evaluate,
		 * simplify the output by combining the new OR and the existing
		 * OR.
		 */
		if (!eval->children->next || (op && xmlStrcmp(op, BAD_CAST "or") == 0)) {
			xmlNodePtr cur, last;

			for (cur = eval->children, last = *assert; cur; cur = cur->next) {
				xmlChar *o;

				o = xmlGetProp(cur, BAD_CAST "andOr");

				/* Combine evaluates with the same operation. */
				if (o && xmlStrcmp(o, op) == 0) {
					xmlNodePtr c;

					for (c = cur->children; c; c = c->next) {
						last = xmlAddNextSibling(last, xmlCopyNode(c, 1));
					}
				} else {
					last = xmlAddNextSibling(last, xmlCopyNode(cur, 1));
				}

				xmlFree(o);
			}

			xmlFreeNode(eval);
		/* Otherwise, just add the new OR. */
		} else {
			xmlAddNextSibling(*assert, eval);
		}

		xmlFree(op);

		xmlUnlinkNode(*assert);
		xmlFreeNode(*assert);
		*assert = NULL;
	} else {
		xmlFreeNode(eval);
	}
}

/* Add a dependency from the CCT. */
static void add_cct_depend(xmlDocPtr doc, xmlNodePtr dep)
{
	xmlChar *id, *test, *vals, *xpath;
	int n;
	xmlNodePtr applic;

	id = xmlGetProp(dep->parent, BAD_CAST "id");
	test = xmlGetProp(dep, BAD_CAST "dependencyTest");
	vals = xmlGetProp(dep, BAD_CAST "forCondValues");

	/* Find the annotation for the dependency test. */
	n = xmlStrlen(test) + 17;
	xpath = malloc(n * sizeof(xmlChar));
	xmlStrPrintf(xpath, n, "//applic[@id='%s']", test);
	applic = xpath_first_node(dep->doc, NULL, xpath);
	xmlFree(xpath);

	if (applic) {
		xmlXPathContextPtr ctx;
		xmlXPathObjectPtr obj;
		xmlChar *v = NULL;
		char *end = NULL;

		ctx = xmlXPathNewContext(doc);

		/* Add dependency tests to assertions that use the dependant values. */
		while ((v = BAD_CAST strtok_r(v ? NULL : (char *) vals, "|", &end))) {
			n = xmlStrlen(id) + 70;
			xpath = malloc(n * sizeof(xmlChar));
			xmlStrPrintf(xpath, n, "//assert[@applicPropertyIdent='%s' and @applicPropertyType='condition']", id);
			obj = xmlXPathEvalExpression(xpath, ctx);
			xmlFree(xpath);

			if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
				int i;

				for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
					add_cct_depend_to_assert(&obj->nodesetval->nodeTab[i], id, v, applic);
				}
			}

			xmlXPathFreeObject(obj);
		}

		xmlXPathFreeContext(ctx);

		/* Handle subdependencies, that is, dependant values which themselves
		 * have dependencies.
		 */
		ctx = xmlXPathNewContext(applic->doc);
		ctx->node = applic;
		obj = xmlXPathEvalExpression(BAD_CAST ".//assert[@applicPropertyIdent and @applicPropertyType='condition']", ctx);

		if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
			int i;

			for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
				xmlChar *ident;

				ident = xmlGetProp(obj->nodesetval->nodeTab[i], BAD_CAST "applicPropertyIdent");
				add_cct_depends(doc, applic->doc, ident);
				xmlFree(ident);
			}
		}

		xmlXPathFreeObject(obj);
		xmlXPathFreeContext(ctx);
	}

	xmlFree(id);
	xmlFree(test);
	xmlFree(vals);
}

/* Add CCT dependencies to the source object.
 *
 * If id is NULL, all conditions will be added.
 *
 * If id is not NULL, then only the dependencies for condition with that id
 * will be added. This is useful when handling sub-depedencies, to only handle
 * the conditions which pertain to a particular dependency test.
 */
void add_cct_depends(xmlDocPtr doc, xmlDocPtr cct, xmlChar *id)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;

	ctx = xmlXPathNewContext(cct);

	if (id) {
		int n;
		xmlChar *xpath;

		n = xmlStrlen(id) + 26;
		xpath = malloc(n * sizeof(xmlChar));
		xmlStrPrintf(xpath, n, "//cond[@id='%s']/dependency", id);

		obj = xmlXPathEvalExpression(xpath, ctx);

		xmlFree(xpath);
	} else {
		obj = xmlXPathEvalExpression(BAD_CAST "//cond/dependency", ctx);
	}

	if (!xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
		int i;

		for (i = 0; i < obj->nodesetval->nodeNr; ++i) {
			add_cct_depend(doc, obj->nodesetval->nodeTab[i]);
		}
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);
}

/* Test whether an object value matches a regex pattern. */
bool match_pattern(const xmlChar *value, const xmlChar *pattern)
{
	xmlRegexpPtr regex;
	bool match;
	regex = xmlRegexpCompile(BAD_CAST pattern);
	match = xmlRegexpExec(regex, BAD_CAST value);
	xmlRegFreeRegexp(regex);
	return match;
}

/* Display a progress bar. */
void print_progress_bar(float cur, float total)
{
	float p;
	int i, b;

	p = cur / total;
	b = PROGRESS_BAR_WIDTH * p;

	fprintf(stderr, "\r[");
	for (i = 0; i < PROGRESS_BAR_WIDTH; ++i) {
		if (i < b) {
			fputc('=', stderr);
		} else {
			fputc(' ', stderr);
		}
	}
	fprintf(stderr, "] %d%% (%d/%d) ", (int)(p * 100.0), (int) cur, (int) total);
	if (cur == total) {
		fputc('\n', stderr);
	}
	fflush(stderr);
}

/* Print progress information in the zenity --progress format. */
void print_zenity_progress(const char *message, float cur, float total)
{
	int p;

	p = (int)((cur / total) * 100.0);

	printf("# %s %d%% (%d/%d)\n%d\n", message, p, (int) cur, (int) total, p);
}

/* Determine if the file is an XML file. */
static bool is_xml(const char *name)
{
	return strncasecmp(name + strlen(name) - 4, ".XML", 4) == 0;
}

/* Determine if the file is a data module. */
bool is_dm(const char *name)
{
	return strncmp(name, "DMC-", 4) == 0 && is_xml(name);
}

/* Determine if the file is a publication module. */
bool is_pm(const char *name)
{
	return strncmp(name, "PMC-", 4) == 0 && is_xml(name);
}

/* Determine if the file is a comment. */
bool is_com(const char *name)
{
	return strncmp(name, "COM-", 4) == 0 && is_xml(name);
}

/* Determine if the file is an ICN metadata file. */
bool is_imf(const char *name)
{
	return strncmp(name, "IMF-", 4) == 0 && is_xml(name);
}

/* Determine if the file is a data dispatch note. */
bool is_ddn(const char *name)
{
	return strncmp(name, "DDN-", 4) == 0 && is_xml(name);
}

/* Determine if the file is a data management list. */
bool is_dml(const char *name)
{
	return strncmp(name, "DML-", 4) == 0 && is_xml(name);
}

/* Determine if the file is an ICN. */
bool is_icn(const char *name)
{
	return strncmp(name, "ICN-", 4) == 0;
}

/* Determine if the file is a SCORM content package. */
bool is_smc(const char *name)
{
	return (strncmp(name, "SMC-", 4) == 0 || strncmp(name, "SME-", 4) == 0) && is_xml(name);
}

/* Determine if the file is a data update file. */
bool is_upf(const char *name)
{
	return (strncmp(name, "UPF-", 4) == 0 || strncmp(name, "UPE-", 4) == 0) && is_xml(name);
}

/* Interpolate a command string with a file name and execute it. */
int execfile(const char *execstr, const char *path)
{
	int i, j, n, e, c = 0;
	char *fmtstr, *cmd;

	n = strlen(execstr);

	fmtstr = malloc(n * 2);

	for (i = 0, j = 0; i < n; ++i) {
		switch (execstr[i]) {
			case '{':
				if (execstr[i+1] && execstr[i+1] == '}') {
					fmtstr[j++] = '%';
					fmtstr[j++] = '1';
					fmtstr[j++] = '$';
					fmtstr[j++] = 's';
					++c;
					++i;
				}
				break;
			case '%':
			case '\\':
				fmtstr[j++] = execstr[i];
				fmtstr[j++] = execstr[i];
				break;
			default:
				fmtstr[j++] = execstr[i];
		}
	}

	fmtstr[j] = 0;

	n = strlen(fmtstr) + strlen(path) * c;
	cmd = malloc(n);
	snprintf(cmd, n, fmtstr, path);
	free(fmtstr);

	e = system(cmd);

	free(cmd);

	if (WIFSIGNALED(e)) {
		raise(WTERMSIG(e));
	}

	return WEXITSTATUS(e);
}

/* Copy only the latest issues of CSDB objects. */
int extract_latest_csdb_objects(char (*latest)[PATH_MAX], char (*files)[PATH_MAX], int nfiles)
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

/* Compare the base names of two files. */
int compare_basename(const void *a, const void *b)
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

/* Determine if a CSDB object is a CIR. */
bool is_cir(const char *path, const bool ignore_del)
{
	xmlDocPtr doc;
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	bool is;

	if (!(doc = read_xml_doc(path))) {
		return false;
	}

	ctx = xmlXPathNewContext(doc);

	/* Check that this is a CIR/TIR DM. */
	obj = xmlXPathEvalExpression(BAD_CAST "//commonRepository|//techRepository|//techrep", ctx);
	is = !xmlXPathNodeSetIsEmpty(obj->nodesetval);
	xmlXPathFreeObject(obj);

	/* Check that the DM is not "deleted" if ignore_del = true. */
	if (is && ignore_del) {
		obj = xmlXPathEvalExpression(BAD_CAST "//dmodule[identAndStatusSection/dmStatus/@issueType='deleted' or status/issno/@type='deleted']", ctx);
		is = xmlXPathNodeSetIsEmpty(obj->nodesetval);
		xmlXPathFreeObject(obj);
	}

	xmlXPathFreeContext(ctx);

	xmlFreeDoc(doc);

	return is;
}

/* Recursively remove nodes marked as "delete". */
static void rem_delete_nodes(xmlNodePtr node)
{
	xmlChar *change;
	xmlNodePtr cur;

	if (!node) {
		return;
	}

	change = xmlGetProp(node, BAD_CAST "change");

	if (!change) {
		change = xmlGetProp(node, BAD_CAST "changeType");
	}

	if (xmlStrcmp(change, BAD_CAST "delete") == 0) {
		xmlUnlinkNode(node);
		xmlFreeNode(node);
		return;
	}

	cur = node->children;
	while (cur) {
		xmlNodePtr next = cur->next;
		rem_delete_nodes(cur);
		cur = next;
	}
}

/* Remove elements marked as "delete". */
void rem_delete_elems(xmlDocPtr doc)
{
	rem_delete_nodes(xmlDocGetRootElement(doc));
}

/* Evaluate an applic statement, returning whether it is valid or invalid given
 * the user-supplied applicability settings.
 *
 * If assume is true, undefined attributes and conditions are ignored. This is
 * primarily useful for determining which elements are not applicable in content
 * and may be removed.
 *
 * If assume is false, undefined attributes or conditions will cause an applic
 * statement to evaluate as invalid. This is primarily useful for determining
 * which applic statements and references are unambiguously true (they do not
 * rely on any undefined attributes or conditions) and therefore may be removed.
 *
 * An undefined attribute/condition is a product attribute (ACT) or
 * condition (CCT) for which a value is asserted in the applic statement but
 * for which no value was supplied by the user.
 */
bool eval_applic(xmlNodePtr defs, xmlNodePtr node, bool assume);

/* Evaluate multiple values for a property */
static bool eval_multi(xmlNodePtr multi, const char *ident, const char *type, const char *value)
{
	xmlNodePtr cur;
	bool result = false;

	for (cur = multi->children; cur; cur = cur->next) {
		xmlChar *cur_value;
		bool in_set;

		cur_value = xmlNodeGetContent(cur);
		in_set = is_in_set((char *) cur_value, value);
		xmlFree(cur_value);

		if (in_set) {
			result = true;
			break;
		}
	}

	return result;
}

/* Tests whether ident:type=value was defined by the user */
bool is_applic(xmlNodePtr defs, const char *ident, const char *type, const char *value, bool assume)
{
	xmlNodePtr cur;

	bool result = assume;

	if (!(ident && type && value)) {
		return assume;
	}

	for (cur = defs->children; cur; cur = cur->next) {
		char *cur_ident = (char *) xmlGetProp(cur, BAD_CAST "applicPropertyIdent");
		char *cur_type  = (char *) xmlGetProp(cur, BAD_CAST "applicPropertyType");
		char *cur_value = (char *) xmlGetProp(cur, BAD_CAST "applicPropertyValues");

		bool match = strcmp(cur_ident, ident) == 0 && strcmp(cur_type, type) == 0;

		if (match) {
			if (cur_value) {
				result = is_in_set(cur_value, value);
			} else if (assume) {
				result = eval_multi(cur, ident, type, value);
			}
		}

		xmlFree(cur_ident);
		xmlFree(cur_type);
		xmlFree(cur_value);

		if (match) {
			break;
		}

	}

	return result;
}

/* Tests whether an <assert> element is applicable */
bool eval_assert(xmlNodePtr defs, xmlNodePtr assert, bool assume)
{
	xmlNodePtr ident_attr, type_attr, values_attr;
	char *ident, *type, *values;

	bool ret;

	ident_attr  = xpath_first_node(NULL, assert, BAD_CAST "@applicPropertyIdent|@actidref");
	type_attr   = xpath_first_node(NULL, assert, BAD_CAST "@applicPropertyType|@actreftype");
	values_attr = xpath_first_node(NULL, assert, BAD_CAST "@applicPropertyValues|@actvalues");

	ident  = (char *) xmlNodeGetContent(ident_attr);
	type   = (char *) xmlNodeGetContent(type_attr);
	values = (char *) xmlNodeGetContent(values_attr);

	ret = is_applic(defs, ident, type, values, assume);

	xmlFree(ident);
	xmlFree(type);
	xmlFree(values);

	return ret;
}

enum operator { AND, OR };

/* Test whether an <evaluate> element is applicable. */
bool eval_evaluate(xmlNodePtr defs, xmlNodePtr evaluate, bool assume)
{
	xmlChar *andOr;
	enum operator op;
	bool ret = assume;
	xmlNodePtr cur;

	andOr = xpath_first_value(NULL, evaluate, BAD_CAST "@andOr|@operator");

	if (!andOr) {
		return false;
	}

	if (xmlStrcmp(andOr, BAD_CAST "and") == 0) {
		op = AND;
	} else {
		op = OR;
	}

	xmlFree(andOr);

	for (cur = evaluate->children; cur; cur = cur->next) {
		if (xmlStrcmp(cur->name, BAD_CAST "assert") == 0 || xmlStrcmp(cur->name, BAD_CAST "evaluate") == 0) {
			ret = eval_applic(defs, cur, assume);

			if ((op == AND && !ret) || (op == OR && ret)) {
				break;
			}
		}
	}

	return ret;
}

/* Generic test for either <assert> or <evaluate> */
bool eval_applic(xmlNodePtr defs, xmlNodePtr node, bool assume)
{
	if (xmlStrcmp(node->name, BAD_CAST "assert") == 0) {
		return eval_assert(defs, node, assume);
	} else if (xmlStrcmp(node->name, BAD_CAST "evaluate") == 0) {
		return eval_evaluate(defs, node, assume);
	}

	return false;
}

/* Determine if two applicability annotations are the same. */
bool same_annotation(xmlNodePtr app1, xmlNodePtr app2)
{
	xmlDocPtr d1, d2;
	xmlChar *s1, *s2;
	bool same;

	/* Compare c14n representation of XML to
	 * determine if the annotations are duplicates.
	 */
	d1 = xmlNewDoc(BAD_CAST "1.0");
	d2 = xmlNewDoc(BAD_CAST "1.0");

	xmlDocSetRootElement(d1, xmlCopyNode(app1, 1));
	xmlDocSetRootElement(d2, xmlCopyNode(app2, 1));

	xmlC14NDocDumpMemory(d1, NULL, XML_C14N_1_0, NULL, 0, &s1);
	xmlC14NDocDumpMemory(d2, NULL, XML_C14N_1_0, NULL, 0, &s2);

	same = xmlStrcmp(s1, s2) == 0;

	xmlFree(s1);
	xmlFree(s2);

	xmlFreeDoc(d1);
	xmlFreeDoc(d2);

	return same;
}
