///////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/window.cpp
// Purpose:     implementation of extra wxWindow methods for wxUniv port
// Author:      Vadim Zeitlin
// Modified by:
// Created:     06.08.00
// RCS-ID:      $Id: winuniv.cpp 46437 2007-06-13 04:35:23Z SC $
// Copyright:   (c) 2000 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

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

#include "wx/window.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/dcclient.h"
    #include "wx/dcmemory.h"
    #include "wx/event.h"
    #include "wx/scrolbar.h"
    #include "wx/menu.h"
    #include "wx/frame.h"
    #include "wx/log.h"
#endif // WX_PRECOMP

#include "wx/univ/colschem.h"
#include "wx/univ/renderer.h"
#include "wx/univ/theme.h"

#if wxUSE_CARET
    #include "wx/caret.h"
#endif // wxUSE_CARET

// turn Refresh() debugging on/off
#define WXDEBUG_REFRESH

#ifndef __WXDEBUG__
    #undef WXDEBUG_REFRESH
#endif

#if defined(WXDEBUG_REFRESH) && defined(__WXMSW__) && !defined(__WXMICROWIN__)
#include "wx/msw/private.h"
#endif

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

// we can't use wxWindowNative here as it won't be expanded inside the macro
#if defined(__WXMSW__)
    IMPLEMENT_DYNAMIC_CLASS(wxWindow, wxWindowMSW)
#elif defined(__WXGTK__)
    IMPLEMENT_DYNAMIC_CLASS(wxWindow, wxWindowGTK)
#elif defined(__WXMGL__)
    IMPLEMENT_DYNAMIC_CLASS(wxWindow, wxWindowMGL)
#elif defined(__WXDFB__)
    IMPLEMENT_DYNAMIC_CLASS(wxWindow, wxWindowDFB)
#elif defined(__WXX11__)
    IMPLEMENT_DYNAMIC_CLASS(wxWindow, wxWindowX11)
#elif defined(__WXPM__)
    IMPLEMENT_DYNAMIC_CLASS(wxWindow, wxWindowOS2)
#elif defined(__WXMAC__)
    IMPLEMENT_DYNAMIC_CLASS(wxWindow, wxWindowMac)
#endif

BEGIN_EVENT_TABLE(wxWindow, wxWindowNative)
    EVT_SIZE(wxWindow::OnSize)

#if wxUSE_ACCEL || wxUSE_MENUS
    EVT_KEY_DOWN(wxWindow::OnKeyDown)
#endif // wxUSE_ACCEL

#if wxUSE_MENUS
    EVT_CHAR(wxWindow::OnChar)
    EVT_KEY_UP(wxWindow::OnKeyUp)
#endif // wxUSE_MENUS

    EVT_PAINT(wxWindow::OnPaint)
    EVT_NC_PAINT(wxWindow::OnNcPaint)
    EVT_ERASE_BACKGROUND(wxWindow::OnErase)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// creation
// ----------------------------------------------------------------------------

void wxWindow::Init()
{
#if wxUSE_SCROLLBAR
    m_scrollbarVert =
    m_scrollbarHorz = (wxScrollBar *)NULL;
#endif // wxUSE_SCROLLBAR

    m_isCurrent = false;

    m_renderer = wxTheme::Get()->GetRenderer();

    m_oldSize.x = wxDefaultCoord;
    m_oldSize.y = wxDefaultCoord;
}

bool wxWindow::Create(wxWindow *parent,
                      wxWindowID id,
                      const wxPoint& pos,
                      const wxSize& size,
                      long style,
                      const wxString& name)
{
    long actualStyle = style;

    // we add wxCLIP_CHILDREN to get the same ("natural") behaviour under MSW
    // as under the other platforms
    actualStyle |= wxCLIP_CHILDREN;

    actualStyle &= ~wxVSCROLL;
    actualStyle &= ~wxHSCROLL;

#ifdef __WXMSW__
    // without this, borders (non-client areas in general) are not repainted
    // correctly when resizing; apparently, native NC areas are fully repainted
    // even without this style by MSW, but wxUniv implements client area
    // itself, so it doesn't work correctly for us
    //
    // FIXME: this is very expensive, we need to fix the (commented-out) code
    //        in OnSize() instead
    actualStyle |= wxFULL_REPAINT_ON_RESIZE;
#endif

    if ( !wxWindowNative::Create(parent, id, pos, size, actualStyle, name) )
        return false;

    // Set full style again, including those we didn't want present
    // when calling the base window Create().
    wxWindowBase::SetWindowStyleFlag(style);

    // if we allow or should always have a vertical scrollbar, make it
    if ( style & wxVSCROLL || style & wxALWAYS_SHOW_SB )
    {
#if wxUSE_TWO_WINDOWS
        SetInsertIntoMain( true );
#endif
#if wxUSE_SCROLLBAR
        m_scrollbarVert = new wxScrollBar(this, wxID_ANY,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxSB_VERTICAL);
#endif // wxUSE_SCROLLBAR
#if wxUSE_TWO_WINDOWS
        SetInsertIntoMain( false );
#endif
    }

    // if we should allow a horizontal scrollbar, make it
    if ( style & wxHSCROLL )
    {
#if wxUSE_TWO_WINDOWS
        SetInsertIntoMain( true );
#endif
#if wxUSE_SCROLLBAR
        m_scrollbarHorz = new wxScrollBar(this, wxID_ANY,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxSB_HORIZONTAL);
#endif // wxUSE_SCROLLBAR
#if wxUSE_TWO_WINDOWS
        SetInsertIntoMain( false );
#endif
    }

#if wxUSE_SCROLLBAR
    if (m_scrollbarHorz || m_scrollbarVert)
    {
        // position it/them
        PositionScrollbars();
    }
#endif // wxUSE_SCROLLBAR

    return true;
}

wxWindow::~wxWindow()
{
    m_isBeingDeleted = true;

#if wxUSE_SCROLLBAR
    // clear pointers to scrollbar before deleting the children: they are
    // children and so will be deleted by DestroyChildren() call below and if
    // any code using the scrollbars would be called in the process or from
    // ~wxWindowBase, the app would crash:
    m_scrollbarVert = m_scrollbarHorz = NULL;
#endif

    // we have to destroy our children before we're destroyed because our
    // children suppose that we're of type wxWindow, not just wxWindowNative,
    // and so bad things may happen if they're deleted from the base class dtor
    // as by then we're not a wxWindow any longer and wxUniv-specific virtual
    // functions can't be called
    DestroyChildren();
}

// ----------------------------------------------------------------------------
// background pixmap
// ----------------------------------------------------------------------------

void wxWindow::SetBackground(const wxBitmap& bitmap,
                              int alignment,
                              wxStretch stretch)
{
    m_bitmapBg = bitmap;
    m_alignBgBitmap = alignment;
    m_stretchBgBitmap = stretch;
}

const wxBitmap& wxWindow::GetBackgroundBitmap(int *alignment,
                                               wxStretch *stretch) const
{
    if ( m_bitmapBg.Ok() )
    {
        if ( alignment )
            *alignment = m_alignBgBitmap;
        if ( stretch )
            *stretch = m_stretchBgBitmap;
    }

    return m_bitmapBg;
}

// ----------------------------------------------------------------------------
// painting
// ----------------------------------------------------------------------------

// the event handlers executed when the window must be repainted
void wxWindow::OnNcPaint(wxNcPaintEvent& WXUNUSED(event))
{
    if ( m_renderer )
    {
        // get the window rect
        wxRect rect(GetSize());

#if wxUSE_SCROLLBAR
        // if the scrollbars are outside the border, we must adjust the rect to
        // exclude them
        if ( !m_renderer->AreScrollbarsInsideBorder() )
        {
            wxScrollBar *scrollbar = GetScrollbar(wxVERTICAL);
            if ( scrollbar )
                rect.width -= scrollbar->GetSize().x;

            scrollbar = GetScrollbar(wxHORIZONTAL);
            if ( scrollbar )
                rect.height -= scrollbar->GetSize().y;
        }
#endif // wxUSE_SCROLLBAR

        // get the DC and draw the border on it
        wxWindowDC dc(this);
        DoDrawBorder(dc, rect);
    }
}

void wxWindow::OnPaint(wxPaintEvent& event)
{
    if ( !m_renderer )
    {
        // it is a native control which paints itself
        event.Skip();
    }
    else
    {
        // get the DC to use and create renderer on it
        wxPaintDC dc(this);
        wxControlRenderer renderer(this, dc, m_renderer);

        // draw the control
        DoDraw(&renderer);
    }
}

// the event handler executed when the window background must be painted
void wxWindow::OnErase(wxEraseEvent& event)
{
    if ( !m_renderer )
    {
        event.Skip();

        return;
    }

    DoDrawBackground(*event.GetDC());

#if wxUSE_SCROLLBAR
    // if we have both scrollbars, we also have a square in the corner between
    // them which we must paint
    if ( m_scrollbarVert && m_scrollbarHorz )
    {
        wxSize size = GetSize();
        wxRect rectClient = GetClientRect(),
               rectBorder = m_renderer->GetBorderDimensions(GetBorder());

        wxRect rectCorner;
        rectCorner.x = rectClient.GetRight() + 1;
        rectCorner.y = rectClient.GetBottom() + 1;
        rectCorner.SetRight(size.x - rectBorder.width);
        rectCorner.SetBottom(size.y - rectBorder.height);

        if ( GetUpdateRegion().Contains(rectCorner) )
        {
            m_renderer->DrawScrollCorner(*event.GetDC(), rectCorner);
        }
    }
#endif // wxUSE_SCROLLBAR
}

bool wxWindow::DoDrawBackground(wxDC& dc)
{
    wxRect rect;

    wxSize size = GetSize();  // Why not GetClientSize() ?
    rect.x = 0;
    rect.y = 0;
    rect.width = size.x;
    rect.height = size.y;

    wxWindow * const parent = GetParent();
    if ( HasTransparentBackground() && !UseBgCol() && parent )
    {
        wxASSERT( !IsTopLevel() );

        wxPoint pos = GetPosition();

        AdjustForParentClientOrigin( pos.x, pos.y, 0 );

        // Adjust DC logical origin
        wxCoord org_x, org_y, x, y;
        dc.GetLogicalOrigin( &org_x, &org_y );
        x = org_x + pos.x;
        y = org_y + pos.y;
        dc.SetLogicalOrigin( x, y );

        // Adjust draw rect
        rect.x = pos.x;
        rect.y = pos.y;

        // Let parent draw the background
        parent->EraseBackground( dc, rect );

        // Restore DC logical origin
        dc.SetLogicalOrigin( org_x, org_y );
    }
    else
    {
        // Draw background ourselves
        EraseBackground( dc, rect );
    }

    return true;
}

