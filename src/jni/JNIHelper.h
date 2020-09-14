/** 
* Copyright 2019 Liu Ziyi.
*/

#ifndef LSSLEEPANALYZE_JNIHELPER_H
#define LSSLEEPANALYZE_JNIHELPER_H

#include <jni.h>

typedef struct JNIObjectInfo_ {
    jclass classID;
    jmethodID methodID;
} JNIObjectInfo;


typedef struct JNIArrayList_ {
    jclass classID;
    jobject objectID;
    jmethodID methodID;
} JNIArrayList;


bool getStaticMethodInfo(JNIEnv *env, JNIObjectInfo *methodinfo,
                         const char *className,
                         const char *methodName,
                         const char *paramCode);

bool getMethodInfo(JNIEnv *env, JNIObjectInfo *methodinfo,
                   const char *className,
                   const char *methodName,
                   const char *paramCode);

JNIArrayList createArrayList(JNIEnv *env);

// void releaseArrayList(JNIArrayList &arrayList);

jfieldID getFieldId(JNIEnv *env, jclass clsId, const char *property, const char *sig);

jstring string2jstring(JNIEnv *env, const char *pat);

#endif  // LSSLEEPANALYZE_JNIHELPER_H
