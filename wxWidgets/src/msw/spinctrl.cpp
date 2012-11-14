/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/spinctrl.cpp
// Purpose:     wxSpinCtrl class implementation for Win32
// Author:      Vadim Zeitlin
// Modified by:
// Created:     22.07.99
// RCS-ID:      $Id: spinctrl.cpp 55622 2008-09-14 19:56:14Z VZ $
// Copyright:   (c) 1999-2005 Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_SPINCTRL

#include "wx/spinctrl.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/event.h"
    #include "wx/textctrl.h"
#endif

#include "wx/msw/private.h"

#if wxUSE_TOOLTIPS
    #include "wx/tooltip.h"
#endif // wxUSE_TOOLTIPS

#include <limits.h>         // for INT_MIN

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxSpinCtrlStyle )

wxBEGIN_FLAGS( wxSpinCtrlStyle )
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

    wxFLAGS_MEMBER(wxSP_HORIZONTAL)
    wxFLAGS_MEMBER(wxSP_VERTICAL)
    wxFLAGS_MEMBER(wxSP_ARROW_KEYS)
    wxFLAGS_MEMBER(wxSP_WRAP)

wxEND_FLAGS( wxSpinCtrlStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxSpinCtrl, wxControl,"wx/spinbut.h")

wxBEGIN_PROPERTIES_TABLE(wxSpinCtrl)
    wxEVENT_RANGE_PROPERTY( Spin , wxEVT_SCROLL_TOP , wxEVT_SCROLL_CHANGED , wxSpinEvent )
    wxEVENT_PROPERTY( Updated , wxEVT_COMMAND_SPINCTRL_UPDATED , wxCommandEvent )
    wxEVENT_PROPERTY( TextUpdated , wxEVT_COMMAND_TEXT_UPDATED , wxCommandEvent )
    wxEVENT_PROPERTY( TextEnter , wxEVT_COMMAND_TEXT_ENTER , wxCommandEvent )

    wxPROPERTY( ValueString , wxString , SetValue , GetValue , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) ;
    wxPROPERTY( Value , int , SetValue, GetValue, 0 , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Min , int , SetMin, GetMin, 0, 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxPROPERTY( Max , int , SetMax, GetMax, 0 , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_FLAGS( WindowStyle , wxSpinCtrlStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
/*
    TODO PROPERTIES
        style wxSP_ARROW_KEYS
*/
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxSpinCtrl)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_6( wxSpinCtrl , wxWindow* , Parent , wxWindowID , Id , wxString , ValueString , wxPoint , Position , wxSize , Size , long , WindowStyle )
#else
IMPLEMENT_DYNAMIC_CLASS(wxSpinCtrl, wxControl)
#endif

BEGIN_EVENT_TABLE(wxSpinCtrl, wxSpinButton)
    EVT_CHAR(wxSpinCtrl::OnChar)

    EVT_SET_FOCUS(wxSpinCtrl::OnSetFocus)
    EVT_KILL_FOCUS(wxSpinCtrl::OnKillFocus)
    EVT_SPIN(wxID_ANY, wxSpinCtrl::OnSpinChange)
END_EVENT_TABLE()

#define GetBuddyHwnd()      (HWND)(m_hwndBuddy)

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the margin between the up-down control and its buddy (can be arbitrary,
// choose what you like - or may be decide during run-time depending on the
// font size?)
static const int MARGIN_BETWEEN = 1;

// ============================================================================
// implementation
// ============================================================================

wxArraySpins wxSpinCtrl::ms_allSpins;

// ----------------------------------------------------------------------------
// wnd proc for the buddy text ctrl
// ----------------------------------------------------------------------------

LRESULT APIENTRY _EXPORT wxBuddyTextWndProc(HWND hwnd,
                                            UINT message,
                                            WPARAM wParam,
                                            LPARAM lParam)
{
    wxSpinCtrl *spin = (wxSpinCtrl *)wxGetWindowUserData(hwnd);

    // forward some messages (mostly the key and focus ones) to the spin ctrl
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
#ifdef WM_HELP
        // we need to forward WM_HELP too to ensure that the context help
        // associated with wxSpinCtrl is shown when the text control part of it
        // is clicked with the "?" cursor
        case WM_HELP:
#endif
            spin->MSWWindowProc(message, wParam, lParam);

            // The control may have been deleted at this point, so check.
            if ( !::IsWindow(hwnd) || wxGetWindowUserData(hwnd) != spin )
                return 0;
            break;

        case WM_GETDLGCODE:
            if ( spin->HasFlag(wxTE_PROCESS_ENTER) )
            {
                long dlgCode = ::CallWindowProc
                                 (
                                    CASTWNDPROC spin->GetBuddyWndProc(),
                                    hwnd,
                                    message,
                                    wParam,
                                    lParam
                                 );
                dlgCode |= DLGC_WANTMESSAGE;
                return dlgCode;
            }
            break;
    }

    return ::CallWindowProc(CASTWNDPROC spin->GetBuddyWndProc(),
                            hwnd, message, wParam, lParam);
}

/* static */
wxSpinCtrl *wxSpinCtrl::GetSpinForTextCtrl(WXHWND hwndBuddy)
{
    wxSpinCtrl *spin = (wxSpinCtrl *)wxGetWindowUserData((HWND)hwndBuddy);

    int i = ms_allSpins.Index(spin);

    if ( i == wxNOT_FOUND )
        return NULL;

    // sanity check
    wxASSERT_MSG( spin->m_hwndBuddy == hwndBuddy,
                  _T("wxSpinCtrl has incorrect buddy HWND!") );

    return spin;
}

// process a WM_COMMAND generated by the buddy text control
bool wxSpinCtrl::ProcessTextCommand(WXWORD cmd, WXWORD WXUNUSED(id))
{
    if ( cmd == EN_CHANGE )
    {
        wxCommandEvent event(wxEVT_COMMAND_TEXT_UPDATED, GetId());
        event.SetEventObject(this);
        wxString val = wxGetWindowText(m_hwndBuddy);
        event.SetString(val);
        event.SetInt(GetValue());
        return GetEventHandler()->ProcessEvent(event);
    }

    // not processed
    return false;
}

void wxSpinCtrl::OnChar(wxKeyEvent& event)
{
    switch ( event.GetKeyCode() )
    {
        case WXK_RETURN:
            {
                wxCommandEvent event(wxEVT_COMMAND_TEXT_ENTER, m_windowId);
                InitCommandEvent(event);
                wxString val = wxGetWindowText(m_hwndBuddy);
                event.SetString(val);
                event.SetInt(GetValue());
                if ( GetEventHandler()->ProcessEvent(event) )
                    return;
                break;
            }

        case WXK_TAB:
            // always produce navigation event - even if we process TAB
            // ourselves the fact that we got here means that the user code
            // decided to skip processing of this TAB - probably to let it
            // do its default job.
            {
                wxNavigationKeyEvent eventNav;
                eventNav.SetDirection(!event.ShiftDown());
                eventNav.SetWindowChange(event.ControlDown());
                eventNav.SetEventObject(this);

                if ( GetParent()->GetEventHandler()->ProcessEvent(eventNav) )
                    return;
            }
            break;
    }

    // no, we didn't process it
    event.Skip();
}

void wxSpinCtrl::OnKillFocus(wxFocusEvent& event)
{
    // ensure that a correct value is shown by the control
    NormalizeValue();
    event.Skip();
}

void wxSpinCtrl::OnSetFocus(wxFocusEvent& event)
{
    // when we get focus, give it to our buddy window as it needs it more than
    // we do
    ::SetFocus((HWND)m_hwndBuddy);

    event.Skip();
}

void wxSpinCtrl::NormalizeValue()
{
    const int value = GetValue();
    const bool changed = value != m_oldValue;

    // notice that we have to call SetValue() even if the value didn't change
    // because otherwise we could be left with empty buddy control when value
    // is 0, see comment in SetValue()
    SetValue(value);

    if ( changed )
    {
        SendSpinUpdate(value);
    }
}

// ----------------------------------------------------------------------------
// construction
// ----------------------------------------------------------------------------

bool wxSpinCtrl::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxString& value,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        int min, int max, int initial,
                        const wxString& name)
{
    // this should be in ctor/init function but I don't want to add one to 2.8
    // to avoid problems with default ctor which can be inlined in the user
    // code and so might not get this fix without recompilation
    m_oldValue = INT_MIN;

    // before using DoGetBestSize(), have to set style to let the base class
    // know whether this is a horizontal or vertical control (we're always
    // vertical)
    style |= wxSP_VERTICAL;

    if ( (style & wxBORDER_MASK) == wxBORDER_DEFAULT )
#ifdef __WXWINCE__
        style |= wxBORDER_SIMPLE;
#else
        style |= wxBORDER_SUNKEN;
#endif

    SetWindowStyle(style);

    WXDWORD exStyle = 0;
    WXDWORD msStyle = MSWGetStyle(GetWindowStyle(), & exStyle) ;

    // calculate the sizes: the size given is the toal size for both controls
    // and we need to fit them both in the given width (height is the same)
    wxSize sizeText(size), sizeBtn(size);
    sizeBtn.x = wxSpinButton::DoGetBestSize().x;
    if ( sizeText.x <= 0 )
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

    // we must create the text control before the spin button for the purpose
    // of the dialog navigation: if there is a static text just before the spin
    // control, activating it by Alt-letter should give focus to the text
    // control, not the spin and the dialog navigation code will give focus to
    // the next control (at Windows level), not the one after it

    // create the text window

    m_hwndBuddy = (WXHWND)::CreateWindowEx
                    (
                     exStyle,                // sunken border
                     _T("EDIT"),             // window class
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


    // create the spin button
    if ( !wxSpinButton::Create(parent, id, posBtn, sizeBtn, style, name) )
    {
        return false;
    }

    wxSpinButtonBase::SetRange(min, max);

    // subclass the text ctrl to be able to intercept some events
    wxSetWindowUserData(GetBuddyHwnd(), this);
    m_wndProcBuddy = (WXFARPROC)wxSetWindowProc(GetBuddyHwnd(),
                                                wxBuddyTextWndProc);

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

    // associate the text window with the spin button
    (void)::SendMessage(GetHwnd(), UDM_SETBUDDY, (WPARAM)m_hwndBuddy, 0);

    SetValue(initial);

    // Set the range in the native control
    SetRange(min, max);

    if ( !value.empty() )
    {
        SetValue(value);
        m_oldValue = (int) wxAtol(value);
    }
    else
    {
        SetValue(wxString::Format(wxT("%d"), initial));
        m_oldValue = initial;
    }

    // do it after finishing with m_hwndBuddy creation to avoid generating
    // initial wxEVT_COMMAND_TEXT_UPDATED message
    ms_allSpins.Add(this);

    return true;
}

wxSpinCtrl::~wxSpinCtrl()
{
    ms_allSpins.Remove(this);

    // This removes spurious memory leak reporting
    if (ms_allSpins.GetCount() == 0)
        ms_allSpins.Clear();

    // destroy the buddy window because this pointer which wxBuddyTextWndProc
    // uses will not soon be valid any more
    ::DestroyWindow(GetBuddyHwnd());
}

// ----------------------------------------------------------------------------
// wxTextCtrl-like methods
// ----------------------------------------------------------------------------

void wxSpinCtrl::SetValue(const wxString& text)
{
    if ( !::SetWindowText(GetBuddyHwnd(), text.c_str()) )
    {
        wxLogLastError(wxT("SetWindowText(buddy)"));
    }
}

void  wxSpinCtrl::SetValue(int val)
{
    wxSpinButton::SetValue(val);

    // normally setting the value of the spin button is enough as it updates
    // its buddy control automatically ...
    if ( wxGetWindowText(m_hwndBuddy).empty() )
    {
        // ... but sometimes it doesn't, notably when the value is 0 and the
        // text control is currently empty, the spin button seems to be happy
        // to leave it like this, while we really want to always show the
        // current value in the control, so do it manually
        ::SetWindowText(GetBuddyHwnd(), wxString::Format(_T("%d"), val));
    }

    m_oldValue = GetValue();
}

int wxSpinCtrl::GetValue() const
{
    wxString val = wxGetWindowText(m_hwndBuddy);

    long n;
    if ( (wxSscanf(val, wxT("%ld"), &n) != 1) )
        n = INT_MIN;

    if ( n < m_min )
        n = m_min;
    if ( n > m_max )
        n = m_max;

    return n;
}

void wxSpinCtrl::SetSelection(long from, long to)
{
    // if from and to are both -1, it means (in wxWidgets) that all text should
    // be selected - translate into Windows convention
    if ( (from == -1) && (to == -1) )
    {
        from = 0;
    }

    ::SendMessage(GetBuddyHwnd(), EM_SETSEL, (WPARAM)from, (LPARAM)to);
}

// ----------------------------------------------------------------------------
// forward some methods to subcontrols
// ----------------------------------------------------------------------------

bool wxSpinCtrl::SetFont(const wxFont& font)
{
    if ( !wxWindowBase::SetFont(font) )
    {
        // nothing to do
        return false;
    }

    WXHANDLE hFont = GetFont().GetResourceHandle();
    (void)::SendMessage(GetBuddyHwnd(), WM_SETFONT, (WPARAM)hFont, TRUE);

    return true;
}

bool wxSpinCtrl::Show(bool show)
{
    if ( !wxControl::Show(show) )
    {
        return false;
    }

    ::ShowWindow(GetBuddyHwnd(), show ? SW_SHOW : SW_HIDE);

    return true;
}

bool wxSpinCtrl::Reparent(wxWindowBase *newParent)
{
    // Reparenting both the updown control and its buddy does not seem to work:
    // they continue to be connected somehow, but visually there is no feedback
    // on the buddy edit control. To avoid this problem, we reparent the buddy
    // window normally, but we recreate the updown control and reassign its
    // buddy.

    if ( !wxWindowBase::Reparent(newParent) )
        return false;

    newParent->GetChildren().DeleteObject(this);

    // preserve the old values
    const wxSize size = GetSize();
    int value = GetValue();
    const wxRect btnRect = wxRectFromRECT(wxGetWindowRect(GetHwnd()));

    // destroy the old spin button
    UnsubclassWin();
    if ( !::DestroyWindow(GetHwnd()) )
        wxLogLastError(wxT("DestroyWindow"));

    // create and initialize the new one
    if ( !wxSpinButton::Create(GetParent(), GetId(),
                               btnRect.GetPosition(), btnRect.GetSize(),
                               GetWindowStyle(), GetName()) )
        return false;

    SetValue(value);
    SetRange(m_min, m_max);
    SetInitialSize(size);

    // associate it with the buddy control again
    ::SetParent(GetBuddyHwnd(), GetHwndOf(GetParent()));
    (void)::SendMessage(GetHwnd(), UDM_SETBUDDY, (WPARAM)GetBuddyHwnd(), 0);

    return true;
}

bool wxSpinCtrl::Enable(bool enable)
{
    if ( !wxControl::Enable(enable) )
    {
        return false;
    }

    ::EnableWindow(GetBuddyHwnd(), enable);

    return true;
}

void wxSpinCtrl::SetFocus()
{
    ::SetFocus(GetBuddyHwnd());
}

#if wxUSE_TOOLTIPS

void wxSpinCtrl::DoSetToolTip(wxToolTip *tip)
{
    wxSpinButton::DoSetToolTip(tip);

    if ( tip )
        tip->Add(m_hwndBuddy);
}

#endif // wxUSE_TOOLTIPS

// ----------------------------------------------------------------------------
// events processing and generation
// ----------------------------------------------------------------------------

void wxSpinCtrl::SendSpinUpdate(int value)
{
    wxCommandEvent event(wxEVT_COMMAND_SPINCTRL_UPDATED, GetId());
    event.SetEventObject(this);
    event.SetInt(value);

    (void)GetEventHandler()->ProcessEvent(event);

    m_oldValue = value;
}

void wxSpinCtrl::OnSpinChange(wxSpinEvent& eventSpin)
{
    const int value = eventSpin.GetPosition();
    if ( value != m_oldValue )
    {
        SendSpinUpdate(value);
    }
}

// ----------------------------------------------------------------------------
// size calculations
// ----------------------------------------------------------------------------

wxSize wxSpinCtrl::DoGetBestSize() const
{
    wxSize sizeBtn = wxSpinButton::DoGetBestSize();
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

void wxSpinCtrl::DoMoveWindow(int x, int y, int width, int height)
{
    int widthBtn = wxSpinButton::DoGetBestSize().x;
    int widthText = width - widthBtn - MARGIN_BETWEEN;
    if ( widthText <= 0 )
    {
        wxLogDebug(_T("not enough space for wxSpinCtrl!"));
    }

    // 1) The buddy window
    DoMoveSibling(m_hwndBuddy, x, y, widthText, height);

    // 2) The button window
    x += widthText + MARGIN_BETWEEN;
    wxSpinButton::DoMoveWindow(x, y, widthBtn, height);
}

// get total size of the control
void wxSpinCtrl::DoGetSize(int *x, int *y) const
{
    RECT spinrect, textrect, ctrlrect;
    GetWindowRect(GetHwnd(), &spinrect);
    GetWindowRect(GetBuddyHwnd(), &textrect);
    UnionRect(&ctrlrect,&textrect, &spinrect);

    if ( x )
        *x = ctrlrect.right - ctrlrect.left;
    if ( y )
        *y = ctrlrect.bottom - ctrlrect.top;
}

void wxSpinCtrl::DoGetClientSize(int *x, int *y) const
{
    RECT spinrect = wxGetClientRect(GetHwnd());
    RECT textrect = wxGetClientRect(GetBuddyHwnd());
    RECT ctrlrect;
    UnionRect(&ctrlrect,&textrect, &spinrect);

    if ( x )
        *x = ctrlrect.right - ctrlrect.left;
    if ( y )
        *y = ctrlrect.bottom - ctrlrect.top;
}

void wxSpinCtrl::DoGetPosition(int *x, int *y) const
{
    // hack: pretend that our HWND is the text control just for a moment
    WXHWND hWnd = GetHWND();
    wxConstCast(this, wxSpinCtrl)->m_hWnd = m_hwndBuddy;

    wxSpinButton::DoGetPosition(x, y);

    wxConstCast(this, wxSpinCtrl)->m_hWnd = hWnd;
}

#endif // wxUSE_SPINCTRL
