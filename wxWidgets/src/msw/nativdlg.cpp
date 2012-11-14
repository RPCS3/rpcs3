/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/nativdlg.cpp
// Purpose:     Native dialog loading code (part of wxWindow)
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: nativdlg.cpp 39720 2006-06-14 15:55:43Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <stdio.h>

    #include "wx/wx.h"
#endif

#include "wx/spinbutt.h"
#include "wx/msw/private.h"

// ---------------------------------------------------------------------------
// global functions
// ---------------------------------------------------------------------------

extern LONG APIENTRY _EXPORT wxDlgProc(HWND hWnd, UINT message,
                                       WPARAM wParam, LPARAM lParam);

// ===========================================================================
// implementation
// ===========================================================================

bool wxWindow::LoadNativeDialog(wxWindow* parent, wxWindowID& id)
{
    m_windowId = id;

    wxWindowCreationHook hook(this);
    m_hWnd = (WXHWND)::CreateDialog((HINSTANCE)wxGetInstance(),
                                    MAKEINTRESOURCE(id),
                                    parent ? (HWND)parent->GetHWND() : 0,
                                    (DLGPROC) wxDlgProc);

    if ( !m_hWnd )
        return false;

    SubclassWin(GetHWND());

    if ( parent )
        parent->AddChild(this);
    else
        wxTopLevelWindows.Append(this);

    // Enumerate all children
    HWND hWndNext;
    hWndNext = ::GetWindow((HWND) m_hWnd, GW_CHILD);

    if (hWndNext)
        CreateWindowFromHWND(this, (WXHWND) hWndNext);

    while (hWndNext != (HWND) NULL)
    {
        hWndNext = ::GetWindow(hWndNext, GW_HWNDNEXT);
        if (hWndNext)
            CreateWindowFromHWND(this, (WXHWND) hWndNext);
    }

    return true;
}

bool wxWindow::LoadNativeDialog(wxWindow* parent, const wxString& name)
{
    SetName(name);

    wxWindowCreationHook hook(this);
    m_hWnd = (WXHWND)::CreateDialog((HINSTANCE) wxGetInstance(),
                                    name.c_str(),
                                    parent ? (HWND)parent->GetHWND() : 0,
                                    (DLGPROC)wxDlgProc);

    if ( !m_hWnd )
        return false;

    SubclassWin(GetHWND());

    if ( parent )
        parent->AddChild(this);
    else
        wxTopLevelWindows.Append(this);

    // Enumerate all children
    HWND hWndNext;
    hWndNext = ::GetWindow((HWND) m_hWnd, GW_CHILD);

    if (hWndNext)
        CreateWindowFromHWND(this, (WXHWND) hWndNext);

    while (hWndNext != (HWND) NULL)
    {
        hWndNext = ::GetWindow(hWndNext, GW_HWNDNEXT);
        if (hWndNext)
            CreateWindowFromHWND(this, (WXHWND) hWndNext);
    }

    return true;
}

// ---------------------------------------------------------------------------
// look for child by id
// ---------------------------------------------------------------------------

wxWindow* wxWindow::GetWindowChild1(wxWindowID id)
{
    if ( m_windowId == id )
        return this;

    wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
    while ( node )
    {
        wxWindow* child = node->GetData();
        wxWindow* win = child->GetWindowChild1(id);
        if ( win )
            return win;

        node = node->GetNext();
    }

    return NULL;
}

wxWindow* wxWindow::GetWindowChild(wxWindowID id)
{
    wxWindow* win = GetWindowChild1(id);
    if ( !win )
    {
        HWND hwnd = ::GetDlgItem(GetHwnd(), id);
        if ( hwnd )
        {
            win = CreateWindowFromHWND(this, (WXHWND) hwnd);
        }
    }

    return win;
}

// ---------------------------------------------------------------------------
// create wxWin window from a native HWND
// ---------------------------------------------------------------------------

