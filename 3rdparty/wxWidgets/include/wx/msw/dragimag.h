/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/dragimag.h
// Purpose:     wxDragImage class: a kind of a cursor, that can cope
//              with more sophisticated images
// Author:      Julian Smart
// Modified by:
// Created:     08/04/99
// RCS-ID:      $Id: dragimag.h 45845 2007-05-05 19:00:35Z PC $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DRAGIMAG_H_
#define _WX_DRAGIMAG_H_

#if wxUSE_DRAGIMAGE

#include "wx/bitmap.h"
#include "wx/icon.h"
#include "wx/cursor.h"
#include "wx/treectrl.h"
#include "wx/listctrl.h"

// If 1, use a simple wxCursor instead of ImageList_SetDragCursorImage
#define wxUSE_SIMPLER_DRAGIMAGE 0

/*
  To use this class, create a wxDragImage when you start dragging, for example:

  void MyTreeCtrl::OnBeginDrag(wxTreeEvent& event)
  {
#ifdef __WXMSW__
    ::UpdateWindow((HWND) GetHWND()); // We need to implement this in wxWidgets
#endif

    CaptureMouse();

    m_dragImage = new wxDragImage(* this, itemId);
    m_dragImage->BeginDrag(wxPoint(0, 0), this);
    m_dragImage->Move(pt, this);
    m_dragImage->Show(this);
    ...
  }

  In your OnMouseMove function, hide the image, do any display updating required,
  then move and show the image again:

  void MyTreeCtrl::OnMouseMove(wxMouseEvent& event)
  {
    if (m_dragMode == MY_TREE_DRAG_NONE)
    {
        event.Skip();
        return;
    }

    // Prevent screen corruption by hiding the image
    if (m_dragImage)
        m_dragImage->Hide(this);

    // Do some updating of the window, such as highlighting the drop target
    ...

#ifdef __WXMSW__
    if (updateWindow)
        ::UpdateWindow((HWND) GetHWND());
#endif

    // Move and show the image again
    m_dragImage->Move(event.GetPosition(), this);
    m_dragImage->Show(this);
 }

 Eventually we end the drag and delete the drag image.

 void MyTreeCtrl::OnLeftUp(wxMouseEvent& event)
 {
    ...

    // End the drag and delete the drag image
    if (m_dragImage)
    {
        m_dragImage->EndDrag(this);
        delete m_dragImage;
        m_dragImage = NULL;
    }
    ReleaseMouse();
 }
*/

/*
 Notes for Unix version:
 Can we simply use cursors instead, creating a cursor dynamically, setting it into the window
 in BeginDrag, and restoring the old cursor in EndDrag?
 For a really bog-standard implementation, we could simply use a normal dragging cursor
 and ignore the image.
*/

/*
 * wxDragImage
 */

class WXDLLEXPORT wxDragImage: public wxObject
{
public:

    // Ctors & dtor
    ////////////////////////////////////////////////////////////////////////////

    wxDragImage();
    wxDragImage(const wxBitmap& image, const wxCursor& cursor = wxNullCursor)
    {
        Init();

        Create(image, cursor);
    }

    // Deprecated form of the above
    wxDragImage(const wxBitmap& image, const wxCursor& cursor, const wxPoint& cursorHotspot)
    {
        Init();

        Create(image, cursor, cursorHotspot);
    }

    wxDragImage(const wxIcon& image, const wxCursor& cursor = wxNullCursor)
    {
        Init();

        Create(image, cursor);
    }

    // Deprecated form of the above
    wxDragImage(const wxIcon& image, const wxCursor& cursor, const wxPoint& cursorHotspot)
    {
        Init();

        Create(image, cursor, cursorHotspot);
    }

    wxDragImage(const wxString& str, const wxCursor& cursor = wxNullCursor)
    {
        Init();

        Create(str, cursor);
    }

    // Deprecated form of the above
    wxDragImage(const wxString& str, const wxCursor& cursor, const wxPoint& cursorHotspot)
    {
        Init();

        Create(str, cursor, cursorHotspot);
    }

#if wxUSE_TREECTRL
    wxDragImage(const wxTreeCtrl& treeCtrl, wxTreeItemId& id)
    {
        Init();

        Create(treeCtrl, id);
    }
#endif

#if wxUSE_LISTCTRL
    wxDragImage(const wxListCtrl& listCtrl, long id)
    {
        Init();

        Create(listCtrl, id);
    }
#endif

    virtual ~wxDragImage();

    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    // Operations
    ////////////////////////////////////////////////////////////////////////////

    // Create a drag image from a bitmap and optional cursor
    bool Create(const wxBitmap& image, const wxCursor& cursor = wxNullCursor);
    bool Create(const wxBitmap& image, const wxCursor& cursor, const wxPoint& WXUNUSED(cursorHotspot))
    {
        wxLogDebug(wxT("wxDragImage::Create: use of a cursor hotspot is now deprecated. Please omit this argument."));
        return Create(image, cursor);
    }

    // Create a drag image from an icon and optional cursor
    bool Create(const wxIcon& image, const wxCursor& cursor = wxNullCursor);
    bool Create(const wxIcon& image, const wxCursor& cursor, const wxPoint& WXUNUSED(cursorHotspot))
    {
        wxLogDebug(wxT("wxDragImage::Create: use of a cursor hotspot is now deprecated. Please omit this argument."));
        return Create(image, cursor);
    }

    // Create a drag image from a string and optional cursor
    bool Create(const wxString& str, const wxCursor& cursor = wxNullCursor);
    bool Create(const wxString& str, const wxCursor& cursor, const wxPoint& WXUNUSED(cursorHotspot))
    {
        wxLogDebug(wxT("wxDragImage::Create: use of a cursor hotspot is now deprecated. Please omit this argument."));
        return Create(str, cursor);
    }

#if wxUSE_TREECTRL
    // Create a drag image for the given tree control item
    bool Create(const wxTreeCtrl& treeCtrl, wxTreeItemId& id);
#endif

#if wxUSE_LISTCTRL
    // Create a drag image for the given list control item
    bool Create(const wxListCtrl& listCtrl, long id);
#endif

    // Begin drag. hotspot is the location of the drag position relative to the upper-left
    // corner of the image.
    bool BeginDrag(const wxPoint& hotspot, wxWindow* window, bool fullScreen = false, wxRect* rect = (wxRect*) NULL);

    // Begin drag. hotspot is the location of the drag position relative to the upper-left
    // corner of the image. This is full screen only. fullScreenRect gives the
    // position of the window on the screen, to restrict the drag to.
    bool BeginDrag(const wxPoint& hotspot, wxWindow* window, wxWindow* fullScreenRect);

    // End drag
    bool EndDrag();

    // Move the image: call from OnMouseMove. Pt is in window client coordinates if window
    // is non-NULL, or in screen coordinates if NULL.
    bool Move(const wxPoint& pt);

    // Show the image
    bool Show();

    // Hide the image
    bool Hide();

    // Implementation
    ////////////////////////////////////////////////////////////////////////////

    // Initialize variables
    void Init();

    // Returns the native image list handle
    WXHIMAGELIST GetHIMAGELIST() const { return m_hImageList; }

#if !wxUSE_SIMPLER_DRAGIMAGE
    // Returns the native image list handle for the cursor
    WXHIMAGELIST GetCursorHIMAGELIST() const { return m_hCursorImageList; }
#endif

protected:
    WXHIMAGELIST    m_hImageList;

#if wxUSE_SIMPLER_DRAGIMAGE
    wxCursor        m_oldCursor;
#else
    WXHIMAGELIST    m_hCursorImageList;
#endif

    wxCursor        m_cursor;
//    wxPoint         m_cursorHotspot; // Obsolete
    wxPoint         m_position;
    wxWindow*       m_window;
    wxRect          m_boundingRect;
    bool            m_fullScreen;

private:
    DECLARE_DYNAMIC_CLASS(wxDragImage)
    DECLARE_NO_COPY_CLASS(wxDragImage)
};

#endif // wxUSE_DRAGIMAGE
#endif
    // _WX_DRAGIMAG_H_
