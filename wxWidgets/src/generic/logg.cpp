/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/logg.cpp
// Purpose:     wxLog-derived classes which need GUI support (the rest is in
//              src/common/log.cpp)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.09.99 (extracted from src/common/log.cpp)
// RCS-ID:      $Id: logg.cpp 43078 2006-11-04 23:46:02Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
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

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/button.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/menu.h"
    #include "wx/frame.h"
    #include "wx/filedlg.h"
    #include "wx/msgdlg.h"
    #include "wx/textctrl.h"
    #include "wx/sizer.h"
    #include "wx/statbmp.h"
    #include "wx/settings.h"
#endif // WX_PRECOMP

#if wxUSE_LOGGUI || wxUSE_LOGWINDOW

#include "wx/file.h"
#include "wx/textfile.h"
#include "wx/statline.h"
#include "wx/artprov.h"

#ifdef  __WXMSW__
    // for OutputDebugString()
    #include  "wx/msw/private.h"
#endif // Windows

#ifdef  __WXPM__
    #include <time.h>
#endif

#if wxUSE_LOG_DIALOG
    #include "wx/listctrl.h"
    #include "wx/imaglist.h"
    #include "wx/image.h"
#endif // wxUSE_LOG_DIALOG/!wxUSE_LOG_DIALOG

#if defined(__MWERKS__) && wxUSE_UNICODE
    #include <wtime.h>
#endif

#include "wx/datetime.h"

// the suffix we add to the button to show that the dialog can be expanded
#define EXPAND_SUFFIX _T(" >>")

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

#if wxUSE_LOG_DIALOG

// this function is a wrapper around strftime(3)
// allows to exclude the usage of wxDateTime
static wxString TimeStamp(const wxChar *format, time_t t)
{
#if wxUSE_DATETIME
    wxChar buf[4096];
    struct tm tm;
    if ( !wxStrftime(buf, WXSIZEOF(buf), format, wxLocaltime_r(&t, &tm)) )
    {
        // buffer is too small?
        wxFAIL_MSG(_T("strftime() failed"));
    }
    return wxString(buf);
#else // !wxUSE_DATETIME
    return wxEmptyString;
#endif // wxUSE_DATETIME/!wxUSE_DATETIME
}


class wxLogDialog : public wxDialog
{
public:
    wxLogDialog(wxWindow *parent,
                const wxArrayString& messages,
                const wxArrayInt& severity,
                const wxArrayLong& timess,
                const wxString& caption,
                long style);
    virtual ~wxLogDialog();

    // event handlers
    void OnOk(wxCommandEvent& event);
    void OnDetails(wxCommandEvent& event);
#if wxUSE_FILE
    void OnSave(wxCommandEvent& event);
#endif // wxUSE_FILE
    void OnListSelect(wxListEvent& event);

private:
    // create controls needed for the details display
    void CreateDetailsControls();

    // the data for the listctrl
    wxArrayString m_messages;
    wxArrayInt m_severity;
    wxArrayLong m_times;

    // the "toggle" button and its state
#ifndef __SMARTPHONE__
    wxButton *m_btnDetails;
#endif
    bool      m_showingDetails;

    // the controls which are not shown initially (but only when details
    // button is pressed)
    wxListCtrl *m_listctrl;
#ifndef __SMARTPHONE__
#if wxUSE_STATLINE
    wxStaticLine *m_statline;
#endif // wxUSE_STATLINE
#if wxUSE_FILE
    wxButton *m_btnSave;
#endif // wxUSE_FILE
#endif // __SMARTPHONE__

    // the translated "Details" string
    static wxString ms_details;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxLogDialog)
};

BEGIN_EVENT_TABLE(wxLogDialog, wxDialog)
    EVT_BUTTON(wxID_OK, wxLogDialog::OnOk)
    EVT_BUTTON(wxID_MORE,   wxLogDialog::OnDetails)
#if wxUSE_FILE
    EVT_BUTTON(wxID_SAVE,   wxLogDialog::OnSave)
#endif // wxUSE_FILE
    EVT_LIST_ITEM_SELECTED(wxID_ANY, wxLogDialog::OnListSelect)
END_EVENT_TABLE()

#endif // wxUSE_LOG_DIALOG

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

#if wxUSE_FILE && wxUSE_FILEDLG

// pass an uninitialized file object, the function will ask the user for the
// filename and try to open it, returns true on success (file was opened),
// false if file couldn't be opened/created and -1 if the file selection
// dialog was cancelled
static int OpenLogFile(wxFile& file, wxString *filename = NULL, wxWindow *parent = NULL);

#endif // wxUSE_FILE

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

// we use a global variable to store the frame pointer for wxLogStatus - bad,
// but it's the easiest way
static wxFrame *gs_pFrame = NULL; // FIXME MT-unsafe

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// accepts an additional argument which tells to which frame the output should
// be directed
void wxVLogStatus(wxFrame *pFrame, const wxChar *szFormat, va_list argptr)
{
  wxString msg;

  wxLog *pLog = wxLog::GetActiveTarget();
  if ( pLog != NULL ) {
    msg.PrintfV(szFormat, argptr);

    wxASSERT( gs_pFrame == NULL ); // should be reset!
    gs_pFrame = pFrame;
#ifdef __WXWINCE__
    wxLog::OnLog(wxLOG_Status, msg, 0);
#else
    wxLog::OnLog(wxLOG_Status, msg, time(NULL));
#endif
    gs_pFrame = (wxFrame *) NULL;
  }
}

void wxLogStatus(wxFrame *pFrame, const wxChar *szFormat, ...)
{
    va_list argptr;
    va_start(argptr, szFormat);
    wxVLogStatus(pFrame, szFormat, argptr);
    va_end(argptr);
}

// ----------------------------------------------------------------------------
// wxLogGui implementation (FIXME MT-unsafe)
// ----------------------------------------------------------------------------

#if wxUSE_LOGGUI

wxLogGui::wxLogGui()
{
    Clear();
}

void wxLogGui::Clear()
{
    m_bErrors =
    m_bWarnings =
    m_bHasMessages = false;

    m_aMessages.Empty();
    m_aSeverity.Empty();
    m_aTimes.Empty();
}

void wxLogGui::Flush()
{
    if ( !m_bHasMessages )
        return;

    // do it right now to block any new calls to Flush() while we're here
    m_bHasMessages = false;

    unsigned repeatCount = 0;
    if ( wxLog::GetRepetitionCounting() )
    {
        repeatCount = wxLog::DoLogNumberOfRepeats();
    }

    wxString appName = wxTheApp->GetAppName();
    if ( !appName.empty() )
        appName[0u] = (wxChar)wxToupper(appName[0u]);

    long style;
    wxString titleFormat;
    if ( m_bErrors ) {
        titleFormat = _("%s Error");
        style = wxICON_STOP;
    }
    else if ( m_bWarnings ) {
        titleFormat = _("%s Warning");
        style = wxICON_EXCLAMATION;
    }
    else {
        titleFormat = _("%s Information");
        style = wxICON_INFORMATION;
    }

    wxString title;
    title.Printf(titleFormat, appName.c_str());

    size_t nMsgCount = m_aMessages.Count();

    // avoid showing other log dialogs until we're done with the dialog we're
    // showing right now: nested modal dialogs make for really bad UI!
    Suspend();

    wxString str;
    if ( nMsgCount == 1 )
    {
        str = m_aMessages[0];
    }
    else // more than one message
    {
#if wxUSE_LOG_DIALOG

        if ( repeatCount > 0 )
            m_aMessages[nMsgCount-1] += wxString::Format(wxT(" (%s)"), m_aMessages[nMsgCount-2].c_str());
        wxLogDialog dlg(NULL,
                        m_aMessages, m_aSeverity, m_aTimes,
                        title, style);

        // clear the message list before showing the dialog because while it's
        // shown some new messages may appear
        Clear();

        (void)dlg.ShowModal();
#else // !wxUSE_LOG_DIALOG
        // concatenate all strings (but not too many to not overfill the msg box)
        size_t nLines = 0;

        // start from the most recent message
        for ( size_t n = nMsgCount; n > 0; n-- ) {
            // for Windows strings longer than this value are wrapped (NT 4.0)
            const size_t nMsgLineWidth = 156;

            nLines += (m_aMessages[n - 1].Len() + nMsgLineWidth - 1) / nMsgLineWidth;

            if ( nLines > 25 )  // don't put too many lines in message box
                break;

            str << m_aMessages[n - 1] << wxT("\n");
        }
#endif // wxUSE_LOG_DIALOG/!wxUSE_LOG_DIALOG
    }

    // this catches both cases of 1 message with wxUSE_LOG_DIALOG and any
    // situation without it
    if ( !str.empty() )
    {
        wxMessageBox(str, title, wxOK | style);

        // no undisplayed messages whatsoever
        Clear();
    }

    // allow flushing the logs again
    Resume();
}

// log all kinds of messages
void wxLogGui::DoLog(wxLogLevel level, const wxChar *szString, time_t t)
{
    switch ( level ) {
        case wxLOG_Info:
            if ( GetVerbose() )
        case wxLOG_Message:
            {
                m_aMessages.Add(szString);
                m_aSeverity.Add(wxLOG_Message);
                m_aTimes.Add((long)t);
                m_bHasMessages = true;
            }
            break;

        case wxLOG_Status:
#if wxUSE_STATUSBAR
            {
                // find the top window and set it's status text if it has any
                wxFrame *pFrame = gs_pFrame;
                if ( pFrame == NULL ) {
                    wxWindow *pWin = wxTheApp->GetTopWindow();
                    if ( pWin != NULL && pWin->IsKindOf(CLASSINFO(wxFrame)) ) {
                        pFrame = (wxFrame *)pWin;
                    }
                }

                if ( pFrame && pFrame->GetStatusBar() )
                    pFrame->SetStatusText(szString);
            }
#endif // wxUSE_STATUSBAR
            break;

        case wxLOG_Trace:
        case wxLOG_Debug:
            #ifdef __WXDEBUG__
            {
                wxString str;
                TimeStamp(&str);
                str += szString;

                #if defined(__WXMSW__) && !defined(__WXMICROWIN__)
                    // don't prepend debug/trace here: it goes to the
                    // debug window anyhow
                    str += wxT("\r\n");
                    OutputDebugString(str);
                #else
                    // send them to stderr
                    wxFprintf(stderr, wxT("[%s] %s\n"),
                              level == wxLOG_Trace ? wxT("Trace")
                                                   : wxT("Debug"),
                              str.c_str());
                    fflush(stderr);
                #endif
            }
            #endif // __WXDEBUG__

            break;

        case wxLOG_FatalError:
            // show this one immediately
            wxMessageBox(szString, _("Fatal error"), wxICON_HAND);
            wxExit();
            break;

        case wxLOG_Error:
            if ( !m_bErrors ) {
#if !wxUSE_LOG_DIALOG
                // discard earlier informational messages if this is the 1st
                // error because they might not make sense any more and showing
                // them in a message box might be confusing
                m_aMessages.Empty();
                m_aSeverity.Empty();
                m_aTimes.Empty();
#endif // wxUSE_LOG_DIALOG
                m_bErrors = true;
            }
            // fall through

        case wxLOG_Warning:
            if ( !m_bErrors ) {
                // for the warning we don't discard the info messages
                m_bWarnings = true;
            }

            m_aMessages.Add(szString);
            m_aSeverity.Add((int)level);
            m_aTimes.Add((long)t);
            m_bHasMessages = true;
            break;
    }
}

#endif   // wxUSE_LOGGUI

// ----------------------------------------------------------------------------
// wxLogWindow and wxLogFrame implementation
// ----------------------------------------------------------------------------

#if wxUSE_LOGWINDOW

// log frame class
// ---------------
class wxLogFrame : public wxFrame
{
public:
    // ctor & dtor
    wxLogFrame(wxWindow *pParent, wxLogWindow *log, const wxChar *szTitle);
    virtual ~wxLogFrame();

    // menu callbacks
    void OnClose(wxCommandEvent& event);
    void OnCloseWindow(wxCloseEvent& event);
#if wxUSE_FILE
    void OnSave (wxCommandEvent& event);
#endif // wxUSE_FILE
    void OnClear(wxCommandEvent& event);

    // accessors
    wxTextCtrl *TextCtrl() const { return m_pTextCtrl; }

private:
    // use standard ids for our commands!
    enum
    {
        Menu_Close = wxID_CLOSE,
        Menu_Save  = wxID_SAVE,
        Menu_Clear = wxID_CLEAR
    };

    // common part of OnClose() and OnCloseWindow()
    void DoClose();

    wxTextCtrl  *m_pTextCtrl;
    wxLogWindow *m_log;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxLogFrame)
};

BEGIN_EVENT_TABLE(wxLogFrame, wxFrame)
    // wxLogWindow menu events
    EVT_MENU(Menu_Close, wxLogFrame::OnClose)
#if wxUSE_FILE
    EVT_MENU(Menu_Save,  wxLogFrame::OnSave)
#endif // wxUSE_FILE
    EVT_MENU(Menu_Clear, wxLogFrame::OnClear)

    EVT_CLOSE(wxLogFrame::OnCloseWindow)
END_EVENT_TABLE()

wxLogFrame::wxLogFrame(wxWindow *pParent, wxLogWindow *log, const wxChar *szTitle)
          : wxFrame(pParent, wxID_ANY, szTitle)
{
    m_log = log;

    m_pTextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
            wxDefaultSize,
            wxTE_MULTILINE  |
            wxHSCROLL       |
            // needed for Win32 to avoid 65Kb limit but it doesn't work well
            // when using RichEdit 2.0 which we always do in the Unicode build
#if !wxUSE_UNICODE
            wxTE_RICH       |
#endif // !wxUSE_UNICODE
            wxTE_READONLY);

#if wxUSE_MENUS
    // create menu
    wxMenuBar *pMenuBar = new wxMenuBar;
    wxMenu *pMenu = new wxMenu;
#if wxUSE_FILE
    pMenu->Append(Menu_Save,  _("&Save..."), _("Save log contents to file"));
#endif // wxUSE_FILE
    pMenu->Append(Menu_Clear, _("C&lear"), _("Clear the log contents"));
    pMenu->AppendSeparator();
    pMenu->Append(Menu_Close, _("&Close"), _("Close this window"));
    pMenuBar->Append(pMenu, _("&Log"));
    SetMenuBar(pMenuBar);
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    // status bar for menu prompts
    CreateStatusBar();
#endif // wxUSE_STATUSBAR

    m_log->OnFrameCreate(this);
}

void wxLogFrame::DoClose()
{
    if ( m_log->OnFrameClose(this) )
    {
        // instead of closing just hide the window to be able to Show() it
        // later
        Show(false);
    }
}

void wxLogFrame::OnClose(wxCommandEvent& WXUNUSED(event))
{
    DoClose();
}

void wxLogFrame::OnCloseWindow(wxCloseEvent& WXUNUSED(event))
{
    DoClose();
}

#if wxUSE_FILE
void wxLogFrame::OnSave(wxCommandEvent& WXUNUSED(event))
{
#if wxUSE_FILEDLG
    wxString filename;
    wxFile file;
    int rc = OpenLogFile(file, &filename, this);
    if ( rc == -1 )
    {
        // cancelled
        return;
    }

    bool bOk = rc != 0;

    // retrieve text and save it
    // -------------------------
    int nLines = m_pTextCtrl->GetNumberOfLines();
    for ( int nLine = 0; bOk && nLine < nLines; nLine++ ) {
        bOk = file.Write(m_pTextCtrl->GetLineText(nLine) +
                         wxTextFile::GetEOL());
    }

    if ( bOk )
        bOk = file.Close();

    if ( !bOk ) {
        wxLogError(_("Can't save log contents to file."));
    }
    else {
        wxLogStatus(this, _("Log saved to the file '%s'."), filename.c_str());
    }
#endif
}
#endif // wxUSE_FILE

void wxLogFrame::OnClear(wxCommandEvent& WXUNUSED(event))
{
    m_pTextCtrl->Clear();
}

wxLogFrame::~wxLogFrame()
{
    m_log->OnFrameDelete(this);
}

// wxLogWindow
// -----------

wxLogWindow::wxLogWindow(wxWindow *pParent,
                         const wxChar *szTitle,
                         bool bShow,
                         bool bDoPass)
{
    PassMessages(bDoPass);

    m_pLogFrame = new wxLogFrame(pParent, this, szTitle);

    if ( bShow )
        m_pLogFrame->Show();
}

void wxLogWindow::Show(bool bShow)
{
    m_pLogFrame->Show(bShow);
}

void wxLogWindow::DoLog(wxLogLevel level, const wxChar *szString, time_t t)
{
    // first let the previous logger show it
    wxLogPassThrough::DoLog(level, szString, t);

    if ( m_pLogFrame ) {
        switch ( level ) {
            case wxLOG_Status:
                // by default, these messages are ignored by wxLog, so process
                // them ourselves
                if ( !wxIsEmpty(szString) )
                {
                    wxString str;
                    str << _("Status: ") << szString;
                    DoLogString(str, t);
                }
                break;

                // don't put trace messages in the text window for 2 reasons:
                // 1) there are too many of them
                // 2) they may provoke other trace messages thus sending a program
                //    into an infinite loop
            case wxLOG_Trace:
                break;

            default:
                // and this will format it nicely and call our DoLogString()
                wxLog::DoLog(level, szString, t);
        }
    }
}

void wxLogWindow::DoLogString(const wxChar *szString, time_t WXUNUSED(t))
{
    // put the text into our window
    wxTextCtrl *pText = m_pLogFrame->TextCtrl();

    // remove selection (WriteText is in fact ReplaceSelection)
#ifdef __WXMSW__
    wxTextPos nLen = pText->GetLastPosition();
    pText->SetSelection(nLen, nLen);
#endif // Windows

    wxString msg;
    TimeStamp(&msg);
    msg << szString << wxT('\n');

    pText->AppendText(msg);

    // TODO ensure that the line can be seen
}

wxFrame *wxLogWindow::GetFrame() const
{
    return m_pLogFrame;
}

void wxLogWindow::OnFrameCreate(wxFrame * WXUNUSED(frame))
{
}

bool wxLogWindow::OnFrameClose(wxFrame * WXUNUSED(frame))
{
    // allow to close
    return true;
}

void wxLogWindow::OnFrameDelete(wxFrame * WXUNUSED(frame))
{
    m_pLogFrame = (wxLogFrame *)NULL;
}

wxLogWindow::~wxLogWindow()
{
    // may be NULL if log frame already auto destroyed itself
    delete m_pLogFrame;
}

#endif // wxUSE_LOGWINDOW

// ----------------------------------------------------------------------------
// wxLogDialog
// ----------------------------------------------------------------------------

#if wxUSE_LOG_DIALOG

#ifndef __SMARTPHONE__
static const size_t MARGIN = 10;
#else
static const size_t MARGIN = 0;
#endif

wxString wxLogDialog::ms_details;

wxLogDialog::wxLogDialog(wxWindow *parent,
                         const wxArrayString& messages,
                         const wxArrayInt& severity,
                         const wxArrayLong& times,
                         const wxString& caption,
                         long style)
           : wxDialog(parent, wxID_ANY, caption,
                      wxDefaultPosition, wxDefaultSize,
                      wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    if ( ms_details.empty() )
    {
        // ensure that we won't loop here if wxGetTranslation()
        // happens to pop up a Log message while translating this :-)
        ms_details = wxTRANSLATE("&Details");
        ms_details = wxGetTranslation(ms_details);
#ifdef __SMARTPHONE__
        ms_details = wxStripMenuCodes(ms_details);
#endif
    }

    size_t count = messages.GetCount();
    m_messages.Alloc(count);
    m_severity.Alloc(count);
    m_times.Alloc(count);

    for ( size_t n = 0; n < count; n++ )
    {
        wxString msg = messages[n];
        msg.Replace(wxT("\n"), wxT(" "));
        m_messages.Add(msg);
        m_severity.Add(severity[n]);
        m_times.Add(times[n]);
    }

    m_showingDetails = false; // not initially
    m_listctrl = (wxListCtrl *)NULL;

#ifndef __SMARTPHONE__

#if wxUSE_STATLINE
    m_statline = (wxStaticLine *)NULL;
#endif // wxUSE_STATLINE

#if wxUSE_FILE
    m_btnSave = (wxButton *)NULL;
#endif // wxUSE_FILE

#endif // __SMARTPHONE__

    bool isPda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);

    // create the controls which are always shown and layout them: we use
    // sizers even though our window is not resizeable to calculate the size of
    // the dialog properly
    wxBoxSizer *sizerTop = new wxBoxSizer(wxVERTICAL);
#ifndef __SMARTPHONE__
    wxBoxSizer *sizerButtons = new wxBoxSizer(isPda ? wxHORIZONTAL : wxVERTICAL);
#endif
    wxBoxSizer *sizerAll = new wxBoxSizer(isPda ? wxVERTICAL : wxHORIZONTAL);

#ifdef __SMARTPHONE__
    SetLeftMenu(wxID_OK);
    SetRightMenu(wxID_MORE, ms_details + EXPAND_SUFFIX);
#else
    wxButton *btnOk = new wxButton(this, wxID_OK);
    sizerButtons->Add(btnOk, 0, isPda ? wxCENTRE : wxCENTRE|wxBOTTOM, MARGIN/2);
    m_btnDetails = new wxButton(this, wxID_MORE, ms_details + EXPAND_SUFFIX);
    sizerButtons->Add(m_btnDetails, 0, isPda ? wxCENTRE|wxLEFT : wxCENTRE | wxTOP, MARGIN/2 - 1);
#endif

    wxBitmap bitmap;
    switch ( style & wxICON_MASK )
    {
        case wxICON_ERROR:
            bitmap = wxArtProvider::GetBitmap(wxART_ERROR, wxART_MESSAGE_BOX);
#ifdef __WXPM__
            bitmap.SetId(wxICON_SMALL_ERROR);
#endif
            break;

        case wxICON_INFORMATION:
            bitmap = wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_MESSAGE_BOX);
#ifdef __WXPM__
            bitmap.SetId(wxICON_SMALL_INFO);
#endif
            break;

        case wxICON_WARNING:
            bitmap = wxArtProvider::GetBitmap(wxART_WARNING, wxART_MESSAGE_BOX);
#ifdef __WXPM__
            bitmap.SetId(wxICON_SMALL_WARNING);
#endif
            break;

        default:
            wxFAIL_MSG(_T("incorrect log style"));
    }

    if (!isPda)
        sizerAll->Add(new wxStaticBitmap(this, wxID_ANY, bitmap), 0,
                  wxALIGN_CENTRE_VERTICAL);

    const wxString& message = messages.Last();
    sizerAll->Add(CreateTextSizer(message), 1,
                  wxALIGN_CENTRE_VERTICAL | wxLEFT | wxRIGHT, MARGIN);
#ifndef __SMARTPHONE__
    sizerAll->Add(sizerButtons, 0, isPda ? wxCENTRE|wxTOP|wxBOTTOM : (wxALIGN_RIGHT | wxLEFT), MARGIN);
#endif

    sizerTop->Add(sizerAll, 0, wxALL | wxEXPAND, MARGIN);

    SetSizer(sizerTop);

    // see comments in OnDetails()
    //
    // Note: Doing this, this way, triggered a nasty bug in
    //       wxTopLevelWindowGTK::GtkOnSize which took -1 literally once
    //       either of maxWidth or maxHeight was set.  This symptom has been
    //       fixed there, but it is a problem that remains as long as we allow
    //       unchecked access to the internal size members.  We really need to
    //       encapuslate window sizes more cleanly and make it clear when -1 will
    //       be substituted and when it will not.

    wxSize size = sizerTop->Fit(this);
    m_maxHeight = size.y;
    SetSizeHints(size.x, size.y, m_maxWidth, m_maxHeight);

#ifndef __SMARTPHONE__
    btnOk->SetFocus();
#endif

    Centre();

    if (isPda)
    {
        // Move up the screen so that when we expand the dialog,
        // there's enough space.
        Move(wxPoint(GetPosition().x, GetPosition().y / 2));
    }
}

void wxLogDialog::CreateDetailsControls()
{
#ifndef __SMARTPHONE__
    // create the save button and separator line if possible
#if wxUSE_FILE
    m_btnSave = new wxButton(this, wxID_SAVE);
#endif // wxUSE_FILE

#if wxUSE_STATLINE
    m_statline = new wxStaticLine(this, wxID_ANY);
#endif // wxUSE_STATLINE

#endif // __SMARTPHONE__

    // create the list ctrl now
    m_listctrl = new wxListCtrl(this, wxID_ANY,
                                wxDefaultPosition, wxDefaultSize,
                                wxSUNKEN_BORDER |
                                wxLC_REPORT |
                                wxLC_NO_HEADER |
                                wxLC_SINGLE_SEL);
#ifdef __WXWINCE__
    // This maks a big aesthetic difference on WinCE but I
    // don't want to risk problems on other platforms
    m_listctrl->Hide();
#endif

    // no need to translate these strings as they're not shown to the
    // user anyhow (we use wxLC_NO_HEADER style)
    m_listctrl->InsertColumn(0, _T("Message"));
    m_listctrl->InsertColumn(1, _T("Time"));

    // prepare the imagelist
    static const int ICON_SIZE = 16;
    wxImageList *imageList = new wxImageList(ICON_SIZE, ICON_SIZE);

    // order should be the same as in the switch below!
    static const wxChar* icons[] =
    {
        wxART_ERROR,
        wxART_WARNING,
        wxART_INFORMATION
    };

    bool loadedIcons = true;

    for ( size_t icon = 0; icon < WXSIZEOF(icons); icon++ )
    {
        wxBitmap bmp = wxArtProvider::GetBitmap(icons[icon], wxART_MESSAGE_BOX,
                                                wxSize(ICON_SIZE, ICON_SIZE));

        // This may very well fail if there are insufficient colours available.
        // Degrade gracefully.
        if ( !bmp.Ok() )
        {
            loadedIcons = false;

            break;
        }

        imageList->Add(bmp);
    }

    m_listctrl->SetImageList(imageList, wxIMAGE_LIST_SMALL);

    // and fill it
    wxString fmt = wxLog::GetTimestamp();
    if ( !fmt )
    {
        // default format
        fmt = _T("%c");
    }

    size_t count = m_messages.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        int image;

        if ( loadedIcons )
        {
            switch ( m_severity[n] )
            {
                case wxLOG_Error:
                    image = 0;
                    break;

                case wxLOG_Warning:
                    image = 1;
                    break;

                default:
                    image = 2;
            }
        }
        else // failed to load images
        {
            image = -1;
        }

        m_listctrl->InsertItem(n, m_messages[n], image);
        m_listctrl->SetItem(n, 1, TimeStamp(fmt, (time_t)m_times[n]));
    }

    // let the columns size themselves
    m_listctrl->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_listctrl->SetColumnWidth(1, wxLIST_AUTOSIZE);

    // calculate an approximately nice height for the listctrl
    int height = GetCharHeight()*(count + 4);

    // but check that the dialog won't fall fown from the screen
    //
    // we use GetMinHeight() to get the height of the dialog part without the
    // details and we consider that the "Save" button below and the separator
    // line (and the margins around it) take about as much, hence double it
    int heightMax = wxGetDisplaySize().y - GetPosition().y - 2*GetMinHeight();

    // we should leave a margin
    heightMax *= 9;
    heightMax /= 10;

    m_listctrl->SetSize(wxDefaultCoord, wxMin(height, heightMax));
}

void wxLogDialog::OnListSelect(wxListEvent& event)
{
    // we can't just disable the control because this looks ugly under Windows
    // (wrong bg colour, no scrolling...), but we still want to disable
    // selecting items - it makes no sense here
    m_listctrl->SetItemState(event.GetIndex(), 0, wxLIST_STATE_SELECTED);
}

void wxLogDialog::OnOk(wxCommandEvent& WXUNUSED(event))
{
    EndModal(wxID_OK);
}

#if wxUSE_FILE

void wxLogDialog::OnSave(wxCommandEvent& WXUNUSED(event))
{
#if wxUSE_FILEDLG
    wxFile file;
    int rc = OpenLogFile(file, NULL, this);
    if ( rc == -1 )
    {
        // cancelled
        return;
    }

    bool ok = rc != 0;

    wxString fmt = wxLog::GetTimestamp();
    if ( !fmt )
    {
        // default format
        fmt = _T("%c");
    }

    size_t count = m_messages.GetCount();
    for ( size_t n = 0; ok && (n < count); n++ )
    {
        wxString line;
        line << TimeStamp(fmt, (time_t)m_times[n])
             << _T(": ")
             << m_messages[n]
             << wxTextFile::GetEOL();

        ok = file.Write(line);
    }

    if ( ok )
        ok = file.Close();

    if ( !ok )
        wxLogError(_("Can't save log contents to file."));
#endif // wxUSE_FILEDLG
}

#endif // wxUSE_FILE

void wxLogDialog::OnDetails(wxCommandEvent& WXUNUSED(event))
{
    wxSizer *sizer = GetSizer();

    if ( m_showingDetails )
    {
#ifdef __SMARTPHONE__
        SetRightMenu(wxID_MORE, ms_details + EXPAND_SUFFIX);
#else
        m_btnDetails->SetLabel(ms_details + EXPAND_SUFFIX);
#endif

        sizer->Detach( m_listctrl );

#ifndef __SMARTPHONE__

#if wxUSE_STATLINE
        sizer->Detach( m_statline );
#endif // wxUSE_STATLINE

#if wxUSE_FILE
        sizer->Detach( m_btnSave );
#endif // wxUSE_FILE

#endif // __SMARTPHONE__
    }
    else // show details now
    {
#ifdef __SMARTPHONE__
        SetRightMenu(wxID_MORE, wxString(_T("<< ")) + ms_details);
#else
        m_btnDetails->SetLabel(wxString(_T("<< ")) + ms_details);
#endif

        if ( !m_listctrl )
        {
            CreateDetailsControls();
        }

#if wxUSE_STATLINE && !defined(__SMARTPHONE__)
        bool isPda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);
        if (!isPda)
            sizer->Add(m_statline, 0, wxEXPAND | (wxALL & ~wxTOP), MARGIN);
#endif // wxUSE_STATLINE

        sizer->Add(m_listctrl, 1, wxEXPAND | (wxALL & ~wxTOP), MARGIN);

        // VZ: this doesn't work as this becomes the initial (and not only
        //     minimal) listctrl height as well - why?
#if 0
        // allow the user to make the dialog shorter than its initial height -
        // without this it wouldn't work as the list ctrl would have been
        // incompressible
        sizer->SetItemMinSize(m_listctrl, 100, 3*GetCharHeight());
#endif // 0

#if wxUSE_FILE && !defined(__SMARTPHONE__)
        sizer->Add(m_btnSave, 0, wxALIGN_RIGHT | (wxALL & ~wxTOP), MARGIN);
#endif // wxUSE_FILE
    }

    m_showingDetails = !m_showingDetails;

    // in any case, our size changed - relayout everything and set new hints
    // ---------------------------------------------------------------------

    // we have to reset min size constraints or Fit() would never reduce the
    // dialog size when collapsing it and we have to reset max constraint
    // because it wouldn't expand it otherwise

    m_minHeight =
    m_maxHeight = -1;

    // wxSizer::FitSize() is private, otherwise we might use it directly...
    wxSize sizeTotal = GetSize(),
           sizeClient = GetClientSize();

    wxSize size = sizer->GetMinSize();
    size.x += sizeTotal.x - sizeClient.x;
    size.y += sizeTotal.y - sizeClient.y;

    // we don't want to allow expanding the dialog in vertical direction as
    // this would show the "hidden" details but we can resize the dialog
    // vertically while the details are shown
    if ( !m_showingDetails )
        m_maxHeight = size.y;

    SetSizeHints(size.x, size.y, m_maxWidth, m_maxHeight);

#ifdef __WXWINCE__
    if (m_showingDetails)
        m_listctrl->Show();
#endif

    // don't change the width when expanding/collapsing
    SetSize(wxDefaultCoord, size.y);

#ifdef __WXGTK__
    // VS: this is necessary in order to force frame redraw under
    // WindowMaker or fvwm2 (and probably other broken WMs).
    // Otherwise, detailed list wouldn't be displayed.
    Show();
#endif // wxGTK
}

wxLogDialog::~wxLogDialog()
{
    if ( m_listctrl )
    {
        delete m_listctrl->GetImageList(wxIMAGE_LIST_SMALL);
    }
}

#endif // wxUSE_LOG_DIALOG

#if wxUSE_FILE && wxUSE_FILEDLG

// pass an uninitialized file object, the function will ask the user for the
// filename and try to open it, returns true on success (file was opened),
// false if file couldn't be opened/created and -1 if the file selection
// dialog was cancelled
static int OpenLogFile(wxFile& file, wxString *pFilename, wxWindow *parent)
{
    // get the file name
    // -----------------
    wxString filename = wxSaveFileSelector(wxT("log"), wxT("txt"), wxT("log.txt"), parent);
    if ( !filename ) {
        // cancelled
        return -1;
    }

    // open file
    // ---------
    bool bOk wxDUMMY_INITIALIZE(false);
    if ( wxFile::Exists(filename) ) {
        bool bAppend = false;
        wxString strMsg;
        strMsg.Printf(_("Append log to file '%s' (choosing [No] will overwrite it)?"),
                      filename.c_str());
        switch ( wxMessageBox(strMsg, _("Question"),
                              wxICON_QUESTION | wxYES_NO | wxCANCEL) ) {
            case wxYES:
                bAppend = true;
                break;

            case wxNO:
                bAppend = false;
                break;

            case wxCANCEL:
                return -1;

            default:
                wxFAIL_MSG(_("invalid message box return value"));
        }

        if ( bAppend ) {
            bOk = file.Open(filename, wxFile::write_append);
        }
        else {
            bOk = file.Create(filename, true /* overwrite */);
        }
    }
    else {
        bOk = file.Create(filename);
    }

    if ( pFilename )
        *pFilename = filename;

    return bOk;
}

#endif // wxUSE_FILE

#endif // !(wxUSE_LOGGUI || wxUSE_LOGWINDOW)

#if wxUSE_LOG && wxUSE_TEXTCTRL

// ----------------------------------------------------------------------------
// wxLogTextCtrl implementation
// ----------------------------------------------------------------------------

wxLogTextCtrl::wxLogTextCtrl(wxTextCtrl *pTextCtrl)
{
    m_pTextCtrl = pTextCtrl;
}

void wxLogTextCtrl::DoLogString(const wxChar *szString, time_t WXUNUSED(t))
{
    wxString msg;
    TimeStamp(&msg);

    msg << szString << wxT('\n');
    m_pTextCtrl->AppendText(msg);
}

#endif // wxUSE_LOG && wxUSE_TEXTCTRL
