/////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/statbmp.cpp
// Purpose:     wxStaticBitmap implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     25.08.00
// RCS-ID:      $Id: statbmp.cpp 42816 2006-10-31 08:50:17Z RD $
// Copyright:   (c) 2000 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_STATBMP

#include "wx/statbmp.h"

#ifndef WX_PRECOMP
    #include "wx/dc.h"
    #include "wx/icon.h"
    #include "wx/validate.h"
#endif

#include "wx/univ/renderer.h"
#include "wx/univ/theme.h"

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxStaticBitmap, wxControl)

// ----------------------------------------------------------------------------
// wxStaticBitmap
// ----------------------------------------------------------------------------

bool wxStaticBitmap::Create(wxWindow *parent,
                            wxWindowID id,
                            const wxBitmap &label,
                            const wxPoint &pos,
                            const wxSize &size,
                            long style,
                            const wxString &name)
{
    if ( !wxControl::Create(parent, id, pos, size, style, wxDefaultValidator, name) )
        return false;

    // set bitmap first
    SetBitmap(label);

    // and adjust our size to fit it after this
    SetInitialSize(size);

    return true;
}

// ----------------------------------------------------------------------------
// bitmap/icon setting/getting and converting between
// ----------------------------------------------------------------------------

void wxStaticBitmap::SetBitmap(const wxBitmap& bitmap)
{
    m_bitmap = bitmap;
}

void wxStaticBitmap::SetIcon(const wxIcon& icon)
{
#ifdef __WXMSW__
    m_bitmap.CopyFromIcon(icon);
#else
    m_bitmap = (const wxBitmap&)icon;
#endif
}

wxIcon wxStaticBitmap::GetIcon() const
{
    wxIcon icon;
#ifdef __WXMSW__
    icon.CopyFromBitmap(m_bitmap);
#else
    icon = (const wxIcon&)m_bitmap;
#endif
    return icon;
}

// ----------------------------------------------------------------------------
// drawing
// ----------------------------------------------------------------------------

void wxStaticBitmap::DoDraw(wxControlRenderer *renderer)
{
    wxControl::DoDraw(renderer);
    renderer->DrawBitmap(GetBitmap());
}

#endif // wxUSE_STATBMP
