
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
 * Module to test ASCII tags read/write functions.
 */

#include "tif_config.h"

#include <stdio.h>
#include <string.h>

#ifdef HAVE_UNISTD_H 
# include <unistd.h> 
#endif 

#include "tiffio.h"

static const char filename[] = "ascii_test.tiff";

static const struct {
	ttag_t		tag;
	const char	*value;
} ascii_tags[] = {
	{ TIFFTAG_DOCUMENTNAME, "Test TIFF image" },
	{ TIFFTAG_IMAGEDESCRIPTION, "Temporary test image" },
	{ TIFFTAG_MAKE, "This is not scanned image" },
	{ TIFFTAG_MODEL, "No scanner" },
	{ TIFFTAG_PAGENAME, "Test page" },
	{ TIFFTAG_SOFTWARE, "Libtiff library" },
	{ TIFFTAG_DATETIME, "2004:09:10 16:09:00" },
	{ TIFFTAG_ARTIST, "Andrey V. Kiselev" },
	{ TIFFTAG_HOSTCOMPUTER, "Debian GNU/Linux (Sarge)" },
	{ TIFFTAG_TARGETPRINTER, "No printer" },
	{ TIFFTAG_COPYRIGHT, "Copyright (c) 2004, Andrey Kiselev" },
	{ TIFFTAG_FAXSUBADDRESS, "Fax subaddress" },
	/* DGN tags */
	{ TIFFTAG_UNIQUECAMERAMODEL, "No camera" },
	{ TIFFTAG_CAMERASERIALNUMBER, "1234567890" }
};
#define NTAGS   (sizeof (ascii_tags) / sizeof (ascii_tags[0]))

static const char ink_names[] = "Red\0Green\0Blue";
const int ink_names_size = 15;

int
main()
{
	TIFF		*tif;
	size_t		i;
	unsigned char	buf[] = { 0, 127, 255 };
	char		*value;

	/* Test whether we can write tags. */
	tif = TIFFOpen(filename, "w");
	if (!tif) {
		fprintf (stderr, "Can't create test TIFF file %s.\n", filename);
		return 1;
	}

	if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, 1)) {
		fprintf (stderr, "Can't set ImageWidth tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_IMAGELENGTH, 1)) {
		fprintf (stderr, "Can't set ImageLength tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8)) {
		fprintf (stderr, "Can't set BitsPerSample tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, sizeof(buf))) {
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
		if (!TIFFSetField(tif, ascii_tags[i].tag,
				  ascii_tags[i].value)) {
			fprintf(stderr, "Can't set tag %lu.\n",
				(unsigned long)ascii_tags[i].tag);
			goto failure;
		}
	}

	/* InkNames tag has special form, so we handle it separately. */
	if (!TIFFSetField(tif, TIFFTAG_NUMBEROFINKS, 3)) {
		fprintf (stderr, "Can't set tag %d (NUMBEROFINKS).\n",
                         TIFFTAG_NUMBEROFINKS);
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_INKNAMES, ink_names_size, ink_names)) {
		fprintf (stderr, "Can't set tag %d (INKNAMES).\n",
			 TIFFTAG_INKNAMES);
		goto failure;
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

	for (i = 0; i < NTAGS; i++) {
		if (!TIFFGetField(tif, ascii_tags[i].tag, &value)
		    || strcmp(value, ascii_tags[i].value)) {
			fprintf(stderr, "Can't get tag %lu.\n",
				(unsigned long)ascii_tags[i].tag);
			goto failure;
		}
	}

	if (!TIFFGetField(tif, TIFFTAG_INKNAMES, &value)
	    || memcmp(value, ink_names, ink_names_size)) {
		fprintf (stderr, "Can't get tag %d (INKNAMES).\n",
			 TIFFTAG_INKNAMES);
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
