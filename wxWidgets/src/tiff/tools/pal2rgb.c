
/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

#include "tif_config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#include "tiffio.h"

#define	streq(a,b)	(strcmp(a,b) == 0)
#define	strneq(a,b,n)	(strncmp(a,b,n) == 0)

static	void usage(void);
static	void cpTags(TIFF* in, TIFF* out);

static int
checkcmap(int n, uint16* r, uint16* g, uint16* b)
{
	while (n-- > 0)
	    if (*r++ >= 256 || *g++ >= 256 || *b++ >= 256)
		return (16);
	fprintf(stderr, "Warning, assuming 8-bit colormap.\n");
	return (8);
}

#define	CopyField(tag, v) \
    if (TIFFGetField(in, tag, &v)) TIFFSetField(out, tag, v)
#define	CopyField3(tag, v1, v2, v3) \
    if (TIFFGetField(in, tag, &v1, &v2, &v3)) TIFFSetField(out, tag, v1, v2, v3)

static	uint16 compression = (uint16) -1;
static	uint16 predictor = 0;
static	int quality = 75;	/* JPEG quality */
static	int jpegcolormode = JPEGCOLORMODE_RGB;
static	int processCompressOptions(char*);

int
main(int argc, char* argv[])
{
	uint16 bitspersample, shortv;
	uint32 imagewidth, imagelength;
	uint16 config = PLANARCONFIG_CONTIG;
	uint32 rowsperstrip = (uint32) -1;
	uint16 photometric = PHOTOMETRIC_RGB;
	uint16 *rmap, *gmap, *bmap;
	uint32 row;
	int cmap = -1;
	TIFF *in, *out;
	int c;
	extern int optind;
	extern char* optarg;

	while ((c = getopt(argc, argv, "C:c:p:r:")) != -1)
		switch (c) {
		case 'C':		/* force colormap interpretation */
			cmap = atoi(optarg);
			break;
		case 'c':		/* compression scheme */
			if (!processCompressOptions(optarg))
				usage();
			break;
		case 'p':		/* planar configuration */
			if (streq(optarg, "separate"))
				config = PLANARCONFIG_SEPARATE;
			else if (streq(optarg, "contig"))
				config = PLANARCONFIG_CONTIG;
			else
				usage();
			break;
		case 'r':		/* rows/strip */
			rowsperstrip = atoi(optarg);
			break;
		case '?':
			usage();
			/*NOTREACHED*/
		}
	if (argc - optind != 2)
		usage();
	in = TIFFOpen(argv[optind], "r");
	if (in == NULL)
		return (-1);
	if (!TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &shortv) ||
	    shortv != PHOTOMETRIC_PALETTE) {
		fprintf(stderr, "%s: Expecting a palette image.\n",
		    argv[optind]);
		return (-1);
	}
	if (!TIFFGetField(in, TIFFTAG_COLORMAP, &rmap, &gmap, &bmap)) {
		fprintf(stderr,
		    "%s: No colormap (not a valid palette image).\n",
		    argv[optind]);
		return (-1);
	}
	bitspersample = 0;
	TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	if (bitspersample != 8) {
		fprintf(stderr, "%s: Sorry, can only handle 8-bit images.\n",
		    argv[optind]);
		return (-1);
	}
	out = TIFFOpen(argv[optind+1], "w");
	if (out == NULL)
		return (-2);
	cpTags(in, out);
	TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &imagewidth);
	TIFFGetField(in, TIFFTAG_IMAGELENGTH, &imagelength);
	if (compression != (uint16)-1)
		TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
	else
		TIFFGetField(in, TIFFTAG_COMPRESSION, &compression);
	switch (compression) {
	case COMPRESSION_JPEG:
		if (jpegcolormode == JPEGCOLORMODE_RGB)
			photometric = PHOTOMETRIC_YCBCR;
		else
			photometric = PHOTOMETRIC_RGB;
		TIFFSetField(out, TIFFTAG_JPEGQUALITY, quality);
		TIFFSetField(out, TIFFTAG_JPEGCOLORMODE, jpegcolormode);
		break;
	case COMPRESSION_LZW:
	case COMPRESSION_DEFLATE:
		if (predictor != 0)
			TIFFSetField(out, TIFFTAG_PREDICTOR, predictor);
		break;
	}
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 3);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, config);
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
	    rowsperstrip = TIFFDefaultStripSize(out, rowsperstrip));
	(void) TIFFGetField(in, TIFFTAG_PLANARCONFIG, &shortv);
	if (cmap == -1)
		cmap = checkcmap(1<<bitspersample, rmap, gmap, bmap);
	if (cmap == 16) {
		/*
		 * Convert 16-bit colormap to 8-bit.
		 */
		int i;

		for (i = (1<<bitspersample)-1; i >= 0; i--) {
#define	CVT(x)		(((x) * 255) / ((1L<<16)-1))
			rmap[i] = CVT(rmap[i]);
			gmap[i] = CVT(gmap[i]);
			bmap[i] = CVT(bmap[i]);
		}
	}
	{ unsigned char *ibuf, *obuf;
	  register unsigned char* pp;
	  register uint32 x;
	  ibuf = (unsigned char*)_TIFFmalloc(TIFFScanlineSize(in));
	  obuf = (unsigned char*)_TIFFmalloc(TIFFScanlineSize(out));
	  switch (config) {
	  case PLANARCONFIG_CONTIG:
		for (row = 0; row < imagelength; row++) {
			if (!TIFFReadScanline(in, ibuf, row, 0))
				goto done;
			pp = obuf;
			for (x = 0; x < imagewidth; x++) {
				*pp++ = (unsigned char) rmap[ibuf[x]];
				*pp++ = (unsigned char) gmap[ibuf[x]];
				*pp++ = (unsigned char) bmap[ibuf[x]];
			}
			if (!TIFFWriteScanline(out, obuf, row, 0))
				goto done;
		}
		break;
	  case PLANARCONFIG_SEPARATE:
		for (row = 0; row < imagelength; row++) {
			if (!TIFFReadScanline(in, ibuf, row, 0))
				goto done;
			for (pp = obuf, x = 0; x < imagewidth; x++)
				*pp++ = (unsigned char) rmap[ibuf[x]];
			if (!TIFFWriteScanline(out, obuf, row, 0))
				goto done;
			for (pp = obuf, x = 0; x < imagewidth; x++)
				*pp++ = (unsigned char) gmap[ibuf[x]];
			if (!TIFFWriteScanline(out, obuf, row, 0))
				goto done;
			for (pp = obuf, x = 0; x < imagewidth; x++)
				*pp++ = (unsigned char) bmap[ibuf[x]];
			if (!TIFFWriteScanline(out, obuf, row, 0))
				goto done;
		}
		break;
	  }
	  _TIFFfree(ibuf);
	  _TIFFfree(obuf);
	}
