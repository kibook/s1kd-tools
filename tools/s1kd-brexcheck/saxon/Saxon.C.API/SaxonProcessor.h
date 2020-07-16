////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// This Source Code Form is "Incompatible With Secondary Licenses", as defined by the Mozilla Public License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SAXON_PROCESSOR_H
#define SAXON_PROCESSOR_H
	
#if defined __linux__ || defined __APPLE__
        #include <stdlib.h>
        #include <string.h>
        #include <dlfcn.h>

        #define HANDLE void*
        #define LoadLibrary(x) dlopen(x, RTLD_LAZY)
        #define GetProcAddress(x,y) dlsym(x,y)
#else
    #include <windows.h>
#endif

//#define DEBUG //remove
#define CVERSION "1.2.1"
#define CVERSION_API_NO 121
#include <string>
#include <iostream>
#include <sstream>  
#include <map>	
#include <vector>
#include <stdexcept>      // std::logic_error

#include "SaxonCGlue.h"
#include "SaxonCXPath.h"
#include "XsltProcessor.h"
#include "Xslt30Processor.h"
#include "XQueryProcessor.h"
#include "XPathProcessor.h"
#include "SchemaValidator.h"
//#include "com_saxonica_functions_extfn_PhpCall.h"
//#include "com_saxonica_functions_extfn_PhpCall_PhpFunctionCall.h"

class XsltProcessor;
class Xslt30Processor;
class XQueryProcessor;
class XPathProcessor;
class SchemaValidator;
class XdmValue;
class XdmNode;
class XdmItem;
class XdmAtomicValue;



// The Saxon XSLT interface class

//std::mutex mtx;
/*! <code>MyException</code>. This struct captures details of the Java exception thrown from Saxon s9api API (Java).
 * <p/>
 */
typedef struct {
		std::string errorCode;
		std::string errorMessage;
		int linenumber;
	    	bool isType;
	    	bool isStatic;
	    	bool isGlobal;
	}MyException;

typedef struct
{
    jobjectArray stringArray;
    jobjectArray objectArray;

}JParameters;


/*! <code>SaxonApiException</code>. An exception thrown by the Saxon s9api API (Java). This is always a C++ wrapper for some other underlying exception in Java
 * <p/>
 */
class SaxonApiException {

public:

    /**
     * A default Constructor. Create a SaxonApiException
     */
     SaxonApiException(){
	exceptions = std::vector<MyException>(0);
    }

    /**
     * A Copy constructor. Create a SaxonApiException
     * @param ex - The exception object to copy
     */
	SaxonApiException(const SaxonApiException &ex){
		exceptions = ex.exceptions;
	}

    /**
     * A constructor. Create a SaxonApiException
     * @param ec - The error code of the underlying exception thrown, if known
     * @param exM - The error message of the underlying exception thrown, if known
     */
	SaxonApiException(const char * ec, const char * exM){
		exceptions = std::vector<MyException>(0);
		MyException newEx;	
		if(ec != NULL){
			newEx.errorCode =   std::string(ec);
		} else {
			newEx.errorCode ="Unknown";	
		}
		if(exM != NULL){
			newEx.errorMessage =  std::string(exM);
		} else {
			newEx.errorMessage="Unkown";		
		}
		newEx.isType = false;
	    	newEx.isStatic = false;
	    	newEx.isGlobal = false;
		newEx.linenumber = 0;
		exceptions.push_back(newEx);
	}

    /**
     * A constructor. Create a SaxonApiException
     * @param ec - The error code of the underlying exception thrown, if known
     * @param exM - The error message of the underlying exception thrown, if known
     * @param typeErr - Flag indicating if the error is a type error
     * @param stat - Flag indicating a static error
     * @param glob - Flag for if the error is global
     * @param l - Line number information of where the error occurred
     */
	SaxonApiException(const char * ec, const char * exM, bool typeErr, bool stat, bool glob, int l){
		exceptions = std::vector<MyException>(20);
		MyException newEx;
		if(ec != NULL){
			newEx.errorCode =   std::string(ec);
		} else {
			newEx.errorCode ="ERROR1";	
		}
		if(exM != NULL){
			newEx.errorMessage =  std::string(exM);
		} else {
			newEx.errorMessage="ERROR2";		
		}
		newEx.isType = typeErr;
	    	newEx.isStatic = stat;
	    	newEx.isGlobal = glob;
		newEx.linenumber = l;
		exceptions.push_back(newEx);
	}

