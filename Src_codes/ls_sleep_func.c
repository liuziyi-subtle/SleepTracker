/**
* Copyright [2019] <Ziyi Liu>.
*/
#include "ls_sleep_func.h"
#include "ls_sleep_predict.h"
#include "ls_sleep_tracker.h"


// 当前文件打印输出宏定义
#ifdef SLEEPALGO_FUNC_OUTPUT_LOG_ENABLE
#define LOCAL_DEBUG(...) log_i(__VA_ARGS__)
#else
#define LOCAL_DEBUG(...)
#endif


//=======================宏定义=============================

#include <stdio.h>
#define data_malloc m_sleep_malloc_cb_fun
#define data_free   m_sleep_free_cb_fun


static uint8_t gActivityBlock[BLOCK_LIMIT];
static uint8_t gCountBlock[BLOCK_LIMIT];
static uint8_t gDepthBlock[BLOCK_LIMIT];
static uint8_t gPostDepthBlock[BLOCK_LIMIT];


// 睡眠算法内存管理
static alg_sleep_malloc_cb_t m_sleep_malloc_cb_fun = NULL;
static alg_sleep_free_cb_t   m_sleep_free_cb_fun = NULL;
bool alg_sleep_malloc_reg(alg_sleep_malloc_cb_t fun)
{
    m_sleep_malloc_cb_fun	 = fun;
    return true;
}
bool alg_sleep_free_reg(alg_sleep_free_cb_t fun)
{
    m_sleep_free_cb_fun = fun;
    return true;
}
void* alg_sleep_malloc(uint32_t size) {

    void *ptr = NULL;
    if (m_sleep_malloc_cb_fun != NULL) {
        ptr = m_sleep_malloc_cb_fun(size);
    } else {
        ptr = malloc(size);
    }
    return ptr;
}
void alg_sleep_free(void* p) {

    if (m_sleep_free_cb_fun != NULL) {
        m_sleep_free_cb_fun(p);
    } else {
        free(p);
    }
}


/**
* 功能:动态计算标准差
* 输入:顺序输入数组的单个数据样本
* 输出:截止数组中该样本处的标准差
*/
float OnePassStd(const float x, bool init) {
    static double lastSum, lastSumSquare;
    static int len;

    if (init) {
        lastSum       = .0;
        lastSumSquare = .0;
        len           =  0;
        return .0;
    }

    double sum, sumSquare, mean, variance;

    if (++len == 1) {
        lastSum = (double) x;
        lastSumSquare = pow((double)x, 2);
        return .0;
    }

    sum = lastSum + (double) x;
    sumSquare = lastSumSquare + pow((double) x, 2);
    mean = sum / len;

    variance = (sumSquare - len * pow(mean, 2)) / len;

    lastSum = sum;
    lastSumSquare = sumSquare;

    if (variance == .0) {
        return .0;
    } else {
        return (float) sqrt(variance);
    }
}


/**
* 平滑睡眠检测结果
*/
bool WindowAverage(uint8_t *window, uint32_t windowLen) {
    uint32_t i;
    uint8_t result = 0;
    for (i = 0; i < windowLen; ++i) {
        result += window[i];
    }

    return (result > 6)?true:false;
}

/**
* 平滑睡眠检测结果 By James
*/
bool WindowAverage_j(const uint8_t win[], uint8_t ind, uint32_t size, uint32_t cur_utc, uint32_t* utc_sleep_end)
{
    *utc_sleep_end = cur_utc;
    uint32_t sum=0;
    uint32_t start_ind = (ind == 0) ? (size - 1) : (ind - 1);
    uint32_t cur_ind;
#define SUM_WIN_LIM 4
    for(uint32_t i=0; i<size; i++)
    {
        cur_ind = (start_ind + size - i) % size;
        sum += win[cur_ind];
        if(sum == 1)
        {
            *utc_sleep_end = cur_utc - i*60;
            //printf( " end_sleep: %d ", *utc_sleep_end/60 );
        }
    }

    return (sum >= SUM_WIN_LIM)?true:false;;
}

/**
* 检测睡眠结果
*/
float PredictSleepStatus(const float* features) {
    const size_t num_feature = get_num_feature();
    union Entry* data = data_malloc(sizeof(union Entry) * num_feature);
    float outProbability;

    // Parse features
    size_t i;
    for (i = 0; i < num_feature; ++i) {
        data[i].fvalue = features[i];
    }

    outProbability = predict_sleep_status(data, 0);

    data_free(data);

    return outProbability;

    // if (outProbability >= 0.5f) {
    //     return 1;
    // } else {
    //     return 0;
    // }
}


/*
 睡眠检测算法的初步滤波
 1. 连接相隔很小的两个睡眠片段
 2. 去除与前后相差远的睡眠片段

uint16_t RemoveNoiseMarker(uint16_t *sleepMarkerBuf,
    uint16_t *sleepMarkerBufLen) {
    // 分别指向第一个end和第二个start
    uint16_t *p = sleepMarkerBuf;
    // uint16_t *q = p + 2;
    uint16_t *q = p;

    uint16_t len = *sleepMarkerBufLen;

    while (q != &sleepMarkerBuf[*sleepMarkerBufLen - 2]) {
        if (*(q) - *(p + 1) < 3 && *(q) - *(p + 1) > 0) {
            // 融合p+1指向的end和q+1指向的end
            *(p + 1) = *(q + 1);
            // q后移两位
            q = q + 2;
            // q前两位置为一个大数
            *(q - 1) = 10000;
            *(q - 2) = 10000;
            // 有效的长度减2
            len = len - 2;
        } else {
            // p跳到q位上
            p = q;
            // q继续后移动
            q = p + 2;
        }
    }

    // Move elements of 10000 to the end of sleepMarkerBuf.
    qsort(sleepMarkerBuf, (size_t) (*sleepMarkerBufLen),
        sizeof(uint16_t), Compare_U16);

    *sleepMarkerBufLen = len;

    // 由于传入的sleepMarkerBufLen为偶数，所以经过上面去除噪声marker后的长度
    // *sleepMarkerBufLen也还是为偶数，所以可以按如下计算
    uint16_t totalSleepDuration = 0;
    for (uint16_t i = 0; i < *sleepMarkerBufLen; i += 2) {
        totalSleepDuration += sleepMarkerBuf[i + 1] - sleepMarkerBuf[i];
    }

    return totalSleepDuration;
}
*/

float FindMedian(uint8_t *nums, uint16_t numsSize) {
    int temp;
    int i, j;
    // Sort the array nums in ascending order
    for(i = 0; i < numsSize - 1; i++) {
        for(j = i + 1; j < numsSize; j++) {
            if(nums[j] < nums[i]) {
                // swap elements
                temp = nums[i];
                nums[i] = nums[j];
                nums[j] = temp;
            }
        }
    }
    if(numsSize % 2 == 0) {
        // If there is an even number of elements,
        // return mean of the two elements in the middle
        // printf("nums[numsSize/2]: %u, nums[numsSize/2-1]: %u\n", nums[numsSize/2], nums[numsSize/2-1]);
        return (((float) (nums[numsSize/2] + nums[numsSize/2-1])) / (double)2.0);
    } else {
        // Else return the element in the middle
        return (float) nums[numsSize/2];
    }
}

