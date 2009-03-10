///////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/spinctlg.cpp
// Purpose:     implements wxSpinCtrl as a composite control
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29.01.01
// RCS-ID:      $Id: spinctlg.cpp 52582 2008-03-17 13:46:31Z VZ $
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
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

// There are port-specific versions for MSW, GTK, OS/2 and Mac, so exclude the
// contents of this file in those cases
#if !(defined(__WXMSW__) || defined(__WXGTK__) || defined(__WXPM__) || \
    defined(__WXMAC__)) || defined(__WXUNIVERSAL__)

#ifndef WX_PRECOMP
    #include "wx/textctrl.h"
#endif //WX_PRECOMP

#if wxUSE_SPINCTRL

#include "wx/spinbutt.h"
#include "wx/spinctrl.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the margin between the text control and the spin
static const wxCoord MARGIN = 2;

// ----------------------------------------------------------------------------
// wxSpinCtrlText: text control used by spin control
// ----------------------------------------------------------------------------

class wxSpinCtrlText : public wxTextCtrl
{
public:
    wxSpinCtrlText(wxSpinCtrl *spin, const wxString& value)
        : wxTextCtrl(spin->GetParent(), wxID_ANY, value)
    {
        m_spin = spin;

        // remove the default minsize, the spinctrl will have one instead
        SetSizeHints(wxDefaultCoord,wxDefaultCoord);
    }

protected:
    void OnTextChange(wxCommandEvent& event)
    {
        int val;
        if ( m_spin->GetTextValue(&val) )
        {
            m_spin->GetSpinButton()->SetValue(val);
        }

        event.Skip();
    }

    bool ProcessEvent(wxEvent &event)
    {
        // Hand button down events to wxSpinCtrl. Doesn't work.
        if (event.GetEventType() == wxEVT_LEFT_DOWN && m_spin->ProcessEvent( event ))
            return true;

        return wxTextCtrl::ProcessEvent( event );
    }

private:
    wxSpinCtrl *m_spin;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxSpinCtrlText, wxTextCtrl)
    EVT_TEXT(wxID_ANY, wxSpinCtrlText::OnTextChange)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxSpinCtrlButton: spin button used by spin control
// ----------------------------------------------------------------------------

class wxSpinCtrlButton : public wxSpinButton
{
public:
    wxSpinCtrlButton(wxSpinCtrl *spin, int style)
        : wxSpinButton(spin->GetParent())
    {
        m_spin = spin;

        SetWindowStyle(style | wxSP_VERTICAL);

        // remove the default minsize, the spinctrl will have one instead
        SetSizeHints(wxDefaultCoord,wxDefaultCoord);
    }

protected:
    void OnSpinButton(wxSpinEvent& eventSpin)
    {
        m_spin->SetTextValue(eventSpin.GetPosition());

        wxCommandEvent event(wxEVT_COMMAND_SPINCTRL_UPDATED, m_spin->GetId());
        event.SetEventObject(m_spin);
        event.SetInt(eventSpin.GetPosition());

        m_spin->GetEventHandler()->ProcessEvent(event);

        eventSpin.Skip();
    }

private:
    wxSpinCtrl *m_spin;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxSpinCtrlButton, wxSpinButton)
    EVT_SPIN(wxID_ANY, wxSpinCtrlButton::OnSpinButton)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(wxSpinCtrl, wxControl)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxSpinCtrl creation
// ----------------------------------------------------------------------------

void wxSpinCtrl::Init()
{
    m_text = NULL;
    m_btn = NULL;
}

bool wxSpinCtrl::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxString& value,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        int min,
                        int max,
                        int initial,
                        const wxString& name)
{
    if ( !wxControl::Create(parent, id, wxDefaultPosition, wxDefaultSize, style,
                            wxDefaultValidator, name) )
    {
        return false;
    }

    // the string value overrides the numeric one (for backwards compatibility
    // reasons and also because it is simpler to satisfy the string value which
    // comes much sooner in the list of arguments and leave the initial
    // parameter unspecified)
    if ( !value.empty() )
    {
        long l;
        if ( value.ToLong(&l) )
            initial = l;
    }

    m_text = new wxSpinCtrlText(this, value);
    m_btn = new wxSpinCtrlButton(this, style);

    m_btn->SetRange(min, max);
    m_btn->SetValue(initial);
    SetInitialSize(size);
    Move(pos);

    // have to disable this window to avoid interfering it with message
    // processing to the text and the button... but pretend it is enabled to
    // make IsEnabled() return true
    wxControl::Enable(false); // don't use non virtual Disable() here!
    m_isEnabled = true;

    // we don't even need to show this window itself - and not doing it avoids
    // that it overwrites the text control
    wxControl::Show(false);
    m_isShown = true;
    return true;
}

wxSpinCtrl::~wxSpinCtrl()
{
    // delete the controls now, don't leave them alive even though they would
    // still be eventually deleted by our parent - but it will be too late, the
    // user code expects them to be gone now
    delete m_text;
    m_text = NULL ;
    delete m_btn;
    m_btn = NULL ;
}

// ----------------------------------------------------------------------------
// geometry
// ----------------------------------------------------------------------------

wxSize wxSpinCtrl::DoGetBestSize() const
{
    wxSize sizeBtn = m_btn->GetBestSize(),
           sizeText = m_text->GetBestSize();

    return wxSize(sizeBtn.x + sizeText.x + MARGIN, sizeText.y);
}

void wxSpinCtrl::DoMoveWindow(int x, int y, int width, int height)
{
    wxControl::DoMoveWindow(x, y, width, height);

    // position the subcontrols inside the client area
    wxSize sizeBtn = m_btn->GetSize();

    wxCoord wText = width - sizeBtn.x;
    m_text->SetSize(x, y, wText, height);
    m_btn->SetSize(x + wText + MARGIN, y, wxDefaultCoord, height);
}

// ----------------------------------------------------------------------------
// operations forwarded to the subcontrols
// ----------------------------------------------------------------------------

bool wxSpinCtrl::Enable(bool enable)
{
    if ( !wxControl::Enable(enable) )
        return false;

    m_btn->Enable(enable);
    m_text->Enable(enable);

    return true;
}

bool wxSpinCtrl::Show(bool show)
{
    if ( !wxControl::Show(show) )
        return false;

    // under GTK Show() is called the first time before we are fully
    // constructed
    if ( m_btn )
    {
        m_btn->Show(show);
        m_text->Show(show);
    }

    return true;
}

bool wxSpinCtrl::Reparent(wxWindow *newParent)
{
    if ( m_btn )
    {
        m_btn->Reparent(newParent);
        m_text->Reparent(newParent);
    }

    return true;
}

// ----------------------------------------------------------------------------
// value and range access
// ----------------------------------------------------------------------------

bool wxSpinCtrl::GetTextValue(int *val) const
{
    long l;
    if ( !m_text->GetValue().ToLong(&l) )
    {
        // not a number at all
        return false;
    }

    if ( l < GetMin() || l > GetMax() )
    {
        // out of range
        return false;
    }

    *val = l;

    return true;
}

int wxSpinCtrl::GetValue() const
{
    return m_btn ? m_btn->GetValue() : 0;
}

int wxSpinCtrl::GetMin() const
{
    return m_btn ? m_btn->GetMin() : 0;
}

int wxSpinCtrl::GetMax() const
{
    return m_btn ? m_btn->GetMax() : 0;
}

// ----------------------------------------------------------------------------
// changing value and range
// ----------------------------------------------------------------------------

void wxSpinCtrl::SetTextValue(int val)
{
    wxCHECK_RET( m_text, _T("invalid call to wxSpinCtrl::SetTextValue") );

    m_text->SetValue(wxString::Format(_T("%d"), val));

    // select all text
    m_text->SetSelection(0, -1);

    // and give focus to the control!
    // m_text->SetFocus();    Why???? TODO.

#ifdef __WXCOCOA__
    /*  It's sort of a hack to do this from here but the idea is that if the
        user has clicked on us, which is the main reason this method is called,
        then focus probably ought to go to the text control since clicking on
        a text control usually gives it focus.

        However, if the focus is already on us (i.e. the user has turned on
        the ability to tab to controls) then we don't want to drop focus.
        So we only set focus if we would steal it away from a different
        control, not if we would steal it away from ourself.
     */
    wxWindow *currentFocusedWindow = wxWindow::FindFocus();
    if(currentFocusedWindow != this && currentFocusedWindow != m_text)
        m_text->SetFocus();
#endif
}

void wxSpinCtrl::SetValue(int val)
{
    wxCHECK_RET( m_btn, _T("invalid call to wxSpinCtrl::SetValue") );

    SetTextValue(val);

    m_btn->SetValue(val);
}

void wxSpinCtrl::SetValue(const wxString& text)
{
    wxCHECK_RET( m_text, _T("invalid call to wxSpinCtrl::SetValue") );

    long val;
    if ( text.ToLong(&val) && ((val > INT_MIN) && (val < INT_MAX)) )
    {
        SetValue((int)val);
    }
    else // not a number at all or out of range
    {
        m_text->SetValue(text);
        m_text->SetSelection(0, -1);
    }
}

void wxSpinCtrl::SetRange(int min, int max)
{
    wxCHECK_RET( m_btn, _T("invalid call to wxSpinCtrl::SetRange") );

    m_btn->SetRange(min, max);
}

void wxSpinCtrl::SetSelection(long from, long to)
{
    wxCHECK_RET( m_text, _T("invalid call to wxSpinCtrl::SetSelection") );

    m_text->SetSelection(from, to);
}

#endif // wxUSE_SPINCTRL
#endif // !wxPort-with-native-spinctrl
