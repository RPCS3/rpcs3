#!/bin/sh
#
# Basic sanity check for tiffps with PostScript Level 2 output
#
. ${srcdir:-.}/common.sh
f_test_stdout "${TIFF2PS} -a -p -2" "${IMG_MINISWHITE_1C_1B}" "o-tiff2ps-PS2.ps"