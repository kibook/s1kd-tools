#ifndef S1KD_METADATA
#define S1KD_METADATA

#include <libxml/xmlstring.h>
#include <libxml/tree.h>

xmlChar *s1kdGetMetadata(xmlDocPtr doc, const xmlChar *name);
int s1kdSetMetadata(xmlDocPtr doc, const xmlChar *name, const xmlChar *value);

#endif
