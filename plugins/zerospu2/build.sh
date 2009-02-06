#!/bin/sh

curdir=`pwd`

echo -----------------
echo Building ZeroSPU2
echo -----------------

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake -a
autoconf
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

#cp libZeroSPU2*.so* ${PCSX2PLUGINS}
