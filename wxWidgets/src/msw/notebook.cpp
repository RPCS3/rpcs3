///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/notebook.cpp
// Purpose:     implementation of wxNotebook
// Author:      Vadim Zeitlin
// Modified by:
// Created:     11.06.98
// RCS-ID:      $Id: notebook.cpp 59897 2009-03-27 22:37:22Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_NOTEBOOK

#include "wx/notebook.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/string.h"
    #include "wx/dc.h"
    #include "wx/log.h"
    #include "wx/event.h"
    #include "wx/app.h"
    #include "wx/dcclient.h"
    #include "wx/dcmemory.h"
    #include "wx/control.h"
#endif  // WX_PRECOMP

#include "wx/imaglist.h"
#include "wx/sysopt.h"

#include "wx/msw/private.h"

#include <windowsx.h>
#include "wx/msw/winundef.h"

#if wxUSE_UXTHEME
    #include "wx/msw/uxtheme.h"
#endif

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// check that the page index is valid
#define IS_VALID_PAGE(nPage) ((nPage) < GetPageCount())

// you can set USE_NOTEBOOK_ANTIFLICKER to 0 for desktop Windows versions too
// to disable code whih results in flicker-less notebook redrawing at the
// expense of some extra GDI resource consumption
#ifdef __WXWINCE__
    // notebooks are never resized under CE anyhow
    #define USE_NOTEBOOK_ANTIFLICKER    0
#else
    #define USE_NOTEBOOK_ANTIFLICKER    1
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// This is a work-around for missing defines in gcc-2.95 headers
#ifndef TCS_RIGHT
    #define TCS_RIGHT       0x0002
#endif

#ifndef TCS_VERTICAL
    #define TCS_VERTICAL    0x0080
#endif

#ifndef TCS_BOTTOM
    #define TCS_BOTTOM      TCS_RIGHT
#endif

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

#if USE_NOTEBOOK_ANTIFLICKER

// the pointer to standard spin button wnd proc
static WXFARPROC gs_wndprocNotebookSpinBtn = (WXFARPROC)NULL;

// the pointer to standard tab control wnd proc
static WXFARPROC gs_wndprocNotebook = (WXFARPROC)NULL;

LRESULT APIENTRY _EXPORT wxNotebookWndProc(HWND hwnd,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam);

#endif // USE_NOTEBOOK_ANTIFLICKER

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

static bool HasTroubleWithNonTopTabs()
{
    const int verComCtl32 = wxApp::GetComCtl32Version();

    // 600 is XP, 616 is Vista -- and both have a problem with tabs not on top
    // (but don't just test for >= 600 as Microsoft might decide to fix it in
    // later versions, who knows...)
    return verComCtl32 >= 600 && verComCtl32 <= 616;
}

// ----------------------------------------------------------------------------
// event table
// ----------------------------------------------------------------------------

#include "wx/listimpl.cpp"

WX_DEFINE_LIST( wxNotebookPageInfoList )

DEFINE_EVENT_TYPE(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING)

BEGIN_EVENT_TABLE(wxNotebook, wxControl)
    EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, wxNotebook::OnSelChange)
    EVT_SIZE(wxNotebook::OnSize)
    EVT_NAVIGATION_KEY(wxNotebook::OnNavigationKey)

#if USE_NOTEBOOK_ANTIFLICKER
    EVT_ERASE_BACKGROUND(wxNotebook::OnEraseBackground)
    EVT_PAINT(wxNotebook::OnPaint)
#endif // USE_NOTEBOOK_ANTIFLICKER
END_EVENT_TABLE()

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxNotebookStyle )

wxBEGIN_FLAGS( wxNotebookStyle )
    // new style border flags, we put them first to
    // use them for streaming out
    wxFLAGS_MEMBER(wxBORDER_SIMPLE)
    wxFLAGS_MEMBER(wxBORDER_SUNKEN)
    wxFLAGS_MEMBER(wxBORDER_DOUBLE)
    wxFLAGS_MEMBER(wxBORDER_RAISED)
    wxFLAGS_MEMBER(wxBORDER_STATIC)
    wxFLAGS_MEMBER(wxBORDER_NONE)

    // old style border flags
    wxFLAGS_MEMBER(wxSIMPLE_BORDER)
    wxFLAGS_MEMBER(wxSUNKEN_BORDER)
    wxFLAGS_MEMBER(wxDOUBLE_BORDER)
    wxFLAGS_MEMBER(wxRAISED_BORDER)
    wxFLAGS_MEMBER(wxSTATIC_BORDER)
    wxFLAGS_MEMBER(wxBORDER)

    // standard window styles
    wxFLAGS_MEMBER(wxTAB_TRAVERSAL)
    wxFLAGS_MEMBER(wxCLIP_CHILDREN)
    wxFLAGS_MEMBER(wxTRANSPARENT_WINDOW)
    wxFLAGS_MEMBER(wxWANTS_CHARS)
    wxFLAGS_MEMBER(wxFULL_REPAINT_ON_RESIZE)
    wxFLAGS_MEMBER(wxALWAYS_SHOW_SB )
    wxFLAGS_MEMBER(wxVSCROLL)
    wxFLAGS_MEMBER(wxHSCROLL)

    wxFLAGS_MEMBER(wxNB_FIXEDWIDTH)
    wxFLAGS_MEMBER(wxBK_DEFAULT)
    wxFLAGS_MEMBER(wxBK_TOP)
    wxFLAGS_MEMBER(wxBK_LEFT)
    wxFLAGS_MEMBER(wxBK_RIGHT)
    wxFLAGS_MEMBER(wxBK_BOTTOM)
    wxFLAGS_MEMBER(wxNB_NOPAGETHEME)
    wxFLAGS_MEMBER(wxNB_FLAT)

