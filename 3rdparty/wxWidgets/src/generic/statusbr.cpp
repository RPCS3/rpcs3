/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/statusbr.cpp
// Purpose:     wxStatusBarGeneric class implementation
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: statusbr.cpp 57542 2008-12-25 13:03:24Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_STATUSBAR

#include "wx/statusbr.h"

#ifndef WX_PRECOMP
    #include "wx/settings.h"
    #include "wx/dcclient.h"
#endif

#ifdef __WXGTK20__
    #include <gtk/gtk.h>
    #include "wx/gtk/win_gtk.h"
#endif

// we only have to do it here when we use wxStatusBarGeneric in addition to the
// standard wxStatusBar class, if wxStatusBarGeneric is the same as
// wxStatusBar, then the corresponding IMPLEMENT_DYNAMIC_CLASS is already in
// common/statbar.cpp
#if defined(__WXMAC__) || \
    (defined(wxUSE_NATIVE_STATUSBAR) && wxUSE_NATIVE_STATUSBAR)
    #include "wx/generic/statusbr.h"

    IMPLEMENT_DYNAMIC_CLASS(wxStatusBarGeneric, wxWindow)
#endif // wxUSE_NATIVE_STATUSBAR

BEGIN_EVENT_TABLE(wxStatusBarGeneric, wxWindow)
    EVT_PAINT(wxStatusBarGeneric::OnPaint)
    EVT_LEFT_DOWN(wxStatusBarGeneric::OnLeftDown)
    EVT_RIGHT_DOWN(wxStatusBarGeneric::OnRightDown)
    EVT_SYS_COLOUR_CHANGED(wxStatusBarGeneric::OnSysColourChanged)
END_EVENT_TABLE()

// Default status border dimensions
#define         wxTHICK_LINE_BORDER 2

void wxStatusBarGeneric::Init()
{
    m_borderX = wxTHICK_LINE_BORDER;
    m_borderY = wxTHICK_LINE_BORDER;
}

wxStatusBarGeneric::~wxStatusBarGeneric()
{
}

bool wxStatusBarGeneric::Create(wxWindow *parent,
                                wxWindowID id,
                                long style,
                                const wxString& name)
{
    style |= wxTAB_TRAVERSAL | wxFULL_REPAINT_ON_RESIZE;
    if ( !wxWindow::Create(parent, id,
                           wxDefaultPosition, wxDefaultSize,
                           style, name) )
        return false;

    // The status bar should have a themed background
    SetThemeEnabled( true );

    InitColours();

#ifdef __WXPM__
    SetFont(*wxSMALL_FONT);
#endif

    wxCoord y;
    {
        // Set the height according to the font and the border size
        wxClientDC dc(this);
        dc.SetFont(GetFont());

        dc.GetTextExtent(_T("X"), NULL, &y );
    }
    int height = (int)( (11*y)/10 + 2*GetBorderY());

    SetSize(wxDefaultCoord, wxDefaultCoord, wxDefaultCoord, height);

    SetFieldsCount(1);

    return true;
}


wxSize wxStatusBarGeneric::DoGetBestSize() const
{
    int width, height;

    // best width is the width of the parent
    GetParent()->GetClientSize(&width, NULL);

    // best height is as calculated above in Create
    wxClientDC dc((wxWindow*)this);
    dc.SetFont(GetFont());
    wxCoord y;
    dc.GetTextExtent(_T("X"), NULL, &y );
    height = (int)( (11*y)/10 + 2*GetBorderY());

    return wxSize(width, height);
}

void wxStatusBarGeneric::SetFieldsCount(int number, const int *widths)
{
    wxASSERT_MSG( number >= 0, _T("negative number of fields in wxStatusBar?") );

    int i;
    for(i = m_nFields; i < number; ++i)
        m_statusStrings.Add( wxEmptyString );

    for (i = m_nFields - 1; i >= number; --i)
        m_statusStrings.RemoveAt(i);

    // forget the old cached pixel widths
    m_widthsAbs.Empty();

    wxStatusBarBase::SetFieldsCount(number, widths);

    wxASSERT_MSG( m_nFields == (int)m_statusStrings.GetCount(),
                  _T("This really should never happen, can we do away with m_nFields here?") );
}

void wxStatusBarGeneric::SetStatusText(const wxString& text, int number)
{
    wxCHECK_RET( (number >= 0) && (number < m_nFields),
                 _T("invalid status bar field index") );

    wxString oldText = m_statusStrings[number];
    if (oldText != text)
    {
        m_statusStrings[number] = text;

        wxRect rect;
        GetFieldRect(number, rect);

        Refresh(true, &rect);

        // it's common to show some text in the status bar before starting a
        // relatively lengthy operation, ensure that the text is shown to the
        // user immediately and not after the lengthy operation end
        Update();
    }
}

wxString wxStatusBarGeneric::GetStatusText(int n) const
{
    wxCHECK_MSG( (n >= 0) && (n < m_nFields), wxEmptyString,
                 _T("invalid status bar field index") );

    return m_statusStrings[n];
}

