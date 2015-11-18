#!/bin/bash

TMP_DIR=tmp

rm -rf $TMP_DIR
cp -R gst-ifdsrc $TMP_DIR
cd $TMP_DIR

autoreconf --verbose --force --install --make
./configure --prefix=/usr
make

cd ..
rm -rf out/i686
mkdir out/i686
cp $TMP_DIR/src/.libs/libgstifdsrc.so out/i686/libgstifdsrc.so_gstreamer1.0
strip out/i686/libgstifdsrc.so_gstreamer1.0
