/////////////////////////////////////////////////////////////////////////////
// Name:        helpchm.h
// Purpose:     Help system: MS HTML Help implementation
// Author:      Julian Smart
// Modified by:
// Created:     16/04/2000
// RCS-ID:      $Id: helpchm.h 39725 2006-06-14 17:46:51Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HELPCHM_H_
#define _WX_HELPCHM_H_

#if wxUSE_MS_HTML_HELP

#include "wx/helpbase.h"

class WXDLLEXPORT wxCHMHelpController : public wxHelpControllerBase
{
public:
    wxCHMHelpController(wxWindow* parentWindow = NULL): wxHelpControllerBase(parentWindow) { }

    // Must call this to set the filename
    virtual bool Initialize(const wxString& file);
    virtual bool Initialize(const wxString& file, int WXUNUSED(server) ) { return Initialize( file ); }

    // If file is "", reloads file given in Initialize
    virtual bool LoadFile(const wxString& file = wxEmptyString);
    virtual bool DisplayContents();
    virtual bool DisplaySection(int sectionNo);
    virtual bool DisplaySection(const wxString& section);
    virtual bool DisplayBlock(long blockNo);
    virtual bool DisplayContextPopup(int contextId);
    virtual bool DisplayTextPopup(const wxString& text, const wxPoint& pos);
    virtual bool KeywordSearch(const wxString& k,
                               wxHelpSearchMode mode = wxHELP_SEARCH_ALL);
    virtual bool Quit();

    wxString GetHelpFile() const { return m_helpFile; }

    // helper of DisplayTextPopup(), also used in wxSimpleHelpProvider::ShowHelp
    static bool ShowContextHelpPopup(const wxString& text,
                                     const wxPoint& pos,
                                     wxWindow *window);

protected:
    // Append extension if necessary.
    wxString GetValidFilename(const wxString& file) const;

protected:
    wxString m_helpFile;

    DECLARE_CLASS(wxCHMHelpController)
};

#endif // wxUSE_MS_HTML_HELP

#endif
// _WX_HELPCHM_H_
