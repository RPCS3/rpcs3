///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/wince/choicece.cpp
// Purpose:     wxChoice implementation for smart phones driven by WinCE
// Author:      Wlodzimierz ABX Skiba
// Modified by:
// Created:     29.07.2004
// RCS-ID:      $Id: choicece.cpp 42816 2006-10-31 08:50:17Z RD $
// Copyright:   (c) Wlodzimierz Skiba
// License:     wxWindows licence
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

#if wxUSE_CHOICE && defined(__SMARTPHONE__) && defined(__WXWINCE__)

#include "wx/choice.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
#endif

#include "wx/spinbutt.h" // for wxSpinnerBestSize

#if wxUSE_EXTENDED_RTTI
// TODO
#else
IMPLEMENT_DYNAMIC_CLASS(wxChoice, wxControl)
#endif

#define GetBuddyHwnd()      (HWND)(m_hwndBuddy)

#define IsVertical(wxStyle) ( (wxStyle & wxSP_HORIZONTAL) != wxSP_HORIZONTAL )

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the margin between the up-down control and its buddy (can be arbitrary,
// choose what you like - or may be decide during run-time depending on the
// font size?)
static const int MARGIN_BETWEEN = 0;

// ============================================================================
// implementation
// ============================================================================

wxArrayChoiceSpins wxChoice::ms_allChoiceSpins;

// ----------------------------------------------------------------------------
// wnd proc for the buddy text ctrl
// ----------------------------------------------------------------------------

LRESULT APIENTRY _EXPORT wxBuddyChoiceWndProc(HWND hwnd,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam)
{
    wxChoice *spin = (wxChoice *)wxGetWindowUserData(hwnd);

    // forward some messages (the key and focus ones only so far) to
    // the spin ctrl
    switch ( message )
    {
        case WM_SETFOCUS:
            // if the focus comes from the spin control itself, don't set it
            // back to it -- we don't want to go into an infinite loop
            if ( (WXHWND)wParam == spin->GetHWND() )
                break;
            //else: fall through

        case WM_KILLFOCUS:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_KEYUP:
        case WM_KEYDOWN:
            spin->MSWWindowProc(message, wParam, lParam);

            // The control may have been deleted at this point, so check.
            if ( !::IsWindow(hwnd) || wxGetWindowUserData(hwnd) != spin )
                return 0;
            break;

        case WM_GETDLGCODE:
            // we want to get WXK_RETURN in order to generate the event for it
            return DLGC_WANTCHARS;
    }

    return ::CallWindowProc(CASTWNDPROC spin->GetBuddyWndProc(),
                            hwnd, message, wParam, lParam);
}

wxChoice *wxChoice::GetChoiceForListBox(WXHWND hwndBuddy)
{
    wxChoice *choice = (wxChoice *)wxGetWindowUserData((HWND)hwndBuddy);

    int i = ms_allChoiceSpins.Index(choice);

    if ( i == wxNOT_FOUND )
        return NULL;

    // sanity check
    wxASSERT_MSG( choice->m_hwndBuddy == hwndBuddy,
                  _T("wxChoice has incorrect buddy HWND!") );

    return choice;
}

// ----------------------------------------------------------------------------
// creation
// ----------------------------------------------------------------------------

bool wxChoice::Create(wxWindow *parent,
                      wxWindowID id,
                      const wxPoint& pos,
                      const wxSize& size,
                      int n, const wxString choices[],
                      long style,
                      const wxValidator& validator,
                      const wxString& name)
{
    return CreateAndInit(parent, id, pos, size, n, choices, style,
                         validator, name);
}

bool wxChoice::CreateAndInit(wxWindow *parent,
                             wxWindowID id,
                             const wxPoint& pos,
                             const wxSize& size,
                             int n, const wxString choices[],
                             long style,
                             const wxValidator& validator,
                             const wxString& name)
{
    if ( !(style & wxSP_VERTICAL) )
        style |= wxSP_HORIZONTAL;

    if ( (style & wxBORDER_MASK) == wxBORDER_DEFAULT )
        style |= wxBORDER_SIMPLE;

    style |= wxSP_ARROW_KEYS;

    SetWindowStyle(style);

    WXDWORD exStyle = 0;
    WXDWORD msStyle = MSWGetStyle(GetWindowStyle(), & exStyle) ;

    wxSize sizeText(size), sizeBtn(size);
    sizeBtn.x = GetBestSpinnerSize(IsVertical(style)).x;

    if ( sizeText.x == wxDefaultCoord )
    {
        // DEFAULT_ITEM_WIDTH is the default width for the text control
        sizeText.x = DEFAULT_ITEM_WIDTH + MARGIN_BETWEEN + sizeBtn.x;
    }

    sizeText.x -= sizeBtn.x + MARGIN_BETWEEN;
    if ( sizeText.x <= 0 )
    {
        wxLogDebug(_T("not enough space for wxSpinCtrl!"));
    }

    wxPoint posBtn(pos);
    posBtn.x += sizeText.x + MARGIN_BETWEEN;

    // we must create the list control before the spin button for the purpose
    // of the dialog navigation: if there is a static text just before the spin
    // control, activating it by Alt-letter should give focus to the text
    // control, not the spin and the dialog navigation code will give focus to
    // the next control (at Windows level), not the one after it

    // create the text window

    m_hwndBuddy = (WXHWND)::CreateWindowEx
                    (
                     exStyle,                // sunken border
                     _T("LISTBOX"),          // window class
                     NULL,                   // no window title
                     msStyle,                // style (will be shown later)
                     pos.x, pos.y,           // position
                     0, 0,                   // size (will be set later)
                     GetHwndOf(parent),      // parent
                     (HMENU)-1,              // control id
                     wxGetInstance(),        // app instance
                     NULL                    // unused client data
                    );

    if ( !m_hwndBuddy )
    {
        wxLogLastError(wxT("CreateWindow(buddy text window)"));

        return false;
    }

    // initialize wxControl
    if ( !CreateControl(parent, id, posBtn, sizeBtn, style, validator, name) )
        return false;

    // now create the real HWND
    WXDWORD spiner_style = WS_VISIBLE |
                           UDS_ALIGNRIGHT |
                           UDS_ARROWKEYS |
                           UDS_SETBUDDYINT |
                           UDS_EXPANDABLE;

    if ( !IsVertical(style) )
        spiner_style |= UDS_HORZ;

    if ( style & wxSP_WRAP )
        spiner_style |= UDS_WRAP;

    if ( !MSWCreateControl(UPDOWN_CLASS, spiner_style, posBtn, sizeBtn, wxEmptyString, 0) )
        return false;

    // subclass the text ctrl to be able to intercept some events
    wxSetWindowUserData(GetBuddyHwnd(), this);
    m_wndProcBuddy = (WXFARPROC)wxSetWindowProc(GetBuddyHwnd(),
                                                wxBuddyChoiceWndProc);

    // set up fonts and colours  (This is nomally done in MSWCreateControl)
    InheritAttributes();
    if (!m_hasFont)
        SetFont(GetDefaultAttributes().font);

    // set the size of the text window - can do it only now, because we
    // couldn't call DoGetBestSize() before as font wasn't set
    if ( sizeText.y <= 0 )
    {
        int cx, cy;
        wxGetCharSize(GetHWND(), &cx, &cy, GetFont());

        sizeText.y = EDIT_HEIGHT_FROM_CHAR_HEIGHT(cy);
    }

    SetInitialSize(size);

    (void)::ShowWindow(GetBuddyHwnd(), SW_SHOW);

    // associate the list window with the spin button
    (void)::SendMessage(GetHwnd(), UDM_SETBUDDY, (WPARAM)GetBuddyHwnd(), 0);

    // do it after finishing with m_hwndBuddy creation to avoid generating
    // initial wxEVT_COMMAND_TEXT_UPDATED message
    ms_allChoiceSpins.Add(this);

    // initialize the controls contents
    for ( int i = 0; i < n; i++ )
    {
        Append(choices[i]);
    }

    return true;
}

bool wxChoice::Create(wxWindow *parent,
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

WXDWORD wxChoice::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    // we never have an external border
    WXDWORD msStyle = wxControl::MSWGetStyle
                      (
                        (style & ~wxBORDER_MASK) | wxBORDER_NONE, exstyle
                      );

    msStyle |= WS_VISIBLE;

    // wxChoice-specific styles
    msStyle |= LBS_NOINTEGRALHEIGHT;
    if ( style & wxCB_SORT )
        msStyle |= LBS_SORT;

    msStyle |= LBS_NOTIFY;

    return msStyle;
}

bool wxChoice::MSWCommand(WXUINT param, WXWORD WXUNUSED(id))
{
    if ( param != LBN_SELCHANGE)
    {
        // "selection changed" is the only event we're after
        return false;
    }

    int n = GetSelection();
    if (n > -1)
    {
        wxCommandEvent event(wxEVT_COMMAND_CHOICE_SELECTED, m_windowId);
        event.SetInt(n);
        event.SetEventObject(this);
        event.SetString(GetStringSelection());
        if ( HasClientObjectData() )
            event.SetClientObject( GetClientObject(n) );
        else if ( HasClientUntypedData() )
            event.SetClientData( GetClientData(n) );
        ProcessCommand(event);
    }

    return true;
}

wxChoice::~wxChoice()
{
    Free();
}

// ----------------------------------------------------------------------------
// adding/deleting items to/from the list
// ----------------------------------------------------------------------------

int wxChoice::DoAppend(const wxString& item)
{
    int n = (int)::SendMessage(GetBuddyHwnd(), LB_ADDSTRING, 0, (LPARAM)item.c_str());

    if ( n == LB_ERR )
    {
        wxLogLastError(wxT("SendMessage(LB_ADDSTRING)"));
    }

    return n;
}

int wxChoice::DoInsert(const wxString& item, unsigned int pos)
{
    wxCHECK_MSG(!(GetWindowStyle() & wxCB_SORT), -1, wxT("can't insert into choice"));
    wxCHECK_MSG(IsValidInsert(pos), -1, wxT("invalid index"));

    int n = (int)::SendMessage(GetBuddyHwnd(), LB_INSERTSTRING, pos, (LPARAM)item.c_str());
    if ( n == LB_ERR )
    {
        wxLogLastError(wxT("SendMessage(LB_INSERTSTRING)"));
    }

    return n;
}

void wxChoice::Delete(unsigned int n)
{
    wxCHECK_RET( IsValid(n), wxT("invalid item index in wxChoice::Delete") );

    if ( HasClientObjectData() )
    {
        delete GetClientObject(n);
    }

    ::SendMessage(GetBuddyHwnd(), LB_DELETESTRING, n, 0);
}

void wxChoice::Clear()
{
    Free();

    ::SendMessage(GetBuddyHwnd(), LB_RESETCONTENT, 0, 0);
}

void wxChoice::Free()
{
    if ( HasClientObjectData() )
    {
        unsigned int count = GetCount();
        for ( unsigned int n = 0; n < count; n++ )
        {
            delete GetClientObject(n);
        }
    }
}

// ----------------------------------------------------------------------------
// selection
// ----------------------------------------------------------------------------

int wxChoice::GetSelection() const
{
    return (int)::SendMessage(GetBuddyHwnd(), LB_GETCURSEL, 0, 0);
}

void wxChoice::SetSelection(int n)
{
    ::SendMessage(GetBuddyHwnd(), LB_SETCURSEL, n, 0);
}

// ----------------------------------------------------------------------------
// string list functions
// ----------------------------------------------------------------------------

unsigned int wxChoice::GetCount() const
{
    return (unsigned int)::SendMessage(GetBuddyHwnd(), LB_GETCOUNT, 0, 0);
}

int wxChoice::FindString(const wxString& s, bool bCase) const
{
    // back to base class search for not native search type
    if (bCase)
       return wxItemContainerImmutable::FindString( s, bCase );

    int pos = (int)::SendMessage(GetBuddyHwnd(), LB_FINDSTRINGEXACT,
                               (WPARAM)-1, (LPARAM)s.c_str());

    return pos == LB_ERR ? wxNOT_FOUND : pos;
}

void wxChoice::SetString(unsigned int n, const wxString& s)
{
    wxCHECK_RET( IsValid(n),
                 wxT("invalid item index in wxChoice::SetString") );

    // we have to delete and add back the string as there is no way to change a
    // string in place

    // we need to preserve the client data
    void *data;
    if ( m_clientDataItemsType != wxClientData_None )
    {
        data = DoGetItemClientData(n);
    }
    else // no client data
    {
        data = NULL;
    }

    ::SendMessage(GetBuddyHwnd(), LB_DELETESTRING, n, 0);
    ::SendMessage(GetBuddyHwnd(), LB_INSERTSTRING, n, (LPARAM)s.c_str() );

    if ( data )
    {
        DoSetItemClientData(n, data);
    }
    //else: it's already NULL by default
}

wxString wxChoice::GetString(unsigned int n) const
{
    int len = (int)::SendMessage(GetBuddyHwnd(), LB_GETTEXTLEN, n, 0);

    wxString str;
    if ( len != LB_ERR && len > 0 )
    {
        if ( ::SendMessage
               (
                GetBuddyHwnd(),
                LB_GETTEXT,
                n,
                (LPARAM)(wxChar *)wxStringBuffer(str, len)
                ) == LB_ERR )
        {
            wxLogLastError(wxT("SendMessage(LB_GETLBTEXT)"));
        }
    }

    return str;
}

// ----------------------------------------------------------------------------
// client data
// ----------------------------------------------------------------------------

void wxChoice::DoSetItemClientData(unsigned int n, void* clientData)
{
    if ( ::SendMessage(GetHwnd(), LB_SETITEMDATA,
                       n, (LPARAM)clientData) == LB_ERR )
    {
        wxLogLastError(wxT("LB_SETITEMDATA"));
    }
}

void* wxChoice::DoGetItemClientData(unsigned int n) const
{
    LPARAM rc = ::SendMessage(GetHwnd(), LB_GETITEMDATA, n, 0);
    if ( rc == LB_ERR )
    {
        wxLogLastError(wxT("LB_GETITEMDATA"));

        // unfortunately, there is no way to return an error code to the user
        rc = (LPARAM) NULL;
    }

    return (void *)rc;
}

void wxChoice::DoSetItemClientObject(unsigned int n, wxClientData* clientData)
{
    DoSetItemClientData(n, clientData);
}

wxClientData* wxChoice::DoGetItemClientObject(unsigned int n) const
{
    return (wxClientData *)DoGetItemClientData(n);
}

// ----------------------------------------------------------------------------
// size calculations
// ----------------------------------------------------------------------------

wxSize wxChoice::DoGetBestSize() const
{
    wxSize sizeBtn = GetBestSpinnerSize(IsVertical(GetWindowStyle()));
    sizeBtn.x += DEFAULT_ITEM_WIDTH + MARGIN_BETWEEN;

    int y;
    wxGetCharSize(GetHWND(), NULL, &y, GetFont());
    y = EDIT_HEIGHT_FROM_CHAR_HEIGHT(y);

    // JACS: we should always use the height calculated
    // from above, because otherwise we'll get a spin control
    // that's too big. So never use the height calculated
    // from wxSpinButton::DoGetBestSize().

    // if ( sizeBtn.y < y )
    {
        // make the text tall enough
        sizeBtn.y = y;
    }

    return sizeBtn;
}

void wxChoice::DoMoveWindow(int x, int y, int width, int height)
{
    int widthBtn = GetBestSpinnerSize(IsVertical(GetWindowStyle())).x;
    int widthText = width - widthBtn - MARGIN_BETWEEN;
    if ( widthText <= 0 )
    {
        wxLogDebug(_T("not enough space for wxSpinCtrl!"));
    }

    if ( !::MoveWindow(GetBuddyHwnd(), x, y, widthText, height, TRUE) )
    {
        wxLogLastError(wxT("MoveWindow(buddy)"));
    }

    x += widthText + MARGIN_BETWEEN;
    if ( !::MoveWindow(GetHwnd(), x, y, widthBtn, height, TRUE) )
    {
        wxLogLastError(wxT("MoveWindow"));
    }
}

// get total size of the control
void wxChoice::DoGetSize(int *x, int *y) const
{
    RECT spinrect, textrect, ctrlrect;
    GetWindowRect(GetHwnd(), &spinrect);
    GetWindowRect(GetBuddyHwnd(), &textrect);
    UnionRect(&ctrlrect, &textrect, &spinrect);

    if ( x )
        *x = ctrlrect.right - ctrlrect.left;
    if ( y )
        *y = ctrlrect.bottom - ctrlrect.top;
}

void wxChoice::DoGetPosition(int *x, int *y) const
{
    // hack: pretend that our HWND is the text control just for a moment
    WXHWND hWnd = GetHWND();
    wxConstCast(this, wxChoice)->m_hWnd = m_hwndBuddy;

    wxChoiceBase::DoGetPosition(x, y);

    wxConstCast(this, wxChoice)->m_hWnd = hWnd;
}

#endif // wxUSE_CHOICE && __SMARTPHONE__ && __WXWINCE__
