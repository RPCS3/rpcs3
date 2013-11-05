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

docdir = $(LIBTIFF_DOCDIR)/html/man
MANSRCDIR = $(top_srcdir)/man
HTMLMANDIR = $(top_srcdir)/html/man

GROFF = groff -Thtml -mandoc
ECHO = echo

indexfile = index.html
docfiles = \
	libtiff.3tiff.html \
	TIFFbuffer.3tiff.html \
	TIFFClose.3tiff.html \
	TIFFcodec.3tiff.html \
	TIFFcolor.3tiff.html \
	TIFFDataWidth.3tiff.html \
	TIFFError.3tiff.html \
	TIFFFieldDataType.3tiff.html \
	TIFFFieldName.3tiff.html \
	TIFFFieldPassCount.3tiff.html \
	TIFFFieldReadCount.3tiff.html \
	TIFFFieldTag.3tiff.html \
	TIFFFieldWriteCount.3tiff.html \
	TIFFFlush.3tiff.html \
	TIFFGetField.3tiff.html \
	TIFFmemory.3tiff.html \
	TIFFOpen.3tiff.html \
	TIFFPrintDirectory.3tiff.html \
	TIFFquery.3tiff.html \
	TIFFReadDirectory.3tiff.html \
	TIFFReadEncodedStrip.3tiff.html \
	TIFFReadEncodedTile.3tiff.html \
	TIFFReadRawStrip.3tiff.html \
	TIFFReadRawTile.3tiff.html \
	TIFFReadRGBAImage.3tiff.html \
	TIFFReadRGBAStrip.3tiff.html \
	TIFFReadRGBATile.3tiff.html \
	TIFFReadScanline.3tiff.html \
	TIFFReadTile.3tiff.html \
	TIFFRGBAImage.3tiff.html \
	TIFFSetDirectory.3tiff.html \
	TIFFSetField.3tiff.html \
	TIFFsize.3tiff.html \
	TIFFstrip.3tiff.html \
	TIFFswab.3tiff.html \
	TIFFtile.3tiff.html \
	TIFFWarning.3tiff.html \
	TIFFWriteDirectory.3tiff.html \
	TIFFWriteEncodedStrip.3tiff.html \
	TIFFWriteEncodedTile.3tiff.html \
	TIFFWriteRawStrip.3tiff.html \
	TIFFWriteRawTile.3tiff.html \
	TIFFWriteScanline.3tiff.html \
	TIFFWriteTile.3tiff.html \
	bmp2tiff.1.html \
	fax2ps.1.html \
	fax2tiff.1.html \
	gif2tiff.1.html \
	pal2rgb.1.html \
	ppm2tiff.1.html \
	ras2tiff.1.html \
	raw2tiff.1.html \
	rgb2ycbcr.1.html \
	sgi2tiff.1.html \
	thumbnail.1.html \
	tiff2bw.1.html \
	tiff2pdf.1.html \
	tiff2ps.1.html \
	tiff2rgba.1.html \
	tiffcmp.1.html \
	tiffcp.1.html \
	tiffcrop.1.html \
	tiffdither.1.html \
	tiffdump.1.html \
	tiffgt.1.html \
	tiffinfo.1.html \
	tiffmedian.1.html \
	tiffset.1.html \
	tiffsplit.1.html \
	tiffsv.1.html

dist_doc_DATA = $(indexfile) $(docfiles)

INDEXSTART = '<HTML><HEAD><TITLE>Libtiff HTML manpage index</TITLE></HEAD><BODY BGCOLOR=white><ul><H2>Man Pages</h2><p>'
INDEXEND = '</ul></BODY></HTML>'

.PHONY: index
index:
	${ECHO} ${INDEXSTART} > $(indexfile)
	for i in $(docfiles); do					\
		${ECHO} '<li><A HREF='$$i'>'$$i'</a>' >> $(indexfile);  \
	done
	${ECHO} ${INDEXEND} >> $(indexfile)

manpages = $(docfiles:.html=)

.PHONY: htmldoc
htmldoc:
	for i in $(manpages); do					\
		${GROFF} $(MANSRCDIR)/$$i > $(HTMLMANDIR)/$$i.html;	\
	done

