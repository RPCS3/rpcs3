///////////////////////////////////////////////////////////////////////////////
// Name:        wx/aui/toolbar.h
// Purpose:     wxaui: wx advanced user interface - docking window manager
// Author:      Benjamin I. Williams
// Modified by:
// Created:     2008-08-04
// RCS-ID:      $Id: auibar.h 59850 2009-03-25 13:41:38Z BIW $
// Copyright:   (C) Copyright 2005, Kirix Corporation, All Rights Reserved.
// Licence:     wxWindows Library Licence, Version 3.1
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_AUIBAR_H_
#define _WX_AUIBAR_H_

#include "wx/defs.h"

#if wxUSE_AUI

#if wxABI_VERSION >= 20809

#include "wx/control.h"

enum wxAuiToolBarStyle
{
    wxAUI_TB_TEXT          = 1 << 0,
    wxAUI_TB_NO_TOOLTIPS   = 1 << 1,
    wxAUI_TB_NO_AUTORESIZE = 1 << 2,
    wxAUI_TB_GRIPPER       = 1 << 3,
    wxAUI_TB_OVERFLOW      = 1 << 4,
    wxAUI_TB_VERTICAL      = 1 << 5,
    wxAUI_TB_HORZ_LAYOUT   = 1 << 6,
    wxAUI_TB_HORZ_TEXT     = (wxAUI_TB_HORZ_LAYOUT | wxAUI_TB_TEXT),
    wxAUI_TB_DEFAULT_STYLE = 0
};

enum wxAuiToolBarArtSetting
{
    wxAUI_TBART_SEPARATOR_SIZE = 0,
    wxAUI_TBART_GRIPPER_SIZE = 1,
    wxAUI_TBART_OVERFLOW_SIZE = 2
};

enum wxAuiToolBarToolTextOrientation
{
    wxAUI_TBTOOL_TEXT_LEFT = 0,     // unused/unimplemented
    wxAUI_TBTOOL_TEXT_RIGHT = 1,
    wxAUI_TBTOOL_TEXT_TOP = 2,      // unused/unimplemented
    wxAUI_TBTOOL_TEXT_BOTTOM = 3
};


// aui toolbar event class

class WXDLLIMPEXP_AUI wxAuiToolBarEvent : public wxNotifyEvent
{
public:
    wxAuiToolBarEvent(wxEventType command_type = wxEVT_NULL,
                      int win_id = 0)
          : wxNotifyEvent(command_type, win_id)
    {
        is_dropdown_clicked = false;
        click_pt = wxPoint(-1, -1);
        rect = wxRect(-1,-1, 0, 0);
        tool_id = -1;
    }
#ifndef SWIG
    wxAuiToolBarEvent(const wxAuiToolBarEvent& c) : wxNotifyEvent(c)
    {
        is_dropdown_clicked = c.is_dropdown_clicked;
        click_pt = c.click_pt;
        rect = c.rect;
        tool_id = c.tool_id;
    }
#endif
    wxEvent *Clone() const { return new wxAuiToolBarEvent(*this); }

    bool IsDropDownClicked() const  { return is_dropdown_clicked; }
    void SetDropDownClicked(bool c) { is_dropdown_clicked = c;    }

    wxPoint GetClickPoint() const        { return click_pt; }
    void SetClickPoint(const wxPoint& p) { click_pt = p;    }

    wxRect GetItemRect() const        { return rect; }
    void SetItemRect(const wxRect& r) { rect = r;    }

    int GetToolId() const  { return tool_id; }
    void SetToolId(int id) { tool_id = id;   }

private:

    bool is_dropdown_clicked;
    wxPoint click_pt;
    wxRect rect;
    int tool_id;

#ifndef SWIG
private:
    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxAuiToolBarEvent)
#endif
};


class WXDLLIMPEXP_AUI wxAuiToolBarItem
{
    friend class wxAuiToolBar;
    
public:

    wxAuiToolBarItem()
    {
        window = NULL;
        sizer_item = NULL;
        spacer_pixels = 0;
        id = 0;
        kind = wxITEM_NORMAL;
        state = 0;  // normal, enabled
        proportion = 0;
        active = true;
        dropdown = true;
        sticky = true;
        user_data = 0;
    }

    wxAuiToolBarItem(const wxAuiToolBarItem& c)
    {
        Assign(c);
    }

    wxAuiToolBarItem& operator=(const wxAuiToolBarItem& c)
    {
        Assign(c);
        return *this;
    }

    void Assign(const wxAuiToolBarItem& c)
    {
        window = c.window;
        label = c.label;
        bitmap = c.bitmap;
        disabled_bitmap = c.disabled_bitmap;
        hover_bitmap = c.hover_bitmap;
        short_help = c.short_help;
        long_help = c.long_help;
        sizer_item = c.sizer_item;
        min_size = c.min_size;
        spacer_pixels = c.spacer_pixels;
        id = c.id;
        kind = c.kind;
        state = c.state;
        proportion = c.proportion;
        active = c.active;
        dropdown = c.dropdown;
        sticky = c.sticky;
        user_data = c.user_data;
    }
    
    
    void SetWindow(wxWindow* w) { window = w; }
    wxWindow* GetWindow() { return window; }
    
    void SetId(int new_id) { id = new_id; }
    int GetId() const { return id; }
    
    void SetKind(int new_kind) { kind = new_kind; }
    int GetKind() const { return kind; }
    
    void SetState(int new_state) { state = new_state; }
    int GetState() const { return state; }
    
    void SetSizerItem(wxSizerItem* s) { sizer_item = s; }
    wxSizerItem* GetSizerItem() const { return sizer_item; }
    
    void SetLabel(const wxString& s) { label = s; }
    const wxString& GetLabel() const { return label; }
    
    void SetBitmap(const wxBitmap& bmp) { bitmap = bmp; }
    const wxBitmap& GetBitmap() const { return bitmap; }
    
    void SetDisabledBitmap(const wxBitmap& bmp) { disabled_bitmap = bmp; }
    const wxBitmap& GetDisabledBitmap() const { return disabled_bitmap; }
    
    void SetHoverBitmap(const wxBitmap& bmp) { hover_bitmap = bmp; }
    const wxBitmap& GetHoverBitmap() const { return hover_bitmap; }
    
    void SetShortHelp(const wxString& s) { short_help = s; }
    const wxString& GetShortHelp() const { return short_help; }
    
    void SetLongHelp(const wxString& s) { long_help = s; }
    const wxString& GetLongHelp() const { return long_help; }
    
    void SetMinSize(const wxSize& s) { min_size = s; }
    const wxSize& GetMinSize() const { return min_size; }
    
    void SetSpacerPixels(int s) { spacer_pixels = s; }
    int GetSpacerPixels() const { return spacer_pixels; }
    
    void SetProportion(int p) { proportion = p; }
    int GetProportion() const { return proportion; }
    
    void SetActive(bool b) { active = b; }
    bool IsActive() const { return active; }
    
    void SetHasDropDown(bool b) { dropdown = b; }
    bool HasDropDown() const { return dropdown; }
    
    void SetSticky(bool b) { sticky = b; }
    bool IsSticky() const { return sticky; }
    
    void SetUserData(long l) { user_data = l; }
    long GetUserData() const { return user_data; }

private:

    wxWindow* window;          // item's associated window
    wxString label;            // label displayed on the item
    wxBitmap bitmap;           // item's bitmap
    wxBitmap disabled_bitmap;  // item's disabled bitmap
    wxBitmap hover_bitmap;     // item's hover bitmap
    wxString short_help;       // short help (for tooltip)
    wxString long_help;        // long help (for status bar)
    wxSizerItem* sizer_item;   // sizer item
    wxSize min_size;           // item's minimum size
    int spacer_pixels;         // size of a spacer
    int id;                    // item's id
    int kind;                  // item's kind
    int state;                 // state
    int proportion;            // proportion
    bool active;               // true if the item is currently active
    bool dropdown;             // true if the item has a dropdown button
    bool sticky;               // overrides button states if true (always active)
    long user_data;            // user-specified data
};

#ifndef SWIG
WX_DECLARE_USER_EXPORTED_OBJARRAY(wxAuiToolBarItem, wxAuiToolBarItemArray, WXDLLIMPEXP_AUI);
#endif




// tab art class

class WXDLLIMPEXP_AUI wxAuiToolBarArt
{
public:

    wxAuiToolBarArt() { }
    virtual ~wxAuiToolBarArt() { }

    virtual wxAuiToolBarArt* Clone() = 0;
    virtual void SetFlags(unsigned int flags) = 0;
    virtual void SetFont(const wxFont& font) = 0;
    virtual void SetTextOrientation(int orientation) = 0;

    virtual void DrawBackground(
                         wxDC& dc,
                         wxWindow* wnd,
                         const wxRect& rect) = 0;

    virtual void DrawLabel(
                         wxDC& dc,
                         wxWindow* wnd,
                         const wxAuiToolBarItem& item,
                         const wxRect& rect) = 0;

    virtual void DrawButton(
                         wxDC& dc,
                         wxWindow* wnd,
                         const wxAuiToolBarItem& item,
                         const wxRect& rect) = 0;

    virtual void DrawDropDownButton(
                         wxDC& dc,
                         wxWindow* wnd,
                         const wxAuiToolBarItem& item,
                         const wxRect& rect) = 0;

    virtual void DrawControlLabel(
                         wxDC& dc,
                         wxWindow* wnd,
                         const wxAuiToolBarItem& item,
                         const wxRect& rect) = 0;

    virtual void DrawSeparator(
                         wxDC& dc,
                         wxWindow* wnd,
                         const wxRect& rect) = 0;

    virtual void DrawGripper(
                         wxDC& dc,
                         wxWindow* wnd,
                         const wxRect& rect) = 0;

    virtual void DrawOverflowButton(
                         wxDC& dc,
                         wxWindow* wnd,
                         const wxRect& rect,
                         int state) = 0;

    virtual wxSize GetLabelSize(
                         wxDC& dc,
                         wxWindow* wnd,
                         const wxAuiToolBarItem& item) = 0;

    virtual wxSize GetToolSize(
                         wxDC& dc,
                         wxWindow* wnd,
                         const wxAuiToolBarItem& item) = 0;

    virtual int GetElementSize(int element_id) = 0;
    virtual void SetElementSize(int element_id, int size) = 0;

    virtual int ShowDropDown(
                         wxWindow* wnd,
                         const wxAuiToolBarItemArray& items) = 0;
};



class WXDLLIMPEXP_AUI wxAuiDefaultToolBarArt : public wxAuiToolBarArt
{

public:

    wxAuiDefaultToolBarArt();
    virtual ~wxAuiDefaultToolBarArt();

    virtual wxAuiToolBarArt* Clone();
    virtual void SetFlags(unsigned int flags);
    virtual void SetFont(const wxFont& font);
    virtual void SetTextOrientation(int orientation);

    virtual void DrawBackground(
                wxDC& dc,
                wxWindow* wnd,
                const wxRect& rect);

    virtual void DrawLabel(
                wxDC& dc,
                wxWindow* wnd,
                const wxAuiToolBarItem& item,
                const wxRect& rect);

    virtual void DrawButton(
                wxDC& dc,
                wxWindow* wnd,
                const wxAuiToolBarItem& item,
                const wxRect& rect);

    virtual void DrawDropDownButton(
                wxDC& dc,
                wxWindow* wnd,
                const wxAuiToolBarItem& item,
                const wxRect& rect);

    virtual void DrawControlLabel(
                wxDC& dc,
                wxWindow* wnd,
                const wxAuiToolBarItem& item,
                const wxRect& rect);

    virtual void DrawSeparator(
                wxDC& dc,
                wxWindow* wnd,
                const wxRect& rect);

    virtual void DrawGripper(
                wxDC& dc,
                wxWindow* wnd,
                const wxRect& rect);

    virtual void DrawOverflowButton(
                wxDC& dc,
                wxWindow* wnd,
                const wxRect& rect,
                int state);

    virtual wxSize GetLabelSize(
                wxDC& dc,
                wxWindow* wnd,
                const wxAuiToolBarItem& item);

    virtual wxSize GetToolSize(
                wxDC& dc,
                wxWindow* wnd,
                const wxAuiToolBarItem& item);

    virtual int GetElementSize(int element);
    virtual void SetElementSize(int element_id, int size);

    virtual int ShowDropDown(wxWindow* wnd,
                             const wxAuiToolBarItemArray& items);

protected:

    wxBitmap m_button_dropdown_bmp;
    wxBitmap m_disabled_button_dropdown_bmp;
    wxBitmap m_overflow_bmp;
    wxBitmap m_disabled_overflow_bmp;
    wxColour m_base_colour;
    wxColour m_highlight_colour;
    wxFont m_font;
    unsigned int m_flags;
    int m_text_orientation;

    wxPen m_gripper_pen1;
    wxPen m_gripper_pen2;
    wxPen m_gripper_pen3;

    int m_separator_size;
    int m_gripper_size;
    int m_overflow_size;
};




class WXDLLIMPEXP_AUI wxAuiToolBar : public wxControl
{
public:

    wxAuiToolBar(wxWindow* parent,
                 wxWindowID id = -1,
                 const wxPoint& position = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = wxAUI_TB_DEFAULT_STYLE);
    ~wxAuiToolBar();

    void SetWindowStyleFlag(long style);
    long GetWindowStyleFlag() const;

    void SetArtProvider(wxAuiToolBarArt* art);
    wxAuiToolBarArt* GetArtProvider() const;

    bool SetFont(const wxFont& font);


    void AddTool(int tool_id,
                 const wxString& label,
                 const wxBitmap& bitmap,
                 const wxString& short_help_string = wxEmptyString,
                 wxItemKind kind = wxITEM_NORMAL);

    void AddTool(int tool_id,
                 const wxString& label,
                 const wxBitmap& bitmap,
                 const wxBitmap& disabled_bitmap,
                 wxItemKind kind,
                 const wxString& short_help_string,
                 const wxString& long_help_string,
                 wxObject* client_data);

    void AddTool(int tool_id,
                 const wxBitmap& bitmap,
                 const wxBitmap& disabled_bitmap,
                 bool toggle = false,
                 wxObject* client_data = NULL,
                 const wxString& short_help_string = wxEmptyString,
                 const wxString& long_help_string = wxEmptyString)
    {
        AddTool(tool_id,
                wxEmptyString,
                bitmap,
                disabled_bitmap,
                toggle ? wxITEM_CHECK : wxITEM_NORMAL,
                short_help_string,
                long_help_string,
                client_data);
    }

    void AddLabel(int tool_id,
                  const wxString& label = wxEmptyString,
                  const int width = -1);
    void AddControl(wxControl* control,
                    const wxString& label = wxEmptyString);
    void AddSeparator();
    void AddSpacer(int pixels);
    void AddStretchSpacer(int proportion = 1);

    bool Realize();

    wxControl* FindControl(int window_id);
    wxAuiToolBarItem* FindToolByPosition(wxCoord x, wxCoord y) const;
    wxAuiToolBarItem* FindToolByIndex(int idx) const;
    wxAuiToolBarItem* FindTool(int tool_id) const;

    void ClearTools() { Clear() ; }
    void Clear();
    bool DeleteTool(int tool_id);
    bool DeleteByIndex(int tool_id);

    size_t GetToolCount() const;
    int GetToolPos(int tool_id) const { return GetToolIndex(tool_id); }
    int GetToolIndex(int tool_id) const;
    bool GetToolFits(int tool_id) const;
    wxRect GetToolRect(int tool_id) const;
    bool GetToolFitsByIndex(int tool_id) const;
    bool GetToolBarFits() const;

    void SetMargins(const wxSize& size) { SetMargins(size.x, size.x, size.y, size.y); }
    void SetMargins(int x, int y) { SetMargins(x, x, y, y); }
    void SetMargins(int left, int right, int top, int bottom);

    void SetToolBitmapSize(const wxSize& size);
    wxSize GetToolBitmapSize() const;

    bool GetOverflowVisible() const;
    void SetOverflowVisible(bool visible);

    bool GetGripperVisible() const;
    void SetGripperVisible(bool visible);

    void ToggleTool(int tool_id, bool state);
    bool GetToolToggled(int tool_id) const;

    void EnableTool(int tool_id, bool state);
    bool GetToolEnabled(int tool_id) const;

    void SetToolDropDown(int tool_id, bool dropdown);
    bool GetToolDropDown(int tool_id) const;

    void SetToolBorderPadding(int padding);
    int  GetToolBorderPadding() const;

    void SetToolTextOrientation(int orientation);
    int  GetToolTextOrientation() const;

    void SetToolPacking(int packing);
    int  GetToolPacking() const;

    void SetToolProportion(int tool_id, int proportion);
    int  GetToolProportion(int tool_id) const;

    void SetToolSeparation(int separation);
    int GetToolSeparation() const;

    void SetToolSticky(int tool_id, bool sticky);
    bool GetToolSticky(int tool_id) const;

    wxString GetToolLabel(int tool_id) const;
    void SetToolLabel(int tool_id, const wxString& label);

    wxBitmap GetToolBitmap(int tool_id) const;
    void SetToolBitmap(int tool_id, const wxBitmap& bitmap);

    wxString GetToolShortHelp(int tool_id) const;
    void SetToolShortHelp(int tool_id, const wxString& help_string);

    wxString GetToolLongHelp(int tool_id) const;
    void SetToolLongHelp(int tool_id, const wxString& help_string);

    void SetCustomOverflowItems(const wxAuiToolBarItemArray& prepend,
                                const wxAuiToolBarItemArray& append);

protected:

    virtual void OnCustomRender(wxDC& WXUNUSED(dc),
                                const wxAuiToolBarItem& WXUNUSED(item),
                                const wxRect& WXUNUSED(rect)) { }

protected:

    void DoIdleUpdate();
    void SetOrientation(int orientation);
    void SetHoverItem(wxAuiToolBarItem* item);
    void SetPressedItem(wxAuiToolBarItem* item);
    void RefreshOverflowState();

    int GetOverflowState() const;
    wxRect GetOverflowRect() const;
    wxSize GetLabelSize(const wxString& label);
    wxAuiToolBarItem* FindToolByPositionWithPacking(wxCoord x, wxCoord y) const;

    void DoSetSize(int x,
                   int y,
                   int width,
                   int height,
                   int sizeFlags = wxSIZE_AUTO);

protected: // handlers

    void OnSize(wxSizeEvent& evt);
    void OnIdle(wxIdleEvent& evt);
    void OnPaint(wxPaintEvent& evt);
    void OnEraseBackground(wxEraseEvent& evt);
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnRightDown(wxMouseEvent& evt);
    void OnRightUp(wxMouseEvent& evt);
    void OnMiddleDown(wxMouseEvent& evt);
    void OnMiddleUp(wxMouseEvent& evt);
    void OnMotion(wxMouseEvent& evt);
    void OnLeaveWindow(wxMouseEvent& evt);
    void OnSetCursor(wxSetCursorEvent& evt);

protected:

    wxAuiToolBarItemArray m_items;      // array of toolbar items
    wxAuiToolBarArt* m_art;             // art provider
    wxBoxSizer* m_sizer;                // main sizer for toolbar
    wxAuiToolBarItem* m_action_item;    // item that's being acted upon (pressed)
    wxAuiToolBarItem* m_tip_item;       // item that has its tooltip shown
    wxBitmap m_bitmap;                  // double-buffer bitmap
    wxSizerItem* m_gripper_sizer_item;
    wxSizerItem* m_overflow_sizer_item;
    wxSize m_absolute_min_size;
    wxPoint m_action_pos;               // position of left-mouse down
    wxAuiToolBarItemArray m_custom_overflow_prepend;
    wxAuiToolBarItemArray m_custom_overflow_append;

    int m_button_width;
    int m_button_height;
    int m_sizer_element_count;
    int m_left_padding;
    int m_right_padding;
    int m_top_padding;
    int m_bottom_padding;
    int m_tool_packing;
    int m_tool_border_padding;
    int m_tool_text_orientation;
    int m_overflow_state;
    bool m_dragging;
    bool m_gripper_visible;
    bool m_overflow_visible;
    long m_style;

    DECLARE_EVENT_TABLE()
    DECLARE_CLASS(wxAuiToolBar)
};




// wx event machinery

#ifndef SWIG

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_AUI, wxEVT_COMMAND_AUITOOLBAR_TOOL_DROPDOWN, 0)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_AUI, wxEVT_COMMAND_AUITOOLBAR_OVERFLOW_CLICK, 0)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_AUI, wxEVT_COMMAND_AUITOOLBAR_RIGHT_CLICK, 0)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_AUI, wxEVT_COMMAND_AUITOOLBAR_MIDDLE_CLICK, 0)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_AUI, wxEVT_COMMAND_AUITOOLBAR_BEGIN_DRAG, 0)
END_DECLARE_EVENT_TYPES()

typedef void (wxEvtHandler::*wxAuiToolBarEventFunction)(wxAuiToolBarEvent&);

#define wxAuiToolBarEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxAuiToolBarEventFunction, &func)

#define EVT_AUITOOLBAR_TOOL_DROPDOWN(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_AUITOOLBAR_TOOL_DROPDOWN, winid, wxAuiToolBarEventHandler(fn))
#define EVT_AUITOOLBAR_OVERFLOW_CLICK(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_AUITOOLBAR_OVERFLOW_CLICK, winid, wxAuiToolBarEventHandler(fn))
#define EVT_AUITOOLBAR_RIGHT_CLICK(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_AUITOOLBAR_RIGHT_CLICK, winid, wxAuiToolBarEventHandler(fn))
#define EVT_AUITOOLBAR_MIDDLE_CLICK(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_AUITOOLBAR_MIDDLE_CLICK, winid, wxAuiToolBarEventHandler(fn))
#define EVT_AUITOOLBAR_BEGIN_DRAG(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_AUITOOLBAR_BEGIN_DRAG, winid, wxAuiToolBarEventHandler(fn))

#else

// wxpython/swig event work
%constant wxEventType wxEVT_COMMAND_AUITOOLBAR_TOOL_DROPDOWN;
%constant wxEventType wxEVT_COMMAND_AUITOOLBAR_OVERFLOW_CLICK;
%constant wxEventType wxEVT_COMMAND_AUITOOLBAR_RIGHT_CLICK;
%constant wxEventType wxEVT_COMMAND_AUITOOLBAR_MIDDLE_CLICK;
%constant wxEventType wxEVT_COMMAND_AUITOOLBAR_BEGIN_DRAG;

%pythoncode {
    EVT_AUITOOLBAR_TOOL_DROPDOWN = wx.PyEventBinder( wxEVT_COMMAND_AUITOOLBAR_TOOL_DROPDOWN, 1 )
    EVT_AUITOOLBAR_OVERFLOW_CLICK = wx.PyEventBinder( wxEVT_COMMAND_AUITOOLBAR_OVERFLOW_CLICK, 1 )
    EVT_AUITOOLBAR_RIGHT_CLICK = wx.PyEventBinder( wxEVT_COMMAND_AUITOOLBAR_RIGHT_CLICK, 1 )
    EVT_AUITOOLBAR_MIDDLE_CLICK = wx.PyEventBinder( wxEVT_COMMAND_AUITOOLBAR_MIDDLE_CLICK, 1 )
    EVT_AUITOOLBAR_BEGIN_DRAG = wx.PyEventBinder( wxEVT_COMMAND_AUITOOLBAR_BEGIN_DRAG, 1 )
}
#endif  // SWIG

#endif  // wxABI_VERSION >= 20809

#endif  // wxUSE_AUI

#endif  // _WX_AUIBAR_H_

