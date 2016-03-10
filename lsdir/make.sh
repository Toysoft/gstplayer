#!/bin/bash


function usage {
   echo "Usage:"
   echo "./make.sh armv7|armv5t|sh4|mipsel|mipsel_1|i686"
   exit 1
}

function build {
    CROSS_COMPILE=$1
    PLATFORM=$2
    PARAMS=$3
    mkdir -p out/$PLATFORM
    $CROSS_COMPILE"gcc" -O2  -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE=1  $PARAMS src/*.c -o out/$PLATFORM/lsdir
    $CROSS_COMPILE"strip" -s out/$PLATFORM/lsdir
}


if [ $# -ne 1 ]
then
    usage
fi

PLATFORM=$1
case "$PLATFORM" in
        armv7)
            export TOOLCHAIN_NAME="arm-oe-linux-gnueabi"
            export PATH="/mnt/new2/vusolo4k/openvuplus_3.0/build/vusolo4k/tmp/sysroots/i686-linux/usr/bin/arm-oe-linux-gnueabi/":$PATH
            export SYSROOT="/mnt/new2/vusolo4k/openvuplus_3.0/build/vusolo4k/tmp/sysroots/vusolo4k"
            export CROSS_COMPILE=$TOOLCHAIN_NAME"-"

            build $CROSS_COMPILE "armv7"  " -march=armv7-a -mfloat-abi=hard -mfpu=neon --sysroot=$SYSROOT "
            ;;
        armv5t)
            export TOOLCHAIN_NAME="arm-oe-linux-gnueabi"
            BASE_PATH="/mnt/new2/openatv2/build-enviroment/builds/openatv/release/cube/tmp/sysroots/"
            export PATH=$BASE_PATH"i686-linux/usr/bin/arm-oe-linux-gnueabi/":$PATH
            export SYSROOT=$BASE_PATH"cube"
            export CROSS_COMPILE=$TOOLCHAIN_NAME"-"
            
            build $CROSS_COMPILE "armv5t" " -mfloat-abi=softfp -mtune=cortex-a9 -mfpu=vfpv3-d16 -pipe --sysroot=$SYSROOT "
            ;;
        sh4)
            build "/opt/STM/STLinux-2.4/devkit/sh4/bin/sh4-linux-" "sh4"
            ;;
         
        mipsel)
            build "/opt/cross/mipsel-unknown-linux-gnu/bin/mipsel-unknown-linux-gnu-" "mipsel"
            ;;

        mipsel_1)
            build "/opt/cross/mipsel-tuxbox-linux-gnu/bin/mipsel-tuxbox-linux-gnu-" "mipsel"
            ;;
         
        i686)
            build "" "i686"
            ;;         
        *)
            usage
            exit 1
esac
