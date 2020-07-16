#include "SaxonCGlue.h"

jobject cpp;
char * dllname;
char * resources_dir;
int jvmCreated = 0;
char * tempDllname =
#if defined (__linux__)
"/libsaxonhec.so";
#elif  defined (__APPLE__) && defined(__MACH__)
"/libsaxonhec.dylib";
#else
"\\libsaxonhec.dll";
#endif


char * tempResources_dir =
#ifdef __linux__
"/saxon-data";
#elif  defined (__APPLE__) && defined(__MACH__)
"/saxon-data";
#else
"\\saxon-data";
#endif

char * dllPath =
#if defined (__linux__)
"/usr/lib";
#elif  defined (__APPLE__) && defined(__MACH__)
"/usr/local/lib";
#else
"C:\\Program Files\\Saxonica\\SaxonHEC1.2.1";
#endif

/*
* Set Dll name. Also set the saxon resources directory.
* If the SAXONC_HOME sxnc_environmental variable is set then use that as base.
*/
void setDllname() {
	size_t name_len = strlen(tempDllname);
	size_t rDir_len = strlen(tempResources_dir);
	char * env = getenv("SAXONC_HOME");
	size_t env_len;
	if (env != NULL) {


		env_len = strlen(env);
		int bufSize = name_len + env_len + 1;
		int rbufSize = rDir_len + env_len + 1;
		dllname = malloc(sizeof(char)*bufSize);
		resources_dir = malloc(sizeof(char)*rbufSize);
		snprintf(dllname, bufSize, "%s%s", env, tempDllname);
		snprintf(resources_dir, rbufSize, "%s%s", env, tempResources_dir);

#ifdef DEBUG	

		printf("envDir: %s\n", env);


#endif

	}
	else {
		env_len = strlen(dllPath);
		int bufSize = name_len + env_len + 1;
		int rbufSize = rDir_len + env_len + 1;
		dllname = malloc(sizeof(char)*bufSize);
		resources_dir = malloc(sizeof(char)*rbufSize);

#ifdef DEBUG
		if (dllname == NULL || resources_dir == NULL)
		{
			// error
			printf("Error in allocation of Dllname\n");
		}
#endif
		if (snprintf(dllname, bufSize, "%s%s", dllPath, tempDllname) >= bufSize) {
			bufSize *= 2;
			free(dllname);
			dllname = malloc(sizeof(char)*bufSize);
			if (snprintf(dllname, bufSize, "%s%s", dllPath, tempDllname) >= bufSize) {
				printf("Saxon/C Error: Unable to allocate space for dllname and path");
				exit(1);
			}
		}
		if (snprintf(resources_dir, rbufSize, "%s%s", dllPath, tempResources_dir) >= rbufSize) {
			printf("Saxon/C warning: Unable to allocate space for resources directory");

		}
	}


#ifdef DEBUG

	printf("Library length: %i\n", name_len);
	printf("Env length: %i\n", env_len);
	printf("size of dllname %i\n", strlen(dllname));
	printf("dllName: %s\n", dllname);
	printf("resources_dir: %s\n", resources_dir);
	printf("size of resources dir %i\n", strlen(resources_dir));
#endif

}

char * getDllname() {
	return dllname;
}

char * getResourceDirectory() {
	return resources_dir;
}

char * _getResourceDirectory() {
	return resources_dir;
}

/*
 * Load dll using the default setting in Saxon/C
 * Recommended method to use to load library
 */
HANDLE loadDefaultDll() {
	return loadDll(NULL);
}


/*
 * Load dll.
 */
HANDLE loadDll(char* name)
{
	if (name == NULL) {
		setDllname();
		name = getDllname();
		//perror("Error1: ");
	}

#if !(defined (__linux__) || (defined (__APPLE__) && defined(__MACH__)))
	HANDLE hDll = LoadLibrary(name); // Used for windows only
#else
	HANDLE hDll = LoadLibrary(name);
#endif
	if (!hDll) {
		fprintf(stderr, "Unable to load %s\n", name);
		perror("Error: ");
		exit(1);
	}
#ifdef DEBUG
	fprintf(stderr, "%s loaded\n", name);
#endif

	return hDll;
}



jint(JNICALL * JNI_GetDefaultJavaVMInitArgs_func) (void *args);
jint(JNICALL * JNI_CreateJavaVM_func) (JavaVM **pvm, void **penv, void *args);



void initDefaultJavaRT(sxnc_environment *env) {
	sxnc_environment *environi = env;
	initJavaRT((environi->myDllHandle), &(environi->jvm), &(environi->env));
}

/*
 * Initialize JET run-time.
 */
void initJavaRT(HANDLE myDllHandle, JavaVM** pjvm, JNIEnv** penv)
{
	if (jvmCreated == 0) {
		jvmCreated = 1;
#ifdef DEBUG
		perror("initJavaRT - load Saxon/C library\n");
#endif
		int result;
		JavaVMInitArgs args;
		JNI_GetDefaultJavaVMInitArgs_func =
			(jint(JNICALL *) (void *args))
#if !(defined (__linux__) || (defined (__APPLE__) && defined(__MACH__)))
			GetProcAddress((HMODULE)myDllHandle, "JNI_GetDefaultJavaVMInitArgs");
#else
			GetProcAddress(myDllHandle, "JNI_GetDefaultJavaVMInitArgs");
#endif


		JNI_CreateJavaVM_func =
			(jint(JNICALL *) (JavaVM **pvm, void **penv, void *args))

#if !(defined (__linux__) || (defined (__APPLE__) && defined(__MACH__)))
			GetProcAddress((HMODULE)myDllHandle, "JNI_CreateJavaVM");
#else
			GetProcAddress(myDllHandle, "JNI_CreateJavaVM");

#endif


		if (!JNI_GetDefaultJavaVMInitArgs_func) {
			fprintf(stderr, "%s doesn't contain public JNI_GetDefaultJavaVMInitArgs\n", getDllname());
			exit(1);
		}

		if (!JNI_CreateJavaVM_func) {
			fprintf(stderr, "%s doesn't contain public JNI_CreateJavaVM\n", getDllname());
			exit(1);
		}

		memset(&args, 0, sizeof(args));

		args.version = JNI_VERSION_1_2;
		result = JNI_GetDefaultJavaVMInitArgs_func(&args);
		if (result != JNI_OK) {
			fprintf(stderr, "JNI_GetDefaultJavaVMInitArgs() failed with result\n");
			exit(1);
		}

		/*
		 * NOTE: no JVM is actually created
		 * this call to JNI_CreateJavaVM is intended for JET RT initialization
		 */
		result = JNI_CreateJavaVM_func(pjvm, (void **)penv, &args);
		if (result != JNI_OK) {
			fprintf(stderr, "JNI_CreateJavaVM() failed with result: %i\n", result);
			exit(1);
		}


		fflush(stdout);
	}
	else {
#ifdef DEBUG
		perror("initJavaRT - Saxon/C library loaded already\n");
#endif
	}
}


/*
 * Look for class.
 */
jclass lookForClass(JNIEnv* penv, const char* name)
{
	jclass clazz = (*penv)->FindClass(penv, name);

	if (!clazz) {
		printf("Unable to find class %s\n", name);
		return NULL;
	}
#ifdef DEBUG
	printf("Class %s found\n", name);
	fflush(stdout);
#endif

	return clazz;
}


/*
 * Create an object and invoke the instance method
 */
void invokeInstanceMethod(JNIEnv* penv, jclass myClassInDll, char * name, char * arguments)
{
	jmethodID MID_init, MID_name;
	jobject obj;

	MID_init = (*penv)->GetMethodID(penv, myClassInDll, "<init>", "()V");
	if (!MID_init) {
		printf("Error: MyClassInDll.<init>() not found\n");
		return;
	}

	obj = (*penv)->NewObject(penv, myClassInDll, MID_init);
	if (!obj) {
		printf("Error: failed to allocate an object\n");
		return;
	}

	MID_name = (*penv)->GetMethodID(penv, myClassInDll, name, arguments);

	if (!MID_name) {
		printf("Error: %s not found\n", name);
		return;
	}

	(*(penv))->CallVoidMethod(penv, obj, MID_name);
}




/*
 * Invoke the static method
 */
void invokeStaticMethod(JNIEnv* penv, jclass myClassInDll, char* name, char* arguments)
{
	jmethodID MID_name;

	MID_name = (*penv)->GetStaticMethodID(penv, myClassInDll, name, arguments);
	if (!MID_name) {
		printf("\nError: method %s not found\n", name);
		return;
	}

	(*penv)->CallStaticVoidMethod(penv, myClassInDll, MID_name);
}


/*
 * Find a constructor with a set arguments
 */
