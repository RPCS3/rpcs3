/* $Header: /cvs/maptools/cvsroot/libtiff/contrib/pds/tif_imageiter.c,v 1.4 2010-06-08 18:55:15 bfriesen Exp $ */

/*
 * Copyright (c) 1991-1996 Sam Leffler
 * Copyright (c) 1991-1996 Silicon Graphics, Inc.
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
 * Written by Conrad J. Poelman, PL/WSAT, Kirtland AFB, NM on 26 Mar 96.
 *
 * This file contains code to allow a calling program to "iterate" over each
 * pixels in an image as it is read from the file. The iterator takes care of
 * reading strips versus (possibly clipped) tiles, decoding the information
 * according to the decoding method, and so on, so that calling program can
 * ignore those details. The calling program does, however, need to be
 * conscious of the type of the pixel data that it is receiving.
 *
 * For reasons of efficiency, the callback function actually gets called for 
 * "blocks" of pixels rather than for individual pixels. The format of the
 * callback arguments is given below.
 *
 * This code was taken from TIFFReadRGBAImage() in tif_getimage.c of the original
 * TIFF distribution, and simplified and generalized to provide this general
 * iteration capability. Those routines could certainly be re-implemented in terms
 * of a TIFFImageIter if desired.
 *
 */
#include "tiffiop.h"
#include "tif_imageiter.h"
#include <assert.h>
#include <stdio.h>

static	int gtTileContig(TIFFImageIter*, void *udata, uint32, uint32);
static	int gtTileSeparate(TIFFImageIter*, void *udata, uint32, uint32);
static	int gtStripContig(TIFFImageIter*, void *udata, uint32, uint32);
static	int gtStripSeparate(TIFFImageIter*, void *udata, uint32, uint32);

static	const char photoTag[] = "PhotometricInterpretation";

static int
isCCITTCompression(TIFF* tif)
{
    uint16 compress;
    TIFFGetField(tif, TIFFTAG_COMPRESSION, &compress);
    return (compress == COMPRESSION_CCITTFAX3 ||
	    compress == COMPRESSION_CCITTFAX4 ||
	    compress == COMPRESSION_CCITTRLE ||
	    compress == COMPRESSION_CCITTRLEW);
}

int
TIFFImageIterBegin(TIFFImageIter* img, TIFF* tif, int stop, char emsg[1024])
{
    uint16* sampleinfo;
    uint16 extrasamples;
    uint16 planarconfig;
    int colorchannels;

    img->tif = tif;
    img->stoponerr = stop;
    TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &img->bitspersample);
    img->alpha = 0;
    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &img->samplesperpixel);
    TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES,
	&extrasamples, &sampleinfo);
    if (extrasamples == 1)
	switch (sampleinfo[0]) {
	case EXTRASAMPLE_ASSOCALPHA:	/* data is pre-multiplied */
	case EXTRASAMPLE_UNASSALPHA:	/* data is not pre-multiplied */
	    img->alpha = sampleinfo[0];
	    break;
	}
    colorchannels = img->samplesperpixel - extrasamples;
    TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planarconfig);
    if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &img->photometric)) {
	switch (colorchannels) {
	case 1:
	    if (isCCITTCompression(tif))
		img->photometric = PHOTOMETRIC_MINISWHITE;
	    else
		img->photometric = PHOTOMETRIC_MINISBLACK;
	    break;
	case 3:
	    img->photometric = PHOTOMETRIC_RGB;
	    break;
	default:
	    sprintf(emsg, "Missing needed %s tag", photoTag);
	    return (0);
	}
    }
    switch (img->photometric) {
    case PHOTOMETRIC_PALETTE:
	if (!TIFFGetField(tif, TIFFTAG_COLORMAP,
	    &img->redcmap, &img->greencmap, &img->bluecmap)) {
		TIFFErrorExt(tif->tif_clientdata, TIFFFileName(tif), "Missing required \"Colormap\" tag");
	    return (0);
	}
	/* fall thru... */
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
/* This should work now so skip the check - BSR
	if (planarconfig == PLANARCONFIG_CONTIG && img->samplesperpixel != 1) {
	    sprintf(emsg,
		"Sorry, can not handle contiguous data with %s=%d, and %s=%d",
		photoTag, img->photometric,
		"Samples/pixel", img->samplesperpixel);
	    return (0);
	}
 */
	break;
    case PHOTOMETRIC_YCBCR:
	if (planarconfig != PLANARCONFIG_CONTIG) {
	    sprintf(emsg, "Sorry, can not handle YCbCr images with %s=%d",
		"Planarconfiguration", planarconfig);
	    return (0);
	}
	/* It would probably be nice to have a reality check here. */
	{ uint16 compress;
	  TIFFGetField(tif, TIFFTAG_COMPRESSION, &compress);
	  if (compress == COMPRESSION_JPEG && planarconfig == PLANARCONFIG_CONTIG) {
	    /* can rely on libjpeg to convert to RGB */
	    /* XXX should restore current state on exit */
	    TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
	    img->photometric = PHOTOMETRIC_RGB;
	  }
	}
	break;
    case PHOTOMETRIC_RGB: 
	if (colorchannels < 3) {
	    sprintf(emsg, "Sorry, can not handle RGB image with %s=%d",
		"Color channels", colorchannels);
	    return (0);
	}
	break;
    case PHOTOMETRIC_SEPARATED: {
	uint16 inkset;
	TIFFGetFieldDefaulted(tif, TIFFTAG_INKSET, &inkset);
	if (inkset != INKSET_CMYK) {
	    sprintf(emsg, "Sorry, can not handle separated image with %s=%d",
		"InkSet", inkset);
	    return (0);
	}
	if (img->samplesperpixel != 4) {
	    sprintf(emsg, "Sorry, can not handle separated image with %s=%d",
		"Samples/pixel", img->samplesperpixel);
	    return (0);
	}
	break;
    }
    default:
	sprintf(emsg, "Sorry, can not handle image with %s=%d",
	    photoTag, img->photometric);
	return (0);
    }
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &img->width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &img->height);

    TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, &img->orientation);
    switch (img->orientation) {
    case ORIENTATION_BOTRIGHT:
    case ORIENTATION_RIGHTBOT:	/* XXX */
    case ORIENTATION_LEFTBOT:	/* XXX */
	TIFFWarning(TIFFFileName(tif), "using bottom-left orientation");
	img->orientation = ORIENTATION_BOTLEFT;
	/* fall thru... */
    case ORIENTATION_BOTLEFT:
	break;
    case ORIENTATION_TOPRIGHT:
    case ORIENTATION_RIGHTTOP:	/* XXX */
    case ORIENTATION_LEFTTOP:	/* XXX */
    default:
	TIFFWarning(TIFFFileName(tif), "using top-left orientation");
	img->orientation = ORIENTATION_TOPLEFT;
	/* fall thru... */
    case ORIENTATION_TOPLEFT:
	break;
    }

    img->isContig =
	!(planarconfig == PLANARCONFIG_SEPARATE && colorchannels > 1);
    if (img->isContig) {
	img->get = TIFFIsTiled(tif) ? gtTileContig : gtStripContig;
    } else {
	img->get = TIFFIsTiled(tif) ? gtTileSeparate : gtStripSeparate;
    }
    return (1);
}

int
TIFFImageIterGet(TIFFImageIter* img, void *udata, uint32 w, uint32 h)
{
    if (img->get == NULL) {
	TIFFErrorExt(img->tif->tif_clientdata, TIFFFileName(img->tif), "No \"get\" routine setup");
	return (0);
    }
    if (img->callback.any == NULL) {
	TIFFErrorExt(img->tif->tif_clientdata, TIFFFileName(img->tif),
		"No \"put\" routine setupl; probably can not handle image format");
	return (0);
    }
    return (*img->get)(img, udata, w, h);
}

TIFFImageIterEnd(TIFFImageIter* img)
{
    /* Nothing to free... ? */
}

/*
 * Read the specified image into an ABGR-format raster.
 */
int
TIFFReadImageIter(TIFF* tif,
    uint32 rwidth, uint32 rheight, uint8* raster, int stop)
{
    char emsg[1024];
    TIFFImageIter img;
    int ok;

    if (TIFFImageIterBegin(&img, tif, stop, emsg)) {
	/* XXX verify rwidth and rheight against width and height */
	ok = TIFFImageIterGet(&img, raster, rwidth, img.height);
	TIFFImageIterEnd(&img);
    } else {
	TIFFErrorExt(tif->tif_clientdata, TIFFFileName(tif), emsg);
	ok = 0;
    }
    return (ok);
}


/*
 * Get an tile-organized image that has
 *	PlanarConfiguration contiguous if SamplesPerPixel > 1
 * or
 *	SamplesPerPixel == 1
 */	
