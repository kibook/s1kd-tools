#include "s1kd_tools.h"

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
