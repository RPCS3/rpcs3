#!/bin/sh

echo ----------------
echo Building USBnull
echo ----------------

curdir=`pwd`

cd ${curdir}/Linux
make $@

if [ $? -ne 0 ]
then
exit 1
fi

if [ -s cfgUSBnull ] && [ -s libUSBnull.so ]
then
cp cfgUSBnull libUSBnull.so ${PCSX2PLUGINS}
fi
