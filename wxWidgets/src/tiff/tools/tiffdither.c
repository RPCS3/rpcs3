
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
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#include "tiffio.h"

#define	streq(a,b)	(strcmp(a,b) == 0)
#define	strneq(a,b,n)	(strncmp(a,b,n) == 0)

#define	CopyField(tag, v) \
	if (TIFFGetField(in, tag, &v)) TIFFSetField(out, tag, v)

uint32	imagewidth;
uint32	imagelength;
int	threshold = 128;

static	void usage(void);

/* 
 * Floyd-Steinberg error propragation with threshold.
 * This code is stolen from tiffmedian.
 */
static void
fsdither(TIFF* in, TIFF* out)
{
	unsigned char *outline, *inputline, *inptr;
	short *thisline, *nextline, *tmpptr;
	register unsigned char	*outptr;
	register short *thisptr, *nextptr;
	register uint32 i, j;
	uint32 imax, jmax;
	int lastline, lastpixel;
	int bit;
	tsize_t outlinesize;

	imax = imagelength - 1;
	jmax = imagewidth - 1;
	inputline = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(in));
	thisline = (short *)_TIFFmalloc(imagewidth * sizeof (short));
	nextline = (short *)_TIFFmalloc(imagewidth * sizeof (short));
	outlinesize = TIFFScanlineSize(out);
	outline = (unsigned char *) _TIFFmalloc(outlinesize);

	/*
	 * Get first line
	 */
	if (TIFFReadScanline(in, inputline, 0, 0) <= 0)
            goto skip_on_error;

	inptr = inputline;
	nextptr = nextline;
	for (j = 0; j < imagewidth; ++j)
		*nextptr++ = *inptr++;
	for (i = 1; i < imagelength; ++i) {
		tmpptr = thisline;
		thisline = nextline;
		nextline = tmpptr;
		lastline = (i == imax);
		if (TIFFReadScanline(in, inputline, i, 0) <= 0)
			break;
		inptr = inputline;
		nextptr = nextline;
		for (j = 0; j < imagewidth; ++j)
			*nextptr++ = *inptr++;
		thisptr = thisline;
		nextptr = nextline;
		_TIFFmemset(outptr = outline, 0, outlinesize);
		bit = 0x80;
		for (j = 0; j < imagewidth; ++j) {
			register int v;

			lastpixel = (j == jmax);
			v = *thisptr++;
			if (v < 0)
				v = 0;
			else if (v > 255)
				v = 255;
			if (v > threshold) {
				*outptr |= bit;
				v -= 255;
			}
			bit >>= 1;
			if (bit == 0) {
				outptr++;
				bit = 0x80;
			}
			if (!lastpixel)
				thisptr[0] += v * 7 / 16;
			if (!lastline) {
				if (j != 0)
					nextptr[-1] += v * 3 / 16;
				*nextptr++ += v * 5 / 16;
				if (!lastpixel)
					nextptr[0] += v / 16;
			}
		}
		if (TIFFWriteScanline(out, outline, i-1, 0) < 0)
			break;
	}
  skip_on_error:
	_TIFFfree(inputline);
	_TIFFfree(thisline);
	_TIFFfree(nextline);
	_TIFFfree(outline);
}

static	uint16 compression = COMPRESSION_PACKBITS;
static	uint16 predictor = 0;
static	uint32 group3options = 0;

static void
processG3Options(char* cp)
{
	if ((cp = strchr(cp, ':'))) {
		do {
			cp++;
			if (strneq(cp, "1d", 2))
				group3options &= ~GROUP3OPT_2DENCODING;
			else if (strneq(cp, "2d", 2))
				group3options |= GROUP3OPT_2DENCODING;
			else if (strneq(cp, "fill", 4))
				group3options |= GROUP3OPT_FILLBITS;
			else
				usage();
		} while ((cp = strchr(cp, ':')));
	}
}

static int
processCompressOptions(char* opt)
{
	if (streq(opt, "none"))
		compression = COMPRESSION_NONE;
	else if (streq(opt, "packbits"))
		compression = COMPRESSION_PACKBITS;
	else if (strneq(opt, "g3", 2)) {
		processG3Options(opt);
		compression = COMPRESSION_CCITTFAX3;
	} else if (streq(opt, "g4"))
		compression = COMPRESSION_CCITTFAX4;
	else if (strneq(opt, "lzw", 3)) {
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

int
main(int argc, char* argv[])
{
	TIFF *in, *out;
	uint16 samplesperpixel, bitspersample = 1, shortv;
	float floatv;
	char thing[1024];
	uint32 rowsperstrip = (uint32) -1;
	uint16 fillorder = 0;
	int c;
	extern int optind;
	extern char *optarg;

	while ((c = getopt(argc, argv, "c:f:r:t:")) != -1)
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
		case 'r':		/* rows/strip */
			rowsperstrip = atoi(optarg);
			break;
		case 't':
			threshold = atoi(optarg);
			if (threshold < 0)
				threshold = 0;
			else if (threshold > 255)
				threshold = 255;
			break;
		case '?':
			usage();
			/*NOTREACHED*/
		}
	if (argc - optind < 2)
		usage();
	in = TIFFOpen(argv[optind], "r");
	if (in == NULL)
		return (-1);
	TIFFGetField(in, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
	if (samplesperpixel != 1) {
		fprintf(stderr, "%s: Not a b&w image.\n", argv[0]);
		return (-1);
	}
	TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	if (bitspersample != 8) {
		fprintf(stderr,
		    " %s: Sorry, only handle 8-bit samples.\n", argv[0]);
		return (-1);
	}
	out = TIFFOpen(argv[optind+1], "w");
	if (out == NULL)
		return (-1);
	CopyField(TIFFTAG_IMAGEWIDTH, imagewidth);
	TIFFGetField(in, TIFFTAG_IMAGELENGTH, &imagelength);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, imagelength-1);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 1);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
	if (fillorder)
		TIFFSetField(out, TIFFTAG_FILLORDER, fillorder);
	else
		CopyField(TIFFTAG_FILLORDER, shortv);
	sprintf(thing, "Dithered B&W version of %s", argv[optind]);
	TIFFSetField(out, TIFFTAG_IMAGEDESCRIPTION, thing);
	CopyField(TIFFTAG_PHOTOMETRIC, shortv);
	CopyField(TIFFTAG_ORIENTATION, shortv);
	CopyField(TIFFTAG_XRESOLUTION, floatv);
	CopyField(TIFFTAG_YRESOLUTION, floatv);
	CopyField(TIFFTAG_RESOLUTIONUNIT, shortv);
        rowsperstrip = TIFFDefaultStripSize(out, rowsperstrip);
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
	switch (compression) {
	case COMPRESSION_CCITTFAX3:
		TIFFSetField(out, TIFFTAG_GROUP3OPTIONS, group3options);
		break;
	case COMPRESSION_LZW:
	case COMPRESSION_DEFLATE:
		if (predictor)
			TIFFSetField(out, TIFFTAG_PREDICTOR, predictor);
		break;
	}
	fsdither(in, out);
	TIFFClose(in);
	TIFFClose(out);
	return (0);
}

char* stuff[] = {
"usage: tiffdither [options] input.tif output.tif",
"where options are:",
" -r #		make each strip have no more than # rows",
" -f lsb2msb	force lsb-to-msb FillOrder for output",
" -f msb2lsb	force msb-to-lsb FillOrder for output",
" -c lzw[:opts]	compress output with Lempel-Ziv & Welch encoding",
" -c zip[:opts]	compress output with deflate encoding",
" -c packbits	compress output with packbits encoding",
" -c g3[:opts]	compress output with CCITT Group 3 encoding",
" -c g4		compress output with CCITT Group 4 encoding",
" -c none	use no compression algorithm on output",
"",
"Group 3 options:",
" 1d		use default CCITT Group 3 1D-encoding",
" 2d		use optional CCITT Group 3 2D-encoding",
" fill		byte-align EOL codes",
"For example, -c g3:2d:fill to get G3-2D-encoded data with byte-aligned EOLs",
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
