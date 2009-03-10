///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/power.cpp
// Purpose:     power management functions for MSW
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2006-05-27
// RCS-ID:      $Id: power.cpp 48517 2007-09-02 20:24:14Z JS $
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
#include "wx/msw/private.h"

#if !defined(__WINCE_STANDARDSDK__)

#ifdef __WXWINCE__
    typedef SYSTEM_POWER_STATUS_EX SYSTEM_POWER_STATUS;
    BOOL GetSystemPowerStatus(SYSTEM_POWER_STATUS *status)
    {
        return GetSystemPowerStatusEx(status, TRUE);
    }
#endif

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

static inline bool wxGetPowerStatus(SYSTEM_POWER_STATUS *sps)
{
    if ( !::GetSystemPowerStatus(sps) )
    {
        wxLogLastError(_T("GetSystemPowerStatus()"));
        return false;
    }

    return true;
}

#endif

// ============================================================================
// implementation
// ============================================================================

wxPowerType wxGetPowerType()
{
#if !defined(__WINCE_STANDARDSDK__)
    SYSTEM_POWER_STATUS sps;
    if ( wxGetPowerStatus(&sps) )
    {
        switch ( sps.ACLineStatus )
        {
            case 0:
                return wxPOWER_BATTERY;

            case 1:
                return wxPOWER_SOCKET;

            default:
                wxLogDebug(_T("Unknown ACLineStatus=%u"), sps.ACLineStatus);
            case 255:
                break;
        }
    }
#endif

    return wxPOWER_UNKNOWN;
}

wxBatteryState wxGetBatteryState()
{
#if !defined(__WINCE_STANDARDSDK__)
    SYSTEM_POWER_STATUS sps;
    if ( wxGetPowerStatus(&sps) )
    {
        // there can be other bits set in the flag field ("charging" and "no
        // battery"), extract only those which we need here
        switch ( sps.BatteryFlag & 7 )
        {
            case 1:
                return wxBATTERY_NORMAL_STATE;

            case 2:
                return wxBATTERY_LOW_STATE;

            case 3:
                return wxBATTERY_CRITICAL_STATE;
        }
    }
#endif

    return wxBATTERY_UNKNOWN_STATE;
}
