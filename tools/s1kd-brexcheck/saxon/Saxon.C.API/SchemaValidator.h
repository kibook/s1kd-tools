////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015 Saxonica Limited.
// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
// This Source Code Form is "Incompatible With Secondary Licenses", as defined by the Mozilla Public License, v. 2.0.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SAXON_SCHEMA_H
#define SAXON_SCHEMA_H


#include "SaxonProcessor.h"
//#include "XdmValue.h"
//#include "XdmItem.h"

#include <string>

class SaxonProcessor;
class XdmValue;
class XdmNode;
class XdmItem;

/*! An <code>SchemaValidator</code> represents factory for validating instance documents against a schema.
 * <p/>
 */
class SchemaValidator {
public:
	
    //! Default constructor.
    /*!
      Creates Schema Validator
    */
    SchemaValidator();

    //! A constructor with a SaxonProcessor.
    /*!
     * The supplied processor should have license flag set to true for the Schema Validator to operate.
     * @param SaxonProcessor
     * @param cwd - set the current working directory
    */
    SchemaValidator(SaxonProcessor* proc, std::string cwd="");

     //! Set the Current working Directory
    /**
     * Set the current working directory for the Schema Validator
     * @param cwd Supplied working directory which replaces any set cwd. Ignore if cwd is NULL. 
    */
   void setcwd(const char* cwd);

       //! Register the schema from file name 
    /**
     * Set the current working directory for the Schema Validator
    * @param xsd    - File name of the schema relative to the cwd 
    */
  void registerSchemaFromFile(const char * xsd);

  void registerSchemaFromString(const char * schemaStr);

  /*!
   Set the name of the output file that will be used by the validator.
  * @param outputFile the output file name for later use
 */
  void setOutputFile(const char * outputFile);


    /**
     * Validate an instance document by a registered schema.
     * @param sourceFile Name of the file to be validated. Allow null when source document is supplied
     * with other method
     */
  void validate(const char * sourceFile = NULL);
   
  //!Validate an instance document supplied as a Source object
   /**
  * @param sourceFile The name of the file to be validated. Default is NULL
  * @return XdmNode - the validated document returned to the calling program
  */
  XdmNode * validateToNode(const char * sourceFile = NULL);


     //!Set the source node for validation
    /**
     * @param The name of the file to be validated. Default is NULL
       @return Result of the validation stored and returned as an XdmNode object
    */
    void setSourceNode(XdmNode * source);

     //! Get the Validation report
   /**
    * The valdiation-report option must have been set to true in the properties to use this feature.
    * @return XdmNode - Pointer to XdmNode. Return NULL if validation reporting feature has not been enabled
   */
    XdmNode* getValidationReport();


    /**
     * Set a parameter value used in the validator
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
    /**
     * Set a property.
     *
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
      * Get all parameters as a std::map
     */
     std::map<std::string,XdmValue*>& getParameters();

     /**
      * Get all properties as a std::map 
     */
     std::map<std::string,std::string>& getProperties();

    /**
     * Checks for pending exceptions without creating a local reference to the exception object
     * @return bool - true when there is a pending exception; otherwise return false
    */
    bool exceptionOccurred();


     //! Check for exception thrown.
   /**
    * @return char*. Returns the main exception message if thrown otherwise return NULL
    */
    const char* checkException();

    //! Clear any exception thrown
    void exceptionClear();

     //!Get number of errors during validation of the source against the schema
    /**
     * @return int - Count of the exceptions reported during validation
    */
    int exceptionCount();

     //! Get the ith error message if there are any validation errors
    /**
     *  May have a number of validation errors reported against it the source.
     * @return char* - The message of the i'th exception 
    */
    const char * getErrorMessage(int i);

     //! Get the i'th error code if there are any error
    /**
     * Validation error are reported as exceptions. All errors can be retrieved.
     * @return char* - The error code of the i'th exception.
    */
    const char * getErrorCode(int i);

     //! The validation mode may be either strict or lax.
    /**
     * The default is strict; this method may be called
     * to indicate that lax validation is required. With strict validation, validation fails if no element
     * declaration can be located for the outermost element. With lax validation, the absence of an
     * element declaration results in the content being considered valid.
     * @param lax true if validation is to be lax, false if it is to be strict
    */
    void setLax(bool l){
      lax = l;
    }


private:
    bool lax;
	SaxonProcessor * proc;
	XdmItem * sourceNode;
	jclass  cppClass;
	jobject cppV;
	std::string cwdV; /*!< current working directory */
	std::string outputFile;
	std::map<std::string,XdmValue*> parameters; /*!< map of parameters used for the transformation as (string, value) pairs */
	std::map<std::string,std::string> properties; /*!< map of properties used for the transformation as (string, string) pairs */

};




#endif /* SAXON_XPATH_H */
