
/*
 * tiff-palette.c -- create a Class P (palette) TIFF file
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
#include <string.h>

#include "tiffio.h"

#define WIDTH       512
#define HEIGHT      WIDTH
#define SCALE(x)    ((x) * 257L)

char *              programName;
void                Usage();

int main(int argc, char **argv)
{
    int             bits_per_pixel = 8, cmsize, i, j, k,
                    cmap_index, chunk_size = 32, nchunks = 16;
    unsigned char * scan_line;
    uint16          *red, *green, *blue;
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
	case 1:
	    nchunks = 2;
	    chunk_size = 256;
	    break;
        default:
            Usage();
    }

    if (bits_per_pixel != 1) {
	cmsize = nchunks * nchunks;
    } else {
	cmsize = 2;
    }
    red = (uint16 *) malloc(cmsize * sizeof(uint16));
    green = (uint16 *) malloc(cmsize * sizeof(uint16));
    blue = (uint16 *) malloc(cmsize * sizeof(uint16));

    switch (bits_per_pixel) {
    case 8:
        for (i = 0; i < cmsize; i++) {
            if (i < 32)
                red[i] = 0;
            else if (i < 64)
                red[i] = SCALE(36);
            else if (i < 96)
                red[i] = SCALE(73);
            else if (i < 128)
                red[i] = SCALE(109);
            else if (i < 160)
                red[i] = SCALE(146);
            else if (i < 192)
                red[i] = SCALE(182);
            else if (i < 224)
                red[i] = SCALE(219);
            else if (i < 256)
                red[i] = SCALE(255);

            if ((i % 32) < 4)
                green[i] = 0;
            else if (i < 8)
                green[i] = SCALE(36);
            else if ((i % 32) < 12)
                green[i] = SCALE(73);
            else if ((i % 32) < 16)
                green[i] = SCALE(109);
            else if ((i % 32) < 20)
                green[i] = SCALE(146);
            else if ((i % 32) < 24)
                green[i] = SCALE(182);
            else if ((i % 32) < 28)
                green[i] = SCALE(219);
            else if ((i % 32) < 32)
                green[i] = SCALE(255);

            if ((i % 4) == 0)
                blue[i] = SCALE(0);
            else if ((i % 4) == 1)
                blue[i] = SCALE(85);
            else if ((i % 4) == 2)
                blue[i] = SCALE(170);
            else if ((i % 4) == 3)
                blue[i] = SCALE(255);
        }
        break;
    case 4:
        red[0] = SCALE(255);
        green[0] = 0;
        blue[0] = 0;

        red[1] = 0;
        green[1] = SCALE(255);
        blue[1] = 0;

        red[2] = 0;
        green[2] = 0;
        blue[2] = SCALE(255);

        red[3] = SCALE(255);
        green[3] = SCALE(255);
        blue[3] = SCALE(255);

        red[4] = 0;
        green[4] = SCALE(255);
        blue[4] = SCALE(255);

        red[5] = SCALE(255);
        green[5] = 0;
        blue[5] = SCALE(255);

        red[6] = SCALE(255);
        green[6] = SCALE(255);
        blue[6] = 0;

        red[7] = 0;
        green[7] = 0;
        blue[7] = 0;

        red[8] = SCALE(176);
        green[8] = SCALE(224);
        blue[8] = SCALE(230);
        red[9] = SCALE(100);
        green[9] = SCALE(149);
        blue[9] = SCALE(237);
        red[10] = SCALE(46);
        green[10] = SCALE(139);
        blue[10] = SCALE(87);
        red[11] = SCALE(160);
        green[11] = SCALE(82);
        blue[11] = SCALE(45);
        red[12] = SCALE(238);
        green[12] = SCALE(130);
        blue[12] = SCALE(238);
        red[13] = SCALE(176);
        green[13] = SCALE(48);
        blue[13] = SCALE(96);
        red[14] = SCALE(50);
        green[14] = SCALE(205);
        blue[14] = SCALE(50);
        red[15] = SCALE(240);
        green[15] = SCALE(152);
        blue[15] = SCALE(35);
        break;
    case 2:
        red[0] = SCALE(255);
        green[0] = 0;
        blue[0] = 0;

        red[1] = 0;
        green[1] = SCALE(255);
        blue[1] = 0;

        red[2] = 0;
        green[2] = 0;
        blue[2] = SCALE(255);
        red[3] = SCALE(255);
        green[3] = SCALE(255);
        blue[3] = SCALE(255);
        break;
    case 1:
        red[0] = 0;
        green[0] = 0;
        blue[0] = 0;

        red[1] = SCALE(255);
        green[1] = SCALE(255);
        blue[1] = SCALE(255);
        break;
    }

    if ((tif = TIFFOpen(argv[3], "w")) == NULL) {
        fprintf(stderr, "can't open %s as a TIFF file\n", argv[3]);
		free(red);free(green);free(blue);
        return 0;
    }

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, WIDTH);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, HEIGHT);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bits_per_pixel);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
    TIFFSetField(tif, TIFFTAG_COLORMAP, red, green, blue);

    scan_line = (unsigned char *) malloc(WIDTH / (8 / bits_per_pixel));

    for (i = 0; i < HEIGHT; i++) {
        for (j = 0, k = 0; j < WIDTH;) {
            cmap_index = (j / chunk_size) + ((i / chunk_size) * nchunks);

            switch (bits_per_pixel) {
            case 8:
                scan_line[k++] = cmap_index;
                j++;
                break;
            case 4:
                scan_line[k++] = (cmap_index << 4) + cmap_index;
                j += 2;
                break;
            case 2:
                scan_line[k++] = (cmap_index << 6) + (cmap_index << 4)
                    + (cmap_index << 2) + cmap_index;
                j += 4;
                break;
	    case 1:
		scan_line[k++] =
			((j / chunk_size) == (i / chunk_size)) ? 0x00 : 0xff;
		j += 8;
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
    fprintf(stderr, "Usage: %s -depth (8 | 4 | 2 | 1) tiff-image\n", programName);
    exit(0);
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
