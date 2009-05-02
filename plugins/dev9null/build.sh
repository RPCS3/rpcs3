#!/bin/sh

echo ---------------
echo Building DEV9null
echo ---------------

curdir=`pwd`


if test "${DEV9nullOPTIONS+set}" != set ; then
export DEV9nullOPTIONS=""
fi

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake -a
autoconf

./configure ${DEV9nullOPTIONS} --prefix=${PCSX2PLUGINS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi
