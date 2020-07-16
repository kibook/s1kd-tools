
#include "XdmNode.h"


/*
const char * baseURI;
	const char * nodeName;
	XdmNode ** children; //caches child nodes when getChildren method is first called;
	int childCount;
	XdmNode * parent;
	XdmNode ** attrValues;//caches attribute nodes when getAttributeNodes method is first called;
	int attrCount;
	XDM_NODE_KIND nodeKind;
*/

XdmNode::XdmNode(jobject obj): XdmItem(obj), baseURI(NULL), nodeName(NULL), children(NULL), childCount(-1), parent(NULL), typedValue(NULL), attrValues(NULL), attrCount(-1), nodeKind(UNKNOWN){

}

XdmNode::XdmNode(XdmNode * p, jobject obj, XDM_NODE_KIND kind): XdmItem(obj), baseURI(NULL), nodeName(NULL), children(NULL),  childCount(-1), parent(p), typedValue(NULL), attrValues(NULL),  attrCount(-1), nodeKind(kind){}

bool XdmNode::isAtomic() {
	return false;
} 
    
    XDM_NODE_KIND XdmNode::getNodeKind(){
	if(nodeKind == UNKNOWN && proc != NULL) {
		nodeKind = static_cast<XDM_NODE_KIND>(proc->getNodeKind(value->xdmvalue));
	} 
	return nodeKind;

    }

    const char * XdmNode::getNodeName(){
	
	if(nodeName != NULL) {
		return nodeName;
	}
	XDM_NODE_KIND kind = getNodeKind();
 	jclass xdmUtilsClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/XdmUtils");
	jmethodID xmID = (jmethodID) SaxonProcessor::sxn_environ->env->GetStaticMethodID(xdmUtilsClass,"getNodeName",
					"(Lnet/sf/saxon/s9api/XdmNode;)Ljava/lang/String;");
	switch (kind) {
            case DOCUMENT:
            case TEXT:
            case COMMENT:
                return NULL;
            case PROCESSING_INSTRUCTION:
            case NAMESPACE:
            case ELEMENT:
            case ATTRIBUTE:
               
		if (!xmID) {
			std::cerr << "Error: MyClassInDll." << "getNodeName"<< " not found\n" << std::endl;
			return NULL;
		} else {
			jstring result = (jstring)(SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(xdmUtilsClass, xmID, value->xdmvalue));
			if(!result) {
				return NULL;
			} else {
				nodeName = SaxonProcessor::sxn_environ->env->GetStringUTFChars(result, NULL);
				return nodeName;
			} 
		}
            default:
                return NULL;
        }
	

    }

    XdmValue * XdmNode::getTypedValue(){
    	if(typedValue == NULL) {
    		jclass xdmNodeClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmNode");
    		jmethodID tbmID = (jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(xdmNodeClass,
    					"getTypedValue",
    					"()Lnet/sf/saxon/s9api/XdmValue;");
    		if (!tbmID) {
    			std::cerr << "Error: Saxonc." << "getTypedValue"
    				<< " not found\n" << std::endl;
    			return NULL;
    		} else {
    			jobject valueObj = (SaxonProcessor::sxn_environ->env->CallObjectMethod(value->xdmvalue, tbmID));
    			if(valueObj) {
    				typedValue = new XdmValue((proc == NULL ? NULL : proc));
    				typedValue->addUnderlyingValue(valueObj);
    				return typedValue;
    			}
    			return NULL;
    		}
    	} else {
    		return typedValue;
    	}


    }

    const char * XdmNode::getStringValue(){
   		return XdmItem::getStringValue();
    }

    const char * XdmNode::toString(){
       	if(stringValue.empty()) {
        		jclass xdmNodeClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmNode");
        		jmethodID strbMID = (jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(xdmNodeClass,
        					"toString",
        					"()Ljava/lang/String;");
        		if (!strbMID) {
        			std::cerr << "Error: Saxonc." << "toString"
        				<< " not found\n" << std::endl;
        			return NULL;
        		} else {
        			jstring result = (jstring) (SaxonProcessor::sxn_environ->env->CallObjectMethod(value->xdmvalue, strbMID));
        			if(result) {
                       const char * str = SaxonProcessor::sxn_environ->env->GetStringUTFChars(result, NULL);
                       stringValue = str;
                  		return str;
                }
                   return NULL;
        		}
        	} else {
        		return stringValue.c_str();
        	}
    }

    
    const char* XdmNode::getBaseUri(){

	if(baseURI != NULL) {
		return baseURI;
	}

	jclass xdmNodeClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmNode");
	jmethodID bmID = (jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(xdmNodeClass,
					"getBaseURI",
					"()Ljava/net/URI;");
	if (!bmID) {
		std::cerr << "Error: MyClassInDll." << "getBaseURI"
				<< " not found\n" << std::endl;
		return NULL;
	} else {
		jobject nodeURIObj = (SaxonProcessor::sxn_environ->env->CallObjectMethod(value->xdmvalue, bmID));
		if(!nodeURIObj){
			return NULL;
		} else {
			jclass URIClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/net/URI");
			jmethodID strMID = (jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(URIClass,
					"toString",
					"()Ljava/lang/String;");
			if(strMID){
				jstring result = (jstring)(
				SaxonProcessor::sxn_environ->env->CallObjectMethod(nodeURIObj, strMID));
				baseURI = SaxonProcessor::sxn_environ->env->GetStringUTFChars(result,
					NULL);
			
				return baseURI;
			}	
		}
	}
	return NULL;	
    }
    
    




    XdmNode* XdmNode::getParent(){
	if(parent == NULL && proc!= NULL) {
		jclass xdmNodeClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmNode");
		jmethodID bmID = (jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(xdmNodeClass,
					"getParent",
					"()Lnet/sf/saxon/s9api/XdmNode;");
		if (!bmID) {
			std::cerr << "Error: MyClassInDll." << "getParent"
				<< " not found\n" << std::endl;
			return NULL;
		} else {
			jobject nodeObj = (SaxonProcessor::sxn_environ->env->CallObjectMethod(value->xdmvalue, bmID));
			if(nodeObj) {
				parent = new XdmNode(NULL, nodeObj, UNKNOWN);
				parent->setProcessor(proc);
				//parent->incrementRefCount();
				return parent;
			}
			return NULL;
		}
	} else {
		return parent;
	}
	
    }
    
    const char* XdmNode::getAttributeValue(const char *str){

	if(str == NULL) { return NULL;}
	jclass xdmUtilsClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/XdmUtils");
	jmethodID xmID = (jmethodID) SaxonProcessor::sxn_environ->env->GetStaticMethodID(xdmUtilsClass,"getAttributeValue",
					"(Lnet/sf/saxon/s9api/XdmNode;Ljava/lang/String;)Ljava/lang/String;");
	if (!xmID) {
			std::cerr << "Error: SaxonDll." << "getAttributeValue"
				<< " not found\n" << std::endl;
			return NULL;
		}
	if(str == NULL) {
		return NULL;
	}
	jstring eqname = SaxonProcessor::sxn_environ->env->NewStringUTF(str);

	jstring result = (jstring)(SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(xdmUtilsClass, xmID,value->xdmvalue, eqname));
	SaxonProcessor::sxn_environ->env->DeleteLocalRef(eqname);
	//failure = checkForException(sxn_environ,  (jobject)result);//Remove code
	if(result) {
		const char * stri = SaxonProcessor::sxn_environ->env->GetStringUTFChars(result,
					NULL);
		
		//SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);

		return stri;
	} else {

		return NULL;
	}

    }

    XdmNode** XdmNode::getAttributeNodes(){
	if(attrValues == NULL) {
		jclass xdmUtilsClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/XdmUtils");
		jmethodID xmID = (jmethodID) SaxonProcessor::sxn_environ->env->GetStaticMethodID(xdmUtilsClass,"getAttributeNodes",
					"(Lnet/sf/saxon/s9api/XdmNode;)[Lnet/sf/saxon/s9api/XdmNode;");
		jobjectArray results = (jobjectArray)(SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(xdmUtilsClass, xmID, 
		value->xdmvalue));
		if(results == NULL) {
			return NULL;	
		}
		int sizex = SaxonProcessor::sxn_environ->env->GetArrayLength(results);
		attrCount = sizex;
		if(sizex>0) {	
			attrValues =  new XdmNode*[sizex];
			XdmNode * tempNode =NULL;
			for (int p=0; p < sizex; ++p){
				jobject resulti = SaxonProcessor::sxn_environ->env->GetObjectArrayElement(results, p);
				tempNode = new XdmNode(this, resulti, ATTRIBUTE);
				tempNode->setProcessor(proc);
				this->incrementRefCount();
				attrValues[p] = tempNode;
			}
		}
	} 
	return attrValues;
    }

    int XdmNode::getAttributeCount(){
	if(attrCount == -1 && proc!= NULL) {
		jclass xdmUtilsClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/XdmUtils");
		jmethodID xmID = (jmethodID) SaxonProcessor::sxn_environ->env->GetStaticMethodID(xdmUtilsClass,"getAttributeCount",
					"(Lnet/sf/saxon/s9api/XdmNode;)I");
		
		if (!xmID) {
			std::cerr << "Error: SaxonDll." << "getAttributeCount"
				<< " not found\n" << std::endl;
			return 0;
		}
		jint result = (jlong)(SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(xdmUtilsClass, xmID,
		value->xdmvalue));

		attrCount =(int)result;
	}
	return attrCount;
    }

    int XdmNode::getChildCount(){
	if(childCount == -1 && proc!= NULL) {
		jclass xdmUtilsClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/XdmUtils");
		jmethodID xmID = (jmethodID) SaxonProcessor::sxn_environ->env->GetStaticMethodID(xdmUtilsClass,"getChildCount",
					"(Lnet/sf/saxon/s9api/XdmNode;)I");
		
		if (!xmID) {
			std::cerr << "Error: SaxonDll." << "getchildCount"
				<< " not found\n" << std::endl;
			return 0;
		}
		jint result = (jlong)(SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(xdmUtilsClass, xmID,
		value->xdmvalue));

		childCount =(int)result;
	}
	return childCount;
    }
    
    XdmNode** XdmNode::getChildren(){

	if(children == NULL && proc!= NULL) {
		jclass xdmUtilsClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/XdmUtils");
		jmethodID xmID = (jmethodID) SaxonProcessor::sxn_environ->env->GetStaticMethodID(xdmUtilsClass,"getChildren",
					"(Lnet/sf/saxon/s9api/XdmNode;)[Lnet/sf/saxon/s9api/XdmNode;");
		
		if (!xmID) {
			std::cerr << "Error: SaxonDll." << "getchildren"
				<< " not found\n" << std::endl;
			return NULL;
		}
		jobjectArray results = (jobjectArray)(SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(xdmUtilsClass, xmID, 
		value->xdmvalue));
		int sizex = SaxonProcessor::sxn_environ->env->GetArrayLength(results);
		childCount = sizex;	
		children =  new XdmNode*[sizex];
		XdmNode * tempNode = NULL;
		for (int p=0; p < sizex; ++p){
			jobject resulti = SaxonProcessor::sxn_environ->env->GetObjectArrayElement(results, p);
			tempNode = new XdmNode(this, resulti, UNKNOWN);
			tempNode->setProcessor(proc);
			//tempNode->incrementRefCount();
			children[p] = tempNode;
		}
	}
	return children;

    }
  
