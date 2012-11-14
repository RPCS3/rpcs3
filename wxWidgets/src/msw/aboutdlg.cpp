///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/aboutdlg.cpp
// Purpose:     implementation of wxAboutBox() for wxMSW
// Author:      Vadim Zeitlin
// Created:     2006-10-07
// RCS-ID:      $Id: aboutdlg.cpp 58748 2009-02-08 09:46:03Z VZ $
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
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

#if wxUSE_ABOUTDLG

#ifndef WX_PRECOMP
    #include "wx/msgdlg.h"
#endif //WX_PRECOMP

#include "wx/aboutdlg.h"
#include "wx/generic/aboutdlgg.h"

// ============================================================================
// implementation
// ============================================================================

// our public entry point
void wxAboutBox(const wxAboutDialogInfo& info)
{
    // we prefer to show a simple message box if we don't have any fields which
    // can't be shown in it because as much as there is a standard about box
    // under MSW at all, this is it
    if ( info.IsSimple() )
    {
        // build the text to show in the box
        const wxString name = info.GetName();
        wxString msg;
        msg << name;
        if ( info.HasVersion() )
            msg << _(" Version ") << info.GetVersion();
        msg << _T('\n');

        if ( info.HasCopyright() )
            msg << info.GetCopyrightToDisplay() << _T('\n');

        // add everything remaining
        msg << info.GetDescriptionAndCredits();

        wxMessageBox(msg, _("About ") + name);
    }
    else // simple "native" version is not enough
    {
        // we need to use the full-blown generic version
        wxGenericAboutBox(info);
    }
}

#endif // wxUSE_ABOUTDLG
