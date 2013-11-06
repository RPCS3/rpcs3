#!/bin/sh
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$srcdir/images/minisblack-2c-8b-alpha.tiff"
outfile="o-tiffcrop-doubleflip-minisblack-2c-8b-alpha.tiff"
f_test_convert "$TIFFCROP -F both" $infile $outfile
f_tiffinfo_validate $outfile
