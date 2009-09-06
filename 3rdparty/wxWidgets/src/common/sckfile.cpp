/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/sckfile.cpp
// Purpose:     File protocol
// Author:      Guilhem Lavaux
// Modified by:
// Created:     20/07/97
// RCS-ID:      $Id: sckfile.cpp 43836 2006-12-06 19:20:40Z VZ $
// Copyright:   (c) 1997, 1998 Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_STREAMS && wxUSE_PROTOCOL_FILE

#ifndef WX_PRECOMP
#endif

#include "wx/uri.h"
#include "wx/wfstream.h"
#include "wx/protocol/file.h"

IMPLEMENT_DYNAMIC_CLASS(wxFileProto, wxProtocol)
IMPLEMENT_PROTOCOL(wxFileProto, wxT("file"), NULL, false)

wxFileProto::wxFileProto()
           : wxProtocol()
{
    m_error = wxPROTO_NOERR;
}

wxFileProto::~wxFileProto()
{
}

wxInputStream *wxFileProto::GetInputStream(const wxString& path)
{
    wxFileInputStream *retval = new wxFileInputStream(wxURI::Unescape(path));
    if ( retval->Ok() )
    {
        m_error = wxPROTO_NOERR;

        return retval;
    }

    m_error = wxPROTO_NOFILE;
    delete retval;

    return NULL;
}

#endif // wxUSE_STREAMS && wxUSE_PROTOCOL_FILE
