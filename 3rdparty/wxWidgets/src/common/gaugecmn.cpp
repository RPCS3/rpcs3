///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/gaugecmn.cpp
// Purpose:     wxGaugeBase: common to all ports methods of wxGauge
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.02.01
// RCS-ID:      $Id: gaugecmn.cpp 41089 2006-09-09 13:36:54Z RR $
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
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

#ifndef WX_PRECOMP
#endif //WX_PRECOMP

#if wxUSE_GAUGE

#include "wx/gauge.h"

const wxChar wxGaugeNameStr[] = wxT("gauge");

// ============================================================================
// implementation
// ============================================================================

wxGaugeBase::~wxGaugeBase()
{
    // this destructor is required for Darwin
}

// ----------------------------------------------------------------------------
// wxGauge creation
// ----------------------------------------------------------------------------

bool wxGaugeBase::Create(wxWindow *parent,
                         wxWindowID id,
                         int range,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         const wxValidator& validator,
                         const wxString& name)
{
    if ( !wxControl::Create(parent, id, pos, size, style, validator, name) )
        return false;

    SetName(name);

#if wxUSE_VALIDATORS
    SetValidator(validator);
#endif // wxUSE_VALIDATORS

    SetRange(range);
    SetValue(0);
#if wxGAUGE_EMULATE_INDETERMINATE_MODE
    m_nDirection = wxRIGHT;
#endif

    return true;
}

// ----------------------------------------------------------------------------
// wxGauge determinate mode range/position
// ----------------------------------------------------------------------------

void wxGaugeBase::SetRange(int range)
{
    m_rangeMax = range;
}

int wxGaugeBase::GetRange() const
{
    return m_rangeMax;
}

void wxGaugeBase::SetValue(int pos)
{
    m_gaugePos = pos;
}

int wxGaugeBase::GetValue() const
{
    return m_gaugePos;
}

// ----------------------------------------------------------------------------
// wxGauge indeterminate mode
// ----------------------------------------------------------------------------

void wxGaugeBase::Pulse()
{
#if wxGAUGE_EMULATE_INDETERMINATE_MODE
    // simulate indeterminate mode
    int curr = GetValue(), max = GetRange();

    if (m_nDirection == wxRIGHT)
    {
        if (curr < max)
            SetValue(curr + 1);
        else
        {
            SetValue(max - 1);
            m_nDirection = wxLEFT;
        }
    }
    else
    {
        if (curr > 0)
            SetValue(curr - 1);
        else
        {
            SetValue(1);
            m_nDirection = wxRIGHT;
        }
    }
#endif
}

// ----------------------------------------------------------------------------
// wxGauge appearance params
// ----------------------------------------------------------------------------

void wxGaugeBase::SetShadowWidth(int WXUNUSED(w))
{
}

int wxGaugeBase::GetShadowWidth() const
{
    return 0;
}


void wxGaugeBase::SetBezelFace(int WXUNUSED(w))
{
}

int wxGaugeBase::GetBezelFace() const
{
    return 0;
}

#endif // wxUSE_GAUGE
