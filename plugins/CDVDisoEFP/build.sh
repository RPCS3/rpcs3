#!/bin/sh

curdir=`pwd`

echo -------------------
echo Building CDVDisoEFP
echo -------------------

cd ${curdir}/src/Linux
make $@

if [ -s cfgCDVDisoEFP ] && [ -s libCDVDisoEFP.so ]
then
# copy the files
cp cfgCDVDisoEFP libCDVDisoEFP.so ${PCSX2PLUGINS}
fi
