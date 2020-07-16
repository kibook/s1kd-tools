#include "SaxonCXPath.h"




    /**
     * Create boxed Boolean value
     * @param val - boolean value
     */
jobject booleanValue(sxnc_environment *environi, bool b){ 
	
	 if(environi->env == NULL) {
		printf("Error: Saxon-C env variable is null\n");
		fflush (stdout);
           	return NULL;
	 }
	 jclass  booleanClass = lookForClass(environi->env, "java/lang/Boolean");
	 static jmethodID MID_init = NULL;
	 if(!MID_init) {
  	   MID_init = (jmethodID)findConstructor (environi->env, booleanClass, "(Z)V");
	 }
	 jobject booleanValue = (jobject)(*(environi->env))->NewObject(environi->env, booleanClass, MID_init, (jboolean)b);
      	 if (!booleanValue) {
	    	printf("Error: failed to allocate Boolean object\n");
		fflush (stdout);
        	return NULL;
      	 }
	 return booleanValue;
}

    /**
     * Create an boxed Integer value
     * @param val - int value
     */
jobject integerValue(sxnc_environment *environi, int i){ 
	
	 if(environi->env == NULL) {
		printf("Error: Saxon-C env variable is null\n");
		fflush (stdout);
           	return NULL;
	 }

	 jclass  integerClass = lookForClass(environi->env, "java/lang/Integer");
 	/*static */ jmethodID intMID = NULL;
	 //if(!intMID){
		intMID = (jmethodID)(*(environi->env))->GetMethodID (environi->env, integerClass, "<init>", "(I)V");
	 //}
if(!intMID){
	printf("error in intMID");
}	
	 jobject intValue = (*(environi->env))->NewObject(environi->env, integerClass, intMID, (jint)i);
      	 if (!intValue) {
	    	printf("Error: failed to allocate Integer object\n");
printf("Value to build: %i",i);
		fflush (stdout);
//(*(environi->env))->ExceptionDescribe(environi->env); //remove line
        	return NULL;
      	 }
	 return intValue;
	}


    /**
     * Create an boxed Double value
     * @param val - double value
     */
jobject doubleValue(sxnc_environment *environi, double d){ 
	
	 if(environi->env == NULL) {
		printf("Error: Saxon-C env variable is null\n");
		fflush (stdout);
           	return NULL;
	 }
	 jclass  doubleClass = lookForClass(environi->env, "java/lang/Double");
	 static jmethodID dbID = NULL;
	if(!dbID) {
		dbID = (jmethodID)findConstructor (environi->env, doubleClass, "(D)V");
	}	
	 jobject doubleValue = (jobject)(*(environi->env))->NewObject(environi->env, doubleClass, dbID, (jdouble)d);
      	 if (!doubleValue) {
	    	printf("Error: failed to allocate Double object\n");
		fflush (stdout);
        	return NULL;
      	 }
	 return doubleValue;
	}

    /**
     * Create an boxed Float value
     * @param val - float value
     */
jobject floatValue(sxnc_environment *environi, float f){ 
	
	 if(environi->env == NULL) {
		printf("Error: Saxon-C env variable is null\n");
		fflush (stdout);
           	return NULL;
	 }
	 jclass  floatClass = lookForClass(environi->env, "java/lang/Float");
	 static jmethodID fID = NULL;
	 if(!fID){
	 	fID = (jmethodID)findConstructor (environi->env, floatClass, "(F)V");
	 }
	 jobject floatValue = (jobject)(*(environi->env))->NewObject(environi->env, floatClass, fID, (jfloat)f);
      	 if (!floatValue) {
	    	printf("Error: failed to allocate float object\n");
		fflush (stdout);
        	return NULL;
      	 }
	 return floatValue;
	}


    /**
     * Create an boxed Long value
     * @param val - Long value
     */
