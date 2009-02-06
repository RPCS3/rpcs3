#!/bin/sh

# Usage: sh build.sh [option]
# option can be all (rebuilds everything), clean, or nothing (incremental build)
# Modify the individual build.sh for specific program options like debug symbols
# Uncomment if building by itself, rather then with all the plugins

#Normal
export PCSX2OPTIONS="--enable-sse3 --enable-sse4 --prefix `pwd`"


#Optimized, but a devbuild
#export PCSX2OPTIONS="--enable-sse3 --enable-sse4 --enable-devbuild --prefix `pwd`"

#Debug / Devbuild version
#export PCSX2OPTIONS="--enable-debug --enable-devbuild --enable-sse3 --prefix `pwd`"

# Make sure we have plugins, and bring the normal plugins in.
sh fetch.sh

option=$@
export PCSX2PLUGINS="`pwd`/bin/plugins"
curdir=`pwd`

cd ${curdir}/plugins
sh build.sh $option

if [ $? -ne 0 ]
then
echo Error with building plugins
exit 1
fi

cd ${curdir}/pcsx2
sh build.sh $option

if [ $? -ne 0 ]
then
echo Error with building pcsx2
exit 1
fi
