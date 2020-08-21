#include "debug.h"

INTERMEDIATES_t PLOT = {0};

static int32_t k_channel0[MAX_LENGTH];
static int32_t k_channel1[MAX_LENGTH];
static int32_t k_channel2[MAX_LENGTH];
static rawdata_t k_raw;

/* Init*/
void DebugInit() {
  PLOT.probs_length = 0;
  PLOT.preds_length = 0;
  PLOT.results_length = 0;
}

/* 获取所有满足target_format格式的文件路径. */
uint16_t ListFilesRecursively(char *base_dir, char *target_format,
                              char (*target_dirs)[1000],
                              char (*target_filenames)[1000]) {
  // char dir[1000];
  // char path[1000];
  struct dirent *dp;
  DIR *dir = opendir(base_dir);

  uint16_t num_targets = 0;

  /* Unable to open directory stream. */
  if (!dir) {
    return num_targets;
  }

  while ((dp = readdir(dir)) != NULL) {
    if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
      /* Construct new path from our base dir. */
      // snprintf(path, 900, "%s/%s", base_dir, dp->d_name);

      if (strstr(dp->d_name, target_format)) {
        strcpy(*target_dirs, base_dir);
        strcpy(*target_filenames, dp->d_name);

        target_dirs += 1;
        target_filenames += 1;
        num_targets += 1;
      }

      char *base_dir_temp[1000];
      snprintf(base_dir_temp, 900, "%s/%s", base_dir, dp->d_name);
      uint16_t num_targets_temp = ListFilesRecursively(
          base_dir_temp, target_format, target_dirs, target_filenames);
      target_dirs += num_targets_temp;
      target_filenames += num_targets_temp;
      num_targets += num_targets_temp;
    }
  }

  closedir(dir);

  return num_targets;
}

rawdata_t *ReadMTKData(const char *path, uint8_t target_value_category_id) {
  uint32_t channel_length = 0;

  FILE *fp_in = fopen(path, "r");

  char *line = malloc(1024);
  while (fgets(line, 1024, fp_in)) {
    uint16_t i = 0;

    char *tok = strtok(line, ",");
    uint8_t value_category_id = (uint8_t)atoi(tok);

    int32_t temp[20];
    uint8_t temp_length = 0u;
    for (tok; tok && *tok; i++, tok = strtok(NULL, ",\n")) {
      temp[temp_length++] = (int32_t)atoi(tok);
    }

    if (value_category_id == target_value_category_id) {
      if (temp_length > 3) {
        k_channel0[channel_length] = temp[0];
        k_channel1[channel_length] = temp[1];
        k_channel2[channel_length] = temp[2];
      } else {
        k_channel0[channel_length] = temp[0];
      }

      channel_length++;
    }
  }

  k_raw.channel0 = k_channel0;
  k_raw.channel1 = k_channel1;
  k_raw.channel2 = k_channel2;
  k_raw.channel_length = channel_length;

  fclose(fp_in);

  return;
}

/* TODO: 此函数目前只支持int32_t和float类型的写入. */
void WriteToFile(const char *path, void *data, uint32_t data_length,
                 uint8_t d_type) {
  FILE *fp = fopen(path, "w");

  if (d_type == 0) {
    int32_t *trans = (int32_t *)data;
    for (uint32_t m = 0; m < data_length; ++m) {
      fprintf(fp, "%d\n", trans[m]);
    }
  } else {
    float *trans = (float *)data;
    for (uint32_t m = 0; m < data_length; ++m) {
      fprintf(fp, "%f\n", trans[m]);
    }
  }

  fclose(fp);
}