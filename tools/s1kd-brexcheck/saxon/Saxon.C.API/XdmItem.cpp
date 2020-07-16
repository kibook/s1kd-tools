

#include "XdmItem.h"

XdmItem::XdmItem(): XdmValue(){
	value = NULL;
}

    XdmItem::XdmItem(const XdmItem &other): XdmValue(other){
	value = (sxnc_value *)malloc(sizeof(sxnc_value));
        value->xdmvalue = other.value->xdmvalue;
	xdmSize =1;
	refCount = other.refCount;
    }


XdmItem::XdmItem(jobject obj){
	value = (sxnc_value *)malloc(sizeof(sxnc_value));
        value->xdmvalue = obj;
	xdmSize =1;
	refCount =1;
}

bool XdmItem::isAtomic(){
	return false;
}




   XdmItem * XdmItem::getHead(){ return this;}

  XdmItem * XdmItem::itemAt(int n){
	if (n < 0 || n >= size()) {
		return NULL;	
	}
	return this;
  }



 int XdmItem::size(){
	return 1;	
   }

jobject XdmItem::getUnderlyingValue(){
#ifdef DEBUG
	std::cerr<<std::endl<<"XdmItem-getUnderlyingValue:"<<std::endl; 
#endif 
	if(value == NULL) {
		return NULL;	
	}
	return value->xdmvalue;
}

    const char * XdmItem::getStringValue(){
        if(stringValue.empty()) {
    		jclass xdmItemClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmItem");
    		jmethodID sbmID = (jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(xdmItemClass,
    					"getStringValue",
    					"()Ljava/lang/String;");
    		if (!sbmID) {
    			std::cerr << "Error: Saxonc." << "getStringValue"
    				<< " not found\n" << std::endl;
    			return NULL;
    		} else {
    			jstring result = (jstring)(SaxonProcessor::sxn_environ->env->CallObjectMethod(value->xdmvalue, sbmID));
    			if(result) {
    					const char * str = SaxonProcessor::sxn_environ->env->GetStringUTFChars(result, NULL);
    					stringValue = std::string(str);
                        return stringValue.c_str();
    			}
    			return NULL;
    		}
    	} else {
    		return stringValue.c_str();
    	}
   }

	/**
	* Get the type of the object
	*/
	XDM_TYPE XdmItem::getType(){
		return XDM_ITEM;
	}
