/////////////////////////////////////////////////////////////////////////////
// Name:        treebase.cpp
// Purpose:     Base wxTreeCtrl classes
// Author:      Julian Smart
// Created:     01/02/97
// Modified:
// Id:          $Id: treebase.cpp 51356 2008-01-24 11:23:30Z VZ $
// Copyright:   (c) 1998 Robert Roebling, Julian Smart et al
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// =============================================================================
// declarations
// =============================================================================

// -----------------------------------------------------------------------------
// headers
// -----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_TREECTRL

#include "wx/treectrl.h"
#include "wx/imaglist.h"

// ----------------------------------------------------------------------------
// events
// ----------------------------------------------------------------------------

DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_BEGIN_DRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_BEGIN_RDRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_END_LABEL_EDIT)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_DELETE_ITEM)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_GET_INFO)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_SET_INFO)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_EXPANDED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_EXPANDING)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_COLLAPSED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_COLLAPSING)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_SEL_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_SEL_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_KEY_DOWN)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_ACTIVATED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_MIDDLE_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_END_DRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_STATE_IMAGE_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_GETTOOLTIP)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_MENU)

// ----------------------------------------------------------------------------
// Tree event
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxTreeEvent, wxNotifyEvent)


wxTreeEvent::wxTreeEvent(wxEventType commandType,
                         wxTreeCtrlBase *tree,
                         const wxTreeItemId& item)
           : wxNotifyEvent(commandType, tree->GetId()),
             m_item(item)
{
    m_editCancelled = false;

    SetEventObject(tree);

    if ( item.IsOk() )
        SetClientObject(tree->GetItemData(item));
}

wxTreeEvent::wxTreeEvent(wxEventType commandType, int id)
           : wxNotifyEvent(commandType, id)
{
    m_itemOld = 0l;
    m_editCancelled = false;
}

wxTreeEvent::wxTreeEvent(const wxTreeEvent & event)
           : wxNotifyEvent(event)
{
    m_evtKey = event.m_evtKey;
    m_item = event.m_item;
    m_itemOld = event.m_itemOld;
    m_pointDrag = event.m_pointDrag;
    m_label = event.m_label;
    m_editCancelled = event.m_editCancelled;
}

// ----------------------------------------------------------------------------
// wxTreeCtrlBase
// ----------------------------------------------------------------------------

wxTreeCtrlBase::~wxTreeCtrlBase()
{
    if (m_ownsImageListNormal)
        delete m_imageListNormal;
    if (m_ownsImageListState)
        delete m_imageListState;
}

static void
wxGetBestTreeSize(const wxTreeCtrlBase* treeCtrl, wxTreeItemId id, wxSize& size)
{
    wxRect rect;

    if ( treeCtrl->GetBoundingRect(id, rect, true /* just the item */) )
    {
        // Translate to logical position so we get the full extent
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
        rect.x += treeCtrl->GetScrollPos(wxHORIZONTAL);
        rect.y += treeCtrl->GetScrollPos(wxVERTICAL);
#endif

        size.IncTo(wxSize(rect.GetRight(), rect.GetBottom()));
    }

    wxTreeItemIdValue cookie;
    for ( wxTreeItemId item = treeCtrl->GetFirstChild(id, cookie);
          item.IsOk();
          item = treeCtrl->GetNextChild(id, cookie) )
    {
        wxGetBestTreeSize(treeCtrl, item, size);
    }
}

wxSize wxTreeCtrlBase::DoGetBestSize() const
{
    wxSize size;

    // this doesn't really compute the total bounding rectangle of all items
    // but a not too bad guess of it which has the advantage of not having to
    // examine all (potentially hundreds or thousands) items in the control

    if (GetQuickBestSize())
    {
        for ( wxTreeItemId item = GetRootItem();
              item.IsOk();
              item = GetLastChild(item) )
        {
            wxRect rect;

            // last parameter is "true" to get only the dimensions of the text
            // label, we don't want to get the entire item width as it's determined
            // by the current size
            if ( GetBoundingRect(item, rect, true) )
            {
                if ( size.x < rect.x + rect.width )
                    size.x = rect.x + rect.width;
                if ( size.y < rect.y + rect.height )
                    size.y = rect.y + rect.height;
            }
        }
    }
    else // use precise, if potentially slow, size computation method
    {
        // iterate over all items recursively
        wxTreeItemId idRoot = GetRootItem();
        if ( idRoot.IsOk() )
            wxGetBestTreeSize(this, idRoot, size);
    }

    // need some minimal size even for empty tree
    if ( !size.x || !size.y )
        size = wxControl::DoGetBestSize();
    else
    {
        // Add border size
        size += GetWindowBorderSize();

        CacheBestSize(size);
    }

    return size;
}

void wxTreeCtrlBase::ExpandAll()
{
    if ( IsEmpty() )
        return;

    ExpandAllChildren(GetRootItem());
}

void wxTreeCtrlBase::ExpandAllChildren(const wxTreeItemId& item)
{
    // expand this item first, this might result in its children being added on
    // the fly
    if ( item != GetRootItem() || !HasFlag(wxTR_HIDE_ROOT) )
        Expand(item);
    //else: expanding hidden root item is unsupported and unnecessary

    // then (recursively) expand all the children
    wxTreeItemIdValue cookie;
    for ( wxTreeItemId idCurr = GetFirstChild(item, cookie);
          idCurr.IsOk();
          idCurr = GetNextChild(item, cookie) )
    {
        ExpandAllChildren(idCurr);
    }
}

void wxTreeCtrlBase::CollapseAll()
{
    if ( IsEmpty() )
        return;

    CollapseAllChildren(GetRootItem());
}

void wxTreeCtrlBase::CollapseAllChildren(const wxTreeItemId& item)
{
    // first (recursively) collapse all the children
    wxTreeItemIdValue cookie;
    for ( wxTreeItemId idCurr = GetFirstChild(item, cookie);
          idCurr.IsOk();
          idCurr = GetNextChild(item, cookie) )
    {
        CollapseAllChildren(idCurr);
    }

    // then collapse this element too
    Collapse(item);
}

bool wxTreeCtrlBase::IsEmpty() const
{
    return !GetRootItem().IsOk();
}

#endif // wxUSE_TREECTRL

