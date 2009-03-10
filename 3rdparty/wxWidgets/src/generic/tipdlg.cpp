///////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/tipdlg.cpp
// Purpose:     implementation of wxTipDialog
// Author:      Vadim Zeitlin
// Modified by:
// Created:     28.06.99
// RCS-ID:      $Id: tipdlg.cpp 52000 2008-02-22 18:37:18Z VS $
// Copyright:   (c) Vadim Zeitlin
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

#if wxUSE_STARTUP_TIPS

#ifndef WX_PRECOMP
    #include "wx/button.h"
    #include "wx/checkbox.h"
    #include "wx/statbox.h"
    #include "wx/dialog.h"
    #include "wx/icon.h"
    #include "wx/intl.h"
    #include "wx/textctrl.h"
    #include "wx/statbmp.h"
    #include "wx/stattext.h"
    #include "wx/sizer.h"
    #include "wx/settings.h"
#endif // WX_PRECOMP

#include "wx/statline.h"
#include "wx/artprov.h"

#include "wx/tipdlg.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const int wxID_NEXT_TIP = 32000;  // whatever

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
// private classes
// ----------------------------------------------------------------------------

// an implementation which takes the tips from the text file - each line
// represents a tip
class WXDLLIMPEXP_ADV wxFileTipProvider : public wxTipProvider
{
public:
    wxFileTipProvider(const wxString& filename, size_t currentTip);

    virtual wxString GetTip();

private:
    wxTextFile m_textfile;

    DECLARE_NO_COPY_CLASS(wxFileTipProvider)
};

#ifdef __WIN32__
// TODO an implementation which takes the tips from the given registry key
class WXDLLIMPEXP_ADV wxRegTipProvider : public wxTipProvider
{
public:
    wxRegTipProvider(const wxString& keyname);

    virtual wxString GetTip();
};

// Empty implementation for now to keep the linker happy
wxString wxRegTipProvider::GetTip()
{
    return wxEmptyString;
}

#endif // __WIN32__

// the dialog we show in wxShowTip()
class WXDLLIMPEXP_ADV wxTipDialog : public wxDialog
{
public:
    wxTipDialog(wxWindow *parent,
                wxTipProvider *tipProvider,
                bool showAtStartup);

    // the tip dialog has "Show tips on startup" checkbox - return true if it
    // was checked (or wasn't unchecked)
    bool ShowTipsOnStartup() const { return m_checkbox->GetValue(); }

    // sets the (next) tip text
    void SetTipText() { m_text->SetValue(m_tipProvider->GetTip()); }

    // "Next" button handler
    void OnNextTip(wxCommandEvent& WXUNUSED(event)) { SetTipText(); }

private:
    wxTipProvider *m_tipProvider;

    wxTextCtrl *m_text;
    wxCheckBox *m_checkbox;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxTipDialog)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxFileTipProvider
// ----------------------------------------------------------------------------

wxFileTipProvider::wxFileTipProvider(const wxString& filename,
                                     size_t currentTip)
                 : wxTipProvider(currentTip), m_textfile(filename)
{
    m_textfile.Open();
}

wxString wxFileTipProvider::GetTip()
{
    size_t count = m_textfile.GetLineCount();
    if ( !count )
    {
        return _("Tips not available, sorry!");
    }

    wxString tip;

    // Comments start with a # symbol.
    // Loop reading lines until get the first one that isn't a comment.
    // The max number of loop executions is the number of lines in the
    // textfile so that can't go into an eternal loop in the [oddball]
    // case of a comment-only tips file, or the developer has vetoed
    // them all via PreprecessTip().
    for ( size_t i=0; i < count; i++ )
    {
        // The current tip may be at the last line of the textfile, (or
        // past it, if the number of lines in the textfile changed, such
        // as changing to a different textfile, with less tips). So check
        // to see at last line of text file, (or past it)...
        if ( m_currentTip >= count )
        {
            // .. and if so, wrap back to line 0.
            m_currentTip = 0;
        }

        // Read the tip, and increment the current tip counter.
        tip = m_textfile.GetLine(m_currentTip++);

        // Allow a derived class's overrided virtual to modify the tip
        // now if so desired.
        tip = PreprocessTip(tip);

        // Break if tip isn't a comment, and isn't an empty string
        // (or only stray space characters).
        if ( !tip.StartsWith(wxT("#")) && (tip.Trim() != wxEmptyString) )
        {
            break;
        }
    }

    // If tip starts with '_(', then it is a gettext string of format
    // _("My \"global\" tip text") so first strip off the leading '_("'...
    if ( tip.StartsWith(wxT("_(\"" ), &tip))
    {
        //...and strip off the trailing '")'...
        tip = tip.BeforeLast(wxT('\"'));
        // ...and replace escaped quotes
        tip.Replace(wxT("\\\""), wxT("\""));

        // and translate it as requested
        tip = wxGetTranslation(tip);
    }

    return tip;
}

// ----------------------------------------------------------------------------
// wxTipDialog
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxTipDialog, wxDialog)
    EVT_BUTTON(wxID_NEXT_TIP, wxTipDialog::OnNextTip)
END_EVENT_TABLE()

wxTipDialog::wxTipDialog(wxWindow *parent,
                         wxTipProvider *tipProvider,
                         bool showAtStartup)
           : wxDialog(parent, wxID_ANY, _("Tip of the Day"),
                      wxDefaultPosition, wxDefaultSize,
                      wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER
                      )
{
    m_tipProvider = tipProvider;
    bool isPda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);

    // 1) create all controls in tab order

    wxStaticText *text = new wxStaticText(this, wxID_ANY, _("Did you know..."));

    if (!isPda)
    {
        wxFont font = text->GetFont();
        font.SetPointSize(int(1.6 * font.GetPointSize()));
        font.SetWeight(wxFONTWEIGHT_BOLD);
        text->SetFont(font);
    }

    m_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                            wxDefaultPosition, wxSize(200, 160),
                            wxTE_MULTILINE |
                            wxTE_READONLY |
                            wxTE_NO_VSCROLL |
                            wxTE_RICH2 | // a hack to get rid of vert scrollbar
                            wxDEFAULT_CONTROL_BORDER
                            );
#if defined(__WXMSW__)
    m_text->SetFont(wxFont(12, wxSWISS, wxNORMAL, wxNORMAL));
#endif

//#if defined(__WXPM__)
    //
    // The only way to get icons into an OS/2 static bitmap control
    //
//    wxBitmap                        vBitmap;

//    vBitmap.SetId(wxICON_TIP); // OS/2 specific bitmap method--OS/2 wxBitmaps all have an ID.
//                               // and for StatBmp's under OS/2 it MUST be a valid resource ID.
//
//    wxStaticBitmap*                 bmp = new wxStaticBitmap(this, wxID_ANY, vBitmap);
//
//#else

    wxIcon icon = wxArtProvider::GetIcon(wxART_TIP, wxART_CMN_DIALOG);
    wxStaticBitmap *bmp = new wxStaticBitmap(this, wxID_ANY, icon);

//#endif

    m_checkbox = new wxCheckBox(this, wxID_ANY, _("&Show tips at startup"));
    m_checkbox->SetValue(showAtStartup);
    m_checkbox->SetFocus();

    // smart phones does not support or do not waste space for wxButtons
#ifndef __SMARTPHONE__
    wxButton *btnNext = new wxButton(this, wxID_NEXT_TIP, _("&Next Tip"));
#endif

    // smart phones does not support or do not waste space for wxButtons
#ifndef __SMARTPHONE__
    wxButton *btnClose = new wxButton(this, wxID_CLOSE);
    SetAffirmativeId(wxID_CLOSE);
#endif


    // 2) put them in boxes

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer *icon_text = new wxBoxSizer( wxHORIZONTAL );
    icon_text->Add( bmp, 0, wxCENTER );
    icon_text->Add( text, 1, wxCENTER | wxLEFT, wxLARGESMALL(20,0) );
    topsizer->Add( icon_text, 0, wxEXPAND | wxALL, wxLARGESMALL(10,0) );

    topsizer->Add( m_text, 1, wxEXPAND | wxLEFT|wxRIGHT, wxLARGESMALL(10,0) );

    wxBoxSizer *bottom = new wxBoxSizer( wxHORIZONTAL );
    if (isPda)
        topsizer->Add( m_checkbox, 0, wxCENTER|wxTOP );
    else
        bottom->Add( m_checkbox, 0, wxCENTER );

    // smart phones does not support or do not waste space for wxButtons
#ifdef __SMARTPHONE__
    SetRightMenu(wxID_NEXT_TIP, _("Next"));
    SetLeftMenu(wxID_CLOSE);
#else
    if (!isPda)
        bottom->Add( 10,10,1 );
    bottom->Add( btnNext, 0, wxCENTER | wxLEFT, wxLARGESMALL(10,0) );
    bottom->Add( btnClose, 0, wxCENTER | wxLEFT, wxLARGESMALL(10,0) );
#endif

    if (isPda)
        topsizer->Add( bottom, 0, wxCENTER | wxALL, 5 );
    else
        topsizer->Add( bottom, 0, wxEXPAND | wxALL, wxLARGESMALL(10,0) );

    SetTipText();

    SetSizer( topsizer );

    topsizer->SetSizeHints( this );
    topsizer->Fit( this );

    Centre(wxBOTH | wxCENTER_FRAME);
}

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------

wxTipProvider *wxCreateFileTipProvider(const wxString& filename,
                                       size_t currentTip)
{
    return new wxFileTipProvider(filename, currentTip);
}

bool wxShowTip(wxWindow *parent,
               wxTipProvider *tipProvider,
               bool showAtStartup)
{
    wxTipDialog dlg(parent, tipProvider, showAtStartup);
    dlg.ShowModal();

    return dlg.ShowTipsOnStartup();
}

#endif // wxUSE_STARTUP_TIPS
