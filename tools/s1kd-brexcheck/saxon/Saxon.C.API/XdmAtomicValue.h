////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// This Source Code Form is "Incompatible With Secondary Licenses", as defined by the Mozilla Public License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SAXON_XDMATOMICVALUE_h
#define SAXON_XDMATOMICVALUE_h


#include "XdmItem.h"
#include <string>

#include <stdlib.h>
#include <string.h>
//#include <dlfcn.h>


class XdmAtomicValue : public XdmItem {
    
public:

    XdmAtomicValue();

    XdmAtomicValue(const XdmAtomicValue &d);


    virtual ~XdmAtomicValue(){
	//std::cerr<<"destructor called fpr XdmAtomicValue"<<std::endl;
	if(!valType.empty()) {
		valType.clear();
	}
    }

    XdmAtomicValue(jobject);

    XdmAtomicValue(jobject, const char* ty);

    const char* getPrimitiveTypeName();

    bool getBooleanValue();

    double getDoubleValue();

    const char * getStringValue();

    long getLongValue();

    void setType(const char* ty);

    
    bool isAtomic(){
        return true;
    }

	/**
	* Get the type of the object
	*/
	XDM_TYPE getType() {
		return XDM_ATOMIC_VALUE;
	}
    
    
private:
     
    std::string valType;


};




#endif
