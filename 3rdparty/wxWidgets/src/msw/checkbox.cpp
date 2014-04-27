/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/checkbox.cpp
// Purpose:     wxCheckBox
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: checkbox.cpp 40331 2006-07-25 18:47:39Z VZ $
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

#if wxUSE_CHECKBOX

#include "wx/checkbox.h"

#ifndef WX_PRECOMP
    #include "wx/brush.h"
    #include "wx/dcscreen.h"
    #include "wx/settings.h"
#endif

#include "wx/msw/uxtheme.h"
#include "wx/msw/private.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#ifndef BST_UNCHECKED
    #define BST_UNCHECKED 0x0000
#endif

#ifndef BST_CHECKED
    #define BST_CHECKED 0x0001
#endif

#ifndef BST_INDETERMINATE
    #define BST_INDETERMINATE 0x0002
#endif

#ifndef DFCS_HOT
    #define DFCS_HOT 0x1000
#endif

#ifndef DT_HIDEPREFIX
    #define DT_HIDEPREFIX 0x00100000
#endif

#ifndef BP_CHECKBOX
    #define BP_CHECKBOX 3
#endif

// these values are defined in tmschema.h (except the first one)
enum
{
    CBS_INVALID,
    CBS_UNCHECKEDNORMAL,
    CBS_UNCHECKEDHOT,
    CBS_UNCHECKEDPRESSED,
    CBS_UNCHECKEDDISABLED,
    CBS_CHECKEDNORMAL,
    CBS_CHECKEDHOT,
    CBS_CHECKEDPRESSED,
    CBS_CHECKEDDISABLED,
    CBS_MIXEDNORMAL,
    CBS_MIXEDHOT,
    CBS_MIXEDPRESSED,
    CBS_MIXEDDISABLED
};

// these are our own
enum
{
    CBS_HOT_OFFSET = 1,
    CBS_PRESSED_OFFSET = 2,
    CBS_DISABLED_OFFSET = 3
};

// ============================================================================
// implementation
// ============================================================================

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxCheckBoxStyle )

wxBEGIN_FLAGS( wxCheckBoxStyle )
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
    wxFLAGS_MEMBER(wxNO_BORDER)

    // standard window styles
    wxFLAGS_MEMBER(wxTAB_TRAVERSAL)
    wxFLAGS_MEMBER(wxCLIP_CHILDREN)
    wxFLAGS_MEMBER(wxTRANSPARENT_WINDOW)
    wxFLAGS_MEMBER(wxWANTS_CHARS)
    wxFLAGS_MEMBER(wxNO_FULL_REPAINT_ON_RESIZE)
    wxFLAGS_MEMBER(wxALWAYS_SHOW_SB )
    wxFLAGS_MEMBER(wxVSCROLL)
    wxFLAGS_MEMBER(wxHSCROLL)

wxEND_FLAGS( wxCheckBoxStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxCheckBox, wxControl,"wx/checkbox.h")

wxBEGIN_PROPERTIES_TABLE(wxCheckBox)
    wxEVENT_PROPERTY( Click , wxEVT_COMMAND_CHECKBOX_CLICKED , wxCommandEvent )

    wxPROPERTY( Font , wxFont , SetFont , GetFont , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Label,wxString, SetLabel, GetLabel, wxString() , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Value ,bool, SetValue, GetValue, EMPTY_MACROVALUE, 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_FLAGS( WindowStyle , wxCheckBoxStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE, 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxCheckBox)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_6( wxCheckBox , wxWindow* , Parent , wxWindowID , Id , wxString , Label , wxPoint , Position , wxSize , Size , long , WindowStyle )
#else
IMPLEMENT_DYNAMIC_CLASS(wxCheckBox, wxControl)
#endif


// ----------------------------------------------------------------------------
// wxCheckBox creation
// ----------------------------------------------------------------------------

void wxCheckBox::Init()
{
    m_state = wxCHK_UNCHECKED;
    m_isPressed =
    m_isHot = false;
}

bool wxCheckBox::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxString& label,
                        const wxPoint& pos,
                        const wxSize& size, long style,
                        const wxValidator& validator,
                        const wxString& name)
{
    Init();

    if ( !CreateControl(parent, id, pos, size, style, validator, name) )
        return false;

    long msStyle = WS_TABSTOP;

    if ( style & wxCHK_3STATE )
    {
        msStyle |= BS_3STATE;
    }
    else
    {
        wxASSERT_MSG( !Is3rdStateAllowedForUser(),
            wxT("Using wxCH_ALLOW_3RD_STATE_FOR_USER")
            wxT(" style flag for a 2-state checkbox is useless") );
        msStyle |= BS_CHECKBOX;
    }

    if ( style & wxALIGN_RIGHT )
    {
        msStyle |= BS_LEFTTEXT | BS_RIGHT;
    }

    return MSWCreateControl(wxT("BUTTON"), msStyle, pos, size, label, 0);
}

// ----------------------------------------------------------------------------
// wxCheckBox geometry
// ----------------------------------------------------------------------------

wxSize wxCheckBox::DoGetBestSize() const
{
    static int s_checkSize = 0;

    if ( !s_checkSize )
    {
        wxScreenDC dc;
        dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

        s_checkSize = dc.GetCharHeight();
    }

    wxString str = wxGetWindowText(GetHWND());

    int wCheckbox, hCheckbox;
    if ( !str.empty() )
    {
        GetTextExtent(GetLabelText(str), &wCheckbox, &hCheckbox);
        wCheckbox += s_checkSize + GetCharWidth();

        if ( hCheckbox < s_checkSize )
            hCheckbox = s_checkSize;
    }
    else
    {
        wCheckbox = s_checkSize;
        hCheckbox = s_checkSize;
    }
#ifdef __WXWINCE__
    hCheckbox += 1;
#endif

    wxSize best(wCheckbox, hCheckbox);
    CacheBestSize(best);
    return best;
}

// ----------------------------------------------------------------------------
// wxCheckBox operations
// ----------------------------------------------------------------------------

void wxCheckBox::SetValue(bool val)
{
    Set3StateValue(val ? wxCHK_CHECKED : wxCHK_UNCHECKED);
}

bool wxCheckBox::GetValue() const
{
    return Get3StateValue() != wxCHK_UNCHECKED;
}

void wxCheckBox::Command(wxCommandEvent& event)
{
    int state = event.GetInt();
    wxCHECK_RET( (state == wxCHK_UNCHECKED) || (state == wxCHK_CHECKED)
        || (state == wxCHK_UNDETERMINED),
        wxT("event.GetInt() returned an invalid checkbox state") );

    Set3StateValue((wxCheckBoxState) state);
    ProcessCommand(event);
}

wxCOMPILE_TIME_ASSERT(wxCHK_UNCHECKED == BST_UNCHECKED
    && wxCHK_CHECKED == BST_CHECKED
    && wxCHK_UNDETERMINED == BST_INDETERMINATE, EnumValuesIncorrect);

void wxCheckBox::DoSet3StateValue(wxCheckBoxState state)
{
    m_state = state;
    if ( !IsOwnerDrawn() )
        ::SendMessage(GetHwnd(), BM_SETCHECK, (WPARAM) state, 0);
    else // owner drawn buttons don't react to this message
        Refresh();
}

wxCheckBoxState wxCheckBox::DoGet3StateValue() const
{
    return m_state;
}

bool wxCheckBox::MSWCommand(WXUINT cmd, WXWORD WXUNUSED(id))
{
    if ( cmd != BN_CLICKED && cmd != BN_DBLCLK )
        return false;

    // first update the value so that user event handler gets the new checkbox
    // value

    // ownerdrawn buttons don't manage their state themselves unlike usual
    // auto checkboxes so do it ourselves in any case
    wxCheckBoxState state;
    if ( Is3rdStateAllowedForUser() )
    {
        state = (wxCheckBoxState)((m_state + 1) % 3);
    }
    else // 2 state checkbox (at least from users point of view)
    {
        // note that wxCHK_UNDETERMINED also becomes unchecked when clicked
        state = m_state == wxCHK_UNCHECKED ? wxCHK_CHECKED : wxCHK_UNCHECKED;
    }

    DoSet3StateValue(state);


    // generate the event
    wxCommandEvent event(wxEVT_COMMAND_CHECKBOX_CLICKED, m_windowId);

    event.SetInt(state);
    event.SetEventObject(this);
    ProcessCommand(event);

    return true;
}

// ----------------------------------------------------------------------------
// owner drawn checkboxes stuff
// ----------------------------------------------------------------------------

