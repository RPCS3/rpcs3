#!/bin/sh

echo ---------------
echo Building USBnull
echo ---------------

curdir=`pwd`


if test "${USBnullOPTIONS+set}" != set ; then
export USBnullOPTIONS=""
fi

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake -a
autoconf

./configure ${USBnullOPTIONS} --prefix=${PCSX2PLUGINS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi
