///////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/treebkg.cpp
// Purpose:     generic implementation of wxTreebook
// Author:      Evgeniy Tarassov, Vadim Zeitlin
// Modified by:
// Created:     2005-09-15
// RCS-ID:      $Id: treebkg.cpp 54645 2008-07-15 21:29:10Z JS $
// Copyright:   (c) 2005 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_TREEBOOK

#include "wx/treebook.h"

#ifndef WX_PRECOMP
    #include "wx/settings.h"
#endif

#include "wx/imaglist.h"

// ----------------------------------------------------------------------------
// various wxWidgets macros
// ----------------------------------------------------------------------------

// check that the page index is valid
#define IS_VALID_PAGE(nPage) ((nPage) < DoInternalGetPageCount())

// ----------------------------------------------------------------------------
// event table
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxTreebook, wxBookCtrlBase)
IMPLEMENT_DYNAMIC_CLASS(wxTreebookEvent, wxNotifyEvent)

#if !WXWIN_COMPATIBILITY_EVENT_TYPES
const wxEventType wxEVT_COMMAND_TREEBOOK_PAGE_CHANGING = wxNewEventType();
const wxEventType wxEVT_COMMAND_TREEBOOK_PAGE_CHANGED = wxNewEventType();
const wxEventType wxEVT_COMMAND_TREEBOOK_NODE_COLLAPSED = wxNewEventType();
const wxEventType wxEVT_COMMAND_TREEBOOK_NODE_EXPANDED = wxNewEventType();
#endif

BEGIN_EVENT_TABLE(wxTreebook, wxBookCtrlBase)
    EVT_TREE_SEL_CHANGED   (wxID_ANY, wxTreebook::OnTreeSelectionChange)
    EVT_TREE_ITEM_EXPANDED (wxID_ANY, wxTreebook::OnTreeNodeExpandedCollapsed)
    EVT_TREE_ITEM_COLLAPSED(wxID_ANY, wxTreebook::OnTreeNodeExpandedCollapsed)

    WX_EVENT_TABLE_CONTROL_CONTAINER(wxTreebook)
END_EVENT_TABLE()

// ============================================================================
// wxTreebook implementation
// ============================================================================

WX_DELEGATE_TO_CONTROL_CONTAINER(wxTreebook, wxControl)

// ----------------------------------------------------------------------------
// wxTreebook creation
// ----------------------------------------------------------------------------

void wxTreebook::Init()
{
    m_container.SetContainerWindow(this);

    m_selection =
    m_actualSelection = wxNOT_FOUND;
}

bool
wxTreebook::Create(wxWindow *parent,
                   wxWindowID id,
                   const wxPoint& pos,
                   const wxSize& size,
                   long style,
                   const wxString& name)
{
    // Check the style flag to have either wxTBK_RIGHT or wxTBK_LEFT
    if ( (style & wxBK_ALIGN_MASK) == wxBK_DEFAULT )
    {
        style |= wxBK_LEFT;
    }
    style |= wxTAB_TRAVERSAL;

    // no border for this control, it doesn't look nice together with the tree
    style &= ~wxBORDER_MASK;
    style |= wxBORDER_NONE;

    if ( !wxControl::Create(parent, id, pos, size,
                            style, wxDefaultValidator, name) )
        return false;

#ifdef __WXMSW__
    long treeStyle = GetThemedBorderStyle();
#else
    long treeStyle = wxBORDER_SUNKEN;
#endif
    m_bookctrl = new wxTreeCtrl
                 (
                    this,
                    wxID_ANY,
                    wxDefaultPosition,
                    wxDefaultSize,
                    treeStyle|
                    wxTR_DEFAULT_STYLE |
                    wxTR_HIDE_ROOT |
                    wxTR_SINGLE
                 );
    GetTreeCtrl()->SetQuickBestSize(false); // do full size calculation
    GetTreeCtrl()->AddRoot(wxEmptyString); // label doesn't matter, it's hidden

#ifdef __WXMSW__
    // We need to add dummy size event to force possible scrollbar hiding
    wxSizeEvent evt;
    GetEventHandler()->AddPendingEvent(evt);
#endif

    return true;
}


