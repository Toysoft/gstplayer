#!/bin/bash

# example how to prepare toolchain for armv7
# mkdir /mnt/new2/openpli_micro/
# cd /mnt/new2/openpli_micro/
# git clone https://github.com/OpenPLi/openpli-oe-core.git
# cd openpli-oe-core
# MACHINE=hd51 make
# MACHINE=hd51 make image

export TOOLCHAIN_NAME="arm-oe-linux-gnueabi"
export PATH="/mnt/new2/openpli_micro/openpli-oe-core/build/tmp/sysroots/i686-linux/usr/bin/arm-oe-linux-gnueabi/":$PATH
export SYSROOT="/mnt/new2/openpli_micro/openpli-oe-core/build/tmp/sysroots/hd51"

export CROSS_COMPILE=$TOOLCHAIN_NAME"-"

INCBASE=""
I0="-I"$SYSROOT"/usr/include/gstreamer-0.10/"
I1="-I"$SYSROOT"/usr/include/glib-2.0/"
I2="-I"$SYSROOT"/usr/lib/glib-2.0/include/"
I3="-I"$SYSROOT"/usr/include/"
I4="-I"$SYSROOT"/usr/include/libxml2"
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
"$CROSS_COMPILE"gcc -D PLATFORM_MIPSEL -march=armv7-a -mfloat-abi=hard -mfpu=neon --sysroot="$SYSROOT" $I0 $I1 $I2 $I3 $I4 $I5 $I6 *.c ../common/*.c -o armv7/gstplayer_gstreamer0.10 $L0 $L1 $L2 $L3 $L4 $L5  -Wfatal-errors -lgstreamer-0.10 -lgstbase-0.10 -lgstpbutils-0.10 -lgobject-2.0 -lgthread-2.0 -lgmodule-2.0 -lxml2 -lglib-2.0
"$CROSS_COMPILE"strip -s armv7/gstplayer_gstreamer0.10

    