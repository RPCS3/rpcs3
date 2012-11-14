/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/docview.cpp
// Purpose:     Document/view classes
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: docview.cpp 66911 2011-02-16 21:31:33Z JS $
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

#if wxUSE_DOC_VIEW_ARCHITECTURE

#include "wx/docview.h"

#ifndef WX_PRECOMP
    #include "wx/list.h"
    #include "wx/string.h"
    #include "wx/utils.h"
    #include "wx/app.h"
    #include "wx/dc.h"
    #include "wx/dialog.h"
    #include "wx/menu.h"
    #include "wx/filedlg.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/msgdlg.h"
    #include "wx/mdi.h"
    #include "wx/choicdlg.h"
#endif

#include "wx/ffile.h"

#ifdef __WXMAC__
    #include "wx/filename.h"
#endif

#if wxUSE_PRINTING_ARCHITECTURE
    #include "wx/prntbase.h"
    #include "wx/printdlg.h"
#endif

#include "wx/confbase.h"
#include "wx/file.h"
#include "wx/cmdproc.h"
#include "wx/tokenzr.h"

#include <stdio.h>
#include <string.h>

#if wxUSE_STD_IOSTREAM
    #include "wx/ioswrap.h"
    #if wxUSE_IOSTREAMH
        #include <fstream.h>
    #else
        #include <fstream>
    #endif
#else
    #include "wx/wfstream.h"
#endif

// ----------------------------------------------------------------------------
// wxWidgets macros
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxDocument, wxEvtHandler)
IMPLEMENT_ABSTRACT_CLASS(wxView, wxEvtHandler)
IMPLEMENT_ABSTRACT_CLASS(wxDocTemplate, wxObject)
IMPLEMENT_DYNAMIC_CLASS(wxDocManager, wxEvtHandler)
IMPLEMENT_CLASS(wxDocChildFrame, wxFrame)
IMPLEMENT_CLASS(wxDocParentFrame, wxFrame)

#if wxUSE_PRINTING_ARCHITECTURE
    IMPLEMENT_DYNAMIC_CLASS(wxDocPrintout, wxPrintout)
#endif

IMPLEMENT_DYNAMIC_CLASS(wxFileHistory, wxObject)

// ----------------------------------------------------------------------------
// function prototypes
// ----------------------------------------------------------------------------

static wxWindow* wxFindSuitableParent(void);

// ----------------------------------------------------------------------------
// local constants
// ----------------------------------------------------------------------------

static const wxChar *s_MRUEntryFormat = wxT("&%d %s");

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// local functions
// ----------------------------------------------------------------------------

static wxString FindExtension(const wxChar *path)
{
    wxString ext;
    wxSplitPath(path, NULL, NULL, &ext);

    // VZ: extensions are considered not case sensitive - is this really a good
    //     idea?
    return ext.MakeLower();
}

// ----------------------------------------------------------------------------
// Definition of wxDocument
// ----------------------------------------------------------------------------

wxDocument::wxDocument(wxDocument *parent)
{
    m_documentModified = false;
    m_documentParent = parent;
    m_documentTemplate = (wxDocTemplate *) NULL;
    m_commandProcessor = (wxCommandProcessor*) NULL;
    m_savedYet = false;
}

bool wxDocument::DeleteContents()
{
    return true;
}

wxDocument::~wxDocument()
{
    DeleteContents();

    if (m_commandProcessor)
        delete m_commandProcessor;

    if (GetDocumentManager())
        GetDocumentManager()->RemoveDocument(this);

    // Not safe to do here, since it'll invoke virtual view functions
    // expecting to see valid derived objects: and by the time we get here,
    // we've called destructors higher up.
    //DeleteAllViews();
}

bool wxDocument::Close()
{
    if (OnSaveModified())
        return OnCloseDocument();
    else
        return false;
}

bool wxDocument::OnCloseDocument()
{
    // Tell all views that we're about to close
    NotifyClosing();
    DeleteContents();
    Modify(false);
    return true;
}

// Note that this implicitly deletes the document when the last view is
// deleted.
bool wxDocument::DeleteAllViews()
{
    wxDocManager* manager = GetDocumentManager();

    // first check if all views agree to be closed
    const wxList::iterator end = m_documentViews.end();
    for ( wxList::iterator i = m_documentViews.begin(); i != end; ++i )
    {
        wxView *view = (wxView *)*i;
        if ( !view->Close() )
            return false;
    }

    // all views agreed to close, now do close them
    if ( m_documentViews.empty() )
    {
        // normally the document would be implicitly deleted when the last view
        // is, but if don't have any views, do it here instead
        if ( manager && manager->GetDocuments().Member(this) )
            delete this;
    }
    else // have views
    {
        // as we delete elements we iterate over, don't use the usual "from
        // begin to end" loop
        for ( ;; )
        {
            wxView *view = (wxView *)*m_documentViews.begin();

            bool isLastOne = m_documentViews.size() == 1;

            // this always deletes the node implicitly and if this is the last
            // view also deletes this object itself (also implicitly, great),
            // so we can't test for m_documentViews.empty() after calling this!
            delete view;

            if ( isLastOne )
                break;
        }
    }

    return true;
}

wxView *wxDocument::GetFirstView() const
{
    if (m_documentViews.GetCount() == 0)
        return (wxView *) NULL;
    return (wxView *)m_documentViews.GetFirst()->GetData();
}

wxDocManager *wxDocument::GetDocumentManager() const
{
    return (m_documentTemplate ? m_documentTemplate->GetDocumentManager() : (wxDocManager*) NULL);
}

bool wxDocument::OnNewDocument()
{
    if (!OnSaveModified())
        return false;

    if (OnCloseDocument()==false) return false;
    DeleteContents();
    Modify(false);
    SetDocumentSaved(false);

    wxString name;
    GetDocumentManager()->MakeDefaultName(name);
    SetTitle(name);
    SetFilename(name, true);

    return true;
}

bool wxDocument::Save()
{
    if (!IsModified() && m_savedYet)
        return true;

    if ( m_documentFile.empty() || !m_savedYet )
        return SaveAs();

    return OnSaveDocument(m_documentFile);
}

bool wxDocument::SaveAs()
{
    wxDocTemplate *docTemplate = GetDocumentTemplate();
    if (!docTemplate)
        return false;

#if defined(__WXMSW__) || defined(__WXGTK__) || defined(__WXMAC__)
    wxString filter = docTemplate->GetDescription() + wxT(" (") + docTemplate->GetFileFilter() + wxT(")|") + docTemplate->GetFileFilter();

    // Now see if there are some other template with identical view and document
    // classes, whose filters may also be used.

    if (docTemplate->GetViewClassInfo() && docTemplate->GetDocClassInfo())
    {
        wxList::compatibility_iterator node = docTemplate->GetDocumentManager()->GetTemplates().GetFirst();
        while (node)
        {
            wxDocTemplate *t = (wxDocTemplate*) node->GetData();

            if (t->IsVisible() && t != docTemplate &&
                t->GetViewClassInfo() == docTemplate->GetViewClassInfo() &&
                t->GetDocClassInfo() == docTemplate->GetDocClassInfo())
            {
                // add a '|' to separate this filter from the previous one
                if ( !filter.empty() )
                    filter << wxT('|');

                filter << t->GetDescription() << wxT(" (") << t->GetFileFilter() << wxT(") |")
                       << t->GetFileFilter();
            }

            node = node->GetNext();
        }
    }
#else
    wxString filter = docTemplate->GetFileFilter() ;
#endif
    wxString defaultDir = docTemplate->GetDirectory();
    if (defaultDir.IsEmpty())
        defaultDir = wxPathOnly(GetFilename());

    wxString tmp = wxFileSelector(_("Save as"),
            defaultDir,
            wxFileNameFromPath(GetFilename()),
            docTemplate->GetDefaultExtension(),
            filter,
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
            GetDocumentWindow());

    if (tmp.empty())
        return false;

    wxString fileName(tmp);
    wxString path, name, ext;
    wxSplitPath(fileName, & path, & name, & ext);

    if (ext.empty())
    {
        fileName += wxT(".");
        fileName += docTemplate->GetDefaultExtension();
    }

    SetFilename(fileName);
    SetTitle(wxFileNameFromPath(fileName));

    // Notify the views that the filename has changed
    wxList::compatibility_iterator node = m_documentViews.GetFirst();
    while (node)
    {
        wxView *view = (wxView *)node->GetData();
        view->OnChangeFilename();
        node = node->GetNext();
    }

    // Files that were not saved correctly are not added to the FileHistory.
    if (!OnSaveDocument(m_documentFile))
        return false;

   // A file that doesn't use the default extension of its document template cannot be opened
   // via the FileHistory, so we do not add it.
   if (docTemplate->FileMatchesTemplate(fileName))
   {
       GetDocumentManager()->AddFileToHistory(fileName);
   }
   else
   {
       // The user will probably not be able to open the file again, so
       // we could warn about the wrong file-extension here.
   }
   return true;
}

bool wxDocument::OnSaveDocument(const wxString& file)
{
    if ( !file )
        return false;

    if ( !DoSaveDocument(file) )
        return false;

    Modify(false);
    SetFilename(file);
    SetDocumentSaved(true);
#ifdef __WXMAC__
    wxFileName fn(file) ;
    fn.MacSetDefaultTypeAndCreator() ;
#endif
    return true;
}

bool wxDocument::OnOpenDocument(const wxString& file)
{
    if (!OnSaveModified())
        return false;

    if ( !DoOpenDocument(file) )
        return false;

    SetFilename(file, true);
    Modify(false);
    m_savedYet = true;

    UpdateAllViews();

    return true;
}

#if wxUSE_STD_IOSTREAM
wxSTD istream& wxDocument::LoadObject(wxSTD istream& stream)
#else
wxInputStream& wxDocument::LoadObject(wxInputStream& stream)
#endif
{
    return stream;
}

#if wxUSE_STD_IOSTREAM
wxSTD ostream& wxDocument::SaveObject(wxSTD ostream& stream)
#else
wxOutputStream& wxDocument::SaveObject(wxOutputStream& stream)
#endif
{
    return stream;
}

bool wxDocument::Revert()
{
    return false;
}


// Get title, or filename if no title, else unnamed
bool wxDocument::GetPrintableName(wxString& buf) const
{
    if (!m_documentTitle.empty())
    {
        buf = m_documentTitle;
        return true;
    }
    else if (!m_documentFile.empty())
    {
        buf = wxFileNameFromPath(m_documentFile);
        return true;
    }
    else
    {
        buf = _("unnamed");
        return true;
    }
}

wxWindow *wxDocument::GetDocumentWindow() const
{
    wxView *view = GetFirstView();
    if (view)
        return view->GetFrame();
    else
        return wxTheApp->GetTopWindow();
}

wxCommandProcessor *wxDocument::OnCreateCommandProcessor()
{
    return new wxCommandProcessor;
}

// true if safe to close
bool wxDocument::OnSaveModified()
{
    if (IsModified())
    {
        wxString title;
        GetPrintableName(title);

        wxString msgTitle;
        if (!wxTheApp->GetAppName().empty())
            msgTitle = wxTheApp->GetAppName();
        else
            msgTitle = wxString(_("Warning"));

        wxString prompt;
        prompt.Printf(_("Do you want to save changes to document %s?"),
                (const wxChar *)title);
        int res = wxMessageBox(prompt, msgTitle,
                wxYES_NO|wxCANCEL|wxICON_QUESTION,
                GetDocumentWindow());
        if (res == wxNO)
        {
            Modify(false);
            return true;
        }
        else if (res == wxYES)
            return Save();
        else if (res == wxCANCEL)
            return false;
    }
    return true;
}

bool wxDocument::Draw(wxDC& WXUNUSED(context))
{
    return true;
}

bool wxDocument::AddView(wxView *view)
{
    if (!m_documentViews.Member(view))
    {
        m_documentViews.Append(view);
        OnChangedViewList();
    }
    return true;
}

bool wxDocument::RemoveView(wxView *view)
{
    (void)m_documentViews.DeleteObject(view);
    OnChangedViewList();
    return true;
}

bool wxDocument::OnCreate(const wxString& WXUNUSED(path), long flags)
{
    if (GetDocumentTemplate()->CreateView(this, flags))
        return true;
    else
        return false;
}

// Called after a view is added or removed.
// The default implementation deletes the document if
// there are no more views.
void wxDocument::OnChangedViewList()
{
    if (m_documentViews.GetCount() == 0)
    {
        if (OnSaveModified())
        {
            delete this;
        }
    }
}

void wxDocument::UpdateAllViews(wxView *sender, wxObject *hint)
{
    wxList::compatibility_iterator node = m_documentViews.GetFirst();
    while (node)
    {
        wxView *view = (wxView *)node->GetData();
        if (view != sender)
            view->OnUpdate(sender, hint);
        node = node->GetNext();
    }
}

void wxDocument::NotifyClosing()
{
    wxList::compatibility_iterator node = m_documentViews.GetFirst();
    while (node)
    {
        wxView *view = (wxView *)node->GetData();
        view->OnClosingDocument();
        node = node->GetNext();
    }
}

void wxDocument::SetFilename(const wxString& filename, bool notifyViews)
{
    m_documentFile = filename;
    if ( notifyViews )
    {
        // Notify the views that the filename has changed
        wxList::compatibility_iterator node = m_documentViews.GetFirst();
        while (node)
        {
            wxView *view = (wxView *)node->GetData();
            view->OnChangeFilename();
            node = node->GetNext();
        }
    }
}

bool wxDocument::DoSaveDocument(const wxString& file)
{
    wxString msgTitle;
    if (!wxTheApp->GetAppName().empty())
        msgTitle = wxTheApp->GetAppName();
    else
        msgTitle = wxString(_("File error"));

#if wxUSE_STD_IOSTREAM
    wxSTD ofstream store(file.mb_str(), wxSTD ios::binary);
    if (store.fail() || store.bad())
#else
    wxFileOutputStream store(file);
    if (store.GetLastError() != wxSTREAM_NO_ERROR)
#endif
    {
        (void)wxMessageBox(_("Sorry, could not open this file for saving."), msgTitle, wxOK | wxICON_EXCLAMATION,
                           GetDocumentWindow());
        // Saving error
        return false;
    }
    if (!SaveObject(store))
    {
        (void)wxMessageBox(_("Sorry, could not save this file."), msgTitle, wxOK | wxICON_EXCLAMATION,
                           GetDocumentWindow());
        // Saving error
        return false;
    }

    return true;
}

bool wxDocument::DoOpenDocument(const wxString& file)
{
#if wxUSE_STD_IOSTREAM
    wxSTD ifstream store(file.mb_str(), wxSTD ios::binary);
    if (!store.fail() && !store.bad())
#else
    wxFileInputStream store(file);
    if (store.GetLastError() == wxSTREAM_NO_ERROR)
#endif
    {
#if wxUSE_STD_IOSTREAM
        LoadObject(store);
        if ( !!store || store.eof() )
#else
        int res = LoadObject(store).GetLastError();
        if ( res == wxSTREAM_NO_ERROR || res == wxSTREAM_EOF )
#endif
            return true;
    }

    wxLogError(_("Sorry, could not open this file."));
    return false;
}


// ----------------------------------------------------------------------------
// Document view
// ----------------------------------------------------------------------------

wxView::wxView()
{
    m_viewDocument = (wxDocument*) NULL;

    m_viewFrame = (wxFrame *) NULL;
}

wxView::~wxView()
{
    if (m_viewDocument && GetDocumentManager())
        GetDocumentManager()->ActivateView(this, false);
    if (m_viewDocument)
        m_viewDocument->RemoveView(this);
}

// Extend event processing to search the document's event table
bool wxView::ProcessEvent(wxEvent& event)
{
    if ( !GetDocument() || !GetDocument()->ProcessEvent(event) )
        return wxEvtHandler::ProcessEvent(event);

    return true;
}

void wxView::OnActivateView(bool WXUNUSED(activate), wxView *WXUNUSED(activeView), wxView *WXUNUSED(deactiveView))
{
}

void wxView::OnPrint(wxDC *dc, wxObject *WXUNUSED(info))
{
    OnDraw(dc);
}

void wxView::OnUpdate(wxView *WXUNUSED(sender), wxObject *WXUNUSED(hint))
{
}

void wxView::OnChangeFilename()
{
    // GetFrame can return wxWindow rather than wxTopLevelWindow due to
    // generic MDI implementation so use SetLabel rather than SetTitle.
    // It should cause SetTitle() for top level windows.
    wxWindow *win = GetFrame();
    if (!win) return;

    wxDocument *doc = GetDocument();
    if (!doc) return;

    wxString name;
    doc->GetPrintableName(name);
    win->SetLabel(name);
}

void wxView::SetDocument(wxDocument *doc)
{
    m_viewDocument = doc;
    if (doc)
        doc->AddView(this);
}

bool wxView::Close(bool deleteWindow)
{
    if (OnClose(deleteWindow))
        return true;
    else
        return false;
}

void wxView::Activate(bool activate)
{
    if (GetDocument() && GetDocumentManager())
    {
        OnActivateView(activate, this, GetDocumentManager()->GetCurrentView());
        GetDocumentManager()->ActivateView(this, activate);
    }
}

bool wxView::OnClose(bool WXUNUSED(deleteWindow))
{
    return GetDocument() ? GetDocument()->Close() : true;
}

#if wxUSE_PRINTING_ARCHITECTURE
wxPrintout *wxView::OnCreatePrintout()
{
    return new wxDocPrintout(this);
}
#endif // wxUSE_PRINTING_ARCHITECTURE

// ----------------------------------------------------------------------------
// wxDocTemplate
// ----------------------------------------------------------------------------

wxDocTemplate::wxDocTemplate(wxDocManager *manager,
                             const wxString& descr,
                             const wxString& filter,
                             const wxString& dir,
                             const wxString& ext,
                             const wxString& docTypeName,
                             const wxString& viewTypeName,
                             wxClassInfo *docClassInfo,
                             wxClassInfo *viewClassInfo,
                             long flags)
{
    m_documentManager = manager;
    m_description = descr;
    m_directory = dir;
    m_defaultExt = ext;
    m_fileFilter = filter;
    m_flags = flags;
    m_docTypeName = docTypeName;
    m_viewTypeName = viewTypeName;
    m_documentManager->AssociateTemplate(this);

    m_docClassInfo = docClassInfo;
    m_viewClassInfo = viewClassInfo;
}

wxDocTemplate::~wxDocTemplate()
{
    m_documentManager->DisassociateTemplate(this);
}

// Tries to dynamically construct an object of the right class.
wxDocument *wxDocTemplate::CreateDocument(const wxString& path, long flags)
{
    wxDocument *doc = DoCreateDocument();
    if ( doc == NULL )
        return (wxDocument *) NULL;

    if (InitDocument(doc, path, flags))
    {
        return doc;
    }
    else
    {
        return (wxDocument *) NULL;
    }
}

bool wxDocTemplate::InitDocument(wxDocument* doc, const wxString& path, long flags)
{
    doc->SetFilename(path);
    doc->SetDocumentTemplate(this);
    GetDocumentManager()->AddDocument(doc);
    doc->SetCommandProcessor(doc->OnCreateCommandProcessor());

    if (doc->OnCreate(path, flags))
        return true;
    else
    {
        if (GetDocumentManager()->GetDocuments().Member(doc))
            doc->DeleteAllViews();
        return false;
    }
}

wxView *wxDocTemplate::CreateView(wxDocument *doc, long flags)
{
    wxView *view = DoCreateView();
    if ( view == NULL )
        return (wxView *) NULL;

    view->SetDocument(doc);
    if (view->OnCreate(doc, flags))
    {
        return view;
    }
    else
    {
        delete view;
        return (wxView *) NULL;
    }
}

// The default (very primitive) format detection: check is the extension is
// that of the template
bool wxDocTemplate::FileMatchesTemplate(const wxString& path)
{
    wxStringTokenizer parser (GetFileFilter(), wxT(";"));
    wxString anything = wxT ("*");
    while (parser.HasMoreTokens())
    {
        wxString filter = parser.GetNextToken();
        wxString filterExt = FindExtension (filter);
        if ( filter.IsSameAs (anything)    ||
             filterExt.IsSameAs (anything) ||
             filterExt.IsSameAs (FindExtension (path)) )
            return true;
    }
    return GetDefaultExtension().IsSameAs(FindExtension(path));
}

wxDocument *wxDocTemplate::DoCreateDocument()
{
    if (!m_docClassInfo)
        return (wxDocument *) NULL;

    return (wxDocument *)m_docClassInfo->CreateObject();
}

wxView *wxDocTemplate::DoCreateView()
{
    if (!m_viewClassInfo)
        return (wxView *) NULL;

    return (wxView *)m_viewClassInfo->CreateObject();
}

// ----------------------------------------------------------------------------
// wxDocManager
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxDocManager, wxEvtHandler)
    EVT_MENU(wxID_OPEN, wxDocManager::OnFileOpen)
    EVT_MENU(wxID_CLOSE, wxDocManager::OnFileClose)
    EVT_MENU(wxID_CLOSE_ALL, wxDocManager::OnFileCloseAll)
    EVT_MENU(wxID_REVERT, wxDocManager::OnFileRevert)
    EVT_MENU(wxID_NEW, wxDocManager::OnFileNew)
    EVT_MENU(wxID_SAVE, wxDocManager::OnFileSave)
    EVT_MENU(wxID_SAVEAS, wxDocManager::OnFileSaveAs)
    EVT_MENU(wxID_UNDO, wxDocManager::OnUndo)
    EVT_MENU(wxID_REDO, wxDocManager::OnRedo)

    EVT_UPDATE_UI(wxID_OPEN, wxDocManager::OnUpdateFileOpen)
    EVT_UPDATE_UI(wxID_CLOSE, wxDocManager::OnUpdateFileClose)
    EVT_UPDATE_UI(wxID_CLOSE_ALL, wxDocManager::OnUpdateFileClose)
    EVT_UPDATE_UI(wxID_REVERT, wxDocManager::OnUpdateFileRevert)
    EVT_UPDATE_UI(wxID_NEW, wxDocManager::OnUpdateFileNew)
    EVT_UPDATE_UI(wxID_SAVE, wxDocManager::OnUpdateFileSave)
    EVT_UPDATE_UI(wxID_SAVEAS, wxDocManager::OnUpdateFileSaveAs)
    EVT_UPDATE_UI(wxID_UNDO, wxDocManager::OnUpdateUndo)
    EVT_UPDATE_UI(wxID_REDO, wxDocManager::OnUpdateRedo)

#if wxUSE_PRINTING_ARCHITECTURE
    EVT_MENU(wxID_PRINT, wxDocManager::OnPrint)
    EVT_MENU(wxID_PREVIEW, wxDocManager::OnPreview)

    EVT_UPDATE_UI(wxID_PRINT, wxDocManager::OnUpdatePrint)
    EVT_UPDATE_UI(wxID_PREVIEW, wxDocManager::OnUpdatePreview)
#endif
END_EVENT_TABLE()

wxDocManager* wxDocManager::sm_docManager = (wxDocManager*) NULL;

wxDocManager::wxDocManager(long flags, bool initialize)
{
    m_defaultDocumentNameCounter = 1;
    m_flags = flags;
    m_currentView = (wxView *) NULL;
    m_maxDocsOpen = 10000;
    m_fileHistory = (wxFileHistory *) NULL;
    if (initialize)
        Initialize();
    sm_docManager = this;
}

wxDocManager::~wxDocManager()
{
    Clear();
    if (m_fileHistory)
        delete m_fileHistory;
    sm_docManager = (wxDocManager*) NULL;
}

// closes the specified document
bool wxDocManager::CloseDocument(wxDocument* doc, bool force)
{
    if (doc->Close() || force)
    {
        // Implicitly deletes the document when
        // the last view is deleted
        doc->DeleteAllViews();

        // Check we're really deleted
        if (m_docs.Member(doc))
            delete doc;

        return true;
    }
    return false;
}

bool wxDocManager::CloseDocuments(bool force)
{
    wxList::compatibility_iterator node = m_docs.GetFirst();
    while (node)
    {
        wxDocument *doc = (wxDocument *)node->GetData();
        wxList::compatibility_iterator next = node->GetNext();

        if (!CloseDocument(doc, force))
            return false;

        // This assumes that documents are not connected in
        // any way, i.e. deleting one document does NOT
        // delete another.
        node = next;
    }
    return true;
}

bool wxDocManager::Clear(bool force)
{
    if (!CloseDocuments(force))
        return false;

    m_currentView = NULL;

    wxList::compatibility_iterator node = m_templates.GetFirst();
    while (node)
    {
        wxDocTemplate *templ = (wxDocTemplate*) node->GetData();
        wxList::compatibility_iterator next = node->GetNext();
        delete templ;
        node = next;
    }
    return true;
}

bool wxDocManager::Initialize()
{
    m_fileHistory = OnCreateFileHistory();
    return true;
}

wxFileHistory *wxDocManager::OnCreateFileHistory()
{
    return new wxFileHistory;
}

void wxDocManager::OnFileClose(wxCommandEvent& WXUNUSED(event))
{
    wxDocument *doc = GetCurrentDocument();
    if (!doc)
        return;
    if (doc->Close())
    {
        doc->DeleteAllViews();
        if (m_docs.Member(doc))
            delete doc;
    }
}

void wxDocManager::OnFileCloseAll(wxCommandEvent& WXUNUSED(event))
{
    CloseDocuments(false);
}

void wxDocManager::OnFileNew(wxCommandEvent& WXUNUSED(event))
{
    CreateDocument( wxEmptyString, wxDOC_NEW );
}

void wxDocManager::OnFileOpen(wxCommandEvent& WXUNUSED(event))
{
    if ( !CreateDocument( wxEmptyString, 0) )
    {
        OnOpenFileFailure();
    }
}

void wxDocManager::OnFileRevert(wxCommandEvent& WXUNUSED(event))
{
    wxDocument *doc = GetCurrentDocument();
    if (!doc)
        return;
    doc->Revert();
}

void wxDocManager::OnFileSave(wxCommandEvent& WXUNUSED(event))
{
    wxDocument *doc = GetCurrentDocument();
    if (!doc)
        return;
    doc->Save();
}

void wxDocManager::OnFileSaveAs(wxCommandEvent& WXUNUSED(event))
{
    wxDocument *doc = GetCurrentDocument();
    if (!doc)
        return;
    doc->SaveAs();
}

void wxDocManager::OnPrint(wxCommandEvent& WXUNUSED(event))
{
#if wxUSE_PRINTING_ARCHITECTURE
    wxView *view = GetCurrentView();
    if (!view)
        return;

    wxPrintout *printout = view->OnCreatePrintout();
    if (printout)
    {
        wxPrinter printer;
        printer.Print(view->GetFrame(), printout, true);

        delete printout;
    }
#endif // wxUSE_PRINTING_ARCHITECTURE
}

void wxDocManager::OnPreview(wxCommandEvent& WXUNUSED(event))
{
#if wxUSE_PRINTING_ARCHITECTURE
    wxBusyCursor busy;
    wxView *view = GetCurrentView();
    if (!view)
        return;

    wxPrintout *printout = view->OnCreatePrintout();
    if (printout)
    {
        // Pass two printout objects: for preview, and possible printing.
        wxPrintPreviewBase *preview = new wxPrintPreview(printout, view->OnCreatePrintout());
        if ( !preview->Ok() )
        {
            delete preview;
            wxMessageBox( _("Sorry, print preview needs a printer to be installed.") );
            return;
        }

        wxPreviewFrame *frame = new wxPreviewFrame(preview, (wxFrame *)wxTheApp->GetTopWindow(), _("Print Preview"),
                wxPoint(100, 100), wxSize(600, 650));
        frame->Centre(wxBOTH);
        frame->Initialize();
        frame->Show(true);
    }
#endif // wxUSE_PRINTING_ARCHITECTURE
}

void wxDocManager::OnUndo(wxCommandEvent& event)
{
    wxDocument *doc = GetCurrentDocument();
    if (!doc)
        return;
    if (doc->GetCommandProcessor())
        doc->GetCommandProcessor()->Undo();
    else
        event.Skip();
}

void wxDocManager::OnRedo(wxCommandEvent& event)
{
    wxDocument *doc = GetCurrentDocument();
    if (!doc)
        return;
    if (doc->GetCommandProcessor())
        doc->GetCommandProcessor()->Redo();
    else
        event.Skip();
}

// Handlers for UI update commands

void wxDocManager::OnUpdateFileOpen(wxUpdateUIEvent& event)
{
    event.Enable( true );
}

void wxDocManager::OnUpdateFileClose(wxUpdateUIEvent& event)
{
    wxDocument *doc = GetCurrentDocument();
    event.Enable( (doc != (wxDocument*) NULL) );
}

void wxDocManager::OnUpdateFileRevert(wxUpdateUIEvent& event)
{
    wxDocument *doc = GetCurrentDocument();
    event.Enable( (doc != (wxDocument*) NULL) );
}

void wxDocManager::OnUpdateFileNew(wxUpdateUIEvent& event)
{
    event.Enable( true );
}

void wxDocManager::OnUpdateFileSave(wxUpdateUIEvent& event)
{
    wxDocument *doc = GetCurrentDocument();
    event.Enable( doc && doc->IsModified() );
}

void wxDocManager::OnUpdateFileSaveAs(wxUpdateUIEvent& event)
{
    wxDocument *doc = GetCurrentDocument();
    event.Enable( (doc != (wxDocument*) NULL) );
}

void wxDocManager::OnUpdateUndo(wxUpdateUIEvent& event)
{
    wxDocument *doc = GetCurrentDocument();
    if (!doc)
        event.Enable(false);
    else if (!doc->GetCommandProcessor())
        event.Skip();
    else
    {
        event.Enable( doc->GetCommandProcessor()->CanUndo() );
        doc->GetCommandProcessor()->SetMenuStrings();
    }
}

void wxDocManager::OnUpdateRedo(wxUpdateUIEvent& event)
{
    wxDocument *doc = GetCurrentDocument();
    if (!doc)
        event.Enable(false);
    else if (!doc->GetCommandProcessor())
        event.Skip();
    else
    {
        event.Enable( doc->GetCommandProcessor()->CanRedo() );
        doc->GetCommandProcessor()->SetMenuStrings();
    }
}

void wxDocManager::OnUpdatePrint(wxUpdateUIEvent& event)
{
    wxDocument *doc = GetCurrentDocument();
    event.Enable( (doc != (wxDocument*) NULL) );
}

void wxDocManager::OnUpdatePreview(wxUpdateUIEvent& event)
{
    wxDocument *doc = GetCurrentDocument();
    event.Enable( (doc != (wxDocument*) NULL) );
}

wxView *wxDocManager::GetCurrentView() const
{
    if (m_currentView)
        return m_currentView;
    if (m_docs.GetCount() == 1)
    {
        wxDocument* doc = (wxDocument*) m_docs.GetFirst()->GetData();
        return doc->GetFirstView();
    }
    return (wxView *) NULL;
}

// Extend event processing to search the view's event table
bool wxDocManager::ProcessEvent(wxEvent& event)
{
    wxView* view = GetCurrentView();
    if (view)
    {
        if (view->ProcessEvent(event))
            return true;
    }
    return wxEvtHandler::ProcessEvent(event);
}

wxDocument *wxDocManager::CreateDocument(const wxString& path, long flags)
{
    wxDocTemplate   **templates = new wxDocTemplate *[m_templates.GetCount()];
    int               n = 0;

    for (size_t i = 0; i < m_templates.GetCount(); i++)
    {
        wxDocTemplate *temp = (wxDocTemplate *)(m_templates.Item(i)->GetData());
        if (temp->IsVisible())
        {
            templates[n] = temp;
            n ++;
        }
    }
    if (n == 0)
    {
        delete[] templates;
        return (wxDocument *) NULL;
    }

    wxDocument* docToClose = NULL;

    // If we've reached the max number of docs, close the
    // first one.
    if ( (int)GetDocuments().GetCount() >= m_maxDocsOpen )
    {
        wxDocument *doc = (wxDocument *)GetDocuments().GetFirst()->GetData();
        docToClose = doc;
    }

    // New document: user chooses a template, unless there's only one.
    if (flags & wxDOC_NEW)
    {
        if (n == 1)
        {
            if (docToClose)
            {
                if (!CloseDocument(docToClose, false))
                {
                    delete[] templates;
                    return NULL;
                }
            }

            wxDocTemplate *temp = templates[0];
            delete[] templates;
            wxDocument *newDoc = temp->CreateDocument(path, flags);

            if (newDoc)
            {
                newDoc->SetDocumentName(temp->GetDocumentName());
                newDoc->SetDocumentTemplate(temp);
                if (!newDoc->OnNewDocument() )
                {
                     // Document is implicitly deleted by DeleteAllViews
                     newDoc->DeleteAllViews();
                     return NULL;
                }
            }
            return newDoc;
        }

        wxDocTemplate *temp = SelectDocumentType(templates, n);
        delete[] templates;
        if (temp)
        {
            if (docToClose)
            {
                if (!CloseDocument(docToClose, false))
                {
                    return NULL;
                }
            }

            wxDocument *newDoc = temp->CreateDocument(path, flags);

            if (newDoc)
            {
                newDoc->SetDocumentName(temp->GetDocumentName());
                newDoc->SetDocumentTemplate(temp);
                if (!newDoc->OnNewDocument() )
                {
                     // Document is implicitly deleted by DeleteAllViews
                     newDoc->DeleteAllViews();
                     return NULL;
                }
            }
            return newDoc;
        }
        else
            return (wxDocument *) NULL;
    }

    // Existing document
    wxDocTemplate *temp;

    wxString path2 = path;

    if (flags & wxDOC_SILENT)
    {
        temp = FindTemplateForPath(path2);
        if (!temp)
        {
            // Since we do not add files with non-default extensions to the FileHistory this
            // can only happen if the application changes the allowed templates in runtime.
            (void)wxMessageBox(_("Sorry, the format for this file is unknown."),
                                _("Open File"),
                               wxOK | wxICON_EXCLAMATION, wxFindSuitableParent());
        }
    }
    else
        temp = SelectDocumentPath(templates, n, path2, flags);

    delete[] templates;

    if (temp)
    {
        if (docToClose)
        {
            if (!CloseDocument(docToClose, false))
            {
                return NULL;
            }
        }

        //see if this file is already open
        for (size_t i = 0; i < GetDocuments().GetCount(); ++i)
        {
            wxDocument* currentDoc = (wxDocument*)(GetDocuments().Item(i)->GetData());
#ifdef __WXMSW__
            //file paths are case-insensitive on Windows
            if (path2.CmpNoCase(currentDoc->GetFilename()) == 0)
#else
            if (path2.Cmp(currentDoc->GetFilename()) == 0)
#endif
            {
                //file already open. Just activate it and return
                if (currentDoc->GetFirstView())
                {
                    ActivateView(currentDoc->GetFirstView(), true);
                    if (currentDoc->GetDocumentWindow())
                        currentDoc->GetDocumentWindow()->SetFocus();
                    return currentDoc;
                }
            }
        }

        wxDocument *newDoc = temp->CreateDocument(path2, flags);
        if (newDoc)
        {
            newDoc->SetDocumentName(temp->GetDocumentName());
            newDoc->SetDocumentTemplate(temp);
            if (!newDoc->OnOpenDocument(path2))
            {
                newDoc->DeleteAllViews();
                // delete newDoc; // Implicitly deleted by DeleteAllViews
                return (wxDocument *) NULL;
            }
            // A file that doesn't use the default extension of its document
            // template cannot be opened via the FileHistory, so we do not
            // add it.
            if (temp->FileMatchesTemplate(path2))
                AddFileToHistory(path2);
        }
        return newDoc;
    }

    return (wxDocument *) NULL;
}

wxView *wxDocManager::CreateView(wxDocument *doc, long flags)
{
    wxDocTemplate   **templates = new wxDocTemplate *[m_templates.GetCount()];
    int               n =0;

    for (size_t i = 0; i < m_templates.GetCount(); i++)
    {
        wxDocTemplate *temp = (wxDocTemplate *)(m_templates.Item(i)->GetData());
        if (temp->IsVisible())
        {
            if (temp->GetDocumentName() == doc->GetDocumentName())
            {
                templates[n] = temp;
                n ++;
            }
        }
    }
    if (n == 0)
    {
        delete[] templates;
        return (wxView *) NULL;
    }
    if (n == 1)
    {
        wxDocTemplate *temp = templates[0];
        delete[] templates;
        wxView *view = temp->CreateView(doc, flags);
        if (view)
            view->SetViewName(temp->GetViewName());
        return view;
    }

    wxDocTemplate *temp = SelectViewType(templates, n);
    delete[] templates;
    if (temp)
    {
        wxView *view = temp->CreateView(doc, flags);
        if (view)
            view->SetViewName(temp->GetViewName());
        return view;
    }
    else
        return (wxView *) NULL;
}

// Not yet implemented
void wxDocManager::DeleteTemplate(wxDocTemplate *WXUNUSED(temp), long WXUNUSED(flags))
{
}

// Not yet implemented
bool wxDocManager::FlushDoc(wxDocument *WXUNUSED(doc))
{
    return false;
}

wxDocument *wxDocManager::GetCurrentDocument() const
{
    wxView *view = GetCurrentView();
    if (view)
        return view->GetDocument();
    else
        return (wxDocument *) NULL;
}

// Make a default document name
bool wxDocManager::MakeDefaultName(wxString& name)
{
    name.Printf(_("unnamed%d"), m_defaultDocumentNameCounter);
    m_defaultDocumentNameCounter++;

    return true;
}

// Make a frame title (override this to do something different)
// If docName is empty, a document is not currently active.
wxString wxDocManager::MakeFrameTitle(wxDocument* doc)
{
    wxString appName = wxTheApp->GetAppName();
    wxString title;
    if (!doc)
        title = appName;
    else
    {
        wxString docName;
        doc->GetPrintableName(docName);
        title = docName + wxString(_(" - ")) + appName;
    }
    return title;
}


// Not yet implemented
wxDocTemplate *wxDocManager::MatchTemplate(const wxString& WXUNUSED(path))
{
    return (wxDocTemplate *) NULL;
}

// File history management
void wxDocManager::AddFileToHistory(const wxString& file)
{
    if (m_fileHistory)
        m_fileHistory->AddFileToHistory(file);
}

void wxDocManager::RemoveFileFromHistory(size_t i)
{
    if (m_fileHistory)
        m_fileHistory->RemoveFileFromHistory(i);
}

wxString wxDocManager::GetHistoryFile(size_t i) const
{
    wxString histFile;

    if (m_fileHistory)
        histFile = m_fileHistory->GetHistoryFile(i);

    return histFile;
}

void wxDocManager::FileHistoryUseMenu(wxMenu *menu)
{
    if (m_fileHistory)
        m_fileHistory->UseMenu(menu);
}

void wxDocManager::FileHistoryRemoveMenu(wxMenu *menu)
{
    if (m_fileHistory)
        m_fileHistory->RemoveMenu(menu);
}

#if wxUSE_CONFIG
void wxDocManager::FileHistoryLoad(wxConfigBase& config)
{
    if (m_fileHistory)
        m_fileHistory->Load(config);
}

void wxDocManager::FileHistorySave(wxConfigBase& config)
{
    if (m_fileHistory)
        m_fileHistory->Save(config);
}
#endif

void wxDocManager::FileHistoryAddFilesToMenu(wxMenu* menu)
{
    if (m_fileHistory)
        m_fileHistory->AddFilesToMenu(menu);
}

void wxDocManager::FileHistoryAddFilesToMenu()
{
    if (m_fileHistory)
        m_fileHistory->AddFilesToMenu();
}

size_t wxDocManager::GetHistoryFilesCount() const
{
    return m_fileHistory ? m_fileHistory->GetCount() : 0;
}


// Find out the document template via matching in the document file format
// against that of the template
wxDocTemplate *wxDocManager::FindTemplateForPath(const wxString& path)
{
    wxDocTemplate *theTemplate = (wxDocTemplate *) NULL;

    // Find the template which this extension corresponds to
    for (size_t i = 0; i < m_templates.GetCount(); i++)
    {
        wxDocTemplate *temp = (wxDocTemplate *)m_templates.Item(i)->GetData();
        if ( temp->FileMatchesTemplate(path) )
        {
            theTemplate = temp;
            break;
        }
    }
    return theTemplate;
}

// Try to get a more suitable parent frame than the top window,
// for selection dialogs. Otherwise you may get an unexpected
// window being activated when a dialog is shown.
static wxWindow* wxFindSuitableParent()
{
    wxWindow* parent = wxTheApp->GetTopWindow();

    wxWindow* focusWindow = wxWindow::FindFocus();
    if (focusWindow)
    {
        while (focusWindow &&
                !focusWindow->IsKindOf(CLASSINFO(wxDialog)) &&
                !focusWindow->IsKindOf(CLASSINFO(wxFrame)))

            focusWindow = focusWindow->GetParent();

        if (focusWindow)
            parent = focusWindow;
    }
    return parent;
}

// Prompts user to open a file, using file specs in templates.
// Must extend the file selector dialog or implement own; OR
// match the extension to the template extension.

wxDocTemplate *wxDocManager::SelectDocumentPath(wxDocTemplate **templates,
#if defined(__WXMSW__) || defined(__WXGTK__) || defined(__WXMAC__)
                                                int noTemplates,
#else
                                                int WXUNUSED(noTemplates),
#endif
                                                wxString& path,
                                                long WXUNUSED(flags),
                                                bool WXUNUSED(save))
{
    // We can only have multiple filters in Windows and GTK
#if defined(__WXMSW__) || defined(__WXGTK__) || defined(__WXMAC__)
    wxString descrBuf;

    int i;
    for (i = 0; i < noTemplates; i++)
    {
        if (templates[i]->IsVisible())
        {
            // add a '|' to separate this filter from the previous one
            if ( !descrBuf.empty() )
                descrBuf << wxT('|');

            descrBuf << templates[i]->GetDescription()
                << wxT(" (") << templates[i]->GetFileFilter() << wxT(") |")
                << templates[i]->GetFileFilter();
        }
    }
#else
    wxString descrBuf = wxT("*.*");
#endif

    int FilterIndex = -1;

    wxWindow* parent = wxFindSuitableParent();

    wxString pathTmp = wxFileSelectorEx(_("Select a file"),
                                        m_lastDirectory,
                                        wxEmptyString,
                                        &FilterIndex,
                                        descrBuf,
                                        0,
                                        parent);

    wxDocTemplate *theTemplate = (wxDocTemplate *)NULL;
    if (!pathTmp.empty())
    {
        if (!wxFileExists(pathTmp))
        {
            wxString msgTitle;
            if (!wxTheApp->GetAppName().empty())
                msgTitle = wxTheApp->GetAppName();
            else
                msgTitle = wxString(_("File error"));

            (void)wxMessageBox(_("Sorry, could not open this file."), msgTitle, wxOK | wxICON_EXCLAMATION,
                parent);

            path = wxEmptyString;
            return (wxDocTemplate *) NULL;
        }
        m_lastDirectory = wxPathOnly(pathTmp);

        path = pathTmp;

        // first choose the template using the extension, if this fails (i.e.
        // wxFileSelectorEx() didn't fill it), then use the path
        if ( FilterIndex != -1 )
            theTemplate = templates[FilterIndex];
        if ( !theTemplate )
            theTemplate = FindTemplateForPath(path);
        if ( !theTemplate )
        {
            // Since we do not add files with non-default extensions to the FileHistory this
            // can only happen if the application changes the allowed templates in runtime.
            (void)wxMessageBox(_("Sorry, the format for this file is unknown."),
                                _("Open File"),
                                wxOK | wxICON_EXCLAMATION, wxFindSuitableParent());
        }
    }
    else
    {
        path = wxEmptyString;
    }

    return theTemplate;
}

wxDocTemplate *wxDocManager::SelectDocumentType(wxDocTemplate **templates,
                                                int noTemplates, bool sort)
{
    wxArrayString strings;
    wxDocTemplate **data = new wxDocTemplate *[noTemplates];
    int i;
    int n = 0;

    for (i = 0; i < noTemplates; i++)
    {
        if (templates[i]->IsVisible())
        {
            int j;
            bool want = true;
            for (j = 0; j < n; j++)
            {
                //filter out NOT unique documents + view combinations
                if ( templates[i]->m_docTypeName == data[j]->m_docTypeName &&
                     templates[i]->m_viewTypeName == data[j]->m_viewTypeName
                   )
                    want = false;
            }

            if ( want )
            {
                strings.Add(templates[i]->m_description);

                data[n] = templates[i];
                n ++;
            }
        }
    }  // for

    if (sort)
    {
        strings.Sort(); // ascending sort
        // Yes, this will be slow, but template lists
        // are typically short.
        int j;
        n = strings.Count();
        for (i = 0; i < n; i++)
        {
            for (j = 0; j < noTemplates; j++)
            {
                if (strings[i] == templates[j]->m_description)
                    data[i] = templates[j];
            }
        }
    }

    wxDocTemplate *theTemplate;

    switch ( n )
    {
        case 0:
            // no visible templates, hence nothing to choose from
            theTemplate = NULL;
            break;

        case 1:
            // don't propose the user to choose if he heas no choice
            theTemplate = data[0];
            break;

        default:
            // propose the user to choose one of several
            theTemplate = (wxDocTemplate *)wxGetSingleChoiceData
                          (
                            _("Select a document template"),
                            _("Templates"),
                            strings,
                            (void **)data,
                            wxFindSuitableParent()
                          );
    }

    delete[] data;

    return theTemplate;
}

wxDocTemplate *wxDocManager::SelectViewType(wxDocTemplate **templates,
                                            int noTemplates, bool sort)
{
    wxArrayString strings;
    wxDocTemplate **data = new wxDocTemplate *[noTemplates];
    int i;
    int n = 0;

    for (i = 0; i < noTemplates; i++)
    {
        wxDocTemplate *templ = templates[i];
        if ( templ->IsVisible() && !templ->GetViewName().empty() )
        {
            int j;
            bool want = true;
            for (j = 0; j < n; j++)
            {
                //filter out NOT unique views
                if ( templates[i]->m_viewTypeName == data[j]->m_viewTypeName )
                    want = false;
            }

            if ( want )
            {
                strings.Add(templ->m_viewTypeName);
                data[n] = templ;
                n ++;
            }
        }
    }

    if (sort)
    {
        strings.Sort(); // ascending sort
        // Yes, this will be slow, but template lists
        // are typically short.
        int j;
        n = strings.Count();
        for (i = 0; i < n; i++)
        {
            for (j = 0; j < noTemplates; j++)
            {
                if (strings[i] == templates[j]->m_viewTypeName)
                    data[i] = templates[j];
            }
        }
    }

    wxDocTemplate *theTemplate;

    // the same logic as above
    switch ( n )
    {
        case 0:
            theTemplate = (wxDocTemplate *)NULL;
            break;

        case 1:
            theTemplate = data[0];
            break;

        default:
            theTemplate = (wxDocTemplate *)wxGetSingleChoiceData
                          (
                            _("Select a document view"),
                            _("Views"),
                            strings,
                            (void **)data,
                            wxFindSuitableParent()
                          );

    }

    delete[] data;
    return theTemplate;
}

void wxDocManager::AssociateTemplate(wxDocTemplate *temp)
{
    if (!m_templates.Member(temp))
        m_templates.Append(temp);
}

void wxDocManager::DisassociateTemplate(wxDocTemplate *temp)
{
    m_templates.DeleteObject(temp);
}

// Add and remove a document from the manager's list
void wxDocManager::AddDocument(wxDocument *doc)
{
    if (!m_docs.Member(doc))
        m_docs.Append(doc);
}

void wxDocManager::RemoveDocument(wxDocument *doc)
{
    m_docs.DeleteObject(doc);
}

// Views or windows should inform the document manager
// when a view is going in or out of focus
void wxDocManager::ActivateView(wxView *view, bool activate)
{
    if ( activate )
    {
        m_currentView = view;
    }
    else // deactivate
    {
        if ( m_currentView == view )
        {
            // don't keep stale pointer
            m_currentView = (wxView *) NULL;
        }
    }
}

// ----------------------------------------------------------------------------
// Default document child frame
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxDocChildFrame, wxFrame)
    EVT_ACTIVATE(wxDocChildFrame::OnActivate)
    EVT_CLOSE(wxDocChildFrame::OnCloseWindow)
END_EVENT_TABLE()

wxDocChildFrame::wxDocChildFrame(wxDocument *doc,
                                 wxView *view,
                                 wxFrame *frame,
                                 wxWindowID id,
                                 const wxString& title,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 long style,
                                 const wxString& name)
               : wxFrame(frame, id, title, pos, size, style, name)
{
    m_childDocument = doc;
    m_childView = view;
    if (view)
        view->SetFrame(this);
}

// Extend event processing to search the view's event table
bool wxDocChildFrame::ProcessEvent(wxEvent& event)
{
    if (m_childView)
        m_childView->Activate(true);

    if ( !m_childView || ! m_childView->ProcessEvent(event) )
    {
        // Only hand up to the parent if it's a menu command
        if (!event.IsKindOf(CLASSINFO(wxCommandEvent)) || !GetParent() || !GetParent()->ProcessEvent(event))
            return wxEvtHandler::ProcessEvent(event);
        else
            return true;
    }
    else
        return true;
}

void wxDocChildFrame::OnActivate(wxActivateEvent& event)
{
    wxFrame::OnActivate(event);

    if (m_childView)
        m_childView->Activate(event.GetActive());
}

void wxDocChildFrame::OnCloseWindow(wxCloseEvent& event)
{
    if (m_childView)
    {
        bool ans = event.CanVeto()
                    ? m_childView->Close(false) // false means don't delete associated window
                    : true; // Must delete.

        if (ans)
        {
            m_childView->Activate(false);
            delete m_childView;
            m_childView = (wxView *) NULL;
            m_childDocument = (wxDocument *) NULL;

            this->Destroy();
        }
        else
            event.Veto();
    }
    else
        event.Veto();
}

// ----------------------------------------------------------------------------
// Default parent frame
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxDocParentFrame, wxFrame)
    EVT_MENU(wxID_EXIT, wxDocParentFrame::OnExit)
    EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, wxDocParentFrame::OnMRUFile)
    EVT_CLOSE(wxDocParentFrame::OnCloseWindow)
END_EVENT_TABLE()

wxDocParentFrame::wxDocParentFrame()
{
    m_docManager = NULL;
}

wxDocParentFrame::wxDocParentFrame(wxDocManager *manager,
                                   wxFrame *frame,
                                   wxWindowID id,
                                   const wxString& title,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   long style,
                                   const wxString& name)
                : wxFrame(frame, id, title, pos, size, style, name)
{
    m_docManager = manager;
}

bool wxDocParentFrame::Create(wxDocManager *manager,
                              wxFrame *frame,
                              wxWindowID id,
                              const wxString& title,
                              const wxPoint& pos,
                              const wxSize& size,
                              long style,
                              const wxString& name)
{
    m_docManager = manager;
    return base_type::Create(frame, id, title, pos, size, style, name);
}

void wxDocParentFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
    Close();
}

void wxDocParentFrame::OnMRUFile(wxCommandEvent& event)
{
    int n = event.GetId() - wxID_FILE1;  // the index in MRU list
    wxString filename(m_docManager->GetHistoryFile(n));
    if ( !filename.empty() )
    {
        // verify that the file exists before doing anything else
        if ( wxFile::Exists(filename) )
        {
            // try to open it
            if (!m_docManager->CreateDocument(filename, wxDOC_SILENT))
            {
                // remove the file from the MRU list. The user should already be notified.
                m_docManager->RemoveFileFromHistory(n);

                wxLogError(_("The file '%s' couldn't be opened.\nIt has been removed from the most recently used files list."),
                       filename.c_str());
            }
        }
        else
        {
            // remove the bogus filename from the MRU list and notify the user
            // about it
            m_docManager->RemoveFileFromHistory(n);

            wxLogError(_("The file '%s' doesn't exist and couldn't be opened.\nIt has been removed from the most recently used files list."),
                       filename.c_str());
        }
    }
}

// Extend event processing to search the view's event table
bool wxDocParentFrame::ProcessEvent(wxEvent& event)
{
    // Try the document manager, then do default processing
    if (!m_docManager || !m_docManager->ProcessEvent(event))
        return wxEvtHandler::ProcessEvent(event);
    else
        return true;
}

// Define the behaviour for the frame closing
// - must delete all frames except for the main one.
void wxDocParentFrame::OnCloseWindow(wxCloseEvent& event)
{
    if ( m_docManager && !m_docManager->Clear(!event.CanVeto()) )
    {
        // The user decided not to close finally, abort.
        event.Veto();
    }
    else
    {
        this->Destroy();
    }
}

#if wxUSE_PRINTING_ARCHITECTURE

wxDocPrintout::wxDocPrintout(wxView *view, const wxString& title)
             : wxPrintout(title)
{
    m_printoutView = view;
}

bool wxDocPrintout::OnPrintPage(int WXUNUSED(page))
{
    wxDC *dc = GetDC();

    // Get the logical pixels per inch of screen and printer
    int ppiScreenX, ppiScreenY;
    GetPPIScreen(&ppiScreenX, &ppiScreenY);
    wxUnusedVar(ppiScreenY);
    int ppiPrinterX, ppiPrinterY;
    GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
    wxUnusedVar(ppiPrinterY);

    // This scales the DC so that the printout roughly represents the
    // the screen scaling. The text point size _should_ be the right size
    // but in fact is too small for some reason. This is a detail that will
    // need to be addressed at some point but can be fudged for the
    // moment.
    float scale = (float)((float)ppiPrinterX/(float)ppiScreenX);

    // Now we have to check in case our real page size is reduced
    // (e.g. because we're drawing to a print preview memory DC)
    int pageWidth, pageHeight;
    int w, h;
    dc->GetSize(&w, &h);
    GetPageSizePixels(&pageWidth, &pageHeight);
    wxUnusedVar(pageHeight);

    // If printer pageWidth == current DC width, then this doesn't
    // change. But w might be the preview bitmap width, so scale down.
    float overallScale = scale * (float)(w/(float)pageWidth);
    dc->SetUserScale(overallScale, overallScale);

    if (m_printoutView)
    {
        m_printoutView->OnDraw(dc);
    }
    return true;
}

bool wxDocPrintout::HasPage(int pageNum)
{
    return (pageNum == 1);
}

bool wxDocPrintout::OnBeginDocument(int startPage, int endPage)
{
    if (!wxPrintout::OnBeginDocument(startPage, endPage))
        return false;

    return true;
}

void wxDocPrintout::GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo)
{
    *minPage = 1;
    *maxPage = 1;
    *selPageFrom = 1;
    *selPageTo = 1;
}

#endif // wxUSE_PRINTING_ARCHITECTURE

// ----------------------------------------------------------------------------
// File history processor
// ----------------------------------------------------------------------------

static inline wxChar* MYcopystring(const wxString& s)
{
    wxChar* copy = new wxChar[s.length() + 1];
    return wxStrcpy(copy, s.c_str());
}

static inline wxChar* MYcopystring(const wxChar* s)
{
    wxChar* copy = new wxChar[wxStrlen(s) + 1];
    return wxStrcpy(copy, s);
}

wxFileHistory::wxFileHistory(size_t maxFiles, wxWindowID idBase)
{
    m_fileMaxFiles = maxFiles;
    m_idBase = idBase;
    m_fileHistoryN = 0;
    m_fileHistory = new wxChar *[m_fileMaxFiles];
}

wxFileHistory::~wxFileHistory()
{
    size_t i;
    for (i = 0; i < m_fileHistoryN; i++)
        delete[] m_fileHistory[i];
    delete[] m_fileHistory;
}

// File history management
void wxFileHistory::AddFileToHistory(const wxString& file)
{
    size_t i;

    // Check we don't already have this file
    for (i = 0; i < m_fileHistoryN; i++)
    {
#if defined( __WXMSW__ ) // Add any other OSes with case insensitive file names
        wxString testString;
        if ( m_fileHistory[i] )
            testString = m_fileHistory[i];
        if ( m_fileHistory[i] && ( file.Lower() == testString.Lower() ) )
#else
        if ( m_fileHistory[i] && ( file == m_fileHistory[i] ) )
#endif
        {
            // we do have it, move it to the top of the history
            RemoveFileFromHistory (i);
            AddFileToHistory (file);
            return;
        }
    }

    // if we already have a full history, delete the one at the end
    if ( m_fileMaxFiles == m_fileHistoryN )
    {
        RemoveFileFromHistory (m_fileHistoryN - 1);
        AddFileToHistory (file);
        return;
    }

    // Add to the project file history:
    // Move existing files (if any) down so we can insert file at beginning.
    if (m_fileHistoryN < m_fileMaxFiles)
    {
        wxList::compatibility_iterator node = m_fileMenus.GetFirst();
        while (node)
        {
            wxMenu* menu = (wxMenu*) node->GetData();
            if ( m_fileHistoryN == 0 && menu->GetMenuItemCount() )
            {
                menu->AppendSeparator();
            }
            menu->Append(m_idBase+m_fileHistoryN, _("[EMPTY]"));
            node = node->GetNext();
        }
        m_fileHistoryN ++;
    }
    // Shuffle filenames down
    for (i = (m_fileHistoryN-1); i > 0; i--)
    {
        m_fileHistory[i] = m_fileHistory[i-1];
    }
    m_fileHistory[0] = MYcopystring(file);

    // this is the directory of the last opened file
    wxString pathCurrent;
    wxSplitPath( m_fileHistory[0], &pathCurrent, NULL, NULL );
    for (i = 0; i < m_fileHistoryN; i++)
    {
        if ( m_fileHistory[i] )
        {
            // if in same directory just show the filename; otherwise the full
            // path
            wxString pathInMenu, path, filename, ext;
            wxSplitPath( m_fileHistory[i], &path, &filename, &ext );
            if ( path == pathCurrent )
            {
                pathInMenu = filename;
                if ( !ext.empty() )
                    pathInMenu = pathInMenu + wxFILE_SEP_EXT + ext;
            }
            else
            {
                // absolute path; could also set relative path
                pathInMenu = m_fileHistory[i];
            }

            // we need to quote '&' characters which are used for mnemonics
            pathInMenu.Replace(_T("&"), _T("&&"));
            wxString buf;
            buf.Printf(s_MRUEntryFormat, i + 1, pathInMenu.c_str());
            wxList::compatibility_iterator node = m_fileMenus.GetFirst();
            while (node)
            {
                wxMenu* menu = (wxMenu*) node->GetData();
                menu->SetLabel(m_idBase + i, buf);
                node = node->GetNext();
            }
        }
    }
}

void wxFileHistory::RemoveFileFromHistory(size_t i)
{
    wxCHECK_RET( i < m_fileHistoryN,
                 wxT("invalid index in wxFileHistory::RemoveFileFromHistory") );

    // delete the element from the array (could use memmove() too...)
    delete [] m_fileHistory[i];

    size_t j;
    for ( j = i; j < m_fileHistoryN - 1; j++ )
    {
        m_fileHistory[j] = m_fileHistory[j + 1];
    }

    wxList::compatibility_iterator node = m_fileMenus.GetFirst();
    while ( node )
    {
        wxMenu* menu = (wxMenu*) node->GetData();

        // shuffle filenames up
        wxString buf;
        for ( j = i; j < m_fileHistoryN - 1; j++ )
        {
            buf.Printf(s_MRUEntryFormat, j + 1, m_fileHistory[j]);
            menu->SetLabel(m_idBase + j, buf);
        }

        node = node->GetNext();

        // delete the last menu item which is unused now
        wxWindowID lastItemId = m_idBase + wx_truncate_cast(wxWindowID, m_fileHistoryN) - 1;
        if (menu->FindItem(lastItemId))
        {
            menu->Delete(lastItemId);
        }

        // delete the last separator too if no more files are left
        if ( m_fileHistoryN == 1 )
        {
            wxMenuItemList::compatibility_iterator nodeLast = menu->GetMenuItems().GetLast();
            if ( nodeLast )
            {
                wxMenuItem *menuItem = nodeLast->GetData();
                if ( menuItem->IsSeparator() )
                {
                    menu->Delete(menuItem);
                }
                //else: should we search backwards for the last separator?
            }
            //else: menu is empty somehow
        }
    }

    m_fileHistoryN--;
}

wxString wxFileHistory::GetHistoryFile(size_t i) const
{
    wxString s;
    if ( i < m_fileHistoryN )
    {
        s = m_fileHistory[i];
    }
    else
    {
        wxFAIL_MSG( wxT("bad index in wxFileHistory::GetHistoryFile") );
    }

    return s;
}

void wxFileHistory::UseMenu(wxMenu *menu)
{
    if (!m_fileMenus.Member(menu))
        m_fileMenus.Append(menu);
}

void wxFileHistory::RemoveMenu(wxMenu *menu)
{
    m_fileMenus.DeleteObject(menu);
}

#if wxUSE_CONFIG
void wxFileHistory::Load(wxConfigBase& config)
{
    m_fileHistoryN = 0;
    wxString buf;
    buf.Printf(wxT("file%d"), (int)m_fileHistoryN+1);
    wxString historyFile;
    while ((m_fileHistoryN < m_fileMaxFiles) && config.Read(buf, &historyFile) && (!historyFile.empty()))
    {
        m_fileHistory[m_fileHistoryN] = MYcopystring((const wxChar*) historyFile);
        m_fileHistoryN ++;
        buf.Printf(wxT("file%d"), (int)m_fileHistoryN+1);
        historyFile = wxEmptyString;
    }
    AddFilesToMenu();
}

void wxFileHistory::Save(wxConfigBase& config)
{
    size_t i;
    for (i = 0; i < m_fileMaxFiles; i++)
    {
        wxString buf;
        buf.Printf(wxT("file%d"), (int)i+1);
        if (i < m_fileHistoryN)
            config.Write(buf, wxString(m_fileHistory[i]));
        else
            config.Write(buf, wxEmptyString);
    }
}
#endif // wxUSE_CONFIG

void wxFileHistory::AddFilesToMenu()
{
    if (m_fileHistoryN > 0)
    {
        wxList::compatibility_iterator node = m_fileMenus.GetFirst();
        while (node)
        {
            wxMenu* menu = (wxMenu*) node->GetData();
            if (menu->GetMenuItemCount())
            {
                menu->AppendSeparator();
            }

            size_t i;
            for (i = 0; i < m_fileHistoryN; i++)
            {
                if (m_fileHistory[i])
                {
                    wxString buf;
                    buf.Printf(s_MRUEntryFormat, i+1, m_fileHistory[i]);
                    menu->Append(m_idBase+i, buf);
                }
            }
            node = node->GetNext();
        }
    }
}

void wxFileHistory::AddFilesToMenu(wxMenu* menu)
{
    if (m_fileHistoryN > 0)
    {
        if (menu->GetMenuItemCount())
        {
            menu->AppendSeparator();
        }

        size_t i;
        for (i = 0; i < m_fileHistoryN; i++)
        {
            if (m_fileHistory[i])
            {
                wxString buf;
                buf.Printf(s_MRUEntryFormat, i+1, m_fileHistory[i]);
                menu->Append(m_idBase+i, buf);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Permits compatibility with existing file formats and functions that
// manipulate files directly
// ----------------------------------------------------------------------------

#if wxUSE_STD_IOSTREAM

bool wxTransferFileToStream(const wxString& filename, wxSTD ostream& stream)
{
    wxFFile file(filename, _T("rb"));
    if ( !file.IsOpened() )
        return false;

    char buf[4096];

    size_t nRead;
    do
    {
        nRead = file.Read(buf, WXSIZEOF(buf));
        if ( file.Error() )
            return false;

        stream.write(buf, nRead);
        if ( !stream )
            return false;
    }
    while ( !file.Eof() );

    return true;
}

bool wxTransferStreamToFile(wxSTD istream& stream, const wxString& filename)
{
    wxFFile file(filename, _T("wb"));
    if ( !file.IsOpened() )
        return false;

    char buf[4096];
    do
    {
        stream.read(buf, WXSIZEOF(buf));
        if ( !stream.bad() ) // fail may be set on EOF, don't use operator!()
        {
            if ( !file.Write(buf, stream.gcount()) )
                return false;
        }
    }
    while ( !stream.eof() );

    return true;
}

#else // !wxUSE_STD_IOSTREAM

bool wxTransferFileToStream(const wxString& filename, wxOutputStream& stream)
{
    wxFFile file(filename, _T("rb"));
    if ( !file.IsOpened() )
        return false;

    char buf[4096];

    size_t nRead;
    do
    {
        nRead = file.Read(buf, WXSIZEOF(buf));
        if ( file.Error() )
            return false;

        stream.Write(buf, nRead);
        if ( !stream )
            return false;
    }
    while ( !file.Eof() );

    return true;
}

bool wxTransferStreamToFile(wxInputStream& stream, const wxString& filename)
{
    wxFFile file(filename, _T("wb"));
    if ( !file.IsOpened() )
        return false;

    char buf[4096];
    for ( ;; )
    {
        stream.Read(buf, WXSIZEOF(buf));

        const size_t nRead = stream.LastRead();
        if ( !nRead )
        {
            if ( stream.Eof() )
                break;

            return false;
        }

        if ( !file.Write(buf, nRead) )
            return false;
    }

    return true;
}

#endif // wxUSE_STD_IOSTREAM/!wxUSE_STD_IOSTREAM

#endif // wxUSE_DOC_VIEW_ARCHITECTURE