void wxStatusBarGeneric::SetStatusWidths(int n, const int widths_field[])
{
    // only set status widths, when n == number of statuswindows
    wxCHECK_RET( n == m_nFields, _T("status bar field count mismatch") );

    // delete the old widths in any case - this function may be used to reset
    // the widths to the default (all equal)
    // MBN: this is incompatible with at least wxMSW and wxMAC and not
    //      documented, but let's keep it for now
    ReinitWidths();

    // forget the old cached pixel widths
    m_widthsAbs.Empty();

    if ( !widths_field )
    {
        // not an error, see the comment above
        Refresh();
        return;
    }

    wxStatusBarBase::SetStatusWidths(n, widths_field);
}

void wxStatusBarGeneric::OnPaint(wxPaintEvent& WXUNUSED(event) )
{
    wxPaintDC dc(this);

    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
#ifdef __WXGTK20__
    // Draw grip first
    if (HasFlag( wxST_SIZEGRIP ))
    {
        int width, height;
        GetClientSize(&width, &height);

        if (GetLayoutDirection() == wxLayout_RightToLeft)
        {
            gtk_paint_resize_grip( m_widget->style,
                               GTK_PIZZA(m_wxwindow)->bin_window,
                               (GtkStateType) GTK_WIDGET_STATE (m_widget),
                               NULL,
                               m_widget,
                               "statusbar",
                               GDK_WINDOW_EDGE_SOUTH_WEST,
                               2, 2, height-2, height-4 );
        }
        else
        {
            gtk_paint_resize_grip( m_widget->style,
                               GTK_PIZZA(m_wxwindow)->bin_window,
                               (GtkStateType) GTK_WIDGET_STATE (m_widget),
                               NULL,
                               m_widget,
                               "statusbar",
                               GDK_WINDOW_EDGE_SOUTH_EAST,
                               width-height-2, 2, height-2, height-4 );
        }
    }
#endif

    if (GetFont().Ok())
        dc.SetFont(GetFont());

    dc.SetBackgroundMode(wxTRANSPARENT);

#ifdef __WXPM__
    wxColour vColor;

    vColor = wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR);
    ::WinFillRect(dc.m_hPS, &dc.m_vRclPaint, vColor.GetPixel());
#endif

    for (int i = 0; i < m_nFields; i ++)
        DrawField(dc, i);
}

void wxStatusBarGeneric::DrawFieldText(wxDC& dc, int i)
{
    int leftMargin = 2;

    wxRect rect;
    GetFieldRect(i, rect);

    wxString text(GetStatusText(i));

    long x = 0, y = 0;

    dc.GetTextExtent(text, &x, &y);

    int xpos = rect.x + leftMargin;
    int ypos = (int) (((rect.height - y) / 2 ) + rect.y + 0.5) ;

#if defined( __WXGTK__ ) || defined(__WXMAC__)
    xpos++;
    ypos++;
#endif

    dc.SetClippingRegion(rect.x, rect.y, rect.width, rect.height);

    dc.DrawText(text, xpos, ypos);

    dc.DestroyClippingRegion();
}

void wxStatusBarGeneric::DrawField(wxDC& dc, int i)
{
    wxRect rect;
    GetFieldRect(i, rect);

    int style = wxSB_NORMAL;
    if (m_statusStyles)
        style = m_statusStyles[i];

    if (style != wxSB_FLAT)
    {
        // Draw border
        // For wxSB_NORMAL:
        // Have grey background, plus 3-d border -
        // One black rectangle.
        // Inside this, left and top sides - dark grey. Bottom and right -
        // white.
        // Reverse it for wxSB_RAISED

        dc.SetPen((style == wxSB_RAISED) ? m_mediumShadowPen : m_hilightPen);

    #ifndef __WXPM__

        // Right and bottom lines
        dc.DrawLine(rect.x + rect.width, rect.y,
                    rect.x + rect.width, rect.y + rect.height);
        dc.DrawLine(rect.x + rect.width, rect.y + rect.height,
                    rect.x, rect.y + rect.height);

        dc.SetPen((style == wxSB_RAISED) ? m_hilightPen : m_mediumShadowPen);

        // Left and top lines
        dc.DrawLine(rect.x, rect.y + rect.height,
               rect.x, rect.y);
        dc.DrawLine(rect.x, rect.y,
            rect.x + rect.width, rect.y);
    #else

        dc.DrawLine(rect.x + rect.width, rect.height + 2,
                    rect.x, rect.height + 2);
        dc.DrawLine(rect.x + rect.width, rect.y,
                    rect.x + rect.width, rect.y + rect.height);

        dc.SetPen((style == wxSB_RAISED) ? m_hilightPen : m_mediumShadowPen);
        dc.DrawLine(rect.x, rect.y,
                    rect.x + rect.width, rect.y);
        dc.DrawLine(rect.x, rect.y + rect.height,
                   rect.x, rect.y);

#endif
    }

    DrawFieldText(dc, i);
}

  // Get the position and size of the field's internal bounding rectangle
