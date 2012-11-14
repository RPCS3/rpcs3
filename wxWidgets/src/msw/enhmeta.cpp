///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/enhmeta.cpp
// Purpose:     implementation of wxEnhMetaFileXXX classes
// Author:      Vadim Zeitlin
// Modified by:
// Created:     13.01.00
// RCS-ID:      $Id: enhmeta.cpp 60850 2009-06-01 10:16:13Z JS $
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

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

#if wxUSE_ENH_METAFILE

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/log.h"
    #include "wx/intl.h"
#endif //WX_PRECOMP

#include "wx/metafile.h"
#include "wx/clipbrd.h"

#include "wx/msw/private.h"

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxEnhMetaFile, wxObject)
IMPLEMENT_ABSTRACT_CLASS(wxEnhMetaFileDC, wxDC)

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#define GetEMF()            ((HENHMETAFILE)m_hMF)
#define GetEMFOf(mf)        ((HENHMETAFILE)((mf).m_hMF))

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// we must pass NULL if the string is empty to metafile functions
static inline const wxChar *GetMetaFileName(const wxString& fn)
    { return !fn ? (wxChar *)NULL : fn.c_str(); }

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxEnhMetaFile
// ----------------------------------------------------------------------------

void wxEnhMetaFile::Init()
{
    if ( m_filename.empty() )
    {
        m_hMF = 0;
    }
    else // have valid file name, load metafile from it
    {
        m_hMF = (WXHANDLE)::GetEnhMetaFile(m_filename);
        if ( !m_hMF )
            wxLogSysError(_("Failed to load metafile from file \"%s\"."),
                          m_filename.c_str());
    }
}

void wxEnhMetaFile::Assign(const wxEnhMetaFile& mf)
{
    if ( &mf == this )
        return;

    if ( mf.m_hMF )
    {
        m_hMF = (WXHANDLE)::CopyEnhMetaFile(GetEMFOf(mf),
                                            GetMetaFileName(m_filename));
        if ( !m_hMF )
        {
            wxLogLastError(_T("CopyEnhMetaFile"));
        }
    }
    else
    {
        m_hMF = 0;
    }
}

void wxEnhMetaFile::Free()
{
    if ( m_hMF )
    {
        if ( !::DeleteEnhMetaFile(GetEMF()) )
        {
            wxLogLastError(_T("DeleteEnhMetaFile"));
        }
    }
}

bool wxEnhMetaFile::Play(wxDC *dc, wxRect *rectBound)
{
    wxCHECK_MSG( Ok(), false, _T("can't play invalid enhanced metafile") );
    wxCHECK_MSG( dc, false, _T("invalid wxDC in wxEnhMetaFile::Play") );

    RECT rect;
    if ( rectBound )
    {
        rect.top = rectBound->y;
        rect.left = rectBound->x;
        rect.right = rectBound->x + rectBound->width;
        rect.bottom = rectBound->y + rectBound->height;
    }
    else
    {
        wxSize size = GetSize();

        rect.top =
        rect.left = 0;
        rect.right = size.x;
        rect.bottom = size.y;
    }

    if ( !::PlayEnhMetaFile(GetHdcOf(*dc), GetEMF(), &rect) )
    {
        wxLogLastError(_T("PlayEnhMetaFile"));

        return false;
    }

    return true;
}

wxSize wxEnhMetaFile::GetSize() const
{
    wxSize size = wxDefaultSize;

    if ( Ok() )
    {
        ENHMETAHEADER hdr;
        if ( !::GetEnhMetaFileHeader(GetEMF(), sizeof(hdr), &hdr) )
        {
            wxLogLastError(_T("GetEnhMetaFileHeader"));
        }
        else
        {
            // the width and height are in HIMETRIC (0.01mm) units, transform
            // them to pixels
            LONG w = hdr.rclFrame.right,
                 h = hdr.rclFrame.bottom;

            HIMETRICToPixel(&w, &h);

            size.x = w;
            size.y = h;
        }
    }

    return size;
}

bool wxEnhMetaFile::SetClipboard(int WXUNUSED(width), int WXUNUSED(height))
{
#if wxUSE_DRAG_AND_DROP && wxUSE_CLIPBOARD
    wxCHECK_MSG( m_hMF, false, _T("can't copy invalid metafile to clipboard") );

    return wxTheClipboard->AddData(new wxEnhMetaFileDataObject(*this));
#else // !wxUSE_DRAG_AND_DROP
    wxFAIL_MSG(_T("not implemented"));
    return false;
#endif // wxUSE_DRAG_AND_DROP/!wxUSE_DRAG_AND_DROP
}

// ----------------------------------------------------------------------------
// wxEnhMetaFileDC
// ----------------------------------------------------------------------------

wxEnhMetaFileDC::wxEnhMetaFileDC(const wxString& filename,
                                 int width, int height,
                                 const wxString& description)
{
    m_width = width;
    m_height = height;

    RECT rect;
    RECT *pRect;
    if ( width && height )
    {
        rect.top =
        rect.left = 0;
        rect.right = width;
        rect.bottom = height;

        // CreateEnhMetaFile() wants them in HIMETRIC
        PixelToHIMETRIC(&rect.right, &rect.bottom);

        pRect = &rect;
    }
    else
    {
        // GDI will try to find out the size for us (not recommended)
        pRect = (LPRECT)NULL;
    }

    ScreenHDC hdcRef;
    m_hDC = (WXHDC)::CreateEnhMetaFile(hdcRef, GetMetaFileName(filename),
                                       pRect, description);
    if ( !m_hDC )
    {
        wxLogLastError(_T("CreateEnhMetaFile"));
    }
}

#if wxUSE_ENH_METAFILE_FROM_DC

void PixelToHIMETRIC(LONG *x, LONG *y, HDC hdcRef)
{
    int iWidthMM = GetDeviceCaps(hdcRef, HORZSIZE),
        iHeightMM = GetDeviceCaps(hdcRef, VERTSIZE),
        iWidthPels = GetDeviceCaps(hdcRef, HORZRES),
        iHeightPels = GetDeviceCaps(hdcRef, VERTRES);

    *x *= (iWidthMM * 100);
    *x /= iWidthPels;
    *y *= (iHeightMM * 100);
    *y /= iHeightPels;
}

void HIMETRICToPixel(LONG *x, LONG *y, HDC hdcRef)
{
    int iWidthMM = GetDeviceCaps(hdcRef, HORZSIZE),
        iHeightMM = GetDeviceCaps(hdcRef, VERTSIZE),
        iWidthPels = GetDeviceCaps(hdcRef, HORZRES),
        iHeightPels = GetDeviceCaps(hdcRef, VERTRES);

    *x *= iWidthPels;
    *x /= (iWidthMM * 100);
    *y *= iHeightPels;
    *y /= (iHeightMM * 100);
}

wxEnhMetaFileDC::wxEnhMetaFileDC(const wxDC& referenceDC,
                                 const wxString& filename,
                                 int width, int height,
                                 const wxString& description)
{
    m_width = width;
    m_height = height;
    HDC hdcRef = GetHdcOf(referenceDC);

    RECT rect;
    RECT *pRect;
    if ( width && height )
    {
        rect.top =
        rect.left = 0;
        rect.right = width;
        rect.bottom = height;

        // CreateEnhMetaFile() wants them in HIMETRIC
        PixelToHIMETRIC(&rect.right, &rect.bottom, hdcRef);

        pRect = &rect;
    }
    else
    {
        // GDI will try to find out the size for us (not recommended)
        pRect = (LPRECT)NULL;
    }

    m_hDC = (WXHDC)::CreateEnhMetaFile(hdcRef, GetMetaFileName(filename),
                                       pRect, description.wx_str());
    if ( !m_hDC )
    {
        wxLogLastError(_T("CreateEnhMetaFile"));
    }
}

#endif

void wxEnhMetaFileDC::DoGetSize(int *width, int *height) const
{
    if ( width )
        *width = m_width;
    if ( height )
        *height = m_height;
}

wxEnhMetaFile *wxEnhMetaFileDC::Close()
{
    wxCHECK_MSG( Ok(), NULL, _T("invalid wxEnhMetaFileDC") );

    HENHMETAFILE hMF = ::CloseEnhMetaFile(GetHdc());
    if ( !hMF )
    {
        wxLogLastError(_T("CloseEnhMetaFile"));

        return NULL;
    }

    wxEnhMetaFile *mf = new wxEnhMetaFile;
    mf->SetHENHMETAFILE((WXHANDLE)hMF);
    return mf;
}

wxEnhMetaFileDC::~wxEnhMetaFileDC()
{
    // avoid freeing it in the base class dtor
    m_hDC = 0;
}

#if wxUSE_DRAG_AND_DROP

// ----------------------------------------------------------------------------
// wxEnhMetaFileDataObject
// ----------------------------------------------------------------------------

wxDataFormat
wxEnhMetaFileDataObject::GetPreferredFormat(Direction WXUNUSED(dir)) const
{
    return wxDF_ENHMETAFILE;
}

size_t wxEnhMetaFileDataObject::GetFormatCount(Direction WXUNUSED(dir)) const
{
    // wxDF_ENHMETAFILE and wxDF_METAFILE
    return 2;
}

void wxEnhMetaFileDataObject::GetAllFormats(wxDataFormat *formats,
                                            Direction WXUNUSED(dir)) const
{
    formats[0] = wxDF_ENHMETAFILE;
    formats[1] = wxDF_METAFILE;
}

size_t wxEnhMetaFileDataObject::GetDataSize(const wxDataFormat& format) const
{
    if ( format == wxDF_ENHMETAFILE )
    {
        // we pass data by handle and not HGLOBAL
        return 0u;
    }
    else
    {
        wxASSERT_MSG( format == wxDF_METAFILE, _T("unsupported format") );

        return sizeof(METAFILEPICT);
    }
}

