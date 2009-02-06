#!/bin/sh

curdir=`pwd`

echo -----------------
echo Building SPU2null
echo -----------------

cd ${curdir}/Src
make $@

cp libSPU2null.so ${PCSX2PLUGINS}
