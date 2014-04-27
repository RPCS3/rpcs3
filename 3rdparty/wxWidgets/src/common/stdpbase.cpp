///////////////////////////////////////////////////////////////////////////////
// Name:        common/stdpbase.cpp
// Purpose:     wxStandardPathsBase methods common to all ports
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-10-19
// RCS-ID:      $Id: stdpbase.cpp 53093 2008-04-08 13:51:17Z JS $
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// License:     wxWindows license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_STDPATHS

#ifndef WX_PRECOMP
    #include "wx/app.h"
#endif //WX_PRECOMP
#include "wx/apptrait.h"

#include "wx/filename.h"
#include "wx/stdpaths.h"

#if defined(__UNIX__) && !defined(__WXMAC__)
#include "wx/log.h"
#include "wx/textfile.h"
#endif

// ----------------------------------------------------------------------------
// module globals
// ----------------------------------------------------------------------------

static wxStandardPaths gs_stdPaths;

// ============================================================================
// implementation
// ============================================================================

/* static */
wxStandardPathsBase& wxStandardPathsBase::Get()
{
    wxAppTraits * const traits = wxTheApp ? wxTheApp->GetTraits() : NULL;
    wxCHECK_MSG( traits, gs_stdPaths, _T("create wxApp before calling this") );

    return traits->GetStandardPaths();
}

wxString wxStandardPathsBase::GetExecutablePath() const
{
    if ( !wxTheApp || !wxTheApp->argv )
        return wxEmptyString;

    wxString argv0 = wxTheApp->argv[0];
    if (wxIsAbsolutePath(argv0))
        return argv0;

    // Search PATH.environment variable...
    wxPathList pathlist;
    pathlist.AddEnvList(wxT("PATH"));
    wxString path = pathlist.FindAbsoluteValidPath(argv0);
    if ( path.empty() )
        return argv0;       // better than nothing

    wxFileName filename(path);
    filename.Normalize();
    return filename.GetFullPath();
}

wxStandardPathsBase& wxAppTraitsBase::GetStandardPaths()
{
    return gs_stdPaths;
}

wxStandardPathsBase::~wxStandardPathsBase()
{
    // nothing to do here
}

wxString wxStandardPathsBase::GetLocalDataDir() const
{
    return GetDataDir();
}

wxString wxStandardPathsBase::GetUserLocalDataDir() const
{
    return GetUserDataDir();
}

wxString wxStandardPathsBase::GetDocumentsDir() const
{
#if defined(__UNIX__) && !defined(__WXMAC__)
    {
        wxLogNull logNull;
        wxString homeDir = wxFileName::GetHomeDir();
        wxString configPath;
        if (wxGetenv(wxT("XDG_CONFIG_HOME")))
            configPath = wxGetenv(wxT("XDG_CONFIG_HOME"));
        else
            configPath = homeDir + wxT("/.config");
        wxString dirsFile = configPath + wxT("/user-dirs.dirs");
        if (wxFileExists(dirsFile))
        {
            wxTextFile textFile;
            if (textFile.Open(dirsFile))
            {
                size_t i;
                for (i = 0; i < textFile.GetLineCount(); i++)
                {
                    wxString line(textFile[i]);
                    int pos = line.Find(wxT("XDG_DOCUMENTS_DIR"));
                    if (pos != wxNOT_FOUND)
                    {
                        wxString value = line.AfterFirst(wxT('='));
                        value.Replace(wxT("$HOME"), homeDir);
                        value.Trim(true);
                        value.Trim(false);
                        if (!value.IsEmpty() && wxDirExists(value))
                            return value;
                        else
                            break;
                    }
                }
            }
        }
    }
#endif

    return wxFileName::GetHomeDir();
}

// return the temporary directory for the current user
wxString wxStandardPathsBase::GetTempDir() const
{
    return wxFileName::GetTempDir();
}

/* static */
wxString wxStandardPathsBase::AppendAppName(const wxString& dir)
{
    wxString subdir(dir);

    // empty string indicates that an error has occurred, don't touch it then
    if ( !subdir.empty() )
    {
        const wxString appname = wxTheApp->GetAppName();
        if ( !appname.empty() )
        {
            const wxChar ch = *(subdir.end() - 1);
            if ( !wxFileName::IsPathSeparator(ch) && ch != _T('.') )
                subdir += wxFileName::GetPathSeparator();

            subdir += appname;
        }
    }

    return subdir;
}

#endif // wxUSE_STDPATHS
