/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/imagepng.cpp
// Purpose:     wxImage PNG handler
// Author:      Robert Roebling
// RCS-ID:      $Id: imagpng.cpp 67009 2011-02-24 08:42:57Z JS $
// Copyright:   (c) Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_IMAGE && wxUSE_LIBPNG

#include "wx/imagpng.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/bitmap.h"
    #include "wx/module.h"
#endif

#include "png.h"
#include "wx/filefn.h"
#include "wx/wfstream.h"
#include "wx/intl.h"
#include "wx/palette.h"

// For memcpy
#include <string.h>

#ifdef __SALFORDC__
#ifdef FAR
#undef FAR
#endif
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// image can not have any transparent pixels at all, have only 100% opaque
// and/or 100% transparent pixels in which case a simple mask is enough to
// store this information in wxImage or have a real alpha channel in which case
// we need to have it in wxImage as well
enum Transparency
{
    Transparency_None,
    Transparency_Mask,
    Transparency_Alpha
};

// ----------------------------------------------------------------------------
// local functions
// ----------------------------------------------------------------------------

// return the kind of transparency needed for this image assuming that it does
// have transparent pixels, i.e. either Transparency_Alpha or Transparency_Mask
static Transparency
CheckTransparency(unsigned char **lines,
                  png_uint_32 x, png_uint_32 y, png_uint_32 w, png_uint_32 h,
                  size_t numColBytes);

// init the alpha channel for the image and fill it with 1s up to (x, y)
static unsigned char *InitAlpha(wxImage *image, png_uint_32 x, png_uint_32 y);

// find a free colour for the mask in the PNG data array
static void
FindMaskColour(unsigned char **lines, png_uint_32 width, png_uint_32 height,
               unsigned char& rMask, unsigned char& gMask, unsigned char& bMask);

// is the pixel with this value of alpha a fully opaque one?
static inline
bool IsOpaque(unsigned char a)
{
    return a == 0xff;
}

// is the pixel with this value of alpha a fully transparent one?
static inline
bool IsTransparent(unsigned char a)
{
    return !a;
}

// ============================================================================
// wxPNGHandler implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxPNGHandler,wxImageHandler)

#if wxUSE_STREAMS

#ifndef PNGLINKAGEMODE
    #ifdef PNGAPI
        #define PNGLINKAGEMODE PNGAPI
    #elif defined(__WATCOMC__)
        // we need an explicit cdecl for Watcom, at least according to
        //
        // http://sf.net/tracker/index.php?func=detail&aid=651492&group_id=9863&atid=109863
        //
        // more testing is needed for this however, please remove this comment
        // if you can confirm that my fix works with Watcom 11
        #define PNGLINKAGEMODE cdecl
    #else
        #define PNGLINKAGEMODE LINKAGEMODE
    #endif
#endif


// VS: wxPNGInfoStruct declared below is a hack that needs some explanation.
//     First, let me describe what's the problem: libpng uses jmp_buf in
//     its png_struct structure. Unfortunately, this structure is
//     compiler-specific and may vary in size, so if you use libpng compiled
//     as DLL with another compiler than the main executable, it may not work
//     (this is for example the case with wxMGL port and SciTech MGL library
//     that provides custom runtime-loadable libpng implementation with jmpbuf
//     disabled altogether). Luckily, it is still possible to use setjmp() &
//     longjmp() as long as the structure is not part of png_struct.
//
//     Sadly, there's no clean way to attach user-defined data to png_struct.
//     There is only one customizable place, png_struct.io_ptr, which is meant
//     only for I/O routines and is set with png_set_read_fn or
//     png_set_write_fn. The hacky part is that we use io_ptr to store
//     a pointer to wxPNGInfoStruct that holds I/O structures _and_ jmp_buf.

struct wxPNGInfoStruct
{
    jmp_buf jmpbuf;
    bool verbose;

    union
    {
        wxInputStream  *in;
        wxOutputStream *out;
    } stream;
};

#define WX_PNG_INFO(png_ptr) ((wxPNGInfoStruct*)png_get_io_ptr(png_ptr))

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

