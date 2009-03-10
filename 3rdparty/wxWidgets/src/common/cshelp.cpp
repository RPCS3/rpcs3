/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/cshelp.cpp
// Purpose:     Context sensitive help class implementation
// Author:      Julian Smart, Vadim Zeitlin
// Modified by:
// Created:     08/09/2000
// RCS-ID:      $Id: cshelp.cpp 52329 2008-03-05 13:20:26Z VZ $
// Copyright:   (c) 2000 Julian Smart, Vadim Zeitlin
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

#if wxUSE_HELP

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/module.h"
#endif

#include "wx/tipwin.h"
#include "wx/cshelp.h"

#if wxUSE_MS_HTML_HELP
    #include "wx/msw/helpchm.h"     // for ShowContextHelpPopup
    #include "wx/utils.h"           // for wxGetMousePosition()
#endif

// ----------------------------------------------------------------------------
// wxContextHelpEvtHandler private class
// ----------------------------------------------------------------------------

// This class exists in order to eat events until the left mouse button is
// pressed
class wxContextHelpEvtHandler: public wxEvtHandler
{
public:
    wxContextHelpEvtHandler(wxContextHelp* contextHelp)
    {
        m_contextHelp = contextHelp;
    }

    virtual bool ProcessEvent(wxEvent& event);

//// Data
    wxContextHelp* m_contextHelp;

    DECLARE_NO_COPY_CLASS(wxContextHelpEvtHandler)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxContextHelp
// ----------------------------------------------------------------------------

/*
 * Invokes context-sensitive help
 */


IMPLEMENT_DYNAMIC_CLASS(wxContextHelp, wxObject)

wxContextHelp::wxContextHelp(wxWindow* win, bool beginHelp)
{
    m_inHelp = false;

    if (beginHelp)
        BeginContextHelp(win);
}

wxContextHelp::~wxContextHelp()
{
    if (m_inHelp)
        EndContextHelp();
}

// Not currently needed, but on some systems capture may not work as
// expected so we'll leave it here for now.
#ifdef __WXMOTIF__
static void wxPushOrPopEventHandlers(wxContextHelp* help, wxWindow* win, bool push)
{
    if (push)
        win->PushEventHandler(new wxContextHelpEvtHandler(help));
    else
        win->PopEventHandler(true);

    wxWindowList::compatibility_iterator node = win->GetChildren().GetFirst();
    while (node)
    {
        wxWindow* child = node->GetData();
        wxPushOrPopEventHandlers(help, child, push);

        node = node->GetNext();
    }
}
#endif

// Begin 'context help mode'
bool wxContextHelp::BeginContextHelp(wxWindow* win)
{
    if (!win)
        win = wxTheApp->GetTopWindow();
    if (!win)
        return false;

    wxCursor cursor(wxCURSOR_QUESTION_ARROW);
    wxCursor oldCursor = win->GetCursor();
    win->SetCursor(cursor);

#ifdef __WXMAC__
    wxSetCursor(cursor);
#endif

    m_status = false;

#ifdef __WXMOTIF__
    wxPushOrPopEventHandlers(this, win, true);
#else
    win->PushEventHandler(new wxContextHelpEvtHandler(this));
#endif

    win->CaptureMouse();

    EventLoop();

    win->ReleaseMouse();

#ifdef __WXMOTIF__
    wxPushOrPopEventHandlers(this, win, false);
#else
    win->PopEventHandler(true);
#endif

    win->SetCursor(oldCursor);

#ifdef __WXMAC__
    wxSetCursor(wxNullCursor);
#endif

    if (m_status)
    {
        wxPoint pt;
        wxWindow* winAtPtr = wxFindWindowAtPointer(pt);

#if 0
        if (winAtPtr)
        {
            printf("Picked %s (%d)\n", winAtPtr->GetName().c_str(),
                   winAtPtr->GetId());
        }
#endif

        if (winAtPtr)
            DispatchEvent(winAtPtr, pt);
    }

    return true;
}

bool wxContextHelp::EndContextHelp()
{
    m_inHelp = false;

    return true;
}

bool wxContextHelp::EventLoop()
{
    m_inHelp = true;

    while ( m_inHelp )
    {
        if (wxTheApp->Pending())
        {
            wxTheApp->Dispatch();
        }
        else
        {
            wxTheApp->ProcessIdle();
        }
    }

    return true;
}

bool wxContextHelpEvtHandler::ProcessEvent(wxEvent& event)
{
    if (event.GetEventType() == wxEVT_LEFT_DOWN)
    {
        m_contextHelp->SetStatus(true);
        m_contextHelp->EndContextHelp();
        return true;
    }

    if ((event.GetEventType() == wxEVT_CHAR) ||
        (event.GetEventType() == wxEVT_KEY_DOWN) ||
        (event.GetEventType() == wxEVT_ACTIVATE) ||
        (event.GetEventType() == wxEVT_MOUSE_CAPTURE_CHANGED))
    {
        // May have already been set to true by a left-click
        //m_contextHelp->SetStatus(false);
        m_contextHelp->EndContextHelp();
        return true;
    }

    if ((event.GetEventType() == wxEVT_PAINT) ||
        (event.GetEventType() == wxEVT_ERASE_BACKGROUND))
    {
        event.Skip();
        return false;
    }

    return true;
}

// Dispatch the help event to the relevant window
bool wxContextHelp::DispatchEvent(wxWindow* win, const wxPoint& pt)
{
    wxCHECK_MSG( win, false, _T("win parameter can't be NULL") );

    wxHelpEvent helpEvent(wxEVT_HELP, win->GetId(), pt,
                          wxHelpEvent::Origin_HelpButton);
    helpEvent.SetEventObject(win);

    return win->GetEventHandler()->ProcessEvent(helpEvent);
}

// ----------------------------------------------------------------------------
// wxContextHelpButton
// ----------------------------------------------------------------------------

/*
 * wxContextHelpButton
 * You can add this to your dialogs (especially on non-Windows platforms)
 * to put the application into context help mode.
 */

#ifndef __WXPM__

static const char * csquery_xpm[] = {
"12 11 2 1",
"  c None",
". c #000000",
"            ",
"    ....    ",
"   ..  ..   ",
"   ..  ..   ",
"      ..    ",
"     ..     ",
"     ..     ",
"            ",
"     ..     ",
"     ..     ",
"            "};

#endif

IMPLEMENT_CLASS(wxContextHelpButton, wxBitmapButton)

BEGIN_EVENT_TABLE(wxContextHelpButton, wxBitmapButton)
    EVT_BUTTON(wxID_CONTEXT_HELP, wxContextHelpButton::OnContextHelp)
END_EVENT_TABLE()

wxContextHelpButton::wxContextHelpButton(wxWindow* parent,
                                         wxWindowID id,
                                         const wxPoint& pos,
                                         const wxSize& size,
                                         long style)
#if defined(__WXPM__)
                   : wxBitmapButton(parent, id, wxBitmap(wxCSQUERY_BITMAP
                                                         ,wxBITMAP_TYPE_RESOURCE
                                                        ),
                                    pos, size, style)
#else
                   : wxBitmapButton(parent, id, wxBitmap(csquery_xpm),
                                    pos, size, style)
#endif
{
}

void wxContextHelpButton::OnContextHelp(wxCommandEvent& WXUNUSED(event))
{
    wxContextHelp contextHelp(GetParent());
}

// ----------------------------------------------------------------------------
// wxHelpProvider
// ----------------------------------------------------------------------------

wxHelpProvider *wxHelpProvider::ms_helpProvider = (wxHelpProvider *)NULL;

// trivial implementation of some methods which we don't want to make pure
// virtual for convenience

void wxHelpProvider::AddHelp(wxWindowBase * WXUNUSED(window),
                             const wxString& WXUNUSED(text))
{
}

void wxHelpProvider::AddHelp(wxWindowID WXUNUSED(id),
                             const wxString& WXUNUSED(text))
{
}

// removes the association
void wxHelpProvider::RemoveHelp(wxWindowBase* WXUNUSED(window))
{
}

wxHelpProvider::~wxHelpProvider()
{
}

wxString wxHelpProvider::GetHelpTextMaybeAtPoint(wxWindowBase *window)
{
    if ( m_helptextAtPoint != wxDefaultPosition ||
            m_helptextOrigin != wxHelpEvent::Origin_Unknown )
    {
        wxCHECK_MSG( window, wxEmptyString, _T("window must not be NULL") );

        wxPoint pt = m_helptextAtPoint;
        wxHelpEvent::Origin origin = m_helptextOrigin;

        m_helptextAtPoint = wxDefaultPosition;
        m_helptextOrigin = wxHelpEvent::Origin_Unknown;

        return window->GetHelpTextAtPoint(pt, origin);
    }

    return GetHelp(window);
}

// ----------------------------------------------------------------------------
// wxSimpleHelpProvider
// ----------------------------------------------------------------------------

#define WINHASH_KEY(w) wxPtrToUInt(w)

wxString wxSimpleHelpProvider::GetHelp(const wxWindowBase *window)
{
    wxSimpleHelpProviderHashMap::iterator it = m_hashWindows.find(WINHASH_KEY(window));

    if ( it == m_hashWindows.end() )
    {
        it = m_hashIds.find(window->GetId());
        if ( it == m_hashIds.end() )
            return wxEmptyString;
    }

    return it->second;
}

void wxSimpleHelpProvider::AddHelp(wxWindowBase *window, const wxString& text)
{
    m_hashWindows.erase(WINHASH_KEY(window));
    m_hashWindows[WINHASH_KEY(window)] = text;
}

void wxSimpleHelpProvider::AddHelp(wxWindowID id, const wxString& text)
{
    wxSimpleHelpProviderHashMap::key_type key = (wxSimpleHelpProviderHashMap::key_type)id;
    m_hashIds.erase(key);
    m_hashIds[key] = text;
}

// removes the association
void wxSimpleHelpProvider::RemoveHelp(wxWindowBase* window)
{
    m_hashWindows.erase(WINHASH_KEY(window));
}

bool wxSimpleHelpProvider::ShowHelp(wxWindowBase *window)
{
#if wxUSE_MS_HTML_HELP || wxUSE_TIPWINDOW
#if wxUSE_MS_HTML_HELP
    // m_helptextAtPoint will be reset by GetHelpTextMaybeAtPoint(), stash it
    const wxPoint posTooltip = m_helptextAtPoint;
#endif // wxUSE_MS_HTML_HELP

    const wxString text = GetHelpTextMaybeAtPoint(window);

    if ( !text.empty() )
    {
        // use the native help popup style if it's available
#if wxUSE_MS_HTML_HELP
        if ( !wxCHMHelpController::ShowContextHelpPopup
                                   (
                                        text,
                                        posTooltip,
                                        (wxWindow *)window
                                   ) )
#endif // wxUSE_MS_HTML_HELP
        {
#if wxUSE_TIPWINDOW
            static wxTipWindow* s_tipWindow = NULL;

            if ( s_tipWindow )
            {
                // Prevent s_tipWindow being nulled in OnIdle, thereby removing
                // the chance for the window to be closed by ShowHelp
                s_tipWindow->SetTipWindowPtr(NULL);
                s_tipWindow->Close();
            }

            s_tipWindow = new wxTipWindow((wxWindow *)window, text,
                                            100, &s_tipWindow);
#else // !wxUSE_TIPWINDOW
            // we tried wxCHMHelpController but it failed and we don't have
            // wxTipWindow to fall back on, so
            return false;
#endif // wxUSE_TIPWINDOW
        }

        return true;
    }
#else // !wxUSE_MS_HTML_HELP && !wxUSE_TIPWINDOW
    wxUnusedVar(window);
#endif // wxUSE_MS_HTML_HELP || wxUSE_TIPWINDOW

    return false;
}

// ----------------------------------------------------------------------------
// wxHelpControllerHelpProvider
// ----------------------------------------------------------------------------

wxHelpControllerHelpProvider::wxHelpControllerHelpProvider(wxHelpControllerBase* hc)
{
    m_helpController = hc;
}

bool wxHelpControllerHelpProvider::ShowHelp(wxWindowBase *window)
{
    const wxString text = GetHelpTextMaybeAtPoint(window);

    if ( text.empty() )
        return false;

    if ( m_helpController )
    {
        // if it's a numeric topic, show it
        long topic;
        if ( text.ToLong(&topic) )
            return m_helpController->DisplayContextPopup(topic);

        // otherwise show the text directly
        if ( m_helpController->DisplayTextPopup(text, wxGetMousePosition()) )
            return true;
    }

    // if there is no help controller or it's not capable of showing the help,
    // fallback to the default method
    return wxSimpleHelpProvider::ShowHelp(window);
}

// Convenience function for turning context id into wxString
wxString wxContextId(int id)
{
    return wxString::Format(_T("%d"), id);
}

// ----------------------------------------------------------------------------
// wxHelpProviderModule: module responsible for cleaning up help provider.
// ----------------------------------------------------------------------------

class wxHelpProviderModule : public wxModule
{
public:
    bool OnInit();
    void OnExit();

private:
    DECLARE_DYNAMIC_CLASS(wxHelpProviderModule)
};

IMPLEMENT_DYNAMIC_CLASS(wxHelpProviderModule, wxModule)

bool wxHelpProviderModule::OnInit()
{
    // Probably we don't want to do anything by default,
    // since it could pull in extra code
    // wxHelpProvider::Set(new wxSimpleHelpProvider);

    return true;
}

void wxHelpProviderModule::OnExit()
{
    if (wxHelpProvider::Get())
    {
        delete wxHelpProvider::Get();
        wxHelpProvider::Set(NULL);
    }
}

#endif // wxUSE_HELP
