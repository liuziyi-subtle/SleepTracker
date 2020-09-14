import com.lifesense.sleep_score.LSSleepScore;
import com.lifesense.sleep_score.LSSleepScoreInput;
import com.lifesense.sleep_score.LSSleepScoreResult;
import com.lifesense.sleep_score.LSSleepScoreContrib;
import com.lifesense.sleep_score.LSSleepScoreName;

import java.util.ArrayList;

public class Main {
	
    // Load lib
	static {
		System.load("/Sleep/Sleep-Rater-Project/lib/libsleepscore.so");
	}


	public static void main(String[] args)  {

        // Input format
        LSSleepScoreInput testData = new LSSleepScoreInput();
        testData.shallowSleep   = 315;
        testData.sex            = 1;
        testData.awakening      = 0;
        testData.birthday       = "2006-01-22";
        testData.awakeningTime  = "2017-03-08 08:29:59";
        testData.deepSleep      = 130;
        testData.awakeningCount = 0;
        testData.height         = 172;
        testData.sleepTime      = "2017-03-08 01:04:59";
        testData.analysisTime   = "2017-03-08 08:29:59";
        testData.modelPath      = "../sleep_score.model";

        // Algorithm inference interface
        LSSleepScoreResult result = LSSleepScore.GetSleepScore(testData);

        // Print score
        System.out.printf("Sleep score: %d\n", result.score);

        /* Print 17 attributs sorted by their contributions on sleep score.
           Below are explainations for the 17 attributes, select the subset
           you are interested in:
           AwakeningTime               - 清醒时间
           SleepTime                   - 入睡时间
           AnalysisTime                - 分析时间
           Awakening                   - 清醒时长
           AwakeningCount              - 清醒次数
           DeepSleep                   - 深睡时长
           ShallowSleep                - 潜睡时长
           TotalSleepDuration          - 睡眠时长
           Height                      - 身高
           Sex                         - 性别
           Age                         - 年龄
           IntAgeSleepTime
           IntAgeShallowSleep
           IntAgeHeight
           IntSleepTimeShallowSleep
           IntShallowSleepHeight
           IntShallowTotalSleepDuration
        */
        ArrayList<LSSleepScoreContrib> contrib = result.contrib;
        for (LSSleepScoreContrib feat : contrib) {
            System.out.printf("%-30s: %6f\n", feat.name, feat.value);
        }

    }
}
