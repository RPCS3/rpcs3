/////////////////////////////////////////////////////////////////////////////
// Name:        wx/taskbar.h
// Purpose:     wxTaskBarIcon base header and class
// Author:      Julian Smart
// Modified by:
// Created:
// Copyright:   (c) Julian Smart
// RCS-ID:      $Id: taskbar.h 53135 2008-04-12 02:31:04Z VZ $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TASKBAR_H_BASE_
#define _WX_TASKBAR_H_BASE_

#include "wx/defs.h"

#ifdef wxHAS_TASK_BAR_ICON

#include "wx/event.h"

class WXDLLIMPEXP_FWD_ADV wxTaskBarIconEvent;

// ----------------------------------------------------------------------------
// wxTaskBarIconBase: define wxTaskBarIcon interface
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxTaskBarIconBase : public wxEvtHandler
{
public:
    wxTaskBarIconBase() { }

    // Operations:
    virtual bool SetIcon(const wxIcon& icon,
                         const wxString& tooltip = wxEmptyString) = 0;
    virtual bool RemoveIcon() = 0;
    virtual bool PopupMenu(wxMenu *menu) = 0;

protected:
    // creates menu to be displayed when user clicks on the icon
    virtual wxMenu *CreatePopupMenu() { return NULL; }

private:
    // default events handling, calls CreatePopupMenu:
    void OnRightButtonDown(wxTaskBarIconEvent& event);

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxTaskBarIconBase)
};


// ----------------------------------------------------------------------------
// now include the actual class declaration
// ----------------------------------------------------------------------------

#if defined(__WXPALMOS__)
    #include "wx/palmos/taskbar.h"
#elif defined(__WXMSW__)
    #include "wx/msw/taskbar.h"
#elif defined(__WXGTK__) || defined(__WXX11__) || defined(__WXMOTIF__)
    #include "wx/unix/taskbarx11.h"
#elif defined (__WXMAC__) && defined(__WXMAC_OSX__)
    #include "wx/mac/taskbarosx.h"
#elif defined (__WXCOCOA__)
    #include "wx/cocoa/taskbar.h"
#endif

// ----------------------------------------------------------------------------
// wxTaskBarIcon events
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxTaskBarIconEvent : public wxEvent
{
public:
    wxTaskBarIconEvent(wxEventType evtType, wxTaskBarIcon *tbIcon)
        : wxEvent(wxID_ANY, evtType)
    {
        SetEventObject(tbIcon);
    }

    virtual wxEvent *Clone() const { return new wxTaskBarIconEvent(*this); }

private:
    DECLARE_NO_ASSIGN_CLASS(wxTaskBarIconEvent)
};

typedef void (wxEvtHandler::*wxTaskBarIconEventFunction)(wxTaskBarIconEvent&);

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV,wxEVT_TASKBAR_MOVE,1550)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV,wxEVT_TASKBAR_LEFT_DOWN,1551)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV,wxEVT_TASKBAR_LEFT_UP,1552)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV,wxEVT_TASKBAR_RIGHT_DOWN,1553)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV,wxEVT_TASKBAR_RIGHT_UP,1554)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV,wxEVT_TASKBAR_LEFT_DCLICK,1555)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV,wxEVT_TASKBAR_RIGHT_DCLICK,1556)
END_DECLARE_EVENT_TYPES()

#define wxTaskBarIconEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxTaskBarIconEventFunction, &func)

#define wx__DECLARE_TASKBAREVT(evt, fn) \
    wx__DECLARE_EVT0(wxEVT_TASKBAR_ ## evt, wxTaskBarIconEventHandler(fn))

#define EVT_TASKBAR_MOVE(fn)         wx__DECLARE_TASKBAREVT(MOVE, fn)
#define EVT_TASKBAR_LEFT_DOWN(fn)    wx__DECLARE_TASKBAREVT(LEFT_DOWN, fn)
#define EVT_TASKBAR_LEFT_UP(fn)      wx__DECLARE_TASKBAREVT(LEFT_UP, fn)
#define EVT_TASKBAR_RIGHT_DOWN(fn)   wx__DECLARE_TASKBAREVT(RIGHT_DOWN, fn)
#define EVT_TASKBAR_RIGHT_UP(fn)     wx__DECLARE_TASKBAREVT(RIGHT_UP, fn)
#define EVT_TASKBAR_LEFT_DCLICK(fn)  wx__DECLARE_TASKBAREVT(LEFT_DCLICK, fn)
#define EVT_TASKBAR_RIGHT_DCLICK(fn) wx__DECLARE_TASKBAREVT(RIGHT_DCLICK, fn)

// taskbar menu is shown on right button press under all platforms except MSW
// where it's shown on right button release, using this event type and macro
// allows to write code which works correctly on all platforms
#ifdef __WXMSW__
    #define wxEVT_TASKBAR_CLICK wxEVT_TASKBAR_RIGHT_UP
#else
    #define wxEVT_TASKBAR_CLICK wxEVT_TASKBAR_RIGHT_DOWN
#endif
#define EVT_TASKBAR_CLICK(fn)        wx__DECLARE_TASKBAREVT(CLICK, fn)

#endif // wxHAS_TASK_BAR_ICON

#endif // _WX_TASKBAR_H_BASE_
