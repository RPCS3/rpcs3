# Tag Image File Format (TIFF) Software
#
# Copyright (C) 2004, Andrey Kiselev <dron@ak4719.spb.edu>
#
# Permission to use, copy, modify, distribute, and sell this software and 
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
# 
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
# 
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
# OF THIS SOFTWARE.

# Process this file with automake to produce Makefile.in.

SUBDIRS = xtiff

LIBTIFF = $(top_builddir)/libtiff/libtiff.la

EXTRA_DIST = README

noinst_PROGRAMS = tiff-bi tiff-grayscale tiff-palette tiff-rgb

tiff_bi_SOURCES = tiff-bi.c
tiff_bi_LDADD = $(LIBTIFF)
tiff_grayscale_SOURCES = tiff-grayscale.c
tiff_grayscale_LDADD = $(LIBTIFF)
tiff_palette_SOURCES = tiff-palette.c
tiff_palette_LDADD = $(LIBTIFF)
tiff_rgb_SOURCES = tiff-rgb.c
tiff_rgb_LDADD = $(LIBTIFF)

AM_CPPFLAGS = -I$(top_srcdir)/libtiff

