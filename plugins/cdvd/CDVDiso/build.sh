#!/bin/sh

curdir=`pwd`

echo ----------------
echo Building CDVDiso
echo ----------------

cd ${curdir}/src/Linux
make $@

# copy the files
if [ -s cfgCDVDiso ] && [ -s libCDVDiso.so ]
then
cp cfgCDVDiso libCDVDiso.so ${PCSX2PLUGINS}
fi
