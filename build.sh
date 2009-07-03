#!/bin/sh

# Usage: sh build.sh [option]
# option can be all (rebuilds everything), clean, or nothing (incremental build)
# Modify the individual build.sh for specific program options like debug symbols
# Uncomment if building by itself, rather then with all the plugins

#Normal
#export PCSX2OPTIONS="--prefix `pwd`"

#Optimized, but a devbuild
export PCSX2OPTIONS="--enable-devbuild --prefix `pwd`"

#Debug version (which now implies a devbuild)
#export PCSX2OPTIONS="--enable-debug --prefix `pwd`"

#Normal, but unoptimized
#export PCSX2OPTIONS="--disable-optimization --prefix `pwd`"

#Normal, with warnings
#export PCSX2OPTIONS="--enable-warnings --prefix `pwd`"

#ZeroGS Normal mode
export ZEROGSOPTIONS="--enable-sse2"

#ZeroGS Debug mode
#export ZEROGSOPTIONS="--enable-debug --enable-devbuild --enable-sse2"

#ZeroSPU2 Debug mode (Don't enable right now)
#export ZEROSPU2OPTIONS="--enable-debug --enable-devbuild"

#GSnull debug options.
#export GSnullOPTIONS="--enable-debug"

option=$@
export PCSX2PLUGINS="`pwd`/bin/plugins"
curdir=`pwd`

echo
echo "Building the Pcsx2 Suite."
echo "Note: binaries generated are 32 bit, and require 32 bit versions of all dependencies."
cd ${curdir}/plugins
sh build.sh $option

if [ $? -ne 0 ]
then
echo Error with building plugins
exit 1
fi

echo
echo "Building Pcsx2."
echo "Note: binaries generated are 32 bit, and require 32 bit versions of all dependencies."
cd ${curdir}/pcsx2
sh build.sh $option

if [ $? -ne 0 ]
then
echo Error with building pcsx2
exit 1
fi
