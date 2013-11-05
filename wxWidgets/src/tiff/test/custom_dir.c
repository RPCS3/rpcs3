
/*
 * Copyright (c) 2012, Frank Warmerdam <warmerdam@pobox.com>
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
 * Module to handling of custom directories like EXIF.
 */

#include "tif_config.h"
#include <stdio.h>
#include <string.h>

#ifdef HAVE_UNISTD_H 
# include <unistd.h> 
#endif 

#include "tiffio.h"
#include "tif_dir.h"
#include "tifftest.h"

static const char filename[] = "custom_dir.tif";

#define	SPP	3		/* Samples per pixel */
const uint16	width = 1;
const uint16	length = 1;
const uint16	bps = 8;
const uint16	photometric = PHOTOMETRIC_RGB;
const uint16	rows_per_strip = 1;
const uint16	planarconfig = PLANARCONFIG_CONTIG;

static TIFFField
customFields[] = {
	{ TIFFTAG_IMAGEWIDTH, -1, -1, TIFF_ASCII, 0, TIFF_SETGET_ASCII, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 1, 0, "Custom1", NULL },
	{ TIFFTAG_DOTRANGE, -1, -1, TIFF_ASCII, 0, TIFF_SETGET_ASCII, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 1, 0, "Custom2", NULL },
};

static TIFFFieldArray customFieldArray = { tfiatOther, 0, 2, customFields };

int
main()
{
	TIFF		*tif;
	unsigned char	buf[SPP] = { 0, 127, 255 };
	uint64          dir_offset = 0, dir_offset2 = 0;
	uint64          read_dir_offset = 0, read_dir_offset2 = 0;
	uint64          *dir_offset2_ptr = NULL;
	char           *ascii_value;
	uint16          count16 = 0;
	

	/* We write the main directory as a simple image. */
	tif = TIFFOpen(filename, "w+");
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

	/* Write dummy pixel data. */
	if (!TIFFWriteScanline(tif, buf, 0, 0) < 0) {
		fprintf (stderr, "Can't write image data.\n");
		goto failure;
	}

        if (!TIFFWriteDirectory( tif )) {
		fprintf (stderr, "TIFFWriteDirectory() failed.\n");
		goto failure;
	}

	/* 
	 * Now create an EXIF directory. 
	 */
	if (TIFFCreateEXIFDirectory(tif) != 0) {
		fprintf (stderr, "TIFFCreateEXIFDirectory() failed.\n" );
		goto failure;
	}

	if (!TIFFSetField( tif, EXIFTAG_SPECTRALSENSITIVITY, "EXIF Spectral Sensitivity")) {
		fprintf (stderr, "Can't write SPECTRALSENSITIVITY\n" );
		goto failure;
	}		
	
        if (!TIFFWriteCustomDirectory( tif, &dir_offset )) {
		fprintf (stderr, "TIFFWriteCustomDirectory() with EXIF failed.\n");
		goto failure;
	}

	/*
	 * Now create a custom directory with tags that conflict with mainline 
	 * TIFF tags.
	 */
	
	TIFFFreeDirectory( tif );
	if (TIFFCreateCustomDirectory(tif, &customFieldArray) != 0) {
		fprintf (stderr, "TIFFCreateEXIFDirectory() failed.\n" );
		goto failure;
	}

	if (!TIFFSetField( tif, TIFFTAG_IMAGEWIDTH, "*Custom1")) { /* not really IMAGEWIDTH */
		fprintf (stderr, "Can't write pseudo-IMAGEWIDTH.\n" );
		goto failure;
	}		
	
	if (!TIFFSetField( tif, TIFFTAG_DOTRANGE, "*Custom2")) { /* not really DOTWIDTH */
		fprintf (stderr, "Can't write pseudo-DOTWIDTH.\n" );
		goto failure;
	}		
	
        if (!TIFFWriteCustomDirectory( tif, &dir_offset2 )) {
		fprintf (stderr, "TIFFWriteCustomDirectory() with EXIF failed.\n");
		goto failure;
	}

	/*
	 * Go back to the first directory, and add the EXIFIFD pointer. 
	 */
	TIFFSetDirectory(tif, 0);
	TIFFSetField(tif, TIFFTAG_EXIFIFD, dir_offset );
	TIFFSetField(tif, TIFFTAG_SUBIFD, 1, &dir_offset2 );

	TIFFClose(tif);
	
	/* Ok, now test whether we can read written values in the EXIF directory. */
	tif = TIFFOpen(filename, "r");
	
	TIFFGetField(tif, TIFFTAG_EXIFIFD, &read_dir_offset );
	if( read_dir_offset != dir_offset ) {
		fprintf (stderr, "Did not get expected EXIFIFD.\n" );
		goto failure;
	}

	TIFFGetField(tif, TIFFTAG_SUBIFD, &count16, &dir_offset2_ptr );
	read_dir_offset2 = dir_offset2_ptr[0];
	if( read_dir_offset2 != dir_offset2 || count16 != 1) {
		fprintf (stderr, "Did not get expected SUBIFD.\n" );
		goto failure;
	}

	if( !TIFFReadEXIFDirectory(tif, read_dir_offset) ) {
		fprintf (stderr, "TIFFReadEXIFDirectory() failed.\n" );
		goto failure;
	}
	
	if (!TIFFGetField( tif, EXIFTAG_SPECTRALSENSITIVITY, &ascii_value) ) {
		fprintf (stderr, "reading SPECTRALSENSITIVITY failed.\n" );
		goto failure;
	}

	if( strcmp(ascii_value,"EXIF Spectral Sensitivity") != 0) {
		fprintf (stderr, "got wrong SPECTRALSENSITIVITY value.\n" );
		goto failure;
	}

	/* Try reading the Custom directory */
	
	if( !TIFFReadCustomDirectory(tif, read_dir_offset2, &customFieldArray) ) {
		fprintf (stderr, "TIFFReadCustomDirectory() failed.\n" );
		goto failure;
	}
	
	if (!TIFFGetField( tif, TIFFTAG_IMAGEWIDTH, &ascii_value) ) {
		fprintf (stderr, "reading pseudo-IMAGEWIDTH failed.\n" );
		goto failure;
	}

	if( strcmp(ascii_value,"*Custom1") != 0) {
		fprintf (stderr, "got wrong pseudo-IMAGEWIDTH value.\n" );
		goto failure;
	}

	if (!TIFFGetField( tif, TIFFTAG_DOTRANGE, &ascii_value) ) {
		fprintf (stderr, "reading pseudo-DOTRANGE failed.\n" );
		goto failure;
	}

	if( strcmp(ascii_value,"*Custom2") != 0) {
		fprintf (stderr, "got wrong pseudo-DOTRANGE value.\n" );
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