    /**
     * Creates a SaxonApiException and adds it to a vector of exceptions
     * @param ec - The error code of the underlying exception thrown, if known
     * @param exM - The error message of the underlying exception thrown, if known
     * @param typeErr - Flag indicating if the error is a type error
     * @param stat - Flag indicating a static error
     * @param glob - Flag for if the error is global
     * @param l - Line number information of where the error occurred
     */
	void add(const char * ec, const char * exM, bool typeErr, bool stat, bool glob, int l){
		MyException newEx;
		if(ec != NULL){
			newEx.errorCode =   std::string(ec);
		} else {
			newEx.errorCode ="ERROR1";	
		}
		if(exM != NULL){
			newEx.errorMessage =  std::string(exM);
		} else {
			newEx.errorMessage="ERROR2";		
		}
		newEx.isType = typeErr;
	    	newEx.isStatic = stat;
	    	newEx.isGlobal = glob;
		newEx.linenumber = l;
		exceptions.push_back(newEx);
	}


    /**
     * A destructor.
     */
	~SaxonApiException(){ 
	  clear();
	}

    /**
     * Get the error code associated with the ith exception in the vector, if there is one
     * @param i - ith exception in the vector
     * @return the associated error code, or null if no error code is available
     */
	const char * getErrorCode(int i){
		if((size_t)i <= exceptions.size()){
			return exceptions[i].errorCode.c_str();
		}
		return NULL;
	}


	int getLineNumber(int i){
		if((size_t)i <= exceptions.size()){
			return exceptions[i].linenumber;	
		}
		return 0;
	}

	bool isGlobalError(int i){
		if((size_t)i <= exceptions.size()){
			return exceptions[i].isGlobal;
		}
		return false;
	}

	bool isStaticError(int i){
		if((size_t)i <= exceptions.size()){
			return exceptions[i].isStatic;
		}
		return false;
	}

	bool isTypeError(int i){
		if((size_t) i <= exceptions.size()){
			return exceptions[i].isType;
		}
		return false;
	}

	void clear(){
	  for(size_t i =0; i< exceptions.size();i++) {
		exceptions[i].errorCode.clear();
		exceptions[i].errorMessage.clear();	
	  }
	  exceptions.clear();
	}

	int count(){
		return (int)exceptions.size();	
	}

    /**
     * Returns the detail message string of the ith throwable, if there is one
     * @param i - ith exception in the vector
     * @return the detail message string of this <tt>Throwable</tt> instance
     *         (which may be <tt>null</tt>).
     */
	const char * getErrorMessage(int i){
		if((size_t)i <= exceptions.size()){
			return exceptions[i].errorMessage.c_str();
		}
		return NULL;
	}

	const char * getErrorMessages(){
		std::string result;
		for(size_t i = 0;i<exceptions.size();i++) {
			result += getErrorMessage(i);
		}
		if(result.empty()) { return NULL;}
		return result.c_str();
	}

    /**
     * Returns the ith Exception added, if there is one
     * @param i - ith exception in the vector
     * @return MyException
     */
	MyException getException(int i){
		if((size_t)i <= exceptions.size()){
			return exceptions[i];	
		}
		throw 0;
	}

	

private:
	std::vector<MyException> exceptions; /*!< Capture exceptions in a std:vector */
};








//==========================================



/*! An <code>SaxonProcessor</code> acts as a factory for generating XQuery, XPath, Schema and XSLT compilers
 */
class SaxonProcessor {
friend class XsltProcessor;
friend class Xslt30Processor;
friend class XQueryProcessor;
friend class SchemaValidator;
friend class XPathProcessor;
friend class XdmValue;
friend class XdmAtomicValue;
public:

   //! A default constructor.
    /*!
      * Create Saxon Processor.
    */

    SaxonProcessor();

   //! constructor based upon a Saxon configuration file.
    /*!
      * Create Saxon Processor.
    */

    SaxonProcessor(const char * configFile);


   //! A constructor.
    /*!
      * Create Saxon Processor.
      * @param l - Flag that a license is to be used. Default is false.	
    */
    SaxonProcessor(bool l);

    SaxonProcessor& operator=( const SaxonProcessor& other );

   /*!

      * Destructor
    */
    ~SaxonProcessor();


   //! Get the Processor object. Method used in Python
   /* SaxonProcessor * getProcessor(){
	return this;
    }*/
	
   /*!

      * Create an XsltProcessor. An XsltProcessor is used to compile XSLT stylesheets.
      * @return a newly created XsltProcessor	
    */	
    XsltProcessor * newXsltProcessor();

