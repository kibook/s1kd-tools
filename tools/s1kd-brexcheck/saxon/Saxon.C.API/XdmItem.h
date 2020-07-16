////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// This Source Code Form is "Incompatible With Secondary Licenses", as defined by the Mozilla Public License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SAXON_XDMITEM_h
#define SAXON_XDMITEM_h

#include "XdmValue.h"

class SaxonProcessor;

class XdmItem : public XdmValue
{

public:

     XdmItem();

     XdmItem(jobject);

    XdmItem(const XdmItem &item);

	
    virtual ~XdmItem(){
	//std::cerr<<std::endl<<"XdmItem destructor called, refCount"<<getRefCount()<<std::endl;
	if(getRefCount()<1){
	  if(value !=NULL && proc != NULL && SaxonProcessor::jvmCreatedCPP>0) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(value->xdmvalue);
	  }
	  free(value);
	  if(stringValue.empty()) {
	    stringValue.clear();
	  }
        }
    }

virtual void incrementRefCount() {
		refCount++;
		//std::cerr<<"refCount-inc-xdmItem="<<refCount<<" ob ref="<<(this)<<std::endl;
	}

virtual void decrementRefCount() {
		if (refCount > 0)
			refCount--;
		//std::cerr<<"refCount-dec-xdmItem="<<refCount<<" ob ref="<<(this)<<std::endl;
	}
    
    virtual bool isAtomic();

//TODO: isNode
//TODO: isFunction

    /**
     * Get Java XdmValue object.
     * @return jobject - The Java object of the XdmValue in its JNI representation
     */
     virtual  jobject getUnderlyingValue();

     sxnc_value * getUnderlyingCValue(){
	return value;
     }


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
     virtual const char * getStringValue();

 /**
     * Get the first item in the sequence
     * @return XdmItem or null if sequence is empty
     */
	XdmItem * getHead();

  /**
     * Get the n'th item in the value, counting from zero.
     *
     * @param n the item that is required, counting the first item in the sequence as item zero
     * @return the n'th item in the sequence making up the value, counting from zero
     * return NULL  if n is less than zero or greater than or equal to the number
     *                                    of items in the value
     * return NULL if the value is lazily evaluated and the delayed
     *                                    evaluation fails with a dynamic error.
     */
	XdmItem * itemAt(int n);

    /**
     * Get the number of items in the sequence
     *
     */
      int size();



	/**
	* Get the type of the object
	*/
	virtual XDM_TYPE getType();

 protected:  
	sxnc_value* value;
	std::string stringValue;  /*!< Cached. String representation of the XdmValue, if available */
};


#endif
