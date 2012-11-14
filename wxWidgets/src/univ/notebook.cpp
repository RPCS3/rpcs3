/////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/notebook.cpp
// Purpose:     wxNotebook implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     01.02.01
// RCS-ID:      $Id: notebook.cpp 50855 2007-12-20 10:51:33Z JS $
// Copyright:   (c) 2001 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_NOTEBOOK

#include "wx/notebook.h"

#ifndef WX_PRECOMP
    #include "wx/dcmemory.h"
#endif

#include "wx/imaglist.h"
#include "wx/spinbutt.h"

#include "wx/univ/renderer.h"

// ----------------------------------------------------------------------------
// wxStdNotebookInputHandler: translates SPACE and ENTER keys and the left mouse
// click into button press/release actions
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStdNotebookInputHandler : public wxStdInputHandler
{
public:
    wxStdNotebookInputHandler(wxInputHandler *inphand);

    virtual bool HandleKey(wxInputConsumer *consumer,
                           const wxKeyEvent& event,
                           bool pressed);
    virtual bool HandleMouse(wxInputConsumer *consumer,
                             const wxMouseEvent& event);
    virtual bool HandleMouseMove(wxInputConsumer *consumer, const wxMouseEvent& event);
    virtual bool HandleFocus(wxInputConsumer *consumer, const wxFocusEvent& event);
    virtual bool HandleActivation(wxInputConsumer *consumer, bool activated);

protected:
    void HandleFocusChange(wxInputConsumer *consumer);
};

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#if 0
// due to unsigned type nPage is always >= 0
#define IS_VALID_PAGE(nPage) (((nPage) >= 0) && ((size_t(nPage)) < GetPageCount()))
#else
#define IS_VALID_PAGE(nPage) (((size_t)nPage) < GetPageCount())
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const size_t INVALID_PAGE = (size_t)-1;

DEFINE_EVENT_TYPE(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING)

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class wxNotebookSpinBtn : public wxSpinButton
{
public:
    wxNotebookSpinBtn(wxNotebook *nb)
        : wxSpinButton(nb, wxID_ANY,
                       wxDefaultPosition, wxDefaultSize,
                       nb->IsVertical() ? wxSP_VERTICAL : wxSP_HORIZONTAL)
    {
        m_nb = nb;
    }

protected:
    void OnSpin(wxSpinEvent& event)
    {
        m_nb->PerformAction(wxACTION_NOTEBOOK_GOTO, event.GetPosition());
    }

private:
    wxNotebook *m_nb;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxNotebookSpinBtn, wxSpinButton)
    EVT_SPIN(wxID_ANY, wxNotebookSpinBtn::OnSpin)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxNotebook, wxBookCtrlBase)
IMPLEMENT_DYNAMIC_CLASS(wxNotebookEvent, wxCommandEvent)

// ----------------------------------------------------------------------------
// wxNotebook creation
// ----------------------------------------------------------------------------

void wxNotebook::Init()
{
    m_sel = INVALID_PAGE;

    m_heightTab =
    m_widthMax = 0;

    m_firstVisible =
    m_lastVisible =
    m_lastFullyVisible = 0;

    m_offset = 0;

    m_spinbtn = NULL;
}

bool wxNotebook::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxString& name)
{
    if ( (style & wxBK_ALIGN_MASK) == wxBK_DEFAULT )
        style |= wxBK_TOP;

    if ( !wxControl::Create(parent, id, pos, size, style,
                            wxDefaultValidator, name) )
        return false;

    m_sizePad = GetRenderer()->GetTabPadding();

    SetInitialSize(size);

    CreateInputHandler(wxINP_HANDLER_NOTEBOOK);

    return true;
}

// ----------------------------------------------------------------------------
// wxNotebook page titles and images
// ----------------------------------------------------------------------------

wxString wxNotebook::GetPageText(size_t nPage) const
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), wxEmptyString, _T("invalid notebook page") );

    return m_titles[nPage];
}

bool wxNotebook::SetPageText(size_t nPage, const wxString& strText)
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), false, _T("invalid notebook page") );

    if ( strText != m_titles[nPage] )
    {
        m_accels[nPage] = FindAccelIndex(strText, &m_titles[nPage]);

        if ( FixedSizeTabs() )
        {
            // it's enough to just reresh this one
            RefreshTab(nPage);
        }
        else // var width tabs
        {
            // we need to resize the tab to fit the new string
            ResizeTab(nPage);
        }
    }

    return true;
}

int wxNotebook::GetPageImage(size_t nPage) const
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), wxNOT_FOUND, _T("invalid notebook page") );

    return m_images[nPage];
}

