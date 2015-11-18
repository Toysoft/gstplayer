#!/bin/bash

INCBASE="-I/mnt/new2/openpli40/openpli-oe-core/build/tmp/work/mips32el-oe-linux"
I0="-I/mnt/new2/openpli40/openpli-oe-core/build/tmp/sysroots/dm500hd/usr/include/gstreamer-1.0"
I1="-I/mnt/new2/openpli40/openpli-oe-core/build/tmp/sysroots/dm500hd/usr/include/glib-2.0"
I2="-I/mnt/new2/openpli40/openpli-oe-core/build/tmp/sysroots/dm500hd/usr/lib/glib-2.0/include/"
I3="$INCBASE/glib-2.0/1_2.40.0-r0/sysroot-destdir/usr/include/glib-2.0/include"
I4="$INCBASE/libxml2/2.9.1-r0/sysroot-destdir/usr/include/libxml2"
I5="$INCBASE/glib-2.0/1_2.40.0-r0/sysroot-destdir/usr/lib/glib-2.0/include/"
I6="-I../common"

LIBBASE="-L/mnt/new2/openpli40/openpli-oe-core/build/tmp/work/mips32el-oe-linux"
L0="$LIBBASE/gst-plugins-base/0.10.36.1+gitAUTOINC+bdb3316347-r2/image/usr/lib"
L1="$LIBBASE/gstreamer/0.10.36.1+gitAUTOINC+1bcabb9a23-r1/sysroot-destdir/usr/lib"
L2="" #$LIBBASE/glib-2.0/1_2.40.0-r0/sysroot-destdir/usr/lib
L3="$LIBBASE/libffi/3.1-r0/sysroot-destdir/usr/lib"
L4="$LIBBASE/libxml2/2.9.1-r0/sysroot-destdir/usr/lib"
L5="-L/mnt/new2/openpli40/openpli-oe-core/build/tmp/work/mips32el-oe-linux/zlib/1.2.8-r0/sysroot-destdir/lib"


rm -rf mipsel
mkdir mipsel
export PATH="/mnt/new2/openpli40/openpli-oe-core/build/tmp/sysroots/i686-linux/usr/bin/mipsel-oe-linux/":$PATH
mipsel-oe-linux-gcc -D PLATFORM_MIPSEL -mel -mabi=32 -mhard-float -march=mips32 --sysroot="/mnt/new2/openpli40/openpli-oe-core/build/tmp/sysroots/dm500hd" $I0 $I1 $I2 $I3 $I4 $I5 $I6 *.c ../common/*.c -o mipsel/gstplayer_gstreamer1.0 $L0 $L1 $L2 $L3 $L4 $L5  -Wfatal-errors -lgstreamer-1.0 -lgstbase-1.0 -lgstpbutils-1.0 -lgobject-2.0 -lgthread-2.0 -lgmodule-2.0 -lxml2 -lglib-2.0
mipsel-oe-linux-strip -s mipsel/gstplayer_gstreamer1.0


exit 0

./build/tmp/sysroots/dm500hd/usr/include/gstreamer-1.0/gst/gst.h
./build/tmp/deploy/licenses/gstreamer1.0/gst.h
./build/tmp/work/mips32el-oe-linux/gstreamer1.0/1.4.5-r0/package/usr/include/gstreamer-1.0/gst/gst.h
./build/tmp/work/mips32el-oe-linux/gstreamer1.0/1.4.5-r0/image/usr/include/gstreamer-1.0/gst/gst.h
./build/tmp/work/mips32el-oe-linux/gstreamer1.0/1.4.5-r0/sysroot-destdir/usr/include/gstreamer-1.0/gst/gst.h
./build/tmp/work/mips32el-oe-linux/gstreamer1.0/1.4.5-r0/packages-split/gstreamer1.0-dev/usr/include/gstreamer-1.0/gst/gst.h
./build/tmp/work/mips32el-oe-linux/gstreamer1.0/1.4.5-r0/gstreamer-1.4.5/gst/gst.h
./build/tmp/work/mips32el-oe-linux/gstreamer1.0/1.4.5-r0/license-destdir/gstreamer1.0/gst.h
    