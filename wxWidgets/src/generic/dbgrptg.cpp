///////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/dbgrptg.cpp
// Purpose:     implementation of wxDebugReportPreviewStd
// Author:      Vadim Zeitlin, Andrej Putrin
// Modified by:
// Created:     2005-01-21
// RCS-ID:      $Id: dbgrptg.cpp 61631 2009-08-09 15:55:56Z JS $
// Copyright:   (c) 2005 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// License:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

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

#if wxUSE_DEBUGREPORT && wxUSE_XML

#include "wx/debugrpt.h"

#ifndef WX_PRECOMP
    #include "wx/sizer.h"
    #include "wx/checklst.h"
    #include "wx/textctrl.h"
    #include "wx/intl.h"
    #include "wx/stattext.h"
    #include "wx/filedlg.h"
    #include "wx/valtext.h"
    #include "wx/button.h"
#endif // WX_PRECOMP

#include "wx/filename.h"
#include "wx/ffile.h"
#include "wx/mimetype.h"

#include "wx/statline.h"

#ifdef __WXMSW__
    #include "wx/evtloop.h"     // for SetCriticalWindow()
#endif // __WXMSW__

// ----------------------------------------------------------------------------
// wxDumpPreviewDlg: simple class for showing ASCII preview of dump files
// ----------------------------------------------------------------------------

class wxDumpPreviewDlg : public wxDialog
{
public:
    wxDumpPreviewDlg(wxWindow *parent,
                     const wxString& title,
                     const wxString& text);

private:
    // the text we show
    wxTextCtrl *m_text;

    DECLARE_NO_COPY_CLASS(wxDumpPreviewDlg)
};

wxDumpPreviewDlg::wxDumpPreviewDlg(wxWindow *parent,
                                   const wxString& title,
                                   const wxString& text)
                : wxDialog(parent, wxID_ANY, title,
                           wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // create controls
    // ---------------

    // use wxTE_RICH2 style to avoid 64kB limit under MSW and display big files
    // faster than with wxTE_RICH
    m_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                            wxPoint(0, 0), wxDefaultSize,
                            wxTE_MULTILINE |
                            wxTE_READONLY |
                            wxTE_NOHIDESEL |
                            wxTE_RICH2);
    m_text->SetValue(text);

    // use fixed-width font
    m_text->SetFont(wxFont(12, wxFONTFAMILY_TELETYPE,
                           wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    wxButton *btnClose = new wxButton(this, wxID_CANCEL, _("Close"));


    // layout them
    // -----------

    wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL),
            *sizerBtns = new wxBoxSizer(wxHORIZONTAL);

    sizerBtns->Add(btnClose, 0, 0, 1);

    sizerTop->Add(m_text, 1, wxEXPAND);
    sizerTop->Add(sizerBtns, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM | wxRIGHT, 1);

    // set the sizer &c
    // ----------------

    // make the text window bigger to show more contents of the file
    sizerTop->SetItemMinSize(m_text, 600, 300);
    SetSizer(sizerTop);

    Layout();
    Fit();

    m_text->SetFocus();
}

// ----------------------------------------------------------------------------
// wxDumpOpenExternalDlg: choose a command for opening the given file
// ----------------------------------------------------------------------------

class wxDumpOpenExternalDlg : public wxDialog
{
public:
    wxDumpOpenExternalDlg(wxWindow *parent, const wxFileName& filename);

    // return the command chosed by user to open this file
    const wxString& GetCommand() const { return m_command; }

    wxString m_command;

private:

#if wxUSE_FILEDLG
    void OnBrowse(wxCommandEvent& event);
#endif // wxUSE_FILEDLG

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxDumpOpenExternalDlg)
};

BEGIN_EVENT_TABLE(wxDumpOpenExternalDlg, wxDialog)

#if wxUSE_FILEDLG
    EVT_BUTTON(wxID_MORE, wxDumpOpenExternalDlg::OnBrowse)
#endif

END_EVENT_TABLE()


wxDumpOpenExternalDlg::wxDumpOpenExternalDlg(wxWindow *parent,
                                             const wxFileName& filename)
                     : wxDialog(parent,
                                wxID_ANY,
                                wxString::Format
                                (
                                    _("Open file \"%s\""),
                                    filename.GetFullPath().c_str()
                                ))
{
    // create controls
    // ---------------

    wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);
    sizerTop->Add(new wxStaticText(this, wxID_ANY,
                                   wxString::Format
                                   (
                                    _("Enter command to open file \"%s\":"),
                                    filename.GetFullName().c_str()
                                   )),
                  wxSizerFlags().Border());

    wxSizer *sizerH = new wxBoxSizer(wxHORIZONTAL);

    wxTextCtrl *command = new wxTextCtrl
                              (
                                this,
                                wxID_ANY,
                                wxEmptyString,
                                wxDefaultPosition,
                                wxSize(250, wxDefaultCoord),
                                0
#if wxUSE_VALIDATORS
                                ,wxTextValidator(wxFILTER_NONE, &m_command)
#endif
                              );
    sizerH->Add(command,
                    wxSizerFlags(1).Align(wxALIGN_CENTER_VERTICAL));

#if wxUSE_FILEDLG

    wxButton *browse = new wxButton(this, wxID_MORE, wxT(">>"),
                                    wxDefaultPosition, wxDefaultSize,
                                    wxBU_EXACTFIT);
    sizerH->Add(browse,
                wxSizerFlags(0).Align(wxALIGN_CENTER_VERTICAL). Border(wxLEFT));

#endif // wxUSE_FILEDLG

    sizerTop->Add(sizerH, wxSizerFlags(0).Expand().Border());

    sizerTop->Add(new wxStaticLine(this), wxSizerFlags().Expand().Border());

    sizerTop->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                      wxSizerFlags().Align(wxALIGN_RIGHT).Border());

    // set the sizer &c
    // ----------------

    SetSizer(sizerTop);

    Layout();
    Fit();

    command->SetFocus();
}

#if wxUSE_FILEDLG

void wxDumpOpenExternalDlg::OnBrowse(wxCommandEvent& )
{
    wxFileName fname(m_command);
    wxFileDialog dlg(this,
                     wxFileSelectorPromptStr,
                     fname.GetPathWithSep(),
                     fname.GetFullName()
#ifdef __WXMSW__
                     , _("Executable files (*.exe)|*.exe|All files (*.*)|*.*||")
#endif // __WXMSW__
                     );
    if ( dlg.ShowModal() == wxID_OK )
    {
        m_command = dlg.GetPath();
        TransferDataToWindow();
    }
}

#endif // wxUSE_FILEDLG

// ----------------------------------------------------------------------------
// wxDebugReportDialog: class showing debug report to the user
// ----------------------------------------------------------------------------

class wxDebugReportDialog : public wxDialog
{
public:
    wxDebugReportDialog(wxDebugReport& dbgrpt);

    virtual bool TransferDataToWindow();
    virtual bool TransferDataFromWindow();

private:
    void OnView(wxCommandEvent& );
    void OnViewUpdate(wxUpdateUIEvent& );
    void OnOpen(wxCommandEvent& );


    // small helper: add wxEXPAND and wxALL flags
    static wxSizerFlags SizerFlags(int proportion)
    {
        return wxSizerFlags(proportion).Expand().Border();
    }


    wxDebugReport& m_dbgrpt;

    wxCheckListBox *m_checklst;
    wxTextCtrl *m_notes;

    wxArrayString m_files;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxDebugReportDialog)
};

// ============================================================================
// wxDebugReportDialog implementation
// ============================================================================

BEGIN_EVENT_TABLE(wxDebugReportDialog, wxDialog)
    EVT_BUTTON(wxID_VIEW_DETAILS, wxDebugReportDialog::OnView)
    EVT_UPDATE_UI(wxID_VIEW_DETAILS, wxDebugReportDialog::OnViewUpdate)
    EVT_BUTTON(wxID_OPEN, wxDebugReportDialog::OnOpen)
    EVT_UPDATE_UI(wxID_OPEN, wxDebugReportDialog::OnViewUpdate)
END_EVENT_TABLE()


// ----------------------------------------------------------------------------
// construction
// ----------------------------------------------------------------------------

wxDebugReportDialog::wxDebugReportDialog(wxDebugReport& dbgrpt)
                   : wxDialog(NULL, wxID_ANY,
                              wxString::Format(_("Debug report \"%s\""),
                              dbgrpt.GetReportName().c_str()),
                              wxDefaultPosition,
                              wxDefaultSize,
                              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
                     m_dbgrpt(dbgrpt)
{
    // upper part of the dialog: explanatory message
    wxString msg;
    wxString debugDir = dbgrpt.GetDirectory();

    // The temporary directory can be the short form on Windows;
    // normalize it for the benefit of users.
#ifdef __WXMSW__
    wxFileName debugDirFilename(debugDir, wxEmptyString);
    debugDirFilename.Normalize(wxPATH_NORM_LONG);
    debugDir = debugDirFilename.GetPath();
#endif
    msg << _("A debug report has been generated in the directory\n")
        << _T('\n')
        << _T("             \"") << debugDir << _T("\"\n")
        << _T('\n')
        << _("The report contains the files listed below. If any of these files contain private information,\nplease uncheck them and they will be removed from the report.\n")
        << _T('\n')
        << _("If you wish to suppress this debug report completely, please choose the \"Cancel\" button,\nbut be warned that it may hinder improving the program, so if\nat all possible please do continue with the report generation.\n")
        << _T('\n')
        << _("              Thank you and we're sorry for the inconvenience!\n")
        << _T("\n\n"); // just some white space to separate from other stuff

    const wxSizerFlags flagsFixed(SizerFlags(0));
    const wxSizerFlags flagsExpand(SizerFlags(1));
    const wxSizerFlags flagsExpand2(SizerFlags(2));

    wxSizer *sizerPreview =
        new wxStaticBoxSizer(wxVERTICAL, this, _("&Debug report preview:"));
    sizerPreview->Add(CreateTextSizer(msg), SizerFlags(0).Centre());

    // ... and the list of files in this debug report with buttons to view them
    wxSizer *sizerFileBtns = new wxBoxSizer(wxVERTICAL);
    sizerFileBtns->AddStretchSpacer(1);
    sizerFileBtns->Add(new wxButton(this, wxID_VIEW_DETAILS, _("&View...")),
                        wxSizerFlags().Border(wxBOTTOM));
    sizerFileBtns->Add(new wxButton(this, wxID_OPEN, _("&Open...")),
                        wxSizerFlags().Border(wxTOP));
    sizerFileBtns->AddStretchSpacer(1);

    m_checklst = new wxCheckListBox(this, wxID_ANY);

    wxSizer *sizerFiles = new wxBoxSizer(wxHORIZONTAL);
    sizerFiles->Add(m_checklst, flagsExpand);
    sizerFiles->Add(sizerFileBtns, flagsFixed);

    sizerPreview->Add(sizerFiles, flagsExpand2);


    // lower part of the dialog: notes field
    wxSizer *sizerNotes = new wxStaticBoxSizer(wxVERTICAL, this, _("&Notes:"));

    msg = _("If you have any additional information pertaining to this bug\nreport, please enter it here and it will be joined to it:");

    m_notes = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE);

    sizerNotes->Add(CreateTextSizer(msg), flagsFixed);
    sizerNotes->Add(m_notes, flagsExpand);


    wxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);
    sizerTop->Add(sizerPreview, flagsExpand2);
    sizerTop->AddSpacer(5);
    sizerTop->Add(sizerNotes, flagsExpand);
    sizerTop->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), flagsFixed);

    SetSizerAndFit(sizerTop);
    Layout();
    CentreOnScreen();
}

// ----------------------------------------------------------------------------
// data exchange
// ----------------------------------------------------------------------------

