#!/bin/sh

curdir=`pwd`

echo -----------------
echo Building CDVDnull
echo -----------------
cd ${curdir}/Src
make $@

# copy the files
cp libCDVDnull.so ${PCSX2PLUGINS}
