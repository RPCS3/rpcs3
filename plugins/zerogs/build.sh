#!/bin/sh

curdir=`pwd`

echo ----------------------
echo Building ZeroGS OpenGL
echo ----------------------

if test "${ZEROGSOPTIONS+set}" != set ; then
export ZEROGSOPTIONS="--enable-sse2"
fi

cd ${curdir}/opengl

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake
autoconf
chmod +x configure
./configure ${ZEROGSOPTIONS} --prefix=${PCSX2PLUGINS}

make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi

#cp libZeroGSogl*.so* ${PCSX2PLUGINS}/
cp Win32/ps2hw.dat ${PCSX2PLUGINS}/