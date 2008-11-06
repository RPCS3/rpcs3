#!/bin/sh

# Usage: sh build.sh [option]
# option can be all (rebuilds everything), clean, or nothing (incremental build)
# Modify the individual build.sh for specific program options like debug symbols
export PCSX2OPTIONS="--enable-devbuild --enable-sse2 --prefix `pwd`"
export PCSX2PLUGINS="`pwd`/bin/plugins"
curdir=`pwd`

cd ${curdir}/plugins
sh build.sh $@

if [ $? -ne 0 ]
then
echo Error with building plugins
exit 1
fi

cd ${curdir}/pcsx2
#mkdir debugdev
#cd debugdev
#sh ../build.sh $@
sh build.sh $@

if [ $? -ne 0 ]
then
echo Error with building pcsx2
exit 1
fi