bool wxNotebook::SetPageImage(size_t nPage, int nImage)
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), false, _T("invalid notebook page") );

    wxCHECK_MSG( m_imageList && nImage < m_imageList->GetImageCount(), false,
                 _T("invalid image index in SetPageImage()") );

    if ( nImage != m_images[nPage] )
    {
        // if the item didn't have an icon before or, on the contrary, did have
        // it but has lost it now, its size will change - but if the icon just
        // changes, it won't
        bool tabSizeChanges = nImage == -1 || m_images[nPage] == -1;
        m_images[nPage] = nImage;

        if ( tabSizeChanges )
            RefreshAllTabs();
        else
            RefreshTab(nPage);
    }

    return true;
}

wxNotebook::~wxNotebook()
{
}

// ----------------------------------------------------------------------------
// wxNotebook page switching
// ----------------------------------------------------------------------------

int wxNotebook::DoSetSelection(size_t nPage, int flags)
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), wxNOT_FOUND, _T("invalid notebook page") );

    if ( (size_t)nPage == m_sel )
    {
        // don't do anything if there is nothing to do
        return m_sel;
    }

    if ( flags & SetSelection_SendEvent )
    {
        if ( !SendPageChangingEvent(nPage) )
        {
            // program doesn't allow the page change
            return m_sel;
        }
    }

    // we need to change m_sel first, before calling RefreshTab() below as
    // otherwise the previously selected tab wouldn't be redrawn properly under
    // wxGTK which calls Refresh() immediately and not during the next event
    // loop iteration as wxMSW does and as it should
    size_t selOld = m_sel;

    m_sel = nPage;

    if ( selOld != INVALID_PAGE )
    {
        RefreshTab(selOld, true /* this tab was selected */);

        m_pages[selOld]->Hide();
    }

    if ( m_sel != INVALID_PAGE ) // this is impossible - but test nevertheless
    {
        if ( HasSpinBtn() )
        {
            // keep it in sync
            m_spinbtn->SetValue(m_sel);
        }

        if ( m_sel < m_firstVisible )
        {
            // selection is to the left of visible part of tabs
            ScrollTo(m_sel);
        }
        else if ( m_sel > m_lastFullyVisible )
        {
            // selection is to the right of visible part of tabs
            ScrollLastTo(m_sel);
        }
        else // we already see this tab
        {
            // no need to scroll
            RefreshTab(m_sel);
        }

        m_pages[m_sel]->SetSize(GetPageRect());
        m_pages[m_sel]->Show();
    }

    if ( flags & SetSelection_SendEvent )
    {
        // event handling
        SendPageChangedEvent(selOld);
    }

    return selOld;
}

// ----------------------------------------------------------------------------
// wxNotebook pages adding/deleting
// ----------------------------------------------------------------------------

bool wxNotebook::InsertPage(size_t nPage,
                            wxNotebookPage *pPage,
                            const wxString& strText,
                            bool bSelect,
                            int imageId)
{
    size_t nPages = GetPageCount();
    wxCHECK_MSG( nPage == nPages || IS_VALID_PAGE(nPage), false,
                 _T("invalid notebook page in InsertPage()") );

    // modify the data
    m_pages.Insert(pPage, nPage);

    wxString label;
    m_accels.Insert(FindAccelIndex(strText, &label), nPage);
    m_titles.Insert(label, nPage);

    m_images.Insert(imageId, nPage);

    // cache the tab geometry here
    wxSize sizeTab = CalcTabSize(nPage);

    if ( sizeTab.y > m_heightTab )
        m_heightTab = sizeTab.y;

    if ( FixedSizeTabs() && sizeTab.x > m_widthMax )
        m_widthMax = sizeTab.x;

    m_widths.Insert(sizeTab.x, nPage);

    // spin button may appear if we didn't have it before - but even if we did,
    // its range should change, so update it unconditionally
    UpdateSpinBtn();

    // if the tab has just appeared, we have to relayout everything, otherwise
    // it's enough to just redraw the tabs
    if ( nPages == 0 )
    {
        // always select the first tab to have at least some selection
        bSelect = true;

        Relayout();
        Refresh();
    }
    else // not the first tab
    {
        RefreshAllTabs();
    }

    if ( bSelect )
    {
        SetSelection(nPage);
    }
    else // pages added to the notebook are initially hidden
    {
        pPage->Hide();
    }

    return true;
}

bool wxNotebook::DeleteAllPages()
{
    if ( !wxNotebookBase::DeleteAllPages() )
        return false;

    // clear the other arrays as well
    m_titles.Clear();
    m_images.Clear();
    m_accels.Clear();
    m_widths.Clear();

    // it is not valid any longer
    m_sel = INVALID_PAGE;

    // spin button is not needed any more
    UpdateSpinBtn();

    Relayout();

    return true;
}

wxNotebookPage *wxNotebook::DoRemovePage(size_t nPage)
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), NULL, _T("invalid notebook page") );

    wxNotebookPage *page = m_pages[nPage];
    m_pages.RemoveAt(nPage);
    m_titles.RemoveAt(nPage);
    m_accels.RemoveAt(nPage);
    m_widths.RemoveAt(nPage);
    m_images.RemoveAt(nPage);

    // the spin button might not be needed any more
    // 2002-08-12 'if' commented out by JACS on behalf
    // of Hans Van Leemputten <Hansvl@softhome.net> who
    // points out that UpdateSpinBtn should always be called,
    // to ensure m_lastVisible is up to date.
    // if ( HasSpinBtn() )
    {
        UpdateSpinBtn();
    }

    size_t count = GetPageCount();
    if ( count )
    {
        if ( m_sel == (size_t)nPage )
        {
            // avoid sending event to this page which doesn't exist in the
            // notebook any more
            m_sel = INVALID_PAGE;

            SetSelection(nPage == count ? nPage - 1 : nPage);
        }
        else if ( m_sel > (size_t)nPage )
        {
            // no need to change selection, just adjust the index
            m_sel--;
        }
    }
    else // no more tabs left
    {
        m_sel = INVALID_PAGE;
    }

    // have to refresh everything
    Relayout();

    return page;
}

// ----------------------------------------------------------------------------
// wxNotebook drawing
// ----------------------------------------------------------------------------

void wxNotebook::RefreshCurrent()
{
    if ( m_sel != INVALID_PAGE )
    {
        RefreshTab(m_sel);
    }
}

void wxNotebook::RefreshTab(int page, bool forceSelected)
{
    wxCHECK_RET( IS_VALID_PAGE(page), _T("invalid notebook page") );

    wxRect rect = GetTabRect(page);
    if ( forceSelected || ((size_t)page == m_sel) )
    {
        const wxSize indent = GetRenderer()->GetTabIndent();
        rect.Inflate(indent.x, indent.y);
    }

    RefreshRect(rect);
}

void wxNotebook::RefreshAllTabs()
{
    wxRect rect = GetAllTabsRect();
    if ( rect.width || rect.height )
    {
        RefreshRect(rect);
    }
    //else: we don't have tabs at all
}

void wxNotebook::DoDrawTab(wxDC& dc, const wxRect& rect, size_t n)
{
    wxBitmap bmp;
    if ( HasImage(n) )
    {
        int image = m_images[n];

        // Not needed now that wxGenericImageList is being
        // used for wxUniversal under MSW
#if 0 // def __WXMSW__    // FIXME
        int w, h;
        m_imageList->GetSize(n, w, h);
        bmp.Create(w, h);
        wxMemoryDC dc;
        dc.SelectObject(bmp);
        dc.SetBackground(wxBrush(GetBackgroundColour(), wxSOLID));
        m_imageList->Draw(image, dc, 0, 0, wxIMAGELIST_DRAW_NORMAL, true);
        dc.SelectObject(wxNullBitmap);
#else
        bmp = m_imageList->GetBitmap(image);
#endif
    }

    int flags = 0;
    if ( n == m_sel )
    {
        flags |= wxCONTROL_SELECTED;

        if ( IsFocused() )
            flags |= wxCONTROL_FOCUSED;
    }

    GetRenderer()->DrawTab
                   (
                     dc,
                     rect,
                     GetTabOrientation(),
                     m_titles[n],
                     bmp,
                     flags,
                     m_accels[n]
                   );
}

void wxNotebook::DoDraw(wxControlRenderer *renderer)
{
    //wxRect rectUpdate = GetUpdateClientRect(); -- unused

    wxDC& dc = renderer->GetDC();
    dc.SetFont(GetFont());
    dc.SetTextForeground(GetForegroundColour());

    // redraw the border - it's simpler to always do it instead of checking
    // whether this needs to be done
    GetRenderer()->DrawBorder(dc, wxBORDER_RAISED, GetPagePart());

    // avoid overwriting the spin button
    if ( HasSpinBtn() )
    {
        wxRect rectTabs = GetAllTabsRect();
        wxSize sizeSpinBtn = m_spinbtn->GetSize();

        if ( IsVertical() )
        {
            rectTabs.height -= sizeSpinBtn.y;

            // Allow for erasing the line under selected tab
            rectTabs.width += 2;
        }
        else
        {
            rectTabs.width -= sizeSpinBtn.x;

            // Allow for erasing the line under selected tab
            rectTabs.height += 2;
        }

        dc.SetClippingRegion(rectTabs);
    }

    wxRect rect = GetTabsPart();
    bool isVertical = IsVertical();

    wxRect rectSel;
    for ( size_t n = m_firstVisible; n < m_lastVisible; n++ )
    {
        GetTabSize(n, &rect.width, &rect.height);

        if ( n == m_sel )
        {
            // don't redraw it now as this tab has to be drawn over the other
            // ones as it takes more place and spills over to them
            rectSel = rect;
        }
        else // not selected tab
        {
            // unfortunately we can't do this because the selected tab hangs
            // over its neighbours and so we might need to refresh more tabs -
            // of course, we could still avoid rereshing some of them with more
            // complicated checks, but it doesn't seem too bad to refresh all
            // of them, I still don't see flicker, so leaving as is for now

            //if ( rectUpdate.Intersects(rect) )
            {
                DoDrawTab(dc, rect, n);
            }
            //else: doesn't need to be refreshed
        }

        // move the rect to the next tab
        if ( isVertical )
            rect.y += rect.height;
        else
            rect.x += rect.width;
    }

    // now redraw the selected tab
    if ( rectSel.width )
    {
        DoDrawTab(dc, rectSel, m_sel);
    }

    dc.DestroyClippingRegion();
}

// ----------------------------------------------------------------------------
// wxNotebook geometry
// ----------------------------------------------------------------------------

int wxNotebook::HitTest(const wxPoint& pt, long *flags) const
{
    if ( flags )
        *flags = wxBK_HITTEST_NOWHERE;

    // first check that it is in this window at all
    if ( !GetClientRect().Contains(pt) )
    {
        return -1;
    }

    wxRect rectTabs = GetAllTabsRect();

    switch ( GetTabOrientation() )
    {
        default:
            wxFAIL_MSG(_T("unknown tab orientation"));
            // fall through

        case wxTOP:
            if ( pt.y > rectTabs.GetBottom() )
                return -1;
            break;

        case wxBOTTOM:
            if ( pt.y < rectTabs.y )
                return -1;
            break;

        case wxLEFT:
            if ( pt.x > rectTabs.GetRight() )
                return -1;
            break;

        case wxRIGHT:
            if ( pt.x < rectTabs.x )
                return -1;
            break;
    }

    for ( size_t n = m_firstVisible; n < m_lastVisible; n++ )
    {
        GetTabSize(n, &rectTabs.width, &rectTabs.height);

        if ( rectTabs.Contains(pt) )
        {
            if ( flags )
            {
                // TODO: be more precise
                *flags = wxBK_HITTEST_ONITEM;
            }

            return n;
        }

        // move the rectTabs to the next tab
        if ( IsVertical() )
            rectTabs.y += rectTabs.height;
        else
            rectTabs.x += rectTabs.width;
    }

    return -1;
}

bool wxNotebook::IsVertical() const
{
    wxDirection dir = GetTabOrientation();

    return dir == wxLEFT || dir == wxRIGHT;
}

wxDirection wxNotebook::GetTabOrientation() const
{
    long style = GetWindowStyle();
    if ( style & wxBK_BOTTOM )
        return wxBOTTOM;
    else if ( style & wxBK_RIGHT )
        return wxRIGHT;
    else if ( style & wxBK_LEFT )
        return wxLEFT;

    // wxBK_TOP == 0 so we don't have to test for it
    return wxTOP;
}

wxRect wxNotebook::GetTabRect(int page) const
{
    wxRect rect;
    wxCHECK_MSG( IS_VALID_PAGE(page), rect, _T("invalid notebook page") );

    // calc the size of this tab and of the preceding ones
    wxCoord widthThis, widthBefore;
    if ( FixedSizeTabs() )
    {
        widthThis = m_widthMax;
        widthBefore = page*m_widthMax;
    }
    else
    {
        widthBefore = 0;
        for ( int n = 0; n < page; n++ )
        {
            widthBefore += m_widths[n];
        }

        widthThis = m_widths[page];
    }

    rect = GetTabsPart();
    if ( IsVertical() )
    {
        rect.y += widthBefore - m_offset;
        rect.height = widthThis;
    }
    else // horz
    {
        rect.x += widthBefore - m_offset;
        rect.width = widthThis;
    }

    return rect;
}

wxRect wxNotebook::GetAllTabsRect() const
{
    wxRect rect;

    if ( GetPageCount() )
    {
        const wxSize indent = GetRenderer()->GetTabIndent();
        wxSize size = GetClientSize();

        if ( IsVertical() )
        {
            rect.width = m_heightTab + indent.x;
            rect.x = GetTabOrientation() == wxLEFT ? 0 : size.x - rect.width;
            rect.y = 0;
            rect.height = size.y;
        }
        else // horz
        {
            rect.x = 0;
            rect.width = size.x;
            rect.height = m_heightTab + indent.y;
            rect.y = GetTabOrientation() == wxTOP ? 0 : size.y - rect.height;
        }
    }
    //else: no pages

    return rect;
}

