/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/helpbest.h
// Purpose:     Tries to load MS HTML Help, falls back to wxHTML upon failure
// Author:      Mattia Barbon
// Modified by:
// Created:     02/04/2001
// RCS-ID:      $Id: helpbest.h 39440 2006-05-29 14:51:42Z VZ $
// Copyright:   (c) Mattia Barbon
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HELPBEST_H_
#define _WX_HELPBEST_H_

#if wxUSE_HELP && wxUSE_MS_HTML_HELP \
    && wxUSE_WXHTML_HELP && !defined(__WXUNIVERSAL__)

#include "wx/helpbase.h"
#include "wx/html/helpfrm.h"        // for wxHF_DEFAULT_STYLE

class WXDLLIMPEXP_HTML wxBestHelpController: public wxHelpControllerBase
{
public:
    wxBestHelpController(wxWindow* parentWindow = NULL,
                         int style = wxHF_DEFAULT_STYLE)
        : wxHelpControllerBase(parentWindow),
          m_helpControllerType(wxUseNone),
          m_helpController(NULL),
          m_style(style)
    {
    }

    virtual ~wxBestHelpController() { delete m_helpController; }

    // Must call this to set the filename
    virtual bool Initialize(const wxString& file);
    virtual bool Initialize(const wxString& file, int WXUNUSED(server) ) { return Initialize( file ); }

    // If file is "", reloads file given in Initialize
    virtual bool LoadFile(const wxString& file = wxEmptyString)
    {
        return m_helpController->LoadFile( GetValidFilename( file ) );
    }

    virtual bool DisplayContents()
    {
        return m_helpController->DisplayContents();
    }

    virtual bool DisplaySection(int sectionNo)
    {
        return m_helpController->DisplaySection( sectionNo );
    }

    virtual bool DisplaySection(const wxString& section)
    {
        return m_helpController->DisplaySection( section );
    }

    virtual bool DisplayBlock(long blockNo)
    {
        return m_helpController->DisplayBlock( blockNo );
    }

    virtual bool DisplayContextPopup(int contextId)
    {
        return m_helpController->DisplayContextPopup( contextId );
    }

    virtual bool DisplayTextPopup(const wxString& text, const wxPoint& pos)
    {
        return m_helpController->DisplayTextPopup( text, pos );
    }

    virtual bool KeywordSearch(const wxString& k,
                               wxHelpSearchMode mode = wxHELP_SEARCH_ALL)
    {
        return m_helpController->KeywordSearch( k, mode );
    }

    virtual bool Quit()
    {
        return m_helpController->Quit();
    }

    // Allows one to override the default settings for the help frame.
    virtual void SetFrameParameters(const wxString& title,
                                    const wxSize& size,
                                    const wxPoint& pos = wxDefaultPosition,
                                    bool newFrameEachTime = false)
    {
        m_helpController->SetFrameParameters( title, size, pos,
                                              newFrameEachTime );
    }

    // Obtains the latest settings used by the help frame and the help frame.
    virtual wxFrame *GetFrameParameters(wxSize *size = NULL,
                                        wxPoint *pos = NULL,
                                        bool *newFrameEachTime = NULL)
    {
        return m_helpController->GetFrameParameters( size, pos,
                                                     newFrameEachTime );
    }

    /// Set the window that can optionally be used for the help window's parent.
    virtual void SetParentWindow(wxWindow* win) { m_helpController->SetParentWindow(win); }

    /// Get the window that can optionally be used for the help window's parent.
    virtual wxWindow* GetParentWindow() const { return m_helpController->GetParentWindow(); }

protected:
    // Append/change extension if necessary.
    wxString GetValidFilename(const wxString& file) const;

protected:
    enum HelpControllerType { wxUseNone, wxUseHtmlHelp, wxUseChmHelp };

    HelpControllerType m_helpControllerType;
    wxHelpControllerBase* m_helpController;
    int m_style;

    DECLARE_DYNAMIC_CLASS(wxBestHelpController)
    DECLARE_NO_COPY_CLASS(wxBestHelpController)
};

#endif // wxUSE_HELP && wxUSE_MS_HTML_HELP && wxUSE_WXHTML_HELP

#endif
    // _WX_HELPBEST_H_
