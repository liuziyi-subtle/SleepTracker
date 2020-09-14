/** 
* Copyright 2019 Liu Ziyi.
*/

#include "jni/JNIHelper.h"
#include <jni.h>
#include <stdlib.h>
#include <string.h>
#if defined(ANDROID) || defined(__ANDROID__)
#include <android/log.h>
#define  LOG_TAG    "LSSleepAnalyze"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define  LOGD(...)
#define  LOGE(...)
#endif

static jclass getClassID_(JNIEnv *env, const char *className) {
    jclass ret = 0;

    do {
        ret = env->FindClass(className);
        if (!ret) {
            LOGD("Failed to find class of %s", className);
            break;
        }
    } while (0);

    return ret;
}

bool getStaticMethodInfo(JNIEnv *env, JNIObjectInfo *methodinfo,
                         const char *className,
                         const char *methodName,
                         const char *paramCode) {
    if ((NULL == className) ||
        (NULL == methodName) ||
        (NULL == paramCode)) {
        return false;
    }

    jclass classID = getClassID_(env, className);
    if (!classID) {
        LOGE("Failed to find class %s", className);
        env->ExceptionClear();
        return false;
    }

    jmethodID methodID = env->GetStaticMethodID(classID, methodName, paramCode);
    if (!methodID) {
        LOGE("Failed to find static method id of %s", methodName);
        env->ExceptionClear();
        return false;
    }

    methodinfo->classID = classID;
    methodinfo->methodID = methodID;
    return true;
}

bool getMethodInfo(JNIEnv *env, JNIObjectInfo *methodinfo,
                   const char *className,
                   const char *methodName,
                   const char *paramCode) {
    if ((NULL == className) ||
        (NULL == methodName) ||
        (NULL == paramCode)) {
        return false;
    }

    jclass classID = getClassID_(env, className);
    if (!classID) {
        LOGE("Failed to find class %s", className);
        env->ExceptionClear();
        return false;
    }

    jmethodID methodID = env->GetMethodID(classID, methodName, paramCode);
    if (!methodID) {
        LOGE("Failed to find method id of %s", methodName);
        env->ExceptionClear();
        return false;
    }

    methodinfo->classID = classID;
    methodinfo->methodID = methodID;

    return true;
}



JNIArrayList createArrayList(JNIEnv *env) {
    JNIObjectInfo methodInfo;
    getMethodInfo(env, &methodInfo, "java/util/ArrayList", "<init>", "()V");
    jobject arr = env->NewObject(methodInfo.classID, methodInfo.methodID, "");
    jmethodID addMid = env->GetMethodID(methodInfo.classID, "add",
                                        "(Ljava/lang/Object;)Z");
    JNIArrayList list;
    list.methodID = addMid;
    list.classID = methodInfo.classID;
    list.objectID = arr;
    return list;
}

// void releaseArrayList(JNIEnv *env, JNIArrayList &arrayList) {
//     env->DeleteLocalRef(arrayList.classID);
// }


jfieldID getFieldId(JNIEnv *env, jclass clsId, const char *property, const char *sig) {
    return env->GetFieldID(clsId, property, sig);
}


jstring string2jstring(JNIEnv *env, const char *pat) {
    JNIObjectInfo methodInfo;
    getMethodInfo(env, &methodInfo, "java/lang/String", "<init>",
                  "([BLjava/lang/String;)V");
    // 建立byte数组
    jbyteArray bytes = env->NewByteArray(strlen(pat));
    // 将char*转换为byte数组
    env->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte *) pat);
    // 设置String, 保存语言类型,用于byte数组转换至String时的参数
    jstring encoding = env->NewStringUTF("utf-8");
    // 将byte数组转换为java String,并输出
    jstring ret = (jstring)env->NewObject(methodInfo.classID, methodInfo.methodID, bytes,
                                          encoding);
    env->DeleteLocalRef(encoding);
    return ret;
}
