#ifndef lint
#endif
/*-
 * tif2ras.c - Converts from a Tagged Image File Format image to a Sun Raster.
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Comments and additions should be sent to the author:
 *
 *                     Patrick J. Naughton
 *                     Sun Microsystems
 *                     2550 Garcia Ave, MS 14-40
 *                     Mountain View, CA 94043
 *                     (415) 336-1080
 *
 * Revision History:
 * 10-Jan-89: Created.
 * 06-Mar-90: Change to byte encoded rasterfiles.
 *	      fix bug in call to ReadScanline().
 *	      fix bug in CVT() macro.
 *	      fix assignment of td, (missing &).
 *
 * Description:
 *   This program takes a MicroSoft/Aldus "Tagged Image File Format" image or
 * "TIFF" file as input and writes a Sun Rasterfile [see rasterfile(5)].  The
 * output file may be standard output, but the input TIFF file must be a real
 * file since seek(2) is used.
 */

#include <stdio.h>
#include <pixrect/pixrect_hs.h>
#include "tiffio.h"

typedef int boolean;
#define True (1)
#define False (0)
#define	CVT(x)		(((x) * 255) / ((1L<<16)-1))

boolean     Verbose = False;
char       *pname;		/* program name (used for error messages) */

void
error(s1, s2)
    char       *s1,
               *s2;
{
    fprintf(stderr, s1, pname, s2);
    exit(1);
}

void
usage()
{
    error("usage: %s -[vq] TIFFfile [rasterfile]\n", NULL);
}