extern "C"
{

void PNGLINKAGEMODE wx_PNG_stream_reader( png_structp png_ptr, png_bytep data,
                                          png_size_t length )
{
    WX_PNG_INFO(png_ptr)->stream.in->Read(data, length);
}

void PNGLINKAGEMODE wx_PNG_stream_writer( png_structp png_ptr, png_bytep data,
                                          png_size_t length )
{
    WX_PNG_INFO(png_ptr)->stream.out->Write(data, length);
}

void
PNGLINKAGEMODE wx_png_warning(png_structp png_ptr, png_const_charp message)
{
    wxPNGInfoStruct *info = png_ptr ? WX_PNG_INFO(png_ptr) : NULL;
    if ( !info || info->verbose )
        wxLogWarning( wxString::FromAscii(message) );
}

// from pngerror.c
// so that the libpng doesn't send anything on stderr
void
PNGLINKAGEMODE wx_png_error(png_structp png_ptr, png_const_charp message)
{
    wx_png_warning(NULL, message);

    // we're not using libpng built-in jump buffer (see comment before
    // wxPNGInfoStruct above) so we have to return ourselves, otherwise libpng
    // would just abort
    longjmp(WX_PNG_INFO(png_ptr)->jmpbuf, 1);
}

} // extern "C"

// ----------------------------------------------------------------------------
// LoadFile() helpers
// ----------------------------------------------------------------------------

// determine the kind of transparency we need for this image: if the only alpha
// values it has are 0 (transparent) and 0xff (opaque) then we can simply
// create a mask for it, we should be ok with a simple mask but otherwise we
// need a full blown alpha channel in wxImage
//
// parameters:
//      lines           raw PNG data
//      x, y            starting position
//      w, h            size of the image
//      numColBytes     number of colour bytes (1 for grey scale, 3 for RGB)
//                      (NB: alpha always follows the colour bytes)
Transparency
CheckTransparency(unsigned char **lines,
                  png_uint_32 x, png_uint_32 y, png_uint_32 w, png_uint_32 h,
                  size_t numColBytes)
{
    // suppose that a mask will suffice and check all the remaining alpha
    // values to see if it does
    for ( ; y < h; y++ )
    {
        // each pixel is numColBytes+1 bytes, offset into the current line by
        // the current x position
        unsigned const char *ptr = lines[y] + (x * (numColBytes + 1));

        for ( png_uint_32 x2 = x; x2 < w; x2++ )
        {
            // skip the grey or colour byte(s)
            ptr += numColBytes;

            unsigned char a2 = *ptr++;

            if ( !IsTransparent(a2) && !IsOpaque(a2) )
            {
                // not fully opaque nor fully transparent, hence need alpha
                return Transparency_Alpha;
            }
        }

        // during the next loop iteration check all the pixels in the row
        x = 0;
    }

    // mask will be enough
    return Transparency_Mask;
}

unsigned char *InitAlpha(wxImage *image, png_uint_32 x, png_uint_32 y)
{
    // create alpha channel
    image->SetAlpha();

    unsigned char *alpha = image->GetAlpha();

    // set alpha for the pixels we had so far
    png_uint_32 end = y * image->GetWidth() + x;
    for ( png_uint_32 i = 0; i < end; i++ )
    {
        // all the previous pixels were opaque
        *alpha++ = 0xff;
    }

    return alpha;
}

void
FindMaskColour(unsigned char **lines, png_uint_32 width, png_uint_32 height,
               unsigned char& rMask, unsigned char& gMask, unsigned char& bMask)
{
    // choosing the colour for the mask is more
    // difficult: we need to iterate over the entire
    // image for this in order to choose an unused
    // colour (this is not very efficient but what else
    // can we do?)
    wxImageHistogram h;
    unsigned nentries = 0;
    unsigned char r2, g2, b2;
    for ( png_uint_32 y2 = 0; y2 < height; y2++ )
    {
        const unsigned char *p = lines[y2];
        for ( png_uint_32 x2 = 0; x2 < width; x2++ )
        {
            r2 = *p++;
            g2 = *p++;
            b2 = *p++;
            ++p; // jump over alpha

            wxImageHistogramEntry&
                entry = h[wxImageHistogram:: MakeKey(r2, g2, b2)];

            if ( entry.value++ == 0 )
                entry.index = nentries++;
        }
    }

    if ( !h.FindFirstUnusedColour(&rMask, &gMask, &bMask) )
    {
        wxLogWarning(_("Too many colours in PNG, the image may be slightly blurred."));

        // use a fixed mask colour and we'll fudge
        // the real pixels with this colour (see
        // below)
        rMask = 0xfe;
        gMask = 0;
        bMask = 0xff;
    }
}

// ----------------------------------------------------------------------------
// reading PNGs
// ----------------------------------------------------------------------------

bool wxPNGHandler::DoCanRead( wxInputStream& stream )
{
    unsigned char hdr[4];

    if ( !stream.Read(hdr, WXSIZEOF(hdr)) )
        return false;

    return memcmp(hdr, "\211PNG", WXSIZEOF(hdr)) == 0;
}

// convert data from RGB to wxImage format
static
void CopyDataFromPNG(wxImage *image,
                     unsigned char **lines,
                     png_uint_32 width,
                     png_uint_32 height,
                     int color_type)
{
    Transparency transparency = Transparency_None;

    // only non NULL if transparency == Transparency_Alpha
    unsigned char *alpha = NULL;

    // RGB of the mask colour if transparency == Transparency_Mask
    // (but init them anyhow to avoid compiler warnings)
    unsigned char rMask = 0,
                  gMask = 0,
                  bMask = 0;

    unsigned char *ptrDst = image->GetData();
    if ( !(color_type & PNG_COLOR_MASK_COLOR) )
    {
        // grey image: GAGAGA... where G == grey component and A == alpha
        for ( png_uint_32 y = 0; y < height; y++ )
        {
            const unsigned char *ptrSrc = lines[y];
            for ( png_uint_32 x = 0; x < width; x++ )
            {
                unsigned char g = *ptrSrc++;
                unsigned char a = *ptrSrc++;

                // the first time we encounter a transparent pixel we must
                // decide about what to do about them
                if ( !IsOpaque(a) && transparency == Transparency_None )
                {
                    // we'll need at least the mask for this image and
                    // maybe even full alpha channel info: the former is
                    // only enough if we have alpha values of 0 and 0xff
                    // only, otherwisewe need the latter
                    transparency = CheckTransparency
                                   (
                                        lines,
                                        x, y,
                                        width, height,
                                        1
                                   );

                    if ( transparency == Transparency_Mask )
                    {
                        // let's choose this colour for the mask: this is
                        // not a problem here as all the other pixels are
                        // grey, i.e. R == G == B which is not the case for
                        // this one so no confusion is possible
                        rMask = 0xff;
                        gMask = 0;
                        bMask = 0xff;
                    }
                    else // transparency == Transparency_Alpha
                    {
                        alpha = InitAlpha(image, x, y);
                    }
                }

                switch ( transparency )
                {
                    case Transparency_Mask:
                        if ( IsTransparent(a) )
                        {
                            *ptrDst++ = rMask;
                            *ptrDst++ = gMask;
                            *ptrDst++ = bMask;
                            break;
                        }
                        // else: !transparent

                        // must be opaque then as otherwise we shouldn't be
                        // using the mask at all
                        wxASSERT_MSG( IsOpaque(a), _T("logic error") );

                        // fall through

                    case Transparency_Alpha:
                        if ( alpha )
                            *alpha++ = a;
                        // fall through

                    case Transparency_None:
                        *ptrDst++ = g;
                        *ptrDst++ = g;
                        *ptrDst++ = g;
                        break;
                }
            }
        }
    }
    else // colour image: RGBRGB...
    {
        for ( png_uint_32 y = 0; y < height; y++ )
        {
            const unsigned char *ptrSrc = lines[y];
            for ( png_uint_32 x = 0; x < width; x++ )
            {
                unsigned char r = *ptrSrc++;
                unsigned char g = *ptrSrc++;
                unsigned char b = *ptrSrc++;
                unsigned char a = *ptrSrc++;

                // the logic here is the same as for the grey case except
                // where noted
                if ( !IsOpaque(a) && transparency == Transparency_None )
                {
                    transparency = CheckTransparency
                                   (
                                        lines,
                                        x, y,
                                        width, height,
                                        3
                                   );

                    if ( transparency == Transparency_Mask )
                    {
                        FindMaskColour(lines, width, height,
                                       rMask, gMask, bMask);
                    }
                    else // transparency == Transparency_Alpha
                    {
                        alpha = InitAlpha(image, x, y);
                    }

                }

                switch ( transparency )
                {
                    case Transparency_Mask:
                        if ( IsTransparent(a) )
                        {
                            *ptrDst++ = rMask;
                            *ptrDst++ = gMask;
                            *ptrDst++ = bMask;
                            break;
                        }
                        else // !transparent
                        {
                            // must be opaque then as otherwise we shouldn't be
                            // using the mask at all
                            wxASSERT_MSG( IsOpaque(a), _T("logic error") );

                            // if we couldn't find a unique colour for the
                            // mask, we can have real pixels with the same
                            // value as the mask and it's better to slightly
                            // change their colour than to make them
                            // transparent
                            if ( r == rMask && g == gMask && b == bMask )
                            {
                                r++;
                            }
                        }

                        // fall through

                    case Transparency_Alpha:
                        if ( alpha )
                            *alpha++ = a;
                        // fall through

                    case Transparency_None:
                        *ptrDst++ = r;
                        *ptrDst++ = g;
                        *ptrDst++ = b;
                        break;
                }
            }
        }
    }

    if ( transparency == Transparency_Mask )
    {
        image->SetMaskColour(rMask, gMask, bMask);
    }
}

// temporarily disable the warning C4611 (interaction between '_setjmp' and
// C++ object destruction is non-portable) - I don't see any dtors here
#ifdef __VISUALC__
    #pragma warning(disable:4611)
#endif /* VC++ */

bool
wxPNGHandler::LoadFile(wxImage *image,
                       wxInputStream& stream,
                       bool verbose,
                       int WXUNUSED(index))
{
    // VZ: as this function uses setjmp() the only fool-proof error handling
    //     method is to use goto (setjmp is not really C++ dtors friendly...)

    unsigned char **lines = NULL;
    png_infop info_ptr = (png_infop) NULL;
    wxPNGInfoStruct wxinfo;

    png_uint_32 i, width, height = 0;
    int bit_depth, color_type, interlace_type;

    wxinfo.verbose = verbose;
    wxinfo.stream.in = &stream;

    image->Destroy();

    png_structp png_ptr = png_create_read_struct
                          (
                            PNG_LIBPNG_VER_STRING,
                            NULL,
                            wx_png_error,
                            wx_png_warning
                          );
    if (!png_ptr)
        goto error;

    // NB: please see the comment near wxPNGInfoStruct declaration for
    //     explanation why this line is mandatory
    png_set_read_fn( png_ptr, &wxinfo, wx_PNG_stream_reader);

    info_ptr = png_create_info_struct( png_ptr );
    if (!info_ptr)
        goto error;

    if (setjmp(wxinfo.jmpbuf))
        goto error;

    png_read_info( png_ptr, info_ptr );
    png_get_IHDR( png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, (int*) NULL, (int*) NULL );

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand( png_ptr );

    // Fix for Bug [ 439207 ] Monochrome PNG images come up black
    if (bit_depth < 8)
        png_set_expand( png_ptr );

    png_set_strip_16( png_ptr );
    png_set_packing( png_ptr );
    if (png_get_valid( png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_expand( png_ptr );
    png_set_filler( png_ptr, 0xff, PNG_FILLER_AFTER );

    image->Create((int)width, (int)height, (bool) false /* no need to init pixels */);

    if (!image->Ok())
        goto error;

    // initialize all line pointers to NULL to ensure that they can be safely
    // free()d if an error occurs before all of them could be allocated
    lines = (unsigned char **)calloc(height, sizeof(unsigned char *));
    if ( !lines )
        goto error;

    for (i = 0; i < height; i++)
    {
        if ((lines[i] = (unsigned char *)malloc( (size_t)(width * (sizeof(unsigned char) * 4)))) == NULL)
            goto error;
    }

    png_read_image( png_ptr, lines );
    png_read_end( png_ptr, info_ptr );

#if wxUSE_PALETTE
    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        int ncolors = 0;
        png_colorp palette;
        png_get_PLTE( png_ptr, info_ptr, &palette, &ncolors);
        unsigned char* r = new unsigned char[ncolors];
        unsigned char* g = new unsigned char[ncolors];
        unsigned char* b = new unsigned char[ncolors];
        int j;

        for (j = 0; j < ncolors; j++)
        {
            r[j] = palette[j].red;
            g[j] = palette[j].green;
            b[j] = palette[j].blue;
        }

        image->SetPalette(wxPalette(ncolors, r, g, b));
        delete[] r;
        delete[] g;
        delete[] b;
    }
#endif // wxUSE_PALETTE

    png_destroy_read_struct( &png_ptr, &info_ptr, (png_infopp) NULL );

    // loaded successfully, now init wxImage with this data
    CopyDataFromPNG(image, lines, width, height, color_type);

    for ( i = 0; i < height; i++ )
        free( lines[i] );
    free( lines );

    return true;

error:
    if (verbose)
       wxLogError(_("Couldn't load a PNG image - file is corrupted or not enough memory."));

    if ( image->Ok() )
    {
        image->Destroy();
    }

    if ( lines )
    {
        for ( unsigned int n = 0; n < height; n++ )
            free( lines[n] );

        free( lines );
    }

    if ( png_ptr )
    {
        if ( info_ptr )
        {
            png_destroy_read_struct( &png_ptr, &info_ptr, (png_infopp) NULL );
            free(info_ptr);
        }
        else
            png_destroy_read_struct( &png_ptr, (png_infopp) NULL, (png_infopp) NULL );
    }
    return false;
}

// ----------------------------------------------------------------------------
// writing PNGs
// ----------------------------------------------------------------------------

bool wxPNGHandler::SaveFile( wxImage *image, wxOutputStream& stream, bool verbose )
{
    wxPNGInfoStruct wxinfo;

    wxinfo.verbose = verbose;
    wxinfo.stream.out = &stream;

    png_structp png_ptr = png_create_write_struct
                          (
                            PNG_LIBPNG_VER_STRING,
                            NULL,
                            wx_png_error,
                            wx_png_warning
                          );
    if (!png_ptr)
    {
        if (verbose)
           wxLogError(_("Couldn't save PNG image."));
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        png_destroy_write_struct( &png_ptr, (png_infopp)NULL );
        if (verbose)
           wxLogError(_("Couldn't save PNG image."));
        return false;
    }

    if (setjmp(wxinfo.jmpbuf))
    {
        png_destroy_write_struct( &png_ptr, (png_infopp)NULL );
        if (verbose)
           wxLogError(_("Couldn't save PNG image."));
        return false;
    }

    // NB: please see the comment near wxPNGInfoStruct declaration for
    //     explanation why this line is mandatory
    png_set_write_fn( png_ptr, &wxinfo, wx_PNG_stream_writer, NULL);

    const int iColorType = image->HasOption(wxIMAGE_OPTION_PNG_FORMAT)
                            ? image->GetOptionInt(wxIMAGE_OPTION_PNG_FORMAT)
                            : wxPNG_TYPE_COLOUR;
    const int iBitDepth = image->HasOption(wxIMAGE_OPTION_PNG_BITDEPTH)
                            ? image->GetOptionInt(wxIMAGE_OPTION_PNG_BITDEPTH)
                            : 8;

    wxASSERT_MSG( iBitDepth == 8 || iBitDepth == 16,
                    _T("PNG bit depth must be 8 or 16") );

    bool bHasAlpha = image->HasAlpha();
    bool bHasMask = image->HasMask();
    bool bUseAlpha = bHasAlpha || bHasMask;

    int iPngColorType;
    if ( iColorType==wxPNG_TYPE_COLOUR )
    {
        iPngColorType = bUseAlpha ? PNG_COLOR_TYPE_RGB_ALPHA
                                  : PNG_COLOR_TYPE_RGB;
    }
    else
    {
        iPngColorType = bUseAlpha ? PNG_COLOR_TYPE_GRAY_ALPHA
                                  : PNG_COLOR_TYPE_GRAY;
    }

    png_set_IHDR( png_ptr, info_ptr, image->GetWidth(), image->GetHeight(),
                  iBitDepth, iPngColorType,
                  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                  PNG_FILTER_TYPE_BASE);

    int iElements;
    png_color_8 sig_bit;

    if ( iPngColorType & PNG_COLOR_MASK_COLOR )
    {
        sig_bit.red =
        sig_bit.green =
        sig_bit.blue = (png_byte)iBitDepth;
        iElements = 3;
    }
    else // grey
    {
        sig_bit.gray = (png_byte)iBitDepth;
        iElements = 1;
    }

    if ( iPngColorType & PNG_COLOR_MASK_ALPHA )
    {
        sig_bit.alpha = (png_byte)iBitDepth;
        iElements++;
    }

    if ( iBitDepth == 16 )
        iElements *= 2;

    png_set_sBIT( png_ptr, info_ptr, &sig_bit );
    png_write_info( png_ptr, info_ptr );
    png_set_shift( png_ptr, &sig_bit );
    png_set_packing( png_ptr );

    unsigned char *
        data = (unsigned char *)malloc( image->GetWidth() * iElements );
    if ( !data )
    {
        png_destroy_write_struct( &png_ptr, (png_infopp)NULL );
        return false;
    }

    unsigned char *
        pAlpha = (unsigned char *)(bHasAlpha ? image->GetAlpha() : NULL);
    int iHeight = image->GetHeight();
    int iWidth = image->GetWidth();

    unsigned char uchMaskRed = 0, uchMaskGreen = 0, uchMaskBlue = 0;

    if ( bHasMask )
    {
        uchMaskRed = image->GetMaskRed();
        uchMaskGreen = image->GetMaskGreen();
        uchMaskBlue = image->GetMaskBlue();
    }

    unsigned char *pColors = image->GetData();

    for (int y = 0; y != iHeight; ++y)
    {
        unsigned char *pData = data;
        for (int x = 0; x != iWidth; x++)
        {
            unsigned char uchRed = *pColors++;
            unsigned char uchGreen = *pColors++;
            unsigned char uchBlue = *pColors++;

            switch ( iColorType )
            {
                default:
                    wxFAIL_MSG( _T("unknown wxPNG_TYPE_XXX") );
                    // fall through

                case wxPNG_TYPE_COLOUR:
                    *pData++ = uchRed;
                    if ( iBitDepth == 16 )
                        *pData++ = 0;
                    *pData++ = uchGreen;
                    if ( iBitDepth == 16 )
                        *pData++ = 0;
                    *pData++ = uchBlue;
                    if ( iBitDepth == 16 )
                        *pData++ = 0;
                    break;

                case wxPNG_TYPE_GREY:
                    {
                        // where do these coefficients come from? maybe we
                        // should have image options for them as well?
                        unsigned uiColor =
                            (unsigned) (76.544*(unsigned)uchRed +
                                        150.272*(unsigned)uchGreen +
                                        36.864*(unsigned)uchBlue);

                        *pData++ = (unsigned char)((uiColor >> 8) & 0xFF);
                        if ( iBitDepth == 16 )
                            *pData++ = (unsigned char)(uiColor & 0xFF);
                    }
                    break;

                case wxPNG_TYPE_GREY_RED:
                    *pData++ = uchRed;
                    if ( iBitDepth == 16 )
                        *pData++ = 0;
                    break;
            }

            if ( bUseAlpha )
            {
                unsigned char uchAlpha = 255;
                if ( bHasAlpha )
                    uchAlpha = *pAlpha++;

                if ( bHasMask )
                {
                    if ( (uchRed == uchMaskRed)
                            && (uchGreen == uchMaskGreen)
                                && (uchBlue == uchMaskBlue) )
                        uchAlpha = 0;
                }

                *pData++ = uchAlpha;
                if ( iBitDepth == 16 )
                    *pData++ = 0;
            }
        }

        png_bytep row_ptr = data;
        png_write_rows( png_ptr, &row_ptr, 1 );
    }

    free(data);
    png_write_end( png_ptr, info_ptr );
    png_destroy_write_struct( &png_ptr, (png_infopp)&info_ptr );

    return true;
}

#ifdef __VISUALC__
    #pragma warning(default:4611)
#endif /* VC++ */

#endif  // wxUSE_STREAMS

#endif  // wxUSE_LIBPNG
