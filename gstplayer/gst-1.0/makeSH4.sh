#!/bin/bash

export TOOLCHAIN_NAME="sh4-oe-linux"
export PATH="/mnt/new2/openatv2/build-enviroment/builds/openatv/release/spark/tmp/sysroots/i686-linux/usr/bin/sh4-oe-linux/":$PATH
export SYSROOT="/mnt/new2/openatv2/build-enviroment/builds/openatv/release/spark/tmp/sysroots/spark"

export CROSS_COMPILE=$TOOLCHAIN_NAME"-"

INCBASE=""
I0="-I"$SYSROOT"/usr/lib/gstreamer-1.0/include/"
I1="-I"$SYSROOT"/usr/include/glib-2.0/"
I2="-I"$SYSROOT"/usr/lib/glib-2.0/include/"
I3="-I"$SYSROOT"/usr/include/gstreamer-1.0/"
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

rm -rf sh4
mkdir sh4
"$CROSS_COMPILE"gcc --sysroot=$SYSROOT $I0 $I1 $I2 $I3 $I4 $I5 $I6 *.c ../common/*.c -o sh4/gstplayer_gstreamer1.0 $L0 $L1 $L2 $L3 $L4 $L5  -Wfatal-errors -lgstreamer-1.0 -lgstbase-1.0 -lgstpbutils-1.0 -lgobject-2.0 -lgthread-2.0 -lgmodule-2.0 -lxml2 -lglib-2.0
"$CROSS_COMPILE"strip -s sh4/gstplayer_gstreamer1.0



