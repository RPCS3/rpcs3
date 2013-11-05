#!/bin/sh
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$IMG_RGB_3C_8B_PPM"
outfile="o-ppm2tiff_ppm.tiff"
f_test_convert "$PPM2TIFF" $infile $outfile
f_tiffinfo_validate $outfile
