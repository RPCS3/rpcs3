/////////////////////////////////////////////////////////////////////////////
// Name:        wx/log.h
// Purpose:     Assorted wxLogXXX functions, and wxLog (sink for logs)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id: log.h 50993 2008-01-02 21:18:15Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_LOG_H_
#define _WX_LOG_H_

#include "wx/defs.h"

// ----------------------------------------------------------------------------
// common constants for use in wxUSE_LOG/!wxUSE_LOG
// ----------------------------------------------------------------------------

// the trace masks have been superceded by symbolic trace constants, they're
// for compatibility only andwill be removed soon - do NOT use them

// meaning of different bits of the trace mask (which allows selectively
// enable/disable some trace messages)
#define wxTraceMemAlloc 0x0001  // trace memory allocation (new/delete)
#define wxTraceMessages 0x0002  // trace window messages/X callbacks
#define wxTraceResAlloc 0x0004  // trace GDI resource allocation
#define wxTraceRefCount 0x0008  // trace various ref counting operations

#ifdef  __WXMSW__
    #define wxTraceOleCalls 0x0100  // OLE interface calls
#endif

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

// NB: these types are needed even if wxUSE_LOG == 0
typedef unsigned long wxTraceMask;
typedef unsigned long wxLogLevel;

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/string.h"

#if wxUSE_LOG

#include "wx/arrstr.h"

#ifndef __WXWINCE__
    #include <time.h>   // for time_t
#endif

#include "wx/dynarray.h"

#ifndef wxUSE_LOG_DEBUG
#  ifdef __WXDEBUG__
#    define wxUSE_LOG_DEBUG 1
#  else // !__WXDEBUG__
#    define wxUSE_LOG_DEBUG 0
#  endif
#endif

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------

#if wxUSE_GUI
    class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
    class WXDLLIMPEXP_FWD_CORE wxLogFrame;
    class WXDLLIMPEXP_FWD_CORE wxFrame;
    class WXDLLIMPEXP_FWD_CORE wxWindow;
#endif // wxUSE_GUI

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// different standard log levels (you may also define your own)
enum
{
    wxLOG_FatalError, // program can't continue, abort immediately
    wxLOG_Error,      // a serious error, user must be informed about it
    wxLOG_Warning,    // user is normally informed about it but may be ignored
    wxLOG_Message,    // normal message (i.e. normal output of a non GUI app)
    wxLOG_Status,     // informational: might go to the status line of GUI app
    wxLOG_Info,       // informational message (a.k.a. 'Verbose')
    wxLOG_Debug,      // never shown to the user, disabled in release mode
    wxLOG_Trace,      // trace messages are also only enabled in debug mode
    wxLOG_Progress,   // used for progress indicator (not yet)
    wxLOG_User = 100, // user defined levels start here
    wxLOG_Max = 10000
};

// symbolic trace masks - wxLogTrace("foo", "some trace message...") will be
// discarded unless the string "foo" has been added to the list of allowed
// ones with AddTraceMask()

#define wxTRACE_MemAlloc wxT("memalloc") // trace memory allocation (new/delete)
#define wxTRACE_Messages wxT("messages") // trace window messages/X callbacks
#define wxTRACE_ResAlloc wxT("resalloc") // trace GDI resource allocation
#define wxTRACE_RefCount wxT("refcount") // trace various ref counting operations

#ifdef  __WXMSW__
    #define wxTRACE_OleCalls wxT("ole")  // OLE interface calls
#endif

#include "wx/iosfwrap.h"

// ----------------------------------------------------------------------------
// derive from this class to redirect (or suppress, or ...) log messages
// normally, only a single instance of this class exists but it's not enforced
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxLog
{
public:
    // ctor
    wxLog(){}

    // these functions allow to completely disable all log messages

    // is logging disabled now?
    static bool IsEnabled() { return ms_doLog; }

    // change the flag state, return the previous one
    static bool EnableLogging(bool doIt = true)
        { bool doLogOld = ms_doLog; ms_doLog = doIt; return doLogOld; }

    // static sink function - see DoLog() for function to overload in the
    // derived classes
    static void OnLog(wxLogLevel level, const wxChar *szString, time_t t);

    // message buffering

    // flush shows all messages if they're not logged immediately (FILE
    // and iostream logs don't need it, but wxGuiLog does to avoid showing
    // 17 modal dialogs one after another)
    virtual void Flush();

    // flush the active target if any
    static void FlushActive()
    {
        if ( !ms_suspendCount )
        {
            wxLog *log = GetActiveTarget();
            if ( log )
                log->Flush();
        }
    }

    // only one sink is active at each moment
    // get current log target, will call wxApp::CreateLogTarget() to
    // create one if none exists
    static wxLog *GetActiveTarget();

    // change log target, pLogger may be NULL
    static wxLog *SetActiveTarget(wxLog *pLogger);

    // suspend the message flushing of the main target until the next call
    // to Resume() - this is mainly for internal use (to prevent wxYield()
    // from flashing the messages)
    static void Suspend() { ms_suspendCount++; }

    // must be called for each Suspend()!
    static void Resume() { ms_suspendCount--; }

    // functions controlling the default wxLog behaviour
    // verbose mode is activated by standard command-line '-verbose'
    // option
    static void SetVerbose(bool bVerbose = true) { ms_bVerbose = bVerbose; }

    // Set log level.  Log messages with level > logLevel will not be logged.
    static void SetLogLevel(wxLogLevel logLevel) { ms_logLevel = logLevel; }

#if wxABI_VERSION >= 20805 /* 2.8.5+ only */
    // should GetActiveTarget() try to create a new log object if the
    // current is NULL?
    static void DontCreateOnDemand();
#endif

    // Make GetActiveTarget() create a new log object again.
    static void DoCreateOnDemand();

    // log the count of repeating messages instead of logging the messages
    // multiple times
    static void SetRepetitionCounting(bool bRepetCounting = true)
        { ms_bRepetCounting = bRepetCounting; }

    // gets duplicate counting status
    static bool GetRepetitionCounting() { return ms_bRepetCounting; }

    // trace mask (see wxTraceXXX constants for details)
    static void SetTraceMask(wxTraceMask ulMask) { ms_ulTraceMask = ulMask; }

    // add string trace mask
    static void AddTraceMask(const wxString& str)
        { ms_aTraceMasks.push_back(str); }

    // add string trace mask
    static void RemoveTraceMask(const wxString& str);

    // remove all string trace masks
    static void ClearTraceMasks();

    // get string trace masks
    static const wxArrayString &GetTraceMasks() { return ms_aTraceMasks; }

    // sets the timestamp string: this is used as strftime() format string
    // for the log targets which add time stamps to the messages - set it
    // to NULL to disable time stamping completely.
    static void SetTimestamp(const wxChar *ts) { ms_timestamp = ts; }


    // accessors

    // gets the verbose status
    static bool GetVerbose() { return ms_bVerbose; }

    // get trace mask
    static wxTraceMask GetTraceMask() { return ms_ulTraceMask; }

    // is this trace mask in the list?
    static bool IsAllowedTraceMask(const wxChar *mask);

    // return the current loglevel limit
    static wxLogLevel GetLogLevel() { return ms_logLevel; }

    // get the current timestamp format string (may be NULL)
    static const wxChar *GetTimestamp() { return ms_timestamp; }


    // helpers

    // put the time stamp into the string if ms_timestamp != NULL (don't
    // change it otherwise)
    static void TimeStamp(wxString *str);

    // make dtor virtual for all derived classes
    virtual ~wxLog();


    // this method exists for backwards compatibility only, don't use
    bool HasPendingMessages() const { return true; }

#if WXWIN_COMPATIBILITY_2_6
    // this function doesn't do anything any more, don't call it
    wxDEPRECATED( static wxChar *SetLogBuffer(wxChar *buf, size_t size = 0) );
#endif

protected:
    // the logging functions that can be overriden

    // default DoLog() prepends the time stamp and a prefix corresponding
    // to the message to szString and then passes it to DoLogString()
    virtual void DoLog(wxLogLevel level, const wxChar *szString, time_t t);

    // default DoLogString does nothing but is not pure virtual because if
    // you override DoLog() you might not need it at all
    virtual void DoLogString(const wxChar *szString, time_t t);

    // log a line containing the number of times the previous message was
    // repeated and returns this number (which can be 0)
    static unsigned DoLogNumberOfRepeats();

private:
    // static variables
    // ----------------

    // traditional behaviour or counting repetitions
    static bool        ms_bRepetCounting;
    static wxString    ms_prevString;   // previous message that was logged
    // how many times the previous message was logged
    static unsigned    ms_prevCounter;
    static time_t      ms_prevTimeStamp;// timestamp of the previous message
    static wxLogLevel  ms_prevLevel;    // level of the previous message

    static wxLog      *ms_pLogger;      // currently active log sink
    static bool        ms_doLog;        // false => all logging disabled
    static bool        ms_bAutoCreate;  // create new log targets on demand?
    static bool        ms_bVerbose;     // false => ignore LogInfo messages

    static wxLogLevel  ms_logLevel;     // limit logging to levels <= ms_logLevel

    static size_t      ms_suspendCount; // if positive, logs are not flushed

    // format string for strftime(), if NULL, time stamping log messages is
    // disabled
    static const wxChar *ms_timestamp;

    static wxTraceMask ms_ulTraceMask;   // controls wxLogTrace behaviour
    static wxArrayString ms_aTraceMasks; // more powerful filter for wxLogTrace


    // this is the replacement of DoLogNumberOfRepeats() (which has to be kept
    // to avoid breaking ABI in this version)
    unsigned LogLastRepeatIfNeeded();

    // implementation of the function above which supposes that the caller had
    // already locked gs_prevCS
    unsigned LogLastRepeatIfNeededUnlocked();
};

// ----------------------------------------------------------------------------
// "trivial" derivations of wxLog
// ----------------------------------------------------------------------------

// log everything to a buffer
class WXDLLIMPEXP_BASE wxLogBuffer : public wxLog
{
public:
    wxLogBuffer() { }

    // get the string contents with all messages logged
    const wxString& GetBuffer() const { return m_str; }

    // show the buffer contents to the user in the best possible way (this uses
    // wxMessageOutputMessageBox) and clear it
    virtual void Flush();

protected:
    virtual void DoLog(wxLogLevel level, const wxChar *szString, time_t t);
    virtual void DoLogString(const wxChar *szString, time_t t);

private:
    wxString m_str;

    DECLARE_NO_COPY_CLASS(wxLogBuffer)
};


// log everything to a "FILE *", stderr by default
class WXDLLIMPEXP_BASE wxLogStderr : public wxLog
{
public:
    // redirect log output to a FILE
    wxLogStderr(FILE *fp = (FILE *) NULL);

protected:
    // implement sink function
    virtual void DoLogString(const wxChar *szString, time_t t);

    FILE *m_fp;

    DECLARE_NO_COPY_CLASS(wxLogStderr)
};

#if wxUSE_STD_IOSTREAM

// log everything to an "ostream", cerr by default
class WXDLLIMPEXP_BASE wxLogStream : public wxLog
{
public:
    // redirect log output to an ostream
    wxLogStream(wxSTD ostream *ostr = (wxSTD ostream *) NULL);

protected:
    // implement sink function
    virtual void DoLogString(const wxChar *szString, time_t t);

    // using ptr here to avoid including <iostream.h> from this file
    wxSTD ostream *m_ostr;
};

#endif // wxUSE_STD_IOSTREAM

// ----------------------------------------------------------------------------
// /dev/null log target: suppress logging until this object goes out of scope
// ----------------------------------------------------------------------------

// example of usage:
/*
    void Foo()
    {
        wxFile file;

        // wxFile.Open() normally complains if file can't be opened, we don't
        // want it
        wxLogNull logNo;

        if ( !file.Open("bar") )
            ... process error ourselves ...

        // ~wxLogNull called, old log sink restored
    }
 */
class WXDLLIMPEXP_BASE wxLogNull
{
public:
    wxLogNull() : m_flagOld(wxLog::EnableLogging(false)) { }
    ~wxLogNull() { (void)wxLog::EnableLogging(m_flagOld); }

private:
    bool m_flagOld; // the previous value of the wxLog::ms_doLog
};

// ----------------------------------------------------------------------------
// chaining log target: installs itself as a log target and passes all
// messages to the real log target given to it in the ctor but also forwards
// them to the previously active one
//
// note that you don't have to call SetActiveTarget() with this class, it
// does it itself in its ctor
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxLogChain : public wxLog
{
public:
    wxLogChain(wxLog *logger);
    virtual ~wxLogChain();

    // change the new log target
    void SetLog(wxLog *logger);

    // this can be used to temporarily disable (and then reenable) passing
    // messages to the old logger (by default we do pass them)
    void PassMessages(bool bDoPass) { m_bPassMessages = bDoPass; }

    // are we passing the messages to the previous log target?
    bool IsPassingMessages() const { return m_bPassMessages; }

    // return the previous log target (may be NULL)
    wxLog *GetOldLog() const { return m_logOld; }

    // override base class version to flush the old logger as well
    virtual void Flush();

    // call to avoid destroying the old log target
#if wxABI_VERSION >= 20805 /* 2.8.5+ only */
    void DetachOldLog() { m_logOld = NULL; }
#endif

protected:
    // pass the chain to the old logger if needed
    virtual void DoLog(wxLogLevel level, const wxChar *szString, time_t t);

private:
    // the current log target
    wxLog *m_logNew;

    // the previous log target
    wxLog *m_logOld;

    // do we pass the messages to the old logger?
    bool m_bPassMessages;

    DECLARE_NO_COPY_CLASS(wxLogChain)
};

// a chain log target which uses itself as the new logger
class WXDLLIMPEXP_BASE wxLogPassThrough : public wxLogChain
{
public:
    wxLogPassThrough();

private:
    DECLARE_NO_COPY_CLASS(wxLogPassThrough)
};

#if wxUSE_GUI
    // include GUI log targets:
    #include "wx/generic/logg.h"
#endif // wxUSE_GUI

// ============================================================================
// global functions
// ============================================================================

// ----------------------------------------------------------------------------
// Log functions should be used by application instead of stdio, iostream &c
// for log messages for easy redirection
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// get error code/error message from system in a portable way
// ----------------------------------------------------------------------------

// return the last system error code
WXDLLIMPEXP_BASE unsigned long wxSysErrorCode();

// return the error message for given (or last if 0) error code
WXDLLIMPEXP_BASE const wxChar* wxSysErrorMsg(unsigned long nErrCode = 0);

// ----------------------------------------------------------------------------
// define wxLog<level>
// ----------------------------------------------------------------------------

#define DECLARE_LOG_FUNCTION(level)                                         \
extern void WXDLLIMPEXP_BASE wxVLog##level(const wxChar *szFormat,          \
                                           va_list argptr);                 \
extern void WXDLLIMPEXP_BASE wxLog##level(const wxChar *szFormat,           \
                                          ...) ATTRIBUTE_PRINTF_1
#define DECLARE_LOG_FUNCTION2_EXP(level, argclass, arg, expdecl)            \
extern void expdecl wxVLog##level(argclass arg,                             \
                                  const wxChar *szFormat,                   \
                                  va_list argptr);                          \
extern void expdecl wxLog##level(argclass arg,                              \
                                 const wxChar *szFormat,                    \
                                 ...) ATTRIBUTE_PRINTF_2
#else // !wxUSE_LOG

// log functions do nothing at all
#define DECLARE_LOG_FUNCTION(level)                                         \
inline void wxVLog##level(const wxChar *WXUNUSED(szFormat),                 \
                          va_list WXUNUSED(argptr)) { }                     \
inline void wxLog##level(const wxChar *WXUNUSED(szFormat),                  \
                         ...) { }
#define DECLARE_LOG_FUNCTION2_EXP(level, argclass, arg, expdecl)            \
inline void wxVLog##level(argclass WXUNUSED(arg),                           \
                          const wxChar *WXUNUSED(szFormat),                 \
                          va_list WXUNUSED(argptr)) {}                      \
inline void wxLog##level(argclass WXUNUSED(arg),                            \
                         const wxChar *WXUNUSED(szFormat),                  \
                         ...) { }

// Empty Class to fake wxLogNull
class WXDLLIMPEXP_BASE wxLogNull
{
public:
    wxLogNull() { }
};

// Dummy macros to replace some functions.
#define wxSysErrorCode() (unsigned long)0
#define wxSysErrorMsg( X ) (const wxChar*)NULL

// Fake symbolic trace masks... for those that are used frequently
#define wxTRACE_OleCalls wxEmptyString // OLE interface calls

#endif // wxUSE_LOG/!wxUSE_LOG

#define DECLARE_LOG_FUNCTION2(level, argclass, arg)                         \
    DECLARE_LOG_FUNCTION2_EXP(level, argclass, arg, WXDLLIMPEXP_BASE)


// a generic function for all levels (level is passes as parameter)
DECLARE_LOG_FUNCTION2(Generic, wxLogLevel, level);

// one function per each level
DECLARE_LOG_FUNCTION(FatalError);
DECLARE_LOG_FUNCTION(Error);
DECLARE_LOG_FUNCTION(Warning);
DECLARE_LOG_FUNCTION(Message);
DECLARE_LOG_FUNCTION(Info);
DECLARE_LOG_FUNCTION(Verbose);

// this function sends the log message to the status line of the top level
// application frame, if any
DECLARE_LOG_FUNCTION(Status);

#if wxUSE_GUI
    // this one is the same as previous except that it allows to explicitly
    class WXDLLIMPEXP_FWD_CORE wxFrame;
    // specify the frame to which the output should go
    DECLARE_LOG_FUNCTION2_EXP(Status, wxFrame *, pFrame, WXDLLIMPEXP_CORE);
#endif // wxUSE_GUI

// additional one: as wxLogError, but also logs last system call error code
// and the corresponding error message if available
DECLARE_LOG_FUNCTION(SysError);

// and another one which also takes the error code (for those broken APIs
// that don't set the errno (like registry APIs in Win32))
DECLARE_LOG_FUNCTION2(SysError, long, lErrCode);

// debug functions do nothing in release mode
#if wxUSE_LOG && wxUSE_LOG_DEBUG
    DECLARE_LOG_FUNCTION(Debug);

    // there is no more unconditional LogTrace: it is not different from
    // LogDebug and it creates overload ambiguities
    //DECLARE_LOG_FUNCTION(Trace);

    // this version only logs the message if the mask had been added to the
    // list of masks with AddTraceMask()
    DECLARE_LOG_FUNCTION2(Trace, const wxChar *, mask);

    // and this one does nothing if all of level bits are not set in
    // wxLog::GetActive()->GetTraceMask() -- it's deprecated in favour of
    // string identifiers
    DECLARE_LOG_FUNCTION2(Trace, wxTraceMask, mask);
#else   //!debug || !wxUSE_LOG
    // these functions do nothing in release builds, but don't define them as
    // nothing as it could result in different code structure in debug and
    // release and this could result in trouble when these macros are used
    // inside if/else
    //
    // note that making wxVLogDebug/Trace() themselves (empty inline) functions
    // is a bad idea as some compilers are stupid enough to not inline even
    // empty functions if their parameters are complicated enough, but by
    // defining them as an empty inline function we ensure that even dumbest
    // compilers optimise them away
    inline void wxLogNop() { }

    #define wxVLogDebug(fmt, valist) wxLogNop()
    #define wxVLogTrace(mask, fmt, valist) wxLogNop()

    #ifdef HAVE_VARIADIC_MACROS
        // unlike the inline functions below, this completely removes the
        // wxLogXXX calls from the object file:
        #define wxLogDebug(fmt, ...) wxLogNop()
        #define wxLogTrace(mask, fmt, ...) wxLogNop()
    #else // !HAVE_VARIADIC_MACROS
        // note that leaving out "fmt" in the vararg functions provokes a warning
        // from SGI CC: "the last argument of the varargs function is unnamed"
        inline void wxLogDebug(const wxChar *fmt, ...) { wxUnusedVar(fmt); }
        inline void wxLogTrace(wxTraceMask, const wxChar *fmt, ...) { wxUnusedVar(fmt); }
        inline void wxLogTrace(const wxChar *, const wxChar *fmt, ...) { wxUnusedVar(fmt); }
    #endif // HAVE_VARIADIC_MACROS/!HAVE_VARIADIC_MACROS
#endif // debug/!debug

// wxLogFatalError helper: show the (fatal) error to the user in a safe way,
// i.e. without using wxMessageBox() for example because it could crash
void WXDLLIMPEXP_BASE
wxSafeShowMessage(const wxString& title, const wxString& text);

// ----------------------------------------------------------------------------
// debug only logging functions: use them with API name and error code
// ----------------------------------------------------------------------------

#ifdef __WXDEBUG__
    // make life easier for people using VC++ IDE: clicking on the message
    // will take us immediately to the place of the failed API
#ifdef __VISUALC__
    #define wxLogApiError(api, rc)                                            \
        wxLogDebug(wxT("%s(%d): '%s' failed with error 0x%08lx (%s)."),       \
                   __TFILE__, __LINE__, api,                                  \
                   (long)rc, wxSysErrorMsg(rc))
#else // !VC++
    #define wxLogApiError(api, rc)                                            \
        wxLogDebug(wxT("In file %s at line %d: '%s' failed with ")            \
                   wxT("error 0x%08lx (%s)."),                                \
                   __TFILE__, __LINE__, api,                                  \
                   (long)rc, wxSysErrorMsg(rc))
#endif // VC++/!VC++

    #define wxLogLastError(api) wxLogApiError(api, wxSysErrorCode())

#else   //!debug
    #define wxLogApiError(api, err) wxLogNop()
    #define wxLogLastError(api) wxLogNop()
#endif  //debug/!debug

// wxCocoa has additiional trace masks
#if defined(__WXCOCOA__)
#include "wx/cocoa/log.h"
#endif

#endif  // _WX_LOG_H_

