// Xslt30Processor.cpp : Defines the exported functions for the DLL application.
//

#include "Xslt30Processor.h"
#include "XdmValue.h"
#include "XdmItem.h"
#include "XdmNode.h"
#include "XdmAtomicValue.h"
#ifdef DEBUG
#include <typeinfo> //used for testing only
#endif

Xslt30Processor::Xslt30Processor() {

	SaxonProcessor *p = new SaxonProcessor(false);
	Xslt30Processor(p, "");

}

Xslt30Processor::Xslt30Processor(SaxonProcessor * p, std::string curr) {

	proc = p;

	/*
	 * Look for class.
	 */
	cppClass = lookForClass(SaxonProcessor::sxn_environ->env,
			"net/sf/saxon/option/cpp/Xslt30Processor");

	cppXT = createSaxonProcessor2(SaxonProcessor::sxn_environ->env, cppClass,
			"(Lnet/sf/saxon/s9api/Processor;)V", proc->proc);

#ifdef DEBUG
	jmethodID debugMID = SaxonProcessor::sxn_environ->env->GetStaticMethodID(cppClass, "setDebugMode", "(Z)V");
	SaxonProcessor::sxn_environ->env->CallStaticVoidMethod(cppClass, debugMID, (jboolean)true);
    
#endif
	nodeCreated = false;
	tunnel = false;
	proc->exception = NULL;
	selection = NULL;
	selectionV=NULL;
	outputfile1 = "";
	if(!(proc->cwd.empty()) && curr.empty()){
		cwdXT = proc->cwd;
	} else if(!curr.empty()){
		cwdXT = curr;
	} 

}

     Xslt30Processor::~Xslt30Processor(){
	clearProperties();
	clearParameters();
	if(selectionV != NULL) {
	  selectionV->decrementRefCount();
	  if(selectionV->getRefCount() == 0) {
		delete selectionV;
	  }
	}
	
     }

bool Xslt30Processor::exceptionOccurred() {
	return proc->exceptionOccurred();
}

const char * Xslt30Processor::getErrorCode(int i) {
 if(proc->exception == NULL) {return NULL;}
 return proc->exception->getErrorCode(i);
 }



void Xslt30Processor::setGlobalContextItem(XdmItem * value){
    if(value != NULL){
      value->incrementRefCount();
      parameters["node"] = value;
    }
}

void Xslt30Processor::setGlobalContextFromFile(const char * ifile) {
	if(ifile != NULL) {
		setProperty("s", ifile);
	}
}

void Xslt30Processor::setInitialMatchSelection(XdmValue * _selection){
     if(_selection != NULL) {
      _selection->incrementRefCount();
      selectionV = _selection;
      selection = _selection->getUnderlyingValue();
    }
}


void Xslt30Processor::setInitialMatchSelectionAsFile(const char * filename){
    if(filename != NULL) {
      selection = SaxonProcessor::sxn_environ->env->NewStringUTF(filename);
    }
}

void Xslt30Processor::setOutputFile(const char * ofile) {
	outputfile1 = std::string(ofile);
	setProperty("o", ofile);
}

void Xslt30Processor::setParameter(const char* name, XdmValue * value, bool _static) {
	if(value != NULL && name != NULL){
		value->incrementRefCount();
		if (_static) {
			parameters["sparam:"+std::string(name)] = value;
		
		} else {
			parameters["param:"+std::string(name)] = value;
		}
	 }
}

    void Xslt30Processor::setInitialTemplateParameters(std::map<std::string,XdmValue*> _itparameters, bool _tunnel){
	for(std::map<std::string, XdmValue*>::iterator itr = _itparameters.begin(); itr != _itparameters.end(); itr++){
		parameters["itparam:"+std::string(itr->first)] = itr->second;	
	}
	tunnel = _tunnel;
	if(tunnel) {
		setProperty("tunnel", "true");
    	}
   }

XdmValue* Xslt30Processor::getParameter(const char* name) {
        std::map<std::string, XdmValue*>::iterator it;
        it = parameters.find("param:"+std::string(name));
        if (it != parameters.end())
          return it->second;
        else {
          it = parameters.find("sparam:"+std::string(name));
        if (it != parameters.end())
	  return it->second;
	  }
	return NULL;
}

bool Xslt30Processor::removeParameter(const char* name) {
	return (bool)(parameters.erase("param:"+std::string(name)));
}

void Xslt30Processor::setJustInTimeCompilation(bool jit){
	static jmethodID jitmID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"setJustInTimeCompilation",
					"(Z)V");
	if (!jitmID) {
		std::cerr << "Error: "<<getDllname() << ".setJustInTimeCompilation"
				<< " not found\n" << std::endl;
		return;
	} else {
		SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, jitmID, jit);

		proc->checkAndCreateException(cppClass);
	}
}

void Xslt30Processor::setResultAsRawValue(bool option) {
	if(option) {
		setProperty("outvalue", "yes");
	}
 }

void Xslt30Processor::setProperty(const char* name, const char* value) {	
	if(name != NULL) {
		properties.insert(std::pair<std::string, std::string>(std::string(name), std::string((value == NULL ? "" : value))));
	}
}

const char* Xslt30Processor::getProperty(const char* name) {
        std::map<std::string, std::string>::iterator it;
        it = properties.find(std::string(name));
        if (it != properties.end())
          return it->second.c_str();
	return NULL;
}

