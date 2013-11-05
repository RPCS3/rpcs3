
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

#include "tif_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_IO_H
# include <io.h>
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#include "tiffio.h"

#ifndef HAVE_GETOPT
extern int getopt(int, char**, char*);
#endif

#define	streq(a,b)	(strcmp(a,b) == 0)
#define	strneq(a,b,n)	(strncmp(a,b,n) == 0)

static	uint16 compression = COMPRESSION_PACKBITS;
static	uint16 predictor = 0;
static	int quality = 75;	/* JPEG quality */
static	int jpegcolormode = JPEGCOLORMODE_RGB;
static  uint32 g3opts;

static	void usage(void);
static	int processCompressOptions(char*);

static void
BadPPM(char* file)
{
	fprintf(stderr, "%s: Not a PPM file.\n", file);
	exit(-2);
}

int
main(int argc, char* argv[])
{
	uint16 photometric = 0;
	uint32 rowsperstrip = (uint32) -1;
	double resolution = -1;
	unsigned char *buf = NULL;
	tsize_t linebytes = 0;
	uint16 spp = 1;
	uint16 bpp = 8;
	TIFF *out;
	FILE *in;
	unsigned int w, h, prec, row;
	char *infile;
	int c;
	extern int optind;
	extern char* optarg;

	if (argc < 2) {
	    fprintf(stderr, "%s: Too few arguments\n", argv[0]);
	    usage();
	}
	while ((c = getopt(argc, argv, "c:r:R:")) != -1)
		switch (c) {
		case 'c':		/* compression scheme */
			if (!processCompressOptions(optarg))
				usage();
			break;
		case 'r':		/* rows/strip */
			rowsperstrip = atoi(optarg);
			break;
		case 'R':		/* resolution */
			resolution = atof(optarg);
			break;
		case '?':
			usage();
			/*NOTREACHED*/
		}

	if (optind + 2 < argc) {
	    fprintf(stderr, "%s: Too many arguments\n", argv[0]);
	    usage();
	}

	/*
	 * If only one file is specified, read input from
	 * stdin; otherwise usage is: ppm2tiff input output.
	 */
	if (argc - optind > 1) {
		infile = argv[optind++];
		in = fopen(infile, "rb");
		if (in == NULL) {
			fprintf(stderr, "%s: Can not open.\n", infile);
			return (-1);
		}
	} else {
		infile = "<stdin>";
		in = stdin;
#if defined(HAVE_SETMODE) && defined(O_BINARY)
		setmode(fileno(stdin), O_BINARY);
#endif
	}

	if (fgetc(in) != 'P')
		BadPPM(infile);
	switch (fgetc(in)) {
		case '4':			/* it's a PBM file */
			bpp = 1;
			spp = 1;
			photometric = PHOTOMETRIC_MINISWHITE;
			break;
		case '5':			/* it's a PGM file */
			bpp = 8;
			spp = 1;
			photometric = PHOTOMETRIC_MINISBLACK;
			break;
		case '6':			/* it's a PPM file */
			bpp = 8;
			spp = 3;
			photometric = PHOTOMETRIC_RGB;
			if (compression == COMPRESSION_JPEG &&
			    jpegcolormode == JPEGCOLORMODE_RGB)
				photometric = PHOTOMETRIC_YCBCR;
			break;
		default:
			BadPPM(infile);
	}

	/* Parse header */
	while(1) {
		if (feof(in))
			BadPPM(infile);
		c = fgetc(in);
		/* Skip whitespaces (blanks, TABs, CRs, LFs) */
		if (strchr(" \t\r\n", c))
			continue;

		/* Check for comment line */
		if (c == '#') {
			do {
			    c = fgetc(in);
			} while(!(strchr("\r\n", c) || feof(in)));
			continue;
		}

		ungetc(c, in);
		break;
	}
	switch (bpp) {
	case 1:
		if (fscanf(in, " %u %u", &w, &h) != 2)
			BadPPM(infile);
		if (fgetc(in) != '\n')
			BadPPM(infile);
		break;
	case 8:
		if (fscanf(in, " %u %u %u", &w, &h, &prec) != 3)
			BadPPM(infile);
		if (fgetc(in) != '\n' || prec != 255)
			BadPPM(infile);
		break;
	}
	out = TIFFOpen(argv[optind], "w");
	if (out == NULL)
		return (-4);
	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, (uint32) w);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, (uint32) h);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bpp);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
	switch (compression) {
	case COMPRESSION_JPEG:
		TIFFSetField(out, TIFFTAG_JPEGQUALITY, quality);
		TIFFSetField(out, TIFFTAG_JPEGCOLORMODE, jpegcolormode);
		break;
	case COMPRESSION_LZW:
	case COMPRESSION_DEFLATE:
		if (predictor != 0)
			TIFFSetField(out, TIFFTAG_PREDICTOR, predictor);
		break;
        case COMPRESSION_CCITTFAX3:
		TIFFSetField(out, TIFFTAG_GROUP3OPTIONS, g3opts);
		break;
	}
	switch (bpp) {
		case 1:
			linebytes = (spp * w + (8 - 1)) / 8;
			if (rowsperstrip == (uint32) -1) {
				TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, h);
			} else {
				TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
				    TIFFDefaultStripSize(out, rowsperstrip));
			}
			break;
		case 8:
			linebytes = spp * w;
			TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
			    TIFFDefaultStripSize(out, rowsperstrip));
			break;
	}
	if (TIFFScanlineSize(out) > linebytes)
		buf = (unsigned char *)_TIFFmalloc(linebytes);
	else
		buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(out));
	if (resolution > 0) {
		TIFFSetField(out, TIFFTAG_XRESOLUTION, resolution);
		TIFFSetField(out, TIFFTAG_YRESOLUTION, resolution);
		TIFFSetField(out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	}
	for (row = 0; row < h; row++) {
		if (fread(buf, linebytes, 1, in) != 1) {
			fprintf(stderr, "%s: scanline %lu: Read error.\n",
			    infile, (unsigned long) row);
			break;
		}
		if (TIFFWriteScanline(out, buf, row, 0) < 0)
			break;
	}
	(void) TIFFClose(out);
	if (buf)
		_TIFFfree(buf);
	return (0);
}

static void
processG3Options(char* cp)
{
	g3opts = 0;
        if( (cp = strchr(cp, ':')) ) {
                do {
                        cp++;
                        if (strneq(cp, "1d", 2))
                                g3opts &= ~GROUP3OPT_2DENCODING;
                        else if (strneq(cp, "2d", 2))
                                g3opts |= GROUP3OPT_2DENCODING;
                        else if (strneq(cp, "fill", 4))
                                g3opts |= GROUP3OPT_FILLBITS;
                        else
                                usage();
                } while( (cp = strchr(cp, ':')) );
        }
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
                while (cp)
                {
                    if (isdigit((int)cp[1]))
			quality = atoi(cp+1);
                    else if (cp[1] == 'r' )
			jpegcolormode = JPEGCOLORMODE_RAW;
                    else
                        usage();

                    cp = strchr(cp+1,':');
                }
	} else if (strneq(opt, "g3", 2)) {
		processG3Options(opt);
		compression = COMPRESSION_CCITTFAX3;
	} else if (streq(opt, "g4")) {
		compression = COMPRESSION_CCITTFAX4;
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
"usage: ppm2tiff [options] input.ppm output.tif",
"where options are:",
" -r #		make each strip have no more than # rows",
" -R #		set x&y resolution (dpi)",
"",
" -c jpeg[:opts]  compress output with JPEG encoding",
" -c lzw[:opts]	compress output with Lempel-Ziv & Welch encoding",
" -c zip[:opts]	compress output with deflate encoding",
" -c packbits	compress output with packbits encoding (the default)",
" -c g3[:opts]  compress output with CCITT Group 3 encoding",
" -c g4         compress output with CCITT Group 4 encoding",
" -c none	use no compression algorithm on output",
"",
"JPEG options:",
" #		set compression quality level (0-100, default 75)",
" r		output color image as RGB rather than YCbCr",
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
