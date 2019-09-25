#ifndef S1KD_INSTANCE
#define S1KD_INSTANCE

#include <stdbool.h>
#include <libxml/tree.h>

void s1kdFilter(xmlDocPtr doc, xmlNodePtr defs, bool reduce);

#endif