bool wxCheckBox::SetForegroundColour(const wxColour& colour)
{
    if ( !wxCheckBoxBase::SetForegroundColour(colour) )
        return false;

    // the only way to change the checkbox foreground colour under Windows XP
    // is to owner draw it
    if ( wxUxThemeEngine::GetIfActive() )
        MakeOwnerDrawn(colour.Ok());

    return true;
}

bool wxCheckBox::IsOwnerDrawn() const
{
    return
        (::GetWindowLong(GetHwnd(), GWL_STYLE) & BS_OWNERDRAW) == BS_OWNERDRAW;
}

void wxCheckBox::MakeOwnerDrawn(bool ownerDrawn)
{
    long style = ::GetWindowLong(GetHwnd(), GWL_STYLE);

    // note that BS_CHECKBOX & BS_OWNERDRAW != 0 so we can't operate on
    // them as on independent style bits
    if ( ownerDrawn )
    {
        style &= ~(BS_CHECKBOX | BS_3STATE);
        style |= BS_OWNERDRAW;

        Connect(wxEVT_ENTER_WINDOW,
                wxMouseEventHandler(wxCheckBox::OnMouseEnterOrLeave));
        Connect(wxEVT_LEAVE_WINDOW,
                wxMouseEventHandler(wxCheckBox::OnMouseEnterOrLeave));
        Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(wxCheckBox::OnMouseLeft));
        Connect(wxEVT_LEFT_UP, wxMouseEventHandler(wxCheckBox::OnMouseLeft));
        Connect(wxEVT_SET_FOCUS, wxFocusEventHandler(wxCheckBox::OnFocus));
        Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(wxCheckBox::OnFocus));
    }
    else // reset to default colour
    {
        style &= ~BS_OWNERDRAW;
        style |= HasFlag(wxCHK_3STATE) ? BS_3STATE : BS_CHECKBOX;

        Disconnect(wxEVT_ENTER_WINDOW,
                   wxMouseEventHandler(wxCheckBox::OnMouseEnterOrLeave));
        Disconnect(wxEVT_LEAVE_WINDOW,
                   wxMouseEventHandler(wxCheckBox::OnMouseEnterOrLeave));
        Disconnect(wxEVT_LEFT_DOWN, wxMouseEventHandler(wxCheckBox::OnMouseLeft));
        Disconnect(wxEVT_LEFT_UP, wxMouseEventHandler(wxCheckBox::OnMouseLeft));
        Disconnect(wxEVT_SET_FOCUS, wxFocusEventHandler(wxCheckBox::OnFocus));
        Disconnect(wxEVT_KILL_FOCUS, wxFocusEventHandler(wxCheckBox::OnFocus));
    }

    ::SetWindowLong(GetHwnd(), GWL_STYLE, style);

    if ( !ownerDrawn )
    {
        // ensure that controls state is consistent with internal state
        DoSet3StateValue(m_state);
    }
}

void wxCheckBox::OnMouseEnterOrLeave(wxMouseEvent& event)
{
    m_isHot = event.GetEventType() == wxEVT_ENTER_WINDOW;
    if ( !m_isHot )
        m_isPressed = false;

    Refresh();

    event.Skip();
}

void wxCheckBox::OnMouseLeft(wxMouseEvent& event)
{
    // TODO: we should capture the mouse here to be notified about left up
    //       event but this interferes with BN_CLICKED generation so if we
    //       want to do this we'd need to generate them ourselves
    m_isPressed = event.GetEventType() == wxEVT_LEFT_DOWN;
    Refresh();

    event.Skip();
}

void wxCheckBox::OnFocus(wxFocusEvent& event)
{
    Refresh();

    event.Skip();
}

bool wxCheckBox::MSWOnDraw(WXDRAWITEMSTRUCT *item)
{
    DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)item;

    if ( !IsOwnerDrawn() || dis->CtlType != ODT_BUTTON )
        return wxCheckBoxBase::MSWOnDraw(item);

    // calculate the rectangles for the check mark itself and the label
    HDC hdc = dis->hDC;
    RECT& rect = dis->rcItem;
    RECT rectCheck,
         rectLabel;
    rectCheck.top =
    rectLabel.top = rect.top;
    rectCheck.bottom =
    rectLabel.bottom = rect.bottom;
    const int checkSize = GetBestSize().y;
    const int MARGIN = 3;

    const bool isRightAligned = HasFlag(wxALIGN_RIGHT);
    if ( isRightAligned )
    {
        rectCheck.right = rect.right;
        rectCheck.left = rectCheck.right - checkSize;

        rectLabel.right = rectCheck.left - MARGIN;
        rectLabel.left = rect.left;
    }
    else // normal, left-aligned checkbox
    {
        rectCheck.left = rect.left;
        rectCheck.right = rectCheck.left + checkSize;

        rectLabel.left = rectCheck.right + MARGIN;
        rectLabel.right = rect.right;
    }

    // show we draw a focus rect?
    const bool isFocused = m_isPressed || FindFocus() == this;


    // draw the checkbox itself: note that this should really, really be in
    // wxRendererNative but unfortunately we can't add a new virtual function
    // to it without breaking backwards compatibility

    // classic Win32 version -- this can be useful when we move this into
    // wxRendererNative
#if defined(__WXWINCE__) || !wxUSE_UXTHEME
    UINT state = DFCS_BUTTONCHECK;
    if ( !IsEnabled() )
        state |= DFCS_INACTIVE;
    switch ( Get3StateValue() )
    {
        case wxCHK_CHECKED:
            state |= DFCS_CHECKED;
            break;

        case wxCHK_UNDETERMINED:
            state |= DFCS_PUSHED;
            break;

        default:
            wxFAIL_MSG( _T("unexpected Get3StateValue() return value") );
            // fall through

        case wxCHK_UNCHECKED:
            // no extra styles needed
            break;
    }

    if ( wxFindWindowAtPoint(wxGetMousePosition()) == this )
        state |= DFCS_HOT;

    if ( !::DrawFrameControl(hdc, &rectCheck, DFC_BUTTON, state) )
    {
        wxLogLastError(_T("DrawFrameControl(DFC_BUTTON)"));
    }
#else // XP version
    wxUxThemeEngine *themeEngine = wxUxThemeEngine::GetIfActive();
    if ( !themeEngine )
        return false;

    wxUxThemeHandle theme(this, L"BUTTON");
    if ( !theme )
        return false;

    int state;
    switch ( Get3StateValue() )
    {
        case wxCHK_CHECKED:
            state = CBS_CHECKEDNORMAL;
            break;

        case wxCHK_UNDETERMINED:
            state = CBS_MIXEDNORMAL;
            break;

        default:
            wxFAIL_MSG( _T("unexpected Get3StateValue() return value") );
            // fall through

        case wxCHK_UNCHECKED:
            state = CBS_UNCHECKEDNORMAL;
            break;
    }

    if ( !IsEnabled() )
        state += CBS_DISABLED_OFFSET;
    else if ( m_isPressed )
        state += CBS_PRESSED_OFFSET;
    else if ( m_isHot )
        state += CBS_HOT_OFFSET;

    HRESULT hr = themeEngine->DrawThemeBackground
                              (
                                theme,
                                hdc,
                                BP_CHECKBOX,
                                state,
                                &rectCheck,
                                NULL
                              );
    if ( FAILED(hr) )
    {
        wxLogApiError(_T("DrawThemeBackground(BP_CHECKBOX)"), hr);
    }
#endif // 0/1

    // draw the text
    const wxString& label = GetLabel();

    // first we need to measure it
    UINT fmt = DT_NOCLIP;

    // drawing underlying doesn't look well with focus rect (and the native
    // control doesn't do it)
    if ( isFocused )
        fmt |= DT_HIDEPREFIX;
    if ( isRightAligned )
        fmt |= DT_RIGHT;
    // TODO: also use DT_HIDEPREFIX if the system is configured so

    // we need to get the label real size first if we have to draw a focus rect
    // around it
    if ( isFocused )
    {
        if ( !::DrawText(hdc, label, label.length(), &rectLabel,
                         fmt | DT_CALCRECT) )
        {
            wxLogLastError(_T("DrawText(DT_CALCRECT)"));
        }
    }

    if ( !IsEnabled() )
    {
        ::SetTextColor(hdc, ::GetSysColor(COLOR_GRAYTEXT));
    }

    if ( !::DrawText(hdc, label, label.length(), &rectLabel, fmt) )
    {
        wxLogLastError(_T("DrawText()"));
    }

    // finally draw the focus
    if ( isFocused )
    {
        rectLabel.left--;
        rectLabel.right++;
        if ( !::DrawFocusRect(hdc, &rectLabel) )
        {
            wxLogLastError(_T("DrawFocusRect()"));
        }
    }

    return true;
}

#endif // wxUSE_CHECKBOX