void wxWindow::EraseBackground(wxDC& dc, const wxRect& rect)
{
    if ( GetBackgroundBitmap().Ok() )
    {
        // Get the bitmap and the flags
        int alignment;
        wxStretch stretch;
        wxBitmap bmp = GetBackgroundBitmap(&alignment, &stretch);
        wxControlRenderer::DrawBitmap(dc, bmp, rect, alignment, stretch);
    }
    else
    {
        // Just fill it with bg colour if no bitmap

        m_renderer->DrawBackground(dc, wxTHEME_BG_COLOUR(this),
                                   rect, GetStateFlags());
    }
}

void wxWindow::DoDrawBorder(wxDC& dc, const wxRect& rect)
{
    // draw outline unless the update region is enitrely inside it in which
    // case we don't need to do it
#if 0 // doesn't seem to work, why?
    if ( wxRegion(rect).Contains(GetUpdateRegion().GetBox()) != wxInRegion )
#endif
    {
        m_renderer->DrawBorder(dc, GetBorder(), rect, GetStateFlags());
    }
}

void wxWindow::DoDraw(wxControlRenderer * WXUNUSED(renderer))
{
}

void wxWindow::Refresh(bool eraseBackground, const wxRect *rect)
{
    wxRect rectClient; // the same rectangle in client coordinates
    wxPoint origin = GetClientAreaOrigin();

    wxSize size = GetClientSize();

    if ( rect )
    {
        // the rectangle passed as argument is in client coordinates
        rectClient = *rect;

        // don't refresh anything beyond the client area (scrollbars for
        // example)
        if ( rectClient.GetRight() > size.x )
            rectClient.SetRight(size.x);
        if ( rectClient.GetBottom() > size.y )
            rectClient.SetBottom(size.y);

    }
    else // refresh the entire client area
    {
        // x,y is already set to 0 by default
        rectClient.SetSize(size);
    }

    // convert refresh rectangle to window coordinates:
    wxRect rectWin(rectClient);
    rectWin.Offset(origin);

    // debugging helper
#ifdef WXDEBUG_REFRESH
    static bool s_refreshDebug = false;
    if ( s_refreshDebug )
    {
        wxWindowDC dc(this);
        dc.SetBrush(*wxCYAN_BRUSH);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(rectWin);

        // under Unix we use "--sync" X option for this
        #if defined(__WXMSW__) && !defined(__WXMICROWIN__)
            ::GdiFlush();
            ::Sleep(200);
        #endif // __WXMSW__
    }
#endif // WXDEBUG_REFRESH

    wxWindowNative::Refresh(eraseBackground, &rectWin);

    // Refresh all sub controls if any.
    wxWindowList& children = GetChildren();
    for ( wxWindowList::iterator i = children.begin(); i != children.end(); ++i )
    {
        wxWindow *child = *i;
        // only refresh subcontrols if they are visible:
        if ( child->IsTopLevel() || !child->IsShown() || child->IsFrozen() )
            continue;

        // ...and when the subcontrols are in the update region:
        wxRect childrect(child->GetRect());
        childrect.Intersect(rectClient);
        if ( childrect.IsEmpty() )
            continue;

        // refresh the subcontrol now:
        childrect.Offset(-child->GetPosition());
        // NB: We must call wxWindowNative version because we need to refresh
        //     the entire control, not just its client area, and this is why we
        //     don't account for child client area origin here neither. Also
        //     note that we don't pass eraseBackground to the child, but use
        //     true instead: this is because we can't be sure that
        //     eraseBackground=false is safe for children as well and not only
        //     for the parent.
        child->wxWindowNative::Refresh(eraseBackground, &childrect);
    }
}

// ----------------------------------------------------------------------------
// state flags
// ----------------------------------------------------------------------------

bool wxWindow::Enable(bool enable)
{
    if ( !wxWindowNative::Enable(enable) )
        return false;

    // disabled window can't keep focus
    if ( FindFocus() == this && GetParent() != NULL )
    {
        GetParent()->SetFocus();
    }

    if ( m_renderer )
    {
        // a window with renderer is drawn by ourselves and it has to be
        // refreshed to reflect its new status
        Refresh();
    }

    return true;
}

bool wxWindow::IsFocused() const
{
    return FindFocus() == this;
}

bool wxWindow::IsPressed() const
{
    return false;
}

bool wxWindow::IsDefault() const
{
    return false;
}

bool wxWindow::IsCurrent() const
{
    return m_isCurrent;
}

bool wxWindow::SetCurrent(bool doit)
{
    if ( doit == m_isCurrent )
        return false;

    m_isCurrent = doit;

    if ( CanBeHighlighted() )
        Refresh();

    return true;
}

