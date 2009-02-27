#!/bin/sh

curdir=`pwd`

if test "${ZEROSPU2OPTIONS+set}" != set ; then
export ZEROSPU2OPTIONS=""
fi

if [ $# -gt 0 ] && [ $1 = "all" ]
then

cd ../../3rdparty/SoundTouch/
sh build.sh $@
cd $curdir

rm libSoundTouch.a
cp ../../3rdparty/SoundTouch/libSoundTouch.a ./

echo -----------------
echo Building ZeroSPU2
echo -----------------

aclocal
automake -a
autoconf
./configure ${ZEROSPU2OPTIONS} --prefix=${PCSX2PLUGINS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi

#cp libZeroSPU2*.so* ${PCSX2PLUGINS}