void Xslt30Processor::clearParameters(bool delValues) {
	if(delValues){
       		for(std::map<std::string, XdmValue*>::iterator itr = parameters.begin(); itr != parameters.end(); itr++){
			
			XdmValue * value = itr->second;
			value->decrementRefCount();
#ifdef DEBUG
			std::cout<<"clearParameter() - XdmValue refCount="<<value->getRefCount()<<std::endl;
#endif
			if(value != NULL && value->getRefCount() < 1){		
	        		delete value;
			}
        	}
				
		SaxonProcessor::sxn_environ->env->DeleteLocalRef(selection);
		selection = NULL;
	} else {
for(std::map<std::string, XdmValue*>::iterator itr = parameters.begin(); itr != parameters.end(); itr++){
		
			XdmValue * value = itr->second;
			value->decrementRefCount();
		
        	}

	selection = NULL;
	}
	parameters.clear();

	
}

void Xslt30Processor::clearProperties() {
	properties.clear();
	
}



std::map<std::string,XdmValue*>& Xslt30Processor::getParameters(){
	std::map<std::string,XdmValue*>& ptr = parameters;
	return ptr;
}

std::map<std::string,std::string>& Xslt30Processor::getProperties(){
	std::map<std::string,std::string> &ptr = properties;
	return ptr;
}

void Xslt30Processor::exceptionClear(){
 if(proc->exception != NULL) {
 	delete proc->exception;
 	proc->exception = NULL;
	SaxonProcessor::sxn_environ->env->ExceptionClear();
 }
  
 }

   void Xslt30Processor::setcwd(const char* dir){
    if (dir!= NULL) {
        cwdXT = std::string(dir);
    }
   }

const char* Xslt30Processor::checkException() {
	/*if(proc->exception == NULL) {
	 proc->exception = proc->checkForException(environi, cpp);
	 }
	 return proc->exception;*/
	return proc->checkException(cppXT);
}

int Xslt30Processor::exceptionCount(){
 if(proc->exception != NULL){
 return proc->exception->count();
 }
 return 0;
 }


    void Xslt30Processor::compileFromXdmNodeAndSave(XdmNode * node, const char* filename) {
	static jmethodID cAndSNodemID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"compileFromXdmNodeAndSave",
					"(Ljava/lang/String;Ljava/lang/Object;Ljava/lang/String;)V");
	if (!cAndSNodemID) {
		std::cerr << "Error: "<<getDllname() << ".compileFromStringAndSave"
				<< " not found\n" << std::endl;

	} else {

		
		SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, cAndSNodemID,
						SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
						node->getUnderlyingValue(), 							SaxonProcessor::sxn_environ->env->NewStringUTF(filename));
		
		proc->checkAndCreateException(cppClass);		

    }



}

    void Xslt30Processor::compileFromStringAndSave(const char* stylesheetStr, const char* filename){
	static jmethodID cAndSStringmID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"compileFromStringAndSave",
					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	if (!cAndSStringmID) {
		std::cerr << "Error: "<<getDllname() << ".compileFromStringAndSave"
				<< " not found\n" << std::endl;

	} else {

		
		SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, cAndSStringmID,
						SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
						SaxonProcessor::sxn_environ->env->NewStringUTF(stylesheetStr), 							SaxonProcessor::sxn_environ->env->NewStringUTF(filename));
		
		proc->checkAndCreateException(cppClass);		

    }
}



    void Xslt30Processor::compileFromFileAndSave(const char* xslFilename, const char* filename){
	static jmethodID cAndFStringmID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"compileFromFileAndSave",
					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	if (!cAndFStringmID) {
		std::cerr << "Error: "<<getDllname() << ".compileFromFileAndSave"
				<< " not found\n" << std::endl;

	} else {

		
		SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, cAndFStringmID,
						SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
						SaxonProcessor::sxn_environ->env->NewStringUTF(xslFilename),SaxonProcessor::sxn_environ->env->NewStringUTF(filename));
		
		proc->checkAndCreateException(cppClass);


     }
}

void Xslt30Processor::compileFromString(const char* stylesheetStr) {
	static jmethodID cStringmID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"createStylesheetFromString",
					"(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Lnet/sf/saxon/s9api/XsltExecutable;");
	if (!cStringmID) {
		std::cerr << "Error: "<<getDllname() << ".createStylesheetFromString"
				<< " not found\n" << std::endl;

	} else {
		JParameters comboArrays;
		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);
		stylesheetObject = (jobject)(
				SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, cStringmID,
						SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
						SaxonProcessor::sxn_environ->env->NewStringUTF(stylesheetStr), comboArrays.stringArray, comboArrays.objectArray));
		if (!stylesheetObject) {
			proc->checkAndCreateException(cppClass);
		}
		if (comboArrays.stringArray != NULL) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
		}
	}

}

void Xslt30Processor::compileFromXdmNode(XdmNode * node) {
	static jmethodID cNodemID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,"createStylesheetFromXdmNode",
			"(Ljava/lang/String;Ljava/lang/Object;[Ljava/lang/String;[Ljava/lang/Object;)Lnet/sf/saxon/s9api/XsltExecutable;");
	if (!cNodemID) {
		std::cerr << "Error: "<< getDllname() << ".createStylesheetFromXdmNode"
				<< " not found\n" << std::endl;

	} else {
		releaseStylesheet();
		JParameters comboArrays;
		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);
		stylesheetObject = (jobject)(
				SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, cNodemID,
						SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
						node->getUnderlyingValue(), comboArrays.stringArray, comboArrays.objectArray));
		if (!stylesheetObject) {
			proc->checkAndCreateException(cppClass);
			//cout << "Error in compileFromXdmNode" << endl;
		}
		if (comboArrays.stringArray != NULL) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
		}
	}

}

void Xslt30Processor::compileFromAssociatedFile(const char* source) {
	static jmethodID cFilemID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"createStylesheetFromAssoicatedFile",
					"(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Lnet/sf/saxon/s9api/XsltExecutable;");
	if (!cFilemID) {
		std::cerr << "Error: "<<getDllname() << ".createStylesheetFromFile"
				<< " not found\n" << std::endl;

	} else {
		releaseStylesheet();
		if(source == NULL) {
			std::cerr << "Error in compileFromFile method - The Stylesheet file is NULL" <<std::endl;
			return;
		}
		JParameters comboArrays;
		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);
		stylesheetObject = (jobject)(
				SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, cFilemID,
						SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
						SaxonProcessor::sxn_environ->env->NewStringUTF(source), comboArrays.stringArray, comboArrays.objectArray));
		if (!stylesheetObject) {
			proc->checkAndCreateException(cppClass);
     		
		}
		if (comboArrays.stringArray != NULL) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
		}
		//SaxonProcessor::sxn_environ->env->NewGlobalRef(stylesheetObject);
	}

}


void Xslt30Processor::compileFromFile(const char* stylesheet) {
	static jmethodID cFilemID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"createStylesheetFromFile",
					"(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Lnet/sf/saxon/s9api/XsltExecutable;");
	if (!cFilemID) {
		std::cerr << "Error: "<<getDllname() << ".createStylesheetFromFile"
				<< " not found\n" << std::endl;

	} else {
		releaseStylesheet();
		if(stylesheet == NULL) {
			std::cerr << "Error in compileFromFile method - The Stylesheet file is NULL" <<std::endl;
			return;
		}
		JParameters comboArrays;
		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);
		stylesheetObject = (jobject)(
				SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, cFilemID,
						SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
						SaxonProcessor::sxn_environ->env->NewStringUTF(stylesheet), comboArrays.stringArray, comboArrays.objectArray));
		if (!stylesheetObject) {
			proc->checkAndCreateException(cppClass);
     		
		}
		if (comboArrays.stringArray != NULL) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
		}
		//SaxonProcessor::sxn_environ->env->NewGlobalRef(stylesheetObject);
	}

}

void Xslt30Processor::releaseStylesheet() {

	stylesheetObject = NULL;
	
}

    void Xslt30Processor::applyTemplatesReturningFile(const char * stylesheetfile, const char* output_filename){
	if(exceptionOccurred()) {
		//Possible error detected in the compile phase. Processor not in a clean state.
		//Require clearing exception.
		return;
	}

	if(selection == NULL) {
	   std::cerr<< "Error: The initial match selection has not been set. Please set it using setInitialMatchSelection or setInitialMatchSelectionFile."<<std::endl;
       		return;
	}

	if(stylesheetfile == NULL && !stylesheetObject){
	
		return;
	}

	setProperty("resources", proc->getResourcesDirectory());
	static jmethodID atmID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"applyTemplatesReturningFile",
					"(Ljava/lang/String;Ljava/lang/Object;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)V");
	if (!atmID) {
		std::cerr << "Error: "<< getDllname() << "applyTemplatesAsFile" << " not found\n"
				<< std::endl;

	} else {
        JParameters comboArrays;
		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);
		SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, atmID,
						SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),selection,
						(stylesheetfile != NULL ?
								SaxonProcessor::sxn_environ->env->NewStringUTF(
										stylesheetfile) :
								NULL), (output_filename != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(
                                       						output_filename) : NULL),
                                comboArrays.stringArray, comboArrays.objectArray);
		if (comboArrays.stringArray != NULL) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
		}
		proc->checkAndCreateException(cppClass);
	   	
	}
	return;

}

const char* Xslt30Processor::applyTemplatesReturningString(const char * stylesheetfile){
	if(exceptionOccurred()) {
		//Possible error detected in the compile phase. Processor not in a clean state.
		//Require clearing exception.
		return NULL;	
	}
	if(selection == NULL) {
	   std::cerr<< "Error: The initial match selection has not been set. Please set it using setInitialMatchSelection or setInitialMatchSelectionFile."<<std::endl;
       		return NULL;
	}
	if(stylesheetfile == NULL && !stylesheetObject){
		std::cerr<< "Error: No stylesheet found. Please compile stylesheet before calling applyTemplatesReturningString or check exceptions"<<std::endl;
		return NULL;
	}
	setProperty("resources", proc->getResourcesDirectory());
	jmethodID atsmID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"applyTemplatesReturningString",
					"(Ljava/lang/String;Ljava/lang/Object;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;");
	if (!atsmID) {
		std::cerr << "Error: "<<getDllname() << "applyTemplatesAsString" << " not found\n"
				<< std::endl;

	} else {
	    JParameters comboArrays;
		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);

	jstring result = NULL;
	jobject obj = (SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, atsmID,
								SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
								selection,
								(stylesheetfile!= NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(stylesheetfile) : NULL ),
								comboArrays.stringArray, comboArrays.objectArray));

		if(obj) {
			result = (jstring)obj;
		}		
		if (comboArrays.stringArray != NULL) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
		}
		if (result) {
			const char * str = SaxonProcessor::sxn_environ->env->GetStringUTFChars(result,
					NULL);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(obj);
			return str;
		} else  {
			proc->checkAndCreateException(cppClass);  
	   		
     		}
	}
	return NULL;


}

XdmValue * Xslt30Processor::applyTemplatesReturningValue(const char * stylesheetfile){
	if(exceptionOccurred()) {
		//Possible error detected in the compile phase. Processor not in a clean state.
		//Require clearing exception.
		return NULL;
	}
	if(selection == NULL) {
	   std::cerr<< "Error: The initial match selection has not been set. Please set it using setInitialMatchSelection or setInitialMatchSelectionFile."<<std::endl;
       		return NULL;
	}
	if(stylesheetfile == NULL && !stylesheetObject){
		std::cerr<< "Error: No stylesheet found. Please compile stylesheet before calling applyTemplatesReturningValue or check exceptions"<<std::endl;
		return NULL;
	}
	setProperty("resources", proc->getResourcesDirectory());
	jmethodID atsmID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"applyTemplatesReturningValue",
					"(Ljava/lang/String;Ljava/lang/Object;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Lnet/sf/saxon/s9api/XdmValue;");
	if (!atsmID) {
		std::cerr << "Error: "<<getDllname() << "applyTemplatesAsValue" << " not found\n"
				<< std::endl;

	} else {
	    JParameters comboArrays;
		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);


	   // jstring result = NULL;
	    jobject result = (jobject)(SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, atsmID,
								SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
								selection,
								( stylesheetfile != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(stylesheetfile) : NULL),
								comboArrays.stringArray, comboArrays.objectArray));
		/*if(obj) {
			result = (jobject)obj;
		}*/
		if (comboArrays.stringArray != NULL) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
		}
        if (result) {
		jclass atomicValueClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmAtomicValue");
		jclass nodeClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmNode");
		jclass functionItemClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmFunctionItem");
        	XdmValue * value = NULL;
		XdmItem * xdmItem = NULL;


			if(SaxonProcessor::sxn_environ->env->IsInstanceOf(result, atomicValueClass)           == JNI_TRUE) {
				xdmItem =  new XdmAtomicValue(result);
				xdmItem->setProcessor(proc);
				SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
				return xdmItem;

			} else if(SaxonProcessor::sxn_environ->env->IsInstanceOf(result, nodeClass)           == JNI_TRUE) {
				xdmItem =  new XdmNode(result);	
				xdmItem->setProcessor(proc);
				SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
				return xdmItem;
			} else if (SaxonProcessor::sxn_environ->env->IsInstanceOf(result, functionItemClass)           == JNI_TRUE) {
				std::cerr<<"Error: applyTemplateToValue: FunctionItem found. Currently not be handled"<<std::endl;
				return NULL;
			} else {
				value = new XdmValue(result, true);
				value->setProcessor(proc);
				for(int z=0;z<value->size();z++) {
					value->itemAt(z)->setProcessor(proc);
				}
				SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
				return value;
			}
		} else  {
			proc->checkAndCreateException(cppClass);

     		}
	}
	return NULL;

}     



    void Xslt30Processor::callFunctionReturningFile(const char * stylesheetfile, const char* functionName, XdmValue ** arguments, int argument_length, const char* outfile){
        if(exceptionOccurred()) {
        		//Possible error detected in the compile phase. Processor not in a clean state.
        		//Require clearing exception.
        		return;
        	}


        	if(stylesheetfile == NULL && !stylesheetObject){

        		return;
        	}

        	setProperty("resources", proc->getResourcesDirectory());
        	static jmethodID afmID =
        			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
        					"callFunctionReturningFile",
        					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/Object;[Ljava/lang/String;[Ljava/lang/Object;)V");
        	if (!afmID) {
        		std::cerr << "Error: "<< getDllname() << "callFunctionReturningFile" << " not found\n"
        				<< std::endl;
                 return;
        	} else {
                JParameters comboArrays;
        		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);

        		jobjectArray argumentJArray = SaxonProcessor::createJArray(arguments, argument_length);

        		SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, afmID,
        						SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
        						(stylesheetfile != NULL ?
        								SaxonProcessor::sxn_environ->env->NewStringUTF(
        										stylesheetfile) :
        								NULL),
        						(functionName != NULL ?
        								SaxonProcessor::sxn_environ->env->NewStringUTF(functionName) :
        								NULL), argumentJArray,
        								(outfile != NULL ?
                                        			SaxonProcessor::sxn_environ->env->NewStringUTF(outfile) :
                             					NULL),
        								comboArrays.stringArray, comboArrays.objectArray);
        		if (comboArrays.stringArray != NULL) {
        			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
        			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
        		}
        		proc->checkAndCreateException(cppClass);

        	}
        	return;




    }

    const char * Xslt30Processor::callFunctionReturningString(const char * stylesheet, const char* functionName, XdmValue ** arguments, int argument_length){
    	if(exceptionOccurred()) {
    		//Possible error detected in the compile phase. Processor not in a clean state.
    		//Require clearing exception.
    		return NULL;
    	}

    	if(stylesheet == NULL && !stylesheetObject){
    		std::cerr<< "Error: No stylesheet found. Please compile stylesheet before calling callFunctionReturningString or check exceptions"<<std::endl;
    		return NULL;
    	}
    	setProperty("resources", proc->getResourcesDirectory());
    	jmethodID afsmID =
    			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
    					"callFunctionReturningString",
    					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/Object;[Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;");
    	if (!afsmID) {
    		std::cerr << "Error: "<<getDllname() << "callFunctionReturningString" << " not found\n"
    				<< std::endl;

    	} else {
    	    JParameters comboArrays;
    		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);
            jobjectArray argumentJArray = SaxonProcessor::createJArray(arguments, argument_length);

    	jstring result = NULL;
    	jobject obj = (SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, afsmID,
    								SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
    								(stylesheet != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(stylesheet) : NULL ),
    								(functionName != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(functionName) : NULL),
    								argumentJArray, comboArrays.stringArray, comboArrays.objectArray));
    		if(obj) {
    			result = (jstring)obj;
    		}
    		if (comboArrays.stringArray != NULL) {
    			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
    			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
    		}
    		if (result) {
    			const char * str = SaxonProcessor::sxn_environ->env->GetStringUTFChars(result,
    					NULL);
    			SaxonProcessor::sxn_environ->env->DeleteLocalRef(obj);
    			return str;
    		} else  {
    			proc->checkAndCreateException(cppClass);

         		}
    	}
    	return NULL;
    }



    XdmValue * Xslt30Processor::callFunctionReturningValue(const char * stylesheet, const char* functionName, XdmValue ** arguments, int argument_length){
         	if(exceptionOccurred()) {
          		//Possible error detected in the compile phase. Processor not in a clean state.
          		//Require clearing exception.
          		return NULL;
          	}
          	
          	if(stylesheet == NULL && !stylesheetObject){
          		std::cerr<< "Error: No stylesheet found. Please compile stylesheet before calling callFunctionReturningValue or check exceptions"<<std::endl;
          		return NULL;
          	}
          	setProperty("resources", proc->getResourcesDirectory());
          	jmethodID cfvmID =
          			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
          					"callFunctionReturningValue",
          					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/Object;[Ljava/lang/String;[Ljava/lang/Object;)Lnet/sf/saxon/s9api/XdmValue;");
          	if (!cfvmID) {
          		std::cerr << "Error: "<<getDllname() << "callFunctionReturningValue" << " not found\n"
          				<< std::endl;

          	} else {
          	    JParameters comboArrays;
          		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);
                jobjectArray argumentJArray = SaxonProcessor::createJArray(arguments, argument_length);
          	   
          	    jobject result = (jobject)(SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, cfvmID,
          								SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
          								(stylesheet != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(stylesheet) : NULL ),
                                        (functionName != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(functionName) : NULL),
          								argumentJArray, comboArrays.stringArray, comboArrays.objectArray));

          		if (comboArrays.stringArray != NULL) {
          			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
          			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
          		}
			if(argumentJArray != NULL) {
          			SaxonProcessor::sxn_environ->env->DeleteLocalRef(argumentJArray);
			}
                  if (result) {
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
					SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
					return value;
			}
			value = new XdmValue();
			value->setProcessor(proc);
          		xdmItem->setProcessor(proc);
          		value->addXdmItem(xdmItem);
	        	SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
			return value;
          	} else  {
          		proc->checkAndCreateException(cppClass);

               	}
          }
          return NULL;

    }

     void Xslt30Processor::addPackages(const char ** fileNames, int length){
              	if(exceptionOccurred()) {
              		//Possible error detected in the compile phase. Processor not in a clean state.
              		//Require clearing exception.
              		return;
              	}

              	if(length<1){

              		return;
              	}

              	jmethodID apmID =
              			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
              					"addPackages",
              					"([Ljava/lang/String;)V");
              	if (!apmID) {
              		std::cerr << "Error: "<<getDllname() << "addPackage" << " not found\n"
              				<< std::endl;

              	} else {

              	 jobjectArray stringArray = NULL;

                 jclass stringClass = lookForClass(SaxonProcessor::sxn_environ->env,
                 				"java/lang/String");


                 stringArray = SaxonProcessor::sxn_environ->env->NewObjectArray((jint) length,
                 					stringClass, 0);

                 for (int i=0; i<length; i++) {

                 SaxonProcessor::sxn_environ->env->SetObjectArrayElement(stringArray, i,
                 						SaxonProcessor::sxn_environ->env->NewStringUTF(fileNames[i]));
                 }

              	SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, apmID,
              								SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()), stringArray);

                proc->checkAndCreateException(cppClass);
              }
              	return;

        }



            void Xslt30Processor::clearPackages(){
                      	if(exceptionOccurred()) {
                      		//Possible error detected in the compile phase. Processor not in a clean state.
                      		//Require clearing exception.
                      		return;
                      	}



                      	jmethodID cpmID =
                      			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
                      					"clearPackages",
                      					"()V");
                      	if (!cpmID) {
                      		std::cerr << "Error: "<<getDllname() << "clearPackage" << " not found\n"
                      				<< std::endl;

                      	} else {


                      	SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, cpmID);

                        proc->checkAndCreateException(cppClass);
                      }
                      	return;




                }

    void Xslt30Processor::callTemplateReturningFile(const char * stylesheetfile, const char* templateName, const char* outfile){
	if(exceptionOccurred()) {
		//Possible error detected in the compile phase. Processor not in a clean state.
		//Require clearing exception.
		return;
	}


	if(stylesheetfile == NULL && !stylesheetObject){
		std::cerr<< "Error: No stylesheet found. Please compile stylesheet before calling callFunctionReturningFile or check exceptions"<<std::endl;
		return;
	}

	setProperty("resources", proc->getResourcesDirectory());
	static jmethodID ctmID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"callTemplateReturningFile",
					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)V");
	if (!ctmID) {
		std::cerr << "Error: "<< getDllname() << "callTemplateReturningFile" << " not found\n"
				<< std::endl;

	} else {
        JParameters comboArrays;
		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);
		SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, ctmID,
						SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
						(stylesheetfile != NULL ?
								SaxonProcessor::sxn_environ->env->NewStringUTF(
										stylesheetfile) :
								NULL),
						(templateName != NULL ?
								SaxonProcessor::sxn_environ->env->NewStringUTF(templateName) :
								NULL),
								(outfile != NULL ?
                                			SaxonProcessor::sxn_environ->env->NewStringUTF(outfile) :
                     					NULL),
								comboArrays.stringArray, comboArrays.objectArray);
		if (comboArrays.stringArray != NULL) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
		}
		proc->checkAndCreateException(cppClass);

	}
	return;


    }




    const char* Xslt30Processor::callTemplateReturningString(const char * stylesheet, const char* templateName){
	if(exceptionOccurred()) {
		//Possible error detected in the compile phase. Processor not in a clean state.
		//Require clearing exception.
		return NULL;
	}

	if(stylesheet == NULL && !stylesheetObject){
		std::cerr<< "Error: No stylesheet found. Please compile stylesheet before calling callTemplateReturningString or check exceptions"<<std::endl;
		return NULL;
	}
	setProperty("resources", proc->getResourcesDirectory());
	jmethodID ctsmID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"callTemplateReturningString",
					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;");
	if (!ctsmID) {
		std::cerr << "Error: "<<getDllname() << "callTemplateReturningString" << " not found\n"
				<< std::endl;

	} else {
	    JParameters comboArrays;
		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);


	jstring result = NULL;
	jobject obj =(jobject)(SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, ctsmID,
								SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
								(stylesheet != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(stylesheet) : NULL ),
								(templateName != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(templateName) : NULL),
								comboArrays.stringArray, comboArrays.objectArray));
		if(obj) {
			result = (jstring)obj;
		}
		if (comboArrays.stringArray != NULL) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
		}
		if (result) {
			const char * str = SaxonProcessor::sxn_environ->env->GetStringUTFChars(result,
					NULL);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(obj);
			return str;
		} else  {
			proc->checkAndCreateException(cppClass);

     		}
	}
	return NULL;


    }

    XdmValue* Xslt30Processor::callTemplateReturningValue(const char * stylesheet, const char* templateName){
          	if(exceptionOccurred()) {
          		//Possible error detected in the compile phase. Processor not in a clean state.
          		//Require clearing exception.
          		return NULL;
          	}
          	if(stylesheet == NULL && !stylesheetObject){
          		std::cerr<< "Error: No stylesheet found. Please compile stylesheet before calling callTemplateReturningValue or check exceptions"<<std::endl;
          		return NULL;
          	}
          	setProperty("resources", proc->getResourcesDirectory());
          	jmethodID ctsmID =
          			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
          					"callTemplateReturningValue",
          					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Lnet/sf/saxon/s9api/XdmValue;");
          	if (!ctsmID) {
          		std::cerr << "Error: "<<getDllname() << "callTemplateReturningValue" << " not found\n"
          				<< std::endl;

          	} else {
          	    JParameters comboArrays;
          		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);


          	    jstring result = NULL;
          	    jobject obj = (jobject)(SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, ctsmID,
          								SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
          								(stylesheet != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(stylesheet) : NULL ),
                                        (templateName != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(templateName) : NULL),
          								comboArrays.stringArray, comboArrays.objectArray));
          		if(obj) {
          			result = (jstring)obj;
          		}
          		if (comboArrays.stringArray != NULL) {
          			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
          			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
          		}
                  if (result) {
          		jclass atomicValueClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmAtomicValue");
          		jclass nodeClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmNode");
          		jclass functionItemClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmFunctionItem");
                  	XdmValue * value = NULL;
          		
          		XdmItem * xdmItem = NULL;
          			if(SaxonProcessor::sxn_environ->env->IsInstanceOf(result, atomicValueClass)           == JNI_TRUE) {
          				xdmItem =  new XdmAtomicValue(result);
					xdmItem->setProcessor(proc);
					SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
					return xdmItem;

          			} else if(SaxonProcessor::sxn_environ->env->IsInstanceOf(result, nodeClass)           == JNI_TRUE) {
          				xdmItem = new XdmNode(result);
					xdmItem->setProcessor(proc);
					SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
					return xdmItem;

          			} else if (SaxonProcessor::sxn_environ->env->IsInstanceOf(result, functionItemClass)           == JNI_TRUE) {
          				std::cerr<<"Error: callTemplateReturningValue: FunctionItem found. Currently not be handled"<<std::endl;
          				return NULL;
          			} else {
					value = new XdmValue(result, true);
					value->setProcessor(proc);
					for(int z=0;z<value->size();z++) {
						value->itemAt(z)->setProcessor(proc);
					}
		          		SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
					return value;
				}
			value = new XdmValue();
			value->setProcessor(proc);	
          		xdmItem->setProcessor(proc);
          		value->addXdmItem(xdmItem);
          		SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
          		return value;
         	} else  {
          		proc->checkAndCreateException(cppClass);
               	}
          }
        return NULL;
    }




