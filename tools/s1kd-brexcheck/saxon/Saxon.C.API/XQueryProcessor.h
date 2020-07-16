////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// This Source Code Form is "Incompatible With Secondary Licenses", as defined by the Mozilla Public License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SAXON_XQUERY_H
#define SAXON_XQUERY_H



#include "SaxonProcessor.h"
//#include "XdmValue.h"
#include <string>



class SaxonProcessor;
class XdmValue;
class XdmItem;

/*! An <code>XQueryProcessor</code> represents factory to compile, load and execute the query.
 * <p/>
 */
class XQueryProcessor {
public:

    //! Default constructor.
    XQueryProcessor();

    //! Constructor with the SaxonProcessor supplied.
    /*!
     * @param SaxonProcessor - Supplied pointer to the SaxonProcessor object
     * @param cwd - set the current working directory. Default is the empty string
    */
    XQueryProcessor(SaxonProcessor *p, std::string cwd="");

    ~XQueryProcessor(){
	clearProperties();
	clearParameters();
    }

    //!Set the initial context item for the query
    /**
      @param value - the initial context item, or null if there is to be no initial context item
    */
    void setContextItem(XdmItem * value);

    /**
     * Set the output file where the result is sent
    */
    void setOutputFile(const char* outfile);

    /**
     * Set the source from file for the query.
    */
    void setContextItemFromFile(const char * filename); 

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
     * @param namespacei currently not used
     * @param name  of the parameter
     * @return bool - outcome of the romoval
     */
    bool removeParameter(const char * name);

    /**
     * Set a property specific to the processor in use. 
     * XQueryProcessor: set serialization properties (names start with '!' i.e. name "!method" -> "xml")
     * 'o':outfile name, 's': source as file name
     * 'q': query file name, 'q': current by name, 'qs': string form of the query, 'base': set the base URI of the query, 'dtd': set DTD validation 'on' or 'off'
     * @param name of the property
     * @param value of the property
     */
    void setProperty(const char * name, const char * value);

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
     * Say whether the query is allowed to be updating. XQuery update syntax will be rejected
     * during query compilation unless this flag is set. XQuery Update is supported only under Saxon-EE.
     * @param updating - true if the query is allowed to use the XQuery Update facility
     *                 (requires Saxon-EE). If set to false, the query must not be an updating query. If set
     *                 to true, it may be either an updating or a non-updating query.
     */
    void setUpdating(bool updating);


    //!Perform the Query to file.
    /**
     * The result is is saved to file
     *
     * @param infilename - The file name of the source document
     * @param ofilename - The file name of where result will be stored
     * @param query - The query as string representation. TODO check
     */
    void executeQueryToFile(const char * infilename, const char * ofilename, const char * query);

     //!Perform the Query to a XdmValue representation.
    /**
     * @param infilename - The file name of the source document
     * @param ofilename - The file name of where result will be stored
     * @param query - The query as string representation
     * @return XdmValue - result of the the query as a XdmValue 
     */
    XdmValue * executeQueryToValue(const char * infilename, const char * query);


    //!Perform the Query to a string representation.
    /**
     * @param infilename - The file name of the source document
     * @param query - The query as string representation
     * @return char array - result of as a string
     */
    const char * executeQueryToString(const char * infilename, const char * query);

    //!Execute the Query cached.
    /** 
     * The use of the context item would have had to be set in advance
     * @return XdmValue of the result
     *
     */
    XdmValue * runQueryToValue();

    /**
     * Execute the Query cached.
     * The use of the context item would have had to be set in advance
     * @return Result as a string (i.e. pointer array of char)
     *
     */
    const char * runQueryToString();


    //!Execute the Query cached to file.
    /**
     * The use of the context item would have had to be set in advance
     * Assume the output filename has been set in advance
     * @return Result as a string (i.e. pointer array of char)
     *
     */
    void runQueryToFile();

     //!Declare a namespace binding.
     /**
     * Declare a namespace binding as part of the static context for queries compiled using this
     * XQueryCompiler. This binding may be overridden by a binding that appears in the query prolog.
     * The namespace binding will form part of the static context of the query, but it will not be copied
     * into result trees unless the prefix is actually used in an element or attribute name.
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


     //!Get all parameters as a std::map
     /**
      * @return std::map  - map of the parameters string->XdmValue* 
     */
     std::map<std::string,XdmValue*>& getParameters();

      //!Get all properties as a std::map
     /**
      * @return std::map map of the properties string->string 
     */
     std::map<std::string,std::string>& getProperties();

     //!Compile a query supplied as a file name.
    /**
     * The supplied query is cached for later execution.
     */
    void setQueryFile(const char* filename);

     //!Compile a query supplied as a string.
    /**
     * The supplied query is cached for later execution.
     */
    void setQueryContent(const char* content);

     //!Set the static base URI for the query
     /**
     * @param baseURI the static base URI; or null to indicate that no base URI is available
     */
    void setQueryBaseURI(const char * baseURI);

    /**
     * set the current working directory
    */
   void setcwd(const char* cwd);


     //! Check for exception thrown.
   /**
    * @return char*. Returns the main exception message if thrown otherwise return NULL
    */
    const char* checkException();

    /**
     * Checks for pending exceptions without creating a local reference to the exception object
     * @return bool - true when there is a pending exception; otherwise return false
    */
    bool exceptionOccurred();

    //! Clear any exception thrown
    void exceptionClear();

     //!Get number of errors reported during execution of the query
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


     //! Get the i'th error code if there are any error
    /**
     * After the execution of the query there may be a number of errors reported against it.
     * @return char* - The error code of the i'th exception.
    */
    const char * getErrorCode(int i);
    

private:
        std::string cwdXQ; /*!< current working directory */
	SaxonProcessor * proc;
	jclass  cppClass;
	jobject cppXQ;
	//std::string outputfile1; /*!< output file where result will be saved */
	bool queryFileExists;
	std::string failure; //for testing
	std::map<std::string,XdmValue*> parameters; /*!< map of parameters used for the transformation as (string, value) pairs */
	std::map<std::string,std::string> properties; /*!< map of properties used for the transformation as (string, string) pairs */
};

#endif /* SAXON_XQUERY_H */
