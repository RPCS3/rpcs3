///////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/tipwin.cpp
// Purpose:     implementation of wxTipWindow
// Author:      Vadim Zeitlin
// Modified by:
// Created:     10.09.00
// RCS-ID:      $Id: tipwin.cpp 43033 2006-11-04 13:31:10Z VZ $
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_TIPWINDOW

#include "wx/tipwin.h"

#ifndef WX_PRECOMP
    #include "wx/dcclient.h"
    #include "wx/timer.h"
    #include "wx/settings.h"
#endif // WX_PRECOMP

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const wxCoord TEXT_MARGIN_X = 3;
static const wxCoord TEXT_MARGIN_Y = 3;

// ----------------------------------------------------------------------------
// wxTipWindowView
// ----------------------------------------------------------------------------

// Viewer window to put in the frame
class WXDLLEXPORT wxTipWindowView : public wxWindow
{
public:
    wxTipWindowView(wxWindow *parent);

    // event handlers
    void OnPaint(wxPaintEvent& event);
    void OnMouseClick(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);

#if !wxUSE_POPUPWIN
    void OnKillFocus(wxFocusEvent& event);
#endif // wxUSE_POPUPWIN

    // calculate the client rect we need to display the text
    void Adjust(const wxString& text, wxCoord maxLength);

private:
    wxTipWindow* m_parent;

#if !wxUSE_POPUPWIN
    long m_creationTime;
#endif // !wxUSE_POPUPWIN

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxTipWindowView)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxTipWindow, wxTipWindowBase)
    EVT_LEFT_DOWN(wxTipWindow::OnMouseClick)
    EVT_RIGHT_DOWN(wxTipWindow::OnMouseClick)
    EVT_MIDDLE_DOWN(wxTipWindow::OnMouseClick)

#if !wxUSE_POPUPWIN
    EVT_KILL_FOCUS(wxTipWindow::OnKillFocus)
    EVT_ACTIVATE(wxTipWindow::OnActivate)
#endif // !wxUSE_POPUPWIN
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxTipWindowView, wxWindow)
    EVT_PAINT(wxTipWindowView::OnPaint)

    EVT_LEFT_DOWN(wxTipWindowView::OnMouseClick)
    EVT_RIGHT_DOWN(wxTipWindowView::OnMouseClick)
    EVT_MIDDLE_DOWN(wxTipWindowView::OnMouseClick)

    EVT_MOTION(wxTipWindowView::OnMouseMove)

#if !wxUSE_POPUPWIN
    EVT_KILL_FOCUS(wxTipWindowView::OnKillFocus)
#endif // !wxUSE_POPUPWIN
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxTipWindow
// ----------------------------------------------------------------------------

wxTipWindow::wxTipWindow(wxWindow *parent,
                         const wxString& text,
                         wxCoord maxLength,
                         wxTipWindow** windowPtr,
                         wxRect *rectBounds)
#if wxUSE_POPUPWIN
           : wxPopupTransientWindow(parent)
#else
           : wxFrame(parent, wxID_ANY, wxEmptyString,
                     wxDefaultPosition, wxDefaultSize,
                     wxNO_BORDER | wxFRAME_NO_TASKBAR )
#endif
{
    SetTipWindowPtr(windowPtr);
    if ( rectBounds )
    {
        SetBoundingRect(*rectBounds);
    }

    // set colours
    SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));

    // set size, position and show it
    m_view = new wxTipWindowView(this);
    m_view->Adjust(text, maxLength);
    m_view->SetFocus();

    int x, y;
    wxGetMousePosition(&x, &y);

    // we want to show the tip below the mouse, not over it
    //
    // NB: the reason we use "/ 2" here is that we don't know where the current
    //     cursors hot spot is... it would be nice if we could find this out
    //     though
    y += wxSystemSettings::GetMetric(wxSYS_CURSOR_Y) / 2;

#if wxUSE_POPUPWIN
    Position(wxPoint(x, y), wxSize(0,0));
    Popup(m_view);
    #ifdef __WXGTK__
        m_view->CaptureMouse();
    #endif
#else
    Move(x, y);
    Show(true);
#endif
}

wxTipWindow::~wxTipWindow()
{
    if ( m_windowPtr )
    {
        *m_windowPtr = NULL;
    }
    #ifdef wxUSE_POPUPWIN
        #ifdef __WXGTK__
            if ( m_view->HasCapture() )
                m_view->ReleaseMouse();
        #endif
    #endif
}

void wxTipWindow::OnMouseClick(wxMouseEvent& WXUNUSED(event))
{
    Close();
}

#if wxUSE_POPUPWIN

void wxTipWindow::OnDismiss()
{
    Close();
}

#else // !wxUSE_POPUPWIN

void wxTipWindow::OnActivate(wxActivateEvent& event)
{
    if (!event.GetActive())
        Close();
}

