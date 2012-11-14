// Name:        src/common/utilscmn.cpp
// Purpose:     Miscellaneous utility functions and classes
// Author:      Julian Smart
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id: utilscmn.cpp 66917 2011-02-16 21:51:31Z JS $
// Copyright:   (c) 1998 Julian Smart
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
    #include "wx/app.h"
    #include "wx/string.h"
    #include "wx/utils.h"
    #include "wx/intl.h"
    #include "wx/log.h"

    #if wxUSE_GUI
        #include "wx/window.h"
        #include "wx/frame.h"
        #include "wx/menu.h"
        #include "wx/msgdlg.h"
        #include "wx/textdlg.h"
        #include "wx/textctrl.h"    // for wxTE_PASSWORD
        #if wxUSE_ACCEL
            #include "wx/menuitem.h"
            #include "wx/accel.h"
        #endif // wxUSE_ACCEL
    #endif // wxUSE_GUI
#endif // WX_PRECOMP

#include "wx/apptrait.h"

#include "wx/process.h"
#include "wx/txtstrm.h"
#include "wx/uri.h"
#include "wx/mimetype.h"
#include "wx/config.h"

#if defined(__WXWINCE__) && wxUSE_DATETIME
#include "wx/datetime.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !wxONLY_WATCOM_EARLIER_THAN(1,4)
    #if !(defined(_MSC_VER) && (_MSC_VER > 800))
        #include <errno.h>
    #endif
#endif

#if wxUSE_GUI
    #include "wx/colordlg.h"
    #include "wx/fontdlg.h"
    #include "wx/notebook.h"
    #include "wx/statusbr.h"
#endif // wxUSE_GUI

#ifndef __WXWINCE__
#include <time.h>
#else
#include "wx/msw/wince/time.h"
#endif

#ifdef __WXMAC__
#include "wx/mac/private.h"
#ifndef __DARWIN__
#include "InternetConfig.h"
#endif
#endif

#if !defined(__MWERKS__) && !defined(__WXWINCE__)
    #include <sys/types.h>
    #include <sys/stat.h>
#endif

#if defined(__WXMSW__)
    #include "wx/msw/private.h"
    #include "wx/msw/registry.h"
    #include <shellapi.h> // needed for SHELLEXECUTEINFO
#endif

#if wxUSE_BASE

// ----------------------------------------------------------------------------
// common data
// ----------------------------------------------------------------------------

// ============================================================================
// implementation
// ============================================================================

#if WXWIN_COMPATIBILITY_2_4

wxChar *
copystring (const wxChar *s)
{
    if (s == NULL) s = wxEmptyString;
    size_t len = wxStrlen (s) + 1;

    wxChar *news = new wxChar[len];
    memcpy (news, s, len * sizeof(wxChar));    // Should be the fastest

    return news;
}

#endif // WXWIN_COMPATIBILITY_2_4

// ----------------------------------------------------------------------------
// String <-> Number conversions (deprecated)
// ----------------------------------------------------------------------------

#if WXWIN_COMPATIBILITY_2_4

WXDLLIMPEXP_DATA_BASE(const wxChar *) wxFloatToStringStr = wxT("%.2f");
WXDLLIMPEXP_DATA_BASE(const wxChar *) wxDoubleToStringStr = wxT("%.2f");

void
StringToFloat (const wxChar *s, float *number)
{
    if (s && *s && number)
        *number = (float) wxStrtod (s, (wxChar **) NULL);
}

void
StringToDouble (const wxChar *s, double *number)
{
    if (s && *s && number)
        *number = wxStrtod (s, (wxChar **) NULL);
}

wxChar *
FloatToString (float number, const wxChar *fmt)
{
    static wxChar buf[256];

    wxSprintf (buf, fmt, number);
    return buf;
}

wxChar *
DoubleToString (double number, const wxChar *fmt)
{
    static wxChar buf[256];

    wxSprintf (buf, fmt, number);
    return buf;
}

void
StringToInt (const wxChar *s, int *number)
{
    if (s && *s && number)
        *number = (int) wxStrtol (s, (wxChar **) NULL, 10);
}

void
StringToLong (const wxChar *s, long *number)
{
    if (s && *s && number)
        *number = wxStrtol (s, (wxChar **) NULL, 10);
}

wxChar *
IntToString (int number)
{
    static wxChar buf[20];

    wxSprintf (buf, wxT("%d"), number);
    return buf;
}

wxChar *
LongToString (long number)
{
    static wxChar buf[20];

    wxSprintf (buf, wxT("%ld"), number);
    return buf;
}

#endif // WXWIN_COMPATIBILITY_2_4

// Array used in DecToHex conversion routine.
static wxChar hexArray[] = wxT("0123456789ABCDEF");

// Convert 2-digit hex number to decimal
int wxHexToDec(const wxString& buf)
{
    int firstDigit, secondDigit;

    if (buf.GetChar(0) >= wxT('A'))
        firstDigit = buf.GetChar(0) - wxT('A') + 10;
    else
       firstDigit = buf.GetChar(0) - wxT('0');

    if (buf.GetChar(1) >= wxT('A'))
        secondDigit = buf.GetChar(1) - wxT('A') + 10;
    else
        secondDigit = buf.GetChar(1) - wxT('0');

    return (firstDigit & 0xF) * 16 + (secondDigit & 0xF );
}

// Convert decimal integer to 2-character hex string
void wxDecToHex(int dec, wxChar *buf)
{
    int firstDigit = (int)(dec/16.0);
    int secondDigit = (int)(dec - (firstDigit*16.0));
    buf[0] = hexArray[firstDigit];
    buf[1] = hexArray[secondDigit];
    buf[2] = 0;
}

// Convert decimal integer to 2-character hex string
wxString wxDecToHex(int dec)
{
    wxChar buf[3];
    wxDecToHex(dec, buf);
    return wxString(buf);
}

// ----------------------------------------------------------------------------
// misc functions
// ----------------------------------------------------------------------------

// Return the current date/time
wxString wxNow()
{
#ifdef __WXWINCE__
#if wxUSE_DATETIME
    wxDateTime now = wxDateTime::Now();
    return now.Format();
#else
    return wxEmptyString;
#endif
#else
    time_t now = time((time_t *) NULL);
    char *date = ctime(&now);
    date[24] = '\0';
    return wxString::FromAscii(date);
#endif
}

void wxUsleep(unsigned long milliseconds)
{
    wxMilliSleep(milliseconds);
}

const wxChar *wxGetInstallPrefix()
{
    wxString prefix;

    if ( wxGetEnv(wxT("WXPREFIX"), &prefix) )
        return prefix.c_str();

#ifdef wxINSTALL_PREFIX
    return wxT(wxINSTALL_PREFIX);
#else
    return wxEmptyString;
#endif
}

wxString wxGetDataDir()
{
    wxString dir = wxGetInstallPrefix();
    dir <<  wxFILE_SEP_PATH << wxT("share") << wxFILE_SEP_PATH << wxT("wx");
    return dir;
}

bool wxIsPlatformLittleEndian()
{
    // Are we little or big endian? This method is from Harbison & Steele.
    union
    {
        long l;
        char c[sizeof(long)];
    } u;
    u.l = 1;

    return u.c[0] == 1;
}


/*
 * Class to make it easier to specify platform-dependent values
 */

wxArrayInt*  wxPlatform::sm_customPlatforms = NULL;

void wxPlatform::Copy(const wxPlatform& platform)
{
    m_longValue = platform.m_longValue;
    m_doubleValue = platform.m_doubleValue;
    m_stringValue = platform.m_stringValue;
}

wxPlatform wxPlatform::If(int platform, long value)
{
    if (Is(platform))
        return wxPlatform(value);
    else
        return wxPlatform();
}

wxPlatform wxPlatform::IfNot(int platform, long value)
{
    if (!Is(platform))
        return wxPlatform(value);
    else
        return wxPlatform();
}

wxPlatform& wxPlatform::ElseIf(int platform, long value)
{
    if (Is(platform))
        m_longValue = value;
    return *this;
}

wxPlatform& wxPlatform::ElseIfNot(int platform, long value)
{
    if (!Is(platform))
        m_longValue = value;
    return *this;
}

wxPlatform wxPlatform::If(int platform, double value)
{
    if (Is(platform))
        return wxPlatform(value);
    else
        return wxPlatform();
}

wxPlatform wxPlatform::IfNot(int platform, double value)
{
    if (!Is(platform))
        return wxPlatform(value);
    else
        return wxPlatform();
}

wxPlatform& wxPlatform::ElseIf(int platform, double value)
{
    if (Is(platform))
        m_doubleValue = value;
    return *this;
}

wxPlatform& wxPlatform::ElseIfNot(int platform, double value)
{
    if (!Is(platform))
        m_doubleValue = value;
    return *this;
}

wxPlatform wxPlatform::If(int platform, const wxString& value)
{
    if (Is(platform))
        return wxPlatform(value);
    else
        return wxPlatform();
}

wxPlatform wxPlatform::IfNot(int platform, const wxString& value)
{
    if (!Is(platform))
        return wxPlatform(value);
    else
        return wxPlatform();
}

wxPlatform& wxPlatform::ElseIf(int platform, const wxString& value)
{
    if (Is(platform))
        m_stringValue = value;
    return *this;
}

wxPlatform& wxPlatform::ElseIfNot(int platform, const wxString& value)
{
    if (!Is(platform))
        m_stringValue = value;
    return *this;
}

wxPlatform& wxPlatform::Else(long value)
{
    m_longValue = value;
    return *this;
}

wxPlatform& wxPlatform::Else(double value)
{
    m_doubleValue = value;
    return *this;
}

wxPlatform& wxPlatform::Else(const wxString& value)
{
    m_stringValue = value;
    return *this;
}

void wxPlatform::AddPlatform(int platform)
{
    if (!sm_customPlatforms)
        sm_customPlatforms = new wxArrayInt;
    sm_customPlatforms->Add(platform);
}

void wxPlatform::ClearPlatforms()
{
    delete sm_customPlatforms;
    sm_customPlatforms = NULL;
}

/// Function for testing current platform

bool wxPlatform::Is(int platform)
{
#ifdef __WXMSW__
    if (platform == wxOS_WINDOWS)
        return true;
#endif
#ifdef __WXWINCE__
    if (platform == wxOS_WINDOWS_CE)
        return true;
#endif

#if 0

// FIXME: wxWinPocketPC and wxWinSmartPhone are unknown symbols

#if defined(__WXWINCE__) && defined(__POCKETPC__)
    if (platform == wxWinPocketPC)
        return true;
#endif
#if defined(__WXWINCE__) && defined(__SMARTPHONE__)
    if (platform == wxWinSmartPhone)
        return true;
#endif

#endif

#ifdef __WXGTK__
    if (platform == wxPORT_GTK)
        return true;
#endif
#ifdef __WXMAC__
    if (platform == wxPORT_MAC)
        return true;
#endif
#ifdef __WXX11__
    if (platform == wxPORT_X11)
        return true;
#endif
#ifdef __UNIX__
    if (platform == wxOS_UNIX)
        return true;
#endif
#ifdef __WXMGL__
    if (platform == wxPORT_MGL)
        return true;
#endif
#ifdef __OS2__
    if (platform == wxOS_OS2)
        return true;
#endif
#ifdef __WXPM__
    if (platform == wxPORT_PM)
        return true;
#endif
#ifdef __WXCOCOA__
    if (platform == wxPORT_MAC)
        return true;
#endif

    if (sm_customPlatforms && sm_customPlatforms->Index(platform) != wxNOT_FOUND)
        return true;

    return false;
}

// ----------------------------------------------------------------------------
// network and user id functions
// ----------------------------------------------------------------------------

// Get Full RFC822 style email address
bool wxGetEmailAddress(wxChar *address, int maxSize)
{
    wxString email = wxGetEmailAddress();
    if ( !email )
        return false;

    wxStrncpy(address, email, maxSize - 1);
    address[maxSize - 1] = wxT('\0');

    return true;
}

wxString wxGetEmailAddress()
{
    wxString email;

    wxString host = wxGetFullHostName();
    if ( !host.empty() )
    {
        wxString user = wxGetUserId();
        if ( !user.empty() )
        {
            email << user << wxT('@') << host;
        }
    }

    return email;
}

wxString wxGetUserId()
{
    static const int maxLoginLen = 256; // FIXME arbitrary number

    wxString buf;
    bool ok = wxGetUserId(wxStringBuffer(buf, maxLoginLen), maxLoginLen);

    if ( !ok )
        buf.Empty();

    return buf;
}

wxString wxGetUserName()
{
    static const int maxUserNameLen = 1024; // FIXME arbitrary number

    wxString buf;
    bool ok = wxGetUserName(wxStringBuffer(buf, maxUserNameLen), maxUserNameLen);

    if ( !ok )
        buf.Empty();

    return buf;
}

wxString wxGetHostName()
{
    static const size_t hostnameSize = 257;

    wxString buf;
    bool ok = wxGetHostName(wxStringBuffer(buf, hostnameSize), hostnameSize);

    if ( !ok )
        buf.Empty();

    return buf;
}

wxString wxGetFullHostName()
{
    static const size_t hostnameSize = 257;

    wxString buf;
    bool ok = wxGetFullHostName(wxStringBuffer(buf, hostnameSize), hostnameSize);

    if ( !ok )
        buf.Empty();

    return buf;
}

wxString wxGetHomeDir()
{
    wxString home;
    wxGetHomeDir(&home);

    return home;
}

#if 0

wxString wxGetCurrentDir()
{
    wxString dir;
    size_t len = 1024;
    bool ok;
    do
    {
        ok = getcwd(dir.GetWriteBuf(len + 1), len) != NULL;
        dir.UngetWriteBuf();

        if ( !ok )
        {
            if ( errno != ERANGE )
            {
                wxLogSysError(_T("Failed to get current directory"));

                return wxEmptyString;
            }
            else
            {
                // buffer was too small, retry with a larger one
                len *= 2;
            }
        }
        //else: ok
    } while ( !ok );

    return dir;
}

#endif // 0

// ----------------------------------------------------------------------------
// wxExecute
// ----------------------------------------------------------------------------

// wxDoExecuteWithCapture() helper: reads an entire stream into one array
//
// returns true if ok, false if error
#if wxUSE_STREAMS
static bool ReadAll(wxInputStream *is, wxArrayString& output)
{
    wxCHECK_MSG( is, false, _T("NULL stream in wxExecute()?") );

    // the stream could be already at EOF or in wxSTREAM_BROKEN_PIPE state
    is->Reset();

    wxTextInputStream tis(*is);

    for ( ;; )
    {
        wxString line = tis.ReadLine();

        // check for EOF before other errors as it's not really an error
        if ( is->Eof() )
        {
            // add the last, possibly incomplete, line
            if ( !line.empty() )
                output.Add(line);
            break;
        }

        // any other error is fatal
        if ( !*is )
            return false;

        output.Add(line);
    }

    return true;
}
#endif // wxUSE_STREAMS

// this is a private function because it hasn't a clean interface: the first
// array is passed by reference, the second by pointer - instead we have 2
// public versions of wxExecute() below
static long wxDoExecuteWithCapture(const wxString& command,
                                   wxArrayString& output,
                                   wxArrayString* error,
                                   int flags)
{
    // create a wxProcess which will capture the output
    wxProcess *process = new wxProcess;
    process->Redirect();

    long rc = wxExecute(command, wxEXEC_SYNC | flags, process);

#if wxUSE_STREAMS
    if ( rc != -1 )
    {
        if ( !ReadAll(process->GetInputStream(), output) )
            rc = -1;

        if ( error )
        {
            if ( !ReadAll(process->GetErrorStream(), *error) )
                rc = -1;
        }

    }
#else
    wxUnusedVar(output);
    wxUnusedVar(error);
#endif // wxUSE_STREAMS/!wxUSE_STREAMS

    delete process;

    return rc;
}

long wxExecute(const wxString& command, wxArrayString& output, int flags)
{
    return wxDoExecuteWithCapture(command, output, NULL, flags);
}

long wxExecute(const wxString& command,
               wxArrayString& output,
               wxArrayString& error,
               int flags)
{
    return wxDoExecuteWithCapture(command, output, &error, flags);
}

// ----------------------------------------------------------------------------
// Launch default browser
// ----------------------------------------------------------------------------

#include "wx/private/browserhack28.h"

static bool wxLaunchDefaultBrowserBaseImpl(const wxString& url, int flags);

// Use wxLaunchDefaultBrowserBaseImpl by default
static wxLaunchDefaultBrowserImpl_t s_launchBrowserImpl = &wxLaunchDefaultBrowserBaseImpl;

// Function the GUI library can call to provide a better implementation
WXDLLIMPEXP_BASE void wxSetLaunchDefaultBrowserImpl(wxLaunchDefaultBrowserImpl_t newImpl)
{
    s_launchBrowserImpl = newImpl!=NULL ? newImpl : &wxLaunchDefaultBrowserBaseImpl;
}

static bool wxLaunchDefaultBrowserBaseImpl(const wxString& url, int flags)
{
    wxUnusedVar(flags);

#if defined(__WXMSW__)

#if wxUSE_IPC
    if ( flags & wxBROWSER_NEW_WINDOW )
    {
        // ShellExecuteEx() opens the URL in an existing window by default so
        // we can't use it if we need a new window
        wxURI uri(url);
        wxRegKey key(wxRegKey::HKCR, uri.GetScheme() + _T("\\shell\\open"));
        if ( !key.Exists() )
        {
            // try default browser, it must be registered at least for http URLs
            key.SetName(wxRegKey::HKCR, _T("http\\shell\\open"));
        }

        if ( key.Exists() )
        {
            wxRegKey keyDDE(key, wxT("DDEExec"));
            if ( keyDDE.Exists() )
            {
                // we only know the syntax of WWW_OpenURL DDE request for IE,
                // optimistically assume that all other browsers are compatible
                // with it
                static const wxString TOPIC_OPEN_URL = wxT("WWW_OpenURL");
                wxString ddeCmd;
                wxRegKey keyTopic(keyDDE, wxT("topic"));
                bool ok = keyTopic.Exists() && (keyTopic.QueryDefaultValue() = TOPIC_OPEN_URL);
                if ( ok )
                {
                    ddeCmd = keyDDE.QueryDefaultValue();
                    ok = !ddeCmd.empty();
                }

                if ( ok )
                {
                    // for WWW_OpenURL, the index of the window to open the URL
                    // in is -1 (meaning "current") by default, replace it with
                    // 0 which means "new" (see KB article 160957)
                    ok = ddeCmd.Replace(wxT("-1"), wxT("0"),
                                        false /* only first occurence */) == 1;
                }

                if ( ok )
                {
                    // and also replace the parameters: the topic should
                    // contain a placeholder for the URL
                    ok = ddeCmd.Replace(wxT("%1"), url, false) == 1;
                }

                if ( ok )
                {
                    // try to send it the DDE request now but ignore the errors
                    wxLogNull noLog;

                    const wxString ddeServer = wxRegKey(keyDDE, wxT("application")).QueryDefaultValue();
                    if ( wxExecuteDDE(ddeServer, TOPIC_OPEN_URL, ddeCmd) )
                        return true;

                    // this is not necessarily an error: maybe browser is
                    // simply not running, but no matter, in any case we're
                    // going to launch it using ShellExecuteEx() below now and
                    // we shouldn't try to open a new window if we open a new
                    // browser anyhow
                }
            }
        }
    }
#endif // wxUSE_IPC

    WinStruct<SHELLEXECUTEINFO> sei;
    sei.lpFile = url.c_str();
    sei.lpVerb = _T("open");
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_FLAG_NO_UI; // we give error message ourselves

    BOOL nExecResult = ::ShellExecuteEx(&sei);

    //From MSDN for wince
    //hInstApp member is only valid if the function fails, in which case it
    //receives one of the following error values, which are less than or
    //equal to 32.
    const int nResult = (int) sei.hInstApp;

    // Firefox returns file not found for some reason, so make an exception
    // for it
    if ( nResult > 32 || nResult == SE_ERR_FNF  || nExecResult == TRUE )
    {
#ifdef __WXDEBUG__
        // Log something if SE_ERR_FNF happens
        if ( nResult == SE_ERR_FNF || nExecResult == FALSE )
            wxLogDebug(wxT("SE_ERR_FNF from ShellExecute -- maybe FireFox?"));
#endif // __WXDEBUG__
        return true;
    }
#elif defined(__WXMAC__)
    OSStatus err;
    ICInstance inst;
    long int startSel;
    long int endSel;

    err = ICStart(&inst, 'STKA'); // put your app creator code here
    if (err == noErr)
    {
#if !TARGET_CARBON
        err = ICFindConfigFile(inst, 0, NULL);
#endif
        if (err == noErr)
        {
            ConstStr255Param hint = 0;
            startSel = 0;
            endSel = url.length();
            err = ICLaunchURL(inst, hint, url.fn_str(), endSel, &startSel, &endSel);
            if (err != noErr)
                wxLogDebug(wxT("ICLaunchURL error %d"), (int) err);
        }
        ICStop(inst);
        return true;
    }
    else
    {
        wxLogDebug(wxT("ICStart error %d"), (int) err);
        return false;
    }
#else
    // (non-Mac, non-MSW)

#ifdef __UNIX__

    // Our best best is to use xdg-open from freedesktop.org cross-desktop
    // compatibility suite xdg-utils
    // (see http://portland.freedesktop.org/wiki/) -- this is installed on
    // most modern distributions and may be tweaked by them to handle
    // distribution specifics. Only if that fails, try to find the right
    // browser ourselves.
    wxString path, xdg_open;
    if ( wxGetEnv(_T("PATH"), &path) &&
         wxFindFileInPath(&xdg_open, path, _T("xdg-open")) )
    {
        if ( wxExecute(xdg_open + _T(" ") + url) )
            return true;
    }

    wxString desktop = wxTheApp->GetTraits()->GetDesktopEnvironment();

    // GNOME and KDE desktops have some applications which should be always installed
    // together with their main parts, which give us the
    if (desktop == wxT("GNOME"))
    {
        wxArrayString errors;
        wxArrayString output;

        // gconf will tell us the path of the application to use as browser
        long res = wxExecute( wxT("gconftool-2 --get /desktop/gnome/applications/browser/exec"),
                              output, errors, wxEXEC_NODISABLE );
        if (res >= 0 && errors.GetCount() == 0)
        {
            wxString cmd = output[0];
            cmd << _T(' ') << url;
            if (wxExecute(cmd))
                return true;
        }
    }
    else if (desktop == wxT("KDE"))
    {
        // kfmclient directly opens the given URL
        if (wxExecute(wxT("kfmclient openURL ") + url))
            return true;
    }
#endif

    bool ok = false;
    wxString cmd;

#if wxUSE_MIMETYPE
    wxFileType *ft = wxTheMimeTypesManager->GetFileTypeFromExtension(_T("html"));
    if ( ft )
    {
        wxString mt;
        ft->GetMimeType(&mt);

        ok = ft->GetOpenCommand(&cmd, wxFileType::MessageParameters(url));
        delete ft;
    }
#endif // wxUSE_MIMETYPE

    if ( !ok || cmd.empty() )
    {
        // fallback to checking for the BROWSER environment variable
        cmd = wxGetenv(wxT("BROWSER"));
        if ( !cmd.empty() )
            cmd << _T(' ') << url;
    }

    ok = ( !cmd.empty() && wxExecute(cmd) );
    if (ok)
        return ok;

    // no file type for HTML extension
    wxLogError(_T("No default application configured for HTML files."));

#endif // !wxUSE_MIMETYPE && !__WXMSW__
    return false;
}

bool wxLaunchDefaultBrowser(const wxString& urlOrig, int flags)
{
    // set the scheme of url to http if it does not have one
    // RR: This doesn't work if the url is just a local path
    wxString url(urlOrig);
    wxURI uri(url);
    if ( !uri.HasScheme() )
    {
        if (wxFileExists(urlOrig))
            url.Prepend( wxT("file://") );
        else
            url.Prepend(wxT("http://"));
    }

    if(s_launchBrowserImpl(url, flags))
        return true;

    wxLogSysError(_T("Failed to open URL \"%s\" in default browser."),
                  url.c_str());

    return false;
}

// ----------------------------------------------------------------------------
// wxApp::Yield() wrappers for backwards compatibility
// ----------------------------------------------------------------------------

bool wxYield()
{
    return wxTheApp && wxTheApp->Yield();
}

bool wxYieldIfNeeded()
{
    return wxTheApp && wxTheApp->Yield(true);
}

#endif // wxUSE_BASE

// ============================================================================
// GUI-only functions from now on
// ============================================================================

#if wxUSE_GUI

// Id generation
static long wxCurrentId = 100;

long wxNewId()
{
    // skip the part of IDs space that contains hard-coded values:
    if (wxCurrentId == wxID_LOWEST)
        wxCurrentId = wxID_HIGHEST + 1;

    return wxCurrentId++;
}

long
wxGetCurrentId(void) { return wxCurrentId; }

void
wxRegisterId (long id)
{
  if (id >= wxCurrentId)
    wxCurrentId = id + 1;
}

// ----------------------------------------------------------------------------
// Menu accelerators related functions
// ----------------------------------------------------------------------------

wxChar *wxStripMenuCodes(const wxChar *in, wxChar *out)
{
#if wxUSE_MENUS
    wxString s = wxMenuItem::GetLabelFromText(in);
#else
    wxString str(in);
    wxString s = wxStripMenuCodes(str);
#endif // wxUSE_MENUS
    if ( out )
    {
        // go smash their buffer if it's not big enough - I love char * params
        memcpy(out, s.c_str(), s.length() * sizeof(wxChar));
    }
    else
    {
        // MYcopystring - for easier search...
        out = new wxChar[s.length() + 1];
        wxStrcpy(out, s.c_str());
    }

    return out;
}

wxString wxStripMenuCodes(const wxString& in, int flags)
{
    wxASSERT_MSG( flags, _T("this is useless to call without any flags") );

    wxString out;

    size_t len = in.length();
    out.reserve(len);

    for ( size_t n = 0; n < len; n++ )
    {
        wxChar ch = in[n];
        if ( (flags & wxStrip_Mnemonics) && ch == _T('&') )
        {
            // skip it, it is used to introduce the accel char (or to quote
            // itself in which case it should still be skipped): note that it
            // can't be the last character of the string
            if ( ++n == len )
            {
                wxLogDebug(_T("Invalid menu string '%s'"), in.c_str());
            }
            else
            {
                // use the next char instead
                ch = in[n];
            }
        }
        else if ( (flags & wxStrip_Accel) && ch == _T('\t') )
        {
            // everything after TAB is accel string, exit the loop
            break;
        }

        out += ch;
    }

    return out;
}

// ----------------------------------------------------------------------------
// Window search functions
// ----------------------------------------------------------------------------