bool wxStatusBarGeneric::GetFieldRect(int n, wxRect& rect) const
{
    wxCHECK_MSG( (n >= 0) && (n < m_nFields), false,
                 _T("invalid status bar field index") );

    // FIXME: workarounds for OS/2 bugs have nothing to do here (VZ)
    int width, height;
#ifdef __WXPM__
    GetSize(&width, &height);
#else
    GetClientSize(&width, &height);
#endif

    // we cache m_widthsAbs between calls and recompute it if client
    // width has changed (or when it is initially empty)
    if ( m_widthsAbs.IsEmpty() || (m_lastClientWidth != width) )
    {
        wxConstCast(this, wxStatusBarGeneric)->
            m_widthsAbs = CalculateAbsWidths(width);
        // remember last width for which we have recomputed the widths in pixels
        wxConstCast(this, wxStatusBarGeneric)->
            m_lastClientWidth = width;
    }

    rect.x = 0;
    for ( int i = 0; i < n; i++ )
    {
        rect.x += m_widthsAbs[i];
    }

    rect.x += m_borderX;
    rect.y = m_borderY;

    rect.width = m_widthsAbs[n] - 2*m_borderX;
    rect.height = height - 2*m_borderY;

    return true;
}

// Initialize colours
void wxStatusBarGeneric::InitColours()
{
#if defined(__WXPM__)
    m_mediumShadowPen = wxPen(wxColour(127, 127, 127), 1, wxSOLID);
    m_hilightPen = *wxWHITE_PEN;

    SetBackgroundColour(*wxLIGHT_GREY);
    SetForegroundColour(*wxBLACK);
#else // !__WXPM__
    m_mediumShadowPen = wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW));
    m_hilightPen = wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DHILIGHT));
#endif // __WXPM__/!__WXPM__
}

// Responds to colour changes, and passes event on to children.
void wxStatusBarGeneric::OnSysColourChanged(wxSysColourChangedEvent& event)
{
    InitColours();

    // Propagate the event to the non-top-level children
    wxWindow::OnSysColourChanged(event);
}

void wxStatusBarGeneric::SetMinHeight(int height)
{
    // check that this min height is not less than minimal height for the
    // current font
    wxClientDC dc(this);
    wxCoord y;
    dc.GetTextExtent( wxT("X"), NULL, &y );

    if ( height > (11*y)/10 )
    {
        SetSize(wxDefaultCoord, wxDefaultCoord, wxDefaultCoord, height + 2*m_borderY);
    }
}

void wxStatusBarGeneric::OnLeftDown(wxMouseEvent& event)
{
#ifdef __WXGTK20__
    int width, height;
    GetClientSize(&width, &height);

    if (HasFlag( wxST_SIZEGRIP ) && (event.GetX() > width-height))
    {
        GtkWidget *ancestor = gtk_widget_get_toplevel( m_widget );

        if (!GTK_IS_WINDOW (ancestor))
            return;

        GdkWindow *source = GTK_PIZZA(m_wxwindow)->bin_window;

        int org_x = 0;
        int org_y = 0;
        gdk_window_get_origin( source, &org_x, &org_y );

        if (GetLayoutDirection() == wxLayout_RightToLeft)
        {
            gtk_window_begin_resize_drag (GTK_WINDOW (ancestor),
                                  GDK_WINDOW_EDGE_SOUTH_WEST,
                                  1,
                                  org_x - event.GetX() + GetSize().x ,
                                  org_y + event.GetY(),
                                  0);
        }
        else
        {
            gtk_window_begin_resize_drag (GTK_WINDOW (ancestor),
                                  GDK_WINDOW_EDGE_SOUTH_EAST,
                                  1,
                                  org_x + event.GetX(),
                                  org_y + event.GetY(),
                                  0);
        }
    }
    else
    {
        event.Skip( true );
    }
#else
    event.Skip( true );
#endif
}

void wxStatusBarGeneric::OnRightDown(wxMouseEvent& event)
{
#ifdef __WXGTK20__
    int width, height;
    GetClientSize(&width, &height);

    if (HasFlag( wxST_SIZEGRIP ) && (event.GetX() > width-height))
    {
        GtkWidget *ancestor = gtk_widget_get_toplevel( m_widget );

        if (!GTK_IS_WINDOW (ancestor))
            return;

        GdkWindow *source = GTK_PIZZA(m_wxwindow)->bin_window;

        int org_x = 0;
        int org_y = 0;
        gdk_window_get_origin( source, &org_x, &org_y );

        gtk_window_begin_move_drag (GTK_WINDOW (ancestor),
                                2,
                                org_x + event.GetX(),
                                org_y + event.GetY(),
                                0);
    }
    else
    {
        event.Skip( true );
    }
#else
    event.Skip( true );
#endif
}

#endif // wxUSE_STATUSBAR