int wxWindow::GetStateFlags() const
{
    int flags = 0;
    if ( !IsEnabled() )
        flags |= wxCONTROL_DISABLED;

    // the following states are only possible if our application is active - if
    // it is not, even our default/focused controls shouldn't appear as such
    if ( wxTheApp->IsActive() )
    {
        if ( IsCurrent() )
            flags |= wxCONTROL_CURRENT;
        if ( IsFocused() )
            flags |= wxCONTROL_FOCUSED;
        if ( IsPressed() )
            flags |= wxCONTROL_PRESSED;
        if ( IsDefault() )
            flags |= wxCONTROL_ISDEFAULT;
    }

    return flags;
}

// ----------------------------------------------------------------------------
// size
// ----------------------------------------------------------------------------

void wxWindow::OnSize(wxSizeEvent& event)
{
    event.Skip();

#if wxUSE_SCROLLBAR
    if ( m_scrollbarVert || m_scrollbarHorz )
    {
        PositionScrollbars();
    }
#endif // wxUSE_SCROLLBAR

#if 0   // ndef __WXMSW__
    // Refresh the area (strip) previously occupied by the border

    if ( !HasFlag(wxFULL_REPAINT_ON_RESIZE) && IsShown() )
    {
        // This code assumes that wxSizeEvent.GetSize() returns
        // the area of the entire window, not just the client
        // area.
        wxSize newSize = event.GetSize();

        if (m_oldSize.x == wxDefaultCoord && m_oldSize.y == wxDefaultCoord)
        {
            m_oldSize = newSize;
            return;
        }

        if (HasFlag( wxSIMPLE_BORDER ))
        {
            if (newSize.y > m_oldSize.y)
            {
                wxRect rect;
                rect.x = 0;
                rect.width = m_oldSize.x;
                rect.y = m_oldSize.y-2;
                rect.height = 1;
                Refresh( true, &rect );
            }
            else if (newSize.y < m_oldSize.y)
            {
                wxRect rect;
                rect.y = newSize.y;
                rect.x = 0;
                rect.height = 1;
                rect.width = newSize.x;
                wxWindowNative::Refresh( true, &rect );
            }

            if (newSize.x > m_oldSize.x)
            {
                wxRect rect;
                rect.y = 0;
                rect.height = m_oldSize.y;
                rect.x = m_oldSize.x-2;
                rect.width = 1;
                Refresh( true, &rect );
            }
            else if (newSize.x < m_oldSize.x)
            {
                wxRect rect;
                rect.x = newSize.x;
                rect.y = 0;
                rect.width = 1;
                rect.height = newSize.y;
                wxWindowNative::Refresh( true, &rect );
            }
        }
        else
        if (HasFlag( wxSUNKEN_BORDER ) || HasFlag( wxRAISED_BORDER ))
        {
            if (newSize.y > m_oldSize.y)
            {
                wxRect rect;
                rect.x = 0;
                rect.width = m_oldSize.x;
                rect.y = m_oldSize.y-4;
                rect.height = 2;
                Refresh( true, &rect );
            }
            else if (newSize.y < m_oldSize.y)
            {
                wxRect rect;
                rect.y = newSize.y;
                rect.x = 0;
                rect.height = 2;
                rect.width = newSize.x;
                wxWindowNative::Refresh( true, &rect );
            }

            if (newSize.x > m_oldSize.x)
            {
                wxRect rect;
                rect.y = 0;
                rect.height = m_oldSize.y;
                rect.x = m_oldSize.x-4;
                rect.width = 2;
                Refresh( true, &rect );
            }
            else if (newSize.x < m_oldSize.x)
            {
                wxRect rect;
                rect.x = newSize.x;
                rect.y = 0;
                rect.width = 2;
                rect.height = newSize.y;
                wxWindowNative::Refresh( true, &rect );
            }
        }

        m_oldSize = newSize;
    }
#endif
}

wxSize wxWindow::DoGetBestSize() const
{
    return AdjustSize(DoGetBestClientSize());
}

wxSize wxWindow::DoGetBestClientSize() const
{
    return wxWindowNative::DoGetBestSize();
}

wxSize wxWindow::AdjustSize(const wxSize& size) const
{
    wxSize sz = size;
    if ( m_renderer )
        m_renderer->AdjustSize(&sz, this);
    return sz;
}

wxPoint wxWindow::GetClientAreaOrigin() const
{
    wxPoint pt = wxWindowBase::GetClientAreaOrigin();

#if wxUSE_TWO_WINDOWS
#else
    if ( m_renderer )
        pt += m_renderer->GetBorderDimensions(GetBorder()).GetPosition();
#endif

    return pt;
}

