#ifndef XQILLA_H
#define XQILLA_H

#include <libxml/tree.h>
#include <libxml/xpath.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Get Xerces and XQilla version information. */
const char *xqilla_version(void);

/* Initialize XQilla and Xerces, and return the DOM implementation. */
void *xqilla_initialize(void);

/* Cleanup XQilla and Xerces. */
void xqilla_terminate(void);

/* Create a new DOMLSParser. */
void *xqilla_create_parser(void *implementation);

/* Release a DOMLSParser. */
void xqilla_free_parser(void *parser);

/* Create a Xerces DOMDocument from a libxml xmlDocPtr. */
void *xqilla_create_doc(void *impl, void *parser, xmlDocPtr doc);

/* Create a new DOMXPathNSResolver. */
void *xqilla_create_ns_resolver(void *doc);

/* Register a namespace with a DOMXPathNSResolver. */
void xqilla_register_namespace(void *resolver, const xmlChar *prefix, const xmlChar *uri);

/* Evaluate an XPath 2.0 expression using XQilla and return a libxml nodeset. */
xmlXPathObjectPtr xqilla_eval_xpath(void *doc, void *ns_resolver, const xmlChar *expr, xmlXPathContextPtr ctx);

#ifdef __cplusplus
}
#endif

#endif
