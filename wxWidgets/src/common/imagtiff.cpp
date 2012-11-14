/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/imagtiff.cpp
// Purpose:     wxImage TIFF handler
// Author:      Robert Roebling
// RCS-ID:      $Id: imagtiff.cpp 60897 2009-06-04 22:28:48Z VZ $
// Copyright:   (c) Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_IMAGE && wxUSE_LIBTIFF

#include "wx/imagtiff.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/intl.h"
    #include "wx/bitmap.h"
    #include "wx/module.h"
#endif

extern "C"
{
    #include "tiff.h"
    #include "tiffio.h"
}
#include "wx/filefn.h"
#include "wx/wfstream.h"

#ifndef TIFFLINKAGEMODE
    #if defined(__WATCOMC__) && defined(__WXMGL__)
        #define TIFFLINKAGEMODE cdecl
    #else
        #define TIFFLINKAGEMODE LINKAGEMODE
    #endif
#endif

//-----------------------------------------------------------------------------
// wxTIFFHandler
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxTIFFHandler,wxImageHandler)

#if wxUSE_STREAMS

// helper to translate our, possibly 64 bit, wxFileOffset to TIFF, always 32
// bit, toff_t
static toff_t wxFileOffsetToTIFF(wxFileOffset ofs)
{
    if ( ofs == wxInvalidOffset )
        return (toff_t)-1;

    toff_t tofs = wx_truncate_cast(toff_t, ofs);
    wxCHECK_MSG( (wxFileOffset)tofs == ofs, (toff_t)-1,
                    _T("TIFF library doesn't support large files") );

    return tofs;
}

// another helper to convert standard seek mode to our
static wxSeekMode wxSeekModeFromTIFF(int whence)
{
    switch ( whence )
    {
        case SEEK_SET:
            return wxFromStart;

        case SEEK_CUR:
            return wxFromCurrent;

        case SEEK_END:
            return wxFromEnd;

        default:
            return wxFromCurrent;
    }
}

extern "C"
{

tsize_t TIFFLINKAGEMODE
wxTIFFNullProc(thandle_t WXUNUSED(handle),
          tdata_t WXUNUSED(buf),
          tsize_t WXUNUSED(size))
{
    return (tsize_t) -1;
}

tsize_t TIFFLINKAGEMODE
wxTIFFReadProc(thandle_t handle, tdata_t buf, tsize_t size)
{
    wxInputStream *stream = (wxInputStream*) handle;
    stream->Read( (void*) buf, (size_t) size );
    return wx_truncate_cast(tsize_t, stream->LastRead());
}

tsize_t TIFFLINKAGEMODE
wxTIFFWriteProc(thandle_t handle, tdata_t buf, tsize_t size)
{
    wxOutputStream *stream = (wxOutputStream*) handle;
    stream->Write( (void*) buf, (size_t) size );
    return wx_truncate_cast(tsize_t, stream->LastWrite());
}

toff_t TIFFLINKAGEMODE
wxTIFFSeekIProc(thandle_t handle, toff_t off, int whence)
{
    wxInputStream *stream = (wxInputStream*) handle;

    return wxFileOffsetToTIFF(stream->SeekI((wxFileOffset)off,
                                            wxSeekModeFromTIFF(whence)));
}

toff_t TIFFLINKAGEMODE
wxTIFFSeekOProc(thandle_t handle, toff_t off, int whence)
{
    wxOutputStream *stream = (wxOutputStream*) handle;

    return wxFileOffsetToTIFF(stream->SeekO((wxFileOffset)off,
                                            wxSeekModeFromTIFF(whence)));
}

int TIFFLINKAGEMODE
wxTIFFCloseIProc(thandle_t WXUNUSED(handle))
{
    // there is no need to close the input stream
    return 0;
}

int TIFFLINKAGEMODE
wxTIFFCloseOProc(thandle_t handle)
{
    wxOutputStream *stream = (wxOutputStream*) handle;

    return stream->Close() ? 0 : -1;
}

toff_t TIFFLINKAGEMODE
wxTIFFSizeProc(thandle_t handle)
{
    wxStreamBase *stream = (wxStreamBase*) handle;
    return (toff_t) stream->GetSize();
}

int TIFFLINKAGEMODE
wxTIFFMapProc(thandle_t WXUNUSED(handle),
             tdata_t* WXUNUSED(pbase),
             toff_t* WXUNUSED(psize))
{
    return 0;
}

void TIFFLINKAGEMODE
wxTIFFUnmapProc(thandle_t WXUNUSED(handle),
               tdata_t WXUNUSED(base),
               toff_t WXUNUSED(size))
{
}

static void
TIFFwxWarningHandler(const char* module,
                     const char* WXUNUSED_IN_UNICODE(fmt),
                     va_list WXUNUSED_IN_UNICODE(ap))
{
    if (module != NULL)
        wxLogWarning(_("tiff module: %s"), wxString::FromAscii(module).c_str());

    // FIXME: this is not terrible informative but better than crashing!
#if wxUSE_UNICODE
    wxLogWarning(_("TIFF library warning."));
#else
    wxVLogWarning(fmt, ap);
#endif
}

static void
TIFFwxErrorHandler(const char* module,
                   const char* WXUNUSED_IN_UNICODE(fmt),
                   va_list WXUNUSED_IN_UNICODE(ap))
{
    if (module != NULL)
        wxLogError(_("tiff module: %s"), wxString::FromAscii(module).c_str());

    // FIXME: as above
#if wxUSE_UNICODE
    wxLogError(_("TIFF library error."));
#else
    wxVLogError(fmt, ap);
#endif
}

} // extern "C"

