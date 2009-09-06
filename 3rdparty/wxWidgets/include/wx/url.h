/////////////////////////////////////////////////////////////////////////////
// Name:        url.h
// Purpose:     URL parser
// Author:      Guilhem Lavaux
// Modified by: Ryan Norton
// Created:     20/07/1997
// RCS-ID:      $Id: url.h 41263 2006-09-17 10:59:18Z RR $
// Copyright:   (c) 1997, 1998 Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_URL_H
#define _WX_URL_H

#include "wx/defs.h"

#if wxUSE_URL

#include "wx/uri.h"
#include "wx/protocol/protocol.h"

#if wxUSE_PROTOCOL_HTTP
  #include "wx/protocol/http.h"
#endif

typedef enum {
  wxURL_NOERR = 0,
  wxURL_SNTXERR,
  wxURL_NOPROTO,
  wxURL_NOHOST,
  wxURL_NOPATH,
  wxURL_CONNERR,
  wxURL_PROTOERR
} wxURLError;

#if wxUSE_URL_NATIVE
class WXDLLIMPEXP_NET wxURL;

class WXDLLIMPEXP_NET wxURLNativeImp : public wxObject
{
public:
    virtual ~wxURLNativeImp() { }
    virtual wxInputStream *GetInputStream(wxURL *owner) = 0;
};
#endif // wxUSE_URL_NATIVE

class WXDLLIMPEXP_NET wxURL : public wxURI
{
public:
    wxURL(const wxString& sUrl = wxEmptyString);
    wxURL(const wxURI& url);
    virtual ~wxURL();

    wxURL& operator = (const wxString& url);
    wxURL& operator = (const wxURI& url);

    wxProtocol& GetProtocol()        { return *m_protocol; }
    wxURLError GetError() const      { return m_error; }
    wxString GetURL() const          { return m_url; }

    wxURLError SetURL(const wxString &url)
        { *this = url; return m_error; }

    bool IsOk() const
        { return m_error == wxURL_NOERR; }

    wxInputStream *GetInputStream();

#if wxUSE_PROTOCOL_HTTP
    static void SetDefaultProxy(const wxString& url_proxy);
    void SetProxy(const wxString& url_proxy);
#endif // wxUSE_PROTOCOL_HTTP

#if WXWIN_COMPATIBILITY_2_4
    //Use the proper wxURI accessors instead
    wxDEPRECATED( wxString GetProtocolName() const );
    wxDEPRECATED( wxString GetHostName() const );
    wxDEPRECATED( wxString GetPath() const );

    //Use wxURI instead - this does not work that well
    wxDEPRECATED( static wxString ConvertToValidURI(
                        const wxString& uri,
                        const wxChar* delims = wxT(";/?:@&=+$,")
                  ) );

    //Use wxURI::Unescape instead
    wxDEPRECATED( static wxString ConvertFromURI(const wxString& uri) );
#endif

protected:
    static wxProtoInfo *ms_protocols;

#if wxUSE_PROTOCOL_HTTP
    static wxHTTP *ms_proxyDefault;
    static bool ms_useDefaultProxy;
    wxHTTP *m_proxy;
#endif // wxUSE_PROTOCOL_HTTP

#if wxUSE_URL_NATIVE
    friend class wxURLNativeImp;
    // pointer to a native URL implementation object
    wxURLNativeImp *m_nativeImp;
    // Creates on the heap and returns a native
    // implementation object for the current platform.
    static wxURLNativeImp *CreateNativeImpObject();
#endif
    wxProtoInfo *m_protoinfo;
    wxProtocol *m_protocol;

    wxURLError m_error;
    wxString m_url;
    bool m_useProxy;

    void Init(const wxString&);
    bool ParseURL();
    void CleanData();
    bool FetchProtocol();

    friend class wxProtoInfo;
    friend class wxURLModule;

private:
    DECLARE_DYNAMIC_CLASS(wxURL)
};

#endif // wxUSE_URL

#endif // _WX_URL_H

