/**
* Copyright 2019 Liu Ziyi.
*/

#include <string>


#define NUM_FEATS (8)


/**
* @details 
* @brief    Algorithm configuration struct.
* @member    featLen            Feature length.
* @member    bottomProb         Bottom cutoff edge of prob.
* @member    topProb            Top cutoff edge of prob.
* @member    lowerPercentile    Lower percentile.
* @member    upperPercentile    Upper percentile.
* @member    featMean           Mean value array for each feature.
* @member    featStd            Standard deviation array for each feature.
* @member    monthDays          Array for Days of different months.
* @member    currentTime        Array for Cutoff date for computing age.
* @member    modelPath          Path to load model.
*/
struct LSSleepConfig_t {
    int   featLen;
    float bottomProb;
    float topProb;
    float lowerPercentile;
    float upperPercentile;
    float featMean[NUM_FEATS];
    float featStd[NUM_FEATS];
    int   monthDays[12];
    int   currentTime[3];
    std::string modelPath;
};


/**
* @details 
* @brief Input struct in algorithm interface.
* @member    shallowSleep     Shallow sleep duration.
* @member    sex              Sex (1-male, 2-female)
* @member    awakening        Awakening duration.
* @member    birthday         Birthday (e.g. "1986-09-17").
* @member    awakeningTime    Getup Time.
* @member    deepSleep        Deep sleep duration.
* @member    awakeningCount   Counts of awakening.
* @member    height           Height.
* @member    sleepTime        Sleep time.
* @member    analysisTime
* @member    modelPath        Path to load model.
*/
struct LSSleepInfo_t {
    int         shallowSleep;
    int         sex;
    int         awakening;
    std::string birthday;
    std::string awakeningTime;
    int         deepSleep;
    int         awakeningCount;
    float       height;
    std::string sleepTime;
    std::string analysisTime;
    std::string modelPath;
};


/**
* @details 
* @brief Output struct in algorithm interface.
* @member    score           Score for one sleep record.
* @member    feaLen          Number of sorted features.
* @member    margin
* @member    *featContrib    Pointer to hash table of feature importances.
*/
struct LSSleepScore_t {
    int    score;
    int    featLen;
    float  margin;
    struct LSSleepFeatContrib_t *featContrib;
};


/**
* @details 
* @brief Hash table for sorting feature contributions.
* @member    index
* @member    value
*/
struct LSSleepFeatContrib_t {
    int   index;
    float value;
};


/**
* @details 
* @brief Algorithm interface.
*/
void LSRateSleepQuality(LSSleepInfo_t *sleepInfo, LSSleepScore_t *sleepScore);
