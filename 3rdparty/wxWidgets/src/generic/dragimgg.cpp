/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/dragimgg.cpp
// Purpose:     Generic wxDragImage implementation
// Author:      Julian Smart
// Modified by:
// Created:     29/2/2000
// RCS-ID:      $Id: dragimgg.cpp 42397 2006-10-25 12:12:56Z VS $
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

#if wxUSE_DRAGIMAGE

#ifndef WX_PRECOMP
    #include <stdio.h>
    #include "wx/window.h"
    #include "wx/frame.h"
    #include "wx/dcclient.h"
    #include "wx/dcscreen.h"
    #include "wx/dcmemory.h"
    #include "wx/settings.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/image.h"
#endif

#define wxUSE_IMAGE_IN_DRAGIMAGE 1

#include "wx/generic/dragimgg.h"

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxGenericDragImage, wxObject)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxGenericDragImage ctors/dtor
// ----------------------------------------------------------------------------

wxGenericDragImage::~wxGenericDragImage()
{
    if (m_windowDC)
    {
        delete m_windowDC;
    }
}

void wxGenericDragImage::Init()
{
    m_isDirty = false;
    m_isShown = false;
    m_windowDC = (wxDC*) NULL;
    m_window = (wxWindow*) NULL;
    m_fullScreen = false;
#ifdef wxHAS_NATIVE_OVERLAY
    m_dcOverlay = NULL;
#else
    m_pBackingBitmap = (wxBitmap*) NULL;
#endif
}

#if WXWIN_COMPATIBILITY_2_6
wxGenericDragImage::wxGenericDragImage(const wxCursor& cursor, const wxPoint& WXUNUSED(cursorHotspot))
{
    Init();
    Create(cursor);
}

wxGenericDragImage::wxGenericDragImage(const wxBitmap& image, const wxCursor& cursor, const wxPoint& WXUNUSED(cursorHotspot))
{
    Init();

    Create(image, cursor);
}

wxGenericDragImage::wxGenericDragImage(const wxIcon& image, const wxCursor& cursor, const wxPoint& WXUNUSED(cursorHotspot))
{
    Init();

    Create(image, cursor);
}

wxGenericDragImage::wxGenericDragImage(const wxString& str, const wxCursor& cursor, const wxPoint& WXUNUSED(cursorHotspot))
{
    Init();

    Create(str, cursor);
}

bool wxGenericDragImage::Create(const wxCursor& cursor, const wxPoint& WXUNUSED(cursorHotspot))
{
    return Create(cursor);
}

bool wxGenericDragImage::Create(const wxBitmap& image, const wxCursor& cursor, const wxPoint& WXUNUSED(cursorHotspot))
{
    return Create(image, cursor);
}

bool wxGenericDragImage::Create(const wxIcon& image, const wxCursor& cursor, const wxPoint& WXUNUSED(cursorHotspot))
{
    return Create(image, cursor);
}

bool wxGenericDragImage::Create(const wxString& str, const wxCursor& cursor, const wxPoint& WXUNUSED(cursorHotspot))
{
    return Create(str, cursor);
}
#endif // WXWIN_COMPATIBILITY_2_6

// Attributes
////////////////////////////////////////////////////////////////////////////


// Operations
////////////////////////////////////////////////////////////////////////////

// Create a drag image with a virtual image (need to override DoDrawImage, GetImageRect)
bool wxGenericDragImage::Create(const wxCursor& cursor)
{
    m_cursor = cursor;

    return true;
}

// Create a drag image from a bitmap and optional cursor
bool wxGenericDragImage::Create(const wxBitmap& image, const wxCursor& cursor)
{
    // We don't have to combine the cursor explicitly since we simply show the cursor
    // as we drag. This currently will only work within one window.

    m_cursor = cursor;
    m_bitmap = image;

    return true ;
}

// Create a drag image from an icon and optional cursor
bool wxGenericDragImage::Create(const wxIcon& image, const wxCursor& cursor)
{
    // We don't have to combine the cursor explicitly since we simply show the cursor
    // as we drag. This currently will only work within one window.

    m_cursor = cursor;
    m_icon = image;

    return true ;
}

