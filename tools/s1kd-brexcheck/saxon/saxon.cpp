#include <string>
#include <libxml/xpathInternals.h>
#include "SaxonProcessor.h"
#include "XdmValue.h"
#include "XdmItem.h"
#include "XdmNode.h"
#include "XdmAtomicValue.h"
#include "saxon.h"

/* Return the string value of the first node matched by an XPath expression. */
static char *first_xpath_value(SaxonProcessor *processor, XdmNode *node, const char *expr)
{
	XPathProcessor *xpath_proc;
	XdmValue *res;
	char *ret;

	xpath_proc = processor->newXPathProcessor();
	xpath_proc->setContextItem(node);

	res = xpath_proc->evaluateSingle(expr);

	if (res == NULL) {
		ret = NULL;
	} else {
		ret = strdup(res->toString());
	}

	delete res;
	delete xpath_proc;

	return ret;
}

/* Generate an XPath expression to an XdmNode. */
static xmlChar *xpath_of(XdmNode *node, SaxonProcessor *processor)
{
	xmlNodePtr path, cur;
	xmlChar *dst = NULL;

	/* FIXME: Not sure how to handle node types besides XDM_ATOMIC_VALUE
	 *        and XDM_NODE, so just returning an XPath expression to the
	 *        root of the document. */
	if (node->getType() != XDM_NODE) {
		return xmlCharStrdup("/*");
	}

	path = xmlNewNode(NULL, BAD_CAST "path");

	/* Build XPath expression node by traversing up the tree. */
	while (node) {
		xmlNodePtr e;
		const xmlChar *name;
		XdmNode *parent;
		XDM_NODE_KIND node_kind;
		char *node_ns, *node_name;

		node_kind = node->getNodeKind();

		if (node_kind == DOCUMENT) {
			break;
		}

		parent = node->getParent();

		/* Get namespace prefix of node. */
		node_ns = first_xpath_value(processor, node, "substring-before(name(), ':')");

		/* Get local name of node. */
		node_name = first_xpath_value(processor, node, "local-name()");

		e = xmlNewChild(path, NULL, BAD_CAST "node", NULL);

		if (node_ns != NULL && strcmp(node_ns, "") != 0) {
			xmlSetProp(e, BAD_CAST "ns", BAD_CAST node_ns);
		}

		switch (node_kind) {
			case COMMENT:
				name = BAD_CAST "comment()";
				break;
			case PROCESSING_INSTRUCTION:
				name = BAD_CAST "processing-instruction()";
				break;
			case TEXT:
				name = BAD_CAST "text()";
				break;
			default:
				name = BAD_CAST node_name;
				break;
		}

		xmlSetProp(e, BAD_CAST "name", name);

		/* Locate the node's position within its parent. */
		if (node_kind != ATTRIBUTE) {
			XPathProcessor *xpath_proc;
			XdmValue *preceding;
			const char *preceding_xpath;

			xpath_proc = processor->newXPathProcessor();
			xpath_proc->setContextItem(node);

			if (node_kind == COMMENT) {
				preceding_xpath = "preceding-sibling::comment()";
			} else if (node_kind == PROCESSING_INSTRUCTION) {
				preceding_xpath = "preceding-sibling::processing-instruction()";
			} else if (node_kind == TEXT) {
				preceding_xpath = "preceding-sibling::text()";
			} else {
				XdmAtomicValue *ns_val   = processor->makeStringValue(node_ns);
				XdmAtomicValue *name_val = processor->makeStringValue(node_name);
				xpath_proc->setParameter("ns", ns_val);
				xpath_proc->setParameter("name", name_val);
				preceding_xpath = "preceding-sibling::*[namespace-uri()=$ns and local-name()=$name]";
			}

			preceding = xpath_proc->evaluate(preceding_xpath);

			if (preceding) {
				xmlChar pos[16];
				xmlStrPrintf(pos, 16, "%d", preceding->size() + 1);
				xmlSetProp(e, BAD_CAST "pos", pos);
			} else{
				xmlSetProp(e, BAD_CAST "pos", BAD_CAST "1");
			}

			delete preceding;
			delete xpath_proc;
		}

		node = parent;

		free(node_ns);
		free(node_name);
	}

	/* Convert XPath expression node to string. */
	for (cur = path->last; cur; cur = cur->prev) {
		xmlChar *ns, *name, *pos;

		ns   = xmlGetProp(cur, BAD_CAST "ns");
		name = xmlGetProp(cur, BAD_CAST "name");
		pos  = xmlGetProp(cur, BAD_CAST "pos");

		dst = xmlStrcat(dst, BAD_CAST "/");
		if (!pos) {
			dst = xmlStrcat(dst, BAD_CAST "@");
		}
		if (ns) {
			dst = xmlStrcat(dst, ns);
			dst = xmlStrcat(dst, BAD_CAST ":");
		}
		dst = xmlStrcat(dst, name);
		if (pos) {
			dst = xmlStrcat(dst, BAD_CAST "[");
			dst = xmlStrcat(dst, pos);
			dst = xmlStrcat(dst, BAD_CAST "]");
		}

		xmlFree(ns);
		xmlFree(name);
		xmlFree(pos);
	}

	xmlFreeNode(path);

	return dst;
}

