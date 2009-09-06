/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/event.cpp
// Purpose:     Event classes
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: event.cpp 56712 2008-11-08 22:41:10Z VZ $
// Copyright:   (c) Julian Smart
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

#include "wx/event.h"

#ifndef WX_PRECOMP
    #include "wx/list.h"
    #include "wx/app.h"
    #include "wx/utils.h"
    #include "wx/stopwatch.h"
    #include "wx/module.h"

    #if wxUSE_GUI
        #include "wx/window.h"
        #include "wx/control.h"
        #include "wx/dc.h"
        #include "wx/textctrl.h"
        #include "wx/validate.h"
    #endif // wxUSE_GUI
#endif

#if wxUSE_BASE
    #include "wx/ptr_scpd.h"

    wxDECLARE_SCOPED_PTR(wxEvent, wxEventPtr)
    wxDEFINE_SCOPED_PTR(wxEvent, wxEventPtr)
#endif // wxUSE_BASE

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

#if wxUSE_BASE
    IMPLEMENT_DYNAMIC_CLASS(wxEvtHandler, wxObject)
    IMPLEMENT_ABSTRACT_CLASS(wxEvent, wxObject)
#endif // wxUSE_BASE

#if wxUSE_GUI
    IMPLEMENT_DYNAMIC_CLASS(wxIdleEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxCommandEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxNotifyEvent, wxCommandEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxScrollEvent, wxCommandEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxScrollWinEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxMouseEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxKeyEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxSizeEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxPaintEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxNcPaintEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxEraseEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxMoveEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxFocusEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxChildFocusEvent, wxCommandEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxCloseEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxShowEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxMaximizeEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxIconizeEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxMenuEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxJoystickEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxDropFilesEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxActivateEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxInitDialogEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxSetCursorEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxSysColourChangedEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxDisplayChangedEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxUpdateUIEvent, wxCommandEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxNavigationKeyEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxPaletteChangedEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxQueryNewPaletteEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxWindowCreateEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxWindowDestroyEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxHelpEvent, wxCommandEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxContextMenuEvent, wxCommandEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxMouseCaptureChangedEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxMouseCaptureLostEvent, wxEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxClipboardTextEvent, wxCommandEvent)
#endif // wxUSE_GUI

#if wxUSE_BASE

const wxEventTable *wxEvtHandler::GetEventTable() const
    { return &wxEvtHandler::sm_eventTable; }

const wxEventTable wxEvtHandler::sm_eventTable =
    { (const wxEventTable *)NULL, &wxEvtHandler::sm_eventTableEntries[0] };

wxEventHashTable &wxEvtHandler::GetEventHashTable() const
    { return wxEvtHandler::sm_eventHashTable; }

wxEventHashTable wxEvtHandler::sm_eventHashTable(wxEvtHandler::sm_eventTable);

const wxEventTableEntry wxEvtHandler::sm_eventTableEntries[] =
    { DECLARE_EVENT_TABLE_ENTRY(wxEVT_NULL, 0, 0, (wxObjectEventFunction)NULL, NULL) };


#ifdef __WXDEBUG__
// Clear up event hash table contents or we can get problems
// when C++ is cleaning up the static object
class wxEventTableEntryModule: public wxModule
{
DECLARE_DYNAMIC_CLASS(wxEventTableEntryModule)
public:
    wxEventTableEntryModule() {}
    bool OnInit()
    {
        wxEventHashTable::ReconstructAll();
        return true;
    }
    void OnExit()
    {
        wxEventHashTable::ClearAll();
    }
};
IMPLEMENT_DYNAMIC_CLASS(wxEventTableEntryModule, wxModule)
#endif

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

// To put pending event handlers
wxList *wxPendingEvents = (wxList *)NULL;

#if wxUSE_THREADS
    // protects wxPendingEvents list
    wxCriticalSection *wxPendingEventsLocker = (wxCriticalSection *)NULL;
#endif

#if !WXWIN_COMPATIBILITY_EVENT_TYPES

// common event types are defined here, other event types are defined by the
// components which use them

const wxEventType wxEVT_FIRST = 10000;
const wxEventType wxEVT_USER_FIRST = wxEVT_FIRST + 2000;

DEFINE_EVENT_TYPE(wxEVT_NULL)
DEFINE_EVENT_TYPE(wxEVT_IDLE)
DEFINE_EVENT_TYPE(wxEVT_SOCKET)

#endif // !WXWIN_COMPATIBILITY_EVENT_TYPES

#endif // wxUSE_BASE

#if wxUSE_GUI

#if !WXWIN_COMPATIBILITY_EVENT_TYPES

DEFINE_EVENT_TYPE(wxEVT_COMMAND_BUTTON_CLICKED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_CHECKBOX_CLICKED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_CHOICE_SELECTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LISTBOX_SELECTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LISTBOX_DOUBLECLICKED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_CHECKLISTBOX_TOGGLED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_MENU_SELECTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_SLIDER_UPDATED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RADIOBOX_SELECTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RADIOBUTTON_SELECTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_SCROLLBAR_UPDATED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_VLBOX_SELECTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_COMBOBOX_SELECTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TOOL_RCLICKED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TOOL_ENTER)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_SPINCTRL_UPDATED)

// Sockets and timers send events, too
DEFINE_EVENT_TYPE(wxEVT_TIMER)

