/////////////////////////////////////////////////////////////////////////////
// Name:        wx/utils.h
// Purpose:     Miscellaneous utilities
// Author:      Julian Smart
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id: utils.h 62544 2009-11-03 14:11:30Z VZ $
// Copyright:   (c) 1998 Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UTILSH__
#define _WX_UTILSH__

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/object.h"
#include "wx/list.h"
#include "wx/filefn.h"
#if wxUSE_GUI
    #include "wx/gdicmn.h"
#endif

class WXDLLIMPEXP_FWD_BASE wxArrayString;
class WXDLLIMPEXP_FWD_BASE wxArrayInt;

// need this for wxGetDiskSpace() as we can't, unfortunately, forward declare
// wxLongLong
#include "wx/longlong.h"

// need for wxOperatingSystemId
#include "wx/platinfo.h"

#ifdef __WATCOMC__
    #include <direct.h>
#elif defined(__X__)
    #include <dirent.h>
    #include <unistd.h>
#endif

#include <stdio.h>

// ----------------------------------------------------------------------------
// Forward declaration
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_BASE wxProcess;
class WXDLLIMPEXP_FWD_CORE wxFrame;
class WXDLLIMPEXP_FWD_CORE wxWindow;
class WXDLLIMPEXP_FWD_CORE wxWindowList;

// ----------------------------------------------------------------------------
// Macros
// ----------------------------------------------------------------------------

#define wxMax(a,b)            (((a) > (b)) ? (a) : (b))
#define wxMin(a,b)            (((a) < (b)) ? (a) : (b))
#define wxClip(a,b,c)         (((a) < (b)) ? (b) : (((a) > (c)) ? (c) : (a)))

// wxGetFreeMemory can return huge amount of memory on 32-bit platforms as well
// so to always use long long for its result type on all platforms which
// support it
#if wxUSE_LONGLONG
    typedef wxLongLong wxMemorySize;
#else
    typedef long wxMemorySize;
#endif

// ----------------------------------------------------------------------------
// String functions (deprecated, use wxString)
// ----------------------------------------------------------------------------

// Make a copy of this string using 'new'
#if WXWIN_COMPATIBILITY_2_4
wxDEPRECATED( WXDLLIMPEXP_BASE wxChar* copystring(const wxChar *s) );
#endif

// A shorter way of using strcmp
#define wxStringEq(s1, s2) (s1 && s2 && (wxStrcmp(s1, s2) == 0))

// ----------------------------------------------------------------------------
// Miscellaneous functions
// ----------------------------------------------------------------------------

// Sound the bell
#if !defined __EMX__ && \
    (defined __WXMOTIF__ || defined __WXGTK__ || defined __WXX11__)
WXDLLIMPEXP_CORE void wxBell();
#else
WXDLLIMPEXP_BASE void wxBell();
#endif

// Get OS description as a user-readable string
WXDLLIMPEXP_BASE wxString wxGetOsDescription();

// Get OS version
WXDLLIMPEXP_BASE wxOperatingSystemId wxGetOsVersion(int *majorVsn = (int *) NULL,
                                                    int *minorVsn = (int *) NULL);

// Get platform endianness
WXDLLIMPEXP_BASE bool wxIsPlatformLittleEndian();

// Get platform architecture
WXDLLIMPEXP_BASE bool wxIsPlatform64Bit();

// Return a string with the current date/time
WXDLLIMPEXP_BASE wxString wxNow();

// Return path where wxWidgets is installed (mostly useful in Unices)
WXDLLIMPEXP_BASE const wxChar *wxGetInstallPrefix();
// Return path to wxWin data (/usr/share/wx/%{version}) (Unices)
WXDLLIMPEXP_BASE wxString wxGetDataDir();

/*
 * Class to make it easier to specify platform-dependent values
 *
 * Examples:
 *  long val = wxPlatform::If(wxMac, 1).ElseIf(wxGTK, 2).ElseIf(stPDA, 5).Else(3);
 *  wxString strVal = wxPlatform::If(wxMac, wxT("Mac")).ElseIf(wxMSW, wxT("MSW")).Else(wxT("Other"));
 *
 * A custom platform symbol:
 *
 *  #define stPDA 100
 *  #ifdef __WXWINCE__
 *      wxPlatform::AddPlatform(stPDA);
 *  #endif
 *
 *  long windowStyle = wxCAPTION | (long) wxPlatform::IfNot(stPDA, wxRESIZE_BORDER);
 *
 */

class WXDLLIMPEXP_BASE wxPlatform
{
public:
    wxPlatform() { Init(); }
    wxPlatform(const wxPlatform& platform) { Copy(platform); }
    void operator = (const wxPlatform& platform) { Copy(platform); }
    void Copy(const wxPlatform& platform);

    // Specify an optional default value
    wxPlatform(int defValue) { Init(); m_longValue = (long)defValue; }
    wxPlatform(long defValue) { Init(); m_longValue = defValue; }
    wxPlatform(const wxString& defValue) { Init(); m_stringValue = defValue; }
    wxPlatform(double defValue) { Init(); m_doubleValue = defValue; }

    static wxPlatform If(int platform, long value);
    static wxPlatform IfNot(int platform, long value);
    wxPlatform& ElseIf(int platform, long value);
    wxPlatform& ElseIfNot(int platform, long value);
    wxPlatform& Else(long value);

    static wxPlatform If(int platform, int value) { return If(platform, (long)value); }
    static wxPlatform IfNot(int platform, int value) { return IfNot(platform, (long)value); }
    wxPlatform& ElseIf(int platform, int value) { return ElseIf(platform, (long) value); }
    wxPlatform& ElseIfNot(int platform, int value) { return ElseIfNot(platform, (long) value); }
    wxPlatform& Else(int value) { return Else((long) value); }

    static wxPlatform If(int platform, double value);
    static wxPlatform IfNot(int platform, double value);
    wxPlatform& ElseIf(int platform, double value);
    wxPlatform& ElseIfNot(int platform, double value);
    wxPlatform& Else(double value);

    static wxPlatform If(int platform, const wxString& value);
    static wxPlatform IfNot(int platform, const wxString& value);
    wxPlatform& ElseIf(int platform, const wxString& value);
    wxPlatform& ElseIfNot(int platform, const wxString& value);
    wxPlatform& Else(const wxString& value);

    long GetInteger() const { return m_longValue; }
    const wxString& GetString() const { return m_stringValue; }
    double GetDouble() const { return m_doubleValue; }

    operator int() const { return (int) GetInteger(); }
    operator long() const { return GetInteger(); }
    operator double() const { return GetDouble(); }
    operator const wxString() const { return GetString(); }
    operator const wxChar*() const { return (const wxChar*) GetString(); }

    static void AddPlatform(int platform);
    static bool Is(int platform);
    static void ClearPlatforms();

private:

    void Init() { m_longValue = 0; m_doubleValue = 0.0; }

    long                m_longValue;
    double              m_doubleValue;
    wxString            m_stringValue;
    static wxArrayInt*  sm_customPlatforms;
};

/// Function for testing current platform
inline bool wxPlatformIs(int platform) { return wxPlatform::Is(platform); }

#if wxUSE_GUI

// Get the state of a key (true if pressed, false if not)
// This is generally most useful getting the state of
// the modifier or toggle keys.
WXDLLEXPORT bool wxGetKeyState(wxKeyCode key);


// Don't synthesize KeyUp events holding down a key and producing
// KeyDown events with autorepeat. On by default and always on
// in wxMSW.
WXDLLEXPORT bool wxSetDetectableAutoRepeat( bool flag );


// wxMouseState is used to hold information about button and modifier state
// and is what is returned from wxGetMouseState.
class WXDLLEXPORT wxMouseState
{
public:
    wxMouseState()
        : m_x(0), m_y(0),
          m_leftDown(false), m_middleDown(false), m_rightDown(false),
          m_controlDown(false), m_shiftDown(false), m_altDown(false),
          m_metaDown(false)
    {}

    wxCoord     GetX() { return m_x; }
    wxCoord     GetY() { return m_y; }

    bool        LeftDown()    { return m_leftDown; }
    bool        MiddleDown()  { return m_middleDown; }
    bool        RightDown()   { return m_rightDown; }

#if wxABI_VERSION >= 20811
    bool        LeftIsDown()    { return m_leftDown; }
    bool        MiddleIsDown()  { return m_middleDown; }
    bool        RightIsDown()   { return m_rightDown; }
#endif // wx >= 2.8.11

    bool        ControlDown() { return m_controlDown; }
    bool        ShiftDown()   { return m_shiftDown; }
    bool        AltDown()     { return m_altDown; }
    bool        MetaDown()    { return m_metaDown; }
    bool        CmdDown()
    {
#if defined(__WXMAC__) || defined(__WXCOCOA__)
        return MetaDown();
#else
        return ControlDown();
#endif
    }

    void        SetX(wxCoord x) { m_x = x; }
    void        SetY(wxCoord y) { m_y = y; }

    void        SetLeftDown(bool down)   { m_leftDown = down; }
    void        SetMiddleDown(bool down) { m_middleDown = down; }
    void        SetRightDown(bool down)  { m_rightDown = down; }

    void        SetControlDown(bool down) { m_controlDown = down; }
    void        SetShiftDown(bool down)   { m_shiftDown = down; }
    void        SetAltDown(bool down)     { m_altDown = down; }
    void        SetMetaDown(bool down)    { m_metaDown = down; }

private:
    wxCoord     m_x;
    wxCoord     m_y;

    bool        m_leftDown : 1;
    bool        m_middleDown : 1;
    bool        m_rightDown : 1;

    bool        m_controlDown : 1;
    bool        m_shiftDown : 1;
    bool        m_altDown : 1;
    bool        m_metaDown : 1;
};


// Returns the current state of the mouse position, buttons and modifers
WXDLLEXPORT wxMouseState wxGetMouseState();


// ----------------------------------------------------------------------------
// Window ID management
// ----------------------------------------------------------------------------

// Generate a unique ID
WXDLLEXPORT long wxNewId();

// Ensure subsequent IDs don't clash with this one
WXDLLEXPORT void wxRegisterId(long id);

// Return the current ID
WXDLLEXPORT long wxGetCurrentId();

#endif // wxUSE_GUI

// ----------------------------------------------------------------------------
// Various conversions
// ----------------------------------------------------------------------------

// these functions are deprecated, use wxString methods instead!
#if WXWIN_COMPATIBILITY_2_4

extern WXDLLIMPEXP_DATA_BASE(const wxChar*) wxFloatToStringStr;
extern WXDLLIMPEXP_DATA_BASE(const wxChar*) wxDoubleToStringStr;

wxDEPRECATED( WXDLLIMPEXP_BASE void StringToFloat(const wxChar *s, float *number) );
wxDEPRECATED( WXDLLIMPEXP_BASE wxChar* FloatToString(float number, const wxChar *fmt = wxFloatToStringStr) );
wxDEPRECATED( WXDLLIMPEXP_BASE void StringToDouble(const wxChar *s, double *number) );
wxDEPRECATED( WXDLLIMPEXP_BASE wxChar* DoubleToString(double number, const wxChar *fmt = wxDoubleToStringStr) );
wxDEPRECATED( WXDLLIMPEXP_BASE void StringToInt(const wxChar *s, int *number) );
wxDEPRECATED( WXDLLIMPEXP_BASE void StringToLong(const wxChar *s, long *number) );
wxDEPRECATED( WXDLLIMPEXP_BASE wxChar* IntToString(int number) );
wxDEPRECATED( WXDLLIMPEXP_BASE wxChar* LongToString(long number) );

#endif // WXWIN_COMPATIBILITY_2_4

// Convert 2-digit hex number to decimal
WXDLLIMPEXP_BASE int wxHexToDec(const wxString& buf);

// Convert decimal integer to 2-character hex string
WXDLLIMPEXP_BASE void wxDecToHex(int dec, wxChar *buf);
WXDLLIMPEXP_BASE wxString wxDecToHex(int dec);

// ----------------------------------------------------------------------------
// Process management
// ----------------------------------------------------------------------------

// NB: for backwards compatibility reasons the values of wxEXEC_[A]SYNC *must*
//     be 0 and 1, don't change!

enum
{
    // execute the process asynchronously
    wxEXEC_ASYNC    = 0,

    // execute it synchronously, i.e. wait until it finishes
    wxEXEC_SYNC     = 1,

    // under Windows, don't hide the child even if it's IO is redirected (this
    // is done by default)
    wxEXEC_NOHIDE   = 2,

    // under Unix, if the process is the group leader then passing wxKILL_CHILDREN to wxKill
    // kills all children as well as pid
    wxEXEC_MAKE_GROUP_LEADER = 4,

    // by default synchronous execution disables all program windows to avoid
    // that the user interacts with the program while the child process is
    // running, you can use this flag to prevent this from happening
    wxEXEC_NODISABLE = 8
};

// Execute another program.
//
// If flags contain wxEXEC_SYNC, return -1 on failure and the exit code of the
// process if everything was ok. Otherwise (i.e. if wxEXEC_ASYNC), return 0 on
// failure and the PID of the launched process if ok.
WXDLLIMPEXP_BASE long wxExecute(wxChar **argv, int flags = wxEXEC_ASYNC,
                           wxProcess *process = (wxProcess *) NULL);
WXDLLIMPEXP_BASE long wxExecute(const wxString& command, int flags = wxEXEC_ASYNC,
                           wxProcess *process = (wxProcess *) NULL);

// execute the command capturing its output into an array line by line, this is
// always synchronous
WXDLLIMPEXP_BASE long wxExecute(const wxString& command,
                                wxArrayString& output,
                                int flags = 0);

// also capture stderr (also synchronous)
WXDLLIMPEXP_BASE long wxExecute(const wxString& command,
                                wxArrayString& output,
                                wxArrayString& error,
                                int flags = 0);

#if defined(__WXMSW__) && wxUSE_IPC
// ask a DDE server to execute the DDE request with given parameters
WXDLLIMPEXP_BASE bool wxExecuteDDE(const wxString& ddeServer,
                                   const wxString& ddeTopic,
                                   const wxString& ddeCommand);
#endif // __WXMSW__ && wxUSE_IPC

enum wxSignal
{
    wxSIGNONE = 0,  // verify if the process exists under Unix
    wxSIGHUP,
    wxSIGINT,
    wxSIGQUIT,
    wxSIGILL,
    wxSIGTRAP,
    wxSIGABRT,
    wxSIGIOT = wxSIGABRT,   // another name
    wxSIGEMT,
    wxSIGFPE,
    wxSIGKILL,
    wxSIGBUS,
    wxSIGSEGV,
    wxSIGSYS,
    wxSIGPIPE,
    wxSIGALRM,
    wxSIGTERM

    // further signals are different in meaning between different Unix systems
};

enum wxKillError
{
    wxKILL_OK,              // no error
    wxKILL_BAD_SIGNAL,      // no such signal
    wxKILL_ACCESS_DENIED,   // permission denied
    wxKILL_NO_PROCESS,      // no such process
    wxKILL_ERROR            // another, unspecified error
};

enum wxKillFlags
{
    wxKILL_NOCHILDREN = 0,  // don't kill children
    wxKILL_CHILDREN = 1     // kill children
};

enum wxShutdownFlags
{
    wxSHUTDOWN_POWEROFF,    // power off the computer
    wxSHUTDOWN_REBOOT       // shutdown and reboot
};

// Shutdown or reboot the PC
WXDLLIMPEXP_BASE bool wxShutdown(wxShutdownFlags wFlags);

// send the given signal to the process (only NONE and KILL are supported under
// Windows, all others mean TERM), return 0 if ok and -1 on error
//
// return detailed error in rc if not NULL
WXDLLIMPEXP_BASE int wxKill(long pid,
                       wxSignal sig = wxSIGTERM,
                       wxKillError *rc = NULL,
                       int flags = wxKILL_NOCHILDREN);

// Execute a command in an interactive shell window (always synchronously)
// If no command then just the shell
WXDLLIMPEXP_BASE bool wxShell(const wxString& command = wxEmptyString);

// As wxShell(), but must give a (non interactive) command and its output will
// be returned in output array
WXDLLIMPEXP_BASE bool wxShell(const wxString& command, wxArrayString& output);