XdmValue * Xslt30Processor::transformFileToValue(const char* sourcefile,
		const char* stylesheetfile) {

	if(exceptionOccurred()) {
		//Possible error detected in the compile phase. Processor not in a clean state.
		//Require clearing exception.
		return NULL;	
	}

	if(sourcefile == NULL && stylesheetfile == NULL && !stylesheetObject){
	
		return NULL;
	}

	setProperty("resources", proc->getResourcesDirectory());
	static jmethodID mID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"transformToValue",
					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Lnet/sf/saxon/s9api/XdmValue;");
	if (!mID) {
		std::cerr << "Error: "<< getDllname() << ".transformtoValue" << " not found\n"
				<< std::endl;

	} else {
	    JParameters comboArrays;
		comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);

		jobject result = (jobject)(
				SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, mID,
						SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
						(sourcefile != NULL ?
								SaxonProcessor::sxn_environ->env->NewStringUTF(sourcefile) :
								NULL),
						(stylesheetfile != NULL ?
								SaxonProcessor::sxn_environ->env->NewStringUTF(
										stylesheetfile) :
								NULL), comboArrays.stringArray, comboArrays.objectArray));
		if (comboArrays.stringArray != NULL) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
		}
		if (result) {
			jclass atomicValueClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmAtomicValue");
          		jclass nodeClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmNode");
          		jclass functionItemClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmFunctionItem");
			XdmValue * value = NULL;
          		XdmItem * xdmItem = NULL;


          			if(SaxonProcessor::sxn_environ->env->IsInstanceOf(result, atomicValueClass)           == JNI_TRUE) {
          				xdmItem = new XdmAtomicValue(result);


          			} else if(SaxonProcessor::sxn_environ->env->IsInstanceOf(result, nodeClass)           == JNI_TRUE) {
          				xdmItem = new XdmNode(result);


          			} else if (SaxonProcessor::sxn_environ->env->IsInstanceOf(result, functionItemClass)           == JNI_TRUE) {
          				std::cerr<<"Error: TransformFileToValue: FunctionItem found. Currently not be handled"<<std::endl;
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

          		SaxonProcessor::sxn_environ->env->DeleteLocalRef(result);
          		return value;
		}else {
	
			proc->checkAndCreateException(cppClass);
	   		
     		}
	}
	return NULL;

}