wxEND_FLAGS( wxNotebookStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxNotebook, wxBookCtrlBase,"wx/notebook.h")
IMPLEMENT_DYNAMIC_CLASS_XTI(wxNotebookPageInfo, wxObject , "wx/notebook.h" )

wxCOLLECTION_TYPE_INFO( wxNotebookPageInfo * , wxNotebookPageInfoList ) ;

template<> void wxCollectionToVariantArray( wxNotebookPageInfoList const &theList, wxxVariantArray &value)
{
    wxListCollectionToVariantArray<wxNotebookPageInfoList::compatibility_iterator>( theList , value ) ;
}

wxBEGIN_PROPERTIES_TABLE(wxNotebook)
    wxEVENT_PROPERTY( PageChanging , wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING , wxNotebookEvent )
    wxEVENT_PROPERTY( PageChanged , wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED , wxNotebookEvent )

    wxPROPERTY_COLLECTION( PageInfos , wxNotebookPageInfoList , wxNotebookPageInfo* , AddPageInfo , GetPageInfos , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_FLAGS( WindowStyle , wxNotebookStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxNotebook)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_5( wxNotebook , wxWindow* , Parent , wxWindowID , Id , wxPoint , Position , wxSize , Size , long , WindowStyle)


wxBEGIN_PROPERTIES_TABLE(wxNotebookPageInfo)
    wxREADONLY_PROPERTY( Page , wxNotebookPage* , GetPage , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxREADONLY_PROPERTY( Text , wxString , GetText , wxString() , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxREADONLY_PROPERTY( Selected , bool , GetSelected , false, 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxREADONLY_PROPERTY( ImageId , int , GetImageId , -1 , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxNotebookPageInfo)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_4( wxNotebookPageInfo , wxNotebookPage* , Page , wxString , Text , bool , Selected , int , ImageId )

#else
IMPLEMENT_DYNAMIC_CLASS(wxNotebook, wxBookCtrlBase)
IMPLEMENT_DYNAMIC_CLASS(wxNotebookPageInfo, wxObject )
#endif
IMPLEMENT_DYNAMIC_CLASS(wxNotebookEvent, wxNotifyEvent)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxNotebook construction
// ----------------------------------------------------------------------------

const wxNotebookPageInfoList& wxNotebook::GetPageInfos() const
{
    wxNotebookPageInfoList* list = const_cast< wxNotebookPageInfoList* >( &m_pageInfos ) ;
    WX_CLEAR_LIST( wxNotebookPageInfoList , *list ) ;
    for( size_t i = 0 ; i < GetPageCount() ; ++i )
    {
        wxNotebookPageInfo *info = new wxNotebookPageInfo() ;
        info->Create( const_cast<wxNotebook*>(this)->GetPage(i) , GetPageText(i) , GetSelection() == int(i) , GetPageImage(i) ) ;
        list->Append( info ) ;
    }
    return m_pageInfos ;
}

// common part of all ctors
void wxNotebook::Init()
{
    m_imageList = NULL;
    m_nSelection = wxNOT_FOUND;

#if wxUSE_UXTHEME
    m_hbrBackground = NULL;
#endif // wxUSE_UXTHEME

#if USE_NOTEBOOK_ANTIFLICKER
    m_hasSubclassedUpdown = false;
#endif // USE_NOTEBOOK_ANTIFLICKER
}

// default for dynamic class
wxNotebook::wxNotebook()
{
  Init();
}

// the same arguments as for wxControl
wxNotebook::wxNotebook(wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name)
{
  Init();

  Create(parent, id, pos, size, style, name);
}

// Create() function
bool wxNotebook::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxString& name)
{
    if ( (style & wxBK_ALIGN_MASK) == wxBK_DEFAULT )
    {
#if defined(__POCKETPC__)
        style |= wxBK_BOTTOM | wxNB_FLAT;
#else
        style |= wxBK_TOP;
#endif
    }

#ifdef __WXWINCE__
    // Not sure why, but without this style, there is no border
    // around the notebook tabs.
    if (style & wxNB_FLAT)
        style |= wxBORDER_SUNKEN;
#endif

#if !wxUSE_UXTHEME
    // ComCtl32 notebook tabs simply don't work unless they're on top if we
    // have uxtheme, we can work around it later (after control creation), but
    // if we have been compiled without uxtheme support, we have to clear those
    // styles
    if ( HasTroubleWithNonTopTabs() )
    {
        style &= ~(wxBK_BOTTOM | wxBK_LEFT | wxBK_RIGHT);
    }
#endif //wxUSE_UXTHEME

#if defined(__WINE__) && wxUSE_UNICODE
    LPCTSTR className = L"SysTabControl32";
#else
    LPCTSTR className = WC_TABCONTROL;
#endif

#if USE_NOTEBOOK_ANTIFLICKER
    // SysTabCtl32 class has natively CS_HREDRAW and CS_VREDRAW enabled and it
    // causes horrible flicker when resizing notebook, so get rid of it by
    // using a class without these styles (but otherwise identical to it)
    if ( !HasFlag(wxFULL_REPAINT_ON_RESIZE) )
    {
        static ClassRegistrar s_clsNotebook;
        if ( !s_clsNotebook.IsInitialized() )
        {
            // get a copy of standard class and modify it
            WNDCLASS wc;

            if ( ::GetClassInfo(NULL, WC_TABCONTROL, &wc) )
            {
                gs_wndprocNotebook =
                    wx_reinterpret_cast(WXFARPROC, wc.lpfnWndProc);
                wc.lpszClassName = wxT("_wx_SysTabCtl32");
                wc.style &= ~(CS_HREDRAW | CS_VREDRAW);
                wc.hInstance = wxGetInstance();
                wc.lpfnWndProc = wxNotebookWndProc;
                s_clsNotebook.Register(wc);
            }
            else
            {
                wxLogLastError(_T("GetClassInfoEx(SysTabCtl32)"));
            }
        }

        // use our custom class if available but fall back to the standard
        // notebook if we failed to register it
        if ( s_clsNotebook.IsRegistered() )
        {
            // it's ok to use c_str() here as the static s_clsNotebook object
            // has sufficiently long lifetime
            className = s_clsNotebook.GetName().c_str();
        }
    }
#endif // USE_NOTEBOOK_ANTIFLICKER

    if ( !CreateControl(parent, id, pos, size, style | wxTAB_TRAVERSAL,
                        wxDefaultValidator, name) )
        return false;

    if ( !MSWCreateControl(className, wxEmptyString, pos, size) )
        return false;

#if wxUSE_UXTHEME
    if ( HasFlag(wxNB_NOPAGETHEME) ||
            wxSystemOptions::IsFalse(wxT("msw.notebook.themed-background")) )
    {
        SetBackgroundColour(GetThemeBackgroundColour());
    }
    else // use themed background by default
    {
        // create backing store
        UpdateBgBrush();
    }

    // comctl32.dll 6.0 doesn't support non-top tabs with visual styles (the
    // control is simply not rendered correctly), so we disable themes
    // if possible, otherwise we simply clear the styles.
    if ( HasTroubleWithNonTopTabs() &&
            (style & (wxBK_BOTTOM | wxBK_LEFT | wxBK_RIGHT)) )
    {
        // check if we use themes at all -- if we don't, we're still okay
        if ( wxUxThemeEngine::GetIfActive() )
        {
            wxUxThemeEngine::GetIfActive()->SetWindowTheme(GetHwnd(), L"", L"");

            // correct the background color for the new non-themed control
            SetBackgroundColour(GetThemeBackgroundColour());
        }
    }
#endif // wxUSE_UXTHEME

    // Undocumented hack to get flat notebook style
    // In fact, we should probably only do this in some
    // curcumstances, i.e. if we know we will have a border
    // at the bottom (the tab control doesn't draw it itself)
#if defined(__POCKETPC__) || defined(__SMARTPHONE__)
    if (HasFlag(wxNB_FLAT))
    {
        SendMessage(GetHwnd(), CCM_SETVERSION, COMCTL32_VERSION, 0);
        if (!m_hasBgCol)
            SetBackgroundColour(*wxWHITE);
    }
#endif
    return true;
}

WXDWORD wxNotebook::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    WXDWORD tabStyle = wxControl::MSWGetStyle(style, exstyle);

    tabStyle |= WS_TABSTOP | TCS_TABS;

    if ( style & wxNB_MULTILINE )
        tabStyle |= TCS_MULTILINE;
    if ( style & wxNB_FIXEDWIDTH )
        tabStyle |= TCS_FIXEDWIDTH;

    if ( style & wxBK_BOTTOM )
        tabStyle |= TCS_RIGHT;
    else if ( style & wxBK_LEFT )
        tabStyle |= TCS_VERTICAL;
    else if ( style & wxBK_RIGHT )
        tabStyle |= TCS_VERTICAL | TCS_RIGHT;

    if ( exstyle )
    {
        // note that we never want to have the default WS_EX_CLIENTEDGE style
        // as it looks too ugly for the notebooks
        *exstyle &= ~WS_EX_CLIENTEDGE;
    }

    return tabStyle;
}

wxNotebook::~wxNotebook()
{
#if wxUSE_UXTHEME
    if ( m_hbrBackground )
        ::DeleteObject((HBRUSH)m_hbrBackground);
#endif // wxUSE_UXTHEME
}

// ----------------------------------------------------------------------------
// wxNotebook accessors
// ----------------------------------------------------------------------------

size_t wxNotebook::GetPageCount() const
{
    // consistency check
    wxASSERT( (int)m_pages.Count() == TabCtrl_GetItemCount(GetHwnd()) );

    return m_pages.Count();
}

int wxNotebook::GetRowCount() const
{
    return TabCtrl_GetRowCount(GetHwnd());
}

int wxNotebook::SetSelection(size_t nPage)
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), wxNOT_FOUND, wxT("notebook page out of range") );

    if ( m_nSelection == wxNOT_FOUND || nPage != (size_t)m_nSelection )
    {
        if ( SendPageChangingEvent(nPage) )
        {
            // program allows the page change
            SendPageChangedEvent(m_nSelection, nPage);

            TabCtrl_SetCurSel(GetHwnd(), nPage);
        }
    }

    return m_nSelection;
}

void wxNotebook::UpdateSelection(int selNew)
{
    if ( m_nSelection != wxNOT_FOUND )
        m_pages[m_nSelection]->Show(false);

    if ( selNew != wxNOT_FOUND )
    {
        wxNotebookPage *pPage = m_pages[selNew];
        pPage->Show(true);
    }

    // Changing the page should give the focus to it but, as per bug report
    // http://sf.net/tracker/index.php?func=detail&aid=1150659&group_id=9863&atid=109863,
    // we should not set the focus to it directly since it erroneously
    // selects radio buttons and breaks keyboard handling for a notebook's
    // scroll buttons. So give focus to the notebook and not the page.

    // but don't do this is the notebook is hidden
    if ( ::IsWindowVisible(GetHwnd()) )
        SetFocus();

    m_nSelection = selNew;
}

int wxNotebook::ChangeSelection(size_t nPage)
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), wxNOT_FOUND, wxT("notebook page out of range") );

    if ( m_nSelection == wxNOT_FOUND || nPage != (size_t)m_nSelection )
    {
        TabCtrl_SetCurSel(GetHwnd(), nPage);

        UpdateSelection(nPage);
    }

    return m_nSelection;
}

bool wxNotebook::SetPageText(size_t nPage, const wxString& strText)
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), false, wxT("notebook page out of range") );

    TC_ITEM tcItem;
    tcItem.mask = TCIF_TEXT;
    tcItem.pszText = (wxChar *)strText.c_str();

    if ( !HasFlag(wxNB_MULTILINE) )
        return TabCtrl_SetItem(GetHwnd(), nPage, &tcItem) != 0;

    // multiline - we need to set new page size if a line is added or removed
    int rows = GetRowCount();
    bool ret = TabCtrl_SetItem(GetHwnd(), nPage, &tcItem) != 0;

    if ( ret && rows != GetRowCount() )
    {
        const wxRect r = GetPageSize();
        const size_t count = m_pages.Count();
        for ( size_t page = 0; page < count; page++ )
            m_pages[page]->SetSize(r);
    }

    return ret;
}

