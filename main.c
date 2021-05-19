#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "ls_sleep_tracker.h"
#include "ls_sleep_IsAwake.h"
#include "ls_sleep_debug.h"
#include "ls_sleep_func.h"
#include <stdlib.h>
#include <time.h>

#define FS           25
#define PC_SIZE      FS
#define AXIS_NUM     3
#define MAX_DAT_NUM  FS * 60 * 1000 /* max number of samples per axis */
uint32_t utc=0; //1618821437
// 嵌入式端模拟
static void CovertData(int16_t *pu16AccInData, uint8_t *pu8AccOutData)
{
    int32_t s32AccData[3];
    for (uint8_t j = 0; j < 3; j++)
    {
        // 左移1bit转成2G量程，再右移8bit保留高8bit数据
        s32AccData[j] = ((int16_t)pu16AccInData[j] << 1) >> 8;
        // 大量程转小量程做封顶处理
        if (s32AccData[j] > 127)
        {
            s32AccData[j] = 127;
        }
        else if (s32AccData[j] < -128)
        {
            s32AccData[j] = -128;
        }
    }

    // 转换方向，并把三轴数值范围跳转到0~255
    pu8AccOutData[0] = (uint8_t)(128 + s32AccData[0]);
    pu8AccOutData[1] = (uint8_t)(127 - s32AccData[1]);
    pu8AccOutData[2] = (uint8_t)(128 + s32AccData[2]);
}
static uint32_t acc_cnt=0;
void emd_CovertData(int data_cnt, int16_t x[], int16_t y[], int16_t z[],uint8_t buf_size, uint32_t cur_utc)
{
    int16_t AccIn[3];
    uint8_t AccOut[3];
    for(uint8_t i=0; i<buf_size; i++)
    {
        AccIn[0] = x[i];
        AccIn[1] = y[i];
        AccIn[2] = z[i];

        CovertData(AccIn, AccOut);
        x[i] = AccOut[0];
        y[i] = AccOut[1];
        z[i] = AccOut[2];

        uint32_t norm = x[i]*x[i] + y[i]*y[i] + z[i]*z[i];
        norm = sqrt(norm);
        acc_cnt += 1;
        printf("\n%d AccIn: %d %d %d, AccOut:", data_cnt-buf_size+i+1, AccIn[0], AccIn[1], AccIn[2]);
        printf(" %d %d %d ",x[i], y[i], z[i]);
        printf(", Acc length: %d acc_cnt: %d", norm, acc_cnt);
    }
}


#define SLEEP_PUSH_1MIN_DATA_MAX 60 //1MIN 推入睡眠算法的数据总个数
//input:睡眠算法的1min 总数据
typedef struct
{
    //数据部分
    uint8_t m_hr;
    uint8_t m_u8Xvalue;
    uint8_t m_u8Yvalue;
    uint8_t m_u8Zvalue;
    //状态部分
    bool m_blHrSwitchOn;          //全局的心率开关，true:on false:off
    bool m_blOnWear;              //是否在佩戴中，true:在佩戴中 false:不在佩戴中
} STRU_SLEEP_PUSH_DATA_1MIN;

//睡眠功能状态结构体
typedef struct
{
    //数据更新锁
    bool m_blCleanDataLock;                     //true:上锁无法清空今日数据
    //状态
    //ENUM_FUN_SLEEP_STATE m_struState;           //睡眠功能状态

    //input:x y z data
    //STRU_SLEEP_PUSH_DATA m_struPushData;        //推入睡眠算法的1s临时数据
    STRU_SLEEP_PUSH_DATA_1MIN m_struPushData1min[SLEEP_PUSH_1MIN_DATA_MAX];         //推入睡眠算法的1min总数据
    uint16_t m_u16Push1minIndex;                //推入睡眠算法的总数据下标

    //算法结果
    struct LSSleepResult m_struResultData;      //获取到的睡眠算法的结果数据
} STRU_FUN_SLEEP_STATE;
STRU_FUN_SLEEP_STATE g_SleepState;

void emd_init()
{
    //data init
    //memset(&(g_SleepState.m_struPushData), 0, sizeof(STRU_SLEEP_PUSH_DATA));
    memset(&(g_SleepState.m_struPushData1min), 0, sizeof(STRU_SLEEP_PUSH_DATA_1MIN)*SLEEP_PUSH_1MIN_DATA_MAX);
    memset(&(g_SleepState.m_struResultData), 0, sizeof(g_SleepState.m_struResultData));

    // 内存管理
    bool retb;
    retb = alg_sleep_malloc_reg((alg_sleep_malloc_cb_t)malloc);
    if(retb == false)
    {
        printf("Reg malloc failed");
        return;
    }

    retb = alg_sleep_free_reg(free);
    if(retb == false)
    {
        printf("Reg free failed");
        return;
    }

    // 睡眠算法初始化和参数设置
    LSSleepInitialize();
    LSSleep_IsAwake_init();
    alg_SleepInfo_t* tmp = alg_SetSleepMode(NAP_MODE); //NAP_MODE ALLDAY_MODE
}

void emd_FunSleepStateLoopProc(int16_t x[],
                               int16_t y[],
                               int16_t z[],
                               uint8_t buf_size,
                               uint32_t cur_utc,
                               bool get_touch)
{
    uint8_t x_8t[255], y_8t[255], z_8t[255];
    for(uint8_t i=0; i<buf_size; i++)
    {
        static uint32_t acc_cnt=0;
        acc_cnt += 1;
        x_8t[i] = (uint8_t )x[i];
        y_8t[i] = (uint8_t )y[i];
        z_8t[i] = (uint8_t )z[i];
        //printf("\n%d %d %d %d", acc_cnt, x_8t[i], y_8t[i], z_8t[i]);
    }

    //TimeStamp to local hour.
    uint8_t cur_hour = 13;
//    time_t t = cur_utc;
//    struct tm   local_tm;
//    local_tm = * localtime(&t);
//    cur_hour = local_tm.tm_hour;

    LSSleep_IsAwake(x_8t, y_8t, z_8t, buf_size, cur_hour, get_touch);

    if(g_SleepState.m_u16Push1minIndex < SLEEP_PUSH_1MIN_DATA_MAX)
    {
        g_SleepState.m_struPushData1min[g_SleepState.m_u16Push1minIndex].m_hr = 0;
        g_SleepState.m_struPushData1min[g_SleepState.m_u16Push1minIndex].m_u8Xvalue = x[0];
        g_SleepState.m_struPushData1min[g_SleepState.m_u16Push1minIndex].m_u8Yvalue = y[0];
        g_SleepState.m_struPushData1min[g_SleepState.m_u16Push1minIndex].m_u8Zvalue = z[0];
        g_SleepState.m_struPushData1min[g_SleepState.m_u16Push1minIndex].m_blOnWear = get_touch;
        g_SleepState.m_u16Push1minIndex++;

    }
    else
    {
        if(cur_utc/60 == 69)
        {
            int aa=3;
        }
        debug_p("\n %d ", cur_utc/60);
        //algorithm entrance
        LSSleepAnalyzeData((struct LSSleepData *)(g_SleepState.m_struPushData1min),
                            SLEEP_PUSH_1MIN_DATA_MAX,
                            cur_utc,
                            false
                           );



        //the result of sleep alg per min.
        LSSleepGetResult(&(g_SleepState.m_struResultData));

        if(g_SleepState.m_struResultData.sleepTimeUtc >0)
        {
            printf("\n~~~~~~~~~~~\n");
            printf("Reesult: %d %d %d",
                   g_SleepState.m_struResultData.sleepTimeUtc/60,
                   g_SleepState.m_struResultData.getupTimeUtc/60,
                   g_SleepState.m_struResultData.completeCycleIndicator
                   );
            printf("\n~~~~~~~~~~~\n");
        }



        //clean data
        memset(g_SleepState.m_struPushData1min, 0, sizeof(g_SleepState.m_struPushData1min));
        memset(&(g_SleepState.m_struResultData), 0, sizeof(g_SleepState.m_struResultData));
        g_SleepState.m_u16Push1minIndex = 0;
    }

}
/**嵌入式端模拟结束****************************************************************/



