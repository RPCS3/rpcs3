#include "StdAfx.h"

//#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <stdlib.h>                     // MAX_ constants
#include "diblib.h"

/*--------------------------------------------------------------------
        READ TIFF
        Load the TIFF data from the file into memory.  Return
        a pointer to a valid DIB (or NULL for errors).
        Uses the TIFFRGBA interface to libtiff.lib to convert
        most file formats to a useable form.  We just keep the 32 bit
        form of the data to display, rather than optimizing for the
        display.

        Main entry points:

            int ChkTIFF ( LPCTSTR lpszPath )
            PVOID ReadTIFF ( LPCTSTR lpszPath )

        RETURN
            A valid DIB pointer for success; NULL for failure.

  --------------------------------------------------------------------*/

#include "TiffLib/tiff.h"
#include "TiffLib/tiffio.h"
#include <assert.h>
#include <stdio.h>


// piggyback some data on top of the RGBA Image
struct TIFFDibImage {
    TIFFRGBAImage tif;
    int  dibinstalled;
} ;


HANDLE LoadTIFFinDIB(LPCTSTR lpFileName);
HANDLE TIFFRGBA2DIB(TIFFDibImage* dib, uint32* raster)  ;

static void
MyWarningHandler(const char* module, const char* fmt, va_list ap)
{
    // ignore all warnings (unused tags, etc)
    return;
}

static void
MyErrorHandler(const char* module, const char* fmt, va_list ap)
{
    return;
}

//  Turn off the error and warning handlers to check if a valid file.
//  Necessary because of the way that the Doc loads images and restart files.
int ChkTIFF ( LPCTSTR lpszPath )
{
    int rtn = 0;

    TIFFErrorHandler  eh;
    TIFFErrorHandler  wh;

    eh = TIFFSetErrorHandler(NULL);
    wh = TIFFSetWarningHandler(NULL);

    TIFF* tif = TIFFOpen(lpszPath, "r");
    if (tif) {
        rtn = 1;
        TIFFClose(tif);
    }

    TIFFSetErrorHandler(eh);
    TIFFSetWarningHandler(wh);

    return rtn;
}

void DibInstallHack(TIFFDibImage* img) ;

PVOID ReadTIFF ( LPCTSTR lpszPath )
{
    void*             pDIB = 0;
    TIFFErrorHandler  wh;

    wh = TIFFSetWarningHandler(MyWarningHandler);

    if (ChkTIFF(lpszPath)) {
        TIFF* tif = TIFFOpen(lpszPath, "r");
        if (tif) {
            char emsg[1024];

            if (TIFFRGBAImageOK(tif, emsg)) {
                TIFFDibImage img;
                char emsg[1024];

                if (TIFFRGBAImageBegin(&img.tif, tif, -1, emsg)) {
                    size_t npixels;
                    uint32* raster;

                    DibInstallHack(&img);

                    npixels = img.tif.width * img.tif.height;
                    raster = (uint32*) _TIFFmalloc(npixels * sizeof (uint32));
                    if (raster != NULL) {
                        if (TIFFRGBAImageGet(&img.tif, raster, img.tif.width, img.tif.height)) {
                            pDIB = TIFFRGBA2DIB(&img, raster);
                        }
                    }
                    _TIFFfree(raster);
                }
                TIFFRGBAImageEnd(&img.tif);
            }
            else {
                TRACE("Unable to open image(%s): %s\n", lpszPath, emsg );
            }
            TIFFClose(tif);
        }
    }

    TIFFSetWarningHandler(wh);

    return pDIB;
}



