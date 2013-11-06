 *
 * Project:  libtiff tools
 * Purpose:  Convert Windows BMP files in TIFF.
 * Author:   Andrey Kiselev, dron@ak4719.spb.edu
 *
 ******************************************************************************
 * Copyright (c) 2004, Andrey Kiselev <dron@ak4719.spb.edu>
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
#include <sys/types.h>
#include <sys/stat.h>

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

#ifndef O_BINARY
# define O_BINARY 0
#endif

enum BMPType
{
    BMPT_WIN4,      /* BMP used in Windows 3.0/NT 3.51/95 */
    BMPT_WIN5,      /* BMP used in Windows NT 4.0/98/Me/2000/XP */
    BMPT_OS21,      /* BMP used in OS/2 PM 1.x */
    BMPT_OS22       /* BMP used in OS/2 PM 2.x */
};

/*
 * Bitmap file consists of a BMPFileHeader structure followed by a
 * BMPInfoHeader structure. An array of BMPColorEntry structures (also called
 * a colour table) follows the bitmap information header structure. The colour
 * table is followed by a second array of indexes into the colour table (the
 * actual bitmap data). Data may be comressed, for 4-bpp and 8-bpp used RLE
 * compression.
 *
 * +---------------------+
 * | BMPFileHeader       |
 * +---------------------+
 * | BMPInfoHeader       |
 * +---------------------+
 * | BMPColorEntry array |
 * +---------------------+
 * | Colour-index array  |
 * +---------------------+
 *
 * All numbers stored in Intel order with least significant byte first.
 */

enum BMPComprMethod
{
    BMPC_RGB = 0L,          /* Uncompressed */
    BMPC_RLE8 = 1L,         /* RLE for 8 bpp images */
    BMPC_RLE4 = 2L,         /* RLE for 4 bpp images */
    BMPC_BITFIELDS = 3L,    /* Bitmap is not compressed and the colour table
			     * consists of three DWORD color masks that specify
			     * the red, green, and blue components of each
			     * pixel. This is valid when used with
			     * 16- and 32-bpp bitmaps. */
    BMPC_JPEG = 4L,         /* Indicates that the image is a JPEG image. */
    BMPC_PNG = 5L           /* Indicates that the image is a PNG image. */
};

enum BMPLCSType                 /* Type of logical color space. */
{
    BMPLT_CALIBRATED_RGB = 0,	/* This value indicates that endpoints and
				 * gamma values are given in the appropriate
				 * fields. */
    BMPLT_DEVICE_RGB = 1,
    BMPLT_DEVICE_CMYK = 2
};

typedef struct
{
    int32   iCIEX;
    int32   iCIEY;
    int32   iCIEZ;
} BMPCIEXYZ;

typedef struct                  /* This structure contains the x, y, and z */
{				/* coordinates of the three colors that */
				/* correspond */
    BMPCIEXYZ   iCIERed;        /* to the red, green, and blue endpoints for */
    BMPCIEXYZ   iCIEGreen;      /* a specified logical color space. */
    BMPCIEXYZ	iCIEBlue;
} BMPCIEXYZTriple;

typedef struct
{
    char	bType[2];       /* Signature "BM" */
    uint32	iSize;          /* Size in bytes of the bitmap file. Should
				 * always be ignored while reading because
				 * of error in Windows 3.0 SDK's description
				 * of this field */
    uint16	iReserved1;     /* Reserved, set as 0 */
    uint16	iReserved2;     /* Reserved, set as 0 */
    uint32	iOffBits;       /* Offset of the image from file start in bytes */
} BMPFileHeader;

/* File header size in bytes: */
const int       BFH_SIZE = 14;

typedef struct
{
    uint32	iSize;          /* Size of BMPInfoHeader structure in bytes.
				 * Should be used to determine start of the
				 * colour table */
    int32	iWidth;         /* Image width */
    int32	iHeight;        /* Image height. If positive, image has bottom
				 * left origin, if negative --- top left. */
    int16	iPlanes;        /* Number of image planes (must be set to 1) */
    int16	iBitCount;      /* Number of bits per pixel (1, 4, 8, 16, 24
				 * or 32). If 0 then the number of bits per
				 * pixel is specified or is implied by the
				 * JPEG or PNG format. */
    uint32	iCompression;	/* Compression method */
    uint32	iSizeImage;     /* Size of uncomressed image in bytes. May
				 * be 0 for BMPC_RGB bitmaps. If iCompression
				 * is BI_JPEG or BI_PNG, iSizeImage indicates
				 * the size of the JPEG or PNG image buffer. */
    int32	iXPelsPerMeter; /* X resolution, pixels per meter (0 if not used) */
    int32	iYPelsPerMeter; /* Y resolution, pixels per meter (0 if not used) */
    uint32	iClrUsed;       /* Size of colour table. If 0, iBitCount should
				 * be used to calculate this value
				 * (1<<iBitCount). This value should be
				 * unsigned for proper shifting. */
    int32	iClrImportant;  /* Number of important colours. If 0, all
				 * colours are required */

    /*
     * Fields above should be used for bitmaps, compatible with Windows NT 3.51
     * and earlier. Windows 98/Me, Windows 2000/XP introduces additional fields:
     */

    int32	iRedMask;       /* Colour mask that specifies the red component
				 * of each pixel, valid only if iCompression
				 * is set to BI_BITFIELDS. */
    int32	iGreenMask;     /* The same for green component */
    int32	iBlueMask;      /* The same for blue component */
    int32	iAlphaMask;     /* Colour mask that specifies the alpha
				 * component of each pixel. */
    uint32	iCSType;        /* Colour space of the DIB. */
    BMPCIEXYZTriple sEndpoints; /* This member is ignored unless the iCSType
				 * member specifies BMPLT_CALIBRATED_RGB. */
    int32	iGammaRed;      /* Toned response curve for red. This member
				 * is ignored unless color values are
				 * calibrated RGB values and iCSType is set to
				 * BMPLT_CALIBRATED_RGB. Specified
				 * in 16^16 format. */
    int32	iGammaGreen;    /* Toned response curve for green. */
    int32	iGammaBlue;     /* Toned response curve for blue. */
} BMPInfoHeader;

/*
 * Info header size in bytes:
 */
const unsigned int  BIH_WIN4SIZE = 40; /* for BMPT_WIN4 */
const unsigned int  BIH_WIN5SIZE = 57; /* for BMPT_WIN5 */
const unsigned int  BIH_OS21SIZE = 12; /* for BMPT_OS21 */
const unsigned int  BIH_OS22SIZE = 64; /* for BMPT_OS22 */

/*
 * We will use plain byte array instead of this structure, but declaration
 * provided for reference
 */
typedef struct
{
    char       bBlue;
    char       bGreen;
    char       bRed;
    char       bReserved;      /* Must be 0 */
} BMPColorEntry;

static	uint16 compression = (uint16) -1;
static	int jpegcolormode = JPEGCOLORMODE_RGB;
static	int quality = 75;		/* JPEG quality */
static	uint16 predictor = 0;

static void usage(void);
static int processCompressOptions(char*);
static void rearrangePixels(char *, uint32, uint32);

int
main(int argc, char* argv[])
{
	uint32	width, length;
	uint16	nbands = 1;		/* number of bands in input image */
        uint16	depth = 8;		/* bits per pixel in input image */
	uint32	rowsperstrip = (uint32) -1;
        uint16	photometric = PHOTOMETRIC_MINISBLACK;
	int	fd = 0;
	struct stat instat;
	char	*outfilename = NULL, *infilename = NULL;
	TIFF	*out = NULL;

	BMPFileHeader file_hdr;
        BMPInfoHeader info_hdr;
        int     bmp_type;
        uint32  clr_tbl_size, n_clr_elems = 3;
        unsigned char *clr_tbl;
	unsigned short *red_tbl = NULL, *green_tbl = NULL, *blue_tbl = NULL;
	uint32	row, clr;

	int	c;
	extern int optind;
	extern char* optarg;

	while ((c = getopt(argc, argv, "c:r:o:h")) != -1) {
		switch (c) {
		case 'c':		/* compression scheme */
			if (!processCompressOptions(optarg))
				usage();
			break;
		case 'r':		/* rows/strip */
			rowsperstrip = atoi(optarg);
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

	if (outfilename == NULL)
		outfilename = argv[argc-1];
	out = TIFFOpen(outfilename, "w");
	if (out == NULL) {
		TIFFError(infilename, "Cannot open file %s for output",
			  outfilename);
		goto bad3;
	}
	

	while (optind < argc-1) {
		infilename = argv[optind];
		optind++;
	    
		fd = open(infilename, O_RDONLY|O_BINARY, 0);
		if (fd < 0) {
			TIFFError(infilename, "Cannot open input file");
			return -1;
		}

		read(fd, file_hdr.bType, 2);
		if(file_hdr.bType[0] != 'B' || file_hdr.bType[1] != 'M') {
			TIFFError(infilename, "File is not BMP");
			goto bad;
		}

/* -------------------------------------------------------------------- */
/*      Read the BMPFileHeader. We need iOffBits value only             */
/* -------------------------------------------------------------------- */
		lseek(fd, 10, SEEK_SET);
		read(fd, &file_hdr.iOffBits, 4);
#ifdef WORDS_BIGENDIAN
		TIFFSwabLong(&file_hdr.iOffBits);
#endif
		fstat(fd, &instat);
		file_hdr.iSize = instat.st_size;

/* -------------------------------------------------------------------- */
/*      Read the BMPInfoHeader.                                         */
/* -------------------------------------------------------------------- */

		lseek(fd, BFH_SIZE, SEEK_SET);
		read(fd, &info_hdr.iSize, 4);
#ifdef WORDS_BIGENDIAN
		TIFFSwabLong(&info_hdr.iSize);
#endif

		if (info_hdr.iSize == BIH_WIN4SIZE)
			bmp_type = BMPT_WIN4;
		else if (info_hdr.iSize == BIH_OS21SIZE)
			bmp_type = BMPT_OS21;
		else if (info_hdr.iSize == BIH_OS22SIZE
			 || info_hdr.iSize == 16)
			bmp_type = BMPT_OS22;
		else
			bmp_type = BMPT_WIN5;

		if (bmp_type == BMPT_WIN4
		    || bmp_type == BMPT_WIN5
		    || bmp_type == BMPT_OS22) {
			read(fd, &info_hdr.iWidth, 4);
			read(fd, &info_hdr.iHeight, 4);
			read(fd, &info_hdr.iPlanes, 2);
			read(fd, &info_hdr.iBitCount, 2);
			read(fd, &info_hdr.iCompression, 4);
			read(fd, &info_hdr.iSizeImage, 4);
			read(fd, &info_hdr.iXPelsPerMeter, 4);
			read(fd, &info_hdr.iYPelsPerMeter, 4);
			read(fd, &info_hdr.iClrUsed, 4);
			read(fd, &info_hdr.iClrImportant, 4);
#ifdef WORDS_BIGENDIAN
			TIFFSwabLong((uint32*) &info_hdr.iWidth);
			TIFFSwabLong((uint32*) &info_hdr.iHeight);
			TIFFSwabShort((uint16*) &info_hdr.iPlanes);
			TIFFSwabShort((uint16*) &info_hdr.iBitCount);
			TIFFSwabLong((uint32*) &info_hdr.iCompression);
			TIFFSwabLong((uint32*) &info_hdr.iSizeImage);
			TIFFSwabLong((uint32*) &info_hdr.iXPelsPerMeter);
			TIFFSwabLong((uint32*) &info_hdr.iYPelsPerMeter);
			TIFFSwabLong((uint32*) &info_hdr.iClrUsed);
			TIFFSwabLong((uint32*) &info_hdr.iClrImportant);
#endif
			n_clr_elems = 4;
		}

		if (bmp_type == BMPT_OS22) {
			/* 
			 * FIXME: different info in different documents
			 * regarding this!
			 */
			 n_clr_elems = 3;
		}

		if (bmp_type == BMPT_OS21) {
			int16  iShort;

			read(fd, &iShort, 2);
#ifdef WORDS_BIGENDIAN
			TIFFSwabShort((uint16*) &iShort);
#endif
			info_hdr.iWidth = iShort;
			read(fd, &iShort, 2);
#ifdef WORDS_BIGENDIAN
			TIFFSwabShort((uint16*) &iShort);
#endif
			info_hdr.iHeight = iShort;
			read(fd, &iShort, 2);
#ifdef WORDS_BIGENDIAN
			TIFFSwabShort((uint16*) &iShort);
#endif
			info_hdr.iPlanes = iShort;
			read(fd, &iShort, 2);
#ifdef WORDS_BIGENDIAN
			TIFFSwabShort((uint16*) &iShort);
#endif
			info_hdr.iBitCount = iShort;
			info_hdr.iCompression = BMPC_RGB;
			n_clr_elems = 3;
		}

		if (info_hdr.iBitCount != 1  && info_hdr.iBitCount != 4  &&
		    info_hdr.iBitCount != 8  && info_hdr.iBitCount != 16 &&
		    info_hdr.iBitCount != 24 && info_hdr.iBitCount != 32) {
		    TIFFError(infilename,
			      "Cannot process BMP file with bit count %d",
			      info_hdr.iBitCount);
		    close(fd);
		    return 0;
		}

		width = info_hdr.iWidth;
		length = (info_hdr.iHeight > 0) ? info_hdr.iHeight : -info_hdr.iHeight;

		switch (info_hdr.iBitCount)
		{
			case 1:
			case 4:
			case 8:
				nbands = 1;
				depth = info_hdr.iBitCount;
				photometric = PHOTOMETRIC_PALETTE;
				/* Allocate memory for colour table and read it. */
				if (info_hdr.iClrUsed)
				    clr_tbl_size =
					    ((uint32)(1<<depth)<info_hdr.iClrUsed)
					    ? (uint32) (1 << depth)
					    : info_hdr.iClrUsed;
				else
				    clr_tbl_size = 1 << depth;
				clr_tbl = (unsigned char *)
					_TIFFmalloc(n_clr_elems * clr_tbl_size);
				if (!clr_tbl) {
					TIFFError(infilename,
					"Can't allocate space for color table");
					goto bad;
				}

				lseek(fd, BFH_SIZE + info_hdr.iSize, SEEK_SET);
				read(fd, clr_tbl, n_clr_elems * clr_tbl_size);

				red_tbl = (unsigned short*)
					_TIFFmalloc(((tmsize_t)1)<<depth * sizeof(unsigned short));
				if (!red_tbl) {
					TIFFError(infilename,
				"Can't allocate space for red component table");
					_TIFFfree(clr_tbl);
					goto bad1;
				}
				green_tbl = (unsigned short*)
					_TIFFmalloc(((tmsize_t)1)<<depth * sizeof(unsigned short));
				if (!green_tbl) {
					TIFFError(infilename,
				"Can't allocate space for green component table");
					_TIFFfree(clr_tbl);
					goto bad2;
				}
				blue_tbl = (unsigned short*)
					_TIFFmalloc(((tmsize_t)1)<<depth * sizeof(unsigned short));
				if (!blue_tbl) {
					TIFFError(infilename,
				"Can't allocate space for blue component table");
					_TIFFfree(clr_tbl);
					goto bad3;
				}

				for(clr = 0; clr < clr_tbl_size; clr++) {
				    red_tbl[clr] = 257*clr_tbl[clr*n_clr_elems+2];
				    green_tbl[clr] = 257*clr_tbl[clr*n_clr_elems+1];
				    blue_tbl[clr] = 257*clr_tbl[clr*n_clr_elems];
				}

				_TIFFfree(clr_tbl);
				break;
			case 16:
			case 24:
				nbands = 3;
				depth = info_hdr.iBitCount / nbands;
				photometric = PHOTOMETRIC_RGB;
				break;
			case 32:
				nbands = 3;
				depth = 8;
				photometric = PHOTOMETRIC_RGB;
				break;
			default:
				break;
		}

/* -------------------------------------------------------------------- */
/*  Create output file.                                                 */
/* -------------------------------------------------------------------- */

		TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
		TIFFSetField(out, TIFFTAG_IMAGELENGTH, length);
		TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, nbands);
		TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, depth);
		TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
			     TIFFDefaultStripSize(out, rowsperstrip));
		
		if (red_tbl && green_tbl && blue_tbl) {
			TIFFSetField(out, TIFFTAG_COLORMAP,
				     red_tbl, green_tbl, blue_tbl);
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

/* -------------------------------------------------------------------- */
/*  Read uncompressed image data.                                       */
/* -------------------------------------------------------------------- */

		if (info_hdr.iCompression == BMPC_RGB) {
			uint32 offset, size;
			char *scanbuf;

			/* XXX: Avoid integer overflow. We can calculate size
			 * in one step using
			 *
			 *  size = ((width * info_hdr.iBitCount + 31) & ~31) / 8
			 *
			 * formulae, but we should check for overflow
			 * conditions during calculation.
			 */
			size = width * info_hdr.iBitCount + 31;
			if (!width || !info_hdr.iBitCount
			    || (size - 31) / info_hdr.iBitCount != width ) {
				TIFFError(infilename,
					  "Wrong image parameters; can't "
					  "allocate space for scanline buffer");
				goto bad3;
			}
			size = (size & ~31) / 8;

			scanbuf = (char *) _TIFFmalloc(size);
			if (!scanbuf) {
				TIFFError(infilename,
				"Can't allocate space for scanline buffer");
				goto bad3;
			}

			for (row = 0; row < length; row++) {
				if (info_hdr.iHeight > 0)
					offset = file_hdr.iOffBits+(length-row-1)*size;
				else
					offset = file_hdr.iOffBits + row * size;
				if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
					TIFFError(infilename,
						  "scanline %lu: Seek error",
						  (unsigned long) row);
					break;
				}

				if (read(fd, scanbuf, size) < 0) {
					TIFFError(infilename,
						  "scanline %lu: Read error",
						  (unsigned long) row);
					break;
				}

				rearrangePixels(scanbuf, width, info_hdr.iBitCount);

				if (TIFFWriteScanline(out, scanbuf, row, 0)<0) {
					TIFFError(infilename,
						  "scanline %lu: Write error",
						  (unsigned long) row);
					break;
				}
			}

			_TIFFfree(scanbuf);

/* -------------------------------------------------------------------- */
/*  Read compressed image data.                                         */
/* -------------------------------------------------------------------- */

		} else if ( info_hdr.iCompression == BMPC_RLE8
			    || info_hdr.iCompression == BMPC_RLE4 ) {
			uint32		i, j, k, runlength;
			uint32		compr_size, uncompr_size;
			unsigned char   *comprbuf;
			unsigned char   *uncomprbuf;

			compr_size = file_hdr.iSize - file_hdr.iOffBits;
			uncompr_size = width * length;
			comprbuf = (unsigned char *) _TIFFmalloc( compr_size );
			if (!comprbuf) {
				TIFFError(infilename,
			"Can't allocate space for compressed scanline buffer");
				goto bad3;
			}
			uncomprbuf = (unsigned char *)_TIFFmalloc(uncompr_size);
			if (!uncomprbuf) {
				TIFFError(infilename,
			"Can't allocate space for uncompressed scanline buffer");
				goto bad3;
			}

			lseek(fd, file_hdr.iOffBits, SEEK_SET);
			read(fd, comprbuf, compr_size);
			i = 0;
			j = 0;
			if (info_hdr.iBitCount == 8) {		/* RLE8 */
			    while(j < uncompr_size && i < compr_size) {
				if ( comprbuf[i] ) {
				    runlength = comprbuf[i++];
				    while( runlength > 0
					   && j < uncompr_size
					   && i < compr_size ) {
					uncomprbuf[j++] = comprbuf[i];
					runlength--;
				    }
				    i++;
				} else {
				    i++;
				    if (comprbuf[i] == 0) /* Next scanline */
					i++;
				    else if (comprbuf[i] == 1) /* End of image */
					break;
				    else if (comprbuf[i] == 2) { /* Move to... */
					i++;
					if (i < compr_size - 1) {
					    j+=comprbuf[i]+comprbuf[i+1]*width;
					    i += 2;
					}
					else
					    break;
				    } else {            /* Absolute mode */
					runlength = comprbuf[i++];
					for (k = 0; k < runlength && j < uncompr_size && i < compr_size; k++)
					    uncomprbuf[j++] = comprbuf[i++];
					if ( k & 0x01 )
					    i++;
				    }
				}
			    }
			}
			else {				    /* RLE4 */
			    while( j < uncompr_size && i < compr_size ) {
				if ( comprbuf[i] ) {
				    runlength = comprbuf[i++];
				    while( runlength > 0 && j < uncompr_size && i < compr_size ) {
					if ( runlength & 0x01 )
					    uncomprbuf[j++] = (comprbuf[i] & 0xF0) >> 4;
					else
					    uncomprbuf[j++] = comprbuf[i] & 0x0F;
					runlength--;
				    }
				    i++;
				} else {
				    i++;
				    if (comprbuf[i] == 0) /* Next scanline */
					i++;
				    else if (comprbuf[i] == 1) /* End of image */
					break;
				    else if (comprbuf[i] == 2) { /* Move to... */
					i++;
					if (i < compr_size - 1) {
					    j+=comprbuf[i]+comprbuf[i+1]*width;
					    i += 2;
					}
					else
					    break;
				    } else {            /* Absolute mode */
					runlength = comprbuf[i++];
					for (k = 0; k < runlength && j < uncompr_size && i < compr_size; k++) {
					    if (k & 0x01)
						uncomprbuf[j++] = comprbuf[i++] & 0x0F;
					    else
						uncomprbuf[j++] = (comprbuf[i] & 0xF0) >> 4;
					}
					if (k & 0x01)
					    i++;
				    }
				}
			    }
			}

			_TIFFfree(comprbuf);

			for (row = 0; row < length; row++) {
				if (TIFFWriteScanline(out,
					uncomprbuf + (length - row - 1) * width,
					row, 0) < 0) {
					TIFFError(infilename,
						"scanline %lu: Write error.\n",
						  (unsigned long) row);
				}
			}

			_TIFFfree(uncomprbuf);
		}
		TIFFWriteDirectory(out);
		if (blue_tbl) {
		  _TIFFfree(blue_tbl);
		  blue_tbl=NULL;
		}
		if (green_tbl) {
		  _TIFFfree(green_tbl);
		  green_tbl=NULL;
		}
		if (red_tbl) {
		  _TIFFfree(red_tbl);
		  red_tbl=NULL;
		}
	}

bad3:
	if (blue_tbl)
		_TIFFfree(blue_tbl);
bad2:
	if (green_tbl)
		_TIFFfree(green_tbl);
bad1:
	if (red_tbl)
		_TIFFfree(red_tbl);
bad:
        close(fd);

	if (out)
		TIFFClose(out);
        return 0;
}

/*
 * Image data in BMP file stored in BGR (or ABGR) format. We should rearrange
 * pixels to RGB (RGBA) format.
 */
static void
rearrangePixels(char *buf, uint32 width, uint32 bit_count)
{
	char tmp;
	uint32 i;

        switch(bit_count) {
		case 16:    /* FIXME: need a sample file */
                        break;
                case 24:
			for (i = 0; i < width; i++, buf += 3) {
				tmp = *buf;
				*buf = *(buf + 2);
				*(buf + 2) = tmp;
			}
                        break;
                case 32:
			{
				char	*buf1 = buf;

				for (i = 0; i < width; i++, buf += 4) {
					tmp = *buf;
					*buf1++ = *(buf + 2);
					*buf1++ = *(buf + 1);
					*buf1++ = tmp;
				}
			}
                        break;
                default:
                        break;
        }
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
"bmp2tiff --- convert Windows BMP files to TIFF",
"usage: bmp2tiff [options] input.bmp [input2.bmp ...] output.tif",
"where options are:",
" -r #		make each strip have no more than # rows",
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
