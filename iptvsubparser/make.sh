#!/bin/bash

set -e

function usage {
   echo "Usage:"
   echo "$0 platform"
   echo "platform:       i686 | sh4 | mipsel | mipsel_softfpu | armv7 | armv5t"
   exit 1
}

if [ "$#" -ne 1 ]; 
then
    usage
fi

EPLATFORM=$1

if [ "$EPLATFORM" != "i686" -a "$EPLATFORM" != "sh4" -a "$EPLATFORM" != "mipsel" -a "$EPLATFORM" != "mipsel_softfpu" -a "$EPLATFORM" != "armv7" -a "$EPLATFORM" != "armv5t" ];
then
    echo "Please give supported platform (i686|sh4|mipsel|mipsel_softfpu|armv7|armv5t) version!"
    usage
fi

OUTPUT_FILE_NAME='_subparser.so'
case "$EPLATFORM" in
    i686)
        CROSS_COMPILE=""
        CFLAGS="-I/usr/include -I/usr/include/python2.7"
        ;;
    sh4)
        BASE_PATH="/home/sulge/e2/tdt/tdt/tufsbox/"
        CROSS_COMPILE="sh4-linux-"
        PATH=$BASE_PATH"devkit/sh4/bin/":$PATH
        CFLAGS=" -I$BASE_PATH/cdkroot/ -L$BASE_PATH/cdkroot/ -I$BASE_PATH/cdkroot/usr/include/python2.7/ "
        LDFLAGS=" -L$BASE_PATH/cdkroot/ -L$BASE_PATH/cdkroot/ "
        ;;
    mipsel)
        BASE_PATH="/mnt/new2/xspeedlx1/build-enviroment/builds/openatv/release/et4x00/tmp/sysroots/"
        CROSS_COMPILE="mipsel-oe-linux-"
        PATH=$BASE_PATH"i686-linux/usr/bin/mipsel-oe-linux/":$PATH
        SYSROOT=$BASE_PATH"et4x00"
        CFLAGS="--sysroot=$SYSROOT -mel -mabi=32 -march=mips32 -I$SYSROOT/usr/include/python2.7/"
        OUTPUT_FILE_NAME='_subparser.so.fpu' # hard float
        ;;
    mipsel_softfpu)
        EPLATFORM="mipsel" # temporary, because there is no platform mipsel_softfpu - soft fpu will work also on CPU with hard CPU
        BASE_PATH="/mnt/new2/softFPU/openpli/build/tmp/sysroots/"
        CROSS_COMPILE="mipsel-oe-linux-"
        PATH=$BASE_PATH"i686-linux/usr/bin/mipsel-oe-linux/":$PATH
        SYSROOT=$BASE_PATH"et4x00"
        CFLAGS="--sysroot=$SYSROOT -mel -mabi=32 -msoft-float -march=mips32 -I$SYSROOT/usr/include/python2.7/"
        ;;
    armv7)
        BASE_PATH="/mnt/new2/vusolo4k/openvuplus_3.0/build/vusolo4k/tmp/sysroots/"
        CROSS_COMPILE="arm-oe-linux-gnueabi-"
        PATH=$BASE_PATH"i686-linux/usr/bin/arm-oe-linux-gnueabi/":$PATH
        SYSROOT=$BASE_PATH"vusolo4k"
        CFLAGS="--sysroot=$SYSROOT -march=armv7-a -mfloat-abi=hard -mfpu=neon -I$SYSROOT/usr/include/python2.7/"
        ;;
    armv5t)
        BASE_PATH="/mnt/new2/openatv2/build-enviroment/builds/openatv/release/cube/tmp/sysroots/"
        CROSS_COMPILE="arm-oe-linux-gnueabi-"
        PATH=$BASE_PATH"i686-linux/usr/bin/arm-oe-linux-gnueabi/":$PATH
        SYSROOT=$BASE_PATH"cube"
        CFLAGS="--sysroot=$SYSROOT -mfloat-abi=softfp -mtune=cortex-a9 -mfpu=vfpv3-d16 -I$SYSROOT/usr/include/python2.7/"
        ;;
    *)
        usage
        exit 1
esac

CFLAGS="$CFLAGS -pipe -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE "

SOURCE_FILES="$PWD/src/subparsermodule.c "

# VLC part
SOURCE_FILES+="$PWD/src/vlc/src/subtitle.c "
CFLAGS+="-I$PWD/src/vlc/include "

# ffmpeg part
SOURCE_FILES+="$PWD/src/ffmpeg/src/htmlsubtitles.c "
CFLAGS+="-I$PWD/src/ffmpeg/include "

# expat part
SOURCE_FILES+="$PWD/src/expat-2.2.0/*.c "
CFLAGS+="-I$PWD/src/expat-2.2.0 -DHAVE_EXPAT_CONFIG_H "


# ttml part
SOURCE_FILES+="$PWD/src/ttml/src/*.c "
CFLAGS+="-I$PWD/src/ttml/include "

rm -rf $PWD/out/$EPLATFORM/$OUTPUT_FILE_NAME
mkdir -p $PWD/out/$EPLATFORM/

OUT_FILE=$PWD/out/$EPLATFORM/$OUTPUT_FILE_NAME

"$CROSS_COMPILE"gcc -shared -DNDEBUG -Os -fdata-sections -ffunction-sections -Wall -Wstrict-prototypes -fPIC -DMAJOR_VERSION=0 -DMINOR_VERSION=2 $CFLAGS $LDFLAGS $SOURCE_FILES -o $OUT_FILE -Wl,--gc-sections
"$CROSS_COMPILE"strip -s $OUT_FILE

echo "DONE: $OUT_FILE"
