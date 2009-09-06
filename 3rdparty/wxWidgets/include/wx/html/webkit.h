/////////////////////////////////////////////////////////////////////////////
// Name:        wx/html/webkit.h
// Purpose:     wxWebKitCtrl - embeddable web kit control
// Author:      Jethro Grassie / Kevin Ollivier
// Modified by:
// Created:     2004-4-16
// RCS-ID:      $Id: webkit.h 53798 2008-05-28 06:12:34Z RD $
// Copyright:   (c) Jethro Grassie / Kevin Ollivier
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_WEBKIT_H
#define _WX_WEBKIT_H

#if wxUSE_WEBKIT

#if !defined(__WXMAC__) && !defined(__WXCOCOA__)
#error "wxWebKitCtrl not implemented for this platform"
#endif

#include "wx/control.h"

// ----------------------------------------------------------------------------
// Web Kit Control
// ----------------------------------------------------------------------------

class wxWebKitCtrl : public wxControl
{
public:
    DECLARE_DYNAMIC_CLASS(wxWebKitCtrl)

    wxWebKitCtrl() {};
    wxWebKitCtrl(wxWindow *parent,
                    wxWindowID winID,
                    const wxString& strURL,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize, long style = 0,
                    const wxValidator& validator = wxDefaultValidator,
                    const wxString& name = wxT("webkitctrl"))
    {
        Create(parent, winID, strURL, pos, size, style, validator, name);
    };
    bool Create(wxWindow *parent,
                wxWindowID winID,
                const wxString& strURL,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxT("webkitctrl"));
    virtual ~wxWebKitCtrl();

    void LoadURL(const wxString &url);

    bool CanGoBack();
    bool CanGoForward();
    bool GoBack();
    bool GoForward();
    void Reload();
    void Stop();
    bool CanGetPageSource();
    wxString GetPageSource();
    void SetPageSource(const wxString& source, const wxString& baseUrl = wxEmptyString);
	wxString GetPageURL(){ return m_currentURL; }
    void SetPageTitle(const wxString& title) { m_pageTitle = title; }
	wxString GetPageTitle(){ return m_pageTitle; }
    
    // since these worked in 2.6, add wrappers
    void SetTitle(const wxString& title) { SetPageTitle(title); }
    wxString GetTitle() { return GetPageTitle(); }
    
    wxString GetSelection();
    
    bool CanIncreaseTextSize();
    void IncreaseTextSize();
    bool CanDecreaseTextSize();
    void DecreaseTextSize();
    
    void Print(bool showPrompt=FALSE);
    
    void MakeEditable(bool enable=TRUE);
    bool IsEditable();
    
    wxString RunScript(const wxString& javascript);
    
    void SetScrollPos(int pos);
    int GetScrollPos();

    //we need to resize the webview when the control size changes
    void OnSize(wxSizeEvent &event);
    void OnMove(wxMoveEvent &event);
    void OnMouseEvents(wxMouseEvent &event);
protected:
    DECLARE_EVENT_TABLE()
    void MacVisibilityChanged();

private:
    wxWindow *m_parent;
    wxWindowID m_windowID;
    wxString m_currentURL;
    wxString m_pageTitle;
    
    struct objc_object *m_webView;
    
    // we may use this later to setup our own mouse events,
    // so leave it in for now.
    void* m_webKitCtrlEventHandler;
    //It should be WebView*, but WebView is an Objective-C class
    //TODO: look into using DECLARE_WXCOCOA_OBJC_CLASS rather than this.
};

// ----------------------------------------------------------------------------
// Web Kit Events
// ----------------------------------------------------------------------------

enum {
    wxWEBKIT_STATE_START = 1,
    wxWEBKIT_STATE_NEGOTIATING = 2,
    wxWEBKIT_STATE_REDIRECTING = 4,
    wxWEBKIT_STATE_TRANSFERRING = 8,
    wxWEBKIT_STATE_STOP = 16,
        wxWEBKIT_STATE_FAILED = 32
};

enum {
    wxWEBKIT_NAV_LINK_CLICKED = 1,
    wxWEBKIT_NAV_BACK_NEXT = 2,
    wxWEBKIT_NAV_FORM_SUBMITTED = 4,
    wxWEBKIT_NAV_RELOAD = 8,
    wxWEBKIT_NAV_FORM_RESUBMITTED = 16,
    wxWEBKIT_NAV_OTHER = 32

};



class wxWebKitBeforeLoadEvent : public wxCommandEvent
{
    DECLARE_DYNAMIC_CLASS( wxWebKitBeforeLoadEvent )
    
public:
    bool IsCancelled() { return m_cancelled; }
    void Cancel(bool cancel = true) { m_cancelled = cancel; }
    wxString GetURL() { return m_url; }
    void SetURL(const wxString& url) { m_url = url; }
    void SetNavigationType(int navType) { m_navType = navType; }
    int GetNavigationType() { return m_navType; }

    wxWebKitBeforeLoadEvent( wxWindow* win = (wxWindow*) NULL );
    wxEvent *Clone(void) const { return new wxWebKitBeforeLoadEvent(*this); }

protected:
    bool m_cancelled;
    wxString m_url;
    int m_navType;
};

class wxWebKitStateChangedEvent : public wxCommandEvent
{
    DECLARE_DYNAMIC_CLASS( wxWebKitStateChangedEvent )

public:
    int GetState() { return m_state; }
    void SetState(const int state) { m_state = state; }
    wxString GetURL() { return m_url; }
    void SetURL(const wxString& url) { m_url = url; }

    wxWebKitStateChangedEvent( wxWindow* win = (wxWindow*) NULL );
    wxEvent *Clone(void) const { return new wxWebKitStateChangedEvent(*this); }

protected:
    int m_state;
    wxString m_url;
};


#if wxABI_VERSION >= 20808
class wxWebKitNewWindowEvent : public wxCommandEvent
{
    DECLARE_DYNAMIC_CLASS( wxWebViewNewWindowEvent )
public:
    wxString GetURL() const { return m_url; }
    void SetURL(const wxString& url) { m_url = url; }
    wxString GetTargetName() const { return m_targetName; }
    void SetTargetName(const wxString& name) { m_targetName = name; }

    wxWebKitNewWindowEvent( wxWindow* win = (wxWindow*)(NULL));
    wxEvent *Clone(void) const { return new wxWebKitNewWindowEvent(*this); }

private:
    wxString m_url;
    wxString m_targetName;
};
#endif

typedef void (wxEvtHandler::*wxWebKitStateChangedEventFunction)(wxWebKitStateChangedEvent&);
typedef void (wxEvtHandler::*wxWebKitBeforeLoadEventFunction)(wxWebKitBeforeLoadEvent&);
typedef void (wxEvtHandler::*wxWebKitNewWindowEventFunction)(wxWebKitNewWindowEvent&);

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_WEBKIT_BEFORE_LOAD, wxID_ANY)
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_WEBKIT_STATE_CHANGED, wxID_ANY)
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_WEBKIT_NEW_WINDOW, wxID_ANY)
END_DECLARE_EVENT_TYPES()

#define EVT_WEBKIT_STATE_CHANGED(func) \
            DECLARE_EVENT_TABLE_ENTRY( wxEVT_WEBKIT_STATE_CHANGED, \
                            wxID_ANY, \
                            wxID_ANY, \
                            (wxObjectEventFunction)   \
                            (wxWebKitStateChangedEventFunction) & func, \
                            (wxObject *) NULL ),
                            
#define EVT_WEBKIT_BEFORE_LOAD(func) \
            DECLARE_EVENT_TABLE_ENTRY( wxEVT_WEBKIT_BEFORE_LOAD, \
                            wxID_ANY, \
                            wxID_ANY, \
                            (wxObjectEventFunction)   \
                            (wxWebKitBeforeLoadEventFunction) & func, \
                            (wxObject *) NULL ),

#define EVT_WEBKIT_NEW_WINDOW(func)                              \
            DECLARE_EVENT_TABLE_ENTRY( wxEVT_WEBKIT_NEW_WINDOW, \
                            wxID_ANY, \
                            wxID_ANY, \
                            (wxObjectEventFunction)   \
                            (wxWebKitNewWindowEventFunction) & func, \
                            (wxObject *) NULL ),
#endif // wxUSE_WEBKIT

#endif
    // _WX_WEBKIT_H_
