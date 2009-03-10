///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/powercmn.cpp
// Purpose:     power event types and stubs for power functions
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2006-05-27
// RCS-ID:      $Id: powercmn.cpp 48811 2007-09-19 23:11:28Z RD $
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
#endif //WX_PRECOMP

#include "wx/power.h"

// ============================================================================
// implementation
// ============================================================================

#ifdef wxHAS_POWER_EVENTS
    DEFINE_EVENT_TYPE(wxEVT_POWER_SUSPENDING)
    DEFINE_EVENT_TYPE(wxEVT_POWER_SUSPENDED)
    DEFINE_EVENT_TYPE(wxEVT_POWER_SUSPEND_CANCEL)
    DEFINE_EVENT_TYPE(wxEVT_POWER_RESUME)

    IMPLEMENT_ABSTRACT_CLASS(wxPowerEvent, wxEvent)
#endif
    
// provide stubs for the systems not implementing these functions
#if !defined(__WXPALMOS__) && !defined(__WXMSW__)

wxPowerType wxGetPowerType()
{
    return wxPOWER_UNKNOWN;
}

wxBatteryState wxGetBatteryState()
{
    return wxBATTERY_UNKNOWN_STATE;
}

#endif // systems without power management functions

