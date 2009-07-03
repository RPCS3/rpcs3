#!/bin/sh

curdir=`pwd`

echo ----------------
echo Building OnePAD
echo ----------------

if [ $# -gt 0 ] && [ $1 = "all" ]
then

#if possible
aclocal
automake -a
autoconf
chmod +x configure
./configure --prefix=${PCSX2PLUGINS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi

