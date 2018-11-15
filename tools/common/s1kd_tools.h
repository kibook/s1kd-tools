#ifndef S1KD_H
#define S1KD_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <libxml/tree.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define DEFAULT_DEFAULTS_FNAME ".defaults"
#define DEFAULT_DMTYPES_FNAME ".dmtypes"
#define DEFAULT_FMTYPES_FNAME ".fmtypes"
#define DEFAULT_ICNCATALOG_FNAME ".icncatalog"
#define DEFAULT_ACRONYMS_FNAME ".acronyms"
#define DEFAULT_INDEXFLAGS_FNAME ".indexflags"
#define DEFAULT_BREXMAP_FNAME ".brexmap"
#define DEFAULT_BRSL_FNAME ".brseveritylevels"
#define DEFAULT_UOM_FNAME ".uom"

char *real_path(const char *path, char *real);
bool find_config(char *dst, const char *name);
xmlChar *xpath_of(xmlNodePtr node);
int copy(const char *from, const char *to);
bool isdir(const char *path, bool recursive);
void xmlFreeEntity(xmlEntityPtr entity);

#endif