static int
gtTileContig(TIFFImageIter* img, void *udata, uint32 w, uint32 h)
{
    TIFF* tif = img->tif;
    ImageIterTileContigRoutine callback = img->callback.contig;
    uint16 orientation;
    uint32 col, row;
    uint32 tw, th;
    u_char* buf;
    int32 fromskew;
    uint32 nrow;

    buf = (u_char*) _TIFFmalloc(TIFFTileSize(tif));
    if (buf == 0) {
	TIFFErrorExt(tif->tif_clientdata, TIFFFileName(tif), "No space for tile buffer");
	return (0);
    }
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tw);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &th);
    orientation = img->orientation;
    for (row = 0; row < h; row += th) {
	nrow = (row + th > h ? h - row : th);
	for (col = 0; col < w; col += tw) {
	    if (TIFFReadTile(tif, buf, col, row, 0, 0) < 0 && img->stoponerr)
		break;
	    if (col + tw > w) {
		/*
		 * Tile is clipped horizontally.  Calculate
		 * visible portion and skewing factors.
		 */
		uint32 npix = w - col;
		fromskew = tw - npix;
		(*callback)(img, udata, col, row, npix, nrow, fromskew, buf);
	    } else {
		(*callback)(img, udata, col, row, tw, nrow, 0, buf);
	    }
	}
    }
    _TIFFfree(buf);
    return (1);
}

/*
 * Get an tile-organized image that has
 *	 SamplesPerPixel > 1
 *	 PlanarConfiguration separated
 * We assume that all such images are RGB.
 */	
static int
gtTileSeparate(TIFFImageIter* img, void *udata, uint32 w, uint32 h)
{
    TIFF* tif = img->tif;
    ImageIterTileSeparateRoutine callback = img->callback.separate;
    uint16 orientation;
    uint32 col, row;
    uint32 tw, th;
    u_char* buf;
    u_char* r;
    u_char* g;
    u_char* b;
    u_char* a;
    tsize_t tilesize;
    int32 fromskew;
    int alpha = img->alpha;
    uint32 nrow;

    tilesize = TIFFTileSize(tif);
    buf = (u_char*) _TIFFmalloc(4*tilesize);
    if (buf == 0) {
	TIFFErrorExt(tif->tif_clientdata, TIFFFileName(tif), "No space for tile buffer");
	return (0);
    }
    r = buf;
    g = r + tilesize;
    b = g + tilesize;
    a = b + tilesize;
    if (!alpha)
	memset(a, 0xff, tilesize);
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tw);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &th);
    orientation = img->orientation;
    for (row = 0; row < h; row += th) {
	nrow = (row + th > h ? h - row : th);
	for (col = 0; col < w; col += tw) {
	    if (TIFFReadTile(tif, r, col, row,0,0) < 0 && img->stoponerr)
		break;
	    if (TIFFReadTile(tif, g, col, row,0,1) < 0 && img->stoponerr)
		break;
	    if (TIFFReadTile(tif, b, col, row,0,2) < 0 && img->stoponerr)
		break;
	    if (alpha && TIFFReadTile(tif,a,col,row,0,3) < 0 && img->stoponerr)
		break;
	    if (col + tw > w) {
		/*
		 * Tile is clipped horizontally.  Calculate
		 * visible portion and skewing factors.
		 */
		uint32 npix = w - col;
		fromskew = tw - npix;
		(*callback)(img, udata, col, row, npix, nrow, fromskew, r, g, b, a);
	    } else {
		(*callback)(img, udata, col, row, tw, nrow, 0, r, g, b, a);
	    }
	}
    }
    _TIFFfree(buf);
    return (1);
}

/*
 * Get a strip-organized image that has
 *	PlanarConfiguration contiguous if SamplesPerPixel > 1
 * or
 *	SamplesPerPixel == 1
 */	
