#ifndef lint
static char sccsid[] = "@(#)ras2tif.c 1.2 90/03/06";
#endif
/*-
 * ras2tif.c - Converts from a Sun Rasterfile to a Tagged Image File.
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
 * 11-Jan-89: Created.
 * 06-Mar-90: fix bug in SCALE() macro.
 *	      got rid of xres and yres, (they weren't working anyways).
 *	      fixed bpsl calculation.
 * 25-Nov-99: y2k fix (year as 1900 + tm_year) <mike@onshore.com>
 *
 * Description:
 *   This program takes a Sun Rasterfile [see rasterfile(5)] as input and
 * writes a MicroSoft/Aldus "Tagged Image File Format" image or "TIFF" file.
 * The input file may be standard input, but the output TIFF file must be a
 * real file since seek(2) is used.
 */

#include <stdio.h>
#include <sys/time.h>
#include <pixrect/pixrect_hs.h>
#include "tiffio.h"

typedef int boolean;
#define True (1)
#define False (0)
#define	SCALE(x)	(((x)*((1L<<16)-1))/255)

boolean     Verbose = False;
boolean     dummyinput = False;
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
    error("usage: %s -[vq] [-|rasterfile] TIFFfile\n", NULL);
}


main(argc, argv)
    int         argc;
    char       *argv[];
{
    char       *inf = NULL;
    char       *outf = NULL;
    FILE       *fp;
    int         depth,
                i;
    long        row;
    TIFF       *tif;
    Pixrect    *pix;		/* The Sun Pixrect */
    colormap_t  Colormap;	/* The Pixrect Colormap */
    u_short     red[256],
                green[256],
                blue[256];
    struct tm  *ct;
    struct timeval tv;
    long        width,
                height;
    long        rowsperstrip;
    int         year; 
    short       photometric;
    short       samplesperpixel;
    short       bitspersample;
    int         bpsl;
    static char *version = "ras2tif 1.0";
    static char *datetime = "1990:01:01 12:00:00";

    gettimeofday(&tv, (struct timezone *) NULL);
    ct = localtime(&tv.tv_sec);
    year=1900 + ct->tm_year; 
    sprintf(datetime, "%04d:%02d:%02d %02d:%02d:%02d",
	    year, ct->tm_mon + 1, ct->tm_mday,
	    ct->tm_hour, ct->tm_min, ct->tm_sec);

    setbuf(stderr, NULL);
    pname = argv[0];

    while (--argc) {
	if ((++argv)[0][0] == '-') {
	    switch (argv[0][1]) {
	    case 'v':
		Verbose = True;
		break;
	    case 'q':
		usage();
		break;
	    case '\0':
		if (inf == NULL)
		    dummyinput = True;
		else
		    usage();
		break;
	    default:
		fprintf(stderr, "%s: illegal option -%c.\n", pname,
			argv[0][1]);
		exit(1);
	    }
	} else if (inf == NULL && !dummyinput) {
	    inf = argv[0];
	} else if (outf == NULL)
	    outf = argv[0];
	else
	    usage();
    }

    if (outf == NULL)
	error("%s: can't write output file to a stream.\n", NULL);

    if (dummyinput || inf == NULL) {
	inf = "Standard Input";
	fp = stdin;
    } else if ((fp = fopen(inf, "r")) == NULL)
	error("%s: %s couldn't be opened.\n", inf);

    if (Verbose)
	fprintf(stderr, "Reading rasterfile from %s...", inf);

    pix = pr_load(fp, &Colormap);
    if (pix == NULL)
	error("%s: %s is not a raster file.\n", inf);

    if (Verbose)
	fprintf(stderr, "done.\n");

    if (Verbose)
	fprintf(stderr, "Writing %s...", outf);

    tif = TIFFOpen(outf, "w");

    if (tif == NULL)
	error("%s: error opening TIFF file %s", outf);

    width = pix->pr_width;
    height = pix->pr_height;
    depth = pix->pr_depth;

    switch (depth) {
    case 1:
	samplesperpixel = 1;
	bitspersample = 1;
	photometric = PHOTOMETRIC_MINISBLACK;
	break;
    case 8:
	samplesperpixel = 1;
	bitspersample = 8;
	photometric = PHOTOMETRIC_PALETTE;
	break;
    case 24:
	samplesperpixel = 3;
	bitspersample = 8;
	photometric = PHOTOMETRIC_RGB;
	break;
    case 32:
	samplesperpixel = 4;
	bitspersample = 8;
	photometric = PHOTOMETRIC_RGB;
	break;
    default:
	error("%s: bogus depth: %d\n", depth);
    }

    bpsl = ((depth * width + 15) >> 3) & ~1;
    rowsperstrip = (8 * 1024) / bpsl;

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bitspersample);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photometric);
    TIFFSetField(tif, TIFFTAG_DOCUMENTNAME, inf);
    TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, "converted Sun rasterfile");
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
    TIFFSetField(tif, TIFFTAG_STRIPBYTECOUNTS, height / rowsperstrip);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_SOFTWARE, version);
    TIFFSetField(tif, TIFFTAG_DATETIME, datetime);

    memset(red, 0, sizeof(red));
    memset(green, 0, sizeof(green));
    memset(blue, 0, sizeof(blue));
    if (depth == 8) {
	TIFFSetField(tif, TIFFTAG_COLORMAP, red, green, blue);
	for (i = 0; i < Colormap.length; i++) {
	    red[i] = SCALE(Colormap.map[0][i]);
	    green[i] = SCALE(Colormap.map[1][i]);
	    blue[i] = SCALE(Colormap.map[2][i]);
	}
    }
    if (Verbose)
	fprintf(stderr, "%dx%dx%d image, ", width, height, depth);

    for (row = 0; row < height; row++)
	if (TIFFWriteScanline(tif,
			      (u_char *) mprd_addr(mpr_d(pix), 0, row),
			      row, 0) < 0) {
	    fprintf("failed a scanline write (%d)\n", row);
	    break;
	}
    TIFFFlushData(tif);
    TIFFClose(tif);

    if (Verbose)
	fprintf(stderr, "done.\n");

    pr_destroy(pix);

    exit(0);
}
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
