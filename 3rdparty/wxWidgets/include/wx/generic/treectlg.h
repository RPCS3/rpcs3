/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/treectlg.h
// Purpose:     wxTreeCtrl class
// Author:      Robert Roebling
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: treectlg.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) 1997,1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _GENERIC_TREECTRL_H_
#define _GENERIC_TREECTRL_H_

#if wxUSE_TREECTRL

#include "wx/scrolwin.h"
#include "wx/pen.h"

// -----------------------------------------------------------------------------
// forward declaration
// -----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxGenericTreeItem;

class WXDLLIMPEXP_FWD_CORE wxTreeItemData;

class WXDLLIMPEXP_FWD_CORE wxTreeRenameTimer;
class WXDLLIMPEXP_FWD_CORE wxTreeFindTimer;
class WXDLLIMPEXP_FWD_CORE wxTreeTextCtrl;
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;

// -----------------------------------------------------------------------------
// wxGenericTreeCtrl - the tree control
// -----------------------------------------------------------------------------

class WXDLLEXPORT wxGenericTreeCtrl : public wxTreeCtrlBase,
                                      public wxScrollHelper
{
public:
    // creation
    // --------

    wxGenericTreeCtrl() : wxTreeCtrlBase(), wxScrollHelper(this) { Init(); }

    wxGenericTreeCtrl(wxWindow *parent, wxWindowID id = wxID_ANY,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = wxTR_DEFAULT_STYLE,
               const wxValidator &validator = wxDefaultValidator,
               const wxString& name = wxTreeCtrlNameStr)
        : wxTreeCtrlBase(),
          wxScrollHelper(this)
    {
        Init();
        Create(parent, id, pos, size, style, validator, name);
    }

    virtual ~wxGenericTreeCtrl();

    bool Create(wxWindow *parent, wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxTR_DEFAULT_STYLE,
                const wxValidator &validator = wxDefaultValidator,
                const wxString& name = wxTreeCtrlNameStr);


    // implement base class pure virtuals
    // ----------------------------------

    virtual unsigned int GetCount() const;

    virtual unsigned int GetIndent() const { return m_indent; }
    virtual void SetIndent(unsigned int indent);


    virtual void SetImageList(wxImageList *imageList);
    virtual void SetStateImageList(wxImageList *imageList);

    virtual wxString GetItemText(const wxTreeItemId& item) const;
    virtual int GetItemImage(const wxTreeItemId& item,
                     wxTreeItemIcon which = wxTreeItemIcon_Normal) const;
    virtual wxTreeItemData *GetItemData(const wxTreeItemId& item) const;
    virtual wxColour GetItemTextColour(const wxTreeItemId& item) const;
    virtual wxColour GetItemBackgroundColour(const wxTreeItemId& item) const;
    virtual wxFont GetItemFont(const wxTreeItemId& item) const;

    virtual void SetItemText(const wxTreeItemId& item, const wxString& text);
    virtual void SetItemImage(const wxTreeItemId& item,
                              int image,
                              wxTreeItemIcon which = wxTreeItemIcon_Normal);
    virtual void SetItemData(const wxTreeItemId& item, wxTreeItemData *data);

    virtual void SetItemHasChildren(const wxTreeItemId& item, bool has = true);
    virtual void SetItemBold(const wxTreeItemId& item, bool bold = true);
    virtual void SetItemDropHighlight(const wxTreeItemId& item, bool highlight = true);
    virtual void SetItemTextColour(const wxTreeItemId& item, const wxColour& col);
    virtual void SetItemBackgroundColour(const wxTreeItemId& item, const wxColour& col);
    virtual void SetItemFont(const wxTreeItemId& item, const wxFont& font);

    virtual bool IsVisible(const wxTreeItemId& item) const;
    virtual bool ItemHasChildren(const wxTreeItemId& item) const;
    virtual bool IsExpanded(const wxTreeItemId& item) const;
    virtual bool IsSelected(const wxTreeItemId& item) const;
    virtual bool IsBold(const wxTreeItemId& item) const;

    virtual size_t GetChildrenCount(const wxTreeItemId& item,
                                    bool recursively = true) const;

    // navigation
    // ----------

    virtual wxTreeItemId GetRootItem() const { return m_anchor; }
    virtual wxTreeItemId GetSelection() const { return m_current; }
    virtual size_t GetSelections(wxArrayTreeItemIds&) const;

    virtual wxTreeItemId GetItemParent(const wxTreeItemId& item) const;
    virtual wxTreeItemId GetFirstChild(const wxTreeItemId& item,
                                       wxTreeItemIdValue& cookie) const;
    virtual wxTreeItemId GetNextChild(const wxTreeItemId& item,
                                      wxTreeItemIdValue& cookie) const;
    virtual wxTreeItemId GetLastChild(const wxTreeItemId& item) const;
    virtual wxTreeItemId GetNextSibling(const wxTreeItemId& item) const;
    virtual wxTreeItemId GetPrevSibling(const wxTreeItemId& item) const;

    virtual wxTreeItemId GetFirstVisibleItem() const;
    virtual wxTreeItemId GetNextVisible(const wxTreeItemId& item) const;
    virtual wxTreeItemId GetPrevVisible(const wxTreeItemId& item) const;


    // operations
    // ----------

    virtual wxTreeItemId AddRoot(const wxString& text,
                         int image = -1, int selectedImage = -1,
                         wxTreeItemData *data = NULL);

    virtual void Delete(const wxTreeItemId& item);
    virtual void DeleteChildren(const wxTreeItemId& item);
    virtual void DeleteAllItems();

    virtual void Expand(const wxTreeItemId& item);
    virtual void Collapse(const wxTreeItemId& item);
    virtual void CollapseAndReset(const wxTreeItemId& item);
    virtual void Toggle(const wxTreeItemId& item);

    virtual void Unselect();
    virtual void UnselectAll();
    virtual void SelectItem(const wxTreeItemId& item, bool select = true);

    virtual void EnsureVisible(const wxTreeItemId& item);
    virtual void ScrollTo(const wxTreeItemId& item);

    virtual wxTextCtrl *EditLabel(const wxTreeItemId& item,
                          wxClassInfo* textCtrlClass = CLASSINFO(wxTextCtrl));
    virtual wxTextCtrl *GetEditControl() const;
    virtual void EndEditLabel(const wxTreeItemId& item,
                              bool discardChanges = false);

    virtual void SortChildren(const wxTreeItemId& item);

    // items geometry
    // --------------

    virtual bool GetBoundingRect(const wxTreeItemId& item,
                                 wxRect& rect,
                                 bool textOnly = false) const;


    // this version specific methods
    // -----------------------------

    wxImageList *GetButtonsImageList() const { return m_imageListButtons; }
    void SetButtonsImageList(wxImageList *imageList);
    void AssignButtonsImageList(wxImageList *imageList);

    void SetDropEffectAboveItem( bool above = false ) { m_dropEffectAboveItem = above; }
    bool GetDropEffectAboveItem() const { return m_dropEffectAboveItem; }

    wxTreeItemId GetNext(const wxTreeItemId& item) const;

#if WXWIN_COMPATIBILITY_2_6
    // use EditLabel() instead
    void Edit( const wxTreeItemId& item ) { EditLabel(item); }
#endif // WXWIN_COMPATIBILITY_2_6

#if WXWIN_COMPATIBILITY_2_4
    // deprecated functions: use Set/GetItemImage directly
    wxDEPRECATED( int GetItemSelectedImage(const wxTreeItemId& item) const );
    wxDEPRECATED( void SetItemSelectedImage(const wxTreeItemId& item, int image) );

    // use the versions taking wxTreeItemIdValue cookies (note that
    // GetNextChild() is not inside wxDEPRECATED on purpose, as otherwise we
    // get twice as many warnings without any added benefit: it is always used
    // with GetFirstChild() anyhow)
    wxDEPRECATED( wxTreeItemId GetFirstChild(const wxTreeItemId& item,
                                             long& cookie) const );
    wxTreeItemId GetNextChild(const wxTreeItemId& item,
                              long& cookie) const;
#endif // WXWIN_COMPATIBILITY_2_4

    // implementation only from now on

    // overridden base class virtuals
    virtual bool SetBackgroundColour(const wxColour& colour);
    virtual bool SetForegroundColour(const wxColour& colour);

    virtual void Freeze();
    virtual void Thaw();
    virtual void Refresh(bool eraseBackground = true, const wxRect *rect = NULL);

    virtual bool SetFont( const wxFont &font );
    virtual void SetWindowStyle(const long styles);

    // callbacks
    void OnPaint( wxPaintEvent &event );
    void OnSetFocus( wxFocusEvent &event );
    void OnKillFocus( wxFocusEvent &event );
    void OnChar( wxKeyEvent &event );
    void OnMouse( wxMouseEvent &event );
    void OnGetToolTip( wxTreeEvent &event );
    void OnSize( wxSizeEvent &event );
    void OnInternalIdle( );

    virtual wxVisualAttributes GetDefaultAttributes() const
    {
        return GetClassDefaultAttributes(GetWindowVariant());
    }

    static wxVisualAttributes
    GetClassDefaultAttributes(wxWindowVariant variant = wxWINDOW_VARIANT_NORMAL);

    // implementation helpers
    void AdjustMyScrollbars();

    WX_FORWARD_TO_SCROLL_HELPER()

protected:
    friend class wxGenericTreeItem;
    friend class wxTreeRenameTimer;
    friend class wxTreeFindTimer;
    friend class wxTreeTextCtrl;

    wxFont               m_normalFont;
    wxFont               m_boldFont;

    wxGenericTreeItem   *m_anchor;
    wxGenericTreeItem   *m_current,
                        *m_key_current,
                        // A hint to select a parent item after deleting a child
                        *m_select_me;
    unsigned short       m_indent;
    int                  m_lineHeight;
    wxPen                m_dottedPen;
    wxBrush             *m_hilightBrush,
                        *m_hilightUnfocusedBrush;
    bool                 m_hasFocus;
    bool                 m_dirty;
    bool                 m_ownsImageListButtons;
    bool                 m_isDragging; // true between BEGIN/END drag events
    bool                 m_lastOnSame;  // last click on the same item as prev
    wxImageList         *m_imageListButtons;

    int                  m_freezeCount;
    int                  m_dragCount;
    wxPoint              m_dragStart;
    wxGenericTreeItem   *m_dropTarget;
    wxCursor             m_oldCursor;  // cursor is changed while dragging
    wxGenericTreeItem   *m_oldSelection;
    wxGenericTreeItem   *m_underMouse; // for visual effects
    wxTreeTextCtrl      *m_textCtrl;

    wxTimer             *m_renameTimer;

    // incremental search data
    wxString             m_findPrefix;
    wxTimer             *m_findTimer;

    bool                 m_dropEffectAboveItem;

    // the common part of all ctors
    void Init();

    // misc helpers
    void SendDeleteEvent(wxGenericTreeItem *itemBeingDeleted);

    void DrawBorder(const wxTreeItemId& item);
    void DrawLine(const wxTreeItemId& item, bool below);
    void DrawDropEffect(wxGenericTreeItem *item);

    void DoSelectItem(const wxTreeItemId& id,
                      bool unselect_others = true,
                      bool extended_select = false);

    virtual wxTreeItemId DoInsertItem(const wxTreeItemId& parent,
                                      size_t previous,
                                      const wxString& text,
                                      int image,
                                      int selectedImage,
                                      wxTreeItemData *data);
    virtual wxTreeItemId DoInsertAfter(const wxTreeItemId& parent,
                                       const wxTreeItemId& idPrevious,
                                       const wxString& text,
                                       int image = -1, int selImage = -1,
                                       wxTreeItemData *data = NULL);
    virtual wxTreeItemId DoTreeHitTest(const wxPoint& point, int& flags) const;

    // called by wxTextTreeCtrl when it marks itself for deletion
    void ResetTextControl();

    // find the first item starting with the given prefix after the given item
    wxTreeItemId FindItem(const wxTreeItemId& id, const wxString& prefix) const;

    bool HasButtons() const { return HasFlag(wxTR_HAS_BUTTONS); }

    void CalculateLineHeight();
    int  GetLineHeight(wxGenericTreeItem *item) const;
    void PaintLevel( wxGenericTreeItem *item, wxDC& dc, int level, int &y );
    void PaintItem( wxGenericTreeItem *item, wxDC& dc);

    void CalculateLevel( wxGenericTreeItem *item, wxDC &dc, int level, int &y );
    void CalculatePositions();
    void CalculateSize( wxGenericTreeItem *item, wxDC &dc );

    void RefreshSubtree( wxGenericTreeItem *item );
    void RefreshLine( wxGenericTreeItem *item );

    // redraw all selected items
    void RefreshSelected();

    // RefreshSelected() recursive helper
    void RefreshSelectedUnder(wxGenericTreeItem *item);

    void OnRenameTimer();
    bool OnRenameAccept(wxGenericTreeItem *item, const wxString& value);
    void OnRenameCancelled(wxGenericTreeItem *item);

    void FillArray(wxGenericTreeItem*, wxArrayTreeItemIds&) const;
    void SelectItemRange( wxGenericTreeItem *item1, wxGenericTreeItem *item2 );
    bool TagAllChildrenUntilLast(wxGenericTreeItem *crt_item, wxGenericTreeItem *last_item, bool select);
    bool TagNextChildren(wxGenericTreeItem *crt_item, wxGenericTreeItem *last_item, bool select);
    void UnselectAllChildren( wxGenericTreeItem *item );
    void ChildrenClosing(wxGenericTreeItem* item);

    void DoDirtyProcessing();

    virtual wxSize DoGetBestSize() const;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxGenericTreeCtrl)
    DECLARE_NO_COPY_CLASS(wxGenericTreeCtrl)
};

#if !defined(__WXMSW__) || defined(__WXUNIVERSAL__)
/*
 * wxTreeCtrl has to be a real class or we have problems with
 * the run-time information.
 */

class WXDLLEXPORT wxTreeCtrl: public wxGenericTreeCtrl
{
    DECLARE_DYNAMIC_CLASS(wxTreeCtrl)

public:
    wxTreeCtrl() {}

    wxTreeCtrl(wxWindow *parent, wxWindowID id = wxID_ANY,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = wxTR_DEFAULT_STYLE,
               const wxValidator &validator = wxDefaultValidator,
               const wxString& name = wxTreeCtrlNameStr)
    : wxGenericTreeCtrl(parent, id, pos, size, style, validator, name)
    {
    }
};
#endif // !__WXMSW__ || __WXUNIVERSAL__

#endif // wxUSE_TREECTRL

#endif // _GENERIC_TREECTRL_H_
