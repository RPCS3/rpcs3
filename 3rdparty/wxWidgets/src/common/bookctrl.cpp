///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/bookctrl.cpp
// Purpose:     wxBookCtrlBase implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.08.03
// RCS-ID:      $Id: bookctrl.cpp 53783 2008-05-27 14:15:14Z SC $
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

#if wxUSE_BOOKCTRL

#include "wx/imaglist.h"

#include "wx/bookctrl.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// event table
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxBookCtrlBase, wxControl)

BEGIN_EVENT_TABLE(wxBookCtrlBase, wxControl)
    EVT_SIZE(wxBookCtrlBase::OnSize)
#if wxUSE_HELP
    EVT_HELP(wxID_ANY, wxBookCtrlBase::OnHelp)
#endif // wxUSE_HELP
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// constructors and destructors
// ----------------------------------------------------------------------------

void wxBookCtrlBase::Init()
{
    m_bookctrl = NULL;
    m_imageList = NULL;
    m_ownsImageList = false;
    m_fitToCurrentPage = false;

#if defined(__WXWINCE__)
    m_internalBorder = 1;
#else
    m_internalBorder = 5;
#endif

    m_controlMargin = 0;
    m_controlSizer = NULL;
}

bool
wxBookCtrlBase::Create(wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name)
{
    return wxControl::Create
                     (
                        parent,
                        id,
                        pos,
                        size,
                        style,
                        wxDefaultValidator,
                        name
                     );
}

wxBookCtrlBase::~wxBookCtrlBase()
{
    if ( m_ownsImageList )
    {
        // may be NULL, ok
        delete m_imageList;
    }
}

// ----------------------------------------------------------------------------
// image list
// ----------------------------------------------------------------------------

void wxBookCtrlBase::SetImageList(wxImageList *imageList)
{
    if ( m_ownsImageList )
    {
        // may be NULL, ok
        delete m_imageList;

        m_ownsImageList = false;
    }

    m_imageList = imageList;
}

void wxBookCtrlBase::AssignImageList(wxImageList* imageList)
{
    SetImageList(imageList);

    m_ownsImageList = true;
}

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

void wxBookCtrlBase::DoInvalidateBestSize()
{
    // notice that it is not necessary to invalidate our own best size
    // explicitly if we have m_bookctrl as it will already invalidate the best
    // size of its parent when its own size is invalidated and its parent is
    // this control
    if ( m_bookctrl )
        m_bookctrl->InvalidateBestSize();
    else
        wxControl::InvalidateBestSize();
}

void wxBookCtrlBase::SetPageSize(const wxSize& size)
{
    SetClientSize(CalcSizeFromPage(size));
}

wxSize wxBookCtrlBase::DoGetBestSize() const
{
    wxSize bestSize;

    // iterate over all pages, get the largest width and height
    const size_t nCount = m_pages.size();
    for ( size_t nPage = 0; nPage < nCount; nPage++ )
    {
        const wxWindow * const pPage = m_pages[nPage];
        if( pPage )
        {
            wxSize childBestSize(pPage->GetBestSize());

            if ( childBestSize.x > bestSize.x )
                bestSize.x = childBestSize.x;

            if ( childBestSize.y > bestSize.y )
                bestSize.y = childBestSize.y;
        }
    }

    if (m_fitToCurrentPage && GetCurrentPage())
        bestSize = GetCurrentPage()->GetBestSize();

    // convert display area to window area, adding the size necessary for the
    // tabs
    wxSize best = CalcSizeFromPage(bestSize);
    CacheBestSize(best);
    return best;
}

wxRect wxBookCtrlBase::GetPageRect() const
{
    const wxSize size = GetControllerSize();

    wxPoint pt;
    wxRect rectPage(pt, GetClientSize());

    switch ( GetWindowStyle() & wxBK_ALIGN_MASK )
    {
        default:
            wxFAIL_MSG( _T("unexpected alignment") );
            // fall through

        case wxBK_TOP:
            rectPage.y = size.y + GetInternalBorder();
            // fall through

        case wxBK_BOTTOM:
            rectPage.height -= size.y + GetInternalBorder();
            if (rectPage.height < 0)
                rectPage.height = 0;
            break;

        case wxBK_LEFT:
            rectPage.x = size.x + GetInternalBorder();
            // fall through

        case wxBK_RIGHT:
            rectPage.width -= size.x + GetInternalBorder();
            if (rectPage.width < 0)
                rectPage.width = 0;
            break;
    }

    return rectPage;
}

// Lay out controls
void wxBookCtrlBase::DoSize()
{
    if ( !m_bookctrl )
    {
        // we're not fully created yet or OnSize() should be hidden by derived class
        return;
    }

    if (GetSizer())
        Layout();
    else
    {
        // resize controller and the page area to fit inside our new size
        const wxSize sizeClient( GetClientSize() ),
                    sizeBorder( m_bookctrl->GetSize() - m_bookctrl->GetClientSize() ),
                    sizeCtrl( GetControllerSize() );

        m_bookctrl->SetClientSize( sizeCtrl.x - sizeBorder.x, sizeCtrl.y - sizeBorder.y );
        // if this changes the visibility of the scrollbars the best size changes, relayout in this case
        wxSize sizeCtrl2 = GetControllerSize();
        if ( sizeCtrl != sizeCtrl2 )
        {
            wxSize sizeBorder2 = m_bookctrl->GetSize() - m_bookctrl->GetClientSize();
            m_bookctrl->SetClientSize( sizeCtrl2.x - sizeBorder2.x, sizeCtrl2.y - sizeBorder2.y );
        }

        const wxSize sizeNew = m_bookctrl->GetSize();
        wxPoint posCtrl;
        switch ( GetWindowStyle() & wxBK_ALIGN_MASK )
        {
            default:
                wxFAIL_MSG( _T("unexpected alignment") );
                // fall through

            case wxBK_TOP:
            case wxBK_LEFT:
                // posCtrl is already ok
                break;

            case wxBK_BOTTOM:
                posCtrl.y = sizeClient.y - sizeNew.y;
                break;

            case wxBK_RIGHT:
                posCtrl.x = sizeClient.x - sizeNew.x;
                break;
        }

        if ( m_bookctrl->GetPosition() != posCtrl )
            m_bookctrl->Move(posCtrl);
    }

    // resize all pages to fit the new control size
    const wxRect pageRect = GetPageRect();
    const unsigned pagesCount = m_pages.Count();
    for ( unsigned int i = 0; i < pagesCount; ++i )
    {
        wxWindow * const page = m_pages[i];
        if ( !page )
        {
            wxASSERT_MSG( AllowNullPage(),
                _T("Null page in a control that does not allow null pages?") );
            continue;
        }

        page->SetSize(pageRect);
    }
}

void wxBookCtrlBase::OnSize(wxSizeEvent& event)
{
    event.Skip();

    DoSize();
}

wxSize wxBookCtrlBase::GetControllerSize() const
{
    if(!m_bookctrl)
        return wxSize(0,0);

    const wxSize sizeClient = GetClientSize(),
                 sizeBorder = m_bookctrl->GetSize() - m_bookctrl->GetClientSize(),
                 sizeCtrl = m_bookctrl->GetBestSize() + sizeBorder;

    wxSize size;

    if ( IsVertical() )
    {
        size.x = sizeClient.x;
        size.y = sizeCtrl.y;
    }
    else // left/right aligned
    {
        size.x = sizeCtrl.x;
        size.y = sizeClient.y;
    }

    return size;
}

// ----------------------------------------------------------------------------
// miscellaneous stuff
// ----------------------------------------------------------------------------

#if wxUSE_HELP

void wxBookCtrlBase::OnHelp(wxHelpEvent& event)
{
    // determine where does this even originate from to avoid redirecting it
    // back to the page which generated it (resulting in an infinite loop)

    // notice that we have to check in the hard(er) way instead of just testing
    // if the event object == this because the book control can have other
    // subcontrols inside it (e.g. wxSpinButton in case of a notebook in wxUniv)
    wxWindow *source = wxStaticCast(event.GetEventObject(), wxWindow);
    while ( source && source != this && source->GetParent() != this )
    {
        source = source->GetParent();
    }

    if ( source && m_pages.Index(source) == wxNOT_FOUND )
    {
        // this event is for the book control itself, redirect it to the
        // corresponding page
        wxWindow *page = NULL;

        if ( event.GetOrigin() == wxHelpEvent::Origin_HelpButton )
        {
            // show help for the page under the mouse
            const int pagePos = HitTest(ScreenToClient(event.GetPosition()));

            if ( pagePos != wxNOT_FOUND)
            {
                page = GetPage((size_t)pagePos);
            }
        }
        else // event from keyboard or unknown source
        {
            // otherwise show the current page help
            page = GetCurrentPage();
        }

        if ( page )
        {
            // change event object to the page to avoid infinite recursion if
            // we get this event ourselves if the page doesn't handle it
            event.SetEventObject(page);

            if ( page->GetEventHandler()->ProcessEvent(event) )
            {
                // don't call event.Skip()
                return;
            }
        }
    }
    //else: event coming from one of our pages already

    event.Skip();
}

#endif // wxUSE_HELP

// ----------------------------------------------------------------------------
// pages management
// ----------------------------------------------------------------------------

bool
wxBookCtrlBase::InsertPage(size_t nPage,
                           wxWindow *page,
                           const wxString& WXUNUSED(text),
                           bool WXUNUSED(bSelect),
                           int WXUNUSED(imageId))
{
    wxCHECK_MSG( page || AllowNullPage(), false,
                 _T("NULL page in wxBookCtrlBase::InsertPage()") );
    wxCHECK_MSG( nPage <= m_pages.size(), false,
                 _T("invalid page index in wxBookCtrlBase::InsertPage()") );

    m_pages.Insert(page, nPage);
    if ( page )
        page->SetSize(GetPageRect());

    DoInvalidateBestSize();

    return true;
}

bool wxBookCtrlBase::DeletePage(size_t nPage)
{
    wxWindow *page = DoRemovePage(nPage);
    if ( !(page || AllowNullPage()) )
        return false;

    // delete NULL is harmless
    delete page;

    return true;
}

wxWindow *wxBookCtrlBase::DoRemovePage(size_t nPage)
{
    wxCHECK_MSG( nPage < m_pages.size(), NULL,
                 _T("invalid page index in wxBookCtrlBase::DoRemovePage()") );

    wxWindow *pageRemoved = m_pages[nPage];
    m_pages.RemoveAt(nPage);
    DoInvalidateBestSize();

    return pageRemoved;
}

int wxBookCtrlBase::GetNextPage(bool forward) const
{
    int nPage;

    int nMax = GetPageCount();
    if ( nMax-- ) // decrement it to get the last valid index
    {
        int nSel = GetSelection();

        // change selection wrapping if it becomes invalid
        nPage = forward ? nSel == nMax ? 0
                                       : nSel + 1
                        : nSel == 0 ? nMax
                                    : nSel - 1;
    }
    else // notebook is empty, no next page
    {
        nPage = wxNOT_FOUND;
    }

    return nPage;
}

int wxBookCtrlBase::DoSetSelection(size_t n, int flags)
{
    wxCHECK_MSG( n < GetPageCount(), wxNOT_FOUND,
                 wxT("invalid page index in wxBookCtrlBase::DoSetSelection()") );

    const int oldSel = GetSelection();

    if ( n != (size_t)oldSel )
    {
        wxBookCtrlBaseEvent *event = CreatePageChangingEvent();
        bool allowed = false;

        if ( flags & SetSelection_SendEvent )
        {
            event->SetSelection(n);
            event->SetOldSelection(oldSel);
            event->SetEventObject(this);

            allowed = !GetEventHandler()->ProcessEvent(*event) || event->IsAllowed();
        }

        if ( !(flags & SetSelection_SendEvent) || allowed)
        {
            if ( oldSel != wxNOT_FOUND )
                m_pages[oldSel]->Hide();

            wxWindow *page = m_pages[n];
            page->SetSize(GetPageRect());
            page->Show();

            // change selection now to ignore the selection change event
            UpdateSelectedPage(n);

            if ( flags & SetSelection_SendEvent )
            {
                // program allows the page change
                MakeChangedEvent(*event);
                (void)GetEventHandler()->ProcessEvent(*event);
            }
        }

        delete event;
    }

    return oldSel;
}


#endif // wxUSE_BOOKCTRL
