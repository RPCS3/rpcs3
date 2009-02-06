#!/bin/sh

curdir=`pwd`

function buildplugin {
cd ${curdir}/$1
sh build.sh $2

if [ $? -ne 0 ]
then
exit 1
fi
}

#buildplugin gs $@
#buildplugin cdvd $@
#buildplugin dev9 $@
#buildplugin fw $@
#buildplugin pad $@
#buildplugin spu2 $@
#buildplugin usb $@

buildplugin CDVDnull $@
buildplugin dev9null $@
buildplugin FWnull $@
buildplugin USBnull $@

buildplugin zerogs $@
buildplugin zeropad $@
buildplugin zerospu2 $@

buildplugin PeopsSPU2 $@

buildplugin CDVDiso $@
buildplugin CDVDisoEFP $@
buildplugin CDVDlinuz $@