jobject longValue(sxnc_environment *environi, long l){ 
	
	 if(environi->env == NULL) {
		printf("Error: Saxon-C env variable is null\n");
		fflush (stdout);
           	return NULL;
	 }
	 jclass  longClass = lookForClass(environi->env, "java/lang/Long");
	 static jmethodID lID = NULL;
	 if(lID) {
		  lID = (jmethodID)findConstructor (environi->env, longClass, "(L)V");
	}	
	 jobject longValue = (jobject)(*(environi->env))->NewObject(environi->env, longClass, lID, (jlong)l);
      	 if (!longValue) {
	    	printf("Error: failed to allocate long object\n");
		fflush (stdout);
        	return NULL;
      	 }
	 return longValue;
	}

    /**
     * Create an boxed String value
     * @param val - as char array value
     */
jobject getJavaStringValue(sxnc_environment *environi, const char *str){ 
	
	 if(environi->env == NULL) {
		printf("Error: Saxon-C env variable is null\n");
		fflush (stdout);
           	return NULL;
	 }
	if(str == NULL) {
		return (*(environi->env))->NewStringUTF(environi->env, "");	
	}
	jstring jstrBuf = (*(environi->env))->NewStringUTF(environi->env, str);
	 
      	 if (!jstrBuf) {
	    	printf("Error: failed to allocate String object\n");
		fflush (stdout);
        	return NULL;
      	 }
	 return jstrBuf;
	}



jobject xdmValueAsObj(sxnc_environment *environi, const char* type, const char* str){ 
	
	jclass  saxoncClass = lookForClass(environi->env, "net/sf/saxon/option/cpp/SaxonCAPI");
	 if(environi->env == NULL) {
		printf("Error: Saxon-C env variable is null\n");
		fflush (stdout);
           	return NULL;
	 }
	char methodName[] = "createXdmAtomicItem";
    	char args[] = "(Ljava/lang/String;Ljava/lang/String;)Lnet/sf/saxon/s9api/XdmValue;";
		
	static jmethodID MID_xdmValueo = NULL;
	if(!MID_xdmValueo) {
		MID_xdmValueo = (jmethodID)(*(environi->env))->GetStaticMethodID(environi->env, saxoncClass, methodName, args);
	}
       if (!MID_xdmValueo) {
	  printf("\nError: MyClassInDll %s() not found\n",methodName);
  	  fflush (stdout);
          return NULL;
      }
   jobject resultObj = ((*(environi->env))->CallStaticObjectMethod(environi->env, saxoncClass, MID_xdmValueo, type, str));
   if(resultObj) {
	return resultObj;
   } 
   return NULL;
}

    /**
     * A Constructor. Create a XdmValue based on the target type. Conversion is applied if possible
     * @param type - specify target type of the value as the local name of the xsd built in type. For example 'gYearMonth' 
     * @param val - Value to convert
     */
sxnc_value * xdmValue(sxnc_environment *environi, const char* type, const char* str){ 
	
	jclass  saxoncClass = lookForClass(environi->env, "net/sf/saxon/option/cpp/SaxonCAPI");
	 if(environi->env == NULL) {
		printf("Error: Saxon-C env variable is null\n");
		fflush (stdout);
           	return NULL;
	 }
	char methodName[] = "createXdmAtomicValue";
    	char args[] = "(Ljava/lang/String;Ljava/lang/String;)Lnet/sf/saxon/s9api/XdmValue;";
		
	static jmethodID MID_xdmValue = NULL;
	if(!MID_xdmValue) {
		MID_xdmValue = (jmethodID)(*(environi->env))->GetStaticMethodID(environi->env, saxoncClass, methodName, args);
	}
       if (!MID_xdmValue) {
	  printf("\nError: MyClassInDll %s() not found\n",methodName);
  	  fflush (stdout);
          return NULL;
      }
   jobject resultObj = ((*(environi->env))->CallStaticObjectMethod(environi->env, saxoncClass, MID_xdmValue, type, str));
   if(resultObj) {
	sxnc_value* result = (sxnc_value *)malloc(sizeof(sxnc_value));
         result->xdmvalue = resultObj; 
	 return result; 
   } 
   return NULL;
}

