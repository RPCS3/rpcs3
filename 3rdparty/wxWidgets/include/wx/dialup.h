/////////////////////////////////////////////////////////////////////////////
// Name:        wx/dialup.h
// Purpose:     Network related wxWidgets classes and functions
// Author:      Vadim Zeitlin
// Modified by:
// Created:     07.07.99
// RCS-ID:      $Id: dialup.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DIALUP_H
#define _WX_DIALUP_H

#if wxUSE_DIALUP_MANAGER

#include "wx/event.h"

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_BASE wxArrayString;

#define WXDIALUP_MANAGER_DEFAULT_BEACONHOST  wxT("www.yahoo.com")

// ----------------------------------------------------------------------------
// A class which groups functions dealing with connecting to the network from a
// workstation using dial-up access to the net. There is at most one instance
// of this class in the program accessed via GetDialUpManager().
// ----------------------------------------------------------------------------

/* TODO
 *
 * 1. more configurability for Unix: i.e. how to initiate the connection, how
 *    to check for online status, &c.
 * 2. a function to enumerate all connections (ISPs) and show a dialog in
 *    Dial() allowing to choose between them if no ISP given
 * 3. add an async version of dialing functions which notify the caller about
 *    the progress (or may be even start another thread to monitor it)
 * 4. the static creation/accessor functions are not MT-safe - but is this
 *    really crucial? I think we may suppose they're always called from the
 *    main thread?
 */

class WXDLLEXPORT wxDialUpManager
{
public:
    // this function should create and return the object of the
    // platform-specific class derived from wxDialUpManager. It's implemented
    // in the platform-specific source files.
    static wxDialUpManager *Create();

    // could the dialup manager be initialized correctly? If this function
    // returns false, no other functions will work neither, so it's a good idea
    // to call this function and check its result before calling any other
    // wxDialUpManager methods
    virtual bool IsOk() const = 0;

    // virtual dtor for any base class
    virtual ~wxDialUpManager() { }

    // operations
    // ----------

    // fills the array with the names of all possible values for the first
    // parameter to Dial() on this machine and returns their number (may be 0)
    virtual size_t GetISPNames(wxArrayString& names) const = 0;

    // dial the given ISP, use username and password to authentificate
    //
    // if no nameOfISP is given, the function will select the default one
    //
    // if no username/password are given, the function will try to do without
    // them, but will ask the user if really needed
    //
    // if async parameter is false, the function waits until the end of dialing
    // and returns true upon successful completion.
    // if async is true, the function only initiates the connection and returns
    // immediately - the result is reported via events (an event is sent
    // anyhow, but if dialing failed it will be a DISCONNECTED one)
    virtual bool Dial(const wxString& nameOfISP = wxEmptyString,
                      const wxString& username = wxEmptyString,
                      const wxString& password = wxEmptyString,
                      bool async = true) = 0;

    // returns true if (async) dialing is in progress
    virtual bool IsDialing() const = 0;

    // cancel dialing the number initiated with Dial(async = true)
    // NB: this won't result in DISCONNECTED event being sent
    virtual bool CancelDialing() = 0;

    // hang up the currently active dial up connection
    virtual bool HangUp() = 0;

    // online status
    // -------------

    // returns true if the computer has a permanent network connection (i.e. is
    // on a LAN) and so there is no need to use Dial() function to go online
    //
    // NB: this functions tries to guess the result and it is not always
    //     guaranteed to be correct, so it's better to ask user for
    //     confirmation or give him a possibility to override it
    virtual bool IsAlwaysOnline() const = 0;

    // returns true if the computer is connected to the network: under Windows,
    // this just means that a RAS connection exists, under Unix we check that
    // the "well-known host" (as specified by SetWellKnownHost) is reachable
    virtual bool IsOnline() const = 0;

    // sometimes the built-in logic for determining the online status may fail,
    // so, in general, the user should be allowed to override it. This function
    // allows to forcefully set the online status - whatever our internal
    // algorithm may think about it.
    virtual void SetOnlineStatus(bool isOnline = true) = 0;

    // set misc wxDialUpManager options
    // --------------------------------

    // enable automatical checks for the connection status and sending of
    // wxEVT_DIALUP_CONNECTED/wxEVT_DIALUP_DISCONNECTED events. The interval
    // parameter is only for Unix where we do the check manually: under
    // Windows, the notification about the change of connection status is
    // instantenous.
    //
    // Returns false if couldn't set up automatic check for online status.
    virtual bool EnableAutoCheckOnlineStatus(size_t nSeconds = 60) = 0;

    // disable automatic check for connection status change - notice that the
    // wxEVT_DIALUP_XXX events won't be sent any more neither.
    virtual void DisableAutoCheckOnlineStatus() = 0;

    // additional Unix-only configuration
    // ----------------------------------

    // under Unix, the value of well-known host is used to check whether we're
    // connected to the internet. It's unused under Windows, but this function
    // is always safe to call. The default value is www.yahoo.com.
    virtual void SetWellKnownHost(const wxString& hostname,
                                  int portno = 80) = 0;

    // Sets the commands to start up the network and to hang up again. Used by
    // the Unix implementations only.
    virtual void
    SetConnectCommand(const wxString& commandDial = wxT("/usr/bin/pon"),
                      const wxString& commandHangup = wxT("/usr/bin/poff")) = 0;
};

// ----------------------------------------------------------------------------
// wxDialUpManager events
// ----------------------------------------------------------------------------

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_DIALUP_CONNECTED, 450)
    DECLARE_EVENT_TYPE(wxEVT_DIALUP_DISCONNECTED, 451)
END_DECLARE_EVENT_TYPES()

// the event class for the dialup events
class WXDLLEXPORT wxDialUpEvent : public wxEvent
{
public:
    wxDialUpEvent(bool isConnected, bool isOwnEvent) : wxEvent(isOwnEvent)
    {
        SetEventType(isConnected ? wxEVT_DIALUP_CONNECTED
                                 : wxEVT_DIALUP_DISCONNECTED);
    }

    // is this a CONNECTED or DISCONNECTED event?
    bool IsConnectedEvent() const
        { return GetEventType() == wxEVT_DIALUP_CONNECTED; }

    // does this event come from wxDialUpManager::Dial() or from some extrenal
    // process (i.e. does it result from our own attempt to establish the
    // connection)?
    bool IsOwnEvent() const { return m_id != 0; }

    // implement the base class pure virtual
    virtual wxEvent *Clone() const { return new wxDialUpEvent(*this); }

private:
    DECLARE_NO_ASSIGN_CLASS(wxDialUpEvent)
};

// the type of dialup event handler function
typedef void (wxEvtHandler::*wxDialUpEventFunction)(wxDialUpEvent&);

#define wxDialUpEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxDialUpEventFunction, &func)

// macros to catch dialup events
#define EVT_DIALUP_CONNECTED(func) \
    wx__DECLARE_EVT0(wxEVT_DIALUP_CONNECTED, wxDialUpEventHandler(func))
#define EVT_DIALUP_DISCONNECTED(func) \
    wx__DECLARE_EVT0(wxEVT_DIALUP_DISCONNECTED, wxDialUpEventHandler(func))


#endif // wxUSE_DIALUP_MANAGER

#endif // _WX_DIALUP_H
