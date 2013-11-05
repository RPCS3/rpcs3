
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
#include <ctype.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#include "rasterfile.h"
#include "tiffio.h"

#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#define	streq(a,b)	(strcmp(a,b) == 0)
#define	strneq(a,b,n)	(strncmp(a,b,n) == 0)

static	uint16 compression = (uint16) -1;
static	int jpegcolormode = JPEGCOLORMODE_RGB;
static	int quality = 75;		/* JPEG quality */
static	uint16 predictor = 0;

static void usage(void);
static	int processCompressOptions(char*);

int
main(int argc, char* argv[])
{
	unsigned char* buf;
	long row;
	tsize_t linebytes, scanline;
	TIFF *out;
	FILE *in;
	struct rasterfile h;
	uint16 photometric;
	uint16 config = PLANARCONFIG_CONTIG;
	uint32 rowsperstrip = (uint32) -1;
	int c;
	extern int optind;
	extern char* optarg;

	while ((c = getopt(argc, argv, "c:r:h")) != -1)
		switch (c) {
		case 'c':		/* compression scheme */
			if (!processCompressOptions(optarg))
				usage();
			break;
		case 'r':		/* rows/strip */
			rowsperstrip = atoi(optarg);
			break;
		case 'h':
			usage();
			/*NOTREACHED*/
		}
	if (argc - optind != 2)
		usage();
	in = fopen(argv[optind], "rb");
	if (in == NULL) {
		fprintf(stderr, "%s: Can not open.\n", argv[optind]);
		return (-1);
	}
	if (fread(&h, sizeof (h), 1, in) != 1) {
		fprintf(stderr, "%s: Can not read header.\n", argv[optind]);
		fclose(in);
		return (-2);
	}
	if (strcmp(h.ras_magic, RAS_MAGIC) == 0) {
#ifndef WORDS_BIGENDIAN
			TIFFSwabLong((uint32 *)&h.ras_width);
			TIFFSwabLong((uint32 *)&h.ras_height);
			TIFFSwabLong((uint32 *)&h.ras_depth);
			TIFFSwabLong((uint32 *)&h.ras_length);
			TIFFSwabLong((uint32 *)&h.ras_type);
			TIFFSwabLong((uint32 *)&h.ras_maptype);
			TIFFSwabLong((uint32 *)&h.ras_maplength);
#endif
	} else if (strcmp(h.ras_magic, RAS_MAGIC_INV) == 0) {
#ifdef WORDS_BIGENDIAN
			TIFFSwabLong((uint32 *)&h.ras_width);
			TIFFSwabLong((uint32 *)&h.ras_height);
			TIFFSwabLong((uint32 *)&h.ras_depth);
			TIFFSwabLong((uint32 *)&h.ras_length);
			TIFFSwabLong((uint32 *)&h.ras_type);
			TIFFSwabLong((uint32 *)&h.ras_maptype);
			TIFFSwabLong((uint32 *)&h.ras_maplength);
#endif
	} else {
		fprintf(stderr, "%s: Not a rasterfile.\n", argv[optind]);
		fclose(in);
		return (-3);
	}
	out = TIFFOpen(argv[optind+1], "w");
	if (out == NULL)
	{
		fclose(in);
		return (-4);
	}
	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, (uint32) h.ras_width);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, (uint32) h.ras_height);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, h.ras_depth > 8 ? 3 : 1);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, h.ras_depth > 1 ? 8 : 1);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, config);
	if (h.ras_maptype != RMT_NONE) {
		uint16* red;
		register uint16* map;
		register int i, j;
		int mapsize;

		buf = (unsigned char *)_TIFFmalloc(h.ras_maplength);
		if (buf == NULL) {
			fprintf(stderr, "No space to read in colormap.\n");
			return (-5);
		}
		if (fread(buf, h.ras_maplength, 1, in) != 1) {
			fprintf(stderr, "%s: Read error on colormap.\n",
			    argv[optind]);
			return (-6);
		}
		mapsize = 1<<h.ras_depth; 
		if (h.ras_maplength > mapsize*3) {
			fprintf(stderr,
			    "%s: Huh, %ld colormap entries, should be %d?\n",
			    argv[optind], h.ras_maplength, mapsize*3);
			return (-7);
		}
		red = (uint16*)_TIFFmalloc(mapsize * 3 * sizeof (uint16));
		if (red == NULL) {
			fprintf(stderr, "No space for colormap.\n");
			return (-8);
		}
		map = red;
		for (j = 0; j < 3; j++) {
#define	SCALE(x)	(((x)*((1L<<16)-1))/255)
			for (i = h.ras_maplength/3; i-- > 0;)
				*map++ = SCALE(*buf++);
			if ((i = h.ras_maplength/3) < mapsize) {
				i = mapsize - i;
				_TIFFmemset(map, 0, i*sizeof (uint16));
				map += i;
			}
		}
		TIFFSetField(out, TIFFTAG_COLORMAP,
		     red, red + mapsize, red + 2*mapsize);
		photometric = PHOTOMETRIC_PALETTE;
		if (compression == (uint16) -1)
			compression = COMPRESSION_PACKBITS;
		TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
	} else {
		/* XXX this is bogus... */
		photometric = h.ras_depth == 24 ?
		    PHOTOMETRIC_RGB : PHOTOMETRIC_MINISBLACK;
		if (compression == (uint16) -1)
			compression = COMPRESSION_LZW;
		TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
	}
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
	linebytes = ((h.ras_depth*h.ras_width+15) >> 3) &~ 1;
	scanline = TIFFScanlineSize(out);
	if (scanline > linebytes) {
		buf = (unsigned char *)_TIFFmalloc(scanline);
		_TIFFmemset(buf+linebytes, 0, scanline-linebytes);
	} else
		buf = (unsigned char *)_TIFFmalloc(linebytes);
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
	    TIFFDefaultStripSize(out, rowsperstrip));
	for (row = 0; row < h.ras_height; row++) {
		if (fread(buf, linebytes, 1, in) != 1) {
			fprintf(stderr, "%s: scanline %ld: Read error.\n",
			    argv[optind], row);
			break;
		}
		if (h.ras_type == RT_STANDARD && h.ras_depth == 24) {
			tsize_t cc = h.ras_width;
			unsigned char* cp = buf;
#define	SWAP(a,b)	{ unsigned char t = (a); (a) = (b); (b) = t; }
			do {
				SWAP(cp[0], cp[2]);
				cp += 3;
			} while (--cc);
		}
		if (TIFFWriteScanline(out, buf, row, 0) < 0)
			break;
	}
	(void) TIFFClose(out);
	fclose(in);
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

char* stuff[] = {
"usage: ras2tiff [options] input.ras output.tif",
"where options are:",
" -r #		make each strip have no more than # rows",
"",
" -c lzw[:opts]	compress output with Lempel-Ziv & Welch encoding",
" -c zip[:opts]	compress output with deflate encoding",
" -c jpeg[:opts]	compress output with JPEG encoding",
" -c packbits	compress output with packbits encoding",
" -c none	use no compression algorithm on output",
"",
"JPEG options:",
" #		set compression quality level (0-100, default 75)",
" r		output color image as RGB rather than YCbCr",
"For example, -c jpeg:r:50 to get JPEG-encoded RGB data with 50% comp. quality",
"",
"LZW and deflate options:",
" #		set predictor value",
"For example, -c lzw:2 to get LZW-encoded data with horizontal differencing",
" -h		this help message",
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
