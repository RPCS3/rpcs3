/////////////////////////////////////////////////////////////////////////////
// Name:        wx/app.h
// Purpose:     wxAppBase class and macros used for declaration of wxApp
//              derived class in the user code
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: app.h 51592 2008-02-08 08:17:41Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_APP_H_BASE_
#define _WX_APP_H_BASE_

// ----------------------------------------------------------------------------
// headers we have to include here
// ----------------------------------------------------------------------------

#include "wx/event.h"       // for the base class
#include "wx/build.h"
#include "wx/init.h"        // we must declare wxEntry()
#include "wx/intl.h"        // for wxLayoutDirection

class WXDLLIMPEXP_FWD_BASE wxAppConsole;
class WXDLLIMPEXP_FWD_BASE wxAppTraits;
class WXDLLIMPEXP_FWD_BASE wxCmdLineParser;
class WXDLLIMPEXP_FWD_BASE wxLog;
class WXDLLIMPEXP_FWD_BASE wxMessageOutput;

#if wxUSE_GUI
    class WXDLLIMPEXP_FWD_BASE wxEventLoop;
    struct WXDLLIMPEXP_FWD_CORE wxVideoMode;
#endif

// ----------------------------------------------------------------------------
// typedefs
// ----------------------------------------------------------------------------

// the type of the function used to create a wxApp object on program start up
typedef wxAppConsole* (*wxAppInitializerFunction)();

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

enum
{
    wxPRINT_WINDOWS = 1,
    wxPRINT_POSTSCRIPT = 2
};

// ----------------------------------------------------------------------------
// wxAppConsole: wxApp for non-GUI applications
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxAppConsole : public wxEvtHandler
{
public:
    // ctor and dtor
    wxAppConsole();
    virtual ~wxAppConsole();


    // the virtual functions which may/must be overridden in the derived class
    // -----------------------------------------------------------------------

    // This is the very first function called for a newly created wxApp object,
    // it is used by the library to do the global initialization. If, for some
    // reason, you must override it (instead of just overriding OnInit(), as
    // usual, for app-specific initializations), do not forget to call the base
    // class version!
    virtual bool Initialize(int& argc, wxChar **argv);

    // This gives wxCocoa a chance to call OnInit() with a memory pool in place
    virtual bool CallOnInit() { return OnInit(); }

    // Called before OnRun(), this is a good place to do initialization -- if
    // anything fails, return false from here to prevent the program from
    // continuing. The command line is normally parsed here, call the base
    // class OnInit() to do it.
    virtual bool OnInit();

    // this is here only temporary hopefully (FIXME)
    virtual bool OnInitGui() { return true; }

    // This is the replacement for the normal main(): all program work should
    // be done here. When OnRun() returns, the programs starts shutting down.
    virtual int OnRun() = 0;

    // This is only called if OnInit() returned true so it's a good place to do
    // any cleanup matching the initializations done there.
    virtual int OnExit();

    // This is the very last function called on wxApp object before it is
    // destroyed. If you override it (instead of overriding OnExit() as usual)
    // do not forget to call the base class version!
    virtual void CleanUp();

    // Called when a fatal exception occurs, this function should take care not
    // to do anything which might provoke a nested exception! It may be
    // overridden if you wish to react somehow in non-default way (core dump
    // under Unix, application crash under Windows) to fatal program errors,
    // however extreme care should be taken if you don't want this function to
    // crash.
    virtual void OnFatalException() { }

    // Called from wxExit() function, should terminate the application a.s.a.p.
    virtual void Exit();


    // application info: name, description, vendor
    // -------------------------------------------

    // NB: all these should be set by the application itself, there are no
    //     reasonable default except for the application name which is taken to
    //     be argv[0]

        // set/get the application name
    wxString GetAppName() const
    {
        return m_appName.empty() ? m_className : m_appName;
    }
    void SetAppName(const wxString& name) { m_appName = name; }

        // set/get the app class name
    wxString GetClassName() const { return m_className; }
    void SetClassName(const wxString& name) { m_className = name; }

        // set/get the vendor name
    const wxString& GetVendorName() const { return m_vendorName; }
    void SetVendorName(const wxString& name) { m_vendorName = name; }


    // cmd line parsing stuff
    // ----------------------

    // all of these methods may be overridden in the derived class to
    // customize the command line parsing (by default only a few standard
    // options are handled)
    //
    // you also need to call wxApp::OnInit() from YourApp::OnInit() for all
    // this to work

#if wxUSE_CMDLINE_PARSER
    // this one is called from OnInit() to add all supported options
    // to the given parser (don't forget to call the base class version if you
    // override it!)
    virtual void OnInitCmdLine(wxCmdLineParser& parser);

    // called after successfully parsing the command line, return true
    // to continue and false to exit (don't forget to call the base class
    // version if you override it!)
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

    // called if "--help" option was specified, return true to continue
    // and false to exit
    virtual bool OnCmdLineHelp(wxCmdLineParser& parser);

    // called if incorrect command line options were given, return
    // false to abort and true to continue
    virtual bool OnCmdLineError(wxCmdLineParser& parser);
#endif // wxUSE_CMDLINE_PARSER


    // miscellaneous customization functions
    // -------------------------------------

    // create the app traits object to which we delegate for everything which
    // either should be configurable by the user (then he can change the
    // default behaviour simply by overriding CreateTraits() and returning his
    // own traits object) or which is GUI/console dependent as then wxAppTraits
    // allows us to abstract the differences behind the common façade
    wxAppTraits *GetTraits();

    // the functions below shouldn't be used now that we have wxAppTraits
#if WXWIN_COMPATIBILITY_2_4

#if wxUSE_LOG
        // override this function to create default log target of arbitrary
        // user-defined class (default implementation creates a wxLogGui
        // object) -- this log object is used by default by all wxLogXXX()
        // functions.
    wxDEPRECATED( virtual wxLog *CreateLogTarget() );
#endif // wxUSE_LOG

        // similar to CreateLogTarget() but for the global wxMessageOutput
        // object
    wxDEPRECATED( virtual wxMessageOutput *CreateMessageOutput() );

#endif // WXWIN_COMPATIBILITY_2_4


    // event processing functions
    // --------------------------

    // this method allows to filter all the events processed by the program, so
    // you should try to return quickly from it to avoid slowing down the
    // program to the crawl
    //
    // return value should be -1 to continue with the normal event processing,
    // or TRUE or FALSE to stop further processing and pretend that the event
    // had been already processed or won't be processed at all, respectively
    virtual int FilterEvent(wxEvent& event);

#if wxUSE_EXCEPTIONS
    // call the specified handler on the given object with the given event
    //
    // this method only exists to allow catching the exceptions thrown by any
    // event handler, it would lead to an extra (useless) virtual function call
    // if the exceptions were not used, so it doesn't even exist in that case
    virtual void HandleEvent(wxEvtHandler *handler,
                             wxEventFunction func,
                             wxEvent& event) const;

    // Called when an unhandled C++ exception occurs inside OnRun(): note that
    // the exception type is lost by now, so if you really want to handle the
    // exception you should override OnRun() and put a try/catch around
    // MainLoop() call there or use OnExceptionInMainLoop()
    virtual void OnUnhandledException() { }
#endif // wxUSE_EXCEPTIONS

    // process all events in the wxPendingEvents list -- it is necessary to
    // call this function to process posted events. This happens during each
    // event loop iteration in GUI mode but if there is no main loop, it may be
    // also called directly.
    virtual void ProcessPendingEvents();

    // doesn't do anything in this class, just a hook for GUI wxApp
    virtual bool Yield(bool WXUNUSED(onlyIfNeeded) = false) { return true; }

    // make sure that idle events are sent again
    virtual void WakeUpIdle() { }

    // this is just a convenience: by providing its implementation here we
    // avoid #ifdefs in the code using it
    static bool IsMainLoopRunning() { return false; }


    // debugging support
    // -----------------

#ifdef __WXDEBUG__
    // this function is called when an assert failure occurs, the base class
    // version does the normal processing (i.e. shows the usual assert failure
    // dialog box)
    //
    // the arguments are the location of the failed assert (func may be empty
    // if the compiler doesn't support C99 __FUNCTION__), the text of the
    // assert itself and the user-specified message
    virtual void OnAssertFailure(const wxChar *file,
                                 int line,
                                 const wxChar *func,
                                 const wxChar *cond,
                                 const wxChar *msg);

    // old version of the function without func parameter, for compatibility
    // only, override OnAssertFailure() in the new code
    virtual void OnAssert(const wxChar *file,
                          int line,
                          const wxChar *cond,
                          const wxChar *msg);
#endif // __WXDEBUG__

    // check that the wxBuildOptions object (constructed in the application
    // itself, usually the one from IMPLEMENT_APP() macro) matches the build
    // options of the library and abort if it doesn't
    static bool CheckBuildOptions(const char *optionsSignature,
                                  const char *componentName);
#if WXWIN_COMPATIBILITY_2_4
    wxDEPRECATED( static bool CheckBuildOptions(const wxBuildOptions& buildOptions) );
#endif

    // implementation only from now on
    // -------------------------------

    // helpers for dynamic wxApp construction
    static void SetInitializerFunction(wxAppInitializerFunction fn)
        { ms_appInitFn = fn; }
    static wxAppInitializerFunction GetInitializerFunction()
        { return ms_appInitFn; }

    // accessors for ms_appInstance field (external code might wish to modify
    // it, this is why we provide a setter here as well, but you should really
    // know what you're doing if you call it), wxTheApp is usually used instead
    // of GetInstance()
    static wxAppConsole *GetInstance() { return ms_appInstance; }
    static void SetInstance(wxAppConsole *app) { ms_appInstance = app; }


    // command line arguments (public for backwards compatibility)
    int      argc;
    wxChar **argv;

protected:
    // the function which creates the traits object when GetTraits() needs it
    // for the first time
    virtual wxAppTraits *CreateTraits();


    // function used for dynamic wxApp creation
    static wxAppInitializerFunction ms_appInitFn;

    // the one and only global application object
    static wxAppConsole *ms_appInstance;


    // application info (must be set from the user code)
    wxString m_vendorName,      // vendor name (ACME Inc)
             m_appName,         // app name
             m_className;       // class name

    // the class defining the application behaviour, NULL initially and created
    // by GetTraits() when first needed
    wxAppTraits *m_traits;


    // the application object is a singleton anyhow, there is no sense in
    // copying it
    DECLARE_NO_COPY_CLASS(wxAppConsole)
};

// ----------------------------------------------------------------------------
// wxAppBase: the common part of wxApp implementations for all platforms
// ----------------------------------------------------------------------------

#if wxUSE_GUI

class WXDLLIMPEXP_CORE wxAppBase : public wxAppConsole
{
public:
    wxAppBase();
    virtual ~wxAppBase();

    // the virtual functions which may/must be overridden in the derived class
    // -----------------------------------------------------------------------

        // very first initialization function
        //
        // Override: very rarely
    virtual bool Initialize(int& argc, wxChar **argv);

        // a platform-dependent version of OnInit(): the code here is likely to
        // depend on the toolkit. default version does nothing.
        //
        // Override: rarely.
    virtual bool OnInitGui();

        // called to start program execution - the default version just enters
        // the main GUI loop in which events are received and processed until
        // the last window is not deleted (if GetExitOnFrameDelete) or
        // ExitMainLoop() is called. In console mode programs, the execution
        // of the program really starts here
        //
        // Override: rarely in GUI applications, always in console ones.
    virtual int OnRun();

        // a matching function for OnInit()
    virtual int OnExit();

        // very last clean up function
        //
        // Override: very rarely
    virtual void CleanUp();


    // the worker functions - usually not used directly by the user code
    // -----------------------------------------------------------------

        // return true if we're running main loop, i.e. if the events can
        // (already) be dispatched
    static bool IsMainLoopRunning()
    {
        wxAppBase *app = wx_static_cast(wxAppBase *, GetInstance());
        return app && app->m_mainLoop != NULL;
    }

        // execute the main GUI loop, the function returns when the loop ends
    virtual int MainLoop();

        // exit the main loop thus terminating the application
    virtual void Exit();

        // exit the main GUI loop during the next iteration (i.e. it does not
        // stop the program immediately!)
    virtual void ExitMainLoop();

        // returns true if there are unprocessed events in the event queue
    virtual bool Pending();

        // process the first event in the event queue (blocks until an event
        // appears if there are none currently, use Pending() if this is not
        // wanted), returns false if the event loop should stop and true
        // otherwise
    virtual bool Dispatch();

        // process all currently pending events right now
        //
        // it is an error to call Yield() recursively unless the value of
        // onlyIfNeeded is true
        //
        // WARNING: this function is dangerous as it can lead to unexpected
        //          reentrancies (i.e. when called from an event handler it
        //          may result in calling the same event handler again), use
        //          with _extreme_ care or, better, don't use at all!
    virtual bool Yield(bool onlyIfNeeded = false) = 0;

        // this virtual function is called in the GUI mode when the application
        // becomes idle and normally just sends wxIdleEvent to all interested
        // parties
        //
        // it should return true if more idle events are needed, false if not
    virtual bool ProcessIdle();

        // Send idle event to window and all subwindows
        // Returns true if more idle time is requested.
    virtual bool SendIdleEvents(wxWindow* win, wxIdleEvent& event);


#if wxUSE_EXCEPTIONS
    // Function called if an uncaught exception is caught inside the main
    // event loop: it may return true to continue running the event loop or
    // false to stop it (in the latter case it may rethrow the exception as
    // well)
    virtual bool OnExceptionInMainLoop();
#endif // wxUSE_EXCEPTIONS


    // top level window functions
    // --------------------------

        // return true if our app has focus
    virtual bool IsActive() const { return m_isActive; }

        // set the "main" top level window
    void SetTopWindow(wxWindow *win) { m_topWindow = win; }

        // return the "main" top level window (if it hadn't been set previously
        // with SetTopWindow(), will return just some top level window and, if
        // there are none, will return NULL)
    virtual wxWindow *GetTopWindow() const;

        // control the exit behaviour: by default, the program will exit the
        // main loop (and so, usually, terminate) when the last top-level
        // program window is deleted. Beware that if you disable this behaviour
        // (with SetExitOnFrameDelete(false)), you'll have to call
        // ExitMainLoop() explicitly from somewhere.
    void SetExitOnFrameDelete(bool flag)
        { m_exitOnFrameDelete = flag ? Yes : No; }
    bool GetExitOnFrameDelete() const
        { return m_exitOnFrameDelete == Yes; }


    // display mode, visual, printing mode, ...
    // ------------------------------------------------------------------------

        // Get display mode that is used use. This is only used in framebuffer
        // wxWin ports (such as wxMGL or wxDFB).
    virtual wxVideoMode GetDisplayMode() const;
        // Set display mode to use. This is only used in framebuffer wxWin
        // ports (such as wxMGL or wxDFB). This method should be called from
        // wxApp::OnInitGui
    virtual bool SetDisplayMode(const wxVideoMode& WXUNUSED(info)) { return true; }

        // set use of best visual flag (see below)
    void SetUseBestVisual( bool flag, bool forceTrueColour = false ) 
        { m_useBestVisual = flag; m_forceTrueColour = forceTrueColour; }
    bool GetUseBestVisual() const { return m_useBestVisual; }

        // set/get printing mode: see wxPRINT_XXX constants.
        //
        // default behaviour is the normal one for Unix: always use PostScript
        // printing.
    virtual void SetPrintMode(int WXUNUSED(mode)) { }
    int GetPrintMode() const { return wxPRINT_POSTSCRIPT; }

    // Return the layout direction for the current locale or wxLayout_Default
    // if it's unknown
    virtual wxLayoutDirection GetLayoutDirection() const;


    // command line parsing (GUI-specific)
    // ------------------------------------------------------------------------

#if wxUSE_CMDLINE_PARSER
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
    virtual void OnInitCmdLine(wxCmdLineParser& parser);
#endif

    // miscellaneous other stuff
    // ------------------------------------------------------------------------

    // called by toolkit-specific code to set the app status: active (we have
    // focus) or not and also the last window which had focus before we were
    // deactivated
    virtual void SetActive(bool isActive, wxWindow *lastFocus);

#if WXWIN_COMPATIBILITY_2_6
    // OBSOLETE: don't use, always returns true
    //
    // returns true if the program is successfully initialized
    wxDEPRECATED( bool Initialized() );
#endif // WXWIN_COMPATIBILITY_2_6

    // perform standard OnIdle behaviour, ensure that this is always called
    void OnIdle(wxIdleEvent& event);


protected:
    // delete all objects in wxPendingDelete list
    void DeletePendingObjects();

    // override base class method to use GUI traits
    virtual wxAppTraits *CreateTraits();


    // the main event loop of the application (may be NULL if the loop hasn't
    // been started yet or has already terminated)
    wxEventLoop *m_mainLoop;

    // the main top level window (may be NULL)
    wxWindow *m_topWindow;

    // if Yes, exit the main loop when the last top level window is deleted, if
    // No don't do it and if Later -- only do it once we reach our OnRun()
    //
    // the explanation for using this strange scheme is given in appcmn.cpp
    enum
    {
        Later = -1,
        No,
        Yes
    } m_exitOnFrameDelete;

    // true if the app wants to use the best visual on systems where
    // more than one are available (Sun, SGI, XFree86 4.0 ?)
    bool m_useBestVisual;
    // force TrueColour just in case "best" isn't TrueColour
    bool m_forceTrueColour;

    // does any of our windows have focus?
    bool m_isActive;


    DECLARE_NO_COPY_CLASS(wxAppBase)
};

#if WXWIN_COMPATIBILITY_2_6
    inline bool wxAppBase::Initialized() { return true; }
#endif // WXWIN_COMPATIBILITY_2_6

#endif // wxUSE_GUI

// ----------------------------------------------------------------------------
// now include the declaration of the real class
// ----------------------------------------------------------------------------

#if wxUSE_GUI
    #if defined(__WXPALMOS__)
        #include "wx/palmos/app.h"
    #elif defined(__WXMSW__)
        #include "wx/msw/app.h"
    #elif defined(__WXMOTIF__)
        #include "wx/motif/app.h"
    #elif defined(__WXMGL__)
        #include "wx/mgl/app.h"
    #elif defined(__WXDFB__)
        #include "wx/dfb/app.h"
    #elif defined(__WXGTK20__)
        #include "wx/gtk/app.h"
    #elif defined(__WXGTK__)
        #include "wx/gtk1/app.h"
    #elif defined(__WXX11__)
        #include "wx/x11/app.h"
    #elif defined(__WXMAC__)
        #include "wx/mac/app.h"
    #elif defined(__WXCOCOA__)
        #include "wx/cocoa/app.h"
    #elif defined(__WXPM__)
        #include "wx/os2/app.h"
    #endif
#else // !GUI
    // allow using just wxApp (instead of wxAppConsole) in console programs
    typedef wxAppConsole wxApp;
#endif // GUI/!GUI

// ----------------------------------------------------------------------------
// the global data
// ----------------------------------------------------------------------------

// for compatibility, we define this macro to access the global application
// object of type wxApp
//
// note that instead of using of wxTheApp in application code you should
// consider using DECLARE_APP() after which you may call wxGetApp() which will
// return the object of the correct type (i.e. MyApp and not wxApp)
//
// the cast is safe as in GUI build we only use wxApp, not wxAppConsole, and in
// console mode it does nothing at all
#define wxTheApp wx_static_cast(wxApp*, wxApp::GetInstance())

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// event loop related functions only work in GUI programs
// ------------------------------------------------------

// Force an exit from main loop
extern void WXDLLIMPEXP_BASE wxExit();

