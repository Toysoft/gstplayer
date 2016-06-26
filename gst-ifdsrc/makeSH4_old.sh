#!/bin/bash

TMP_DIR=tmp

rm -rf $TMP_DIR
cp -R gst-ifdsrc $TMP_DIR
cd $TMP_DIR

autoreconf --verbose --force --install --make

export TOOLCHAIN_NAME="sh4-linux"
export PATH="/opt/STM1/STLinux-2.4/devkit/sh4/bin":$PATH
export SYSROOT="/mnt/new2/openatv2/build-enviroment/builds/openatv/release/spark/tmp/sysroots/spark"

export CROSS_COMPILE=$TOOLCHAIN_NAME"-"

I0="-I"$SYSROOT"/usr/lib/gstreamer-1.0/include/"
I3="-I"$SYSROOT"/usr/include/gstreamer-1.0/"
L0="-L/mnt/new2/openatv2/build-enviroment/builds/fakeSH4"

./configure \
	--target=$TOOLCHAIN_NAME \
	--host=$TOOLCHAIN_NAME \
	--build=i686-pc-linux-gnu CFLAGS=" $I0 $I1 $I2 $I3 $I4 $I5 $I6 $L0 " \

make
cd ..
rm -rf out/sh4
mkdir out/sh4
cp $TMP_DIR/src/.libs/libgstifdsrc.so out/sh4/libgstifdsrc.so_gstreamer1.0
"$CROSS_COMPILE"strip out/sh4/libgstifdsrc.so_gstreamer1.0

