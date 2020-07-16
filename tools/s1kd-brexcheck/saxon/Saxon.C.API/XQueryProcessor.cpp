#include "XQueryProcessor.h"
#include "XdmValue.h"
#include "XdmItem.h"
#include "XdmNode.h"
#include "XdmAtomicValue.h"

    XQueryProcessor::XQueryProcessor() {
	SaxonProcessor *p = new SaxonProcessor(false);
	XQueryProcessor(p, "");
     }

    XQueryProcessor::XQueryProcessor(SaxonProcessor *p, std::string curr) {
    proc = p;

  
     cppClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/XQueryEngine");


    cppXQ = createSaxonProcessor2 (SaxonProcessor::sxn_environ->env, cppClass, "(Lnet/sf/saxon/s9api/Processor;)V", proc->proc);
    
#ifdef DEBUG
	jmethodID debugMID = SaxonProcessor::sxn_environ->env->GetStaticMethodID(cppClass, "setDebugMode", "(Z)V");
	SaxonProcessor::sxn_environ->env->CallStaticVoidMethod(cppClass, debugMID, (jboolean)true);
#endif

    proc->exception = NULL;
   // outputfile1 = "";
	if(!(proc->cwd.empty()) && curr.empty()){
		cwdXQ = proc->cwd;
	} else {
		cwdXQ = curr;
	}
        queryFileExists = false;
}


std::map<std::string,XdmValue*>& XQueryProcessor::getParameters(){
	std::map<std::string,XdmValue*>& ptr = parameters;
	return ptr;
}

