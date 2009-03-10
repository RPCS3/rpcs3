/////////////////////////////////////////////////////////////////////////////
// Name:        socket.h
// Purpose:     Socket handling classes
// Authors:     Guilhem Lavaux, Guillermo Rodriguez Garcia
// Modified by:
// Created:     April 1997
// RCS-ID:      $Id: socket.h 45498 2007-04-16 13:03:05Z VZ $
// Copyright:   (c) Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SOCKET_H_
#define _WX_SOCKET_H_

#include "wx/defs.h"

#if wxUSE_SOCKETS

// ---------------------------------------------------------------------------
// wxSocket headers
// ---------------------------------------------------------------------------

#include "wx/event.h"
#include "wx/sckaddr.h"
#include "wx/gsocket.h"
#include "wx/list.h"

// ------------------------------------------------------------------------
// Types and constants
// ------------------------------------------------------------------------

enum wxSocketNotify
{
  wxSOCKET_INPUT = GSOCK_INPUT,
  wxSOCKET_OUTPUT = GSOCK_OUTPUT,
  wxSOCKET_CONNECTION = GSOCK_CONNECTION,
  wxSOCKET_LOST = GSOCK_LOST
};

enum
{
  wxSOCKET_INPUT_FLAG = GSOCK_INPUT_FLAG,
  wxSOCKET_OUTPUT_FLAG = GSOCK_OUTPUT_FLAG,
  wxSOCKET_CONNECTION_FLAG = GSOCK_CONNECTION_FLAG,
  wxSOCKET_LOST_FLAG = GSOCK_LOST_FLAG
};

typedef GSocketEventFlags wxSocketEventFlags;

enum wxSocketError
{
  // from GSocket
  wxSOCKET_NOERROR = GSOCK_NOERROR,
  wxSOCKET_INVOP = GSOCK_INVOP,
  wxSOCKET_IOERR = GSOCK_IOERR,
  wxSOCKET_INVADDR = GSOCK_INVADDR,
  wxSOCKET_INVSOCK = GSOCK_INVSOCK,
  wxSOCKET_NOHOST = GSOCK_NOHOST,
  wxSOCKET_INVPORT = GSOCK_INVPORT,
  wxSOCKET_WOULDBLOCK = GSOCK_WOULDBLOCK,
  wxSOCKET_TIMEDOUT = GSOCK_TIMEDOUT,
  wxSOCKET_MEMERR = GSOCK_MEMERR,

  // wxSocket-specific (not yet implemented)
  wxSOCKET_DUMMY
};

enum
{
  wxSOCKET_NONE = 0,
  wxSOCKET_NOWAIT = 1,
  wxSOCKET_WAITALL = 2,
  wxSOCKET_BLOCK = 4,
  wxSOCKET_REUSEADDR = 8
};

enum wxSocketType
{
  wxSOCKET_UNINIT,
  wxSOCKET_CLIENT,
  wxSOCKET_SERVER,
  wxSOCKET_BASE,
  wxSOCKET_DATAGRAM
};

typedef int wxSocketFlags;



// --------------------------------------------------------------------------
// wxSocketBase
// --------------------------------------------------------------------------

class WXDLLIMPEXP_NET wxSocketBase : public wxObject
{
  DECLARE_CLASS(wxSocketBase)

public:

  // Public interface
  // ----------------

  // ctors and dtors
  wxSocketBase();
  wxSocketBase(wxSocketFlags flags, wxSocketType type);
  virtual ~wxSocketBase();
  void Init();
  bool Destroy();

  // state
  inline bool Ok() const { return IsOk(); }
  inline bool IsOk() const { return (m_socket != NULL); }
  inline bool Error() const { return m_error; }
  inline bool IsConnected() const { return m_connected; }
  inline bool IsData() { return WaitForRead(0, 0); }
  inline bool IsDisconnected() const { return !IsConnected(); }
  inline wxUint32 LastCount() const { return m_lcount; }
  inline wxSocketError LastError() const { return (wxSocketError)m_socket->GetError(); }
  void SaveState();
  void RestoreState();

