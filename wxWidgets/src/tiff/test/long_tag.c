
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
 * Module to test LONG tags read/write functions.
 */

#include "tif_config.h"

#include <stdio.h>

#ifdef HAVE_UNISTD_H 
# include <unistd.h> 
#endif 

#include "tiffio.h"

extern int CheckLongField(TIFF *, ttag_t, uint32);

const char	*filename = "long_test.tiff";

static struct Tags {
	ttag_t		tag;
	short		count;
	uint32		value;
} long_tags[] = {
	{ TIFFTAG_SUBFILETYPE, 1, FILETYPE_REDUCEDIMAGE|FILETYPE_PAGE|FILETYPE_MASK }
};
#define NTAGS   (sizeof (long_tags) / sizeof (long_tags[0]))

const uint32	width = 1;
const uint32	length = 1;
const uint32	rows_per_strip = 1;

int
main(int argc, char **argv)
{
	TIFF		*tif;
	unsigned int	i;
	unsigned char	buf[3] = { 0, 127, 255 };
        (void) argc;
        (void) argv;

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
	if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8)) {
		fprintf (stderr, "Can't set BitsPerSample tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rows_per_strip)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)) {
		fprintf (stderr, "Can't set PlanarConfiguration tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB)) {
		fprintf (stderr, "Can't set PhotometricInterpretation tag.\n");
		goto failure;
	}

	for (i = 0; i < NTAGS; i++) {
		if (!TIFFSetField(tif, long_tags[i].tag,
				  long_tags[i].value)) {
			fprintf(stderr, "Can't set tag %d.\n",
				(int)long_tags[i].tag);
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

	if (CheckLongField(tif, TIFFTAG_ROWSPERSTRIP, rows_per_strip) < 0)
		goto failure;

	for (i = 0; i < NTAGS; i++) {
		if (CheckLongField(tif, long_tags[i].tag,
				   long_tags[i].value) < 0)
			goto failure;
	}

	TIFFClose(tif);
	
	/* All tests passed; delete file and exit with success status. */
	unlink(filename);
	return 0;

failure:
	/* Something goes wrong; close file and return unsuccessful status. */
	TIFFClose(tif);
	unlink(filename);
	return 1;
}

/* vim: set ts=8 sts=8 sw=8 noet: */
