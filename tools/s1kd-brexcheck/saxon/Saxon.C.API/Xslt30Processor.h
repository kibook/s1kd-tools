////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// This Source Code Form is "Incompatible With Secondary Licenses", as defined by the Mozilla Public License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SAXON_XSLT30_H
#define SAXON_XSLT30_H


#include "SaxonProcessor.h"
//#include "XdmValue.h"
#include <string>

class SaxonProcessor;
class XdmValue;
class XdmItem;
class XdmNode;

/*! An <code>Xslt30Processor</code> represents factory to compile, load and execute a stylesheet.
 * It is possible to cache the context and the stylesheet in the <code>Xslt30Processor</code>.
 */
class Xslt30Processor {

public:

    //! Default constructor.
    /*!
      Creates a Saxon-HE product
    */
    Xslt30Processor();

    //! Constructor with the SaxonProcessor supplied.
    /*!
      @param proc - Supplied pointer to the SaxonProcessor object
      cwd - The current working directory
    */
    Xslt30Processor(SaxonProcessor* proc, std::string cwd="");

     ~Xslt30Processor();

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

    void setGlobalContextItem(XdmItem * value);

    /**
     * Set the source from file for the transformation.
    */
    void setGlobalContextFromFile(const char * filename);


    //!The initial value to which templates are to be applied (equivalent to the <code>select</code> attribute of <code>xsl:apply-templates</code>)
    /**
    *  @param selection - The value to which the templates are to be applied
    */
    void setInitialMatchSelection(XdmValue * selection);

        //!The initial filename to which templates are to be applied (equivalent to the <code>select</code> attribute of <code>xsl:apply-templates</code>).
        /**
        * The file is parsed internall
        *  @param filename - The file name to which the templates are to be applied
        */
    void setInitialMatchSelectionAsFile(const char * filename);

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
    *Set true if the return type of callTemplate, applyTemplates and transform methods is to return XdmValue,
    *otherwise return XdmNode object with root Document node
    * @param option true if return raw result, i.e. XdmValue, otherwise return XdmNode
    *
    */	
    void setResultAsRawValue(bool option);

    /**
     * Set the value of a stylesheet parameter
     *
     * @param name  the name of the stylesheet parameter, as a string. For namespaced parameter use the JAXP solution i.e. "{uri}name"
     * @param value the value of the stylesheet parameter, or null to clear a previously set value
     * @param _static For static (compile-time) parameters we set this flag to true, which means the parameter is
     * must be set on the XsltCompiler object, prior to stylesheet compilation. The default is false. Non-static parameters
     * may also be provided.
     */
    void setParameter(const char* name, XdmValue*value, bool _static=false);



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
     * Set parameters to be passed to the initial template. These are used
     * whether the transformation is invoked by applying templates to an initial source item,
     * or by invoking a named template. The parameters in question are the xsl:param elements
     * appearing as children of the xsl:template element.
     * <p>The parameters are supplied in the form of a map; the key is a QName given as a string which must
     * match the name of the parameter; the associated value is an XdmValue containing the
     * value to be used for the parameter. If the initial template defines any required
     * parameters, the map must include a corresponding value. If the initial template defines
     * any parameters that are not present in the map, the default value is used. If the map
     * contains any parameters that are not defined in the initial template, these values
     * are silently ignored.</p>
     * <p>The supplied values are converted to the required type using the function conversion
     * rules. If conversion is not possible, a run-time error occurs (not now, but later, when
     * the transformation is actually run).</p>
     * <p>The <code>XsltTransformer</code> retains a reference to the supplied map, so parameters can be added or
     * changed until the point where the transformation is run.</p>
     * <p>The XSLT 3.0 specification makes provision for supplying parameters to the initial
     * template, as well as global stylesheet parameters. Although there is no similar provision
     * in the XSLT 1.0 or 2.0 specifications, this method works for all stylesheets, regardless whether
     * XSLT 3.0 is enabled or not.</p>
     *
     * @param parameters the parameters to be used for the initial template
     * @param tunnel     true if these values are to be used for setting tunnel parameters;
     *                   false if they are to be used for non-tunnel parameters
     */

    void setInitialTemplateParameters(std::map<std::string,XdmValue*> parameters, bool tunnel);

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

    XdmValue ** createXdmValueArray(int len){
	return (new XdmValue*[len]);
    }