bool wxEnhMetaFileDataObject::GetDataHere(const wxDataFormat& format, void *buf) const
{
    wxCHECK_MSG( m_metafile.Ok(), false, _T("copying invalid enh metafile") );

    HENHMETAFILE hEMF = (HENHMETAFILE)m_metafile.GetHENHMETAFILE();

    if ( format == wxDF_ENHMETAFILE )
    {
        HENHMETAFILE hEMFCopy = ::CopyEnhMetaFile(hEMF, NULL);
        if ( !hEMFCopy )
        {
            wxLogLastError(_T("CopyEnhMetaFile"));

            return false;
        }

        *(HENHMETAFILE *)buf = hEMFCopy;
    }
    else
    {
        wxASSERT_MSG( format == wxDF_METAFILE, _T("unsupported format") );

        // convert to WMF

        ScreenHDC hdc;

        // first get the buffer size and alloc memory
        size_t size = ::GetWinMetaFileBits(hEMF, 0, NULL, MM_ANISOTROPIC, hdc);
        wxCHECK_MSG( size, false, _T("GetWinMetaFileBits() failed") );

        BYTE *bits = (BYTE *)malloc(size);

        // then get the enh metafile bits
        if ( !::GetWinMetaFileBits(hEMF, size, bits, MM_ANISOTROPIC, hdc) )
        {
            wxLogLastError(_T("GetWinMetaFileBits"));

            free(bits);

            return false;
        }

        // and finally convert them to the WMF
        HMETAFILE hMF = ::SetMetaFileBitsEx(size, bits);
        free(bits);
        if ( !hMF )
        {
            wxLogLastError(_T("SetMetaFileBitsEx"));

            return false;
        }

        METAFILEPICT *mfpict = (METAFILEPICT *)buf;

        wxSize sizeMF = m_metafile.GetSize();
        mfpict->hMF  = hMF;
        mfpict->mm   = MM_ANISOTROPIC;
        mfpict->xExt = sizeMF.x;
        mfpict->yExt = sizeMF.y;

        PixelToHIMETRIC(&mfpict->xExt, &mfpict->yExt);
    }

    return true;
}

bool wxEnhMetaFileDataObject::SetData(const wxDataFormat& format,
                                      size_t WXUNUSED(len),
                                      const void *buf)
{
    HENHMETAFILE hEMF;

    if ( format == wxDF_ENHMETAFILE )
    {
        hEMF = *(HENHMETAFILE *)buf;

        wxCHECK_MSG( hEMF, false, _T("pasting invalid enh metafile") );
    }
    else
    {
        wxASSERT_MSG( format == wxDF_METAFILE, _T("unsupported format") );

        // convert from WMF
        const METAFILEPICT *mfpict = (const METAFILEPICT *)buf;

        // first get the buffer size
        size_t size = ::GetMetaFileBitsEx(mfpict->hMF, 0, NULL);
        wxCHECK_MSG( size, false, _T("GetMetaFileBitsEx() failed") );

        // then get metafile bits
        BYTE *bits = (BYTE *)malloc(size);
        if ( !::GetMetaFileBitsEx(mfpict->hMF, size, bits) )
        {
            wxLogLastError(_T("GetMetaFileBitsEx"));

            free(bits);

            return false;
        }

        ScreenHDC hdcRef;

        // and finally create an enhanced metafile from them
        hEMF = ::SetWinMetaFileBits(size, bits, hdcRef, mfpict);
        free(bits);
        if ( !hEMF )
        {
            wxLogLastError(_T("SetWinMetaFileBits"));

            return false;
        }
    }

    m_metafile.SetHENHMETAFILE((WXHANDLE)hEMF);

    return true;
}

// ----------------------------------------------------------------------------
// wxEnhMetaFileSimpleDataObject
// ----------------------------------------------------------------------------

size_t wxEnhMetaFileSimpleDataObject::GetDataSize() const
{
    // we pass data by handle and not HGLOBAL
    return 0u;
}

bool wxEnhMetaFileSimpleDataObject::GetDataHere(void *buf) const
{
    wxCHECK_MSG( m_metafile.Ok(), false, _T("copying invalid enh metafile") );

    HENHMETAFILE hEMF = (HENHMETAFILE)m_metafile.GetHENHMETAFILE();

    HENHMETAFILE hEMFCopy = ::CopyEnhMetaFile(hEMF, NULL);
    if ( !hEMFCopy )
    {
        wxLogLastError(_T("CopyEnhMetaFile"));

        return false;
    }

    *(HENHMETAFILE *)buf = hEMFCopy;
    return true;
}

bool wxEnhMetaFileSimpleDataObject::SetData(size_t WXUNUSED(len),
                                            const void *buf)
{
    HENHMETAFILE hEMF = *(HENHMETAFILE *)buf;

    wxCHECK_MSG( hEMF, false, _T("pasting invalid enh metafile") );
    m_metafile.SetHENHMETAFILE((WXHANDLE)hEMF);

    return true;
}


#endif // wxUSE_DRAG_AND_DROP

#endif // wxUSE_ENH_METAFILE
