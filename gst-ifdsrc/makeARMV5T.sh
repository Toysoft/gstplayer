#!/bin/bash

TMP_DIR=tmp

rm -rf $TMP_DIR
cp -R gst-ifdsrc $TMP_DIR
cd $TMP_DIR

autoreconf --verbose --force --install --make

export TOOLCHAIN_NAME="arm-oe-linux-gnueabi"
BASE_PATH="/mnt/new2/openatv2/build-enviroment/builds/openatv/release/cube/tmp/sysroots/"
export PATH=$BASE_PATH"i686-linux/usr/bin/arm-oe-linux-gnueabi/":$PATH
export SYSROOT=$BASE_PATH"cube"

export CROSS_COMPILE=$TOOLCHAIN_NAME"-"

./configure \
	--target=$TOOLCHAIN_NAME \
	--host=$TOOLCHAIN_NAME \
	--build=i686-pc-linux-gnu CFLAGS=" -mfloat-abi=softfp -mtune=cortex-a9 -mfpu=vfpv3-d16 -pipe  --sysroot=$SYSROOT " \

make
cd ..
rm -rf out/armv5t
mkdir out/armv5t
cp $TMP_DIR/src/.libs/libgstifdsrc.so out/armv5t/libgstifdsrc.so_gstreamer1.0
"$CROSS_COMPILE"strip out/armv5t/libgstifdsrc.so_gstreamer1.0

