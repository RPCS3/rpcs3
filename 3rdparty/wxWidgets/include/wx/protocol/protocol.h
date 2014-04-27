/////////////////////////////////////////////////////////////////////////////
// Name:        wx/protocol/protocol.h
// Purpose:     Protocol base class
// Author:      Guilhem Lavaux
// Modified by:
// Created:     10/07/1997
// RCS-ID:      $Id: protocol.h 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) 1997, 1998 Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PROTOCOL_PROTOCOL_H
#define _WX_PROTOCOL_PROTOCOL_H

#include "wx/defs.h"

#if wxUSE_PROTOCOL

#include "wx/object.h"
#include "wx/string.h"
#include "wx/stream.h"

#if wxUSE_SOCKETS
    #include "wx/socket.h"
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

typedef enum
{
    wxPROTO_NOERR = 0,
    wxPROTO_NETERR,
    wxPROTO_PROTERR,
    wxPROTO_CONNERR,
    wxPROTO_INVVAL,
    wxPROTO_NOHNDLR,
    wxPROTO_NOFILE,
    wxPROTO_ABRT,
    wxPROTO_RCNCT,
    wxPROTO_STREAMING
} wxProtocolError;

// ----------------------------------------------------------------------------
// wxProtocol: abstract base class for all protocols
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_NET wxProtocol
#if wxUSE_SOCKETS
 : public wxSocketClient
#else
 : public wxObject
#endif
{
public:
    wxProtocol();

#if wxUSE_SOCKETS
    bool Reconnect();
    virtual bool Connect( const wxString& WXUNUSED(host) ) { return FALSE; }
    virtual bool Connect( wxSockAddress& addr, bool WXUNUSED(wait) = TRUE) { return wxSocketClient::Connect(addr); }

    // read a '\r\n' terminated line from the given socket and put it in
    // result (without the terminators)
    static wxProtocolError ReadLine(wxSocketBase *socket, wxString& result);

    // read a line from this socket - this one can be overridden in the
    // derived classes if different line termination convention is to be used
    virtual wxProtocolError ReadLine(wxString& result);
#endif // wxUSE_SOCKETS

    virtual bool Abort() = 0;
    virtual wxInputStream *GetInputStream(const wxString& path) = 0;
    virtual wxProtocolError GetError() = 0;
    virtual wxString GetContentType() { return wxEmptyString; }
    virtual void SetUser(const wxString& WXUNUSED(user)) {}
    virtual void SetPassword(const wxString& WXUNUSED(passwd) ) {}

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxProtocol)
};

#if wxUSE_SOCKETS
wxProtocolError WXDLLIMPEXP_NET GetLine(wxSocketBase *sock, wxString& result);
#endif

// ----------------------------------------------------------------------------
// macros for protocol classes
// ----------------------------------------------------------------------------

#define DECLARE_PROTOCOL(class) \
public: \
  static wxProtoInfo g_proto_##class;

#define IMPLEMENT_PROTOCOL(class, name, serv, host) \
wxProtoInfo class::g_proto_##class(name, serv, host, CLASSINFO(class)); \
bool wxProtocolUse##class = TRUE;

#define USE_PROTOCOL(class) \
    extern bool wxProtocolUse##class ; \
    static struct wxProtocolUserFor##class \
    { \
        wxProtocolUserFor##class() { wxProtocolUse##class = TRUE; } \
    } wxProtocolDoUse##class;

class WXDLLIMPEXP_NET wxProtoInfo : public wxObject
{
public:
    wxProtoInfo(const wxChar *name,
                const wxChar *serv_name,
                const bool need_host1,
                wxClassInfo *info);

protected:
    wxProtoInfo *next;
    wxString m_protoname;
    wxString prefix;
    wxString m_servname;
    wxClassInfo *m_cinfo;
    bool m_needhost;

    friend class wxURL;

    DECLARE_DYNAMIC_CLASS(wxProtoInfo)
    DECLARE_NO_COPY_CLASS(wxProtoInfo)
};

#endif // wxUSE_PROTOCOL

#endif // _WX_PROTOCOL_PROTOCOL_H