// Mouse event types
DEFINE_EVENT_TYPE(wxEVT_LEFT_DOWN)
DEFINE_EVENT_TYPE(wxEVT_LEFT_UP)
DEFINE_EVENT_TYPE(wxEVT_MIDDLE_DOWN)
DEFINE_EVENT_TYPE(wxEVT_MIDDLE_UP)
DEFINE_EVENT_TYPE(wxEVT_RIGHT_DOWN)
DEFINE_EVENT_TYPE(wxEVT_RIGHT_UP)
DEFINE_EVENT_TYPE(wxEVT_MOTION)
DEFINE_EVENT_TYPE(wxEVT_ENTER_WINDOW)
DEFINE_EVENT_TYPE(wxEVT_LEAVE_WINDOW)
DEFINE_EVENT_TYPE(wxEVT_LEFT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_MIDDLE_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_RIGHT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_SET_FOCUS)
DEFINE_EVENT_TYPE(wxEVT_KILL_FOCUS)
DEFINE_EVENT_TYPE(wxEVT_CHILD_FOCUS)
DEFINE_EVENT_TYPE(wxEVT_MOUSEWHEEL)

// Non-client mouse events
DEFINE_EVENT_TYPE(wxEVT_NC_LEFT_DOWN)
DEFINE_EVENT_TYPE(wxEVT_NC_LEFT_UP)
DEFINE_EVENT_TYPE(wxEVT_NC_MIDDLE_DOWN)
DEFINE_EVENT_TYPE(wxEVT_NC_MIDDLE_UP)
DEFINE_EVENT_TYPE(wxEVT_NC_RIGHT_DOWN)
DEFINE_EVENT_TYPE(wxEVT_NC_RIGHT_UP)
DEFINE_EVENT_TYPE(wxEVT_NC_MOTION)
DEFINE_EVENT_TYPE(wxEVT_NC_ENTER_WINDOW)
DEFINE_EVENT_TYPE(wxEVT_NC_LEAVE_WINDOW)
DEFINE_EVENT_TYPE(wxEVT_NC_LEFT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_NC_MIDDLE_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_NC_RIGHT_DCLICK)

// Character input event type
DEFINE_EVENT_TYPE(wxEVT_CHAR)
DEFINE_EVENT_TYPE(wxEVT_CHAR_HOOK)
DEFINE_EVENT_TYPE(wxEVT_NAVIGATION_KEY)
DEFINE_EVENT_TYPE(wxEVT_KEY_DOWN)
DEFINE_EVENT_TYPE(wxEVT_KEY_UP)
#if wxUSE_HOTKEY
DEFINE_EVENT_TYPE(wxEVT_HOTKEY)
#endif

// Set cursor event
DEFINE_EVENT_TYPE(wxEVT_SET_CURSOR)

// wxScrollbar and wxSlider event identifiers
DEFINE_EVENT_TYPE(wxEVT_SCROLL_TOP)
DEFINE_EVENT_TYPE(wxEVT_SCROLL_BOTTOM)
DEFINE_EVENT_TYPE(wxEVT_SCROLL_LINEUP)
DEFINE_EVENT_TYPE(wxEVT_SCROLL_LINEDOWN)
DEFINE_EVENT_TYPE(wxEVT_SCROLL_PAGEUP)
DEFINE_EVENT_TYPE(wxEVT_SCROLL_PAGEDOWN)
DEFINE_EVENT_TYPE(wxEVT_SCROLL_THUMBTRACK)
DEFINE_EVENT_TYPE(wxEVT_SCROLL_THUMBRELEASE)
DEFINE_EVENT_TYPE(wxEVT_SCROLL_CHANGED)

// Scroll events from wxWindow
DEFINE_EVENT_TYPE(wxEVT_SCROLLWIN_TOP)
DEFINE_EVENT_TYPE(wxEVT_SCROLLWIN_BOTTOM)
DEFINE_EVENT_TYPE(wxEVT_SCROLLWIN_LINEUP)
DEFINE_EVENT_TYPE(wxEVT_SCROLLWIN_LINEDOWN)
DEFINE_EVENT_TYPE(wxEVT_SCROLLWIN_PAGEUP)
DEFINE_EVENT_TYPE(wxEVT_SCROLLWIN_PAGEDOWN)
DEFINE_EVENT_TYPE(wxEVT_SCROLLWIN_THUMBTRACK)
DEFINE_EVENT_TYPE(wxEVT_SCROLLWIN_THUMBRELEASE)

// System events
DEFINE_EVENT_TYPE(wxEVT_SIZE)
DEFINE_EVENT_TYPE(wxEVT_SIZING)
DEFINE_EVENT_TYPE(wxEVT_MOVE)
DEFINE_EVENT_TYPE(wxEVT_MOVING)
DEFINE_EVENT_TYPE(wxEVT_CLOSE_WINDOW)
DEFINE_EVENT_TYPE(wxEVT_END_SESSION)
DEFINE_EVENT_TYPE(wxEVT_QUERY_END_SESSION)
DEFINE_EVENT_TYPE(wxEVT_HIBERNATE)
DEFINE_EVENT_TYPE(wxEVT_ACTIVATE_APP)
DEFINE_EVENT_TYPE(wxEVT_POWER)
DEFINE_EVENT_TYPE(wxEVT_ACTIVATE)
DEFINE_EVENT_TYPE(wxEVT_CREATE)
DEFINE_EVENT_TYPE(wxEVT_DESTROY)
DEFINE_EVENT_TYPE(wxEVT_SHOW)
DEFINE_EVENT_TYPE(wxEVT_ICONIZE)
DEFINE_EVENT_TYPE(wxEVT_MAXIMIZE)
DEFINE_EVENT_TYPE(wxEVT_MOUSE_CAPTURE_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_MOUSE_CAPTURE_LOST)
DEFINE_EVENT_TYPE(wxEVT_PAINT)
DEFINE_EVENT_TYPE(wxEVT_ERASE_BACKGROUND)
DEFINE_EVENT_TYPE(wxEVT_NC_PAINT)
DEFINE_EVENT_TYPE(wxEVT_PAINT_ICON)
DEFINE_EVENT_TYPE(wxEVT_MENU_OPEN)
DEFINE_EVENT_TYPE(wxEVT_MENU_CLOSE)
DEFINE_EVENT_TYPE(wxEVT_MENU_HIGHLIGHT)
DEFINE_EVENT_TYPE(wxEVT_CONTEXT_MENU)
DEFINE_EVENT_TYPE(wxEVT_SYS_COLOUR_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_DISPLAY_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_SETTING_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_QUERY_NEW_PALETTE)
DEFINE_EVENT_TYPE(wxEVT_PALETTE_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_JOY_BUTTON_DOWN)
DEFINE_EVENT_TYPE(wxEVT_JOY_BUTTON_UP)
DEFINE_EVENT_TYPE(wxEVT_JOY_MOVE)
DEFINE_EVENT_TYPE(wxEVT_JOY_ZMOVE)
DEFINE_EVENT_TYPE(wxEVT_DROP_FILES)
DEFINE_EVENT_TYPE(wxEVT_DRAW_ITEM)
DEFINE_EVENT_TYPE(wxEVT_MEASURE_ITEM)
DEFINE_EVENT_TYPE(wxEVT_COMPARE_ITEM)
DEFINE_EVENT_TYPE(wxEVT_INIT_DIALOG)
DEFINE_EVENT_TYPE(wxEVT_UPDATE_UI)

// Clipboard events
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TEXT_COPY)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TEXT_CUT)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TEXT_PASTE)

// Generic command events
// Note: a click is a higher-level event than button down/up
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LEFT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LEFT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_RIGHT_DCLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_SET_FOCUS)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_KILL_FOCUS)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_ENTER)

// Help events
DEFINE_EVENT_TYPE(wxEVT_HELP)
DEFINE_EVENT_TYPE(wxEVT_DETAILED_HELP)

#endif // !WXWIN_COMPATIBILITY_EVENT_TYPES

#endif // wxUSE_GUI

#if wxUSE_BASE

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// event initialization
// ----------------------------------------------------------------------------

int wxNewEventType()
{
    // MT-FIXME
    static int s_lastUsedEventType = wxEVT_FIRST;

    return s_lastUsedEventType++;
}

// ----------------------------------------------------------------------------
// wxEvent
// ----------------------------------------------------------------------------

/*
 * General wxWidgets events, covering
 * all interesting things that might happen (button clicking, resizing,
 * setting text in widgets, etc.).
 *
 * For each completely new event type, derive a new event class.
 *
 */

wxEvent::wxEvent(int theId, wxEventType commandType )
{
    m_eventType = commandType;
    m_eventObject = (wxObject *) NULL;
    m_timeStamp = 0;
    m_id = theId;
    m_skipped = false;
    m_callbackUserData = (wxObject *) NULL;
    m_isCommandEvent = false;
    m_propagationLevel = wxEVENT_PROPAGATE_NONE;
}

wxEvent::wxEvent(const wxEvent &src)
    : wxObject(src)
    , m_eventObject(src.m_eventObject)
    , m_eventType(src.m_eventType)
    , m_timeStamp(src.m_timeStamp)
    , m_id(src.m_id)
    , m_callbackUserData(src.m_callbackUserData)
    , m_propagationLevel(src.m_propagationLevel)
    , m_skipped(src.m_skipped)
    , m_isCommandEvent(src.m_isCommandEvent)
{
}

#endif // wxUSE_BASE

#if wxUSE_GUI

/*
 * Command events
 *
 */

#ifdef __VISUALC__
    // 'this' : used in base member initializer list (for m_commandString)
    #pragma warning(disable:4355)
#endif

wxCommandEvent::wxCommandEvent(wxEventType commandType, int theId)
              : wxEvent(theId, commandType)
#if WXWIN_COMPATIBILITY_2_4
              , m_commandString(this)
#endif
{
    m_clientData = (char *) NULL;
    m_clientObject = (wxClientData *) NULL;
    m_extraLong = 0;
    m_commandInt = 0;
    m_isCommandEvent = true;

    // the command events are propagated upwards by default
    m_propagationLevel = wxEVENT_PROPAGATE_MAX;
}

#ifdef __VISUALC__
    #pragma warning(default:4355)
#endif

wxString wxCommandEvent::GetString() const
{
    if(m_eventType != wxEVT_COMMAND_TEXT_UPDATED || !m_eventObject)
        return m_cmdString;
    else
    {
#if wxUSE_TEXTCTRL
        wxTextCtrl *txt = wxDynamicCast(m_eventObject, wxTextCtrl);
        if(txt)
            return txt->GetValue();
        else
#endif // wxUSE_TEXTCTRL
            return m_cmdString;
    }
}

/*
 * UI update events
 */

#if wxUSE_LONGLONG
wxLongLong wxUpdateUIEvent::sm_lastUpdate = 0;
#endif

long wxUpdateUIEvent::sm_updateInterval = 0;

wxUpdateUIMode wxUpdateUIEvent::sm_updateMode = wxUPDATE_UI_PROCESS_ALL;

// Can we update?
bool wxUpdateUIEvent::CanUpdate(wxWindowBase *win)
{
    // Don't update if we've switched global updating off
    // and this window doesn't support updates.
    if (win &&
       (GetMode() == wxUPDATE_UI_PROCESS_SPECIFIED &&
       ((win->GetExtraStyle() & wxWS_EX_PROCESS_UI_UPDATES) == 0)))
        return false;

    if (sm_updateInterval == -1)
        return false;

    if (sm_updateInterval == 0)
        return true;

#if wxUSE_STOPWATCH && wxUSE_LONGLONG
    wxLongLong now = wxGetLocalTimeMillis();
    if (now > (sm_lastUpdate + sm_updateInterval))
    {
        return true;
    }

    return false;
#else
    // If we don't have wxStopWatch or wxLongLong, we
    // should err on the safe side and update now anyway.
    return true;
#endif
}

// Reset the update time to provide a delay until the next
// time we should update
void wxUpdateUIEvent::ResetUpdateTime()
{
#if wxUSE_STOPWATCH && wxUSE_LONGLONG
    if (sm_updateInterval > 0)
    {
        wxLongLong now = wxGetLocalTimeMillis();
        if (now > (sm_lastUpdate + sm_updateInterval))
        {
            sm_lastUpdate = now;
        }
    }
#endif
}

/*
 * Idle events
 */

wxIdleMode wxIdleEvent::sm_idleMode = wxIDLE_PROCESS_ALL;

// Can we send an idle event?
bool wxIdleEvent::CanSend(wxWindow* win)
{
    // Don't update if we've switched global updating off
    // and this window doesn't support updates.
    if (win &&
       (GetMode() == wxIDLE_PROCESS_SPECIFIED &&
       ((win->GetExtraStyle() & wxWS_EX_PROCESS_IDLE) == 0)))
        return false;

    return true;
}

/*
 * Scroll events
 */

wxScrollEvent::wxScrollEvent(wxEventType commandType,
                             int id,
                             int pos,
                             int orient)
    : wxCommandEvent(commandType, id)
{
    m_extraLong = orient;
    m_commandInt = pos;
}

/*
 * ScrollWin events
 */

wxScrollWinEvent::wxScrollWinEvent(wxEventType commandType,
                                   int pos,
                                   int orient)
{
    m_eventType = commandType;
    m_extraLong = orient;
    m_commandInt = pos;
}

/*
 * Mouse events
 *
 */

wxMouseEvent::wxMouseEvent(wxEventType commandType)
{
    m_eventType = commandType;
    m_metaDown = false;
    m_altDown = false;
    m_controlDown = false;
    m_shiftDown = false;
    m_leftDown = false;
    m_rightDown = false;
    m_middleDown = false;
    m_x = 0;
    m_y = 0;
    m_wheelRotation = 0;
    m_wheelDelta = 0;
    m_linesPerAction = 0;
}

void wxMouseEvent::Assign(const wxMouseEvent& event)
{
    m_eventType = event.m_eventType;

    m_x = event.m_x;
    m_y = event.m_y;

    m_leftDown = event.m_leftDown;
    m_middleDown = event.m_middleDown;
    m_rightDown = event.m_rightDown;

    m_controlDown = event.m_controlDown;
    m_shiftDown = event.m_shiftDown;
    m_altDown = event.m_altDown;
    m_metaDown = event.m_metaDown;

    m_wheelRotation = event.m_wheelRotation;
    m_wheelDelta = event.m_wheelDelta;
    m_linesPerAction = event.m_linesPerAction;
}

// return true if was a button dclick event
bool wxMouseEvent::ButtonDClick(int but) const
{
    switch (but)
    {
        default:
            wxFAIL_MSG(wxT("invalid parameter in wxMouseEvent::ButtonDClick"));
            // fall through

        case wxMOUSE_BTN_ANY:
            return (LeftDClick() || MiddleDClick() || RightDClick());

        case wxMOUSE_BTN_LEFT:
            return LeftDClick();

        case wxMOUSE_BTN_MIDDLE:
            return MiddleDClick();

        case wxMOUSE_BTN_RIGHT:
            return RightDClick();
    }
}

// return true if was a button down event
bool wxMouseEvent::ButtonDown(int but) const
{
    switch (but)
    {
        default:
            wxFAIL_MSG(wxT("invalid parameter in wxMouseEvent::ButtonDown"));
            // fall through

        case wxMOUSE_BTN_ANY:
            return (LeftDown() || MiddleDown() || RightDown());

        case wxMOUSE_BTN_LEFT:
            return LeftDown();

        case wxMOUSE_BTN_MIDDLE:
            return MiddleDown();

        case wxMOUSE_BTN_RIGHT:
            return RightDown();
    }
}

// return true if was a button up event
bool wxMouseEvent::ButtonUp(int but) const
{
    switch (but)
    {
        default:
            wxFAIL_MSG(wxT("invalid parameter in wxMouseEvent::ButtonUp"));
            // fall through

        case wxMOUSE_BTN_ANY:
            return (LeftUp() || MiddleUp() || RightUp());

        case wxMOUSE_BTN_LEFT:
            return LeftUp();

        case wxMOUSE_BTN_MIDDLE:
            return MiddleUp();

        case wxMOUSE_BTN_RIGHT:
            return RightUp();
    }
}

// return true if the given button is currently changing state
bool wxMouseEvent::Button(int but) const
{
    switch (but)
    {
        default:
            wxFAIL_MSG(wxT("invalid parameter in wxMouseEvent::Button"));
            // fall through

        case wxMOUSE_BTN_ANY:
            return ButtonUp(wxMOUSE_BTN_ANY) ||
                    ButtonDown(wxMOUSE_BTN_ANY) ||
                        ButtonDClick(wxMOUSE_BTN_ANY);

        case wxMOUSE_BTN_LEFT:
            return LeftDown() || LeftUp() || LeftDClick();

        case wxMOUSE_BTN_MIDDLE:
            return MiddleDown() || MiddleUp() || MiddleDClick();

        case wxMOUSE_BTN_RIGHT:
            return RightDown() || RightUp() || RightDClick();
    }
}

bool wxMouseEvent::ButtonIsDown(int but) const
{
    switch (but)
    {
        default:
            wxFAIL_MSG(wxT("invalid parameter in wxMouseEvent::ButtonIsDown"));
            // fall through

        case wxMOUSE_BTN_ANY:
            return LeftIsDown() || MiddleIsDown() || RightIsDown();

        case wxMOUSE_BTN_LEFT:
            return LeftIsDown();

        case wxMOUSE_BTN_MIDDLE:
            return MiddleIsDown();

        case wxMOUSE_BTN_RIGHT:
            return RightIsDown();
    }
}

int wxMouseEvent::GetButton() const
{
    for ( int i = 1; i <= 3; i++ )
    {
        if ( Button(i) )
        {
            return i;
        }
    }

    return wxMOUSE_BTN_NONE;
}

