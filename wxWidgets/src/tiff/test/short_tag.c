
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
 * Module to test SHORT tags read/write functions.
 */

#include "tif_config.h"

#include <stdio.h>

#ifdef HAVE_UNISTD_H 
# include <unistd.h> 
#endif 

#include "tiffio.h"
#include "tifftest.h"

static const char filename[] = "short_test.tiff";

#define	SPP	3		/* Samples per pixel */
const uint16	width = 1;
const uint16	length = 1;
const uint16	bps = 8;
const uint16	photometric = PHOTOMETRIC_RGB;
const uint16	rows_per_strip = 1;
const uint16	planarconfig = PLANARCONFIG_CONTIG;

static const struct {
	const ttag_t	tag;
	const uint16	value;
} short_single_tags[] = {
	{ TIFFTAG_COMPRESSION, COMPRESSION_NONE },
	{ TIFFTAG_FILLORDER, FILLORDER_MSB2LSB },
	{ TIFFTAG_ORIENTATION, ORIENTATION_BOTRIGHT },
	{ TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH },
	{ TIFFTAG_MINSAMPLEVALUE, 23 },
	{ TIFFTAG_MAXSAMPLEVALUE, 241 },
	{ TIFFTAG_INKSET, INKSET_MULTIINK },
	{ TIFFTAG_NUMBEROFINKS, SPP },
	{ TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT }
};
#define NSINGLETAGS   (sizeof(short_single_tags) / sizeof(short_single_tags[0]))

static const struct {
	const ttag_t	tag;
	const uint16	values[2];
} short_paired_tags[] = {
	{ TIFFTAG_PAGENUMBER, {1, 1} },
	{ TIFFTAG_HALFTONEHINTS, {0, 255} },
	{ TIFFTAG_DOTRANGE, {8, 16} },
	{ TIFFTAG_YCBCRSUBSAMPLING, {2, 1} }
};
#define NPAIREDTAGS   (sizeof(short_paired_tags) / sizeof(short_paired_tags[0]))

int
main()
{
	TIFF		*tif;
	size_t		i;
	unsigned char	buf[SPP] = { 0, 127, 255 };

	/* Test whether we can write tags. */
	tif = TIFFOpen(filename, "w");
	if (!tif) {
		fprintf (stderr, "Can't create test TIFF file %s.\n", filename);
		return 1;
	}

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
	if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, SPP)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rows_per_strip)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
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

	for (i = 0; i < NSINGLETAGS; i++) {
		if (!TIFFSetField(tif, short_single_tags[i].tag,
				  short_single_tags[i].value)) {
			fprintf(stderr, "Can't set tag %lu.\n",
				(unsigned long)short_single_tags[i].tag);
			goto failure;
		}
	}

	for (i = 0; i < NPAIREDTAGS; i++) {
		if (!TIFFSetField(tif, short_paired_tags[i].tag,
				  short_paired_tags[i].values[0],
				  short_paired_tags[i].values[1])) {
			fprintf(stderr, "Can't set tag %lu.\n",
				(unsigned long)short_paired_tags[i].tag);
			goto failure;
		}
	}

	/* Write dummy pixel data. */
	if (!TIFFWriteScanline(tif, buf, 0, 0) < 0) {
		fprintf (stderr, "Can't write image data.\n");
		goto failure;
	}

	TIFFClose(tif);
	
	/* Ok, now test whether we can read written values. */
	tif = TIFFOpen(filename, "r");
	if (!tif) {
		fprintf (stderr, "Can't open test TIFF file %s.\n", filename);
		return 1;
	}
	
	if (CheckLongField(tif, TIFFTAG_IMAGEWIDTH, width) < 0)
		goto failure;

	if (CheckLongField(tif, TIFFTAG_IMAGELENGTH, length) < 0)
		goto failure;

	if (CheckShortField(tif, TIFFTAG_BITSPERSAMPLE, bps) < 0)
		goto failure;

	if (CheckShortField(tif, TIFFTAG_PHOTOMETRIC, photometric) < 0)
		goto failure;

	if (CheckShortField(tif, TIFFTAG_SAMPLESPERPIXEL, SPP) < 0)
		goto failure;

	if (CheckLongField(tif, TIFFTAG_ROWSPERSTRIP, rows_per_strip) < 0)
		goto failure;

	if (CheckShortField(tif, TIFFTAG_PLANARCONFIG, planarconfig) < 0)
		goto failure;

	for (i = 0; i < NSINGLETAGS; i++) {
		if (CheckShortField(tif, short_single_tags[i].tag,
				    short_single_tags[i].value) < 0)
			goto failure;
	}

	for (i = 0; i < NPAIREDTAGS; i++) {
		if (CheckShortPairedField(tif, short_paired_tags[i].tag,
					  short_paired_tags[i].values) < 0)
			goto failure;
	}

	TIFFClose(tif);
	
	/* All tests passed; delete file and exit with success status. */
	unlink(filename);
	return 0;

failure:
	/* 
	 * Something goes wrong; close file and return unsuccessful status.
	 * Do not remove the file for further manual investigation.
	 */
	TIFFClose(tif);
	return 1;
}

/* vim: set ts=8 sts=8 sw=8 noet: */
