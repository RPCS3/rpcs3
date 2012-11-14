/////////////////////////////////////////////////////////////////////////////
// Name:        src/univ/checkbox.cpp
// Purpose:     wxCheckBox implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     25.08.00
// RCS-ID:      $Id: checkbox.cpp 42816 2006-10-31 08:50:17Z RD $
// Copyright:   (c) 2000 SciTech Software, Inc. (www.scitechsoft.com)
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

#if wxUSE_CHECKBOX

#include "wx/checkbox.h"

#ifndef WX_PRECOMP
    #include "wx/dcclient.h"
    #include "wx/validate.h"

    #include "wx/button.h" // for wxACTION_BUTTON_XXX
#endif

#include "wx/univ/theme.h"
#include "wx/univ/renderer.h"
#include "wx/univ/inphand.h"
#include "wx/univ/colschem.h"

// ----------------------------------------------------------------------------
// wxStdCheckboxInputHandler: handles the mouse events for the check and radio
// boxes (handling the keyboard input is simple, but its handling differs a
// lot between GTK and MSW, so a new class should be derived for this)
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStdCheckboxInputHandler : public wxStdInputHandler
{
public:
    wxStdCheckboxInputHandler(wxInputHandler *inphand);

    // we have to override this one as wxStdButtonInputHandler version works
    // only with the buttons
    virtual bool HandleActivation(wxInputConsumer *consumer, bool activated);
};

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxCheckBox, wxControl)

// ----------------------------------------------------------------------------
// wxCheckBox
// ----------------------------------------------------------------------------

void wxCheckBox::Init()
{
    m_isPressed = false;
    m_status = Status_Unchecked;
}

bool wxCheckBox::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxString &label,
                        const wxPoint &pos,
                        const wxSize &size,
                        long style,
                        const wxValidator& validator,
                        const wxString &name)
{
    if ( !wxControl::Create(parent, id, pos, size, style, validator, name) )
        return false;

    SetLabel(label);
    SetInitialSize(size);

    CreateInputHandler(wxINP_HANDLER_CHECKBOX);

    return true;
}

// ----------------------------------------------------------------------------
// checkbox interface
// ----------------------------------------------------------------------------

bool wxCheckBox::GetValue() const
{
    return (Get3StateValue() != wxCHK_UNCHECKED);
}

void wxCheckBox::SetValue(bool value)
{
    Set3StateValue( value ? wxCHK_CHECKED : wxCHK_UNCHECKED );
}

void wxCheckBox::OnCheck()
{
    // we do nothing here
}

// ----------------------------------------------------------------------------
// indicator bitmaps
// ----------------------------------------------------------------------------

wxBitmap wxCheckBox::GetBitmap(State state, Status status) const
{
    wxBitmap bmp = m_bitmaps[state][status];
    if ( !bmp.Ok() )
        bmp = m_bitmaps[State_Normal][status];

    return bmp;
}

void wxCheckBox::SetBitmap(const wxBitmap& bmp, State state, Status status)
{
    m_bitmaps[state][status] = bmp;
}

// ----------------------------------------------------------------------------
// drawing
// ----------------------------------------------------------------------------

wxCheckBox::State wxCheckBox::GetState(int flags) const
{
    if ( flags & wxCONTROL_DISABLED )
        return State_Disabled;
    else if ( flags & wxCONTROL_PRESSED )
        return State_Pressed;
    else if ( flags & wxCONTROL_CURRENT )
        return State_Current;
    else
        return State_Normal;
}

void wxCheckBox::DoDraw(wxControlRenderer *renderer)
{
    int flags = GetStateFlags();

    wxDC& dc = renderer->GetDC();
    dc.SetFont(GetFont());
    dc.SetTextForeground(GetForegroundColour());

    switch ( Get3StateValue() )
    {
        case wxCHK_CHECKED:      flags |= wxCONTROL_CHECKED;      break;
        case wxCHK_UNDETERMINED: flags |= wxCONTROL_UNDETERMINED; break;
        default:                 /* do nothing */                 break;
    }

    wxBitmap bitmap(GetBitmap(GetState(flags), m_status));

    renderer->GetRenderer()->
        DrawCheckButton(dc,
                        GetLabel(),
                        bitmap,
                        renderer->GetRect(),
                        flags,
                        GetWindowStyle() & wxALIGN_RIGHT ? wxALIGN_RIGHT
                                                         : wxALIGN_LEFT,
                        GetAccelIndex());
}

// ----------------------------------------------------------------------------
// geometry calculations
// ----------------------------------------------------------------------------

wxSize wxCheckBox::GetBitmapSize() const
{
    wxBitmap bmp = GetBitmap(State_Normal, Status_Checked);
    return bmp.Ok() ? wxSize(bmp.GetWidth(), bmp.GetHeight())
                    : GetRenderer()->GetCheckBitmapSize();
}

