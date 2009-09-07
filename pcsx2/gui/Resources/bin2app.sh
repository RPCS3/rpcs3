#!/bin/sh

# Probably self-explanatory: This batch file compiles a single souce image into a
# CPP header file for use by pcsx2.
#
#  bin2cpp.sh   SrcImage
#
# Parameters
#   SrcImage - Complete filename with extension.
#

../../../Tools/bin2cpp/bin2cpp %1