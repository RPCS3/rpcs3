/////////////////////////////////////////////////////////////////////////////
// Name:        sashwin.h
// Purpose:     wxSashWindow implementation. A sash window has an optional
//              sash on each edge, allowing it to be dragged. An event
//              is generated when the sash is released.
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: sashwin.h 41020 2006-09-05 20:47:48Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SASHWIN_H_G_
#define _WX_SASHWIN_H_G_

#if wxUSE_SASH

#include "wx/defs.h"
#include "wx/window.h"
#include "wx/string.h"

#define wxSASH_DRAG_NONE       0
#define wxSASH_DRAG_DRAGGING   1
#define wxSASH_DRAG_LEFT_DOWN  2

enum wxSashEdgePosition {
    wxSASH_TOP = 0,
    wxSASH_RIGHT,
    wxSASH_BOTTOM,
    wxSASH_LEFT,
    wxSASH_NONE = 100
};

/*
 * wxSashEdge represents one of the four edges of a window.
 */

class WXDLLIMPEXP_ADV wxSashEdge
{
public:
    wxSashEdge()
    { m_show = false;
#if WXWIN_COMPATIBILITY_2_6
      m_border = false;
#endif
      m_margin = 0; }

    bool    m_show;     // Is the sash showing?
#if WXWIN_COMPATIBILITY_2_6
    bool    m_border;   // Do we draw a border?
#endif
    int     m_margin;   // The margin size
};

/*
 * wxSashWindow flags
 */

#define wxSW_NOBORDER         0x0000
//#define wxSW_3D               0x0010
#define wxSW_BORDER           0x0020
#define wxSW_3DSASH           0x0040
#define wxSW_3DBORDER         0x0080
#define wxSW_3D (wxSW_3DSASH | wxSW_3DBORDER)

/*
 * wxSashWindow allows any of its edges to have a sash which can be dragged
 * to resize the window. The actual content window will be created as a child
 * of wxSashWindow.
 */

class WXDLLIMPEXP_ADV wxSashWindow: public wxWindow
{
public:
    // Default constructor
    wxSashWindow()
    {
        Init();
    }

    // Normal constructor
    wxSashWindow(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxSW_3D|wxCLIP_CHILDREN, const wxString& name = wxT("sashWindow"))
    {
        Init();
        Create(parent, id, pos, size, style, name);
    }

    virtual ~wxSashWindow();

    bool Create(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxSW_3D|wxCLIP_CHILDREN, const wxString& name = wxT("sashWindow"));

    // Set whether there's a sash in this position
    void SetSashVisible(wxSashEdgePosition edge, bool sash);

    // Get whether there's a sash in this position
    bool GetSashVisible(wxSashEdgePosition edge) const { return m_sashes[edge].m_show; }

#if WXWIN_COMPATIBILITY_2_6
    // Set whether there's a border in this position
    // This value is unused in wxSashWindow.
    void SetSashBorder(wxSashEdgePosition edge, bool border) { m_sashes[edge].m_border = border; }

    // Get whether there's a border in this position
    // This value is unused in wxSashWindow.
    bool HasBorder(wxSashEdgePosition edge) const { return m_sashes[edge].m_border; }
#endif

    // Get border size
    int GetEdgeMargin(wxSashEdgePosition edge) const { return m_sashes[edge].m_margin; }

    // Sets the default sash border size
    void SetDefaultBorderSize(int width) { m_borderSize = width; }

    // Gets the default sash border size
    int GetDefaultBorderSize() const { return m_borderSize; }

    // Sets the addition border size between child and sash window
    void SetExtraBorderSize(int width) { m_extraBorderSize = width; }

    // Gets the addition border size between child and sash window
    int GetExtraBorderSize() const { return m_extraBorderSize; }

    virtual void SetMinimumSizeX(int min) { m_minimumPaneSizeX = min; }
    virtual void SetMinimumSizeY(int min) { m_minimumPaneSizeY = min; }
    virtual int GetMinimumSizeX() const { return m_minimumPaneSizeX; }
    virtual int GetMinimumSizeY() const { return m_minimumPaneSizeY; }

    virtual void SetMaximumSizeX(int max) { m_maximumPaneSizeX = max; }
    virtual void SetMaximumSizeY(int max) { m_maximumPaneSizeY = max; }
    virtual int GetMaximumSizeX() const { return m_maximumPaneSizeX; }
    virtual int GetMaximumSizeY() const { return m_maximumPaneSizeY; }

////////////////////////////////////////////////////////////////////////////
// Implementation

    // Paints the border and sash
    void OnPaint(wxPaintEvent& event);

    // Handles mouse events
    void OnMouseEvent(wxMouseEvent& ev);

    // Adjusts the panes
    void OnSize(wxSizeEvent& event);

#if defined(__WXMSW__) || defined(__WXMAC__)
    // Handle cursor correctly
    void OnSetCursor(wxSetCursorEvent& event);
#endif // wxMSW

    // Draws borders
    void DrawBorders(wxDC& dc);

    // Draws the sashes
    void DrawSash(wxSashEdgePosition edge, wxDC& dc);

    // Draws the sashes
    void DrawSashes(wxDC& dc);

    // Draws the sash tracker (for whilst moving the sash)
    void DrawSashTracker(wxSashEdgePosition edge, int x, int y);

    // Tests for x, y over sash
    wxSashEdgePosition SashHitTest(int x, int y, int tolerance = 2);

    // Resizes subwindows
    void SizeWindows();

    // Initialize colours
    void InitColours();

private:
    void Init();

    wxSashEdge  m_sashes[4];
    int         m_dragMode;
    wxSashEdgePosition m_draggingEdge;
    int         m_oldX;
    int         m_oldY;
    int         m_borderSize;
    int         m_extraBorderSize;
    int         m_firstX;
    int         m_firstY;
    int         m_minimumPaneSizeX;
    int         m_minimumPaneSizeY;
    int         m_maximumPaneSizeX;
    int         m_maximumPaneSizeY;
    wxCursor*   m_sashCursorWE;
    wxCursor*   m_sashCursorNS;
    wxColour    m_lightShadowColour;
    wxColour    m_mediumShadowColour;
    wxColour    m_darkShadowColour;
    wxColour    m_hilightColour;
    wxColour    m_faceColour;
    bool        m_mouseCaptured;
    wxCursor*   m_currentCursor;

private:
    DECLARE_DYNAMIC_CLASS(wxSashWindow)
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxSashWindow)
};

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV,
                                wxEVT_SASH_DRAGGED, wxEVT_FIRST + 1200)
END_DECLARE_EVENT_TYPES()

enum wxSashDragStatus
{
    wxSASH_STATUS_OK,
    wxSASH_STATUS_OUT_OF_RANGE
};

class WXDLLIMPEXP_ADV wxSashEvent: public wxCommandEvent
{
public:
    wxSashEvent(int id = 0, wxSashEdgePosition edge = wxSASH_NONE)
    {
        m_eventType = (wxEventType) wxEVT_SASH_DRAGGED;
        m_id = id;
        m_edge = edge;
    }

    void SetEdge(wxSashEdgePosition edge) { m_edge = edge; }
    wxSashEdgePosition GetEdge() const { return m_edge; }

    //// The rectangle formed by the drag operation
    void SetDragRect(const wxRect& rect) { m_dragRect = rect; }
    wxRect GetDragRect() const { return m_dragRect; }

    //// Whether the drag caused the rectangle to be reversed (e.g.
    //// dragging the top below the bottom)
    void SetDragStatus(wxSashDragStatus status) { m_dragStatus = status; }
    wxSashDragStatus GetDragStatus() const { return m_dragStatus; }

private:
    wxSashEdgePosition  m_edge;
    wxRect              m_dragRect;
    wxSashDragStatus    m_dragStatus;

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxSashEvent)
};

typedef void (wxEvtHandler::*wxSashEventFunction)(wxSashEvent&);

#define wxSashEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxSashEventFunction, &func)

#define EVT_SASH_DRAGGED(id, fn) \
    wx__DECLARE_EVT1(wxEVT_SASH_DRAGGED, id, wxSashEventHandler(fn))
#define EVT_SASH_DRAGGED_RANGE(id1, id2, fn) \
    wx__DECLARE_EVT2(wxEVT_SASH_DRAGGED, id1, id2, wxSashEventHandler(fn))

#endif // wxUSE_SASH

#endif
  // _WX_SASHWIN_H_G_