wxSize wxCheckBox::DoGetBestClientSize() const
{
    wxClientDC dc(wxConstCast(this, wxCheckBox));
    dc.SetFont(GetFont());
    wxCoord width, height;
    dc.GetMultiLineTextExtent(GetLabel(), &width, &height);

    wxSize sizeBmp = GetBitmapSize();
    if ( height < sizeBmp.y )
        height = sizeBmp.y;

#if defined(wxUNIV_COMPATIBLE_MSW) && wxUNIV_COMPATIBLE_MSW
    // FIXME: flag nowhere defined so perhaps should be removed?

    // this looks better but is different from what wxMSW does
    height += GetCharHeight()/2;
#endif // wxUNIV_COMPATIBLE_MSW

    width += sizeBmp.x + 2*GetCharWidth();

    return wxSize(width, height);
}

// ----------------------------------------------------------------------------
// checkbox actions
// ----------------------------------------------------------------------------

void wxCheckBox::DoSet3StateValue(wxCheckBoxState state)
{
    Status status;
    switch ( state )
    {
        case wxCHK_UNCHECKED:    status = Status_Unchecked;   break;
        case wxCHK_CHECKED:      status = Status_Checked; break;
        default:                 wxFAIL_MSG(_T("Unknown checkbox state"));
        case wxCHK_UNDETERMINED: status = Status_3rdState;  break;
    }
    if ( status != m_status )
    {
        m_status = status;

        if ( m_status == Status_Checked )
        {
            // invoke the hook
            OnCheck();
        }

        Refresh();
    }
}

wxCheckBoxState wxCheckBox::DoGet3StateValue() const
{
    switch ( m_status )
    {
        case Status_Checked:    return wxCHK_CHECKED;
        case Status_Unchecked:  return wxCHK_UNCHECKED;
        default:                /* go further */ break;
    }
    return wxCHK_UNDETERMINED;
}

void wxCheckBox::Press()
{
    if ( !m_isPressed )
    {
        m_isPressed = true;

        Refresh();
    }
}

void wxCheckBox::Release()
{
    if ( m_isPressed )
    {
        m_isPressed = false;

        Refresh();
    }
}

void wxCheckBox::Toggle()
{
    m_isPressed = false;

    Status status = m_status;

    switch ( Get3StateValue() )
    {
        case wxCHK_CHECKED:
            Set3StateValue(Is3rdStateAllowedForUser() ? wxCHK_UNDETERMINED : wxCHK_UNCHECKED);
            break;

        case wxCHK_UNCHECKED:
            Set3StateValue(wxCHK_CHECKED);
            break;

        case wxCHK_UNDETERMINED:
            Set3StateValue(wxCHK_UNCHECKED);
            break;
    }

    if( status != m_status )
        SendEvent();
}

void wxCheckBox::ChangeValue(bool value)
{
    SetValue(value);

    SendEvent();
}

void wxCheckBox::SendEvent()
{
    wxCommandEvent event(wxEVT_COMMAND_CHECKBOX_CLICKED, GetId());
    InitCommandEvent(event);
    wxCheckBoxState state = Get3StateValue();

    // If the style flag to allow the user setting the undetermined state
    // is not set, then skip the undetermined state and set it to unchecked.
    if ( state == wxCHK_UNDETERMINED && !Is3rdStateAllowedForUser() )
    {
        state = wxCHK_UNCHECKED;
        Set3StateValue(state);
    }

    event.SetInt(state);
    Command(event);
}

// ----------------------------------------------------------------------------
// input handling
// ----------------------------------------------------------------------------

bool wxCheckBox::PerformAction(const wxControlAction& action,
                               long numArg,
                               const wxString& strArg)
{
    if ( action == wxACTION_BUTTON_PRESS )
        Press();
    else if ( action == wxACTION_BUTTON_RELEASE )
        Release();
    if ( action == wxACTION_CHECKBOX_CHECK )
        ChangeValue(true);
    else if ( action == wxACTION_CHECKBOX_CLEAR )
        ChangeValue(false);
    else if ( action == wxACTION_CHECKBOX_TOGGLE )
        Toggle();
    else
        return wxControl::PerformAction(action, numArg, strArg);

    return true;
}

/* static */
wxInputHandler *wxCheckBox::CreateStdInputHandler(wxInputHandler *handlerDef)
{
    static wxStdCheckboxInputHandler s_handler(handlerDef);

    return &s_handler;
}

// ----------------------------------------------------------------------------
// wxStdCheckboxInputHandler
// ----------------------------------------------------------------------------

wxStdCheckboxInputHandler::wxStdCheckboxInputHandler(wxInputHandler *def)
                         : wxStdInputHandler(wxButton::GetStdInputHandler(def))
{
}

bool wxStdCheckboxInputHandler::HandleActivation(wxInputConsumer *consumer,
                                                 bool WXUNUSED(activated))
{
    // only the focused checkbox appearance changes when the app gains/loses
    // activation
    return consumer->GetInputWindow()->IsFocused();
}

#endif // wxUSE_CHECKBOX
