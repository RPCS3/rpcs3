/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/main.cpp
// Purpose:     WinMain/DllMain
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: main.cpp 44727 2007-03-10 17:24:09Z VZ $
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

#ifndef WX_PRECOMP
    #include "wx/event.h"
    #include "wx/app.h"
    #include "wx/utils.h"
#endif //WX_PRECOMP

#include "wx/cmdline.h"
#include "wx/scopeguard.h"

#include "wx/msw/private.h"
#include "wx/msw/seh.h"

#if wxUSE_ON_FATAL_EXCEPTION
    #include "wx/datetime.h"
    #include "wx/msw/crashrpt.h"
#endif // wxUSE_ON_FATAL_EXCEPTION

#ifdef __WXWINCE__
    // there is no ExitProcess() under CE but exiting the main thread has the
    // same effect
    #ifndef ExitProcess
        #define ExitProcess ExitThread
    #endif
#endif // __WXWINCE__

#ifdef __BORLANDC__
    // BC++ has to be special: its run-time expects the DLL entry point to be
    // named DllEntryPoint instead of the (more) standard DllMain
    #define DllMain DllEntryPoint
#endif // __BORLANDC__

#if defined(__WXMICROWIN__)
    #define HINSTANCE HANDLE
#endif

// defined in common/init.cpp
extern int wxEntryReal(int& argc, wxChar **argv);

// ============================================================================
// implementation: various entry points
// ============================================================================

#if wxUSE_BASE

#if wxUSE_ON_FATAL_EXCEPTION && defined(__VISUALC__) && !defined(__WXWINCE__)
    // VC++ (at least from 4.0 up to version 7.1) is incredibly broken in that
    // a "catch ( ... )" will *always* catch SEH exceptions in it even though
    // it should have never been the case... to prevent such catches from
    // stealing the exceptions from our wxGlobalSEHandler which is only called
    // if the exception is not handled elsewhere, we have to also call it from
    // a special SEH translator function which is called by VC CRT when a Win32
    // exception occurs

    // this warns that /EHa (async exceptions) should be used when using
    // _set_se_translator but, in fact, this doesn't seem to change anything
    // with VC++ up to 8.0
    #if _MSC_VER <= 1400
        #pragma warning(disable:4535)
    #endif

    // note that the SE translator must be called wxSETranslator!
    #define DisableAutomaticSETranslator() _set_se_translator(wxSETranslator)
#else // !__VISUALC__
    #define DisableAutomaticSETranslator()
#endif // __VISUALC__/!__VISUALC__

// ----------------------------------------------------------------------------
// wrapper wxEntry catching all Win32 exceptions occurring in a wx program
// ----------------------------------------------------------------------------

// wrap real wxEntry in a try-except block to be able to call
// OnFatalException() if necessary
#if wxUSE_ON_FATAL_EXCEPTION

// global pointer to exception information, only valid inside OnFatalException,
// used by wxStackWalker and wxCrashReport
extern EXCEPTION_POINTERS *wxGlobalSEInformation = NULL;

// flag telling us whether the application wants to handle exceptions at all
static bool gs_handleExceptions = false;

static void wxFatalExit()
{
    // use the same exit code as abort()
    ::ExitProcess(3);
}

unsigned long wxGlobalSEHandler(EXCEPTION_POINTERS *pExcPtrs)
{
    if ( gs_handleExceptions && wxTheApp )
    {
        // store the pointer to exception info
        wxGlobalSEInformation = pExcPtrs;

        // give the user a chance to do something special about this
        wxSEH_TRY
        {
            wxTheApp->OnFatalException();
        }
        wxSEH_IGNORE      // ignore any exceptions inside the exception handler

        wxGlobalSEInformation = NULL;

        // this will execute our handler and terminate the process
        return EXCEPTION_EXECUTE_HANDLER;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

#ifdef __VISUALC__

void wxSETranslator(unsigned int WXUNUSED(code), EXCEPTION_POINTERS *ep)
{
    switch ( wxGlobalSEHandler(ep) )
    {
        default:
            wxFAIL_MSG( _T("unexpected wxGlobalSEHandler() return value") );
            // fall through

        case EXCEPTION_EXECUTE_HANDLER:
            // if wxApp::OnFatalException() had been called we should exit the
            // application -- but we shouldn't kill our host when we're a DLL
#ifndef WXMAKINGDLL
            wxFatalExit();
#endif // not a DLL
            break;

        case EXCEPTION_CONTINUE_SEARCH:
            // we're called for each "catch ( ... )" and if we (re)throw from
            // here, the catch handler body is not executed, so the effect is
            // as if had inhibited translation of SE to C++ ones because the
            // handler will never see any structured exceptions
            throw;
    }
}

#endif // __VISUALC__

bool wxHandleFatalExceptions(bool doit)
{
    // assume this can only be called from the main thread
    gs_handleExceptions = doit;

#if wxUSE_CRASHREPORT
    if ( doit )
    {
        // try to find a place where we can put out report file later
        wxChar fullname[MAX_PATH];
        if ( !::GetTempPath(WXSIZEOF(fullname), fullname) )
        {
            wxLogLastError(_T("GetTempPath"));

            // when all else fails...
            wxStrcpy(fullname, _T("c:\\"));
        }

        // use PID and date to make the report file name more unique
        wxString name = wxString::Format
                        (
                            _T("%s_%s_%lu.dmp"),
                            wxTheApp ? wxTheApp->GetAppName().c_str()
                                     : _T("wxwindows"),
                            wxDateTime::Now().Format(_T("%Y%m%dT%H%M%S")).c_str(),
                            ::GetCurrentProcessId()
                        );

        wxStrncat(fullname, name, WXSIZEOF(fullname) - wxStrlen(fullname) - 1);

        wxCrashReport::SetFileName(fullname);
    }
#endif // wxUSE_CRASHREPORT

    return true;
}

int wxEntry(int& argc, wxChar **argv)
{
    DisableAutomaticSETranslator();

    wxSEH_TRY
    {
        return wxEntryReal(argc, argv);
    }
    wxSEH_HANDLE(-1)
}

#else // !wxUSE_ON_FATAL_EXCEPTION

#if defined(__VISUALC__) && !defined(__WXWINCE__)

static void
wxSETranslator(unsigned int WXUNUSED(code), EXCEPTION_POINTERS * WXUNUSED(ep))
{
    // see wxSETranslator() version for wxUSE_ON_FATAL_EXCEPTION above
    throw;
}

#endif // __VISUALC__

int wxEntry(int& argc, wxChar **argv)
{
    DisableAutomaticSETranslator();

    return wxEntryReal(argc, argv);
}

#endif // wxUSE_ON_FATAL_EXCEPTION/!wxUSE_ON_FATAL_EXCEPTION

#endif // wxUSE_BASE

#if wxUSE_GUI && defined(__WXMSW__)

#if wxUSE_UNICODE && !defined(__WXWINCE__)
    #define NEED_UNICODE_CHECK
#endif

#ifdef NEED_UNICODE_CHECK

// check whether Unicode is available
static bool wxIsUnicodeAvailable()
{
    static const wchar_t *ERROR_STRING = L"wxWidgets Fatal Error";

    if ( wxGetOsVersion() != wxOS_WINDOWS_NT )
    {
        // we need to be built with MSLU support
#if !wxUSE_UNICODE_MSLU
        // note that we can use MessageBoxW() as it's implemented even under
        // Win9x - OTOH, we can't use wxGetTranslation() because the file APIs
        // used by wxLocale are not
        ::MessageBox
        (
         NULL,
         L"This program uses Unicode and requires Windows NT/2000/XP.\n"
         L"\n"
         L"Program aborted.",
         ERROR_STRING,
         MB_ICONERROR | MB_OK
        );

        return false;
#else // wxUSE_UNICODE_MSLU
        // and the MSLU DLL must also be available
        HMODULE hmod = ::LoadLibraryA("unicows.dll");
        if ( !hmod )
        {
            ::MessageBox
            (
             NULL,
             L"This program uses Unicode and requires unicows.dll to work "
             L"under current operating system.\n"
             L"\n"
             L"Please install unicows.dll and relaunch the program.",
             ERROR_STRING,
             MB_ICONERROR | MB_OK
            );
            return false;
        }

        // this is not really necessary but be tidy
        ::FreeLibrary(hmod);

        // finally do the last check: has unicows.lib initialized correctly?
        hmod = ::LoadLibraryW(L"unicows.dll");
        if ( !hmod )
        {
            ::MessageBox
            (
             NULL,
             L"This program uses Unicode but is not using unicows.dll\n"
             L"correctly and so cannot work under current operating system.\n"
             L"Please contact the program author for an updated version.\n"
             L"\n"
             L"Program aborted.",
             ERROR_STRING,
             MB_ICONERROR | MB_OK
            );

            return false;
        }

        ::FreeLibrary(hmod);
#endif // !wxUSE_UNICODE_MSLU
    }

    return true;
}

#endif // NEED_UNICODE_CHECK

// ----------------------------------------------------------------------------
// Windows-specific wxEntry
// ----------------------------------------------------------------------------

// helper function used to clean up in wxEntry() just below
//
// notice that argv elements are supposed to be allocated using malloc() while
// argv array itself is allocated with new
static void wxFreeArgs(int argc, wxChar **argv)
{
    for ( int i = 0; i < argc; i++ )
    {
        free(argv[i]);
    }

    delete [] argv;
}

WXDLLEXPORT int wxEntry(HINSTANCE hInstance,
                        HINSTANCE WXUNUSED(hPrevInstance),
                        wxCmdLineArgType WXUNUSED(pCmdLine),
                        int nCmdShow)
{
    // the first thing to do is to check if we're trying to run an Unicode
    // program under Win9x w/o MSLU emulation layer - if so, abort right now
    // as it has no chance to work and has all chances to crash
#ifdef NEED_UNICODE_CHECK
    if ( !wxIsUnicodeAvailable() )
        return -1;
#endif // NEED_UNICODE_CHECK


    // remember the parameters Windows gave us
    wxSetInstance(hInstance);
    wxApp::m_nCmdShow = nCmdShow;

    // parse the command line: we can't use pCmdLine in Unicode build so it is
    // simpler to never use it at all (this also results in a more correct
    // argv[0])

    // break the command line in words
    wxArrayString args;

    const wxChar *cmdLine = ::GetCommandLine();
    if ( cmdLine )
    {
        args = wxCmdLineParser::ConvertStringToArgs(cmdLine);
    }

#ifdef __WXWINCE__
    // WinCE doesn't insert the program itself, so do it ourselves.
    args.Insert(wxGetFullModuleName(), 0);
#endif

    int argc = args.GetCount();

    // +1 here for the terminating NULL
    wxChar **argv = new wxChar *[argc + 1];
    for ( int i = 0; i < argc; i++ )
    {
        argv[i] = wxStrdup(args[i]);
    }

    // argv[] must be NULL-terminated
    argv[argc] = NULL;

    wxON_BLOCK_EXIT2(wxFreeArgs, argc, argv);

    return wxEntry(argc, argv);
}

#endif // wxUSE_GUI && __WXMSW__

// ----------------------------------------------------------------------------
// global HINSTANCE
// ----------------------------------------------------------------------------

#if wxUSE_BASE

HINSTANCE wxhInstance = 0;

extern "C" HINSTANCE wxGetInstance()
{
    return wxhInstance;
}

void wxSetInstance(HINSTANCE hInst)
{
    wxhInstance = hInst;
}

#endif // wxUSE_BASE
