 *
 * Project:  libtiff tools
 * Purpose:  Convert raw byte sequences in TIFF images
 * Author:   Andrey Kiselev, dron@ak4719.spb.edu
 *
 ******************************************************************************
 * Copyright (c) 2002, Andrey Kiselev <dron@ak4719.spb.edu>
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
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#if HAVE_IO_H
# include <io.h>
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#include "tiffio.h"

#ifndef HAVE_GETOPT
extern int getopt(int, char**, char*);
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

typedef enum {
	PIXEL,
	BAND
} InterleavingType;

static	uint16 compression = (uint16) -1;
static	int jpegcolormode = JPEGCOLORMODE_RGB;
static	int quality = 75;		/* JPEG quality */
static	uint16 predictor = 0;

static void swapBytesInScanline(void *, uint32, TIFFDataType);
static int guessSize(int, TIFFDataType, off_t, uint32, int,
		     uint32 *, uint32 *);
static double correlation(void *, void *, uint32, TIFFDataType);
static void usage(void);
static	int processCompressOptions(char*);

int
main(int argc, char* argv[])
{
	uint32	width = 0, length = 0, linebytes, bufsize;
	uint32	nbands = 1;		    /* number of bands in input image*/
	off_t	hdr_size = 0;		    /* size of the header to skip */
	TIFFDataType dtype = TIFF_BYTE;
	int16	depth = 1;		    /* bytes per pixel in input image */
	int	swab = 0;		    /* byte swapping flag */
	InterleavingType interleaving = 0;  /* interleaving type flag */
	uint32  rowsperstrip = (uint32) -1;
	uint16	photometric = PHOTOMETRIC_MINISBLACK;
	uint16	config = PLANARCONFIG_CONTIG;
	uint16	fillorder = FILLORDER_LSB2MSB;
	int	fd;
	char	*outfilename = NULL;
	TIFF	*out;

	uint32 row, col, band;
	int	c;
	unsigned char *buf = NULL, *buf1 = NULL;
	extern int optind;
	extern char* optarg;

	while ((c = getopt(argc, argv, "c:r:H:w:l:b:d:LMp:si:o:h")) != -1) {
		switch (c) {
		case 'c':		/* compression scheme */
			if (!processCompressOptions(optarg))
				usage();
			break;
		case 'r':		/* rows/strip */
			rowsperstrip = atoi(optarg);
			break;
		case 'H':		/* size of input image file header */
			hdr_size = atoi(optarg);
			break;
		case 'w':		/* input image width */
			width = atoi(optarg);
			break;
		case 'l':		/* input image length */
			length = atoi(optarg);
			break;
		case 'b':		/* number of bands in input image */
			nbands = atoi(optarg);
			break;
		case 'd':		/* type of samples in input image */
			if (strncmp(optarg, "byte", 4) == 0)
				dtype = TIFF_BYTE;
			else if (strncmp(optarg, "short", 5) == 0)
				dtype = TIFF_SHORT;
			else if  (strncmp(optarg, "long", 4) == 0)
				dtype = TIFF_LONG;
			else if  (strncmp(optarg, "sbyte", 5) == 0)
				dtype = TIFF_SBYTE;
			else if  (strncmp(optarg, "sshort", 6) == 0)
				dtype = TIFF_SSHORT;
			else if  (strncmp(optarg, "slong", 5) == 0)
				dtype = TIFF_SLONG;
			else if  (strncmp(optarg, "float", 5) == 0)
				dtype = TIFF_FLOAT;
			else if  (strncmp(optarg, "double", 6) == 0)
				dtype = TIFF_DOUBLE;
			else
				dtype = TIFF_BYTE;
			depth = TIFFDataWidth(dtype);
			break;
		case 'L':		/* input has lsb-to-msb fillorder */
			fillorder = FILLORDER_LSB2MSB;
			break;
		case 'M':		/* input has msb-to-lsb fillorder */
			fillorder = FILLORDER_MSB2LSB;
			break;
		case 'p':		/* photometric interpretation */
			if (strncmp(optarg, "miniswhite", 10) == 0)
				photometric = PHOTOMETRIC_MINISWHITE;
			else if (strncmp(optarg, "minisblack", 10) == 0)
				photometric = PHOTOMETRIC_MINISBLACK;
			else if (strncmp(optarg, "rgb", 3) == 0)
				photometric = PHOTOMETRIC_RGB;
			else if (strncmp(optarg, "cmyk", 4) == 0)
				photometric = PHOTOMETRIC_SEPARATED;
			else if (strncmp(optarg, "ycbcr", 5) == 0)
				photometric = PHOTOMETRIC_YCBCR;
			else if (strncmp(optarg, "cielab", 6) == 0)
				photometric = PHOTOMETRIC_CIELAB;
			else if (strncmp(optarg, "icclab", 6) == 0)
				photometric = PHOTOMETRIC_ICCLAB;
			else if (strncmp(optarg, "itulab", 6) == 0)
				photometric = PHOTOMETRIC_ITULAB;
			else
				photometric = PHOTOMETRIC_MINISBLACK;
			break;
		case 's':		/* do we need to swap bytes? */
			swab = 1;
			break;
		case 'i':		/* type of interleaving */
			if (strncmp(optarg, "pixel", 4) == 0)
				interleaving = PIXEL;
			else if  (strncmp(optarg, "band", 6) == 0)
				interleaving = BAND;
			else
				interleaving = 0;
			break;
		case 'o':
			outfilename = optarg;
			break;
		case 'h':
			usage();
		default:
			break;
		}
        }

        if (argc - optind < 2)
		usage();

        fd = open(argv[optind], O_RDONLY|O_BINARY, 0);
	if (fd < 0) {
		fprintf(stderr, "%s: %s: Cannot open input file.\n",
			argv[0], argv[optind]);
		return (-1);
	}

	if (guessSize(fd, dtype, hdr_size, nbands, swab, &width, &length) < 0)
		return 1;

	if (outfilename == NULL)
		outfilename = argv[optind+1];
	out = TIFFOpen(outfilename, "w");
	if (out == NULL) {
		fprintf(stderr, "%s: %s: Cannot open file for output.\n",
			argv[0], outfilename);
		return (-1);
	}
	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, length);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, nbands);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, depth * 8);
	TIFFSetField(out, TIFFTAG_FILLORDER, fillorder);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, config);
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
	switch (dtype) {
	case TIFF_BYTE:
	case TIFF_SHORT:
	case TIFF_LONG:
		TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
		break;
	case TIFF_SBYTE:
	case TIFF_SSHORT:
	case TIFF_SLONG:
		TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_INT);
		break;
	case TIFF_FLOAT:
	case TIFF_DOUBLE:
		TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
		break;
	default:
		TIFFSetField(out, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_VOID);
		break;
	}
	if (compression == (uint16) -1)
		compression = COMPRESSION_PACKBITS;
	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
	switch (compression) {
	case COMPRESSION_JPEG:
		if (photometric == PHOTOMETRIC_RGB
		    && jpegcolormode == JPEGCOLORMODE_RGB)
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
	switch(interleaving) {
	case BAND:				/* band interleaved data */
		linebytes = width * depth;
		buf = (unsigned char *)_TIFFmalloc(linebytes);
		break;
	case PIXEL:				/* pixel interleaved data */
	default:
		linebytes = width * nbands * depth;
		break;
	}
	bufsize = width * nbands * depth;
	buf1 = (unsigned char *)_TIFFmalloc(bufsize);

	rowsperstrip = TIFFDefaultStripSize(out, rowsperstrip);
	if (rowsperstrip > length) {
		rowsperstrip = length;
	}
	TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rowsperstrip );

	lseek(fd, hdr_size, SEEK_SET);		/* Skip the file header */
	for (row = 0; row < length; row++) {
		switch(interleaving) {
		case BAND:			/* band interleaved data */
			for (band = 0; band < nbands; band++) {
				lseek(fd,
				      hdr_size + (length*band+row)*linebytes,
				      SEEK_SET);
				if (read(fd, buf, linebytes) < 0) {
					fprintf(stderr,
					"%s: %s: scanline %lu: Read error.\n",
					argv[0], argv[optind],
					(unsigned long) row);
				break;
				}
				if (swab)	/* Swap bytes if needed */
					swapBytesInScanline(buf, width, dtype);
				for (col = 0; col < width; col++)
					memcpy(buf1 + (col*nbands+band)*depth,
					       buf + col * depth, depth);
			}
			break;
		case PIXEL:			/* pixel interleaved data */
		default:
			if (read(fd, buf1, bufsize) < 0) {
				fprintf(stderr,
					"%s: %s: scanline %lu: Read error.\n",
					argv[0], argv[optind],
					(unsigned long) row);
				break;
			}
			if (swab)		/* Swap bytes if needed */
				swapBytesInScanline(buf1, width, dtype);
			break;
		}
				
		if (TIFFWriteScanline(out, buf1, row, 0) < 0) {
			fprintf(stderr,	"%s: %s: scanline %lu: Write error.\n",
				argv[0], outfilename, (unsigned long) row);
			break;
		}
	}
	if (buf)
		_TIFFfree(buf);
	if (buf1)
		_TIFFfree(buf1);
	TIFFClose(out);
	return (0);
}

