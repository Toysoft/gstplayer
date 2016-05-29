export PATH="/opt/STM1/STLinux-2.4/devkit/sh4/bin/":$PATH

STATIC_LINK=""
BIN_POSTFIX=""
if [ "$1" == "static" ];
then
    STATIC_LINK=" -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic " #-static-libstdc++
    BIN_POSTFIX="_static_libstdc++"
fi

OUT_NAME="../out/sh4/uchardet"$BIN_POSTFIX
cd src

sh4-linux-g++ -DVERSION='"0.0.5"' -fdata-sections -ffunction-sections -Wl,--gc-sections -Os -o $OUT_NAME *.cpp tools/*.cpp LangModels/*.cpp $STATIC_LINK
sh4-linux-strip -s $OUT_NAME

