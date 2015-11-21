OUT_NAME="../out/i686/uchardet"
cd src
g++ -DVERSION='"0.0.2"' -fdata-sections -ffunction-sections -Wl,--gc-sections -Os -o $OUT_NAME *.cpp tools/*.cpp
strip -s $OUT_NAME

