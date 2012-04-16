#!/bin/sh -e

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

# This script is a small wrapper to the PCSX2 exectuable. The purpose is to
# launch PCSX2 from the same repository every times.
# Rationale: There is no guarantee on the directory when PCSX2 is launched from a shortcut.
#            This behavior trigger the first time wizards everytime...

current_script=$0

[ $current_script = "launch_pcsx2_linux.sh" ] || [ $current_script = "sh" ] || [ $current_script = "bash" ] && \
    echo "Error the script was directly 'called'" && \
    echo "Use either /absolute_path/launch_pcsx2_linux.sh or ./relative_path/launch_pcsx2_linux.sh" && exit 1;

# Note: sh (dash on debian) does not support pushd, popd...
# Save current directory
PWD_old=$PWD

# Go to the script directory
cd `dirname $current_script`

# Setup LD_PRELOAD to work-around issue 1003
SDL_SO=`pwd`/plugins/libpcsx2_SDL.so
if [ -e "$SDL_SO" ]
then
    echo "INFO: LD_PRELOAD $SDL_SO"
    if [ -n "$LD_PRELOAD" ]
    then
        LD_PRELOAD="$SDL_SO:$LD_PRELOAD"
    else
        LD_PRELOAD="$SDL_SO"
    fi
fi

# Launch PCSX2
if [ -x pcsx2 ]
then
    ./pcsx2
else
    echo "Error PCSX2 not found"
    echo "Maybe the script was directly 'called'"
    echo "Use either /absolute_path/launch_pcsx2_linux.sh or ./relative_path/launch_pcsx2_linux.sh"
    exit 1
fi

# Go back to the old directory
cd $PWD_old
