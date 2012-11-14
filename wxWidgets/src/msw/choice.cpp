/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/choice.cpp
// Purpose:     wxChoice
// Author:      Julian Smart
// Modified by: Vadim Zeitlin to derive from wxChoiceBase
// Created:     04/01/98
// RCS-ID:      $Id: choice.cpp 51616 2008-02-09 15:22:15Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

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

#if wxUSE_CHOICE && !(defined(__SMARTPHONE__) && defined(__WXWINCE__))

#include "wx/choice.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/log.h"
    #include "wx/brush.h"
    #include "wx/settings.h"
#endif

#include "wx/msw/private.h"

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxChoiceStyle )

wxBEGIN_FLAGS( wxChoiceStyle )
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

wxEND_FLAGS( wxChoiceStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxChoice, wxControl,"wx/choice.h")

wxBEGIN_PROPERTIES_TABLE(wxChoice)
    wxEVENT_PROPERTY( Select , wxEVT_COMMAND_CHOICE_SELECTED , wxCommandEvent )

    wxPROPERTY( Font , wxFont , SetFont , GetFont  , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_COLLECTION( Choices , wxArrayString , wxString , AppendString , GetStrings , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Selection ,int, SetSelection, GetSelection, EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_FLAGS( WindowStyle , wxChoiceStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxChoice)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_4( wxChoice , wxWindow* , Parent , wxWindowID , Id , wxPoint , Position , wxSize , Size )
#else
IMPLEMENT_DYNAMIC_CLASS(wxChoice, wxControl)
#endif
/*
    TODO PROPERTIES
        selection (long)
        content (list)
            item
*/

// ============================================================================
// implementation
// ============================================================================

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
    // Experience shows that wxChoice vs. wxComboBox distinction confuses
    // quite a few people - try to help them
    wxASSERT_MSG( !(style & wxCB_DROPDOWN) &&
                  !(style & wxCB_READONLY) &&
                  !(style & wxCB_SIMPLE),
                  _T("this style flag is ignored by wxChoice, you ")
                  _T("probably want to use a wxComboBox") );

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
    // initialize wxControl
    if ( !CreateControl(parent, id, pos, size, style, validator, name) )
        return false;

    // now create the real HWND
    if ( !MSWCreateControl(wxT("COMBOBOX"), wxEmptyString, pos, size) )
        return false;


    // choice/combobox normally has "white" (depends on colour scheme, of
    // course) background rather than inheriting the parent's background
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    // initialize the controls contents
    for ( int i = 0; i < n; i++ )
    {
        Append(choices[i]);
    }

    // and now we may finally size the control properly (if needed)
    SetInitialSize(size);

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

void wxChoice::SetLabel(const wxString& label)
{
    if ( FindString(label) == wxNOT_FOUND )
    {
        // unless we explicitly do this here, CB_GETCURSEL will continue to
        // return the index of the previously selected item which will result
        // in wrongly replacing the value being set now with the previously
        // value if the user simply opens and closes (without selecting
        // anything) the combobox popup
        SetSelection(-1);
    }

    wxChoiceBase::SetLabel(label);
}

bool wxChoice::MSWShouldPreProcessMessage(WXMSG *pMsg)
{
    MSG *msg = (MSG *) pMsg;

    // if the dropdown list is visible, don't preprocess certain keys
    if ( msg->message == WM_KEYDOWN
        && (msg->wParam == VK_ESCAPE || msg->wParam == VK_RETURN) )
    {
        if (::SendMessage(GetHwndOf(this), CB_GETDROPPEDSTATE, 0, 0))
        {
            return false;
        }
    }

    return wxControl::MSWShouldPreProcessMessage(pMsg);
}

WXDWORD wxChoice::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    // we never have an external border
    WXDWORD msStyle = wxControl::MSWGetStyle
                      (
                        (style & ~wxBORDER_MASK) | wxBORDER_NONE, exstyle
                      );

    // WS_CLIPSIBLINGS is useful with wxChoice and doesn't seem to result in
    // any problems
    msStyle |= WS_CLIPSIBLINGS;

    // wxChoice-specific styles
    msStyle |= CBS_DROPDOWNLIST | WS_HSCROLL | WS_VSCROLL;
    if ( style & wxCB_SORT )
        msStyle |= CBS_SORT;

    return msStyle;
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
    int n = (int)SendMessage(GetHwnd(), CB_ADDSTRING, 0, (LPARAM)item.c_str());
    if ( n == CB_ERR )
    {
        wxLogLastError(wxT("SendMessage(CB_ADDSTRING)"));
    }
    else // ok
    {
        // we need to refresh our size in order to have enough space for the
        // newly added items
        if ( !IsFrozen() )
            UpdateVisibleHeight();
    }

    InvalidateBestSize();
    return n;
}

int wxChoice::DoInsert(const wxString& item, unsigned int pos)
{
    wxCHECK_MSG(!(GetWindowStyle() & wxCB_SORT), -1, wxT("can't insert into sorted list"));
    wxCHECK_MSG(IsValidInsert(pos), -1, wxT("invalid index"));

    int n = (int)SendMessage(GetHwnd(), CB_INSERTSTRING, pos, (LPARAM)item.c_str());
    if ( n == CB_ERR )
    {
        wxLogLastError(wxT("SendMessage(CB_INSERTSTRING)"));
    }
    else // ok
    {
        if ( !IsFrozen() )
            UpdateVisibleHeight();
    }

    InvalidateBestSize();
    return n;
}

void wxChoice::Delete(unsigned int n)
{
    wxCHECK_RET( IsValid(n), wxT("invalid item index in wxChoice::Delete") );

    if ( HasClientObjectData() )
    {
        delete GetClientObject(n);
    }

    SendMessage(GetHwnd(), CB_DELETESTRING, n, 0);

    if ( !IsFrozen() )
        UpdateVisibleHeight();

    InvalidateBestSize();
}

void wxChoice::Clear()
{
    Free();

    SendMessage(GetHwnd(), CB_RESETCONTENT, 0, 0);

    if ( !IsFrozen() )
        UpdateVisibleHeight();

    InvalidateBestSize();
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
    // if m_lastAcceptedSelection is set, it means that the dropdown is
    // currently shown and that we want to use the last "permanent" selection
    // instead of whatever is under the mouse pointer currently
    //
    // otherwise, get the selection from the control
    return m_lastAcceptedSelection == wxID_NONE ? GetCurrentSelection()
                                                : m_lastAcceptedSelection;
}

int wxChoice::GetCurrentSelection() const
{
    return (int)SendMessage(GetHwnd(), CB_GETCURSEL, 0, 0);
}

void wxChoice::SetSelection(int n)
{
    SendMessage(GetHwnd(), CB_SETCURSEL, n, 0);
}

// ----------------------------------------------------------------------------
// string list functions
// ----------------------------------------------------------------------------

unsigned int wxChoice::GetCount() const
{
    return (unsigned int)SendMessage(GetHwnd(), CB_GETCOUNT, 0, 0);
}

int wxChoice::FindString(const wxString& s, bool bCase) const
{
#if defined(__WATCOMC__) && defined(__WIN386__)
    // For some reason, Watcom in WIN386 mode crashes in the CB_FINDSTRINGEXACT message.
    // wxChoice::Do it the long way instead.
    unsigned int count = GetCount();
    for ( unsigned int i = 0; i < count; i++ )
    {
        // as CB_FINDSTRINGEXACT is case insensitive, be case insensitive too
        if (GetString(i).IsSameAs(s, bCase))
            return i;
    }

    return wxNOT_FOUND;
#else // !Watcom
   //TODO:  Evidently some MSW versions (all?) don't like empty strings
   //passed to SendMessage, so we have to do it ourselves in that case
   if ( s.empty() )
   {
       unsigned int count = GetCount();
       for ( unsigned int i = 0; i < count; i++ )
       {
         if (GetString(i).empty())
             return i;
       }

       return wxNOT_FOUND;
   }
   else if (bCase)
   {
       // back to base class search for not native search type
       return wxItemContainerImmutable::FindString( s, bCase );
   }
   else
   {
       int pos = (int)SendMessage(GetHwnd(), CB_FINDSTRINGEXACT,
                                  (WPARAM)-1, (LPARAM)s.c_str());

       return pos == LB_ERR ? wxNOT_FOUND : pos;
   }
#endif // Watcom/!Watcom
}

void wxChoice::SetString(unsigned int n, const wxString& s)
{
    wxCHECK_RET( IsValid(n), wxT("invalid item index in wxChoice::SetString") );

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

    ::SendMessage(GetHwnd(), CB_DELETESTRING, n, 0);
    ::SendMessage(GetHwnd(), CB_INSERTSTRING, n, (LPARAM)s.c_str() );

    if ( data )
    {
        DoSetItemClientData(n, data);
    }
    //else: it's already NULL by default

    InvalidateBestSize();
}

wxString wxChoice::GetString(unsigned int n) const
{
    int len = (int)::SendMessage(GetHwnd(), CB_GETLBTEXTLEN, n, 0);

    wxString str;
    if ( len != CB_ERR && len > 0 )
    {
        if ( ::SendMessage
               (
                GetHwnd(),
                CB_GETLBTEXT,
                n,
                (LPARAM)(wxChar *)wxStringBuffer(str, len)
               ) == CB_ERR )
        {
            wxLogLastError(wxT("SendMessage(CB_GETLBTEXT)"));
        }
    }

    return str;
}

// ----------------------------------------------------------------------------
// client data
// ----------------------------------------------------------------------------

void wxChoice::DoSetItemClientData(unsigned int n, void* clientData)
{
    if ( ::SendMessage(GetHwnd(), CB_SETITEMDATA,
                       n, (LPARAM)clientData) == CB_ERR )
    {
        wxLogLastError(wxT("CB_SETITEMDATA"));
    }
}

void* wxChoice::DoGetItemClientData(unsigned int n) const
{
    LPARAM rc = SendMessage(GetHwnd(), CB_GETITEMDATA, n, 0);
    if ( rc == CB_ERR )
    {
        wxLogLastError(wxT("CB_GETITEMDATA"));

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
// wxMSW specific helpers
// ----------------------------------------------------------------------------

void wxChoice::UpdateVisibleHeight()
{
    // be careful to not change the width here
    DoSetSize(wxDefaultCoord, wxDefaultCoord, wxDefaultCoord, GetSize().y, wxSIZE_USE_EXISTING);
}

void wxChoice::DoMoveWindow(int x, int y, int width, int height)
{
    // here is why this is necessary: if the width is negative, the combobox
    // window proc makes the window of the size width*height instead of
    // interpreting height in the usual manner (meaning the height of the drop
    // down list - usually the height specified in the call to MoveWindow()
    // will not change the height of combo box per se)
    //
    // this behaviour is not documented anywhere, but this is just how it is
    // here (NT 4.4) and, anyhow, the check shouldn't hurt - however without
    // the check, constraints/sizers using combos may break the height
    // constraint will have not at all the same value as expected
    if ( width < 0 )
        return;

    wxControl::DoMoveWindow(x, y, width, height);
}

void wxChoice::DoGetSize(int *w, int *h) const
{
    // this is weird: sometimes, the height returned by Windows is clearly the
    // total height of the control including the drop down list -- but only
    // sometimes, and normally it isn't... I have no idea about what to do with
    // this
    wxControl::DoGetSize(w, h);
}

void wxChoice::DoSetSize(int x, int y,
                         int width, int height,
                         int sizeFlags)
{
    int heightOrig = height;
    
    // the height which we must pass to Windows should be the total height of
    // the control including the drop down list while the height given to us
    // is, of course, just the height of the permanently visible part of it
    if ( height != wxDefaultCoord )
    {
        // don't make the drop down list too tall, arbitrarily limit it to 40
        // items max and also don't leave it empty
        size_t nItems = GetCount();
        if ( !nItems )
            nItems = 9;
        else if ( nItems > 24 )
            nItems = 24;

        // add space for the drop down list
        const int hItem = SendMessage(GetHwnd(), CB_GETITEMHEIGHT, 0, 0);
        height += hItem*(nItems + 1);
    }
    else
    {
        // We cannot pass wxDefaultCoord as height to wxControl. wxControl uses
        // wxGetWindowRect() to determine the current height of the combobox,
        // and then again sets the combobox's height to that value. Unfortunately,
        // wxGetWindowRect doesn't include the dropdown list's height (at least
        // on Win2K), so this would result in a combobox with dropdown height of
        // 1 pixel. We have to determine the default height ourselves and call
        // wxControl with that value instead.
        int w, h;
        RECT r;
        DoGetSize(&w, &h);
        if (::SendMessage(GetHwnd(), CB_GETDROPPEDCONTROLRECT, 0, (LPARAM) &r) != 0)
        {
            height = h + r.bottom - r.top;
        }
    }

    wxControl::DoSetSize(x, y, width, height, sizeFlags);

    // If we're storing a pending size, make sure we store
    // the original size for reporting back to the app.
    if (m_pendingSize != wxDefaultSize)
        m_pendingSize = wxSize(width, heightOrig);

    // This solution works on XP, but causes choice/combobox lists to be
    // too short on W2K and earlier.
#if 0
    int widthCurrent, heightCurrent;
    DoGetSize(&widthCurrent, &heightCurrent);

    // the height which we must pass to Windows should be the total height of
    // the control including the drop down list while the height given to us
    // is, of course, just the height of the permanently visible part of it
    if ( height != wxDefaultCoord && height != heightCurrent )
    {
        // don't make the drop down list too tall, arbitrarily limit it to 40
        // items max and also don't leave it empty
        unsigned int nItems = GetCount();
        if ( !nItems )
            nItems = 9;
        else if ( nItems > 24 )
            nItems = 24;

        // add space for the drop down list
        const int hItem = SendMessage(GetHwnd(), CB_GETITEMHEIGHT, 0, 0);
        height += hItem*(nItems + 1);
    }
    else // keep the same height as now
    {
        // normally wxWindow::DoSetSize() checks if we set the same size as the
        // window already has and does nothing in this case, but for us the
        // check fails as the size we pass to it includes the dropdown while
        // the size returned by our GetSize() does not, so test if the size
        // didn't really change ourselves here
        if ( width == wxDefaultCoord || width == widthCurrent )
        {
            // size doesn't change, what about position?
            int xCurrent, yCurrent;
            DoGetPosition(&xCurrent, &yCurrent);
            const bool defMeansUnchanged = !(sizeFlags & wxSIZE_ALLOW_MINUS_ONE);
            if ( ((x == wxDefaultCoord && defMeansUnchanged) || x == xCurrent)
                    &&
                 ((y == wxDefaultCoord && defMeansUnchanged) || y == yCurrent) )
            {
                // nothing changes, nothing to do
                return;
            }
        }

        // We cannot pass wxDefaultCoord as height to wxControl. wxControl uses
        // wxGetWindowRect() to determine the current height of the combobox,
        // and then again sets the combobox's height to that value. Unfortunately,
        // wxGetWindowRect doesn't include the dropdown list's height (at least
        // on Win2K), so this would result in a combobox with dropdown height of
        // 1 pixel. We have to determine the default height ourselves and call
        // wxControl with that value instead.
        //
        // Also notice that sometimes CB_GETDROPPEDCONTROLRECT seems to return
        // wildly incorrect values (~32000) which looks like a bug in it, just
        // ignore them in this case
        RECT r;
        if ( ::SendMessage(GetHwnd(), CB_GETDROPPEDCONTROLRECT, 0, (LPARAM) &r)
                    && r.bottom < 30000 )
        {
            height = heightCurrent + r.bottom - r.top;
        }
    }

    wxControl::DoSetSize(x, y, width, height, sizeFlags);
#endif
}

wxSize wxChoice::DoGetBestSize() const
{
    // find the widest string
    int wChoice = 0;
    const unsigned int nItems = GetCount();
    for ( unsigned int i = 0; i < nItems; i++ )
    {
        int wLine;
        GetTextExtent(GetString(i), &wLine, NULL);
        if ( wLine > wChoice )
            wChoice = wLine;
    }

    // give it some reasonable default value if there are no strings in the
    // list
    if ( wChoice == 0 )
        wChoice = 100;

    // the combobox should be slightly larger than the widest string
    wChoice += 5*GetCharWidth();

    wxSize best(wChoice, EDIT_HEIGHT_FROM_CHAR_HEIGHT(GetCharHeight()));
    CacheBestSize(best);
    return best;
}

WXLRESULT wxChoice::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    switch ( nMsg )
    {
        case WM_LBUTTONUP:
            {
                int x = (int)LOWORD(lParam);
                int y = (int)HIWORD(lParam);

                // Ok, this is truly weird, but if a panel with a wxChoice
                // loses the focus, then you get a *fake* WM_LBUTTONUP message
                // with x = 65535 and y = 65535. Filter out this nonsense.
                //
                // VZ: I'd like to know how to reproduce this please...
                if ( x == 65535 && y == 65535 )
                    return 0;
            }
            break;

            // we have to handle both: one for the normal case and the other
            // for readonly
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORSTATIC:
            {
                WXHDC hdc;
                WXHWND hwnd;
                UnpackCtlColor(wParam, lParam, &hdc, &hwnd);

                WXHBRUSH hbr = MSWControlColor((WXHDC)hdc, hwnd);
                if ( hbr )
                    return (WXLRESULT)hbr;
                //else: fall through to default window proc
            }
    }

    return wxWindow::MSWWindowProc(nMsg, wParam, lParam);
}

bool wxChoice::MSWCommand(WXUINT param, WXWORD WXUNUSED(id))
{
    /*
        The native control provides a great variety in the events it sends in
        the different selection scenarios (undoubtedly for greater amusement of
        the programmers using it). For the reference, here are the cases when
        the final selection is accepted (things are quite interesting when it
        is cancelled too):

        A. Selecting with just the arrows without opening the dropdown:
            1. CBN_SELENDOK
            2. CBN_SELCHANGE

        B. Opening dropdown with F4 and selecting with arrows:
            1. CBN_DROPDOWN
            2. many CBN_SELCHANGE while changing selection in the list
            3. CBN_SELENDOK
            4. CBN_CLOSEUP

        C. Selecting with the mouse:
            1. CBN_DROPDOWN
            -- no intermediate CBN_SELCHANGEs --
            2. CBN_SELENDOK
            3. CBN_CLOSEUP
            4. CBN_SELCHANGE

        Admire the different order of messages in all of those cases, it must
        surely have taken a lot of effort to Microsoft developers to achieve
        such originality.
     */
    switch ( param )
    {
        case CBN_DROPDOWN:
            // we use this value both because we don't want to track selection
            // using CB_GETCURSEL while the dropdown is opened and because we
            // need to reset the selection back to it if it's eventually
            // cancelled by user
            m_lastAcceptedSelection = GetCurrentSelection();
            if ( m_lastAcceptedSelection == -1 )
            {
                // no current selection so no need to restore it later (this
                // happens when opening a combobox containing text not from its
                // list of items and we shouldn't erase this text)
                m_lastAcceptedSelection = wxID_NONE;
            }
            break;

        case CBN_CLOSEUP:
            // if the selection was accepted by the user, it should have been
            // reset to wxID_NONE by CBN_SELENDOK, otherwise the selection was
            // cancelled and we must restore the old one
            if ( m_lastAcceptedSelection != wxID_NONE )
            {
                SetSelection(m_lastAcceptedSelection);
                m_lastAcceptedSelection = wxID_NONE;
            }
            break;

        case CBN_SELENDOK:
            // reset it to prevent CBN_CLOSEUP from undoing the selection (it's
            // ok to reset it now as GetCurrentSelection() will now return the
            // same thing anyhow)
            m_lastAcceptedSelection = wxID_NONE;

            {
                const int n = GetSelection();

                wxCommandEvent event(wxEVT_COMMAND_CHOICE_SELECTED, m_windowId);
                event.SetInt(n);
                event.SetEventObject(this);

                if ( n > -1 )
                {
                    event.SetString(GetStringSelection());
                    InitCommandEventWithItems(event, n);
                }

                ProcessCommand(event);
            }
            break;

        // don't handle CBN_SELENDCANCEL: just leave m_lastAcceptedSelection
        // valid and the selection will be undone in CBN_CLOSEUP above

        // don't handle CBN_SELCHANGE neither, we don't want to generate events
        // while the dropdown is opened -- but do add it if we ever need this

        default:
            return false;
    }

    return true;
}

WXHBRUSH wxChoice::MSWControlColor(WXHDC hDC, WXHWND hWnd)
{
    if ( !IsEnabled() )
        return MSWControlColorDisabled(hDC);

    return wxChoiceBase::MSWControlColor(hDC, hWnd);
}

#endif // wxUSE_CHOICE && !(__SMARTPHONE__ && __WXWINCE__)