main(argc, argv)
    int         argc;
    char       *argv[];
{
    char       *inf = NULL;
    char       *outf = NULL;
    FILE       *fp;
    long        width,
                height;
    int         depth,
                numcolors;
    register TIFF *tif;
    TIFFDirectory *td;
    register u_char *inp,
               *outp;
    register int col,
                i;
    register long row;
    u_char     *Map = NULL;
    u_char     *buf;
    short	bitspersample;
    short	samplesperpixel;
    short	photometric;
    u_short    *redcolormap,
	       *bluecolormap,
	       *greencolormap;

    Pixrect    *pix;		/* The Sun Pixrect */
    colormap_t  Colormap;	/* The Pixrect Colormap */
    u_char      red[256],
                green[256],
                blue[256];

    setbuf(stderr, NULL);
    pname = argv[0];

    while (--argc) {
	if ((++argv)[0][0] == '-')
	    switch (argv[0][1]) {
	    case 'v':
		Verbose = True;
		break;
	    case 'q':
		usage();
		break;
	    default:
		fprintf(stderr, "%s: illegal option -%c.\n", pname,
			argv[0][1]);
		exit(1);
	    }
	else if (inf == NULL)
	    inf = argv[0];
	else if (outf == NULL)
	    outf = argv[0];
	else
	    usage();

    }

    if (inf == NULL)
	error("%s: can't read input file from a stream.\n", NULL);

    if (Verbose)
	fprintf(stderr, "Reading %s...", inf);

    tif = TIFFOpen(inf, "r");

    if (tif == NULL)
	error("%s: error opening TIFF file %s", inf);

    if (Verbose)
	TIFFPrintDirectory(tif, stderr, True, False, False);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
    if (bitspersample > 8)
	error("%s: can't handle more than 8-bits per sample\n", NULL);

    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
    switch (samplesperpixel) {
    case 1:
	if (bitspersample == 1)
	    depth = 1;
	else
	    depth = 8;
	break;
    case 3:
    case 4:
	depth = 24;
	break;
    default:
	error("%s: only handle 1-channel gray scale or 3-channel color\n");
    }

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

    if (Verbose)
	fprintf(stderr, "%dx%dx%d image, ", width, height, depth);
    if (Verbose)
	fprintf(stderr, "%d bits/sample, %d samples/pixel, ",
		bitspersample, samplesperpixel);

    pix = mem_create(width, height, depth);
    if (pix == (Pixrect *) NULL)
	error("%s: can't allocate memory for output pixrect...\n", NULL);

    numcolors = (1 << bitspersample);

    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
    if (numcolors == 2) {
	if (Verbose)
	    fprintf(stderr, "monochrome ");
	Colormap.type = RMT_NONE;
	Colormap.length = 0;
	Colormap.map[0] = Colormap.map[1] = Colormap.map[2] = NULL;
    } else {
	switch (photometric) {
	case PHOTOMETRIC_MINISBLACK:
	    if (Verbose)
		fprintf(stderr, "%d graylevels (min=black), ", numcolors);
	    Map = (u_char *) malloc(numcolors * sizeof(u_char));
	    for (i = 0; i < numcolors; i++)
		Map[i] = (255 * i) / numcolors;
	    Colormap.type = RMT_EQUAL_RGB;
	    Colormap.length = numcolors;
	    Colormap.map[0] = Colormap.map[1] = Colormap.map[2] = Map;
	    break;
	case PHOTOMETRIC_MINISWHITE:
	    if (Verbose)
		fprintf(stderr, "%d graylevels (min=white), ", numcolors);
	    Map = (u_char *) malloc(numcolors * sizeof(u_char));
	    for (i = 0; i < numcolors; i++)
		Map[i] = 255 - ((255 * i) / numcolors);
	    Colormap.type = RMT_EQUAL_RGB;
	    Colormap.length = numcolors;
	    Colormap.map[0] = Colormap.map[1] = Colormap.map[2] = Map;
	    break;
	case PHOTOMETRIC_RGB:
	    if (Verbose)
		fprintf(stderr, "truecolor ");
	    Colormap.type = RMT_NONE;
	    Colormap.length = 0;
	    Colormap.map[0] = Colormap.map[1] = Colormap.map[2] = NULL;
	    break;
	case PHOTOMETRIC_PALETTE:
	    if (Verbose)
		fprintf(stderr, "colormapped ");
	    Colormap.type = RMT_EQUAL_RGB;
	    Colormap.length = numcolors;
	    memset(red, 0, sizeof(red));
	    memset(green, 0, sizeof(green));
	    memset(blue, 0, sizeof(blue));
	    TIFFGetField(tif, TIFFTAG_COLORMAP,
		&redcolormap, &greencolormap, &bluecolormap);
	    for (i = 0; i < numcolors; i++) {
		red[i] = (u_char) CVT(redcolormap[i]);
		green[i] = (u_char) CVT(greencolormap[i]);
		blue[i] = (u_char) CVT(bluecolormap[i]);
	    }
	    Colormap.map[0] = red;
	    Colormap.map[1] = green;
	    Colormap.map[2] = blue;
	    break;
	case PHOTOMETRIC_MASK:
	    error("%s: Don't know how to handle PHOTOMETRIC_MASK\n");
	    break;
	case PHOTOMETRIC_DEPTH:
	    error("%s: Don't know how to handle PHOTOMETRIC_DEPTH\n");
	    break;
	default:
	    error("%s: unknown photometric (cmap): %d\n", photometric);
	}
    }

    buf = (u_char *) malloc(TIFFScanlineSize(tif));
    if (buf == NULL)
	error("%s: can't allocate memory for scanline buffer...\n", NULL);

    for (row = 0; row < height; row++) {
	if (TIFFReadScanline(tif, buf, row, 0) < 0)
	    error("%s: bad data read on line: %d\n", row);
	inp = buf;
	outp = (u_char *) mprd_addr(mpr_d(pix), 0, row);
	switch (photometric) {
	case PHOTOMETRIC_RGB:
	    if (samplesperpixel == 4)
		for (col = 0; col < width; col++) {
		    *outp++ = *inp++;	/* Blue */
		    *outp++ = *inp++;	/* Green */
		    *outp++ = *inp++;	/* Red */
		    inp++;	/* skip alpha channel */
		}
	    else
		for (col = 0; col < width; col++) {
		    *outp++ = *inp++;	/* Blue */
		    *outp++ = *inp++;	/* Green */
		    *outp++ = *inp++;	/* Red */
		}
	    break;
	case PHOTOMETRIC_MINISWHITE:
	case PHOTOMETRIC_MINISBLACK:
	    switch (bitspersample) {
	    case 1:
		for (col = 0; col < ((width + 7) / 8); col++)
		    *outp++ = *inp++;
		break;
	    case 2:
		for (col = 0; col < ((width + 3) / 4); col++) {
		    *outp++ = (*inp >> 6) & 3;
		    *outp++ = (*inp >> 4) & 3;
		    *outp++ = (*inp >> 2) & 3;
		    *outp++ = *inp++ & 3;
		}
		break;
	    case 4:
		for (col = 0; col < width / 2; col++) {
		    *outp++ = *inp >> 4;
		    *outp++ = *inp++ & 0xf;
		}
		break;
	    case 8:
		for (col = 0; col < width; col++)
		    *outp++ = *inp++;
		break;
	    default:
		error("%s: bad bits/sample: %d\n", bitspersample);
	    }
	    break;
	case PHOTOMETRIC_PALETTE:
	    memcpy(outp, inp, width);
	    break;
	default:
	    error("%s: unknown photometric (write): %d\n", photometric);
	}
    }

    free((char *) buf);

    if (Verbose)
	fprintf(stderr, "done.\n");

    if (outf == NULL || strcmp(outf, "Standard Output") == 0) {
	outf = "Standard Output";
	fp = stdout;
    } else {
	if (!(fp = fopen(outf, "w")))
	    error("%s: %s couldn't be opened for writing.\n", outf);
    }

    if (Verbose)
	fprintf(stderr, "Writing rasterfile in %s...", outf);

    if (pr_dump(pix, fp, &Colormap, RT_BYTE_ENCODED, 0) == PIX_ERR)
	error("%s: error writing Sun Rasterfile: %s\n", outf);

    if (Verbose)
	fprintf(stderr, "done.\n");

    pr_destroy(pix);

    if (fp != stdout)
	fclose(fp);

    exit(0);
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
