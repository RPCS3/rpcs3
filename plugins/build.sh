#!/bin/sh

curdir=`pwd`

cd ${curdir}/gs
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi

cd ${curdir}/cdvd
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi

cd ${curdir}/dev9
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi

cd ${curdir}/fw
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi

cd ${curdir}/pad
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi

cd ${curdir}/spu2
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi

cd ${curdir}/usb
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi
