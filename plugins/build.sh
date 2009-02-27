#!/bin/sh

curdir=`pwd`

buildplugin() {
cd ${curdir}/$1
sh build.sh $2

if [ $? -ne 0 ]
then
exit 1
fi
}

buildplugin CDVDnull $@
buildplugin dev9null $@
buildplugin FWnull $@
buildplugin USBnull $@
buildplugin SPU2null $@

buildplugin zerogs $@
buildplugin zeropad $@
buildplugin zerospu2 $@

buildplugin PeopsSPU2 $@

buildplugin CDVDiso $@
buildplugin CDVDisoEFP $@
buildplugin CDVDlinuz $@