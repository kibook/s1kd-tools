#include "SaxonCProcessor.h"


/*
 * Get the Saxon version 
 */
const char * version(sxnc_environment environi) {


    jmethodID MID_version;
    jclass  versionClass;
     versionClass = lookForClass(environi.env, "net/sf/saxon/Version");
    char methodName[] = "getProductVersion";
    char args[] = "()Ljava/lang/String;";
    MID_version = (jmethodID)(*(environi.env))->GetStaticMethodID(environi.env, versionClass, methodName, args);
    if (!MID_version) {
	printf("\nError: MyClassInDll %s() not found\n",methodName);
	fflush (stdout);
        return NULL;
    }
   jstring jstr = (jstring)((*(environi.env))->CallStaticObjectMethod(environi.env, versionClass, MID_version));
   const char * str = (*(environi.env))->GetStringUTFChars(environi.env, jstr, NULL);
  
    //(*(environi.env))->ReleaseStringUTFChars(environi.env, jstr,str);
    return str;
}


/*
 * Get the Saxon version 
 */
const char * getProductVariantAndVersion(sxnc_environment environi) {


    jmethodID MID_version;
    jclass  versionClass;
     versionClass = lookForClass(environi.env, "net/sf/saxon/Version");
    char methodName[] = "getProductVariantAndVersion";
    char args[] = "()Ljava/lang/String;";
    MID_version = (jmethodID)(*(environi.env))->GetStaticMethodID(environi.env, versionClass, methodName, args);
    if (!MID_version) {
	printf("\nError: SaxonCDll %s() not found\n",methodName);
	fflush (stdout);
        return NULL;
    }
   jstring jstr = (jstring)((*(environi.env))->CallStaticObjectMethod(environi.env, versionClass, MID_version));
   const char * str = (*(environi.env))->GetStringUTFChars(environi.env, jstr, NULL);
  
    (*(environi.env))->ReleaseStringUTFChars(environi.env, jstr,str);
    return str;
}

void initSaxonc(sxnc_environment ** environi, sxnc_processor ** proc, sxnc_parameter **param, sxnc_property ** prop, int cap, int propCap){
    
    *param = (sxnc_parameter *)calloc(cap, sizeof(sxnc_parameter));
    *prop = (sxnc_property *)calloc(propCap, sizeof(sxnc_property));
    * environi =  (sxnc_environment *)malloc(sizeof(sxnc_environment));
    *proc = (sxnc_processor *)malloc(sizeof(sxnc_processor));
}


void freeSaxonc(sxnc_environment ** environi, sxnc_processor ** proc, sxnc_parameter **param, sxnc_property ** prop){
	free(*environi);
	free(*proc);
	free(*param);
	free(*prop);
}

void xsltSaveResultToFile(sxnc_environment environi, sxnc_processor ** proc, char * cwd, char * source, char* stylesheet, char* outputfile, sxnc_parameter *parameters, sxnc_property * properties, int parLen, int propLen) {
	jclass cppClass = lookForClass(environi.env, "net/sf/saxon/option/cpp/XsltProcessor");
	static jmethodID xsltFileID = NULL; //cache the methodID
 	
	if(!cpp) {
		cpp = (jobject) createSaxonProcessor (environi.env, cppClass, "(Z)V", NULL, (jboolean)sxn_license);
	}
 	
	if(!xsltFileID) {
	 	xsltFileID = (jmethodID)(*(environi.env))->GetMethodID (environi.env, cppClass,"transformToFile", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)V");
	 	if (!xsltFileID) {
	        	printf("Error: MyClassInDll. transformToFile not found\n");
			fflush (stdout);
			return;
	    	} 
	}
	
 	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;
	int size = parLen + propLen+1;
	
	

	if(size>0) {
           jclass objectClass = lookForClass(environi.env, "java/lang/Object");
	   jclass stringClass = lookForClass(environi.env, "java/lang/String");
	   objectArray = (*(environi.env))->NewObjectArray(environi.env, (jint)size, objectClass, 0 );
	   stringArray = (*(environi.env))->NewObjectArray(environi.env, (jint)size, stringClass, 0 );
	if(size >0) {

	   if((!objectArray) || (!stringArray)) { 
		printf("Error: parameter and property have some inconsistencies\n");
		fflush (stdout);
		return;}
	   int i=0;
	   for( i =0; i< parLen; i++) {
		
	     (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env,  parameters[i].name) );
		
	     (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)(parameters[i].value) );
	   }
 	   (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env,"resources"));
	   (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)((*(environi.env))->NewStringUTF(environi.env, getResourceDirectory())) );
	   i++;
            int j=0;
  	   for(; j< propLen; j++, i++) {
	     (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env, properties[j].name));
	     (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)((*(environi.env))->NewStringUTF(environi.env, properties[j].value)) );
	   }
	   
	}
	}

      (*(environi.env))->CallVoidMethod(environi.env, cpp, xsltFileID, (cwd== NULL ? (*(environi.env))->NewStringUTF(environi.env, "") : (*(environi.env))->NewStringUTF(environi.env, cwd)),(*(environi.env))->NewStringUTF(environi.env, source), (*(environi.env))->NewStringUTF(environi.env, stylesheet), (*(environi.env))->NewStringUTF(environi.env, outputfile), stringArray, objectArray );
	if(size>0) {    
	  (*(environi.env))->DeleteLocalRef(environi.env, objectArray);
	  (*(environi.env))->DeleteLocalRef(environi.env, stringArray);
	}