  // addresses
  virtual bool GetLocal(wxSockAddress& addr_man) const;
  virtual bool GetPeer(wxSockAddress& addr_man) const;
  virtual bool SetLocal(wxIPV4address& local);

  // base IO
  virtual bool  Close();
  wxSocketBase& Discard();
  wxSocketBase& Peek(void* buffer, wxUint32 nbytes);
  wxSocketBase& Read(void* buffer, wxUint32 nbytes);
  wxSocketBase& ReadMsg(void *buffer, wxUint32 nbytes);
  wxSocketBase& Unread(const void *buffer, wxUint32 nbytes);
  wxSocketBase& Write(const void *buffer, wxUint32 nbytes);
  wxSocketBase& WriteMsg(const void *buffer, wxUint32 nbytes);

  void InterruptWait() { m_interrupt = true; }
  bool Wait(long seconds = -1, long milliseconds = 0);
  bool WaitForRead(long seconds = -1, long milliseconds = 0);
  bool WaitForWrite(long seconds = -1, long milliseconds = 0);
  bool WaitForLost(long seconds = -1, long milliseconds = 0);

  inline wxSocketFlags GetFlags() const { return m_flags; }
  void SetFlags(wxSocketFlags flags);
  void SetTimeout(long seconds);

  bool GetOption(int level, int optname, void *optval, int *optlen);
  bool SetOption(int level, int optname, const void *optval, int optlen);
  inline wxUint32 GetLastIOSize() const { return m_lcount; }

  // event handling
  void *GetClientData() const { return m_clientData; }
  void SetClientData(void *data) { m_clientData = data; }
  void SetEventHandler(wxEvtHandler& handler, int id = wxID_ANY);
  void SetNotify(wxSocketEventFlags flags);
  void Notify(bool notify);

  // initialize/shutdown the sockets (usually called automatically)
  static bool IsInitialized();
  static bool Initialize();
  static void Shutdown();


  // Implementation from now on
  // --------------------------

  // do not use, should be private (called from GSocket)
  void OnRequest(wxSocketNotify notify);

  // do not use, not documented nor supported
  inline bool IsNoWait() const { return ((m_flags & wxSOCKET_NOWAIT) != 0); }
  inline wxSocketType GetType() const { return m_type; }

private:
  friend class wxSocketClient;
  friend class wxSocketServer;
  friend class wxDatagramSocket;

  // low level IO
  wxUint32 _Read(void* buffer, wxUint32 nbytes);
  wxUint32 _Write(const void *buffer, wxUint32 nbytes);
  bool     _Wait(long seconds, long milliseconds, wxSocketEventFlags flags);

  // pushback buffer
  void     Pushback(const void *buffer, wxUint32 size);
  wxUint32 GetPushback(void *buffer, wxUint32 size, bool peek);

private:
  // socket
  GSocket      *m_socket;           // GSocket
  wxSocketType  m_type;             // wxSocket type

  // state
  wxSocketFlags m_flags;            // wxSocket flags
  bool          m_connected;        // connected?
  bool          m_establishing;     // establishing connection?
  bool          m_reading;          // busy reading?
  bool          m_writing;          // busy writing?
  bool          m_error;            // did last IO call fail?
  wxSocketError m_lasterror;        // last error (not cleared on success)
  wxUint32      m_lcount;           // last IO transaction size
  unsigned long m_timeout;          // IO timeout value
  wxList        m_states;           // stack of states
  bool          m_interrupt;        // interrupt ongoing wait operations?
  bool          m_beingDeleted;     // marked for delayed deletion?
  wxIPV4address m_localAddress;     // bind to local address?

  // pushback buffer
  void         *m_unread;           // pushback buffer
  wxUint32      m_unrd_size;        // pushback buffer size
  wxUint32      m_unrd_cur;         // pushback pointer (index into buffer)