// Find the logical position of the event given the DC
wxPoint wxMouseEvent::GetLogicalPosition(const wxDC& dc) const
{
    wxPoint pt(dc.DeviceToLogicalX(m_x), dc.DeviceToLogicalY(m_y));
    return pt;
}


/*
 * Keyboard event
 *
 */

wxKeyEvent::wxKeyEvent(wxEventType type)
{
    m_eventType = type;
    m_shiftDown = false;
    m_controlDown = false;
    m_metaDown = false;
    m_altDown = false;
    m_keyCode = 0;
    m_scanCode = 0;
#if wxUSE_UNICODE
    m_uniChar = 0;
#endif
}

wxKeyEvent::wxKeyEvent(const wxKeyEvent& evt)
    : wxEvent(evt)
{
    m_x = evt.m_x;
    m_y = evt.m_y;

    m_keyCode = evt.m_keyCode;

    m_controlDown = evt.m_controlDown;
    m_shiftDown = evt.m_shiftDown;
    m_altDown = evt.m_altDown;
    m_metaDown = evt.m_metaDown;
    m_scanCode = evt.m_scanCode;
    m_rawCode = evt.m_rawCode;
    m_rawFlags = evt.m_rawFlags;

#if wxUSE_UNICODE
    m_uniChar = evt.m_uniChar;
#endif
}

#if WXWIN_COMPATIBILITY_2_6
long wxKeyEvent::KeyCode() const
{
    return m_keyCode;
}
#endif // WXWIN_COMPATIBILITY_2_6

wxWindowCreateEvent::wxWindowCreateEvent(wxWindow *win)
{
    SetEventType(wxEVT_CREATE);
    SetEventObject(win);
}

wxWindowDestroyEvent::wxWindowDestroyEvent(wxWindow *win)
{
    SetEventType(wxEVT_DESTROY);
    SetEventObject(win);
}

wxChildFocusEvent::wxChildFocusEvent(wxWindow *win)
                 : wxCommandEvent(wxEVT_CHILD_FOCUS)
{
    SetEventObject(win);
}

// ----------------------------------------------------------------------------
// wxHelpEvent
// ----------------------------------------------------------------------------

/* static */
wxHelpEvent::Origin wxHelpEvent::GuessOrigin(Origin origin)
{
    if ( origin == Origin_Unknown )
    {
        // assume that the event comes from the help button if it's not from
        // keyboard and that pressing F1 always results in the help event
        origin = wxGetKeyState(WXK_F1) ? Origin_Keyboard : Origin_HelpButton;
    }

    return origin;
}

#endif // wxUSE_GUI


#if wxUSE_BASE

// ----------------------------------------------------------------------------
// wxEventHashTable
// ----------------------------------------------------------------------------

static const int EVENT_TYPE_TABLE_INIT_SIZE = 31; // Not too big not too small...

wxEventHashTable* wxEventHashTable::sm_first = NULL;

wxEventHashTable::wxEventHashTable(const wxEventTable &table)
                : m_table(table),
                  m_rebuildHash(true)
{
    AllocEventTypeTable(EVENT_TYPE_TABLE_INIT_SIZE);

    m_next = sm_first;
    if (m_next)
        m_next->m_previous = this;
    sm_first = this;
}

wxEventHashTable::~wxEventHashTable()
{
    if (m_next)
        m_next->m_previous = m_previous;
    if (m_previous)
        m_previous->m_next = m_next;
    if (sm_first == this)
        sm_first = m_next;

    Clear();
}

void wxEventHashTable::Clear()
{
    size_t i;
    for(i = 0; i < m_size; i++)
    {
        EventTypeTablePointer  eTTnode = m_eventTypeTable[i];
        if (eTTnode)
        {
            delete eTTnode;
        }
    }

    // Necessary in order to not invoke the
    // overloaded delete operator when statics are cleaned up
    if (m_eventTypeTable)
        delete[] m_eventTypeTable;

    m_eventTypeTable = NULL;
    m_size = 0;
}

// Clear all tables
void wxEventHashTable::ClearAll()
{
    wxEventHashTable* table = sm_first;
    while (table)
    {
        table->Clear();
        table = table->m_next;
    }
}

// Rebuild all tables if they were cleared by ClearAll
void wxEventHashTable::ReconstructAll()
{
    wxEventHashTable* table = sm_first;
    while (table)
    {
        // This will only be true if the event table was cleared.
        // What we do here is basically what the constructor does.
        if(table->m_eventTypeTable == NULL)
        {
            table->AllocEventTypeTable(EVENT_TYPE_TABLE_INIT_SIZE);
            table->m_rebuildHash = true;
        }
        table = table->m_next;
    }
}

bool wxEventHashTable::HandleEvent(wxEvent &event, wxEvtHandler *self)
{
    if (m_rebuildHash)
    {
        InitHashTable();
        m_rebuildHash = false;
    }

    if (!m_eventTypeTable)
        return false;

    // Find all entries for the given event type.
    wxEventType eventType = event.GetEventType();
    const EventTypeTablePointer eTTnode = m_eventTypeTable[eventType % m_size];
    if (eTTnode && eTTnode->eventType == eventType)
    {
        // Now start the search for an event handler
        // that can handle an event with the given ID.
        const wxEventTableEntryPointerArray&
            eventEntryTable = eTTnode->eventEntryTable;

        const size_t count = eventEntryTable.GetCount();
        for (size_t n = 0; n < count; n++)
        {
            if ( wxEvtHandler::
                    ProcessEventIfMatches(*eventEntryTable[n], self, event) )
            {
                return true;
            }
        }
    }

    return false;
}

void wxEventHashTable::InitHashTable()
{
    // Loop over the event tables and all its base tables.
    const wxEventTable *table = &m_table;
    while (table)
    {
        // Retrieve all valid event handler entries
        const wxEventTableEntry *entry = table->entries;
        while (entry->m_fn != 0)
        {
            // Add the event entry in the Hash.
            AddEntry(*entry);

            entry++;
        }

        table = table->baseTable;
    }

    // Lets free some memory.
    size_t i;
    for(i = 0; i < m_size; i++)
    {
        EventTypeTablePointer  eTTnode = m_eventTypeTable[i];
        if (eTTnode)
        {
            eTTnode->eventEntryTable.Shrink();
        }
    }
}

void wxEventHashTable::AddEntry(const wxEventTableEntry &entry)
{
    // This might happen 'accidentally' as the app is exiting
    if (!m_eventTypeTable)
        return;

    EventTypeTablePointer *peTTnode = &m_eventTypeTable[entry.m_eventType % m_size];
    EventTypeTablePointer  eTTnode = *peTTnode;

    if (eTTnode)
    {
        if (eTTnode->eventType != entry.m_eventType)
        {
            // Resize the table!
            GrowEventTypeTable();
            // Try again to add it.
            AddEntry(entry);
            return;
        }
    }
    else
    {
        eTTnode = new EventTypeTable;
        eTTnode->eventType = entry.m_eventType;
        *peTTnode = eTTnode;
    }

    // Fill all hash entries between entry.m_id and entry.m_lastId...
    eTTnode->eventEntryTable.Add(&entry);
}

void wxEventHashTable::AllocEventTypeTable(size_t size)
{
    m_eventTypeTable = new EventTypeTablePointer[size];
    memset((void *)m_eventTypeTable, 0, sizeof(EventTypeTablePointer)*size);
    m_size = size;
}

void wxEventHashTable::GrowEventTypeTable()
{
    size_t oldSize = m_size;
    EventTypeTablePointer *oldEventTypeTable = m_eventTypeTable;

    // TODO: Search the most optimal grow sequence
    AllocEventTypeTable(/* GetNextPrime(oldSize) */oldSize*2+1);

    for ( size_t i = 0; i < oldSize; /* */ )
    {
        EventTypeTablePointer  eTToldNode = oldEventTypeTable[i];
        if (eTToldNode)
        {
            EventTypeTablePointer *peTTnode = &m_eventTypeTable[eTToldNode->eventType % m_size];
            EventTypeTablePointer  eTTnode = *peTTnode;

            // Check for collision, we don't want any.
            if (eTTnode)
            {
                GrowEventTypeTable();
                continue; // Don't increment the counter,
                          // as we still need to add this element.
            }
            else
            {
                // Get the old value and put it in the new table.
                *peTTnode = oldEventTypeTable[i];
            }
        }

        i++;
    }

    delete[] oldEventTypeTable;
}


// ----------------------------------------------------------------------------
// wxEvtHandler
// ----------------------------------------------------------------------------

/*
 * Event handler
 */

wxEvtHandler::wxEvtHandler()
{
    m_nextHandler = (wxEvtHandler *) NULL;
    m_previousHandler = (wxEvtHandler *) NULL;
    m_enabled = true;
    m_dynamicEvents = (wxList *) NULL;
    m_pendingEvents = (wxList *) NULL;
#if wxUSE_THREADS
#  if !defined(__VISAGECPP__)
    m_eventsLocker = new wxCriticalSection;
#  endif
#endif

    // no client data (yet)
    m_clientData = NULL;
    m_clientDataType = wxClientData_None;
}

wxEvtHandler::~wxEvtHandler()
{
    // Takes itself out of the list of handlers
    if (m_previousHandler)
        m_previousHandler->m_nextHandler = m_nextHandler;

    if (m_nextHandler)
        m_nextHandler->m_previousHandler = m_previousHandler;

    if (m_dynamicEvents)
    {
        for ( wxList::iterator it = m_dynamicEvents->begin(),
                               end = m_dynamicEvents->end();
              it != end;
              ++it )
        {
#if WXWIN_COMPATIBILITY_EVENT_TYPES
            wxEventTableEntry *entry = (wxEventTableEntry*)*it;
#else // !WXWIN_COMPATIBILITY_EVENT_TYPES
            wxDynamicEventTableEntry *entry = (wxDynamicEventTableEntry*)*it;
#endif // WXWIN_COMPATIBILITY_EVENT_TYPES/!WXWIN_COMPATIBILITY_EVENT_TYPES

            if (entry->m_callbackUserData)
                delete entry->m_callbackUserData;
            delete entry;
        }
        delete m_dynamicEvents;
    };

    if (m_pendingEvents)
        m_pendingEvents->DeleteContents(true);
    delete m_pendingEvents;

#if wxUSE_THREADS
#  if !defined(__VISAGECPP__)
    delete m_eventsLocker;
#  endif

    // Remove us from wxPendingEvents if necessary.
    if(wxPendingEventsLocker)
        wxENTER_CRIT_SECT(*wxPendingEventsLocker);
    if ( wxPendingEvents )
    {
        // Delete all occurences of this from the list of pending events
        while (wxPendingEvents->DeleteObject(this)) { } // Do nothing
    }
    if(wxPendingEventsLocker)
        wxLEAVE_CRIT_SECT(*wxPendingEventsLocker);
#endif

    // we only delete object data, not untyped
    if ( m_clientDataType == wxClientData_Object )
        delete m_clientObject;
}

#if wxUSE_THREADS

bool wxEvtHandler::ProcessThreadEvent(wxEvent& event)
{
    // check that we are really in a child thread
    wxASSERT_MSG( !wxThread::IsMain(),
                  wxT("use ProcessEvent() in main thread") );

    AddPendingEvent(event);

    return true;
}

void wxEvtHandler::ClearEventLocker()
{
#if !defined(__VISAGECPP__)
    delete m_eventsLocker;
    m_eventsLocker = NULL;
#endif
}

#endif // wxUSE_THREADS

void wxEvtHandler::AddPendingEvent(wxEvent& event)
{
    // 1) Add event to list of pending events of this event handler

    wxEvent *eventCopy = event.Clone();

    // we must be able to copy the events here so the event class must
    // implement Clone() properly instead of just providing a NULL stab for it
    wxCHECK_RET( eventCopy,
                 _T("events of this type aren't supposed to be posted") );

    wxENTER_CRIT_SECT( Lock() );

    if ( !m_pendingEvents )
      m_pendingEvents = new wxList;

    m_pendingEvents->Append(eventCopy);

    // 2) Add this event handler to list of event handlers that
    //    have pending events.

    wxENTER_CRIT_SECT(*wxPendingEventsLocker);

    if ( !wxPendingEvents )
        wxPendingEvents = new wxList;
    wxPendingEvents->Append(this);

    wxLEAVE_CRIT_SECT(*wxPendingEventsLocker);

    wxLEAVE_CRIT_SECT( Lock() );

    // 3) Inform the system that new pending events are somewhere,
    //    and that these should be processed in idle time.
    wxWakeUpIdle();
}

void wxEvtHandler::ProcessPendingEvents()
{
    // this method is only called by wxApp if this handler does have
    // pending events
    wxCHECK_RET( m_pendingEvents,
                 wxT("Please call wxApp::ProcessPendingEvents() instead") );

    wxENTER_CRIT_SECT( Lock() );

    // we leave the loop once we have processed all events that were present at
    // the start of ProcessPendingEvents because otherwise we could get into
    // infinite loop if the pending event handler execution resulted in another
    // event being posted
    size_t n = m_pendingEvents->size();
    for ( wxList::compatibility_iterator node = m_pendingEvents->GetFirst();
          node;
          node = m_pendingEvents->GetFirst() )
    {
        wxEventPtr event(wx_static_cast(wxEvent *, node->GetData()));

        // It's important we remove event from list before processing it.
        // Else a nested event loop, for example from a modal dialog, might
        // process the same event again.

        m_pendingEvents->Erase(node);

        wxLEAVE_CRIT_SECT( Lock() );

        ProcessEvent(*event);

        wxENTER_CRIT_SECT( Lock() );

        if ( --n == 0 )
            break;
    }

    wxLEAVE_CRIT_SECT( Lock() );
}

/*
 * Event table stuff
 */
/* static */ bool
wxEvtHandler::ProcessEventIfMatches(const wxEventTableEntryBase& entry,
                                    wxEvtHandler *handler,
                                    wxEvent& event)
{
    int tableId1 = entry.m_id,
        tableId2 = entry.m_lastId;

    // match only if the event type is the same and the id is either -1 in
    // the event table (meaning "any") or the event id matches the id
    // specified in the event table either exactly or by falling into
    // range between first and last
    if ((tableId1 == wxID_ANY) ||
        (tableId2 == wxID_ANY && tableId1 == event.GetId()) ||
        (tableId2 != wxID_ANY &&
         (event.GetId() >= tableId1 && event.GetId() <= tableId2)))
    {
        event.Skip(false);
        event.m_callbackUserData = entry.m_callbackUserData;

#if wxUSE_EXCEPTIONS
        if ( wxTheApp )
        {
            // call the handler via wxApp method which allows the user to catch
            // any exceptions which may be thrown by any handler in the program
            // in one place
            wxTheApp->HandleEvent(handler, (wxEventFunction)entry.m_fn, event);
        }
        else
#endif // wxUSE_EXCEPTIONS
        {
            // no need for an extra virtual function call
            (handler->*((wxEventFunction) (entry.m_fn)))(event);
        }

        if (!event.GetSkipped())
            return true;
    }

    return false;
}

bool wxEvtHandler::TryParent(wxEvent& event)
{
    if ( wxTheApp && (this != wxTheApp) )
    {
        // Special case: don't pass wxEVT_IDLE to wxApp, since it'll always
        // swallow it. wxEVT_IDLE is sent explicitly to wxApp so it will be
        // processed appropriately via SearchEventTable.
        if ( event.GetEventType() != wxEVT_IDLE )
        {
            if ( wxTheApp->ProcessEvent(event) )
                return true;
        }
    }

    return false;
}

bool wxEvtHandler::ProcessEvent(wxEvent& event)
{
    // allow the application to hook into event processing
    if ( wxTheApp )
    {
        int rc = wxTheApp->FilterEvent(event);
        if ( rc != -1 )
        {
            wxASSERT_MSG( rc == 1 || rc == 0,
                          _T("unexpected wxApp::FilterEvent return value") );

            return rc != 0;
        }
        //else: proceed normally
    }

    // An event handler can be enabled or disabled
    if ( GetEvtHandlerEnabled() )
    {
        // if we have a validator, it has higher priority than our own event
        // table
        if ( TryValidator(event) )
            return true;

        // Handle per-instance dynamic event tables first
        if ( m_dynamicEvents && SearchDynamicEventTable(event) )
            return true;

        // Then static per-class event tables
        if ( GetEventHashTable().HandleEvent(event, this) )
            return true;
    }

    // Try going down the event handler chain
    if ( GetNextHandler() )
    {
        if ( GetNextHandler()->ProcessEvent(event) )
            return true;
    }

    // Finally propagate the event upwards the window chain and/or to the
    // application object as necessary
    return TryParent(event);
}


bool wxEvtHandler::SearchEventTable(wxEventTable& table, wxEvent& event)
{
    const wxEventType eventType = event.GetEventType();
    for ( int i = 0; table.entries[i].m_fn != 0; i++ )
    {
        const wxEventTableEntry& entry = table.entries[i];
        if ( eventType == entry.m_eventType )
        {
            if ( ProcessEventIfMatches(entry, this, event) )
                return true;
        }
    }

    return false;
}

