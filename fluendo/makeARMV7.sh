#!/bin/bash

TOP_DIR=$PWD
TMP_DIR=tmp
rm -rf $TMP_DIR
mkdir $TMP_DIR
cd $TMP_DIR
wget http://core.fluendo.com/gstreamer/src/gst-fluendo-mpegdemux/gst-fluendo-mpegdemux-0.10.85.tar.gz
tar -xvf gst-fluendo-mpegdemux-0.10.85.tar.gz


export TOOLCHAIN_NAME="arm-oe-linux-gnueabi"
export PATH="/mnt/new2/openpli_micro/openpli-oe-core/build/tmp/sysroots/i686-linux/usr/bin/arm-oe-linux-gnueabi/":$PATH
export SYSROOT="/mnt/new2/openpli_micro/openpli-oe-core/build/tmp/sysroots/hd51"

export CROSS_COMPILE=$TOOLCHAIN_NAME"-"
#export CC="$CROSS_COMPILE-gcc $CFLAGS"
cd gst-fluendo-mpegdemux-0.10.85

./configure \
	--target=$TOOLCHAIN_NAME \
	--host=$TOOLCHAIN_NAME \
	--build=i686-pc-linux-gnu CFLAGS="-Os --sysroot=$SYSROOT -march=armv7-a -mfloat-abi=hard -mfpu=neon " \

make

cd $TOP_DIR
rm -rf armv7
mkdir armv7
cp $TOP_DIR/tmp/gst-fluendo-mpegdemux-0.10.85/src/.libs/libgstflumpegdemux.so armv7/libgstflumpegdemux.so_gstreamer0.10
"$CROSS_COMPILE"strip -s armv7/libgstflumpegdemux.so_gstreamer0.10
rm -rf tmp
rm gst-fluendo-mpegdemux-0.10.85.tar.gz