static void
swapBytesInScanline(void *buf, uint32 width, TIFFDataType dtype)
{
	switch (dtype) {
		case TIFF_SHORT:
		case TIFF_SSHORT:
			TIFFSwabArrayOfShort((uint16*)buf,
                                             (unsigned long)width);
			break;
		case TIFF_LONG:
		case TIFF_SLONG:
			TIFFSwabArrayOfLong((uint32*)buf,
                                            (unsigned long)width);
			break;
		/* case TIFF_FLOAT: */	/* FIXME */
		case TIFF_DOUBLE:
			TIFFSwabArrayOfDouble((double*)buf,
                                              (unsigned long)width);
			break;
		default:
			break;
	}
}

static int
guessSize(int fd, TIFFDataType dtype, off_t hdr_size, uint32 nbands,
	  int swab, uint32 *width, uint32 *length)
{
	const float longt = 40.0;    /* maximum possible height/width ratio */
	char	    *buf1, *buf2;
	struct stat filestat;
	uint32	    w, h, scanlinesize, imagesize;
	uint32	    depth = TIFFDataWidth(dtype);
	float	    cor_coef = 0, tmp;

	fstat(fd, &filestat);

	if (filestat.st_size < hdr_size) {
		fprintf(stderr, "Too large header size specified.\n");
		return -1;
	}

	imagesize = (filestat.st_size - hdr_size) / nbands / depth;

	if (*width != 0 && *length == 0) {
		fprintf(stderr,	"Image height is not specified.\n");

		*length = imagesize / *width;
		
		fprintf(stderr, "Height is guessed as %lu.\n",
			(unsigned long)*length);

		return 1;
	} else if (*width == 0 && *length != 0) {
		fprintf(stderr, "Image width is not specified.\n");

		*width = imagesize / *length;
		
		fprintf(stderr,	"Width is guessed as %lu.\n",
			(unsigned long)*width);

		return 1;
	} else if (*width == 0 && *length == 0) {
		fprintf(stderr,	"Image width and height are not specified.\n");

		for (w = (uint32) sqrt(imagesize / longt);
		     w < sqrt(imagesize * longt);
		     w++) {
			if (imagesize % w == 0) {
				scanlinesize = w * depth;
				buf1 = _TIFFmalloc(scanlinesize);
				buf2 = _TIFFmalloc(scanlinesize);
				h = imagesize / w;
				lseek(fd, hdr_size + (int)(h/2)*scanlinesize,
				      SEEK_SET);
				read(fd, buf1, scanlinesize);
				read(fd, buf2, scanlinesize);
				if (swab) {
					swapBytesInScanline(buf1, w, dtype);
					swapBytesInScanline(buf2, w, dtype);
				}
				tmp = (float) fabs(correlation(buf1, buf2,
							       w, dtype));
				if (tmp > cor_coef) {
					cor_coef = tmp;
					*width = w, *length = h;
				}

				_TIFFfree(buf1);
				_TIFFfree(buf2);
			}
		}

		fprintf(stderr,
			"Width is guessed as %lu, height is guessed as %lu.\n",
			(unsigned long)*width, (unsigned long)*length);

		return 1;
	} else {
		if (filestat.st_size<(off_t)(hdr_size+(*width)*(*length)*nbands*depth)) {
			fprintf(stderr, "Input file too small.\n");
		return -1;
		}
	}

	return 1;
}

