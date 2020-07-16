////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// This Source Code Form is "Incompatible With Secondary Licenses", as defined by the Mozilla Public License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SAXON_XDMNODE_h
#define SAXON_XDMNODE_h

#include "XdmItem.h"
#include <string.h>
//#include "XdmValue.h"

typedef enum eXdmNodeKind { DOCUMENT = 9, ELEMENT = 1, ATTRIBUTE = 2, TEXT = 3, COMMENT = 8, PROCESSING_INSTRUCTION = 7, NAMESPACE = 13, UNKNOWN = 0 } XDM_NODE_KIND;



class XdmValue;

class XdmNode : public XdmItem
{

public:


	/* XdmNode(const XdmValue& valuei){
	 //value = (sxnc_value *)malloc(sizeof(sxnc_value));
		 value = valuei.values[0]->getUnderlyingCValue();
	 xdmSize =1;
	 refCount = 1;
	 nodeKind = UNKNOWN;
	 }*/

	XdmNode(jobject);

	XdmNode(XdmNode *parent, jobject, XDM_NODE_KIND);

	virtual ~XdmNode() {
		if (getRefCount() <1){
				delete baseURI;
			delete nodeName;
		}

		//There might be potential issues with children and attribute node not being deleted when the parent node has been deleted
		//we need to track this kind of problem.
	}

	virtual bool isAtomic();


	XDM_NODE_KIND getNodeKind();

	/**
	 * Get the name of the node, as a string in the form of a EQName
	 *
	 * @return the name of the node. In the case of unnamed nodes (for example, text and comment nodes)
	 *         return null.
	 */
	const char * getNodeName();

	/**
	 * Get the typed value of this node, as defined in XDM
	 *
	 * @return the typed value. If the typed value is a single atomic value, this will be returned
	 * as an instance of {@link XdmAtomicValue}
	 */
	XdmValue * getTypedValue();

	const char* getBaseUri();

	/**
	 * Get the string value of the item. For a node, this gets the string value
	 * of the node. For an atomic value, it has the same effect as casting the value
	 * to a string. In all cases the result is the same as applying the XPath string()
	 * function.
	 * <p>For atomic values, the result is the same as the result of calling
	 * <code>toString</code>. This is not the case for nodes, where <code>toString</code>
	 * returns an XML serialization of the node.</p>
	 *
	 * @return the result of converting the item to a string.
	 * @deprecated the SaxonProcessor argument. It has been removed from release version 1.2.1
	 */
	const char * getStringValue();

	
	const char * toString();

	XdmNode* getParent();


	const char* getAttributeValue(const char *str);

	int getAttributeCount();

	XdmNode** getAttributeNodes();

	jobject getUnderlyingValue() {

		return XdmItem::getUnderlyingValue();

	}


	XdmNode** getChildren();

	int getChildCount();

	/**
   * Get the type of the object
   */
	XDM_TYPE getType() {
		return XDM_NODE;
	}

	// const char* getOuterXml();



private:
	const char * baseURI;
	const char * nodeName;
	XdmNode ** children; //caches child nodes when getChildren method is first called;
	int childCount;
	XdmNode * parent;
	XdmValue * typedValue;
	XdmNode ** attrValues;//caches attribute nodes when getAttributeNodes method is first called;
	int attrCount;
	XDM_NODE_KIND nodeKind;

};

#endif