static int
gtStripContig(TIFFImageIter* img, void *udata, uint32 w, uint32 h)
{
    TIFF* tif = img->tif;
    ImageIterTileContigRoutine callback = img->callback.contig;
    uint16 orientation;
    uint32 row, nrow;
    u_char* buf;
    uint32 rowsperstrip;
    uint32 imagewidth = img->width;
    tsize_t scanline;
    int32 fromskew;

    buf = (u_char*) _TIFFmalloc(TIFFStripSize(tif));
    if (buf == 0) {
	TIFFErrorExt(tif->tif_clientdata, TIFFFileName(tif), "No space for strip buffer");
	return (0);
    }
    orientation = img->orientation;
    TIFFGetFieldDefaulted(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
    scanline = TIFFScanlineSize(tif);
    fromskew = (w < imagewidth ? imagewidth - w : 0);
    for (row = 0; row < h; row += rowsperstrip) {
	nrow = (row + rowsperstrip > h ? h - row : rowsperstrip);
	if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, row, 0),
	    buf, nrow*scanline) < 0 && img->stoponerr)
		break;
	(*callback)(img, udata, 0, row, w, nrow, fromskew, buf);
    }
    _TIFFfree(buf);
    return (1);
}

/*
 * Get a strip-organized image with
 *	 SamplesPerPixel > 1
 *	 PlanarConfiguration separated
 * We assume that all such images are RGB.
 */
static int
gtStripSeparate(TIFFImageIter* img, void *udata, uint32 w, uint32 h)
{
    TIFF* tif = img->tif;
    ImageIterTileSeparateRoutine callback = img->callback.separate;
    uint16 orientation;
    u_char *buf;
    u_char *r, *g, *b, *a;
    uint32 row, nrow;
    tsize_t scanline;
    uint32 rowsperstrip;
    uint32 imagewidth = img->width;
    tsize_t stripsize;
    int32 fromskew;
    int alpha = img->alpha;

    stripsize = TIFFStripSize(tif);
    r = buf = (u_char *)_TIFFmalloc(4*stripsize);
    if (buf == 0) {
	TIFFErrorExt(tif->tif_clientdata, TIFFFileName(tif), "No space for tile buffer");
	return (0);
    }
    g = r + stripsize;
    b = g + stripsize;
    a = b + stripsize;
    if (!alpha)
	memset(a, 0xff, stripsize);
    orientation = img->orientation;
    TIFFGetFieldDefaulted(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
    scanline = TIFFScanlineSize(tif);
    fromskew = (w < imagewidth ? imagewidth - w : 0);
    for (row = 0; row < h; row += rowsperstrip) {
	nrow = (row + rowsperstrip > h ? h - row : rowsperstrip);
	if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, row, 0),
	    r, nrow*scanline) < 0 && img->stoponerr)
	    break;
	if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, row, 1),
	    g, nrow*scanline) < 0 && img->stoponerr)
	    break;
	if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, row, 2),
	    b, nrow*scanline) < 0 && img->stoponerr)
	    break;
	if (alpha &&
	    (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, row, 3),
	    a, nrow*scanline) < 0 && img->stoponerr))
	    break;
	(*callback)(img, udata, 0, row, w, nrow, fromskew, r, g, b, a);
    }
    _TIFFfree(buf);
    return (1);
}

DECLAREContigCallbackFunc(TestContigCallback)
{
    printf("Contig Callback called with x = %d, y = %d, w = %d, h = %d, fromskew = %d\n",
	   x, y, w, h, fromskew);
}


DECLARESepCallbackFunc(TestSepCallback)
{
    printf("Sep Callback called with x = %d, y = %d, w = %d, h = %d, fromskew = %d\n",
	   x, y, w, h, fromskew);
}


#ifdef MAIN
main(int argc, char **argv)
{
    char emsg[1024];
    TIFFImageIter img;
    int ok;
    int stop = 1;

    TIFF *tif;
    unsigned long nx, ny;
    unsigned short BitsPerSample, SamplesPerPixel;
    int isColorMapped, isPliFile;
    unsigned char *ColorMap;
    unsigned char *data;

    if (argc < 2) {
	fprintf(stderr,"usage: %s tiff_file\n",argv[0]);
	exit(1);
    }
    tif = (TIFF *)PLIGetImage(argv[1], (void *) &data, &ColorMap, 
			      &nx, &ny, &BitsPerSample, &SamplesPerPixel, 
			      &isColorMapped, &isPliFile);
    if (tif != NULL) {

	if (TIFFImageIterBegin(&img, tif, stop, emsg)) {
	    /* Here need to set data and callback function! */
	    if (img.isContig) {
		img.callback = TestContigCallback;
	    } else {
		img.callback = TestSepCallback;
	    }
	    ok = TIFFImageIterGet(&img, NULL, img.width, img.height);
	    TIFFImageIterEnd(&img);
	} else {
	    TIFFErrorExt(tif->tif_clientdata, TIFFFileName(tif), emsg);
	}
    }
    
}
#endif
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