/* Calculate correlation coefficient between two numeric vectors */
static double
correlation(void *buf1, void *buf2, uint32 n_elem, TIFFDataType dtype)
{
	double	X, Y, M1 = 0.0, M2 = 0.0, D1 = 0.0, D2 = 0.0, K = 0.0;
	uint32	i;

	switch (dtype) {
		case TIFF_BYTE:
		default:
                        for (i = 0; i < n_elem; i++) {
				X = ((unsigned char *)buf1)[i];
				Y = ((unsigned char *)buf2)[i];
				M1 += X, M2 += Y;
				D1 += X * X, D2 += Y * Y;
				K += X * Y;
                        }
			break;
		case TIFF_SBYTE:
                        for (i = 0; i < n_elem; i++) {
				X = ((signed char *)buf1)[i];
				Y = ((signed char *)buf2)[i];
				M1 += X, M2 += Y;
				D1 += X * X, D2 += Y * Y;
				K += X * Y;
                        }
			break;
		case TIFF_SHORT:
                        for (i = 0; i < n_elem; i++) {
				X = ((uint16 *)buf1)[i];
				Y = ((uint16 *)buf2)[i];
				M1 += X, M2 += Y;
				D1 += X * X, D2 += Y * Y;
				K += X * Y;
                        }
			break;
		case TIFF_SSHORT:
                        for (i = 0; i < n_elem; i++) {
				X = ((int16 *)buf1)[i];
				Y = ((int16 *)buf2)[i];
				M1 += X, M2 += Y;
				D1 += X * X, D2 += Y * Y;
				K += X * Y;
                        }
			break;
		case TIFF_LONG:
                        for (i = 0; i < n_elem; i++) {
				X = ((uint32 *)buf1)[i];
				Y = ((uint32 *)buf2)[i];
				M1 += X, M2 += Y;
				D1 += X * X, D2 += Y * Y;
				K += X * Y;
                        }
			break;
		case TIFF_SLONG:
                        for (i = 0; i < n_elem; i++) {
				X = ((int32 *)buf1)[i];
				Y = ((int32 *)buf2)[i];
				M1 += X, M2 += Y;
				D1 += X * X, D2 += Y * Y;
				K += X * Y;
                        }
			break;
		case TIFF_FLOAT:
                        for (i = 0; i < n_elem; i++) {
				X = ((float *)buf1)[i];
				Y = ((float *)buf2)[i];
				M1 += X, M2 += Y;
				D1 += X * X, D2 += Y * Y;
				K += X * Y;
                        }
			break;
		case TIFF_DOUBLE:
                        for (i = 0; i < n_elem; i++) {
				X = ((double *)buf1)[i];
				Y = ((double *)buf2)[i];
				M1 += X, M2 += Y;
				D1 += X * X, D2 += Y * Y;
				K += X * Y;
                        }
			break;
	}

	M1 /= n_elem;
	M2 /= n_elem;
	D1 -= M1 * M1 * n_elem;
	D2 -= M2 * M2 * n_elem;
	K = (K - M1 * M2 * n_elem) / sqrt(D1 * D2);

	return K;
}

static int
processCompressOptions(char* opt)
{
	if (strcmp(opt, "none") == 0)
		compression = COMPRESSION_NONE;
	else if (strcmp(opt, "packbits") == 0)
		compression = COMPRESSION_PACKBITS;
	else if (strncmp(opt, "jpeg", 4) == 0) {
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
	} else if (strncmp(opt, "lzw", 3) == 0) {
		char* cp = strchr(opt, ':');
		if (cp)
			predictor = atoi(cp+1);
		compression = COMPRESSION_LZW;
	} else if (strncmp(opt, "zip", 3) == 0) {
		char* cp = strchr(opt, ':');
		if (cp)
			predictor = atoi(cp+1);
		compression = COMPRESSION_DEFLATE;
	} else
		return (0);
	return (1);
}

static char* stuff[] = {
"raw2tiff --- tool for converting raw byte sequences in TIFF images",
"usage: raw2tiff [options] input.raw output.tif",
"where options are:",
" -L		input data has LSB2MSB bit order (default)",
" -M		input data has MSB2LSB bit order",
" -r #		make each strip have no more than # rows",
" -H #		size of input image file header in bytes (0 by default)",
" -w #		width of input image in pixels",
" -l #		length of input image in lines",
" -b #		number of bands in input image (1 by default)",
"",
" -d data_type	type of samples in input image",
"where data_type may be:",
" byte		8-bit unsigned integer (default)",
" short		16-bit unsigned integer",
" long		32-bit unsigned integer",
" sbyte		8-bit signed integer",
" sshort		16-bit signed integer",
" slong		32-bit signed integer",
" float		32-bit IEEE floating point",
" double		64-bit IEEE floating point",
"",
" -p photo	photometric interpretation (color space) of the input image",
"where photo may be:",
" miniswhite	white color represented with 0 value",
" minisblack	black color represented with 0 value (default)",
" rgb		image has RGB color model",
" cmyk		image has CMYK (separated) color model",
" ycbcr		image has YCbCr color model",
" cielab		image has CIE L*a*b color model",
" icclab		image has ICC L*a*b color model",
" itulab		image has ITU L*a*b color model",
"",
" -s		swap bytes fetched from input file",
"",
" -i config	type of samples interleaving in input image",
"where config may be:",
" pixel		pixel interleaved data (default)",
" band		band interleaved data",
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
" -o out.tif	write output to out.tif",
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
