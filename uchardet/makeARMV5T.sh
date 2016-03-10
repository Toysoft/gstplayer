#!/bin/bash

export TOOLCHAIN_NAME="arm-oe-linux-gnueabi"
BASE_PATH="/mnt/new2/openatv2/build-enviroment/builds/openatv/release/cube/tmp/sysroots/"
export PATH=$BASE_PATH"i686-linux/usr/bin/arm-oe-linux-gnueabi/":$PATH
export SYSROOT=$BASE_PATH"cube"

export CROSS_COMPILE=$TOOLCHAIN_NAME"-"

STATIC_LINK=""
BIN_POSTFIX=""
if [ "$1" == "static" ];
then
    STATIC_LINK=" -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic " #-static-libstdc++
    BIN_POSTFIX="_static_libstdc++"
fi

OUT_NAME="../out/armv5t/uchardet"$BIN_POSTFIX
cd src

"$CROSS_COMPILE"g++ -DVERSION='"0.0.2"' -mfloat-abi=softfp -mtune=cortex-a9 -mfpu=vfpv3-d16 -pipe -fdata-sections -ffunction-sections -Wl,--gc-sections -Os --sysroot=$SYSROOT -o $OUT_NAME *.cpp tools/*.cpp $STATIC_LINK
"$CROSS_COMPILE"strip -s $OUT_NAME

