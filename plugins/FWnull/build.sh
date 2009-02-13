#!/bin/sh

echo ---------------
echo Building FWnull
echo ---------------

curdir=`pwd`

cd ${curdir}/Linux
make clean
make $@

if [ -s cfgFWnull ] && [ -s libFWnull.so ]
then
cp cfgFWnull libFWnull.so ${PCSX2PLUGINS}
fi
