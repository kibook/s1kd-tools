#include <string>
#include <iostream>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xqilla/xqilla-dom3.hpp>
#include <libxml/xpathInternals.h>
#include "xqilla.h"

using namespace xercesc;

/* Get an XPath 1.0 expression to a DOMNode. */
static xmlChar *xpath_of(DOMNode *node)
{
	xmlNodePtr path = xmlNewNode(NULL, BAD_CAST "path");
	xmlChar *dst = NULL;

	while (node) {
		DOMNode::NodeType type;
		xmlNodePtr e;

		type = node->getNodeType();

		if (type == DOMNode::DOCUMENT_NODE) {
			break;
		}

		e = xmlNewChild(path, NULL, BAD_CAST "node", NULL);

		/* Get the name of the node. */
		char *name;

		switch (type) {
			case DOMNode::COMMENT_NODE:
				name = strdup("comment()");
				break;
			case DOMNode::PROCESSING_INSTRUCTION_NODE:
				name = strdup("processing-instruction()");
				break;
			case DOMNode::TEXT_NODE:
				name = strdup("text()");
				break;
			default:
				name = XMLString::transcode(node->getNodeName());
				break;
		}

		xmlSetProp(e, BAD_CAST "name", BAD_CAST name);
		delete name;

		/* Get the position of the node relative to other nodes with
		 * the same name.
		 */
		if (type != DOMNode::ATTRIBUTE_NODE) {
			int n = 1;
			xmlChar pos[16];

			for (DOMNode *cur = node->getFirstChild(); cur; cur = cur->getNextSibling()) {
				if (cur == node) {
					break;
				} else if (cur->getNodeType() == type && (type != DOMNode::ELEMENT_NODE || XMLString::equals(cur->getNodeName(), node->getNodeName()))) {
					++n;
				}
			}

			xmlStrPrintf(pos, 16, "%d", n);
			xmlSetProp(e, BAD_CAST "pos", pos);
		}

		node = node->getParentNode();
	}

	/* Construct the XPath 1.0 expression. */
	for (xmlNodePtr cur = path->last; cur; cur = cur->prev) {
		xmlChar *name = xmlGetProp(cur, BAD_CAST "name");
		xmlChar *pos  = xmlGetProp(cur, BAD_CAST "pos");

		dst = xmlStrcat(dst, BAD_CAST "/");

		if (!pos) {
			dst = xmlStrcat(dst, BAD_CAST "@");
		}
		dst = xmlStrcat(dst, name);
		if (pos) {
			dst = xmlStrcat(dst, BAD_CAST "[");
			dst = xmlStrcat(dst, pos);
			dst = xmlStrcat(dst, BAD_CAST "]");
		}

		xmlFree(name);
		xmlFree(pos);
	}

	xmlFreeNode(path);

	return dst;
}

/* Get Xerces and XQilla version information. */
extern "C" const char *xqilla_version(void)
{
	return XERCES_FULLVERSIONDOT;
}

/* Initialize XQilla and Xerces, and return the DOM implementation. */
extern "C" void *xqilla_initialize(void)
{
	XQillaPlatformUtils::initialize();
	return DOMImplementationRegistry::getDOMImplementation(X("XPath2"));
}

/* Cleanup XQilla and Xerces. */
extern "C" void xqilla_terminate(void)
{
	XQillaPlatformUtils::terminate();
}

/* Create a new DOMLSParser. */
extern "C" void *xqilla_create_parser(void *impl)
{
	DOMImplementation *i = (DOMImplementation *) impl;
	DOMLSParser *parser = i->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
	return parser;
}

/* Release a DOMLSParser. */
extern "C" void xqilla_free_parser(void *parser)
{
	DOMLSParser *p = (DOMLSParser *) parser;
	p->release();
}

/* Create a Xerces DOMDocument from a libxml xmlDocPtr. */
extern "C" void *xqilla_create_doc(void *impl, void *parser, xmlDocPtr doc)
{
	DOMImplementation *i = (DOMImplementation *) impl;
	DOMLSParser *p = (DOMLSParser *) parser;
	xmlChar *xml;
	int size;

	/* The DTD in S1000D CSDB objects is often incorrect, which causes
	 * issues with stricter parsers like Xerces, so this strips it off
	 * before parsing. It should not be required for BREX validation.
	 */
	xmlDocPtr doc_no_dtd = xmlNewDoc(BAD_CAST "1.0");
	xmlDocSetRootElement(doc_no_dtd, xmlCopyNode(xmlDocGetRootElement(doc), 1));

	xmlDocDumpMemory(doc_no_dtd, &xml, &size);

	xmlFreeDoc(doc_no_dtd);

	DOMLSInput *input = i->createLSInput();

	XMLCh *stringData = XMLString::transcode((char *) xml);
	input->setStringData(stringData);

	DOMDocument *d = p->parse(input);

	xmlFree(xml);
	delete stringData;
	delete input;

	return d;
}

/* Create a new DOMXPathNSResolver. */
extern "C" void *xqilla_create_ns_resolver(void *doc)
{
	DOMDocument *d = (DOMDocument *) doc;
	DOMXPathNSResolver *resolver = d->createNSResolver(NULL);

	/* Apparently required for default XPath 2.0 functions/types to work? */
	resolver->addNamespaceBinding(X("xs"), X("http://www.w3.org/2001/XMLSchema"));

	return resolver;
}

/* Register a namespace with a DOMXPathNSResolver. */
extern "C" void xqilla_register_namespace(void *resolver, const xmlChar *prefix, const xmlChar *uri)
{
	DOMXPathNSResolver *r = (DOMXPathNSResolver *) resolver;
	r->addNamespaceBinding(X((const char *) prefix), X((const char *) uri));
}

/* Evaluate an XPath 2.0 expression using XQilla and return a libxml nodeset. */
extern "C" xmlXPathObjectPtr xqilla_eval_xpath(void *doc, void *ns_resolver, const xmlChar *expr, xmlXPathContextPtr ctx)
{
	DOMDocument *d = (DOMDocument *) doc;
	DOMXPathNSResolver *n = (DOMXPathNSResolver *) ns_resolver;

	try {
		AutoRelease<DOMXPathExpression> expression(d->createExpression(X((const char *) expr), n));
		AutoRelease<DOMXPathResult> result(expression->evaluate(d, DOMXPathResult::ITERATOR_RESULT_TYPE, 0));
		xmlNodeSetPtr nodeset = xmlXPathNodeSetCreate(NULL);
		xmlXPathObjectPtr obj = NULL;

		while (result->iterateNext()) {
			if (result->isNode()) {
				DOMNode *node = result->getNodeValue();
				xmlChar *xpath = xpath_of(node);
				xmlXPathObjectPtr o = xmlXPathEval(xpath, ctx);

				if (o && !xmlXPathNodeSetIsEmpty(o->nodesetval)) {
					xmlXPathNodeSetAdd(nodeset, o->nodesetval->nodeTab[0]);
				}

				xmlFree(xpath);
				xmlXPathFreeObject(o);

			} else {
				obj = xmlXPathNewBoolean(result->getBooleanValue());
			}
		}

		if (!obj) {
			obj = xmlXPathNewNodeSetList(nodeset);
		}

		xmlXPathFreeNodeSet(nodeset);

		return obj;
	} catch (DOMXPathException e) {
		char *m = XMLString::transcode(e.msg);
		std::cerr << "XQilla: " << m << std::endl;
		delete m;

		return NULL;
	}
}
