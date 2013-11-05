#!/bin/sh
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$srcdir/images/rgb-3c-8b.tiff"
outfile="o-tiffcrop-extract-rgb-3c-8b.tiff"
f_test_convert "$TIFFCROP -U px -E top -X 60 -Y 60" $infile $outfile
f_tiffinfo_validate $outfile
