/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/metafile.cpp
// Purpose:     wxMetafileDC etc.
// Author:      Julian Smart
// Modified by: VZ 07.01.00: implemented wxMetaFileDataObject
// Created:     04/01/98
// RCS-ID:      $Id: metafile.cpp 46103 2007-05-18 15:14:44Z VZ $
// Copyright:   (c) Julian Smart
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

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/app.h"
#endif

#include "wx/metafile.h"

#if wxUSE_METAFILE && !defined(wxMETAFILE_IS_ENH)

#include "wx/clipbrd.h"
#include "wx/msw/private.h"

#include <stdio.h>
#include <string.h>

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxMetafile, wxObject)
IMPLEMENT_ABSTRACT_CLASS(wxMetafileDC, wxDC)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMetafileRefData
// ----------------------------------------------------------------------------

/*
 * Metafiles
 * Currently, the only purpose for making a metafile is to put
 * it on the clipboard.
 */

wxMetafileRefData::wxMetafileRefData()
{
    m_metafile = 0;
    m_windowsMappingMode = wxMM_ANISOTROPIC;
    m_width = m_height = 0;
}

wxMetafileRefData::~wxMetafileRefData()
{
    if (m_metafile)
    {
        DeleteMetaFile((HMETAFILE) m_metafile);
        m_metafile = 0;
    }
}

// ----------------------------------------------------------------------------
// wxMetafile
// ----------------------------------------------------------------------------

wxMetafile::wxMetafile(const wxString& file)
{
    m_refData = new wxMetafileRefData;

    M_METAFILEDATA->m_windowsMappingMode = wxMM_ANISOTROPIC;
    M_METAFILEDATA->m_metafile = 0;
    if (!file.empty())
        M_METAFILEDATA->m_metafile = (WXHANDLE) GetMetaFile(file);
}

wxMetafile::~wxMetafile()
{
}

bool wxMetafile::SetClipboard(int width, int height)
{
#if !wxUSE_CLIPBOARD
    return false;
#else
    if (!m_refData)
        return false;

    bool alreadyOpen = wxClipboardOpen();
    if (!alreadyOpen)
    {
        wxOpenClipboard();
        if (!wxEmptyClipboard())
            return false;
    }
    bool success = wxSetClipboardData(wxDF_METAFILE, this, width,height);
    if (!alreadyOpen)
        wxCloseClipboard();

    return success;
#endif
}

bool wxMetafile::Play(wxDC *dc)
{
    if (!m_refData)
        return false;

    if (dc->GetHDC() && M_METAFILEDATA->m_metafile)
    {
        if ( !::PlayMetaFile(GetHdcOf(*dc), (HMETAFILE)
                             M_METAFILEDATA->m_metafile) )
        {
            wxLogLastError(_T("PlayMetaFile"));
        }
    }

    return true;
}

void wxMetafile::SetHMETAFILE(WXHANDLE mf)
{
    if (!m_refData)
        m_refData = new wxMetafileRefData;

    M_METAFILEDATA->m_metafile = mf;
}

void wxMetafile::SetWindowsMappingMode(int mm)
{
    if (!m_refData)
        m_refData = new wxMetafileRefData;

    M_METAFILEDATA->m_windowsMappingMode = mm;
}

// ----------------------------------------------------------------------------
// Metafile device context
// ----------------------------------------------------------------------------

// Original constructor that does not takes origin and extent. If you use this,
// *DO* give origin/extent arguments to wxMakeMetafilePlaceable.
wxMetafileDC::wxMetafileDC(const wxString& file)
{
    m_metaFile = NULL;
    m_minX = 10000;
    m_minY = 10000;
    m_maxX = -10000;
    m_maxY = -10000;
    //  m_title = NULL;

    if (!file.IsNull() && wxFileExists(file))
        wxRemoveFile(file);

    if (!file.IsNull() && (file != wxEmptyString))
        m_hDC = (WXHDC) CreateMetaFile(file);
    else
        m_hDC = (WXHDC) CreateMetaFile(NULL);

    m_ok = (m_hDC != (WXHDC) 0) ;

    // Actual Windows mapping mode, for future reference.
    m_windowsMappingMode = wxMM_TEXT;

    SetMapMode(wxMM_TEXT); // NOTE: does not set HDC mapmode (this is correct)
}

// New constructor that takes origin and extent. If you use this, don't
// give origin/extent arguments to wxMakeMetafilePlaceable.
wxMetafileDC::wxMetafileDC(const wxString& file, int xext, int yext, int xorg, int yorg)
{
    m_minX = 10000;
    m_minY = 10000;
    m_maxX = -10000;
    m_maxY = -10000;
    if ( !file.empty() && wxFileExists(file) )
        wxRemoveFile(file);
    m_hDC = (WXHDC) CreateMetaFile(file.empty() ? NULL : file.c_str());

    m_ok = true;

    ::SetWindowOrgEx((HDC) m_hDC,xorg,yorg, NULL);
    ::SetWindowExtEx((HDC) m_hDC,xext,yext, NULL);

    // Actual Windows mapping mode, for future reference.
    m_windowsMappingMode = wxMM_ANISOTROPIC;

    SetMapMode(wxMM_TEXT); // NOTE: does not set HDC mapmode (this is correct)
}

wxMetafileDC::~wxMetafileDC()
{
    m_hDC = 0;
}

void wxMetafileDC::DoGetTextExtent(const wxString& string,
                                   wxCoord *x, wxCoord *y,
                                   wxCoord *descent,
                                   wxCoord *externalLeading,
                                   const wxFont *theFont) const
{
    const wxFont *fontToUse = theFont;
    if (!fontToUse)
        fontToUse = &m_font;

    ScreenHDC dc;
    SelectInHDC selFont(dc, GetHfontOf(*fontToUse));

    SIZE sizeRect;
    TEXTMETRIC tm;
    ::GetTextExtentPoint32(dc, string, string.length(), &sizeRect);
    ::GetTextMetrics(dc, &tm);

    if ( x )
        *x = sizeRect.cx;
    if ( y )
        *y = sizeRect.cy;
    if ( descent )
        *descent = tm.tmDescent;
    if ( externalLeading )
        *externalLeading = tm.tmExternalLeading;
}

void wxMetafileDC::GetTextExtent(const wxString& string, long *x, long *y,
                                 long *descent, long *externalLeading, wxFont *theFont, bool WXUNUSED(use16bit)) const
{
    wxCoord xc, yc, dc, elc;
    DoGetTextExtent(string, &xc, &yc, &dc, &elc, theFont);

    if ( x )
        *x = xc;
    if ( y )
        *y = yc;
    if ( descent )
        *descent = dc;
    if ( externalLeading )
        *externalLeading = elc;
}

void wxMetafileDC::DoGetSize(int *width, int *height) const
{
    wxCHECK_RET( m_refData, _T("invalid wxMetafileDC") );

    if ( width )
        *width = M_METAFILEDATA->m_width;
    if ( height )
        *height = M_METAFILEDATA->m_height;
}

wxMetafile *wxMetafileDC::Close()
{
    SelectOldObjects(m_hDC);
    HANDLE mf = CloseMetaFile((HDC) m_hDC);
    m_hDC = 0;
    if (mf)
    {
        wxMetafile *wx_mf = new wxMetafile;
        wx_mf->SetHMETAFILE((WXHANDLE) mf);
        wx_mf->SetWindowsMappingMode(m_windowsMappingMode);
        return wx_mf;
    }
    return NULL;
}

void wxMetafileDC::SetMapMode(int mode)
{
    m_mappingMode = mode;

    //  int pixel_width = 0;
    //  int pixel_height = 0;
    //  int mm_width = 0;
    //  int mm_height = 0;

    float mm2pixelsX = 10.0;
    float mm2pixelsY = 10.0;

    switch (mode)
    {
        case wxMM_TWIPS:
            {
                m_logicalScaleX = (float)(twips2mm * mm2pixelsX);
                m_logicalScaleY = (float)(twips2mm * mm2pixelsY);
                break;
            }
        case wxMM_POINTS:
            {
                m_logicalScaleX = (float)(pt2mm * mm2pixelsX);
                m_logicalScaleY = (float)(pt2mm * mm2pixelsY);
                break;
            }
        case wxMM_METRIC:
            {
                m_logicalScaleX = mm2pixelsX;
                m_logicalScaleY = mm2pixelsY;
                break;
            }
        case wxMM_LOMETRIC:
            {
                m_logicalScaleX = (float)(mm2pixelsX/10.0);
                m_logicalScaleY = (float)(mm2pixelsY/10.0);
                break;
            }
        default:
        case wxMM_TEXT:
            {
                m_logicalScaleX = 1.0;
                m_logicalScaleY = 1.0;
                break;
            }
    }
}

// ----------------------------------------------------------------------------
// wxMakeMetafilePlaceable
// ----------------------------------------------------------------------------

#ifdef __WIN32__
struct RECT32
{
  short left;
  short top;
  short right;
  short bottom;
};

struct mfPLACEABLEHEADER {
    DWORD    key;
    short    hmf;
    RECT32    bbox;
    WORD    inch;
    DWORD    reserved;
    WORD    checksum;
};
#else
struct mfPLACEABLEHEADER {
    DWORD    key;
    HANDLE    hmf;
    RECT    bbox;
    WORD    inch;
    DWORD    reserved;
    WORD    checksum;
};
#endif

/*
 * Pass filename of existing non-placeable metafile, and bounding box.
 * Adds a placeable metafile header, sets the mapping mode to anisotropic,
 * and sets the window origin and extent to mimic the wxMM_TEXT mapping mode.
 *
 */

bool wxMakeMetafilePlaceable(const wxString& filename, float scale)
{
    return wxMakeMetafilePlaceable(filename, 0, 0, 0, 0, scale, false);
}

bool wxMakeMetafilePlaceable(const wxString& filename, int x1, int y1, int x2, int y2, float scale, bool useOriginAndExtent)
{
    // I'm not sure if this is the correct way of suggesting a scale
    // to the client application, but it's the only way I can find.
    int unitsPerInch = (int)(576/scale);

    mfPLACEABLEHEADER header;
    header.key = 0x9AC6CDD7L;
    header.hmf = 0;
    header.bbox.left = (int)(x1);
    header.bbox.top = (int)(y1);
    header.bbox.right = (int)(x2);
    header.bbox.bottom = (int)(y2);
    header.inch = unitsPerInch;
    header.reserved = 0;

    // Calculate checksum
    WORD *p;
    mfPLACEABLEHEADER *pMFHead = &header;
    for (p =(WORD *)pMFHead,pMFHead -> checksum = 0;
            p < (WORD *)&pMFHead ->checksum; ++p)
        pMFHead ->checksum ^= *p;

    FILE *fd = wxFopen(filename.fn_str(), _T("rb"));
    if (!fd) return false;

    wxChar tempFileBuf[256];
    wxGetTempFileName(wxT("mf"), tempFileBuf);
    FILE *fHandle = wxFopen(wxFNCONV(tempFileBuf), _T("wb"));
    if (!fHandle)
        return false;
    fwrite((void *)&header, sizeof(unsigned char), sizeof(mfPLACEABLEHEADER), fHandle);

    // Calculate origin and extent
    int originX = x1;
    int originY = y1;
    int extentX = x2 - x1;
    int extentY = (y2 - y1);

    // Read metafile header and write
    METAHEADER metaHeader;
    fread((void *)&metaHeader, sizeof(unsigned char), sizeof(metaHeader), fd);

    if (useOriginAndExtent)
        metaHeader.mtSize += 15;
    else
        metaHeader.mtSize += 5;

    fwrite((void *)&metaHeader, sizeof(unsigned char), sizeof(metaHeader), fHandle);

    // Write SetMapMode, SetWindowOrigin and SetWindowExt records
    char modeBuffer[8];
    char originBuffer[10];
    char extentBuffer[10];
    METARECORD *modeRecord = (METARECORD *)&modeBuffer;

    METARECORD *originRecord = (METARECORD *)&originBuffer;
    METARECORD *extentRecord = (METARECORD *)&extentBuffer;

    modeRecord->rdSize = 4;
    modeRecord->rdFunction = META_SETMAPMODE;
    modeRecord->rdParm[0] = MM_ANISOTROPIC;

    originRecord->rdSize = 5;
    originRecord->rdFunction = META_SETWINDOWORG;
    originRecord->rdParm[0] = originY;
    originRecord->rdParm[1] = originX;

    extentRecord->rdSize = 5;
    extentRecord->rdFunction = META_SETWINDOWEXT;
    extentRecord->rdParm[0] = extentY;
    extentRecord->rdParm[1] = extentX;

    fwrite((void *)modeBuffer, sizeof(char), 8, fHandle);

    if (useOriginAndExtent)
    {
        fwrite((void *)originBuffer, sizeof(char), 10, fHandle);
        fwrite((void *)extentBuffer, sizeof(char), 10, fHandle);
    }

    int ch = -2;
    while (ch != EOF)
    {
        ch = getc(fd);
        if (ch != EOF)
        {
            putc(ch, fHandle);
        }
    }
    fclose(fHandle);
    fclose(fd);
    wxRemoveFile(filename);
    wxCopyFile(tempFileBuf, filename);
    wxRemoveFile(tempFileBuf);
    return true;
}


#if wxUSE_DRAG_AND_DROP

// ----------------------------------------------------------------------------
// wxMetafileDataObject
// ----------------------------------------------------------------------------

size_t wxMetafileDataObject::GetDataSize() const
{
    return sizeof(METAFILEPICT);
}

bool wxMetafileDataObject::GetDataHere(void *buf) const
{
    METAFILEPICT *mfpict = (METAFILEPICT *)buf;
    const wxMetafile& mf = GetMetafile();

    wxCHECK_MSG( mf.GetHMETAFILE(), false, _T("copying invalid metafile") );

    // doesn't seem to work with any other mapping mode...
    mfpict->mm   = MM_ANISOTROPIC; //mf.GetWindowsMappingMode();
    mfpict->xExt = mf.GetWidth();
    mfpict->yExt = mf.GetHeight();

    // transform the picture size to HIMETRIC units (0.01mm) - as we don't know
    // what DC the picture will be rendered to, use the default display one
    PixelToHIMETRIC(&mfpict->xExt, &mfpict->yExt);

    mfpict->hMF  = CopyMetaFile((HMETAFILE)mf.GetHMETAFILE(), NULL);

    return true;
}

bool wxMetafileDataObject::SetData(size_t WXUNUSED(len), const void *buf)
{
    const METAFILEPICT *mfpict = (const METAFILEPICT *)buf;

    wxMetafile mf;
    mf.SetWindowsMappingMode(mfpict->mm);

    LONG w = mfpict->xExt,
         h = mfpict->yExt;
    if ( mfpict->mm == MM_ANISOTROPIC )
    {
        // in this case xExt and yExt contain suggested size in HIMETRIC units
        // (0.01 mm) - transform this to something more reasonable (pixels)
        HIMETRICToPixel(&w, &h);
    }

    mf.SetWidth(w);
    mf.SetHeight(h);
    mf.SetHMETAFILE((WXHANDLE)mfpict->hMF);

    wxCHECK_MSG( mfpict->hMF, false, _T("pasting invalid metafile") );

    SetMetafile(mf);

    return true;
}

#endif // wxUSE_DRAG_AND_DROP

#endif // wxUSE_METAFILE