  // events
  int           m_id;               // socket id
  wxEvtHandler *m_handler;          // event handler
  void         *m_clientData;       // client data for events
  bool          m_notify;           // notify events to users?
  wxSocketEventFlags  m_eventmask;  // which events to notify?

  // the initialization count, GSocket is initialized if > 0
  static size_t m_countInit;

  DECLARE_NO_COPY_CLASS(wxSocketBase)
};


// --------------------------------------------------------------------------
// wxSocketServer
// --------------------------------------------------------------------------

class WXDLLIMPEXP_NET wxSocketServer : public wxSocketBase
{
  DECLARE_CLASS(wxSocketServer)

public:
  wxSocketServer(const wxSockAddress& addr, wxSocketFlags flags = wxSOCKET_NONE);

  wxSocketBase* Accept(bool wait = true);
  bool AcceptWith(wxSocketBase& socket, bool wait = true);

  bool WaitForAccept(long seconds = -1, long milliseconds = 0);

  DECLARE_NO_COPY_CLASS(wxSocketServer)
};


// --------------------------------------------------------------------------
// wxSocketClient
// --------------------------------------------------------------------------

class WXDLLIMPEXP_NET wxSocketClient : public wxSocketBase
{
  DECLARE_CLASS(wxSocketClient)

public:
  wxSocketClient(wxSocketFlags flags = wxSOCKET_NONE);
  virtual ~wxSocketClient();

  virtual bool Connect(wxSockAddress& addr, bool wait = true);
  bool Connect(wxSockAddress& addr, wxSockAddress& local, bool wait = true);

  bool WaitOnConnect(long seconds = -1, long milliseconds = 0);

private:
  virtual bool DoConnect(wxSockAddress& addr, wxSockAddress* local, bool wait = true);

  DECLARE_NO_COPY_CLASS(wxSocketClient)
};


// --------------------------------------------------------------------------
// wxDatagramSocket
// --------------------------------------------------------------------------

// WARNING: still in alpha stage

class WXDLLIMPEXP_NET wxDatagramSocket : public wxSocketBase
{
  DECLARE_CLASS(wxDatagramSocket)

public:
  wxDatagramSocket(const wxSockAddress& addr, wxSocketFlags flags = wxSOCKET_NONE);

  wxDatagramSocket& RecvFrom( wxSockAddress& addr,
                              void* buf,
                              wxUint32 nBytes );
  wxDatagramSocket& SendTo( const wxSockAddress& addr,
                            const void* buf,
                            wxUint32 nBytes );

/* TODO:
  bool Connect(wxSockAddress& addr);
*/
  DECLARE_NO_COPY_CLASS(wxDatagramSocket)
};


// --------------------------------------------------------------------------
// wxSocketEvent
// --------------------------------------------------------------------------

class WXDLLIMPEXP_NET wxSocketEvent : public wxEvent
{
public:
  wxSocketEvent(int id = 0)
      : wxEvent(id, wxEVT_SOCKET)
      {
      }

  wxSocketNotify  GetSocketEvent() const { return m_event; }
  wxSocketBase   *GetSocket() const      { return (wxSocketBase *) GetEventObject(); }
  void           *GetClientData() const  { return m_clientData; }

  virtual wxEvent *Clone() const { return new wxSocketEvent(*this); }

public:
  wxSocketNotify  m_event;
  void           *m_clientData;

  DECLARE_DYNAMIC_CLASS_NO_ASSIGN(wxSocketEvent)
};


typedef void (wxEvtHandler::*wxSocketEventFunction)(wxSocketEvent&);

#define wxSocketEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxSocketEventFunction, &func)

#define EVT_SOCKET(id, func) \
    wx__DECLARE_EVT1(wxEVT_SOCKET, id, wxSocketEventHandler(func))

#endif // wxUSE_SOCKETS

#endif // _WX_SOCKET_H_

