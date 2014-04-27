///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/pickerbase.cpp
// Purpose:     wxPickerBase class implementation
// Author:      Francesco Montorsi
// Modified by:
// Created:     15/04/2006
// RCS-ID:      $Id: pickerbase.cpp 58463 2009-01-27 17:39:50Z BP $
// Copyright:   (c) Francesco Montorsi
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

#if wxUSE_COLOURPICKERCTRL || \
    wxUSE_DIRPICKERCTRL    || \
    wxUSE_FILEPICKERCTRL   || \
    wxUSE_FONTPICKERCTRL

#include "wx/pickerbase.h"
#include "wx/tooltip.h"

#ifndef WX_PRECOMP
    #include "wx/textctrl.h"
#endif


// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_ABSTRACT_CLASS(wxPickerBase, wxControl)

BEGIN_EVENT_TABLE(wxPickerBase, wxControl)
    EVT_SIZE(wxPickerBase::OnSize)
    WX_EVENT_TABLE_CONTROL_CONTAINER(wxPickerBase)
END_EVENT_TABLE()
WX_DELEGATE_TO_CONTROL_CONTAINER(wxPickerBase, wxControl)


// ----------------------------------------------------------------------------
// wxPickerBase
// ----------------------------------------------------------------------------

bool wxPickerBase::CreateBase(wxWindow *parent,
                         wxWindowID id,
                         const wxString &text,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         const wxValidator& validator,
                         const wxString& name)
{
    // remove any border style from our style as wxPickerBase's window must be
    // invisible (user styles must be set on the textctrl or the platform-dependent picker)
    style &= ~wxBORDER_MASK;
    if (!wxControl::Create(parent, id, pos, size, style | wxNO_BORDER | wxTAB_TRAVERSAL,
                           validator, name))
        return false;

    m_sizer = new wxBoxSizer(wxHORIZONTAL);

    if (HasFlag(wxPB_USE_TEXTCTRL))
    {
        // NOTE: the style of this class (wxPickerBase) and the style of the
        //       attached text control are different: GetTextCtrlStyle() extracts
        //       the styles related to the textctrl from the styles passed here
        m_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                GetTextCtrlStyle(style));
        if (!m_text)
        {
            wxFAIL_MSG( wxT("wxPickerBase's textctrl creation failed") );
            return false;
        }

        // set the maximum lenght allowed for this textctrl.
        // This is very important since any change to it will trigger an update in
        // the m_picker; for very long strings, this real-time synchronization could
        // become a CPU-blocker and thus should be avoided.
        // 32 characters will be more than enough for all common uses.
        m_text->SetMaxLength(32);

        // set the initial contents of the textctrl
        m_text->SetValue(text);

        m_text->Connect(m_text->GetId(), wxEVT_COMMAND_TEXT_UPDATED,
                wxCommandEventHandler(wxPickerBase::OnTextCtrlUpdate),
                NULL, this);
        m_text->Connect(m_text->GetId(), wxEVT_KILL_FOCUS,
                wxFocusEventHandler(wxPickerBase::OnTextCtrlKillFocus),
                NULL, this);

        m_text->Connect(m_text->GetId(), wxEVT_DESTROY,
                wxWindowDestroyEventHandler(wxPickerBase::OnTextCtrlDelete),
                NULL, this);

        // the text control's proportion values defaults to 2
        m_sizer->Add(m_text, 2, GetDefaultTextCtrlFlag(), 5);
    }

    return true;
}

void wxPickerBase::PostCreation()
{
    // the picker's proportion value defaults to 1 when there's no text control
    // associated with it - in that case it defaults to 0
    m_sizer->Add(m_picker, HasTextCtrl() ? 0 : 1, GetDefaultPickerCtrlFlag(), 5);

    SetSizer(m_sizer);
    SetMinSize( m_sizer->GetMinSize() );
}

#if wxUSE_TOOLTIPS

void wxPickerBase::DoSetToolTip(wxToolTip *tip)
{
    // don't set the tooltip on us but rather on our two child windows
    // as otherwise it would appear only when the cursor is placed on the
    // small area around the child windows which belong to wxPickerBase
    m_picker->SetToolTip(tip);

    // do a copy as wxWindow will own the pointer we pass
    if ( m_text )
        m_text->SetToolTip(tip ? new wxToolTip(tip->GetTip()) : NULL);
}

#endif // wxUSE_TOOLTIPS

// ----------------------------------------------------------------------------
// wxPickerBase - event handlers
// ----------------------------------------------------------------------------

void wxPickerBase::OnTextCtrlKillFocus(wxFocusEvent& event)
{
    event.Skip();

    // don't leave the textctrl empty
    if (m_text && m_text->GetValue().empty())
        UpdateTextCtrlFromPicker();
}

void wxPickerBase::OnTextCtrlDelete(wxWindowDestroyEvent &)
{
    // the textctrl has been deleted; our pointer is invalid!
    m_text = NULL;
}

void wxPickerBase::OnTextCtrlUpdate(wxCommandEvent &)
{
    // for each text-change, update the picker
    UpdatePickerFromTextCtrl();
}

void wxPickerBase::OnSize(wxSizeEvent &event)
{
    if (GetAutoLayout())
        Layout();
    event.Skip();
}

#endif // Any picker in use
