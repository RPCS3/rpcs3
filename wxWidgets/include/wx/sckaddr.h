/////////////////////////////////////////////////////////////////////////////
// Name:        sckaddr.h
// Purpose:     Network address classes
// Author:      Guilhem Lavaux
// Modified by:
// Created:     26/04/1997
// RCS-ID:      $Id: sckaddr.h 35665 2005-09-24 21:43:15Z VZ $
// Copyright:   (c) 1997, 1998 Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_NETWORK_ADDRESS_H
#define _WX_NETWORK_ADDRESS_H

#include "wx/defs.h"

#if wxUSE_SOCKETS

#include "wx/string.h"
#include "wx/gsocket.h"


class WXDLLIMPEXP_NET wxSockAddress : public wxObject {
  DECLARE_ABSTRACT_CLASS(wxSockAddress)
public:
  typedef enum { IPV4=1, IPV6=2, UNIX=3 } Addr;

  wxSockAddress();
  wxSockAddress(const wxSockAddress& other);
  virtual ~wxSockAddress();

  wxSockAddress& operator=(const wxSockAddress& other);

  virtual void Clear();
  virtual int Type() = 0;

  GAddress *GetAddress() const { return m_address; }
  void SetAddress(GAddress *address);

  // we need to be able to create copies of the addresses polymorphically (i.e.
  // without knowing the exact address class)
  virtual wxSockAddress *Clone() const = 0;

protected:
  GAddress *m_address;

private:
  void Init();
};

// Interface to an IP address (either IPV4 or IPV6)
class WXDLLIMPEXP_NET wxIPaddress : public wxSockAddress {
  DECLARE_ABSTRACT_CLASS(wxIPaddress)
public:
  wxIPaddress();
  wxIPaddress(const wxIPaddress& other);
  virtual ~wxIPaddress();

  virtual bool Hostname(const wxString& name) = 0;
  virtual bool Service(const wxString& name) = 0;
  virtual bool Service(unsigned short port) = 0;

  virtual bool LocalHost() = 0;
  virtual bool IsLocalHost() const = 0;

  virtual bool AnyAddress() = 0;

  virtual wxString IPAddress() const = 0;

  virtual wxString Hostname() const = 0;
  virtual unsigned short Service() const = 0;
};

class WXDLLIMPEXP_NET wxIPV4address : public wxIPaddress {
  DECLARE_DYNAMIC_CLASS(wxIPV4address)
public:
  wxIPV4address();
  wxIPV4address(const wxIPV4address& other);
  virtual ~wxIPV4address();

  // IPV4 name formats
  //
  //                    hostname
  // dot format         a.b.c.d
  virtual bool Hostname(const wxString& name);
  bool Hostname(unsigned long addr);
  virtual bool Service(const wxString& name);
  virtual bool Service(unsigned short port);

  // localhost (127.0.0.1)
  virtual bool LocalHost();
  virtual bool IsLocalHost() const;

  // any (0.0.0.0)
  virtual bool AnyAddress();

  virtual wxString Hostname() const;
  wxString OrigHostname() { return m_origHostname; }
  virtual unsigned short Service() const;

  // a.b.c.d
  virtual wxString IPAddress() const;

  virtual int Type() { return wxSockAddress::IPV4; }
  virtual wxSockAddress *Clone() const;

  bool operator==(const wxIPV4address& addr) const;

private:
  wxString m_origHostname;
};


// the IPv6 code probably doesn't work, untested -- set to 1 at your own risk
#ifndef wxUSE_IPV6
    #define wxUSE_IPV6 0
#endif

#if wxUSE_IPV6

// Experimental Only:
//
// IPV6 has not yet been implemented in socket layer
class WXDLLIMPEXP_NET wxIPV6address : public wxIPaddress {
  DECLARE_DYNAMIC_CLASS(wxIPV6address)
private:
  struct sockaddr_in6 *m_addr;
public:
  wxIPV6address();
  wxIPV6address(const wxIPV6address& other);
  virtual ~wxIPV6address();

  // IPV6 name formats
  //
  //                          hostname
  //                          3ffe:ffff:0100:f101:0210:a4ff:fee3:9566
  // compact (base85)         Itu&-ZQ82s>J%s99FJXT
  // compressed format        ::1
  // ipv4 mapped              ::ffff:1.2.3.4
  virtual bool Hostname(const wxString& name);

  bool Hostname(unsigned char addr[16]);
  virtual bool Service(const wxString& name);
  virtual bool Service(unsigned short port);

  // localhost (0000:0000:0000:0000:0000:0000:0000:0001 (::1))
  virtual bool LocalHost();
  virtual bool IsLocalHost() const;

  // any (0000:0000:0000:0000:0000:0000:0000:0000 (::))
  virtual bool AnyAddress();

  // 3ffe:ffff:0100:f101:0210:a4ff:fee3:9566
  virtual wxString IPAddress() const;

  virtual wxString Hostname() const;
  virtual unsigned short Service() const;

  virtual int Type() { return wxSockAddress::IPV6; }
  virtual wxSockAddress *Clone() const { return new wxIPV6address(*this); }
};

#endif // wxUSE_IPV6

#if defined(__UNIX__) && !defined(__WINE__) && (!defined(__WXMAC__) || defined(__DARWIN__)) && !defined(__WXMSW__)
#include <sys/socket.h>
#ifndef __VMS__
# include <sys/un.h>
#endif

class WXDLLIMPEXP_NET wxUNIXaddress : public wxSockAddress {
  DECLARE_DYNAMIC_CLASS(wxUNIXaddress)
private:
  struct sockaddr_un *m_addr;
public:
  wxUNIXaddress();
  wxUNIXaddress(const wxUNIXaddress& other);
  virtual ~wxUNIXaddress();

  void Filename(const wxString& name);
  wxString Filename();

  virtual int Type() { return wxSockAddress::UNIX; }
  virtual wxSockAddress *Clone() const { return new wxUNIXaddress(*this); }
};
#endif
  // __UNIX__

#endif
  // wxUSE_SOCKETS

#endif
  // _WX_NETWORK_ADDRESS_H
