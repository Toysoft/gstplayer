#!/bin/bash

export TOOLCHAIN_NAME="arm-oe-linux-gnueabi"
export PATH="/mnt/new2/vusolo4k/openvuplus_3.0/build/vusolo4k/tmp/sysroots/i686-linux/usr/bin/arm-oe-linux-gnueabi/":$PATH
export SYSROOT="/mnt/new2/vusolo4k/openvuplus_3.0/build/vusolo4k/tmp/sysroots/vusolo4k"

export CROSS_COMPILE=$TOOLCHAIN_NAME"-"

STATIC_LINK=""
BIN_POSTFIX=""
if [ "$1" == "static" ];
then
    STATIC_LINK=" -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic " #-static-libstdc++
    BIN_POSTFIX="_static_libstdc++"
fi

OUT_NAME="../out/armv7/uchardet"$BIN_POSTFIX
cd src

"$CROSS_COMPILE"g++ -DVERSION='"0.0.5"' -march=armv7-a -mfloat-abi=hard -mfpu=neon -fdata-sections -ffunction-sections -Wl,--gc-sections -Os --sysroot=$SYSROOT -o $OUT_NAME *.cpp tools/*.cpp  LangModels/*.cpp $STATIC_LINK
"$CROSS_COMPILE"strip -s $OUT_NAME

