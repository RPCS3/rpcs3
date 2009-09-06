/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/sckipc.cpp
// Purpose:     Interprocess communication implementation (wxSocket version)
// Author:      Julian Smart
// Modified by: Guilhem Lavaux (big rewrite) May 1997, 1998
//              Guillermo Rodriguez (updated for wxSocket v2) Jan 2000
//                                  (callbacks deprecated)    Mar 2000
//              Vadim Zeitlin (added support for Unix sockets) Apr 2002
// Created:     1993
// RCS-ID:      $Id: sckipc.cpp 41982 2006-10-13 09:00:06Z RR $
// Copyright:   (c) Julian Smart 1993
//              (c) Guilhem Lavaux 1997, 1998
//              (c) 2000 Guillermo Rodriguez <guille@iies.es>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ==========================================================================
// declarations
// ==========================================================================

// --------------------------------------------------------------------------
// headers
// --------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_SOCKETS && wxUSE_IPC && wxUSE_STREAMS

#include "wx/sckipc.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/event.h"
    #include "wx/module.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "wx/socket.h"

// --------------------------------------------------------------------------
// macros and constants
// --------------------------------------------------------------------------

// It seems to be already defined somewhere in the Xt includes.
#ifndef __XT__
// Message codes
enum
{
  IPC_EXECUTE = 1,
  IPC_REQUEST,
  IPC_POKE,
  IPC_ADVISE_START,
  IPC_ADVISE_REQUEST,
  IPC_ADVISE,
  IPC_ADVISE_STOP,
  IPC_REQUEST_REPLY,
  IPC_FAIL,
  IPC_CONNECT,
  IPC_DISCONNECT
};
#endif

// All sockets will be created with the following flags
#define SCKIPC_FLAGS (wxSOCKET_WAITALL)

// headers needed for umask()
#ifdef __UNIX_LIKE__
    #include <sys/types.h>
    #include <sys/stat.h>
#endif // __UNIX_LIKE__

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// get the address object for the given server name, the caller must delete it
static wxSockAddress *
GetAddressFromName(const wxString& serverName, const wxString& host = wxEmptyString)
{
    // we always use INET sockets under non-Unix systems
#if defined(__UNIX__) && !defined(__WINDOWS__) && !defined(__WINE__) && (!defined(__WXMAC__) || defined(__DARWIN__))
    // under Unix, if the server name looks like a path, create a AF_UNIX
    // socket instead of AF_INET one
    if ( serverName.Find(_T('/')) != wxNOT_FOUND )
    {
        wxUNIXaddress *addr = new wxUNIXaddress;
        addr->Filename(serverName);

        return addr;
    }
#endif // Unix/!Unix
    {
        wxIPV4address *addr = new wxIPV4address;
        addr->Service(serverName);
        if ( !host.empty() )
        {
            addr->Hostname(host);
        }

        return addr;
    }
}

// --------------------------------------------------------------------------
// wxTCPEventHandler stuff (private class)
// --------------------------------------------------------------------------

class wxTCPEventHandler : public wxEvtHandler
{
public:
  wxTCPEventHandler() : wxEvtHandler() {}

  void Client_OnRequest(wxSocketEvent& event);
  void Server_OnRequest(wxSocketEvent& event);

  DECLARE_EVENT_TABLE()
  DECLARE_NO_COPY_CLASS(wxTCPEventHandler)
};

enum
{
  _CLIENT_ONREQUEST_ID = 1000,
  _SERVER_ONREQUEST_ID
};

static wxTCPEventHandler *gs_handler = NULL;

// ==========================================================================
// implementation
// ==========================================================================

IMPLEMENT_DYNAMIC_CLASS(wxTCPServer, wxServerBase)
IMPLEMENT_DYNAMIC_CLASS(wxTCPClient, wxClientBase)
IMPLEMENT_CLASS(wxTCPConnection, wxConnectionBase)

// --------------------------------------------------------------------------
// wxTCPClient
// --------------------------------------------------------------------------

wxTCPClient::wxTCPClient () : wxClientBase()
{
}