   /*!

      * Create an Xslt30Processor. An Xslt30Processor is used to compile XSLT30 stylesheets.
      * @return a newly created Xslt30Processor	
    */	
    Xslt30Processor * newXslt30Processor();


    /*!
     * Create an XQueryProcessor. An XQueryProcessor is used to compile XQuery queries.
     *
     * @return a newly created XQueryProcessor
     */
    XQueryProcessor * newXQueryProcessor();


    /*!
     * Create an XPathProcessor. An XPathProcessor is used to compile XPath expressions.
     *
     * @return a newly created XPathProcessor
     */
    XPathProcessor * newXPathProcessor();

    /*!
     * Create a SchemaValidator which can be used to validate instance documents against the schema held by this
     * SchemaManager
     *
     * @return a new SchemaValidator
     */
    SchemaValidator * newSchemaValidator();


    /*!
     * Factory method. Unlike the constructor, this avoids creating a new StringValue in the case
     * of a zero-length string (and potentially other strings, in future)
     *
     * @param value the String value. Null is taken as equivalent to "".
     * @return the corresponding StringValue
     */
    XdmAtomicValue * makeStringValue(std::string str);

    /*!
     * Factory method. Unlike the constructor, this avoids creating a new StringValue in the case
     * of a zero-length string (and potentially other strings, in future)
     *
     * @param value the char pointer array. Null is taken as equivalent to "".
     * @return the corresponding StringValue
     */
    XdmAtomicValue * makeStringValue(const char * str);

    /*!
     * Factory method: makes either an Int64Value or a BigIntegerValue depending on the value supplied
     *
     * @param i the supplied primitive integer value
     * @return the value as a XdmAtomicValue which is a BigIntegerValue or Int64Value as appropriate
     */
    XdmAtomicValue * makeIntegerValue(int i);


    /*!
     * Factory method (for convenience in compiled bytecode)
     *
     * @param d the value of the double
     * @return a new XdmAtomicValue
     */
    XdmAtomicValue * makeDoubleValue(double d);

    /*!
     * Factory method (for convenience in compiled bytecode)
     *
     * @param f the value of the foat
     * @return a new XdmAtomicValue
     */
    XdmAtomicValue * makeFloatValue(float);

    /*!
     * Factory method: makes either an Int64Value or a BigIntegerValue depending on the value supplied
     *
     * @param l the supplied primitive long value
     * @return the value as a XdmAtomicValue which is a BigIntegerValue or Int64Value as appropriate
     */
    XdmAtomicValue * makeLongValue(long l);

    /*!
     * Factory method: makes a XdmAtomicValue representing a boolean Value
     *
     * @param b true or false, to determine which boolean value is
     *              required
     * @return the XdmAtomicValue requested
     */
    XdmAtomicValue * makeBooleanValue(bool b);

    /**
     * Create an QName Xdm value from string representation in clark notation
     * @param str - The value given in a string form in clark notation. {uri}local
     * @return XdmAtomicValue - value
    */
    XdmAtomicValue * makeQNameValue(const char * str);

    /*!
     * Create an Xdm Atomic value from string representation
     * @param type    - Local name of a type in the XML Schema namespace.
     * @param value - The value given in a string form.
     * In the case of a QName the value supplied must be in clark notation. {uri}local
     * @return XdmValue - value
    */
    XdmAtomicValue * makeAtomicValue(const char * type, const char * value);

     /**
     * Get the string representation of the XdmValue.
     * @return char array
     */
    const char * getStringValue(XdmItem * item);

    /**
     * Parse a lexical representation of the source document and return it as an XdmNode
    */
    XdmNode * parseXmlFromString(const char* source);

    /**
     * Parse a source document file and return it as an XdmNode.
    */
    XdmNode * parseXmlFromFile(const char* source);

    /**
     * Parse a source document available by URI and return it as an XdmNode.
    */
    XdmNode * parseXmlFromUri(const char* source);

    int getNodeKind(jobject);

    bool isSchemaAwareProcessor();

 

    /**
     * Checks for pending exceptions without creating a local reference to the exception object
     * @return bool - true when there is a pending exception; otherwise return false
    */
    bool exceptionOccurred();

    /**

     * Clears any exception that is currently being thrown. If no exception is currently being thrown, this routine has no effect.
    */
    void exceptionClear(bool clearCPPException=true);

    /**
     * Checks for pending exceptions and creates a SaxonApiException object, which handles one or more local exceptions objects
     * @param env
     * @param callingClass
     * @param callingObject
     * @return SaxonApiException
    */
    SaxonApiException * checkForExceptionCPP(JNIEnv* env, jclass callingClass,  jobject callingObject);

