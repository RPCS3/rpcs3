#!/bin/sh

echo ---------------
echo Building Padnull
echo ---------------

curdir=`pwd`


if test "${PadnullOPTIONS+set}" != set ; then
export PadnullOPTIONS=""
fi

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake -a
autoconf

./configure ${PadnullOPTIONS} --prefix=${PCSX2PLUGINS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi