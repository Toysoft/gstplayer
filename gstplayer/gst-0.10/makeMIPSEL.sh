#!/bin/bash

INCBASE="-I/mnt/new/openpli40/openpli-oe-core/build/tmp/work/mips32el-oe-linux"
I0="$INCBASE/gst-plugins-base/0.10.36.1+gitAUTOINC+bdb3316347-r2/sysroot-destdir/usr/include/gstreamer-0.10/"
I1="$INCBASE/gstreamer/0.10.36.1+gitAUTOINC+1bcabb9a23-r1/sysroot-destdir/usr/include/gstreamer-0.10"
I2="$INCBASE/glib-2.0/1_2.40.0-r0/sysroot-destdir/usr/include/glib-2.0"
I3="$INCBASE/glib-2.0/1_2.40.0-r0/sysroot-destdir/usr/include/glib-2.0/include"
I4="$INCBASE/libxml2/2.9.1-r0/sysroot-destdir/usr/include/libxml2"
I5="$INCBASE/glib-2.0/1_2.40.0-r0/sysroot-destdir/usr/lib/glib-2.0/include/"
I6="-I../common"

LIBBASE="-L/mnt/new/openpli40/openpli-oe-core/build/tmp/work/mips32el-oe-linux"
L0="$LIBBASE/gst-plugins-base/0.10.36.1+gitAUTOINC+bdb3316347-r2/image/usr/lib"
L1="$LIBBASE/gstreamer/0.10.36.1+gitAUTOINC+1bcabb9a23-r1/sysroot-destdir/usr/lib"
L2="" #$LIBBASE/glib-2.0/1_2.40.0-r0/sysroot-destdir/usr/lib
L3="$LIBBASE/libffi/3.1-r0/sysroot-destdir/usr/lib"
L4="$LIBBASE/libxml2/2.9.1-r0/sysroot-destdir/usr/lib"
L5="-L/mnt/new/openpli40/openpli-oe-core/build/tmp/work/mips32el-oe-linux/zlib/1.2.8-r0/sysroot-destdir/lib"

rm -rf mipsel
mkdir mipsel
export PATH="/mnt/new/openpli40/openpli-oe-core/build/tmp/sysroots/i686-linux/usr/bin/mipsel-oe-linux/":$PATH
mipsel-oe-linux-gcc -D PLATFORM_MIPSEL -mel -mabi=32 -mhard-float -march=mips32 --sysroot="/mnt/new/openpli40/openpli-oe-core/build/tmp/sysroots/dm8000" $I0 $I1 $I2 $I3 $I4 $I5 $I6 *.c ../common/*.c -o mipsel/gstplayer_gstreamer0.10 $L0 $L1 $L2 $L3 $L4 $L5  -Wfatal-errors -lgstreamer-0.10 -lgstbase-0.10 -lgstpbutils-0.10 -lgobject-2.0 -lgthread-2.0 -lgmodule-2.0 -lxml2 -lglib-2.0
mipsel-oe-linux-strip -s mipsel/gstplayer_gstreamer0.10

    