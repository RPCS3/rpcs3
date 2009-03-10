/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/wince/filedlgwce.cpp
// Purpose:     wxFileDialog implementation for smart phones driven by WinCE
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: filedlgwce.cpp 39651 2006-06-09 17:50:46Z ABX $
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

// Only use this for MS SmartPhone. Use standard file dialog
// for Pocket PC.

#if wxUSE_FILEDLG && defined(__SMARTPHONE__) && defined(__WXWINCE__)

#include "wx/filedlg.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/msgdlg.h"
    #include "wx/dialog.h"
    #include "wx/filefn.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/app.h"
#endif

#include "wx/msw/private.h"

#include <stdlib.h>
#include <string.h>

#include "wx/filename.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

IMPLEMENT_CLASS(wxFileDialog, wxDialog)

// ----------------------------------------------------------------------------
// wxFileDialog
// ----------------------------------------------------------------------------

wxFileDialog::wxFileDialog(wxWindow *parent,
                           const wxString& message,
                           const wxString& defaultDir,
                           const wxString& defaultFileName,
                           const wxString& wildCard,
                           long style,
                           const wxPoint& WXUNUSED(pos),
                           const wxSize& WXUNUSED(sz),
                           const wxString& WXUNUSED(name))
{
    m_message = message;
    m_windowStyle = style;
    if ( ( m_windowStyle & wxFD_MULTIPLE ) && ( m_windowStyle & wxFD_SAVE ) )
        m_windowStyle &= ~wxFD_MULTIPLE;
    m_parent = parent;
    m_path = wxEmptyString;
    m_fileName = defaultFileName;
    m_dir = defaultDir;
    m_wildCard = wildCard;
    m_filterIndex = 0;
}

void wxFileDialog::GetPaths(wxArrayString& paths) const
{
    paths.Empty();

    wxString dir(m_dir);
    if ( m_dir.Last() != _T('\\') )
        dir += _T('\\');

    size_t count = m_fileNames.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        if (wxFileName(m_fileNames[n]).IsAbsolute())
            paths.Add(m_fileNames[n]);
        else
            paths.Add(dir + m_fileNames[n]);
    }
}

void wxFileDialog::SetPath(const wxString& path)
{
    wxString ext;
    wxSplitPath(path, &m_dir, &m_fileName, &ext);
    if ( !ext.empty() )
        m_fileName << _T('.') << ext;
}

int wxFileDialog::ShowModal()
{
    wxWindow* parentWindow = GetParent();
    if (!parentWindow)
        parentWindow = wxTheApp->GetTopWindow();

    wxString str = wxGetTextFromUser(m_message, _("File"), m_fileName, parentWindow);
    if (str.empty())
        return wxID_CANCEL;

    m_fileName = str;
    m_fileNames.Add(str);
    return wxID_OK;
}

void wxFileDialog::GetFilenames(wxArrayString& files) const
{
    files = m_fileNames;
}

#endif // wxUSE_FILEDLG && __SMARTPHONE__ && __WXWINCE__
