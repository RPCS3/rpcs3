#!/bin/sh

echo ----------------------------------------
echo Building Graphics Synthesizer plugins...
echo ----------------------------------------

curdir=`pwd`

cd ${curdir}/zerogs
sh build.sh $@
