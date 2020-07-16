#ifndef SAXON_H
#define SAXON_H

#include <libxml/tree.h>
#include <libxml/xpath.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *saxon_version(void *saxon_processor);
void *saxon_new_processor(void);
void saxon_free_processor(void *saxon_processor);
void *saxon_new_xpath_processor(void *saxon_processor);
void saxon_free_xpath_processor(void *xpath_processor);
void saxon_register_namespace(void *xpath_processor, const xmlChar *prefix, const xmlChar *href);
xmlXPathObjectPtr saxon_eval_xpath(void *saxon_processor, void *xpath_processor, const xmlNodePtr ns, const xmlChar *expr, xmlXPathContextPtr ctx);

#ifdef __cplusplus
}
#endif

#endif
