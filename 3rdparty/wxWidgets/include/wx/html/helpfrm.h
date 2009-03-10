/////////////////////////////////////////////////////////////////////////////
// Name:        helpfrm.h
// Purpose:     wxHtmlHelpFrame
// Notes:       Based on htmlhelp.cpp, implementing a monolithic
//              HTML Help controller class,  by Vaclav Slavik
// Author:      Harm van der Heijden and Vaclav Slavik
// RCS-ID:      $Id: helpfrm.h 50202 2007-11-23 21:29:29Z VZ $
// Copyright:   (c) Harm van der Heijden and Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HELPFRM_H_
#define _WX_HELPFRM_H_

#include "wx/defs.h"

#if wxUSE_WXHTML_HELP

#include "wx/helpbase.h"
#include "wx/html/helpdata.h"
#include "wx/window.h"
#include "wx/frame.h"
#include "wx/config.h"
#include "wx/splitter.h"
#include "wx/notebook.h"
#include "wx/listbox.h"
#include "wx/choice.h"
#include "wx/combobox.h"
#include "wx/checkbox.h"
#include "wx/stattext.h"
#include "wx/html/htmlwin.h"
#include "wx/html/helpwnd.h"
#include "wx/html/htmprint.h"

class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
class WXDLLIMPEXP_FWD_CORE wxTreeEvent;
class WXDLLIMPEXP_FWD_CORE wxTreeCtrl;


// style flags for the Help Frame
#define wxHF_TOOLBAR                0x0001
#define wxHF_CONTENTS               0x0002
#define wxHF_INDEX                  0x0004
#define wxHF_SEARCH                 0x0008
#define wxHF_BOOKMARKS              0x0010
#define wxHF_OPEN_FILES             0x0020
#define wxHF_PRINT                  0x0040
#define wxHF_FLAT_TOOLBAR           0x0080
#define wxHF_MERGE_BOOKS            0x0100
#define wxHF_ICONS_BOOK             0x0200
#define wxHF_ICONS_BOOK_CHAPTER     0x0400
#define wxHF_ICONS_FOLDER           0x0000 // this is 0 since it is default
#define wxHF_DEFAULT_STYLE          (wxHF_TOOLBAR | wxHF_CONTENTS | \
                                     wxHF_INDEX | wxHF_SEARCH | \
                                     wxHF_BOOKMARKS | wxHF_PRINT)
//compatibility:
#define wxHF_OPENFILES               wxHF_OPEN_FILES
#define wxHF_FLATTOOLBAR             wxHF_FLAT_TOOLBAR
#define wxHF_DEFAULTSTYLE            wxHF_DEFAULT_STYLE

struct wxHtmlHelpMergedIndexItem;
class wxHtmlHelpMergedIndex;

class WXDLLIMPEXP_FWD_CORE wxHelpControllerBase;
class WXDLLIMPEXP_FWD_HTML wxHtmlHelpController;
class WXDLLIMPEXP_FWD_HTML wxHtmlHelpWindow;

class WXDLLIMPEXP_HTML wxHtmlHelpFrame : public wxFrame
{
    DECLARE_DYNAMIC_CLASS(wxHtmlHelpFrame)

public:
    wxHtmlHelpFrame(wxHtmlHelpData* data = NULL) { Init(data); }
    wxHtmlHelpFrame(wxWindow* parent, wxWindowID wxWindowID,
                    const wxString& title = wxEmptyString,
                    int style = wxHF_DEFAULT_STYLE, wxHtmlHelpData* data = NULL,
                    wxConfigBase *config=NULL, const wxString& rootpath = wxEmptyString);
    bool Create(wxWindow* parent, wxWindowID id, const wxString& title = wxEmptyString,
                int style = wxHF_DEFAULT_STYLE,
                wxConfigBase *config=NULL, const wxString& rootpath = wxEmptyString);
    virtual ~wxHtmlHelpFrame();

    /// Returns the data associated with the window.
    wxHtmlHelpData* GetData() { return m_Data; }

    /// Returns the help controller associated with the window.
    wxHtmlHelpController* GetController() const { return m_helpController; }

    /// Sets the help controller associated with the window.
    void SetController(wxHtmlHelpController* controller) { m_helpController = controller; }

    /// Returns the help window.
    wxHtmlHelpWindow* GetHelpWindow() const { return m_HtmlHelpWin; }

    // Sets format of title of the frame. Must contain exactly one "%s"
    // (for title of displayed HTML page)
    void SetTitleFormat(const wxString& format);

    // For compatibility
    void UseConfig(wxConfigBase *config, const wxString& rootpath = wxEmptyString);

    // Make the help controller's frame 'modal' if
    // needed
    void AddGrabIfNeeded();

    // Override to add custom buttons to the toolbar
    virtual void AddToolbarButtons(wxToolBar* WXUNUSED(toolBar), int WXUNUSED(style)) {}

    // we don't want to prevent the app from closing just because a help window
    // remains opened
    virtual bool ShouldPreventAppExit() const { return false; }

protected:
    void Init(wxHtmlHelpData* data = NULL);

    void OnCloseWindow(wxCloseEvent& event);
    void OnActivate(wxActivateEvent& event);

#ifdef __WXMAC__
    void OnClose(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
#endif

    // Images:
    enum {
        IMG_Book = 0,
        IMG_Folder,
        IMG_Page
    };

protected:
    wxHtmlHelpData* m_Data;
    bool m_DataCreated;  // m_Data created by frame, or supplied?
    wxString m_TitleFormat;  // title of the help frame
    wxHtmlHelpWindow *m_HtmlHelpWin;
    wxHtmlHelpController* m_helpController;

private:

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxHtmlHelpFrame)
};

#endif // wxUSE_WXHTML_HELP

#endif