jmethodID findConstructor(JNIEnv* penv, jclass myClassInDll, char* arguments)
{
	jmethodID MID_initj;

	MID_initj = (jmethodID)(*penv)->GetMethodID(penv, myClassInDll, "<init>", arguments);
	if (!MID_initj) {
		printf("Error: MyClassInDll.<init>() not found\n");
		fflush(stdout);
		return 0;
	}

	return MID_initj;
}

/*
 * Create the Java SaxonProcessor
 */
jobject createSaxonProcessor(JNIEnv* penv, jclass myClassInDll, const char * arguments, jobject argument1, jboolean license)
{
	jmethodID MID_initi;

	jobject obj;

	MID_initi = (jmethodID)(*(penv))->GetMethodID(penv, myClassInDll, "<init>", arguments);
	if (!MID_initi) {
		printf("Error: MyClassInDll.<init>() not found\n");
		return NULL;
	}

	if (argument1) {

		obj = (jobject)(*(penv))->NewObject(penv, myClassInDll, MID_initi, argument1, license);
	}
	else {

		obj = (jobject)(*(penv))->NewObject(penv, myClassInDll, MID_initi, license);
	}
	if (!obj) {
		printf("Error: failed to allocate an object\n");
		return NULL;
	}
	return obj;
}


/*
 * Create the Java SaxonProcessor without boolean license argument
 */
jobject createSaxonProcessor2(JNIEnv* penv, jclass myClassInDll, const char * arguments, jobject argument1)
{
	jmethodID MID_initi;

	jobject obj;

	MID_initi = (jmethodID)(*(penv))->GetMethodID(penv, myClassInDll, "<init>", arguments);
	if (!MID_initi) {
		printf("Error: MyClassInDll.<init>() not found\n");
		return NULL;
	}

	if (argument1) {

		obj = (jobject)(*(penv))->NewObject(penv, myClassInDll, MID_initi, argument1);
	}
	else {

		obj = (jobject)(*(penv))->NewObject(penv, myClassInDll, MID_initi);
	}
	if (!obj) {
		printf("Error: failed to allocate an object\n");
		return NULL;
	}
	return obj;
}




/*
 * Callback to check for exceptions. When called it returns the exception as a string
 */
const char * checkForException(sxnc_environment *environii, jobject callingObject) {
	if (callingObject) {} // TODO: Remove the callingObject variable
	if ((*(environii->env))->ExceptionCheck(environii->env)) {

		jthrowable exc = (*(environii->env))->ExceptionOccurred(environii->env);
#ifdef DEBUG
		(*(environii->env))->ExceptionDescribe(environii->env);
#endif
		if (exc) {
			printf("Exception Occurred!");
		}
		jclass exccls = (jclass)(*(environii->env))->GetObjectClass(environii->env, exc);

		jclass clscls = (jclass)(*(environii->env))->FindClass(environii->env, "java/lang/Class");

		jmethodID getName = (jmethodID)(*(environii->env))->GetMethodID(environii->env, clscls, "getName", "()Ljava/lang/String;");
		jstring name = (jstring)((*(environii->env))->CallObjectMethod(environii->env, exccls, getName));
		char const* utfName = (*(environii->env))->GetStringUTFChars(environii->env, name, NULL);

		//if(callingObject != NULL && strcmp(utfName, "net.sf.saxon.s9api.SaxonApiException") == 0){

		jmethodID  getMessage = (jmethodID)(*(environii->env))->GetMethodID(environii->env, exccls, "getMessage", "()Ljava/lang/String;");

		if (getMessage) {

			jstring message = (jstring)((*(environii->env))->CallObjectMethod(environii->env, exc, getMessage));

			if (!message) {

				(*(environii->env))->ExceptionClear(environii->env);
				return 0;
			}
			char const* utfMessage = (*(environii->env))->GetStringUTFChars(environii->env, message, NULL);

			if (utfMessage != NULL) {
				(*(environii->env))->ReleaseStringUTFChars(environii->env, name, utfName);
			}

			(*(environii->env))->ExceptionClear(environii->env);
			return utfMessage;
		}
		// }
		(*(environii->env))->ReleaseStringUTFChars(environii->env, name, utfName);
	}
	//(*(environii->env))->ExceptionClear(environii->env);
	return 0;
}


/*
 * Clean up and destroy Java VM to release memory used.
 */
void finalizeJavaRT(JavaVM* jvm)
{

	if (jvmCreated != 0) {
		(*jvm)->DestroyJavaVM(jvm);
		jvmCreated = 0;
	}
}