wxTCPClient::~wxTCPClient ()
{
}

bool wxTCPClient::ValidHost(const wxString& host)
{
  wxIPV4address addr;

  return addr.Hostname(host);
}

wxConnectionBase *wxTCPClient::MakeConnection (const wxString& host,
                                               const wxString& serverName,
                                               const wxString& topic)
{
  wxSockAddress *addr = GetAddressFromName(serverName, host);
  if ( !addr )
      return NULL;

  wxSocketClient *client = new wxSocketClient(SCKIPC_FLAGS);
  wxSocketStream *stream = new wxSocketStream(*client);
  wxDataInputStream *data_is = new wxDataInputStream(*stream);
  wxDataOutputStream *data_os = new wxDataOutputStream(*stream);

  bool ok = client->Connect(*addr);
  delete addr;

  if ( ok )
  {
    unsigned char msg;

    // Send topic name, and enquire whether this has succeeded
    data_os->Write8(IPC_CONNECT);
    data_os->WriteString(topic);

    msg = data_is->Read8();

    // OK! Confirmation.
    if (msg == IPC_CONNECT)
    {
      wxTCPConnection *connection = (wxTCPConnection *)OnMakeConnection ();

      if (connection)
      {
        if (connection->IsKindOf(CLASSINFO(wxTCPConnection)))
        {
          connection->m_topic = topic;
          connection->m_sock  = client;
          connection->m_sockstrm = stream;
          connection->m_codeci = data_is;
          connection->m_codeco = data_os;
          client->SetEventHandler(*gs_handler, _CLIENT_ONREQUEST_ID);
          client->SetClientData(connection);
          client->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
          client->Notify(true);
          return connection;
        }
        else
        {
          delete connection;
          // and fall through to delete everything else
        }
      }
    }
  }

  // Something went wrong, delete everything
  delete data_is;
  delete data_os;
  delete stream;
  client->Destroy();

  return NULL;
}

wxConnectionBase *wxTCPClient::OnMakeConnection()
{
  return new wxTCPConnection();
}

// --------------------------------------------------------------------------
// wxTCPServer
// --------------------------------------------------------------------------

wxTCPServer::wxTCPServer () : wxServerBase()
{
  m_server = NULL;
}

bool wxTCPServer::Create(const wxString& serverName)
{
  // Destroy previous server, if any
  if (m_server)
  {
    m_server->SetClientData(NULL);
    m_server->Destroy();
    m_server = NULL;
  }

  wxSockAddress *addr = GetAddressFromName(serverName);
  if ( !addr )
      return false;

#ifdef __UNIX_LIKE__
  mode_t umaskOld;
  if ( addr->Type() == wxSockAddress::UNIX )
  {
      // ensure that the file doesn't exist as otherwise calling socket() would
      // fail
      int rc = remove(serverName.fn_str());
      if ( rc < 0 && errno != ENOENT )
      {
          delete addr;

          return false;
      }

      // also set the umask to prevent the others from reading our file
      umaskOld = umask(077);
  }
  else
  {
      // unused anyhow but shut down the compiler warnings
      umaskOld = 0;
  }
#endif // __UNIX_LIKE__

  // Create a socket listening on the specified port
  m_server = new wxSocketServer(*addr, SCKIPC_FLAGS);

#ifdef __UNIX_LIKE__
  if ( addr->Type() == wxSockAddress::UNIX )
  {
      // restore the umask
      umask(umaskOld);

      // save the file name to remove it later
      m_filename = serverName;
  }
#endif // __UNIX_LIKE__

  delete addr;

  if (!m_server->Ok())
  {
    m_server->Destroy();
    m_server = NULL;

    return false;
  }

  m_server->SetEventHandler(*gs_handler, _SERVER_ONREQUEST_ID);
  m_server->SetClientData(this);
  m_server->SetNotify(wxSOCKET_CONNECTION_FLAG);
  m_server->Notify(true);

  return true;
}

