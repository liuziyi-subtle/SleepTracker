/*
 * Copyright(c) Liu Ziyi. Licensed under the MIT license.
 */

#include <stdint.h>

typedef enum {
  T_NONE = 0, /*<< no sleep */
  T_TEMP = 1, /*<< temporal sleep */
  T_COMP = 2, /*<< complete sleep */
} sleep_terminator_t;

/* description of a sleep segment. */
typedef struct {
  uint32_t from;
  uint32_t to;
  uint8_t depth;
} sleep_marker_t;

/* input */
typedef struct {
  uint32_t utc;
  int32_t sample_length;
  struct {
    int16_t *x;
    int16_t *y;
    int16_t *z;
  } acc_t;
  uint8_t nonwear; /*<< 0 not wear, 1 wear */
} sleep_tracker_input_t;

/* output */
typedef struct {
  uint32_t start; /*<< sleep time */
  uint32_t end;   /*<< bed time */
  uint32_t awake_counts;
  uint32_t awake_duration;
  uint32_t rem_duration;
  uint32_t light_duration;
  uint32_t deep_duration;

  sleep_marker_t *segments;
  uint16_t segments_length;

  sleep_terminator_t complete_cycle_indicator;
} sleep_tracker_output_t;

void SleepInput(sleep_tracker_input_t *input);
sleep_tracker_output_t *SleepOutput();