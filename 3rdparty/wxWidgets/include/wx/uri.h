/////////////////////////////////////////////////////////////////////////////
// Name:        uri.h
// Purpose:     wxURI - Class for parsing URIs
// Author:      Ryan Norton
// Modified By:
// Created:     07/01/2004
// RCS-ID:      $Id: uri.h 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) Ryan Norton
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_URI_H_
#define _WX_URI_H_

#include "wx/defs.h"
#include "wx/object.h"
#include "wx/string.h"

// Host Type that the server component can be
enum wxURIHostType
{
    wxURI_REGNAME,      // Host is a normal register name (www.mysite.com etc.)
    wxURI_IPV4ADDRESS,  // Host is a version 4 ip address (192.168.1.100)
    wxURI_IPV6ADDRESS,  // Host is a version 6 ip address [aa:aa:aa:aa::aa:aa]:5050
    wxURI_IPVFUTURE     // Host is a future ip address (wxURI is unsure what kind)
};

// Component Flags
enum wxURIFieldType
{
    wxURI_SCHEME = 1,
    wxURI_USERINFO = 2,
    wxURI_SERVER = 4,
    wxURI_PORT = 8,
    wxURI_PATH = 16,
    wxURI_QUERY = 32,
    wxURI_FRAGMENT = 64
};

// Miscellaneous other flags
enum wxURIFlags
{
    wxURI_STRICT = 1
};


// Generic class for parsing URIs.
//
// See RFC 3986
class WXDLLIMPEXP_BASE wxURI : public wxObject
{
public:
    wxURI();
    wxURI(const wxString& uri);
    wxURI(const wxURI& uri);

    virtual ~wxURI();

    const wxChar* Create(const wxString& uri);

    bool HasScheme() const      {   return (m_fields & wxURI_SCHEME) == wxURI_SCHEME;       }
    bool HasUserInfo() const    {   return (m_fields & wxURI_USERINFO) == wxURI_USERINFO;   }
    bool HasServer() const      {   return (m_fields & wxURI_SERVER) == wxURI_SERVER;       }
    bool HasPort() const        {   return (m_fields & wxURI_PORT) == wxURI_PORT;           }
    bool HasPath() const        {   return (m_fields & wxURI_PATH) == wxURI_PATH;           }
    bool HasQuery() const       {   return (m_fields & wxURI_QUERY) == wxURI_QUERY;         }
    bool HasFragment() const    {   return (m_fields & wxURI_FRAGMENT) == wxURI_FRAGMENT;   }

    const wxString& GetScheme() const           {   return m_scheme;    }
    const wxString& GetPath() const             {   return m_path;      }
    const wxString& GetQuery() const            {   return m_query;     }
    const wxString& GetFragment() const         {   return m_fragment;  }
    const wxString& GetPort() const             {   return m_port;      }
    const wxString& GetUserInfo() const         {   return m_userinfo;  }
    const wxString& GetServer() const           {   return m_server;    }
    const wxURIHostType& GetHostType() const    {   return m_hostType;  }

    //Note that the following two get functions are explicitly depreciated by RFC 2396
    wxString GetUser() const;
    wxString GetPassword() const;

    wxString BuildURI() const;
    wxString BuildUnescapedURI() const;

    void Resolve(const wxURI& base, int flags = wxURI_STRICT);
    bool IsReference() const;

    wxURI& operator = (const wxURI& uri);
    wxURI& operator = (const wxString& string);
    bool operator == (const wxURI& uri) const;

    static wxString Unescape (const wxString& szEscapedURI);

protected:
    wxURI& Assign(const wxURI& uri);

    void Clear();

    const wxChar* Parse          (const wxChar* uri);
    const wxChar* ParseAuthority (const wxChar* uri);
    const wxChar* ParseScheme    (const wxChar* uri);
    const wxChar* ParseUserInfo  (const wxChar* uri);
    const wxChar* ParseServer    (const wxChar* uri);
    const wxChar* ParsePort      (const wxChar* uri);
    const wxChar* ParsePath      (const wxChar* uri,
                                  bool bReference = false,
                                  bool bNormalize = true);
    const wxChar* ParseQuery     (const wxChar* uri);
    const wxChar* ParseFragment  (const wxChar* uri);


    static bool ParseH16(const wxChar*& uri);
    static bool ParseIPv4address(const wxChar*& uri);
    static bool ParseIPv6address(const wxChar*& uri);
    static bool ParseIPvFuture(const wxChar*& uri);

    static void Normalize(wxChar* uri, bool bIgnoreLeads = false);
    static void UpTree(const wxChar* uristart, const wxChar*& uri);

    static wxChar TranslateEscape(const wxChar* s);
    static void Escape  (wxString& s, const wxChar& c);
    static bool IsEscape(const wxChar*& uri);

    static wxChar CharToHex(const wxChar& c);

    static bool IsUnreserved (const wxChar& c);
    static bool IsReserved (const wxChar& c);
    static bool IsGenDelim (const wxChar& c);
    static bool IsSubDelim (const wxChar& c);
    static bool IsHex(const wxChar& c);
    static bool IsAlpha(const wxChar& c);
    static bool IsDigit(const wxChar& c);

    wxString m_scheme;
    wxString m_path;
    wxString m_query;
    wxString m_fragment;

    wxString m_userinfo;
    wxString m_server;
    wxString m_port;

    wxURIHostType m_hostType;

    size_t m_fields;

    DECLARE_DYNAMIC_CLASS(wxURI)
};

#endif // _WX_URI_H_

