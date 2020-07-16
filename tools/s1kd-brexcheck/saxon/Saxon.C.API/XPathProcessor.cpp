#include "XPathProcessor.h"
#include "XdmValue.h"
#include "XdmItem.h"
#include "XdmNode.h"
#include "XdmAtomicValue.h"

XPathProcessor::XPathProcessor() {
	SaxonProcessor *p = new SaxonProcessor(false);
	XPathProcessor(p, "");
}

XPathProcessor::XPathProcessor(SaxonProcessor* p, std::string curr) {
	proc = p;

	/*
	 * Look for class.
	 */
	cppClass = lookForClass(SaxonProcessor::sxn_environ->env,
			"net/sf/saxon/option/cpp/XPathProcessor");
	if ((proc->proc) == NULL) {
		std::cerr << "Processor is NULL" << std::endl;
	}

	cppXP = createSaxonProcessor2(SaxonProcessor::sxn_environ->env, cppClass,
			"(Lnet/sf/saxon/s9api/Processor;)V", proc->proc);



#ifdef DEBUG
	jmethodID debugMID = SaxonProcessor::sxn_environ->env->GetStaticMethodID(cppClass, "setDebugMode", "(Z)V");
	SaxonProcessor::sxn_environ->env->CallStaticVoidMethod(cppClass, debugMID, (jboolean)true);
#endif    

	proc->exception = NULL;
	if(!(proc->cwd.empty()) && curr.empty()){
		cwdXP = proc->cwd;
	} else {
		cwdXP = curr;
	}

}

XdmValue * XPathProcessor::evaluate(const char * xpathStr) {
	if (xpathStr == NULL) {
		std::cerr << "Error:: XPath string cannot be empty or NULL" << std::endl;
	return NULL;
}
setProperty("resources", proc->getResourcesDirectory());
jmethodID mID =
		(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass, "evaluate",
				"(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)[Lnet/sf/saxon/s9api/XdmValue;");
if (!mID) {
	std::cerr << "Error: "<<getDllname() << ".evaluate" << " not found\n"
			<< std::endl;

} else {
	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;
	jclass objectClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/Object");
	jclass stringClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/String");

	int size = parameters.size() + properties.size();

	if (size > 0) {
		objectArray = SaxonProcessor::sxn_environ->env->NewObjectArray((jint) size,
				objectClass, 0);
		stringArray = SaxonProcessor::sxn_environ->env->NewObjectArray((jint) size,
				stringClass, 0);
		int i = 0;
		for (std::map<std::string, XdmValue*>::iterator iter = parameters.begin();
				iter != parameters.end(); ++iter, i++) {
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(stringArray, i,
					SaxonProcessor::sxn_environ->env->NewStringUTF((iter->first).c_str()));
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(objectArray, i,
					(iter->second)->getUnderlyingValue());
		}
		for (std::map<std::string, std::string>::iterator iter = properties.begin();
				iter != properties.end(); ++iter, i++) {
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(stringArray, i,
					SaxonProcessor::sxn_environ->env->NewStringUTF((iter->first).c_str()));
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(objectArray, i,
					SaxonProcessor::sxn_environ->env->NewStringUTF((iter->second).c_str()));
		}
	} 
	jobjectArray results = (jobjectArray)(
			SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXP, mID,
					SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXP.c_str()),
					SaxonProcessor::sxn_environ->env->NewStringUTF(xpathStr), stringArray, objectArray));
	if(!results) {
		proc->checkAndCreateException(cppClass);
		return NULL;
	}
	
	int sizex = SaxonProcessor::sxn_environ->env->GetArrayLength(results);
	if (size > 0) {
		SaxonProcessor::sxn_environ->env->DeleteLocalRef(stringArray);
		SaxonProcessor::sxn_environ->env->DeleteLocalRef(objectArray);
	}
	if (sizex>0) {
		jclass atomicValueClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmAtomicValue");
		jclass nodeClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmNode");
		jclass functionItemClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmFunctionItem");

		XdmValue * value = new XdmValue();
		XdmItem * xdmItem = NULL;
		for (int p=0; p < sizex; ++p) 
		{
			jobject resulti = SaxonProcessor::sxn_environ->env->GetObjectArrayElement(results, p);
			//value->addUnderlyingValue(resulti);

			if(SaxonProcessor::sxn_environ->env->IsInstanceOf(resulti, atomicValueClass)           == JNI_TRUE) {
				xdmItem = new XdmAtomicValue(resulti);

			} else if(SaxonProcessor::sxn_environ->env->IsInstanceOf(resulti, nodeClass)           == JNI_TRUE) {
				xdmItem = new XdmNode(resulti);

			} else if (SaxonProcessor::sxn_environ->env->IsInstanceOf(resulti, functionItemClass)           == JNI_TRUE) {
				continue;
			}
			xdmItem->setProcessor(proc);
			if(sizex == 1) {
				value->decrementRefCount();
				delete value;
				SaxonProcessor::sxn_environ->env->DeleteLocalRef(results);
				return xdmItem;		
			}
			
			value->addXdmItem(xdmItem);
		}
		value->setProcessor(proc);
		SaxonProcessor::sxn_environ->env->DeleteLocalRef(results);
		return value;
	}
}
return NULL;

}

XdmItem * XPathProcessor::evaluateSingle(const char * xpathStr) {
	if (xpathStr == NULL) {
		std::cerr << "Error:: XPath string cannot be empty or NULL" << std::endl;
	     return NULL;
        }
setProperty("resources", proc->getResourcesDirectory());
jmethodID mID =
		(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass, "evaluateSingle",
				"(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Lnet/sf/saxon/s9api/XdmItem;");
if (!mID) {
	std::cerr << "Error: MyClassInDll." << "evaluateSingle" << " not found\n"
			<< std::endl;

} else {
	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;
	jclass objectClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/Object");
	jclass stringClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/String");

	int size = parameters.size() + properties.size();
#ifdef DEBUG
		std::cerr<<"Properties size: "<<properties.size()<<std::endl;
		std::cerr<<"Parameter size: "<<parameters.size()<<std::endl;
		std::cerr<<"size:"<<size<<std::endl;
#endif
	if (size > 0) {
		objectArray = SaxonProcessor::sxn_environ->env->NewObjectArray((jint) size,
				objectClass, 0);
		stringArray = SaxonProcessor::sxn_environ->env->NewObjectArray((jint) size,
				stringClass, 0);
		int i = 0;
		for (std::map<std::string, XdmValue*>::iterator iter = parameters.begin();
				iter != parameters.end(); ++iter, i++) {
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(stringArray, i,
					SaxonProcessor::sxn_environ->env->NewStringUTF((iter->first).c_str()));
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(objectArray, i,
					(iter->second)->getUnderlyingValue());
#ifdef DEBUG
				std::string s1 = typeid(iter->second).name();
				std::cerr<<"Type of itr:"<<s1<<std::endl;
				jobject xx = (iter->second)->getUnderlyingValue();
				if(xx == NULL) {
					std::cerr<<"value failed"<<std::endl;
				} else {

					std::cerr<<"Type of value:"<<(typeid(xx).name())<<std::endl;
				}
				if((iter->second)->getUnderlyingValue() == NULL) {
					std::cerr<<"(iter->second)->getUnderlyingValue() is NULL"<<std::endl;
				}
#endif
		}
		for (std::map<std::string, std::string>::iterator iter = properties.begin();
				iter != properties.end(); ++iter, i++) {
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(stringArray, i,
					SaxonProcessor::sxn_environ->env->NewStringUTF((iter->first).c_str()));
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(objectArray, i,
					SaxonProcessor::sxn_environ->env->NewStringUTF((iter->second).c_str()));
		}
	}
	jobject result = (jobject)(
			SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXP, mID,
					SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXP.c_str()),
					SaxonProcessor::sxn_environ->env->NewStringUTF(xpathStr), stringArray, objectArray));
	if (size > 0) {
		SaxonProcessor::sxn_environ->env->DeleteLocalRef(stringArray);
		SaxonProcessor::sxn_environ->env->DeleteLocalRef(objectArray);
	}
	if (result) {
		jclass atomicValueClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmAtomicValue");
		jclass nodeClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmNode");
		jclass functionItemClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmFunctionItem");
		XdmItem * xdmItem = NULL;
		if(SaxonProcessor::sxn_environ->env->IsInstanceOf(result, atomicValueClass)           == JNI_TRUE) {
			xdmItem = new XdmAtomicValue(result);

		} else if(SaxonProcessor::sxn_environ->env->IsInstanceOf(result, nodeClass)           == JNI_TRUE) {
			
			xdmItem = new XdmNode(result);

		} else if (SaxonProcessor::sxn_environ->env->IsInstanceOf(result, functionItemClass)           == JNI_TRUE) {
			return NULL;
		}
		xdmItem->setProcessor(proc);
		//SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
		return xdmItem;
	} else  {
		proc->checkAndCreateException(cppClass);
	   		
     		}
}

return NULL;

}

void XPathProcessor::setContextItem(XdmItem * item) {
	contextItem = item;
    	if(item != NULL){
     	 parameters["node"] = (XdmValue *)item;
    	}
}

void XPathProcessor::setContextFile(const char * filename) {
	if (filename != NULL) {
		setProperty("s", filename);
	}
}

/* Fix for bug #4513: https://saxonica.plan.io/issues/4513 */
void XPathProcessor::declareNamespace(const char *prefix, const char * uri){
        if (prefix == NULL || uri == NULL) {
		return;
        }
	jmethodID mID =
		/*(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass, "declareNamespace",
				"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");*/
		(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass, "declareNamespace",
				"(Ljava/lang/String;Ljava/lang/String;)V");
	if (!mID) {
	/*std::cerr << "Error: "<<getDllname() << ".declareNameSpace" << " not found\n"
			<< std::endl;*/
	std::cerr << "Error: "<<getDllname() << ".declareNamespace" << " not found\n"
			<< std::endl;

	} else {
		SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXP, mID,
				SaxonProcessor::sxn_environ->env->NewStringUTF(prefix),
				SaxonProcessor::sxn_environ->env->NewStringUTF(uri));
		//proc->checkAndCreateException(cppClass);
	}

}

#if CVERSION_API_NO >= 121


void XPathProcessor::setBackwardsCompatible(bool option) {
    if (option) {
        		setProperty("backwardsCom:", "true");
    } else {
       properties.erase("backwardsCom:");
    }
}

void XPathProcessor::setCaching(bool caching){
    if (caching) {
        	setProperty("caching:", "true");
    } else {
       properties.erase("caching:");
    }

}

void XPathProcessor::importSchemaNamespace(const char* uri){
      if (uri != NULL) {
       		setProperty("importSN", uri);
      }
}


#endif


void XPathProcessor::setBaseURI(const char * uriStr) {
	if (uriStr == NULL) {
		std::cerr << "Error:: XPath string cannot be empty or NULL" << std::endl;
	     return;
        }
setProperty("resources", proc->getResourcesDirectory());
jmethodID mID =
		(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass, "setBaseURI",
				"(Ljava/lang/String;)Z");
if (!mID) {
	std::cerr << "Error: MyClassInDll." << "setBaseURI" << " not found\n"
			<< std::endl;

} else {

	SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXP, mID,
					SaxonProcessor::sxn_environ->env->NewStringUTF(uriStr));
}

}

bool XPathProcessor::effectiveBooleanValue(const char * xpathStr) {
	if (xpathStr == NULL) {
		std::cerr << "Error:: XPath string cannot be empty or NULL" << std::endl;
	     return false;
        }
setProperty("resources", proc->getResourcesDirectory());
jmethodID mID =
		(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass, "effectiveBooleanValue",
				"(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Z");
if (!mID) {
	std::cerr << "Error: MyClassInDll." << "effectiveBooleanValue" << " not found\n"<< std::endl;

} else {
	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;
	jclass objectClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/Object");
	jclass stringClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/String");

	int size = parameters.size() + properties.size();
	if (size > 0) {
		objectArray = SaxonProcessor::sxn_environ->env->NewObjectArray((jint) size,
				objectClass, 0);
		stringArray = SaxonProcessor::sxn_environ->env->NewObjectArray((jint) size,
				stringClass, 0);
		int i = 0;
		for (std::map<std::string, XdmValue*>::iterator iter = parameters.begin();
				iter != parameters.end(); ++iter, i++) {
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(stringArray, i,
					SaxonProcessor::sxn_environ->env->NewStringUTF((iter->first).c_str()));
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(objectArray, i,
					(iter->second)->getUnderlyingValue());
		}
		for (std::map<std::string, std::string>::iterator iter = properties.begin();
				iter != properties.end(); ++iter, i++) {
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(stringArray, i,
					SaxonProcessor::sxn_environ->env->NewStringUTF((iter->first).c_str()));
			SaxonProcessor::sxn_environ->env->SetObjectArrayElement(objectArray, i,
					SaxonProcessor::sxn_environ->env->NewStringUTF((iter->second).c_str()));
		}
	}
	jboolean result = (jboolean)(
			SaxonProcessor::sxn_environ->env->CallBooleanMethod(cppXP, mID,
					SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXP.c_str()),
					SaxonProcessor::sxn_environ->env->NewStringUTF(xpathStr), stringArray, objectArray));
	if (size > 0) {
		SaxonProcessor::sxn_environ->env->DeleteLocalRef(stringArray);
		SaxonProcessor::sxn_environ->env->DeleteLocalRef(objectArray);
	}
	proc->checkAndCreateException(cppClass);
	return result;
}
return false;
}

void XPathProcessor::setParameter(const char * name, XdmValue* value) {
	if(value != NULL){
		value->incrementRefCount();
		parameters["param:"+std::string(name)] = value;
	}
}

bool XPathProcessor::removeParameter(const char * name) {
	return (bool)(parameters.erase("param:"+std::string(name)));
}

void XPathProcessor::setProperty(const char * name, const char * value) {
#ifdef DEBUG	
	if(value == NULL) {
		std::cerr<<"XPathProc setProperty is NULL"<<std::endl;
	}
#endif
	properties.insert(std::pair<std::string, std::string>(std::string(name), std::string((value == NULL ? "" : value))));
}

void XPathProcessor::clearParameters(bool delVal) {
	if(delVal){
       		for(std::map<std::string, XdmValue*>::iterator itr = parameters.begin(); itr != parameters.end(); itr++){
			XdmValue * value = itr->second;
			value->decrementRefCount();
#ifdef DEBUG
			std::cerr<<"XPathProc.clearParameter() - XdmValue refCount="<<value->getRefCount()<<std::endl;
#endif
			if(value != NULL && value->getRefCount() < 1){		
	        		delete value;
			}
        	}
		parameters.clear();
	}
}

void XPathProcessor::clearProperties() {
	properties.clear();
}


   void XPathProcessor::setcwd(const char* dir){
    cwdXP = std::string(dir);
   }

std::map<std::string,XdmValue*>& XPathProcessor::getParameters(){
	std::map<std::string,XdmValue*>& ptr = parameters;
	return ptr;
}

std::map<std::string,std::string>& XPathProcessor::getProperties(){
	std::map<std::string,std::string> &ptr = properties;
	return ptr;
}

void XPathProcessor::exceptionClear(){
	if(proc->exception != NULL) {
		delete proc->exception;
		proc->exception = NULL;	
	}

   SaxonProcessor::sxn_environ->env->ExceptionClear();
 
}

int XPathProcessor::exceptionCount(){
 if(proc->exception != NULL){
 return proc->exception->count();
 }
 return 0;
 }

const char * XPathProcessor::getErrorCode(int i) {
	if(proc->exception == NULL) {return NULL;}
	return proc->exception->getErrorCode(i);
}

const char * XPathProcessor::getErrorMessage(int i ){
	if(proc->exception == NULL) {return NULL;}
	return proc->exception->getErrorMessage(i);
}

    bool XPathProcessor::exceptionOccurred(){
	return proc->exceptionOccurred();
    }



    const char* XPathProcessor::checkException(){
	return proc->checkException(cppXP);
    }