    void deleteXdmValueArray(XdmValue ** arr, int len){
	for(int i =0; i< len; i++) {
		//delete arr[i];	
	}
	delete [] arr;
    }

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



     //! Get the stylesheet associated
     /* via the xml-stylesheet processing instruction (see
     * http://www.w3.org/TR/xml-stylesheet/) with the document
     * document specified in the source parameter, and that match
     * the given criteria.  If there are several suitable xml-stylesheet
     * processing instructions, then the returned Source will identify
     * a synthesized stylesheet module that imports all the referenced
     * stylesheet module.*/
    /**
     * The compiled stylesheet is cached and available for execution later.
     * @param sourceFile  - The file name of the XML document.
     */
    void compileFromAssociatedFile(const char* sourceFile);


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

     //!Invoke the stylesheet by applying templates to a supplied input sequence, Saving the results to file.
    /**
     * The initial match selection must be set using one of the two methods setInitialMatchSelection or setInitialMatchSelectionFile.
     * The result is stored in the supplied output file.
     *
     * @param stylesheetFile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used. It is possible to set the stylsheet using one of the following methods: compileFromFile, compileFromString or compileFromAssociatedFile
     * @param outputfile - The file name where results will be stored
     */
    void applyTemplatesReturningFile(const char * stylesheetFilename, const char* outfile);

     //!Invoke the stylesheet by applying templates to a supplied input sequence, Saving the results as serialized string.
    /**
     * The initial match selection must be set using one of the two methods setInitialMatchSelection or setInitialMatchSelectionFile.
     *
     * @param stylesheetFile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used. It is possible to set the stylsheet using one of the following methods: compileFromFile, compileFromString or compileFromAssociatedFile
     */
    const char* applyTemplatesReturningString(const char * stylesheetFilename=NULL);

     //!Invoke the stylesheet by applying templates to a supplied input sequence, Saving the results as an XdmValue.
    /**
     * The initial match selection must be set using one of the two methods setInitialMatchSelection or setInitialMatchSelectionFile.
     *
     * @param stylesheetfile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used. It is possible to set the stylsheet using one of the following methods: compileFromFile, compileFromString or compileFromAssociatedFile
     */
    XdmValue * applyTemplatesReturningValue(const char * stylesheetFilename=NULL);

    //! Invoke a transformation by calling a named template and save result to file.
    /** The results of calling the template are wrapped in a document node, which is then sent to the specified
    * file destination. If setInitialTemplateParameters(std::Map, boolean) has been
    * called, then the parameters supplied are made available to the called template (no error
    * occurs if parameters are supplied that are not used).
    * @param stylesheetfile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used. It is possible to set the stylsheet using one of the following methods: compileFromFile, compileFromString or compileFromAssociatedFile
    * @param templateName - The name of the initial template. This must match the name of a public named template in the stylesheet. If the value is null, the clark name for xsl:initial-template is used.
    * @param outputfile - The file name where results will be stored,
    */
    void callTemplateReturningFile(const char * stylesheetFilename, const char* templateName, const char* outfile);

    //! Invoke a transformation by calling a named template and return result as a string.
    /** The results of calling the template are wrapped in a document node, which is then serialized as a string.
    * If setInitialTemplateParameters(std::Map, boolean) has been
    * called, then the parameters supplied are made available to the called template (no error
    * occurs if parameters are supplied that are not used).
    * @param stylesheetfile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used. It is possible to set the stylsheet using one of the following methods: compileFromFile, compileFromString or compileFromAssociatedFile
    * @param templateName - the name of the initial template. This must match the name of a public named template in the stylesheet. If the value is null, the clark name for xsl:initial-template is used.
    * @param outputfile - The file name where results will be stored,
    */
    const char* callTemplateReturningString(const char * stylesheetFilename, const char* templateName);

    //! Invoke a transformation by calling a named template and return result as a string.
    /** The results of calling the template are wrapped in a document node, which is then returned as an XdmValue.
    * If setInitialTemplateParameters(std::Map, boolean) has been
    * called, then the parameters supplied are made available to the called template (no error
    * occurs if parameters are supplied that are not used).
    * @param stylesheetfile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used. It is possible to set the stylsheet using one of the following methods: compileFromFile, compileFromString or compileFromAssociatedFile
    * @param templateName - the name of the initial template. This must match the name of a public named template in the stylesheet. If the value is null, the clark name for xsl:initial-template is used.
    * @param outputfile - The file name where results will be stored,
    */
    XdmValue* callTemplateReturningValue(const char * stylesheetFilename, const char* templateName);



