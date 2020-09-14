## 睡眠质量分数子项目
此项目的用途在于，基于用户某一次睡眠的特征，给出此次睡眠的质量分数，并就睡眠特征对质量分数的影响进行量化和排序。

### 项目构建

#### 构建环境
CentOS Linux release 7.5.1804 (Core)

java version "1.8.0_191"

Java(TM) SE Runtime Environment (build 1.8.0_191-b12)

Java HotSpot(TM) 64-Bit Server VM (build 25.191-b12, mixed mode)

gcc version 4.8.5 20150623 (Red Hat 4.8.5-28) (GCC)

#### 构建动态库

```
cd ~/Sleep-Rater-Project
./AutoBuild.sh
```

生成的库文件将位于～/Sleep-Rater-Project/lib文件夹中：
- ~/Sleep-Rater-Project/lib/lssleepscore.so
- ~/Sleep-Rater-Project/lib/lssleepscore.jar

### 项目测试

~/Sleep-Rater-Project/demo文件夹中提供了一个简单的测试代码Main.java

```
cd ~/Sleep-Rater-Project/demo
./DemoBuild.sh
```

### 接口说明
基于Main.java对算法接口说明：

输入结构体格式
```
LSSleepScoreInput testData = new LSSleepScoreInput();

testData.shallowSleep   = 315;                    // 浅睡时长
testData.sex            = 1;                      // 性别
testData.awakening      = 0;                      // 清醒时长
testData.birthday       = "2006-01-22";           // 出生日期
testData.awakeningTime  = "2017-03-08 08:29:59";  // 起床时间
testData.deepSleep      = 130;                    // 深睡时长
testData.awakeningCount = 0;                      // 清醒次数
testData.height         = 172;                    // 身高
testData.sleepTime      = "2017-03-08 01:04:59";  // 入睡时间
testData.analysisTime   = "2017-03-08 08:29:59";  // 分析时间
testData.modelPath      = "../sleep_score.model"; // 模型路径
```

算法接口示例
```
LSSleepScoreResult result = LSSleepScore.GetSleepScore(testData);
```

输出结构体格式
result包含两个域:
- result.score: 此次睡眠的分数
- result.contrib: 以链表形式给出影响睡眠分数的特征名称以及对应的影响值

```
System.out.printf("Sleep score: %d\n", result.score);         // 打印分数

ArrayList<LSSleepScoreContrib> contrib = result.contrib;
for (LSSleepScoreContrib feat : contrib) {
    System.out.printf("%-30s: %6f\n", feat.name, feat.value); // 打印睡眠特征的名称
                                                              // 以及对应的影响值
}
```
上述打印结果：
```
Sleep score: 88                                (睡眠分数)

SleepTime                     : -0.399673      (入睡时间)     
IntSleepTimeShallowSleep      : -0.159541      
IntAgeShallowSleep            : -0.040439      
AwakeningTime                 : -0.028353      (起床时间)
IntAgeSleepTime               : -0.013737
AnalysisTime                  : -0.003251      (分析时间)
AwakeningCount                : 0.002881       (清醒次数)
ShallowSleep                  : 0.004325       (浅睡时长)
Height                        : 0.004448       (身高)
IntShallowSleepHeight         : 0.008799       
IntShallowTotalSleepDuration  : 0.013483       
IntAgeHeight                  : 0.042577      
Awakening                     : 0.065152       (清醒时长)
DeepSleep                     : 0.076337       (深睡时长)
Age                           : 0.098672       (年龄)
Sex                           : 0.139168       (性别)
TotalSleepDuration            : 0.303495       (睡眠时长)
```
仅需关注其中作中文注释的睡眠特征：
- 数值解释：正值表示对睡眠质量分数起提升作用，越高则提升作用越大，如上例中睡眠时长贡献最明显；负值表示对睡眠质量分数起抑制作用，越低则抑制越强，如入睡时间会明显降低分数。
- 数值排序：结果已经按照数值，对所有特征进行过排序。