wxWindow* wxWindow::CreateWindowFromHWND(wxWindow* parent, WXHWND hWnd)
{
    wxString str(wxGetWindowClass(hWnd));
    str.UpperCase();

    long id = wxGetWindowId(hWnd);
    long style = GetWindowLong((HWND) hWnd, GWL_STYLE);

    wxWindow* win = NULL;

    if (str == wxT("BUTTON"))
    {
        int style1 = (style & 0xFF);
#if wxUSE_CHECKBOX
        if ((style1 == BS_3STATE) || (style1 == BS_AUTO3STATE) || (style1 == BS_AUTOCHECKBOX) ||
            (style1 == BS_CHECKBOX))
        {
            win = new wxCheckBox;
        }
        else
#endif
#if wxUSE_RADIOBTN
        if ((style1 == BS_AUTORADIOBUTTON) || (style1 == BS_RADIOBUTTON))
        {
            win = new wxRadioButton;
        }
        else
#endif
#if wxUSE_BMPBUTTON
#if defined(__WIN32__) && defined(BS_BITMAP)
        if (style & BS_BITMAP)
        {
            // TODO: how to find the bitmap?
            win = new wxBitmapButton;
            wxLogError(wxT("Have not yet implemented bitmap button as BS_BITMAP button."));
        }
        else
#endif
        if (style1 == BS_OWNERDRAW)
        {
            // TODO: how to find the bitmap?
            // TODO: can't distinguish between bitmap button and bitmap static.
            // Change implementation of wxStaticBitmap to SS_BITMAP.
            // PROBLEM: this assumes that we're using resource-based bitmaps.
            // So maybe need 2 implementations of bitmap buttons/static controls,
            // with a switch in the drawing code. Call default proc if BS_BITMAP.
            win = new wxBitmapButton;
        }
        else
#endif
#if wxUSE_BUTTON
        if ((style1 == BS_PUSHBUTTON) || (style1 == BS_DEFPUSHBUTTON))
        {
            win = new wxButton;
        }
        else
#endif
#if wxUSE_STATBOX
        if (style1 == BS_GROUPBOX)
        {
            win = new wxStaticBox;
        }
        else
#endif
        {
            wxLogError(wxT("Don't know what kind of button this is: id = %ld"),
                       id);
        }
    }
#if wxUSE_COMBOBOX
    else if (str == wxT("COMBOBOX"))
    {
        win = new wxComboBox;
    }
#endif
#if wxUSE_TEXTCTRL
    // TODO: Problem if the user creates a multiline - but not rich text - text control,
    // since wxWin assumes RichEdit control for this. Should have m_isRichText in
    // wxTextCtrl. Also, convert as much of the window style as is necessary
    // for correct functioning.
    // Could have wxWindow::AdoptAttributesFromHWND(WXHWND)
    // to be overridden by each control class.
    else if (str == wxT("EDIT"))
    {
        win = new wxTextCtrl;
    }
#endif
#if wxUSE_LISTBOX
    else if (str == wxT("LISTBOX"))
    {
        win = new wxListBox;
    }
#endif
#if wxUSE_SCROLLBAR
    else if (str == wxT("SCROLLBAR"))
    {
        win = new wxScrollBar;
    }
#endif
#if wxUSE_SPINBTN
    else if (str == wxT("MSCTLS_UPDOWN32"))
    {
        win = new wxSpinButton;
    }
#endif
#if wxUSE_SLIDER
    else if (str == wxT("MSCTLS_TRACKBAR32"))
    {
        // Need to ascertain if it's horiz or vert
        win = new wxSlider;
    }
#endif // wxUSE_SLIDER
#if wxUSE_STATTEXT
    else if (str == wxT("STATIC"))
    {
        int style1 = (style & 0xFF);

        if ((style1 == SS_LEFT) || (style1 == SS_RIGHT)
#ifndef __WXWINCE__
            || (style1 == SS_SIMPLE)
#endif
            )
            win = new wxStaticText;
#if wxUSE_STATBMP
#if defined(__WIN32__) && defined(BS_BITMAP)
        else if (style1 == SS_BITMAP)
        {
            win = new wxStaticBitmap;

            // Help! this doesn't correspond with the wxWin implementation.
            wxLogError(wxT("Please make SS_BITMAP statics into owner-draw buttons."));
        }
#endif
#endif /* wxUSE_STATBMP */
    }
#endif
    else
    {
        wxString msg(wxT("Don't know how to convert from Windows class "));
        msg += str;
        wxLogError(msg);
    }

    if (win)
    {
        parent->AddChild(win);
        win->SetEventHandler(win);
        win->SetHWND(hWnd);
        win->SetId(id);
        win->SubclassWin(hWnd);
        win->AdoptAttributesFromHWND();
        win->SetupColours();
    }

    return win;
}

// Make sure the window style (etc.) reflects the HWND style (roughly)
void wxWindow::AdoptAttributesFromHWND(void)
{
    HWND hWnd = (HWND) GetHWND();
    long style = GetWindowLong((HWND) hWnd, GWL_STYLE);

    if (style & WS_VSCROLL)
        m_windowStyle |= wxVSCROLL;
    if (style & WS_HSCROLL)
        m_windowStyle |= wxHSCROLL;
}
