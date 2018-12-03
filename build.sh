#!/bin/sh

# mac 下需要安装如下两个工具
# brew install libtool
# brew install automake



cd ./webrtc-audio-processing/
sh ./build.sh

cmake ./CMakeLists.txt

make


