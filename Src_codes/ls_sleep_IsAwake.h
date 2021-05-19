/**
 * @file ls_sleep_IsAwake.h
 *
 * @brief Header file for the wake identifier.
 *
 * @author Wen Wang
 *
 * @copyright Lifesense 2021-.
 */


#ifndef __LS_SLEEP_ISAWAKE_H__
#define __LS_SLEEP_ISAWAKE_H__

#define SLEEP_ENABLE_DEBUG 0
#if SLEEP_ENABLE_DEBUG
#define debug_p(fmt, args...) printf(fmt, ##args)
#else
#define debug_p(fmt, args...)
#endif




#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/** 分类器日志 */
typedef enum
{
    IS_AWAKE_YES,             /**< 确定清醒 */
    IS_AWAKE_NO,              /**< 确定不清醒 */
    IS_AWAKE_UNKNOWN=-1,      /**< 不确定任何事 */
}alg_isawake_log_t;

typedef struct {
    float prob;
    uint32_t acc_amp;
    int8_t lower_xyz;
    int8_t IsAwake_log;
    uint8_t cur_hour;
    uint16_t waiting_cnt;
}alg_IsAwake_t;


alg_IsAwake_t *Get_IsAwakeInfo_ByRef(void);
alg_IsAwake_t Get_IsAwakeInfo_ByVal(void);

/** Initialize parameters in the identifier */
void LSSleep_IsAwake_init(void);

/**
 * @brief 为了午睡侦测而增加的新模型，建议一秒调用一次
 * @param[in]  x         加速度X轴
 * @param[in]  y         加速度Y轴
 * @param[in]  z         加速度Z轴
 * @param[in]  buf_size  缓存的个数
 * @param[in]  cur_hour  设备现在的时间（小时）
 * @param[in]  get_touch 配戴状态，0:未配戴，1:配戴
 * @param[out] ret       void
 */
void LSSleep_IsAwake(const uint8_t x[],
                     const uint8_t y[],
                     const uint8_t z[],
                     uint8_t buf_size,
                     uint8_t cur_hour,
                     bool get_touch);



#endif

