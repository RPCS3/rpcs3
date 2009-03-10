/////////////////////////////////////////////////////////////////////////////
// Name:        laywin.h
// Purpose:     Implements a simple layout algorithm, plus
//              wxSashLayoutWindow which is an example of a window with
//              layout-awareness (via event handlers). This is suited to
//              IDE-style window layout.
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: laywin.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_LAYWIN_H_G_
#define _WX_LAYWIN_H_G_

#if wxUSE_SASH
    #include "wx/sashwin.h"
#endif // wxUSE_SASH

#include "wx/event.h"

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_QUERY_LAYOUT_INFO, 1500)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_CALCULATE_LAYOUT, 1501)
END_DECLARE_EVENT_TYPES()

enum wxLayoutOrientation
{
    wxLAYOUT_HORIZONTAL,
    wxLAYOUT_VERTICAL
};

enum wxLayoutAlignment
{
    wxLAYOUT_NONE,
    wxLAYOUT_TOP,
    wxLAYOUT_LEFT,
    wxLAYOUT_RIGHT,
    wxLAYOUT_BOTTOM
};

// Not sure this is necessary
// Tell window which dimension we're sizing on
#define wxLAYOUT_LENGTH_Y       0x0008
#define wxLAYOUT_LENGTH_X       0x0000

// Use most recently used length
#define wxLAYOUT_MRU_LENGTH     0x0010

// Only a query, so don't actually move it.
#define wxLAYOUT_QUERY          0x0100

/*
 * This event is used to get information about window alignment,
 * orientation and size.
 */

class WXDLLIMPEXP_ADV wxQueryLayoutInfoEvent: public wxEvent
{
public:
    wxQueryLayoutInfoEvent(wxWindowID id = 0)
    {
        SetEventType(wxEVT_QUERY_LAYOUT_INFO);
        m_requestedLength = 0;
        m_flags = 0;
        m_id = id;
        m_alignment = wxLAYOUT_TOP;
        m_orientation = wxLAYOUT_HORIZONTAL;
    }

    // Read by the app
    void SetRequestedLength(int length) { m_requestedLength = length; }
    int GetRequestedLength() const { return m_requestedLength; }

    void SetFlags(int flags) { m_flags = flags; }
    int GetFlags() const { return m_flags; }

    // Set by the app
    void SetSize(const wxSize& size) { m_size = size; }
    wxSize GetSize() const { return m_size; }

    void SetOrientation(wxLayoutOrientation orient) { m_orientation = orient; }
    wxLayoutOrientation GetOrientation() const { return m_orientation; }

    void SetAlignment(wxLayoutAlignment align) { m_alignment = align; }
    wxLayoutAlignment GetAlignment() const { return m_alignment; }

    virtual wxEvent *Clone() const { return new wxQueryLayoutInfoEvent(*this); }

protected:
    int                     m_flags;
    int                     m_requestedLength;
    wxSize                  m_size;
    wxLayoutOrientation     m_orientation;
    wxLayoutAlignment       m_alignment;

private:
    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxQueryLayoutInfoEvent)
};

typedef void (wxEvtHandler::*wxQueryLayoutInfoEventFunction)(wxQueryLayoutInfoEvent&);

#define EVT_QUERY_LAYOUT_INFO(func) \
    DECLARE_EVENT_TABLE_ENTRY( wxEVT_QUERY_LAYOUT_INFO, wxID_ANY, wxID_ANY, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxQueryLayoutInfoEventFunction, & func ), NULL ),

/*
 * This event is used to take a bite out of the available client area.
 */

class WXDLLIMPEXP_ADV wxCalculateLayoutEvent: public wxEvent
{
public:
    wxCalculateLayoutEvent(wxWindowID id = 0)
    {
        SetEventType(wxEVT_CALCULATE_LAYOUT);
        m_flags = 0;
        m_id = id;
    }

    // Read by the app
    void SetFlags(int flags) { m_flags = flags; }
    int GetFlags() const { return m_flags; }

    // Set by the app
    void SetRect(const wxRect& rect) { m_rect = rect; }
    wxRect GetRect() const { return m_rect; }

    virtual wxEvent *Clone() const { return new wxCalculateLayoutEvent(*this); }

protected:
    int                     m_flags;
    wxRect                  m_rect;

private:
    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxCalculateLayoutEvent)
};

typedef void (wxEvtHandler::*wxCalculateLayoutEventFunction)(wxCalculateLayoutEvent&);

#define EVT_CALCULATE_LAYOUT(func) \
    DECLARE_EVENT_TABLE_ENTRY( wxEVT_CALCULATE_LAYOUT, wxID_ANY, wxID_ANY, (wxObjectEventFunction) (wxEventFunction)  wxStaticCastEvent( wxCalculateLayoutEventFunction, & func ), NULL ),

#if wxUSE_SASH

// This is window that can remember alignment/orientation, does its own layout,
// and can provide sashes too. Useful for implementing docked windows with sashes in
// an IDE-style interface.
class WXDLLIMPEXP_ADV wxSashLayoutWindow: public wxSashWindow
{
public:
    wxSashLayoutWindow()
    {
        Init();
    }

    wxSashLayoutWindow(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxSW_3D|wxCLIP_CHILDREN, const wxString& name = wxT("layoutWindow"))
    {
        Create(parent, id, pos, size, style, name);
    }

    bool Create(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = wxSW_3D|wxCLIP_CHILDREN, const wxString& name = wxT("layoutWindow"));

// Accessors
    inline wxLayoutAlignment GetAlignment() const { return m_alignment; }
    inline wxLayoutOrientation GetOrientation() const { return m_orientation; }

    inline void SetAlignment(wxLayoutAlignment align) { m_alignment = align; }
    inline void SetOrientation(wxLayoutOrientation orient) { m_orientation = orient; }

    // Give the window default dimensions
    inline void SetDefaultSize(const wxSize& size) { m_defaultSize = size; }

// Event handlers
    // Called by layout algorithm to allow window to take a bit out of the
    // client rectangle, and size itself if not in wxLAYOUT_QUERY mode.
    void OnCalculateLayout(wxCalculateLayoutEvent& event);

    // Called by layout algorithm to retrieve information about the window.
    void OnQueryLayoutInfo(wxQueryLayoutInfoEvent& event);

private:
    void Init();

    wxLayoutAlignment           m_alignment;
    wxLayoutOrientation         m_orientation;
    wxSize                      m_defaultSize;

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxSashLayoutWindow)
    DECLARE_EVENT_TABLE()
};

#endif // wxUSE_SASH

class WXDLLIMPEXP_FWD_CORE wxMDIParentFrame;
class WXDLLIMPEXP_FWD_CORE wxFrame;

// This class implements the layout algorithm
class WXDLLIMPEXP_ADV wxLayoutAlgorithm: public wxObject
{
public:
    wxLayoutAlgorithm() {}

#if wxUSE_MDI_ARCHITECTURE
    // The MDI client window is sized to whatever's left over.
    bool LayoutMDIFrame(wxMDIParentFrame* frame, wxRect* rect = (wxRect*) NULL);
#endif // wxUSE_MDI_ARCHITECTURE

    // mainWindow is sized to whatever's left over. This function for backward
    // compatibility; use LayoutWindow.
    bool LayoutFrame(wxFrame* frame, wxWindow* mainWindow = (wxWindow*) NULL);

    // mainWindow is sized to whatever's left over.
    bool LayoutWindow(wxWindow* frame, wxWindow* mainWindow = (wxWindow*) NULL);
};

#endif
    // _WX_LAYWIN_H_G_
