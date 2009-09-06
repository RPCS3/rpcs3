/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/progdlgg.cpp
// Purpose:     wxProgressDialog class
// Author:      Karsten Ballueder
// Modified by:
// Created:     09.05.1999
// RCS-ID:      $Id: progdlgg.cpp 50711 2007-12-15 02:57:58Z VZ $
// Copyright:   (c) Karsten Ballueder
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

#if wxUSE_PROGRESSDLG

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/frame.h"
    #include "wx/button.h"
    #include "wx/stattext.h"
    #include "wx/sizer.h"
    #include "wx/event.h"
    #include "wx/gauge.h"
    #include "wx/intl.h"
    #include "wx/dcclient.h"
    #include "wx/timer.h"
    #include "wx/settings.h"
#endif

#include "wx/progdlg.h"

// ---------------------------------------------------------------------------
// macros
// ---------------------------------------------------------------------------

/* Macro for avoiding #ifdefs when value have to be different depending on size of
   device we display on - take it from something like wxDesktopPolicy in the future
 */

#if defined(__SMARTPHONE__)
    #define wxLARGESMALL(large,small) small
#else
    #define wxLARGESMALL(large,small) large
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define LAYOUT_MARGIN wxLARGESMALL(8,2)

static const int wxID_SKIP = 32000;  // whatever

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// update the label to show the given time (in seconds)
static void SetTimeLabel(unsigned long val, wxStaticText *label);

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxProgressDialog, wxDialog)
    EVT_BUTTON(wxID_CANCEL, wxProgressDialog::OnCancel)
    EVT_BUTTON(wxID_SKIP, wxProgressDialog::OnSkip)

    EVT_CLOSE(wxProgressDialog::OnClose)
END_EVENT_TABLE()

IMPLEMENT_CLASS(wxProgressDialog, wxDialog)

// ============================================================================
// wxProgressDialog implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxProgressDialog creation
// ----------------------------------------------------------------------------

wxProgressDialog::wxProgressDialog(wxString const &title,
                                   wxString const &message,
                                   int maximum,
                                   wxWindow *parent,
                                   int style)
                : wxDialog(parent, wxID_ANY, title),
                  m_skip(false),
                  m_delay(3),
                  m_hasAbortButton(false),
                  m_hasSkipButton(false)
{
    // we may disappear at any moment, let the others know about it
    SetExtraStyle(GetExtraStyle() | wxWS_EX_TRANSIENT);
    m_windowStyle |= style;

    m_hasAbortButton = (style & wxPD_CAN_ABORT) != 0;
    m_hasSkipButton = (style & wxPD_CAN_SKIP) != 0;

    bool isPda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);

#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
    // we have to remove the "Close" button from the title bar then as it is
    // confusing to have it - it doesn't work anyhow
    //
    // FIXME: should probably have a (extended?) window style for this
    if ( !m_hasAbortButton )
    {
        EnableCloseButton(false);
    }
#endif // wxMSW

#if defined(__SMARTPHONE__)
    SetLeftMenu();
#endif

    m_state = m_hasAbortButton ? Continue : Uncancelable;
    m_maximum = maximum;

#if defined(__WXMSW__) || defined(__WXPM__)
    // we can't have values > 65,536 in the progress control under Windows, so
    // scale everything down
    m_factor = m_maximum / 65536 + 1;
    m_maximum /= m_factor;
#endif // __WXMSW__

    m_parentTop = wxGetTopLevelParent(parent);

    wxClientDC dc(this);
    dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    long widthText = 0;
    dc.GetTextExtent(message, &widthText, NULL, NULL, NULL, NULL);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    m_msg = new wxStaticText(this, wxID_ANY, message);
    sizer->Add(m_msg, 0, wxLEFT | wxTOP, 2*LAYOUT_MARGIN);

    wxSize sizeDlg,
           sizeLabel = m_msg->GetSize();
    sizeDlg.y = 2*LAYOUT_MARGIN + sizeLabel.y;

    if ( maximum > 0 )
    {
        int gauge_style = wxGA_HORIZONTAL;
        if ( ( style & wxPD_SMOOTH ) == wxPD_SMOOTH )
            gauge_style |= wxGA_SMOOTH;
        m_gauge = new wxGauge(this, wxID_ANY, m_maximum,
                              wxDefaultPosition, wxDefaultSize,
                              gauge_style );

        sizer->Add(m_gauge, 0, wxLEFT | wxRIGHT | wxTOP | wxEXPAND, 2*LAYOUT_MARGIN);
        m_gauge->SetValue(0);

        wxSize sizeGauge = m_gauge->GetSize();
        sizeDlg.y += 2*LAYOUT_MARGIN + sizeGauge.y;
    }
    else
        m_gauge = (wxGauge *)NULL;

    // create the estimated/remaining/total time zones if requested
    m_elapsed = m_estimated = m_remaining = (wxStaticText*)NULL;
    m_display_estimated = m_last_timeupdate = m_break = 0;
    m_ctdelay = 0;

    // if we are going to have at least one label, remember it in this var
    wxStaticText *label = NULL;

    // also count how many labels we really have
    size_t nTimeLabels = 0;

    if ( style & wxPD_ELAPSED_TIME )
    {
        nTimeLabels++;

        label =
        m_elapsed = CreateLabel(_("Elapsed time : "), sizer);
    }

    if ( style & wxPD_ESTIMATED_TIME )
    {
        nTimeLabels++;

        label =
        m_estimated = CreateLabel(_("Estimated time : "), sizer);
    }

    if ( style & wxPD_REMAINING_TIME )
    {
        nTimeLabels++;

        label =
        m_remaining = CreateLabel(_("Remaining time : "), sizer);
    }

    if ( nTimeLabels > 0 )
    {
        // set it to the current time
        m_timeStart = wxGetCurrentTime();
        sizeDlg.y += nTimeLabels * (label->GetSize().y + LAYOUT_MARGIN);
    }