void Xslt30Processor::transformFileToFile(const char* source,
		const char* stylesheet, const char* outputfile) {

	if(exceptionOccurred()) {
		//Possible error detected in the compile phase. Processor not in a clean state.
		//Require clearing exception.
		return;	
	}
	if(!stylesheetObject && stylesheet==NULL){
		std::cerr<< "Error: stylesheet has not been set or created using the compile methods."<<std::endl;
		return;
	}
	setProperty("resources", proc->getResourcesDirectory());
	jmethodID mID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"transformToFile",
					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)V");
	if (!mID) {
		std::cerr << "Error: "<<getDllname() << "transformToFile" << " not found\n"
				<< std::endl;

	} else {
	    JParameters comboArrays;
        comboArrays = SaxonProcessor::createParameterJArray(parameters, properties);

		SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, mID,
								SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
								(source != NULL ?
										SaxonProcessor::sxn_environ->env->NewStringUTF(
												source) : NULL),
								SaxonProcessor::sxn_environ->env->NewStringUTF(stylesheet),							(outputfile != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(outputfile) :NULL),
								comboArrays.stringArray, comboArrays.objectArray);
		if (comboArrays.stringArray!= NULL) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(comboArrays.objectArray);
		}
		}
		proc->checkAndCreateException(cppClass);
	}






