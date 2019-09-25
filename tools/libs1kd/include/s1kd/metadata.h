/**
 * @file metadata.h
 * @brief Retrieve and set metadata on CSDB objects
 */

#ifndef S1KD_METADATA
#define S1KD_METADATA

#include <libxml/xmlstring.h>
#include <libxml/tree.h>

/**
 * Retrieve metadata from a CSDB object.
 *
 * @param doc The CSDB object
 * @param name Name of the metadata
 * @return A new xmlChar * containing the value of the metadata
 */
xmlChar *s1kdGetMetadata(xmlDocPtr doc, const xmlChar *name);

/**
 * Set metadata in a CSDB object.
 *
 * @param doc The CSDB object
 * @param name Name of the metadata
 * @param value New value of the metadata
 * @return 0 if successful, non-zero otherwise
 */
int s1kdSetMetadata(xmlDocPtr doc, const xmlChar *name, const xmlChar *value);

#endif