sxnc_value * evaluate(sxnc_environment *environi, sxnc_processor ** proc, char * cwd, char * xpathStr, sxnc_parameter *parameters, sxnc_property * properties, int parLen, int propLen){
	
	static jmethodID emID = NULL; //cache the methodID
	if(!proc){} // code to avoid warning of unused variable. TODO: Remove use of variable in next release
	jclass cppClass = lookForClass(environi->env, "net/sf/saxon/option/cpp/XPathProcessor");

	if(!cpp) {
		cpp = (jobject) createSaxonProcessor (environi->env, cppClass, "(Z)V", NULL, (jboolean)sxn_license);
	}

	if(emID == NULL) {
		emID = (jmethodID)(*(environi->env))->GetMethodID (environi->env, cppClass,"evaluateSingle", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Lnet/sf/saxon/s9api/XdmItem;");
		if (!emID) {
		        printf("Error: MyClassInDll. evaluateSingle not found\n");
			fflush (stdout);
			return 0;
		 } 
	}
 	
	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;
	int size = parLen + propLen+1; //We add one here for the resources-dir property

	if(size >0) {
           jclass objectClass = lookForClass(environi->env, "java/lang/Object");
	   jclass stringClass = lookForClass(environi->env, "java/lang/String");
	   objectArray = (*(environi->env))->NewObjectArray(environi->env, (jint)size, objectClass, 0 );
	   stringArray = (*(environi->env))->NewObjectArray(environi->env, (jint)size, stringClass, 0 );
	   if((!objectArray) || (!stringArray)) { return 0;}
	   int i=0;
	   for(i =0; i< parLen; i++) {
		
	     (*(environi->env))->SetObjectArrayElement(environi->env, stringArray, i, (*(environi->env))->NewStringUTF(environi->env, parameters[i].name) );
	     (*(environi->env))->SetObjectArrayElement(environi->env, objectArray, i, (jobject)(parameters[i].value) );
	
	   }

	   (*(environi->env))->SetObjectArrayElement(environi->env, stringArray, i, (*(environi->env))->NewStringUTF(environi->env,"resources"));
	     (*(environi->env))->SetObjectArrayElement(environi->env, objectArray, i, (jobject)((*(environi->env))->NewStringUTF(environi->env, getResourceDirectory())) );
	    i++;
	   int j=0;	
  	   for(; j < propLen; j++, i++) {
	     (*(environi->env))->SetObjectArrayElement(environi->env, stringArray, i, (*(environi->env))->NewStringUTF(environi->env, properties[j].name));
	     (*(environi->env))->SetObjectArrayElement(environi->env, objectArray, i, (jobject)((*(environi->env))->NewStringUTF(environi->env, properties[j].value)) );
	   }
	  
	}
      jobject resultObj = (jstring)((*(environi->env))->CallObjectMethod(environi->env, cpp, emID, (cwd== NULL ? (*(environi->env))->NewStringUTF(environi->env, "") : (*(environi->env))->NewStringUTF(environi->env, cwd)), (*(environi->env))->NewStringUTF(environi->env,xpathStr), stringArray, objectArray ));

      (*(environi->env))->DeleteLocalRef(environi->env, objectArray);
      (*(environi->env))->DeleteLocalRef(environi->env, stringArray);
      if(resultObj) {
	 sxnc_value* result = (sxnc_value *)malloc(sizeof(sxnc_value));
         result->xdmvalue = resultObj;  
	
	 
	return result;
     }

    checkForException(environi,cpp);
    return 0;

}


bool effectiveBooleanValue(sxnc_environment *environi, sxnc_processor ** proc, char * cwd, char * xpathStr, sxnc_parameter *parameters, sxnc_property * properties, int parLen, int propLen){
	
	static jmethodID bmID = NULL; //cache the methodID
	if(!proc){} // code to avoid warning of unused variable. TODO: Remove use of variable in next release
	jclass cppClass = lookForClass(environi->env, "net/sf/saxon/option/cpp/XPathProcessor");

	if(!cpp) {
		cpp = (jobject) createSaxonProcessor (environi->env, cppClass, "(Z)V", NULL, (jboolean)sxn_license);
	}

	if(bmID == NULL) {
		bmID = (jmethodID)(*(environi->env))->GetMethodID (environi->env, cppClass,"effectiveBooleanValue", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Z");
		if (!bmID) {
		        printf("Error: MyClassInDll. effectiveBooleanValue not found\n");
			fflush (stdout);
			return 0;
		 } 
	}
 	
	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;
	int size = parLen + propLen+1; //We add one here for the resources-dir property

	if(size >0) {
           jclass objectClass = lookForClass(environi->env, "java/lang/Object");
	   jclass stringClass = lookForClass(environi->env, "java/lang/String");
	   objectArray = (*(environi->env))->NewObjectArray(environi->env, (jint)size, objectClass, 0 );
	   stringArray = (*(environi->env))->NewObjectArray(environi->env, (jint)size, stringClass, 0 );
	   if((!objectArray) || (!stringArray)) { 
		printf("Error: parameter and property arrays have some inconsistencies \n");
		fflush (stdout);		
		return 0; //false
	   }
	   int i=0;
	   for(i =0; i< parLen; i++) {
		
	     (*(environi->env))->SetObjectArrayElement(environi->env, stringArray, i, (*(environi->env))->NewStringUTF(environi->env, parameters[i].name) );
	     (*(environi->env))->SetObjectArrayElement(environi->env, objectArray, i, (jobject)(parameters[i].value) );
	
	   }

	   (*(environi->env))->SetObjectArrayElement(environi->env, stringArray, i, (*(environi->env))->NewStringUTF(environi->env,"resources"));
	     (*(environi->env))->SetObjectArrayElement(environi->env, objectArray, i, (jobject)((*(environi->env))->NewStringUTF(environi->env, getResourceDirectory())) );
	    i++;
	   int j=0;	
  	   for(; j<propLen; j++, i++) {
	     (*(environi->env))->SetObjectArrayElement(environi->env, stringArray, i, (*(environi->env))->NewStringUTF(environi->env, properties[j].name));
	     (*(environi->env))->SetObjectArrayElement(environi->env, objectArray, i, (jobject)((*(environi->env))->NewStringUTF(environi->env, properties[j].value)) );
	   }
	  
	}
      jboolean resultObj = (jboolean)((*(environi->env))->CallBooleanMethod(environi->env, cpp, bmID, (cwd== NULL ? (*(environi->env))->NewStringUTF(environi->env, "") : (*(environi->env))->NewStringUTF(environi->env, cwd)), (*(environi->env))->NewStringUTF(environi->env,xpathStr), (jobjectArray)stringArray, (jobjectArray)objectArray ));

      (*(environi->env))->DeleteLocalRef(environi->env, objectArray);
      (*(environi->env))->DeleteLocalRef(environi->env, stringArray);
      if(resultObj) {
	 checkForException(environi, cpp);    
	return resultObj;
     }

    checkForException(environi, cpp);
    return 0; //false
}

const char * getStringValue(sxnc_environment *environi, sxnc_value value){
	return stringValue(environi, value.xdmvalue);
}

int size(sxnc_environment *environi, sxnc_value val){
	
	jclass  xdmValueClass = lookForClass(environi->env, "net/sf/saxon/s9api/XdmValue");
	 if(environi->env == NULL) {
		printf("Error: Saxon-C env variable is null\n");
		fflush (stdout);
           	return 0;
	 }
	char methodName[] = "size";
    	char args[] = "()I";
		
	static jmethodID MID_xdmValue = NULL;
	if(!MID_xdmValue) {
		MID_xdmValue = (jmethodID)(*(environi->env))->GetMethodID(environi->env, xdmValueClass, methodName, args);
	}
       if (!MID_xdmValue) {
	  printf("\nError: Saxon-C %s() not found\n",methodName);
  	  fflush (stdout);
          return 0;
      }
      jint result = (jint)(*(environi->env))->CallIntMethod(environi->env,val.xdmvalue, MID_xdmValue);
          
	return result;
    
}

sxnc_value * itemAt(sxnc_environment *environi, sxnc_value val, int i){
	
	jclass  xdmValueClass = lookForClass(environi->env, "net/sf/saxon/s9api/XdmValue");
	 if(environi->env == NULL) {
		printf("Error: Saxon-C env variable is null\n");
		fflush (stdout);
           	return 0;
	 }
	char methodName[] = "itemAt";
    	char args[] = "(I)Lnet/sf/saxon/s9api/XdmItem;";
		
	static jmethodID MID_xdmValue = NULL;
	if(!MID_xdmValue) {
		MID_xdmValue = (jmethodID)(*(environi->env))->GetMethodID(environi->env, xdmValueClass, methodName, args);
	}
       if (!MID_xdmValue) {
	  printf("\nError: MyClassInDll %s() not found\n",methodName);
  	  fflush (stdout);
          return 0;
      }
      jobject xdmItemObj = (*(environi->env))->CallObjectMethod(environi->env,val.xdmvalue, MID_xdmValue, i);
      if(xdmItemObj) {   
	 sxnc_value* result = (sxnc_value *)malloc(sizeof(sxnc_value));
         result->xdmvalue = xdmItemObj;  
	
	//checkForException(environi, cppClass, cpp);     
	return result;
     }

    return 0;
}

bool isAtomicvalue(sxnc_environment *environi, sxnc_value val){
	
jclass  xdmItemClass = lookForClass(environi->env, "net/sf/saxon/s9api/XdmItem");
	 if(environi->env == NULL) {
		printf("Error: Saxon-C env variable is null\n");
		fflush (stdout);
           	return 0;
	 }
	char methodName[] = "isAtomicValue";
    	char args[] = "()Z";
		
	static jmethodID MID_xdmValue = NULL;
	if(!MID_xdmValue) {
		MID_xdmValue = (jmethodID)(*(environi->env))->GetMethodID(environi->env, xdmItemClass, methodName, args);
	}
       if (!MID_xdmValue) {
	  printf("\nError: Saxon library - %s() not found\n",methodName);
  	  fflush (stdout);
          return 0;
      }
      jboolean result = (jboolean)(*(environi->env))->CallBooleanMethod(environi->env,val.xdmvalue, MID_xdmValue);
     return (bool)result;
}

int getIntegerValue(sxnc_environment *environi, sxnc_value value,  int failureVal){
	
	const char * valueStr = getStringValue(environi, value);
	if(valueStr != NULL) {		
		int value = atoi(valueStr);
		if(value != 0) {
			return value;
		}
	} 

	if (strcmp(valueStr,"0") == 0) {
		return 0;
	} else {
		return failureVal;	
	}}

bool getBooleanValue(sxnc_environment *environi, sxnc_value value){
	
	jclass booleanClass = lookForClass(environi->env,"java/lang/Boolean");
	static jmethodID strMID = NULL;
	if(!strMID) {
		strMID = (jmethodID)(*(environi->env))->GetMethodID(environi->env, booleanClass, "booleanValue", "()Z");
		if(!strMID) {
	 		 printf("\nError: Boolean %s() not found\n","booleanValue");
  	 		 fflush (stdout);
         		 return 0;//false
		}
        }
	jboolean result = (jboolean)((*(environi->env))->CallBooleanMethod(environi->env, value.xdmvalue, strMID));
	return (bool)result;
}

long getLongValue(sxnc_environment *environi, sxnc_value value,  long failureVal){
	
	const char * valueStr = getStringValue(environi, value);
	if(valueStr != NULL) {		
		long value = atol(valueStr);
		if(value != 0) {
			return value;
		}
	} 

	if (strcmp(valueStr,"0") == 0) {
		return 0;
	} else {
		return failureVal;	
	}
}

float getFloatValue(sxnc_environment *environi, sxnc_value value,  float failureVal){
	
	jclass floatClass = lookForClass(environi->env,"java/lang/Float");
	static jmethodID strMID = NULL;
	if(!strMID) {
		strMID = (jmethodID)(*(environi->env))->GetMethodID(environi->env, floatClass, "floatValue", "()F");
		if(!strMID) {
	 		 printf("\nError: Float %s() not found\n","floatValue");
  	 		 fflush (stdout);
         		 return 0;
		}
        }
	jfloat result = (jfloat)((*(environi->env))->CallFloatMethod(environi->env, value.xdmvalue, strMID));
	if ((*(environi->env))->ExceptionCheck(environi->env)) {
		return failureVal;
	 }
	return (float)result;


}

double getDoubleValue(sxnc_environment *environi, sxnc_value value, double failureVal){
	
	const char * valueStr = getStringValue(environi, value);
	if(valueStr != NULL) {		
		double value = atof(valueStr);
		if(value != 0) {
			return value;
		}
	} 

	if (strcmp(valueStr,"0") == 0) {
		return 0;
	} else {
		return failureVal;	
	}

}

