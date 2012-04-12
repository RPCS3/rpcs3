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

# Script default parameter
MAX_DEPTH=1
DIR=$PWD
CMAKE_FILE=CMakeLists.txt
SKIP_H=FALSE

## Help message
help()
{
    cat <<EOF
This script detects '.cpp/.h' files that are in directory (and sub) but not in the current cmake.
options:
-dir   <dirname>   : give the directory to check. Default '.'
-help              : print this help message and exit
-max_depth <depth> : how many depth to check missing file. Default 1
-skip              : skip the check of '.h' file. Disable by default
EOF

    exit 0
}

## Handle option
while [ -n "$1" ]; do
case $1 in
    -help|-h) help;shift 1;; # appel de la fonction help
    -skip|-s) SKIP_H=TRUE;shift 1;;
    -dir|-d) DIR=$2;shift 2;;
    -max_depth|-m) MAX_DEPTH=$2;shift 2;;
    --) shift;break;;
    -*) echo "ERROR: $1 option does not exists. Use -h for help";exit 1;;
    *)  break;;
esac
done


## Main sript
for file in `find $DIR -maxdepth $MAX_DEPTH -name "*.cpp"` ; do
    PATTERN=`basename $file`
    grep $PATTERN $DIR/$CMAKE_FILE > /dev/null || echo $file is missing in $CMAKE_FILE
done

if [ "${SKIP_H}" = "FALSE" ] ; then
    for file in `find $DIR -maxdepth $MAX_DEPTH -name "*.h"` ; do
        PATTERN=`basename $file`
        grep $PATTERN $DIR/$CMAKE_FILE > /dev/null || echo $file is missing in $CMAKE_FILE
    done
fi
exit
