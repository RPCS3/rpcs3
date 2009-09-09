#!/bin/sh

# Usage: sh build.sh [option]
# option can be all (rebuilds everything), clean, or nothing (incremental build)
# Modify the individual build.sh for specific program options like debug symbols
#This is just for building the plugins; pcsx2 is build using codeblocks.


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
echo "Building the Pcsx2 Plugins."
echo "Note: binaries generated are 32 bit, and require 32 bit versions of all dependencies."
cd ${curdir}/plugins
sh build.sh $option

if [ $? -ne 0 ]
then
echo Error with building plugins
exit 1
fi
