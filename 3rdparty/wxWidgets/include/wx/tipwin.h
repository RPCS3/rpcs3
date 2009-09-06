///////////////////////////////////////////////////////////////////////////////
// Name:        wx/tipwin.h
// Purpose:     wxTipWindow is a window like the one typically used for
//              showing the tooltips
// Author:      Vadim Zeitlin
// Modified by:
// Created:     10.09.00
// RCS-ID:      $Id: tipwin.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TIPWIN_H_
#define _WX_TIPWIN_H_

#if wxUSE_TIPWINDOW

#if wxUSE_POPUPWIN
    #include "wx/popupwin.h"

    #define wxTipWindowBase wxPopupTransientWindow
#else
    #include "wx/frame.h"

    #define wxTipWindowBase wxFrame
#endif
#include "wx/arrstr.h"

class WXDLLIMPEXP_FWD_CORE wxTipWindowView;

// ----------------------------------------------------------------------------
// wxTipWindow
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxTipWindow : public wxTipWindowBase
{
public:
    // the mandatory ctor parameters are: the parent window and the text to
    // show
    //
    // optionally you may also specify the length at which the lines are going
    // to be broken in rows (100 pixels by default)
    //
    // windowPtr and rectBound are just passed to SetTipWindowPtr() and
    // SetBoundingRect() - see below
    wxTipWindow(wxWindow *parent,
                const wxString& text,
                wxCoord maxLength = 100,
                wxTipWindow** windowPtr = NULL,
                wxRect *rectBound = NULL);

    virtual ~wxTipWindow();

    // If windowPtr is not NULL the given address will be NULLed when the
    // window has closed
    void SetTipWindowPtr(wxTipWindow** windowPtr) { m_windowPtr = windowPtr; }

    // If rectBound is not NULL, the window will disappear automatically when
    // the mouse leave the specified rect: note that rectBound should be in the
    // screen coordinates!
    void SetBoundingRect(const wxRect& rectBound);

    // Hide and destroy the window
    void Close();

protected:
    // called by wxTipWindowView only
    bool CheckMouseInBounds(const wxPoint& pos);

    // event handlers
    void OnMouseClick(wxMouseEvent& event);

#if !wxUSE_POPUPWIN
    void OnActivate(wxActivateEvent& event);
    void OnKillFocus(wxFocusEvent& event);
#else // wxUSE_POPUPWIN
    virtual void OnDismiss();
#endif // wxUSE_POPUPWIN/!wxUSE_POPUPWIN

private:
    wxArrayString m_textLines;
    wxCoord m_heightLine;

    wxTipWindowView *m_view;

    wxTipWindow** m_windowPtr;
    wxRect m_rectBound;

    DECLARE_EVENT_TABLE()

    friend class wxTipWindowView;

    DECLARE_NO_COPY_CLASS(wxTipWindow)
};

#endif // wxUSE_TIPWINDOW

#endif // _WX_TIPWIN_H_
