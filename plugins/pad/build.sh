#!/bin/sh

echo -----------------------
echo Building PAD plugins...
echo -----------------------

curdir=`pwd`

# disable PADwin for linux builds
#cd ${curdir}/PADwin
#sh build.sh $@

#if [ $? -ne 0 ]
#then
#exit 1
#fi

cd ${curdir}/zeropad
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi
