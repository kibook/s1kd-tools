#ifndef S1KD_H
#define S1KD_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libxml/tree.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* Default names for configuration files. */
#define DEFAULT_DEFAULTS_FNAME ".defaults"
#define DEFAULT_DMTYPES_FNAME ".dmtypes"
#define DEFAULT_FMTYPES_FNAME ".fmtypes"
#define DEFAULT_ICNCATALOG_FNAME ".icncatalog"
#define DEFAULT_ACRONYMS_FNAME ".acronyms"
#define DEFAULT_INDEXFLAGS_FNAME ".indexflags"
#define DEFAULT_BREXMAP_FNAME ".brexmap"
#define DEFAULT_BRSL_FNAME ".brseveritylevels"
#define DEFAULT_UOM_FNAME ".uom"
#define DEFAULT_UOMDISP_FNAME ".uomdisplay"

/* Default global XML parsing options. */
extern int DEFAULT_PARSE_OPTS;

/* Macros for standard libxml2 parser options. */
#define LIBXML2_PARSE_LONGOPT_DEFS \
	{"dtdload", no_argument, 0, 0},\
	{"net", no_argument, 0, 0},\
	{"noent", no_argument, 0, 0},
#define LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind) \
	else if (strcmp(lopts[loptind].name, "dtdload") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_DTDLOAD;\
	} else if (strcmp(lopts[loptind].name, "net") == 0) {\
		DEFAULT_PARSE_OPTS &= ~XML_PARSE_NONET;\
	} else if (strcmp(lopts[loptind].name, "noent") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_NOENT;\
	}
#define LIBXML2_PARSE_LONGOPT_HELP \
	puts("");\
	puts("XML parser options:");\
	puts("  --dtdload  Load external DTD.");\
	puts("  --net      Allow network access.");\
	puts("  --noent    Resolve entities.");

/* Common read/write macros that use the default parser options. */
#define read_xml_doc(path) xmlReadFile(path, NULL, DEFAULT_PARSE_OPTS)
#define read_xml_mem(buffer, size) xmlReadMemory(buffer, size, NULL, NULL, DEFAULT_PARSE_OPTS)
#define save_xml_doc(doc, path) xmlSaveFile(path, doc)

/* Return the full path name from a relative path. */
char *real_path(const char *path, char *real);

/* Search up the directory tree to find a configuration file. */
bool find_config(char *dst, const char *name);

/* Generate an XPath expression for a node. */
xmlChar *xpath_of(xmlNodePtr node);

/* Make a copy of a file. */
int copy(const char *from, const char *to);

/* Determine if a path is a directory. */
bool isdir(const char *path, bool recursive);

/* Free an XML entity. From libxml2, but not exposed by the API. */
void xmlFreeEntity(xmlEntityPtr entity);

/* Compare the codes of two CSDB objects. */
int codecmp(const char *p1, const char *p2);

/* Find a CSDB object in a directory hierarchy based on its code. */
bool find_csdb_object(char *dst, const char *path, const char *code, bool (*is)(const char *), bool recursive);

/* Convert string to double. Returns true if the string contained only a double
 * value or false if it contained extra content. */
bool strtodbl(double *d, const char *s);

/* Tests whether a value is in an S1000D range (a~c is equivalent to a|b|c) */
bool is_in_range(const char *value, const char *range);

/* Tests whether a value is in an S1000D set (a|b|c) */
bool is_in_set(const char *value, const char *set);

/* Add a NOTATION to the DTD. */
void add_notation(xmlDocPtr doc, const xmlChar *name, const xmlChar *pubId, const xmlChar *sysId);

/* Add an ICN entity from a file path. */
xmlEntityPtr add_icn(xmlDocPtr doc, const char *path, bool fullpath);

/* Make a file read-only. */
void mkreadonly(const char *path);

/* Insert a child node instead of appending one. */
xmlNodePtr add_first_child(xmlNodePtr parent, xmlNodePtr child);

#endif
