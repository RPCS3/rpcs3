/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/sysopt.cpp
// Purpose:     wxSystemOptions
// Author:      Julian Smart
// Modified by:
// Created:     2001-07-10
// RCS-ID:      $Id: sysopt.cpp 39851 2006-06-27 14:33:14Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if defined(__BORLANDC__)
    #pragma hdrstop
#endif

#if wxUSE_SYSTEM_OPTIONS

#include "wx/sysopt.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/list.h"
    #include "wx/string.h"
    #include "wx/arrstr.h"
#endif

// ----------------------------------------------------------------------------
// private globals
// ----------------------------------------------------------------------------

static wxArrayString gs_optionNames,
                     gs_optionValues;

// ============================================================================
// wxSystemOptions implementation
// ============================================================================

// Option functions (arbitrary name/value mapping)
void wxSystemOptions::SetOption(const wxString& name, const wxString& value)
{
    int idx = gs_optionNames.Index(name, false);
    if (idx == wxNOT_FOUND)
    {
        gs_optionNames.Add(name);
        gs_optionValues.Add(value);
    }
    else
    {
        gs_optionNames[idx] = name;
        gs_optionValues[idx] = value;
    }
}

void wxSystemOptions::SetOption(const wxString& name, int value)
{
    SetOption(name, wxString::Format(wxT("%d"), value));
}

wxString wxSystemOptions::GetOption(const wxString& name)
{
    wxString val;

    int idx = gs_optionNames.Index(name, false);
    if ( idx != wxNOT_FOUND )
    {
        val = gs_optionValues[idx];
    }
    else // not set explicitely
    {
        // look in the environment: first for a variable named "wx_appname_name"
        // which can be set to affect the behaviour or just this application
        // and then for "wx_name" which can be set to change the option globally
        wxString var(name);
        var.Replace(_T("."), _T("_"));  // '.'s not allowed in env var names

        wxString appname;
        if ( wxTheApp )
            appname = wxTheApp->GetAppName();

        if ( !appname.empty() )
            val = wxGetenv(_T("wx_") + appname + _T('_') + var);

        if ( val.empty() )
            val = wxGetenv(_T("wx_") + var);
    }

    return val;
}

int wxSystemOptions::GetOptionInt(const wxString& name)
{
    return wxAtoi(GetOption(name));
}

bool wxSystemOptions::HasOption(const wxString& name)
{
    return !GetOption(name).empty();
}

#endif // wxUSE_SYSTEM_OPTIONS
