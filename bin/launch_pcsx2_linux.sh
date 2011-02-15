#!/bin/sh -e

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
if [ -e pcsx2 ] ; then
    ./pcsx2
else
    echo "Error PCSX2 not found"
    echo "Maybe the script was directly 'called'"
    echo "Use either /absolute_path/launch_pcsx2_linux.sh or ./relative_path/launch_pcsx2_linux.sh"
    exit 1
fi

# Go back to the old directory
cd $PWD_old
