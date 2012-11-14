/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/init.cpp
// Purpose:     initialisation for the library
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.10.99
// RCS-ID:      $Id: init.cpp 61555 2009-07-30 07:42:54Z VS $
// Copyright:   (c) Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef    __BORLANDC__
    #pragma hdrstop
#endif  //__BORLANDC__

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/filefn.h"
    #include "wx/log.h"
    #include "wx/thread.h"
    #include "wx/intl.h"
    #include "wx/module.h"
#endif

#include "wx/init.h"

#include "wx/ptr_scpd.h"
#include "wx/except.h"

#if defined(__WXMSW__) && defined(__WXDEBUG__)
    #include "wx/msw/msvcrt.h"

    static struct EnableMemLeakChecking
    {
        EnableMemLeakChecking()
        {
            // do check for memory leaks on program exit (another useful flag
            // is _CRTDBG_DELAY_FREE_MEM_DF which doesn't free deallocated
            // memory which may be used to simulate low-memory condition)
            wxCrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF);
        }
    } gs_enableLeakChecks;
#endif // __WXMSW__ && __WXDEBUG__

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// we need a dummy app object if the user doesn't want to create a real one
class wxDummyConsoleApp : public wxAppConsole
{
public:
    wxDummyConsoleApp() { }

    virtual int OnRun() { wxFAIL_MSG( _T("unreachable code") ); return 0; }

    DECLARE_NO_COPY_CLASS(wxDummyConsoleApp)
};

// we need a special kind of auto pointer to wxApp which not only deletes the
// pointer it holds in its dtor but also resets the global application pointer
wxDECLARE_SCOPED_PTR(wxAppConsole, wxAppPtrBase)
wxDEFINE_SCOPED_PTR(wxAppConsole, wxAppPtrBase)

class wxAppPtr : public wxAppPtrBase
{
public:
    wxEXPLICIT wxAppPtr(wxAppConsole *ptr = NULL) : wxAppPtrBase(ptr) { }
    ~wxAppPtr()
    {
        if ( get() )
        {
            // the pointer is going to be deleted in the base class dtor, don't
            // leave the dangling pointer!
            wxApp::SetInstance(NULL);
        }
    }

    void Set(wxAppConsole *ptr)
    {
        reset(ptr);

        wxApp::SetInstance(ptr);
    }

    DECLARE_NO_COPY_CLASS(wxAppPtr)
};

// class to ensure that wxAppBase::CleanUp() is called if our Initialize()
// fails
class wxCallAppCleanup
{
public:
    wxCallAppCleanup(wxAppConsole *app) : m_app(app) { }
    ~wxCallAppCleanup() { if ( m_app ) m_app->CleanUp(); }

    void Dismiss() { m_app = NULL; }

private:
    wxAppConsole *m_app;
};

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// suppress warnings about unused variables
static inline void Use(void *) { }

#define WX_SUPPRESS_UNUSED_WARN(x) Use(&x)

// ----------------------------------------------------------------------------
// initialization data
// ----------------------------------------------------------------------------

static struct InitData
{
    InitData()
    {
        nInitCount = 0;

#if wxUSE_UNICODE
        argc = 0;
        // argv = NULL; -- not even really needed
#endif // wxUSE_UNICODE
    }

    // critical section protecting this struct
    wxCRIT_SECT_DECLARE_MEMBER(csInit);

    // number of times wxInitialize() was called minus the number of times
    // wxUninitialize() was
    size_t nInitCount;

#if wxUSE_UNICODE
    int argc;

    // if we receive the command line arguments as ASCII and have to convert
    // them to Unicode ourselves (this is the case under Unix but not Windows,
    // for example), we remember the converted argv here because we'll have to
    // free it when doing cleanup to avoid memory leaks
    wchar_t **argv;
#endif // wxUSE_UNICODE

    DECLARE_NO_COPY_CLASS(InitData)
} gs_initData;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// command line arguments ANSI -> Unicode conversion
// ----------------------------------------------------------------------------

#if wxUSE_UNICODE

static void ConvertArgsToUnicode(int argc, char **argv)
{
    gs_initData.argv = new wchar_t *[argc + 1];
    int wargc = 0;
    for ( int i = 0; i < argc; i++ )
    {
#ifdef __DARWIN__
        wxWCharBuffer buf(wxConvFileName->cMB2WX(argv[i]));
#else
        wxWCharBuffer buf(wxConvLocal.cMB2WX(argv[i]));
#endif
        if ( !buf )
        {
            wxLogWarning(_("Command line argument %d couldn't be converted to Unicode and will be ignored."),
                         i);
        }
        else // converted ok
        {
            gs_initData.argv[wargc++] = wxStrdup(buf);
        }
    }

    gs_initData.argc = wargc;
    gs_initData.argv[wargc] = NULL;
}

static void FreeConvertedArgs()
{
    if ( gs_initData.argv )
    {
        for ( int i = 0; i < gs_initData.argc; i++ )
        {
            free(gs_initData.argv[i]);
        }

        delete [] gs_initData.argv;
        gs_initData.argv = NULL;
        gs_initData.argc = 0;
    }
}

#endif // wxUSE_UNICODE

// ----------------------------------------------------------------------------
// start up
// ----------------------------------------------------------------------------

// initialization which is always done (not customizable) before wxApp creation
static bool DoCommonPreInit()
{
#if wxUSE_LOG
    // Reset logging in case we were cleaned up and are being reinitialized.
    wxLog::DoCreateOnDemand();

    // install temporary log sink: we can't use wxLogGui before wxApp is
    // constructed and if we use wxLogStderr, all messages during
    // initialization simply disappear under Windows
    //
    // note that we will delete this log target below
    delete wxLog::SetActiveTarget(new wxLogBuffer);
#endif // wxUSE_LOG

    return true;
}

// non customizable initialization done after wxApp creation and initialization
static bool DoCommonPostInit()
{
    wxModule::RegisterModules();

    if ( !wxModule::InitializeModules() )
    {
        wxLogError(_("Initialization failed in post init, aborting."));
        return false;
    }

    return true;
}

bool wxEntryStart(int& argc, wxChar **argv)
{
    // do minimal, always necessary, initialization
    // --------------------------------------------

    // initialize wxRTTI
    if ( !DoCommonPreInit() )
    {
        return false;
    }


    // first of all, we need an application object
    // -------------------------------------------

    // the user might have already created it himself somehow
    wxAppPtr app(wxTheApp);
    if ( !app.get() )
    {
        // if not, he might have used IMPLEMENT_APP() to give us a function to
        // create it
        wxAppInitializerFunction fnCreate = wxApp::GetInitializerFunction();

        if ( fnCreate )
        {
            // he did, try to create the custom wxApp object
            app.Set((*fnCreate)());
        }
    }

    if ( !app.get() )
    {
        // either IMPLEMENT_APP() was not used at all or it failed -- in any
        // case we still need something
        app.Set(new wxDummyConsoleApp);
    }


    // wxApp initialization: this can be customized
    // --------------------------------------------

    if ( !app->Initialize(argc, argv) )
    {
        return false;
    }

    wxCallAppCleanup callAppCleanup(app.get());

    // for compatibility call the old initialization function too
    if ( !app->OnInitGui() )
        return false;


    // common initialization after wxTheApp creation
    // ---------------------------------------------

    if ( !DoCommonPostInit() )
        return false;


    // prevent the smart pointer from destroying its contents
    app.release();

    // and the cleanup object from doing cleanup
    callAppCleanup.Dismiss();

#if wxUSE_LOG
    // now that we have a valid wxApp (wxLogGui would have crashed if we used
    // it before now), we can delete the temporary sink we had created for the
    // initialization messages -- the next time logging function is called, the
    // sink will be recreated but this time wxAppTraits will be used
    delete wxLog::SetActiveTarget(NULL);
#endif // wxUSE_LOG

    return true;
}

#if wxUSE_UNICODE

// we provide a wxEntryStart() wrapper taking "char *" pointer too
bool wxEntryStart(int& argc, char **argv)
{
    ConvertArgsToUnicode(argc, argv);

    if ( !wxEntryStart(gs_initData.argc, gs_initData.argv) )
    {
        FreeConvertedArgs();

        return false;
    }

    return true;
}

#endif // wxUSE_UNICODE

// ----------------------------------------------------------------------------
// clean up
// ----------------------------------------------------------------------------

// cleanup done before destroying wxTheApp
static void DoCommonPreCleanup()
{
#if wxUSE_LOG
    // flush the logged messages if any and install a 'safer' log target: the
    // default one (wxLogGui) can't be used after the resources are freed just
    // below and the user supplied one might be even more unsafe (using any
    // wxWidgets GUI function is unsafe starting from now)
    wxLog::DontCreateOnDemand();

    // this will flush the old messages if any
    delete wxLog::SetActiveTarget(new wxLogStderr);
#endif // wxUSE_LOG
}

// cleanup done after destroying wxTheApp
static void DoCommonPostCleanup()
{
    wxModule::CleanUpModules();

    // we can't do this in wxApp itself because it doesn't know if argv had
    // been allocated
#if wxUSE_UNICODE
    FreeConvertedArgs();
#endif // wxUSE_UNICODE

    // use Set(NULL) and not Get() to avoid creating a message output object on
    // demand when we just want to delete it
    delete wxMessageOutput::Set(NULL);

#if wxUSE_LOG
    // and now delete the last logger as well
    delete wxLog::SetActiveTarget(NULL);
#endif // wxUSE_LOG
}

void wxEntryCleanup()
{
    DoCommonPreCleanup();


    // delete the application object
    if ( wxTheApp )
    {
        wxTheApp->CleanUp();

        // reset the global pointer to it to NULL before destroying it as in
        // some circumstances this can result in executing the code using
        // wxTheApp and using half-destroyed object is no good
        wxAppConsole * const app = wxApp::GetInstance();
        wxApp::SetInstance(NULL);
        delete app;
    }


    DoCommonPostCleanup();
}

// ----------------------------------------------------------------------------
// wxEntry
// ----------------------------------------------------------------------------

// for MSW the real wxEntry is defined in msw/main.cpp
#ifndef __WXMSW__
    #define wxEntryReal wxEntry
#endif // !__WXMSW__

int wxEntryReal(int& argc, wxChar **argv)
{
    // library initialization
    wxInitializer initializer(argc, argv);

    if ( !initializer.IsOk() )
    {
#if wxUSE_LOG
        // flush any log messages explaining why we failed
        delete wxLog::SetActiveTarget(NULL);
#endif
        return -1;
    }

    wxTRY
    {

        // app initialization
        if ( !wxTheApp->CallOnInit() )
        {
            // don't call OnExit() if OnInit() failed
            return -1;
        }

        // ensure that OnExit() is called if OnInit() had succeeded
        class CallOnExit
        {
        public:
            ~CallOnExit() { wxTheApp->OnExit(); }
        } callOnExit;

        WX_SUPPRESS_UNUSED_WARN(callOnExit);

        // app execution
        return wxTheApp->OnRun();
    }
    wxCATCH_ALL( wxTheApp->OnUnhandledException(); return -1; )
}

#if wxUSE_UNICODE

// as with wxEntryStart, we provide an ANSI wrapper
int wxEntry(int& argc, char **argv)
{
    ConvertArgsToUnicode(argc, argv);

    return wxEntry(gs_initData.argc, gs_initData.argv);
}

#endif // wxUSE_UNICODE

// ----------------------------------------------------------------------------
// wxInitialize/wxUninitialize
// ----------------------------------------------------------------------------

bool wxInitialize(int argc, wxChar **argv)
{
    wxCRIT_SECT_LOCKER(lockInit, gs_initData.csInit);

    if ( gs_initData.nInitCount++ )
    {
        // already initialized
        return true;
    }

    return wxEntryStart(argc, argv);
}

#if wxUSE_UNICODE
bool wxInitialize(int argc, char **argv)
{
    wxCRIT_SECT_LOCKER(lockInit, gs_initData.csInit);

    if ( gs_initData.nInitCount++ )
    {
        // already initialized
        return true;
    }

    return wxEntryStart(argc, argv);
}
#endif // wxUSE_UNICODE

void wxUninitialize()
{
    wxCRIT_SECT_LOCKER(lockInit, gs_initData.csInit);

    if ( --gs_initData.nInitCount == 0 )
    {
        wxEntryCleanup();
    }
}
