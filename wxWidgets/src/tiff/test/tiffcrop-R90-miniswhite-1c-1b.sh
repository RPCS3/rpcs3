#!/bin/sh
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$srcdir/images/miniswhite-1c-1b.tiff"
outfile="o-tiffcrop-R90-miniswhite-1c-1b.tiff"
f_test_convert "$TIFFCROP -R90" $infile $outfile
f_tiffinfo_validate $outfile
