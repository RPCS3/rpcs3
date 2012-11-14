///////////////////////////////////////////////////////////////////////////////
// Name:        wx/vlbox.h
// Purpose:     wxVListBox is a virtual listbox with lines of variable height
// Author:      Vadim Zeitlin
// Modified by:
// Created:     31.05.03
// RCS-ID:      $Id: vlbox.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_VLBOX_H_
#define _WX_VLBOX_H_

#include "wx/vscroll.h"         // base class
#include "wx/bitmap.h"

class WXDLLIMPEXP_FWD_CORE wxSelectionStore;

#define wxVListBoxNameStr wxT("wxVListBox")

// ----------------------------------------------------------------------------
// wxVListBox
// ----------------------------------------------------------------------------

/*
    This class has two main differences from a regular listbox: it can have an
    arbitrarily huge number of items because it doesn't store them itself but
    uses OnDrawItem() callback to draw them and its items can have variable
    height as determined by OnMeasureItem().

    It emits the same events as wxListBox and the same event macros may be used
    with it.
 */
class WXDLLEXPORT wxVListBox : public wxVScrolledWindow
{
public:
    // constructors and such
    // ---------------------

    // default constructor, you must call Create() later
    wxVListBox() { Init(); }

    // normal constructor which calls Create() internally
    wxVListBox(wxWindow *parent,
               wxWindowID id = wxID_ANY,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxString& name = wxVListBoxNameStr)
    {
        Init();

        (void)Create(parent, id, pos, size, style, name);
    }

    // really creates the control and sets the initial number of items in it
    // (which may be changed later with SetItemCount())
    //
    // the only special style which may be specified here is wxLB_MULTIPLE
    //
    // returns true on success or false if the control couldn't be created
    bool Create(wxWindow *parent,
                wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxVListBoxNameStr);

    // dtor does some internal cleanup (deletes m_selStore if any)
    virtual ~wxVListBox();


    // accessors
    // ---------

    // get the number of items in the control
    size_t GetItemCount() const { return GetLineCount(); }

    // does this control use multiple selection?
    bool HasMultipleSelection() const { return m_selStore != NULL; }

    // get the currently selected item or wxNOT_FOUND if there is no selection
    //
    // this method is only valid for the single selection listboxes
    int GetSelection() const
    {
        wxASSERT_MSG( !HasMultipleSelection(),
                        wxT("GetSelection() can't be used with wxLB_MULTIPLE") );

        return m_current;
    }

    // is this item the current one?
    bool IsCurrent(size_t item) const { return item == (size_t)m_current; }
    #ifdef __WXUNIVERSAL__
    bool IsCurrent() const { return wxVScrolledWindow::IsCurrent(); }
    #endif

    // is this item selected?
    bool IsSelected(size_t item) const;

    // get the number of the selected items (maybe 0)
    //
    // this method is valid for both single and multi selection listboxes
    size_t GetSelectedCount() const;

    // get the first selected item, returns wxNOT_FOUND if none
    //
    // cookie is an opaque parameter which should be passed to
    // GetNextSelected() later
    //
    // this method is only valid for the multi selection listboxes
    int GetFirstSelected(unsigned long& cookie) const;

    // get next selection item, return wxNOT_FOUND if no more
    //
    // cookie must be the same parameter that was passed to GetFirstSelected()
    // before
    //
    // this method is only valid for the multi selection listboxes
    int GetNextSelected(unsigned long& cookie) const;

    // get the margins around each item
    wxPoint GetMargins() const { return m_ptMargins; }

    // get the background colour of selected cells
    const wxColour& GetSelectionBackground() const { return m_colBgSel; }


    // operations
    // ----------

    // set the number of items to be shown in the control
    //
    // this is just a synonym for wxVScrolledWindow::SetLineCount()
    virtual void SetItemCount(size_t count);

    // delete all items from the control
    void Clear() { SetItemCount(0); }

    // set the selection to the specified item, if it is wxNOT_FOUND the
    // selection is unset
    //
    // this function is only valid for the single selection listboxes
    void SetSelection(int selection);

    // selects or deselects the specified item which must be valid (i.e. not
    // equal to wxNOT_FOUND)
    //
    // return true if the items selection status has changed or false
    // otherwise
    //
    // this function is only valid for the multiple selection listboxes
    bool Select(size_t item, bool select = true);

