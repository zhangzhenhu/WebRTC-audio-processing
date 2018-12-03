#!/bin/sh


cd ./webrtc-audio-processing/
./build.sh

cmake ./CMakeLists.txt

make


