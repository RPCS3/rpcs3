#!/bin/sh

echo ----------------------------
echo Building FireWire plugins...
echo ----------------------------

curdir=`pwd`

cd ${curdir}/FWnull
sh build.sh $@

#cd ${curdir}/FWlinuz
#sh build.sh $@
