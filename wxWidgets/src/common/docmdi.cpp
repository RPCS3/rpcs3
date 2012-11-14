/////////////////////////////////////////////////////////////////////////////
// Name:        docmdi.cpp
// Purpose:     Frame classes for MDI document/view applications
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: docmdi.cpp 66911 2011-02-16 21:31:33Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_MDI_ARCHITECTURE

#include "wx/docmdi.h"

/*
 * Docview MDI parent frame
 */

IMPLEMENT_CLASS(wxDocMDIParentFrame, wxMDIParentFrame)

BEGIN_EVENT_TABLE(wxDocMDIParentFrame, wxMDIParentFrame)
    EVT_MENU(wxID_EXIT, wxDocMDIParentFrame::OnExit)
    EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, wxDocMDIParentFrame::OnMRUFile)
    EVT_CLOSE(wxDocMDIParentFrame::OnCloseWindow)
END_EVENT_TABLE()

wxDocMDIParentFrame::wxDocMDIParentFrame()
{
    Init();
}

wxDocMDIParentFrame::wxDocMDIParentFrame(wxDocManager *manager, wxFrame *frame, wxWindowID id, const wxString& title,
  const wxPoint& pos, const wxSize& size, long style, const wxString& name)
{
    Init();
    Create(manager, frame, id, title, pos, size, style, name);
}

bool wxDocMDIParentFrame::Create(wxDocManager *manager, wxFrame *frame, wxWindowID id, const wxString& title,
  const wxPoint& pos, const wxSize& size, long style, const wxString& name)
{
    m_docManager = manager;
    return wxMDIParentFrame::Create(frame, id, title, pos, size, style, name);
}

void wxDocMDIParentFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
    Close();
}

void wxDocMDIParentFrame::Init()
{
    m_docManager = NULL;
}

void wxDocMDIParentFrame::OnMRUFile(wxCommandEvent& event)
{
    wxString f(m_docManager->GetHistoryFile(event.GetId() - wxID_FILE1));
    if (!f.empty())
        (void)m_docManager->CreateDocument(f, wxDOC_SILENT);
}

// Extend event processing to search the view's event table
bool wxDocMDIParentFrame::ProcessEvent(wxEvent& event)
{
    // Try the document manager, then do default processing
    if (!m_docManager || !m_docManager->ProcessEvent(event))
        return wxEvtHandler::ProcessEvent(event);
    else
        return true;
}

void wxDocMDIParentFrame::OnCloseWindow(wxCloseEvent& event)
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

/*
 * Default document child frame for MDI children
 */

IMPLEMENT_CLASS(wxDocMDIChildFrame, wxMDIChildFrame)

BEGIN_EVENT_TABLE(wxDocMDIChildFrame, wxMDIChildFrame)
    EVT_ACTIVATE(wxDocMDIChildFrame::OnActivate)
    EVT_CLOSE(wxDocMDIChildFrame::OnCloseWindow)
END_EVENT_TABLE()

void wxDocMDIChildFrame::Init()
{
    m_childDocument = (wxDocument*)  NULL;
    m_childView = (wxView*) NULL;
}

wxDocMDIChildFrame::wxDocMDIChildFrame()
{
    Init();
}

wxDocMDIChildFrame::wxDocMDIChildFrame(wxDocument *doc, wxView *view, wxMDIParentFrame *frame, wxWindowID  id,
  const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
{
    Init();
    Create(doc, view, frame, id, title, pos, size, style, name);
}

bool wxDocMDIChildFrame::Create(wxDocument *doc, wxView *view, wxMDIParentFrame *frame, wxWindowID  id,
  const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
{
    m_childDocument = doc;
    m_childView = view;
    if (wxMDIChildFrame::Create(frame, id, title, pos, size, style, name))
    {
        if (view)
            view->SetFrame(this);
        return true;
    }

    return false;
}

wxDocMDIChildFrame::~wxDocMDIChildFrame(void)
{
    m_childView = (wxView *) NULL;
}

// Extend event processing to search the view's event table
bool wxDocMDIChildFrame::ProcessEvent(wxEvent& event)
{
    static wxEvent *ActiveEvent = NULL;

    // Break recursion loops
    if (ActiveEvent == &event)
        return false;

    ActiveEvent = &event;

    bool ret;
    if ( !m_childView || ! m_childView->ProcessEvent(event) )
    {
        // Only hand up to the parent if it's a menu command
        if (!event.IsKindOf(CLASSINFO(wxCommandEvent)) || !GetParent() || !GetParent()->ProcessEvent(event))
            ret = wxEvtHandler::ProcessEvent(event);
        else
            ret = true;
    }
    else
        ret = true;

    ActiveEvent = NULL;
    return ret;
}

void wxDocMDIChildFrame::OnActivate(wxActivateEvent& event)
{
  wxMDIChildFrame::OnActivate(event);

  if (event.GetActive() && m_childView)
    m_childView->Activate(event.GetActive());
}

void wxDocMDIChildFrame::OnCloseWindow(wxCloseEvent& event)
{
  // Close view but don't delete the frame while doing so!
  // ...since it will be deleted by wxWidgets if we return true.
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

#endif
    // wxUSE_DOC_VIEW_ARCHITECTURE

