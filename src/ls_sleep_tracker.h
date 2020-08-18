#ifdef GLOBAL_SLEEP_ALGO_OPEN
/**
 * Copyright [2019] <Ziyi Liu>.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
// #include "ls_log.h"

#ifndef _LS_SLEEP_TRACKER_H_
#define _LS_SLEEP_TRACKER_H_

#define SLEEPALGO_DEF_OUTPUT_LOG_ENABLExx

#define STD_DURATION (3)
// Length of buffer storing component acceleration.
#define COMP_ACC_BUF_LEN (3 * (STD_DURATION + 1))
// Length of buffer storing resultant acceleration.
#define RES_ACC_BUF_LEN (2 * STD_DURATION + 1)
// Length of buffer storing heart rate.
#define HR_BUF_LEN (STD_DURATION + 1)
// Length of wear
#define WEAR_INDICATOR_BUF_LEN (STD_DURATION + 1)
// Size of window averaging sleep status.
#define SLEEP_STATUS_AVG_WINDOW_LEN (13)
// Size of window counting sleep activities.
#define COUNT_AVG_WINDOW_LEN (13)
// Limit of recorded sleep duration, 1440 = 24h * 60.
#define MAX_SLEEP_RECORD_LEN (1440)
// Limit of recorded sleep markers, 100 means 50 pairs of [D_ASLEEP, D_AWAKE].
#define MAX_SLEEP_MARKER_LEN (100)
// Limit of sleep segments recorded in a sleep cycle.
#define MAX_SLEEP_SEGMENT_LEN (50)  //           (100)
// Gap between sleep cycles.
#define SLEEP_CYCLE_GAP (120)
// 最短睡眠时长
#define MIN_SLEEP_DURATION (60)
// 睡眠深度 - 清醒
#define D_AWAKE (1)
// 睡眠深度 - 眼动
#define D_REM (4)
// 睡眠深度 - 浅睡
#define D_LIGHT (2)
// 睡眠深度 - 深睡
#define D_DEEP (3)

// 佩戴检测
#define DIFF_BUF_SIZE (180)
#define NON_MOTION (1.2)
#define LARGE_MOTION (12.0)
#define SUCCESSIVE_MOTION (6.0)
#define MEAN_DiffSUM_THRESHOLD (6.0)

// Input struct. (暂定这个结构体，等测试功能稳定再修改)
struct LSSleepData {
  // uint8_t hr;  // 1秒钟1个，1hz
  uint8_t accX;          // 1秒钟1个，1hz
  uint8_t accY;          // 1秒钟1个，1hz
  uint8_t accZ;          // 1秒钟1个，1hz
  bool hrSwitch;         // 1分钟1个, 1/60hz
  bool hrWearIndicator;  // 1分钟1个，1/60hz
};

// Input struct.
struct LSSleepInput {
  // Utc.
  uint32_t utcTime;
  // Heart rate.
  // uint8_t hr;
  // Acceleration X.
  uint8_t accX;
  // Acceleration Y.
  uint8_t accY;
  // Acceleration Z.
  uint8_t accZ;
  // Wear detection
  bool wearIndicator;
};

// Indicator to parse a sleep result, whether it is a temporal/complete sleep
// cycle, or no sleep was detected (T_NONE).
typedef enum LSSleepCycleTerminator {
  // No sleep.
  T_NONE = 0,
  // Temporal sleep.
  T_TEMP = 1,
  // Complete sleep.
  T_COMP = 2,
} Terminator_t;

// Description of a sleep segment.
struct LSSleepSegment {
  uint32_t startTimeUtc;
  uint32_t endTimeUtc;
  uint8_t depth;
  uint32_t duration;
};

// Description of output sleep result.
struct LSSleepResult {
  // Bed time.
  uint32_t sleepTimeUtc;
  // Get-up time.
  uint32_t getupTimeUtc;
  // Awake duration in a sleep cycle.
  uint32_t awakeDuration;
  // Awake count in a sleep cycle.
  uint32_t awakeCount;
  // Rem duration in a sleep cycle.
  uint32_t remDuration;
  // Light sleep duration in a sleep cycle.
  uint32_t lightDuration;
  // Deep sleep duration in a sleep cycle.
  uint32_t deepDuration;

  // Description of all successive sleep segments.
  struct LSSleepSegment sleepSegments[MAX_SLEEP_SEGMENT_LEN];
  // Length of sleep segments in a sleep cycle.
  uint16_t numSleepSegments;

  uint8_t *wearBuf;
  uint16_t wearBufLen;

  Terminator_t completeCycleIndicator;
};

void LSSleepInitialize(void);
void LSSleepAnalyzeData(struct LSSleepData *data, uint8_t dataSize,
                        uint32_t utcTime, bool init);
// void LSSleepPutData(struct LSSleepInput *info, uint32_t utcTime,
//        bool wearDetection, bool init);
void LSSleepPutData(struct LSSleepInput *info, bool wearIndicator, bool init);
void LSSleepGetResult(struct LSSleepResult *result);
#endif
#endif
