/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/icon.cpp
// Purpose:     wxIcon class
// Author:      Julian Smart
// Modified by: 20.11.99 (VZ): don't derive from wxBitmap any more
// Created:     04/01/98
// RCS-ID:      $Id: icon.cpp 39551 2006-06-04 15:51:39Z VZ $
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
    #include "wx/list.h"
    #include "wx/utils.h"
    #include "wx/app.h"
    #include "wx/icon.h"
    #include "wx/bitmap.h"
    #include "wx/log.h"
#endif

#include "wx/msw/private.h"

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxIcon, wxGDIObject)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxIconRefData
// ----------------------------------------------------------------------------

void wxIconRefData::Free()
{
    if ( m_hIcon )
    {
#ifndef __WXMICROWIN__
        ::DestroyIcon((HICON) m_hIcon);
#endif

        m_hIcon = 0;
    }
}

// ----------------------------------------------------------------------------
// wxIcon
// ----------------------------------------------------------------------------

wxIcon::wxIcon(const char bits[], int width, int height)
{
    wxBitmap bmp(bits, width, height);
    CopyFromBitmap(bmp);
}

wxIcon::wxIcon(const wxString& iconfile,
               long flags,
               int desiredWidth,
               int desiredHeight)

{
    LoadFile(iconfile, flags, desiredWidth, desiredHeight);
}

wxIcon::wxIcon(const wxIconLocation& loc)
{
    // wxICOFileHandler accepts names in the format "filename;index"
    wxString fullname = loc.GetFileName();
    if ( loc.GetIndex() )
    {
        fullname << _T(';') << loc.GetIndex();
    }
    //else: 0 is default

    LoadFile(fullname, wxBITMAP_TYPE_ICO);
}

wxIcon::~wxIcon()
{
}

wxObjectRefData *wxIcon::CloneRefData(const wxObjectRefData *dataOrig) const
{
    const wxIconRefData *
        data = wx_static_cast(const wxIconRefData *, dataOrig);
    if ( !data )
        return NULL;

    // we don't have to copy m_hIcon because we're only called from SetHICON()
    // which overwrites m_hIcon anyhow currently
    //
    // and if we're called from SetWidth/Height/Depth(), it doesn't make sense
    // to copy it neither as the handle would be inconsistent with the new size
    return new wxIconRefData(*data);
}

void wxIcon::CopyFromBitmap(const wxBitmap& bmp)
{
#ifndef __WXMICROWIN__
    HICON hicon = wxBitmapToHICON(bmp);
    if ( !hicon )
    {
        wxLogLastError(wxT("CreateIconIndirect"));
    }
    else
    {
        SetHICON((WXHICON)hicon);
        SetSize(bmp.GetWidth(), bmp.GetHeight());
    }
#endif // __WXMICROWIN__
}

void wxIcon::CreateIconFromXpm(const char **data)
{
    wxBitmap bmp(data);
    CopyFromBitmap(bmp);
}

bool wxIcon::LoadFile(const wxString& filename,
                      long type,
                      int desiredWidth, int desiredHeight)
{
    UnRef();

    wxGDIImageHandler *handler = FindHandler(type);

    if ( !handler )
    {
        // load via wxBitmap which, in turn, uses wxImage allowing us to
        // support more formats
        wxBitmap bmp;
        if ( !bmp.LoadFile(filename, type) )
            return false;

        CopyFromBitmap(bmp);
        return true;
    }

    return handler->Load(this, filename, type, desiredWidth, desiredHeight);
}
