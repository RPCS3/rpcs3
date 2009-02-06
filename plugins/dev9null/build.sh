#!/bin/sh

echo Building dev9null...
curdir=`pwd`

cd ${curdir}/src
make $@

cp libDEV9null.so ${PCSX2PLUGINS}
