/**
 * @file instance.h
 * @brief Create instances of S1000D CSDB objects
 */

#ifndef S1KD_INSTANCE
#define S1KD_INSTANCE

#include <stdbool.h>
#include <libxml/tree.h>

/**
 * Create a filtered instance based on user-defined applicability.
 *
 * @param doc The CSDB object
 * @param defs Applicability definitions to filter on
 * @param reduce Whether or not to hide redundant applicability
 */
void s1kdFilter(xmlDocPtr doc, xmlNodePtr defs, bool reduce);

#endif
