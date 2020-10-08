/*
 * Copyright(c) Liu Ziyi. Licensed under the MIT license.
 */

#include <stdint.h>

#include "sleep_tracker.h"

#if ALGO_DEBUG
#include "debug/debug.h"
#endif

typedef float float32_t;

void BaseRoll(sleep_marker_t *data, int16_t data_length, int16_t shift);
int32_t BaseCmpFunc(const void *a, const void *b);
uint8_t BaseSearchMostFrequentElement(const uint8_t *srcArr, uint8_t arrLen);
uint8_t BaseQuantile(const uint8_t *data, uint32_t data_length, float32_t q);
float32_t BaseOnePassStd(const float32_t x, uint8_t init);
