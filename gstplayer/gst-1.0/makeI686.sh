#!/bin/bash

I0="-I/usr/include/gstreamer-1.0"
I1="-I/usr/include/glib-2.0/"
I2="-I/usr/lib/i386-linux-gnu/glib-2.0/include/"
I3="-I/usr/include/libxml2/"
I4="-I../common"
I5=""


rm -rf i686
mkdir i686
gcc -g  -D PLATFORM_I686 $I0 $I1 $I2 $I3 $I4 $I5 *.c ../common/*.c -o i686/gstplayer_gstreamer1.0 -Wfatal-errors -lgstreamer-1.0 -lgstbase-1.0 -lgstpbutils-1.0 -lgobject-2.0 -lgthread-2.0 -lglib-2.0 -lgmodule-2.0 -lxml2
strip -s i686/gstplayer_gstreamer1.0

    
