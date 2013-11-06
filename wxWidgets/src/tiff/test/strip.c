
/*
 * Copyright (c) 2004, Andrey Kiselev  <dron@ak4719.spb.edu>
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

/*
 * TIFF Library
 *
 * Functions to test strip interface of libtiff.
 */

#include <stdio.h>
#include <string.h>

#include "tiffio.h"

int
write_strips(TIFF *tif, const tdata_t array, const tsize_t size)
{
	tstrip_t	strip, nstrips;
	tsize_t		stripsize, offset;

	stripsize = TIFFStripSize(tif);
	if (!stripsize) {
		fprintf (stderr, "Wrong size of strip.\n");
		return -1;
	}

	nstrips = TIFFNumberOfStrips(tif);
	for (offset = 0, strip = 0;
	     offset < size && strip < nstrips;
	     offset+=stripsize, strip++) {
		/*
		 * Properly write last strip.
		 */
		tsize_t	bufsize = size - offset;
		if (bufsize > stripsize)
			bufsize = stripsize;

		if (TIFFWriteEncodedStrip(tif, strip, (char *)array + offset,
					  bufsize) != bufsize) {
			fprintf (stderr, "Can't write strip %lu.\n",
				 (unsigned long)strip);
			return -1;
		}
        }

	return 0;
}

int
read_strips(TIFF *tif, const tdata_t array, const tsize_t size)
{
	tstrip_t	strip, nstrips;
	tsize_t		stripsize, offset;
	tdata_t		buf = NULL;

	stripsize = TIFFStripSize(tif);
	if (!stripsize) {
		fprintf (stderr, "Wrong size of strip.\n");
		return -1;
	}

	buf = _TIFFmalloc(stripsize);
	if (!buf) {
		fprintf (stderr, "Can't allocate space for strip buffer.\n");
		return -1;
	}

	nstrips = TIFFNumberOfStrips(tif);
	for (offset = 0, strip = 0;
	     offset < size && strip < nstrips;
	     offset+=stripsize, strip++) {
		/*
		 * Properly read last strip.
		 */
		tsize_t	bufsize = size - offset;
		if (bufsize > stripsize)
			bufsize = stripsize;

		if (TIFFReadEncodedStrip(tif, strip, buf, -1) != bufsize) {
			fprintf (stderr, "Can't read strip %lu.\n",
				 (unsigned long)strip);
			return -1;
		}
		if (memcmp(buf, (char *)array + offset, bufsize) != 0) {
			fprintf (stderr, "Wrong data read for strip %lu.\n",
				 (unsigned long)strip);
			_TIFFfree(buf);
			return -1;
		}
        }

	_TIFFfree(buf);

	return 0;
}

int
create_image_striped(const char *name, uint32 width, uint32 length,
		      uint32 rowsperstrip, uint16 compression,
		      uint16 spp, uint16 bps, uint16 photometric,
		      uint16 sampleformat, uint16 planarconfig,
		      const tdata_t array, const tsize_t size)
{
	TIFF		*tif;

	/* Test whether we can write tags. */
	tif = TIFFOpen(name, "w");
	if (!tif)
		goto openfailure;

	if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width)) {
		fprintf (stderr, "Can't set ImageWidth tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_IMAGELENGTH, length)) {
		fprintf (stderr, "Can't set ImageLength tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bps)) {
		fprintf (stderr, "Can't set BitsPerSample tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, spp)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip)) {
		fprintf (stderr, "Can't set RowsPerStrip tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PLANARCONFIG, planarconfig)) {
		fprintf (stderr, "Can't set PlanarConfiguration tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photometric)) {
		fprintf (stderr, "Can't set PhotometricInterpretation tag.\n");
		goto failure;
	}

	if (write_strips(tif, array, size) < 0) {
		fprintf (stderr, "Can't write image data.\n");
		goto failure;
	}

	TIFFClose(tif);
	return 0;

failure:
	TIFFClose(tif);
openfailure:
	fprintf (stderr, "Can't create test TIFF file %s:\n"
"    ImageWidth=%ld, ImageLength=%ld, RowsPerStrip=%ld, Compression=%d,\n"
"    BitsPerSample=%d, SamplesPerPixel=%d, SampleFormat=%d,\n"
"    PlanarConfiguration=%d, PhotometricInterpretation=%d.\n",
		 name, (long) width, (long) length, (long) rowsperstrip,
                 compression, bps, spp, sampleformat, planarconfig,
		 photometric);
	return -1;
}

int
read_image_striped(const char *name, uint32 width, uint32 length,
		    uint32 rowsperstrip, uint16 compression,
		    uint16 spp, uint16 bps, uint16 photometric,
		    uint16 sampleformat, uint16 planarconfig,
		    const tdata_t array, const tsize_t size)
{
	TIFF		*tif;
	uint16		value_u16;
	uint32		value_u32;

	/* Test whether we can read written values. */
	tif = TIFFOpen(name, "r");
	if (!tif)
		goto openfailure;
	
	if (TIFFIsTiled(tif)) {
		fprintf (stderr, "Can't read image %s, it is tiled.\n",
			 name);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &value_u32)
	    || value_u32 != width) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_IMAGEWIDTH);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &value_u32)
	    || value_u32 != length) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_IMAGELENGTH);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &value_u16)
	    || value_u16 != bps) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_BITSPERSAMPLE);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &value_u16)
	    || value_u16 != photometric) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_PHOTOMETRIC);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &value_u16)
	    || value_u16 != spp) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_SAMPLESPERPIXEL);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &value_u32)
	    || value_u32 != rowsperstrip) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_ROWSPERSTRIP);
		goto failure;
	}
	if (!TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &value_u16)
	    || value_u16 != planarconfig) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_PLANARCONFIG);
		goto failure;
	}

	if (read_strips(tif, array, size) < 0) {
		fprintf (stderr, "Can't read image data.\n");
		goto failure;
	}

	TIFFClose(tif);
	return 0;

failure:
	TIFFClose(tif);
openfailure:
	fprintf (stderr, "Can't read test TIFF file %s:\n"
"    ImageWidth=%ld, ImageLength=%ld, RowsPerStrip=%ld, Compression=%d,\n"
"    BitsPerSample=%d, SamplesPerPixel=%d, SampleFormat=%d,\n"
"    PlanarConfiguration=%d, PhotometricInterpretation=%d.\n",
		 name, (long) width, (long) length, (long) rowsperstrip,
                 compression, bps, spp, sampleformat, planarconfig,
		 photometric);
	return -1;
}

int
write_scanlines(TIFF *tif, const tdata_t array, const tsize_t size)
{
	uint32		length, row;
	tsize_t		scanlinesize, offset;
        (void) size;

	if (!TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &length)) {
		fprintf (stderr, "Can't get tag %d.\n", TIFFTAG_IMAGELENGTH);
		return -1;
	}
	
	scanlinesize = TIFFScanlineSize(tif);
	if (!scanlinesize) {
		fprintf (stderr, "Wrong size of scanline.\n");
		return -1;
	}

	for (offset = 0, row = 0; row < length; offset+=scanlinesize, row++) {
		if (TIFFWriteScanline(tif, (char *)array + offset, row, 0) < 0) {
			fprintf (stderr,
				 "Can't write image data at row %lu.\n", (long) row);
			return -1;
		}
        }

	return 0;
}

/* vim: set ts=8 sts=8 sw=8 noet: */
