#!/bin/bash

export PATH=/home/sulge/e2/tdt/tdt/tufsbox/devkit/sh4/bin/:$PATH
export CROSS_COMPILE="sh4-linux-"

INCBASE="-I/home/sulge/e2/tdt/tdt"
I0="$INCBASE/tufsbox/cdkroot/usr/include/gstreamer-0.10/"
I1="$INCBASE/tufsbox/cdkroot/usr/include/glib-2.0"
I2="$INCBASE/tufsbox/cdkroot/usr/include"
I3="$INCBASE/tufsbox/cdkroot/usr/include/libxml2/libxml"
I4="$INCBASE/tufsbox/cdkroot/usr/include/libxml2"
I5="$INCBASE/tufsbox/cdkroot/usr/lib/glib-2.0/include"
I6="-I../common"

LIBBASE="-L/home/sulge/e2/tdt/tdt"
L0="$LIBBASE/tufsbox/cdkroot/usr/lib/"
L1="$LIBBASE/tufsbox/cdkroot/lib"
L2=""
L3=""
L4=""
L5=""


rm -rf sh4
mkdir sh4
"$CROSS_COMPILE"gcc -g  -D PLATFORM_I686 $I0 $I1 $I2 $I3 $I4 $I5 $I6 *.c ../common/*.c -o sh4/gstplayer_gstreamer0.10 $L0 $L1 $L2 $L3 $L4 $L5 -Wfatal-errors -lgstreamer-0.10 -lgstbase-0.10 -lgstpbutils-0.10 -lgobject-2.0 -lgthread-2.0 -lglib-2.0 -lgmodule-2.0 -lxml2
"$CROSS_COMPILE"strip -s sh4/gstplayer_gstreamer0.10

    
