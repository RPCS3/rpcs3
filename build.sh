#!/bin/sh

# PCSX2 - PS2 Emulator for PCs
# Copyright (C) 2002-2011  PCSX2 Dev Team
#
# PCSX2 is free software: you can redistribute it and/or modify it under the terms
# of the GNU Lesser General Public License as published by the Free Software Found-
# ation, either version 3 of the License, or (at your option) any later version.
#
# PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with PCSX2.
# If not, see <http://www.gnu.org/licenses/>.

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
CORE=`grep -w -c processor /proc/cpuinfo`
make -j $CORE 2>&1 | tee -a ../install_log.txt 
make install 2>&1 | tee -a ../install_log.txt 

cd ..
