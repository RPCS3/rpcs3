
/*
 * tiff-bi.c -- create a Class B (bilevel) TIFF file
 *
 * Copyright 1990 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                        All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>

#include "tiffio.h"

#define WIDTH       512
#define HEIGHT      WIDTH

int main(int argc, char **argv)
{
    int             i;
    unsigned char * scan_line;
    TIFF *          tif;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s tiff-image\n", argv[0]);
        return 0;
    }

    if ((tif = TIFFOpen(argv[1], "w")) == NULL) {
        fprintf(stderr, "can't open %s as a TIFF file\n", argv[1]);
        return 0;
    }

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, WIDTH);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, HEIGHT);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 1);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

    scan_line = (unsigned char *) malloc(WIDTH / 8);

    for (i = 0; i < (WIDTH / 8) / 2; i++)
        scan_line[i] = 0;

    for (i = (WIDTH / 8) / 2; i < (WIDTH / 8); i++)
        scan_line[i] = 255;

    for (i = 0; i < HEIGHT / 2; i++)
        TIFFWriteScanline(tif, scan_line, i, 0);

    for (i = 0; i < (WIDTH / 8) / 2; i++)
        scan_line[i] = 255;

    for (i = (WIDTH / 8) / 2; i < (WIDTH / 8); i++)
        scan_line[i] = 0;

    for (i = HEIGHT / 2; i < HEIGHT; i++)
        TIFFWriteScanline(tif, scan_line, i, 0);

    free(scan_line);
    TIFFClose(tif);
    return 0;
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
