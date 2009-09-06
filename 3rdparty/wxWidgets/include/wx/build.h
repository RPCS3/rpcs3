///////////////////////////////////////////////////////////////////////////////
// Name:        wx/build.h
// Purpose:     Runtime build options checking
// Author:      Vadim Zeitlin, Vaclav Slavik
// Modified by:
// Created:     07.05.02
// RCS-ID:      $Id: build.h 35858 2005-10-09 15:48:42Z MBN $
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_BUILD_H_
#define _WX_BUILD_H_

#include "wx/version.h"

// NB: This file contains macros for checking binary compatibility of libraries
//     in multilib buildm, plugins and user components.
//     The WX_BUILD_OPTIONS_SIGNATURE macro expands into string that should
//     uniquely identify binary compatible builds: i.e. if two builds of the
//     library are binary compatible, their signature string should be the
//     same; if two builds are binary incompatible, their signatures should
//     be different.
//
//     Therefore, wxUSE_XXX flags that affect binary compatibility (vtables,
//     function signatures) should be accounted for here. So should compilers
//     and compiler versions (but note that binary compatible compiler versions
//     such as gcc-2.95.2 and gcc-2.95.3 should have same signature!).

// ----------------------------------------------------------------------------
// WX_BUILD_OPTIONS_SIGNATURE
// ----------------------------------------------------------------------------

#define __WX_BO_STRINGIZE(x)   __WX_BO_STRINGIZE0(x)
#define __WX_BO_STRINGIZE0(x)  #x

#if (wxMINOR_VERSION % 2) == 0
    #define __WX_BO_VERSION(x,y,z) \
        __WX_BO_STRINGIZE(x) "." __WX_BO_STRINGIZE(y)
#else
    #define __WX_BO_VERSION(x,y,z) \
        __WX_BO_STRINGIZE(x) "." __WX_BO_STRINGIZE(y) "." __WX_BO_STRINGIZE(z)
#endif

#ifdef __WXDEBUG__
    #define __WX_BO_DEBUG "debug"
#else
    #define __WX_BO_DEBUG "no debug"
#endif

#if wxUSE_UNICODE
    #define __WX_BO_UNICODE "Unicode"
#else
    #define __WX_BO_UNICODE "ANSI"
#endif

// GCC and Intel C++ share same C++ ABI (and possibly others in the future),
// check if compiler versions are compatible:
#if defined(__GXX_ABI_VERSION)
    #define __WX_BO_COMPILER \
            ",compiler with C++ ABI " __WX_BO_STRINGIZE(__GXX_ABI_VERSION)
#elif defined(__INTEL_COMPILER)
    #define __WX_BO_COMPILER ",Intel C++"
#elif defined(__GNUG__)
    #define __WX_BO_COMPILER ",GCC " \
            __WX_BO_STRINGIZE(__GNUC__) "." __WX_BO_STRINGIZE(__GNUC_MINOR__)
#elif defined(__VISUALC__)
    #define __WX_BO_COMPILER ",Visual C++"
#elif defined(__BORLANDC__)
    #define __WX_BO_COMPILER ",Borland C++"
#elif defined(__DIGITALMARS__)
    #define __WX_BO_COMPILER ",DigitalMars"
#elif defined(__WATCOMC__)
    #define __WX_BO_COMPILER ",Watcom C++"
#else
    #define __WX_BO_COMPILER
#endif

// WXWIN_COMPATIBILITY macros affect presence of virtual functions
#if WXWIN_COMPATIBILITY_2_4
    #define __WX_BO_WXWIN_COMPAT_2_4 ",compatible with 2.4"
#else
    #define __WX_BO_WXWIN_COMPAT_2_4
#endif
#if WXWIN_COMPATIBILITY_2_6
    #define __WX_BO_WXWIN_COMPAT_2_6 ",compatible with 2.6"
#else
    #define __WX_BO_WXWIN_COMPAT_2_6
#endif

// deriving wxWin containers from STL ones changes them completely:
#if wxUSE_STL
    #define __WX_BO_STL ",STL containers"
#else
    #define __WX_BO_STL ",wx containers"
#endif

// This macro is passed as argument to wxConsoleApp::CheckBuildOptions()
#define WX_BUILD_OPTIONS_SIGNATURE \
    __WX_BO_VERSION(wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER) \
    " (" __WX_BO_DEBUG "," __WX_BO_UNICODE \
     __WX_BO_COMPILER \
     __WX_BO_STL \
     __WX_BO_WXWIN_COMPAT_2_4 __WX_BO_WXWIN_COMPAT_2_6 \
     ")"


// ----------------------------------------------------------------------------
// WX_CHECK_BUILD_OPTIONS
// ----------------------------------------------------------------------------

// Use this macro to check build options. Adding it to a file in DLL will
// ensure that the DLL checks build options in same way IMPLEMENT_APP() does.
#define WX_CHECK_BUILD_OPTIONS(libName)                                 \
    static struct wxBuildOptionsChecker                                 \
    {                                                                   \
        wxBuildOptionsChecker()                                         \
        {                                                               \
            wxAppConsole::CheckBuildOptions(WX_BUILD_OPTIONS_SIGNATURE, \
                                            libName);                   \
        }                                                               \
    } gs_buildOptionsCheck;


#if WXWIN_COMPATIBILITY_2_4

// ----------------------------------------------------------------------------
// wxBuildOptions
// ----------------------------------------------------------------------------

// NB: Don't use this class in new code, it relies on the ctor being always
//     inlined. WX_BUILD_OPTIONS_SIGNATURE always works.
class wxBuildOptions
{
public:
    // the ctor must be inline to get the compilation settings of the code
    // which included this header
    wxBuildOptions() : m_signature(WX_BUILD_OPTIONS_SIGNATURE) {}

private:
    const char *m_signature;

    // actually only CheckBuildOptions() should be our friend but well...
    friend class wxAppConsole;
};

#endif // WXWIN_COMPATIBILITY_2_4

#endif // _WX_BUILD_H_
