
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define MAX_LENGTH (1000000)

typedef struct {
  float probs[MAX_LENGTH];
  uint32_t probs_length;

  float preds[MAX_LENGTH];
  uint32_t preds_length;

  uint8_t results[MAX_LENGTH];
  uint32_t results_length;
} INTERMEDIATES_t;

/* 可以扩展. */
typedef struct {
  uint32_t channel_length;
  int32_t* channel0;
  int32_t* channel1;
  int32_t* channel2;
} rawdata_t;

typedef enum { u8, u16, u32, i8, i16, i32, f32, f64 } data_type_t;

void DebugInit();

uint16_t ListFilesRecursively(char* base_dir, char* target_format,
                              char (*target_dirs)[1000],
                              char (*target_filenames)[1000]);

rawdata_t* ReadMTKData(const char* path, uint8_t target_value_category_id);