/*
 * If parent is non-NULL, look through children for a label or title
 * matching the specified string. If NULL, look through all top-level windows.
 *
 */

wxWindow *
wxFindWindowByLabel (const wxString& title, wxWindow * parent)
{
    return wxWindow::FindWindowByLabel( title, parent );
}


/*
 * If parent is non-NULL, look through children for a name
 * matching the specified string. If NULL, look through all top-level windows.
 *
 */

wxWindow *
wxFindWindowByName (const wxString& name, wxWindow * parent)
{
    return wxWindow::FindWindowByName( name, parent );
}

// Returns menu item id or wxNOT_FOUND if none.
int
wxFindMenuItemId (wxFrame * frame, const wxString& menuString, const wxString& itemString)
{
#if wxUSE_MENUS
    wxMenuBar *menuBar = frame->GetMenuBar ();
    if ( menuBar )
        return menuBar->FindMenuItem (menuString, itemString);
#endif // wxUSE_MENUS

    return wxNOT_FOUND;
}

// Try to find the deepest child that contains 'pt'.
// We go backwards, to try to allow for controls that are spacially
// within other controls, but are still siblings (e.g. buttons within
// static boxes). Static boxes are likely to be created _before_ controls
// that sit inside them.
wxWindow* wxFindWindowAtPoint(wxWindow* win, const wxPoint& pt)
{
    if (!win->IsShown())
        return NULL;

    // Hack for wxNotebook case: at least in wxGTK, all pages
    // claim to be shown, so we must only deal with the selected one.
#if wxUSE_NOTEBOOK
    if (win->IsKindOf(CLASSINFO(wxNotebook)))
    {
      wxNotebook* nb = (wxNotebook*) win;
      int sel = nb->GetSelection();
      if (sel >= 0)
      {
        wxWindow* child = nb->GetPage(sel);
        wxWindow* foundWin = wxFindWindowAtPoint(child, pt);
        if (foundWin)
           return foundWin;
      }
    }
#endif

    wxWindowList::compatibility_iterator node = win->GetChildren().GetLast();
    while (node)
    {
        wxWindow* child = node->GetData();
        wxWindow* foundWin = wxFindWindowAtPoint(child, pt);
        if (foundWin)
          return foundWin;
        node = node->GetPrevious();
    }

    wxPoint pos = win->GetPosition();
    wxSize sz = win->GetSize();
    if ( !win->IsTopLevel() && win->GetParent() )
    {
        pos = win->GetParent()->ClientToScreen(pos);
    }

    wxRect rect(pos, sz);
    if (rect.Contains(pt))
        return win;

    return NULL;
}

wxWindow* wxGenericFindWindowAtPoint(const wxPoint& pt)
{
    // Go backwards through the list since windows
    // on top are likely to have been appended most
    // recently.
    wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetLast();
    while (node)
    {
        wxWindow* win = node->GetData();
        wxWindow* found = wxFindWindowAtPoint(win, pt);
        if (found)
            return found;
        node = node->GetPrevious();
    }
    return NULL;
}

// ----------------------------------------------------------------------------
// GUI helpers
// ----------------------------------------------------------------------------

/*
 * N.B. these convenience functions must be separate from msgdlgg.cpp, textdlgg.cpp
 * since otherwise the generic code may be pulled in unnecessarily.
 */

#if wxUSE_MSGDLG

int wxMessageBox(const wxString& message, const wxString& caption, long style,
                 wxWindow *parent, int WXUNUSED(x), int WXUNUSED(y) )
{
    long decorated_style = style;

    if ( ( style & ( wxICON_EXCLAMATION | wxICON_HAND | wxICON_INFORMATION | wxICON_QUESTION ) ) == 0 )
    {
        decorated_style |= ( style & wxYES ) ? wxICON_QUESTION : wxICON_INFORMATION ;
    }

    wxMessageDialog dialog(parent, message, caption, decorated_style);

    int ans = dialog.ShowModal();
    switch ( ans )
    {
        case wxID_OK:
            return wxOK;
        case wxID_YES:
            return wxYES;
        case wxID_NO:
            return wxNO;
        case wxID_CANCEL:
            return wxCANCEL;
    }

    wxFAIL_MSG( _T("unexpected return code from wxMessageDialog") );

    return wxCANCEL;
}

#endif // wxUSE_MSGDLG

#if wxUSE_TEXTDLG

