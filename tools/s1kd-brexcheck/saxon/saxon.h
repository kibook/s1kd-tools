#ifndef SAXON_H
#define SAXON_H

#include <libxml/tree.h>
#include <libxml/xpath.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Report version of Saxon/C. */
const char *saxon_version(void *saxon_processor);

/* Create new Saxon processor. */
void *saxon_new_processor(void);

/* Free Saxon processor. */
void saxon_free_processor(void *saxon_processor);

/* Cleanup Saxon JVM. */
void saxon_cleanup(void);

/* Create new Saxon XPath processor. */
void *saxon_new_xpath_processor(void *saxon_processor);

/* Free Saxon XPath processor. */
void saxon_free_xpath_processor(void *xpath_processor);

/* Register a namespace prefix in a Saxon XPath processor. */
void saxon_register_namespace(void *xpath_processor, const xmlChar *prefix, const xmlChar *href);

/* Create a Saxon XdmNode from a libxml2 doc. */
void *saxon_new_node(void *saxon_processor, xmlDocPtr doc);

/* Free a Saxon XdmNode. */
void saxon_free_node(void *saxon_node);

/* Use Saxon to evaluate an XPath expression in a libxml2 XPath context,
 * returning a libxml2 nodeset. */
xmlXPathObjectPtr saxon_eval_xpath(void *saxon_processor, void *xpath_processor, void *saxon_node, const xmlChar *expr, xmlXPathContextPtr ctx);

#ifdef __cplusplus
}
#endif

#endif
