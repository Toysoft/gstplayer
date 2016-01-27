export PATH="/mnt/new2/xspeedlx1/build-enviroment/builds/openatv/release/et4x00/tmp/sysroots/i686-linux/usr/bin/mipsel-oe-linux/":$PATH
SYSROOT="/mnt/new2/xspeedlx1/build-enviroment/builds/openatv/release/et4x00/tmp/sysroots/et4x00"

STATIC_LINK=""
BIN_POSTFIX=""
if [ "$1" == "static" ];
then
    STATIC_LINK=" -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic " #-static-libstdc++
    BIN_POSTFIX="_static_libstdc++"
fi

OUT_NAME="../out/mipsel/uchardet"$BIN_POSTFIX
cd src

mipsel-oe-linux-g++ -DVERSION='"0.0.2"' -fdata-sections -ffunction-sections -Wl,--gc-sections -Os --sysroot=$SYSROOT -o $OUT_NAME *.cpp tools/*.cpp $STATIC_LINK
mipsel-oe-linux-strip -s $OUT_NAME

