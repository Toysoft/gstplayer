#!/bin/bash

TMP_DIR=tmp

rm -rf $TMP_DIR
cp -R gst-ifdsrc $TMP_DIR
cd $TMP_DIR

autoreconf --verbose --force --install --make

export TOOLCHAIN_NAME="arm-oe-linux-gnueabi"
export PATH="/mnt/new2/vusolo4k/openvuplus_3.0/build/vusolo4k/tmp/sysroots/i686-linux/usr/bin/arm-oe-linux-gnueabi/":$PATH
export SYSROOT="/mnt/new2/vusolo4k/openvuplus_3.0/build/vusolo4k/tmp/sysroots/vusolo4k"

export CROSS_COMPILE=$TOOLCHAIN_NAME"-"

./configure \
	--target=$TOOLCHAIN_NAME \
	--host=$TOOLCHAIN_NAME \
	--build=i686-pc-linux-gnu CFLAGS=" -march=armv7-a -mfloat-abi=hard -mfpu=neon --sysroot=$SYSROOT " \

make
cd ..
rm -rf out/armv7
mkdir out/armv7
cp $TMP_DIR/src/.libs/libgstifdsrc.so out/armv7/libgstifdsrc.so_gstreamer1.0
"$CROSS_COMPILE"strip out/armv7/libgstifdsrc.so_gstreamer1.0

