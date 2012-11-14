///////////////////////////////////////////////////////////////////////////////
// Name:        wx/univ/inphand.h
// Purpose:     wxInputHandler class maps the keyboard and mouse events to the
//              actions which then are performed by the control
// Author:      Vadim Zeitlin
// Modified by:
// Created:     18.08.00
// RCS-ID:      $Id: inphand.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 2000 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIV_INPHAND_H_
#define _WX_UNIV_INPHAND_H_

#include "wx/univ/inpcons.h"         // for wxControlAction(s)

// ----------------------------------------------------------------------------
// types of the standard input handlers which can be passed to
// wxTheme::GetInputHandler()
// ----------------------------------------------------------------------------

#define wxINP_HANDLER_DEFAULT           wxT("")
#define wxINP_HANDLER_BUTTON            wxT("button")
#define wxINP_HANDLER_CHECKBOX          wxT("checkbox")
#define wxINP_HANDLER_CHECKLISTBOX      wxT("checklistbox")
#define wxINP_HANDLER_COMBOBOX          wxT("combobox")
#define wxINP_HANDLER_LISTBOX           wxT("listbox")
#define wxINP_HANDLER_NOTEBOOK          wxT("notebook")
#define wxINP_HANDLER_RADIOBTN          wxT("radiobtn")
#define wxINP_HANDLER_SCROLLBAR         wxT("scrollbar")
#define wxINP_HANDLER_SLIDER            wxT("slider")
#define wxINP_HANDLER_SPINBTN           wxT("spinbtn")
#define wxINP_HANDLER_STATUSBAR         wxT("statusbar")
#define wxINP_HANDLER_TEXTCTRL          wxT("textctrl")
#define wxINP_HANDLER_TOOLBAR           wxT("toolbar")
#define wxINP_HANDLER_TOPLEVEL          wxT("toplevel")

// ----------------------------------------------------------------------------
// wxInputHandler: maps the events to the actions
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxInputHandler : public wxObject
{
public:
    // map a keyboard event to one or more actions (pressed == true if the key
    // was pressed, false if released), returns true if something was done
    virtual bool HandleKey(wxInputConsumer *consumer,
                           const wxKeyEvent& event,
                           bool pressed) = 0;

    // map a mouse (click) event to one or more actions
    virtual bool HandleMouse(wxInputConsumer *consumer,
                             const wxMouseEvent& event) = 0;

    // handle mouse movement (or enter/leave) event: it is separated from
    // HandleMouse() for convenience as many controls don't care about mouse
    // movements at all
    virtual bool HandleMouseMove(wxInputConsumer *consumer,
                                 const wxMouseEvent& event);

    // do something with focus set/kill event: this is different from
    // HandleMouseMove() as the mouse maybe over the control without it having
    // focus
    //
    // return true to refresh the control, false otherwise
    virtual bool HandleFocus(wxInputConsumer *consumer, const wxFocusEvent& event);

    // react to the app getting/losing activation
    //
    // return true to refresh the control, false otherwise
    virtual bool HandleActivation(wxInputConsumer *consumer, bool activated);

    // virtual dtor for any base class
    virtual ~wxInputHandler();
};

// ----------------------------------------------------------------------------
// wxStdInputHandler is just a base class for all other "standard" handlers
// and also provides the way to chain input handlers together
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStdInputHandler : public wxInputHandler
{
public:
    wxStdInputHandler(wxInputHandler *handler) : m_handler(handler) { }

    virtual bool HandleKey(wxInputConsumer *consumer,
                           const wxKeyEvent& event,
                           bool pressed)
    {
        return m_handler ? m_handler->HandleKey(consumer, event, pressed)
                         : false;
    }

    virtual bool HandleMouse(wxInputConsumer *consumer,
                             const wxMouseEvent& event)
    {
        return m_handler ? m_handler->HandleMouse(consumer, event) : false;
    }

    virtual bool HandleMouseMove(wxInputConsumer *consumer, const wxMouseEvent& event)
    {
        return m_handler ? m_handler->HandleMouseMove(consumer, event) : false;
    }

    virtual bool HandleFocus(wxInputConsumer *consumer, const wxFocusEvent& event)
    {
        return m_handler ? m_handler->HandleFocus(consumer, event) : false;
    }

private:
    wxInputHandler *m_handler;
};

#endif // _WX_UNIV_INPHAND_H_