void wxWindow::DoGetClientSize(int *width, int *height) const
{
    // if it is a native window, we assume it handles the scrollbars itself
    // too - and if it doesn't, there is not much we can do
    if ( !m_renderer )
    {
        wxWindowNative::DoGetClientSize(width, height);

        return;
    }

    int w, h;
    wxWindowNative::DoGetClientSize(&w, &h);

    // we assume that the scrollbars are positioned correctly (by a previous
    // call to PositionScrollbars()) here

    wxRect rectBorder;
    if ( m_renderer )
        rectBorder = m_renderer->GetBorderDimensions(GetBorder());

    if ( width )
    {
#if wxUSE_SCROLLBAR
        // in any case, take account of the scrollbar
        if ( m_scrollbarVert )
            w -= m_scrollbarVert->GetSize().x;
#endif // wxUSE_SCROLLBAR

        // account for the left and right borders
        *width = w - rectBorder.x - rectBorder.width;

        // we shouldn't return invalid width
        if ( *width < 0 )
            *width = 0;
    }

    if ( height )
    {
#if wxUSE_SCROLLBAR
        if ( m_scrollbarHorz )
            h -= m_scrollbarHorz->GetSize().y;
#endif // wxUSE_SCROLLBAR

        *height = h - rectBorder.y - rectBorder.height;

        // we shouldn't return invalid height
        if ( *height < 0 )
            *height = 0;
    }
}

void wxWindow::DoSetClientSize(int width, int height)
{
    // take into account the borders
    wxRect rectBorder = m_renderer->GetBorderDimensions(GetBorder());
    width += rectBorder.x;
    height += rectBorder.y;

    // and the scrollbars (as they may be offset into the border, use the
    // scrollbar position, not size - this supposes that PositionScrollbars()
    // had been called before)
    wxSize size = GetSize();
#if wxUSE_SCROLLBAR
    if ( m_scrollbarVert )
        width += size.x - m_scrollbarVert->GetPosition().x;
#endif // wxUSE_SCROLLBAR
    width += rectBorder.width;

#if wxUSE_SCROLLBAR
    if ( m_scrollbarHorz )
        height += size.y - m_scrollbarHorz->GetPosition().y;
#endif // wxUSE_SCROLLBAR
    height += rectBorder.height;

    wxWindowNative::DoSetClientSize(width, height);
}

wxHitTest wxWindow::DoHitTest(wxCoord x, wxCoord y) const
{
    wxHitTest ht = wxWindowNative::DoHitTest(x, y);

#if wxUSE_SCROLLBAR
    if ( ht == wxHT_WINDOW_INSIDE )
    {
        if ( m_scrollbarVert && x >= m_scrollbarVert->GetPosition().x )
        {
            // it can still be changed below because it may also be the corner
            ht = wxHT_WINDOW_VERT_SCROLLBAR;
        }

        if ( m_scrollbarHorz && y >= m_scrollbarHorz->GetPosition().y )
        {
            ht = ht == wxHT_WINDOW_VERT_SCROLLBAR ? wxHT_WINDOW_CORNER
                                                  : wxHT_WINDOW_HORZ_SCROLLBAR;
        }
    }
#endif // wxUSE_SCROLLBAR

    return ht;
}

// ----------------------------------------------------------------------------
// scrolling: we implement it entirely ourselves except for ScrollWindow()
// function which is supposed to be (efficiently) implemented by the native
// window class
// ----------------------------------------------------------------------------

void wxWindow::RefreshScrollbars()
{
#if wxUSE_SCROLLBAR
    if ( m_scrollbarHorz )
        m_scrollbarHorz->Refresh();

    if ( m_scrollbarVert )
        m_scrollbarVert->Refresh();
#endif // wxUSE_SCROLLBAR
}

void wxWindow::PositionScrollbars()
{
#if wxUSE_SCROLLBAR
    // do not use GetClientSize/Rect as it relies on the scrollbars being
    // correctly positioned

    wxSize size = GetSize();
    wxBorder border = GetBorder();
    wxRect rectBorder = m_renderer->GetBorderDimensions(border);
    bool inside = m_renderer->AreScrollbarsInsideBorder();

    int height = m_scrollbarHorz ? m_scrollbarHorz->GetSize().y : 0;
    int width = m_scrollbarVert ? m_scrollbarVert->GetSize().x : 0;

    wxRect rectBar;
    if ( m_scrollbarVert )
    {
        rectBar.x = size.x - width;
        if ( inside )
           rectBar.x -= rectBorder.width;
        rectBar.width = width;
        rectBar.y = 0;
        if ( inside )
            rectBar.y += rectBorder.y;
        rectBar.height = size.y - height;
        if ( inside )
            rectBar.height -= rectBorder.y + rectBorder.height;

        m_scrollbarVert->SetSize(rectBar, wxSIZE_NO_ADJUSTMENTS);
    }

    if ( m_scrollbarHorz )
    {
        rectBar.y = size.y - height;
        if ( inside )
            rectBar.y -= rectBorder.height;
        rectBar.height = height;
        rectBar.x = 0;
        if ( inside )
            rectBar.x += rectBorder.x;
        rectBar.width = size.x - width;
        if ( inside )
            rectBar.width -= rectBorder.x + rectBorder.width;

        m_scrollbarHorz->SetSize(rectBar, wxSIZE_NO_ADJUSTMENTS);
    }

    RefreshScrollbars();
#endif // wxUSE_SCROLLBAR
}