TIFF*
TIFFwxOpen(wxInputStream &stream, const char* name, const char* mode)
{
    TIFF* tif = TIFFClientOpen(name, mode,
        (thandle_t) &stream,
        wxTIFFReadProc, wxTIFFNullProc,
        wxTIFFSeekIProc, wxTIFFCloseIProc, wxTIFFSizeProc,
        wxTIFFMapProc, wxTIFFUnmapProc);

    return tif;
}

TIFF*
TIFFwxOpen(wxOutputStream &stream, const char* name, const char* mode)
{
    TIFF* tif = TIFFClientOpen(name, mode,
        (thandle_t) &stream,
        wxTIFFNullProc, wxTIFFWriteProc,
        wxTIFFSeekOProc, wxTIFFCloseOProc, wxTIFFSizeProc,
        wxTIFFMapProc, wxTIFFUnmapProc);

    return tif;
}

wxTIFFHandler::wxTIFFHandler()
{
    m_name = wxT("TIFF file");
    m_extension = wxT("tif");
    m_type = wxBITMAP_TYPE_TIF;
    m_mime = wxT("image/tiff");
    TIFFSetWarningHandler((TIFFErrorHandler) TIFFwxWarningHandler);
    TIFFSetErrorHandler((TIFFErrorHandler) TIFFwxErrorHandler);
}

bool wxTIFFHandler::LoadFile( wxImage *image, wxInputStream& stream, bool verbose, int index )
{
    if (index == -1)
        index = 0;

    image->Destroy();

    TIFF *tif = TIFFwxOpen( stream, "image", "r" );

    if (!tif)
    {
        if (verbose)
            wxLogError( _("TIFF: Error loading image.") );

        return false;
    }

    if (!TIFFSetDirectory( tif, (tdir_t)index ))
    {
        if (verbose)
            wxLogError( _("Invalid TIFF image index.") );

        TIFFClose( tif );

        return false;
    }

    uint32 w, h;
    uint32 *raster;

    TIFFGetField( tif, TIFFTAG_IMAGEWIDTH, &w );
    TIFFGetField( tif, TIFFTAG_IMAGELENGTH, &h );

    uint16 extraSamples;
    uint16* samplesInfo;
    TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES,
                          &extraSamples, &samplesInfo);
    const bool hasAlpha = (extraSamples == 1 &&
                           (samplesInfo[0] == EXTRASAMPLE_ASSOCALPHA ||
                            samplesInfo[0] == EXTRASAMPLE_UNASSALPHA));

    // guard against integer overflow during multiplication which could result
    // in allocating a too small buffer and then overflowing it
    const double bytesNeeded = (double)w * (double)h * sizeof(uint32);
    if ( bytesNeeded >= 4294967295U /* UINT32_MAX */ )
    {
        if ( verbose )
            wxLogError( _("TIFF: Image size is abnormally big.") );

        TIFFClose(tif);

        return false;
    }

    raster = (uint32*) _TIFFmalloc( bytesNeeded );

    if (!raster)
    {
        if (verbose)
            wxLogError( _("TIFF: Couldn't allocate memory.") );

        TIFFClose( tif );

        return false;
    }

    image->Create( (int)w, (int)h );
    if (!image->Ok())
    {
        if (verbose)
            wxLogError( _("TIFF: Couldn't allocate memory.") );

        _TIFFfree( raster );
        TIFFClose( tif );

        return false;
    }

    if ( hasAlpha )
        image->SetAlpha();

    if (!TIFFReadRGBAImage( tif, w, h, raster, 0 ))
    {
        if (verbose)
            wxLogError( _("TIFF: Error reading image.") );

        _TIFFfree( raster );
        image->Destroy();
        TIFFClose( tif );

        return false;
    }

    unsigned char *ptr = image->GetData();
    ptr += w*3*(h-1);

    unsigned char *alpha = hasAlpha ? image->GetAlpha() : NULL;
    if ( hasAlpha )
        alpha += w*(h-1);

    uint32 pos = 0;

    for (uint32 i = 0; i < h; i++)
    {
        for (uint32 j = 0; j < w; j++)
        {
            *(ptr++) = (unsigned char)TIFFGetR(raster[pos]);
            *(ptr++) = (unsigned char)TIFFGetG(raster[pos]);
            *(ptr++) = (unsigned char)TIFFGetB(raster[pos]);
            if ( hasAlpha )
                *(alpha++) = (unsigned char)TIFFGetA(raster[pos]);

            pos++;
        }

        // subtract line we just added plus one line:
        ptr -= 2*w*3;
        if ( hasAlpha )
            alpha -= 2*w;
    }

    _TIFFfree( raster );

    TIFFClose( tif );

    return true;
}

int wxTIFFHandler::GetImageCount( wxInputStream& stream )
{
    TIFF *tif = TIFFwxOpen( stream, "image", "r" );

    if (!tif)
        return 0;

    int dircount = 0;  // according to the libtiff docs, dircount should be set to 1 here???
    do {
        dircount++;
    } while (TIFFReadDirectory(tif));

    TIFFClose( tif );

    return dircount;
}

bool wxTIFFHandler::SaveFile( wxImage *image, wxOutputStream& stream, bool verbose )
{
    TIFF *tif = TIFFwxOpen( stream, "image", "w" );

    if (!tif)
    {
        if (verbose)
            wxLogError( _("TIFF: Error saving image.") );

        return false;
    }

    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH,  (uint32)image->GetWidth());
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32)image->GetHeight());
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    if ( image->HasOption(wxIMAGE_OPTION_RESOLUTIONX) &&
            image->HasOption(wxIMAGE_OPTION_RESOLUTIONY) )
    {
        TIFFSetField(tif, TIFFTAG_XRESOLUTION,
                        (float)image->GetOptionInt(wxIMAGE_OPTION_RESOLUTIONX));
        TIFFSetField(tif, TIFFTAG_YRESOLUTION,
                        (float)image->GetOptionInt(wxIMAGE_OPTION_RESOLUTIONY));
    }

    int spp = image->GetOptionInt(wxIMAGE_OPTION_SAMPLESPERPIXEL);
    if ( !spp )
        spp = 3;

    int bpp = image->GetOptionInt(wxIMAGE_OPTION_BITSPERSAMPLE);
    if ( !bpp )
        bpp=8;

    int compression = image->GetOptionInt(wxIMAGE_OPTION_COMPRESSION);
    if ( !compression )
    {
        // we can't use COMPRESSION_LZW because current version of libtiff
        // doesn't implement it ("no longer implemented due to Unisys patent
        // enforcement") and other compression methods are lossy so we
        // shouldn't use them by default -- and the only remaining one is none
        compression = COMPRESSION_NONE;
    }

    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, spp);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bpp);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, spp*bpp == 1 ? PHOTOMETRIC_MINISBLACK
                                                        : PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, compression);

    // scanlinesize if determined by spp and bpp
    tsize_t linebytes = (tsize_t)image->GetWidth() * spp * bpp / 8;

    if ( (image->GetWidth() % 8 > 0) && (spp * bpp < 8) )
        linebytes+=1;

    unsigned char *buf;

    if (TIFFScanlineSize(tif) > linebytes || (spp * bpp < 24))
    {
        buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(tif));
        if (!buf)
        {
            if (verbose)
                wxLogError( _("TIFF: Couldn't allocate memory.") );

            TIFFClose( tif );

            return false;
        }
    }
    else
    {
        buf = NULL;
    }

    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,TIFFDefaultStripSize(tif, (uint32) -1));

    unsigned char *ptr = image->GetData();
    for ( int row = 0; row < image->GetHeight(); row++ )
    {
        if ( buf )
        {
            if ( spp * bpp > 1 )
            {
                // color image
                memcpy(buf, ptr, image->GetWidth());
            }
            else // black and white image
            {
                for ( int column = 0; column < linebytes; column++ )
                {
                    uint8 reverse = 0;
                    for ( int bp = 0; bp < 8; bp++ )
                    {
                        if ( ptr[column*24 + bp*3] > 0 )
                        {
                            // check only red as this is sufficient
                            reverse = (uint8)(reverse | 128 >> bp);
                        }
                    }

                    buf[column] = reverse;
                }
            }
        }

        if ( TIFFWriteScanline(tif, buf ? buf : ptr, (uint32)row, 0) < 0 )
        {
            if (verbose)
                wxLogError( _("TIFF: Error writing image.") );

            TIFFClose( tif );
            if (buf)
                _TIFFfree(buf);

            return false;
        }

        ptr += image->GetWidth()*3;
    }

    (void) TIFFClose(tif);

    if (buf)
        _TIFFfree(buf);

    return true;
}

bool wxTIFFHandler::DoCanRead( wxInputStream& stream )
{
    unsigned char hdr[2];

    if ( !stream.Read(&hdr[0], WXSIZEOF(hdr)) )
        return false;

    return (hdr[0] == 'I' && hdr[1] == 'I') ||
           (hdr[0] == 'M' && hdr[1] == 'M');
}

#endif  // wxUSE_STREAMS

#endif  // wxUSE_LIBTIFF