#ifdef DEBUG
	checkForException( *(environi.env), cpp);
#endif
  
}

const char * xsltApplyStylesheet(sxnc_environment environi, sxnc_processor ** proc, char * cwd, const char * source, const char* stylesheet, sxnc_parameter *parameters, sxnc_property * properties, int parLen, int propLen) {
	static jmethodID mID = NULL; //cache the methodID

	jclass cppClass = lookForClass(environi.env, "net/sf/saxon/option/cpp/XsltProcessor");

	if(!cpp) {
		cpp = (jobject) createSaxonProcessor (environi.env, cppClass, "(Z)V", NULL, (jboolean)sxn_license);
	}
#ifdef DEBUG
        jmethodID debugMID = (*(environi.env))->GetStaticMethodID(environi.env, cppClass, "setDebugMode", "(Z)V");
	(*(environi.env))->CallStaticVoidMethod(environi.env, cppClass, debugMID, (jboolean)__true);
#endif
	if(mID == NULL) {
 		mID = (jmethodID)(*(environi.env))->GetMethodID (environi.env, cppClass,"transformToString", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;");
 		if (!mID) {
 		       printf("Error: MyClassInDll. xsltApplyStylesheet not found\n");
			fflush (stdout);
			return 0;
 		 } 
	}

 	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;
	int size = parLen + propLen+1; //We add one here for the resources-dir property

	if(size >0) {
           jclass objectClass = lookForClass(environi.env, "java/lang/Object");
	   jclass stringClass = lookForClass(environi.env, "java/lang/String");
	   objectArray = (*(environi.env))->NewObjectArray(environi.env, (jint)size, objectClass, 0 );
	   stringArray = (*(environi.env))->NewObjectArray(environi.env, (jint)size, stringClass, 0 );
	   if((!objectArray) || (!stringArray)) { return NULL;}
	   int i=0;
	   for(i =0; i< parLen; i++) {
		
	     (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env, parameters[i].name) );
	     (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)(parameters[i].value) );
	
	   }

	   (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env,"resources"));
	     (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)((*(environi.env))->NewStringUTF(environi.env, getResourceDirectory())) );
	    i++;
	    int j=0;		
  	   for(; j<propLen; j++, i++) {
	     (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env, properties[j].name));
	     (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)((*(environi.env))->NewStringUTF(environi.env, properties[j].value)) );
	   }
	  
	}

      jstring result = (jstring)((*(environi.env))->CallObjectMethod(environi.env, cpp, mID, (cwd== NULL ? (*(environi.env))->NewStringUTF(environi.env, "") : (*(environi.env))->NewStringUTF(environi.env, cwd)), (*(environi.env))->NewStringUTF(environi.env, source), (*(environi.env))->NewStringUTF(environi.env, stylesheet), stringArray, objectArray ));

	
      if(result) {
        const char * str = (*(environi.env))->GetStringUTFChars(environi.env, result, NULL);    
	return str;
     }

    checkForException( *(environi.env), cpp);
    return 0;
}


