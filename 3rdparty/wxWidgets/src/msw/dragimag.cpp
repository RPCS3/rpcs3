/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/dragimag.cpp
// Purpose:     wxDragImage
// Author:      Julian Smart
// Modified by:
// Created:     08/04/99
// RCS-ID:      $Id: dragimag.cpp 43883 2006-12-09 19:49:02Z PC $
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
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include <stdio.h>
    #include "wx/window.h"
    #include "wx/dcclient.h"
    #include "wx/dcscreen.h"
    #include "wx/dcmemory.h"
    #include "wx/settings.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/frame.h"
    #include "wx/image.h"
#endif

#include "wx/msw/private.h"

#include "wx/msw/dragimag.h"
#include "wx/msw/private.h"

#ifdef __WXWINCE__  // for SM_CXCURSOR and SM_CYCURSOR
#include "wx/msw/wince/missing.h"
#endif // __WXWINCE__

// Wine doesn't have this yet
#ifndef ListView_CreateDragImage
#define ListView_CreateDragImage(hwnd, i, lpptUpLeft) \
    (HIMAGELIST)SNDMSG((hwnd), LVM_CREATEDRAGIMAGE, (WPARAM)(int)(i), (LPARAM)(LPPOINT)(lpptUpLeft))
#endif

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxDragImage, wxObject)

#define GetHimageList() ((HIMAGELIST) m_hImageList)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxDragImage ctors/dtor
// ----------------------------------------------------------------------------

wxDragImage::wxDragImage()
{
    Init();
}

wxDragImage::~wxDragImage()
{
    if ( m_hImageList )
        ImageList_Destroy(GetHimageList());
#if !wxUSE_SIMPLER_DRAGIMAGE
    if ( m_hCursorImageList )
        ImageList_Destroy((HIMAGELIST) m_hCursorImageList);
#endif
}

void wxDragImage::Init()
{
    m_hImageList = 0;
#if !wxUSE_SIMPLER_DRAGIMAGE
    m_hCursorImageList = 0;
#endif
    m_window = (wxWindow*) NULL;
    m_fullScreen = false;
}

// Attributes
////////////////////////////////////////////////////////////////////////////


// Operations
////////////////////////////////////////////////////////////////////////////

// Create a drag image from a bitmap and optional cursor
bool wxDragImage::Create(const wxBitmap& image, const wxCursor& cursor)
{
    if ( m_hImageList )
        ImageList_Destroy(GetHimageList());
    m_hImageList = 0;

#ifdef __WXWINCE__
    UINT flags = ILC_COLOR;
#else
    UINT flags wxDUMMY_INITIALIZE(0) ;
    if (image.GetDepth() <= 4)
        flags = ILC_COLOR4;
    else if (image.GetDepth() <= 8)
        flags = ILC_COLOR8;
    else if (image.GetDepth() <= 16)
        flags = ILC_COLOR16;
    else if (image.GetDepth() <= 24)
        flags = ILC_COLOR24;
    else
        flags = ILC_COLOR32;
#endif

    bool mask = (image.GetMask() != 0);

    // Curiously, even if the image doesn't have a mask,
    // we still have to use ILC_MASK or the image won't show
    // up when dragged.
//    if ( mask )
    flags |= ILC_MASK;

    m_hImageList = (WXHIMAGELIST) ImageList_Create(image.GetWidth(), image.GetHeight(), flags, 1, 1);

    int index;
    if (!mask)
    {
        HBITMAP hBitmap1 = (HBITMAP) image.GetHBITMAP();
        index = ImageList_Add(GetHimageList(), hBitmap1, 0);
    }
    else
    {
        HBITMAP hBitmap1 = (HBITMAP) image.GetHBITMAP();
        HBITMAP hBitmap2 = (HBITMAP) image.GetMask()->GetMaskBitmap();
        HBITMAP hbmpMask = wxInvertMask(hBitmap2);

        index = ImageList_Add(GetHimageList(), hBitmap1, hbmpMask);
        ::DeleteObject(hbmpMask);
    }
    if ( index == -1 )
    {
        wxLogError(_("Couldn't add an image to the image list."));
    }
    m_cursor = cursor; // Can only combine with drag image after calling BeginDrag.

    return (index != -1) ;
}

