#ifdef GLOBAL_SLEEP_ALGO_OPEN
/**
* Copyright [2019] <Ziyi Liu>.
*/
#ifdef DEBUG_LOCAL

#include <stdio.h>

#define LARGE_ARRAY_SIZE (50000)
#define SMALL_ARRAY_SIZE (10000)

typedef struct {
    float stdPreBuf[LARGE_ARRAY_SIZE];
    int stdPreBufLen;

    float stdPosBuf[LARGE_ARRAY_SIZE];
    int stdPosBufLen;

    int xBuf[LARGE_ARRAY_SIZE];
    int xBufLen;

    int yBuf[LARGE_ARRAY_SIZE];
    int yBufLen;

    int zBuf[LARGE_ARRAY_SIZE];
    int zBufLen;

    int hrBuf[LARGE_ARRAY_SIZE];
    int hrBufLen;

    int predBuf[LARGE_ARRAY_SIZE];
    int predBufLen;

    int statusBuf[LARGE_ARRAY_SIZE];
    int statusBufLen;

    int sleepMarkerBuf[LARGE_ARRAY_SIZE];
    int sleepMarkerBufLen;

    int activityBuf[LARGE_ARRAY_SIZE];
    int activityBufLen;

    int countBuf[LARGE_ARRAY_SIZE];
    int countBufLen;

    int depthBuf[LARGE_ARRAY_SIZE];
    int depthBufLen;

    int postDepthBuf[LARGE_ARRAY_SIZE];
    int postDepthBufLen;

    int sleepBuf[LARGE_ARRAY_SIZE];
    int sleepBufLen;

    int getupBuf[LARGE_ARRAY_SIZE];
    int getupBufLen;

    int wearIndBuf[LARGE_ARRAY_SIZE];
    int wearIndBufLen;
} PLOT_STRUC;

extern PLOT_STRUC PLOT;

#endif  // DEBUG_LOCAL

#endif