wxTCPServer::~wxTCPServer()
{
    if (m_server)
    {
        m_server->SetClientData(NULL);
        m_server->Destroy();
    }

#ifdef __UNIX_LIKE__
    if ( !m_filename.empty() )
    {
        if ( remove(m_filename.fn_str()) != 0 )
        {
            wxLogDebug(_T("Stale AF_UNIX file '%s' left."), m_filename.c_str());
        }
    }
#endif // __UNIX_LIKE__
}

wxConnectionBase *wxTCPServer::OnAcceptConnection( const wxString& WXUNUSED(topic) )
{
  return new wxTCPConnection();
}

// --------------------------------------------------------------------------
// wxTCPConnection
// --------------------------------------------------------------------------

wxTCPConnection::wxTCPConnection () : wxConnectionBase()
{
  m_sock     = NULL;
  m_sockstrm = NULL;
  m_codeci   = NULL;
  m_codeco   = NULL;
}

wxTCPConnection::wxTCPConnection(wxChar *buffer, int size)
       : wxConnectionBase(buffer, size)
{
  m_sock     = NULL;
  m_sockstrm = NULL;
  m_codeci   = NULL;
  m_codeco   = NULL;
}

wxTCPConnection::~wxTCPConnection ()
{
  Disconnect();

  if (m_sock)
  {
    m_sock->SetClientData(NULL);
    m_sock->Destroy();
  }

  /* Delete after destroy */
  wxDELETE(m_codeci);
  wxDELETE(m_codeco);
  wxDELETE(m_sockstrm);
}

void wxTCPConnection::Compress(bool WXUNUSED(on))
{
  // Use wxLZWStream
}

// Calls that CLIENT can make.
bool wxTCPConnection::Disconnect ()
{
  if ( !GetConnected() )
      return true;
  // Send the the disconnect message to the peer.
  m_codeco->Write8(IPC_DISCONNECT);
  m_sock->Notify(false);
  m_sock->Close();
  SetConnected(false);

  return true;
}

bool wxTCPConnection::Execute(const wxChar *data, int size, wxIPCFormat format)
{
  if (!m_sock->IsConnected())
    return false;

  // Prepare EXECUTE message
  m_codeco->Write8(IPC_EXECUTE);
  m_codeco->Write8(format);

  if (size < 0)
    size = (wxStrlen(data) + 1) * sizeof(wxChar);    // includes final NUL

  m_codeco->Write32(size);
  m_sockstrm->Write(data, size);

  return true;
}

wxChar *wxTCPConnection::Request (const wxString& item, int *size, wxIPCFormat format)
{
  if (!m_sock->IsConnected())
    return NULL;

  m_codeco->Write8(IPC_REQUEST);
  m_codeco->WriteString(item);
  m_codeco->Write8(format);

  // If Unpack doesn't initialize it.
  int ret;

  ret = m_codeci->Read8();
  if (ret == IPC_FAIL)
    return NULL;
  else
  {
    size_t s;

    s = m_codeci->Read32();

    wxChar *data = GetBufferAtLeast( s );
    wxASSERT_MSG(data != NULL,
                 _T("Buffer too small in wxTCPConnection::Request") );
    m_sockstrm->Read(data, s);

    if (size)
      *size = s;
    return data;
  }
}

bool wxTCPConnection::Poke (const wxString& item, wxChar *data, int size, wxIPCFormat format)
{
  if (!m_sock->IsConnected())
    return false;

  m_codeco->Write8(IPC_POKE);
  m_codeco->WriteString(item);
  m_codeco->Write8(format);

  if (size < 0)
    size = (wxStrlen(data) + 1) * sizeof(wxChar);    // includes final NUL

  m_codeco->Write32(size);
  m_sockstrm->Write(data, size);

  return true;
}

bool wxTCPConnection::StartAdvise (const wxString& item)
{
  int ret;

  if (!m_sock->IsConnected())
    return false;

  m_codeco->Write8(IPC_ADVISE_START);
  m_codeco->WriteString(item);

  ret = m_codeci->Read8();

  if (ret != IPC_FAIL)
    return true;
  else
    return false;
}