done:
	(void) TIFFClose(in);
	(void) TIFFClose(out);
	return (0);
}

static int
processCompressOptions(char* opt)
{
	if (streq(opt, "none"))
		compression = COMPRESSION_NONE;
	else if (streq(opt, "packbits"))
		compression = COMPRESSION_PACKBITS;
	else if (strneq(opt, "jpeg", 4)) {
		char* cp = strchr(opt, ':');

                compression = COMPRESSION_JPEG;
                while( cp )
                {
                    if (isdigit((int)cp[1]))
			quality = atoi(cp+1);
                    else if (cp[1] == 'r' )
			jpegcolormode = JPEGCOLORMODE_RAW;
                    else
                        usage();

                    cp = strchr(cp+1,':');
                }
	} else if (strneq(opt, "lzw", 3)) {
		char* cp = strchr(opt, ':');
		if (cp)
			predictor = atoi(cp+1);
		compression = COMPRESSION_LZW;
	} else if (strneq(opt, "zip", 3)) {
		char* cp = strchr(opt, ':');
		if (cp)
			predictor = atoi(cp+1);
		compression = COMPRESSION_DEFLATE;
	} else
		return (0);
	return (1);
}

#define	CopyField(tag, v) \
    if (TIFFGetField(in, tag, &v)) TIFFSetField(out, tag, v)
#define	CopyField2(tag, v1, v2) \
    if (TIFFGetField(in, tag, &v1, &v2)) TIFFSetField(out, tag, v1, v2)
#define	CopyField3(tag, v1, v2, v3) \
    if (TIFFGetField(in, tag, &v1, &v2, &v3)) TIFFSetField(out, tag, v1, v2, v3)
#define	CopyField4(tag, v1, v2, v3, v4) \
    if (TIFFGetField(in, tag, &v1, &v2, &v3, &v4)) TIFFSetField(out, tag, v1, v2, v3, v4)

static void
cpTag(TIFF* in, TIFF* out, uint16 tag, uint16 count, TIFFDataType type)
{
	switch (type) {
	case TIFF_SHORT:
		if (count == 1) {
			uint16 shortv;
			CopyField(tag, shortv);
		} else if (count == 2) {
			uint16 shortv1, shortv2;
			CopyField2(tag, shortv1, shortv2);
		} else if (count == 4) {
			uint16 *tr, *tg, *tb, *ta;
			CopyField4(tag, tr, tg, tb, ta);
		} else if (count == (uint16) -1) {
			uint16 shortv1;
			uint16* shortav;
			CopyField2(tag, shortv1, shortav);
		}
		break;
	case TIFF_LONG:
		{ uint32 longv;
		  CopyField(tag, longv);
		}
		break;
	case TIFF_RATIONAL:
		if (count == 1) {
			float floatv;
			CopyField(tag, floatv);
		} else if (count == (uint16) -1) {
			float* floatav;
			CopyField(tag, floatav);
		}
		break;
	case TIFF_ASCII:
		{ char* stringv;
		  CopyField(tag, stringv);
		}
		break;
	case TIFF_DOUBLE:
		if (count == 1) {
			double doublev;
			CopyField(tag, doublev);
		} else if (count == (uint16) -1) {
			double* doubleav;
			CopyField(tag, doubleav);
		}
		break;
          default:
                TIFFError(TIFFFileName(in),
                          "Data type %d is not supported, tag %d skipped.",
                          tag, type);
	}
}

#undef CopyField4
#undef CopyField3
#undef CopyField2
#undef CopyField

static struct cpTag {
    uint16	tag;
    uint16	count;
    TIFFDataType type;
} tags[] = {
    { TIFFTAG_IMAGEWIDTH,		1, TIFF_LONG },
    { TIFFTAG_IMAGELENGTH,		1, TIFF_LONG },
    { TIFFTAG_BITSPERSAMPLE,		1, TIFF_SHORT },
    { TIFFTAG_COMPRESSION,		1, TIFF_SHORT },
    { TIFFTAG_FILLORDER,		1, TIFF_SHORT },
    { TIFFTAG_ROWSPERSTRIP,		1, TIFF_LONG },
    { TIFFTAG_GROUP3OPTIONS,		1, TIFF_LONG },
    { TIFFTAG_SUBFILETYPE,		1, TIFF_LONG },
    { TIFFTAG_THRESHHOLDING,		1, TIFF_SHORT },
    { TIFFTAG_DOCUMENTNAME,		1, TIFF_ASCII },
    { TIFFTAG_IMAGEDESCRIPTION,		1, TIFF_ASCII },
    { TIFFTAG_MAKE,			1, TIFF_ASCII },
    { TIFFTAG_MODEL,			1, TIFF_ASCII },
    { TIFFTAG_ORIENTATION,		1, TIFF_SHORT },
    { TIFFTAG_MINSAMPLEVALUE,		1, TIFF_SHORT },
    { TIFFTAG_MAXSAMPLEVALUE,		1, TIFF_SHORT },
    { TIFFTAG_XRESOLUTION,		1, TIFF_RATIONAL },
    { TIFFTAG_YRESOLUTION,		1, TIFF_RATIONAL },
    { TIFFTAG_PAGENAME,			1, TIFF_ASCII },
    { TIFFTAG_XPOSITION,		1, TIFF_RATIONAL },
    { TIFFTAG_YPOSITION,		1, TIFF_RATIONAL },
    { TIFFTAG_GROUP4OPTIONS,		1, TIFF_LONG },
    { TIFFTAG_RESOLUTIONUNIT,		1, TIFF_SHORT },
    { TIFFTAG_PAGENUMBER,		2, TIFF_SHORT },
    { TIFFTAG_SOFTWARE,			1, TIFF_ASCII },
    { TIFFTAG_DATETIME,			1, TIFF_ASCII },
    { TIFFTAG_ARTIST,			1, TIFF_ASCII },
    { TIFFTAG_HOSTCOMPUTER,		1, TIFF_ASCII },
    { TIFFTAG_WHITEPOINT,		2, TIFF_RATIONAL },
    { TIFFTAG_PRIMARYCHROMATICITIES,	(uint16) -1,TIFF_RATIONAL },
    { TIFFTAG_HALFTONEHINTS,		2, TIFF_SHORT },
    { TIFFTAG_BADFAXLINES,		1, TIFF_LONG },
    { TIFFTAG_CLEANFAXDATA,		1, TIFF_SHORT },
    { TIFFTAG_CONSECUTIVEBADFAXLINES,	1, TIFF_LONG },
    { TIFFTAG_INKSET,			1, TIFF_SHORT },
    { TIFFTAG_INKNAMES,			1, TIFF_ASCII },
    { TIFFTAG_DOTRANGE,			2, TIFF_SHORT },
    { TIFFTAG_TARGETPRINTER,		1, TIFF_ASCII },
    { TIFFTAG_SAMPLEFORMAT,		1, TIFF_SHORT },
    { TIFFTAG_YCBCRCOEFFICIENTS,	(uint16) -1,TIFF_RATIONAL },
    { TIFFTAG_YCBCRSUBSAMPLING,		2, TIFF_SHORT },
    { TIFFTAG_YCBCRPOSITIONING,		1, TIFF_SHORT },
    { TIFFTAG_REFERENCEBLACKWHITE,	(uint16) -1,TIFF_RATIONAL },
};
#define	NTAGS	(sizeof (tags) / sizeof (tags[0]))

static void
cpTags(TIFF* in, TIFF* out)
{
    struct cpTag *p;
    for (p = tags; p < &tags[NTAGS]; p++)
	cpTag(in, out, p->tag, p->count, p->type);
}
#undef NTAGS

char* stuff[] = {
"usage: pal2rgb [options] input.tif output.tif",
"where options are:",
" -p contig	pack samples contiguously (e.g. RGBRGB...)",
" -p separate	store samples separately (e.g. RRR...GGG...BBB...)",
" -r #		make each strip have no more than # rows",
" -C 8		assume 8-bit colormap values (instead of 16-bit)",
" -C 16		assume 16-bit colormap values",
"",
" -c lzw[:opts]	compress output with Lempel-Ziv & Welch encoding",
" -c zip[:opts]	compress output with deflate encoding",
" -c packbits	compress output with packbits encoding",
" -c none	use no compression algorithm on output",
"",
"LZW and deflate options:",
" #		set predictor value",
"For example, -c lzw:2 to get LZW-encoded data with horizontal differencing",
NULL
};

static void
usage(void)
{
	char buf[BUFSIZ];
	int i;

	setbuf(stderr, buf);
        fprintf(stderr, "%s\n\n", TIFFGetVersion());
	for (i = 0; stuff[i] != NULL; i++)
		fprintf(stderr, "%s\n", stuff[i]);
	exit(-1);
}

/* vim: set ts=8 sts=8 sw=8 noet: */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
