/*
 * Copyright(c) Liu Ziyi. Licensed under the MIT license.
 */

#include "sleep_tracker.h"

#include <math.h>
#include <stdlib.h>

#include "sleep_model.h"
#include "sleep_utils.h"

static uint32_t k_time = 0;
static uint32_t k_utc = 0;

static float32_t k_depth_feats[SLEEP_MAX_RECORD_LENGTH] = {.0f};
static uint16_t k_depth_feats_length = 0;

static sleep_marker_t k_markers[SLEEP_MAX_SLEEP_MARKER_LENGTH] = {{0, 0, 0}};
static sleep_marker_t k_temp_markers[SLEEP_MAX_SLEEP_MARKER_LENGTH] = {
    {0, 0, 0}};
static uint16_t k_markers_length = 0;

static uint16_t k_cycle_detector = 0;
static uint16_t k_cycle_detector_compens = 0;

static uint16_t k_total_duration = 0;

/* do not initialize k_output. */
static sleep_tracker_output_t k_output = {0, 0, 0, 0, 0, 0, 0, NULL, 0, T_NONE};

/* extern functions. */
extern float32_t BaseOnePassStd(const float32_t x, uint8_t init);

static uint8_t _Predict(const float32_t features[]) {
  union Entry data[SLEEP_FEATS_LENGTH];

  // Parse features
  for (uint8_t i = 0; i < SLEEP_FEATS_LENGTH; ++i) {
    data[i].fvalue = features[i];
  }

  float32_t prob = predict_sleep_status(data, 0);

  if (prob >= 0.5f) {
    return 1;
  } else {
    return 0;
  }
}

static uint16_t _RemoveNoiseMarkers(sleep_marker_t markers[],
                                    uint16_t markers_length) {
  if (markers_length < 2) {
    return 0;
  }

  uint16_t new_markers_length = markers_length;

  sleep_marker_t* p = &markers[1];
  uint16_t shift = 0;
  while (p->from != 0) {
    uint16_t pre_duration = 0, gap = 0;

    sleep_marker_t* q = &markers[0];
    while (q < p) {
      pre_duration += q->to - q->from;
      q++;
      shift++;
    }

    gap = p->from - (p - 1)->to;

    if ((gap >= 10 && pre_duration < 10) || (gap >= 30 && pre_duration < 30) ||
        (gap >= 60 && pre_duration < 60)) {
      BaseRoll(markers, (int16_t)markers_length, (int16_t)shift);
      /* reset the removed markers. */
      for (uint16_t i = 0; i < shift; ++i) {
        markers[markers_length - 1 - i].from = 0;
        markers[markers_length - 1 - i].to = 0;
        markers[markers_length - 1 - i].depth = 0;
      }
      p = &markers[1];
      new_markers_length -= shift;
    } else {
      p++;
    }
  }

  return new_markers_length;
}