std::map<std::string,std::string>& XQueryProcessor::getProperties(){
	std::map<std::string,std::string> &ptr = properties;
	return ptr;
}


    /**
     * Set the source document for the query
    */
    void XQueryProcessor::setContextItem(XdmItem * value){
    	if(value != NULL){
	 value->incrementRefCount();
     	 parameters["node"] = (XdmValue *)value;
    	}
    }


     void XQueryProcessor::declareNamespace(const char *prefix, const char * uri){
        if (prefix == NULL || uri == NULL) {
		return;
        }
	jmethodID mID =
		(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass, "declareNamespace",
				"(Ljava/lang/String;Ljava/lang/String;)V");
	if (!mID) {
	std::cerr << "Error: Saxonc library." << "declareNameSpace" << " not found\n"
			<< std::endl;

	} else {
	
			SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXQ, mID,
					SaxonProcessor::sxn_environ->env->NewStringUTF(prefix),
					SaxonProcessor::sxn_environ->env->NewStringUTF(uri));
	}

}


    /**
     * Set the source document for the query
    */
    void XQueryProcessor::setContextItemFromFile(const char * ifile){
	setProperty("s", ifile);
    }

    /**
     * Set the output file where the result is sent
    */
    void XQueryProcessor::setOutputFile(const char* ofile){
      // outputfile1 = std::string(ofile); 
       setProperty("o", ofile);
    }

    /**
     * Set a parameter value used in the query
     *
     * @param name  of the parameter, as a string
     * @param value of the query parameter, or null to clear a previously set value
     */
    void XQueryProcessor::setParameter(const char * name, XdmValue*value){
	if(value != NULL){
		value->incrementRefCount();
		parameters["param:"+std::string(name)] = value;
	} 
    }


    /**
     * Remove a parameter (name, value) pair
     *
     * @param namespacei currently not used
     * @param name  of the parameter
     * @return bool - outcome of the romoval
     */
    bool XQueryProcessor::removeParameter(const char * name){
	return (bool)(parameters.erase("param:"+std::string(name)));
    }
    /**
     * Set a property.
     *
     * @param name of the property
     * @param value of the property
     */
    void XQueryProcessor::setProperty(const char * name, const char * value){
#ifdef DEBUG	
	if(value == NULL) {
		std::cerr<<"XQueryProc setProperty is NULL"<<std::endl;
	}
#endif
	properties.insert(std::pair<std::string, std::string>(std::string(name), std::string((value== NULL ? "" : value))));
    }

    void XQueryProcessor::clearParameters(bool delVal){
	if(delVal){
       		for(std::map<std::string, XdmValue*>::iterator itr = parameters.begin(); itr != parameters.end(); itr++){
			XdmValue * value = itr->second;
			value->decrementRefCount();
#ifdef DEBUG
			std::cerr<<"XQueryProc.clearParameter() - XdmValue refCount="<<value->getRefCount()<<std::endl;
#endif
			if(value != NULL && value->getRefCount() < 1){		
	        		delete value;
			}
        	}
		
	} else {

		for(std::map<std::string, XdmValue*>::iterator itr = parameters.begin(); itr != parameters.end(); itr++){
			XdmValue * value = itr->second;
			value->decrementRefCount();

        	}
	}

	parameters.clear();
    }

   void XQueryProcessor::clearProperties(){
	properties.clear();
        //outputfile1.clear();
   }


   void XQueryProcessor::setcwd(const char* dir){
    cwdXQ = std::string(dir);
   }

    void XQueryProcessor::setQueryBaseURI(const char * baseURI){
	setProperty("base", baseURI);
    }


    void XQueryProcessor::setUpdating(bool updating){
     
    	jmethodID mID =
    		(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass, "setUpdating",
    				"(Z)V");
    	if (!mID) {
    	std::cerr << "Error: Saxonc library." << "setUpdating" << " not found\n"
    			<< std::endl;

    	} else {

    			SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXQ, mID,
    					(jboolean)updating);
    	}

    }

    void XQueryProcessor::executeQueryToFile(const char * infilename, const char * ofilename, const char * query){
	setProperty("resources", proc->getResourcesDirectory());  

	jmethodID mID = (jmethodID)SaxonProcessor::sxn_environ->env->GetMethodID (cppClass,"executeQueryToFile", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)V");
 	if (!mID) {
        std::cerr<<"Error: MyClassInDll."<<"executeQueryToFile"<<" not found\n"<<std::endl;
    } else {
	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;

	int size = parameters.size() + properties.size();
	if(query!= NULL) size++;
	if(infilename!= NULL) size++;	
	if(size >0) {

	   int i=0;
           jclass objectClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/Object");
	   jclass stringClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/String");
	   objectArray = SaxonProcessor::sxn_environ->env->NewObjectArray( (jint)size, objectClass, 0 );
	   stringArray = SaxonProcessor::sxn_environ->env->NewObjectArray( (jint)size, stringClass, 0 );
	   if(query!= NULL) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF("qs") );
     	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF(query));
	     i++;	
	   }
	   if(infilename!= NULL) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF("s") );
     	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF(infilename));
	     i++;	
	   }
	   for(std::map<std::string, XdmValue* >::iterator iter=parameters.begin(); iter!=parameters.end(); ++iter, i++) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF( (iter->first).c_str() ) );
		bool checkCast = SaxonProcessor::sxn_environ->env->IsInstanceOf((iter->second)->getUnderlyingValue(), lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/XdmValueForCpp") );
		if(( (bool)checkCast)==false ){
			failure = "FAILURE in  array of XdmValueForCpp";
		} 
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, (jobject)((iter->second)->getUnderlyingValue()) );
	   }
  	   for(std::map<std::string, std::string >::iterator iter=properties.begin(); iter!=properties.end(); ++iter, i++) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF( (iter->first).c_str()  ));
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, (jobject)(SaxonProcessor::sxn_environ->env->NewStringUTF((iter->second).c_str())) );
	   }
	}

	 SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXQ, mID, SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXQ.c_str()), SaxonProcessor::sxn_environ->env->NewStringUTF(ofilename), stringArray, objectArray );
	  SaxonProcessor::sxn_environ->env->DeleteLocalRef(objectArray);
	  SaxonProcessor::sxn_environ->env->DeleteLocalRef(stringArray);

	proc->checkAndCreateException(cppClass);
	 
  }


   }


    XdmValue * XQueryProcessor::executeQueryToValue(const char * infilename, const char * query){
	setProperty("resources", proc->getResourcesDirectory()); 
 jmethodID mID = (jmethodID)SaxonProcessor::sxn_environ->env->GetMethodID (cppClass,"executeQueryToValue", "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Lnet/sf/saxon/s9api/XdmValue;");
 if (!mID) {
        std::cerr<<"Error: MyClassInDll."<<"executeQueryToValue"<<" not found\n"<<std::endl;
    } else {
	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;

	int size = parameters.size() + properties.size();
	if(query!= NULL) size++;
	if(infilename!= NULL) size++;
	if(size >0) {
	   int i=0;
           jclass objectClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/Object");
	   jclass stringClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/String");
	   objectArray = SaxonProcessor::sxn_environ->env->NewObjectArray( (jint)size, objectClass, 0 );
	   stringArray = SaxonProcessor::sxn_environ->env->NewObjectArray( (jint)size, stringClass, 0 );

	   if(query!= NULL) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF("qs") );
     	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF(query));
	     i++;	
	   }
	   if(infilename!= NULL) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF("s") );
     	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF(infilename));
	     i++;	
	   }
	   for(std::map<std::string, XdmValue* >::iterator iter=parameters.begin(); iter!=parameters.end(); ++iter, i++) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF( (iter->first).c_str() ) );
		bool checkCast = SaxonProcessor::sxn_environ->env->IsInstanceOf((iter->second)->getUnderlyingValue(), lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/XdmValueForCpp") );
		if(( (bool)checkCast)==false ){
			failure = "FAILURE in  array of XdmValueForCpp";
		} 
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, (jobject)((iter->second)->getUnderlyingValue()) );
	   }
  	   for(std::map<std::string, std::string >::iterator iter=properties.begin(); iter!=properties.end(); ++iter, i++) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF( (iter->first).c_str()  ));
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, (jobject)(SaxonProcessor::sxn_environ->env->NewStringUTF((iter->second).c_str())) );
	   }
	}

	  jobject result = (jobject)(SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXQ, mID, SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXQ.c_str()), stringArray, objectArray ));
	  SaxonProcessor::sxn_environ->env->DeleteLocalRef(objectArray);
	  SaxonProcessor::sxn_environ->env->DeleteLocalRef(stringArray);
    if(result) {
		jclass atomicValueClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmAtomicValue");
		jclass nodeClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmNode");
		jclass functionItemClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmFunctionItem");
		XdmValue * value = NULL;
		XdmItem * xdmItem = NULL;

		if(SaxonProcessor::sxn_environ->env->IsInstanceOf(result, atomicValueClass)           == JNI_TRUE) {
				xdmItem = new XdmAtomicValue(result);
				xdmItem->setProcessor(proc);
				SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
				return xdmItem;

			} else if(SaxonProcessor::sxn_environ->env->IsInstanceOf(result, nodeClass)           == JNI_TRUE) {
				xdmItem = new XdmNode(result);
				xdmItem->setProcessor(proc);
				SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
				return xdmItem;
			
			} else if (SaxonProcessor::sxn_environ->env->IsInstanceOf(result, functionItemClass)           == JNI_TRUE) {
				return NULL;
			} else {
				value = new XdmValue(result, true);
				value->setProcessor(proc);
				for(int z=0;z<value->size();z++) {
					value->itemAt(z)->setProcessor(proc);
				}
				return value;
			}
			value = new XdmValue();
			value->setProcessor(proc);
			xdmItem->setProcessor(proc);
			value->addXdmItem(xdmItem);
			return value;
     } else {
	   
	proc->checkAndCreateException(cppClass);		
     	} 
  }
  return NULL;

}

    const char * XQueryProcessor::executeQueryToString(const char * infilename, const char * query){
	setProperty("resources", proc->getResourcesDirectory()); 
 jmethodID mID = (jmethodID)SaxonProcessor::sxn_environ->env->GetMethodID (cppClass,"executeQueryToString", "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;");
 if (!mID) {
        std::cerr<<"Error: MyClassInDll."<<"executeQueryToString"<<" not found\n"<<std::endl;
    } else {
	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;

	int size = parameters.size() + properties.size();
	if(query!= NULL) size++;
	if(infilename!= NULL) size++;
	if(size >0) {
	   int i=0;
           jclass objectClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/Object");
	   jclass stringClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/String");
	   objectArray = SaxonProcessor::sxn_environ->env->NewObjectArray( (jint)size, objectClass, 0 );
	   stringArray = SaxonProcessor::sxn_environ->env->NewObjectArray( (jint)size, stringClass, 0 );

	   if(query!= NULL) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF("qs") );
     	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF(query));
	     i++;	
	   }
	   if(infilename!= NULL) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF("s") );
     	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF(infilename));
	     i++;	
	   }
	   for(std::map<std::string, XdmValue* >::iterator iter=parameters.begin(); iter!=parameters.end(); ++iter, i++) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF( (iter->first).c_str() ) );
		bool checkCast = SaxonProcessor::sxn_environ->env->IsInstanceOf((iter->second)->getUnderlyingValue(), lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/XdmValueForCpp") );
		if(( (bool)checkCast)==false ){
			failure = "FAILURE in  array of XdmValueForCpp";
		} 
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, (jobject)((iter->second)->getUnderlyingValue()) );
	   }
  	   for(std::map<std::string, std::string >::iterator iter=properties.begin(); iter!=properties.end(); ++iter, i++) {
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray, i, SaxonProcessor::sxn_environ->env->NewStringUTF( (iter->first).c_str()  ));
	     SaxonProcessor::sxn_environ->env->SetObjectArrayElement( objectArray, i, (jobject)(SaxonProcessor::sxn_environ->env->NewStringUTF((iter->second).c_str())) );
	   }
	}

	  jstring result = (jstring)(SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXQ, mID, SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXQ.c_str()), stringArray, objectArray ));
	  SaxonProcessor::sxn_environ->env->DeleteLocalRef(objectArray);
	  SaxonProcessor::sxn_environ->env->DeleteLocalRef(stringArray);

	  if(result) {
             const char * str = SaxonProcessor::sxn_environ->env->GetStringUTFChars(result, NULL);
            //return "result should be ok";            
	    return str;
	   } else {
		proc->checkAndCreateException(cppClass);
	   		
     		}
  }
  return NULL;


    }


    const char * XQueryProcessor::runQueryToString(){
	return executeQueryToString(NULL, NULL);	

    }


    XdmValue * XQueryProcessor::runQueryToValue(){
	return executeQueryToValue(NULL, NULL);
   }

    void XQueryProcessor::runQueryToFile(){
	executeQueryToFile(NULL, NULL, NULL);
   }

    void XQueryProcessor::setQueryFile(const char * ofile){
	   //outputfile1 = std::string(ofile); 
	   setProperty("q", ofile);
	    queryFileExists = true;
    }

   void XQueryProcessor::setQueryContent(const char* content){
	  // outputfile1 = std::string(content); 
	   setProperty("qs", content);
	    queryFileExists = false;
  }



void XQueryProcessor::exceptionClear(){
	if(proc->exception != NULL) {
		delete proc->exception;
		proc->exception = NULL;	
		SaxonProcessor::sxn_environ->env->ExceptionClear();
	}

   
 
}

bool XQueryProcessor::exceptionOccurred(){
	return proc->exceptionOccurred();

}


const char * XQueryProcessor::getErrorCode(int i) {
	if(proc->exception == NULL) {return NULL;}
	return proc->exception->getErrorCode(i);
}

const char * XQueryProcessor::getErrorMessage(int i ){
	if(proc->exception == NULL) {return NULL;}
	return proc->exception->getErrorMessage(i);
}

const char* XQueryProcessor::checkException(){
	/*if(proc->exception == NULL) {
		proc->exception = proc->checkForException(SaxonProcessor::sxn_environ->env, cppClass, cppXQ);
	}
        return proc->exception;*/
	return proc->checkException(cppXQ);
}



int XQueryProcessor::exceptionCount(){
	if(proc->exception != NULL){
		return proc->exception->count();
	}
	return 0;
}