    SaxonApiException * getException();

    /*
      * Clean up and destroy Java VM to release memory used. 
     */
    static void release();


    /**
     * set the current working directory
    */
   void setcwd(const char* cwd);

    /**
     * get the current working directory
    */
   const char* getcwd();


    /**
     * set saxon resources directory
    */
   void setResourcesDirectory(const char* dir);
	
    /**
     * set catalog to be used in Saxon
    */
   void setCatalog(const char* catalogFile, bool isTracing);

    /**
     * get saxon resources directory
    */
   const char * getResourcesDirectory();

    /**
     * Set a configuration property specific to the processor in use. 
     * Properties specified here are common across all the processors.
     * Example 'l':enable line number has the value 'on' or 'off'
     * @param name of the property
     * @param value of the property
     */
    void setConfigurationProperty(const char * name, const char * value);

    /**
     * Clear configuration properties specific to the processor in use. 
     */
     void clearConfigurationProperties();


    /**
     * Get the Saxon version
     * @return char array
     */
    const char * version();

/*
     * Add a native method.
     * @param name of the native method
     * @param signature of the native method
     * @param fnPtr Pointer to the native method
 */
void addNativeMethod(char *name, char* signature, void * fnPtr){

	JNINativeMethod method;
	method.name = name;
	method.signature = signature;
	method.fnPtr = fnPtr;

	nativeMethodVect.push_back(method);

	

}

/*
     * Register several native methods for one class.
     * @param libName name of the library which contains the function(s). Loads the library
     * @param gMethods Register native methods. Default is NULL, also NULL allowed in which cause assumption is made the user has added native methods using the method addNativeMethod .
 * @return bool success of registered native method
 */
bool registerCPPFunction(char * libName, JNINativeMethod * gMethods=NULL){
	if(libName != NULL) {
		setConfigurationProperty("extc", libName);
			
	}

	if(gMethods == NULL && nativeMethodVect.size()==0) {
	return false;
	} else {
		if(gMethods == NULL) {
			//copy vector to gMethods
			gMethods = new JNINativeMethod[nativeMethodVect.size()];
		} 
		return registerNativeMethods(sxn_environ->env, "com/saxonica/functions/extfn/CppCall$PhpFunctionCall",
    gMethods, sizeof(gMethods) / sizeof(gMethods[0]));
	

	}
	return false;
}

/*
 * Register several native methods for one class.
 * @return bool success of registered native method
 */
static bool registerNativeMethods(JNIEnv* env, const char* className,
    JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        std::cerr<<"Native registration unable to find class "<< className<<std::endl;
        return false;
    }
	
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
       // std::cerr<<"RegisterNatives failed for "<< className<<std::endl;
        return false;
    }
    return true;
}


	/* TODO: Remove use of this method.*/
	const char* checkException(jobject cpp);

	/* Internal use*/
	void checkAndCreateException(jclass cppClass);



//	XPathEngine
//	XQueryEngine
//	SchemaManager

   // static JNIEnv *env;
    static int jvmCreatedCPP;
    static sxnc_environment * sxn_environ;
    static int refCount;
    std::string cwd; /*!< current working directory */
    jobject proc; /*!< Java Processor object */
    
    /*static JavaVM *jvm;*/
    
protected:



	jclass xdmAtomicClass;
	jclass  versionClass;
	jclass  procClass;
	jclass  saxonCAPIClass;
	std::string cwdV; /*!< current working directory */
	//std::string resources_dir; /*!< current Saxon resources directory */
	char * versionStr;
	std::map<std::string,XdmValue*> parameters; /*!< map of parameters used for the transformation as (string, value) pairs */
	std::map<std::string,std::string> configProperties; /*!< map of properties used for the transformation as (string, string) pairs */	 
	bool licensei; /*!< indicates whether the Processor requires a Saxon that needs a license file (i.e. Saxon-EE) other a Saxon-HE Processor is created  */
	bool closed;
	SaxonApiException* exception; /*!< Pointer to any potential exception thrown */

	JNINativeMethod * nativeMethods;
	std::vector<JNINativeMethod> nativeMethodVect; /*!< Vector of native methods defined by user */



private:

    

	void applyConfigurationProperties();
	// Saxon/C method for internal use
    static JParameters createParameterJArray(std::map<std::string,XdmValue*> parameters, std::map<std::string,std::string> properties);
    static jobjectArray createJArray(XdmValue ** values, int length);
};

//===============================================================================================

#endif /* SAXON_PROCESSOR_H */
