/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/msgout.cpp
// Purpose:     wxMessageOutput implementation
// Author:      Mattia Barbon
// Modified by:
// Created:     17.07.02
// RCS-ID:      $Id: msgout.cpp 38920 2006-04-26 08:21:31Z ABX $
// Copyright:   (c) the wxWidgets team
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

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/ffile.h"
    #include "wx/app.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #if wxUSE_GUI
        #include "wx/msgdlg.h"
    #endif // wxUSE_GUI
#endif

#include "wx/msgout.h"
#include "wx/apptrait.h"
#include <stdarg.h>
#include <stdio.h>

#if defined(__WINDOWS__)
    #include "wx/msw/private.h"
#endif
#ifdef __WXMAC__
    #include "wx/mac/private.h"
#endif

// ===========================================================================
// implementation
// ===========================================================================

#if wxUSE_BASE

// ----------------------------------------------------------------------------
// wxMessageOutput
// ----------------------------------------------------------------------------

wxMessageOutput* wxMessageOutput::ms_msgOut = 0;

wxMessageOutput* wxMessageOutput::Get()
{
    if ( !ms_msgOut && wxTheApp )
    {
        ms_msgOut = wxTheApp->GetTraits()->CreateMessageOutput();
    }

    return ms_msgOut;
}

wxMessageOutput* wxMessageOutput::Set(wxMessageOutput* msgout)
{
    wxMessageOutput* old = ms_msgOut;
    ms_msgOut = msgout;
    return old;
}

// ----------------------------------------------------------------------------
// wxMessageOutputBest
// ----------------------------------------------------------------------------

#ifdef __WINDOWS__

// check if we're running in a console under Windows
static inline bool IsInConsole()
{
#ifdef __WXWINCE__
    return false;
#else // !__WXWINCE__
    HANDLE hStdErr = ::GetStdHandle(STD_ERROR_HANDLE);
    return hStdErr && hStdErr != INVALID_HANDLE_VALUE;
#endif // __WXWINCE__/!__WXWINCE__
}

#endif // __WINDOWS__

void wxMessageOutputBest::Printf(const wxChar* format, ...)
{
    va_list args;
    va_start(args, format);
    wxString out;

    out.PrintfV(format, args);
    va_end(args);

#ifdef __WINDOWS__
    if ( !IsInConsole() )
    {
        ::MessageBox(NULL, out, _T("wxWidgets"), MB_ICONINFORMATION | MB_OK);
    }
    else
#endif // __WINDOWS__/!__WINDOWS__
    {
        fprintf(stderr, "%s", (const char*) out.mb_str());
    }
}

// ----------------------------------------------------------------------------
// wxMessageOutputStderr
// ----------------------------------------------------------------------------

void wxMessageOutputStderr::Printf(const wxChar* format, ...)
{
    va_list args;
    va_start(args, format);
    wxString out;

    out.PrintfV(format, args);
    va_end(args);

    fprintf(stderr, "%s", (const char*) out.mb_str());
}

// ----------------------------------------------------------------------------
// wxMessageOutputDebug
// ----------------------------------------------------------------------------

void wxMessageOutputDebug::Printf(const wxChar* format, ...)
{
    wxString out;

    va_list args;
    va_start(args, format);

    out.PrintfV(format, args);
    va_end(args);

#if defined(__WXMSW__) && !defined(__WXMICROWIN__)
    out.Replace(wxT("\t"), wxT("        "));
    out.Replace(wxT("\n"), wxT("\r\n"));
    ::OutputDebugString(out);
#elif defined(__WXMAC__) && !defined(__DARWIN__)
    if ( wxIsDebuggerRunning() )
    {
        Str255 pstr;
        wxString output = out + wxT(";g") ;
        wxMacStringToPascal(output.c_str(), pstr);

        #ifdef __powerc
            DebugStr(pstr);
        #else
            SysBreakStr(pstr);
        #endif
    }
#else
    wxFputs( out , stderr ) ;
    if ( out.Right(1) != wxT("\n") )
        wxFputs( wxT("\n") , stderr ) ;
    fflush( stderr ) ;
#endif // platform
}

// ----------------------------------------------------------------------------
// wxMessageOutputLog
// ----------------------------------------------------------------------------

void wxMessageOutputLog::Printf(const wxChar* format, ...)
{
    wxString out;

    va_list args;
    va_start(args, format);

    out.PrintfV(format, args);
    va_end(args);

    out.Replace(wxT("\t"), wxT("        "));

    ::wxLogMessage(wxT("%s"), out.c_str());
}

#endif // wxUSE_BASE

// ----------------------------------------------------------------------------
// wxMessageOutputMessageBox
// ----------------------------------------------------------------------------

#if wxUSE_GUI

void wxMessageOutputMessageBox::Printf(const wxChar* format, ...)
{
    va_list args;
    va_start(args, format);
    wxString out;

    out.PrintfV(format, args);
    va_end(args);

    // the native MSW msg box understands the TABs, others don't
#ifndef __WXMSW__
    out.Replace(wxT("\t"), wxT("        "));
#endif

    wxString title;
    if ( wxTheApp )
        title.Printf(_("%s message"), wxTheApp->GetAppName().c_str());

    ::wxMessageBox(out, title);
}

#endif // wxUSE_GUI
