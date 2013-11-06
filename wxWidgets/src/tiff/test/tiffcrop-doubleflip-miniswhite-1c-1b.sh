#!/bin/sh
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$srcdir/images/miniswhite-1c-1b.tiff"
outfile="o-tiffcrop-doubleflip-miniswhite-1c-1b.tiff"
f_test_convert "$TIFFCROP -F both" $infile $outfile
f_tiffinfo_validate $outfile
