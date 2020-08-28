/*
 * MIT License
 *
 * Copyright (c) 2020 Liu Ziyi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
    uint8_t *x;
    uint8_t *y;
    uint8_t *z;
  } acc_t;
  uint8_t nonwear; /*<< 1-nonwear & 0-wear */
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