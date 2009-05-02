#!/bin/sh

echo ---------------
echo Building SPU2null
echo ---------------

curdir=`pwd`


if test "${SPU2nullOPTIONS+set}" != set ; then
export SPU2nullOPTIONS=""
fi

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake -a
autoconf

./configure ${SPU2nullOPTIONS} --prefix=${PCSX2PLUGINS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi
