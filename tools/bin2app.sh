#!/bin/sh

# Probably self-explanatory: This batch file compiles a single souce image into a
# CPP header file for use by pcsx2.
#
#  bin2cpp.sh   SrcImage
#
# Parameters
#   SrcImage - Complete filename with extension.
#

$1/tools/bin/bin2cpp $2