wxString wxNotebook::GetPageText(size_t nPage) const
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), wxEmptyString, wxT("notebook page out of range") );

    wxChar buf[256];
    TC_ITEM tcItem;
    tcItem.mask = TCIF_TEXT;
    tcItem.pszText = buf;
    tcItem.cchTextMax = WXSIZEOF(buf);

    wxString str;
    if ( TabCtrl_GetItem(GetHwnd(), nPage, &tcItem) )
        str = tcItem.pszText;

    return str;
}

int wxNotebook::GetPageImage(size_t nPage) const
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), wxNOT_FOUND, wxT("notebook page out of range") );

    TC_ITEM tcItem;
    tcItem.mask = TCIF_IMAGE;

    return TabCtrl_GetItem(GetHwnd(), nPage, &tcItem) ? tcItem.iImage
                                                      : wxNOT_FOUND;
}

bool wxNotebook::SetPageImage(size_t nPage, int nImage)
{
    wxCHECK_MSG( IS_VALID_PAGE(nPage), false, wxT("notebook page out of range") );

    TC_ITEM tcItem;
    tcItem.mask = TCIF_IMAGE;
    tcItem.iImage = nImage;

    return TabCtrl_SetItem(GetHwnd(), nPage, &tcItem) != 0;
}

void wxNotebook::SetImageList(wxImageList* imageList)
{
    wxNotebookBase::SetImageList(imageList);

    if ( imageList )
    {
        (void) TabCtrl_SetImageList(GetHwnd(), GetHimagelistOf(imageList));
    }
}

// ----------------------------------------------------------------------------
// wxNotebook size settings
// ----------------------------------------------------------------------------

wxRect wxNotebook::GetPageSize() const
{
    wxRect r;

    RECT rc;
    ::GetClientRect(GetHwnd(), &rc);

    // This check is to work around a bug in TabCtrl_AdjustRect which will
    // cause a crash on win2k or on XP with themes disabled if either
    // wxNB_MULTILINE is used or tabs are placed on a side, if the rectangle
    // is too small.
    //
    // The value of 20 is chosen arbitrarily but seems to work
    if ( rc.right > 20 && rc.bottom > 20 )
    {
        TabCtrl_AdjustRect(GetHwnd(), false, &rc);

        wxCopyRECTToRect(rc, r);
    }

    return r;
}

void wxNotebook::SetPageSize(const wxSize& size)
{
    // transform the page size into the notebook size
    RECT rc;
    rc.left =
    rc.top = 0;
    rc.right = size.x;
    rc.bottom = size.y;

    TabCtrl_AdjustRect(GetHwnd(), true, &rc);

    // and now set it
    SetSize(rc.right - rc.left, rc.bottom - rc.top);
}

void wxNotebook::SetPadding(const wxSize& padding)
{
    TabCtrl_SetPadding(GetHwnd(), padding.x, padding.y);
}

// Windows-only at present. Also, you must use the wxNB_FIXEDWIDTH
// style.
void wxNotebook::SetTabSize(const wxSize& sz)
{
    ::SendMessage(GetHwnd(), TCM_SETITEMSIZE, 0, MAKELPARAM(sz.x, sz.y));
}

wxSize wxNotebook::CalcSizeFromPage(const wxSize& sizePage) const
{
    // we can't use TabCtrl_AdjustRect here because it only works for wxNB_TOP
    wxSize sizeTotal = sizePage;

    wxSize tabSize;
    if ( GetPageCount() > 0 )
    {
        RECT rect;
        TabCtrl_GetItemRect(GetHwnd(), 0, &rect);
        tabSize.x = rect.right - rect.left;
        tabSize.y = rect.bottom - rect.top;
    }

    const int rows = GetRowCount();

    // add an extra margin in both directions
    const int MARGIN = 8;
    if ( IsVertical() )
    {
        sizeTotal.x += MARGIN;
        sizeTotal.y += tabSize.y * rows + MARGIN;
    }
    else // horizontal layout
    {
        sizeTotal.x += tabSize.x * rows + MARGIN;
        sizeTotal.y += MARGIN;
    }

    return sizeTotal;
}

void wxNotebook::AdjustPageSize(wxNotebookPage *page)
{
    wxCHECK_RET( page, _T("NULL page in wxNotebook::AdjustPageSize") );

    const wxRect r = GetPageSize();
    if ( !r.IsEmpty() )
    {
        page->SetSize(r);
    }
}

// ----------------------------------------------------------------------------
// wxNotebook operations
// ----------------------------------------------------------------------------

// remove one page from the notebook, without deleting
wxNotebookPage *wxNotebook::DoRemovePage(size_t nPage)
{
    wxNotebookPage *pageRemoved = wxNotebookBase::DoRemovePage(nPage);
    if ( !pageRemoved )
        return NULL;

    TabCtrl_DeleteItem(GetHwnd(), nPage);

    if ( m_pages.IsEmpty() )
    {
        // no selection any more, the notebook becamse empty
        m_nSelection = wxNOT_FOUND;
    }
    else // notebook still not empty
    {
        int selNew = TabCtrl_GetCurSel(GetHwnd());
        if ( selNew != wxNOT_FOUND )
        {
            // No selection change, just refresh the current selection.
            // Because it could be that the slection index changed
            // we need to update it.
            // Note: this does not mean the selection it self changed.
            m_nSelection = selNew;
            m_pages[m_nSelection]->Refresh();
        }
        else if (int(nPage) == m_nSelection)
        {
            // The selection was deleted.

            // Determine new selection.
            if (m_nSelection == int(GetPageCount()))
                selNew = m_nSelection - 1;
            else
                selNew = m_nSelection;

            // m_nSelection must be always valid so reset it before calling
            // SetSelection()
            m_nSelection = wxNOT_FOUND;
            SetSelection(selNew);
        }
        else
        {
            wxFAIL; // Windows did not behave ok.
        }
    }

    return pageRemoved;
}

// remove all pages
bool wxNotebook::DeleteAllPages()
{
    size_t nPageCount = GetPageCount();
    size_t nPage;
    for ( nPage = 0; nPage < nPageCount; nPage++ )
        delete m_pages[nPage];

    m_pages.Clear();

    TabCtrl_DeleteAllItems(GetHwnd());

    m_nSelection = wxNOT_FOUND;

    InvalidateBestSize();
    return true;
}

// same as AddPage() but does it at given position
bool wxNotebook::InsertPage(size_t nPage,
                            wxNotebookPage *pPage,
                            const wxString& strText,
                            bool bSelect,
                            int imageId)
{
    wxCHECK_MSG( pPage != NULL, false, _T("NULL page in wxNotebook::InsertPage") );
    wxCHECK_MSG( IS_VALID_PAGE(nPage) || nPage == GetPageCount(), false,
                 _T("invalid index in wxNotebook::InsertPage") );

    wxASSERT_MSG( pPage->GetParent() == this,
                    _T("notebook pages must have notebook as parent") );

    // add a new tab to the control
    // ----------------------------

    // init all fields to 0
    TC_ITEM tcItem;
    wxZeroMemory(tcItem);

    // set the image, if any
    if ( imageId != -1 )
    {
        tcItem.mask |= TCIF_IMAGE;
        tcItem.iImage  = imageId;
    }

    // and the text
    if ( !strText.empty() )
    {
        tcItem.mask |= TCIF_TEXT;
        tcItem.pszText = (wxChar *)strText.c_str(); // const_cast
    }

    // hide the page: unless it is selected, it shouldn't be shown (and if it
    // is selected it will be shown later)
    HWND hwnd = GetWinHwnd(pPage);
    SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_VISIBLE);

    // this updates internal flag too -- otherwise it would get out of sync
    // with the real state
    pPage->Show(false);


    // fit the notebook page to the tab control's display area: this should be
    // done before adding it to the notebook or TabCtrl_InsertItem() will
    // change the notebooks size itself!
    AdjustPageSize(pPage);

    // finally do insert it
    if ( TabCtrl_InsertItem(GetHwnd(), nPage, &tcItem) == -1 )
    {
        wxLogError(wxT("Can't create the notebook page '%s'."), strText.c_str());

        return false;
    }

    // need to update the bg brush when the first page is added
    // so the first panel gets the correct themed background
    if ( m_pages.empty() )
    {
#if wxUSE_UXTHEME
        UpdateBgBrush();
#endif // wxUSE_UXTHEME
    }

    // succeeded: save the pointer to the page
    m_pages.Insert(pPage, nPage);

    // we may need to adjust the size again if the notebook size changed:
    // normally this only happens for the first page we add (the tabs which
    // hadn't been there before are now shown) but for a multiline notebook it
    // can happen for any page at all as a new row could have been started
    if ( m_pages.GetCount() == 1 || HasFlag(wxNB_MULTILINE) )
    {
        AdjustPageSize(pPage);
    }

    // now deal with the selection
    // ---------------------------

    // if the inserted page is before the selected one, we must update the
    // index of the selected page
    if ( int(nPage) <= m_nSelection )
    {
        // one extra page added
        m_nSelection++;
    }

    // some page should be selected: either this one or the first one if there
    // is still no selection
    int selNew = wxNOT_FOUND;
    if ( bSelect )
        selNew = nPage;
    else if ( m_nSelection == wxNOT_FOUND )
        selNew = 0;

    if ( selNew != wxNOT_FOUND )
        SetSelection(selNew);

    InvalidateBestSize();

    return true;
}

int wxNotebook::HitTest(const wxPoint& pt, long *flags) const
{
    TC_HITTESTINFO hitTestInfo;
    hitTestInfo.pt.x = pt.x;
    hitTestInfo.pt.y = pt.y;
    int item = TabCtrl_HitTest(GetHwnd(), &hitTestInfo);

    if ( flags )
    {
        *flags = 0;

        if ((hitTestInfo.flags & TCHT_NOWHERE) == TCHT_NOWHERE)
            *flags |= wxBK_HITTEST_NOWHERE;
        if ((hitTestInfo.flags & TCHT_ONITEM) == TCHT_ONITEM)
            *flags |= wxBK_HITTEST_ONITEM;
        if ((hitTestInfo.flags & TCHT_ONITEMICON) == TCHT_ONITEMICON)
            *flags |= wxBK_HITTEST_ONICON;
        if ((hitTestInfo.flags & TCHT_ONITEMLABEL) == TCHT_ONITEMLABEL)
            *flags |= wxBK_HITTEST_ONLABEL;
        if ( item == wxNOT_FOUND && GetPageSize().Contains(pt) )
            *flags |= wxBK_HITTEST_ONPAGE;
    }

    return item;
}

// ----------------------------------------------------------------------------
// flicker-less notebook redraw
// ----------------------------------------------------------------------------

#if USE_NOTEBOOK_ANTIFLICKER

// wnd proc for the spin button
LRESULT APIENTRY _EXPORT wxNotebookSpinBtnWndProc(HWND hwnd,
                                                  UINT message,
                                                  WPARAM wParam,
                                                  LPARAM lParam)
{
    if ( message == WM_ERASEBKGND )
        return 0;

    return ::CallWindowProc(CASTWNDPROC gs_wndprocNotebookSpinBtn,
                            hwnd, message, wParam, lParam);
}

LRESULT APIENTRY _EXPORT wxNotebookWndProc(HWND hwnd,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam)
{
    return ::CallWindowProc(CASTWNDPROC gs_wndprocNotebook,
                            hwnd, message, wParam, lParam);
}

void wxNotebook::OnEraseBackground(wxEraseEvent& WXUNUSED(event))
{
    // do nothing here
}

void wxNotebook::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    wxMemoryDC memdc;
    RECT rc;
    ::GetClientRect(GetHwnd(), &rc);
    wxBitmap bmp(rc.right, rc.bottom);
    memdc.SelectObject(bmp);

    const wxLayoutDirection dir = dc.GetLayoutDirection();
    memdc.SetLayoutDirection(dir);

    // if there is no special brush just use the solid background colour
#if wxUSE_UXTHEME
    HBRUSH hbr = (HBRUSH)m_hbrBackground;
#else
    HBRUSH hbr = 0;
#endif
    wxBrush brush;
    if ( !hbr )
    {
        brush = wxBrush(GetBackgroundColour());
        hbr = GetHbrushOf(brush);
    }

    ::FillRect(GetHdcOf(memdc), &rc, hbr);

    MSWDefWindowProc(WM_PAINT, (WPARAM)memdc.GetHDC(), 0);

    // For some reason in RTL mode, source offset has to be -1, otherwise the
    // right border (physical) remains unpainted.
    const wxCoord ofs = dir == wxLayout_RightToLeft ? -1 : 0;
    dc.Blit(ofs, 0, rc.right, rc.bottom, &memdc, ofs, 0);
}

#endif // USE_NOTEBOOK_ANTIFLICKER

// ----------------------------------------------------------------------------
// wxNotebook callbacks
// ----------------------------------------------------------------------------

void wxNotebook::OnSize(wxSizeEvent& event)
{
    if ( GetPageCount() == 0 )
    {
        // Prevents droppings on resize, but does cause some flicker
        // when there are no pages.
        Refresh();
        event.Skip();
        return;
    }
#ifndef __WXWINCE__
    else
    {
        // Without this, we can sometimes get droppings at the edges
        // of a notebook, for example a notebook in a splitter window.
        // This needs to be reconciled with the RefreshRect calls
        // at the end of this function, which weren't enough to prevent
        // the droppings.

        wxSize sz = GetClientSize();

        // Refresh right side
        wxRect rect(sz.x-4, 0, 4, sz.y);
        RefreshRect(rect);

        // Refresh bottom side
        rect = wxRect(0, sz.y-4, sz.x, 4);
        RefreshRect(rect);

        // Refresh left side
        rect = wxRect(0, 0, 4, sz.y);
        RefreshRect(rect);
    }
#endif // !__WXWINCE__

    // fit all the notebook pages to the tab control's display area

    RECT rc;
    rc.left = rc.top = 0;
    GetSize((int *)&rc.right, (int *)&rc.bottom);

    // save the total size, we'll use it below
    int widthNbook = rc.right - rc.left,
        heightNbook = rc.bottom - rc.top;

    // there seems to be a bug in the implementation of TabCtrl_AdjustRect(): it
    // returns completely false values for multiline tab controls after the tabs
    // are added but before getting the first WM_SIZE (off by ~50 pixels, see
    //
    // http://sf.net/tracker/index.php?func=detail&aid=645323&group_id=9863&atid=109863
    //
    // and the only work around I could find was this ugly hack... without it
    // simply toggling the "multiline" checkbox in the notebook sample resulted
    // in a noticeable page displacement
    if ( HasFlag(wxNB_MULTILINE) )
    {
        // avoid an infinite recursion: we get another notification too!
        static bool s_isInOnSize = false;

        if ( !s_isInOnSize )
        {
            s_isInOnSize = true;
            SendMessage(GetHwnd(), WM_SIZE, SIZE_RESTORED,
                    MAKELPARAM(rc.right, rc.bottom));
            s_isInOnSize = false;
        }

        // The best size depends on the number of rows of tabs, which can
        // change when the notepad is resized.
        InvalidateBestSize();
    }

#if wxUSE_UXTHEME
    // background bitmap size has changed, update the brush using it too
    UpdateBgBrush();
#endif // wxUSE_UXTHEME

    TabCtrl_AdjustRect(GetHwnd(), false, &rc);

    int width = rc.right - rc.left,
        height = rc.bottom - rc.top;
    size_t nCount = m_pages.Count();
    for ( size_t nPage = 0; nPage < nCount; nPage++ ) {
        wxNotebookPage *pPage = m_pages[nPage];
        pPage->SetSize(rc.left, rc.top, width, height);
    }


    // unless we had already repainted everything, we now need to refresh
    if ( !HasFlag(wxFULL_REPAINT_ON_RESIZE) )
    {
        // invalidate areas not covered by pages
        RefreshRect(wxRect(0, 0, widthNbook, rc.top), false);
        RefreshRect(wxRect(0, rc.top, rc.left, height), false);
        RefreshRect(wxRect(0, rc.bottom, widthNbook, heightNbook - rc.bottom),
                    false);
        RefreshRect(wxRect(rc.right, rc.top, widthNbook - rc.right, height),
                    false);
    }

#if USE_NOTEBOOK_ANTIFLICKER
    // subclass the spin control used by the notebook to scroll pages to
    // prevent it from flickering on resize
    if ( !m_hasSubclassedUpdown )
    {
        // iterate over all child windows to find spin button
        for ( HWND child = ::GetWindow(GetHwnd(), GW_CHILD);
              child;
              child = ::GetWindow(child, GW_HWNDNEXT) )
        {
            wxWindow *childWindow = wxFindWinFromHandle((WXHWND)child);

            // see if it exists, if no wxWindow found then assume it's the spin
            // btn
            if ( !childWindow )
            {
                // subclass the spin button to override WM_ERASEBKGND
                if ( !gs_wndprocNotebookSpinBtn )
                    gs_wndprocNotebookSpinBtn = (WXFARPROC)wxGetWindowProc(child);

                wxSetWindowProc(child, wxNotebookSpinBtnWndProc);
                m_hasSubclassedUpdown = true;
                break;
            }
        }
    }
#endif // USE_NOTEBOOK_ANTIFLICKER

    event.Skip();
}

void wxNotebook::OnSelChange(wxNotebookEvent& event)
{
    // is it our tab control?
    if ( event.GetEventObject() == this )
    {
        UpdateSelection(event.GetSelection());
    }

    // we want to give others a chance to process this message as well
    event.Skip();
}

void wxNotebook::OnNavigationKey(wxNavigationKeyEvent& event)
{
    if ( event.IsWindowChange() ) {
        // change pages
        AdvanceSelection(event.GetDirection());
    }
    else {
        // we get this event in 3 cases
        //
        // a) one of our pages might have generated it because the user TABbed
        // out from it in which case we should propagate the event upwards and
        // our parent will take care of setting the focus to prev/next sibling
        //
        // or
        //
        // b) the parent panel wants to give the focus to us so that we
        // forward it to our selected page. We can't deal with this in
        // OnSetFocus() because we don't know which direction the focus came
        // from in this case and so can't choose between setting the focus to
        // first or last panel child
        //
        // or
        //
        // c) we ourselves (see MSWTranslateMessage) generated the event
        //
        wxWindow * const parent = GetParent();

        // the wxObject* casts are required to avoid MinGW GCC 2.95.3 ICE
        const bool isFromParent = event.GetEventObject() == (wxObject*) parent;
        const bool isFromSelf = event.GetEventObject() == (wxObject*) this;

        if ( isFromParent || isFromSelf )
        {
            // no, it doesn't come from child, case (b) or (c): forward to a
            // page but only if direction is backwards (TAB) or from ourselves,
            if ( m_nSelection != wxNOT_FOUND &&
                    (!event.GetDirection() || isFromSelf) )
            {
                // so that the page knows that the event comes from it's parent
                // and is being propagated downwards
                event.SetEventObject(this);

                wxWindow *page = m_pages[m_nSelection];
                if ( !page->GetEventHandler()->ProcessEvent(event) )
                {
                    page->SetFocus();
                }
                //else: page manages focus inside it itself
            }
            else // otherwise set the focus to the notebook itself
            {
                SetFocus();
            }
        }
        else
        {
            // it comes from our child, case (a), pass to the parent, but only
            // if the direction is forwards. Otherwise set the focus to the
            // notebook itself. The notebook is always the 'first' control of a
            // page.
            if ( !event.GetDirection() )
            {
                SetFocus();
            }
            else if ( parent )
            {
                event.SetCurrentFocus(this);
                parent->GetEventHandler()->ProcessEvent(event);
            }
        }
    }
}

#if wxUSE_UXTHEME

bool wxNotebook::DoDrawBackground(WXHDC hDC, wxWindow *child)
{
    wxUxThemeHandle theme(child ? child : this, L"TAB");
    if ( !theme )
        return false;

    // get the notebook client rect (we're not interested in drawing tabs
    // themselves)
    wxRect r = GetPageSize();
    if ( r.IsEmpty() )
        return false;

    RECT rc;
    wxCopyRectToRECT(r, rc);

    // map rect to the coords of the window we're drawing in
    if ( child )
        ::MapWindowPoints(GetHwnd(), GetHwndOf(child), (POINT *)&rc, 2);

    // we have the content area (page size), but we need to draw all of the
    // background for it to be aligned correctly
    wxUxThemeEngine::Get()->GetThemeBackgroundExtent
                            (
                                theme,
                                (HDC) hDC,
                                9 /* TABP_PANE */,
                                0,
                                &rc,
                                &rc
                            );
    wxUxThemeEngine::Get()->DrawThemeBackground
                            (
                                theme,
                                (HDC) hDC,
                                9 /* TABP_PANE */,
                                0,
                                &rc,
                                NULL
                            );

    return true;
}

WXHBRUSH wxNotebook::QueryBgBitmap()
{
    wxRect r = GetPageSize();
    if ( r.IsEmpty() )
        return 0;

    WindowHDC hDC(GetHwnd());
    MemoryHDC hDCMem(hDC);
    CompatibleBitmap hBmp(hDC, r.x + r.width, r.y + r.height);

    SelectInHDC selectBmp(hDCMem, hBmp);

    if ( !DoDrawBackground((WXHDC)(HDC)hDCMem) )
        return 0;

    return (WXHBRUSH)::CreatePatternBrush(hBmp);
}

void wxNotebook::UpdateBgBrush()
{
    if ( m_hbrBackground )
        ::DeleteObject((HBRUSH)m_hbrBackground);

    if ( !m_hasBgCol && wxUxThemeEngine::GetIfActive() )
    {
        m_hbrBackground = QueryBgBitmap();
    }
    else // no themes or we've got user-defined solid colour
    {
        m_hbrBackground = NULL;
    }
}

WXHBRUSH wxNotebook::MSWGetBgBrushForChild(WXHDC hDC, WXHWND hWnd)
{
    if ( m_hbrBackground )
    {
        // before drawing with the background brush, we need to position it
        // correctly
        RECT rc;
        ::GetWindowRect((HWND)hWnd, &rc);

        ::MapWindowPoints(NULL, GetHwnd(), (POINT *)&rc, 1);

        if ( !::SetBrushOrgEx((HDC)hDC, -rc.left, -rc.top, NULL) )
        {
            wxLogLastError(_T("SetBrushOrgEx(notebook bg brush)"));
        }

        return m_hbrBackground;
    }

    return wxNotebookBase::MSWGetBgBrushForChild(hDC, hWnd);
}

bool wxNotebook::MSWPrintChild(WXHDC hDC, wxWindow *child)
{
    // solid background colour overrides themed background drawing
    if ( !UseBgCol() && DoDrawBackground(hDC, child) )
        return true;

    // If we're using a solid colour (for example if we've switched off
    // theming for this notebook), paint it
    if (UseBgCol())
    {
        wxRect r = GetPageSize();
        if ( r.IsEmpty() )
            return false;

        RECT rc;
        wxCopyRectToRECT(r, rc);

        // map rect to the coords of the window we're drawing in
        if ( child )
            ::MapWindowPoints(GetHwnd(), GetHwndOf(child), (POINT *)&rc, 2);

        wxBrush brush(GetBackgroundColour());
        HBRUSH hbr = GetHbrushOf(brush);

        ::FillRect((HDC) hDC, &rc, hbr);

        return true;
    }

    return wxNotebookBase::MSWPrintChild(hDC, child);
}

#endif // wxUSE_UXTHEME

// Windows only: attempts to get colour for UX theme page background
wxColour wxNotebook::GetThemeBackgroundColour() const
{
#if wxUSE_UXTHEME
    if (wxUxThemeEngine::Get())
    {
        wxUxThemeHandle hTheme((wxNotebook*) this, L"TAB");
        if (hTheme)
        {
            // This is total guesswork.
            // See PlatformSDK\Include\Tmschema.h for values.
            // JACS: can also use 9 (TABP_PANE)
            COLORREF themeColor;
            bool success = (S_OK == wxUxThemeEngine::Get()->GetThemeColor(
                                        hTheme,
                                        10 /* TABP_BODY */,
                                        1 /* NORMAL */,
                                        3821 /* FILLCOLORHINT */,
                                        &themeColor));
            if (!success)
                return GetBackgroundColour();

            /*
            [DS] Workaround for WindowBlinds:
            Some themes return a near black theme color using FILLCOLORHINT,
            this makes notebook pages have an ugly black background and makes
            text (usually black) unreadable. Retry again with FILLCOLOR.

            This workaround potentially breaks appearance of some themes,
            but in practice it already fixes some themes.
            */
            if (themeColor == 1)
            {
                wxUxThemeEngine::Get()->GetThemeColor(
                                            hTheme,
                                            10 /* TABP_BODY */,
                                            1 /* NORMAL */,
                                            3802 /* FILLCOLOR */,
                                            &themeColor);
            }

            wxColour colour = wxRGBToColour(themeColor);

            // Under Vista, the tab background colour is reported incorrectly.
            // So for the default theme at least, hard-code the colour to something
            // that will blend in.

            static int s_AeroStatus = -1;
            if (s_AeroStatus == -1)
            {
                WCHAR szwThemeFile[1024];
                WCHAR szwThemeColor[256];
                if (S_OK == wxUxThemeEngine::Get()->GetCurrentThemeName(szwThemeFile, 1024, szwThemeColor, 256, NULL, 0))
                {
                    wxString themeFile(szwThemeFile), themeColor(szwThemeColor);
                    if (themeFile.Find(wxT("Aero")) != -1 && themeColor == wxT("NormalColor"))
                        s_AeroStatus = 1;
                    else
                        s_AeroStatus = 0;
                }
                else
                    s_AeroStatus = 0;
            }

            if (s_AeroStatus == 1)
                colour = wxColour(255, 255, 255);

            return colour;
        }
    }
#endif // wxUSE_UXTHEME

    return GetBackgroundColour();
}

// ----------------------------------------------------------------------------
// wxNotebook base class virtuals
// ----------------------------------------------------------------------------

#if wxUSE_CONSTRAINTS

// override these 2 functions to do nothing: everything is done in OnSize

void wxNotebook::SetConstraintSizes(bool WXUNUSED(recurse))
{
  // don't set the sizes of the pages - their correct size is not yet known
  wxControl::SetConstraintSizes(false);
}

bool wxNotebook::DoPhase(int WXUNUSED(nPhase))
{
  return true;
}

#endif // wxUSE_CONSTRAINTS

// ----------------------------------------------------------------------------
// wxNotebook Windows message handlers
// ----------------------------------------------------------------------------

bool wxNotebook::MSWOnScroll(int orientation, WXWORD nSBCode,
                             WXWORD pos, WXHWND control)
{
    // don't generate EVT_SCROLLWIN events for the WM_SCROLLs coming from the
    // up-down control
    if ( control )
        return false;

    return wxNotebookBase::MSWOnScroll(orientation, nSBCode, pos, control);
}

bool wxNotebook::MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM* result)
{
  wxNotebookEvent event(wxEVT_NULL, m_windowId);

  NMHDR* hdr = (NMHDR *)lParam;
  switch ( hdr->code ) {
    case TCN_SELCHANGE:
      event.SetEventType(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED);
      break;

    case TCN_SELCHANGING:
      event.SetEventType(wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING);
      break;

    default:
      return wxControl::MSWOnNotify(idCtrl, lParam, result);
  }

  event.SetSelection(TabCtrl_GetCurSel(GetHwnd()));
  event.SetOldSelection(m_nSelection);
  event.SetEventObject(this);
  event.SetInt(idCtrl);

  bool processed = GetEventHandler()->ProcessEvent(event);
  *result = !event.IsAllowed();
  return processed;
}

#endif // wxUSE_NOTEBOOK
