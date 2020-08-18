#ifdef GLOBAL_SLEEP_ALGO_OPEN
//
// Created by Ziyi Liu on 2019-04-15.
//

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ls_sleep_tracker.h"

void WriteInt(char *path, int32_t *data, int32_t dataSize) {
  FILE *fp = fopen(path, "w");
  int32_t m;
  for (m = 0; m < dataSize; ++m) {
    fprintf(fp, "%d\n", data[m]);
  }
  fclose(fp);
}

// 写入浮点类型中间变量
void WriteFloat(char *path, float *data, int32_t dataSize) {
  FILE *fp = fopen(path, "w");
  int32_t m;
  for (m = 0; m < dataSize; ++m) {
    fprintf(fp, "%f\n", data[m]);
  }
  fclose(fp);
}

// PLOT: get intermediate variables to plot
#include "ls_sleep_debug.h"
PLOT_STRUC PLOT = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define data_malloc malloc
#define data_free free

int main(int argc, char **argv) {
  char *tok;
  int i = 0;

  DIR *d;
  struct dirent *dir;
  d = opendir(argv[1]);

  if (d) {
    // Exclude ., .. and .DS_Store
    while ((dir = readdir(d)) != NULL) {
      if ((strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0) ||
          (strcmp(dir->d_name, ".DS_Store") == 0)) {
        printf("%s not a file\n", dir->d_name);
        continue;
      }

      // Open txt/csv file to read
      int length = snprintf(NULL, 0, "%s/%s", argv[1], dir->d_name);
      ++length;
      char *pathIn = data_malloc(length);
      snprintf(pathIn, length, "%s/%s", argv[1], dir->d_name);
      FILE *fpIn = fopen(pathIn, "r");
      if (fpIn == NULL) {
        printf("Cannot open file %s !", dir->d_name);
        exit(-1);
      }
      printf("Processing %s\n", pathIn);

      // 写入result的文件.
      // length = snprintf(NULL, 0, "%s/%s", argv[2], dir->d_name);
      // ++length;
      // char *pathOut = malloc(length);
      // snprintf(pathOut, length, "%s/%s", argv[2], dir->d_name);
      // FILE *fpOut = fopen(pathOut, "w");
      // fprintf(fpOut,
      // "sleepTimeUtc,getupTimeUtc,awakeDuration,awakeCount,lightDuration,deepDuration,remDuration,completeCycleIndicator,numSleepSegments,sleepSegments\n");
      // Timer to check result.
      int timer = 0;
      uint32_t utcTime = 1565252014;  // 0808: 1565166775 0809: 1565252014
      // char *sleepSegment2vec = data_malloc(sizeof(char) * 50000);
      // int sleepSegment2vecSize = 0;
      // Initialize algorithm
      // printf("11111111111111111111111111111111111111111111111111111\n");
      LSSleepInitialize();
      // printf("22222222222222222222222222222222222222222222222222222\n");
      // Input data buffer.
      struct LSSleepData *data =
          (struct LSSleepData *)data_malloc(sizeof(struct LSSleepData) * 60);
      uint16_t dataCounter;
      // 取出睡眠深度结果
      struct LSSleepResult *result =
          (struct LSSleepResult *)data_malloc(sizeof(struct LSSleepResult));
      // result->numSleepSegments = 0;
      // result->completeCycleIndicator = T_NONE;
      int counter = 0;
      // Skip the first line
      char *line = data_malloc(sizeof(char) * 1024);
      fgets(line, 1024, fpIn);
      fgets(line, 1024, fpIn);

      while (fgets(line, 1024, fpIn)) {
        for (tok = strtok(line, ","); tok && *tok;
             i++, tok = strtok(NULL, ",\n")) {
          switch (i) {
            case (0):
              // data[dataCounter].hr = (uint8_t) atoi(tok);
              break;
            case (1):
              data[dataCounter].accX = (uint8_t)atoi(tok);
              break;
            case (2):
              data[dataCounter].accY = (uint8_t)atoi(tok);
              break;
            case (3):
              data[dataCounter].accZ = (uint8_t)atoi(tok);
              break;
            case (4):
              data[dataCounter].hrSwitch = (bool)atoi(tok);
            case (5):
              data[dataCounter].hrWearIndicator = (bool)atoi(tok);
              // data[dataCounter].hrWearIndicator = true;
              break;
            default:
              continue;
          }
        }
        i = 0;

        if (++dataCounter % 60 == 0) {
          utcTime += 60;
          LSSleepAnalyzeData(data, 60, utcTime, false);
          LSSleepGetResult(result);
          timer++;

          // 记录清醒后的睡眠深度
          if (result->completeCycleIndicator == 2) {
            PLOT.sleepBuf[PLOT.sleepBufLen++] = result->sleepTimeUtc;
            PLOT.getupBuf[PLOT.getupBufLen++] = result->getupTimeUtc;
            //                        for (uint16_t j = 0; j <
            //                        result->numSleepSegments; ++j) {
            //                            for (uint32_t k = 0; k <
            //                            result->sleepSegments[j].duration;
            //                            ++k) {
            //                                PLOT.depthBuf[PLOT.depthBufLen++]
            //                                = result->sleepSegments[j].depth;
            //                            }
            //                        }
          }
          dataCounter = 0;
        }
      }

#ifdef DEBUG_LOCAL
      char *fileNamePrefix = strtok(dir->d_name, ".");

      // 写入入睡时间
      char *sleepPath = data_malloc(256 * sizeof(char));
      sprintf(sleepPath, "%s/%s_sleep.csv", argv[2], fileNamePrefix);
      WriteInt(sleepPath, PLOT.sleepBuf, PLOT.sleepBufLen);
      PLOT.sleepBufLen = 0;
      data_free(sleepPath);

      // 写入起床时间
      char *getupPath = data_malloc(256 * sizeof(char));
      sprintf(getupPath, "%s/%s_getup.csv", argv[2], fileNamePrefix);
      WriteInt(getupPath, PLOT.getupBuf, PLOT.getupBufLen);
      PLOT.getupBufLen = 0;
      data_free(getupPath);

      // 写入x特征
      //            char* xPath = data_malloc(256 * sizeof(char));
      //            sprintf(xPath, "%s/%s_x.csv", argv[2], fileNamePrefix);
      //            WriteInt(xPath, PLOT.xBuf, PLOT.xBufLen);
      //            PLOT.xBufLen = 0;
      //            data_free(xPath);
      //
      //            // 写入y特征
      //            char* yPath = data_malloc(256 * sizeof(char));
      //            sprintf(yPath, "%s/%s_y.csv", argv[2], fileNamePrefix);
      //            WriteInt(yPath, PLOT.yBuf, PLOT.yBufLen);
      //            PLOT.yBufLen = 0;
      //            data_free(yPath);
      //
      //            // 写入z特征
      //            char* zPath = data_malloc(256 * sizeof(char));
      //            sprintf(zPath, "%s/%s_z.csv", argv[2], fileNamePrefix);
      //            WriteInt(zPath, PLOT.zBuf, PLOT.zBufLen);
      //            PLOT.zBufLen = 0;
      //            data_free(zPath);

      // 写入睡眠状态
      char *statusPath = data_malloc(256 * sizeof(char));
      sprintf(statusPath, "%s/%s_status.csv", argv[2], fileNamePrefix);
      WriteInt(statusPath, PLOT.statusBuf, PLOT.statusBufLen);
      PLOT.statusBufLen = 0;
      data_free(statusPath);
      //
      //            // 写入睡眠深度
      //            char* depthPath = data_malloc(256 * sizeof(char));
      //            sprintf(depthPath, "%s/%s_depth.csv", argv[2],
      //            fileNamePrefix); WriteInt(depthPath, PLOT.depthBuf,
      //            PLOT.depthBufLen); printf("PLOT.depthBufLen: %d\n",
      //            PLOT.depthBufLen); PLOT.depthBufLen = 0;
      //            data_free(depthPath);

      // 写入佩戴检测状态
      char *wearIndPath = data_malloc(256 * sizeof(char));
      sprintf(wearIndPath, "%s/%s_wearInd.csv", argv[2], fileNamePrefix);
      WriteInt(wearIndPath, PLOT.wearIndBuf, PLOT.wearIndBufLen);
      PLOT.wearIndBufLen = 0;
      data_free(wearIndPath);
#endif  // DEBUG_LCOAL

      data_free(pathIn);
      data_free(line);
      data_free(data);
      data_free(result);

      fclose(fpIn);
    }
    closedir(d);
  }

  return 0;
}
#endif
