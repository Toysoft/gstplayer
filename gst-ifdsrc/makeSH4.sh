#!/bin/bash

TMP_DIR=tmp

rm -rf $TMP_DIR
cp -R gst-ifdsrc $TMP_DIR
cd $TMP_DIR

autoreconf --verbose --force --install --make

export TOOLCHAIN_NAME="sh4-oe-linux"
export PATH="/mnt/new2/openatv2/build-enviroment/builds/openatv/release/spark/tmp/sysroots/i686-linux/usr/bin/sh4-oe-linux/":$PATH
export SYSROOT="/mnt/new2/openatv2/build-enviroment/builds/openatv/release/spark/tmp/sysroots/spark"

export CROSS_COMPILE=$TOOLCHAIN_NAME"-"

./configure \
	--target=$TOOLCHAIN_NAME \
	--host=$TOOLCHAIN_NAME \
	--build=i686-pc-linux-gnu CFLAGS=" --sysroot=$SYSROOT " \

make
cd ..
rm -rf out/sh4
mkdir out/sh4
cp $TMP_DIR/src/.libs/libgstifdsrc.so out/sh4/libgstifdsrc.so_gstreamer1.0
"$CROSS_COMPILE"strip out/sh4/libgstifdsrc.so_gstreamer1.0

