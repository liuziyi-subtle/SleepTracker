#include "../sleep_tracker.h"
#include "debug.h"

char target_dirs[5000][1000]; /* 作为局部变量定义在stack会过大. */
char target_filenames[5000][1000];

int main(int argc, char** argv) {
  uint16_t i = 0, j = 0;

  char path[1000];

  uint16_t p_count =
      ListFilesRecursively(argv[1], ".csv", target_dirs, target_filenames);

  while (i < p_count) {
    snprintf(path, 1000, "%s/%s", target_dirs[i], target_filenames[i]);

    DebugInit();

    rawdata_t* raw = ReadMTKData(path, 2);

    sleep_tracker_input_t input;

    NonWalkCheck(NULL, NULL, NULL, 0, 0, 1);
    for (j = 0u; j < acc_length; j += 25) {
      NonWalkCheck(&accx[j], &accy[j], &accz[j], 25, 0, 0);
      uint8_t result = NonWalkCheck(NULL, NULL, NULL, 25, 1, 0);
      PLOT.results[PLOT.results_length++] = result;
    }

    snprintf(path, 1000, "%s/%s", argv[2], target_filenames[i]);
    WriteInt(path, PLOT.results, PLOT.results_length);

    i++;
  }

  return 0;
}