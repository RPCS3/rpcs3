#!/bin/sh
#
# Basic sanity check for thumbnail
#
. ${srcdir:-.}/common.sh

outfile1=o-tiffcp-thumbnail-in.tif
outfile2=o-tiffcp-thumbnail-out.tif
f_test_convert "${TIFFCP} -c g3:1d" "${IMG_MINISWHITE_1C_1B}" "${outfile1}"
f_test_convert "${THUMBNAIL}" "${outfile1}" "${outfile2}"