/**
 * @file instance.h
 * @brief Create instances of S1000D CSDB objects
 */

#ifndef S1KD_INSTANCE
#define S1KD_INSTANCE

#include <stdbool.h>
#include <libxml/tree.h>

/**
 * A set of applicability definitions used to filter objects.
 */
#define s1kdApplicDefs xmlNodePtr

/**
 * Create a new set of applicability definitions.
 *
 * @return A pointer to a new set of applicability definitions.
 */
s1kdApplicDefs s1kdNewApplicDefs(void);

/**
 * Free a set of applicability definitions.
 *
 * @param defs The set of applicability definitions to free
 */
void s1kdFreeApplicDefs(s1kdApplicDefs defs);

/**
 * Add an applicability definition to a set of definitions.
 *
 * @param defs A set of applicability definitions
 * @param ident The ID of the applicability property
 * @param type The type of the applicability property (prodattr or condition)
 * @param value The value assigned to the applicability property
 */
void s1kdAssign(s1kdApplicDefs defs, const xmlChar *ident, const xmlChar *type, const xmlChar *value);

/**
 * Create a filtered instance based on user-defined applicability.
 *
 * @param doc The CSDB object
 * @param defs Applicability definitions to filter on
 * @param reduce Whether or not to hide redundant applicability
 * @return A new XML document for the filtered instance
 */
xmlDocPtr s1kdFilter(xmlDocPtr doc, s1kdApplicDefs defs, bool reduce);

#endif
