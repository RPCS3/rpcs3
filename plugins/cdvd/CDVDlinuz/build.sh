#!/bin/sh

curdir=`pwd`

echo ------------------
echo Building CDVDlinuz
echo ------------------

cd ${curdir}/Src/Linux
make $@

if [ -s cfgCDVDlinuz ] && [ -s libCDVDlinuz.so ]
then
# copy the files
cp cfgCDVDlinuz libCDVDlinuz.so ${PCSX2PLUGINS}
fi