// Sleep for nSecs seconds
WXDLLIMPEXP_BASE void wxSleep(int nSecs);

// Sleep for a given amount of milliseconds
WXDLLIMPEXP_BASE void wxMilliSleep(unsigned long milliseconds);

// Sleep for a given amount of microseconds
WXDLLIMPEXP_BASE void wxMicroSleep(unsigned long microseconds);

// Sleep for a given amount of milliseconds (old, bad name), use wxMilliSleep
wxDEPRECATED( WXDLLIMPEXP_BASE void wxUsleep(unsigned long milliseconds) );

// Get the process id of the current process
WXDLLIMPEXP_BASE unsigned long wxGetProcessId();

// Get free memory in bytes, or -1 if cannot determine amount (e.g. on UNIX)
WXDLLIMPEXP_BASE wxMemorySize wxGetFreeMemory();

#if wxUSE_ON_FATAL_EXCEPTION

// should wxApp::OnFatalException() be called?
WXDLLIMPEXP_BASE bool wxHandleFatalExceptions(bool doit = true);

#endif // wxUSE_ON_FATAL_EXCEPTION

// flags for wxLaunchDefaultBrowser
enum
{
    wxBROWSER_NEW_WINDOW = 1
};

// Launch url in the user's default internet browser
WXDLLIMPEXP_BASE bool wxLaunchDefaultBrowser(const wxString& url, int flags = 0);

// ----------------------------------------------------------------------------
// Environment variables
// ----------------------------------------------------------------------------

// returns true if variable exists (value may be NULL if you just want to check
// for this)
WXDLLIMPEXP_BASE bool wxGetEnv(const wxString& var, wxString *value);

// set the env var name to the given value, return true on success
WXDLLIMPEXP_BASE bool wxSetEnv(const wxString& var, const wxChar *value);

// remove the env var from environment
inline bool wxUnsetEnv(const wxString& var) { return wxSetEnv(var, NULL); }

// ----------------------------------------------------------------------------
// Network and username functions.
// ----------------------------------------------------------------------------

// NB: "char *" functions are deprecated, use wxString ones!

// Get eMail address
WXDLLIMPEXP_BASE bool wxGetEmailAddress(wxChar *buf, int maxSize);
WXDLLIMPEXP_BASE wxString wxGetEmailAddress();

// Get hostname.
WXDLLIMPEXP_BASE bool wxGetHostName(wxChar *buf, int maxSize);
WXDLLIMPEXP_BASE wxString wxGetHostName();

// Get FQDN
WXDLLIMPEXP_BASE wxString wxGetFullHostName();
WXDLLIMPEXP_BASE bool wxGetFullHostName(wxChar *buf, int maxSize);

// Get user ID e.g. jacs (this is known as login name under Unix)
WXDLLIMPEXP_BASE bool wxGetUserId(wxChar *buf, int maxSize);
WXDLLIMPEXP_BASE wxString wxGetUserId();

// Get user name e.g. Julian Smart
WXDLLIMPEXP_BASE bool wxGetUserName(wxChar *buf, int maxSize);
WXDLLIMPEXP_BASE wxString wxGetUserName();

// Get current Home dir and copy to dest (returns pstr->c_str())
WXDLLIMPEXP_BASE wxString wxGetHomeDir();
WXDLLIMPEXP_BASE const wxChar* wxGetHomeDir(wxString *pstr);

// Get the user's home dir (caller must copy --- volatile)
// returns NULL is no HOME dir is known
#if defined(__UNIX__) && wxUSE_UNICODE && !defined(__WINE__)
WXDLLIMPEXP_BASE const wxMB2WXbuf wxGetUserHome(const wxString& user = wxEmptyString);
#else
WXDLLIMPEXP_BASE wxChar* wxGetUserHome(const wxString& user = wxEmptyString);
#endif

#if wxUSE_LONGLONG
    typedef wxLongLong wxDiskspaceSize_t;
#else
    typedef long wxDiskspaceSize_t;
#endif

// get number of total/free bytes on the disk where path belongs
WXDLLIMPEXP_BASE bool wxGetDiskSpace(const wxString& path,
                                     wxDiskspaceSize_t *pTotal = NULL,
                                     wxDiskspaceSize_t *pFree = NULL);

#if wxUSE_GUI // GUI only things from now on

// ----------------------------------------------------------------------------
// Menu accelerators related things
// ----------------------------------------------------------------------------

// flags for wxStripMenuCodes
enum
{
    // strip '&' characters
    wxStrip_Mnemonics = 1,

    // strip everything after '\t'
    wxStrip_Accel = 2,

    // strip everything (this is the default)
    wxStrip_All = wxStrip_Mnemonics | wxStrip_Accel
};

// strip mnemonics and/or accelerators from the label
WXDLLEXPORT wxString
wxStripMenuCodes(const wxString& str, int flags = wxStrip_All);

#if WXWIN_COMPATIBILITY_2_6
// obsolete and deprecated version, do not use, use the above overload instead
wxDEPRECATED(
    WXDLLEXPORT wxChar* wxStripMenuCodes(const wxChar *in, wxChar *out = NULL)
);

#if wxUSE_ACCEL
class WXDLLIMPEXP_FWD_CORE wxAcceleratorEntry;

// use wxAcceleratorEntry::Create() or FromString() methods instead
wxDEPRECATED(
    WXDLLEXPORT wxAcceleratorEntry *wxGetAccelFromString(const wxString& label)
);
#endif // wxUSE_ACCEL

#endif // WXWIN_COMPATIBILITY_2_6

// ----------------------------------------------------------------------------
// Window search
// ----------------------------------------------------------------------------

// Returns menu item id or wxNOT_FOUND if none.
WXDLLEXPORT int wxFindMenuItemId(wxFrame *frame, const wxString& menuString, const wxString& itemString);

// Find the wxWindow at the given point. wxGenericFindWindowAtPoint
// is always present but may be less reliable than a native version.
WXDLLEXPORT wxWindow* wxGenericFindWindowAtPoint(const wxPoint& pt);
WXDLLEXPORT wxWindow* wxFindWindowAtPoint(const wxPoint& pt);

// NB: this function is obsolete, use wxWindow::FindWindowByLabel() instead
//
// Find the window/widget with the given title or label.
// Pass a parent to begin the search from, or NULL to look through
// all windows.
WXDLLEXPORT wxWindow* wxFindWindowByLabel(const wxString& title, wxWindow *parent = (wxWindow *) NULL);

// NB: this function is obsolete, use wxWindow::FindWindowByName() instead
//
// Find window by name, and if that fails, by label.
WXDLLEXPORT wxWindow* wxFindWindowByName(const wxString& name, wxWindow *parent = (wxWindow *) NULL);

// ----------------------------------------------------------------------------
// Message/event queue helpers
// ----------------------------------------------------------------------------

// Yield to other apps/messages and disable user input
WXDLLEXPORT bool wxSafeYield(wxWindow *win = NULL, bool onlyIfNeeded = false);

// Enable or disable input to all top level windows
WXDLLEXPORT void wxEnableTopLevelWindows(bool enable = true);

// Check whether this window wants to process messages, e.g. Stop button
// in long calculations.
WXDLLEXPORT bool wxCheckForInterrupt(wxWindow *wnd);

// Consume all events until no more left
WXDLLEXPORT void wxFlushEvents();

// a class which disables all windows (except, may be, thegiven one) in its
// ctor and enables them back in its dtor
class WXDLLEXPORT wxWindowDisabler
{
public:
    wxWindowDisabler(wxWindow *winToSkip = (wxWindow *)NULL);
    ~wxWindowDisabler();

private:
    wxWindowList *m_winDisabled;

    DECLARE_NO_COPY_CLASS(wxWindowDisabler)
};

// ----------------------------------------------------------------------------
// Cursors
// ----------------------------------------------------------------------------

// Set the cursor to the busy cursor for all windows
WXDLLIMPEXP_CORE void wxBeginBusyCursor(const wxCursor *cursor = wxHOURGLASS_CURSOR);

// Restore cursor to normal
WXDLLEXPORT void wxEndBusyCursor();

// true if we're between the above two calls
WXDLLEXPORT bool wxIsBusy();

// Convenience class so we can just create a wxBusyCursor object on the stack
class WXDLLEXPORT wxBusyCursor
{
public:
    wxBusyCursor(const wxCursor* cursor = wxHOURGLASS_CURSOR)
        { wxBeginBusyCursor(cursor); }
    ~wxBusyCursor()
        { wxEndBusyCursor(); }

    // FIXME: These two methods are currently only implemented (and needed?)
    //        in wxGTK.  BusyCursor handling should probably be moved to
    //        common code since the wxGTK and wxMSW implementations are very
    //        similar except for wxMSW using HCURSOR directly instead of
    //        wxCursor..  -- RL.
    static const wxCursor &GetStoredCursor();
    static const wxCursor GetBusyCursor();
};


// ----------------------------------------------------------------------------
// Reading and writing resources (eg WIN.INI, .Xdefaults)
// ----------------------------------------------------------------------------

#if wxUSE_RESOURCES
WXDLLEXPORT bool wxWriteResource(const wxString& section, const wxString& entry, const wxString& value, const wxString& file = wxEmptyString);
WXDLLEXPORT bool wxWriteResource(const wxString& section, const wxString& entry, float value, const wxString& file = wxEmptyString);
WXDLLEXPORT bool wxWriteResource(const wxString& section, const wxString& entry, long value, const wxString& file = wxEmptyString);
WXDLLEXPORT bool wxWriteResource(const wxString& section, const wxString& entry, int value, const wxString& file = wxEmptyString);

WXDLLEXPORT bool wxGetResource(const wxString& section, const wxString& entry, wxChar **value, const wxString& file = wxEmptyString);
WXDLLEXPORT bool wxGetResource(const wxString& section, const wxString& entry, float *value, const wxString& file = wxEmptyString);
WXDLLEXPORT bool wxGetResource(const wxString& section, const wxString& entry, long *value, const wxString& file = wxEmptyString);
WXDLLEXPORT bool wxGetResource(const wxString& section, const wxString& entry, int *value, const wxString& file = wxEmptyString);
#endif // wxUSE_RESOURCES

void WXDLLEXPORT wxGetMousePosition( int* x, int* y );

// MSW only: get user-defined resource from the .res file.
// Returns NULL or newly-allocated memory, so use delete[] to clean up.
#ifdef __WXMSW__
    extern WXDLLEXPORT const wxChar* wxUserResourceStr;
    WXDLLEXPORT wxChar* wxLoadUserResource(const wxString& resourceName, const wxString& resourceType = wxUserResourceStr);
#endif // MSW

// ----------------------------------------------------------------------------
// Display and colorss (X only)
// ----------------------------------------------------------------------------

#ifdef __WXGTK__
    void *wxGetDisplay();
#endif

#ifdef __X__
    WXDLLIMPEXP_CORE WXDisplay *wxGetDisplay();
    WXDLLIMPEXP_CORE bool wxSetDisplay(const wxString& display_name);
    WXDLLIMPEXP_CORE wxString wxGetDisplayName();
#endif // X or GTK+

#ifdef __X__

#ifdef __VMS__ // Xlib.h for VMS is not (yet) compatible with C++
               // The resulting warnings are switched off here
#pragma message disable nosimpint
#endif
// #include <X11/Xlib.h>
#ifdef __VMS__
#pragma message enable nosimpint
#endif

#endif //__X__

#endif // wxUSE_GUI

// ----------------------------------------------------------------------------
// wxYield(): these functions are obsolete, please use wxApp methods instead!
// ----------------------------------------------------------------------------

// avoid redeclaring this function here if it had been already declated by
// wx/app.h, this results in warnings from g++ with -Wredundant-decls
#ifndef wx_YIELD_DECLARED
#define wx_YIELD_DECLARED

// Yield to other apps/messages
WXDLLIMPEXP_BASE bool wxYield();

#endif // wx_YIELD_DECLARED

// Like wxYield, but fails silently if the yield is recursive.
WXDLLIMPEXP_BASE bool wxYieldIfNeeded();

// ----------------------------------------------------------------------------
// Error message functions used by wxWidgets (deprecated, use wxLog)
// ----------------------------------------------------------------------------

#endif
    // _WX_UTILSH__
