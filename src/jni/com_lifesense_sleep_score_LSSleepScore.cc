/** 
* Copyright 2019 Liu Ziyi.
*/

#include "jni/com_lifesense_sleep_score_LSSleepScore.h"
#include <stdio.h>
#include <string>
#include "ls_sleep_rate_quality.h"
#include "JNIHelper.h"

using std::string;

string jstring2string(JNIEnv *env, jstring jStr) {
    // using std::string;

    if (!jStr) {
        return "";
    }

    const jclass stringClass = env->GetObjectClass(jStr);
    const jmethodID getBytes = env->GetMethodID(stringClass,
        "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(jStr,
        getBytes, env->NewStringUTF("UTF-8"));

    size_t length = (size_t) env->GetArrayLength(stringJbytes);
    jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

    string ret = string((char *)pBytes, length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

    env->DeleteLocalRef(stringJbytes);
    env->DeleteLocalRef(stringClass);
    return ret;
}

jobject LSSleepScoreWrapper(JNIEnv *env, jclass type, jobject obj) {
    // using std::string;

    jclass cls;

    jfieldID shallowSleepID;
    jfieldID sexID;
    jfieldID awakeningID;
    jfieldID birthdayID;
    jfieldID awakeningTimeID;
    jfieldID deepSleepID;
    jfieldID awakeningCountID;
    jfieldID heightID;
    jfieldID sleepTimeID;
    jfieldID analysisTimeID;
    jfieldID modelPathID;

    int shallowSleep;
    int sex;
    int awakening;
    string birthday;
    string awakeningTime;
    int deepSleep;
    int awakeningCount;
    int height;
    string sleepTime;
    string analysisTime;
    string modelPath;

    LSSleepInfo_t sleepInfo;
    LSSleepScore_t sleepScore;

    cls = env->GetObjectClass(obj);

    // 获取LSSleepScoreInput的成员变量
    shallowSleepID   = env->GetFieldID(cls, "shallowSleep", "I");
    sexID            = env->GetFieldID(cls, "sex", "I");
    awakeningID      = env->GetFieldID(cls, "awakening", "I");
    birthdayID       = env->GetFieldID(cls, "birthday", "Ljava/lang/String;");
    awakeningTimeID  = env->GetFieldID(cls, "awakeningTime", "Ljava/lang/String;");
    deepSleepID      = env->GetFieldID(cls, "deepSleep", "I");
    awakeningCountID = env->GetFieldID(cls, "awakeningCount", "I");
    heightID         = env->GetFieldID(cls, "height", "I");
    sleepTimeID      = env->GetFieldID(cls, "sleepTime", "Ljava/lang/String;");
    analysisTimeID   = env->GetFieldID(cls, "analysisTime", "Ljava/lang/String;");
    modelPathID      = env->GetFieldID(cls, "modelPath", "Ljava/lang/String;");

    shallowSleep   = (int) env->GetIntField(obj, shallowSleepID);
    sex            = (int) env->GetIntField(obj, sexID);
    awakening      = (int) env->GetIntField(obj, awakeningID);
    birthday       = jstring2string(env, (jstring) env->GetObjectField(obj, birthdayID));
    awakeningTime  = jstring2string(env, (jstring) env->GetObjectField(obj, awakeningTimeID));
    deepSleep      = (int) env->GetIntField(obj, deepSleepID);
    awakeningCount = (int) env->GetIntField(obj, awakeningCountID);
    height         = (int) env->GetIntField(obj, heightID);
    sleepTime      = jstring2string(env, (jstring) env->GetObjectField(obj, sleepTimeID));
    analysisTime   = jstring2string(env, (jstring) env->GetObjectField(obj, analysisTimeID));
    modelPath      = jstring2string(env, (jstring) env->GetObjectField(obj, modelPathID));

    // Assign value to algorithm input
    sleepInfo.shallowSleep   = shallowSleep;
    sleepInfo.sex            = sex;
    sleepInfo.awakening      = awakening;
    sleepInfo.birthday       = birthday;
    sleepInfo.awakeningTime  = awakeningTime;
    sleepInfo.deepSleep      = deepSleep;
    sleepInfo.awakeningCount = awakeningCount;
    sleepInfo.height         = height;
    sleepInfo.sleepTime      = sleepTime;
    sleepInfo.analysisTime   = analysisTime;
    sleepInfo.modelPath      = modelPath;

    // Call scoring function
    LSRateSleepQuality(&sleepInfo, &sleepScore);

    // 一个LSSleepScoreResult对象
    JNIObjectInfo result;
    getMethodInfo(env, &result, "com/lifesense/sleep_score/LSSleepScoreResult",
        "<init>", "()V");
    jobject resultObj  = env->NewObject(result.classID, result.methodID, "");
    jfieldID scoreID   = env->GetFieldID(result.classID, "score", "I");
    jfieldID contribID = env->GetFieldID(result.classID,
        "contrib", "Ljava/util/ArrayList;");

    // 存储LSSleepScoreContrib的数组
    JNIArrayList array = createArrayList(env);

    // 数组设置完后，插入LSSleepResult对象，同时设置score
    int i;
    for (i = 0; i < sleepScore.featLen; ++i) {
        JNIObjectInfo contrib;
        getMethodInfo(env, &contrib,
            "com/lifesense/sleep_score/LSSleepScoreContrib", "<init>", "()V");
        jobject contribObj = env->NewObject(contrib.classID,
            contrib.methodID, "");

        jfieldID valueID = env->GetFieldID(contrib.classID, "value", "F");
        jfieldID nameID  = env->GetFieldID(contrib.classID, "name",
            "Lcom/lifesense/sleep_score/LSSleepScoreName;");

        env->SetFloatField(contribObj, valueID, sleepScore.featContrib[i].value);

        // Set name
        const char *property_name = NULL;
        int index = sleepScore.featContrib[i].index;
        switch (index) {
            case 0:
                property_name = "AwakeningTime";
                break;
            case 1:
                property_name = "SleepTime";
                break;
            case 2:
                property_name = "AnalysisTime";
                break;
            case 3:
                property_name = "Awakening";
                break;
            case 4:
                property_name = "AwakeningCount";
                break;
            case 5:
                property_name = "DeepSleep";
                break;
            case 6:
                property_name = "ShallowSleep";
                break;
            case 7:
                property_name = "TotalSleepDuration";
                break;
            case 8:
                property_name = "Height";
                break;
            case 9:
                property_name = "Sex";
                break;
            case 10:
                property_name = "Age";
                break;
            case 11:
                property_name = "IntAgeSleepTime";
                break;
            case 12:
                property_name = "IntAgeShallowSleep";
                break;
            case 13:
                property_name = "IntAgeHeight";
                break;
            case 14:
                property_name = "IntSleepTimeShallowSleep";
                break;
            case 15:
                property_name = "IntShallowSleepHeight";
                break;
            case 16:
                property_name = "IntShallowTotalSleepDuration";
                break;
            default:
            printf("Error field\n");
        }

        jstring str_property_N = string2jstring(env, property_name);
        JNIObjectInfo name;
        getStaticMethodInfo(env, &name,
            "com/lifesense/sleep_score/LSSleepScoreName", "valueOf",
            "(Ljava/lang/String;)Lcom/lifesense/sleep_score/LSSleepScoreName;");
        jobject nameObj = env->CallStaticObjectMethod(name.classID,
            name.methodID, str_property_N);
        env->SetObjectField(contribObj, nameID, nameObj);

        env->CallBooleanMethod(array.objectID, array.methodID, contribObj);

        env->DeleteLocalRef(str_property_N);
        env->DeleteLocalRef(contrib.classID);
        env->DeleteLocalRef(contribObj);
        env->DeleteLocalRef(nameObj);
        env->DeleteLocalRef(name.classID);
    }

    env->SetObjectField(resultObj, contribID, array.objectID);
    env->SetIntField(resultObj, scoreID, sleepScore.score);

    env->DeleteLocalRef(array.objectID);
    env->DeleteLocalRef(array.classID);

    return resultObj;
}


JNIEXPORT jobject JNICALL Java_com_lifesense_sleep_1score_LSSleepScore_RateSleepQuality
  (JNIEnv *env, jclass type, jobject obj) {
    return LSSleepScoreWrapper(env, type, obj);
}
