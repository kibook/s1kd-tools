////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// This Source Code Form is "Incompatible With Secondary Licenses", as defined by the Mozilla Public License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SAXON_XPATH_H
#define SAXON_XPATH_H



#include "SaxonProcessor.h"
//#include "XdmValue.h"
//#include "XdmItem.h"

#include <string>

class SaxonProcessor;
class XdmValue;
class XdmItem;

/*! An <code>XPathProcessor</code> represents factory to compile, load and execute the XPath query.
 * <p/>
 */
class XPathProcessor {
public:
	
    //! Default constructor.
    /*!
      Creates a Saxon-HE XPath product
    */
    XPathProcessor();

    ~XPathProcessor(){
	clearProperties();
	clearParameters(false);
	//delete contextItem;
    }

    //! Constructor with the SaxonProcessor supplied.
    /*!
      @param proc - Pointer to the SaxonProcessor object
      @param cwd - The current working directory
    */
    XPathProcessor(SaxonProcessor* proc, std::string cwd="");

    //! Set the static base URI for XPath expressions compiled using this XPathCompiler.
    /**
     * The base URI is part of the static context, and is used to resolve any relative URIs appearing within an XPath
     * expression, for example a relative URI passed as an argument to the doc() function. If no
     * static base URI is supplied, then the current working directory is used.
     * @param uriStr
     */
     void setBaseURI(const char * uriStr);

    //! Compile and evaluate an XPath expression
   /**
     * @param xpathStr - supplied as a character string
	@return XdmValue
   */
   XdmValue * evaluate(const char * xpathStr);
   

    //! Compile and evaluate an XPath expression. The result is expected to be a single XdmItem
   /**
     * @param xpathStr - supplied as a character string
	@return XdmItem
   */
   XdmItem * evaluateSingle(const char * xpathStr);

   void setContextItem(XdmItem * item);

    /**
     * set the current working directory
    */
   void setcwd(const char* cwd);

    //! Set the context item from  file
    void setContextFile(const char * filename); //TODO: setContextItemFromFile

    //! Evaluate the XPath expression, returning the effective boolean value of the result.
     /** @param xpathStr - supplied as a character string
	@return bool
   */
   bool effectiveBooleanValue(const char * xpathStr);

    /**
     * Set a parameter value used in the query
     *s
     * @param name  of the parameter, as a string. For namespaced parameter use the JAXP solution i.e. "{uri}name"
     * @param value of the query parameter, or null to clear a previously set value
     */
    void setParameter(const char * name, XdmValue*value);


    /**
     * Remove a parameter (name, value) pair
     *
     * @param name  of the parameter
     * @return bool - outcome of the romoval
     */
    bool removeParameter(const char * name);

    //!Set a property specific to the processor in use.
    /**
     * XPathProcessor: set serialization properties (names start with '!' i.e. name "!method" -> "xml")
     * 'o':outfile name, 's': context item supplied as file name
     * @param name of the property
     * @param value of the property
     */
    void setProperty(const char * name, const char * value);

    //!Declare a namespace binding as part of the static context for XPath expressions compiled using this XPathCompiler
     /**
     *
     * @param prefix The namespace prefix. If the value is a zero-length string, this method sets the default
     *               namespace for elements and types.
     * @param uri    The namespace URI. It is possible to specify a zero-length string to "undeclare" a namespace;
     *               in this case the prefix will not be available for use, except in the case where the prefix
     *               is also a zero length string, in which case the absence of a prefix implies that the name
     *               is in no namespace.
     * Assume the prefix or uri is null.
     */
    void declareNamespace(const char *prefix, const char * uri);

#if CVERSION_API_NO >= 121



    //! Say whether XPath 1.0 backwards compatibility mode is to be used
    /**
    * In backwards compatibility
    * mode, more implicit type conversions are allowed in XPath expressions, for example it
    * is possible to compare a number with a string. The default is false (backwards compatibility
    * mode is off).
    *
    * @param option true if XPath 1.0 backwards compatibility is to be enabled, false if it is to be disabled.
    */
    void setBackwardsCompatible(bool option);


    //! Say whether the compiler should maintain a cache of compiled expressions.
    /**
     * @param caching if set to true, caching of compiled expressions is enabled.
     *                If set to false, any existing cache is cleared, and future compiled expressions
     *                will not be cached until caching is re-enabled. The cache is also cleared
     *                (but without disabling future caching)
     *                if any method is called that changes the static context for compiling
     *                expressions, for example {@link #declareVariable(QName)} or
     *                {@link #declareNamespace(String, String)}.
    */

    void setCaching(bool caching);


    //! Import a schema namespace
    /**
     * Here we add the element and attribute declarations and type definitions
     * contained in a given namespace to the static context for the XPath expression.
     * <p>This method will not cause the schema to be loaded. That must be done separately, using the
     * {@link SchemaManager}. This method will not fail if the schema has not been loaded (but in that case
     * the set of declarations and definitions made available to the XPath expression is empty). The schema
     * document for the specified namespace may be loaded before or after this method is called.</p>
     * <p>This method does not bind a prefix to the namespace. That must be done separately, using the
     * {@link #declareNamespace(String, String)} method.</p>
     *
     * @param uri The schema namespace to be imported. To import declarations in a no-namespace schema,
     *            supply a zero-length string.
     */
    void importSchemaNamespace(const char * uri);

#endif
     /**
      * Get all parameters as a std::map
     */
     std::map<std::string,XdmValue*>& getParameters();

     /**
      * Get all properties as a std::map 
     */
     std::map<std::string,std::string>& getProperties();

    /**
     * Clear parameter values set
     *  @param deleteValues.  Individual pointers to XdmValue objects have to be deleted in the calling program
     * Default behaviour (false) is to leave XdmValues in memory
     *  true then XdmValues are deleted
     */
    void clearParameters(bool deleteValues=false);

    /**
     * Clear property values set
     */
    void clearProperties();

    /**
     * Checks for pending exceptions without creating a local reference to the exception object
     * @return bool - true when there is a pending exception; otherwise return false
    */

   // const char* checkException();

    /**
     * Checks for pending exceptions without creating a local reference to the exception object
     * @return bool - true when there is a pending exception; otherwise return false
    */
    bool exceptionOccurred();

    //! Clear any exception thrown
    void exceptionClear();

     //!Get number of errors reported during evaluation of the XPath
    /**
     * After the evalution of the XPAth expression there may be a number of errors reported against it.
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
     * After the execution of the XPath expression there may be  a number of errors reported against it.
     * @return char* - The error code of the i'th exception. 
    */
    const char * getErrorCode(int i);

     //! Check for exception thrown.
	/**
	* @return cha*. Returns the exception message if thrown otherwise return NULL
	*/
    const char* checkException();


private:
	SaxonProcessor * proc;
	XdmItem * contextItem;
        std::string cwdXP; /*!< current working directory */
	jclass  cppClass;
	jobject cppXP;
	std::map<std::string,XdmValue*> parameters; /*!< map of parameters used for the transformation as (string, value) pairs */
	std::map<std::string,std::string> properties; /*!< map of properties used for the transformation as (string, string) pairs */

};




#endif /* SAXON_XPATH_H */
