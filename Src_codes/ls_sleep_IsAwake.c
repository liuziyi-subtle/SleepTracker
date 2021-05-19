/**
* Copyright [2021] <Wen Wang>.
*/

#include "ls_sleep_IsAwake.h"
#include "ls_sleep_tracker.h"
#include <string.h>
#define FS 25
#define MAX_LOWER_XYZ 2
#define LOWER_XYZ_WAITING_LIM 10
#define MY_ABS(a, b) (((a) > (b)) ? ((a) - (b)) : ((b) - (a)))
alg_IsAwake_t m_MyWatch={.acc_amp=100};
static uint8_t max_AV[3], min_AV[3], acc_AV[3], last_acc_AV[3]; //Amplitude Variation.
static uint16_t intervals=0;
static uint16_t intervals_1min=0;
void LSSleep_IsAwake_init(void)
{
    intervals = 0;
    intervals_1min = 0;
    for(uint8_t i=0; i<3; i++)
    {
        max_AV[i] = 0;
        min_AV[i] = 255;
        acc_AV[i] = 0;
        last_acc_AV[i] = 0;
    }

    //memset(&m_MyWatch, 0, sizeof(m_MyWatch));
    m_MyWatch.IsAwake_log = IS_AWAKE_YES;
    m_MyWatch.acc_amp = 0;
    m_MyWatch.lower_xyz = 0;//MAX_LOWER_XYZ;
}

static void feed_data(const uint8_t acc[3])
{
    for(uint8_t i=0; i<3; i++)
    {
        max_AV[i] = (acc[i]>max_AV[i]) ? acc[i]:max_AV[i];
        min_AV[i] = (acc[i]<min_AV[i]) ? acc[i]:min_AV[i];
    }
}


typedef enum {
    ALG_MOVING_FOR_SURE = 15,
    ALG_MAYBE_MOVING = 5,
    ALG_NO_MOVING = 1,
}alg_sleep_check_moving;
static void LSSleep_check_moving(uint16_t acc_amp)
{

    if(acc_amp > ALG_MOVING_FOR_SURE)
    {
        m_MyWatch.lower_xyz = 0;
        m_MyWatch.waiting_cnt = 0;
        m_MyWatch.IsAwake_log  = IS_AWAKE_YES;
    }
    else if(acc_amp > ALG_MAYBE_MOVING)
    {

    }
    else if(acc_amp > ALG_NO_MOVING)
    {
        if(m_MyWatch.lower_xyz < MAX_LOWER_XYZ)
        {
            m_MyWatch.lower_xyz++;
            m_MyWatch.IsAwake_log = IS_AWAKE_NO;
            m_MyWatch.waiting_cnt = 0;
        }
        else
        {
            m_MyWatch.IsAwake_log  = IS_AWAKE_YES;
            m_MyWatch.waiting_cnt = LOWER_XYZ_WAITING_LIM;
        }
    }
    else
    {
        if(m_MyWatch.lower_xyz < MAX_LOWER_XYZ)
        {
            m_MyWatch.lower_xyz = 0;
            m_MyWatch.IsAwake_log  = IS_AWAKE_NO;
        }
        else if(m_MyWatch.waiting_cnt)
        {
            m_MyWatch.waiting_cnt--;
            //printf("waiting_cnt: %d", m_MyWatch.waiting_cnt);
            if(m_MyWatch.waiting_cnt == 0)
            {
                m_MyWatch.lower_xyz = 0;
                m_MyWatch.IsAwake_log  = IS_AWAKE_NO;
            }
        }
    }

}

void LSSleep_IsAwake(const uint8_t x[],
                     const uint8_t y[],
                     const uint8_t z[],
                     uint8_t buf_size,
                     uint8_t cur_hour,
                     bool get_touch)
{

    m_MyWatch.cur_hour = cur_hour;
    uint8_t i, j;

    for(i=0; i<buf_size; i++)
    {
        uint8_t acc[3]={x[i], y[i], z[i]};
        feed_data(acc);

        //Cal features.
//        if(intervals % FS == 0)
//        {
//            for(j=0; j<3; j++)
//            {
//                acc_AV[j] = (max_AV[j]>min_AV[j])? (max_AV[j] - min_AV[j]): 0;
//                //m_MyWatch.acc_amp += acc_AV[j];
//                m_MyWatch.acc_amp += MY_ABS(acc_AV[j], last_acc_AV[j]);
//                max_AV[j] = 0;
//                min_AV[j] = 255;
//                last_acc_AV[j] = acc_AV[j];
//                //For watching
//#define PY_WATCH_ACC 255
//                if(m_MyWatch.acc_amp > PY_WATCH_ACC)
//                {
//                    //m_MyWatch.acc_amp = PY_WATCH_ACC;
//                }
//            }
//        }

        if(intervals < FS*60)
        {
            intervals += 1;
        }
        else
        {

            //Reset vars.
            intervals = 0;
            m_MyWatch.acc_amp = 0;

            //Cal features.
            for(j=0; j<3; j++)
            {
                acc_AV[j] = (max_AV[j]>min_AV[j])? (max_AV[j] - min_AV[j]): 0;
                //In case AV is too big.
                if(acc_AV[j] > 5)
                {
                    last_acc_AV[j] = 0;
                }

                //m_MyWatch.acc_amp += acc_AV[j];
                m_MyWatch.acc_amp = MY_ABS(acc_AV[j], last_acc_AV[j]);
                max_AV[j] = 0;
                min_AV[j] = 255;
                last_acc_AV[j] = acc_AV[j];
            }

            //Check if get touch
            if(get_touch == false)
            {
                LSSleep_IsAwake_init();
                m_MyWatch.IsAwake_log = IS_AWAKE_YES;
            }
            else
            {
                //Calculating.
                LSSleep_check_moving(m_MyWatch.acc_amp);
                debug_p(" pred: %d ", m_MyWatch.IsAwake_log);
            }

            intervals_1min += 1;
        }
    }

}


alg_IsAwake_t *Get_IsAwakeInfo_ByRef(void)
{
    return &m_MyWatch;
}

alg_IsAwake_t Get_IsAwakeInfo_ByVal(void)
{
    return m_MyWatch;
}