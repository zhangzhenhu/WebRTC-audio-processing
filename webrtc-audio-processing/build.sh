#!/bin/sh
__script_dir=$(cd "$(dirname "$0")"; pwd)


./autogen.sh --prefix=${__script_dir}/release

make

make install