// Create a drag image from a string and optional cursor
bool wxGenericDragImage::Create(const wxString& str, const wxCursor& cursor)
{
    wxFont font(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

    long w = 0, h = 0;
    wxScreenDC dc;
    dc.SetFont(font);
    dc.GetTextExtent(str, & w, & h);
    dc.SetFont(wxNullFont);

    wxMemoryDC dc2;

    // Sometimes GetTextExtent isn't accurate enough, so make it longer
    wxBitmap bitmap((int) ((w+2) * 1.5), (int) h+2);
    dc2.SelectObject(bitmap);

    dc2.SetFont(font);
    dc2.SetBackground(* wxWHITE_BRUSH);
    dc2.Clear();
    dc2.SetBackgroundMode(wxTRANSPARENT);
    dc2.SetTextForeground(* wxLIGHT_GREY);
    dc2.DrawText(str, 0, 0);
    dc2.DrawText(str, 1, 0);
    dc2.DrawText(str, 2, 0);
    dc2.DrawText(str, 1, 1);
    dc2.DrawText(str, 2, 1);
    dc2.DrawText(str, 1, 2);
    dc2.DrawText(str, 2, 2);

    dc2.SetTextForeground(* wxBLACK);
    dc2.DrawText(str, 1, 1);

    dc2.SelectObject(wxNullBitmap);

#if wxUSE_IMAGE_IN_DRAGIMAGE && (!defined(__WXMSW__) || wxUSE_WXDIB)
    // Make the bitmap masked
    wxImage image = bitmap.ConvertToImage();
    image.SetMaskColour(255, 255, 255);
    bitmap = wxBitmap(image);
#endif

    return Create(bitmap, cursor);
}

#if wxUSE_TREECTRL
// Create a drag image for the given tree control item
bool wxGenericDragImage::Create(const wxTreeCtrl& treeCtrl, wxTreeItemId& id)
{
    wxString str = treeCtrl.GetItemText(id);
    return Create(str);
}
#endif

#if wxUSE_LISTCTRL
// Create a drag image for the given list control item
bool wxGenericDragImage::Create(const wxListCtrl& listCtrl, long id)
{
    wxString str = listCtrl.GetItemText(id);
    return Create(str);
}
#endif

// Begin drag
bool wxGenericDragImage::BeginDrag(const wxPoint& hotspot,
                                   wxWindow* window,
                                   bool fullScreen,
                                   wxRect* rect)
{
    wxASSERT_MSG( (window != 0), wxT("Window must not be null in BeginDrag."));

    // The image should be offset by this amount
    m_offset = hotspot;
    m_window = window;
    m_fullScreen = fullScreen;

    if (rect)
        m_boundingRect = * rect;

    m_isDirty = false;
    m_isDirty = false;

    if (window)
    {
        window->CaptureMouse();

        if (m_cursor.Ok())
        {
            m_oldCursor = window->GetCursor();
            window->SetCursor(m_cursor);
        }
    }

    // Make a copy of the window so we can repair damage done as the image is
    // dragged.

    wxSize clientSize;
    wxPoint pt;
    if (!m_fullScreen)
    {
        clientSize = window->GetClientSize();
        m_boundingRect.x = 0; m_boundingRect.y = 0;
        m_boundingRect.width = clientSize.x; m_boundingRect.height = clientSize.y;
    }
    else
    {
        int w, h;
        wxDisplaySize(& w, & h);
        clientSize.x = w; clientSize.y = h;
        if (rect)
        {
            pt.x = m_boundingRect.x; pt.y = m_boundingRect.y;
            clientSize.x = m_boundingRect.width; clientSize.y = m_boundingRect.height;
        }
        else
        {
            m_boundingRect.x = 0; m_boundingRect.y = 0;
            m_boundingRect.width = w; m_boundingRect.height = h;
        }
    }

#ifndef wxHAS_NATIVE_OVERLAY
    wxBitmap* backing = (m_pBackingBitmap ? m_pBackingBitmap : (wxBitmap*) & m_backingBitmap);

    if (!backing->Ok() || (backing->GetWidth() < clientSize.x || backing->GetHeight() < clientSize.y))
        (*backing) = wxBitmap(clientSize.x, clientSize.y);
#endif // !wxHAS_NATIVE_OVERLAY

    if (!m_fullScreen)
    {
        m_windowDC = new wxClientDC(window);
    }
    else
    {
        m_windowDC = new wxScreenDC;

#if 0
        // Use m_boundingRect to limit the area considered.
        ((wxScreenDC*) m_windowDC)->StartDrawingOnTop(rect);
#endif

        m_windowDC->SetClippingRegion(m_boundingRect.x, m_boundingRect.y,
            m_boundingRect.width, m_boundingRect.height);
    }

    return true;
}

// Begin drag. hotspot is the location of the drag position relative to the upper-left
// corner of the image. This is full screen only. fullScreenRect gives the
// position of the window on the screen, to restrict the drag to.
bool wxGenericDragImage::BeginDrag(const wxPoint& hotspot, wxWindow* window, wxWindow* fullScreenRect)
{
    wxRect rect;

    int x = fullScreenRect->GetPosition().x;
    int y = fullScreenRect->GetPosition().y;

    wxSize sz = fullScreenRect->GetSize();

    if (fullScreenRect->GetParent() && !fullScreenRect->IsKindOf(CLASSINFO(wxFrame)))
        fullScreenRect->GetParent()->ClientToScreen(& x, & y);

    rect.x = x; rect.y = y;
    rect.width = sz.x; rect.height = sz.y;

    return BeginDrag(hotspot, window, true, & rect);
}

// End drag
bool wxGenericDragImage::EndDrag()
{
    if (m_window)
    {
#ifdef __WXMSW__
        // Under Windows we can be pretty sure this test will give
        // the correct results
        if (wxWindow::GetCapture() == m_window)
#endif
            m_window->ReleaseMouse();

        if (m_cursor.Ok() && m_oldCursor.Ok())
        {
            m_window->SetCursor(m_oldCursor);
        }
    }

    if (m_windowDC)
    {
#ifdef wxHAS_NATIVE_OVERLAY
        m_overlay.Reset();
#else
        m_windowDC->DestroyClippingRegion();
#endif
        delete m_windowDC;
        m_windowDC = (wxDC*) NULL;
    }

#ifndef wxHAS_NATIVE_OVERLAY
    m_repairBitmap = wxNullBitmap;
#endif

    return true;
}

// Move the image: call from OnMouseMove. Pt is in window client coordinates if window
// is non-NULL, or in screen coordinates if NULL.
bool wxGenericDragImage::Move(const wxPoint& pt)
{
    wxASSERT_MSG( (m_windowDC != (wxDC*) NULL), wxT("No window DC in wxGenericDragImage::Move()") );

    wxPoint pt2(pt);
    if (m_fullScreen)
        pt2 = m_window->ClientToScreen(pt);

    // Erase at old position, then show at the current position
    wxPoint oldPos = m_position;

    bool eraseOldImage = (m_isDirty && m_isShown);

    if (m_isShown)
        RedrawImage(oldPos - m_offset, pt2 - m_offset, eraseOldImage, true);

    m_position = pt2;

    if (m_isShown)
        m_isDirty = true;

    return true;
}

bool wxGenericDragImage::Show()
{
    wxASSERT_MSG( (m_windowDC != (wxDC*) NULL), wxT("No window DC in wxGenericDragImage::Show()") );

    // Show at the current position

    if (!m_isShown)
    {
        // This is where we restore the backing bitmap, in case
        // something has changed on the window.

#ifndef wxHAS_NATIVE_OVERLAY
        wxBitmap* backing = (m_pBackingBitmap ? m_pBackingBitmap : (wxBitmap*) & m_backingBitmap);
        wxMemoryDC memDC;
        memDC.SelectObject(* backing);

        UpdateBackingFromWindow(* m_windowDC, memDC, m_boundingRect, wxRect(0, 0, m_boundingRect.width, m_boundingRect.height));

        //memDC.Blit(0, 0, m_boundingRect.width, m_boundingRect.height, m_windowDC, m_boundingRect.x, m_boundingRect.y);
        memDC.SelectObject(wxNullBitmap);
#endif // !wxHAS_NATIVE_OVERLAY

        RedrawImage(m_position - m_offset, m_position - m_offset, false, true);
    }

    m_isShown = true;
    m_isDirty = true;

    return true;
}

bool wxGenericDragImage::UpdateBackingFromWindow(wxDC& windowDC, wxMemoryDC& destDC,
    const wxRect& sourceRect, const wxRect& destRect) const
{
    return destDC.Blit(destRect.x, destRect.y, destRect.width, destRect.height, & windowDC,
        sourceRect.x, sourceRect.y);
}

bool wxGenericDragImage::Hide()
{
    wxASSERT_MSG( (m_windowDC != (wxDC*) NULL), wxT("No window DC in wxGenericDragImage::Hide()") );

    // Repair the old position

    if (m_isShown && m_isDirty)
    {
        RedrawImage(m_position - m_offset, m_position - m_offset, true, false);
    }

    m_isShown = false;
    m_isDirty = false;

    return true;
}

// More efficient: erase and redraw simultaneously if possible
bool wxGenericDragImage::RedrawImage(const wxPoint& oldPos, const wxPoint& newPos,
                                     bool eraseOld, bool drawNew)
{
    if (!m_windowDC)
        return false;

#ifdef wxHAS_NATIVE_OVERLAY
    wxDCOverlay dcoverlay( m_overlay, (wxWindowDC*) m_windowDC ) ;
    if ( eraseOld )
        dcoverlay.Clear() ;
    if (drawNew)
        DoDrawImage(*m_windowDC, newPos);
#else // !wxHAS_NATIVE_OVERLAY
    wxBitmap* backing = (m_pBackingBitmap ? m_pBackingBitmap : (wxBitmap*) & m_backingBitmap);
    if (!backing->Ok())
        return false;

    wxRect oldRect(GetImageRect(oldPos));
    wxRect newRect(GetImageRect(newPos));

    wxRect fullRect;

    // Full rect: the combination of both rects
    if (eraseOld && drawNew)
    {
        int oldRight = oldRect.GetRight();
        int oldBottom = oldRect.GetBottom();
        int newRight = newRect.GetRight();
        int newBottom = newRect.GetBottom();

        wxPoint topLeft = wxPoint(wxMin(oldPos.x, newPos.x), wxMin(oldPos.y, newPos.y));
        wxPoint bottomRight = wxPoint(wxMax(oldRight, newRight), wxMax(oldBottom, newBottom));

        fullRect.x = topLeft.x; fullRect.y = topLeft.y;
        fullRect.SetRight(bottomRight.x);
        fullRect.SetBottom(bottomRight.y);
    }
    else if (eraseOld)
        fullRect = oldRect;
    else if (drawNew)
        fullRect = newRect;

    // Make the bitmap bigger than it need be, so we don't
    // keep reallocating all the time.
    int excess = 50;

    if (!m_repairBitmap.Ok() || (m_repairBitmap.GetWidth() < fullRect.GetWidth() || m_repairBitmap.GetHeight() < fullRect.GetHeight()))
    {
        m_repairBitmap = wxBitmap(fullRect.GetWidth() + excess, fullRect.GetHeight() + excess);
    }

    wxMemoryDC memDC;
    memDC.SelectObject(* backing);

    wxMemoryDC memDCTemp;
    memDCTemp.SelectObject(m_repairBitmap);

    // Draw the backing bitmap onto the repair bitmap.
    // If full-screen, we may have specified the rect on the
    // screen that we're using for our backing bitmap.
    // So subtract this when we're blitting from the backing bitmap
    // (translate from screen to backing-bitmap coords).

    memDCTemp.Blit(0, 0, fullRect.GetWidth(), fullRect.GetHeight(), & memDC, fullRect.x - m_boundingRect.x, fullRect.y - m_boundingRect.y);

    // If drawing, draw the image onto the mem DC
    if (drawNew)
    {
        wxPoint pos(newPos.x - fullRect.x, newPos.y - fullRect.y) ;
        DoDrawImage(memDCTemp, pos);
    }

    // Now blit to the window
    // Finally, blit the temp mem DC to the window.
    m_windowDC->Blit(fullRect.x, fullRect.y, fullRect.width, fullRect.height, & memDCTemp, 0, 0);

    memDCTemp.SelectObject(wxNullBitmap);
    memDC.SelectObject(wxNullBitmap);
#endif // wxHAS_NATIVE_OVERLAY/!wxHAS_NATIVE_OVERLAY

    return true;
}

// Override this if you are using a virtual image (drawing your own image)
bool wxGenericDragImage::DoDrawImage(wxDC& dc, const wxPoint& pos) const
{
    if (m_bitmap.Ok())
    {
        dc.DrawBitmap(m_bitmap, pos.x, pos.y, (m_bitmap.GetMask() != 0));
        return true;
    }
    else if (m_icon.Ok())
    {
        dc.DrawIcon(m_icon, pos.x, pos.y);
        return true;
    }
    else
        return false;
}

// Override this if you are using a virtual image (drawing your own image)
wxRect wxGenericDragImage::GetImageRect(const wxPoint& pos) const
{
    if (m_bitmap.Ok())
    {
        return wxRect(pos.x, pos.y, m_bitmap.GetWidth(), m_bitmap.GetHeight());
    }
    else if (m_icon.Ok())
    {
        return wxRect(pos.x, pos.y, m_icon.GetWidth(), m_icon.GetHeight());
    }
    else
    {
        return wxRect(pos.x, pos.y, 0, 0);
    }
}

#endif // wxUSE_DRAGIMAGE
