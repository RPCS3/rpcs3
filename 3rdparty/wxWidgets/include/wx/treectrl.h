/////////////////////////////////////////////////////////////////////////////
// Name:        wx/treectrl.h
// Purpose:     wxTreeCtrl base header
// Author:      Karsten Ballueder
// Modified by:
// Created:
// Copyright:   (c) Karsten Ballueder
// RCS-ID:      $Id: treectrl.h 49563 2007-10-31 20:46:21Z VZ $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TREECTRL_H_BASE_
#define _WX_TREECTRL_H_BASE_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"

#if wxUSE_TREECTRL

#include "wx/control.h"
#include "wx/treebase.h"
#include "wx/textctrl.h" // wxTextCtrl::ms_classinfo used through CLASSINFO macro

class WXDLLIMPEXP_FWD_CORE wxImageList;

// ----------------------------------------------------------------------------
// wxTreeCtrlBase
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxTreeCtrlBase : public wxControl
{
public:
    wxTreeCtrlBase()
    {
        m_imageListNormal =
        m_imageListState = NULL;
        m_ownsImageListNormal =
        m_ownsImageListState = false;

        // arbitrary default
        m_spacing = 18;

        // quick DoGetBestSize calculation
        m_quickBestSize = true;
    }

    virtual ~wxTreeCtrlBase();

    // accessors
    // ---------

        // get the total number of items in the control
    virtual unsigned int GetCount() const = 0;

        // indent is the number of pixels the children are indented relative to
        // the parents position. SetIndent() also redraws the control
        // immediately.
    virtual unsigned int GetIndent() const = 0;
    virtual void SetIndent(unsigned int indent) = 0;

        // spacing is the number of pixels between the start and the Text
        // (has no effect under wxMSW)
    unsigned int GetSpacing() const { return m_spacing; }
    void SetSpacing(unsigned int spacing) { m_spacing = spacing; }

        // image list: these functions allow to associate an image list with
        // the control and retrieve it. Note that the control does _not_ delete
        // the associated image list when it's deleted in order to allow image
        // lists to be shared between different controls.
        //
        // The normal image list is for the icons which correspond to the
        // normal tree item state (whether it is selected or not).
        // Additionally, the application might choose to show a state icon
        // which corresponds to an app-defined item state (for example,
        // checked/unchecked) which are taken from the state image list.
    wxImageList *GetImageList() const { return m_imageListNormal; }
    wxImageList *GetStateImageList() const { return m_imageListState; }

    virtual void SetImageList(wxImageList *imageList) = 0;
    virtual void SetStateImageList(wxImageList *imageList) = 0;
    void AssignImageList(wxImageList *imageList)
    {
        SetImageList(imageList);
        m_ownsImageListNormal = true;
    }
    void AssignStateImageList(wxImageList *imageList)
    {
        SetStateImageList(imageList);
        m_ownsImageListState = true;
    }


    // Functions to work with tree ctrl items. Unfortunately, they can _not_ be
    // member functions of wxTreeItem because they must know the tree the item
    // belongs to for Windows implementation and storing the pointer to
    // wxTreeCtrl in each wxTreeItem is just too much waste.

    // accessors
    // ---------

        // retrieve items label
    virtual wxString GetItemText(const wxTreeItemId& item) const = 0;
        // get one of the images associated with the item (normal by default)
    virtual int GetItemImage(const wxTreeItemId& item,
                     wxTreeItemIcon which = wxTreeItemIcon_Normal) const = 0;
        // get the data associated with the item
    virtual wxTreeItemData *GetItemData(const wxTreeItemId& item) const = 0;

        // get the item's text colour
    virtual wxColour GetItemTextColour(const wxTreeItemId& item) const = 0;

        // get the item's background colour
    virtual wxColour GetItemBackgroundColour(const wxTreeItemId& item) const = 0;

        // get the item's font
    virtual wxFont GetItemFont(const wxTreeItemId& item) const = 0;

    // modifiers
    // ---------

        // set items label
    virtual void SetItemText(const wxTreeItemId& item, const wxString& text) = 0;
        // get one of the images associated with the item (normal by default)
    virtual void SetItemImage(const wxTreeItemId& item,
                              int image,
                              wxTreeItemIcon which = wxTreeItemIcon_Normal) = 0;
        // associate some data with the item
    virtual void SetItemData(const wxTreeItemId& item, wxTreeItemData *data) = 0;

        // force appearance of [+] button near the item. This is useful to
        // allow the user to expand the items which don't have any children now
        // - but instead add them only when needed, thus minimizing memory
        // usage and loading time.
    virtual void SetItemHasChildren(const wxTreeItemId& item,
                                    bool has = true) = 0;

        // the item will be shown in bold
    virtual void SetItemBold(const wxTreeItemId& item, bool bold = true) = 0;

        // the item will be shown with a drop highlight
    virtual void SetItemDropHighlight(const wxTreeItemId& item,
                                      bool highlight = true) = 0;

        // set the items text colour
    virtual void SetItemTextColour(const wxTreeItemId& item,
                                   const wxColour& col) = 0;

        // set the items background colour
    virtual void SetItemBackgroundColour(const wxTreeItemId& item,
                                         const wxColour& col) = 0;

        // set the items font (should be of the same height for all items)
    virtual void SetItemFont(const wxTreeItemId& item,
                             const wxFont& font) = 0;

    // item status inquiries
    // ---------------------

        // is the item visible (it might be outside the view or not expanded)?
    virtual bool IsVisible(const wxTreeItemId& item) const = 0;
        // does the item has any children?
    virtual bool ItemHasChildren(const wxTreeItemId& item) const = 0;
        // same as above
    bool HasChildren(const wxTreeItemId& item) const
      { return ItemHasChildren(item); }
        // is the item expanded (only makes sense if HasChildren())?
    virtual bool IsExpanded(const wxTreeItemId& item) const = 0;
        // is this item currently selected (the same as has focus)?
    virtual bool IsSelected(const wxTreeItemId& item) const = 0;
        // is item text in bold font?
    virtual bool IsBold(const wxTreeItemId& item) const = 0;
#if wxABI_VERSION >= 20801
        // is the control empty?
    bool IsEmpty() const;
#endif // wxABI 2.8.1+


    // number of children
    // ------------------

        // if 'recursively' is false, only immediate children count, otherwise
        // the returned number is the number of all items in this branch
    virtual size_t GetChildrenCount(const wxTreeItemId& item,
                                    bool recursively = true) const = 0;

    // navigation
    // ----------

    // wxTreeItemId.IsOk() will return false if there is no such item

        // get the root tree item
    virtual wxTreeItemId GetRootItem() const = 0;

        // get the item currently selected (may return NULL if no selection)
    virtual wxTreeItemId GetSelection() const = 0;

        // get the items currently selected, return the number of such item
        //
        // NB: this operation is expensive and can take a long time for a
        //     control with a lot of items (~ O(number of items)).
    virtual size_t GetSelections(wxArrayTreeItemIds& selections) const = 0;

        // get the parent of this item (may return NULL if root)
    virtual wxTreeItemId GetItemParent(const wxTreeItemId& item) const = 0;

        // for this enumeration function you must pass in a "cookie" parameter
        // which is opaque for the application but is necessary for the library
        // to make these functions reentrant (i.e. allow more than one
        // enumeration on one and the same object simultaneously). Of course,
        // the "cookie" passed to GetFirstChild() and GetNextChild() should be
        // the same!

        // get the first child of this item
    virtual wxTreeItemId GetFirstChild(const wxTreeItemId& item,
                                       wxTreeItemIdValue& cookie) const = 0;
        // get the next child
    virtual wxTreeItemId GetNextChild(const wxTreeItemId& item,
                                      wxTreeItemIdValue& cookie) const = 0;
        // get the last child of this item - this method doesn't use cookies
    virtual wxTreeItemId GetLastChild(const wxTreeItemId& item) const = 0;

        // get the next sibling of this item
    virtual wxTreeItemId GetNextSibling(const wxTreeItemId& item) const = 0;
        // get the previous sibling
    virtual wxTreeItemId GetPrevSibling(const wxTreeItemId& item) const = 0;

        // get first visible item
    virtual wxTreeItemId GetFirstVisibleItem() const = 0;
        // get the next visible item: item must be visible itself!
        // see IsVisible() and wxTreeCtrl::GetFirstVisibleItem()
    virtual wxTreeItemId GetNextVisible(const wxTreeItemId& item) const = 0;
        // get the previous visible item: item must be visible itself!
    virtual wxTreeItemId GetPrevVisible(const wxTreeItemId& item) const = 0;

    // operations
    // ----------

        // add the root node to the tree
    virtual wxTreeItemId AddRoot(const wxString& text,
                                 int image = -1, int selImage = -1,
                                 wxTreeItemData *data = NULL) = 0;

        // insert a new item in as the first child of the parent
    wxTreeItemId PrependItem(const wxTreeItemId& parent,
                             const wxString& text,
                             int image = -1, int selImage = -1,
                             wxTreeItemData *data = NULL)
    {
        return DoInsertItem(parent, 0u, text, image, selImage, data);
    }

        // insert a new item after a given one
    wxTreeItemId InsertItem(const wxTreeItemId& parent,
                            const wxTreeItemId& idPrevious,
                            const wxString& text,
                            int image = -1, int selImage = -1,
                            wxTreeItemData *data = NULL)
    {
        return DoInsertAfter(parent, idPrevious, text, image, selImage, data);
    }

        // insert a new item before the one with the given index
    wxTreeItemId InsertItem(const wxTreeItemId& parent,
                            size_t pos,
                            const wxString& text,
                            int image = -1, int selImage = -1,
                            wxTreeItemData *data = NULL)
    {
        return DoInsertItem(parent, pos, text, image, selImage, data);
    }

        // insert a new item in as the last child of the parent
    wxTreeItemId AppendItem(const wxTreeItemId& parent,
                            const wxString& text,
                            int image = -1, int selImage = -1,
                            wxTreeItemData *data = NULL)
    {
        return DoInsertItem(parent, (size_t)-1, text, image, selImage, data);
    }

        // delete this item and associated data if any
    virtual void Delete(const wxTreeItemId& item) = 0;
        // delete all children (but don't delete the item itself)
        // NB: this won't send wxEVT_COMMAND_TREE_ITEM_DELETED events
    virtual void DeleteChildren(const wxTreeItemId& item) = 0;
        // delete all items from the tree
        // NB: this won't send wxEVT_COMMAND_TREE_ITEM_DELETED events
    virtual void DeleteAllItems() = 0;

        // expand this item
    virtual void Expand(const wxTreeItemId& item) = 0;
        // expand the item and all its childs and thats childs
    void ExpandAllChildren(const wxTreeItemId& item);
        // expand all items
    void ExpandAll();
        // collapse the item without removing its children
    virtual void Collapse(const wxTreeItemId& item) = 0;
#if wxABI_VERSION >= 20801
        // collapse the item and all its childs and thats childs
    void CollapseAllChildren(const wxTreeItemId& item);
        // collapse all items
    void CollapseAll();
#endif // wxABI 2.8.1+
        // collapse the item and remove all children
    virtual void CollapseAndReset(const wxTreeItemId& item) = 0;
        // toggles the current state
    virtual void Toggle(const wxTreeItemId& item) = 0;

        // remove the selection from currently selected item (if any)
    virtual void Unselect() = 0;
        // unselect all items (only makes sense for multiple selection control)
    virtual void UnselectAll() = 0;
        // select this item
    virtual void SelectItem(const wxTreeItemId& item, bool select = true) = 0;
        // unselect this item
    void UnselectItem(const wxTreeItemId& item) { SelectItem(item, false); }
        // toggle item selection
    void ToggleItemSelection(const wxTreeItemId& item)
    {
        SelectItem(item, !IsSelected(item));
    }

        // make sure this item is visible (expanding the parent item and/or
        // scrolling to this item if necessary)
    virtual void EnsureVisible(const wxTreeItemId& item) = 0;
        // scroll to this item (but don't expand its parent)
    virtual void ScrollTo(const wxTreeItemId& item) = 0;

        // start editing the item label: this (temporarily) replaces the item
        // with a one line edit control. The item will be selected if it hadn't
        // been before. textCtrlClass parameter allows you to create an edit
        // control of arbitrary user-defined class deriving from wxTextCtrl.
    virtual wxTextCtrl *EditLabel(const wxTreeItemId& item,
                      wxClassInfo* textCtrlClass = CLASSINFO(wxTextCtrl)) = 0;
        // returns the same pointer as StartEdit() if the item is being edited,
        // NULL otherwise (it's assumed that no more than one item may be
        // edited simultaneously)
    virtual wxTextCtrl *GetEditControl() const = 0;
        // end editing and accept or discard the changes to item label
    virtual void EndEditLabel(const wxTreeItemId& item,
                              bool discardChanges = false) = 0;

    // sorting
    // -------

        // this function is called to compare 2 items and should return -1, 0
        // or +1 if the first item is less than, equal to or greater than the
        // second one. The base class version performs alphabetic comparaison
        // of item labels (GetText)
    virtual int OnCompareItems(const wxTreeItemId& item1,
                               const wxTreeItemId& item2)
    {
        return wxStrcmp(GetItemText(item1), GetItemText(item2));
    }

        // sort the children of this item using OnCompareItems
        //
        // NB: this function is not reentrant and not MT-safe (FIXME)!
    virtual void SortChildren(const wxTreeItemId& item) = 0;

    // items geometry
    // --------------

        // determine to which item (if any) belongs the given point (the
        // coordinates specified are relative to the client area of tree ctrl)
        // and, in the second variant, fill the flags parameter with a bitmask
        // of wxTREE_HITTEST_xxx constants.
    wxTreeItemId HitTest(const wxPoint& point) const
        { int dummy; return DoTreeHitTest(point, dummy); }
    wxTreeItemId HitTest(const wxPoint& point, int& flags) const
        { return DoTreeHitTest(point, flags); }

        // get the bounding rectangle of the item (or of its label only)
    virtual bool GetBoundingRect(const wxTreeItemId& item,
                                 wxRect& rect,
                                 bool textOnly = false) const = 0;


    // implementation
    // --------------

    virtual bool ShouldInheritColours() const { return false; }

    // hint whether to calculate best size quickly or accurately
    void SetQuickBestSize(bool q) { m_quickBestSize = q; }
    bool GetQuickBestSize() const { return m_quickBestSize; }

protected:
    virtual wxSize DoGetBestSize() const;

    // common part of Append/Prepend/InsertItem()
    //
    // pos is the position at which to insert the item or (size_t)-1 to append
    // it to the end
    virtual wxTreeItemId DoInsertItem(const wxTreeItemId& parent,
                                      size_t pos,
                                      const wxString& text,
                                      int image, int selImage,
                                      wxTreeItemData *data) = 0;

    // and this function implements overloaded InsertItem() taking wxTreeItemId
    // (it can't be called InsertItem() as we'd have virtual function hiding
    // problem in derived classes then)
    virtual wxTreeItemId DoInsertAfter(const wxTreeItemId& parent,
                                       const wxTreeItemId& idPrevious,
                                       const wxString& text,
                                       int image = -1, int selImage = -1,
                                       wxTreeItemData *data = NULL) = 0;

    // real HitTest() implementation: again, can't be called just HitTest()
    // because it's overloaded and so the non-virtual overload would be hidden
    // (and can't be called DoHitTest() because this is already in wxWindow)
    virtual wxTreeItemId DoTreeHitTest(const wxPoint& point,
                                        int& flags) const = 0;


    wxImageList *m_imageListNormal, // images for tree elements
                *m_imageListState;  // special images for app defined states
    bool         m_ownsImageListNormal,
                 m_ownsImageListState;

    // spacing between left border and the text
    unsigned int m_spacing;

    // whether full or quick calculation is done in DoGetBestSize
    bool        m_quickBestSize;


    DECLARE_NO_COPY_CLASS(wxTreeCtrlBase)
};

// ----------------------------------------------------------------------------
// include the platform-dependent wxTreeCtrl class
// ----------------------------------------------------------------------------

#if defined(__WXUNIVERSAL__)
    #include "wx/generic/treectlg.h"
#elif defined(__WXPALMOS__)
    #include "wx/palmos/treectrl.h"
#elif defined(__WXMSW__)
    #include "wx/msw/treectrl.h"
#elif defined(__WXMOTIF__)
    #include "wx/generic/treectlg.h"
#elif defined(__WXGTK__)
    #include "wx/generic/treectlg.h"
#elif defined(__WXMAC__)
    #include "wx/generic/treectlg.h"
#elif defined(__WXCOCOA__)
    #include "wx/generic/treectlg.h"
#elif defined(__WXPM__)
    #include "wx/generic/treectlg.h"
#endif

#endif // wxUSE_TREECTRL

#endif // _WX_TREECTRL_H_BASE_
