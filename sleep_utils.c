/*
 * Copyright(c) Liu Ziyi. Licensed under the MIT license.
 */

#include "sleep_utils.h"

#include "sleep_tracker.h"

/* To left if shift > 0 else right. */
void BaseRoll(sleep_marker_t *data, int16_t data_length, int16_t shift) {
  /* first roll within [0, shift - 1] and [shift, data_length - 1],
   * then flip the whole data. */
  if (shift == 0) {
    return;
  }

  /* p points to the element which will be the new data[0],
   * p must exchange with data[data_length - 1]. */
  sleep_marker_t *p = &data[(data_length + shift) % data_length];
  sleep_marker_t *q = &data[data_length - 1];
  sleep_marker_t temp;
  while (p < q) {
    temp = *p;
    *p++ = *q;
    *q-- = temp;
  }

  /* p points to the element which will be the new data[data_length - 1],
   * p must exchange with data[0]. */
  p = &data[(data_length + shift - 1) % data_length];
  q = data;
  while (p > q) {
    temp = *p;
    *p-- = *q;
    *q++ = temp;
  }

  /* roll the whole data*/
  p = data;
  q = &data[data_length - 1];
  while (p < q) {
    temp = *p;
    *p++ = *q;
    *q-- = temp;
  }

  return;
}

int32_t BaseCmpFunc(const void *a, const void *b) {
  return (*(uint8_t *)a > *(uint8_t *)b) ? 1 : -1;
}

uint8_t BaseSearchMostFrequentElement(const uint8_t *srcArr, uint8_t arrLen) {
  /* TODO: 改为全局数组. */
  uint8_t *arr = (uint8_t *)malloc(sizeof(uint8_t) * arrLen);
  memcpy(arr, srcArr, sizeof(uint8_t) * arrLen);
  qsort(arr, arrLen, sizeof(uint8_t), BaseCmpFunc);

  // 线性遍历
  uint8_t max_count = 1, res = arr[0], curr_count = 1;
  for (uint8_t i = 1; i < arrLen; i++) {
    if (arr[i] == arr[i - 1]) {
      curr_count++;
    } else {
      if (curr_count > max_count) {
        max_count = curr_count;
        res = arr[i - 1];
      }
      curr_count = 1;
    }
  }

  // 如果最后一个元素满足条件
  if (curr_count > max_count) {
    // max_count = curr_count;
    res = arr[arrLen - 1];
  }
  free(arr);

  return res;
}

/* TODO: 转化为可对任何类型数组求分位数. */
uint8_t BaseQuantile(const uint8_t *data, uint32_t data_length, float32_t q) {
  float32_t qIndexRight = 1.0 + (data_length - 1.0) * q;
  float32_t qIndexLeft = floor(qIndexRight);
  float32_t fraction = qIndexRight - qIndexLeft;
  uint32_t qIndex = (uint32_t)qIndexLeft;
  float32_t quantile =
      data[qIndex - 1u] + (data[qIndex] - data[qIndex - 1u]) * fraction;
  return (uint8_t)quantile;
}

float32_t BaseOnePassStd(const float32_t x, uint8_t init) {
  static double lastSum, lastSumSquare;
  static int32_t len;

  if (init) {
    lastSum = .0;
    lastSumSquare = .0;
    len = 0;
    return .0;
  }

  double sum, sumSquare, mean, variance;

  if (++len == 1) {
    lastSum = (double)x;
    lastSumSquare = pow((double)x, 2);
    return .0;
  }

  sum = lastSum + (double)x;
  sumSquare = lastSumSquare + pow((double)x, 2);
  mean = sum / len;

  variance = (sumSquare - len * pow(mean, 2)) / len;

  lastSum = sum;
  lastSumSquare = sumSquare;

  if (variance == .0) {
    return .0;
  } else {
    return (float32_t)sqrt(variance);
  }
}