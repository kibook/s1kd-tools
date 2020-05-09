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
#define s1kdApplicability xmlNodePtr

/**
 * Filtering mode.
 */
typedef enum {
	S1KD_FILTER_DEFAULT,  /**< No extra processing */
	S1KD_FILTER_REDUCE,   /**< Remove wholly resolved annotations */
	S1KD_FILTER_SIMPLIFY, /**< Remove resolved parts of annotations */
	S1KD_FILTER_PRUNE     /**< Only remove false parts of annotations */
} s1kdFilterMode;

/**
 * Create a new set of applicability definitions.
 *
 * @return A pointer to a new set of applicability definitions.
 */
s1kdApplicability s1kdNewApplicability(void);

/**
 * Free a set of applicability definitions.
 *
 * @param app The set of applicability definitions to free
 */
void s1kdFreeApplicability(s1kdApplicability app);

/**
 * Add an applicability definition to a set of definitions.
 *
 * @param app A set of applicability definitions
 * @param ident The ID of the applicability property
 * @param type The type of the applicability property (prodattr or condition)
 * @param value The value assigned to the applicability property
 */
void s1kdAssign(s1kdApplicability app, const xmlChar *ident, const xmlChar *type, const xmlChar *value);

/**
 * Create a filtered instance based on user-defined applicability.
 *
 * @param doc The CSDB object
 * @param app Applicability definitions to filter on
 * @param mode Filtering mode
 * @return A new XML document for the filtered instance
 */
xmlDocPtr s1kdDocFilter(xmlDocPtr doc, s1kdApplicability app, s1kdFilterMode mode);

/**
 * Create a filtered instance based on user-defined applicability.
 *
 * @param object_xml Input buffer containing the XML of the CSDB object to filter
 * @param object_size Size of the object XML buffer
 * @param app Applicability definitions to filter on
 * @param mode Filtering mode
 * @param result_xml Output buffer containing the XML of the resulting instance
 * @param result_size Size of the result XML buffer
 */
void s1kdFilter(const char *object_xml, int object_size, s1kdApplicability app, s1kdFilterMode mode, char **result_xml, int *result_size);

#endif
