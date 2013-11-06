
/*
 * tiff-rgb.c -- create a 24-bit Class R (rgb) TIFF file
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tiffio.h"

#define ROUND(x)    (uint16) ((x) + 0.5)
#define CMSIZE      256
#define WIDTH       525
#define HEIGHT      512
#define TIFF_GAMMA  2.2

void                Usage();
char *              programName;

int main(int argc, char **argv)
{
    char *          input_file = NULL;
    double          image_gamma = TIFF_GAMMA;
    int             i, j;
    TIFF *          tif;
    unsigned char * scan_line;
    uint16          red[CMSIZE], green[CMSIZE], blue[CMSIZE];
    float	    refblackwhite[2*3];

    programName = argv[0];

    switch (argc) {
    case 2:
        image_gamma = TIFF_GAMMA;
        input_file = argv[1];
        break;
    case 4:
        if (!strcmp(argv[1], "-gamma")) {
            image_gamma = atof(argv[2]);
            input_file = argv[3];
        } else
            Usage();
        break;
    default:
        Usage();
    }

    for (i = 0; i < CMSIZE; i++) {
        if (i == 0)
            red[i] = green[i] = blue[i] = 0;
        else {
            red[i] = ROUND((pow(i / 255.0, 1.0 / image_gamma) * 65535.0));
            green[i] = ROUND((pow(i / 255.0, 1.0 / image_gamma) * 65535.0));
            blue[i] = ROUND((pow(i / 255.0, 1.0 / image_gamma) * 65535.0));
        }
    }
    refblackwhite[0] = 0.0; refblackwhite[1] = 255.0;
    refblackwhite[2] = 0.0; refblackwhite[3] = 255.0;
    refblackwhite[4] = 0.0; refblackwhite[5] = 255.0;

    if ((tif = TIFFOpen(input_file, "w")) == NULL) {
        fprintf(stderr, "can't open %s as a TIFF file\n", input_file);
        exit(0);
    }

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, WIDTH);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, HEIGHT);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
#ifdef notdef
    TIFFSetField(tif, TIFFTAG_WHITEPOINT, whitex, whitey);
    TIFFSetField(tif, TIFFTAG_PRIMARYCHROMATICITIES, primaries);
#endif
    TIFFSetField(tif, TIFFTAG_REFERENCEBLACKWHITE, refblackwhite);
    TIFFSetField(tif, TIFFTAG_TRANSFERFUNCTION, red, green, blue);

    scan_line = (unsigned char *) malloc(WIDTH * 3);

    for (i = 0; i < 255; i++) {
        for (j = 0; j < 75; j++) {
             scan_line[j * 3] = 255;
             scan_line[(j * 3) + 1] = 255 - i;
             scan_line[(j * 3) + 2] = 255 - i;
        }
        for (j = 75; j < 150; j++) {
             scan_line[j * 3] = 255 - i;
             scan_line[(j * 3) + 1] = 255;
             scan_line[(j * 3) + 2] = 255 - i;
        }
        for (j = 150; j < 225; j++) {
             scan_line[j * 3] = 255 - i;
             scan_line[(j * 3) + 1] = 255 - i;
             scan_line[(j * 3) + 2] = 255;
        }
        for (j = 225; j < 300; j++) {
             scan_line[j * 3] = (i - 1) / 2;
             scan_line[(j * 3) + 1] = (i - 1) / 2;
             scan_line[(j * 3) + 2] = (i - 1) / 2;
        }
        for (j = 300; j < 375; j++) {
             scan_line[j * 3] = 255 - i;
             scan_line[(j * 3) + 1] = 255;
             scan_line[(j * 3) + 2] = 255;
        }
        for (j = 375; j < 450; j++) {
             scan_line[j * 3] = 255;
             scan_line[(j * 3) + 1] = 255 - i;
             scan_line[(j * 3) + 2] = 255;
        }
        for (j = 450; j < 525; j++) {
             scan_line[j * 3] = 255;
             scan_line[(j * 3) + 1] = 255;
             scan_line[(j * 3) + 2] = 255 - i;
        }
        TIFFWriteScanline(tif, scan_line, i, 0);
    }
    for (i = 255; i < 512; i++) {
        for (j = 0; j < 75; j++) {
             scan_line[j * 3] = i;
             scan_line[(j * 3) + 1] = 0;
             scan_line[(j * 3) + 2] = 0;
        }
        for (j = 75; j < 150; j++) {
             scan_line[j * 3] = 0;
             scan_line[(j * 3) + 1] = i;
             scan_line[(j * 3) + 2] = 0;
        }
        for (j = 150; j < 225; j++) {
             scan_line[j * 3] = 0;
             scan_line[(j * 3) + 1] = 0;
             scan_line[(j * 3) + 2] = i;
        }
        for (j = 225; j < 300; j++) {
             scan_line[j * 3] = (i - 1) / 2;
             scan_line[(j * 3) + 1] = (i - 1) / 2;
             scan_line[(j * 3) + 2] = (i - 1) / 2;
        }
        for (j = 300; j < 375; j++) {
             scan_line[j * 3] = 0;
             scan_line[(j * 3) + 1] = i;
             scan_line[(j * 3) + 2] = i;
        }
        for (j = 375; j < 450; j++) {
             scan_line[j * 3] = i;
             scan_line[(j * 3) + 1] = 0;
             scan_line[(j * 3) + 2] = i;
        }
        for (j = 450; j < 525; j++) {
             scan_line[j * 3] = i;
             scan_line[(j * 3) + 1] = i;
             scan_line[(j * 3) + 2] = 0;
        }
        TIFFWriteScanline(tif, scan_line, i, 0);
    }

    free(scan_line);
    TIFFClose(tif);
    exit(0);
}

void
Usage()
{
    fprintf(stderr, "Usage: %s -gamma gamma tiff-image\n", programName);
    exit(0);
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
