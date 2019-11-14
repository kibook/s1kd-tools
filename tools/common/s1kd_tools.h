#ifndef S1KD_H
#define S1KD_H

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libxml/tree.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>

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
#define DEFAULT_EXTPUBS_FNAME ".externalpubs"

/* Default global XML parsing options. */
extern int DEFAULT_PARSE_OPTS;

/* Macros for standard libxml2 parser options. */
#define LIBXML2_PARSE_LONGOPT_DEFS \
	{"dtdload", no_argument, 0, 0},\
	{"net", no_argument, 0, 0},\
	{"noent", no_argument, 0, 0},\
	{"xinclude", no_argument, 0, 0},
#define LIBXML2_PARSE_LONGOPT_HANDLE(lopts, loptind) \
	else if (strcmp(lopts[loptind].name, "dtdload") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_DTDLOAD;\
	} else if (strcmp(lopts[loptind].name, "net") == 0) {\
		DEFAULT_PARSE_OPTS &= ~XML_PARSE_NONET;\
	} else if (strcmp(lopts[loptind].name, "noent") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_NOENT;\
	} else if (strcmp(lopts[loptind].name, "xinclude") == 0) {\
		DEFAULT_PARSE_OPTS |= XML_PARSE_XINCLUDE | XML_PARSE_NOBASEFIX | XML_PARSE_NOXINCNODE;\
	}
#define LIBXML2_PARSE_LONGOPT_HELP \
	puts("");\
	puts("XML parser options:");\
	puts("  --dtdload   Load external DTD.");\
	puts("  --net       Allow network access.");\
	puts("  --noent     Resolve entities.");\
	puts("  --xinclude  Do XInclude processing.");

#define LIBXML2_PARSE_INIT \
	if (optset(DEFAULT_PARSE_OPTS, XML_PARSE_NONET)) {\
		xmlSetExternalEntityLoader(xmlNoNetExternalEntityLoader);\
	}

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

/* Match a string with a pattern case-insensitively, using ? as a wildcard. */
bool strmatch(const char *p, const char *s);

/* Find a CSDB object in a directory hierarchy based on its code. */
bool find_csdb_object(char *dst, const char *path, const char *code, bool (*is)(const char *), bool recursive);

/* Find a CSDB object in a list of paths. */
bool find_csdb_object_in_list(char *dst, char (*objects)[PATH_MAX], int n, const char *code);

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

/* Convert string to lowercase. */
void lowercase(char *s);

/* Convert string to uppercase. */
void uppercase(char *s);

/* Return whether a bitset contains an option. */
bool optset(int opts, int opt);

/* Read an XML document from a file. */
xmlDocPtr read_xml_doc(const char *path);

/* Read an XML document from memory. */
xmlDocPtr read_xml_mem(const char *buffer, int size);

/* Save an XML document to a file. */
int save_xml_doc(xmlDocPtr doc, const char *path);

/* Return the first node matching an XPath expression. */
xmlNodePtr xpath_first_node(xmlDocPtr doc, xmlNodePtr node, const xmlChar *path);

/* Return the value of the first node matching an XPath expression. */
xmlChar *xpath_first_value(xmlDocPtr doc, xmlNodePtr node, const xmlChar *path);

/* Add CCT dependencies to an object's annotations. */
void add_cct_depends(xmlDocPtr doc, xmlDocPtr cct, xmlChar *id);

/* Test whether an object value matches a regex pattern. */
bool match_pattern(const xmlChar *value, const xmlChar *pattern);

/* Display a progress bar. */
void print_progress_bar(float cur, float total);

/* Determine if the file is a data module. */
bool is_dm(const char *name);

/* Determine if the file is a publication module. */
bool is_pm(const char *fname);

/* Determine if the file is a comment. */
bool is_com(const char *name);

/* Determine if the file is an ICN metadata file. */
bool is_imf(const char *name);

/* Determine if the file is a data dispatch note. */
bool is_ddn(const char *name);

/* Determine if the file is a data management list. */
bool is_dml(const char *name);

/* Determine if the file is an ICN. */
bool is_icn(const char *name);

/* Determine if the file is a SCORM content package. */
bool is_smc(const char *name);

/* Determine if the file is a data update file. */
bool is_upf(const char *name);

/* Interpolate a command string with a file name and execute it. */
int execfile(const char *execstr, const char *path);

/* Copy only the latest issues of CSDB objects. */
int extract_latest_csdb_objects(char (*latest)[PATH_MAX], char (*files)[PATH_MAX], int nfiles);

/* Compare the base names of two files. */
int compare_basename(const void *a, const void *b);

/* Determine if a CSDB object is a CIR. */
bool is_cir(const char *path);

#endif