uint16_t RemoveNoiseMarker(uint16_t *sleepMarkerBuf,
                           uint16_t *sleepMarkerBufLen) {
    uint16_t tempMarkerBufSize = *sleepMarkerBufLen;
    // 如果sleepMarkerBuf没有内容.
    if (tempMarkerBufSize == 0) {
        return 0;
    }

    // 如果sleepMarkerBuf仅包含一对起始点则返回.
    if (tempMarkerBufSize == 2) {
        return sleepMarkerBuf[1] - sleepMarkerBuf[0];
    }

    // TODO(Ziyi Liu): 强制修正gSleepMarkerBufLen <= MAX_SLEEP_MARKER_LEN.
    // 如果包含两对及以上.
    uint16_t i = 2, j = 0;
    int16_t k = 0;
    uint16_t preDuration = 0;
    while (i < tempMarkerBufSize) {
        // 计算当前点之前的所有marker pair的总和
        while (j < i) {
            preDuration += (sleepMarkerBuf[j+1] - sleepMarkerBuf[j]);
            j += 2;
        }

        // 如果当前起始marker距离前一个结束marker的preDuration>= 1h，且前面marker之和 < 1h.
        // 如果当前起始marker距离前一个结束marker的preDuration >= 0.5h, 且前面marker之和 < 0.5h.
        // 如果当前起始marker距离前一个结束marker的preDuration >= 10min, 且前面marker之和 < 10min.
        uint16_t gap = sleepMarkerBuf[i] - sleepMarkerBuf[i-1];
        if ((gap >= 10 && preDuration < 10) || (gap >= 30 && preDuration < 30) || (gap >= 60 && preDuration < 60)) {
            k = (int16_t) i;
            while (k--) {
                // 数组前移至当前marker为起点.
                for (j = 0; j < tempMarkerBufSize; ++j) {
                    sleepMarkerBuf[j] = sleepMarkerBuf[j+1];
                }
            }
            tempMarkerBufSize -= i;
            i = 2;
        } else {
            i += 2;
        }

        preDuration = 0;
        j = 0;
    }
    *sleepMarkerBufLen = tempMarkerBufSize;

    uint16_t totalSleepDuration = 0;
    for (i = 0; i < *sleepMarkerBufLen; i += 2) {
        totalSleepDuration += (sleepMarkerBuf[i+1] - sleepMarkerBuf[i]);
    }

    return totalSleepDuration;
}


/**
* 计算一分钟是否出现了一次activity
*/
void ComputeActivity(const float *sleepDepthFeatureBuf,
    uint16_t sleepDepthFeatureBufLen, uint8_t *activityBlock) {
    float dynamicStd;
    uint8_t activity;
    uint16_t i;

    OnePassStd(0, true);
    for (i = 0; i < sleepDepthFeatureBufLen; ++i) {
        dynamicStd = OnePassStd(sleepDepthFeatureBuf[i], false);
        /* TODO::0.1设置成宏常量 */
        activity = (sleepDepthFeatureBuf[i] > 0.1f * dynamicStd)?1:0;
        activityBlock[i] = activity;
    }
}

/**
* 计算每一分钟周围出现了多少次activity
* TODO::函数中的window是否可以直接用全局变量.
*/
void ComputeCount(uint8_t *activityBlock, uint16_t blockLen,
    uint8_t windowSize, uint8_t *countBlock) {
    uint8_t *window = (uint8_t*) data_malloc (sizeof(uint8_t) * windowSize);
    uint16_t i;
    uint8_t sum;
    uint8_t ptr;

    // 假设在进入睡眠之前每分钟都有1次activity
    for (i = 0; i < windowSize; ++i) {
        window[i] = 1;
    }
    sum = windowSize;

    ptr = 0;
    for (i = 0; i < blockLen; ++i) {
        // 加上最新的activity，减去最旧的activity
        sum += activityBlock[i];
        sum -= window[ptr];
        window[ptr] = activityBlock[i];

        if (++ptr == windowSize) {
            ptr = 0;
        }

        countBlock[i] = sum;
    }

    data_free(window);
}


/**
* 8位无符号数组快排
*/
int Compare_U8(void const* a, void const* b) {
    uint8_t arg1 = *(const uint8_t*) (a);
    uint8_t arg2 = *(const uint8_t*) (b);

    if (arg1 < arg2) {
        return -1;
    }

    if (arg1 > arg2) {
        return 1;
    }

    return 0;
}


/**
* 16位无符号数组快排
*/
int Compare_U16(void const* a, void const* b) {
    uint16_t arg1 = *(const uint16_t*) (a);
    uint16_t arg2 = *(const uint16_t*) (b);
    if (arg1 < arg2) {
        return -1;
    }

    if (arg1 > arg2) {
        return 1;
    }

    return 0;
}


/*
 计算分位数
*/
uint8_t GetPercentile(const uint8_t *block, uint16_t len, uint8_t percentile) {
    uint8_t *block_copy = (uint8_t*) data_malloc (sizeof(uint8_t) * len);
    for (uint16_t i = 0; i < len; ++i) {
        block_copy[i] = block[i];
    }

    // 排序:快排
    qsort(block_copy, (size_t) len, sizeof(uint8_t), Compare_U8);

    uint16_t index = (uint16_t) round(0.01 * percentile * len);

    uint8_t p = block_copy[index];

    data_free(block_copy);

    return p;
}

/**
* 计算三层次睡眠
*/
void Compute3StageDepth(const uint8_t *countBlock, const uint16_t blockLen,
    const uint16_t *sleepMarkerBuf, const uint16_t sleepMarkerBufLen,
    uint8_t* depthBlock) {
    uint16_t start;
    uint16_t end;
    uint8_t depth;

    // 计算上下分位数
    // TODO(Liu Ziyi): 1. 分位数的范围改成宏常量
    // 2. 确认lowPercentile和highPercentile用浮点数还是整数合适
    uint8_t lowPercentile  = GetPercentile(countBlock, blockLen, 20);
    uint8_t highPercentile = GetPercentile(countBlock, blockLen, 80);

    uint16_t i, j;
    for (i = 0; i < blockLen; ++i) {
        depthBlock[i] = D_AWAKE;
    }

    for (i = 0; i < sleepMarkerBufLen / 2; ++i) {
        start = sleepMarkerBuf[2 * i];
        end   = sleepMarkerBuf[2 * i + 1];

        for (j = start; j <= end; ++j) {
            if (countBlock[j] <= lowPercentile) {
                depth = D_DEEP;
            } else if (countBlock[j] >= highPercentile) {
                depth = D_REM;
            } else {
                depth = D_LIGHT;
            }
            depthBlock[j] = depth;
        }
    }
}


/**
* 寻找数组中出现次数最多的元素，用于寻找窗口内出现次数最多的睡眠深度
*/
uint8_t MostFrequent(const uint8_t *srcArr, uint8_t arrLen) {
    uint8_t *arr = (uint8_t*) data_malloc (sizeof(uint8_t) * arrLen);
    memcpy(arr, srcArr, sizeof(uint8_t) * arrLen);
    qsort(arr, arrLen, sizeof(uint8_t), Compare_U8);

    // 线性遍历
    uint8_t max_count = 1, res = arr[0], curr_count = 1;
    for (uint8_t i = 1; i < arrLen; i++) {
        if (arr[i] == arr[i - 1]) {
            curr_count++;
        } else {
            if (curr_count > max_count) {
                max_count = curr_count;
                res = arr[i - 1];
            }
            curr_count = 1;
        }
    }

    // 如果最后一个元素满足条件
    if (curr_count > max_count) {
        // max_count = curr_count;
        res = arr[arrLen - 1];
    }
    data_free(arr);

    return res;
}

/** 
* 补丁函数: 针对手表放置于桌面上被判断为睡眠的情况进行识别
*/
/** 
* 补丁函数: 针对手表放置于桌面上被判断为睡眠的情况进行识别
*/
/*
bool CheckWearingStepOne(uint8_t x, uint8_t y, uint8_t z, bool init) {
    static uint16_t timer;
    static uint8_t lastX, lastY, lastZ;

    if (init) {
        timer = 10;
        lastX = 0;
        lastY = 0;
        lastZ = 0;
    }

    uint8_t diffX, diffY, diffZ;

    diffX = abs(x - lastX);
    diffY = abs(y - lastY);
    diffZ = abs(z - lastZ);

    if ((diffX <= 2) && (diffY <= 2) && (diffZ <= 2)) {
        if (timer > 0) {
            timer--;
        } else {
            timer = 0;
        }
    } else {
        timer = 10;
    }

    lastX = x;
    lastY = y;
    lastZ = z;

    // printf("diffX: %u, diffY: %u, diffZ: %u, timer: %u\n", diffX,
    //     diffY, diffZ, timer);
    
    if (timer == 0) {
        return true;
    } else {
        return false;
    }
}


bool CheckWearingStepTwo(uint8_t *x, uint8_t *y, uint8_t *z, uint16_t len) {

    uint16_t i;
    uint8_t diffX, diffY, diffZ;
    uint8_t wearFlag = 0;
    for (i = 1; i < len; ++i) {
        diffX = abs(x[i] - x[i-1]);
        diffY = abs(y[i] - y[i-1]);
        diffZ = abs(z[i] - z[i-1]);

        if ((diffX < MAX_DIFFERENCE) && (diffY < MAX_DIFFERENCE) && (diffZ < MAX_DIFFERENCE)) {
            wearFlag += 1;
        }
    }

    if (wearFlag > len - 4) {
        return true;
    } else {
        return false;
    }
}
*/

bool SecondRoundCheckWear(void) {
    return false;
}
/*
第二轮佩戴检测。
bool SecondRoundCheckWear(float *Sum2CountBuf, uint16_t markerOn, uint16_t markerOff) {
    return false;
}
bool CheckWear(float *sum2CountBuf, uint16_t sum2CountBufSize, uint16_t *sleepMarkerBuf, uint16_t *sleepMarkerBufSize) {

    uint16_t i;
    // 计算markerDuration
    uint16_t markerDuration = markerOff - MarkerOn;
    if (markerDuration <= 30 && sleepMarkerBufSize == 2) {
        // 小于30分钟时，且只存在一对marker，如果出现连续大动作点则删除这一对marker。
        for (i = 0; i < markerDuration; ++i) {
            float sum2Count = sum2CountBuf[sum2CountBufSize - 1 - i];
            float lastSum2Count = sum2CountBuf[sum2CountBufSize - 2 - i];
            if (sum2Count > SUCCESSIVE_MOTION &&
            lastSum2Count > SUCCESSIVE_MOTION) {
                sleepMarkerBufSize = sleepMarkerBufSize - 2;
            }
        }
    } else {
        // 统计markerDuration内NON_MOTION的点数。
        // 首先找到离markerOff最近的NON_MOTION点。
        for (i = 0; i < markerDuration; ++i) {
            if (sum2CountBuf[sum2CountBufSize - 1 - i] < NON_MOTION) {
                break;
            }
        }
        uint16_t correctedMarkerOff = i;
        uint16_t correctedMarkerDuration = correctedMarkerOff - markerOn;
        uint16_t numNonMotion = 0;
        // 统计从markerOn到位置i之间的sum2Count < NON_MOTION的数目。
        for (i = markerOn; i < correctedMarkerOff; ++i) {
            if (sum2CountBuf[i] < NON_MOTION) {
                numNonMotion++;
            }
        }

        if (numNonMotion > coorectedMarkerDuration - coorectedMarkerDuration / 30) {
            sleepMarkerBufSize = sleepMarkerBufSize - 2;
        }
    }

    // 如果超过120分钟，寻找这一对marker中距离当前点最近的一个连续大动作点，并且寻找这个连续
    // 大动作点之前最近的一个NON_MOTION点。暂时将sleepMarkerBuf中最后一个marker更新到这个点
    // 上。
    if (markerDuration > 120) {
        // 查看是否存在连续大动作点
    }
    // 计算marker内

    // 如果小于半小时，设定开始条件

    // 如果小于2h，一般条件

    // 如果大于2h，设定结束条件

    float sum2Count = sum2CountBuf[sum2CountBufSize - 1];
    float lastSum2Count = sum2CountBuf[sum2CountBufSize - 2];

    if ((sum2Count > SUCCESSIVE_MOTION) &&
    (lastSum2Count > SUCCESSIVE_MOTION)) {

    }


}
*/


