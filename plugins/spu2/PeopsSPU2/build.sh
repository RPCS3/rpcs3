#!/bin/sh

curdir=`pwd`

echo ------------------
echo Building PeopsSPU2
echo ------------------

make $@

if [ $? -ne 0 ]
then
exit 1
fi

if [ -s libspu2Peops*.so* ]
then
cp libspu2Peops*.so*  ${PCSX2PLUGINS}
fi