// Create a drag image from an icon and optional cursor
bool wxDragImage::Create(const wxIcon& image, const wxCursor& cursor)
{
    if ( m_hImageList )
        ImageList_Destroy(GetHimageList());
    m_hImageList = 0;

#ifdef __WXWINCE__
    UINT flags = ILC_COLOR;
#else
    UINT flags wxDUMMY_INITIALIZE(0) ;
    if (image.GetDepth() <= 4)
        flags = ILC_COLOR4;
    else if (image.GetDepth() <= 8)
        flags = ILC_COLOR8;
    else if (image.GetDepth() <= 16)
        flags = ILC_COLOR16;
    else if (image.GetDepth() <= 24)
        flags = ILC_COLOR24;
    else
        flags = ILC_COLOR32;
#endif

    flags |= ILC_MASK;

    m_hImageList = (WXHIMAGELIST) ImageList_Create(image.GetWidth(), image.GetHeight(), flags, 1, 1);

    HICON hIcon = (HICON) image.GetHICON();

    int index = ImageList_AddIcon(GetHimageList(), hIcon);
    if ( index == -1 )
    {
        wxLogError(_("Couldn't add an image to the image list."));
    }

    m_cursor = cursor; // Can only combine with drag image after calling BeginDrag.

    return (index != -1) ;
}

// Create a drag image from a string and optional cursor
bool wxDragImage::Create(const wxString& str, const wxCursor& cursor)
{
    wxFont font(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

    long w = 0, h = 0;
    wxScreenDC dc;
    dc.SetFont(font);
    dc.GetTextExtent(str, & w, & h);
    dc.SetFont(wxNullFont);

    wxMemoryDC dc2;
    dc2.SetFont(font);
    wxBitmap bitmap((int) w+2, (int) h+2);
    dc2.SelectObject(bitmap);

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

#if wxUSE_WXDIB
    // Make the bitmap masked
    wxImage image = bitmap.ConvertToImage();
    image.SetMaskColour(255, 255, 255);
    return Create(wxBitmap(image), cursor);
#else
    return false;
#endif
}

#if wxUSE_TREECTRL
// Create a drag image for the given tree control item
bool wxDragImage::Create(const wxTreeCtrl& treeCtrl, wxTreeItemId& id)
{
    if ( m_hImageList )
        ImageList_Destroy(GetHimageList());
    m_hImageList = (WXHIMAGELIST)
        TreeView_CreateDragImage(GetHwndOf(&treeCtrl), (HTREEITEM) id.m_pItem);
    return m_hImageList != 0;
}
#endif

#if wxUSE_LISTCTRL
// Create a drag image for the given list control item
bool wxDragImage::Create(const wxListCtrl& listCtrl, long id)
{
    if ( m_hImageList )
        ImageList_Destroy(GetHimageList());
    POINT pt;
    pt.x = 0; pt.y = 0;
    m_hImageList = (WXHIMAGELIST) ListView_CreateDragImage((HWND) listCtrl.GetHWND(), id, & pt);
    return true;
}
#endif

// Begin drag
bool wxDragImage::BeginDrag(const wxPoint& hotspot, wxWindow* window, bool fullScreen, wxRect* rect)
{
    wxASSERT_MSG( (m_hImageList != 0), wxT("Image list must not be null in BeginDrag."));
    wxASSERT_MSG( (window != 0), wxT("Window must not be null in BeginDrag."));

    m_fullScreen = fullScreen;
    if (rect)
        m_boundingRect = * rect;

    bool ret = (ImageList_BeginDrag(GetHimageList(), 0, hotspot.x, hotspot.y) != 0);

    if (!ret)
    {
        wxFAIL_MSG( _T("BeginDrag failed.") );

        return false;
    }

    if (m_cursor.Ok())
    {
#if wxUSE_SIMPLER_DRAGIMAGE
        m_oldCursor = window->GetCursor();
        window->SetCursor(m_cursor);
#else
        if (!m_hCursorImageList)
        {
#ifndef SM_CXCURSOR
            // Smartphone may not have these metric symbol
            int cxCursor = 16;
            int cyCursor = 16;
#else
            int cxCursor = ::GetSystemMetrics(SM_CXCURSOR);
            int cyCursor = ::GetSystemMetrics(SM_CYCURSOR);
#endif
            m_hCursorImageList = (WXHIMAGELIST) ImageList_Create(cxCursor, cyCursor, ILC_MASK, 1, 1);
        }

        // See if we can find the cursor hotspot
        wxPoint curHotSpot(hotspot);

        // Although it seems to produce the right position, when the hotspot goeos
        // negative it has strange effects on the image.
        // How do we stop the cursor jumping right and below of where it should be?
#if 0
        ICONINFO iconInfo;
        if (::GetIconInfo((HICON) (HCURSOR) m_cursor.GetHCURSOR(), & iconInfo) != 0)
        {
            curHotSpot.x -= iconInfo.xHotspot;
            curHotSpot.y -= iconInfo.yHotspot;
        }
#endif
        //wxString msg;
        //msg.Printf("Hotspot = %d, %d", curHotSpot.x, curHotSpot.y);
        //wxLogDebug(msg);

        // First add the cursor to the image list
        HCURSOR hCursor = (HCURSOR) m_cursor.GetHCURSOR();
        int cursorIndex = ImageList_AddIcon((HIMAGELIST) m_hCursorImageList, (HICON) hCursor);

        wxASSERT_MSG( (cursorIndex != -1), wxT("ImageList_AddIcon failed in BeginDrag."));

        if (cursorIndex != -1)
        {
            ImageList_SetDragCursorImage((HIMAGELIST) m_hCursorImageList, cursorIndex, curHotSpot.x, curHotSpot.y);
        }
#endif
    }

#if !wxUSE_SIMPLER_DRAGIMAGE
    if (m_cursor.Ok())
        ::ShowCursor(FALSE);
#endif

    m_window = window;

    ::SetCapture(GetHwndOf(window));

    return true;
}

// Begin drag. hotspot is the location of the drag position relative to the upper-left
// corner of the image. This is full screen only. fullScreenRect gives the
// position of the window on the screen, to restrict the drag to.
bool wxDragImage::BeginDrag(const wxPoint& hotspot, wxWindow* window, wxWindow* fullScreenRect)
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
bool wxDragImage::EndDrag()
{
    wxASSERT_MSG( (m_hImageList != 0), wxT("Image list must not be null in EndDrag."));

    ImageList_EndDrag();

    if ( !::ReleaseCapture() )
    {
        wxLogLastError(wxT("ReleaseCapture"));
    }

#if wxUSE_SIMPLER_DRAGIMAGE
    if (m_cursor.Ok() && m_oldCursor.Ok())
        m_window->SetCursor(m_oldCursor);
#else
    ::ShowCursor(TRUE);
#endif

    m_window = (wxWindow*) NULL;

    return true;
}

// Move the image: call from OnMouseMove. Pt is in window client coordinates if window
// is non-NULL, or in screen coordinates if NULL.
bool wxDragImage::Move(const wxPoint& pt)
{
    wxASSERT_MSG( (m_hImageList != 0), wxT("Image list must not be null in Move."));

    // These are in window, not client coordinates.
    // So need to convert to client coordinates.
    wxPoint pt2(pt);
    if (m_window && !m_fullScreen)
    {
        RECT rect;
        rect.left = 0; rect.top = 0;
        rect.right = 0; rect.bottom = 0;
        DWORD style = ::GetWindowLong((HWND) m_window->GetHWND(), GWL_STYLE);
#ifdef __WIN32__
        DWORD exStyle = ::GetWindowLong((HWND) m_window->GetHWND(), GWL_EXSTYLE);
        ::AdjustWindowRectEx(& rect, style, FALSE, exStyle);
#else
        ::AdjustWindowRect(& rect, style, FALSE);
#endif
        // Subtract the (negative) values, i.e. add a small increment
        pt2.x -= rect.left; pt2.y -= rect.top;
    }
    else if (m_window && m_fullScreen)
    {
        pt2 = m_window->ClientToScreen(pt2);
    }

    bool ret = (ImageList_DragMove( pt2.x, pt2.y ) != 0);

    m_position = pt2;

    return ret;
}

bool wxDragImage::Show()
{
    wxASSERT_MSG( (m_hImageList != 0), wxT("Image list must not be null in Show."));

    HWND hWnd = 0;
    if (m_window && !m_fullScreen)
        hWnd = (HWND) m_window->GetHWND();

    bool ret = (ImageList_DragEnter( hWnd, m_position.x, m_position.y ) != 0);

    return ret;
}

bool wxDragImage::Hide()
{
    wxASSERT_MSG( (m_hImageList != 0), wxT("Image list must not be null in Hide."));

    HWND hWnd = 0;
    if (m_window && !m_fullScreen)
        hWnd = (HWND) m_window->GetHWND();

    bool ret = (ImageList_DragLeave( hWnd ) != 0);

    return ret;
}

#endif // wxUSE_DRAGIMAGE
