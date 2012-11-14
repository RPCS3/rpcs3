// -*- c++ -*- ////////////////////////////////////////////////////////////////
// Name:        src/unix/dialup.cpp
// Purpose:     Network related wxWidgets classes and functions
// Author:      Karsten Ballüder
// Modified by:
// Created:     03.10.99
// RCS-ID:      $Id: dialup.cpp 41020 2006-09-05 20:47:48Z VZ $
// Copyright:   (c) Karsten Ballüder
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_DIALUP_MANAGER

#include "wx/dialup.h"

#ifndef  WX_PRECOMP
    #include "wx/string.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/event.h"
    #include "wx/app.h"
    #include "wx/utils.h"
    #include "wx/timer.h"
#endif // !PCH

#include "wx/filefn.h"
#include "wx/ffile.h"
#include "wx/process.h"
#include "wx/wxchar.h"

#include <stdlib.h>

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#define __STRICT_ANSI__
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

DEFINE_EVENT_TYPE(wxEVT_DIALUP_CONNECTED)
DEFINE_EVENT_TYPE(wxEVT_DIALUP_DISCONNECTED)

// ----------------------------------------------------------------------------
// A class which groups functions dealing with connecting to the network from a
// workstation using dial-up access to the net. There is at most one instance
// of this class in the program accessed via GetDialUpManager().
// ----------------------------------------------------------------------------

/* TODO
 *
 * 1. more configurability for Unix: i.e. how to initiate the connection, how
 *    to check for online status, &c.
 * 2. add a "long Dial(long connectionId = -1)" function which asks the user
 *    about which connection to dial (this may be done using native dialogs
 *    under NT, need generic dialogs for all others) and returns the identifier
 *    of the selected connection (it's opaque to the application) - it may be
 *    reused later to dial the same connection later (or use strings instead of
 *    longs may be?)
 * 3. add an async version of dialing functions which notify the caller about
 *    the progress (or may be even start another thread to monitor it)
 * 4. the static creation/accessor functions are not MT-safe - but is this
 *    really crucial? I think we may suppose they're always called from the
 *    main thread?
 */

class WXDLLEXPORT wxDialUpManagerImpl : public wxDialUpManager
{
public:
   wxDialUpManagerImpl();
   virtual ~wxDialUpManagerImpl();

   /** Could the dialup manager be initialized correctly? If this function
       returns false, no other functions will work neither, so it's a good idea
       to call this function and check its result before calling any other
       wxDialUpManager methods.
   */
   virtual bool IsOk() const
      { return true; }

   /** The simplest way to initiate a dial up: this function dials the given
       ISP (exact meaning of the parameter depends on the platform), returns
       true on success or false on failure and logs the appropriate error
       message in the latter case.
       @param nameOfISP optional paramater for dial program
       @param username unused
       @param password unused
   */
   virtual bool Dial(const wxString& nameOfISP,
                     const wxString& WXUNUSED(username),
                     const wxString& WXUNUSED(password),
                     bool async);

   // Hang up the currently active dial up connection.
   virtual bool HangUp();

   // returns true if the computer is connected to the network: under Windows,
   // this just means that a RAS connection exists, under Unix we check that
   // the "well-known host" (as specified by SetWellKnownHost) is reachable
   virtual bool IsOnline() const
      {
         CheckStatus();
         return m_IsOnline == Net_Connected;
      }

   // do we have a constant net connection?
   virtual bool IsAlwaysOnline() const;

   // returns true if (async) dialing is in progress
   virtual bool IsDialing() const
      { return m_DialProcess != NULL; }

   // cancel dialing the number initiated with Dial(async = true)
   // NB: this won't result in DISCONNECTED event being sent
   virtual bool CancelDialing();

   size_t GetISPNames(class wxArrayString &) const
      { return 0; }

   // sometimes the built-in logic for determining the online status may fail,
   // so, in general, the user should be allowed to override it. This function
   // allows to forcefully set the online status - whatever our internal
   // algorithm may think about it.
   virtual void SetOnlineStatus(bool isOnline = true)
      { m_IsOnline = isOnline ? Net_Connected : Net_No; }

   // set misc wxDialUpManager options
   // --------------------------------

   // enable automatical checks for the connection status and sending of
   // wxEVT_DIALUP_CONNECTED/wxEVT_DIALUP_DISCONNECTED events. The interval
   // parameter is only for Unix where we do the check manually: under
   // Windows, the notification about the change of connection status is
   // instantenous.
   //
   // Returns false if couldn't set up automatic check for online status.
   virtual bool EnableAutoCheckOnlineStatus(size_t nSeconds);

   // disable automatic check for connection status change - notice that the
   // wxEVT_DIALUP_XXX events won't be sent any more neither.
   virtual void DisableAutoCheckOnlineStatus();

   // under Unix, the value of well-known host is used to check whether we're
   // connected to the internet. It's unused under Windows, but this function
   // is always safe to call. The default value is www.yahoo.com.
   virtual void SetWellKnownHost(const wxString& hostname,
                                 int portno = 80);
   /** Sets the commands to start up the network and to hang up
       again. Used by the Unix implementations only.
   */
   virtual void SetConnectCommand(const wxString &command, const wxString &hupcmd)
      { m_ConnectCommand = command; m_HangUpCommand = hupcmd; }

//private: -- Sun CC 4.2 objects to using NetConnection enum as the return
//            type if it is declared private

   // the possible results of testing for Online() status
   enum NetConnection
   {
       Net_Unknown = -1,    // we couldn't learn anything
       Net_No,              // no network connection [currently]
       Net_Connected        // currently connected
   };

   // the possible net connection types
   enum NetDeviceType
   {
       NetDevice_None    = 0x0000,  // no network devices (authoritative)
       NetDevice_Unknown = 0x0001,  // test doesn't work on this OS
       NetDevice_Modem   = 0x0002,  // we have a modem
       NetDevice_LAN     = 0x0004   //         a network card
   };

private:
   // the current status
   NetConnection m_IsOnline;

   // the connection we have with the network card
   NetConnection m_connCard;

   // Can we use ifconfig to list active devices?
   int m_CanUseIfconfig;

   // The path to ifconfig
   wxString m_IfconfigPath;

   //  Can we use ping to find hosts?
   int m_CanUsePing;
   // The path to ping program
   wxString m_PingPath;

   // beacon host:
   wxString m_BeaconHost;
   // beacon host portnumber for connect:
   int m_BeaconPort;

   // command to connect to network
   wxString m_ConnectCommand;
   // command to hang up
   wxString m_HangUpCommand;
   // name of ISP
   wxString m_ISPname;
   // a timer for regular testing
   class AutoCheckTimer *m_timer;
   friend class AutoCheckTimer;

   // a wxProcess for dialling in background
   class wxDialProcess *m_DialProcess;
   // pid of dial process
   int m_DialPId;
   friend class wxDialProcess;

   // determine status
   void CheckStatus(bool fromAsync = false) const;

   // real status check
   void CheckStatusInternal();

   // check /proc/net (Linux only) for ppp/eth interfaces, returns the bit
   // mask of NetDeviceType constants
   int CheckProcNet();

   // check output of ifconfig command for PPP/SLIP/PLIP devices, returns the
   // bit mask of NetDeviceType constants
   int CheckIfconfig();

   // combines the 2 possible checks for determining the connection status
   NetConnection CheckConnectAndPing();

   // pings a host
   NetConnection CheckPing();

   // check by connecting to host on given port.
   NetConnection CheckConnect();
};


class AutoCheckTimer : public wxTimer
{
public:
   AutoCheckTimer(wxDialUpManagerImpl *dupman)
   {
       m_dupman = dupman;
   }

   virtual void Notify()
   {
       wxLogTrace(_T("dialup"), wxT("Checking dial up network status."));

       m_dupman->CheckStatus();
   }

public:
   wxDialUpManagerImpl *m_dupman;
};

class wxDialProcess : public wxProcess
{
public:
   wxDialProcess(wxDialUpManagerImpl *dupman)
      {
         m_DupMan = dupman;
      }
   void Disconnect() { m_DupMan = NULL; }
   virtual void OnTerminate(int WXUNUSED(pid), int WXUNUSED(status))
      {
         if(m_DupMan)
         {
            m_DupMan->m_DialProcess = NULL;
            m_DupMan->CheckStatus(true);
         }
      }
private:
      wxDialUpManagerImpl *m_DupMan;
};


wxDialUpManagerImpl::wxDialUpManagerImpl()
{
   m_IsOnline =
   m_connCard = Net_Unknown;
   m_DialProcess = NULL;
   m_timer = NULL;
   m_CanUseIfconfig = -1; // unknown
   m_CanUsePing = -1; // unknown
   m_BeaconHost = WXDIALUP_MANAGER_DEFAULT_BEACONHOST;
   m_BeaconPort = 80;

#ifdef __SGI__
   m_ConnectCommand = _T("/usr/etc/ppp");
#elif defined(__LINUX__)
   // default values for Debian/GNU linux
   m_ConnectCommand = _T("pon");
   m_HangUpCommand = _T("poff");
#endif

   wxChar * dial = wxGetenv(_T("WXDIALUP_DIALCMD"));
   wxChar * hup = wxGetenv(_T("WXDIALUP_HUPCMD"));
   SetConnectCommand(dial ? wxString(dial) : m_ConnectCommand,
                     hup ? wxString(hup) : m_HangUpCommand);
}

wxDialUpManagerImpl::~wxDialUpManagerImpl()
{
   if(m_timer) delete m_timer;
   if(m_DialProcess)
   {
      m_DialProcess->Disconnect();
      m_DialProcess->Detach();
   }
}

bool
wxDialUpManagerImpl::Dial(const wxString &isp,
                          const wxString & WXUNUSED(username),
                          const wxString & WXUNUSED(password),
                          bool async)
{
    if(m_IsOnline == Net_Connected)
        return false;
    m_ISPname = isp;
    wxString cmd;
    if(m_ConnectCommand.Find(wxT("%s")))
        cmd.Printf(m_ConnectCommand,m_ISPname.c_str());
    else
        cmd = m_ConnectCommand;

    if ( async )
    {
        m_DialProcess = new wxDialProcess(this);
        m_DialPId = (int)wxExecute(cmd, false, m_DialProcess);
        if(m_DialPId == 0)
        {
            delete m_DialProcess;
            m_DialProcess = NULL;
            return false;
        }
        else
            return true;
    }
    else
        return wxExecute(cmd, /* sync */ true) == 0;
}

bool wxDialUpManagerImpl::HangUp()
{
    if(m_IsOnline == Net_No)
        return false;
    if(IsDialing())
    {
        wxLogError(_("Already dialling ISP."));
        return false;
    }
    wxString cmd;
    if(m_HangUpCommand.Find(wxT("%s")))
        cmd.Printf(m_HangUpCommand,m_ISPname.c_str(), m_DialProcess);
    else
        cmd = m_HangUpCommand;
    return wxExecute(cmd, /* sync */ true) == 0;
}


bool wxDialUpManagerImpl::CancelDialing()
{
   if(! IsDialing())
      return false;
   return kill(m_DialPId, SIGTERM) > 0;
}

bool wxDialUpManagerImpl::EnableAutoCheckOnlineStatus(size_t nSeconds)
{
   DisableAutoCheckOnlineStatus();
   m_timer = new AutoCheckTimer(this);
   bool rc = m_timer->Start(nSeconds*1000);
   if(! rc)
   {
      delete m_timer;
      m_timer = NULL;
   }
   return rc;
}

void wxDialUpManagerImpl::DisableAutoCheckOnlineStatus()
{
   if(m_timer != NULL)
   {
      m_timer->Stop();
      delete m_timer;
      m_timer = NULL;
   }
}


void wxDialUpManagerImpl::SetWellKnownHost(const wxString& hostname, int portno)
{
   if(hostname.length() == 0)
   {
      m_BeaconHost = WXDIALUP_MANAGER_DEFAULT_BEACONHOST;
      m_BeaconPort = 80;
      return;
   }

   // does hostname contain a port number?
   wxString port = hostname.After(wxT(':'));
   if(port.length())
   {
      m_BeaconHost = hostname.Before(wxT(':'));
      m_BeaconPort = wxAtoi(port);
   }
   else
   {
      m_BeaconHost = hostname;
      m_BeaconPort = portno;
   }
}


void wxDialUpManagerImpl::CheckStatus(bool fromAsync) const
{
    // This function calls the CheckStatusInternal() helper function
    // which is OS - specific and then sends the events.

    NetConnection oldIsOnline = m_IsOnline;
    ( /* non-const */ (wxDialUpManagerImpl *)this)->CheckStatusInternal();

    // now send the events as appropriate: i.e. if the status changed and
    // if we're in defined state
    if(m_IsOnline != oldIsOnline
            && m_IsOnline != Net_Unknown
            && oldIsOnline != Net_Unknown )
    {
        wxDialUpEvent event(m_IsOnline == Net_Connected, ! fromAsync);
        (void)wxTheApp->ProcessEvent(event);
    }
}

/*
   We first try to find out if ppp interface is active. If it is, we assume
   that we're online but don't have a permanent connection (this is false if a
   networked machine uses modem to connect to somewhere else, but we can't do
   anything in this case anyhow).

   If no ppp interface is detected, we check for eth interface. If it is
   found, we check that we can, indeed, connect to an Internet host. The logic
   here is that connection check should be fast enough in this case and we
   don't want to give false positives in a (common) case of a machine on a LAN
   which is not connected to the outside.

   If we didn't find either ppp or eth interfaces, we stop here and decide
   that we're connected. However, if couldn't check for this, we try to ping a
   remote host just in case.

   NB1: Checking for the interface presence can be done in 2 ways
        a) reading /proc/net/dev under Linux
        b) spawning ifconfig under any OS

        The first method is faster but only works under Linux.

   NB2: pinging, actually, means that we first try to connect "manually" to
        a port on remove machine and if it fails, we run ping.
*/

void wxDialUpManagerImpl::CheckStatusInternal()
{
    m_IsOnline = Net_Unknown;

    // first do quick checks to determine what kind of network devices do we
    // have
    int netDeviceType = CheckProcNet();
    if ( netDeviceType == NetDevice_Unknown )
    {
        // nothing found, try ifconfig too
        netDeviceType = CheckIfconfig();
    }

    switch ( netDeviceType )
    {
        case NetDevice_None:
            // no network devices, no connection
            m_IsOnline = Net_No;
            break;

        case NetDevice_LAN:
            // we still do ping to confirm that we're connected but we only do
            // it once and hope that the purpose of the network card (i.e.
            // whether it used for connecting to the Internet or just to a
            // LAN) won't change during the program lifetime
            if ( m_connCard == Net_Unknown )
            {
                m_connCard = CheckConnectAndPing();
            }
            m_IsOnline = m_connCard;
            break;

        case NetDevice_Unknown:
            // try to ping just in case
            m_IsOnline = CheckConnectAndPing();
            break;

        case NetDevice_LAN + NetDevice_Modem:
        case NetDevice_Modem:
            // assume we're connected
            m_IsOnline = Net_Connected;
            break;

        default:
            wxFAIL_MSG(_T("Unexpected netDeviceType"));
    }
}

bool wxDialUpManagerImpl::IsAlwaysOnline() const
{
    wxDialUpManagerImpl *self = wxConstCast(this, wxDialUpManagerImpl);

    int netDeviceType = self->CheckProcNet();
    if ( netDeviceType == NetDevice_Unknown )
    {
        // nothing found, try ifconfig too
        netDeviceType = self->CheckIfconfig();
    }

    if ( netDeviceType == NetDevice_Unknown )
    {
        // this is the only thing we can do unfortunately...
        self->HangUp();
        return IsOnline();
    }
    else
    {
        // we are only permanently online if we have a network card
        return (netDeviceType & NetDevice_LAN) != 0;
    }
}

wxDialUpManagerImpl::NetConnection wxDialUpManagerImpl::CheckConnectAndPing()
{
    NetConnection conn;

    // first try connecting - faster
    conn = CheckConnect();
    if ( conn == Net_Unknown )
    {
        // try pinging too
        conn = CheckPing();
    }

    return conn;
}

wxDialUpManagerImpl::NetConnection wxDialUpManagerImpl::CheckConnect()
{
   // second method: try to connect to a well known host:
   // This can be used under Win 9x, too!
   struct hostent     *hp;
   struct sockaddr_in  serv_addr;

   if((hp = gethostbyname(m_BeaconHost.mb_str())) == NULL)
      return Net_No; // no DNS no net

   serv_addr.sin_family = hp->h_addrtype;
   memcpy(&serv_addr.sin_addr,hp->h_addr, hp->h_length);
   serv_addr.sin_port = htons(m_BeaconPort);

   int sockfd;
   if( ( sockfd = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0)
   {
      return Net_Unknown;  // no info
   }

   if( connect(sockfd, (struct sockaddr *) &serv_addr,
               sizeof(serv_addr)) >= 0)
   {
      close(sockfd);
      return Net_Connected; // we can connect, so we have a network!
   }
   else // failed to connect
   {
#ifdef ENETUNREACH
       if(errno == ENETUNREACH)
          return Net_No; // network is unreachable
       else
#endif
          return Net_Unknown; // connect failed, but don't know why
   }
}


int
wxDialUpManagerImpl::CheckProcNet()
{
    // assume that the test doesn't work
    int netDevice = NetDevice_Unknown;

#ifdef __LINUX__
    if (wxFileExists(_T("/proc/net/route")))
    {
        // cannot use wxFile::Length because file doesn't support seeking, so
        // use stdio directly
        FILE *f = fopen("/proc/net/route", "rt");
        if (f != NULL)
        {
            // now we know that we will find all devices we may have
            netDevice = NetDevice_None;

            char output[256];

            while (fgets(output, 256, f) != NULL)
            {
                if ( strstr(output, "eth") ) // network card
                {
                    netDevice |= NetDevice_LAN;
                }
                else if (strstr(output,"ppp")   // ppp
                        || strstr(output,"sl")  // slip
                        || strstr(output,"pl")) // plip
                {
                    netDevice |= NetDevice_Modem;
                }
            }

            fclose(f);
        }
    }
#endif // __LINUX__

    return netDevice;
}


int
wxDialUpManagerImpl::CheckIfconfig()
{
#ifdef __VMS
    m_CanUseIfconfig = 0;
    return -1;
#else
    // assume that the test doesn't work
    int netDevice = NetDevice_Unknown;

    // first time check for ifconfig location
    if ( m_CanUseIfconfig == -1 ) // unknown
    {
        static const wxChar *ifconfigLocations[] =
        {
            _T("/sbin"),         // Linux, FreeBSD, Darwin
            _T("/usr/sbin"),     // SunOS, Solaris, AIX, HP-UX
            _T("/usr/etc"),      // IRIX
            _T("/etc"),          // AIX 5
        };

        for ( size_t n = 0; n < WXSIZEOF(ifconfigLocations); n++ )
        {
            wxString path(ifconfigLocations[n]);
            path << _T("/ifconfig");

            if ( wxFileExists(path) )
            {
                m_IfconfigPath = path;
                break;
            }
        }
    }

    if ( m_CanUseIfconfig != 0 ) // unknown or yes
    {
        wxLogNull ln; // suppress all error messages

        wxASSERT_MSG( m_IfconfigPath.length(),
                      _T("can't use ifconfig if it wasn't found") );

        wxString tmpfile = wxGetTempFileName( wxT("_wxdialuptest") );
        wxString cmd = wxT("/bin/sh -c \'");
        cmd << m_IfconfigPath;
#if defined(__AIX__) || \
    defined(__OSF__) || \
    defined(__SOLARIS__) || defined (__SUNOS__)
        // need to add -a flag
        cmd << wxT(" -a");
#elif defined(__LINUX__) || defined(__SGI__)
        // nothing to be added to ifconfig
#elif defined(__FREEBSD__) || defined(__DARWIN__)
        // add -l flag
        cmd << wxT(" -l");
#elif defined(__HPUX__)
        // VZ: a wild guess (but without it, ifconfig fails completely)
        cmd << wxT(" ppp0");
#else
        #if defined(__GNUG__)
            #warning "No ifconfig information for this OS."
        #else
            #pragma warning "No ifconfig information for this OS."
        #endif

        m_CanUseIfconfig = 0;
        return -1;
#endif
       cmd << wxT(" >") << tmpfile <<  wxT('\'');
        /* I tried to add an option to wxExecute() to not close stdout,
           so we could let ifconfig write directly to the tmpfile, but
           this does not work. That should be faster, as it doesn´t call
           the shell first. I have no idea why. :-(  (KB) */
        if ( wxExecute(cmd,true /* sync */) == 0 )
        {
            m_CanUseIfconfig = 1;
            wxFFile file;
            if( file.Open(tmpfile) )
            {
                wxString output;
                if ( file.ReadAll(&output) )
                {
                    // FIXME shouldn't we grep for "^ppp"? (VZ)

                    bool hasModem = false,
                         hasLAN = false;

#if defined(__SOLARIS__) || defined (__SUNOS__)
                    // dialup device under SunOS/Solaris
                    hasModem = strstr(output.fn_str(),"ipdptp") != (char *)NULL;
                    hasLAN = strstr(output.fn_str(), "hme") != (char *)NULL;
#elif defined(__LINUX__) || defined (__FREEBSD__)
                    hasModem = strstr(output.fn_str(),"ppp")    // ppp
                        || strstr(output.fn_str(),"sl")  // slip
                        || strstr(output.fn_str(),"pl"); // plip
                    hasLAN = strstr(output.fn_str(), "eth") != NULL;
#elif defined(__SGI__)  // IRIX
                    hasModem = strstr(output.fn_str(), "ppp") != NULL; // PPP
#elif defined(__HPUX__)
                    // if could run ifconfig on interface, then it exists
                    hasModem = true;
#endif

                    netDevice = NetDevice_None;
                    if ( hasModem )
                        netDevice |= NetDevice_Modem;
                    if ( hasLAN )
                        netDevice |= NetDevice_LAN;
                }
                //else: error reading the file
            }
            //else: error opening the file
        }
        else // could not run ifconfig correctly
        {
            m_CanUseIfconfig = 0; // don´t try again
        }

        (void) wxRemoveFile(tmpfile);
    }

    return netDevice;
#endif
}

wxDialUpManagerImpl::NetConnection wxDialUpManagerImpl::CheckPing()
{
    // First time check for ping location. We only use the variant
    // which does not take arguments, a la GNU.
    if(m_CanUsePing == -1) // unknown
    {
#ifdef __VMS
        if (wxFileExists( wxT("SYS$SYSTEM:TCPIP$PING.EXE") ))
            m_PingPath = wxT("$SYS$SYSTEM:TCPIP$PING");
#elif defined(__AIX__)
        m_PingPath = _T("/etc/ping");
#elif defined(__SGI__)
        m_PingPath = _T("/usr/etc/ping");
#else
        if (wxFileExists( wxT("/bin/ping") ))
            m_PingPath = wxT("/bin/ping");
        else if (wxFileExists( wxT("/usr/sbin/ping") ))
            m_PingPath = wxT("/usr/sbin/ping");
#endif
        if (!m_PingPath)
        {
            m_CanUsePing = 0;
        }
    }

    if(! m_CanUsePing)
    {
       // we didn't find ping
       return Net_Unknown;
    }

    wxLogNull ln; // suppress all error messages
    wxASSERT(m_PingPath.length());
    wxString cmd;
    cmd << m_PingPath << wxT(' ');
#if defined(__SOLARIS__) || defined (__SUNOS__)
    // nothing to add to ping command
#elif defined(__AIX__) || \
      defined (__BSD__) || \
      defined(__LINUX__) || \
      defined(__OSF__) || \
      defined(__SGI__) || \
      defined(__VMS)
    cmd << wxT("-c 1 "); // only ping once
#elif defined(__HPUX__)
    cmd << wxT("64 1 "); // only ping once (need also specify the packet size)
#else
    #if defined(__GNUG__)
        #warning "No Ping information for this OS."
    #else
        #pragma warning "No Ping information for this OS."
    #endif

    m_CanUsePing = 0;
    return Net_Unknown;
#endif
    cmd << m_BeaconHost;
    if(wxExecute(cmd, true /* sync */) == 0)
        return Net_Connected;
    else
        return Net_No;
}

/* static */
wxDialUpManager *wxDialUpManager::Create()
{
   return new wxDialUpManagerImpl;
}

#endif // wxUSE_DIALUP_MANAGER
