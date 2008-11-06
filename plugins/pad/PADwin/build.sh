#!/bin/sh

echo ---------------
echo Building PADwin
echo ---------------

curdir=`pwd`

cd ${curdir}/Src
make $@

if [ -s libPADwin.so ]
then
cp libPADwin.so ${PCSX2PLUGINS}
fi
