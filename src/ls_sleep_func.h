/**
* Copyright [2019] <Ziyi Liu>.
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
// #include "ls_log.h"

//debug output
#define SLEEPALGO_FUNC_OUTPUT_LOG_ENABLE

#define BLOCK_LIMIT (1440)

#define CHECK_WEAR_DURATION (6)
#define MAX_DIFFERENCE      (2)

float OnePassStd(const float x, bool init);

bool WindowAverage(uint8_t *window, uint32_t windowLen);

float PredictSleepStatus(const float* features);

float FindMedian(uint8_t *nums, uint16_t numsSize);

// void RemoveNoiseMarker(uint16_t *sleepMarkerBuf, uint16_t *sleepMarkerBufLen);
uint16_t RemoveNoiseMarker(uint16_t *sleepMarkerBuf, uint16_t *sleepMarkerBufLen);

void ComputeActivity(const float *sleepDepthFeatureBuf,
    uint16_t sleepDepthFeatureBufLen, uint8_t *activityBlock);

void ComputeCount(uint8_t *activityBlock, uint16_t blockLen,
    uint8_t windowSize, uint8_t *countBlock);

int Compare(void const* a, void const* b);

int Compare_U16(void const* a, void const* b);

uint8_t GetPercentile(const uint8_t *block, uint16_t len, uint8_t percentile);

void Compute3StageDepth(const uint8_t *countBlock, const uint16_t blockLen,
    const uint16_t *sleepMarkerBuf, const uint16_t sleepMarkerBufLen,
    uint8_t* depthBlock);

uint8_t MostFrequent(const uint8_t *srcArr, uint8_t arrLen);

bool CheckWearing(uint8_t x, uint8_t y, uint8_t z, bool init);

void computePostDepth(uint8_t *postDepthBlock, uint8_t *depthBlock,
    uint8_t windowSize);

// void ComputeSleepDepth(float *sleepDepthFeatureBuf, uint32_t *sleepMarkerBuf,
//    uint32_t sleepCycleOn, uint32_t sleepCycleOff, uint32_t *sleepCycleDepth);
// uint16_t ComputeSleepDepth(const float *sleepDepthFeatBuf,
   //  const uint16_t sleepDepthFeatBufLen, const uint16_t *sleepMarkerBuf,
    // const uint16_t sleepMarkerBufLen, uint16_t *sleepCycleDepth);
uint16_t ComputeSleepDepth(const float *sleepDepthFeatBuf,
    const uint16_t sleepDepthFeatBufLen, const uint16_t *sleepMarkerBuf,
    const uint16_t sleepMarkerBufLen, uint16_t *sleepCycleDepth,
    const uint16_t sleepCycleDepthSize);

uint32_t Time2Utc(uint32_t time, uint32_t utcTime);

bool CheckWearingStepOne(uint8_t x, uint8_t y, uint8_t z, bool init);
bool CheckWearingStepTwo(uint8_t *x, uint8_t *y, uint8_t *z, uint16_t len);
bool SecondRoundCheckWear(void);

void LSSleepFuncInitialize(void);