#if defined(__SMARTPHONE__)
    if ( m_hasSkipButton )
        SetRightMenu(wxID_SKIP, _("Skip"));
    if ( m_hasAbortButton )
        SetLeftMenu(wxID_CANCEL);
#else
    m_btnAbort = m_btnSkip = (wxButton *)NULL;
    bool sizeDlgModified = false;
    wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    const int sizerFlags =
#if defined(__WXMSW__) || defined(__WXPM__)
                           wxALIGN_RIGHT | wxALL
#else // !MSW
                           wxALIGN_CENTER_HORIZONTAL | wxBOTTOM | wxTOP
#endif // MSW/!MSW
                           ;

    if ( m_hasSkipButton )
    {
        m_btnSkip = new wxButton(this, wxID_SKIP, _("Skip"));

        // Windows dialogs usually have buttons in the lower right corner
        buttonSizer->Add(m_btnSkip, 0, sizerFlags, LAYOUT_MARGIN);
        sizeDlg.y += 2*LAYOUT_MARGIN + wxButton::GetDefaultSize().y;
        sizeDlgModified = true;
    }

    if ( m_hasAbortButton )
    {
        m_btnAbort = new wxButton(this, wxID_CANCEL);

        // Windows dialogs usually have buttons in the lower right corner
        buttonSizer->Add(m_btnAbort, 0, sizerFlags, LAYOUT_MARGIN);
        if(!sizeDlgModified)
            sizeDlg.y += 2*LAYOUT_MARGIN + wxButton::GetDefaultSize().y;
    }

    sizer->Add(buttonSizer, 0, sizerFlags, LAYOUT_MARGIN );
#endif // __SMARTPHONE__/!__SMARTPHONE__

    SetSizerAndFit(sizer);

    if (!isPda)
    {
        sizeDlg.y += 2*LAYOUT_MARGIN;

        // try to make the dialog not square but rectangular of reasonable width
        sizeDlg.x = (wxCoord)wxMax(3*widthText/2, 4*sizeDlg.y/3);
        SetClientSize(sizeDlg);
    }

    Centre(wxCENTER_FRAME | wxBOTH);

    if ( style & wxPD_APP_MODAL )
    {
        m_winDisabler = new wxWindowDisabler(this);
    }
    else
    {
        if ( m_parentTop )
            m_parentTop->Disable();
        m_winDisabler = NULL;
    }

    Show();
    Enable();

    // this one can be initialized even if the others are unknown for now
    //
    // NB: do it after calling Layout() to keep the labels correctly aligned
    if ( m_elapsed )
    {
        SetTimeLabel(0, m_elapsed);
    }

    Update();
}

wxStaticText *wxProgressDialog::CreateLabel(const wxString& text,
                                            wxSizer *sizer)
{
    wxBoxSizer *locsizer = new wxBoxSizer(wxLARGESMALL(wxHORIZONTAL,wxVERTICAL));

    wxStaticText *dummy = new wxStaticText(this, wxID_ANY, text);
    wxStaticText *label = new wxStaticText(this, wxID_ANY, _("unknown"));

    // select placement most native or nice on target GUI
#if defined(__SMARTPHONE__)
    // label and time to the left in two rows
    locsizer->Add(dummy, 1, wxALIGN_LEFT);
    locsizer->Add(label, 1, wxALIGN_LEFT);
    sizer->Add(locsizer, 0, wxALIGN_LEFT | wxTOP | wxLEFT, LAYOUT_MARGIN);
#elif defined(__WXMSW__) || defined(__WXPM__) || defined(__WXMAC__) || defined(__WXGTK20__)
    // label and time centered in one row
    locsizer->Add(dummy, 1, wxLARGESMALL(wxALIGN_RIGHT,wxALIGN_LEFT));
    locsizer->Add(label, 1, wxALIGN_LEFT | wxLEFT, LAYOUT_MARGIN);
    sizer->Add(locsizer, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, LAYOUT_MARGIN);
#else
    // label and time to the right in one row
    sizer->Add(locsizer, 0, wxALIGN_RIGHT | wxRIGHT | wxTOP, LAYOUT_MARGIN);
    locsizer->Add(dummy);
    locsizer->Add(label, 0, wxLEFT, LAYOUT_MARGIN);
#endif

    return label;
}