bool wxTCPConnection::StopAdvise (const wxString& item)
{
  int msg;

  if (!m_sock->IsConnected())
    return false;

  m_codeco->Write8(IPC_ADVISE_STOP);
  m_codeco->WriteString(item);

  msg = m_codeci->Read8();

  if (msg != IPC_FAIL)
    return true;
  else
    return false;
}

// Calls that SERVER can make
bool wxTCPConnection::Advise (const wxString& item,
                              wxChar *data, int size, wxIPCFormat format)
{
  if (!m_sock->IsConnected())
    return false;

  m_codeco->Write8(IPC_ADVISE);
  m_codeco->WriteString(item);
  m_codeco->Write8(format);

  if (size < 0)
    size = (wxStrlen(data) + 1) * sizeof(wxChar);    // includes final NUL

  m_codeco->Write32(size);
  m_sockstrm->Write(data, size);

  return true;
}

// --------------------------------------------------------------------------
// wxTCPEventHandler (private class)
// --------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxTCPEventHandler, wxEvtHandler)
  EVT_SOCKET(_CLIENT_ONREQUEST_ID, wxTCPEventHandler::Client_OnRequest)
  EVT_SOCKET(_SERVER_ONREQUEST_ID, wxTCPEventHandler::Server_OnRequest)
END_EVENT_TABLE()

void wxTCPEventHandler::Client_OnRequest(wxSocketEvent &event)
{
  wxSocketBase *sock = event.GetSocket();
  if (!sock) {		/* No socket, no glory */
    return ;
  }
  wxSocketNotify evt = event.GetSocketEvent();
  wxTCPConnection *connection = (wxTCPConnection *)(sock->GetClientData());

  // This socket is being deleted; skip this event
  if (!connection)
    return;

  wxDataInputStream *codeci;
  wxDataOutputStream *codeco;
  wxSocketStream *sockstrm;
  wxString topic_name = connection->m_topic;
  wxString item;

  // We lost the connection: destroy everything
  if (evt == wxSOCKET_LOST)
  {
    sock->Notify(false);
    sock->Close();
    connection->OnDisconnect();
    return;
  }

  // Receive message number.
  codeci = connection->m_codeci;
  codeco = connection->m_codeco;
  sockstrm = connection->m_sockstrm;
  int msg = codeci->Read8();

  switch (msg)
  {
  case IPC_EXECUTE:
  {
    wxChar *data;
    size_t size;
    wxIPCFormat format;

    format = (wxIPCFormat)codeci->Read8();
    size = codeci->Read32();
    
    data = connection->GetBufferAtLeast( size );
    wxASSERT_MSG(data != NULL,
                 _T("Buffer too small in wxTCPEventHandler::Client_OnRequest") );
    sockstrm->Read(data, size);

    connection->OnExecute (topic_name, data, size, format);

    break;
  }
  case IPC_ADVISE:
  {
    wxChar *data;
    size_t size;
    wxIPCFormat format;

    item = codeci->ReadString();
    format = (wxIPCFormat)codeci->Read8();
    size = codeci->Read32();
    data = connection->GetBufferAtLeast( size );
    wxASSERT_MSG(data != NULL,
                 _T("Buffer too small in wxTCPEventHandler::Client_OnRequest") );
    sockstrm->Read(data, size);

    connection->OnAdvise (topic_name, item, data, size, format);

    break;
  }
  case IPC_ADVISE_START:
  {
    item = codeci->ReadString();

    bool ok = connection->OnStartAdvise (topic_name, item);
    if (ok)
      codeco->Write8(IPC_ADVISE_START);
    else
      codeco->Write8(IPC_FAIL);

    break;
  }
  case IPC_ADVISE_STOP:
  {
    item = codeci->ReadString();

    bool ok = connection->OnStopAdvise (topic_name, item);
    if (ok)
      codeco->Write8(IPC_ADVISE_STOP);
    else
      codeco->Write8(IPC_FAIL);

    break;
  }
  case IPC_POKE:
  {
    wxIPCFormat format;
    size_t size;
    wxChar *data;

    item = codeci->ReadString();
    format = (wxIPCFormat)codeci->Read8();
    size = codeci->Read32();
    data = connection->GetBufferAtLeast( size );
    wxASSERT_MSG(data != NULL,
                 _T("Buffer too small in wxTCPEventHandler::Client_OnRequest") );
    sockstrm->Read(data, size);

    connection->OnPoke (topic_name, item, data, size, format);

    break;
  }
  case IPC_REQUEST:
  {
    wxIPCFormat format;

    item = codeci->ReadString();
    format = (wxIPCFormat)codeci->Read8();

    int user_size = -1;
    wxChar *user_data = connection->OnRequest (topic_name, item, &user_size, format);

    if (user_data)
    {
      codeco->Write8(IPC_REQUEST_REPLY);

      if (user_size == -1)
        user_size = (wxStrlen(user_data) + 1) * sizeof(wxChar);    // includes final NUL

      codeco->Write32(user_size);
      sockstrm->Write(user_data, user_size);
    }
    else
      codeco->Write8(IPC_FAIL);

    break;
  }
  case IPC_DISCONNECT:
  {
    sock->Notify(false);
    sock->Close();
    connection->SetConnected(false);
    connection->OnDisconnect();
    break;
  }
  default:
    codeco->Write8(IPC_FAIL);
    break;
  }
}