HANDLE TIFFRGBA2DIB(TIFFDibImage* dib, uint32* raster)
{
    void*   pDIB = 0;
    TIFFRGBAImage* img = &dib->tif;

    uint32 imageLength;
    uint32 imageWidth;
    uint16 BitsPerSample;
    uint16 SamplePerPixel;
    uint32 RowsPerStrip;
    uint16 PhotometricInterpretation;

    BITMAPINFOHEADER   bi;
    int                dwDIBSize ;

    TIFFGetField(img->tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
    TIFFGetField(img->tif, TIFFTAG_IMAGELENGTH, &imageLength);
    TIFFGetField(img->tif, TIFFTAG_BITSPERSAMPLE, &BitsPerSample);
    TIFFGetField(img->tif, TIFFTAG_ROWSPERSTRIP, &RowsPerStrip);
    TIFFGetField(img->tif, TIFFTAG_SAMPLESPERPIXEL, &SamplePerPixel);
    TIFFGetField(img->tif, TIFFTAG_PHOTOMETRIC, &PhotometricInterpretation);

    if ( BitsPerSample == 1 && SamplePerPixel == 1 && dib->dibinstalled ) {   // bilevel
        bi.biSize           = sizeof(BITMAPINFOHEADER);
        bi.biWidth          = imageWidth;
        bi.biHeight         = imageLength;
        bi.biPlanes         = 1;  // always
        bi.biBitCount       = 1;
        bi.biCompression    = BI_RGB;
        bi.biSizeImage      = WIDTHBYTES(bi.biWidth * bi.biBitCount) * bi.biHeight;
        bi.biXPelsPerMeter  = 0;
        bi.biYPelsPerMeter  = 0;
        bi.biClrUsed        = 0;  //  must be zero for RGB compression (none)
        bi.biClrImportant   = 0;  // always

        // Get the size of the DIB
        dwDIBSize = GetDIBSize( &bi );

        // Allocate for the BITMAPINFO structure and the color table.
        pDIB = GlobalAllocPtr( GHND, dwDIBSize );
        if (pDIB == 0) {
            return( NULL );
        }

        // Copy the header info
        *((BITMAPINFOHEADER*)pDIB) = bi;

        // Get a pointer to the color table
        RGBQUAD   *pRgbq = (RGBQUAD   *)((LPSTR)pDIB + sizeof(BITMAPINFOHEADER));

        pRgbq[0].rgbRed      = 0;
        pRgbq[0].rgbBlue     = 0;
        pRgbq[0].rgbGreen    = 0;
        pRgbq[0].rgbReserved = 0;
        pRgbq[1].rgbRed      = 255;
        pRgbq[1].rgbBlue     = 255;
        pRgbq[1].rgbGreen    = 255;
        pRgbq[1].rgbReserved = 255;

        // Pointers to the bits
        //PVOID pbiBits = (LPSTR)pRgbq + bi.biClrUsed * sizeof(RGBQUAD);
        //
        // In the BITMAPINFOHEADER documentation, it appears that
        // there should be no color table for 32 bit images, but
        // experience shows that the image is off by 3 words if it
        // is not included.  So here it is.
        PVOID pbiBits = GetDIBImagePtr((BITMAPINFOHEADER*)pDIB);  //(LPSTR)pRgbq + 3 * sizeof(RGBQUAD);

        int       sizeWords = bi.biSizeImage/4;
        RGBQUAD*  rgbDib = (RGBQUAD*)pbiBits;
        long*     rgbTif = (long*)raster;

        _TIFFmemcpy(pbiBits, raster, bi.biSizeImage);
    }

        //  For now just always default to the RGB 32 bit form.                                                       // save as 32 bit for simplicity
    else if ( true /*BitsPerSample == 8 && SamplePerPixel == 3*/ ) {   // 24 bit color

        bi.biSize           = sizeof(BITMAPINFOHEADER);
        bi.biWidth          = imageWidth;
        bi.biHeight         = imageLength;
        bi.biPlanes         = 1;  // always
        bi.biBitCount       = 32;
        bi.biCompression    = BI_RGB;
        bi.biSizeImage      = WIDTHBYTES(bi.biWidth * bi.biBitCount) * bi.biHeight;
        bi.biXPelsPerMeter  = 0;
        bi.biYPelsPerMeter  = 0;
        bi.biClrUsed        = 0;  //  must be zero for RGB compression (none)
        bi.biClrImportant   = 0;  // always

        // Get the size of the DIB
        dwDIBSize = GetDIBSize( &bi );

        // Allocate for the BITMAPINFO structure and the color table.
        pDIB = GlobalAllocPtr( GHND, dwDIBSize );
        if (pDIB == 0) {
            return( NULL );
        }

        // Copy the header info
        *((BITMAPINFOHEADER*)pDIB) = bi;

        // Get a pointer to the color table
        RGBQUAD   *pRgbq = (RGBQUAD   *)((LPSTR)pDIB + sizeof(BITMAPINFOHEADER));

        // Pointers to the bits
        //PVOID pbiBits = (LPSTR)pRgbq + bi.biClrUsed * sizeof(RGBQUAD);
        //
        // In the BITMAPINFOHEADER documentation, it appears that
        // there should be no color table for 32 bit images, but
        // experience shows that the image is off by 3 words if it
        // is not included.  So here it is.
        PVOID pbiBits = (LPSTR)pRgbq + 3 * sizeof(RGBQUAD);

        int       sizeWords = bi.biSizeImage/4;
        RGBQUAD*  rgbDib = (RGBQUAD*)pbiBits;
        long*     rgbTif = (long*)raster;

        // Swap the byte order while copying
        for ( int i = 0 ; i < sizeWords ; ++i )
        {
            rgbDib[i].rgbRed   = TIFFGetR(rgbTif[i]);
            rgbDib[i].rgbBlue  = TIFFGetB(rgbTif[i]);
            rgbDib[i].rgbGreen = TIFFGetG(rgbTif[i]);
            rgbDib[i].rgbReserved = 0;
        }
    }

    return pDIB;
}




///////////////////////////////////////////////////////////////
//
//  Hacked from tif_getimage.c in libtiff in v3.5.7
//
//
typedef unsigned char u_char;


#define DECLAREContigPutFunc(name) \
static void name(\
    TIFFRGBAImage* img, \
    uint32* cp, \
    uint32 x, uint32 y, \
    uint32 w, uint32 h, \
    int32 fromskew, int32 toskew, \
    u_char* pp \
)

#define DECLARESepPutFunc(name) \
static void name(\
    TIFFRGBAImage* img,\
    uint32* cp,\
    uint32 x, uint32 y, \
    uint32 w, uint32 h,\
    int32 fromskew, int32 toskew,\
    u_char* r, u_char* g, u_char* b, u_char* a\
)

DECLAREContigPutFunc(putContig1bitTile);
static int getStripContig1Bit(TIFFRGBAImage* img, uint32* uraster, uint32 w, uint32 h);

//typdef struct TIFFDibImage {
//    TIFFRGBAImage tif;
//    dibinstalled;
//} TIFFDibImage ;

void DibInstallHack(TIFFDibImage* dib) {
    TIFFRGBAImage* img = &dib->tif;
    dib->dibinstalled = false;
    switch (img->photometric) {
        case PHOTOMETRIC_MINISWHITE:
        case PHOTOMETRIC_MINISBLACK:
        switch (img->bitspersample) {
            case 1:
                img->put.contig = putContig1bitTile;
                img->get = getStripContig1Bit;
                dib->dibinstalled = true;
                break;
        }
        break;
    }
}

/*
 * 1-bit packed samples => 1-bit
 *
 *   Override to just copy the data
 */
DECLAREContigPutFunc(putContig1bitTile)
{
    int samplesperpixel = img->samplesperpixel;

    (void) y;
    fromskew *= samplesperpixel;
    int wb = WIDTHBYTES(w);
    u_char*  ucp = (u_char*)cp;

    /* Conver 'w' to bytes from pixels (rounded up) */
    w = (w+7)/8;

    while (h-- > 0) {
        _TIFFmemcpy(ucp, pp, w);
        /*
        for (x = wb; x-- > 0;) {
            *cp++ = rgbi(Map[pp[0]], Map[pp[1]], Map[pp[2]]);
            pp += samplesperpixel;
        }
        */
        ucp += (wb + toskew);
        pp += (w + fromskew);
    }
}

/*
 *  Hacked from the tif_getimage.c file.
 */
static uint32
setorientation(TIFFRGBAImage* img, uint32 h)
{
    TIFF* tif = img->tif;
    uint32 y;

    switch (img->orientation) {
    case ORIENTATION_BOTRIGHT:
    case ORIENTATION_RIGHTBOT:  /* XXX */
    case ORIENTATION_LEFTBOT:   /* XXX */
    TIFFWarning(TIFFFileName(tif), "using bottom-left orientation");
    img->orientation = ORIENTATION_BOTLEFT;
    /* fall thru... */
    case ORIENTATION_BOTLEFT:
    y = 0;
    break;
    case ORIENTATION_TOPRIGHT:
    case ORIENTATION_RIGHTTOP:  /* XXX */
    case ORIENTATION_LEFTTOP:   /* XXX */
    default:
    TIFFWarning(TIFFFileName(tif), "using top-left orientation");
    img->orientation = ORIENTATION_TOPLEFT;
    /* fall thru... */
    case ORIENTATION_TOPLEFT:
    y = h-1;
    break;
    }
    return (y);
}

/*
 * Get a strip-organized image that has
 *  PlanarConfiguration contiguous if SamplesPerPixel > 1
 * or
 *  SamplesPerPixel == 1
 *
 *  Hacked from the tif_getimage.c file.
 *
 *    This is set up to allow us to just copy the data to the raster
 *    for 1-bit bitmaps
 */
static int
getStripContig1Bit(TIFFRGBAImage* img, uint32* raster, uint32 w, uint32 h)
{
    TIFF* tif = img->tif;
    tileContigRoutine put = img->put.contig;
    uint16 orientation;
    uint32 row, y, nrow, rowstoread;
    uint32 pos;
    u_char* buf;
    uint32 rowsperstrip;
    uint32 imagewidth = img->width;
    tsize_t scanline;
    int32 fromskew, toskew;
    tstrip_t strip;
    tsize_t  stripsize;
    u_char* braster = (u_char*)raster; // byte wide raster
    uint32  wb = WIDTHBYTES(w);
    int ret = 1;

    buf = (u_char*) _TIFFmalloc(TIFFStripSize(tif));
    if (buf == 0) {
        TIFFErrorExt(tif->tif_clientdata, TIFFFileName(tif), "No space for strip buffer");
        return (0);
    }
    y = setorientation(img, h);
    orientation = img->orientation;
    toskew = -(int32) (orientation == ORIENTATION_TOPLEFT ? wb+wb : wb-wb);
    TIFFGetFieldDefaulted(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
    scanline = TIFFScanlineSize(tif);
    fromskew = (w < imagewidth ? imagewidth - w : 0)/8;
    for (row = 0; row < h; row += nrow)
    {
        rowstoread = rowsperstrip - (row + img->row_offset) % rowsperstrip;
        nrow = (row + rowstoread > h ? h - row : rowstoread);
        strip = TIFFComputeStrip(tif,row+img->row_offset, 0);
        stripsize = ((row + img->row_offset)%rowsperstrip + nrow) * scanline;
        if (TIFFReadEncodedStrip(tif, strip, buf, stripsize ) < 0
            && img->stoponerr)
        {
            ret = 0;
            break;
        }

        pos = ((row + img->row_offset) % rowsperstrip) * scanline;
        (*put)(img, (uint32*)(braster+y*wb), 0, y, w, nrow, fromskew, toskew, buf + pos);
        y += (orientation == ORIENTATION_TOPLEFT ?-(int32) nrow : (int32) nrow);
    }
    _TIFFfree(buf);
    return (ret);
}

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
