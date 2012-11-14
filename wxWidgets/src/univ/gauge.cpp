///////////////////////////////////////////////////////////////////////////////
// Name:        src/gauge/gaugecmn.cpp
// Purpose:     wxGauge for wxUniversal
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.02.01
// RCS-ID:      $Id: gauge.cpp 42816 2006-10-31 08:50:17Z RD $
// Copyright:   (c) 2001 SciTech Software, Inc. (www.scitechsoft.com)
// License:     wxWindows licence
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

#if wxUSE_GAUGE

#include "wx/gauge.h"

#ifndef WX_PRECOMP
#endif //WX_PRECOMP

#include "wx/univ/renderer.h"

IMPLEMENT_DYNAMIC_CLASS(wxGauge, wxControl)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxGauge creation
// ----------------------------------------------------------------------------

void wxGauge::Init()
{
    m_gaugePos =
    m_rangeMax = 0;
}

bool wxGauge::Create(wxWindow *parent,
                     wxWindowID id,
                     int range,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxValidator& validator,
                     const wxString& name)
{
    if ( !wxGaugeBase::Create(parent, id, range, pos, size, style,
                              validator, name) )
    {
        return false;
    }

    SetInitialSize(size);

    return true;
}

// ----------------------------------------------------------------------------
// wxGauge range/position
// ----------------------------------------------------------------------------

void wxGauge::SetRange(int range)
{
    wxGaugeBase::SetRange(range);

    Refresh();
}

void wxGauge::SetValue(int pos)
{
    wxGaugeBase::SetValue(pos);

    Refresh();
}

// ----------------------------------------------------------------------------
// wxGauge geometry
// ----------------------------------------------------------------------------

wxSize wxGauge::DoGetBestClientSize() const
{
    wxSize size = GetRenderer()->GetProgressBarStep();

    // these adjustments are really ridiculous - they are just done to find the
    // "correct" result under Windows (FIXME)
    if ( IsVertical() )
    {
        size.x = (3*size.y) / 2 + 2;
        size.y = wxDefaultCoord;
    }
    else
    {
        size.y = (3*size.x) / 2 + 2;
        size.x = wxDefaultCoord;
    }

    return size;
}

// ----------------------------------------------------------------------------
// wxGauge drawing
// ----------------------------------------------------------------------------

wxBorder wxGauge::GetDefaultBorder() const
{
    return wxBORDER_STATIC;
}

void wxGauge::DoDraw(wxControlRenderer *renderer)
{
    renderer->DrawProgressBar(this);
}

#endif // wxUSE_GAUGE
