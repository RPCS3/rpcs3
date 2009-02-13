#!/bin/sh

curdir=`pwd`

echo -----------------
echo Building CDVDnull
echo -----------------
cd ${curdir}/Src
make clean
make $@

# copy the files
cp libCDVDnull.so ${PCSX2PLUGINS}