void wxWindow::SetScrollbar(int orient,
                            int pos,
                            int pageSize,
                            int range,
                            bool refresh)
{
#if wxUSE_SCROLLBAR
    wxASSERT_MSG( pageSize <= range,
                    _T("page size can't be greater than range") );

    bool hasClientSizeChanged = false;
    wxScrollBar *scrollbar = GetScrollbar(orient);
    if ( range && (pageSize < range) )
    {
        if ( !scrollbar )
        {
            // create it
#if wxUSE_TWO_WINDOWS
            SetInsertIntoMain( true );
#endif
            scrollbar = new wxScrollBar(this, wxID_ANY,
                                        wxDefaultPosition, wxDefaultSize,
                                        orient & wxVERTICAL ? wxSB_VERTICAL
                                                            : wxSB_HORIZONTAL);
#if wxUSE_TWO_WINDOWS
            SetInsertIntoMain( false );
#endif
            if ( orient & wxVERTICAL )
                m_scrollbarVert = scrollbar;
            else
                m_scrollbarHorz = scrollbar;

            // the client area diminished as we created a scrollbar
            hasClientSizeChanged = true;

            PositionScrollbars();
        }
        else if ( GetWindowStyle() & wxALWAYS_SHOW_SB )
        {
            // we might have disabled it before
            scrollbar->Enable();
        }

        scrollbar->SetScrollbar(pos, pageSize, range, pageSize, refresh);
    }
    else // no range means no scrollbar
    {
        if ( scrollbar )
        {
            // wxALWAYS_SHOW_SB only applies to the vertical scrollbar
            if ( (orient & wxVERTICAL) && (GetWindowStyle() & wxALWAYS_SHOW_SB) )
            {
                // just disable the scrollbar
                scrollbar->SetScrollbar(pos, pageSize, range, pageSize, refresh);
                scrollbar->Disable();
            }
            else // really remove the scrollbar
            {
                delete scrollbar;

                if ( orient & wxVERTICAL )
                    m_scrollbarVert = NULL;
                else
                    m_scrollbarHorz = NULL;

                // the client area increased as we removed a scrollbar
                hasClientSizeChanged = true;

                // the size of the remaining scrollbar must be adjusted
                if ( m_scrollbarHorz || m_scrollbarVert )
                {
                    PositionScrollbars();
                }
            }
        }
    }

    // give the window a chance to relayout
    if ( hasClientSizeChanged )
    {
#if wxUSE_TWO_WINDOWS
        wxWindowNative::SetSize( GetSize() );
#else
        wxSizeEvent event(GetSize());
        (void)GetEventHandler()->ProcessEvent(event);
#endif
    }
#else
    wxUnusedVar(orient);
    wxUnusedVar(pos);
    wxUnusedVar(pageSize);
    wxUnusedVar(range);
    wxUnusedVar(refresh);
#endif // wxUSE_SCROLLBAR
}

void wxWindow::SetScrollPos(int orient, int pos, bool WXUNUSED(refresh))
{
#if wxUSE_SCROLLBAR
    wxScrollBar *scrollbar = GetScrollbar(orient);

    if (scrollbar)
        scrollbar->SetThumbPosition(pos);

    // VZ: I think we can safely ignore this as we always refresh it
    //     automatically whenever the value chanegs
#if 0
    if ( refresh )
        Refresh();
#endif
#else
    wxUnusedVar(orient);
    wxUnusedVar(pos);
#endif // wxUSE_SCROLLBAR
}

int wxWindow::GetScrollPos(int orient) const
{
#if wxUSE_SCROLLBAR
    wxScrollBar *scrollbar = GetScrollbar(orient);
    return scrollbar ? scrollbar->GetThumbPosition() : 0;
#else
    wxUnusedVar(orient);
    return 0;
#endif // wxUSE_SCROLLBAR
}

int wxWindow::GetScrollThumb(int orient) const
{
#if wxUSE_SCROLLBAR
    wxScrollBar *scrollbar = GetScrollbar(orient);
    return scrollbar ? scrollbar->GetThumbSize() : 0;
#else
    wxUnusedVar(orient);
    return 0;
#endif // wxUSE_SCROLLBAR
}

int wxWindow::GetScrollRange(int orient) const
{
#if wxUSE_SCROLLBAR
    wxScrollBar *scrollbar = GetScrollbar(orient);
    return scrollbar ? scrollbar->GetRange() : 0;
#else
    wxUnusedVar(orient);
    return 0;
#endif // wxUSE_SCROLLBAR
}

void wxWindow::ScrollWindow(int dx, int dy, const wxRect *rect)
{
    // use native scrolling when available and do it in generic way
    // otherwise:
#ifdef __WXX11__

    wxWindowNative::ScrollWindow(dx, dy, rect);

#else // !wxX11

    // before scrolling it, ensure that we don't have any unpainted areas
    Update();

    wxRect r;

    if ( dx )
    {
        r = ScrollNoRefresh(dx, 0, rect);
        Refresh(true /* erase bkgnd */, &r);
    }

    if ( dy )
    {
        r = ScrollNoRefresh(0, dy, rect);
        Refresh(true /* erase bkgnd */, &r);
    }

    // scroll children accordingly:
    wxPoint offset(dx, dy);

    for (wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
         node; node = node->GetNext())
    {
        wxWindow *child = node->GetData();
#if wxUSE_SCROLLBAR
        if ( child == m_scrollbarVert || child == m_scrollbarHorz )
            continue;
#endif // wxUSE_SCROLLBAR

        // VS: Scrolling children has non-trivial semantics. If rect=NULL then
        //     it is easy: we scroll all children. Otherwise it gets
        //     complicated:
        //       1. if scrolling in one direction only, scroll only
        //          those children that intersect shaft defined by the rectangle
        //          and scrolling direction
        //       2. if scrolling in both axes, scroll all children

        bool shouldMove = false;

        if ( rect && (dx * dy == 0 /* moving in only one of x, y axis */) )
        {
            wxRect childRect = child->GetRect();
            if ( dx == 0 && (childRect.GetLeft() <= rect->GetRight() ||
                             childRect.GetRight() >= rect->GetLeft()) )
            {
                shouldMove = true;
            }
            else if ( dy == 0 && (childRect.GetTop() <= rect->GetBottom() ||
                                  childRect.GetBottom() >= rect->GetTop()) )
            {
                shouldMove = true;
            }
            // else: child outside of scrolling shaft, don't move
        }
        else // scrolling in both axes or rect=NULL
        {
            shouldMove = true;
        }

        if ( shouldMove )
            child->Move(child->GetPosition() + offset, wxSIZE_ALLOW_MINUS_ONE);
    }
#endif // wxX11/!wxX11
}

wxRect wxWindow::ScrollNoRefresh(int dx, int dy, const wxRect *rectTotal)
{
    wxASSERT_MSG( !dx || !dy, _T("can't be used for diag scrolling") );

    // the rect to refresh (which we will calculate)
    wxRect rect;

    if ( !dx && !dy )
    {
        // nothing to do
        return rect;
    }

    // calculate the part of the window which we can just redraw in the new
    // location
    wxSize sizeTotal = rectTotal ? rectTotal->GetSize() : GetClientSize();

    wxLogTrace(_T("scroll"), _T("rect is %dx%d, scroll by %d, %d"),
               sizeTotal.x, sizeTotal.y, dx, dy);

    // the initial and end point of the region we move in client coords
    wxPoint ptSource, ptDest;
    if ( rectTotal )
    {
        ptSource = rectTotal->GetPosition();
        ptDest = rectTotal->GetPosition();
    }

    // the size of this region
    wxSize size;
    size.x = sizeTotal.x - abs(dx);
    size.y = sizeTotal.y - abs(dy);
    if ( size.x <= 0 || size.y <= 0 )
    {
        // just redraw everything as nothing of the displayed image will stay
        wxLogTrace(_T("scroll"), _T("refreshing everything"));

        rect = rectTotal ? *rectTotal : wxRect(0, 0, sizeTotal.x, sizeTotal.y);
    }
    else // move the part which doesn't change to the new location
    {
        // note that when we scroll the canvas in some direction we move the
        // block which doesn't need to be refreshed in the opposite direction

        if ( dx < 0 )
        {
            // scroll to the right, move to the left
            ptSource.x -= dx;
        }
        else
        {
            // scroll to the left, move to the right
            ptDest.x += dx;
        }

        if ( dy < 0 )
        {
            // scroll down, move up
            ptSource.y -= dy;
        }
        else
        {
            // scroll up, move down
            ptDest.y += dy;
        }

#if wxUSE_CARET
        // we need to hide the caret before moving or it will erase itself at
        // the wrong (old) location
        wxCaret *caret = GetCaret();
        if ( caret )
            caret->Hide();
#endif // wxUSE_CARET

        // do move
        wxClientDC dc(this);
        wxBitmap bmp(size.x, size.y);
        wxMemoryDC dcMem;
        dcMem.SelectObject(bmp);

        dcMem.Blit(wxPoint(0,0), size, &dc, ptSource
#if defined(__WXGTK__) && !defined(wxHAS_WORKING_GTK_DC_BLIT)
                + GetClientAreaOrigin()
#endif // broken wxGTK wxDC::Blit
                  );
        dc.Blit(ptDest, size, &dcMem, wxPoint(0,0));

        wxLogTrace(_T("scroll"),
                   _T("Blit: (%d, %d) of size %dx%d -> (%d, %d)"),
                   ptSource.x, ptSource.y,
                   size.x, size.y,
                   ptDest.x, ptDest.y);

        // and now repaint the uncovered area

        // FIXME: We repaint the intersection of these rectangles twice - is
        //        it bad? I don't think so as it is rare to scroll the window
        //        diagonally anyhow and so adding extra logic to compute
        //        rectangle intersection is probably not worth the effort

        rect.x = ptSource.x;
        rect.y = ptSource.y;

        if ( dx )
        {
            if ( dx < 0 )
            {
                // refresh the area along the right border
                rect.x += size.x + dx;
                rect.width = -dx;
            }
            else
            {
                // refresh the area along the left border
                rect.width = dx;
            }

            rect.height = sizeTotal.y;

            wxLogTrace(_T("scroll"), _T("refreshing (%d, %d)-(%d, %d)"),
                       rect.x, rect.y,
                       rect.GetRight() + 1, rect.GetBottom() + 1);
        }

        if ( dy )
        {
            if ( dy < 0 )
            {
                // refresh the area along the bottom border
                rect.y += size.y + dy;
                rect.height = -dy;
            }
            else
            {
                // refresh the area along the top border
                rect.height = dy;
            }

            rect.width = sizeTotal.x;

            wxLogTrace(_T("scroll"), _T("refreshing (%d, %d)-(%d, %d)"),
                       rect.x, rect.y,
                       rect.GetRight() + 1, rect.GetBottom() + 1);
        }

#if wxUSE_CARET
        if ( caret )
            caret->Show();
#endif // wxUSE_CARET
    }

    return rect;
}

