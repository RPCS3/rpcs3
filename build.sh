#!/bin/sh

flags=""
args="$@"
clean_build=false

for f in $*
do
	case $f in
		--sdl13)
			flags="$flags -DFORCE_INTERNAL_SDL=TRUE"
			;;
		--dev)
			flags="$flags -DCMAKE_BUILD_TYPE=Devel"
			;;
		--devel)
			flags="$flags -DCMAKE_BUILD_TYPE=Devel"
			;;
		--debug)
			flags="$flags -DCMAKE_BUILD_TYPE=Debug"
			;;
		--release)
			flags="$flags -DCMAKE_BUILD_TYPE=Release"
			;;
		--clean)
			clean_build=true
			;;
		*)
			# unknown option
			echo "Valid options are:"
			echo "--dev / --devel - Build pcsx2 as a Development build."
			echo "--debug - Build pcsx2 as a Debug build."
			echo "--release - Build pcsx2 as a Release build."
			echo "--clean - Do a clean build."
			echo "--sdl13 - Use the internal copy of sdl (needed for gsdx to use sdl)."
			exit 1;;
  	esac
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
if [ $clean_build = true ]; then
	echo "Doing a clean build."
	make clean 2>&1 | tee -a ../install_log.txt 
fi
make 2>&1 | tee -a ../install_log.txt 
make install 2>&1 | tee -a ../install_log.txt 

cd ..