void _DetectSleep(uint16_t acc_x, uint16_t acc_y, uint16_t acc_z,
                  uint8_t nonwear, uint8_t init) {
  static uint16_t comp_accs[SLEEP_COMP_ACC_LENGTH]; /*<< component buffer. */
  static uint8_t c_ptr;

  static float32_t res_accs[SLEEP_RES_ACC_LENGTH]; /*<< resultant buffer. */
  static uint8_t r_ptr;

  static uint8_t average_window[SLEEP_STATUS_AVG_WINDOW_LEN];
  static uint8_t a_ptr;
  static uint8_t average_window_sum;

  static uint8_t nonwears[SLEEP_NONWEAR_LENGTH];
  static uint8_t n_ptr;

  uint16_t i = 0;

  /* algorithm init. */
  if (init) {
    for (i = 0; i < SLEEP_COMP_ACC_LENGTH; ++i) {
      comp_accs[i] = 0;
    }
    c_ptr = 0;

    for (i = 0; i < SLEEP_RES_ACC_LENGTH; ++i) {
      res_accs[i] = .0f;
    }
    r_ptr = 0;

    for (i = 0; i < SLEEP_STATUS_AVG_WINDOW_LEN; ++i) {
      average_window[i] = 0;
    }
    a_ptr = 0;
    average_window_sum = 0;

    for (i = 0; i < SLEEP_NONWEAR_LENGTH; ++i) {
      nonwears[i] = 0;
    }
    n_ptr = 0;

    return;
  }

  /* update resultant acc buffer. */
  float32_t res_acc =
      (float32_t)(pow(acc_x, 2) + pow(acc_y, 2) + pow(acc_z, 2));
  res_accs[r_ptr++] = sqrtf(res_acc);
  if (r_ptr == SLEEP_RES_ACC_LENGTH) {
    r_ptr = 0;
  }

  /* update component acc buffer. */
  comp_accs[c_ptr++] = acc_x;
  comp_accs[c_ptr++] = acc_y;
  comp_accs[c_ptr++] = acc_z;
  if (c_ptr == SLEEP_COMP_ACC_LENGTH) {
    c_ptr = 0;
  }

  /* update nonwear indicator buffer. */
  nonwears[n_ptr++] = nonwear;
  if (n_ptr == SLEEP_NONWEAR_LENGTH) {
    n_ptr = 0;
  }

  /* If global time counter < 7 minutes, update sleep depth feature buffer with
   * 0 and return. */
  if (k_time < 2 * SLEEP_STD_DURATION + 1) {
    k_depth_feats[k_depth_feats_length++] = .0f;
    return;
  }

  /* compute std with one pass style. */
  float32_t std_pre;
  BaseOnePassStd(0, 1);
  for (i = 0; i < SLEEP_STD_DURATION; ++i) {
    std_pre = BaseOnePassStd(res_accs[(r_ptr + i) % SLEEP_RES_ACC_LENGTH], 0);
  }

  float32_t std_pos;
  BaseOnePassStd(0, 1);
  for (i = 0; i < SLEEP_STD_DURATION; ++i) {
    std_pos = BaseOnePassStd(
        res_accs[(r_ptr + SLEEP_STD_DURATION + i + 1) % SLEEP_RES_ACC_LENGTH],
        0);
  }

  k_depth_feats[k_depth_feats_length++] = std_pos;

  /* wrap feature. */
  float32_t feats[SLEEP_FEATS_LENGTH] = {std_pre, std_pos, comp_accs[c_ptr],
                                         comp_accs[c_ptr + 1],
                                         comp_accs[c_ptr + 2]};

  /* get result from model. */
  uint8_t pred = _Predict(feats);

  /* force pred to awake if nonwear. */
  if (!(nonwears[n_ptr])) {
    pred = 0;
  }

  /* update average window. */
  average_window[a_ptr++] = pred;
  if (a_ptr == SLEEP_STATUS_AVG_WINDOW_LEN) {
    a_ptr = 0;
  }
  average_window_sum += pred;
  average_window_sum -= average_window[a_ptr];

  uint8_t sleep_status = average_window_sum > 6 ? 1 : 0; /*<< majority-vote. */

  /* if sleep status = WAKE, k_cycle_detector + 1, otherwise 0. */
  if (sleep_status) {
    k_cycle_detector_compens++;
    if (k_cycle_detector_compens > 15) {
      k_cycle_detector = 0;
    }
  } else {
    k_cycle_detector++;
    k_cycle_detector_compens = 0;
  }

  if (sleep_status) {
    /* new segment from point. */
    if (k_markers_length == 0) {
      k_markers_length++;
      k_markers[k_markers_length - 1].from = k_time;
    } else {
      if (k_markers[k_markers_length - 1].to != 0 &&
          k_markers[k_markers_length].from == 0) {
        k_markers_length++;
        k_markers[k_markers_length - 1].from = k_time;
      }
    }
  } else {
    /* new segment to point. */
    if (k_markers_length > 0) {
      if (k_markers[k_markers_length - 1].from != 0 &&
          k_markers[k_markers_length - 1].to == 0) {
        k_markers[k_markers_length - 1].to = k_time;
      }
    }
  }

  return;
}

/* NOTE: this func does not initialize k_output, which would only be intialize
 * as k_markers_length == 0. */
void SleepInit() {
  uint16_t i;

  k_time = 0;
  k_utc = 0;

  for (i = 0; i < SLEEP_MAX_RECORD_LENGTH; ++i) {
    k_depth_feats[i] = 0;
  }
  k_depth_feats_length = 0;

  for (i = 0; i < SLEEP_MAX_SLEEP_MARKER_LENGTH; ++i) {
    k_markers[i].from = 0;
    k_markers[i].to = 0;
    k_markers[i].depth = SLEEP_D_AWAKE;
  }
  k_markers_length = 0;

  k_cycle_detector = 0;
  k_cycle_detector_compens = 0;

  k_total_duration = 0;

  _DetectSleep(0, 0, 0, 0, 1);
  // _AnalyzeDepth();
}

