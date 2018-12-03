#!/bin/sh

# mac 下需要安装如下工具
# brew install libtool
# brew install automake
# brew install libsndfile



cd ./webrtc-audio-processing/
sh ./build.sh

cmake ./CMakeLists.txt

make


