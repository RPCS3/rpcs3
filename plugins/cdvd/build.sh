#!/bin/sh

echo ------------------------
echo Building CDVD plugins...
echo ------------------------

curdir=`pwd`

cd ${curdir}/CDVDiso
sh build.sh $@

if [ $? -ne 0 ] 
then
exit 1
fi

cd ${curdir}/CDVDisoEFP
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi

cd ${curdir}/CDVDlinuz
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi

cd ${curdir}/CDVDnull
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi
