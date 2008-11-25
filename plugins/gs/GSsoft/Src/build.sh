#!/bin/sh

curdir=`pwd`

echo ----------------------
echo Building GSSoft
echo ----------------------

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake
autoconf
chmod +x configure
./configure --enable-sse2 --prefix=${PCSX2PLUGINS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi
