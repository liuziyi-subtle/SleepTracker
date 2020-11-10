//
// Created by Ziyi Liu on 2019-04-15.
//
#include "ls_sleep_tracker.h"

#include <math.h>
#include <stdlib.h>

#include "ls_sleep_func.h"
#include "ls_sleep_predict.h"

// 当前文件打印输出宏定义
#ifdef SLEEPALGO_DEF_OUTPUT_LOG_ENABLE
#define LOCAL_DEBUG(...) log_i(__VA_ARGS__)
#else
#define LOCAL_DEBUG(...)
#endif

#ifdef DEBUG_LOCAL  // 用于GCC调试
#include <stdio.h>

#include "ls_sleep_debug.h"
#define data_malloc malloc
#define data_free free
#else  // 手表运行
#include "FreeRTOS.h"
//#include "ls_log.h"
#include "rtos.h"
#define data_malloc pvPortMalloc
#define data_free vPortFree
#endif  // DEBUG_LOCAL

// TODO:
//  1. 宏定义的名字精简，如SLEEP不需要
//  2. 全局变量名字精简，Sleep字样不需要
//  3.
//  gSleepDepthFeatBuf是否需要改成其它类型，尽量不用float,可以用[0,256]来算一下最大的标准差
//  4. gWearIndicator的作用不明确
//  5. 如果修改接口的话，有助于减少接口的转换

// Timer started at each time of algorithm initialization.
static uint32_t gTime;
static uint32_t gUtcTime;
static float gSleepDepthFeatBuf[MAX_SLEEP_RECORD_LEN];
static uint16_t gSleepDepthFeatBufLen;
#ifdef FUNC_SLEEP_CHECK_WEAR_TEST
static uint8_t kWearBuf[MAX_SLEEP_RECORD_LEN];
static uint16_t kWearBufLen;
#endif
static uint16_t gSleepMarkerBuf[MAX_SLEEP_MARKER_LEN];
static uint16_t gSleepMarkerBufLen;
static uint16_t gSleepCycleDetector;
static uint16_t gSleepCycleDetectorCompens;

static uint16_t gCounter = 0;

static uint16_t gTotalSleepDuration = 0;

//  新
static float gSum2CountBuf[MAX_SLEEP_RECORD_LEN];
static uint16_t gSum2CountBufLen;
static bool gWearIndicator;
static float gMeanDiffSum;

/**
 * @details
 * @brief  Initialization function for sleep detection.
 * @return        none
 *
 */
void LSSleepInitialize(void) {
  uint32_t i;

  gTime = 0;
  gUtcTime = 0;

  for (i = 0; i < MAX_SLEEP_MARKER_LEN; ++i) {
    gSleepMarkerBuf[i] = 0;
  }
  gSleepMarkerBufLen = 0;

  for (i = 0; i < MAX_SLEEP_RECORD_LEN; ++i) {
    gSleepDepthFeatBuf[i] = 0;
  }
  gSleepDepthFeatBufLen = 0;
  #ifdef FUNC_SLEEP_CHECK_WEAR_TEST
  for (i = 0; i < MAX_SLEEP_RECORD_LEN; ++i) {
    kWearBuf[i] = 1;
  }
  kWearBufLen = 0;
  #endif

  gSleepCycleDetector = 0;
  gSleepCycleDetectorCompens = 0;

  // gAddFlag = false;
  gCounter = 0;

  // 新
  for (i = 0; i < MAX_SLEEP_RECORD_LEN; ++i) {
    gSum2CountBuf[i] = .0f;
  }
  gSum2CountBufLen = 0;

  gWearIndicator = false;  // 初始化为false比较安全

  gMeanDiffSum = .0f;

  LSSleepAnalyzeData(NULL, 0, 0, true);

  LSSleepPutData(NULL, true, true);

  LSSleepFuncInitialize();

  gTotalSleepDuration = 0;
}

/**
 * 维护可用于纯粹基于加速度计尽心佩戴检测的数组.
 * 输入：1hz的采样数据
 * 输出：diffSum，diffCount，sum2Count，dPtr
 * 输出：基于心率的佩戴检测.
 */
// TODO(Ziyi Liu): LSSleepAnalyzeData不再需要init标志位。
void LSSleepAnalyzeData(struct LSSleepData *data, uint8_t dataSize,
                        uint32_t utcTime, bool init) {
  if (init) {
    return;
  }

  if(data==NULL)
  {
    return;
  }
  /* TODO(LIZIYI): Move UTC time calibration to LSSleepGetResult. */
  gCounter++;
  gTime++;
  if (gTime == 1) {
    gUtcTime = utcTime;
  }

  uint16_t i;
  // 每6s计算一个中位数，每10个中位数计算差分信息.
  uint8_t ptr = 0;
  uint8_t accXBuf[6], accYBuf[6], accZBuf[6];
  float medianXBuf[9], medianYBuf[9], medianZBuf[9];
  uint16_t medianBufLen = 0;
  while (ptr++ < dataSize) {
    if (ptr % 6 == 0) {
      for (i = 0; i < 6; ++i) {
        accXBuf[i] = (data[ptr - 6 + i]).accX;
        accYBuf[i] = (data[ptr - 6 + i]).accY;
        accZBuf[i] = (data[ptr - 6 + i]).accZ;
      }
      medianXBuf[medianBufLen] = FindMedian(accXBuf, 6);
      medianYBuf[medianBufLen] = FindMedian(accYBuf, 6);
      medianZBuf[medianBufLen] = FindMedian(accZBuf, 6);
      medianBufLen++;
    }
  }

  // 10个中位数计算9个差分值.
  uint16_t diffCount = 0, diffSum = 0;
  for (i = 1; i < medianBufLen; ++i) {
    float diffX = medianXBuf[i] - medianXBuf[i - 1];
    float diffY = medianYBuf[i] - medianYBuf[i - 1];
    float diffZ = medianZBuf[i] - medianZBuf[i - 1];

    diffCount += (uint16_t)((floorf(fabs(diffX)) != 0.0f) ? 1 : 0) +
                 (uint16_t)((floorf(fabs(diffY)) != 0.0f) ? 1 : 0) +
                 (uint16_t)((floorf(fabs(diffZ)) != 0.0f) ? 1 : 0);

    diffSum += (uint16_t)(floorf(fabs(diffX)) + floorf(fabs(diffY)) +
                          floorf(fabs(diffZ)));
  }

  // 更新两个可用于作佩戴检测的标志。
  gSum2CountBuf[gSum2CountBufLen++] =
      ((float)diffSum) / (((float)diffCount) + 0.000001f);
  gMeanDiffSum = (gMeanDiffSum + diffSum) / (float)gTime;

  // 基于3种情况进行佩戴检测:
  // 1. 全局心率打开，且心率检测未佩戴(完全可信).
  // 2. 全局心率打开，且心率检测为佩戴(不完全可信).
  // 3. 全局心率关闭，依靠加速度计进行检测.
  bool hrSwitch = data[0].hrSwitch;
  bool hrWearindicator = data[0].hrWearIndicator;
  bool wearIndicator = true;

  // TODO(Ziyi Liu): 如果心率打开且检测为佩戴实际不可信。
  if (hrSwitch) {
    if (!hrWearindicator) {
      wearIndicator = false;
    }
  } else {
    // TODO(Ziyi Liu): 全局心率关闭时，佩戴检测置为true。
    // wearIndicator = true;
    wearIndicator = false;
  }

  // TODO(Ziyi Liu): 如果gWearIndicator为true，需要在LSSleepGetResult中作第二轮
  //  佩戴检测。
  // gWearIndicator = wearIndicator;
  gWearIndicator = false;

  // 封装LSSleepPutData数据输入接口.
  struct LSSleepInput *info =
      (struct LSSleepInput *)data_malloc(sizeof(struct LSSleepInput));
  info->hr = data[0].hr;
  info->accX = data[0].accX;
  info->accY = data[0].accY;
  info->accZ = data[0].accZ;
  info->utcTime = utcTime;
  LSSleepPutData(info, wearIndicator, false);

  data_free(info);

  return;
}

/**
 * @details
 * @brief  Receive sleep information each minute.
 * @param[in]    *info    Struct to describe 3-axis acc and heart rate.
 * @param[in]    init     Flag to initialize LSSleepPutData function.
 * @return       none.
 *
 */
// void LSSleepPutData(uint8_t *data, uint16_t dataLen, uint32_t utcTime, bool
// init)
void LSSleepPutData(struct LSSleepInput *info, bool wearIndicator, bool init) {
  // Component acc buffer.
  static uint8_t compAccBuf[COMP_ACC_BUF_LEN];
  static uint8_t cPtr;

  // Resultant acc buffer.
  static float resAccBuf[RES_ACC_BUF_LEN];
  static uint8_t rPtr;

  // Sleep status of the last time.
  // static bool lastSleepStatus;

  // Heart rate buffer
  static uint8_t hrBuf[RES_ACC_BUF_LEN];
  static uint8_t hPtr;

  // Window for post-processing on model result.
  static uint8_t averageWindow[SLEEP_STATUS_AVG_WINDOW_LEN];
  static uint8_t aPtr;

  static bool wearIndicatorBuf[WEAR_INDICATOR_BUF_LEN];
  static uint8_t wPtr;

  static uint16_t count_stdPos;
  static double sum_stdPos;
  static double mean_stdPos;
  static uint16_t count_sleep;
  static uint16_t count_awake;

  uint16_t i = 0;

  if(info ==NULL)
  {
    return;
  }

  // Algorithm initialization
  if (init) {
    // Initialize component acc buffer.
    for (i = 0; i < COMP_ACC_BUF_LEN; ++i) {
      compAccBuf[i] = 0;
    }
    cPtr = 0;

    // Initialize resultant acc buffer.
    for (i = 0; i < RES_ACC_BUF_LEN; ++i) {
      resAccBuf[i] = .0;
    }
    rPtr = 0;

    for (i = 0; i < RES_ACC_BUF_LEN; ++i) {
      hrBuf[i] = 0;
    }
    hPtr = 0;

    // Set initially the sleep status as wake.
    // lastSleepStatus = false;

    // Initialize average window.
    for (i = 0; i < SLEEP_STATUS_AVG_WINDOW_LEN; ++i) {
      averageWindow[i] = 0;
    }
    aPtr = 0;

    // Initialize wear indicator buffer.
    for (i = 0; i < WEAR_INDICATOR_BUF_LEN; ++i) {
      wearIndicatorBuf[i] = false;
    }
    wPtr = 0;

    count_stdPos = 0;
    sum_stdPos = .0;
    mean_stdPos = .0;
    count_sleep = 0;
    count_awake = 0;

    return;
  }

  // Update heart rate buffer.
  hrBuf[hPtr++] = info->hr;
  if (hPtr == RES_ACC_BUF_LEN) {
    hPtr = 0;
  }

  // Update resultant acc buffer.
  float resAcc = pow(info->accX, 2) + pow(info->accY, 2) + pow(info->accZ, 2);
  resAccBuf[rPtr++] = sqrt(resAcc);
  if (rPtr == RES_ACC_BUF_LEN) {
    rPtr = 0;
  }

  // Update component acc buffer.
  compAccBuf[cPtr++] = info->accX;
  compAccBuf[cPtr++] = info->accY;
  compAccBuf[cPtr++] = info->accZ;
  if (cPtr == COMP_ACC_BUF_LEN) {
    cPtr = 0;
  }

  // Update wear indicator buffer.
  wearIndicatorBuf[wPtr++] = wearIndicator;
  if (wPtr == WEAR_INDICATOR_BUF_LEN) {
    wPtr = 0;
  }
  #ifdef FUNC_SLEEP_CHECK_WEAR_TEST
  kWearBuf[kWearBufLen++] = wearIndicator;
  #endif

  // If global time counter does not exceed 7 minutes, update sleep depth
  // feature buffer with 0 and return. Note: if LSSleepGetResult is called,
  // SleepCycleFinish field of result would be 0 (no sleep detected).
  if (gTime < 2 * STD_DURATION + 1) {
    gSleepDepthFeatBuf[gSleepDepthFeatBufLen++] = .0;
#ifdef DEBUG_LOCAL
    PLOT.xBuf[PLOT.xBufLen++] = 0;
    PLOT.yBuf[PLOT.yBufLen++] = 0;
    PLOT.zBuf[PLOT.zBufLen++] = 0;
    PLOT.statusBuf[PLOT.statusBufLen++] = 0;
    PLOT.wearIndBuf[PLOT.wearIndBufLen++] = wearIndicator;
#endif  // DEBUG_LOCAL
    return;
  }

  // Compute std with one pass style.
  float stdPre;
  OnePassStd(0, true);
  for (i = 0; i < STD_DURATION; ++i) {
    stdPre = OnePassStd(resAccBuf[(rPtr + i) % RES_ACC_BUF_LEN], false);
  }

  float stdPos;
  OnePassStd(0, true);
  for (i = 0; i < STD_DURATION; ++i) {
    stdPos = OnePassStd(
        resAccBuf[(rPtr + STD_DURATION + i + 1) % RES_ACC_BUF_LEN], false);
  }

  gSleepDepthFeatBuf[gSleepDepthFeatBufLen++] = stdPos;

  // Wrap feature
  float feats[5];
  feats[0] = stdPre;
  feats[1] = stdPos;
  feats[2] = compAccBuf[cPtr];
  feats[3] = compAccBuf[cPtr + 1];
  feats[4] = compAccBuf[cPtr + 2];

  // Get result from model.
  float prob = PredictSleepStatus(feats);
  if (prob >= 0.5) {
    count_stdPos++;
    sum_stdPos += stdPos;
    mean_stdPos /= count_stdPos;
    count_sleep++;
    /* 克服清醒初期10分钟内出现的睡眠. */
    if (count_sleep > 3) {
      count_awake = 0;
    }
  } else {
    count_awake++;
    count_sleep = 0;
    if ((gTotalSleepDuration > 90) && (gTotalSleepDuration < 300)) {
      float awake2sleep = stdPos / (float)mean_stdPos;
      if (awake2sleep < 2) {
        if (count_awake < 10) {
          if (prob > 0.3) {
            prob = 0.51;
          }
        }
      }
    }
  }

  int pred;
  if (prob >= 0.5) {
    pred = 1;
  } else {
    pred = 0;
  }

  // 根据wear indicator修正pred，区分睡眠/静置.
  if (!(wearIndicatorBuf[wPtr])) {
    pred = 0;
  }

  // 如果当前hrBuf中心率在7分钟内的变化量为0，则认为是佩戴检测失效所致.  //yuS
//  int hrDiffSum = 0;
//  for (i = 1; i < RES_ACC_BUF_LEN; i++) {
//    hrDiffSum += hrBuf[i] - hrBuf[i - 1];
//  }
//  if ((hrDiffSum == 0) && (gTotalSleepDuration < 10)) {
//    pred = 0;
//  }

  // Update average window.
  averageWindow[aPtr++] = pred;
  if (aPtr == SLEEP_STATUS_AVG_WINDOW_LEN) {
    aPtr = 0;
  }

  // Post-precess the result from model.
  bool sleepStatus = WindowAverage(averageWindow, SLEEP_STATUS_AVG_WINDOW_LEN);

#ifdef DEBUG_LOCAL
  PLOT.xBuf[PLOT.xBufLen++] = feats[2];
  PLOT.yBuf[PLOT.yBufLen++] = feats[3];
  PLOT.zBuf[PLOT.zBufLen++] = feats[4];
  PLOT.statusBuf[PLOT.statusBufLen++] = sleepStatus;
  PLOT.wearIndBuf[PLOT.wearIndBufLen++] = wearIndicatorBuf[wPtr];
#endif  // DEBUG_LOCAL

  // If sleep status is WAKE, gSleepCycleDetector + 1, otherwise set it as 0.
  if (sleepStatus) {
    gSleepCycleDetectorCompens++;
    if (gSleepCycleDetectorCompens > 15) {
      gSleepCycleDetector = 0;
    }
  } else {
    gSleepCycleDetector++;
    gSleepCycleDetectorCompens = 0;
  }

  if ((sleepStatus && gSleepMarkerBufLen % 2 == 0) ||
      (!sleepStatus && gSleepMarkerBufLen % 2 == 1)) {
    gSleepMarkerBuf[gSleepMarkerBufLen++] = gTime;
  }

#ifdef DEBUG_LOCAL
  //    printf("gCounter (LSSleepPutData): %u\n", gCounter);
  //    for (i = 0; i < gSleepMarkerBufLen; ++i) {
  //        printf("[%u]: %u, ", i, gSleepMarkerBuf[i]);
  //    }
  //    printf("\n");
#endif  // DEBUG_LOCAL

  return;
}

/*
 睡眠深度分析:根据睡眠深度特征gSleepDepthFeatBuf,判定gSleepMarkerBuf所记录睡眠段
 的深度信息。同时，去除睡眠中小的清醒片段，分割不同的整段睡眠.

 输入:
  gSleepDepthFeatBuf    - 睡眠深度特征
  gSleepDepthFeatureBufLen - 睡眠深度特征有效长度
  gSleepMarkerBuf          - 睡眠状态起始点及持续时长
  gSleepMarkerBufLen       - 睡眠状态起始点及持续时长的有效长度
  utcTime                  - 调用算法时的UTC时间戳

 输出:
  result                   - 链接不同的整段睡眠(整段睡眠包含睡眠深度片段)

*/
void LSSleepGetResult(struct LSSleepResult *result) {
  // Indicate whether the last marker of Global sleep marker buffer was
  // temporarily subtracted.
  bool addMarkerIndicator = false;
  // uint16_t tempSleepMarkerBufLen = gSleepMarkerBufLen;
  uint16_t i;

  // gSleepMarkerBufLen为奇数则补为偶数长度。
  if (gSleepMarkerBufLen % 2 != 0) {
    gSleepMarkerBufLen += 1;
    gSleepMarkerBuf[gSleepMarkerBufLen - 1] = gTime;
    addMarkerIndicator = true;
  }

  // TODO(Ziyi Liu): 第二轮佩戴检测,完善CheckWear函数。
  /*
  bool secondRoundWearIndicator;
  if (gWearIndicator && gSleepMarkerBufLen > 0) {
    uint16_t markerOn = gSleepMarkerBuf[gSleepMarkerBufLen - 2];
    uint16_t markerOff = gSleepMarkerBuf[gSleepMarkerBufLen - 1];
    secondRoundWearIndicator = SecondRoundCheckWear();
    if (!secondRoundWearIndicator) {
      gSleepMarkerBufLen = gSleepMarkerBufLen - 2;
      addMarkerIndicator = false;
    }
  }
      */

  // 越界检查：gSleepMarkerBuf中每一个点必须大于前面的点且gSleepMarkerBuf记录的
  // 长度小于MAX_SLEEP_RECORD_LEN。
  if (gSleepMarkerBufLen > 0) {
    for (i = 1; i < gSleepMarkerBufLen; ++i) {
      if ((gSleepMarkerBuf[i] < gSleepMarkerBuf[i - 1]) ||
          (gSleepMarkerBuf[i] >= MAX_SLEEP_RECORD_LEN) ||
          (gSleepMarkerBuf[i - 1] <= 0) || (gSleepMarkerBuf[i] <= 0)) {
#ifdef DEBUG_LOCAL
        //                printf("gSleepMarkerBuf[i - 1]: %u,
        //                gSleepMarkerBuf[i]: %u\n", gSleepMarkerBuf[i - 1],
        //                gSleepMarkerBuf[i]); printf("LSSleepGetResult ---
        //                gSleepMarkerBuf异常\n");
#endif  // #ifdef DEBUG_LOCAL
        //                printf("44444444444444444\n");
        LSSleepInitialize();
      }
    }

    if (gSleepMarkerBufLen > MAX_SLEEP_MARKER_LEN - 2) {
      //            printf("5555555555555555\n");
      LSSleepInitialize();
    }
  }

  // 计算时长并去除与后段大于10/30/60分钟，自身之和小于10/30/60分钟的marker对。
  uint16_t totalSleepDuration =
      RemoveNoiseMarker(gSleepMarkerBuf, &gSleepMarkerBufLen);
  gTotalSleepDuration = totalSleepDuration;
  //    printf("totalSleepDuration: %u, gSleepCycleDetector: %u\n",
  //    totalSleepDuration, gSleepCycleDetector);

  // TODO(Ziyi Liu): MIN_SLEEP_DURATION在心率灯关闭时需要延长至2h以便适应2h内
  //  gMeanDiffSum的判定。
  // TODO(Ziyi Liu): T_COMP和T_TEMP的操作需要位于整个result的计算之后。
  if (totalSleepDuration < MIN_SLEEP_DURATION) {
    // 如果时长小于2h，无睡眠监测到，结果状态位置为T_NONE并返回.
    result->completeCycleIndicator = T_NONE;
    if (addMarkerIndicator && gSleepMarkerBufLen > 0) {
      gSleepMarkerBufLen -= 1;
    }

    // 如果没有下述条件，将会造成清醒时间不停地延长，全局变量不停地增加.
    // 下面也可以改成if (totalSleepDuration == 0) {
    if (gSleepCycleDetector > SLEEP_CYCLE_GAP) {
      //            printf("6666666666666666\n");
      LSSleepInitialize();
    }

    return;
  } else {
    // 如果满足最小睡眠时长要求。
    if (gSleepCycleDetector > SLEEP_CYCLE_GAP) {
      // 与前面的睡眠间隔SLEEP_CYCLE_GAP以上。
      //            printf("gSleepCycleDetector: %u\n", gSleepCycleDetector);
      //            printf("result->awakeDuration: %u\n",
      //            result->awakeDuration); printf("result->lightDuration:
      //            %u\n", result->lightDuration); printf("result->deepDuration:
      //            %u\n",  result->deepDuration); printf("result->remDuration:
      //            %u\n",   result->remDuration); printf("gCounter: %u\n",
      //            gCounter);
      result->completeCycleIndicator = T_COMP;
      //            printf("7777777777777777\n");
      LSSleepInitialize();
      return;
    } else {
      // 如果满足最小睡眠时长要求。
      if (gSleepCycleDetector > SLEEP_CYCLE_GAP) {
        // 与前面的睡眠间隔SLEEP_CYCLE_GAP以上。
        result->completeCycleIndicator = T_COMP;
        LSSleepInitialize();
        return;
      } else if (gSleepCycleDetector > 0) {
        // 此时处于清醒期，不用对result更新。
        result->completeCycleIndicator = T_TEMP;
        if (addMarkerIndicator && gSleepMarkerBufLen > 0) {
          gSleepMarkerBufLen -= 1;
        }
        return;
      } else {
        result->completeCycleIndicator = T_TEMP;
      }
    }
  }

#ifdef DEBUG_LOCAL
  //    printf("gCounter (LSSleepGetResult): %u\n", gCounter);
  //    for (i = 0; i < gSleepMarkerBufLen; ++i) {
  //        printf("[%u]: %u, ", i, gSleepMarkerBuf[i]);
  //    }
  //    printf("\n");
#endif  // DEBUG_LOCAL

  // Compute the depth of the detected sleep. Util here, the global sleep
  // marker buffer length must be changed, and a required sleep cycle exists.

  // Sleep segment start and end utc time.
  uint32_t start = 0, end = 0, duration = 0;
  // Sleep segment depth.
  uint8_t depth;
  // Number of sleep segments in Sleep cycle.
  uint16_t numSleepSegments = 0;
  // Sleep cycle awake duration and counts.
  uint16_t awakeDuration = 0, awakeCount = 0;
  // Sleep cycle light/deep/rem duration.
  uint16_t lightDuration = 0, deepDuration = 0, remDuration = 0;

  // 已经对gSleepMarkerBuf做过检查后，不需要再对sleepCycleOn和sleepCycleLen检查。
  uint16_t sleepCycleOn = gSleepMarkerBuf[0];
  uint16_t sleepCycleLen =
      (gSleepMarkerBuf[gSleepMarkerBufLen - 1] - gSleepMarkerBuf[0]);
  uint16_t *sleepMarkerBuf =
      (uint16_t *)data_malloc(gSleepMarkerBufLen * sizeof(uint16_t));
  for (i = 0; i < gSleepMarkerBufLen; ++i) {
    sleepMarkerBuf[i] = gSleepMarkerBuf[i] - sleepCycleOn;
  }

  // Compute the depth of each sleep segment in the sleep cycle.
  uint16_t sleepCycleDepthSize = MAX_SLEEP_SEGMENT_LEN * 3;
  uint16_t *sleepCycleDepth =
      (uint16_t *)data_malloc(sizeof(uint16_t) * sleepCycleDepthSize);
  uint16_t validSleepCycleDepthSize = ComputeSleepDepth(
      &gSleepDepthFeatBuf[sleepCycleOn], sleepCycleLen, sleepMarkerBuf,
      gSleepMarkerBufLen, sleepCycleDepth, sleepCycleDepthSize);
  data_free(sleepMarkerBuf);

  // 暂时还是对sleepCycleLen进行检查，如果越界且现在存在临时睡眠，则初始化并将临时睡眠改为完整睡眠
  if (validSleepCycleDepthSize == 0) {
    if (result->completeCycleIndicator == T_TEMP) {
      result->completeCycleIndicator = T_COMP;
    }

    if (sleepCycleDepth != NULL) {
      data_free(sleepCycleDepth);
    }

    //        printf("888888888888888\n");
    LSSleepInitialize();
  }

  // TODO(Ziyi
  // Liu):检查睡眠总时长和（终点-起点）一致性，检查所有segments和总时长一致
  //  性，检查各segments前后衔接一致性。
  // Number of sleep segments.
  numSleepSegments = validSleepCycleDepthSize / 3;
  //    printf("++++++++++++++++numSleepSegments: %u\n", numSleepSegments);
  for (i = 0; i < numSleepSegments; i++) {
    start = (uint32_t)sleepCycleDepth[3 * i];
    end = (uint32_t)sleepCycleDepth[3 * i + 2];
    depth = (uint8_t)sleepCycleDepth[3 * i + 1];
    duration = end - start + 1;

    // uint32_t corrrctedUtcTime = gUtcTime - 10 * 60;
    uint32_t corrrctedUtcTime = gUtcTime;

    // result->sleepSegments[i].startTimeUtc = gUtcTime + 60 * (start +
    // sleepCycleOn); result->sleepSegments[i].endTimeUtc = gUtcTime + 60 * (end
    // + sleepCycleOn + 1);
    result->sleepSegments[i].startTimeUtc =
        corrrctedUtcTime + 60 * (start + sleepCycleOn);
    result->sleepSegments[i].endTimeUtc =
        corrrctedUtcTime + 60 * (end + sleepCycleOn + 1);
    result->sleepSegments[i].depth = (uint8_t)depth;
    result->sleepSegments[i].duration = duration;

    switch (depth) {
      case D_AWAKE:
        awakeDuration += duration;
        awakeCount += 1;
        break;
      case D_LIGHT:
        lightDuration += duration;
        break;
      case D_DEEP:
        deepDuration += duration;
        break;
      case D_REM:
        remDuration += duration;
        break;
      default:
        break;
    }
  }

  result->sleepTimeUtc = gUtcTime + 60 * sleepCycleOn;
  // result->getupTimeUtc = gUtcTime + 60 * (sleepCycleOn + sleepCycleLen - 1);
  result->getupTimeUtc = gUtcTime + 60 * (sleepCycleOn + sleepCycleLen);
  result->awakeDuration = awakeDuration;
  result->awakeCount = awakeCount;
  result->lightDuration = lightDuration;
  result->deepDuration = deepDuration;
  result->remDuration = remDuration;
  result->numSleepSegments = numSleepSegments;
  //    printf("result->awakeDuration: %u\n", result->awakeDuration);
  //    printf("result->lightDuration: %u\n", result->lightDuration);
  //    printf("result->deepDuration: %u\n",  result->deepDuration);
  //    printf("result->remDuration: %u\n",   result->remDuration);
  //    printf("gCounter: %u\n", gCounter);
  #ifdef FUNC_SLEEP_CHECK_WEAR_TEST
  result->wearBuf = &kWearBuf[sleepCycleOn];
  result->wearBufLen = sleepCycleLen;
  #endif

  if (addMarkerIndicator && gSleepMarkerBufLen > 0) {
    gSleepMarkerBufLen -= 1;
  }

  if (sleepCycleDepth != NULL) {
    data_free(sleepCycleDepth);
  }

  // TODO(Ziyi Liu): gSleepMarkerBuf和gSleepDepthFeatBuf的越界检查需要位于result
  //  的计算之后，gSleepDepthFeatBuf和gSum2CountBuf的长度可通过gTime表征。
  // 如果gSleepDepthFeatBufLen达到上限则初始化.
  if (gSleepDepthFeatBufLen >= MAX_SLEEP_RECORD_LEN) {
    if (result->completeCycleIndicator == T_TEMP) {
      result->completeCycleIndicator = T_COMP;
    }
    //        printf("9999999999999\n");
    LSSleepInitialize();
  }

  return;
}