void wxTipWindow::OnKillFocus(wxFocusEvent& WXUNUSED(event))
{
    // Under Windows at least, we will get this immediately
    // because when the view window is focussed, the
    // tip window goes out of focus.
#ifdef __WXGTK__
    Close();
#endif
}

#endif // wxUSE_POPUPWIN // !wxUSE_POPUPWIN

void wxTipWindow::SetBoundingRect(const wxRect& rectBound)
{
    m_rectBound = rectBound;
}

void wxTipWindow::Close()
{
    if ( m_windowPtr )
    {
        *m_windowPtr = NULL;
        m_windowPtr = NULL;
    }

#if wxUSE_POPUPWIN
    Show(false);
    #ifdef __WXGTK__
        if ( m_view->HasCapture() )
            m_view->ReleaseMouse();
    #endif
    Destroy();
#else
    wxFrame::Close();
#endif
}

// ----------------------------------------------------------------------------
// wxTipWindowView
// ----------------------------------------------------------------------------

wxTipWindowView::wxTipWindowView(wxWindow *parent)
               : wxWindow(parent, wxID_ANY,
                          wxDefaultPosition, wxDefaultSize,
                          wxNO_BORDER)
{
    // set colours
    SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));

#if !wxUSE_POPUPWIN
    m_creationTime = wxGetLocalTime();
#endif // !wxUSE_POPUPWIN

    m_parent = (wxTipWindow*)parent;
}

void wxTipWindowView::Adjust(const wxString& text, wxCoord maxLength)
{
    wxClientDC dc(this);
    dc.SetFont(GetFont());

    // calculate the length: we want each line be no longer than maxLength
    // pixels and we only break lines at words boundary
    wxString current;
    wxCoord height, width,
            widthMax = 0;
    m_parent->m_heightLine = 0;

    bool breakLine = false;
    for ( const wxChar *p = text.c_str(); ; p++ )
    {
        if ( *p == _T('\n') || *p == _T('\0') )
        {
            dc.GetTextExtent(current, &width, &height);
            if ( width > widthMax )
                widthMax = width;

            if ( height > m_parent->m_heightLine )
                m_parent->m_heightLine = height;

            m_parent->m_textLines.Add(current);

            if ( !*p )
            {
                // end of text
                break;
            }

            current.clear();
            breakLine = false;
        }
        else if ( breakLine && (*p == _T(' ') || *p == _T('\t')) )
        {
            // word boundary - break the line here
            m_parent->m_textLines.Add(current);
            current.clear();
            breakLine = false;
        }
        else // line goes on
        {
            current += *p;
            dc.GetTextExtent(current, &width, &height);
            if ( width > maxLength )
                breakLine = true;

            if ( width > widthMax )
                widthMax = width;

            if ( height > m_parent->m_heightLine )
                m_parent->m_heightLine = height;
        }
    }

    // take into account the border size and the margins
    width  = 2*(TEXT_MARGIN_X + 1) + widthMax;
    height = 2*(TEXT_MARGIN_Y + 1) + wx_truncate_cast(wxCoord, m_parent->m_textLines.GetCount())*m_parent->m_heightLine;
    m_parent->SetClientSize(width, height);
    SetSize(0, 0, width, height);
}

void wxTipWindowView::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);

    wxRect rect;
    wxSize size = GetClientSize();
    rect.width = size.x;
    rect.height = size.y;

    // first filll the background
    dc.SetBrush(wxBrush(GetBackgroundColour(), wxSOLID));
    dc.SetPen( wxPen(GetForegroundColour(), 1, wxSOLID) );
    dc.DrawRectangle(rect);

    // and then draw the text line by line
    dc.SetTextBackground(GetBackgroundColour());
    dc.SetTextForeground(GetForegroundColour());
    dc.SetFont(GetFont());

    wxPoint pt;
    pt.x = TEXT_MARGIN_X;
    pt.y = TEXT_MARGIN_Y;
    size_t count = m_parent->m_textLines.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        dc.DrawText(m_parent->m_textLines[n], pt);

        pt.y += m_parent->m_heightLine;
    }
}

void wxTipWindowView::OnMouseClick(wxMouseEvent& WXUNUSED(event))
{
    m_parent->Close();
}

void wxTipWindowView::OnMouseMove(wxMouseEvent& event)
{
    const wxRect& rectBound = m_parent->m_rectBound;

    if ( rectBound.width &&
            !rectBound.Contains(ClientToScreen(event.GetPosition())) )
    {
        // mouse left the bounding rect, disappear
        m_parent->Close();
    }
    else
    {
        event.Skip();
    }
}

#if !wxUSE_POPUPWIN
void wxTipWindowView::OnKillFocus(wxFocusEvent& WXUNUSED(event))
{
    // Workaround the kill focus event happening just after creation in wxGTK
    if (wxGetLocalTime() > m_creationTime + 1)
        m_parent->Close();
}
#endif // !wxUSE_POPUPWIN

#endif // wxUSE_TIPWINDOW