/*
 * Get a parameter from list
 */
jobject getParameter(sxnc_parameter *parameters, int parLen, const char* namespacei, const char * name) {
	int i = 0;
	namespacei = NULL; // variable not used yet
	if (namespacei == NULL) {} // avoiding warning. In next release fix this
	for (i = 0; i < parLen; i++) {
		if (strcmp(parameters[i].name, name) == 0)
			return (jobject)parameters[i].value;
	}
	return NULL;
}


/*
 * Get a property from list
 */
char* getProperty(sxnc_property * properties, int propLen, const char* namespacei, const char * name) {
	int i = 0;
	namespacei = NULL; // variable not used yet
	if (namespacei == NULL) {} // avoiding warning. In next release fix this
	for (i = 0; i < propLen; i++) {
		if (strcmp(properties[i].name, name) == 0)
			return properties[i].value;
	}
	return 0;
}


/*
 * set a parameter
 */
void setParameter(sxnc_parameter **parameters, int * parLen, int * parCap, const char * namespacei, const char * name, jobject value) {

	namespacei = NULL;
	if (getParameter(*parameters, (*parLen), "", name) != 0) {
		return;
	}
	(*parLen)++;
	if ((*parLen) >= (*parCap)) {
		(*parCap) *= 2;
		sxnc_parameter* temp = (sxnc_parameter*)malloc(sizeof(sxnc_parameter)*(*parCap));
		int i = 0;
		for (i = 0; i < (*parLen) - 1; i++) {
			temp[i] = (*parameters)[i];
		}
		free(parameters);
		parameters = &temp;
	}
	int nameLen = strlen(name) + 7;
	char *newName = malloc(sizeof(char)*nameLen);
	int namespaceLen = strlen(namespacei);
	char *newNamespace = malloc(sizeof(char)*namespaceLen);
	snprintf(newName, nameLen, "%s%s", "param:", name);
	snprintf(newNamespace, namespaceLen, "%s", name);
	(*parameters)[(*parLen) - 1].name = (char*)newName;
	(*parameters)[(*parLen) - 1].namespacei = (char*)newNamespace;
	(*parameters)[(*parLen) - 1].value = (jobject)value;
}


/*
 * set a property
 */
void setProperty(sxnc_property ** properties, int *propLen, int *propCap, const char* name, const char* value) {
	if (getProperty(*properties, (*propLen), "", name) != 0) {
		return;
	}

	if (((*propLen) + 1) >= (*propCap)) {
		(*propCap) *= 2;
		sxnc_property* temp = (sxnc_property*)malloc(sizeof(sxnc_property)*  (*propCap));
		int i = 0;
		for (i = 0; i < (*propLen) - 1; i++) {
			temp[i] = (*properties)[i];
		}
		free(properties);
		properties = &temp;

	}
	int nameLen = strlen(name) + 1;
	char *newName = (char*)malloc(sizeof(char)*nameLen);
	snprintf(newName, nameLen, "%s", name);
	char *newValue = (char*)malloc(sizeof(char)*strlen(value) + 1);
	snprintf(newValue, strlen(value) + 1, "%s", value);
	(*properties)[(*propLen)].name = (char*)newName;
	(*properties)[(*propLen)].value = (char*)newValue;
	(*propLen)++;
}

/*
 * clear parameter
 */
void clearSettings(sxnc_parameter **parameters, int *parLen, sxnc_property ** properties, int *propLen) {
	free(*parameters);
	free(*parameters);


	*parameters = (sxnc_parameter *)calloc(10, sizeof(sxnc_parameter));
	*properties = (sxnc_property *)calloc(10, sizeof(sxnc_property));
	(*parLen) = 0;
	(*propLen) = 0;
}


const char * stringValue(sxnc_environment *environi, jobject value) {
	jclass  objClass = lookForClass(environi->env, "java/lang/Object");
	static jmethodID strMID = NULL;
	if (!strMID) {
		strMID = (jmethodID)(*(environi->env))->GetMethodID(environi->env, objClass, "toString", "()Ljava/lang/String;");
		if (!strMID) {
			printf("\nError: Object %s() not found\n", "toString");
			fflush(stdout);
			return NULL;
		}
	}
	jstring result = (jstring)((*(environi->env))->CallObjectMethod(environi->env, value, strMID));
	if (result) {
		const char * str = (*(environi->env))->GetStringUTFChars(environi->env, result, NULL);
		return str;
	}

	//checkForException(environ, cpp);
	return 0;

}