wxRect wxNotebook::GetTabsPart() const
{
    wxRect rect = GetAllTabsRect();

    wxDirection dir = GetTabOrientation();

    const wxSize indent = GetRenderer()->GetTabIndent();
    if ( IsVertical() )
    {
        rect.y += indent.x;
        if ( dir == wxLEFT )
        {
            rect.x += indent.y;
            rect.width -= indent.y;
        }
        else // wxRIGHT
        {
            rect.width -= indent.y;
        }
    }
    else // horz
    {
        rect.x += indent.x;
        if ( dir == wxTOP )
        {
            rect.y += indent.y;
            rect.height -= indent.y;
        }
        else // wxBOTTOM
        {
            rect.height -= indent.y;
        }
    }

    return rect;
}

void wxNotebook::GetTabSize(int page, wxCoord *w, wxCoord *h) const
{
    wxCHECK_RET( w && h, _T("NULL pointer in GetTabSize") );

    if ( IsVertical() )
    {
        // width and height have inverted meaning
        wxCoord *tmp = w;
        w = h;
        h = tmp;
    }

    // height is always fixed
    *h = m_heightTab;

    // width may also be fixed and be the same for all tabs
    *w = GetTabWidth(page);
}

void wxNotebook::SetTabSize(const wxSize& sz)
{
    wxCHECK_RET( FixedSizeTabs(), _T("SetTabSize() ignored") );

    if ( IsVertical() )
    {
        m_heightTab = sz.x;
        m_widthMax = sz.y;
    }
    else // horz
    {
        m_widthMax = sz.x;
        m_heightTab = sz.y;
    }
}

wxSize wxNotebook::CalcTabSize(int page) const
{
    // NB: don't use m_widthMax, m_heightTab or m_widths here because this
    //     method is called to calculate them

    wxSize size;

    wxCHECK_MSG( IS_VALID_PAGE(page), size, _T("invalid notebook page") );

    GetTextExtent(m_titles[page], &size.x, &size.y);

    if ( HasImage(page) )
    {
        wxSize sizeImage;
        m_imageList->GetSize(m_images[page], sizeImage.x, sizeImage.y);

        size.x += sizeImage.x + 5; // FIXME: hard coded margin

        if ( sizeImage.y > size.y )
            size.y = sizeImage.y;
    }

    size.x += 2*m_sizePad.x;
    size.y += 2*m_sizePad.y;

    return size;
}

void wxNotebook::ResizeTab(int page)
{
    wxSize sizeTab = CalcTabSize(page);

    // we only need full relayout if the page size changes
    bool needsRelayout = false;

    if ( IsVertical() )
    {
        // swap coordinates
        wxCoord tmp = sizeTab.x;
        sizeTab.x = sizeTab.y;
        sizeTab.y = tmp;
    }

    if ( sizeTab.y > m_heightTab )
    {
        needsRelayout = true;

        m_heightTab = sizeTab.y;
    }

    m_widths[page] = sizeTab.x;

    if ( sizeTab.x > m_widthMax )
        m_widthMax = sizeTab.x;

    // the total of the tabs has changed too
    UpdateSpinBtn();

    if ( needsRelayout )
        Relayout();
    else
        RefreshAllTabs();
}

void wxNotebook::SetPadding(const wxSize& padding)
{
    if ( padding != m_sizePad )
    {
        m_sizePad = padding;

        Relayout();
    }
}

void wxNotebook::Relayout()
{
    if ( GetPageCount() )
    {
        RefreshAllTabs();

        UpdateSpinBtn();

        if ( m_sel != INVALID_PAGE )
        {
            // resize the currently shown page
            wxRect rectPage = GetPageRect();

            m_pages[m_sel]->SetSize(rectPage);

            // also scroll it into view if needed (note that m_lastVisible
            // was updated by the call to UpdateSpinBtn() above, this is why it
            // is needed here)
            if ( HasSpinBtn() )
            {
                if ( m_sel < m_firstVisible )
                {
                    // selection is to the left of visible part of tabs
                    ScrollTo(m_sel);
                }
                else if ( m_sel > m_lastFullyVisible )
                {
                    // selection is to the right of visible part of tabs
                    ScrollLastTo(m_sel);
                }
            }
        }
    }
    else // we have no pages
    {
        // just refresh everything
        Refresh();
    }
}

wxRect wxNotebook::GetPagePart() const
{
    wxRect rectPage = GetClientRect();

    if ( GetPageCount() )
    {
        wxRect rectTabs = GetAllTabsRect();
        wxDirection dir = GetTabOrientation();
        if ( IsVertical() )
        {
            rectPage.width -= rectTabs.width;
            if ( dir == wxLEFT )
                rectPage.x += rectTabs.width;
        }
        else // horz
        {
            rectPage.height -= rectTabs.height;
            if ( dir == wxTOP )
                rectPage.y += rectTabs.height;
        }
    }
    //else: no pages at all

    return rectPage;
}

wxRect wxNotebook::GetPageRect() const
{
    wxRect rect = GetPagePart();

    // leave space for the border
    wxRect rectBorder = GetRenderer()->GetBorderDimensions(wxBORDER_RAISED);

    // FIXME: hardcoded +2!
    rect.Inflate(-(rectBorder.x + rectBorder.width + 2),
                 -(rectBorder.y + rectBorder.height + 2));

    return rect;
}

wxSize wxNotebook::GetSizeForPage(const wxSize& size) const
{
    wxSize sizeNb = size;
    wxRect rect = GetAllTabsRect();
    if ( IsVertical() )
        sizeNb.x += rect.width;
    else
        sizeNb.y += rect.height;

    return sizeNb;
}

void wxNotebook::SetPageSize(const wxSize& size)
{
    SetClientSize(GetSizeForPage(size));
}

wxSize wxNotebook::CalcSizeFromPage(const wxSize& sizePage) const
{
    return AdjustSize(GetSizeForPage(sizePage));
}

// ----------------------------------------------------------------------------
// wxNotebook spin button
// ----------------------------------------------------------------------------

bool wxNotebook::HasSpinBtn() const
{
    return m_spinbtn && m_spinbtn->IsShown();
}

void wxNotebook::CalcLastVisibleTab()
{
    bool isVertical = IsVertical();

    wxCoord width = GetClientSize().x;

    wxRect rect = GetTabsPart();

    size_t count = GetPageCount();

    wxCoord widthLast = 0;
    size_t n;
    for ( n = m_firstVisible; n < count; n++ )
    {
        GetTabSize(n, &rect.width, &rect.height);
        if ( rect.GetRight() > width )
        {
            break;
        }

        // remember it to use below
        widthLast = rect.GetRight();

        // move the rect to the next tab
        if ( isVertical )
            rect.y += rect.height;
        else
            rect.x += rect.width;
    }

    if ( n == m_firstVisible )
    {
        // even the first tab isn't fully visible - but still pretend it is as
        // we have to show something
        m_lastFullyVisible = m_firstVisible;
    }
    else // more than one tab visible
    {
        m_lastFullyVisible = n - 1;

        // but is it really fully visible? it shouldn't overlap with the spin
        // button if it is present (again, even if the first button does
        // overlap with it, we pretend that it doesn't as there is not much
        // else we can do)
        if ( (m_lastFullyVisible > m_firstVisible) && HasSpinBtn() )
        {
            // adjust width to be the width before the spin button
            wxSize sizeSpinBtn = m_spinbtn->GetSize();
            if ( IsVertical() )
                width -= sizeSpinBtn.y;
            else
                width -= sizeSpinBtn.x;

            if ( widthLast > width )
            {
                // the last button overlaps with spin button, so take he
                // previous one
                m_lastFullyVisible--;
            }
        }
    }

    if ( n == count )
    {
        // everything is visible
        m_lastVisible = n;
    }
    else
    {
        // this tab is still (partially) visible, so m_lastVisible is the
        // next tab (remember, this is "exclusive" last)
        m_lastVisible = n + 1;

    }
}

void wxNotebook::UpdateSpinBtn()
{
    // first decide if we need a spin button
    bool allTabsShown;

    size_t count = (size_t)GetPageCount();
    if ( count == 0 )
    {
        // this case is special, get rid of it immediately: everything is
        // visible and we don't need any spin buttons
        allTabsShown = true;

        // have to reset them manually as we don't call CalcLastVisibleTab()
        m_firstVisible =
        m_lastVisible =
        m_lastFullyVisible = 0;
    }
    else
    {
        CalcLastVisibleTab();

        // if all tabs after the first visible one are shown, it doesn't yet
        // mean that all tabs are shown - so we go backwards until we arrive to
        // the beginning (then all tabs are indeed shown) or find a tab such
        // that not all tabs after it are shown
        while ( (m_lastFullyVisible == count - 1) && (m_firstVisible > 0) )
        {
            // this is equivalent to ScrollTo(m_firstVisible - 1) but more
            // efficient
            m_offset -= GetTabWidth(m_firstVisible--);

            // reclaculate after m_firstVisible change
            CalcLastVisibleTab();
        }

        allTabsShown = m_lastFullyVisible == count - 1;
    }

    if ( !allTabsShown )
    {
        if ( !m_spinbtn )
        {
            // create it once only
            m_spinbtn = new wxNotebookSpinBtn(this);

            // set the correct value to keep it in sync
            m_spinbtn->SetValue(m_sel);
        }

        // position it correctly
        PositionSpinBtn();

        // and show it
        m_spinbtn->Show();

        // also set/update the range
        m_spinbtn->SetRange(0, count - 1);

        // update m_lastFullyVisible once again as it might have changed
        // because the spin button appeared
        //
        // FIXME: might do it more efficiently
        CalcLastVisibleTab();
    }
    else // all tabs are visible, we don't need spin button
    {
        if ( m_spinbtn && m_spinbtn -> IsShown() )
        {
            m_spinbtn->Hide();
        }
    }
}

void wxNotebook::PositionSpinBtn()
{
    if ( !m_spinbtn )
        return;

    wxCoord wBtn, hBtn;
    m_spinbtn->GetSize(&wBtn, &hBtn);

    wxRect rectTabs = GetAllTabsRect();

    wxCoord x, y;
    switch ( GetTabOrientation() )
    {
        default:
            wxFAIL_MSG(_T("unknown tab orientation"));
            // fall through

        case wxTOP:
            x = rectTabs.GetRight() - wBtn;
            y = rectTabs.GetBottom() - hBtn;
            break;

        case wxBOTTOM:
            x = rectTabs.GetRight() - wBtn;
            y = rectTabs.GetTop();
            break;

        case wxLEFT:
            x = rectTabs.GetRight() - wBtn;
            y = rectTabs.GetBottom() - hBtn;
            break;

        case wxRIGHT:
            x = rectTabs.GetLeft();
            y = rectTabs.GetBottom() - hBtn;
            break;
    }

    m_spinbtn->Move(x, y);
}

// ----------------------------------------------------------------------------
// wxNotebook scrolling
// ----------------------------------------------------------------------------

void wxNotebook::ScrollTo(int page)
{
    wxCHECK_RET( IS_VALID_PAGE(page), _T("invalid notebook page") );

    // set the first visible tab and offset (easy)
    m_firstVisible = (size_t)page;
    m_offset = 0;
    for ( size_t n = 0; n < m_firstVisible; n++ )
    {
        m_offset += GetTabWidth(n);
    }

    // find the last visible tab too
    CalcLastVisibleTab();

    RefreshAllTabs();
}

void wxNotebook::ScrollLastTo(int page)
{
    wxCHECK_RET( IS_VALID_PAGE(page), _T("invalid notebook page") );

    // go backwards until we find the first tab which can be made visible
    // without hiding the given one
    wxSize size = GetClientSize();
    wxCoord widthAll = IsVertical() ? size.y : size.x,
            widthTabs = GetTabWidth(page);

    // the total width is less than the width of the window if we have the spin
    // button
    if ( HasSpinBtn() )
    {
        wxSize sizeSpinBtn = m_spinbtn->GetSize();
        if ( IsVertical() )
            widthAll -= sizeSpinBtn.y;
        else
            widthAll -= sizeSpinBtn.x;
    }

    m_firstVisible = page;
    while ( (m_firstVisible > 0) && (widthTabs <= widthAll) )
    {
        widthTabs += GetTabWidth(--m_firstVisible);
    }

    if ( widthTabs > widthAll )
    {
        // take one step back (that it is forward) if we can
        if ( m_firstVisible < (size_t)GetPageCount() - 1 )
            m_firstVisible++;
    }

    // go to it
    ScrollTo(m_firstVisible);

    // consitency check: the page we were asked to show should be shown
    wxASSERT_MSG( (size_t)page < m_lastVisible, _T("bug in ScrollLastTo") );
}

// ----------------------------------------------------------------------------
// wxNotebook sizing/moving
// ----------------------------------------------------------------------------

wxSize wxNotebook::DoGetBestClientSize() const
{
    // calculate the max page size
    wxSize size;

    size_t count = GetPageCount();
    if ( count )
    {
        for ( size_t n = 0; n < count; n++ )
        {
            wxSize sizePage = m_pages[n]->GetSize();

            if ( size.x < sizePage.x )
                size.x = sizePage.x;
            if ( size.y < sizePage.y )
                size.y = sizePage.y;
        }
    }
    else // no pages
    {
        // use some arbitrary default size
        size.x =
        size.y = 100;
    }

    return GetSizeForPage(size);
}

void wxNotebook::DoMoveWindow(int x, int y, int width, int height)
{
    wxControl::DoMoveWindow(x, y, width, height);

    // move the spin ctrl too (NOP if it doesn't exist)
    PositionSpinBtn();
}

void wxNotebook::DoSetSize(int x, int y,
                           int width, int height,
                           int sizeFlags)
{
    wxSize old_client_size = GetClientSize();

    wxControl::DoSetSize(x, y, width, height, sizeFlags);

    wxSize new_client_size = GetClientSize();

    if (old_client_size != new_client_size)
        Relayout();
}

// ----------------------------------------------------------------------------
// wxNotebook input processing
// ----------------------------------------------------------------------------

bool wxNotebook::PerformAction(const wxControlAction& action,
                               long numArg,
                               const wxString& strArg)
{
    if ( action == wxACTION_NOTEBOOK_NEXT )
        SetSelection(GetNextPage(true));
    else if ( action == wxACTION_NOTEBOOK_PREV )
        SetSelection(GetNextPage(false));
    else if ( action == wxACTION_NOTEBOOK_GOTO )
        SetSelection((int)numArg);
    else
        return wxControl::PerformAction(action, numArg, strArg);

    return true;
}

/* static */
wxInputHandler *wxNotebook::GetStdInputHandler(wxInputHandler *handlerDef)
{
    static wxStdNotebookInputHandler s_handler(handlerDef);

    return &s_handler;
}

// ----------------------------------------------------------------------------
// wxStdNotebookInputHandler
// ----------------------------------------------------------------------------

wxStdNotebookInputHandler::wxStdNotebookInputHandler(wxInputHandler *inphand)
                         : wxStdInputHandler(inphand)
{
}

bool wxStdNotebookInputHandler::HandleKey(wxInputConsumer *consumer,
                                          const wxKeyEvent& event,
                                          bool pressed)
{
    // ignore the key releases
    if ( pressed )
    {
        wxNotebook *notebook = wxStaticCast(consumer->GetInputWindow(), wxNotebook);

        int page = 0;
        wxControlAction action;
        switch ( event.GetKeyCode() )
        {
            case WXK_UP:
                if ( notebook->IsVertical() )
                    action = wxACTION_NOTEBOOK_PREV;
                break;

            case WXK_LEFT:
                if ( !notebook->IsVertical() )
                    action = wxACTION_NOTEBOOK_PREV;
                break;

            case WXK_DOWN:
                if ( notebook->IsVertical() )
                    action = wxACTION_NOTEBOOK_NEXT;
                break;

            case WXK_RIGHT:
                if ( !notebook->IsVertical() )
                    action = wxACTION_NOTEBOOK_NEXT;
                break;

            case WXK_HOME:
                action = wxACTION_NOTEBOOK_GOTO;
                // page = 0; -- already has this value
                break;

            case WXK_END:
                action = wxACTION_NOTEBOOK_GOTO;
                page = notebook->GetPageCount() - 1;
                break;
        }

        if ( !action.IsEmpty() )
        {
            return consumer->PerformAction(action, page);
        }
    }

    return wxStdInputHandler::HandleKey(consumer, event, pressed);
}

bool wxStdNotebookInputHandler::HandleMouse(wxInputConsumer *consumer,
                                            const wxMouseEvent& event)
{
    if ( event.ButtonDown(1) )
    {
        wxNotebook *notebook = wxStaticCast(consumer->GetInputWindow(), wxNotebook);
        int page = notebook->HitTest(event.GetPosition());
        if ( page != -1 )
        {
            consumer->PerformAction(wxACTION_NOTEBOOK_GOTO, page);

            return false;
        }
    }

    return wxStdInputHandler::HandleMouse(consumer, event);
}

bool wxStdNotebookInputHandler::HandleMouseMove(wxInputConsumer *consumer,
                                                const wxMouseEvent& event)
{
    return wxStdInputHandler::HandleMouseMove(consumer, event);
}

bool
wxStdNotebookInputHandler::HandleFocus(wxInputConsumer *consumer,
                                       const wxFocusEvent& WXUNUSED(event))
{
    HandleFocusChange(consumer);

    return false;
}

bool wxStdNotebookInputHandler::HandleActivation(wxInputConsumer *consumer,
                                                 bool WXUNUSED(activated))
{
    // we react to the focus change in the same way as to the [de]activation
    HandleFocusChange(consumer);

    return false;
}

void wxStdNotebookInputHandler::HandleFocusChange(wxInputConsumer *consumer)
{
    wxNotebook *notebook = wxStaticCast(consumer->GetInputWindow(), wxNotebook);
    notebook->RefreshCurrent();
}

#endif // wxUSE_NOTEBOOK
