#!/bin/sh

echo ----------------
echo Building CDVDiso
echo ----------------

curdir=`pwd`


if test "${CDVDisoOPTIONS+set}" != set ; then
export CDVDisoOPTIONS=""
fi

cd src

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake -a
autoconf

./configure ${CDVDisoOPTIONS} --prefix=${PCSX2PLUGINS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi
