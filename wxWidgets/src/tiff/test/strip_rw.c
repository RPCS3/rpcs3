
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
 * Test libtiff input/output routines.
 */

#include "tif_config.h"

#include <stdio.h>

#ifdef HAVE_UNISTD_H 
# include <unistd.h> 
#endif 

#include "tiffio.h"
#include "test_arrays.h"

extern int
create_image_striped(const char *, uint32, uint32, uint32, uint16, uint16,
		     uint16, uint16, uint16, uint16, const tdata_t,
		     const tsize_t);
extern int
read_image_striped(const char *, uint32, uint32, uint32, uint16, uint16,
		   uint16, uint16, uint16, uint16, const tdata_t,
		   const tsize_t);

const char	*filename = "strip_test.tiff";

int
main(int argc, char **argv)
{
	uint32		rowsperstrip;
	uint16		compression;
	uint16		spp, bps, photometric, sampleformat, planarconfig;
        (void) argc;
        (void) argv;

	/* 
	 * Test two special cases: image consisting from single line and image
	 * consisting from single column.
	 */
	rowsperstrip = 1;
	compression = COMPRESSION_NONE;
	spp = 1;
	bps = 8;
        photometric = PHOTOMETRIC_MINISBLACK;
	sampleformat = SAMPLEFORMAT_UINT;
	planarconfig = PLANARCONFIG_CONTIG;

	if (create_image_striped(filename, XSIZE * YSIZE, 1, rowsperstrip,
				  compression, spp, bps, photometric,
				  sampleformat, planarconfig,
				  (const tdata_t) byte_array1, byte_array1_size) < 0) {
		fprintf (stderr, "Can't create TIFF file %s.\n", filename);
		goto failure;
	}
	if (read_image_striped(filename, XSIZE * YSIZE, 1, rowsperstrip,
				compression, spp, bps, photometric,
				sampleformat, planarconfig,
				(const tdata_t) byte_array1, byte_array1_size) < 0) {
		fprintf (stderr, "Can't read TIFF file %s.\n", filename);
		goto failure;
	}
	unlink(filename);
		
	if (create_image_striped(filename, 1, XSIZE * YSIZE, rowsperstrip,
				  compression, spp, bps, photometric,
				  sampleformat, planarconfig,
				  (const tdata_t) byte_array1, byte_array1_size) < 0) {
		fprintf (stderr, "Can't create TIFF file %s.\n", filename);
		goto failure;
	}
	if (read_image_striped(filename, 1, XSIZE * YSIZE, rowsperstrip,
				compression, spp, bps, photometric,
				sampleformat, planarconfig,
				(const tdata_t) byte_array1, byte_array1_size) < 0) {
		fprintf (stderr, "Can't read TIFF file %s.\n", filename);
		goto failure;
	}
	unlink(filename);
		
	/* 
	 * Test one-channel image with different parameters.
	 */
	rowsperstrip = 1;
	spp = 1;
	bps = 8;
        photometric = PHOTOMETRIC_MINISBLACK;
	sampleformat = SAMPLEFORMAT_UINT;
	planarconfig = PLANARCONFIG_CONTIG;

	if (create_image_striped(filename, XSIZE, YSIZE, rowsperstrip,
				  compression, spp, bps, photometric,
				  sampleformat, planarconfig,
				  (const tdata_t) byte_array1, byte_array1_size) < 0) {
		fprintf (stderr, "Can't create TIFF file %s.\n", filename);
		goto failure;
	}
	if (read_image_striped(filename, XSIZE, YSIZE, rowsperstrip,
				compression, spp, bps, photometric,
				sampleformat, planarconfig,
				(const tdata_t) byte_array1, byte_array1_size) < 0) {
		fprintf (stderr, "Can't read TIFF file %s.\n", filename);
		goto failure;
	}
	unlink(filename);
	
	rowsperstrip = YSIZE;
	if (create_image_striped(filename, XSIZE, YSIZE, rowsperstrip,
				  compression, spp, bps, photometric,
				  sampleformat, planarconfig,
				  (const tdata_t) byte_array1, byte_array1_size) < 0) {
		fprintf (stderr, "Can't create TIFF file %s.\n", filename);
		goto failure;
	}
	if (read_image_striped(filename, XSIZE, YSIZE, rowsperstrip,
				compression, spp, bps, photometric,
				sampleformat, planarconfig,
				(const tdata_t) byte_array1, byte_array1_size) < 0) {
		fprintf (stderr, "Can't read TIFF file %s.\n", filename);
		goto failure;
	}
	unlink(filename);

	return 0;

failure:
	unlink(filename);
	return 1;
}

/* vim: set ts=8 sts=8 sw=8 noet: */
