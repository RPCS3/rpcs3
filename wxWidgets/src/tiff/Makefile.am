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

docdir = $(LIBTIFF_DOCDIR)

AUTOMAKE_OPTIONS = 1.12 dist-zip foreign
ACLOCAL_AMFLAGS = -I m4

docfiles = \
	COPYRIGHT \
	ChangeLog \
	README \
	README.vms \
	RELEASE-DATE \
	TODO \
	VERSION

EXTRA_DIST = \
	HOWTO-RELEASE \
	Makefile.vc \
	SConstruct \
	autogen.sh \
	configure.com \
	nmake.opt \
	libtiff-4.pc.in

dist_doc_DATA = $(docfiles)

SUBDIRS = port libtiff tools build contrib test man html

release:
	(rm -f $(top_srcdir)/RELEASE-DATE && echo $(LIBTIFF_RELEASE_DATE) > $(top_srcdir)/RELEASE-DATE)
	(rm -f $(top_srcdir)/VERSION && echo $(LIBTIFF_VERSION) > $(top_srcdir)/VERSION)
	(rm -f $(top_srcdir)/libtiff/tiffvers.h && sed 's,LIBTIFF_VERSION,$(LIBTIFF_VERSION),;s,LIBTIFF_RELEASE_DATE,$(LIBTIFF_RELEASE_DATE),' $(top_srcdir)/libtiff/tiffvers.h.in > $(top_srcdir)/libtiff/tiffvers.h)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtiff-4.pc
