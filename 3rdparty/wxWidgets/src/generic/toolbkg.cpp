///////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/toolbkg.cpp
// Purpose:     generic implementation of wxToolbook
// Author:      Julian Smart
// Modified by:
// Created:     2006-01-29
// RCS-ID:      $Id: toolbkg.cpp 44271 2007-01-21 00:52:05Z VZ $
// Copyright:   (c) 2006 Julian Smart
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_TOOLBOOK

#ifndef WX_PRECOMP
    #include "wx/icon.h"
    #include "wx/settings.h"
    #include "wx/toolbar.h"
#endif

#include "wx/imaglist.h"
#include "wx/sysopt.h"
#include "wx/toolbook.h"

#if defined(__WXMAC__) && wxUSE_TOOLBAR && wxUSE_BMPBUTTON
#include "wx/generic/buttonbar.h"
#endif

// ----------------------------------------------------------------------------
// various wxWidgets macros
// ----------------------------------------------------------------------------

// check that the page index is valid
#define IS_VALID_PAGE(nPage) ((nPage) < GetPageCount())

// ----------------------------------------------------------------------------
// event table
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxToolbook, wxBookCtrlBase)
IMPLEMENT_DYNAMIC_CLASS(wxToolbookEvent, wxNotifyEvent)

#if !WXWIN_COMPATIBILITY_EVENT_TYPES
const wxEventType wxEVT_COMMAND_TOOLBOOK_PAGE_CHANGING = wxNewEventType();
const wxEventType wxEVT_COMMAND_TOOLBOOK_PAGE_CHANGED = wxNewEventType();
#endif

BEGIN_EVENT_TABLE(wxToolbook, wxBookCtrlBase)
    EVT_SIZE(wxToolbook::OnSize)
    EVT_TOOL_RANGE(1, 50, wxToolbook::OnToolSelected)
    EVT_IDLE(wxToolbook::OnIdle)
END_EVENT_TABLE()

// ============================================================================
// wxToolbook implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxToolbook creation
// ----------------------------------------------------------------------------

void wxToolbook::Init()
{
    m_selection = wxNOT_FOUND;
    m_needsRealizing = false;
}

bool wxToolbook::Create(wxWindow *parent,
                   wxWindowID id,
                   const wxPoint& pos,
                   const wxSize& size,
                   long style,
                   const wxString& name)
{
    if ( (style & wxBK_ALIGN_MASK) == wxBK_DEFAULT )
        style |= wxBK_TOP;

    // no border for this control
    style &= ~wxBORDER_MASK;
    style |= wxBORDER_NONE;

    if ( !wxControl::Create(parent, id, pos, size, style,
                            wxDefaultValidator, name) )
        return false;

    int orient = wxTB_HORIZONTAL;
    if ( (style & (wxBK_LEFT | wxBK_RIGHT)) != 0)
        orient = wxTB_VERTICAL;

    // TODO: make more configurable

#if defined(__WXMAC__) && wxUSE_TOOLBAR && wxUSE_BMPBUTTON
    if (style & wxBK_BUTTONBAR)
    {
        m_bookctrl = new wxButtonToolBar
                 (
                    this,
                    wxID_ANY,
                    wxDefaultPosition,
                    wxDefaultSize,
                    orient|wxTB_TEXT|wxTB_FLAT|wxNO_BORDER
                 );
    }
    else
#endif
    {
        m_bookctrl = new wxToolBar
                 (
                    this,
                    wxID_ANY,
                    wxDefaultPosition,
                    wxDefaultSize,
                    orient|wxTB_TEXT|wxTB_FLAT|wxTB_NODIVIDER|wxNO_BORDER
                 );
    }

    return true;
}

// ----------------------------------------------------------------------------
// wxToolbook geometry management
// ----------------------------------------------------------------------------

wxSize wxToolbook::GetControllerSize() const
{
    const wxSize sizeClient = GetClientSize(),
                 sizeBorder = m_bookctrl->GetSize() - m_bookctrl->GetClientSize(),
                 sizeToolBar = GetToolBar()->GetSize() + sizeBorder;

    wxSize size;

    if ( IsVertical() )
    {
        size.x = sizeClient.x;
        size.y = sizeToolBar.y;
    }
    else // left/right aligned
    {
        size.x = sizeToolBar.x;
        size.y = sizeClient.y;
    }

    return size;
}

void wxToolbook::OnSize(wxSizeEvent& event)
{
    if (m_needsRealizing)
        Realize();

    wxBookCtrlBase::OnSize(event);
}

wxSize wxToolbook::CalcSizeFromPage(const wxSize& sizePage) const
{
    // we need to add the size of the list control and the border between
    const wxSize sizeToolBar = GetControllerSize();

    wxSize size = sizePage;
    if ( IsVertical() )
    {
        size.y += sizeToolBar.y + GetInternalBorder();
    }
    else // left/right aligned
    {
        size.x += sizeToolBar.x + GetInternalBorder();
    }

    return size;
}

// ----------------------------------------------------------------------------
// accessing the pages
// ----------------------------------------------------------------------------

bool wxToolbook::SetPageText(size_t n, const wxString& strText)
{
    // Assume tool ids start from 1
    wxToolBarToolBase* tool = GetToolBar()->FindById(n + 1);
    if (tool)
    {
        tool->SetLabel(strText);
        return true;
    }
    else
        return false;
}

wxString wxToolbook::GetPageText(size_t n) const
{
    wxToolBarToolBase* tool = GetToolBar()->FindById(n + 1);
    if (tool)
        return tool->GetLabel();
    else
        return wxEmptyString;
}

int wxToolbook::GetPageImage(size_t WXUNUSED(n)) const
{
    wxFAIL_MSG( _T("wxToolbook::GetPageImage() not implemented") );

    return wxNOT_FOUND;
}

bool wxToolbook::SetPageImage(size_t n, int imageId)
{
    wxASSERT( GetImageList() != NULL );
    if (!GetImageList())
        return false;

    wxToolBarToolBase* tool = GetToolBar()->FindById(n + 1);
    if (tool)
    {
        // Find the image list index for this tool
        wxBitmap bitmap = GetImageList()->GetBitmap(imageId);
        tool->SetNormalBitmap(bitmap);
        return true;
    }
    else
        return false;
}

// ----------------------------------------------------------------------------
// image list stuff
// ----------------------------------------------------------------------------

void wxToolbook::SetImageList(wxImageList *imageList)
{
    wxBookCtrlBase::SetImageList(imageList);
}

// ----------------------------------------------------------------------------
// selection
// ----------------------------------------------------------------------------

int wxToolbook::GetSelection() const
{
    return m_selection;
}

wxBookCtrlBaseEvent* wxToolbook::CreatePageChangingEvent() const
{
    return new wxToolbookEvent(wxEVT_COMMAND_TOOLBOOK_PAGE_CHANGING, m_windowId);
}

void wxToolbook::MakeChangedEvent(wxBookCtrlBaseEvent &event)
{
    event.SetEventType(wxEVT_COMMAND_TOOLBOOK_PAGE_CHANGED);
}

void wxToolbook::UpdateSelectedPage(size_t newsel)
{
    m_selection = newsel;
    GetToolBar()->ToggleTool(newsel + 1, true);
}

// Not part of the wxBookctrl API, but must be called in OnIdle or
// by application to realize the toolbar and select the initial page.
void wxToolbook::Realize()
{
    if (m_needsRealizing)
    {
        GetToolBar()->SetToolBitmapSize(m_maxBitmapSize);

        int remap = wxSystemOptions::GetOptionInt(wxT("msw.remap"));
        wxSystemOptions::SetOption(wxT("msw.remap"), 0);
        GetToolBar()->Realize();
        wxSystemOptions::SetOption(wxT("msw.remap"), remap);
    }

    m_needsRealizing = false;

    if (m_selection == -1)
        m_selection = 0;

    if (GetPageCount() > 0)
    {
        int sel = m_selection;
        m_selection = -1;
        SetSelection(sel);
    }

    DoSize();
}

int wxToolbook::HitTest(const wxPoint& pt, long *flags) const
{
    int pagePos = wxNOT_FOUND;

    if ( flags )
        *flags = wxBK_HITTEST_NOWHERE;

    // convert from wxToolbook coordinates to wxToolBar ones
    const wxToolBarBase * const tbar = GetToolBar();
    const wxPoint tbarPt = tbar->ScreenToClient(ClientToScreen(pt));

    // is the point over the toolbar?
    if ( wxRect(tbar->GetSize()).Contains(tbarPt) )
    {
        const wxToolBarToolBase * const
            tool = tbar->FindToolForPosition(tbarPt.x, tbarPt.y);

        if ( tool )
        {
            pagePos = tbar->GetToolPos(tool->GetId());
            if ( flags )
                *flags = wxBK_HITTEST_ONICON | wxBK_HITTEST_ONLABEL;
        }
    }
    else // not over the toolbar
    {
        if ( flags && GetPageRect().Contains(pt) )
            *flags |= wxBK_HITTEST_ONPAGE;
    }

    return pagePos;
}

void wxToolbook::OnIdle(wxIdleEvent& event)
{
    if (m_needsRealizing)
        Realize();
    event.Skip();
}

// ----------------------------------------------------------------------------
// adding/removing the pages
// ----------------------------------------------------------------------------

bool wxToolbook::InsertPage(size_t n,
                       wxWindow *page,
                       const wxString& text,
                       bool bSelect,
                       int imageId)
{
    if ( !wxBookCtrlBase::InsertPage(n, page, text, bSelect, imageId) )
        return false;

    m_needsRealizing = true;

    wxASSERT(GetImageList() != NULL);

    if (!GetImageList())
        return false;

    // TODO: make sure all platforms can convert between icon and bitmap,
    // and/or test whether the image is a bitmap or an icon.
#ifdef __WXMAC__
    wxBitmap bitmap = GetImageList()->GetBitmap(imageId);
#else
    // On Windows, we can lose information by using GetBitmap, so extract icon instead
    wxIcon icon = GetImageList()->GetIcon(imageId);
    wxBitmap bitmap;
    bitmap.CopyFromIcon(icon);
#endif

    m_maxBitmapSize.x = wxMax(bitmap.GetWidth(), m_maxBitmapSize.x);
    m_maxBitmapSize.y = wxMax(bitmap.GetHeight(), m_maxBitmapSize.y);

    GetToolBar()->SetToolBitmapSize(m_maxBitmapSize);
    GetToolBar()->AddRadioTool(n + 1, text, bitmap, wxNullBitmap, text);

    if (bSelect)
    {
        GetToolBar()->ToggleTool(n, true);
        m_selection = n;
    }
    else
        page->Hide();

    InvalidateBestSize();
    return true;
}

wxWindow *wxToolbook::DoRemovePage(size_t page)
{
    const size_t page_count = GetPageCount();
    wxWindow *win = wxBookCtrlBase::DoRemovePage(page);

    if ( win )
    {
        GetToolBar()->DeleteTool(page + 1);

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
    }

    return win;
}


bool wxToolbook::DeleteAllPages()
{
    GetToolBar()->ClearTools();
    return wxBookCtrlBase::DeleteAllPages();
}

// ----------------------------------------------------------------------------
// wxToolbook events
// ----------------------------------------------------------------------------

void wxToolbook::OnToolSelected(wxCommandEvent& event)
{
    const int selNew = event.GetId() - 1;

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
        GetToolBar()->ToggleTool(m_selection, false);
    }
}

#endif // wxUSE_TOOLBOOK
