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
xmlChar *s1kdDocGetMetadata(xmlDocPtr doc, const xmlChar *name);


/**
 * Retrieve metadata from a CSDB object.
 *
 * @param object_xml Input buffer containing the XML of the object
 * @param object_size Size of the object XML buffer
 * @param name Name of the metadata
 * @return A new char * containing the value of the metadata
 */
char *s1kdGetMetadata(const char *object_xml, int object_size, const char *name);

/**
 * Set metadata in a CSDB object.
 *
 * @param doc The CSDB object
 * @param name Name of the metadata
 * @param value New value of the metadata
 * @return 0 if successful, non-zero otherwise
 */
int s1kdDocSetMetadata(xmlDocPtr doc, const xmlChar *name, const xmlChar *value);

/**
 * Set metadata in a CSDB object
 *
 * @param object_xml Input buffer containing the XML of the object
 * @param object_size Size of the object XML buffer
 * @param name Name of the metadata
 * @param value New value of the metadata
 * @return 0 if successful, non-zero otherwise
 */
int s1kdSetMetadata(const char *object_xml, int object_size, const char *name, const char *value, char **result_xml, int *result_size);

#endif