// ----------------------------------------------------------------------------
// wxProgressDialog operations
// ----------------------------------------------------------------------------

bool
wxProgressDialog::Update(int value, const wxString& newmsg, bool *skip)
{
    wxASSERT_MSG( value == -1 || m_gauge, wxT("cannot update non existent dialog") );

#ifdef __WXMSW__
    value /= m_factor;
#endif // __WXMSW__

    wxASSERT_MSG( value <= m_maximum, wxT("invalid progress value") );

    if ( m_gauge )
        m_gauge->SetValue(value);

    UpdateMessage(newmsg);

    if ( (m_elapsed || m_remaining || m_estimated) && (value != 0) )
    {
        unsigned long elapsed = wxGetCurrentTime() - m_timeStart;
        if (    m_last_timeupdate < elapsed
             || value == m_maximum
           )
        {
            m_last_timeupdate = elapsed;
            unsigned long estimated = m_break +
                  (unsigned long)(( (double) (elapsed-m_break) * m_maximum ) / ((double)value)) ;
            if (    estimated > m_display_estimated
                 && m_ctdelay >= 0
               )
            {
                ++m_ctdelay;
            }
            else if (    estimated < m_display_estimated
                      && m_ctdelay <= 0
                    )
            {
                --m_ctdelay;
            }
            else
            {
                m_ctdelay = 0;
            }
            if (    m_ctdelay >= m_delay          // enough confirmations for a higher value
                 || m_ctdelay <= (m_delay*-1)     // enough confirmations for a lower value
                 || value == m_maximum            // to stay consistent
                 || elapsed > m_display_estimated // to stay consistent
                 || ( elapsed > 0 && elapsed < 4 ) // additional updates in the beginning
               )
            {
                m_display_estimated = estimated;
                m_ctdelay = 0;
            }
        }

        long display_remaining = m_display_estimated - elapsed;
        if ( display_remaining < 0 )
        {
            display_remaining = 0;
        }

        SetTimeLabel(elapsed, m_elapsed);
        SetTimeLabel(m_display_estimated, m_estimated);
        SetTimeLabel(display_remaining, m_remaining);
    }

    if ( value == m_maximum )
    {
        if ( m_state == Finished )
        {
            // ignore multiple calls to Update(m_maximum): it may sometimes be
            // troublesome to ensure that Update() is not called twice with the
            // same value (e.g. because of the rounding errors) and if we don't
            // return now we're going to generate asserts below
            return true;
        }

        // so that we return true below and that out [Cancel] handler knew what
        // to do
        m_state = Finished;
        if( !(GetWindowStyle() & wxPD_AUTO_HIDE) )
        {
            EnableClose();
            DisableSkip();
#if defined(__WXMSW__) && !defined(__WXUNIVERSAL__)
            EnableCloseButton();
#endif // __WXMSW__

            if ( newmsg.empty() )
            {
                // also provide the finishing message if the application didn't
                m_msg->SetLabel(_("Done."));
            }

            wxYieldIfNeeded() ;

            (void)ShowModal();
        }
        else // auto hide
        {
            // reenable other windows before hiding this one because otherwise
            // Windows wouldn't give the focus back to the window which had
            // been previously focused because it would still be disabled
            ReenableOtherWindows();

            Hide();
        }
    }
    else // not at maximum yet
    {
        return DoAfterUpdate(skip);
    }

    // update the display in case yielding above didn't do it
    Update();

    return m_state != Canceled;
}

bool wxProgressDialog::Pulse(const wxString& newmsg, bool *skip)
{
    wxASSERT_MSG( m_gauge, wxT("cannot update non existent dialog") );

    // show a bit of progress
    m_gauge->Pulse();

    UpdateMessage(newmsg);

    if (m_elapsed || m_remaining || m_estimated)
    {
        unsigned long elapsed = wxGetCurrentTime() - m_timeStart;

        SetTimeLabel(elapsed, m_elapsed);
        SetTimeLabel((unsigned long)-1, m_estimated);
        SetTimeLabel((unsigned long)-1, m_remaining);
    }

    return DoAfterUpdate(skip);
}

bool wxProgressDialog::DoAfterUpdate(bool *skip)
{
    // we have to yield because not only we want to update the display but
    // also to process the clicks on the cancel and skip buttons
    wxYieldIfNeeded();

    Update();

    if ( m_skip && skip && !*skip )
    {
        *skip = true;
        m_skip = false;
        EnableSkip();
    }

    return m_state != Canceled;
}

void wxProgressDialog::Resume()
{
    m_state = Continue;
    m_ctdelay = m_delay; // force an update of the elapsed/estimated/remaining time
    m_break += wxGetCurrentTime()-m_timeStop;

    EnableAbort();
    EnableSkip();
    m_skip = false;
}

bool wxProgressDialog::Show( bool show )
{
    // reenable other windows before hiding this one because otherwise
    // Windows wouldn't give the focus back to the window which had
    // been previously focused because it would still be disabled
    if(!show)
        ReenableOtherWindows();

    return wxDialog::Show(show);
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

void wxProgressDialog::OnCancel(wxCommandEvent& event)
{
    if ( m_state == Finished )
    {
        // this means that the count down is already finished and we're being
        // shown as a modal dialog - so just let the default handler do the job
        event.Skip();
    }
    else
    {
        // request to cancel was received, the next time Update() is called we
        // will handle it
        m_state = Canceled;

        // update the buttons state immediately so that the user knows that the
        // request has been noticed
        DisableAbort();
        DisableSkip();

        // save the time when the dialog was stopped
        m_timeStop = wxGetCurrentTime();
    }
}

void wxProgressDialog::OnSkip(wxCommandEvent& WXUNUSED(event))
{
    DisableSkip();
    m_skip = true;
}

void wxProgressDialog::OnClose(wxCloseEvent& event)
{
    if ( m_state == Uncancelable )
    {
        // can't close this dialog
        event.Veto();
    }
    else if ( m_state == Finished )
    {
        // let the default handler close the window as we already terminated
        event.Skip();
    }
    else
    {
        // next Update() will notice it
        m_state = Canceled;
        DisableAbort();
        DisableSkip();

        m_timeStop = wxGetCurrentTime();
    }
}

// ----------------------------------------------------------------------------
// destruction
// ----------------------------------------------------------------------------

wxProgressDialog::~wxProgressDialog()
{
    // normally this should have been already done, but just in case
    ReenableOtherWindows();
}

void wxProgressDialog::ReenableOtherWindows()
{
    if ( GetWindowStyle() & wxPD_APP_MODAL )
    {
        delete m_winDisabler;
        m_winDisabler = (wxWindowDisabler *)NULL;
    }
    else
    {
        if ( m_parentTop )
            m_parentTop->Enable();
    }
}

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static void SetTimeLabel(unsigned long val, wxStaticText *label)
{
    if ( label )
    {
        wxString s;

        if (val != (unsigned long)-1)
        {
        unsigned long hours = val / 3600;
        unsigned long minutes = (val % 3600) / 60;
        unsigned long seconds = val % 60;
        s.Printf(wxT("%lu:%02lu:%02lu"), hours, minutes, seconds);
        }
        else
        {
            s = _("Unknown");
        }

        if ( s != label->GetLabel() )
            label->SetLabel(s);
    }
}

void wxProgressDialog::EnableSkip(bool enable)
{
    if(m_hasSkipButton)
    {
#ifdef __SMARTPHONE__
        if(enable)
            SetRightMenu(wxID_SKIP, _("Skip"));
        else
            SetRightMenu();
#else
        if(m_btnSkip)
            m_btnSkip->Enable(enable);
#endif
    }
}

void wxProgressDialog::EnableAbort(bool enable)
{
    if(m_hasAbortButton)
    {
#ifdef __SMARTPHONE__
        if(enable)
            SetLeftMenu(wxID_CANCEL); // stock buttons makes Cancel label
        else
            SetLeftMenu();
#else
        if(m_btnAbort)
            m_btnAbort->Enable(enable);
#endif
    }
}

void wxProgressDialog::EnableClose()
{
    if(m_hasAbortButton)
    {
#ifdef __SMARTPHONE__
        SetLeftMenu(wxID_CANCEL, _("Close"));
#else
        if(m_btnAbort)
        {
            m_btnAbort->Enable();
            m_btnAbort->SetLabel(_("Close"));
        }
#endif
    }
}

void wxProgressDialog::UpdateMessage(const wxString &newmsg)
{
    if ( !newmsg.empty() && newmsg != m_msg->GetLabel() )
    {
        m_msg->SetLabel(newmsg);

        wxYieldIfNeeded() ;
    }
}

#endif // wxUSE_PROGRESSDLG