bool wxDebugReportDialog::TransferDataToWindow()
{
    // all files are included in the report by default
    const size_t count = m_dbgrpt.GetFilesCount();
    for ( size_t n = 0; n < count; n++ )
    {
        wxString name,
            desc;
        if ( m_dbgrpt.GetFile(n, &name, &desc) )
        {
            m_checklst->Append(name + _T(" (") + desc + _T(')'));
            m_checklst->Check(n);

            m_files.Add(name);
        }
    }

    return true;
}

bool wxDebugReportDialog::TransferDataFromWindow()
{
    // any unchecked files should be removed from the report
    const size_t count = m_checklst->GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        if ( !m_checklst->IsChecked(n) )
        {
            m_dbgrpt.RemoveFile(m_files[n]);
        }
    }

    // if the user entered any notes, add them to the report
    const wxString notes = m_notes->GetValue();
    if ( !notes.empty() )
    {
        // for now filename fixed, could make it configurable in the future...
        m_dbgrpt.AddText(_T("notes.txt"), notes, _T("user notes"));
    }

    return true;
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

void wxDebugReportDialog::OnView(wxCommandEvent& )
{
    const int sel = m_checklst->GetSelection();
    wxCHECK_RET( sel != wxNOT_FOUND, _T("invalid selection in OnView()") );

    wxFileName fn(m_dbgrpt.GetDirectory(), m_files[sel]);
    wxString str;

    wxFFile file(fn.GetFullPath());
    if ( file.IsOpened() && file.ReadAll(&str) )
    {
        wxDumpPreviewDlg dlg(this, m_files[sel], str);
        dlg.ShowModal();
    }
}

void wxDebugReportDialog::OnOpen(wxCommandEvent& )
{
    const int sel = m_checklst->GetSelection();
    wxCHECK_RET( sel != wxNOT_FOUND, _T("invalid selection in OnOpen()") );

    wxFileName fn(m_dbgrpt.GetDirectory(), m_files[sel]);

    // try to get the command to open this kind of files ourselves
    wxString command;
#if wxUSE_MIMETYPE
    wxFileType *
        ft = wxTheMimeTypesManager->GetFileTypeFromExtension(fn.GetExt());
    if ( ft )
    {
        command = ft->GetOpenCommand(fn.GetFullPath());
        delete ft;
    }
#endif // wxUSE_MIMETYPE

    // if we couldn't, ask the user
    if ( command.empty() )
    {
        wxDumpOpenExternalDlg dlg(this, fn);
        if ( dlg.ShowModal() == wxID_OK )
        {
            // get the command chosen by the user and append file name to it

            // if we don't have place marker for file name in the command...
            wxString cmd = dlg.GetCommand();
            if ( !cmd.empty() )
            {
#if wxUSE_MIMETYPE
                if ( cmd.find(_T('%')) != wxString::npos )
                {
                    command = wxFileType::ExpandCommand(cmd, fn.GetFullPath());
                }
                else // no %s nor %1
#endif // wxUSE_MIMETYPE
                {
                    // append the file name to the end
                    command << cmd << _T(" \"") << fn.GetFullPath() << _T('"');
                }
            }
        }
    }

    if ( !command.empty() )
        ::wxExecute(command);
}

void wxDebugReportDialog::OnViewUpdate(wxUpdateUIEvent& event)
{
    int sel = m_checklst->GetSelection();
    if (sel >= 0)
    {
        wxFileName fn(m_dbgrpt.GetDirectory(), m_files[sel]);
        event.Enable(fn.FileExists());
    }
    else
        event.Enable(false);
}


// ============================================================================
// wxDebugReportPreviewStd implementation
// ============================================================================

bool wxDebugReportPreviewStd::Show(wxDebugReport& dbgrpt) const
{
    if ( !dbgrpt.GetFilesCount() )
        return false;

    wxDebugReportDialog dlg(dbgrpt);

#ifdef __WXMSW__
    // before entering the event loop (from ShowModal()), block the event
    // handling for all other windows as this could result in more crashes
    wxEventLoop::SetCriticalWindow(&dlg);
#endif // __WXMSW__

    return dlg.ShowModal() == wxID_OK && dbgrpt.GetFilesCount() != 0;
}

#endif // wxUSE_DEBUGREPORT && wxUSE_XML