    // selects the items in the specified range whose end points may be given
    // in any order
    //
    // return true if any items selection status has changed, false otherwise
    //
    // this function is only valid for the single selection listboxes
    bool SelectRange(size_t from, size_t to);

    // toggle the selection of the specified item (must be valid)
    //
    // this function is only valid for the multiple selection listboxes
    void Toggle(size_t item) { Select(item, !IsSelected(item)); }

    // select all items in the listbox
    //
    // the return code indicates if any items were affected by this operation
    // (true) or if nothing has changed (false)
    bool SelectAll() { return DoSelectAll(true); }

    // unselect all items in the listbox
    //
    // the return code has the same meaning as for SelectAll()
    bool DeselectAll() { return DoSelectAll(false); }

    // set the margins: horizontal margin is the distance between the window
    // border and the item contents while vertical margin is half of the
    // distance between items
    //
    // by default both margins are 0
    void SetMargins(const wxPoint& pt);
    void SetMargins(wxCoord x, wxCoord y) { SetMargins(wxPoint(x, y)); }

    // change the background colour of the selected cells
    void SetSelectionBackground(const wxColour& col);


    virtual wxVisualAttributes GetDefaultAttributes() const
    {
        return GetClassDefaultAttributes(GetWindowVariant());
    }

    static wxVisualAttributes
    GetClassDefaultAttributes(wxWindowVariant variant = wxWINDOW_VARIANT_NORMAL);

protected:
    // the derived class must implement this function to actually draw the item
    // with the given index on the provided DC
    virtual void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const = 0;

    // the derived class must implement this method to return the height of the
    // specified item
    virtual wxCoord OnMeasureItem(size_t n) const = 0;

    // this method may be used to draw separators between the lines; note that
    // the rectangle may be modified, typically to deflate it a bit before
    // passing to OnDrawItem()
    //
    // the base class version doesn't do anything
    virtual void OnDrawSeparator(wxDC& dc, wxRect& rect, size_t n) const;

    // this method is used to draw the items background and, maybe, a border
    // around it
    //
    // the base class version implements a reasonable default behaviour which
    // consists in drawing the selected item with the standard background
    // colour and drawing a border around the item if it is either selected or
    // current
    virtual void OnDrawBackground(wxDC& dc, const wxRect& rect, size_t n) const;

    // we implement OnGetLineHeight() in terms of OnMeasureItem() because this
    // allows us to add borders to the items easily
    //
    // this function is not supposed to be overridden by the derived classes
    virtual wxCoord OnGetLineHeight(size_t line) const;


    // event handlers
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftDClick(wxMouseEvent& event);


    // common part of all ctors
    void Init();

    // send the wxEVT_COMMAND_LISTBOX_SELECTED event
    void SendSelectedEvent();

    // common implementation of SelectAll() and DeselectAll()
    bool DoSelectAll(bool select);

    // change the current item (in single selection listbox it also implicitly
    // changes the selection); current may be wxNOT_FOUND in which case there
    // will be no current item any more
    //
    // return true if the current item changed, false otherwise
    bool DoSetCurrent(int current);

    // flags for DoHandleItemClick
    enum
    {
        ItemClick_Shift = 1,        // item shift-clicked
        ItemClick_Ctrl  = 2,        //       ctrl
        ItemClick_Kbd   = 4         // item selected from keyboard
    };

    // common part of keyboard and mouse handling processing code
    void DoHandleItemClick(int item, int flags);

private:
    // the current item or wxNOT_FOUND
    //
    // if m_selStore == NULL this is also the selected item, otherwise the
    // selections are managed by m_selStore
    int m_current;

    // the anchor of the selection for the multiselection listboxes:
    // shift-clicking an item extends the selection from m_anchor to the item
    // clicked, for example
    //
    // always wxNOT_FOUND for single selection listboxes
    int m_anchor;

    // the object managing our selected items if not NULL
    wxSelectionStore *m_selStore;

    // margins
    wxPoint m_ptMargins;

    // the selection bg colour
    wxColour m_colBgSel;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxVListBox)
    DECLARE_ABSTRACT_CLASS(wxVListBox)
};

#endif // _WX_VLBOX_H_

