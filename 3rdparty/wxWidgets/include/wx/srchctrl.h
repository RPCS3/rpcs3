/////////////////////////////////////////////////////////////////////////////
// Name:        srchctrl.h
// Purpose:     wxSearchCtrlBase class
// Author:      Vince Harron
// Created:     2006-02-18
// RCS-ID:      $Id: srchctrl.h 45828 2007-05-05 14:51:51Z VZ $
// Copyright:   (c) Vince Harron
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SEARCHCTRL_H_BASE_
#define _WX_SEARCHCTRL_H_BASE_

#include "wx/defs.h"

#if wxUSE_SEARCHCTRL

#include "wx/textctrl.h"

#if !defined(__WXUNIVERSAL__) && defined(__WXMAC__) && defined(__WXMAC_OSX__) \
        && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3)
    // search control was introduced in Mac OS X 10.3 Panther
    #define wxUSE_NATIVE_SEARCH_CONTROL 1

    #define wxSearchCtrlBaseBaseClass wxTextCtrl
#else
    // no native version, use the generic one
    #define wxUSE_NATIVE_SEARCH_CONTROL 0

    #define wxSearchCtrlBaseBaseClass wxTextCtrlBase
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

extern WXDLLEXPORT_DATA(const wxChar) wxSearchCtrlNameStr[];

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, 1119)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, 1120)
END_DECLARE_EVENT_TYPES()

// ----------------------------------------------------------------------------
// a search ctrl is a text control with a search button and a cancel button
// it is based on the MacOSX 10.3 control HISearchFieldCreate
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxSearchCtrlBase : public wxSearchCtrlBaseBaseClass
{
public:
    wxSearchCtrlBase() { }
    virtual ~wxSearchCtrlBase() { }

    // search control
#if wxUSE_MENUS
    virtual void SetMenu(wxMenu *menu) = 0;
    virtual wxMenu *GetMenu() = 0;
#endif // wxUSE_MENUS

    // get/set options
    virtual void ShowSearchButton( bool show ) = 0;
    virtual bool IsSearchButtonVisible() const = 0;

    virtual void ShowCancelButton( bool show ) = 0;
    virtual bool IsCancelButtonVisible() const = 0;
};


// include the platform-dependent class implementation
#if wxUSE_NATIVE_SEARCH_CONTROL
    #if defined(__WXMAC__)
        #include "wx/mac/srchctrl.h"
    #endif
#else
    #include "wx/generic/srchctlg.h"
#endif

// ----------------------------------------------------------------------------
// macros for handling search events
// ----------------------------------------------------------------------------

#define EVT_SEARCHCTRL_CANCEL_BTN(id, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_SEARCHCTRL_CANCEL_BTN, id, wxCommandEventHandler(fn))

#define EVT_SEARCHCTRL_SEARCH_BTN(id, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_SEARCHCTRL_SEARCH_BTN, id, wxCommandEventHandler(fn))

#endif // wxUSE_SEARCHCTRL

#endif // _WX_SEARCHCTRL_H_BASE_
