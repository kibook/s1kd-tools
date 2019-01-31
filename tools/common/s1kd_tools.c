#include "s1kd_tools.h"

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
		xmlChar *name, *pos;

		name = xmlGetProp(cur, BAD_CAST "name");
		pos = xmlGetProp(cur, BAD_CAST "pos");

		dst = xmlStrcat(dst, BAD_CAST "/");
		if (!pos) {
			dst = xmlStrcat(dst, BAD_CAST "@");
		}
		dst = xmlStrcat(dst, name);
		if (pos) {
			dst = xmlStrcat(dst, BAD_CAST "[");
			dst = xmlStrcat(dst, pos);
			dst = xmlStrcat(dst, BAD_CAST "]");
		}

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

/* Not exposed by the libxml API */
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

int codecmp(const char *p1, const char *p2)
{
	char s1[PATH_MAX], s2[PATH_MAX], *b1, *b2;

	strcpy(s1, p1);
	strcpy(s2, p2);

	b1 = basename(s1);
	b2 = basename(s2);

	return strcasecmp(b1, b2);
}

bool find_csdb_object(char *dst, const char *path, const char *code, bool (*is)(const char *), bool recursive)
{
	DIR *dir;
	struct dirent *cur;
	int n;
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

	n = strlen(code);

	while ((cur = readdir(dir))) {
		strcpy(cpath, fpath);
		strcat(cpath, cur->d_name);

		if (recursive && isdir(cpath, true)) {
			char tmp[PATH_MAX];

			if (find_csdb_object(tmp, cpath, code, is, true) && (!found || codecmp(tmp, dst) > 0)) {
				strcpy(dst, tmp);
				found = true;
			}
		} else if ((!is || is(cur->d_name)) && strncasecmp(cur->d_name, code, n) == 0) {
			if (!found || codecmp(cpath, dst) > 0) {
				strcpy(dst, cpath);
				found = true;
			}
		}
	}

	closedir(dir);

	return found;
}