wxString wxGetTextFromUser(const wxString& message, const wxString& caption,
                        const wxString& defaultValue, wxWindow *parent,
                        wxCoord x, wxCoord y, bool centre )
{
    wxString str;
    long style = wxTextEntryDialogStyle;

    if (centre)
        style |= wxCENTRE;
    else
        style &= ~wxCENTRE;

    wxTextEntryDialog dialog(parent, message, caption, defaultValue, style, wxPoint(x, y));

    if (dialog.ShowModal() == wxID_OK)
    {
        str = dialog.GetValue();
    }

    return str;
}

wxString wxGetPasswordFromUser(const wxString& message,
                               const wxString& caption,
                               const wxString& defaultValue,
                               wxWindow *parent,
                               wxCoord x, wxCoord y, bool centre )
{
    wxString str;
    long style = wxTextEntryDialogStyle;

    if (centre)
        style |= wxCENTRE;
    else
        style &= ~wxCENTRE;

    wxPasswordEntryDialog dialog(parent, message, caption, defaultValue,
                             style, wxPoint(x, y));
    if ( dialog.ShowModal() == wxID_OK )
    {
        str = dialog.GetValue();
    }

    return str;
}

#endif // wxUSE_TEXTDLG

#if wxUSE_COLOURDLG

wxColour wxGetColourFromUser(wxWindow *parent, const wxColour& colInit, const wxString& caption)
{
    static wxColourData data;
    data.SetChooseFull(true);
    if ( colInit.Ok() )
    {
        data.SetColour((wxColour &)colInit); // const_cast
    }

    wxColour colRet;
    wxColourDialog dialog(parent, &data);
    if (!caption.empty())
        dialog.SetTitle(caption);
    if ( dialog.ShowModal() == wxID_OK )
    {
        colRet = dialog.GetColourData().GetColour();
    }
    //else: leave it invalid

    return colRet;
}

#endif // wxUSE_COLOURDLG

#if wxUSE_FONTDLG

wxFont wxGetFontFromUser(wxWindow *parent, const wxFont& fontInit, const wxString& caption)
{
    wxFontData data;
    if ( fontInit.Ok() )
    {
        data.SetInitialFont(fontInit);
    }

    wxFont fontRet;
    wxFontDialog dialog(parent, data);
    if (!caption.empty())
        dialog.SetTitle(caption);
    if ( dialog.ShowModal() == wxID_OK )
    {
        fontRet = dialog.GetFontData().GetChosenFont();
    }
    //else: leave it invalid

    return fontRet;
}

#endif // wxUSE_FONTDLG

// ----------------------------------------------------------------------------
// wxSafeYield and supporting functions
// ----------------------------------------------------------------------------

void wxEnableTopLevelWindows(bool enable)
{
    wxWindowList::compatibility_iterator node;
    for ( node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext() )
        node->GetData()->Enable(enable);
}

wxWindowDisabler::wxWindowDisabler(wxWindow *winToSkip)
{
    // remember the top level windows which were already disabled, so that we
    // don't reenable them later
    m_winDisabled = NULL;

    wxWindowList::compatibility_iterator node;
    for ( node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext() )
    {
        wxWindow *winTop = node->GetData();
        if ( winTop == winToSkip )
            continue;

        // we don't need to disable the hidden or already disabled windows
        if ( winTop->IsEnabled() && winTop->IsShown() )
        {
            winTop->Disable();
        }
        else
        {
            if ( !m_winDisabled )
            {
                m_winDisabled = new wxWindowList;
            }

            m_winDisabled->Append(winTop);
        }
    }
}

wxWindowDisabler::~wxWindowDisabler()
{
    wxWindowList::compatibility_iterator node;
    for ( node = wxTopLevelWindows.GetFirst(); node; node = node->GetNext() )
    {
        wxWindow *winTop = node->GetData();
        if ( !m_winDisabled || !m_winDisabled->Find(winTop) )
        {
            winTop->Enable();
        }
        //else: had been already disabled, don't reenable
    }

    delete m_winDisabled;
}

// Yield to other apps/messages and disable user input to all windows except
// the given one
bool wxSafeYield(wxWindow *win, bool onlyIfNeeded)
{
    wxWindowDisabler wd(win);

    bool rc;
    if (onlyIfNeeded)
        rc = wxYieldIfNeeded();
    else
        rc = wxYield();

    return rc;
}

// Don't synthesize KeyUp events holding down a key and producing KeyDown
// events with autorepeat. On by default and always on in wxMSW. wxGTK version
// in utilsgtk.cpp.
#ifndef __WXGTK__
bool wxSetDetectableAutoRepeat( bool WXUNUSED(flag) )
{
    return true;    // detectable auto-repeat is the only mode MSW supports
}
#endif // !wxGTK

#endif // wxUSE_GUI
