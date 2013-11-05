
/*
 * Copyright (c) 1991-1997 Sam Leffler
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gl/image.h>
#include <ctype.h>

#include "tiffio.h"

#define	streq(a,b)	(strcmp(a,b) == 0)
#define	strneq(a,b,n)	(strncmp(a,b,n) == 0)

static	short config = PLANARCONFIG_CONTIG;
static	uint16 compression = COMPRESSION_PACKBITS;
static	uint16 predictor = 0;
static	uint16 fillorder = 0;
static	uint32 rowsperstrip = (uint32) -1;
static	int jpegcolormode = JPEGCOLORMODE_RGB;
static	int quality = 75;		/* JPEG quality */
static	uint16 photometric;

static	void usage(void);
static	int cpContig(IMAGE*, TIFF*);
static	int cpSeparate(IMAGE*, TIFF*);
static	int processCompressOptions(char*);

/* XXX image library has no prototypes */
extern	IMAGE* iopen(const char*, const char*);
extern	void iclose(IMAGE*);
extern	void getrow(IMAGE*, short*, int, int);

int
main(int argc, char* argv[])
{
	IMAGE *in;
	TIFF *out;
	int c;
	extern int optind;
	extern char* optarg;

	while ((c = getopt(argc, argv, "c:p:r:")) != -1)
		switch (c) {
		case 'c':		/* compression scheme */
			if (!processCompressOptions(optarg))
				usage();
			break;
		case 'f':		/* fill order */
			if (streq(optarg, "lsb2msb"))
				fillorder = FILLORDER_LSB2MSB;
			else if (streq(optarg, "msb2lsb"))
				fillorder = FILLORDER_MSB2LSB;
			else
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
	in = iopen(argv[optind], "r");
	if (in == NULL)
		return (-1);
	out = TIFFOpen(argv[optind+1], "w");
	if (out == NULL)
		return (-2);
	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, (uint32) in->xsize);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, (uint32) in->ysize);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
	if (in->zsize == 1)
		photometric = PHOTOMETRIC_MINISBLACK;
	else
		photometric = PHOTOMETRIC_RGB;
	switch (compression) {
	case COMPRESSION_JPEG:
		if (photometric == PHOTOMETRIC_RGB && jpegcolormode == JPEGCOLORMODE_RGB)
			photometric = PHOTOMETRIC_YCBCR;
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
	if (fillorder != 0)
		TIFFSetField(out, TIFFTAG_FILLORDER, fillorder);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, in->zsize);
	if (in->zsize > 3) {
	    uint16 v[1];
	    v[0] = EXTRASAMPLE_UNASSALPHA;
	    TIFFSetField(out, TIFFTAG_EXTRASAMPLES, 1, v);
	}
	TIFFSetField(out, TIFFTAG_MINSAMPLEVALUE, (uint16) in->min);
	TIFFSetField(out, TIFFTAG_MAXSAMPLEVALUE, (uint16) in->max);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, config);
	if (config != PLANARCONFIG_SEPARATE)
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
		    TIFFDefaultStripSize(out, rowsperstrip));
	else			/* force 1 row/strip for library limitation */
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, 1L);
	if (in->name[0] != '\0')
		TIFFSetField(out, TIFFTAG_IMAGEDESCRIPTION, in->name);
	if (config == PLANARCONFIG_CONTIG)
		cpContig(in, out);
	else
		cpSeparate(in, out);
	(void) iclose(in);
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

                defcompression = COMPRESSION_JPEG;
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

static int
cpContig(IMAGE* in, TIFF* out)
{
	tdata_t buf = _TIFFmalloc(TIFFScanlineSize(out));
	short *r = NULL;
	int x, y;

	if (in->zsize == 3) {
		short *g, *b;

		r = (short *)_TIFFmalloc(3 * in->xsize * sizeof (short));
		g = r + in->xsize;
		b = g + in->xsize;
		for (y = in->ysize-1; y >= 0; y--) {
			uint8* pp = (uint8*) buf;

			getrow(in, r, y, 0);
			getrow(in, g, y, 1);
			getrow(in, b, y, 2);
			for (x = 0; x < in->xsize; x++) {
				pp[0] = r[x];
				pp[1] = g[x];
				pp[2] = b[x];
				pp += 3;
			}
			if (TIFFWriteScanline(out, buf, in->ysize-y-1, 0) < 0)
				goto bad;
		}
	} else if (in->zsize == 4) {
		short *g, *b, *a;

		r = (short *)_TIFFmalloc(4 * in->xsize * sizeof (short));
		g = r + in->xsize;
		b = g + in->xsize;
		a = b + in->xsize;
		for (y = in->ysize-1; y >= 0; y--) {
			uint8* pp = (uint8*) buf;

			getrow(in, r, y, 0);
			getrow(in, g, y, 1);
			getrow(in, b, y, 2);
			getrow(in, a, y, 3);
			for (x = 0; x < in->xsize; x++) {
				pp[0] = r[x];
				pp[1] = g[x];
				pp[2] = b[x];
				pp[3] = a[x];
				pp += 4;
			}
			if (TIFFWriteScanline(out, buf, in->ysize-y-1, 0) < 0)
				goto bad;
		}
	} else {
		uint8* pp = (uint8*) buf;

		r = (short *)_TIFFmalloc(in->xsize * sizeof (short));
		for (y = in->ysize-1; y >= 0; y--) {
			getrow(in, r, y, 0);
			for (x = in->xsize-1; x >= 0; x--)
				pp[x] = r[x];
			if (TIFFWriteScanline(out, buf, in->ysize-y-1, 0) < 0)
				goto bad;
		}
	}
	if (r)
		_TIFFfree(r);
	_TIFFfree(buf);
	return (1);
bad:
	if (r)
		_TIFFfree(r);
	_TIFFfree(buf);
	return (0);
}

static int
cpSeparate(IMAGE* in, TIFF* out)
{
	tdata_t buf = _TIFFmalloc(TIFFScanlineSize(out));
	short *r = (short *)_TIFFmalloc(in->xsize * sizeof (short));
	uint8* pp = (uint8*) buf;
	int x, y, z;

	for (z = 0; z < in->zsize; z++) {
		for (y = in->ysize-1; y >= 0; y--) {
			getrow(in, r, y, z);
			for (x = 0; x < in->xsize; x++)
				pp[x] = r[x];
			if (TIFFWriteScanline(out, buf, in->ysize-y-1, z) < 0)
				goto bad;
		}
	}
	_TIFFfree(r);
	_TIFFfree(buf);
	return (1);
bad:
	_TIFFfree(r);
	_TIFFfree(buf);
	return (0);
}

char* stuff[] = {
"usage: sgi2tiff [options] input.rgb output.tif",
"where options are:",
" -r #		make each strip have no more than # rows",
"",
" -p contig	pack samples contiguously (e.g. RGBRGB...)",
" -p separate	store samples separately (e.g. RRR...GGG...BBB...)",
"",
" -f lsb2msb	force lsb-to-msb FillOrder for output",
" -f msb2lsb	force msb-to-lsb FillOrder for output",
"",
" -c lzw[:opts]	compress output with Lempel-Ziv & Welch encoding",
" -c zip[:opts]	compress output with deflate encoding",
" -c jpeg[:opts]compress output with JPEG encoding",
" -c packbits	compress output with packbits encoding",
" -c none	use no compression algorithm on output",
"",
"JPEG options:",
" #		set compression quality level (0-100, default 75)",
" r		output color image as RGB rather than YCbCr",
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
	for (i = 0; stuff[i] != NULL; i++)
		fprintf(stderr, "%s\n", stuff[i]);
	exit(-1);
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
