///////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/listbkg.cpp
// Purpose:     generic implementation of wxListbook
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.08.03
// RCS-ID:      $Id: listbkg.cpp 48783 2007-09-19 11:24:38Z RR $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwindows.org>
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

#if wxUSE_LISTBOOK

#include "wx/listbook.h"

#ifndef WX_PRECOMP
    #include "wx/settings.h"
#endif

#include "wx/listctrl.h"
#include "wx/statline.h"
#include "wx/imaglist.h"

// ----------------------------------------------------------------------------
// various wxWidgets macros
// ----------------------------------------------------------------------------

// check that the page index is valid
#define IS_VALID_PAGE(nPage) ((nPage) < GetPageCount())

// ----------------------------------------------------------------------------
// event table
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxListbook, wxBookCtrlBase)
IMPLEMENT_DYNAMIC_CLASS(wxListbookEvent, wxNotifyEvent)

#if !WXWIN_COMPATIBILITY_EVENT_TYPES
const wxEventType wxEVT_COMMAND_LISTBOOK_PAGE_CHANGING = wxNewEventType();
const wxEventType wxEVT_COMMAND_LISTBOOK_PAGE_CHANGED = wxNewEventType();
#endif

BEGIN_EVENT_TABLE(wxListbook, wxBookCtrlBase)
    EVT_SIZE(wxListbook::OnSize)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, wxListbook::OnListSelected)
END_EVENT_TABLE()

// ============================================================================
// wxListbook implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxListbook creation
// ----------------------------------------------------------------------------

void wxListbook::Init()
{
    m_selection = wxNOT_FOUND;
}

bool
wxListbook::Create(wxWindow *parent,
                   wxWindowID id,
                   const wxPoint& pos,
                   const wxSize& size,
                   long style,
                   const wxString& name)
{
    if ( (style & wxBK_ALIGN_MASK) == wxBK_DEFAULT )
    {
#ifdef __WXMAC__
        style |= wxBK_TOP;
#else // !__WXMAC__
        style |= wxBK_LEFT;
#endif // __WXMAC__/!__WXMAC__
    }

    // no border for this control, it doesn't look nice together with
    // wxListCtrl border
    style &= ~wxBORDER_MASK;
    style |= wxBORDER_NONE;

    if ( !wxControl::Create(parent, id, pos, size, style,
                            wxDefaultValidator, name) )
        return false;

    m_bookctrl = new wxListView
                 (
                    this,
                    wxID_ANY,
                    wxDefaultPosition,
                    wxDefaultSize,
                    wxLC_ICON | wxLC_SINGLE_SEL |
                        (IsVertical() ? wxLC_ALIGN_LEFT : wxLC_ALIGN_TOP)
                 );

#ifdef __WXMSW__
    // On XP with themes enabled the GetViewRect used in GetControllerSize() to
    // determine the space needed for the list view will incorrectly return
    // (0,0,0,0) the first time.  So send a pending event so OnSize will be
    // called again after the window is ready to go.  Technically we don't
    // need to do this on non-XP windows, but if things are already sized
    // correctly then nothing changes and so there is no harm.
    wxSizeEvent evt;
    GetEventHandler()->AddPendingEvent(evt);
#endif
    return true;
}

// ----------------------------------------------------------------------------
// wxListbook geometry management
// ----------------------------------------------------------------------------

wxSize wxListbook::GetControllerSize() const
{
    const wxSize sizeClient = GetClientSize(),
                 sizeBorder = m_bookctrl->GetSize() - m_bookctrl->GetClientSize(),
                 sizeList = GetListView()->GetViewRect().GetSize() + sizeBorder;

    wxSize size;

    if ( IsVertical() )
    {
        size.x = sizeClient.x;
        size.y = sizeList.y;
    }
    else // left/right aligned
    {
        size.x = sizeList.x;
        size.y = sizeClient.y;
    }

    return size;
}

void wxListbook::OnSize(wxSizeEvent& event)
{
    // arrange the icons before calling SetClientSize(), otherwise it wouldn't
    // account for the scrollbars the list control might need and, at least
    // under MSW, we'd finish with an ugly looking list control with both
    // vertical and horizontal scrollbar (with one of them being added because
    // the other one is not accounted for in client size computations)
    wxListView *list = GetListView();
    if (list) list->Arrange();
    wxBookCtrlBase::OnSize(event);
}

int wxListbook::HitTest(const wxPoint& pt, long *flags) const
{
    int pagePos = wxNOT_FOUND;

    if ( flags )
        *flags = wxBK_HITTEST_NOWHERE;

    // convert from listbook control coordinates to list control coordinates
    const wxListView * const list = GetListView();
    const wxPoint listPt = list->ScreenToClient(ClientToScreen(pt));

    // is the point inside list control?
    if ( wxRect(list->GetSize()).Contains(listPt) )
    {
        int flagsList;
        pagePos = list->HitTest(listPt, flagsList);

        if ( flags )
        {
            if ( pagePos != wxNOT_FOUND )
                *flags = 0;

            if ( flagsList & (wxLIST_HITTEST_ONITEMICON |
                              wxLIST_HITTEST_ONITEMSTATEICON ) )
                *flags |= wxBK_HITTEST_ONICON;

            if ( flagsList & wxLIST_HITTEST_ONITEMLABEL )
                *flags |= wxBK_HITTEST_ONLABEL;
        }
    }
    else // not over list control at all
    {
        if ( flags && GetPageRect().Contains(pt) )
            *flags |= wxBK_HITTEST_ONPAGE;
    }

    return pagePos;
}

wxSize wxListbook::CalcSizeFromPage(const wxSize& sizePage) const
{
    // we need to add the size of the list control and the border between
    const wxSize sizeList = GetControllerSize();

    wxSize size = sizePage;
    if ( IsVertical() )
    {
        size.y += sizeList.y + GetInternalBorder();
    }
    else // left/right aligned
    {
        size.x += sizeList.x + GetInternalBorder();
    }

    return size;
}


// ----------------------------------------------------------------------------
// accessing the pages
// ----------------------------------------------------------------------------

bool wxListbook::SetPageText(size_t n, const wxString& strText)
{
    GetListView()->SetItemText(n, strText);

    return true;
}

wxString wxListbook::GetPageText(size_t n) const
{
    return GetListView()->GetItemText(n);
}

int wxListbook::GetPageImage(size_t WXUNUSED(n)) const
{
    wxFAIL_MSG( _T("wxListbook::GetPageImage() not implemented") );

    return wxNOT_FOUND;
}

bool wxListbook::SetPageImage(size_t n, int imageId)
{
    return GetListView()->SetItemImage(n, imageId);
}

// ----------------------------------------------------------------------------
// image list stuff
// ----------------------------------------------------------------------------

void wxListbook::SetImageList(wxImageList *imageList)
{
    GetListView()->SetImageList(imageList, wxIMAGE_LIST_NORMAL);

    wxBookCtrlBase::SetImageList(imageList);
}

// ----------------------------------------------------------------------------
// selection
// ----------------------------------------------------------------------------

void wxListbook::UpdateSelectedPage(size_t newsel)
{
    m_selection = newsel;
    GetListView()->Select(newsel);
    GetListView()->Focus(newsel);
}

int wxListbook::GetSelection() const
{
    return m_selection;
}

wxBookCtrlBaseEvent* wxListbook::CreatePageChangingEvent() const
{
    return new wxListbookEvent(wxEVT_COMMAND_LISTBOOK_PAGE_CHANGING, m_windowId);
}

void wxListbook::MakeChangedEvent(wxBookCtrlBaseEvent &event)
{
    event.SetEventType(wxEVT_COMMAND_LISTBOOK_PAGE_CHANGED);
}


// ----------------------------------------------------------------------------
// adding/removing the pages
// ----------------------------------------------------------------------------

bool
wxListbook::InsertPage(size_t n,
                       wxWindow *page,
                       const wxString& text,
                       bool bSelect,
                       int imageId)
{
    if ( !wxBookCtrlBase::InsertPage(n, page, text, bSelect, imageId) )
        return false;

    GetListView()->InsertItem(n, text, imageId);

    // if the inserted page is before the selected one, we must update the
    // index of the selected page
    if ( int(n) <= m_selection )
    {
        // one extra page added
        m_selection++;
        GetListView()->Select(m_selection);
        GetListView()->Focus(m_selection);
    }

    // some page should be selected: either this one or the first one if there
    // is still no selection
    int selNew = -1;
    if ( bSelect )
        selNew = n;
    else if ( m_selection == -1 )
        selNew = 0;

    if ( selNew != m_selection )
        page->Hide();

    if ( selNew != -1 )
        SetSelection(selNew);

    wxSizeEvent sz(GetSize(), GetId());
    GetEventHandler()->ProcessEvent(sz);

    return true;
}

wxWindow *wxListbook::DoRemovePage(size_t page)
{
    const size_t page_count = GetPageCount();
    wxWindow *win = wxBookCtrlBase::DoRemovePage(page);

    if ( win )
    {
        GetListView()->DeleteItem(page);

        if (m_selection >= (int)page)
        {
            // force new sel valid if possible
            int sel = m_selection - 1;
            if (page_count == 1)
                sel = wxNOT_FOUND;
            else if ((page_count == 2) || (sel == -1))
                sel = 0;

            // force sel invalid if deleting current page - don't try to hide it
            m_selection = (m_selection == (int)page) ? wxNOT_FOUND : m_selection - 1;

            if ((sel != wxNOT_FOUND) && (sel != m_selection))
                SetSelection(sel);
        }

        GetListView()->Arrange();
        if (GetPageCount() == 0)
        {
            wxSizeEvent sz(GetSize(), GetId());
            ProcessEvent(sz);
        }
    }

    return win;
}


bool wxListbook::DeleteAllPages()
{
    GetListView()->DeleteAllItems();
    if (!wxBookCtrlBase::DeleteAllPages())
        return false;

    m_selection = -1;

    wxSizeEvent sz(GetSize(), GetId());
    ProcessEvent(sz);

    return true;
}

// ----------------------------------------------------------------------------
// wxListbook events
// ----------------------------------------------------------------------------

void wxListbook::OnListSelected(wxListEvent& eventList)
{
    if ( eventList.GetEventObject() != m_bookctrl )
    {
        eventList.Skip();
        return;
    }

    const int selNew = eventList.GetIndex();

    if ( selNew == m_selection )
    {
        // this event can only come from our own Select(m_selection) below
        // which we call when the page change is vetoed, so we should simply
        // ignore it
        return;
    }

    SetSelection(selNew);

    // change wasn't allowed, return to previous state
    if (m_selection != selNew)
    {
        GetListView()->Select(m_selection);
        GetListView()->Focus(m_selection);
    }
}

#endif // wxUSE_LISTBOOK
