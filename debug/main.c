#include "../sleep_tracker.h"
#include "debug.h"

char target_dirs[5000][1000]; /* 作为局部变量定义在stack会过大. */
char target_filenames[5000][1000];

extern INTERMEDIATES_t PLOT;

int main(int argc, char** argv) {
  uint16_t i = 0, j = 0;

  char path[1000];

  uint16_t p_count =
      ListFilesRecursively(argv[1], ".csv", target_dirs, target_filenames);

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

    uint32_t j = 0;
    while (j < raw->channel_length) {
      sleep_tracker_input_t input = {
          1598003086, 60, {&acc_x[j], &acc_y[j], &acc_z[j]}, 0};
      SleepInput(&input);

      sleep_tracker_output_t* output = SleepOutput();
      if (output->complete_cycle_indicator == T_COMP) {
        PLOT.outputs[PLOT.outputs_length++] = {(int32_t)output->start,
                                               (int32_t)output->end,
                                               (int32_t)output->awake_counts,
                                               (int32_t)output->awake_duration,
                                               (int32_t)output->rem_duration,
                                               (int32_t)output->light_duration,
                                               (int32_t)output->deep_duration,};
      }

      j += 60; /* 每60秒调用一次 */
    }

    /* 写入结果. */
    snprintf(path, 1000, "%s/%s", argv[2], target_filenames[i]);
    Write2File(path, PLOT.results, PLOT.results_length, 0);

    i++;
  }

  return 0;
}