    //! Call a public user-defined function in the stylesheet
    /** Here we wrap the result in an XML document, and sending this document to a specified file
    * @param stylesheetfile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used. It is possible to set the stylsheet using one of the following methods: compileFromFile, compileFromString or compileFromAssociatedFile
    * @param functionName - The name of the function to be called
    * @param arguments - Pointer array of XdmValue object - he values of the arguments to be supplied to the function. These
    *                    will be converted if necessary to the type as defined in the function signature, using
    *                    the function conversion rules.
    * @param argument_length - the Coutn of arguments objects in the array
    * @param outputfile - The file name where results will be stored,
    */
    void callFunctionReturningFile(const char * stylesheetFilename, const char* functionName, XdmValue ** arguments, int argument_length, const char* outfile);


    //! Call a public user-defined function in the stylesheet
    /** Here we wrap the result in an XML document, and serialized this document to string value
    * @param stylesheetfile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used. It is possible to set the stylsheet using one of the following methods: compileFromFile, compileFromString or compileFromAssociatedFile
    * @param functionName - The name of the function to be called
    * @param arguments - Pointer array of XdmValue object - he values of the arguments to be supplied to the function. These
    *                    will be converted if necessary to the type as defined in the function signature, using
    *                    the function conversion rules.
    * @param argument_length - the Coutn of arguments objects in the array
    * @param outputfile - The file name where results will be stored,
    */
    const char * callFunctionReturningString(const char * stylesheetFilename, const char* functionName, XdmValue ** arguments, int argument_length);

    //! Call a public user-defined function in the stylesheet
    /** Here we wrap the result in an XML document, and return the document as an XdmValue
    * @param stylesheetfile - The file name of the stylesheet document. If NULL the most recently compiled stylesheet is used. It is possible to set the stylsheet using one of the following methods: compileFromFile, compileFromString or compileFromAssociatedFile
    * @param functionName - The name of the function to be called
    * @param arguments - Pointer array of XdmValue object - he values of the arguments to be supplied to the function. These
    *                    will be converted if necessary to the type as defined in the function signature, using
    *                    the function conversion rules.
    * @param argument_length - the Coutn of arguments objects in the array
    * @param outputfile - The file name where results will be stored,
    */
    XdmValue * callFunctionReturningValue(const char * stylesheetFilename, const char* functionName, XdmValue ** arguments, int argument_length);


    //! File names to XsltPackages stored on filestore are added to a set of packages.
    /***
     * The added XSLT packages will be imported later when compiling
     * @param fileNames - packs array of file names of XSLT packages stored in filestore
     * @param length - The number of package names in the array
     */

    void addPackages(const char ** fileNames, int length);

    void clearPackages();



    //! Internal method to release cached stylesheet
    /**
     *
     * @param void
     */
    void releaseStylesheet();


    //! Execute transformation to string. Properties supplied in advance.
    /**
     * Perform the transformation based upon what has been cached.
     * @param source - source document suppplied as an XdmNode object
     * @return char*. Pointer to Array of chars. Result returned as a string.
     *
     */
    const char * transformToString(XdmNode * source);

    //! Execute transformation to Xdm Value. Properties supplied in advance.
    /**
     * Perform the transformation based upon cached stylesheet and any source document.
     * @param source - source document suppplied as an XdmNode object
     * @return as an XdmValue.
     *
     */
    XdmValue * transformToValue(XdmNode * source);

    //! Execute transformation to file. Properties supplied in advance.
    /**
     * Perform the transformation based upon cached stylesheet and source document.
     * Assume the outputfile has been set in advance
     * @param source - source document suppplied as an XdmNode object
     * @return as an XdmValue.
     *

     */
    void transformToFile(XdmNode * source);

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
	jobject cppXT, stylesheetObject, xdmValuei, selection;
	XdmValue * selectionV;
        std::string cwdXT; /*!< current working directory */
	std::string outputfile1; /*!< output file where result will be saved */
	std::string failure; //for testing
	bool nodeCreated;
	bool tunnel;
	std::map<std::string,XdmValue*> parameters; /*!< map of parameters used for the transformation as (string, value) pairs */
	
	std::map<std::string,std::string> properties; /*!< map of properties used for the transformation as (string, string) pairs */

};


#endif /* SAXON_XSLT30_H */
