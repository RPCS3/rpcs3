/////////////////////////////////////////////////////////////////////////////
// Name:        helpctrl.cpp
// Purpose:     wxHtmlHelpController
// Notes:       Based on htmlhelp.cpp, implementing a monolithic
//              HTML Help controller class,  by Vaclav Slavik
// Author:      Harm van der Heijden and Vaclav Slavik
// RCS-ID:      $Id: helpctrl.cpp 52470 2008-03-13 23:29:27Z VS $
// Copyright:   (c) Harm van der Heijden and Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_WXHTML_HELP

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/intl.h"
#endif // WX_PRECOMP

#include "wx/busyinfo.h"
#include "wx/html/helpctrl.h"
#include "wx/html/helpwnd.h"
#include "wx/html/helpfrm.h"
#include "wx/html/helpdlg.h"

#if wxUSE_HELP
    #include "wx/tipwin.h"
#endif

#if wxUSE_LIBMSPACK
#include "wx/html/forcelnk.h"
FORCE_LINK(wxhtml_chm_support)
#endif

IMPLEMENT_DYNAMIC_CLASS(wxHtmlHelpController, wxHelpControllerBase)

wxHtmlHelpController::wxHtmlHelpController(int style, wxWindow* parentWindow):
    wxHelpControllerBase(parentWindow)
{
    m_helpWindow = NULL;
    m_helpFrame = NULL;
    m_helpDialog = NULL;
    m_Config = NULL;
    m_ConfigRoot = wxEmptyString;
    m_titleFormat = _("Help: %s");
    m_FrameStyle = style;
}

wxHtmlHelpController::~wxHtmlHelpController()
{
    if (m_Config)
        WriteCustomization(m_Config, m_ConfigRoot);
    if (m_helpWindow)
        DestroyHelpWindow();
}


void wxHtmlHelpController::DestroyHelpWindow()
{
    if (m_FrameStyle & wxHF_EMBEDDED)
        return;

    // Find top-most parent window
    // If a modal dialog
    wxWindow* parent = FindTopLevelWindow();
    if (parent)
    {
        wxDialog* dialog = wxDynamicCast(parent, wxDialog);
        if (dialog && dialog->IsModal())
        {
            dialog->EndModal(wxID_OK);
        }
        parent->Destroy();
        m_helpWindow = NULL;
    }
    m_helpDialog = NULL;
    m_helpFrame = NULL;
}

void wxHtmlHelpController::OnCloseFrame(wxCloseEvent& evt)
{
    if (m_Config)
        WriteCustomization(m_Config, m_ConfigRoot);

    evt.Skip();

    OnQuit();

    if ( m_helpWindow )
        m_helpWindow->SetController(NULL);
    m_helpWindow = NULL;
    m_helpDialog = NULL;
    m_helpFrame = NULL;
}

void wxHtmlHelpController::SetTitleFormat(const wxString& title)
{
    m_titleFormat = title;
    wxHtmlHelpFrame* frame = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpFrame);
    wxHtmlHelpDialog* dialog = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpDialog);
    if (frame)
    {
        frame->SetTitleFormat(title);
    }
    else if (dialog)
        dialog->SetTitleFormat(title);
}

// Find the top-most parent window
wxWindow* wxHtmlHelpController::FindTopLevelWindow()
{
    wxWindow* parent = m_helpWindow;
    while (parent && !parent->IsTopLevel())
    {
        parent = parent->GetParent();
    }
    return parent;
}

bool wxHtmlHelpController::AddBook(const wxFileName& book_file, bool show_wait_msg)
{
    return AddBook(wxFileSystem::FileNameToURL(book_file), show_wait_msg);
}

bool wxHtmlHelpController::AddBook(const wxString& book, bool show_wait_msg)
{
    wxBusyCursor cur;
#if wxUSE_BUSYINFO
    wxBusyInfo* busy = NULL;
    wxString info;
    if (show_wait_msg)
    {
        info.Printf(_("Adding book %s"), book.c_str());
        busy = new wxBusyInfo(info);
    }
#endif
    bool retval = m_helpData.AddBook(book);
#if wxUSE_BUSYINFO
    if (show_wait_msg)
        delete busy;
#else
    wxUnusedVar(show_wait_msg);
#endif
    if (m_helpWindow)
        m_helpWindow->RefreshLists();
    return retval;
}

wxHtmlHelpFrame* wxHtmlHelpController::CreateHelpFrame(wxHtmlHelpData *data)
{
    wxHtmlHelpFrame* frame = new wxHtmlHelpFrame(data);
    frame->SetController(this);
    frame->Create(m_parentWindow, -1, wxEmptyString, m_FrameStyle, m_Config, m_ConfigRoot);
    frame->SetTitleFormat(m_titleFormat);    
    m_helpFrame = frame;
    return frame;
}

wxHtmlHelpDialog* wxHtmlHelpController::CreateHelpDialog(wxHtmlHelpData *data)
{
    wxHtmlHelpDialog* dialog = new wxHtmlHelpDialog(data);
    dialog->SetController(this);
    dialog->SetTitleFormat(m_titleFormat);    
    dialog->Create(m_parentWindow, -1, wxEmptyString, m_FrameStyle);
    m_helpDialog = dialog;
    return dialog;
}

wxWindow* wxHtmlHelpController::CreateHelpWindow()
{
    if (m_helpWindow)
    {
        if (m_FrameStyle & wxHF_EMBEDDED)
            return m_helpWindow;

        wxWindow* topLevelWindow = FindTopLevelWindow();
        if (topLevelWindow)
            topLevelWindow->Raise();
        return m_helpWindow;
    }

    if (m_Config == NULL)
    {
        m_Config = wxConfigBase::Get(false);
        if (m_Config != NULL)
            m_ConfigRoot = _T("wxWindows/wxHtmlHelpController");
    }

    if (m_FrameStyle & wxHF_DIALOG)
    {
        wxHtmlHelpDialog* dialog = CreateHelpDialog(&m_helpData);
        m_helpWindow = dialog->GetHelpWindow();
    }
    else if ((m_FrameStyle & wxHF_EMBEDDED) && m_parentWindow)
    {
        m_helpWindow = new wxHtmlHelpWindow(m_parentWindow, -1, wxDefaultPosition, wxDefaultSize,
            wxTAB_TRAVERSAL|wxNO_BORDER, m_FrameStyle, &m_helpData);
    }
    else // wxHF_FRAME
    {
        wxHtmlHelpFrame* frame = CreateHelpFrame(&m_helpData);
        m_helpWindow = frame->GetHelpWindow();
        frame->Show(true);
    }

    return m_helpWindow;
}

void wxHtmlHelpController::ReadCustomization(wxConfigBase* cfg, const wxString& path)
{
    /* should not be called by the user; call UseConfig, and the controller
     * will do the rest */
    if (m_helpWindow && cfg)
        m_helpWindow->ReadCustomization(cfg, path);
}

void wxHtmlHelpController::WriteCustomization(wxConfigBase* cfg, const wxString& path)
{
    /* typically called by the controllers OnCloseFrame handler */
    if (m_helpWindow && cfg)
        m_helpWindow->WriteCustomization(cfg, path);
}

void wxHtmlHelpController::UseConfig(wxConfigBase *config, const wxString& rootpath)
{
    m_Config = config;
    m_ConfigRoot = rootpath;
    if (m_helpWindow) m_helpWindow->UseConfig(config, rootpath);
    ReadCustomization(config, rootpath);
}

//// Backward compatibility with wxHelpController API

bool wxHtmlHelpController::Initialize(const wxString& file)
{
    wxString dir, filename, ext;
    wxSplitPath(file, & dir, & filename, & ext);

    if (!dir.empty())
        dir = dir + wxFILE_SEP_PATH;

    // Try to find a suitable file
    wxString actualFilename = dir + filename + wxString(wxT(".zip"));
    if (!wxFileExists(actualFilename))
    {
        actualFilename = dir + filename + wxString(wxT(".htb"));
        if (!wxFileExists(actualFilename))
        {
            actualFilename = dir + filename + wxString(wxT(".hhp"));
            if (!wxFileExists(actualFilename))
            {
#if wxUSE_LIBMSPACK
                actualFilename = dir + filename + wxString(wxT(".chm"));
                if (!wxFileExists(actualFilename))
#endif
                    return false;
            }
        }
    }
    return AddBook(wxFileName(actualFilename));
}

bool wxHtmlHelpController::LoadFile(const wxString& WXUNUSED(file))
{
    // Don't reload the file or we'll have it appear again, presumably.
    return true;
}

bool wxHtmlHelpController::DisplaySection(int sectionNo)
{
    return Display(sectionNo);
}

bool wxHtmlHelpController::DisplayTextPopup(const wxString& text, const wxPoint& WXUNUSED(pos))
{
#if wxUSE_TIPWINDOW
    static wxTipWindow* s_tipWindow = NULL;

    if (s_tipWindow)
    {
        // Prevent s_tipWindow being nulled in OnIdle,
        // thereby removing the chance for the window to be closed by ShowHelp
        s_tipWindow->SetTipWindowPtr(NULL);
        s_tipWindow->Close();
    }
    s_tipWindow = NULL;

    if ( !text.empty() )
    {
        s_tipWindow = new wxTipWindow(wxTheApp->GetTopWindow(), text, 100, & s_tipWindow);

        return true;
    }
#else
    wxUnusedVar(text);
#endif // wxUSE_TIPWINDOW

    return false;
}

void wxHtmlHelpController::SetHelpWindow(wxHtmlHelpWindow* helpWindow)
{
    m_helpWindow = helpWindow;
    if (helpWindow)
        helpWindow->SetController(this);
}

void wxHtmlHelpController::SetFrameParameters(const wxString& title,
                                   const wxSize& size,
                                   const wxPoint& pos,
                                   bool WXUNUSED(newFrameEachTime))
{
    SetTitleFormat(title);
    wxHtmlHelpFrame* frame = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpFrame);
    wxHtmlHelpDialog* dialog = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpDialog);
    if (frame)
        frame->SetSize(pos.x, pos.y, size.x, size.y);
    else if (dialog)
        dialog->SetSize(pos.x, pos.y, size.x, size.y);
}

wxFrame* wxHtmlHelpController::GetFrameParameters(wxSize *size,
                                   wxPoint *pos,
                                   bool *newFrameEachTime)
{
    if (newFrameEachTime)
        (* newFrameEachTime) = false;

    wxHtmlHelpFrame* frame = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpFrame);
    wxHtmlHelpDialog* dialog = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpDialog);
    if (frame)
    {
        if (size)
            (* size) = frame->GetSize();
        if (pos)
            (* pos) = frame->GetPosition();
        return frame;
    }
    else if (dialog)
    {
        if (size)
            (* size) = dialog->GetSize();
        if (pos)
            (* pos) = dialog->GetPosition();
        return NULL;
    }
    return NULL;
}

bool wxHtmlHelpController::Quit()
{
    DestroyHelpWindow();
    return true;
}

// Make the help controller's frame 'modal' if
// needed
void wxHtmlHelpController::MakeModalIfNeeded()
{
    if ((m_FrameStyle & wxHF_EMBEDDED) == 0)
    {
        wxHtmlHelpFrame* frame = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpFrame);
        wxHtmlHelpDialog* dialog = wxDynamicCast(FindTopLevelWindow(), wxHtmlHelpDialog);
        if (frame)
            frame->AddGrabIfNeeded();
        else if (dialog && (m_FrameStyle & wxHF_MODAL))
        {
            dialog->ShowModal();
        }
    }
}

bool wxHtmlHelpController::Display(const wxString& x)
{
    CreateHelpWindow();
    bool success = m_helpWindow->Display(x);
    MakeModalIfNeeded();
    return success;
}

bool wxHtmlHelpController::Display(int id)
{
    CreateHelpWindow();
    bool success = m_helpWindow->Display(id);
    MakeModalIfNeeded();
    return success;
}

bool wxHtmlHelpController::DisplayContents()
{
    CreateHelpWindow();
    bool success = m_helpWindow->DisplayContents();
    MakeModalIfNeeded();
    return success;
}

bool wxHtmlHelpController::DisplayIndex()
{
    CreateHelpWindow();
    bool success = m_helpWindow->DisplayIndex();
    MakeModalIfNeeded();
    return success;
}

bool wxHtmlHelpController::KeywordSearch(const wxString& keyword,
                                         wxHelpSearchMode mode)
{
    CreateHelpWindow();
    bool success = m_helpWindow->KeywordSearch(keyword, mode);
    MakeModalIfNeeded();
    return success;
}

/*
 * wxHtmlModalHelp
 * A convenience class, to use like this:
 *
 * wxHtmlModalHelp help(parent, helpFile, topic);
 */

wxHtmlModalHelp::wxHtmlModalHelp(wxWindow* parent, const wxString& helpFile, const wxString& topic, int style)
{
    // Force some mandatory styles
    style |= wxHF_DIALOG | wxHF_MODAL;

    wxHtmlHelpController controller(style, parent);
    controller.Initialize(helpFile);

    if (topic.IsEmpty())
        controller.DisplayContents();
    else
        controller.DisplaySection(topic);
}

#endif // wxUSE_WXHTML_HELP

