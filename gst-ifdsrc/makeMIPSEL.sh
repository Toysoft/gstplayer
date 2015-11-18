#!/bin/bash

TMP_DIR=tmp

rm -rf $TMP_DIR
cp -R gst-ifdsrc $TMP_DIR
cd $TMP_DIR

autoreconf --verbose --force --install --make

export PATH="/mnt/new2/openpli40/openpli-oe-core/build/tmp/sysroots/i686-linux/usr/bin/mipsel-oe-linux/":$PATH

./configure \
	--target=mipsel-oe-linux \
	--host=mipsel-oe-linux \
	--build=i686-pc-linux-gnu CFLAGS=" -mel -mabi=32 -mhard-float -march=mips32 --sysroot=/mnt/new2/openpli40/openpli-oe-core/build/tmp/sysroots/dm500hd " \

make
cd ..
rm -rf out/mipsel
mkdir out/mipsel
cp $TMP_DIR/src/.libs/libgstifdsrc.so out/mipsel/libgstifdsrc.so_gstreamer1.0
mipsel-oe-linux-strip out/mipsel/libgstifdsrc.so_gstreamer1.0
