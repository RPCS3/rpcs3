/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/splash.cpp
// Purpose:     wxSplashScreen class
// Author:      Julian Smart
// Modified by:
// Created:     28/6/2000
// RCS-ID:      $Id: splash.cpp 58746 2009-02-08 09:16:50Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_SPLASH

#ifdef __WXGTK20__
    #include <gtk/gtk.h>
#endif

#include "wx/splash.h"

#ifndef WX_PRECOMP
    #include "wx/dcmemory.h"
    #include "wx/dcclient.h"
#endif

/*
 * wxSplashScreen
 */

#define wxSPLASH_TIMER_ID 9999

IMPLEMENT_DYNAMIC_CLASS(wxSplashScreen, wxFrame)

BEGIN_EVENT_TABLE(wxSplashScreen, wxFrame)
    EVT_TIMER(wxSPLASH_TIMER_ID, wxSplashScreen::OnNotify)
    EVT_CLOSE(wxSplashScreen::OnCloseWindow)
END_EVENT_TABLE()

/* Note that unless we pass a non-default size to the frame, SetClientSize
 * won't work properly under Windows, and the splash screen frame is sized
 * slightly too small.
 */

wxSplashScreen::wxSplashScreen(const wxBitmap& bitmap, long splashStyle, int milliseconds, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style):
    wxFrame(parent, id, wxEmptyString, wxPoint(0,0), wxSize(100, 100), style)
{
    // At least for GTK+ 2.0, this hint is not available.
#if defined(__WXGTK20__)
#if GTK_CHECK_VERSION(2,2,0)
    gtk_window_set_type_hint(GTK_WINDOW(m_widget),
                             GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
#endif
#endif

    m_window = NULL;
    m_splashStyle = splashStyle;
    m_milliseconds = milliseconds;

    m_window = new wxSplashScreenWindow(bitmap, this, wxID_ANY, pos, size, wxNO_BORDER);

    SetClientSize(bitmap.GetWidth(), bitmap.GetHeight());

    if (m_splashStyle & wxSPLASH_CENTRE_ON_PARENT)
        CentreOnParent();
    else if (m_splashStyle & wxSPLASH_CENTRE_ON_SCREEN)
        CentreOnScreen();

    if (m_splashStyle & wxSPLASH_TIMEOUT)
    {
        m_timer.SetOwner(this, wxSPLASH_TIMER_ID);
        m_timer.Start(milliseconds, true);
    }

    Show(true);
    m_window->SetFocus();
#if defined( __WXMSW__ ) || defined(__WXMAC__)
    Update(); // Without this, you see a blank screen for an instant
#else
    wxYieldIfNeeded(); // Should eliminate this
#endif
}

wxSplashScreen::~wxSplashScreen()
{
    m_timer.Stop();
}

void wxSplashScreen::OnNotify(wxTimerEvent& WXUNUSED(event))
{
    Close(true);
}

void wxSplashScreen::OnCloseWindow(wxCloseEvent& WXUNUSED(event))
{
    m_timer.Stop();
    this->Destroy();
}

/*
 * wxSplashScreenWindow
 */

BEGIN_EVENT_TABLE(wxSplashScreenWindow, wxWindow)
#ifdef __WXGTK__
    EVT_PAINT(wxSplashScreenWindow::OnPaint)
#endif
    EVT_ERASE_BACKGROUND(wxSplashScreenWindow::OnEraseBackground)
    EVT_CHAR(wxSplashScreenWindow::OnChar)
    EVT_MOUSE_EVENTS(wxSplashScreenWindow::OnMouseEvent)
END_EVENT_TABLE()

wxSplashScreenWindow::wxSplashScreenWindow(const wxBitmap& bitmap, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style):
    wxWindow(parent, id, pos, size, style)
{
    m_bitmap = bitmap;

#if !defined(__WXGTK__) && wxUSE_PALETTE
    bool hiColour = (wxDisplayDepth() >= 16) ;

    if (bitmap.GetPalette() && !hiColour)
    {
        SetPalette(* bitmap.GetPalette());
    }
#endif

}

// VZ: why don't we do it under wxGTK?
#if !defined(__WXGTK__) && wxUSE_PALETTE
    #define USE_PALETTE_IN_SPLASH
#endif

static void wxDrawSplashBitmap(wxDC& dc, const wxBitmap& bitmap, int WXUNUSED(x), int WXUNUSED(y))
{
    wxMemoryDC dcMem;

#ifdef USE_PALETTE_IN_SPLASH
    bool hiColour = (wxDisplayDepth() >= 16) ;

    if (bitmap.GetPalette() && !hiColour)
    {
        dcMem.SetPalette(* bitmap.GetPalette());
    }
#endif // USE_PALETTE_IN_SPLASH

    dcMem.SelectObjectAsSource(bitmap);
    dc.Blit(0, 0, bitmap.GetWidth(), bitmap.GetHeight(), &dcMem, 0, 0, wxCOPY,
            true /* use mask */);
    dcMem.SelectObject(wxNullBitmap);

#ifdef USE_PALETTE_IN_SPLASH
    if (bitmap.GetPalette() && !hiColour)
    {
        dcMem.SetPalette(wxNullPalette);
    }
#endif // USE_PALETTE_IN_SPLASH
}

void wxSplashScreenWindow::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    if (m_bitmap.Ok())
        wxDrawSplashBitmap(dc, m_bitmap, 0, 0);
}

void wxSplashScreenWindow::OnEraseBackground(wxEraseEvent& event)
{
    if (event.GetDC())
    {
        if (m_bitmap.Ok())
        {
            wxDrawSplashBitmap(* event.GetDC(), m_bitmap, 0, 0);
        }
    }
    else
    {
        wxClientDC dc(this);
        if (m_bitmap.Ok())
        {
            wxDrawSplashBitmap(dc, m_bitmap, 0, 0);
        }
    }
}

void wxSplashScreenWindow::OnMouseEvent(wxMouseEvent& event)
{
    if (event.LeftDown() || event.RightDown())
        GetParent()->Close(true);
}

void wxSplashScreenWindow::OnChar(wxKeyEvent& WXUNUSED(event))
{
    GetParent()->Close(true);
}

#endif // wxUSE_SPLASH
