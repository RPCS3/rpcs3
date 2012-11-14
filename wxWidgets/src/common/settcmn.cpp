/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/settcmn.cpp
// Purpose:     common (to all ports) wxWindow functions
// Author:      Robert Roebling
// RCS-ID:      $Id: settcmn.cpp 39310 2006-05-24 07:16:32Z ABX $
// Copyright:   (c) wxWidgets team
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

#include "wx/settings.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
#endif //WX_PRECOMP

// ----------------------------------------------------------------------------
// static data
// ----------------------------------------------------------------------------

wxSystemScreenType wxSystemSettings::ms_screen = wxSYS_SCREEN_NONE;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

wxSystemScreenType wxSystemSettings::GetScreenType()
{
    if (ms_screen == wxSYS_SCREEN_NONE)
    {
        // wxUniv will be used on small devices, too.
        int x = GetMetric( wxSYS_SCREEN_X );

        ms_screen = wxSYS_SCREEN_DESKTOP;

        if (x < 800)
            ms_screen = wxSYS_SCREEN_SMALL;

        if (x < 640)
            ms_screen = wxSYS_SCREEN_PDA;

        if (x < 200)
            ms_screen = wxSYS_SCREEN_TINY;

        // This is probably a bug, but VNC seems to report 0
        if (x < 10)
            ms_screen = wxSYS_SCREEN_DESKTOP;
    }

    return ms_screen;
}

void wxSystemSettings::SetScreenType( wxSystemScreenType screen )
{
    ms_screen = screen;
}

#if WXWIN_COMPATIBILITY_2_4

wxColour wxSystemSettings::GetSystemColour(int index)
{
    return GetColour((wxSystemColour)index);
}

wxFont wxSystemSettings::GetSystemFont(int index)
{
    return GetFont((wxSystemFont)index);
}

int wxSystemSettings::GetSystemMetric(int index)
{
    return GetMetric((wxSystemMetric)index);
}

#endif // WXWIN_COMPATIBILITY_2_4
