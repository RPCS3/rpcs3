/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/statline.cpp
// Purpose:     a generic wxStaticLine class
// Author:      Vadim Zeitlin
// Created:     28.06.99
// Version:     $Id: statline.cpp 39487 2006-05-31 18:27:51Z ABX $
// Copyright:   (c) 1998 Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

// For compilers that support precompilation, includes "wx.h".

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_STATLINE

#include "wx/statline.h"

#ifndef WX_PRECOMP
    #include "wx/statbox.h"
#endif

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxStaticLine, wxControl)

// ----------------------------------------------------------------------------
// wxStaticLine
// ----------------------------------------------------------------------------

bool wxStaticLine::Create( wxWindow *parent,
                           wxWindowID id,
                           const wxPoint &pos,
                           const wxSize &size,
                           long style,
                           const wxString &name)
{
    m_statbox = NULL;

    if ( !CreateBase(parent, id, pos, size, style, wxDefaultValidator, name) )
        return false;

    // ok, this is ugly but it's better than nothing: use a thin static box to
    // emulate static line

    wxSize sizeReal = AdjustSize(size);

    m_statbox = new wxStaticBox(parent, id, wxEmptyString, pos, sizeReal, style, name);

    return true;
}

wxStaticLine::~wxStaticLine()
{
    delete m_statbox;
}

WXWidget wxStaticLine::GetMainWidget() const
{
    return m_statbox->GetMainWidget();
}

void wxStaticLine::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    m_statbox->SetSize(x, y, width, height, sizeFlags);
}

void wxStaticLine::DoMoveWindow(int x, int y, int width, int height)
{
    m_statbox->SetSize(x, y, width, height);
}

#endif
  // wxUSE_STATLINE
