///////////////////////////////////////////////////////////////////////////////
// Name:        wx/treebook.h
// Purpose:     wxTreebook: wxNotebook-like control presenting pages in a tree
// Author:      Evgeniy Tarassov, Vadim Zeitlin
// Modified by:
// Created:     2005-09-15
// RCS-ID:      $Id: treebook.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) 2005 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TREEBOOK_H_
#define _WX_TREEBOOK_H_

#include "wx/defs.h"

#if wxUSE_TREEBOOK

#include "wx/bookctrl.h"
#include "wx/treectrl.h"        // for wxArrayTreeItemIds
#include "wx/containr.h"

typedef wxWindow wxTreebookPage;

class WXDLLIMPEXP_FWD_CORE wxTreeEvent;

// ----------------------------------------------------------------------------
// wxTreebook
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxTreebook : public wxBookCtrlBase
{
public:
    // Constructors and such
    // ---------------------

    // Default ctor doesn't create the control, use Create() afterwards
    wxTreebook()
    {
        Init();
    }

    // This ctor creates the tree book control
    wxTreebook(wxWindow *parent,
               wxWindowID id,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = wxBK_DEFAULT,
               const wxString& name = wxEmptyString)
    {
        Init();

        (void)Create(parent, id, pos, size, style, name);
    }

    // Really creates the control
    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxBK_DEFAULT,
                const wxString& name = wxEmptyString);


    // Page insertion operations
    // -------------------------

    // Notice that page pointer may be NULL in which case the next non NULL
    // page (usually the first child page of a node) is shown when this page is
    // selected

    // Inserts a new page just before the page indicated by page.
    // The new page is placed on the same level as page.
    virtual bool InsertPage(size_t pos,
                            wxWindow *page,
                            const wxString& text,
                            bool bSelect = false,
                            int imageId = wxNOT_FOUND);

    // Inserts a new sub-page to the end of children of the page at given pos.
    virtual bool InsertSubPage(size_t pos,
                               wxWindow *page,
                               const wxString& text,
                               bool bSelect = false,
                               int imageId = wxNOT_FOUND);

    // Adds a new page at top level after all other pages.
    virtual bool AddPage(wxWindow *page,
                         const wxString& text,
                         bool bSelect = false,
                         int imageId = wxNOT_FOUND);

    // Adds a new child-page to the last top-level page inserted.
    // Useful when constructing 1 level tree structure.
    virtual bool AddSubPage(wxWindow *page,
                            const wxString& text,
                            bool bSelect = false,
                            int imageId = wxNOT_FOUND);

    // Deletes the page and ALL its children. Could trigger page selection
    // change in a case when selected page is removed. In that case its parent
    // is selected (or the next page if no parent).
    virtual bool DeletePage(size_t pos);


    // Tree operations
    // ---------------

    // Gets the page node state -- node is expanded or collapsed
    virtual bool IsNodeExpanded(size_t pos) const;

    // Expands or collapses the page node. Returns the previous state.
    // May generate page changing events (if selected page
    // is under the collapsed branch, then parent is autoselected).
    virtual bool ExpandNode(size_t pos, bool expand = true);

    // shortcut for ExpandNode(pos, false)
    bool CollapseNode(size_t pos) { return ExpandNode(pos, false); }

    // get the parent page or wxNOT_FOUND if this is a top level page
    int GetPageParent(size_t pos) const;

    // the tree control we use for showing the pages index tree
    wxTreeCtrl* GetTreeCtrl() const { return (wxTreeCtrl*)m_bookctrl; }


    // Standard operations inherited from wxBookCtrlBase
    // -------------------------------------------------

    virtual int GetSelection() const;
    virtual bool SetPageText(size_t n, const wxString& strText);
    virtual wxString GetPageText(size_t n) const;
    virtual int GetPageImage(size_t n) const;
    virtual bool SetPageImage(size_t n, int imageId);
    virtual wxSize CalcSizeFromPage(const wxSize& sizePage) const;
    virtual int SetSelection(size_t n) { return DoSetSelection(n, SetSelection_SendEvent); }
    virtual int ChangeSelection(size_t n) { return DoSetSelection(n); }
    virtual int HitTest(const wxPoint& pt, long *flags = NULL) const;
    virtual void SetImageList(wxImageList *imageList);
    virtual void AssignImageList(wxImageList *imageList);
    virtual bool DeleteAllPages();

protected:
    // Implementation of a page removal. See DeletPage for comments.
    wxTreebookPage *DoRemovePage(size_t pos);

    // This subclass of wxBookCtrlBase accepts NULL page pointers (empty pages)
    virtual bool AllowNullPage() const { return true; }

    // event handlers
    void OnTreeSelectionChange(wxTreeEvent& event);
    void OnTreeNodeExpandedCollapsed(wxTreeEvent& event);

    // array of page ids and page windows
    wxArrayTreeItemIds m_treeIds;

    // the currently selected page or wxNOT_FOUND if none
    int m_selection;

    // in the situation when m_selection page is not wxNOT_FOUND but page is
    // NULL this is the first (sub)child that has a non-NULL page
    int m_actualSelection;

private:
    // common part of all constructors
    void Init();

    // The real implementations of page insertion functions
    // ------------------------------------------------------
    // All DoInsert/Add(Sub)Page functions add the page into :
    // - the base class
    // - the tree control
    // - update the index/TreeItemId corespondance array
    bool DoInsertPage(size_t pos,
                      wxWindow *page,
                      const wxString& text,
                      bool bSelect = false,
                      int imageId = wxNOT_FOUND);
    bool DoInsertSubPage(size_t pos,
                         wxWindow *page,
                         const wxString& text,
                         bool bSelect = false,
                         int imageId = wxNOT_FOUND);
    bool DoAddSubPage(wxWindow *page,
                         const wxString& text,
                         bool bSelect = false,
                         int imageId = wxNOT_FOUND);

    // Sets selection in the tree control and updates the page being shown.
    int DoSetSelection(size_t pos, int flags = 0);

    // Returns currently shown page. In a case when selected the node
    // has empty (NULL) page finds first (sub)child with not-empty page.
    wxTreebookPage *DoGetCurrentPage() const;

    // Does the selection update. Called from page insertion functions
    // to update selection if the selected page was pushed by the newly inserted
    void DoUpdateSelection(bool bSelect, int page);


    // Operations on the internal private members of the class
    // -------------------------------------------------------
    // Returns the page TreeItemId for the page.
    // Or, if the page index is incorrect, a fake one (fakePage.IsOk() == false)
    wxTreeItemId DoInternalGetPage(size_t pos) const;

    // Linear search for a page with the id specified. If no page
    // found wxNOT_FOUND is returned. The function is used when we catch an event
    // from m_tree (wxTreeCtrl) component.
    int DoInternalFindPageById(wxTreeItemId page) const;

    // Updates page and wxTreeItemId correspondance.
    void DoInternalAddPage(size_t newPos, wxWindow *page, wxTreeItemId pageId);

    // Removes the page from internal structure.
    void DoInternalRemovePage(size_t pos)
        { DoInternalRemovePageRange(pos, 0); }

    // Removes the page and all its children designated by subCount
    // from internal structures of the control.
    void DoInternalRemovePageRange(size_t pos, size_t subCount);

    // Returns internal number of pages which can be different from
    // GetPageCount() while performing a page insertion or removal.
    size_t DoInternalGetPageCount() const { return m_treeIds.Count(); }


    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxTreebook)
    WX_DECLARE_CONTROL_CONTAINER();
};


// ----------------------------------------------------------------------------
// treebook event class and related stuff
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxTreebookEvent : public wxBookCtrlBaseEvent
{
public:
    wxTreebookEvent(wxEventType commandType = wxEVT_NULL, int id = 0,
                    int nSel = wxNOT_FOUND, int nOldSel = wxNOT_FOUND)
        : wxBookCtrlBaseEvent(commandType, id, nSel, nOldSel)
    {
    }

    wxTreebookEvent(const wxTreebookEvent& event)
        : wxBookCtrlBaseEvent(event)
    {
    }

    virtual wxEvent *Clone() const { return new wxTreebookEvent(*this); }

private:
    DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxTreebookEvent)
};

extern WXDLLIMPEXP_CORE const wxEventType wxEVT_COMMAND_TREEBOOK_PAGE_CHANGED;
extern WXDLLIMPEXP_CORE const wxEventType wxEVT_COMMAND_TREEBOOK_PAGE_CHANGING;
extern WXDLLIMPEXP_CORE const wxEventType wxEVT_COMMAND_TREEBOOK_NODE_COLLAPSED;
extern WXDLLIMPEXP_CORE const wxEventType wxEVT_COMMAND_TREEBOOK_NODE_EXPANDED;

typedef void (wxEvtHandler::*wxTreebookEventFunction)(wxTreebookEvent&);

#define wxTreebookEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxTreebookEventFunction, &func)

#define EVT_TREEBOOK_PAGE_CHANGED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_TREEBOOK_PAGE_CHANGED, winid, wxTreebookEventHandler(fn))

#define EVT_TREEBOOK_PAGE_CHANGING(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_TREEBOOK_PAGE_CHANGING, winid, wxTreebookEventHandler(fn))

#define EVT_TREEBOOK_NODE_COLLAPSED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_TREEBOOK_NODE_COLLAPSED, winid, wxTreebookEventHandler(fn))

#define EVT_TREEBOOK_NODE_EXPANDED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_TREEBOOK_NODE_EXPANDED, winid, wxTreebookEventHandler(fn))


#endif // wxUSE_TREEBOOK

#endif // _WX_TREEBOOK_H_
