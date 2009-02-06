#!/bin/sh
# Pcsx2 - Pc Ps2 Emulator
# Copyright (C) 2002-2008  Pcsx2 Team
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#

#Normal
#export PCSX2OPTIONS="--enable-debug --enable-devbuild --enable-sse3 --enable-sse4 --prefix `pwd`"

echo ---------------
echo Building Pcsx2 
echo ---------------

if [ $# -gt 0 ] && [ $1 = "all" ]
then

aclocal
automake
autoconf
chmod +x configure
./configure ${PCSX2OPTIONS}
make clean
make install

else
make $@
fi

if [ $? -ne 0 ]
then
exit 1
fi
