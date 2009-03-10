/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/helpbest.cpp
// Purpose:     Tries to load MS HTML Help, falls back to wxHTML upon failure
// Author:      Mattia Barbon
// Modified by:
// Created:     02/04/2001
// RCS-ID:      $Id: helpbest.cpp 39398 2006-05-28 23:07:02Z VZ $
// Copyright:   (c) Mattia Barbon
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/log.h"
#endif

#include "wx/filefn.h"

#if wxUSE_HELP && wxUSE_MS_HTML_HELP \
    && wxUSE_WXHTML_HELP && !defined(__WXUNIVERSAL__)

#include "wx/msw/helpchm.h"
#include "wx/html/helpctrl.h"
#include "wx/msw/helpbest.h"

IMPLEMENT_DYNAMIC_CLASS( wxBestHelpController, wxHelpControllerBase )

bool wxBestHelpController::Initialize( const wxString& filename )
{
    // try wxCHMHelpController
    wxCHMHelpController* chm = new wxCHMHelpController(m_parentWindow);

    m_helpControllerType = wxUseChmHelp;
    // do not warn upon failure
    wxLogNull dontWarnOnFailure;

    if( chm->Initialize( GetValidFilename( filename ) ) )
    {
        m_helpController = chm;
        m_parentWindow = NULL;
        return true;
    }

    // failed
    delete chm;

    // try wxHtmlHelpController
    wxHtmlHelpController *
        html = new wxHtmlHelpController(m_style, m_parentWindow);

    m_helpControllerType = wxUseHtmlHelp;
    if( html->Initialize( GetValidFilename( filename ) ) )
    {
        m_helpController = html;
        m_parentWindow = NULL;
        return true;
    }

    // failed
    delete html;

    return false;
}

wxString wxBestHelpController::GetValidFilename( const wxString& filename ) const
{
    wxString tmp = filename;
    ::wxStripExtension( tmp );

    switch( m_helpControllerType )
    {
        case wxUseChmHelp:
            if( ::wxFileExists( tmp + wxT(".chm") ) )
                return tmp + wxT(".chm");

            return filename;

        case wxUseHtmlHelp:
            if( ::wxFileExists( tmp + wxT(".htb") ) )
                return tmp + wxT(".htb");
            if( ::wxFileExists( tmp + wxT(".zip") ) )
                return tmp + wxT(".zip");
            if( ::wxFileExists( tmp + wxT(".hhp") ) )
                return tmp + wxT(".hhp");

            return filename;

        default:
            // we CAN'T get here
            wxFAIL_MSG( wxT("wxBestHelpController: Must call Initialize, first!") );
    }

    return wxEmptyString;
}

#endif
    // wxUSE_HELP && wxUSE_MS_HTML_HELP && wxUSE_WXHTML_HELP
