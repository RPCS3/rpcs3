///////////////////////////////////////////////////////////////////////////////
// Name:        unix/stdpaths.cpp
// Purpose:     wxStandardPaths implementation for Unix & OpenVMS systems
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-10-19
// RCS-ID:      $Id: stdpaths.cpp 43443 2006-11-16 13:31:40Z JJ $
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

#include "wx/stdpaths.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
#endif //WX_PRECOMP

#include "wx/filename.h"

#if defined( __LINUX__ ) || defined( __VMS )
    #include <unistd.h>
#endif

// ============================================================================
// common VMS/Unix part of wxStandardPaths implementation
// ============================================================================

void wxStandardPaths::SetInstallPrefix(const wxString& prefix)
{
    m_prefix = prefix;
}

wxString wxStandardPaths::GetUserConfigDir() const
{
    return wxFileName::GetHomeDir();
}

// ============================================================================
// wxStandardPaths implementation for VMS
// ============================================================================

#ifdef __VMS

wxString wxStandardPaths::GetInstallPrefix() const
{
    if ( m_prefix.empty() )
    {
        wx_const_cast(wxStandardPaths *, this)->m_prefix = wxT("/sys$system");
    }

    return m_prefix;
}

wxString wxStandardPaths::GetConfigDir() const
{
   return _T("/sys$manager");
}

wxString wxStandardPaths::GetDataDir() const
{
   return AppendAppName(GetInstallPrefix() + _T("/sys$share"));
}

wxString wxStandardPaths::GetLocalDataDir() const
{
   return AppendAppName(_T("/sys$manager"));
}

wxString wxStandardPaths::GetUserDataDir() const
{
   return wxFileName::GetHomeDir();
}

wxString wxStandardPaths::GetPluginsDir() const
{
    return wxString(); // TODO: this is wrong, it should return something
}

wxString
wxStandardPaths::GetLocalizedResourcesDir(const wxChar *lang,
                                          ResourceCat category) const
{
    return wxStandardPathsBase::GetLocalizedResourcesDir(lang, category);
}

wxString wxStandardPaths::GetExecutablePath() const
{
    return wxStandardPathsBase::GetExecutablePath();
}

#else // !__VMS

// ============================================================================
// wxStandardPaths implementation for Unix
// ============================================================================

wxString wxStandardPaths::GetExecutablePath() const
{
#ifdef __LINUX__
    wxString exeStr;

    char buf[4096];
    int result = readlink("/proc/self/exe", buf, WXSIZEOF(buf) - sizeof(char));
    if ( result != -1 )
    {
        buf[result] = '\0'; // readlink() doesn't NUL-terminate the buffer

        // if the /proc/self/exe symlink has been dropped by the kernel for
        // some reason, then readlink() could also return success but
        // "(deleted)" as link destination...
        if ( strcmp(buf, "(deleted)") != 0 )
            exeStr = wxString(buf, wxConvLibc);
    }

    if ( exeStr.empty() )
    {
        // UPX-specific hack: when using UPX on linux, the kernel will drop the
        // /proc/self/exe link; in this case we try to look for a special
        // environment variable called "   " which is created by UPX to save
        // /proc/self/exe contents. See
        //      http://sf.net/tracker/?func=detail&atid=309863&aid=1565357&group_id=9863
        // for more information about this issue.
        wxGetEnv(wxT("   "), &exeStr);
    }

    if ( !exeStr.empty() )
        return exeStr;
#endif // __LINUX__

    return wxStandardPathsBase::GetExecutablePath();
}

void wxStandardPaths::DetectPrefix()
{
    // we can try to infer the prefix from the location of the executable
    wxString exeStr = GetExecutablePath();
    if ( !exeStr.empty() )
    {
        // consider that we're in the last "bin" subdirectory of our prefix
        size_t pos = exeStr.rfind(wxT("/bin/"));
        if ( pos != wxString::npos )
            m_prefix.assign(exeStr, 0, pos);
    }

    if ( m_prefix.empty() )
    {
        m_prefix = wxT("/usr/local");
    }
}

wxString wxStandardPaths::GetInstallPrefix() const
{
    if ( m_prefix.empty() )
    {
        wxStandardPaths *pathPtr = wx_const_cast(wxStandardPaths *, this);
        pathPtr->DetectPrefix();
    }

    return m_prefix;
}

// ----------------------------------------------------------------------------
// public functions
// ----------------------------------------------------------------------------

wxString wxStandardPaths::GetConfigDir() const
{
   return _T("/etc");
}

wxString wxStandardPaths::GetDataDir() const
{
   return AppendAppName(GetInstallPrefix() + _T("/share"));
}

wxString wxStandardPaths::GetLocalDataDir() const
{
   return AppendAppName(_T("/etc"));
}

wxString wxStandardPaths::GetUserDataDir() const
{
   return AppendAppName(wxFileName::GetHomeDir() + _T("/."));
}

wxString wxStandardPaths::GetPluginsDir() const
{
    return AppendAppName(GetInstallPrefix() + _T("/lib"));
}

wxString
wxStandardPaths::GetLocalizedResourcesDir(const wxChar *lang,
                                          ResourceCat category) const
{
    if ( category != ResourceCat_Messages )
        return wxStandardPathsBase::GetLocalizedResourcesDir(lang, category);

    return GetInstallPrefix() + _T("/share/locale/") + lang + _T("/LC_MESSAGES");
}

#endif // __VMS/!__VMS

#endif // wxUSE_STDPATHS
