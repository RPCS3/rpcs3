/////////////////////////////////////////////////////////////////////////////
// Name:        helpbase.h
// Purpose:     Help system base classes
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: helpbase.h 45498 2007-04-16 13:03:05Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HELPBASEH__
#define _WX_HELPBASEH__

#include "wx/defs.h"

#if wxUSE_HELP

#include "wx/object.h"
#include "wx/string.h"
#include "wx/gdicmn.h"
#include "wx/frame.h"

// Flags for SetViewer
#define wxHELP_NETSCAPE     1

// Search modes:
enum wxHelpSearchMode
{
    wxHELP_SEARCH_INDEX,
    wxHELP_SEARCH_ALL
};

// Defines the API for help controllers
class WXDLLEXPORT wxHelpControllerBase: public wxObject
{
public:
    inline wxHelpControllerBase(wxWindow* parentWindow = NULL) { m_parentWindow = parentWindow; }
    inline ~wxHelpControllerBase() {}

    // Must call this to set the filename and server name.
    // server is only required when implementing TCP/IP-based
    // help controllers.
    virtual bool Initialize(const wxString& WXUNUSED(file), int WXUNUSED(server) ) { return false; }
    virtual bool Initialize(const wxString& WXUNUSED(file)) { return false; }

    // Set viewer: only relevant to some kinds of controller
    virtual void SetViewer(const wxString& WXUNUSED(viewer), long WXUNUSED(flags) = 0) {}

    // If file is "", reloads file given  in Initialize
    virtual bool LoadFile(const wxString& file = wxEmptyString) = 0;

    // Displays the contents
    virtual bool DisplayContents(void) = 0;

    // Display the given section
    virtual bool DisplaySection(int sectionNo) = 0;

    // Display the section using a context id
    virtual bool DisplayContextPopup(int WXUNUSED(contextId)) { return false; }

    // Display the text in a popup, if possible
    virtual bool DisplayTextPopup(const wxString& WXUNUSED(text), const wxPoint& WXUNUSED(pos)) { return false; }

    // By default, uses KeywordSection to display a topic. Implementations
    // may override this for more specific behaviour.
    virtual bool DisplaySection(const wxString& section) { return KeywordSearch(section); }
    virtual bool DisplayBlock(long blockNo) = 0;
    virtual bool KeywordSearch(const wxString& k,
                               wxHelpSearchMode mode = wxHELP_SEARCH_ALL) = 0;
    /// Allows one to override the default settings for the help frame.
    virtual void SetFrameParameters(const wxString& WXUNUSED(title),
        const wxSize& WXUNUSED(size),
        const wxPoint& WXUNUSED(pos) = wxDefaultPosition,
        bool WXUNUSED(newFrameEachTime) = false)
    {
        // does nothing by default
    }
    /// Obtains the latest settings used by the help frame and the help
    /// frame.
    virtual wxFrame *GetFrameParameters(wxSize *WXUNUSED(size) = NULL,
        wxPoint *WXUNUSED(pos) = NULL,
        bool *WXUNUSED(newFrameEachTime) = NULL)
    {
        return (wxFrame*) NULL; // does nothing by default
    }

    virtual bool Quit() = 0;
    virtual void OnQuit() {}

    /// Set the window that can optionally be used for the help window's parent.
    virtual void SetParentWindow(wxWindow* win) { m_parentWindow = win; }

    /// Get the window that can optionally be used for the help window's parent.
    virtual wxWindow* GetParentWindow() const { return m_parentWindow; }

protected:
    wxWindow* m_parentWindow;
private:
    DECLARE_CLASS(wxHelpControllerBase)
};

#endif // wxUSE_HELP

#endif
// _WX_HELPBASEH__
