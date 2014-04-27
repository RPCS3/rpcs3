///////////////////////////////////////////////////////////////////////////////
// Name:        wx/listbase.h
// Purpose:     wxListCtrl class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.12.99
// RCS-ID:      $Id: listbase.h 46313 2007-06-03 22:38:28Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_LISTBASE_H_BASE_
#define _WX_LISTBASE_H_BASE_

#include "wx/colour.h"
#include "wx/font.h"
#include "wx/gdicmn.h"
#include "wx/event.h"

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

// type of compare function for wxListCtrl sort operation
typedef int (wxCALLBACK *wxListCtrlCompare)(long item1, long item2, long sortData);

// ----------------------------------------------------------------------------
// wxListCtrl constants
// ----------------------------------------------------------------------------

// style flags
#define wxLC_VRULES          0x0001
#define wxLC_HRULES          0x0002

#define wxLC_ICON            0x0004
#define wxLC_SMALL_ICON      0x0008
#define wxLC_LIST            0x0010
#define wxLC_REPORT          0x0020

#define wxLC_ALIGN_TOP       0x0040
#define wxLC_ALIGN_LEFT      0x0080
#define wxLC_AUTOARRANGE     0x0100
#define wxLC_VIRTUAL         0x0200
#define wxLC_EDIT_LABELS     0x0400
#define wxLC_NO_HEADER       0x0800
#define wxLC_NO_SORT_HEADER  0x1000
#define wxLC_SINGLE_SEL      0x2000
#define wxLC_SORT_ASCENDING  0x4000
#define wxLC_SORT_DESCENDING 0x8000

#define wxLC_MASK_TYPE       (wxLC_ICON | wxLC_SMALL_ICON | wxLC_LIST | wxLC_REPORT)
#define wxLC_MASK_ALIGN      (wxLC_ALIGN_TOP | wxLC_ALIGN_LEFT)
#define wxLC_MASK_SORT       (wxLC_SORT_ASCENDING | wxLC_SORT_DESCENDING)

// for compatibility only
#define wxLC_USER_TEXT       wxLC_VIRTUAL

// Omitted because
//  (a) too much detail
//  (b) not enough style flags
//  (c) not implemented anyhow in the generic version
//
// #define wxLC_NO_SCROLL
// #define wxLC_NO_LABEL_WRAP
// #define wxLC_OWNERDRAW_FIXED
// #define wxLC_SHOW_SEL_ALWAYS

// Mask flags to tell app/GUI what fields of wxListItem are valid
#define wxLIST_MASK_STATE           0x0001
#define wxLIST_MASK_TEXT            0x0002
#define wxLIST_MASK_IMAGE           0x0004
#define wxLIST_MASK_DATA            0x0008
#define wxLIST_SET_ITEM             0x0010
#define wxLIST_MASK_WIDTH           0x0020
#define wxLIST_MASK_FORMAT          0x0040

// State flags for indicating the state of an item
#define wxLIST_STATE_DONTCARE       0x0000
#define wxLIST_STATE_DROPHILITED    0x0001      // MSW only
#define wxLIST_STATE_FOCUSED        0x0002
#define wxLIST_STATE_SELECTED       0x0004
#define wxLIST_STATE_CUT            0x0008      // MSW only
#define wxLIST_STATE_DISABLED       0x0010      // OS2 only
#define wxLIST_STATE_FILTERED       0x0020      // OS2 only
#define wxLIST_STATE_INUSE          0x0040      // OS2 only
#define wxLIST_STATE_PICKED         0x0080      // OS2 only
#define wxLIST_STATE_SOURCE         0x0100      // OS2 only

// Hit test flags, used in HitTest
#define wxLIST_HITTEST_ABOVE            0x0001  // Above the client area.
#define wxLIST_HITTEST_BELOW            0x0002  // Below the client area.
#define wxLIST_HITTEST_NOWHERE          0x0004  // In the client area but below the last item.
#define wxLIST_HITTEST_ONITEMICON       0x0020  // On the bitmap associated with an item.
#define wxLIST_HITTEST_ONITEMLABEL      0x0080  // On the label (string) associated with an item.
#define wxLIST_HITTEST_ONITEMRIGHT      0x0100  // In the area to the right of an item.
#define wxLIST_HITTEST_ONITEMSTATEICON  0x0200  // On the state icon for a tree view item that is in a user-defined state.
#define wxLIST_HITTEST_TOLEFT           0x0400  // To the left of the client area.
#define wxLIST_HITTEST_TORIGHT          0x0800  // To the right of the client area.

#define wxLIST_HITTEST_ONITEM (wxLIST_HITTEST_ONITEMICON | wxLIST_HITTEST_ONITEMLABEL | wxLIST_HITTEST_ONITEMSTATEICON)

// GetSubItemRect constants
#define wxLIST_GETSUBITEMRECT_WHOLEITEM -1l

// Flags for GetNextItem (MSW only except wxLIST_NEXT_ALL)
enum
{
    wxLIST_NEXT_ABOVE,          // Searches for an item above the specified item
    wxLIST_NEXT_ALL,            // Searches for subsequent item by index
    wxLIST_NEXT_BELOW,          // Searches for an item below the specified item
    wxLIST_NEXT_LEFT,           // Searches for an item to the left of the specified item
    wxLIST_NEXT_RIGHT           // Searches for an item to the right of the specified item
};

// Alignment flags for Arrange (MSW only except wxLIST_ALIGN_LEFT)
enum
{
    wxLIST_ALIGN_DEFAULT,
    wxLIST_ALIGN_LEFT,
    wxLIST_ALIGN_TOP,
    wxLIST_ALIGN_SNAP_TO_GRID
};

// Column format (MSW only except wxLIST_FORMAT_LEFT)
enum wxListColumnFormat
{
    wxLIST_FORMAT_LEFT,
    wxLIST_FORMAT_RIGHT,
    wxLIST_FORMAT_CENTRE,
    wxLIST_FORMAT_CENTER = wxLIST_FORMAT_CENTRE
};

// Autosize values for SetColumnWidth
enum
{
    wxLIST_AUTOSIZE = -1,
    wxLIST_AUTOSIZE_USEHEADER = -2      // partly supported by generic version
};

// Flag values for GetItemRect
enum
{
    wxLIST_RECT_BOUNDS,
    wxLIST_RECT_ICON,
    wxLIST_RECT_LABEL
};

// Flag values for FindItem (MSW only)
enum
{
    wxLIST_FIND_UP,
    wxLIST_FIND_DOWN,
    wxLIST_FIND_LEFT,
    wxLIST_FIND_RIGHT
};

// ----------------------------------------------------------------------------
// wxListItemAttr: a structure containing the visual attributes of an item
// ----------------------------------------------------------------------------

// TODO: this should be renamed to wxItemAttr or something general like this
//       and used as base class for wxTextAttr which duplicates this class
//       entirely currently
class WXDLLEXPORT wxListItemAttr
{
public:
    // ctors
    wxListItemAttr() { }
    wxListItemAttr(const wxColour& colText,
                   const wxColour& colBack,
                   const wxFont& font)
        : m_colText(colText), m_colBack(colBack), m_font(font)
    {
    }

    // default copy ctor, assignment operator and dtor are ok


    // setters
    void SetTextColour(const wxColour& colText) { m_colText = colText; }
    void SetBackgroundColour(const wxColour& colBack) { m_colBack = colBack; }
    void SetFont(const wxFont& font) { m_font = font; }

    // accessors
    bool HasTextColour() const { return m_colText.Ok(); }
    bool HasBackgroundColour() const { return m_colBack.Ok(); }
    bool HasFont() const { return m_font.Ok(); }

    const wxColour& GetTextColour() const { return m_colText; }
    const wxColour& GetBackgroundColour() const { return m_colBack; }
    const wxFont& GetFont() const { return m_font; }


    // this is almost like assignment operator except it doesn't overwrite the
    // fields unset in the source attribute
    void AssignFrom(const wxListItemAttr& source)
    {
        if ( source.HasTextColour() )
            SetTextColour(source.GetTextColour());
        if ( source.HasBackgroundColour() )
            SetBackgroundColour(source.GetBackgroundColour());
        if ( source.HasFont() )
            SetFont(source.GetFont());
    }

private:
    wxColour m_colText,
             m_colBack;
    wxFont   m_font;
};

// ----------------------------------------------------------------------------
// wxListItem: the item or column info, used to exchange data with wxListCtrl
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxListItem : public wxObject
{
public:
    wxListItem() { Init(); m_attr = NULL; }
    wxListItem(const wxListItem& item)
        : wxObject(),
          m_mask(item.m_mask),
          m_itemId(item.m_itemId),
          m_col(item.m_col),
          m_state(item.m_state),
          m_stateMask(item.m_stateMask),
          m_text(item.m_text),
          m_image(item.m_image),
          m_data(item.m_data),
          m_format(item.m_format),
          m_width(item.m_width),
          m_attr(NULL)
    {
        // copy list item attributes
        if ( item.HasAttributes() )
            m_attr = new wxListItemAttr(*item.GetAttributes());
    }
    virtual ~wxListItem() { delete m_attr; }

    // resetting
    void Clear() { Init(); m_text.clear(); ClearAttributes(); }
    void ClearAttributes() { if ( m_attr ) { delete m_attr; m_attr = NULL; } }

    // setters
    void SetMask(long mask)
        { m_mask = mask; }
    void SetId(long id)
        { m_itemId = id; }
    void SetColumn(int col)
        { m_col = col; }
    void SetState(long state)
        { m_mask |= wxLIST_MASK_STATE; m_state = state; m_stateMask |= state; }
    void SetStateMask(long stateMask)
        { m_stateMask = stateMask; }
    void SetText(const wxString& text)
        { m_mask |= wxLIST_MASK_TEXT; m_text = text; }
    void SetImage(int image)
        { m_mask |= wxLIST_MASK_IMAGE; m_image = image; }
    void SetData(long data)
        { m_mask |= wxLIST_MASK_DATA; m_data = data; }
    void SetData(void *data)
        { m_mask |= wxLIST_MASK_DATA; m_data = wxPtrToUInt(data); }

    void SetWidth(int width)
        { m_mask |= wxLIST_MASK_WIDTH; m_width = width; }
    void SetAlign(wxListColumnFormat align)
        { m_mask |= wxLIST_MASK_FORMAT; m_format = align; }

    void SetTextColour(const wxColour& colText)
        { Attributes().SetTextColour(colText); }
    void SetBackgroundColour(const wxColour& colBack)
        { Attributes().SetBackgroundColour(colBack); }
    void SetFont(const wxFont& font)
        { Attributes().SetFont(font); }

    // accessors
    long GetMask() const { return m_mask; }
    long GetId() const { return m_itemId; }
    int GetColumn() const { return m_col; }
    long GetState() const { return m_state & m_stateMask; }
    const wxString& GetText() const { return m_text; }
    int GetImage() const { return m_image; }
    wxUIntPtr GetData() const { return m_data; }

    int GetWidth() const { return m_width; }
    wxListColumnFormat GetAlign() const { return (wxListColumnFormat)m_format; }

    wxListItemAttr *GetAttributes() const { return m_attr; }
    bool HasAttributes() const { return m_attr != NULL; }

    wxColour GetTextColour() const
        { return HasAttributes() ? m_attr->GetTextColour() : wxNullColour; }
    wxColour GetBackgroundColour() const
        { return HasAttributes() ? m_attr->GetBackgroundColour()
                                 : wxNullColour; }
    wxFont GetFont() const
        { return HasAttributes() ? m_attr->GetFont() : wxNullFont; }

    // this conversion is necessary to make old code using GetItem() to
    // compile
    operator long() const { return m_itemId; }

    // these members are public for compatibility

    long            m_mask;     // Indicates what fields are valid
    long            m_itemId;   // The zero-based item position
    int             m_col;      // Zero-based column, if in report mode
    long            m_state;    // The state of the item
    long            m_stateMask;// Which flags of m_state are valid (uses same flags)
    wxString        m_text;     // The label/header text
    int             m_image;    // The zero-based index into an image list
    wxUIntPtr       m_data;     // App-defined data

    // For columns only
    int             m_format;   // left, right, centre
    int             m_width;    // width of column

#ifdef __WXPM__
    int             m_miniImage; // handle to the mini image for OS/2
#endif

protected:
    // creates m_attr if we don't have it yet
    wxListItemAttr& Attributes()
    {
        if ( !m_attr )
            m_attr = new wxListItemAttr;

        return *m_attr;
    }

    void Init()
    {
        m_mask = 0;
        m_itemId = 0;
        m_col = 0;
        m_state = 0;
        m_stateMask = 0;
        m_image = -1;
        m_data = 0;

        m_format = wxLIST_FORMAT_CENTRE;
        m_width = 0;
    }

    wxListItemAttr *m_attr;     // optional pointer to the items style

private:
    // VZ: this is strange, we have a copy ctor but not operator=(), why?
    wxListItem& operator=(const wxListItem& item);

    DECLARE_DYNAMIC_CLASS(wxListItem)
};

// ----------------------------------------------------------------------------
// wxListEvent - the event class for the wxListCtrl notifications
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxListEvent : public wxNotifyEvent
{
public:
    wxListEvent(wxEventType commandType = wxEVT_NULL, int winid = 0)
        : wxNotifyEvent(commandType, winid)
        , m_code(0)
        , m_oldItemIndex(0)
        , m_itemIndex(0)
        , m_col(0)
        , m_pointDrag()
        , m_item()
        , m_editCancelled(false)
        { }

    wxListEvent(const wxListEvent& event)
        : wxNotifyEvent(event)
        , m_code(event.m_code)
        , m_oldItemIndex(event.m_oldItemIndex)
        , m_itemIndex(event.m_itemIndex)
        , m_col(event.m_col)
        , m_pointDrag(event.m_pointDrag)
        , m_item(event.m_item)
        , m_editCancelled(event.m_editCancelled)
        { }

    int GetKeyCode() const { return m_code; }
    long GetIndex() const { return m_itemIndex; }
    int GetColumn() const { return m_col; }
    wxPoint GetPoint() const { return m_pointDrag; }
    const wxString& GetLabel() const { return m_item.m_text; }
    const wxString& GetText() const { return m_item.m_text; }
    int GetImage() const { return m_item.m_image; }
    long GetData() const { return wx_static_cast(long, m_item.m_data); }
    long GetMask() const { return m_item.m_mask; }
    const wxListItem& GetItem() const { return m_item; }

    // for wxEVT_COMMAND_LIST_CACHE_HINT only
    long GetCacheFrom() const { return m_oldItemIndex; }
    long GetCacheTo() const { return m_itemIndex; }

    // was label editing canceled? (for wxEVT_COMMAND_LIST_END_LABEL_EDIT only)
    bool IsEditCancelled() const { return m_editCancelled; }
    void SetEditCanceled(bool editCancelled) { m_editCancelled = editCancelled; }

    virtual wxEvent *Clone() const { return new wxListEvent(*this); }

//protected: -- not for backwards compatibility
    int           m_code;
    long          m_oldItemIndex; // only for wxEVT_COMMAND_LIST_CACHE_HINT
    long          m_itemIndex;
    int           m_col;
    wxPoint       m_pointDrag;

    wxListItem    m_item;

protected:
    bool          m_editCancelled;

private:
    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxListEvent)
};

// ----------------------------------------------------------------------------
// wxListCtrl event macros
// ----------------------------------------------------------------------------

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_BEGIN_DRAG, 700)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_BEGIN_RDRAG, 701)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_BEGIN_LABEL_EDIT, 702)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_END_LABEL_EDIT, 703)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_DELETE_ITEM, 704)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_DELETE_ALL_ITEMS, 705)
#if WXWIN_COMPATIBILITY_2_4
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_GET_INFO, 706)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_SET_INFO, 707)
#endif
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_SELECTED, 708)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_DESELECTED, 709)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_KEY_DOWN, 710)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_INSERT_ITEM, 711)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_CLICK, 712)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, 713)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_MIDDLE_CLICK, 714)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_ACTIVATED, 715)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_CACHE_HINT, 716)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_RIGHT_CLICK, 717)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, 718)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_DRAGGING, 719)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_END_DRAG, 720)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_FOCUSED, 721)
END_DECLARE_EVENT_TYPES()

typedef void (wxEvtHandler::*wxListEventFunction)(wxListEvent&);

#define wxListEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxListEventFunction, &func)

#define wx__DECLARE_LISTEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_LIST_ ## evt, id, wxListEventHandler(fn))

#define EVT_LIST_BEGIN_DRAG(id, fn) wx__DECLARE_LISTEVT(BEGIN_DRAG, id, fn)
#define EVT_LIST_BEGIN_RDRAG(id, fn) wx__DECLARE_LISTEVT(BEGIN_RDRAG, id, fn)
#define EVT_LIST_BEGIN_LABEL_EDIT(id, fn) wx__DECLARE_LISTEVT(BEGIN_LABEL_EDIT, id, fn)
#define EVT_LIST_END_LABEL_EDIT(id, fn) wx__DECLARE_LISTEVT(END_LABEL_EDIT, id, fn)
#define EVT_LIST_DELETE_ITEM(id, fn) wx__DECLARE_LISTEVT(DELETE_ITEM, id, fn)
#define EVT_LIST_DELETE_ALL_ITEMS(id, fn) wx__DECLARE_LISTEVT(DELETE_ALL_ITEMS, id, fn)
#define EVT_LIST_KEY_DOWN(id, fn) wx__DECLARE_LISTEVT(KEY_DOWN, id, fn)
#define EVT_LIST_INSERT_ITEM(id, fn) wx__DECLARE_LISTEVT(INSERT_ITEM, id, fn)

#define EVT_LIST_COL_CLICK(id, fn) wx__DECLARE_LISTEVT(COL_CLICK, id, fn)
#define EVT_LIST_COL_RIGHT_CLICK(id, fn) wx__DECLARE_LISTEVT(COL_RIGHT_CLICK, id, fn)
#define EVT_LIST_COL_BEGIN_DRAG(id, fn) wx__DECLARE_LISTEVT(COL_BEGIN_DRAG, id, fn)
#define EVT_LIST_COL_DRAGGING(id, fn) wx__DECLARE_LISTEVT(COL_DRAGGING, id, fn)
#define EVT_LIST_COL_END_DRAG(id, fn) wx__DECLARE_LISTEVT(COL_END_DRAG, id, fn)

#define EVT_LIST_ITEM_SELECTED(id, fn) wx__DECLARE_LISTEVT(ITEM_SELECTED, id, fn)
#define EVT_LIST_ITEM_DESELECTED(id, fn) wx__DECLARE_LISTEVT(ITEM_DESELECTED, id, fn)
#define EVT_LIST_ITEM_RIGHT_CLICK(id, fn) wx__DECLARE_LISTEVT(ITEM_RIGHT_CLICK, id, fn)
#define EVT_LIST_ITEM_MIDDLE_CLICK(id, fn) wx__DECLARE_LISTEVT(ITEM_MIDDLE_CLICK, id, fn)
#define EVT_LIST_ITEM_ACTIVATED(id, fn) wx__DECLARE_LISTEVT(ITEM_ACTIVATED, id, fn)
#define EVT_LIST_ITEM_FOCUSED(id, fn) wx__DECLARE_LISTEVT(ITEM_FOCUSED, id, fn)

#define EVT_LIST_CACHE_HINT(id, fn) wx__DECLARE_LISTEVT(CACHE_HINT, id, fn)


#if WXWIN_COMPATIBILITY_2_4
#define EVT_LIST_GET_INFO(id, fn) wx__DECLARE_LISTEVT(GET_INFO, id, fn)
#define EVT_LIST_SET_INFO(id, fn) wx__DECLARE_LISTEVT(SET_INFO, id, fn)
#endif

#endif
    // _WX_LISTCTRL_H_BASE_
