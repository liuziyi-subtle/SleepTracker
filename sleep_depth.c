/*
 * Copyright(c) Liu Ziyi. Licensed under the MIT license.
 */

#include <stdlib.h>

#include "sleep_tracker.h"
#include "sleep_utils.h"

static uint8_t k_mem_pool[2 * SLEEP_MAX_RECORD_LENGTH];

/* extern functions. */
extern int32_t BaseCmpFunc(const void *a, const void *b);
extern uint8_t BaseSearchMostFrequentElement(const uint8_t *srcArr,
                                             uint8_t arrLen);

/* compute activities. */
static void _GetActivity(float32_t depth_feats[],
                         const uint16_t depth_feats_length,
                         uint8_t activities[]) {
  OnePassStd(0, 1);

  uint16_t i;
  for (i = 0; i < depth_feats_length; ++i) {
    float32_t dynamic_std = OnePassStd(depth_feats[i], 0);
    uint8_t activity = depth_feats[i] > 0.1f * dynamic_std ? 1 : 0;
  }
}

/* compute counts. */
static void _GetCount(uint8_t activities[], uint16_t activities_length,
                      uint8_t counts[], uint8_t avg_window_length) {
  uint8_t sum = avg_window_length;
  for (uint16_t i = 0; i < activities_length; ++i) {
    uint8_t out = i < avg_window_length ? 1 : activities[i - avg_window_length];
    uint8_t in = activities[i];
    sum -= out;
    sum += in;
    counts[i] = sum;
  }
}

/* _Get3StageDepth. */
static void _Get3StageDepth(const uint8_t counts[],
                            const uint16_t counts_length,
                            const sleep_marker_t markers[],
                            const uint16_t markers_length, uint8_t depths[]) {
  uint8_t lower_quantile = _GetPercentile(counts, counts_length, 20);
  uint8_t higher_quantile = _GetPercentile(counts, counts_length, 80);

  /* 为节省空间让depths暂时先存储排序后的counts. */
  for (uint16_t i = 0; i < counts_length; ++i) {
    depths[i] = counts[i];
  }
  qsort(depths, (size_t)counts_length, sizeof(uint8_t), BaseCmpFunc);
  uint8_t lower_quantile = _Quantile(depths, (uint32_t)counts_length, 0.2);
  uint8_t higher_quantile = _Quantile(depths, (uint32_t)counts_length, 0.8);

  for (uint16_t i = 0; i < counts_length; ++i) {
    if (counts[i] <= lower_quantile) {
      depths[i] = SLEEP_D_DEEP;
    } else if (counts[i] >= higher_quantile) {
      depths[i] = SLEEP_D_REM;
    } else {
      depths[i] = SLEEP_D_LIGHT;
    }
  }

  for (uint16_t i = 0; i < markers_length - 1; ++i) {
    uint16_t awake_from = markers[i].to + 1;
    uint16_t awake_to = markers[i + 1].from - 1;
    int16_t awake_duration = (int16_t)awake_to - (int16_t)awake_from + 1;
    if (awake_duration > 0) {
      for (uint16_t j = awake_from; j < awake_to; ++j) {
        depths[j] = SLEEP_D_AWAKE;
      }
    }
  }

  return;
}

/* _GetPostDepth. */
static void _GetPostDepth(const uint8_t depths[], uint16_t depths_length,
                          uint8_t window_length, uint8_t post_depths[]) {
  /* make sure the first and the last stages are D_LIGHT. */
  for (uint16_t i = 0; i < depths_length; ++i) {
    if (i < window_length || i > depths_length - window_length) {
      post_depths[i] = SLEEP_D_LIGHT;
    } else {
      post_depths[i] = BaseSearchMostFrequentElement(&depths[i], window_length);
    }
  }

  return;
}

uint16_t ComputeSleepDepth(const float32_t depth_feats[],
                           const uint16_t depth_feats_length,
                           sleep_marker_t markers[],
                           const uint16_t markers_length) {
  if (depth_feats_length > SLEEP_MAX_RECORD_LENGTH) {
    return NULL;
  }

  /* activities */
  uint8_t *activities = k_mem_pool;
  _GetActivity(depth_feats, depth_feats_length, activities);

  /* counts */
  uint8_t *counts = k_mem_pool + depth_feats_length;
  _GetCount(activities, depth_feats_length, counts, SLEEP_COUNTS_WINDOW_LENGTH);

  /* depths */
  uint8_t *depths = k_mem_pool;
  _Get3StageDepth(counts, depth_feats_length, markers, markers_length, depths);

  /* post_depths  */
  uint8_t *post_depths = k_mem_pool + depth_feats_length;
  _GetPostDepth(depths, depth_feats_length, SLEEP_POST_DEPTH_AVG_WINDOW_LENGTH,
                post_depths);

  /* wrap depth_markers. */
  uint8_t *p = post_depths, *q = post_depths;
  uint16_t segments_length = 0, duration = 0, counter = 0;
  while (q < post_depths + depth_feats_length) {
    if (p == q) {
      if (duration == 0) {
        segments_length++;
        markers[segments_length - 1].from = counter;
      } else {
        if (q == post_depths + depth_feats_length - 1) {
          markers[segments_length - 1].to = depth_feats_length - 1;
        }
      }
      duration++;
    } else {
      markers[segments_length - 1].to = counter - 1;
      p = q;
      duration = 0;
    }

    q++;
    counter++;
  }

  return segments_length;
}