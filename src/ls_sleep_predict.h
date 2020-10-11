#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#if defined(__clang__) || defined(__GNUC__)
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

union Entry {
  int missing;
  float fvalue;
  int qvalue;
};

struct Node {
  uint8_t default_left;
  unsigned int split_index;
  int threshold;
  int left_child;
  int right_child;
};

extern const unsigned char is_categorical[];

size_t get_num_output_group(void);
size_t get_num_feature(void);
float predict_sleep_status(union Entry* data, int pred_margin);

