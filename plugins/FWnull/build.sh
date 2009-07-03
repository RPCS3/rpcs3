#!/bin/sh

echo ---------------
echo Building FWnull
echo ---------------

curdir=`pwd`


if test "${FWnullOPTIONS+set}" != set ; then
export FWnullOPTIONS=""
fi

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake -a
autoconf

./configure ${FWnullOPTIONS} --prefix=${PCSX2PLUGINS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi