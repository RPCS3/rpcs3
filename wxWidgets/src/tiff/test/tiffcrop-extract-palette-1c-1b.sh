#!/bin/sh
# Generated file, master is Makefile.am
. ${srcdir:-.}/common.sh
infile="$srcdir/images/palette-1c-1b.tiff"
outfile="o-tiffcrop-extract-palette-1c-1b.tiff"
f_test_convert "$TIFFCROP -U px -E top -X 60 -Y 60" $infile $outfile
f_tiffinfo_validate $outfile
