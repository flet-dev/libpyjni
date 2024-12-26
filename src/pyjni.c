#include <pthread.h>
#include <jni.h>

#define LOG_TAG "Python_android"
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static pthread_key_t mThreadKey;
static JavaVM* mJavaVM;
static jobject mClassLoader;    // Global reference to the class loader
static jmethodID mFindClassMethod; // Method ID for ClassLoader.loadClass

JNIEnv* Android_JNI_GetEnv(void);
static void Android_JNI_ThreadDestroyed(void*);

/* Library init */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv *env;
    mJavaVM = vm;
    LOGI("JNI_OnLoad called");
    if ((*mJavaVM)->GetEnv(mJavaVM, (void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("Failed to get the environment using GetEnv()");
        return -1;
    }

    // Cache the ClassLoader and its loadClass method
    jclass activityThreadClass = (*env)->FindClass(env, "android/app/ActivityThread");
    if (!activityThreadClass) {
        LOGE("Failed to find android/app/ActivityThread class");
        return -1;
    }

    jmethodID currentApplication = (*env)->GetStaticMethodID(env, activityThreadClass, "currentApplication", "()Landroid/app/Application;");
    if (!currentApplication) {
        LOGE("Failed to find ActivityThread.currentApplication method");
        return -1;
    }

    jobject app = (*env)->CallStaticObjectMethod(env, activityThreadClass, currentApplication);
    if (!app) {
        LOGE("Failed to get the current Application instance");
        return -1;
    }

    jclass contextClass = (*env)->FindClass(env, "android/content/Context");
    if (!contextClass) {
        LOGE("Failed to find android/content/Context class");
        return -1;
    }

    jmethodID getClassLoader = (*env)->GetMethodID(env, contextClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    if (!getClassLoader) {
        LOGE("Failed to find Context.getClassLoader method");
        return -1;
    }

    jobject classLoader = (*env)->CallObjectMethod(env, app, getClassLoader);
    if (!classLoader) {
        LOGE("Failed to get the ClassLoader instance");
        return -1;
    }

    jclass classLoaderClass = (*env)->FindClass(env, "java/lang/ClassLoader");
    if (!classLoaderClass) {
        LOGE("Failed to find java/lang/ClassLoader class");
        return -1;
    }

    jmethodID loadClass = (*env)->GetMethodID(env, classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (!loadClass) {
        LOGE("Failed to find ClassLoader.loadClass method");
        return -1;
    }

    mClassLoader = (*env)->NewGlobalRef(env, classLoader);
    mFindClassMethod = loadClass;

    // Create pthread key for thread-specific JNIEnv storage
    if (pthread_key_create(&mThreadKey, Android_JNI_ThreadDestroyed) != 0) {
        LOGE("Error initializing pthread key");
    } else {
        Android_JNI_SetupThread();
    }

    return JNI_VERSION_1_4;
}

JNIEnv* Android_JNI_GetEnv(void)
{
    JNIEnv *env;
    int status = (*mJavaVM)->AttachCurrentThread(mJavaVM, &env, NULL);
    if (status < 0) {
        LOGE("failed to attach current thread");
        return 0;
    }

    pthread_setspecific(mThreadKey, (void*)env);

    return env;
}

static void Android_JNI_ThreadDestroyed(void* value)
{
    JNIEnv *env = (JNIEnv*)value;
    if (env != NULL) {
        (*mJavaVM)->DetachCurrentThread(mJavaVM);
        pthread_setspecific(mThreadKey, NULL);
    }
}

int Android_JNI_SetupThread(void)
{
    Android_JNI_GetEnv();
    return 1;
}

jclass WebView_FindClass(const char* className)
{
    JNIEnv* env = Android_JNI_GetEnv();
    if (!env || !mClassLoader || !mFindClassMethod) {
        LOGE("JNI environment or ClassLoader is not initialized");
        return NULL;
    }

    jstring jClassName = (*env)->NewStringUTF(env, className);
    if (!jClassName) {
        LOGE("Failed to allocate string for class name");
        return NULL;
    }

    jclass clazz = (jclass)(*env)->CallObjectMethod(env, mClassLoader, mFindClassMethod, jClassName);
    (*env)->DeleteLocalRef(env, jClassName);

    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        LOGE("Failed to find class: %s", className);
        return NULL;
    }

    return clazz;
}

void* WebView_AndroidGetJNIEnv()
{
    return Android_JNI_GetEnv();
}
