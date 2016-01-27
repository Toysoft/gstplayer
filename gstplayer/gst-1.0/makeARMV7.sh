#!/bin/bash

export TOOLCHAIN_NAME="arm-oe-linux-gnueabi"
export PATH="/mnt/new2/vusolo4k/openvuplus_3.0/build/vusolo4k/tmp/sysroots/i686-linux/usr/bin/arm-oe-linux-gnueabi/":$PATH
export SYSROOT="/mnt/new2/vusolo4k/openvuplus_3.0/build/vusolo4k/tmp/sysroots/vusolo4k"

export CROSS_COMPILE=$TOOLCHAIN_NAME"-"

INCBASE=""
I0="-I"$SYSROOT"/usr/include/gstreamer-1.0/"
I1="-I"$SYSROOT"/usr/include/glib-2.0/"
I2="-I"$SYSROOT"/usr/lib/glib-2.0/include/"
I3=""
I4=""
I5=""
I6="-I../common"

LIBBASE=""
L0=""
L1=""
L2="" 
L3=""
L4=""
L5=""

rm -rf armv7
mkdir armv7
"$CROSS_COMPILE"gcc -D PLATFORM_MIPSEL -march=armv7-a -mfloat-abi=hard -mfpu=neon --sysroot=$SYSROOT $I0 $I1 $I2 $I3 $I4 $I5 $I6 *.c ../common/*.c -o armv7/gstplayer_gstreamer1.0 $L0 $L1 $L2 $L3 $L4 $L5  -Wfatal-errors -lgstreamer-1.0 -lgstbase-1.0 -lgstpbutils-1.0 -lgobject-2.0 -lgthread-2.0 -lgmodule-2.0 -lxml2 -lglib-2.0
"$CROSS_COMPILE"strip -s armv7/gstplayer_gstreamer1.0

