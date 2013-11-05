#!/bin/sh
#
# Basic sanity check for tiffps with PostScript Level 1 encapsulated output
#
. ${srcdir:-.}/common.sh
f_test_stdout "${TIFF2PS} -e -1" "${IMG_MINISWHITE_1C_1B}" "o-tiff2ps-EPS1.ps"