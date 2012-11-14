///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/listbox.cpp
// Purpose:     wxListBox
// Author:      Julian Smart
// Modified by: Vadim Zeitlin (owner drawn stuff)
// Created:
// RCS-ID:      $Id: listbox.cpp 66941 2011-02-17 11:01:22Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_LISTBOX

#include "wx/listbox.h"

#ifndef WX_PRECOMP
    #include "wx/dynarray.h"
    #include "wx/settings.h"
    #include "wx/brush.h"
    #include "wx/font.h"
    #include "wx/dc.h"
    #include "wx/utils.h"
    #include "wx/log.h"
    #include "wx/window.h"
#endif

#include "wx/msw/private.h"

#include <windowsx.h>

#if wxUSE_OWNER_DRAWN
    #include  "wx/ownerdrw.h"
#endif

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxListBoxStyle )

wxBEGIN_FLAGS( wxListBoxStyle )
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

    wxFLAGS_MEMBER(wxLB_SINGLE)
    wxFLAGS_MEMBER(wxLB_MULTIPLE)
    wxFLAGS_MEMBER(wxLB_EXTENDED)
    wxFLAGS_MEMBER(wxLB_HSCROLL)
    wxFLAGS_MEMBER(wxLB_ALWAYS_SB)
    wxFLAGS_MEMBER(wxLB_NEEDED_SB)
    wxFLAGS_MEMBER(wxLB_SORT)

wxEND_FLAGS( wxListBoxStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxListBox, wxControl,"wx/listbox.h")

wxBEGIN_PROPERTIES_TABLE(wxListBox)
    wxEVENT_PROPERTY( Select , wxEVT_COMMAND_LISTBOX_SELECTED , wxCommandEvent )
    wxEVENT_PROPERTY( DoubleClick , wxEVT_COMMAND_LISTBOX_DOUBLECLICKED , wxCommandEvent )

    wxPROPERTY( Font , wxFont , SetFont , GetFont  , EMPTY_MACROVALUE, 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_COLLECTION( Choices , wxArrayString , wxString , AppendString , GetStrings, 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxPROPERTY( Selection ,int, SetSelection, GetSelection, EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxPROPERTY_FLAGS( WindowStyle , wxListBoxStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxListBox)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_4( wxListBox , wxWindow* , Parent , wxWindowID , Id , wxPoint , Position , wxSize , Size )
#else
IMPLEMENT_DYNAMIC_CLASS(wxListBox, wxControl)
#endif

/*
TODO PROPERTIES
    selection
    content
        item
*/

// ============================================================================
// list box item declaration and implementation
// ============================================================================

#if wxUSE_OWNER_DRAWN

class wxListBoxItem : public wxOwnerDrawn
{
public:
    wxListBoxItem(const wxString& str = wxEmptyString);
};

wxListBoxItem::wxListBoxItem(const wxString& str) : wxOwnerDrawn(str, false)
{
    // no bitmaps/checkmarks
    SetMarginWidth(0);
}

wxOwnerDrawn *wxListBox::CreateLboxItem(size_t WXUNUSED(n))
{
    return new wxListBoxItem();
}

#endif  //USE_OWNER_DRAWN

// ============================================================================
// list box control implementation
// ============================================================================

// ----------------------------------------------------------------------------
// creation
// ----------------------------------------------------------------------------

// Listbox item
wxListBox::wxListBox()
{
    m_noItems = 0;
    m_selected = 0;
#if wxUSE_LISTBOX_SELECTION_FIX
    m_selectedByKeyboard = false;
#endif
}

bool wxListBox::Create(wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       int n, const wxString choices[],
                       long style,
                       const wxValidator& validator,
                       const wxString& name)
{
    m_noItems = 0;
    m_selected = 0;

    // initialize base class fields
    if ( !CreateControl(parent, id, pos, size, style, validator, name) )
        return false;

    // create the native control
    if ( !MSWCreateControl(_T("LISTBOX"), wxEmptyString, pos, size) )
    {
        // control creation failed
        return false;
    }

    // initialize the contents
    for ( int i = 0; i < n; i++ )
    {
        Append(choices[i]);
    }

    // now we can compute our best size correctly, so do it if necessary
    SetInitialSize(size);

    return true;
}

bool wxListBox::Create(wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       const wxArrayString& choices,
                       long style,
                       const wxValidator& validator,
                       const wxString& name)
{
    wxCArrayString chs(choices);
    return Create(parent, id, pos, size, chs.GetCount(), chs.GetStrings(),
                  style, validator, name);
}

wxListBox::~wxListBox()
{
    Free();
}

WXDWORD wxListBox::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    WXDWORD msStyle = wxControl::MSWGetStyle(style, exstyle);

    // always show the vertical scrollbar if necessary -- otherwise it is
    // impossible to use the control with the mouse
    msStyle |= WS_VSCROLL;

    // we always want to get the notifications
    msStyle |= LBS_NOTIFY;

    // without this style, you get unexpected heights, so e.g. constraint
    // layout doesn't work properly
    msStyle |= LBS_NOINTEGRALHEIGHT;

    wxASSERT_MSG( !(style & wxLB_MULTIPLE) || !(style & wxLB_EXTENDED),
                  _T("only one of listbox selection modes can be specified") );

    if ( style & wxLB_MULTIPLE )
        msStyle |= LBS_MULTIPLESEL;
    else if ( style & wxLB_EXTENDED )
        msStyle |= LBS_EXTENDEDSEL;

    if ( m_windowStyle & wxLB_ALWAYS_SB )
        msStyle |= LBS_DISABLENOSCROLL;
    if ( m_windowStyle & wxLB_HSCROLL )
        msStyle |= WS_HSCROLL;
    if ( m_windowStyle & wxLB_SORT )
        msStyle |= LBS_SORT;

#if wxUSE_OWNER_DRAWN && !defined(__WXWINCE__)
    if ( m_windowStyle & wxLB_OWNERDRAW )
    {
        // we don't support LBS_OWNERDRAWVARIABLE yet and we also always put
        // the strings in the listbox for simplicity even though we could have
        // avoided it in this case
        msStyle |= LBS_OWNERDRAWFIXED | LBS_HASSTRINGS;
    }
#endif // wxUSE_OWNER_DRAWN

    return msStyle;
}

// ----------------------------------------------------------------------------
// implementation of wxListBoxBase methods
// ----------------------------------------------------------------------------

void wxListBox::DoSetFirstItem(int N)
{
    wxCHECK_RET( IsValid(N),
                 wxT("invalid index in wxListBox::SetFirstItem") );

    SendMessage(GetHwnd(), LB_SETTOPINDEX, (WPARAM)N, (LPARAM)0);
}

void wxListBox::Delete(unsigned int n)
{
    wxCHECK_RET( IsValid(n),
                 wxT("invalid index in wxListBox::Delete") );

    // for owner drawn objects, the data is used for storing wxOwnerDrawn
    // pointers and we shouldn't touch it
#if !wxUSE_OWNER_DRAWN
    if ( !(m_windowStyle & wxLB_OWNERDRAW) )
#endif // !wxUSE_OWNER_DRAWN
        if ( HasClientObjectData() )
        {
            delete GetClientObject(n);
        }

    SendMessage(GetHwnd(), LB_DELETESTRING, n, 0);
    m_noItems--;

    SetHorizontalExtent(wxEmptyString);
}

int wxListBox::DoAppend(const wxString& item)
{
    int index = ListBox_AddString(GetHwnd(), item);
    m_noItems++;

#if wxUSE_OWNER_DRAWN
    if ( m_windowStyle & wxLB_OWNERDRAW ) {
        wxOwnerDrawn *pNewItem = CreateLboxItem(index); // dummy argument
        pNewItem->SetName(item);
        m_aItems.Insert(pNewItem, index);
        ListBox_SetItemData(GetHwnd(), index, pNewItem);
        pNewItem->SetFont(GetFont());
    }
#endif // wxUSE_OWNER_DRAWN

    SetHorizontalExtent(item);

    return index;
}

void wxListBox::DoSetItems(const wxArrayString& choices, void** clientData)
{
    // avoid flicker - but don't need to do this for a hidden listbox
    bool hideAndShow = IsShown();
    if ( hideAndShow )
    {
        ShowWindow(GetHwnd(), SW_HIDE);
    }

    ListBox_ResetContent(GetHwnd());

    m_noItems = choices.GetCount();
    unsigned int i;
    for (i = 0; i < m_noItems; i++)
    {
        ListBox_AddString(GetHwnd(), choices[i]);
        if ( clientData )
        {
            SetClientData(i, clientData[i]);
        }
    }

#if wxUSE_OWNER_DRAWN
    if ( m_windowStyle & wxLB_OWNERDRAW ) {
        // first delete old items
        WX_CLEAR_ARRAY(m_aItems);

        // then create new ones
        for ( unsigned int ui = 0; ui < m_noItems; ui++ ) {
            wxOwnerDrawn *pNewItem = CreateLboxItem(ui);
            pNewItem->SetName(choices[ui]);
            m_aItems.Add(pNewItem);
            ListBox_SetItemData(GetHwnd(), ui, pNewItem);
        }
    }
#endif // wxUSE_OWNER_DRAWN

    SetHorizontalExtent();

    if ( hideAndShow )
    {
        // show the listbox back if we hid it
        ShowWindow(GetHwnd(), SW_SHOW);
    }
}

int wxListBox::FindString(const wxString& s, bool bCase) const
{
    // back to base class search for not native search type
    if (bCase)
       return wxItemContainerImmutable::FindString( s, bCase );

    int pos = ListBox_FindStringExact(GetHwnd(), -1, s);
    if (pos == LB_ERR)
        return wxNOT_FOUND;
    else
        return pos;
}

void wxListBox::Clear()
{
    Free();

    ListBox_ResetContent(GetHwnd());

    m_noItems = 0;
    SetHorizontalExtent();
}

void wxListBox::Free()
{
#if wxUSE_OWNER_DRAWN
    if ( m_windowStyle & wxLB_OWNERDRAW )
    {
        WX_CLEAR_ARRAY(m_aItems);
    }
    else
#endif // wxUSE_OWNER_DRAWN
    if ( HasClientObjectData() )
    {
        for ( unsigned int n = 0; n < m_noItems; n++ )
        {
            delete GetClientObject(n);
        }
    }
}

void wxListBox::DoSetSelection(int N, bool select)
{
    wxCHECK_RET( N == wxNOT_FOUND || IsValid(N),
                 wxT("invalid index in wxListBox::SetSelection") );

    if ( HasMultipleSelection() )
    {
        SendMessage(GetHwnd(), LB_SETSEL, select, N);
    }
    else
    {
        SendMessage(GetHwnd(), LB_SETCURSEL, select ? N : -1, 0);
    }
}

bool wxListBox::IsSelected(int N) const
{
    wxCHECK_MSG( IsValid(N), false,
                 wxT("invalid index in wxListBox::Selected") );

    return SendMessage(GetHwnd(), LB_GETSEL, N, 0) == 0 ? false : true;
}

wxClientData* wxListBox::DoGetItemClientObject(unsigned int n) const
{
    return (wxClientData *)DoGetItemClientData(n);
}

void *wxListBox::DoGetItemClientData(unsigned int n) const
{
    wxCHECK_MSG( IsValid(n), NULL,
                 wxT("invalid index in wxListBox::GetClientData") );

    return (void *)SendMessage(GetHwnd(), LB_GETITEMDATA, n, 0);
}

void wxListBox::DoSetItemClientObject(unsigned int n, wxClientData* clientData)
{
    DoSetItemClientData(n, clientData);
}

void wxListBox::DoSetItemClientData(unsigned int n, void *clientData)
{
    wxCHECK_RET( IsValid(n),
                 wxT("invalid index in wxListBox::SetClientData") );

#if wxUSE_OWNER_DRAWN
    if ( m_windowStyle & wxLB_OWNERDRAW )
    {
        // client data must be pointer to wxOwnerDrawn, otherwise we would crash
        // in OnMeasure/OnDraw.
        wxFAIL_MSG(wxT("Can't use client data with owner-drawn listboxes"));
    }
#endif // wxUSE_OWNER_DRAWN

    if ( ListBox_SetItemData(GetHwnd(), n, clientData) == LB_ERR )
        wxLogDebug(wxT("LB_SETITEMDATA failed"));
}

// Return number of selections and an array of selected integers
int wxListBox::GetSelections(wxArrayInt& aSelections) const
{
    aSelections.Empty();

    if ( HasMultipleSelection() )
    {
        int countSel = ListBox_GetSelCount(GetHwnd());
        if ( countSel == LB_ERR )
        {
            wxLogDebug(_T("ListBox_GetSelCount failed"));
        }
        else if ( countSel != 0 )
        {
            int *selections = new int[countSel];

            if ( ListBox_GetSelItems(GetHwnd(),
                                     countSel, selections) == LB_ERR )
            {
                wxLogDebug(wxT("ListBox_GetSelItems failed"));
                countSel = -1;
            }
            else
            {
                aSelections.Alloc(countSel);
                for ( int n = 0; n < countSel; n++ )
                    aSelections.Add(selections[n]);
            }

            delete [] selections;
        }

        return countSel;
    }
    else  // single-selection listbox
    {
        if (ListBox_GetCurSel(GetHwnd()) > -1)
            aSelections.Add(ListBox_GetCurSel(GetHwnd()));

        return aSelections.Count();
    }
}

// Get single selection, for single choice list items
int wxListBox::GetSelection() const
{
    wxCHECK_MSG( !HasMultipleSelection(),
                 -1,
                 wxT("GetSelection() can't be used with multiple-selection listboxes, use GetSelections() instead.") );

    return ListBox_GetCurSel(GetHwnd());
}

// Find string for position
wxString wxListBox::GetString(unsigned int n) const
{
    wxCHECK_MSG( IsValid(n), wxEmptyString,
                 wxT("invalid index in wxListBox::GetString") );

    int len = ListBox_GetTextLen(GetHwnd(), n);

    // +1 for terminating NUL
    wxString result;
    ListBox_GetText(GetHwnd(), n, (wxChar*)wxStringBuffer(result, len + 1));

    return result;
}

void
wxListBox::DoInsertItems(const wxArrayString& items, unsigned int pos)
{
    wxCHECK_RET( IsValidInsert(pos),
                 wxT("invalid index in wxListBox::InsertItems") );

    unsigned int nItems = items.GetCount();
    for ( unsigned int i = 0; i < nItems; i++ )
    {
        int idx = ListBox_InsertString(GetHwnd(), i + pos, items[i]);

#if wxUSE_OWNER_DRAWN
        if ( m_windowStyle & wxLB_OWNERDRAW )
        {
            wxOwnerDrawn *pNewItem = CreateLboxItem(idx);
            pNewItem->SetName(items[i]);
            pNewItem->SetFont(GetFont());
            m_aItems.Insert(pNewItem, idx);

            ListBox_SetItemData(GetHwnd(), idx, pNewItem);
        }
#else
        wxUnusedVar(idx);
#endif // wxUSE_OWNER_DRAWN
    }

    m_noItems += nItems;

    SetHorizontalExtent();
}

int wxListBox::DoListHitTest(const wxPoint& point) const
{
    LRESULT lRes = ::SendMessage(GetHwnd(), LB_ITEMFROMPOINT,
                                 0, MAKELPARAM(point.x, point.y));

    // non zero high-order word means that this item is outside of the client
    // area, IOW the point is outside of the listbox
    return HIWORD(lRes) ? wxNOT_FOUND : LOWORD(lRes);
}

void wxListBox::SetString(unsigned int n, const wxString& s)
{
    wxCHECK_RET( IsValid(n),
                 wxT("invalid index in wxListBox::SetString") );

    // remember the state of the item
    bool wasSelected = IsSelected(n);

    void *oldData = NULL;
    wxClientData *oldObjData = NULL;
    if ( m_clientDataItemsType == wxClientData_Void )
        oldData = GetClientData(n);
    else if ( m_clientDataItemsType == wxClientData_Object )
        oldObjData = GetClientObject(n);

    // delete and recreate it
    SendMessage(GetHwnd(), LB_DELETESTRING, n, 0);

    int newN = n;
    if ( n == (m_noItems - 1) )
        newN = -1;

    ListBox_InsertString(GetHwnd(), newN, s);

    // restore the client data
    if ( oldData )
        SetClientData(n, oldData);
    else if ( oldObjData )
        SetClientObject(n, oldObjData);

#if wxUSE_OWNER_DRAWN
    if ( m_windowStyle & wxLB_OWNERDRAW )
    {
        // update item's text
        m_aItems[n]->SetName(s);

        // reassign the item's data
        ListBox_SetItemData(GetHwnd(), n, m_aItems[n]);
    }
#endif  //USE_OWNER_DRAWN

    // we may have lost the selection
    if ( wasSelected )
        Select(n);

    SetHorizontalExtent();
}

unsigned int wxListBox::GetCount() const
{
    return m_noItems;
}

// ----------------------------------------------------------------------------
// size-related stuff
// ----------------------------------------------------------------------------

void wxListBox::SetHorizontalExtent(const wxString& s)
{
    // in any case, our best size could have changed
    InvalidateBestSize();

    // the rest is only necessary if we want a horizontal scrollbar
    if ( !HasFlag(wxHSCROLL) )
        return;


    WindowHDC dc(GetHwnd());
    SelectInHDC selFont(dc, GetHfontOf(GetFont()));

    TEXTMETRIC lpTextMetric;
    ::GetTextMetrics(dc, &lpTextMetric);

    int largestExtent = 0;
    SIZE extentXY;

    if ( s.empty() )
    {
        // set extent to the max length of all strings
        for ( unsigned int i = 0; i < m_noItems; i++ )
        {
            const wxString str = GetString(i);
            ::GetTextExtentPoint32(dc, str.c_str(), str.length(), &extentXY);

            int extentX = (int)(extentXY.cx + lpTextMetric.tmAveCharWidth);
            if ( extentX > largestExtent )
                largestExtent = extentX;
        }
    }
    else // just increase the extent to the length of this string
    {
        int existingExtent = (int)SendMessage(GetHwnd(),
                                              LB_GETHORIZONTALEXTENT, 0, 0L);

        ::GetTextExtentPoint32(dc, s.c_str(), s.length(), &extentXY);

        int extentX = (int)(extentXY.cx + lpTextMetric.tmAveCharWidth);
        if ( extentX > existingExtent )
            largestExtent = extentX;
    }

    if ( largestExtent )
        SendMessage(GetHwnd(), LB_SETHORIZONTALEXTENT, LOWORD(largestExtent), 0L);
    //else: it shouldn't change
}

wxSize wxListBox::DoGetBestSize() const
{
    // find the widest string
    int wLine;
    int wListbox = 0;
    for (unsigned int i = 0; i < m_noItems; i++)
    {
        wxString str(GetString(i));
        GetTextExtent(str, &wLine, NULL);
        if ( wLine > wListbox )
            wListbox = wLine;
    }

    // give it some reasonable default value if there are no strings in the
    // list
    if ( wListbox == 0 )
        wListbox = 100;

    // the listbox should be slightly larger than the widest string
    int cx, cy;
    wxGetCharSize(GetHWND(), &cx, &cy, GetFont());

    wListbox += 3*cx;

    // Add room for the scrollbar
    wListbox += wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);

    // don't make the listbox too tall (limit height to 10 items) but don't
    // make it too small neither
    int hListbox = EDIT_HEIGHT_FROM_CHAR_HEIGHT(cy)*
                    wxMin(wxMax(m_noItems, 3), 10);

    wxSize best(wListbox, hListbox);
    CacheBestSize(best);
    return best;
}

// ----------------------------------------------------------------------------
// callbacks
// ----------------------------------------------------------------------------

bool wxListBox::MSWCommand(WXUINT param, WXWORD WXUNUSED(id))
{
    wxEventType evtType;
    int n = wxNOT_FOUND;

#if wxUSE_LISTBOX_SELECTION_FIX
    if ( param == LBN_SELCHANGE )
    {
        evtType = wxEVT_COMMAND_LISTBOX_SELECTED;
        if ( m_selectedByKeyboard )
        {
            // We shouldn't use the mouse position to find the item as mouse
            // can be anywhere, ask the listbox itself. Notice that this can't
            // be used when the item is selected using the mouse however as
            // LB_GETCARETINDEX will always return a valid item, even if the
            // mouse is clicked below all the items, which is why we find the
            // item ourselves below in this case.
            n = SendMessage(GetHwnd(), LB_GETCARETINDEX, 0, 0);
        }
        //else: n will be determined below from the mouse position

        // NB: conveniently enough, LB_ERR is the same as wxNOT_FOUND
    }
    else if ( param == LBN_DBLCLK )
    {
        evtType = wxEVT_COMMAND_LISTBOX_DOUBLECLICKED;
        n = HitTest(ScreenToClient(wxGetMousePosition()));
    }
    else
    {
        // some event we're not interested in
        return false;
    }

    // Find the item position if it was a mouse-generated selection event or a
    // double click event (which is always generated using the mouse)
    if ( n == wxNOT_FOUND )
    {
        const DWORD pos = ::GetMessagePos();
        const wxPoint pt(GET_X_LPARAM(pos), GET_Y_LPARAM(pos));
        n = HitTest(ScreenToClient(wxPoint(pt)));
    }
#else
    if ( param == LBN_SELCHANGE )
    {
        evtType = wxEVT_COMMAND_LISTBOX_SELECTED;
        n = SendMessage(GetHwnd(), LB_GETCARETINDEX, 0, 0);

        // NB: conveniently enough, LB_ERR is the same as wxNOT_FOUND
    }
    else if ( param == LBN_DBLCLK )
    {
        evtType = wxEVT_COMMAND_LISTBOX_DOUBLECLICKED;
        n = HitTest(ScreenToClient(wxGetMousePosition()));
    }
    else
    {
        // some event we're not interested in
        return false;
    }
#endif

    // retrieve the affected item
    if ( n == wxNOT_FOUND )
        return false;

    wxCommandEvent event(evtType, m_windowId);
    event.SetEventObject(this);

    if ( HasClientObjectData() )
        event.SetClientObject( GetClientObject(n) );
    else if ( HasClientUntypedData() )
        event.SetClientData( GetClientData(n) );

    event.SetString(GetString(n));
    event.SetInt(n);
    event.SetExtraLong( HasMultipleSelection() ? IsSelected(n) : true );

    return GetEventHandler()->ProcessEvent(event);
}

#if wxUSE_LISTBOX_SELECTION_FIX
WXLRESULT
wxListBox::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    // Remember whether there was a keyboard or mouse event before
    // LBN_SELCHANGE: this allows us to correctly determine the item affected
    // by it in MSWCommand() above in any case.
    if ( WM_KEYFIRST <= nMsg && nMsg <= WM_KEYLAST )
        m_selectedByKeyboard = true;
    else if ( WM_MOUSEFIRST <= nMsg && nMsg <= WM_MOUSELAST )
        m_selectedByKeyboard = false;

    return wxListBoxBase::MSWWindowProc(nMsg, wParam, lParam);
}
#endif

// ----------------------------------------------------------------------------
// wxCheckListBox support
// ----------------------------------------------------------------------------

#if wxUSE_OWNER_DRAWN

// drawing
// -------

// space beneath/above each row in pixels
// "standard" checklistbox use 1 here, some might prefer 2. 0 is ugly.
#define OWNER_DRAWN_LISTBOX_EXTRA_SPACE    (1)

// the height is the same for all items
// TODO should be changed for LBS_OWNERDRAWVARIABLE style listboxes

// NB: can't forward this to wxListBoxItem because LB_SETITEMDATA
//     message is not yet sent when we get here!
bool wxListBox::MSWOnMeasure(WXMEASUREITEMSTRUCT *item)
{
    // only owner-drawn control should receive this message
    wxCHECK( ((m_windowStyle & wxLB_OWNERDRAW) == wxLB_OWNERDRAW), false );

    MEASUREITEMSTRUCT *pStruct = (MEASUREITEMSTRUCT *)item;

#ifdef __WXWINCE__
    HDC hdc = GetDC(NULL);
#else
    HDC hdc = CreateIC(wxT("DISPLAY"), NULL, NULL, 0);
#endif

    {
        wxDCTemp dc((WXHDC)hdc);
        dc.SetFont(GetFont());

        pStruct->itemHeight = dc.GetCharHeight() + 2*OWNER_DRAWN_LISTBOX_EXTRA_SPACE;
        pStruct->itemWidth  = dc.GetCharWidth();
    }

#ifdef __WXWINCE__
    ReleaseDC(NULL, hdc);
#else
    DeleteDC(hdc);
#endif

    return true;
}

// forward the message to the appropriate item
bool wxListBox::MSWOnDraw(WXDRAWITEMSTRUCT *item)
{
    // only owner-drawn control should receive this message
    wxCHECK( ((m_windowStyle & wxLB_OWNERDRAW) == wxLB_OWNERDRAW), false );

    DRAWITEMSTRUCT *pStruct = (DRAWITEMSTRUCT *)item;
    UINT itemID = pStruct->itemID;

    // the item may be -1 for an empty listbox
    if ( itemID == (UINT)-1 )
        return false;

    LRESULT data = ListBox_GetItemData(GetHwnd(), pStruct->itemID);

    wxCHECK( data && (data != LB_ERR), false );

    wxListBoxItem *pItem = (wxListBoxItem *)data;

    wxDCTemp dc((WXHDC)pStruct->hDC);

    return pItem->OnDrawItem(dc, wxRectFromRECT(pStruct->rcItem),
                             (wxOwnerDrawn::wxODAction)pStruct->itemAction,
                             (wxOwnerDrawn::wxODStatus)pStruct->itemState);
}

#endif // wxUSE_OWNER_DRAWN

#endif // wxUSE_LISTBOX
