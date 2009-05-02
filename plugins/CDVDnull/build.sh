#!/bin/sh

echo ---------------
echo Building CDVDnull
echo ---------------

curdir=`pwd`


if test "${CDVDnullOPTIONS+set}" != set ; then
export CDVDnullOPTIONS=""
fi

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake -a
autoconf

./configure ${CDVDnullOPTIONS} --prefix=${PCSX2PLUGINS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi
