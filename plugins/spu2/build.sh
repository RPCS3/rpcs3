#!/bin/sh

curdir=`pwd`

echo ------------------------
echo Building SPU2 plugins...
echo ------------------------

cd ${curdir}/zerospu2
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi

cd ${curdir}/PeopsSPU2
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi

cd ${curdir}/SPU2null
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi