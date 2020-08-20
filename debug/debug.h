#ifdef ALGO_DEBUG

#include <stdint.h>

#define MAX_LENGTH (1000000)

typedef struct {
  float probas[MAX_LENGTH];
  uint32_t probas_length;

  float preds[MAX_LENGTH];
  uint32_t preds_length;

  uint8_t results[MAX_LENGTH];
  uint32_t results_length;
} INTERMEDIATES;

extern INTERMEDIATES PLOT;

void DebugInit();

uint16_t ListFilesRecursively(char *base_dir, char *target_format,
                              char (*target_dirs)[1000],
                              char (*target_filenames)[1000]);

uint16_t ReadMTKData(const char *path, uint8_t value_category_id,
                     int16_t accx[], int16_t accy[], int16_t accz[],
                     uint16_t length);

#endif /* ALGO_DEBUG */