///////////////////////////////////////////////////////////////////////////////
// Name:        wx/apptrait.h
// Purpose:     declaration of wxAppTraits and derived classes
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.06.2003
// RCS-ID:      $Id: apptrait.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_APPTRAIT_H_
#define _WX_APPTRAIT_H_

#include "wx/string.h"
#include "wx/platinfo.h"

class WXDLLIMPEXP_FWD_BASE wxObject;
class WXDLLIMPEXP_FWD_BASE wxAppTraits;
#if wxUSE_FONTMAP
    class WXDLLIMPEXP_FWD_CORE wxFontMapper;
#endif // wxUSE_FONTMAP
class WXDLLIMPEXP_FWD_BASE wxLog;
class WXDLLIMPEXP_FWD_BASE wxMessageOutput;
class WXDLLIMPEXP_FWD_CORE wxRendererNative;
class WXDLLIMPEXP_FWD_BASE wxString;

class GSocketGUIFunctionsTable;


// ----------------------------------------------------------------------------
// wxAppTraits: this class defines various configurable aspects of wxApp
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_BASE wxStandardPathsBase;

class WXDLLIMPEXP_BASE wxAppTraitsBase
{
public:
    // needed since this class declares virtual members
    virtual ~wxAppTraitsBase() { }

    // hooks for creating the global objects, may be overridden by the user
    // ------------------------------------------------------------------------

#if wxUSE_LOG
    // create the default log target
    virtual wxLog *CreateLogTarget() = 0;
#endif // wxUSE_LOG

    // create the global object used for printing out messages
    virtual wxMessageOutput *CreateMessageOutput() = 0;

#if wxUSE_FONTMAP
    // create the global font mapper object used for encodings/charset mapping
    virtual wxFontMapper *CreateFontMapper() = 0;
#endif // wxUSE_FONTMAP

    // get the renderer to use for drawing the generic controls (return value
    // may be NULL in which case the default renderer for the current platform
    // is used); this is used in GUI only and always returns NULL in console
    //
    // NB: returned pointer will be deleted by the caller
    virtual wxRendererNative *CreateRenderer() = 0;

#if wxUSE_STDPATHS
    // wxStandardPaths object is normally the same for wxBase and wxGUI
    // except in the case of wxMac and wxCocoa
    virtual wxStandardPathsBase& GetStandardPaths();
#endif // wxUSE_STDPATHS

    // functions abstracting differences between GUI and console modes
    // ------------------------------------------------------------------------

#ifdef __WXDEBUG__
    // show the assert dialog with the specified message in GUI or just print
    // the string to stderr in console mode
    //
    // base class version has an implementation (in spite of being pure
    // virtual) in base/appbase.cpp which can be called as last resort.
    //
    // return true to suppress subsequent asserts, false to continue as before
    virtual bool ShowAssertDialog(const wxString& msg) = 0;
#endif // __WXDEBUG__

    // return true if fprintf(stderr) goes somewhere, false otherwise
    virtual bool HasStderr() = 0;

    // managing "pending delete" list: in GUI mode we can't immediately delete
    // some objects because there may be unprocessed events for them and so we
    // only do it during the next idle loop iteration while this is, of course,
    // unnecessary in wxBase, so we have a few functions to abstract these
    // operations

    // add the object to the pending delete list in GUI, delete it immediately
    // in wxBase
    virtual void ScheduleForDestroy(wxObject *object) = 0;

    // remove this object from the pending delete list in GUI, do nothing in
    // wxBase
    virtual void RemoveFromPendingDelete(wxObject *object) = 0;

#if wxUSE_SOCKETS
    // return table of GUI callbacks for GSocket code or NULL in wxBase. This
    // is needed because networking classes are in their own library and so
    // they can't directly call GUI functions (the same net library can be
    // used in both GUI and base apps). To complicate it further, GUI library
    // ("wxCore") doesn't depend on networking library and so only a functions
    // table can be passed around
    virtual GSocketGUIFunctionsTable* GetSocketGUIFunctionsTable() = 0;
#endif

    // return information about the (native) toolkit currently used and its
    // runtime (not compile-time) version.
    // returns wxPORT_BASE for console applications and one of the remaining
    // wxPORT_* values for GUI applications.
    virtual wxPortId GetToolkitVersion(int *majVer, int *minVer) const = 0;

    // return true if the port is using wxUniversal for the GUI, false if not
    virtual bool IsUsingUniversalWidgets() const = 0;

    // return the name of the Desktop Environment such as
    // "KDE" or "GNOME". May return an empty string.
    virtual wxString GetDesktopEnvironment() const { return wxEmptyString; }

protected:
#if wxUSE_STACKWALKER && defined( __WXDEBUG__ )
    // utility function: returns the stack frame as a plain wxString
    virtual wxString GetAssertStackTrace();
#endif
};

// ----------------------------------------------------------------------------
// include the platform-specific version of the class
// ----------------------------------------------------------------------------

// NB:  test for __UNIX__ before __WXMAC__ as under Darwin we want to use the
//      Unix code (and otherwise __UNIX__ wouldn't be defined)
// ABX: check __WIN32__ instead of __WXMSW__ for the same MSWBase in any Win32 port
#if defined(__WXPALMOS__)
    #include "wx/palmos/apptbase.h"
#elif defined(__WIN32__)
    #include "wx/msw/apptbase.h"
#elif defined(__UNIX__) && !defined(__EMX__)
    #include "wx/unix/apptbase.h"
#elif defined(__WXMAC__)
    #include "wx/mac/apptbase.h"
#elif defined(__OS2__)
    #include "wx/os2/apptbase.h"
#else // no platform-specific methods to add to wxAppTraits
    // wxAppTraits must be a class because it was forward declared as class
    class WXDLLIMPEXP_BASE wxAppTraits : public wxAppTraitsBase
    {
    };
#endif // platform

// ============================================================================
// standard traits for console and GUI applications
// ============================================================================

// ----------------------------------------------------------------------------
// wxConsoleAppTraitsBase: wxAppTraits implementation for the console apps
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxConsoleAppTraitsBase : public wxAppTraits
{
public:
#if wxUSE_LOG
    virtual wxLog *CreateLogTarget();
#endif // wxUSE_LOG
    virtual wxMessageOutput *CreateMessageOutput();
#if wxUSE_FONTMAP
    virtual wxFontMapper *CreateFontMapper();
#endif // wxUSE_FONTMAP
    virtual wxRendererNative *CreateRenderer();
#if wxUSE_SOCKETS
    virtual GSocketGUIFunctionsTable* GetSocketGUIFunctionsTable();
#endif

#ifdef __WXDEBUG__
    virtual bool ShowAssertDialog(const wxString& msg);
#endif // __WXDEBUG__
    virtual bool HasStderr();

    virtual void ScheduleForDestroy(wxObject *object);
    virtual void RemoveFromPendingDelete(wxObject *object);

    // the GetToolkitVersion for console application is always the same
    virtual wxPortId GetToolkitVersion(int *verMaj, int *verMin) const
    {
        // no toolkits (wxBase is for console applications without GUI support)
        // NB: zero means "no toolkit", -1 means "not initialized yet"
        //     so we must use zero here!
        if (verMaj) *verMaj = 0;
        if (verMin) *verMin = 0;
        return wxPORT_BASE;
    }

    virtual bool IsUsingUniversalWidgets() const { return false; }
};

// ----------------------------------------------------------------------------
// wxGUIAppTraitsBase: wxAppTraits implementation for the GUI apps
// ----------------------------------------------------------------------------

#if wxUSE_GUI

class WXDLLEXPORT wxGUIAppTraitsBase : public wxAppTraits
{
public:
#if wxUSE_LOG
    virtual wxLog *CreateLogTarget();
#endif // wxUSE_LOG
    virtual wxMessageOutput *CreateMessageOutput();
#if wxUSE_FONTMAP
    virtual wxFontMapper *CreateFontMapper();
#endif // wxUSE_FONTMAP
    virtual wxRendererNative *CreateRenderer();
#if wxUSE_SOCKETS
    virtual GSocketGUIFunctionsTable* GetSocketGUIFunctionsTable();
#endif

#ifdef __WXDEBUG__
    virtual bool ShowAssertDialog(const wxString& msg);
#endif // __WXDEBUG__
    virtual bool HasStderr();

    virtual void ScheduleForDestroy(wxObject *object);
    virtual void RemoveFromPendingDelete(wxObject *object);

    virtual bool IsUsingUniversalWidgets() const
    {
    #ifdef __WXUNIVERSAL__
        return true;
    #else
        return false;
    #endif
    }
};

#endif // wxUSE_GUI

// ----------------------------------------------------------------------------
// include the platform-specific version of the classes above
// ----------------------------------------------------------------------------

// ABX: check __WIN32__ instead of __WXMSW__ for the same MSWBase in any Win32 port
#if defined(__WXPALMOS__)
    #include "wx/palmos/apptrait.h"
#elif defined(__WIN32__)
    #include "wx/msw/apptrait.h"
#elif defined(__OS2__)
    #include "wx/os2/apptrait.h"
#elif defined(__UNIX__)
    #include "wx/unix/apptrait.h"
#elif defined(__WXMAC__)
    #include "wx/mac/apptrait.h"
#elif defined(__DOS__)
    #include "wx/msdos/apptrait.h"
#else
    #if wxUSE_GUI
        class wxGUIAppTraits : public wxGUIAppTraitsBase
        {
        };
    #endif // wxUSE_GUI
    class wxConsoleAppTraits: public wxConsoleAppTraitsBase
    {
    };
#endif // platform

#endif // _WX_APPTRAIT_H_

