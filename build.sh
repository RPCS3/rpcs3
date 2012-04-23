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
		clean_build=true
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
if [ $clean_build = true ]; then
	echo "Doing a clean build."
	make clean 2>&1 | tee -a ../install_log.txt 
fi
CORE=`grep -w -c processor /proc/cpuinfo`
make -j $CORE 2>&1 | tee -a ../install_log.txt 
make install 2>&1 | tee -a ../install_log.txt 

cd ..
