#!/bin/sh

echo -----------------------
echo Building USB plugins...
echo -----------------------

curdir=`pwd`

cd ${curdir}/USBnull
sh build.sh $@

if [ $? -ne 0 ]
then
exit 1
fi
