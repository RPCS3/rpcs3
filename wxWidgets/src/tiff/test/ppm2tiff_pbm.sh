#!/bin/sh
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$IMG_MINISWHITE_1C_1B_PBM"
outfile="o-ppm2tiff_pbm.tiff"
f_test_convert "$PPM2TIFF" $infile $outfile
f_tiffinfo_validate $outfile