XdmValue * Xslt30Processor::getXslMessages(){

jmethodID mID =   (jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"getXslMessages",
					"()[Lnet/sf/saxon/s9api/XdmValue;");
	if (!mID) {
		std::cerr << "Error: "<<getDllname() << "transformToString" << " not found\n"
				<< std::endl;

	} else {
jobjectArray results = (jobjectArray)(
			SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, mID));
	int sizex = SaxonProcessor::sxn_environ->env->GetArrayLength(results);

	if (sizex>0) {
		XdmValue * value = new XdmValue();
		
		for (int p=0; p < sizex; ++p) 
		{
			jobject resulti = SaxonProcessor::sxn_environ->env->GetObjectArrayElement(results, p);
			value->addUnderlyingValue(resulti);
		}
		SaxonProcessor::sxn_environ->env->DeleteLocalRef(results);
		return value;
	}
	proc->checkAndCreateException(cppClass);
    }
    return NULL;


}

const char * Xslt30Processor::transformFileToString(const char* source,
		const char* stylesheet) {

	if(exceptionOccurred()) {
		//Possible error detected in the compile phase. Processor not in a clean state.
		//Require clearing exception.
		return NULL;	
	}
	if(source == NULL && stylesheet == NULL && !stylesheetObject){
		std::cerr<< "Error: No stylesheet found. Please compile stylesheet before calling transformFileToString or check exceptions"<<std::endl;
		return NULL;
	}
	setProperty("resources", proc->getResourcesDirectory());
	jmethodID mID =
			(jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(cppClass,
					"transformToString",
					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;");
	if (!mID) {
		std::cerr << "Error: "<<getDllname() << "transformFileToString" << " not found\n"
				<< std::endl;

	} else {
		jobjectArray stringArray = NULL;
		jobjectArray objectArray = NULL;
		jclass objectClass = lookForClass(SaxonProcessor::sxn_environ->env,
				"java/lang/Object");
		jclass stringClass = lookForClass(SaxonProcessor::sxn_environ->env,
				"java/lang/String");

		int size = parameters.size() + properties.size();
#ifdef DEBUG
		std::cerr<<"Properties size: "<<properties.size()<<std::endl;
		std::cerr<<"Parameter size: "<<parameters.size()<<std::endl;
#endif
		if (size > 0) {
			objectArray = SaxonProcessor::sxn_environ->env->NewObjectArray((jint) size,
					objectClass, 0);
			stringArray = SaxonProcessor::sxn_environ->env->NewObjectArray((jint) size,
					stringClass, 0);
			int i = 0;
			for (std::map<std::string, XdmValue*>::iterator iter =
					parameters.begin(); iter != parameters.end(); ++iter, i++) {

#ifdef DEBUG
				std::cerr<<"map 1"<<std::endl;
				std::cerr<<"iter->first"<<(iter->first).c_str()<<std::endl;
#endif
				SaxonProcessor::sxn_environ->env->SetObjectArrayElement(stringArray, i,
						SaxonProcessor::sxn_environ->env->NewStringUTF(
								(iter->first).c_str()));
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

				SaxonProcessor::sxn_environ->env->SetObjectArrayElement(objectArray, i,
						(iter->second)->getUnderlyingValue());

			}

			for (std::map<std::string, std::string>::iterator iter =
					properties.begin(); iter != properties.end(); ++iter, i++) {
				SaxonProcessor::sxn_environ->env->SetObjectArrayElement(stringArray, i,
						SaxonProcessor::sxn_environ->env->NewStringUTF(
								(iter->first).c_str()));
				SaxonProcessor::sxn_environ->env->SetObjectArrayElement(objectArray, i,
						SaxonProcessor::sxn_environ->env->NewStringUTF(
								(iter->second).c_str()));
			}
		}

	jstring result = NULL;
	jobject obj =
				(
						SaxonProcessor::sxn_environ->env->CallObjectMethod(cppXT, mID,
								SaxonProcessor::sxn_environ->env->NewStringUTF(cwdXT.c_str()),
						(source != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(
												source) : NULL),
								(stylesheet != NULL ? SaxonProcessor::sxn_environ->env->NewStringUTF(stylesheet) : NULL),
								stringArray, objectArray));
		if(obj) {
			result = (jstring)obj;
		}		
		if (size > 0) {
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(stringArray);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(objectArray);
		}
		if (result) {
			const char * str = SaxonProcessor::sxn_environ->env->GetStringUTFChars(result,
					NULL);
			SaxonProcessor::sxn_environ->env->DeleteLocalRef(obj);
			return str;
		} else  {
			proc->checkAndCreateException(cppClass);  
	   		
     		}
	}
	return NULL;
}


   const char * Xslt30Processor::transformToString(XdmNode * source){
	if(!stylesheetObject){
		std::cerr<< "Error: No stylesheet found. Please compile stylesheet before calling transformToString or check exceptions"<<std::endl;
		return NULL;
	}
	if(source != NULL){
      		source->incrementRefCount();
      		parameters["node"] = source;
    	}
	return transformFileToString(NULL, NULL);
   }


    XdmValue * Xslt30Processor::transformToValue(XdmNode * source){
	if(!stylesheetObject){
		std::cerr<< "Error: No stylesheet found. Please compile stylesheet before calling transformToValue or check exceptions"<<std::endl;
		return NULL;
	}
	if(source != NULL){
      		source->incrementRefCount();
      		parameters["node"] = source;
    	}
	return transformFileToValue(NULL, NULL);
   }

    void Xslt30Processor::transformToFile(XdmNode * source){
	if(!stylesheetObject){
		std::cerr<< "Error: No stylesheet found. Please compile stylesheet before calling transformToFile or check exceptions"<<std::endl;
		return;
	}
	if(source != NULL){
      		source->incrementRefCount();
      		parameters["node"] = source;
    	}
	transformFileToFile(NULL, NULL, NULL);
   }

const char * Xslt30Processor::getErrorMessage(int i ){
 	if(proc->exception == NULL) {return NULL;}
 	return proc->exception->getErrorMessage(i);
 }

