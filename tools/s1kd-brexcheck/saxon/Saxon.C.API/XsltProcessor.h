////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// This Source Code Form is "Incompatible With Secondary Licenses", as defined by the Mozilla Public License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SAXON_XSLT_H
#define SAXON_XSLT_H


#include "SaxonProcessor.h"
//#include "XdmValue.h"
#include <string>

class SaxonProcessor;
class XdmValue;
class XdmItem;
class XdmNode;

/*! An <code>XsltProcessor</code> represents factory to compile, load and execute a stylesheet.
 * It is possible to cache the context and the stylesheet in the <code>XsltProcessor</code>.
 */
class XsltProcessor {

public:

    //! Default constructor.
    /*!
      Creates a Saxon-HE product
    */
    XsltProcessor();

    //! Constructor with the SaxonProcessor supplied.
    /*!
      @param proc - Supplied pointer to the SaxonProcessor object
      cwd - The current working directory
    */
    XsltProcessor(SaxonProcessor* proc, std::string cwd="");

     ~XsltProcessor(){
	clearProperties();
	clearParameters();
     }

	//! Get the SaxonProcessor object
	/**
	* @return SaxonProcessor - Pointer to the object
	*/
    SaxonProcessor * getSaxonProcessor(){return proc;}

    //!set the current working directory
    /**
      * @param cwd - Current working directory
     */
   void setcwd(const char* cwd);



    //!Set the source document from an XdmNode for the transformation.
     /**
	* @param value - The source to the stylesheet as a pointer to the XdmNode object.
	*/	
    void setSourceFromXdmNode(XdmNode * value);

    /**
     * Set the source from file for the transformation.
    */
    void setSourceFromFile(const char * filename);

    /**
     * Set the output file of where the transformation result is sent
    */
    void setOutputFile(const char* outfile);


    /**
    * Say whether just-in-time compilation of template rules should be used.
    * @param jit true if just-in-time compilation is to be enabled. With this option enabled,
    *            static analysis of a template rule is deferred until the first time that the
    *            template is matched. This can improve performance when many template
    *            rules are rarely used during the course of a particular transformation; however,
    *            it means that static errors in the stylesheet will not necessarily cause the
    *            {@link #compile(Source)} method to throw an exception (errors in code that is
    *            actually executed will still be notified to the registered <code>ErrorListener</code>
    *            or <code>ErrorList</code>, but this may happen after the {@link #compile(Source)}
    *            method returns). This option is enabled by default in Saxon-EE, and is not available
    *            in Saxon-HE or Saxon-PE.
    *            <p><b>Recommendation:</b> disable this option unless you are confident that the
    *            stylesheet you are compiling is error-free.</p>
    */
    void setJustInTimeCompilation(bool jit);

    /**
     * Set the value of a stylesheet parameter
     *
     * @param name  the name of the stylesheet parameter, as a string. For namespaced parameter use the JAXP solution i.e. "{uri}name"
     * @param value the value of the stylesheet parameter, or null to clear a previously set value
     */
    void setParameter(const char* name, XdmValue*value);

    /**
     * Get a parameter value by name
     * @param name - Specified paramater name to get
     * @return XdmValue
    */
     XdmValue* getParameter(const char* name);


    /**
     * Remove a parameter (name, value) pair from a stylesheet
     *
     * @param name  the name of the stylesheet parameter
     * @return bool - outcome of the romoval
     */
    bool removeParameter(const char* name);

    /**
     * Set a property specific to the processor in use. 
     * XsltProcessor: set serialization properties (names start with '!' i.e. name "!method" -> "xml")
     * 'o':outfile name, 'it': initial template, 'im': initial mode, 's': source as file name
     * 'm': switch on message listener for xsl:message instructions (TODO: this feature should be event based), 'item'| 'node' : source supplied as an XdmNode object
     * @param name of the property
     * @param value of the property
     */
    void setProperty(const char* name, const char* value);

    /**
     * Get a property value by name
     * @param name - Specified paramater name to get
     * @return string - Get string of the property as char pointer array
    */
    const char* getProperty(const char* name);

	//! Get all parameters as a std::map
     /**
      * 
      * Please note that the key name has been prefixed with 'param:', for example 'param:name'
      * @return std:map with key as string name mapped to XdmValue. 
      * 
     */
     std::map<std::string,XdmValue*>& getParameters();

	//! Get all properties as a std::map
     /**
      *  
      * @return std:map with key as string name mapped to string values.
     */
     std::map<std::string,std::string>& getProperties();

    //!Clear parameter values set
    /**
     * Default behaviour (false) is to leave XdmValues in memory
     *  true then XdmValues are deleted
     *  @param deleteValues.  Individual pointers to XdmValue objects have to be deleted in the calling program
     */
    void clearParameters(bool deleteValues=false);

     //! Clear property values set
    void clearProperties();

    /**
     * Get the messages written using the <code>xsl:message</code> instruction
     * @return XdmValue - Messages returned as an XdmValue.
     */
    XdmValue * getXslMessages();//TODO allow notification of message as they occur


      //!Perform a one shot transformation.
    /**
     * The result is stored in the supplied outputfile.
     *
     * @param sourcefile - The file name of the source document
     * @param stylesheetfile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used
     * @param outputfile - The file name where results will be stored
     */
    void transformFileToFile(const char* sourcefile, const char* stylesheetfile, const char* outputfile); 

	//!Perform a one shot transformation.
    /**
     * The result is returned as a string
     *
     * @param sourcefile - The file name of the source document
     * @param stylesheetfile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used
     * @return char array - result of the transformation
     */
    const char * transformFileToString(const char* sourcefile, const char* stylesheetfile);

    /**
     * Perform a one shot transformation. The result is returned as an XdmValue
     *
     * @param sourcefile - The file name of the source document
     * @param stylesheetfile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used
     * @return XdmValue - result of the transformation
     */
    XdmValue * transformFileToValue(const char* sourcefile, const char* stylesheetfile);


     //! compile a stylesheet file.
    /**
     * The compiled stylesheet is cached and available for execution later.
     * @param stylesheet  - The file name of the stylesheet document.
     */
    void compileFromFile(const char* stylesheet);

     //!compile a stylesheet received as a string.
    /**
     * 
     * The compiled stylesheet is cached and available for execution later.
     * @param stylesheet as a lexical string representation
     */
    void compileFromString(const char* stylesheet);


     //!compile a stylesheet received as a string and save to an exported file (SEF).
    /**
     * 
     * The compiled stylesheet is saved as SEF to file store
     * @param stylesheet as a lexical string representation
     * @param filename - the file to which the compiled package should be saved
     */
    void compileFromStringAndSave(const char* stylesheet, const char* filename);


     //!compile a stylesheet received as a file and save to an exported file (SEF).
    /**
     * 
     * The compiled stylesheet is saved as SEF to file store
     * @param xslFilename - file name of the stylesheet
     * @param filename - the file to which the compiled package should be saved
     */
    void compileFromFileAndSave(const char* xslFilename, const char* filename);


     //!compile a stylesheet received as an XdmNode.
    /**
     * The compiled stylesheet is cached and available for execution later.
     * @param stylesheet as a lexical string representation
     * @param filename - the file to which the compiled package should be saved
     */
    void compileFromXdmNodeAndSave(XdmNode * node, const char* filename);

     //!compile a stylesheet received as an XdmNode.
    /**
     * The compiled stylesheet is cached and available for execution later.
     * @param stylesheet as a lexical string representation
     */
    void compileFromXdmNode(XdmNode * node);


    //! Internal method to release cached stylesheet
    /**
     *
     * @param void
     */
    void releaseStylesheet();


    //! Execute transformation to string. Properties supplied in advance.
    /**
     * Perform the transformation based upon what has been cached.
     * @return char*. Pointer to Array of chars. Result returned as a string.
     *
     */
    const char * transformToString();

    //! Execute transformation to Xdm Value. Properties supplied in advance.
    /**
     * Perform the transformation based upon cached stylesheet and any source document.
     * @return as an XdmValue.
     *
     */
    XdmValue * transformToValue();

    //! Execute transformation to file. Properties supplied in advance.
    /**
     * Perform the transformation based upon cached stylesheet and source document.
     * Assume the outputfile has been set in advance
     * @return as an XdmValue.
     *

     */
    void transformToFile();

    /**
     * Checks for pending exceptions without creating a local reference to the exception object
     * @return bool - true when there is a pending exception; otherwise return false
    */
    bool exceptionOccurred();


     //! Check for exception thrown.
	/**
	* @return cha*. Returns the exception message if thrown otherwise return NULL
	*/
    const char* checkException();


     //! Clear any exception thrown
    void exceptionClear();

     //!Get number of errors reported during execution or evaluate of stylesheet
    /**
     * A transformation may have a number of errors reported against it.
     * @return int - Count of the exceptions recorded against the transformation
    */
    int exceptionCount();

     //! Get the ith error message if there are any error
    /**
     * A transformation may have a number of errors reported against it.
     * @return char* - The message of the i'th exception 
    */
    const char * getErrorMessage(int i);

     //! Get the ith error code if there are any error
    /**
     * A transformation may have a number of errors reported against it.
     * @return char* - The error code of the i'th exception. The error code are related to the specific specification 
    */
    const char * getErrorCode(int i);



private:
	SaxonProcessor* proc;/*! */
	jclass  cppClass;
	jobject cppXT, stylesheetObject, xdmValuei;
        std::string cwdXT; /*!< current working directory */
	std::string outputfile1; /*!< output file where result will be saved */
	std::string failure; //for testing
	bool nodeCreated;
	std::map<std::string,XdmValue*> parameters; /*!< map of parameters used for the transformation as (string, value) pairs */
	std::map<std::string,std::string> properties; /*!< map of properties used for the transformation as (string, string) pairs */

};


#endif /* SAXON_XSLT_H */
