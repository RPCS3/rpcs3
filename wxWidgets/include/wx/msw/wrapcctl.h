///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/wrapcctl.h
// Purpose:     Wrapper for the standard <commctrl.h> header
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.08.2003
// RCS-ID:      $Id: wrapcctl.h 41054 2006-09-07 19:01:45Z ABX $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_WRAPCCTL_H_
#define _WX_MSW_WRAPCCTL_H_

// define _WIN32_IE to a high value because we always check for the version
// of installed DLLs at runtime anyway (see wxApp::GetComCtl32Version()) unless
// the user really doesn't want it and had defined it to a (presumably lower)
// value
//
// just for the reference, here is the table showing what the different value
// of _WIN32_IE correspond to:
//
// 0x0200     for comctl32.dll 4.00 shipped with Win95/NT 4.0
// 0x0300                      4.70              IE 3.x
// 0x0400                      4.71              IE 4.0
// 0x0401                      4.72              IE 4.01 and Win98
// 0x0500                      5.80              IE 5.x
// 0x0500                      5.81              Win2k/ME
// 0x0600                      6.00              WinXP

#ifndef _WIN32_IE
    // use maximal set of features by default, we check for them during
    // run-time anyhow
    #define _WIN32_IE 0x0600
#endif // !defined(_WIN32_IE)

#include "wx/msw/wrapwin.h"

#include <commctrl.h>

// define things which might be missing from our commctrl.h
#include "wx/msw/missing.h"

// Set Unicode format for a common control
inline void wxSetCCUnicodeFormat(HWND WXUNUSED_IN_WINCE(hwnd))
{
#ifndef __WXWINCE__
    ::SendMessage(hwnd, CCM_SETUNICODEFORMAT, wxUSE_UNICODE, 0);
#else // !__WXWINCE__
    // here it should be already in Unicode anyhow
#endif // __WXWINCE__/!__WXWINCE__
}

#if wxUSE_GUI
// Return the default font for the common controls
//
// this is implemented in msw/settings.cpp
class wxFont;
extern wxFont wxGetCCDefaultFont();
#endif

#endif // _WX_MSW_WRAPCCTL_H_
