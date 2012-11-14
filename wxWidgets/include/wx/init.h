///////////////////////////////////////////////////////////////////////////////
// Name:        wx/init.h
// Purpose:     wxWidgets initialization and finalization functions
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29.06.2003
// RCS-ID:      $Id: init.h 61558 2009-07-30 10:14:36Z VS $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_INIT_H_
#define _WX_INIT_H_

#include "wx/defs.h"
#include "wx/wxchar.h"

// ----------------------------------------------------------------------------
// wxEntry helper functions which allow to have more fine grained control
// ----------------------------------------------------------------------------

// do common initialization, return true if ok (in this case wxEntryCleanup
// must be called later), otherwise the program can't use wxWidgets at all
//
// this function also creates wxTheApp as a side effect, if IMPLEMENT_APP
// hadn't been used a dummy default application object is created
//
// note that the parameters may be modified, this is why we pass them by
// reference!
extern bool WXDLLIMPEXP_BASE wxEntryStart(int& argc, wxChar **argv);

// free the resources allocated by the library in wxEntryStart() and shut it
// down (wxEntryStart() may be called again afterwards if necessary)
extern void WXDLLIMPEXP_BASE wxEntryCleanup();


// ----------------------------------------------------------------------------
// wxEntry: this function initializes the library, runs the main event loop
//          and cleans it up
// ----------------------------------------------------------------------------

// note that other, platform-specific, overloads of wxEntry may exist as well
// but this one always exists under all platforms
//
// returns the program exit code
extern int WXDLLIMPEXP_BASE wxEntry(int& argc, wxChar **argv);

// we overload wxEntry[Start]() to take "char **" pointers too
#if wxUSE_UNICODE

extern bool WXDLLIMPEXP_BASE wxEntryStart(int& argc, char **argv);
extern int WXDLLIMPEXP_BASE wxEntry(int& argc, char **argv);

#endif// wxUSE_UNICODE

// ----------------------------------------------------------------------------
// Using the library without (explicit) application object: you may avoid using
// DECLARE_APP and IMPLEMENT_APP macros and call the functions below instead at
// the program startup and termination
// ----------------------------------------------------------------------------

// initialize the library (may be called as many times as needed, but each
// call to wxInitialize() must be matched by wxUninitialize())
extern bool WXDLLIMPEXP_BASE wxInitialize(int argc = 0, wxChar **argv = NULL);
#if wxUSE_UNICODE && wxABI_VERSION >= 20811
extern bool WXDLLIMPEXP_BASE wxInitialize(int argc, char **argv);
#endif

// clean up -- the library can't be used any more after the last call to
// wxUninitialize()
extern void WXDLLIMPEXP_BASE wxUninitialize();

// create an object of this class on stack to initialize/cleanup the library
// automatically
class WXDLLIMPEXP_BASE wxInitializer
{
public:
    // initialize the library
    wxInitializer(int argc = 0, wxChar **argv = NULL)
    {
        m_ok = wxInitialize(argc, argv);
    }

#if wxUSE_UNICODE && wxABI_VERSION >= 20811
    wxInitializer(int argc, char **argv)
    {
        m_ok = wxInitialize(argc, argv);
    }
#endif // wxUSE_UNICODE

    // has the initialization been successful? (explicit test)
    bool IsOk() const { return m_ok; }

    // has the initialization been successful? (implicit test)
    operator bool() const { return m_ok; }

    // dtor only does clean up if we initialized the library properly
    ~wxInitializer() { if ( m_ok ) wxUninitialize(); }

private:
    bool m_ok;
};

#endif // _WX_INIT_H_