void executeQueryToFile(sxnc_environment environi, sxnc_processor ** proc, char * cwd, char* outputfile, sxnc_parameter *parameters, sxnc_property * properties, int parLen, int propLen){
	static jmethodID queryFileID = NULL; //cache the methodID
	jclass cppClass = lookForClass(environi.env, "net/sf/saxon/option/cpp/XQueryEngine");
 	
	if(!cpp) {
		cpp = (jobject) createSaxonProcessor (environi.env, cppClass, "(Z)V", NULL, (jboolean)sxn_license);
	}
 	
	if(queryFileID == NULL) {	
		queryFileID = (jmethodID)(*(environi.env))->GetMethodID (environi.env, cppClass,"executeQueryToFile", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)V");
 		if (!queryFileID) {
        		printf("Error: MyClassInDll. executeQueryToString not found\n");
			fflush (stdout);
			return;
    		} 
	}
	
 	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;
	int size = parLen + propLen+1;
	
	

	if(size>0) {
           jclass objectClass = lookForClass(environi.env, "java/lang/Object");
	   jclass stringClass = lookForClass(environi.env, "java/lang/String");
	   objectArray = (*(environi.env))->NewObjectArray(environi.env, (jint)size, objectClass, 0 );
	   stringArray = (*(environi.env))->NewObjectArray(environi.env, (jint)size, stringClass, 0 );
	if(size >0) {

	   if((!objectArray) || (!stringArray)) { return;}
	   int i=0;
	   for( i =0; i< parLen; i++) {
		
	     (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env,  parameters[i].name) );
		
	     (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)(parameters[i].value) );
	   }
 	   (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env,"resources"));
	   (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)((*(environi.env))->NewStringUTF(environi.env, getResourceDirectory())) );
	   i++;
	  int j=0;
  	   for(; j<propLen; i++, j++) {
	     (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env, properties[j].name));
	     (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)((*(environi.env))->NewStringUTF(environi.env, properties[j].value)) );
	   }
	   
	}
	}
      (*(environi.env))->CallVoidMethod(environi.env,cpp, queryFileID, (cwd== NULL ? (*(environi.env))->NewStringUTF(environi.env, "") : (*(environi.env))->NewStringUTF(environi.env, cwd)), (*(environi.env))->NewStringUTF(environi.env, outputfile), stringArray, objectArray );    
	  (*(environi.env))->DeleteLocalRef(environi.env, objectArray);
	  (*(environi.env))->DeleteLocalRef(environi.env, stringArray);

}

const char * executeQueryToString(sxnc_environment environi, sxnc_processor ** proc, char * cwd, sxnc_parameter *parameters, sxnc_property * properties, int parLen, int propLen){
	static jmethodID queryStrID = NULL; //cache the methodID
	jclass cppClass = lookForClass(environi.env, "net/sf/saxon/option/cpp/XQueryEngine");

	if(!cpp) {
		cpp = (jobject) createSaxonProcessor (environi.env, cppClass, "(Z)V", NULL, (jboolean)sxn_license);
	}

	if(queryStrID == NULL) {
		queryStrID = (jmethodID)(*(environi.env))->GetMethodID (environi.env, cppClass,"executeQueryToString", "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;");
		if (!queryStrID) {
		        printf("Error: MyClassInDll. executeQueryToString not found\n");
			fflush (stdout);
			return 0;
		 } 
	}
 	
	jobjectArray stringArray = NULL;
	jobjectArray objectArray = NULL;
	int size = parLen + propLen+1; //We add one here for the resources-dir property

	if(size >0) {
           jclass objectClass = lookForClass(environi.env, "java/lang/Object");
	   jclass stringClass = lookForClass(environi.env, "java/lang/String");
	   objectArray = (*(environi.env))->NewObjectArray(environi.env, (jint)size, objectClass, 0 );
	   stringArray = (*(environi.env))->NewObjectArray(environi.env, (jint)size, stringClass, 0 );
	   if((!objectArray) || (!stringArray)) { return NULL;}
	   int i=0;
	   for(i =0; i< parLen; i++) {
		
	     (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env, parameters[i].name) );
	     (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)(parameters[i].value) );
	
	   }


	   (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env,"resources"));
	     (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)((*(environi.env))->NewStringUTF(environi.env, getResourceDirectory())) );
	    i++;
	    int j=0;	
  	   for(; j<propLen; i++, j++) {
	     (*(environi.env))->SetObjectArrayElement(environi.env, stringArray, i, (*(environi.env))->NewStringUTF(environi.env, properties[j].name));
	     (*(environi.env))->SetObjectArrayElement(environi.env, objectArray, i, (jobject)((*(environi.env))->NewStringUTF(environi.env, properties[j].value)) );
	   }
	  
	}
      jstring result = (jstring)((*(environi.env))->CallObjectMethod(environi.env, cpp, queryStrID, (cwd== NULL ? (*(environi.env))->NewStringUTF(environi.env, "") : (*(environi.env))->NewStringUTF(environi.env, cwd)), stringArray, objectArray ));

      (*(environi.env))->DeleteLocalRef(environi.env, objectArray);
      (*(environi.env))->DeleteLocalRef(environi.env, stringArray);
      if(result) {

       const char * str = (*(environi.env))->GetStringUTFChars(environi.env, result, NULL);
       //return "result should be ok";       
	//checkForException( environi, cpp);     
	return str;
     }

    checkForException( *(environi.env), cpp);
    return 0;

}