// ----------------------------------------------------------------------------
// accelerators and menu hot keys
// ----------------------------------------------------------------------------

#if wxUSE_MENUS
    // the last window over which Alt was pressed (used by OnKeyUp)
    wxWindow *wxWindow::ms_winLastAltPress = NULL;
#endif // wxUSE_MENUS

#if wxUSE_ACCEL || wxUSE_MENUS

void wxWindow::OnKeyDown(wxKeyEvent& event)
{
#if wxUSE_MENUS
    int key = event.GetKeyCode();
    if ( !event.ControlDown() && (key == WXK_ALT || key == WXK_F10) )
    {
        ms_winLastAltPress = this;

        // it can't be an accel anyhow
        return;
    }

    ms_winLastAltPress = NULL;
#endif // wxUSE_MENUS

#if wxUSE_ACCEL
    for ( wxWindow *win = this; win; win = win->GetParent() )
    {
        int command = win->GetAcceleratorTable()->GetCommand(event);
        if ( command != -1 )
        {
            wxCommandEvent eventCmd(wxEVT_COMMAND_MENU_SELECTED, command);
            if ( win->GetEventHandler()->ProcessEvent(eventCmd) )
            {
                // skip "event.Skip()" below
                return;
            }
        }

        if ( win->IsTopLevel() )
        {
            // try the frame menu bar
#if wxUSE_MENUS
            wxFrame *frame = wxDynamicCast(win, wxFrame);
            if ( frame )
            {
                wxMenuBar *menubar = frame->GetMenuBar();
                if ( menubar && menubar->ProcessAccelEvent(event) )
                {
                    // skip "event.Skip()" below
                    return;
                }
            }
#endif // wxUSE_MENUS

#if wxUSE_BUTTON
            // if it wasn't in a menu, try to find a button
            if ( command != -1 )
            {
                wxWindow* child = win->FindWindow(command);
                if ( child && wxDynamicCast(child, wxButton) )
                {
                    wxCommandEvent eventCmd(wxEVT_COMMAND_BUTTON_CLICKED, command);
                    eventCmd.SetEventObject(child);
                    if ( child->GetEventHandler()->ProcessEvent(eventCmd) )
                    {
                        // skip "event.Skip()" below
                        return;
                    }
                }
            }
#endif // wxUSE_BUTTON

            // don't propagate accels from the child frame to the parent one
            break;
        }
    }
#endif // wxUSE_ACCEL

    event.Skip();
}

#endif // wxUSE_ACCEL

#if wxUSE_MENUS

wxMenuBar *wxWindow::GetParentFrameMenuBar() const
{
    for ( const wxWindow *win = this; win; win = win->GetParent() )
    {
        if ( win->IsTopLevel() )
        {
            wxFrame *frame = wxDynamicCast(win, wxFrame);
            if ( frame )
            {
                return frame->GetMenuBar();
            }

            // don't look further - we don't want to return the menubar of the
            // parent frame
            break;
        }
    }

    return NULL;
}

void wxWindow::OnChar(wxKeyEvent& event)
{
    if ( event.AltDown() && !event.ControlDown() )
    {
        int key = event.GetKeyCode();

        wxMenuBar *menubar = GetParentFrameMenuBar();
        if ( menubar )
        {
            int item = menubar->FindNextItemForAccel(-1, key);
            if ( item != -1 )
            {
                menubar->PopupMenu((size_t)item);

                // skip "event.Skip()" below
                return;
            }
        }
    }

    event.Skip();
}

void wxWindow::OnKeyUp(wxKeyEvent& event)
{
    int key = event.GetKeyCode();
    if ( !event.HasModifiers() && (key == WXK_ALT || key == WXK_F10) )
    {
        // only process Alt release specially if there were no other key
        // presses since Alt had been pressed and if both events happened in
        // the same window
        if ( ms_winLastAltPress == this )
        {
            wxMenuBar *menubar = GetParentFrameMenuBar();
            if ( menubar && this != menubar )
            {
                menubar->SelectMenu(0);
            }
        }
    }
    else
    {
        event.Skip();
    }

    // in any case reset it
    ms_winLastAltPress = NULL;
}

#endif // wxUSE_MENUS

// ----------------------------------------------------------------------------
// MSW-specific section
// ----------------------------------------------------------------------------

#ifdef __WXMSW__

#include "wx/msw/private.h"

WXLRESULT wxWindow::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
    if ( message == WM_NCHITTEST )
    {
        // the windows which contain the other windows should let the mouse
        // events through, otherwise a window inside a static box would
        // never get any events at all
        if ( IsStaticBox() )
        {
            return HTTRANSPARENT;
        }
    }

    return wxWindowNative::MSWWindowProc(message, wParam, lParam);
}

#endif // __WXMSW__
