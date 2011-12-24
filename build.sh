#!/bin/bash

flags=""
args="$@"
clean_build=false

for f in $args; do
	if [ "$f" = "gsdx" ] ; then
		flags="$flags -DFORCE_INTERNAL_SDL=TRUE"
	fi
	if [ "$f" = "dev" ] ; then
		flags="$flags -DCMAKE_BUILD_TYPE=Devel"
	fi
	if [ "$f" = "debug" ] ; then
		flags="$flags -DCMAKE_BUILD_TYPE=Debug"
	fi
	if [ "$f" = "release" ] ; then
		flags="$flags -DCMAKE_BUILD_TYPE=Release"
	fi
	if [ "$f" = "clean" ] ; then
		clean=true
	fi
done 

rm install_log.txt

if [ $flags ]; then
	echo "Building pcsx2 with $flags"
	echo "Building pcsx2 with $flags" > install_log.txt
fi

if [ ! -d "build" ]; then
    mkdir build
fi
cd build

cmake $flags .. 2>&1 | tee -a ../install_log.txt 
if [ $clean ]; then
	make clean 2>&1 | tee -a ../install_log.txt 
fi
make 2>&1 | tee -a ../install_log.txt 
make install 2>&1 | tee -a ../install_log.txt 

cd ..