time_t my_timegm(struct tm *tm) {
    time_t epoch = 0;
    time_t offset = mktime(gmtime(&epoch));
    time_t tmp_utc = mktime(tm);
    return difftime(tmp_utc, offset);
}


int main(int argc, char *argv[])
{
    FILE *fp;
    char *file_name;


    //Sleep
    //Normal data


    //Questionable data
    //file_name = "D:\\Life_sense\\data\\Sleep\\LS435_20210325_night\\data\\acc\\20210325221627_44_494.txt";

    //被gSleepCycleDetector =61挡住的睡眠
    file_name = "D:\\Life_sense\\data\\Sleep\\LS435_20210325_nap\\data\\acc\\20210329114936_49_101.txt";
    file_name = "D:\\Life_sense\\data\\Sleep\\LS435_20210423_nap\\data\\acc\\20210420120055_50_90.txt";
    //file_name = "D:\\Life_sense\\data\\Sleep\\LS435_20210325_nap\\data\\acc\\20210329114936_49_101.txt";



    fp = fopen(file_name, "r");
    if(NULL == fp)
    {
        printf("Can't open %s\n", file_name);
    }

    char c;
    int i,j;
    int nl=0; // number of lines
    while((c = getc(fp)) != EOF)
    {
        if(c == '\n')
        {
            nl++;
        }
    }
    rewind(fp);
    if(nl > MAX_DAT_NUM)
    {
        nl = MAX_DAT_NUM;
    }

    int16_t *data[AXIS_NUM];
    for(i = 0; i < AXIS_NUM; i++)
    {
        data[i] = (int16_t *)malloc(nl * sizeof(int));
    }

    for(j = 0; j < nl; j++)
    {
        for(i = 0; i < AXIS_NUM; i++)
        {
            fscanf(fp, "%d", &data[i][j]);
        }
    }





    // 嵌入式初始化
    emd_init();
    int start_i=0;
    utc=0; //1619062731
    uint32_t ind=0;
    for(i = 0; i < nl; i++)
    {
        if(i == (ind + FS))
        {
            utc += 1;
            uint8_t offset = i - (ind);
            int16_t *x = (int16_t *) &data[0][ind];
            int16_t *y = (int16_t *) &data[1][ind];
            int16_t *z = (int16_t *) &data[2][ind];
            //printf("\nind: %d i: %d cur_utc: %d", ind, i, cur_utc);

            //emd_CovertData(i, x, y, z, offset, cur_utc);
            bool get_touch = true;
            emd_FunSleepStateLoopProc(x, y, z, offset, utc, get_touch);
            ind = i;
        }




        // 动态offset调整
//        uint8_t label = data[0][i];
//        if(label == 0)
//        {
//            uint8_t offset = i - (start_i);
//            int16_t *x = (int16_t *) &data[1][start_i];
//            int16_t *y = (int16_t *) &data[2][start_i];
//            int16_t *z = (int16_t *) &data[3][start_i];
//            uint32_t cur_utc = data[1][i];
//
//            printf("\n%d From %d to %d cur_utc:%d", i-offset, start_i+1, i+1, cur_utc);
//            if(offset > 0)
//            {
//                emd_CovertData(i, x, y, z, offset, cur_utc);
//                bool get_touch = true;
//                emd_FunSleepStateLoopProc(x, y, z, offset, cur_utc, get_touch);
//            }
//            else
//            {
//                printf("Offset is 0.\n");
//            }
//
//
//            start_i = i+1;
//        }
        //printf("\ni:%d %d", i, nl);



        if(i > 10000)
        {
            //break;
        }
    }

    fclose(fp);
    return 0;
}
