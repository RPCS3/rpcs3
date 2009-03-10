/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/wince/libraries.h
// Purpose:     VC++ pragmas for linking against SDK libs
// Author:      Vaclav Slavik
// Modified by:
// Created:     2004-04-11
// RCS-ID:      $Id: libraries.h 38141 2006-03-16 11:03:26Z JS $
// Copyright:   (c) 2004 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_LIBRARIES_H_
#define _WX_LIBRARIES_H_

// NB: According to Microsoft, it is up to the OEM to decide whether
//     some of libraries will be included in the system or not. For example,
//     MS' STANDARDSDK does not include cyshell.lib and aygshell.lib, while
//     Pocket PC 2003 SDK does. We depend on some symbols that are in these
//     libraries in some SDKs and in different libs in others. Fortunately we
//     can detect what SDK is used in C++ code, so we take advantage of
//     VC++'s #pragma to link against the libraries conditionally, instead of
//     including libraries in project files.

#if defined(__VISUALC__) && defined(__WXWINCE__)

#if (_WIN32_WCE >= 400) || defined(__POCKETPC__)
    // No commdlg.lib in Mobile 5.0 Smartphone
#if !(defined(__SMARTPHONE__) && _WIN32_WCE >= 1200)
    #pragma comment(lib,"commdlg.lib")
#endif
#endif

#if (_WIN32_WCE >= 400) && !defined(wxNO_RTTI)
    #pragma comment(lib,"ccrtrtti.lib")
#endif

#if defined(__WINCE_STANDARDSDK__)
    // DoDragDrop:
    #pragma comment(lib,"olece400.lib")
#elif defined(__POCKETPC__)
    // PocketPC build:
    // DoDragDrop:
    #pragma comment(lib,"ceshell.lib")

    #pragma comment(lib,"aygshell.lib")
#elif defined(__HANDHELDPC__)
    // Handheld PC builds. Maybe WindowsCE.NET 4.X needs another symbol.
    #pragma comment(lib,"ceshell.lib")
#elif defined(__SMARTPHONE__)
    // Smartphone build:
    #pragma comment(lib,"ceshell.lib")
    #pragma comment(lib,"aygshell.lib")
#else
    #error "Unknown SDK, please fill-in missing pieces"
#endif

#endif // __VISUALC__ && __WXWINCE__

#endif // _WX_LIBRARIES_H_
