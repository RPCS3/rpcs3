#!/bin/sh
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$IMG_PALETTE_1C_8B_GIF"
outfile="o-gif2tiff.tiff"
f_test_convert "$GIF2TIFF" $infile $outfile
f_tiffinfo_validate $outfile