// insert a new page just before the pagePos
bool wxTreebook::InsertPage(size_t pagePos,
                            wxWindow *page,
                            const wxString& text,
                            bool bSelect,
                            int imageId)
{
    return DoInsertPage(pagePos, page, text, bSelect, imageId);
}

bool wxTreebook::InsertSubPage(size_t pagePos,
                               wxWindow *page,
                               const wxString& text,
                               bool bSelect,
                               int imageId)
{
    return DoInsertSubPage(pagePos, page, text, bSelect, imageId);
}

bool wxTreebook::AddPage(wxWindow *page, const wxString& text, bool bSelect,
                         int imageId)
{
    return DoInsertPage(m_treeIds.GetCount(), page, text, bSelect, imageId);
}

// insertion time is linear to the number of top-pages
bool wxTreebook::AddSubPage(wxWindow *page, const wxString& text, bool bSelect, int imageId)
{
    return DoAddSubPage(page, text, bSelect, imageId);
}


bool wxTreebook::DoInsertPage(size_t pagePos,
                              wxWindow *page,
                              const wxString& text,
                              bool bSelect,
                              int imageId)
{
    wxCHECK_MSG( pagePos <= DoInternalGetPageCount(), false,
                        wxT("Invalid treebook page position") );

    if ( !wxBookCtrlBase::InsertPage(pagePos, page, text, bSelect, imageId) )
        return false;

    wxTreeCtrl *tree = GetTreeCtrl();
    wxTreeItemId newId;
    if ( pagePos == DoInternalGetPageCount() )
    {
        // append the page to the end
        wxTreeItemId rootId = tree->GetRootItem();

        newId = tree->AppendItem(rootId, text, imageId);
    }
    else // insert the new page before the given one
    {
        wxTreeItemId nodeId = m_treeIds[pagePos];

        wxTreeItemId previousId = tree->GetPrevSibling(nodeId);
        wxTreeItemId parentId = tree->GetItemParent(nodeId);

        if ( previousId.IsOk() )
        {
            // insert before the sibling - previousId
            newId = tree->InsertItem(parentId, previousId, text, imageId);
        }
        else // no prev siblings -- insert as a first child
        {
            wxASSERT_MSG( parentId.IsOk(), wxT( "Tree has no root node?" ) );

            newId = tree->PrependItem(parentId, text, imageId);
        }
    }

    if ( !newId.IsOk() )
    {
        //something wrong -> cleaning and returning with false
        (void)wxBookCtrlBase::DoRemovePage(pagePos);

        wxFAIL_MSG( wxT("Failed to insert treebook page") );
        return false;
    }

    DoInternalAddPage(pagePos, page, newId);

    DoUpdateSelection(bSelect, pagePos);

    return true;
}

bool wxTreebook::DoAddSubPage(wxWindow *page, const wxString& text, bool bSelect, int imageId)
{
    wxTreeCtrl *tree = GetTreeCtrl();

    wxTreeItemId rootId = tree->GetRootItem();

    wxTreeItemId lastNodeId = tree->GetLastChild(rootId);

    wxCHECK_MSG( lastNodeId.IsOk(), false,
                        _T("Can't insert sub page when there are no pages") );

    // now calculate its position (should we save/update it too?)
    size_t newPos = tree->GetCount() -
                        (tree->GetChildrenCount(lastNodeId, true) + 1);

    return DoInsertSubPage(newPos, page, text, bSelect, imageId);
}

bool wxTreebook::DoInsertSubPage(size_t pagePos,
                                 wxTreebookPage *page,
                                 const wxString& text,
                                 bool bSelect,
                                 int imageId)
{
    wxTreeItemId parentId = DoInternalGetPage(pagePos);
    wxCHECK_MSG( parentId.IsOk(), false, wxT("invalid tree item") );

    wxTreeCtrl *tree = GetTreeCtrl();

    size_t newPos = pagePos + tree->GetChildrenCount(parentId, true) + 1;
    wxASSERT_MSG( newPos <= DoInternalGetPageCount(),
                    wxT("Internal error in tree insert point calculation") );

    if ( !wxBookCtrlBase::InsertPage(newPos, page, text, bSelect, imageId) )
        return false;

    wxTreeItemId newId = tree->AppendItem(parentId, text, imageId);

    if ( !newId.IsOk() )
    {
        (void)wxBookCtrlBase::DoRemovePage(newPos);

        wxFAIL_MSG( wxT("Failed to insert treebook page") );
        return false;
    }

    DoInternalAddPage(newPos, page, newId);

    DoUpdateSelection(bSelect, newPos);

    return true;
}

bool wxTreebook::DeletePage(size_t pagePos)
{
    wxCHECK_MSG( IS_VALID_PAGE(pagePos), false, wxT("Invalid tree index") );

    wxTreebookPage *oldPage = DoRemovePage(pagePos);
    if ( !oldPage )
        return false;

    delete oldPage;

    return true;
}

wxTreebookPage *wxTreebook::DoRemovePage(size_t pagePos)
{
    wxTreeItemId pageId = DoInternalGetPage(pagePos);
    wxCHECK_MSG( pageId.IsOk(), NULL, wxT("Invalid tree index") );

    wxTreebookPage * oldPage = GetPage(pagePos);
    wxTreeCtrl *tree = GetTreeCtrl();

    size_t subCount = tree->GetChildrenCount(pageId, true);
    wxASSERT_MSG ( IS_VALID_PAGE(pagePos + subCount),
                        wxT("Internal error in wxTreebook::DoRemovePage") );

    // here we are going to delete ALL the pages in the range
    // [pagePos, pagePos + subCount] -- the page and its children

    // deleting all the pages from the base class
    for ( size_t i = 0; i <= subCount; ++i )
    {
        wxTreebookPage *page = wxBookCtrlBase::DoRemovePage(pagePos);

        // don't delete the page itself though -- it will be deleted in
        // DeletePage() when we return
        if ( i )
        {
            delete page;
        }
    }

    DoInternalRemovePageRange(pagePos, subCount);

    tree->DeleteChildren( pageId );
    tree->Delete( pageId );

    return oldPage;
}

bool wxTreebook::DeleteAllPages()
{
    wxBookCtrlBase::DeleteAllPages();
    m_treeIds.Clear();
    m_selection =
    m_actualSelection = wxNOT_FOUND;

    wxTreeCtrl *tree = GetTreeCtrl();
    tree->DeleteChildren(tree->GetRootItem());

    return true;
}

void wxTreebook::DoInternalAddPage(size_t newPos,
                                   wxTreebookPage *page,
                                   wxTreeItemId pageId)
{
    wxASSERT_MSG( newPos <= m_treeIds.GetCount(), wxT("Ivalid index passed to wxTreebook::DoInternalAddPage") );

    // hide newly inserted page initially (it will be shown when selected)
    if ( page )
        page->Hide();

    if ( newPos == m_treeIds.GetCount() )
    {
        // append
        m_treeIds.Add(pageId);
    }
    else // insert
    {
        m_treeIds.Insert(pageId, newPos);

        if ( m_selection != wxNOT_FOUND && newPos <= (size_t)m_selection )
        {
            // selection has been moved one unit toward the end
            ++m_selection;
            if ( m_actualSelection != wxNOT_FOUND )
                ++m_actualSelection;
        }
        else if ( m_actualSelection != wxNOT_FOUND &&
                    newPos <= (size_t)m_actualSelection )
        {
            DoSetSelection(m_selection);
        }
    }
}

void wxTreebook::DoInternalRemovePageRange(size_t pagePos, size_t subCount)
{
    // Attention: this function is only for a situation when we delete a node
    // with all its children so pagePos is the node's index and subCount is the
    // node children count
    wxASSERT_MSG( pagePos + subCount < m_treeIds.GetCount(),
                    wxT("Ivalid page index") );

    wxTreeItemId pageId = m_treeIds[pagePos];

    m_treeIds.RemoveAt(pagePos, subCount + 1);

    if ( m_selection != wxNOT_FOUND )
    {
        if ( (size_t)m_selection > pagePos + subCount)
        {
            // selection is far after the deleted page, so just update the index and move on
            m_selection -= 1 + subCount;
            if ( m_actualSelection != wxNOT_FOUND)
            {
                m_actualSelection -= subCount + 1;
            }
        }
        else if ( (size_t)m_selection >= pagePos )
        {
            wxTreeCtrl *tree = GetTreeCtrl();

            // as selected page is going to be deleted, try to select the next
            // sibling if exists, if not then the parent
            wxTreeItemId nodeId = tree->GetNextSibling(pageId);

            m_selection = wxNOT_FOUND;
            m_actualSelection = wxNOT_FOUND;

            if ( nodeId.IsOk() )
            {
                // selecting next siblings
                tree->SelectItem(nodeId);
            }
            else // no next sibling, select the parent
            {
                wxTreeItemId parentId = tree->GetItemParent(pageId);

                if ( parentId.IsOk() && parentId != tree->GetRootItem() )
                {
                    tree->SelectItem(parentId);
                }
                else // parent is root
                {
                    // we can't select it as it's hidden
                    DoUpdateSelection(false, wxNOT_FOUND);
                }
            }
        }
        else if ( m_actualSelection != wxNOT_FOUND &&
                    (size_t)m_actualSelection >= pagePos )
        {
            // nothing to do -- selection is before the deleted node, but
            // actually shown page (the first (sub)child with page != NULL) is
            // already deleted
            m_actualSelection = m_selection;

            // send event as documented
            DoSetSelection(m_selection, SetSelection_SendEvent);
        }
        //else: nothing to do -- selection is before the deleted node
    }
    else
    {
        DoUpdateSelection(false, wxNOT_FOUND);
    }
}


void wxTreebook::DoUpdateSelection(bool bSelect, int newPos)
{
    int newSelPos;
    if ( bSelect )
    {
        newSelPos = newPos;
    }
    else if ( m_selection == wxNOT_FOUND && DoInternalGetPageCount() > 0 )
    {
        newSelPos = 0;
    }
    else
    {
        newSelPos = wxNOT_FOUND;
    }

    if ( newSelPos != wxNOT_FOUND )
    {
        SetSelection((size_t)newSelPos);
    }
}

wxTreeItemId wxTreebook::DoInternalGetPage(size_t pagePos) const
{
    if ( pagePos >= m_treeIds.GetCount() )
    {
        // invalid position but ok here, in this internal function, don't assert
        // (the caller will do it)
        return wxTreeItemId();
    }

    return m_treeIds[pagePos];
}

int wxTreebook::DoInternalFindPageById(wxTreeItemId pageId) const
{
    const size_t count = m_treeIds.GetCount();
    for ( size_t i = 0; i < count; ++i )
    {
        if ( m_treeIds[i] == pageId )
            return i;
    }

    return wxNOT_FOUND;
}

bool wxTreebook::IsNodeExpanded(size_t pagePos) const
{
    wxTreeItemId pageId = DoInternalGetPage(pagePos);

    wxCHECK_MSG( pageId.IsOk(), false, wxT("invalid tree item") );

    return GetTreeCtrl()->IsExpanded(pageId);
}

bool wxTreebook::ExpandNode(size_t pagePos, bool expand)
{
    wxTreeItemId pageId = DoInternalGetPage(pagePos);

    wxCHECK_MSG( pageId.IsOk(), false, wxT("invalid tree item") );

    if ( expand )
    {
        GetTreeCtrl()->Expand( pageId );
    }
    else // collapse
    {
        GetTreeCtrl()->Collapse( pageId );

        // rely on the events generated by wxTreeCtrl to update selection
    }

    return true;
}

int wxTreebook::GetPageParent(size_t pagePos) const
{
    wxTreeItemId nodeId = DoInternalGetPage( pagePos );
    wxCHECK_MSG( nodeId.IsOk(), wxNOT_FOUND, wxT("Invalid page index spacified!") );

    const wxTreeItemId parent = GetTreeCtrl()->GetItemParent( nodeId );

    return parent.IsOk() ? DoInternalFindPageById(parent) : wxNOT_FOUND;
}

bool wxTreebook::SetPageText(size_t n, const wxString& strText)
{
    wxTreeItemId pageId = DoInternalGetPage(n);

    wxCHECK_MSG( pageId.IsOk(), false, wxT("invalid tree item") );

    GetTreeCtrl()->SetItemText(pageId, strText);

    return true;
}

wxString wxTreebook::GetPageText(size_t n) const
{
    wxTreeItemId pageId = DoInternalGetPage(n);

    wxCHECK_MSG( pageId.IsOk(), wxString(), wxT("invalid tree item") );

    return GetTreeCtrl()->GetItemText(pageId);
}

int wxTreebook::GetPageImage(size_t n) const
{
    wxTreeItemId pageId = DoInternalGetPage(n);

    wxCHECK_MSG( pageId.IsOk(), wxNOT_FOUND, wxT("invalid tree item") );

    return GetTreeCtrl()->GetItemImage(pageId);
}

bool wxTreebook::SetPageImage(size_t n, int imageId)
{
    wxTreeItemId pageId = DoInternalGetPage(n);

    wxCHECK_MSG( pageId.IsOk(), false, wxT("invalid tree item") );

    GetTreeCtrl()->SetItemImage(pageId, imageId);

    return true;
}

wxSize wxTreebook::CalcSizeFromPage(const wxSize& sizePage) const
{
    const wxSize sizeTree = GetControllerSize();

    wxSize size = sizePage;
    size.x += sizeTree.x;

    return size;
}

int wxTreebook::GetSelection() const
{
   return m_selection;
}

int wxTreebook::DoSetSelection(size_t pagePos, int flags)
{
    wxCHECK_MSG( IS_VALID_PAGE(pagePos), wxNOT_FOUND,
                 wxT("invalid page index in wxListbook::DoSetSelection()") );
    wxASSERT_MSG( GetPageCount() == DoInternalGetPageCount(),
                  wxT("wxTreebook logic error: m_treeIds and m_pages not in sync!"));

    wxTreebookEvent event(wxEVT_COMMAND_TREEBOOK_PAGE_CHANGING, m_windowId);
    const int oldSel = m_selection;
    wxTreeCtrl *tree = GetTreeCtrl();
    bool allowed = false;

    if (flags & SetSelection_SendEvent)
    {
        event.SetEventObject(this);
        event.SetSelection(pagePos);
        event.SetOldSelection(m_selection);

        // don't send the event if the old and new pages are the same; do send it
        // otherwise and be prepared for it to be vetoed
        allowed = (int)pagePos == m_selection ||
                  !GetEventHandler()->ProcessEvent(event) ||
                  event.IsAllowed();
    }

    if ( !(flags & SetSelection_SendEvent) || allowed )
    {
        // hide the previously shown page
        wxTreebookPage * const oldPage = DoGetCurrentPage();
        if ( oldPage )
            oldPage->Hide();

        // then show the new one
        m_selection = pagePos;
        wxTreebookPage *page = wxBookCtrlBase::GetPage(m_selection);
        if ( !page )
        {
            // find the next page suitable to be shown: the first (grand)child
            // of this one with a non-NULL associated page
            wxTreeItemId childId = m_treeIds[pagePos];
            int actualPagePos = pagePos;
            while ( !page && childId.IsOk() )
            {
                wxTreeItemIdValue cookie;
                childId = tree->GetFirstChild( childId, cookie );
                if ( childId.IsOk() )
                {
                    page = wxBookCtrlBase::GetPage(++actualPagePos);
                }
            }

            m_actualSelection = page ? actualPagePos : m_selection;
        }

        if ( page )
            page->Show();

        tree->SelectItem(DoInternalGetPage(pagePos));

        if (flags & SetSelection_SendEvent)
        {
            // notify about the (now completed) page change
            event.SetEventType(wxEVT_COMMAND_TREEBOOK_PAGE_CHANGED);
            (void)GetEventHandler()->ProcessEvent(event);
        }
    }
    else if ( (flags & SetSelection_SendEvent) && !allowed) // page change vetoed
    {
        // tree selection might have already had changed
        if ( oldSel != wxNOT_FOUND )
            tree->SelectItem(DoInternalGetPage(oldSel));
    }

    return oldSel;
}

wxTreebookPage *wxTreebook::DoGetCurrentPage() const
{
    if ( m_selection == wxNOT_FOUND )
        return NULL;

    wxTreebookPage *page = wxBookCtrlBase::GetPage(m_selection);
    if ( !page && m_actualSelection != wxNOT_FOUND )
    {
        page = wxBookCtrlBase::GetPage(m_actualSelection);
    }

    return page;
}

void wxTreebook::SetImageList(wxImageList *imageList)
{
    wxBookCtrlBase::SetImageList(imageList);
    GetTreeCtrl()->SetImageList(imageList);
}

void wxTreebook::AssignImageList(wxImageList *imageList)
{
    wxBookCtrlBase::AssignImageList(imageList);
    GetTreeCtrl()->SetImageList(imageList);
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

void wxTreebook::OnTreeSelectionChange(wxTreeEvent& event)
{
    if ( event.GetEventObject() != m_bookctrl )
    {
        event.Skip();
        return;
    }

    wxTreeItemId newId = event.GetItem();

    if ( (m_selection == wxNOT_FOUND &&
                (!newId.IsOk() || newId == GetTreeCtrl()->GetRootItem())) ||
            (m_selection != wxNOT_FOUND && newId == m_treeIds[m_selection]) )
    {
        // this event can only come when we modify the tree selection ourselves
        // so we should simply ignore it
        return;
    }

    int newPos = DoInternalFindPageById(newId);

    if ( newPos != wxNOT_FOUND )
        SetSelection( newPos );
}

void wxTreebook::OnTreeNodeExpandedCollapsed(wxTreeEvent & event)
{
    if ( event.GetEventObject() != m_bookctrl )
    {
        event.Skip();
        return;
    }

    wxTreeItemId nodeId = event.GetItem();
    if ( !nodeId.IsOk() || nodeId == GetTreeCtrl()->GetRootItem() )
        return;
    int pagePos = DoInternalFindPageById(nodeId);
    wxCHECK_RET( pagePos != wxNOT_FOUND, wxT("Internal problem in wxTreebook!..") );

    wxTreebookEvent ev(GetTreeCtrl()->IsExpanded(nodeId)
            ? wxEVT_COMMAND_TREEBOOK_NODE_EXPANDED
            : wxEVT_COMMAND_TREEBOOK_NODE_COLLAPSED,
        m_windowId);

    ev.SetSelection(pagePos);
    ev.SetOldSelection(pagePos);
    ev.SetEventObject(this);

    GetEventHandler()->ProcessEvent(ev);
}

// ----------------------------------------------------------------------------
// wxTreebook geometry management
// ----------------------------------------------------------------------------

int wxTreebook::HitTest(wxPoint const & pt, long * flags) const
{
    int pagePos = wxNOT_FOUND;

    if ( flags )
        *flags = wxBK_HITTEST_NOWHERE;

    // convert from wxTreebook coorindates to wxTreeCtrl ones
    const wxTreeCtrl * const tree = GetTreeCtrl();
    const wxPoint treePt = tree->ScreenToClient(ClientToScreen(pt));

    // is it over the tree?
    if ( wxRect(tree->GetSize()).Contains(treePt) )
    {
        int flagsTree;
        wxTreeItemId id = tree->HitTest(treePt, flagsTree);

        if ( id.IsOk() && (flagsTree & wxTREE_HITTEST_ONITEM) )
        {
            pagePos = DoInternalFindPageById(id);
        }

        if ( flags )
        {
            if ( pagePos != wxNOT_FOUND )
                *flags = 0;

            if ( flagsTree & (wxTREE_HITTEST_ONITEMBUTTON |
                              wxTREE_HITTEST_ONITEMICON |
                              wxTREE_HITTEST_ONITEMSTATEICON) )
                *flags |= wxBK_HITTEST_ONICON;

            if ( flagsTree & wxTREE_HITTEST_ONITEMLABEL )
                *flags |= wxBK_HITTEST_ONLABEL;
        }
    }
    else // not over the tree
    {
        if ( flags && GetPageRect().Contains( pt ) )
            *flags |= wxBK_HITTEST_ONPAGE;
    }

    return pagePos;
}

#endif // wxUSE_TREEBOOK