void wxEvtHandler::Connect( int id, int lastId,
                            int eventType,
                            wxObjectEventFunction func,
                            wxObject *userData,
                            wxEvtHandler* eventSink )
{
#if WXWIN_COMPATIBILITY_EVENT_TYPES
    wxEventTableEntry *entry = new wxEventTableEntry;
    entry->m_eventType = eventType;
    entry->m_id = id;
    entry->m_lastId = lastId;
    entry->m_fn = func;
    entry->m_callbackUserData = userData;
#else // !WXWIN_COMPATIBILITY_EVENT_TYPES
    wxDynamicEventTableEntry *entry =
        new wxDynamicEventTableEntry(eventType, id, lastId, func, userData, eventSink);
#endif // WXWIN_COMPATIBILITY_EVENT_TYPES/!WXWIN_COMPATIBILITY_EVENT_TYPES

    if (!m_dynamicEvents)
        m_dynamicEvents = new wxList;

    // Insert at the front of the list so most recent additions are found first
    m_dynamicEvents->Insert( (wxObject*) entry );
}

bool wxEvtHandler::Disconnect( int id, int lastId, wxEventType eventType,
                  wxObjectEventFunction func,
                  wxObject *userData,
                  wxEvtHandler* eventSink )
{
    if (!m_dynamicEvents)
        return false;

    wxList::compatibility_iterator node = m_dynamicEvents->GetFirst();
    while (node)
    {
#if WXWIN_COMPATIBILITY_EVENT_TYPES
            wxEventTableEntry *entry = (wxEventTableEntry*)node->GetData();
#else // !WXWIN_COMPATIBILITY_EVENT_TYPES
            wxDynamicEventTableEntry *entry = (wxDynamicEventTableEntry*)node->GetData();
#endif // WXWIN_COMPATIBILITY_EVENT_TYPES/!WXWIN_COMPATIBILITY_EVENT_TYPES

        if ((entry->m_id == id) &&
            ((entry->m_lastId == lastId) || (lastId == wxID_ANY)) &&
            ((entry->m_eventType == eventType) || (eventType == wxEVT_NULL)) &&
            ((entry->m_fn == func) || (func == (wxObjectEventFunction)NULL)) &&
            ((entry->m_eventSink == eventSink) || (eventSink == (wxEvtHandler*)NULL)) &&
            ((entry->m_callbackUserData == userData) || (userData == (wxObject*)NULL)))
        {
            if (entry->m_callbackUserData)
                delete entry->m_callbackUserData;
            m_dynamicEvents->Erase( node );
            delete entry;
            return true;
        }
        node = node->GetNext();
    }
    return false;
}

bool wxEvtHandler::SearchDynamicEventTable( wxEvent& event )
{
    wxCHECK_MSG( m_dynamicEvents, false,
                 wxT("caller should check that we have dynamic events") );

    wxList::compatibility_iterator node = m_dynamicEvents->GetFirst();
    while (node)
    {
#if WXWIN_COMPATIBILITY_EVENT_TYPES
        wxEventTableEntry *entry = (wxEventTableEntry*)node->GetData();
#else // !WXWIN_COMPATIBILITY_EVENT_TYPES
        wxDynamicEventTableEntry *entry = (wxDynamicEventTableEntry*)node->GetData();
#endif // WXWIN_COMPATIBILITY_EVENT_TYPES/!WXWIN_COMPATIBILITY_EVENT_TYPES

        // get next node before (maybe) calling the event handler as it could
        // call Disconnect() invalidating the current node
        node = node->GetNext();

        if ((event.GetEventType() == entry->m_eventType) && (entry->m_fn != 0))
        {
            wxEvtHandler *handler =
#if !WXWIN_COMPATIBILITY_EVENT_TYPES
                                    entry->m_eventSink ? entry->m_eventSink
                                                       :
#endif
                                                         this;

            if ( ProcessEventIfMatches(*entry, handler, event) )
            {
                return true;
            }
        }
    }

    return false;
}

void wxEvtHandler::DoSetClientObject( wxClientData *data )
{
    wxASSERT_MSG( m_clientDataType != wxClientData_Void,
                  wxT("can't have both object and void client data") );

    if ( m_clientObject )
        delete m_clientObject;

    m_clientObject = data;
    m_clientDataType = wxClientData_Object;
}

wxClientData *wxEvtHandler::DoGetClientObject() const
{
    // it's not an error to call GetClientObject() on a window which doesn't
    // have client data at all - NULL will be returned
    wxASSERT_MSG( m_clientDataType != wxClientData_Void,
                  wxT("this window doesn't have object client data") );

    return m_clientObject;
}

void wxEvtHandler::DoSetClientData( void *data )
{
    wxASSERT_MSG( m_clientDataType != wxClientData_Object,
                  wxT("can't have both object and void client data") );

    m_clientData = data;
    m_clientDataType = wxClientData_Void;
}

void *wxEvtHandler::DoGetClientData() const
{
    // it's not an error to call GetClientData() on a window which doesn't have
    // client data at all - NULL will be returned
    wxASSERT_MSG( m_clientDataType != wxClientData_Object,
                  wxT("this window doesn't have void client data") );

    return m_clientData;
}

#endif // wxUSE_BASE

#if wxUSE_GUI

// Find a window with the focus, that is also a descendant of the given window.
// This is used to determine the window to initially send commands to.
wxWindow* wxFindFocusDescendant(wxWindow* ancestor)
{
    // Process events starting with the window with the focus, if any.
    wxWindow* focusWin = wxWindow::FindFocus();
    wxWindow* win = focusWin;

    // Check if this is a descendant of this frame.
    // If not, win will be set to NULL.
    while (win)
    {
        if (win == ancestor)
            break;
        else
            win = win->GetParent();
    }
    if (win == (wxWindow*) NULL)
        focusWin = (wxWindow*) NULL;

    return focusWin;
}

#endif // wxUSE_GUI
