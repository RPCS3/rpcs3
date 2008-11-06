#!/bin/sh

echo ------------------------
echo Building DEV9 plugins...
echo ------------------------
curdir=`pwd`

cd ${curdir}/dev9null
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi