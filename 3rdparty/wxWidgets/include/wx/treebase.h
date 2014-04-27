/////////////////////////////////////////////////////////////////////////////
// Name:        wx/treebase.h
// Purpose:     wxTreeCtrl base classes and types
// Author:      Julian Smart et al
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: treebase.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 1997,1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TREEBASE_H_
#define _WX_TREEBASE_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"

#if wxUSE_TREECTRL

#include "wx/window.h"  // for wxClientData
#include "wx/event.h"
#include "wx/dynarray.h"

#if WXWIN_COMPATIBILITY_2_6

// flags for deprecated `Expand(int action)', will be removed in next versions
enum
{
    wxTREE_EXPAND_EXPAND,
    wxTREE_EXPAND_COLLAPSE,
    wxTREE_EXPAND_COLLAPSE_RESET,
    wxTREE_EXPAND_TOGGLE
};

#endif // WXWIN_COMPATIBILITY_2_6

// ----------------------------------------------------------------------------
// wxTreeItemId identifies an element of the tree. In this implementation, it's
// just a trivial wrapper around Win32 HTREEITEM or a pointer to some private
// data structure in the generic version. It's opaque for the application and
// the only method which can be used by user code is IsOk().
// ----------------------------------------------------------------------------

// Using this typedef removes an ambiguity when calling Remove()
typedef void *wxTreeItemIdValue;

class WXDLLEXPORT wxTreeItemId
{
    friend bool operator==(const wxTreeItemId&, const wxTreeItemId&);
public:
    // ctors
        // 0 is invalid value for HTREEITEM
    wxTreeItemId() { m_pItem = 0; }

        // construct wxTreeItemId from the native item id
    wxTreeItemId(void *pItem) { m_pItem = pItem; }

        // default copy ctor/assignment operator are ok for us

    // accessors
        // is this a valid tree item?
    bool IsOk() const { return m_pItem != 0; }
        // return true if this item is not valid
    bool operator!() const { return !IsOk(); }

    // operations
        // invalidate the item
    void Unset() { m_pItem = 0; }

#if WXWIN_COMPATIBILITY_2_4
    // deprecated: only for compatibility, don't work on 64 bit archs
    wxTreeItemId(long item) { m_pItem = wxUIntToPtr(item); }
    operator long() const { return (long)wxPtrToUInt(m_pItem); }
#else // !WXWIN_COMPATIBILITY_2_4
    operator bool() const { return IsOk(); }
#endif // WXWIN_COMPATIBILITY_2_4/!WXWIN_COMPATIBILITY_2_4

    wxTreeItemIdValue m_pItem;
};

inline bool operator==(const wxTreeItemId& i1, const wxTreeItemId& i2)
{
    return i1.m_pItem == i2.m_pItem;
}

inline bool operator!=(const wxTreeItemId& i1, const wxTreeItemId& i2)
{
    return i1.m_pItem != i2.m_pItem;
}

// ----------------------------------------------------------------------------
// wxTreeItemData is some (arbitrary) user class associated with some item. The
// main advantage of having this class (compared to old untyped interface) is
// that wxTreeItemData's are destroyed automatically by the tree and, as this
// class has virtual dtor, it means that the memory will be automatically
// freed. OTOH, we don't just use wxObject instead of wxTreeItemData because
// the size of this class is critical: in any real application, each tree leaf
// will have wxTreeItemData associated with it and number of leaves may be
// quite big.
//
// Because the objects of this class are deleted by the tree, they should
// always be allocated on the heap!
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxTreeItemData: public wxClientData
{
friend class WXDLLIMPEXP_FWD_CORE wxTreeCtrl;
friend class WXDLLIMPEXP_FWD_CORE wxGenericTreeCtrl;
public:
    // creation/destruction
    // --------------------
        // default ctor
    wxTreeItemData() { }

        // default copy ctor/assignment operator are ok

    // accessor: get the item associated with us
    const wxTreeItemId& GetId() const { return m_pItem; }
    void SetId(const wxTreeItemId& id) { m_pItem = id; }

protected:
    wxTreeItemId m_pItem;
};

WX_DEFINE_EXPORTED_ARRAY_PTR(wxTreeItemIdValue, wxArrayTreeItemIdsBase);

// this is a wrapper around the array class defined above which allow to wok
// with vaue of natural wxTreeItemId type instead of using wxTreeItemIdValue
// and does it without any loss of efficiency
class WXDLLEXPORT wxArrayTreeItemIds : public wxArrayTreeItemIdsBase
{
public:
    void Add(const wxTreeItemId& id)
        { wxArrayTreeItemIdsBase::Add(id.m_pItem); }
    void Insert(const wxTreeItemId& id, size_t pos)
        { wxArrayTreeItemIdsBase::Insert(id.m_pItem, pos); }
    wxTreeItemId Item(size_t i) const
        { return wxTreeItemId(wxArrayTreeItemIdsBase::Item(i)); }
    wxTreeItemId operator[](size_t i) const { return Item(i); }
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// enum for different images associated with a treectrl item
enum wxTreeItemIcon
{
    wxTreeItemIcon_Normal,              // not selected, not expanded
    wxTreeItemIcon_Selected,            //     selected, not expanded
    wxTreeItemIcon_Expanded,            // not selected,     expanded
    wxTreeItemIcon_SelectedExpanded,    //     selected,     expanded
    wxTreeItemIcon_Max
};

// ----------------------------------------------------------------------------
// wxTreeCtrl flags
// ----------------------------------------------------------------------------

#define wxTR_NO_BUTTONS              0x0000     // for convenience
#define wxTR_HAS_BUTTONS             0x0001     // draw collapsed/expanded btns
#define wxTR_NO_LINES                0x0004     // don't draw lines at all
#define wxTR_LINES_AT_ROOT           0x0008     // connect top-level nodes
#define wxTR_TWIST_BUTTONS           0x0010     // still used by wxTreeListCtrl

#define wxTR_SINGLE                  0x0000     // for convenience
#define wxTR_MULTIPLE                0x0020     // can select multiple items
#define wxTR_EXTENDED                0x0040     // TODO: allow extended selection
#define wxTR_HAS_VARIABLE_ROW_HEIGHT 0x0080     // what it says

#define wxTR_EDIT_LABELS             0x0200     // can edit item labels
#define wxTR_ROW_LINES               0x0400     // put border around items
#define wxTR_HIDE_ROOT               0x0800     // don't display root node

#define wxTR_FULL_ROW_HIGHLIGHT      0x2000     // highlight full horz space

#ifdef __WXGTK20__
#define wxTR_DEFAULT_STYLE           (wxTR_HAS_BUTTONS | wxTR_NO_LINES)
#else
#define wxTR_DEFAULT_STYLE           (wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT)
#endif

#if WXWIN_COMPATIBILITY_2_6
// deprecated, don't use
#define wxTR_MAC_BUTTONS             0
#define wxTR_AQUA_BUTTONS            0
#endif // WXWIN_COMPATIBILITY_2_6


// values for the `flags' parameter of wxTreeCtrl::HitTest() which determine
// where exactly the specified point is situated:

static const int wxTREE_HITTEST_ABOVE            = 0x0001;
static const int wxTREE_HITTEST_BELOW            = 0x0002;
static const int wxTREE_HITTEST_NOWHERE          = 0x0004;
    // on the button associated with an item.
static const int wxTREE_HITTEST_ONITEMBUTTON     = 0x0008;
    // on the bitmap associated with an item.
static const int wxTREE_HITTEST_ONITEMICON       = 0x0010;
    // on the indent associated with an item.
static const int wxTREE_HITTEST_ONITEMINDENT     = 0x0020;
    // on the label (string) associated with an item.
static const int wxTREE_HITTEST_ONITEMLABEL      = 0x0040;
    // on the right of the label associated with an item.
static const int wxTREE_HITTEST_ONITEMRIGHT      = 0x0080;
    // on the label (string) associated with an item.
static const int wxTREE_HITTEST_ONITEMSTATEICON  = 0x0100;
    // on the left of the wxTreeCtrl.
static const int wxTREE_HITTEST_TOLEFT           = 0x0200;
    // on the right of the wxTreeCtrl.
static const int wxTREE_HITTEST_TORIGHT          = 0x0400;
    // on the upper part (first half) of the item.
static const int wxTREE_HITTEST_ONITEMUPPERPART  = 0x0800;
    // on the lower part (second half) of the item.
static const int wxTREE_HITTEST_ONITEMLOWERPART  = 0x1000;

    // anywhere on the item
static const int wxTREE_HITTEST_ONITEM  = wxTREE_HITTEST_ONITEMICON |
                                          wxTREE_HITTEST_ONITEMLABEL;

// tree ctrl default name
extern WXDLLEXPORT_DATA(const wxChar) wxTreeCtrlNameStr[];

// ----------------------------------------------------------------------------
// wxTreeItemAttr: a structure containing the visual attributes of an item
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxTreeItemAttr
{
public:
    // ctors
    wxTreeItemAttr() { }
    wxTreeItemAttr(const wxColour& colText,
                   const wxColour& colBack,
                   const wxFont& font)
        : m_colText(colText), m_colBack(colBack), m_font(font) { }

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

private:
    wxColour m_colText,
             m_colBack;
    wxFont   m_font;
};

// ----------------------------------------------------------------------------
// wxTreeEvent is a special class for all events associated with tree controls
//
// NB: note that not all accessors make sense for all events, see the event
//     descriptions below
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxTreeCtrlBase;

class WXDLLEXPORT wxTreeEvent : public wxNotifyEvent
{
public:
    wxTreeEvent(wxEventType commandType,
                wxTreeCtrlBase *tree,
                const wxTreeItemId &item = wxTreeItemId());
    wxTreeEvent(wxEventType commandType = wxEVT_NULL, int id = 0);
    wxTreeEvent(const wxTreeEvent& event);

    virtual wxEvent *Clone() const { return new wxTreeEvent(*this); }

    // accessors
        // get the item on which the operation was performed or the newly
        // selected item for wxEVT_COMMAND_TREE_SEL_CHANGED/ING events
    wxTreeItemId GetItem() const { return m_item; }
    void SetItem(const wxTreeItemId& item) { m_item = item; }

        // for wxEVT_COMMAND_TREE_SEL_CHANGED/ING events, get the previously
        // selected item
    wxTreeItemId GetOldItem() const { return m_itemOld; }
    void SetOldItem(const wxTreeItemId& item) { m_itemOld = item; }

        // the point where the mouse was when the drag operation started (for
        // wxEVT_COMMAND_TREE_BEGIN_(R)DRAG events only) or click position
    wxPoint GetPoint() const { return m_pointDrag; }
    void SetPoint(const wxPoint& pt) { m_pointDrag = pt; }

        // keyboard data (for wxEVT_COMMAND_TREE_KEY_DOWN only)
    const wxKeyEvent& GetKeyEvent() const { return m_evtKey; }
    int GetKeyCode() const { return m_evtKey.GetKeyCode(); }
    void SetKeyEvent(const wxKeyEvent& evt) { m_evtKey = evt; }

        // label (for EVT_TREE_{BEGIN|END}_LABEL_EDIT only)
    const wxString& GetLabel() const { return m_label; }
    void SetLabel(const wxString& label) { m_label = label; }

        // edit cancel flag (for EVT_TREE_{BEGIN|END}_LABEL_EDIT only)
    bool IsEditCancelled() const { return m_editCancelled; }
    void SetEditCanceled(bool editCancelled) { m_editCancelled = editCancelled; }

        // Set the tooltip for the item (for EVT\_TREE\_ITEM\_GETTOOLTIP events)
    void SetToolTip(const wxString& toolTip) { m_label = toolTip; }
    wxString GetToolTip() { return m_label; }

private:
    // not all of the members are used (or initialized) for all events
    wxKeyEvent    m_evtKey;
    wxTreeItemId  m_item,
                  m_itemOld;
    wxPoint       m_pointDrag;
    wxString      m_label;
    bool          m_editCancelled;

    friend class WXDLLIMPEXP_FWD_CORE wxTreeCtrl;
    friend class WXDLLIMPEXP_FWD_CORE wxGenericTreeCtrl;

    DECLARE_DYNAMIC_CLASS(wxTreeEvent)
};

typedef void (wxEvtHandler::*wxTreeEventFunction)(wxTreeEvent&);

// ----------------------------------------------------------------------------
// tree control events and macros for handling them
// ----------------------------------------------------------------------------

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_BEGIN_DRAG, 600)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_BEGIN_RDRAG, 601)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, 602)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_END_LABEL_EDIT, 603)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_DELETE_ITEM, 604)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_GET_INFO, 605)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_SET_INFO, 606)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_EXPANDED, 607)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_EXPANDING, 608)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_COLLAPSED, 609)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_COLLAPSING, 610)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_SEL_CHANGED, 611)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_SEL_CHANGING, 612)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_KEY_DOWN, 613)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_ACTIVATED, 614)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, 615)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_MIDDLE_CLICK, 616)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_END_DRAG, 617)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_STATE_IMAGE_CLICK, 618)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_GETTOOLTIP, 619)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_MENU, 620)
END_DECLARE_EVENT_TYPES()

#define wxTreeEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxTreeEventFunction, &func)

#define wx__DECLARE_TREEEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_TREE_ ## evt, id, wxTreeEventHandler(fn))

// GetItem() returns the item being dragged, GetPoint() the mouse coords
//
// if you call event.Allow(), the drag operation will start and a
// EVT_TREE_END_DRAG event will be sent when the drag is over.
#define EVT_TREE_BEGIN_DRAG(id, fn) wx__DECLARE_TREEEVT(BEGIN_DRAG, id, fn)
#define EVT_TREE_BEGIN_RDRAG(id, fn) wx__DECLARE_TREEEVT(BEGIN_RDRAG, id, fn)

// GetItem() is the item on which the drop occurred (if any) and GetPoint() the
// current mouse coords
#define EVT_TREE_END_DRAG(id, fn) wx__DECLARE_TREEEVT(END_DRAG, id, fn)

// GetItem() returns the itme whose label is being edited, GetLabel() returns
// the current item label for BEGIN and the would be new one for END.
//
// Vetoing BEGIN event means that label editing won't happen at all,
// vetoing END means that the new value is discarded and the old one kept
#define EVT_TREE_BEGIN_LABEL_EDIT(id, fn) wx__DECLARE_TREEEVT(BEGIN_LABEL_EDIT, id, fn)
#define EVT_TREE_END_LABEL_EDIT(id, fn) wx__DECLARE_TREEEVT(END_LABEL_EDIT, id, fn)

// provide/update information about GetItem() item
#define EVT_TREE_GET_INFO(id, fn) wx__DECLARE_TREEEVT(GET_INFO, id, fn)
#define EVT_TREE_SET_INFO(id, fn) wx__DECLARE_TREEEVT(SET_INFO, id, fn)

// GetItem() is the item being expanded/collapsed, the "ING" versions can use
#define EVT_TREE_ITEM_EXPANDED(id, fn) wx__DECLARE_TREEEVT(ITEM_EXPANDED, id, fn)
#define EVT_TREE_ITEM_EXPANDING(id, fn) wx__DECLARE_TREEEVT(ITEM_EXPANDING, id, fn)
#define EVT_TREE_ITEM_COLLAPSED(id, fn) wx__DECLARE_TREEEVT(ITEM_COLLAPSED, id, fn)
#define EVT_TREE_ITEM_COLLAPSING(id, fn) wx__DECLARE_TREEEVT(ITEM_COLLAPSING, id, fn)

// GetOldItem() is the item which had the selection previously, GetItem() is
// the item which acquires selection
#define EVT_TREE_SEL_CHANGED(id, fn) wx__DECLARE_TREEEVT(SEL_CHANGED, id, fn)
#define EVT_TREE_SEL_CHANGING(id, fn) wx__DECLARE_TREEEVT(SEL_CHANGING, id, fn)

// GetKeyCode() returns the key code
// NB: this is the only message for which GetItem() is invalid (you may get the
//     item from GetSelection())
#define EVT_TREE_KEY_DOWN(id, fn) wx__DECLARE_TREEEVT(KEY_DOWN, id, fn)

// GetItem() returns the item being deleted, the associated data (if any) will
// be deleted just after the return of this event handler (if any)
#define EVT_TREE_DELETE_ITEM(id, fn) wx__DECLARE_TREEEVT(DELETE_ITEM, id, fn)

// GetItem() returns the item that was activated (double click, enter, space)
#define EVT_TREE_ITEM_ACTIVATED(id, fn) wx__DECLARE_TREEEVT(ITEM_ACTIVATED, id, fn)

// GetItem() returns the item for which the context menu shall be shown
#define EVT_TREE_ITEM_MENU(id, fn) wx__DECLARE_TREEEVT(ITEM_MENU, id, fn)

// GetItem() returns the item that was clicked on
#define EVT_TREE_ITEM_RIGHT_CLICK(id, fn) wx__DECLARE_TREEEVT(ITEM_RIGHT_CLICK, id, fn)
#define EVT_TREE_ITEM_MIDDLE_CLICK(id, fn) wx__DECLARE_TREEEVT(ITEM_MIDDLE_CLICK, id, fn)

// GetItem() returns the item whose state image was clicked on
#define EVT_TREE_STATE_IMAGE_CLICK(id, fn) wx__DECLARE_TREEEVT(STATE_IMAGE_CLICK, id, fn)

// GetItem() is the item for which the tooltip is being requested
#define EVT_TREE_ITEM_GETTOOLTIP(id, fn) wx__DECLARE_TREEEVT(ITEM_GETTOOLTIP, id, fn)

#endif // wxUSE_TREECTRL

#endif // _WX_TREEBASE_H_
