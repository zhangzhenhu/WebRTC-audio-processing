#!/bin/sh


# mac 下可以用 glibtoolize 替代

if [ "$(uname)"=="Darwin" ];then
# Mac OS X 操作系统

glibtoolize

elif [ "$(expr substr $(uname -s) 1 5)"=="Linux" ] ; then
# GNU/Linux操作系统

libtoolize


elif [ "$(expr substr $(uname -s) 1 10)"=="MINGW32_NT" ] ; then

# Windows NT操作系统
exit 1;

fi



aclocal
automake --add-missing --copy
autoconf
./configure ${@}
