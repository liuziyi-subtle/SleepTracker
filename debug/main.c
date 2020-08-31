#include "../sleep_tracker.h"
#include "debug.h"

char target_dirs[5000][1000]; /* 作为局部变量定义在stack会过大. */
char target_filenames[5000][1000];

// extern ALGO_DEBUG_DATA_t PLOT;

/*
 * Usage:
 * ./executable
 * /Users/liuziyi/Documents/Lifesense/data/sleep-tracker/raw/20200831
 * /Users/liuziyi/Workspace/sleep-tracker/432-dev/results
 */
int main(int argc, char** argv) {
  char path[1000];

  uint16_t p_count =
      ListFilesRecursively(argv[1], ".csv", target_dirs, target_filenames);

  uint16_t i = 0;
  while (i < p_count) {
    snprintf(path, 1000, "%s/%s", target_dirs[i], target_filenames[i]);

    DebugInit();

    rawdata_t* raw = ReadMTKData(path, 2);
    uint8_t acc_x[raw->channel_length];
    uint8_t acc_y[raw->channel_length];
    uint8_t acc_z[raw->channel_length];

    /* 转换数据精度并降采样至1hz. */
    Convert2SleepData(raw->channel0, raw->channel_length, acc_x);
    Convert2SleepData(raw->channel1, raw->channel_length, acc_y);
    Convert2SleepData(raw->channel2, raw->channel_length, acc_z);

    uint32_t utc = 1598003086;
    uint32_t j = 0;
    while (j < raw->channel_length) {
      sleep_tracker_input_t input = {
          utc, 60, {&acc_x[j], &acc_y[j], &acc_z[j]}, 0};
      SleepInput(&input);

      sleep_tracker_output_t* output = SleepOutput();

      /* TODO: save results as json formats. */
      if (output->complete_cycle_indicator == T_COMP) {
        for (uint32_t i = 0; i < output->segments_length; i++) {
          sleep_marker_t* segment = &output->segments[i];
          uint32_t segment_duration = segment->to - segment->from;
          uint8_t depth = segment->depth;
          for (uint32_t k = 0; k < segment_duration; k++) {
            PLOT.depths[PLOT.depths_length++] = depth;
          }
        }
      }

      utc += 60;
      j += 60; /* 每60秒调用一次 */
    }

    /* 写入结果. */
    snprintf(path, 1000, "%s/%s", argv[2], target_filenames[i]);
    Write2File(path, PLOT.results, PLOT.results_length, 0);

    i++;
  }

  return 0;
}