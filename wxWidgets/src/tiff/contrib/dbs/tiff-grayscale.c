
/*
 * tiff-grayscale.c -- create a Class G (grayscale) TIFF file
 *      with a gray response curve in linear optical density
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

#define WIDTH       512
#define HEIGHT      WIDTH

char *              programName;
void                Usage();

int main(int argc, char **argv)
{
    int             bits_per_pixel = 8, cmsize, i, j, k,
                    gray_index, chunk_size = 32, nchunks = 16;
    unsigned char * scan_line;
    uint16 *        gray;
    float           refblackwhite[2*1];
    TIFF *          tif;

    programName = argv[0];

    if (argc != 4)
        Usage();

    if (!strcmp(argv[1], "-depth"))
         bits_per_pixel = atoi(argv[2]);
    else
         Usage();

    switch (bits_per_pixel) {
        case 8:
            nchunks = 16;
            chunk_size = 32;
            break;
        case 4:
            nchunks = 4;
            chunk_size = 128;
            break;
        case 2:
            nchunks = 2;
            chunk_size = 256;
            break;
        default:
            Usage();
    }

    cmsize = nchunks * nchunks;
    gray = (uint16 *) malloc(cmsize * sizeof(uint16));

    gray[0] = 3000;
    for (i = 1; i < cmsize; i++)
        gray[i] = (uint16) (-log10((double) i / (cmsize - 1)) * 1000);

    refblackwhite[0] = 0.0;
    refblackwhite[1] = (float)((1L<<bits_per_pixel) - 1);

    if ((tif = TIFFOpen(argv[3], "w")) == NULL) {
        fprintf(stderr, "can't open %s as a TIFF file\n", argv[3]);
		free(gray);
        return 0;
    }

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, WIDTH);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, HEIGHT);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bits_per_pixel);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_REFERENCEBLACKWHITE, refblackwhite);
    TIFFSetField(tif, TIFFTAG_TRANSFERFUNCTION, gray);
    TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

    scan_line = (unsigned char *) malloc(WIDTH / (8 / bits_per_pixel));

    for (i = 0; i < HEIGHT; i++) {
        for (j = 0, k = 0; j < WIDTH;) {
            gray_index = (j / chunk_size) + ((i / chunk_size) * nchunks);

            switch (bits_per_pixel) {
            case 8:
                scan_line[k++] = gray_index;
                j++;
                break;
            case 4:
                scan_line[k++] = (gray_index << 4) + gray_index;
                j += 2;
                break;
            case 2:
                scan_line[k++] = (gray_index << 6) + (gray_index << 4)
                    + (gray_index << 2) + gray_index;
                j += 4;
                break;
            }
        }
        TIFFWriteScanline(tif, scan_line, i, 0);
    }

    free(scan_line);
    TIFFClose(tif);
    return 0;
}

void
Usage()
{
    fprintf(stderr, "Usage: %s -depth (8 | 4 | 2) tiff-image\n", programName);
    exit(0);
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
