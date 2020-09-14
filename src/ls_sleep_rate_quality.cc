/**
* Copyright 2019 Liu Ziyi.
*/


#include "ls_sleep_rate_quality.h"
#include <math.h>
#include <cstdlib>
#include "xgboost/include/xgboost/c_api.h"


/**
* @details
* @brief Configuration function of the sleep quality rate algorithm.
* @param[in]    modelPath    Path to load model.
* @return       *sleepConfig Pointer to struct defining configuration info.
*/
static struct LSSleepConfig_t* GetSleepConfig(std::string modelPath) {
    static struct LSSleepConfig_t sleepConfig = {
        8,
        0.17,
        0.85,
        0.38,
        0.77,
        {
            7.13847447,   15.49713309,  6.75770955,   1.02496104,
            129.93384121, 316.30233162, 446.23617283, 5144.93325141
        },
        {
            1.24841656,  10.27034153,  9.33116605,    1.25108301,
            47.23802568, 67.32815001,  79.56401853,   3617.09954227
        },
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        { 2017, 1, 1 },
        modelPath
    };

    return &sleepConfig;
}


/**
* @details
* @brief Function for the count of leap years.
* @param[in]    year.
* @param[in]    month.
* @return       count of leap years.
*/
static inline int CountLeapYears(int year, int month) {
    // Check if the current year needs to be considered for the count of
    // leap years or not.
    if (month <= 2) {
        year--;
    }

    // An year is a leap year if it is a multiple of 4, multiple of 400
    // and not a multiple of 100.
    return year / 4 - year / 100 + year / 400;
}


static float GetDifference(int (&start_time)[3], int (&current_time)[3],
    int (&monthDays)[12]) {
    // initialize count using years and day
    long int n1 = start_time[0] * 365 + start_time[2];

    // Add days for months in given date
    for (int i = 0; i < start_time[1] - 1; i++) {
        n1 += monthDays[i];
    }

    // Since every leap year is of 366 days, add a day for every leap year
    n1 += CountLeapYears(start_time[0], start_time[1]);

    // SIMILARLY, current_time
    long int n2 = current_time[0] * 365 + current_time[2];
    for (int i=0; i < current_time[1] - 1; i++) {
        n2 += monthDays[i];
    }

    n2 += CountLeapYears(current_time[0], current_time[1]);

    // return difference between two counts
    return float(n2 - n1) / 365;
}


/**
* @details
* @brief Function for wrapping sleep information into format required by mdoel.
* @param[in]         *sleepInfo      Sleep record.
* @param[in]         *sleepConfig    Algorithm configuration.
* @param[in, out]    *features       The specified format required by model.
* @return            none.
*/
static void ExtractFeatures(LSSleepInfo_t *sleepInfo,
    LSSleepConfig_t *sleepConfig, float *features) {
    // using std::sscanf;

    int year, month, day, hour, minute, second;

    // 1. awakening_time_hm2
    std::sscanf(sleepInfo->awakeningTime.c_str(),
        "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    features[0] = float(hour + minute / 60.0);

    // 2. sleep_time_hm2
    std::sscanf(sleepInfo->sleepTime.c_str(),
        "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    features[1] = hour + minute / 60.0;

    // 3. analysis_time2
    // std::sscanf(sleepInfo->analysisTime.c_str(),
    //    "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    // features[2] = hour + minute / 60.0;

    // 4. awakening
    features[2] = sleepInfo->awakening;

    // 5. awakening_count
    features[3] = sleepInfo->awakeningCount;

    // 6. deep_sleep
    features[4] = sleepInfo->deepSleep;

    // 7. shallow_sleep
    features[5] = sleepInfo->shallowSleep;

    // 8. total_sleep_duration
    features[6] = sleepInfo->shallowSleep + sleepInfo->deepSleep;

    // 9. height
    // features[8] = sleepInfo->height;

    // 10. sex
    // features[9] = sleepInfo->sex;

    // 11. Age
    // std::sscanf(sleepInfo->birthday.c_str(), "%d-%d-%d", &year, &month, &day);
    // int startTime[3] = {year, month, day};
    // features[10] = GetDifference(startTime, sleepConfig->currentTime,
    //    sleepConfig->monthDays);

    // 12. INT_age_sleeptime
    // features[11] = features[10] * features[1];

    // 13. INT_age_shallowsleep
    // features[12] = features[10] * features[6];

    // 14. INT_age_height
    // features[13] = features[10] * features[8];

     // 15. INT_sleeptime_shallowsleep
    // features[14] = features[1]  * features[6];
    features[7] = features[1]  * features[5];

    // 16. INT_shallowsleep_height
    // features[15] = features[8] *  features[6];

    // 17. INT_shallow2tot
    // features[16] = features[6] /  features[7];

    // Standardization
    for (size_t i = 0; i < sleepConfig->featLen; ++i) {
        features[i] = ((features[i] - sleepConfig->featMean[i])
            / sleepConfig->featStd[i]);
    }

    // TODO(Liu Ziyi): Next we need to define a general feature format as the
    // input to different models and algorithms, thus a wrapper function might
    // be required.
}



/**
* @details
* @brief Function for scoring sleep record.
* @param[in]    *prob              Output probability of model.
* @param[in]    bottomProb         Bottom cutoff edge of prob.
* @param[in]    topProb            Top cutoff edge of prob.
* @param[in]    lowerPercentile    Lower percentile.
* @param[in]    upperPercentile    Upper percentile.
* @return       x                  score.
*/
static inline int Score(const float *prob, float bottomProb, float topProb,
                        float lowerPercentile, float upperPercentile) {
    float x;

    // Check whether prob beyond [bottomProb, topProb]
    if (*prob <= bottomProb) {
        x = bottomProb + 0.001;
    } else if (*prob >= topProb) {
        x = topProb - 0.001;
    } else {
        x = *prob;
    }

    // Scale x to different range
    if (x < lowerPercentile) {
        x = (x - bottomProb) / (lowerPercentile - bottomProb) * 20 + 40;
    } else if ((x >= lowerPercentile) && (x < upperPercentile)) {
        x = (x - lowerPercentile) / (upperPercentile - lowerPercentile) * 20 + 60;
    } else {
        x = (x - upperPercentile) / (topProb - upperPercentile) * 20 + 80;
    }

    return round(x);
}


static inline int CompareFeatContrib(void const* a, void const* b) {
    float arg1 = (static_cast<const struct LSSleepFeatContrib_t*> (a))->value;
    float arg2 = (static_cast<const struct LSSleepFeatContrib_t*> (b))->value;

    if (arg1 < arg2) {
        return -1;
    }

    if (arg1 > arg2) {
        return 1;
    }

    return 0;
}


/**
* @details
* @brief Function for outputing score of sleep record and contributions of the
         different features.
* @param[in]    *features       The specified format required by model.
* @param[in]    *sleepInfo      Sleep record.
* @param[in]    *sleepConfig    Algorithm configuration.
* @param[in]    *sleepScore     score and feature contributions.
* @return       none.
*/
static void Predict(const float *features, const LSSleepInfo_t *sleepInfo,
    const LSSleepConfig_t *sleepConfig, LSSleepScore_t *sleepScore) {
    // XGBoost parameters
    DMatrixHandle hData;
    int numSamples = 1;
    XGDMatrixCreateFromMat(features, numSamples, sleepConfig->featLen,
        -1, &hData);
    bst_ulong out_len;
    const float *f;
    int optionMask;
    int i;

    // Load model
    BoosterHandle hBooster;
    XGBoosterCreate(0, 0, &hBooster);
    XGBoosterLoadModel(hBooster, sleepConfig->modelPath.c_str());

    // Compute score
    optionMask = 0;
    XGBoosterPredict(hBooster, hData, optionMask, 0, &out_len, &f);
    sleepScore->score = Score(&f[0], sleepConfig->bottomProb,
        sleepConfig->topProb,
        sleepConfig->lowerPercentile,
        sleepConfig->upperPercentile);

    // Compute feature contributions
    optionMask = 4;
    XGBoosterPredict(hBooster, hData, optionMask, 0, &out_len, &f);
    // Sort feature contribs
    sleepScore->featContrib = static_cast<LSSleepFeatContrib_t*>(malloc(
        sleepConfig->featLen * sizeof(LSSleepFeatContrib_t)));
    for (i = 0; i < sleepConfig->featLen; ++i) {
        sleepScore->featContrib[i].index = i;

        // Patch #1: Ignore some negtive values which are very close to 0,
        // such as 0.0005.
        if ((f[i] > -0.001) && (f[i] < 0)) {
            sleepScore->featContrib[i].value = .0;
        } else {
            sleepScore->featContrib[i].value = f[i];
        }

        // Patch #2: If awakeningCount > 0, then make its contribution
        // non-positive. So does awakening.
        if ((i == 2) || (i == 3)) {
            if (sleepInfo->awakeningCount > 0) {
                sleepScore->featContrib[i].value = -fabs(f[i]);
            } else {
                sleepScore->featContrib[i].value = fabs(f[i]);
            }
        }

        // Patch #3: If contributions of sleepTime and totalDuration are
        // simultaneously positive, then make that of awakeningTime (getup)
        // positive.
        if (i == 0) {
            if (f[1] >= .0 && f[6] >= .0) {
                sleepScore->featContrib[i].value = fabs(f[i]);
            }
        }
    }

    qsort(sleepScore->featContrib, sleepConfig->featLen,
        sizeof(struct LSSleepFeatContrib_t), CompareFeatContrib);
    sleepScore->featLen = sleepConfig->featLen;
    sleepScore->margin  = f[i];

    // Free
    XGDMatrixFree(hData);
    XGBoosterFree(hBooster);
}


/**
* @details
* @brief External interface.
* @param[in]    *sleepInfo      Sleep record.
* @param[in]    *sleepScore     score and feature contributions.
* @return       none.
*/
void LSRateSleepQuality(LSSleepInfo_t *sleepInfo, LSSleepScore_t *sleepScore) {
    // get algorithm configurations
    LSSleepConfig_t *sleepConfig = GetSleepConfig(sleepInfo->modelPath);

    // float features[sleepConfig->featLen];
    float *features = (float*) malloc(sizeof(float) * sleepConfig->featLen);

    // Extract features
    ExtractFeatures(sleepInfo, sleepConfig, features);

    // Get sleep score and the corresponding feat contribs
    Predict(features, sleepInfo, sleepConfig, sleepScore);
}

// TODO(Liu Ziyi): check input error

// TODO(Liu Ziyi): free model and other mems

// TODO(Liu Ziyi): check whether model file exists
