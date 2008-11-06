#!/bin/sh

curdir=`pwd`

echo ------------------
echo Building CDVDpeops
echo ------------------

cd ${curdir}/src/Linux
make $@

# copy the files
if [ -s cfgCDVDpeops ] && [ -s libCDVDpeops.so ]
then
cp cfgCDVDpeops libCDVDpeops.so ${PCSX2PLUGINS}
fi