void SleepInput(sleep_tracker_input_t* input) {
  k_time++;
  if (k_time == 1) {
    k_utc = input->utc;
  }

  uint16_t acc_x = (uint16_t)input->acc_t.x[0];
  uint16_t acc_y = (uint16_t)input->acc_t.y[0];
  uint16_t acc_z = (uint16_t)input->acc_t.z[0];
  uint8_t nonwear = input->nonwear;

  _DetectSleep(acc_x, acc_y, acc_z, nonwear, 0);

  return;
}

sleep_tracker_output_t* SleepOutput() {
  /* directly return if no sleep. */
  if (k_markers_length == 0) {
    k_output.complete_cycle_indicator = T_NONE;
    return &k_output;
  }

  memcpy(k_temp_markers, k_markers, sizeof(sleep_marker_t));
  sleep_marker_t* markers = k_temp_markers;
  uint16_t markers_length = k_markers_length;
  if (markers[markers_length - 1].to == 0) {
    markers[markers_length - 1].to = k_time;
  }

  /* 计算时长并去除与后段大于10/30/60分钟，自身之和小于10/30/60分钟的marker对 */
  markers_length = _RemoveNoiseMarkers(markers, markers_length);

  /* total sleep duration. */
  sleep_marker_t* p = markers;
  while (p->from != 0) {
    k_total_duration += (p->to - p->from);
  }

  /* must longer than SLEEP_MIN_SLEEP_DURATION. */
  if (k_total_duration < SLEEP_MIN_SLEEP_DURATION) {
    k_output.complete_cycle_indicator = T_NONE;

    /* terminate sleep cycle. */
    if (k_cycle_detector > SLEEP_CYCLE_GAP) {
      SleepInit();
    }
    return &k_output;
  } else {
    if (k_cycle_detector > SLEEP_CYCLE_GAP) {
      /* completely awake, unecessary to update. */
      k_output.complete_cycle_indicator = T_COMP;
      SleepInit();
      return &k_output; /*<< return the previous k_output. */
    } else {
      if (k_cycle_detector > 0) {
        /* not sure completely awake, unecessary to update. */
        k_output.complete_cycle_indicator = T_TEMP;
        return &k_output;
      } else {
        /* asleep. */
        k_output.complete_cycle_indicator = T_TEMP;
      }
    }
  }

  /* COMPUTE THE DEPTH OF THE DETECT SLEEP. */

  uint16_t cycle_start = markers[0].from;
  for (uint16_t i = 0; i < markers_length; ++i) {
    markers[i].from = markers[i].from - cycle_start;
    markers[i].to = markers[i].to - cycle_start;
  }

  /* the depth of each sleep segment in the sleep cycle. */
  float32_t* depth_feats = &k_depth_feats[cycle_start];
  uint16_t depth_feats_length =
      markers[markers_length - 1].to - markers[0].from + 1;
  markers_length = ComputeSleepDepth(depth_feats, depth_feats_length, markers,
                                     markers_length);

  /* wrap output. */
  uint32_t from = 0, to = 0, depth = 0; /*<< segment start, end, duration. */
  uint16_t segments_length = 0;
  uint16_t awake_duration = 0, awake_counts = 0;
  uint16_t light_duration = 0, deep_duration = 0, rem_duration = 0;

  for (uint16_t i = 0; i < markers_length; i++) {
    uint32_t adjusted_utc = k_utc - 10 * 60;
    markers[i].from = adjusted_utc + 60 * (markers[i].from + cycle_start);
    markers[i].to = adjusted_utc + 60 * (markers[i].to + cycle_start);

    uint32_t duration = markers[i].to - markers[i].from + 1;
    switch (markers[i].depth) {
      case SLEEP_D_AWAKE:
        k_output.awake_duration += duration;
        k_output.awake_counts += 1;
        break;
      case SLEEP_D_LIGHT:
        k_output.light_duration += duration;
        break;
      case SLEEP_D_DEEP:
        k_output.deep_duration += duration;
        break;
      case SLEEP_D_REM:
        k_output.rem_duration += duration;
        break;
      default:
        break;
    }
  }
  k_output.segments = markers;
  k_output.segments_length = markers_length;

  k_output.start = k_output.segments[0].from;
  k_output.end = k_output.segments[k_output.segments_length - 1].to;

  /* check k_depth_feats. */
  if (k_depth_feats_length >= SLEEP_MAX_RECORD_LENGTH) {
    if (k_output.complete_cycle_indicator == T_TEMP) {
      k_output.complete_cycle_indicator = T_COMP;
    }
    SleepInit();
  }

  return &k_output;
}