/*
 对睡眠深度进行后处理，平滑结果
 TODO(Liu Ziyi): 将windowSize改为宏常量.
*/
void ComputePostDepth(uint8_t *depthBlock, uint16_t blockLen,
    uint8_t windowSize, uint8_t *postDepthBlock) {
    uint8_t *window = (uint8_t*) data_malloc (sizeof(uint8_t) * windowSize);
    uint8_t ptr = 0;
    uint8_t depth;

    // TODO(Liu Ziyi): 这里是将window中的值初始化为清醒还是浅睡合适?
    for (uint8_t j = 0; j < windowSize; ++j) {
        window[j] = D_LIGHT;
    }

    // 向window中加入睡眠深度，统计window中各睡眠深度出现的次数
    for (uint16_t i = 0; i < blockLen; ++i) {
        window[ptr] = depthBlock[i];
        if (++ptr == windowSize) {
            ptr = 0;
        }

        // 寻找window中出现次数最多的睡眠深度
        depth = MostFrequent(window, windowSize);
        postDepthBlock[i] = depth;
    }

    data_free(window);
}


uint16_t ComputeSleepDepth(const float *sleepDepthFeatBuf,
    const uint16_t sleepDepthFeatBufLen, const uint16_t *sleepMarkerBuf,
    const uint16_t sleepMarkerBufLen, uint16_t *sleepCycleDepth,
    const uint16_t sleepCycleDepthSize) {

    if (sleepDepthFeatBufLen > BLOCK_LIMIT) {
        return 0;
    }

    // 计算activity
    ComputeActivity(sleepDepthFeatBuf, sleepDepthFeatBufLen, gActivityBlock);
    // 计算count
    ComputeCount(gActivityBlock, sleepDepthFeatBufLen, 13, gCountBlock);
    // 计算depth
    Compute3StageDepth(gCountBlock, sleepDepthFeatBufLen, sleepMarkerBuf,
        sleepMarkerBufLen, gDepthBlock);
    // 平滑结果
    ComputePostDepth(gDepthBlock, sleepDepthFeatBufLen, 13, gPostDepthBlock);

    uint16_t sleepCycleDepthLen = 0;
    uint16_t start = 0, end = 0;
    uint8_t depth, lastDepth = gPostDepthBlock[0];
    uint16_t i;
    for (i = 0; i < sleepDepthFeatBufLen; ++i) {
        // 越界检查: 如果sleepCycleDepthLen大于sleepCycleDepthSize，则将最后一个segment
        // 改变为浅睡，结束时刻延长至sleepDepthFeatBufLen-1.
        if (sleepCycleDepthLen >= sleepCycleDepthSize) {
            sleepCycleDepth[sleepCycleDepthLen - 2] = D_LIGHT;
            sleepCycleDepth[sleepCycleDepthLen - 1] = sleepDepthFeatBufLen - 1;
            break;
        }

        depth = gPostDepthBlock[i];

        if (depth != lastDepth || (i == (sleepDepthFeatBufLen - 1))) {
            sleepCycleDepth[sleepCycleDepthLen++] = start;
            sleepCycleDepth[sleepCycleDepthLen++] = (uint16_t) lastDepth;

            if (i == (sleepDepthFeatBufLen - 1)) {
                sleepCycleDepth[sleepCycleDepthLen++] = end + 1;
            } else {
                sleepCycleDepth[sleepCycleDepthLen++] = end;
            }
            start = i;
            end = start;
        } else {
            end = i;
        }

        lastDepth = depth;
    }

    return sleepCycleDepthLen;
}

void LSSleepFuncInitialize(void) {
    uint16_t i;
    for (i = 0; i < BLOCK_LIMIT; ++i) {
        gActivityBlock[i] = 0;
        gCountBlock[i] = 0;
        gDepthBlock[i] = 0;
        gPostDepthBlock[i] = 0;
    }
    OnePassStd(0, true);
    // CheckWearingStepOne(0, 0, 0, true);

    return;
}

