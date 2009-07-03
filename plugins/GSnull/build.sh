#!/bin/sh

echo ---------------
echo Building GSnull
echo ---------------

curdir=`pwd`


if test "${GSnullOPTIONS+set}" != set ; then
export GSnullOPTIONS=""
fi

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake -a
autoconf

./configure ${GSnullOPTIONS} --prefix=${PCSX2PLUGINS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi