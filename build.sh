#!/bin/sh

# Usage: sh build.sh [option]
# option can be all (rebuilds everything), clean, or nothing (incremental build)
# Modify the individual build.sh for specific program options like debug symbols
# Uncomment if building by itself, rather then with all the plugins

#Normal
#export PCSX2OPTIONS="--enable-sse3 --enable-sse4 --prefix `pwd`"

#Optimized, but a devbuild
export PCSX2OPTIONS="--enable-sse3 --enable-sse4 --enable-devbuild --prefix `pwd`"

#Debug / Devbuild version
#export PCSX2OPTIONS="--enable-debug --enable-devbuild --enable-sse3 --prefix `pwd`"

#Optimized, but a devbuild - with memcpy_fast_ enabled. - BROKEN!
#export PCSX2OPTIONS="--enable-sse3 --enable-sse4 --enable-devbuild --enable-memcpyfast --prefix `pwd`"

#Optimized, but a devbuild - with microVU enabled. - NOT FULLY IMPLEMENTED!!
#export PCSX2OPTIONS="--enable-sse3 --enable-sse4 --enable-devbuild --enable-microVU --prefix `pwd`"

#ZeroGS Normal mode
export ZEROGSOPTIONS="--enable-sse2"

#ZeroGS Debug mode
#export ZEROGSOPTIONS="--enable-debug --enable-devbuild --enable-sse2"

#ZeroSPU2 Debug mode (Don't enable right now)
#export ZEROSPU2OPTIONS="--enable-debug --enable-devbuild"

option=$@
export PCSX2PLUGINS="`pwd`/bin/plugins"
curdir=`pwd`

echo "Building the Pcsx2 Suite."
echo "Note: will not compile on Linux x64."
cd ${curdir}/plugins
sh build.sh $option

if [ $? -ne 0 ]
then
echo Error with building plugins
exit 1
fi

echo "Building Pcsx2."
echo "Note: will not compile on Linux x64."
cd ${curdir}/pcsx2
sh build.sh $option

if [ $? -ne 0 ]
then
echo Error with building pcsx2
exit 1
fi
