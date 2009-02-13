#!/bin/sh

echo -----------------
echo Building dev9null
echo -----------------

curdir=`pwd`

cd ${curdir}/src
make clean
make $@

cp libDEV9null.so ${PCSX2PLUGINS}
