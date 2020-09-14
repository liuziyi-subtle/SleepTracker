package com.lifesense.sleep_score;

public class LSSleepScore {

    // 睡眠分数接口
    public static LSSleepScoreResult GetSleepScore(LSSleepScoreInput data)
    {
        return RateSleepQuality(data);
    }

    private static native LSSleepScoreResult RateSleepQuality(LSSleepScoreInput data);

}