void wxTCPEventHandler::Server_OnRequest(wxSocketEvent &event)
{
  wxSocketServer *server = (wxSocketServer *) event.GetSocket();
  if (!server) {		/* No server, Then exit */
	  return ;
  }
  wxTCPServer *ipcserv = (wxTCPServer *) server->GetClientData();

  // This socket is being deleted; skip this event
  if (!ipcserv)
    return;

  if (event.GetSocketEvent() != wxSOCKET_CONNECTION)
    return;

  // Accept the connection, getting a new socket
  wxSocketBase *sock = server->Accept();
  if (!sock) {		/* No socket, no glory */
	  return ;
  }
  if (!sock->Ok())
  {
    sock->Destroy();
    return;
  }

  wxSocketStream *stream     = new wxSocketStream(*sock);
  wxDataInputStream *codeci  = new wxDataInputStream(*stream);
  wxDataOutputStream *codeco = new wxDataOutputStream(*stream);

  int msg;
  msg = codeci->Read8();

  if (msg == IPC_CONNECT)
  {
    wxString topic_name;
    topic_name = codeci->ReadString();

    wxTCPConnection *new_connection =
         (wxTCPConnection *)ipcserv->OnAcceptConnection (topic_name);

    if (new_connection)
    {
      if (new_connection->IsKindOf(CLASSINFO(wxTCPConnection)))
      {
        // Acknowledge success
        codeco->Write8(IPC_CONNECT);
        new_connection->m_topic = topic_name;
        new_connection->m_sock = sock;
        new_connection->m_sockstrm = stream;
        new_connection->m_codeci = codeci;
        new_connection->m_codeco = codeco;
        sock->SetEventHandler(*gs_handler, _CLIENT_ONREQUEST_ID);
        sock->SetClientData(new_connection);
        sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
        sock->Notify(true);
        return;
      }
      else
      {
        delete new_connection;
        // and fall through to delete everything else
      }
    }
  }

  // Something went wrong, send failure message and delete everything
  codeco->Write8(IPC_FAIL);

  delete codeco;
  delete codeci;
  delete stream;
  sock->Destroy();
}

// --------------------------------------------------------------------------
// wxTCPEventHandlerModule (private class)
// --------------------------------------------------------------------------

class wxTCPEventHandlerModule: public wxModule
{
  DECLARE_DYNAMIC_CLASS(wxTCPEventHandlerModule)

public:
  bool OnInit() { gs_handler = new wxTCPEventHandler(); return true; }
  void OnExit() { wxDELETE(gs_handler); }
};

IMPLEMENT_DYNAMIC_CLASS(wxTCPEventHandlerModule, wxModule)


#endif
   // wxUSE_SOCKETS && wxUSE_IPC && wxUSE_STREAMS