/* Report version of Saxon/C. */
extern "C" const char *saxon_version(void *saxon_processor)
{
	return ((SaxonProcessor *) saxon_processor)->version();
}

/* Create new Saxon processor. */
extern "C" void *saxon_new_processor(void)
{
	return new SaxonProcessor(false);
}

/* Free Saxon processor. */
extern "C" void saxon_free_processor(void *saxon_processor)
{
	SaxonProcessor *p = (SaxonProcessor *) saxon_processor;
	delete p;
}

/* Cleanup Saxon JVM. */
extern "C" void saxon_cleanup(void)
{
	SaxonProcessor::release();
}

/* Create new Saxon XPath processor. */
extern "C" void *saxon_new_xpath_processor(void *saxon_processor)
{
	SaxonProcessor *p = (SaxonProcessor *) saxon_processor;
	return p->newXPathProcessor();
}

/* Free Saxon XPath processor. */
extern "C" void saxon_free_xpath_processor(void *xpath_processor)
{
	XPathProcessor *p = (XPathProcessor *) xpath_processor;
	delete p;
}

/* Register a namespace prefix in a Saxon XPath processor. */
extern "C" void saxon_register_namespace(void *xpath_processor, const xmlChar *prefix, const xmlChar *href)
{
	((XPathProcessor *) xpath_processor)->declareNamespace((const char *) prefix, (const char *) href);
}

/* Create a Saxon XdmNode from a libxml2 doc. */
extern "C" void *saxon_new_node(void *saxon_processor, xmlDocPtr doc)
{
	SaxonProcessor *p = (SaxonProcessor *) saxon_processor;
	xmlChar *xml;
	int size;
	XdmNode *node;

	/* Pass libxml2 doc to Saxon processor as string. */
	xmlDocDumpMemory(doc, &xml, &size);
	node = p->parseXmlFromString((const char *) xml);
	xmlFree(xml);

	return node;
}

/* Free a Saxon XdmNode. */
extern "C" void saxon_free_node(void *saxon_node)
{
	XdmNode *node = (XdmNode *) saxon_node;

	delete node;
}

/* Use Saxon to evaluate an XPath expression in a libxml2 XPath context,
 * returning a libxml2 nodeset. */
extern "C" xmlXPathObjectPtr saxon_eval_xpath(void *saxon_processor, void *xpath_processor, void *saxon_node, xmlNodePtr ns, const xmlChar *expr, xmlXPathContextPtr ctx)
{
	SaxonProcessor *saxon_proc = (SaxonProcessor *) saxon_processor;
	XPathProcessor *xpath_proc = (XPathProcessor *) xpath_processor;
	XdmNode *node = (XdmNode *) saxon_node;
	XdmValue *value;
	xmlXPathObjectPtr obj;

	/* Evaluate the XPath expression. */
	xpath_proc->setContextItem(node);
	value = xpath_proc->evaluate((const char *) expr);

	if (value) {
		XDM_TYPE type = value->getType();

		/* Create XPath object with boolean interpretation of atomic value. */
		if (type == XDM_ATOMIC_VALUE) {
			obj = xmlXPathNewBoolean(((XdmAtomicValue *) value)->getBooleanValue());
		/* Create XPath object with a nodeset of the matched nodes. */
		} else {
			xmlNodeSetPtr nodeset;
			int count, i;

			/* Create an empty libxml2 nodeset. */
			nodeset = xmlXPathNodeSetCreate(NULL);

			/* For each Saxon node, construct an XPath 1.0 expression to it
			 * in the document, and use that to add the libxml2 node to the
			 * nodeset. */
			for (i = 0, count = value->size(); i < count; ++i) {
				XdmNode *n = (XdmNode *) value->itemAt(i);
				xmlChar *xpath = xpath_of(n, saxon_proc);
				xmlXPathObjectPtr o = xmlXPathEval(xpath, ctx);

				if (o && !xmlXPathNodeSetIsEmpty(o->nodesetval)) {
					xmlXPathNodeSetAdd(nodeset, o->nodesetval->nodeTab[0]);
				}

				xmlFree(xpath);
				xmlXPathFreeObject(o);
			}

			/* Wrap the nodeset in a libxml2 XPath object. */
			obj = xmlXPathNewNodeSetList(nodeset);
		}
	} else {
		/* Default XPath object if nothing was found. */
		obj = xmlXPathNewBoolean(false);
	}

	delete value;

	return obj;
}
