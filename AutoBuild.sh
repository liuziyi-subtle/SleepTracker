#!/bin/bash
CUR_DIR=$(pwd)
# JAVA_DIR=/jdk1.8.0_191/include
MD_DIR=$JAVA_DIR/linux
XGBOOST_DIR=$CUR_DIR/xgboost
SRC_DIR=$CUR_DIR/src
TMP_DIR=$CUR_DIR/tmp
LIB_DIR=$CUR_DIR/lib
DEMO_DIR=$CUR_DIR/demo

# Build libdmlc.a librabit.a libxgboost.so libxgboost.a
echo "building libdmlc.a librabit.a libxgboost.a..."
#echo "git clone --recursive https://github.com/dmlc/xgboost"
#git clone --recursive https://github.com/dmlc/xgboost
cd $XGBOOST_DIR

if make; then
    echo "Successfully build multi-thread xgboost ..."
else
    echo "xgboost build failed"
fi

# Build libsleepscore.jar
echo "building libsleepscore.jar..."
mkdir $TMP_DIR
mkdir $LIB_DIR
cd $SRC_DIR
javac -sourcepath . -d $TMP_DIR ./com/lifesense/sleep_score/*.java
cd $TMP_DIR
jar -cvf $LIB_DIR/libsleepscore.jar ./com/lifesense/sleep_score/*.class

#2. Build libsleepscore.so
echo "building libsleepscore.so..."
cd $SRC_DIR
g++ -std=c++11 -Wall -frtti -fPIC -fsigned-char -fopenmp -fPIC -shared -o \
$LIB_DIR/libsleepscore.so -Wl,--whole-archive \
-I $SRC_DIR \
-I $JAVA_DIR \
-I $MD_DIR \
-I $CUR_DIR \
-I $XGBOOST_DIR/include/xgboost \
-I $XGBOOST_DIR/rabit/include \
$SRC_DIR/jni/JNIHelper.cc \
$SRC_DIR/jni/com_lifesense_sleep_score_LSSleepScore.cc \
$SRC_DIR/ls_sleep_rate_quality.cc \
$XGBOOST_DIR/lib/libxgboost.a \
$XGBOOST_DIR/rabit/lib/librabit.a \
$XGBOOST_DIR/dmlc-core/libdmlc.a -Wl,--no-whole-archive

# Remove tmp xgboost folders
echo "removing tmp folder"
cd $CUR_DIR
rm -r $TMP_DIR