// avoid redeclaring this function here if it had been already declated by
// wx/utils.h, this results in warnings from g++ with -Wredundant-decls
#ifndef wx_YIELD_DECLARED
#define wx_YIELD_DECLARED

// Yield to other apps/messages
extern bool WXDLLIMPEXP_BASE wxYield();

#endif // wx_YIELD_DECLARED

// Yield to other apps/messages
extern void WXDLLIMPEXP_BASE wxWakeUpIdle();

// ----------------------------------------------------------------------------
// macros for dynamic creation of the application object
// ----------------------------------------------------------------------------

// Having a global instance of this class allows wxApp to be aware of the app
// creator function. wxApp can then call this function to create a new app
// object. Convoluted, but necessary.

class WXDLLIMPEXP_BASE wxAppInitializer
{
public:
    wxAppInitializer(wxAppInitializerFunction fn)
        { wxApp::SetInitializerFunction(fn); }
};

// the code below defines a IMPLEMENT_WXWIN_MAIN macro which you can use if
// your compiler really, really wants main() to be in your main program (e.g.
// hello.cpp). Now IMPLEMENT_APP should add this code if required.

#define IMPLEMENT_WXWIN_MAIN_CONSOLE \
        int main(int argc, char **argv) { return wxEntry(argc, argv); }

// port-specific header could have defined it already in some special way
#ifndef IMPLEMENT_WXWIN_MAIN
    #define IMPLEMENT_WXWIN_MAIN IMPLEMENT_WXWIN_MAIN_CONSOLE
#endif // defined(IMPLEMENT_WXWIN_MAIN)

#ifdef __WXUNIVERSAL__
    #include "wx/univ/theme.h"

    #ifdef wxUNIV_DEFAULT_THEME
        #define IMPLEMENT_WX_THEME_SUPPORT \
            WX_USE_THEME(wxUNIV_DEFAULT_THEME);
    #else
        #define IMPLEMENT_WX_THEME_SUPPORT
    #endif
#else
    #define IMPLEMENT_WX_THEME_SUPPORT
#endif

// Use this macro if you want to define your own main() or WinMain() function
// and call wxEntry() from there.
#define IMPLEMENT_APP_NO_MAIN(appname)                                      \
    wxAppConsole *wxCreateApp()                                             \
    {                                                                       \
        wxAppConsole::CheckBuildOptions(WX_BUILD_OPTIONS_SIGNATURE,         \
                                        "your program");                    \
        return new appname;                                                 \
    }                                                                       \
    wxAppInitializer                                                        \
        wxTheAppInitializer((wxAppInitializerFunction) wxCreateApp);        \
    DECLARE_APP(appname)                                                    \
    appname& wxGetApp() { return *wx_static_cast(appname*, wxApp::GetInstance()); }

// Same as IMPLEMENT_APP() normally but doesn't include themes support in
// wxUniversal builds
#define IMPLEMENT_APP_NO_THEMES(appname)    \
    IMPLEMENT_APP_NO_MAIN(appname)          \
    IMPLEMENT_WXWIN_MAIN

// Use this macro exactly once, the argument is the name of the wxApp-derived
// class which is the class of your application.
#define IMPLEMENT_APP(appname)              \
    IMPLEMENT_APP_NO_THEMES(appname)        \
    IMPLEMENT_WX_THEME_SUPPORT

// Same as IMPLEMENT_APP(), but for console applications.
#define IMPLEMENT_APP_CONSOLE(appname)      \
    IMPLEMENT_APP_NO_MAIN(appname)          \
    IMPLEMENT_WXWIN_MAIN_CONSOLE

// this macro can be used multiple times and just allows you to use wxGetApp()
// function
#define DECLARE_APP(appname) extern appname& wxGetApp();


// declare the stuff defined by IMPLEMENT_APP() macro, it's not really needed
// anywhere else but at the very least it suppresses icc warnings about
// defining extern symbols without prior declaration, and it shouldn't do any
// harm
extern wxAppConsole *wxCreateApp();
extern wxAppInitializer wxTheAppInitializer;

#endif // _WX_APP_H_BASE_
