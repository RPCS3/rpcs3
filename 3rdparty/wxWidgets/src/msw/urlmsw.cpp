/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/urlmsw.cpp
// Purpose:     MS-Windows native URL support based on WinINet
// Author:      Hajo Kirchhoff
// Modified by:
// Created:     06/11/2003
// RCS-ID:      $Id: urlmsw.cpp 58116 2009-01-15 12:45:22Z VZ $
// Copyright:   (c) 2003 Hajo Kirchhoff
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_URL_NATIVE

#ifndef WX_PRECOMP
    #include "wx/list.h"
    #include "wx/string.h"
    #include "wx/utils.h"
    #include "wx/module.h"
    #include "wx/log.h"
#endif

#if !wxUSE_PROTOCOL_HTTP
#include "wx/protocol/protocol.h"

// empty http protocol replacement (for now)
// so that wxUSE_URL_NATIVE can be used with
// wxSOCKETS==0 and wxUSE_PROTOCOL_HTTP==0
class wxHTTPDummyProto : public wxProtocol
{
public:
    wxHTTPDummyProto() : wxProtocol() { }

    wxProtocolError GetError() { return m_error; }

    virtual bool Abort() { return true; }

    wxInputStream *GetInputStream(const wxString& WXUNUSED(path))
    {
        return 0;   // input stream is returned by wxURLNativeImp
    }

protected:
    wxProtocolError m_error;

    DECLARE_DYNAMIC_CLASS_NO_COPY(wxHTTPDummyProto)
    DECLARE_PROTOCOL(wxHTTPDummyProto)
};

// the only "reason for being" for this class is to tell
// wxURL that there is someone dealing with the http protocol
IMPLEMENT_DYNAMIC_CLASS(wxHTTPDummyProto, wxProtocol)
IMPLEMENT_PROTOCOL(wxHTTPDummyProto, wxT("http"), NULL, false)
USE_PROTOCOL(wxHTTPDummyProto)

#endif // !wxUSE_PROTOCOL_HTTP


#ifdef __VISUALC__  // be conservative about this pragma
    // tell the linker to include wininet.lib automatically
    #pragma comment(lib, "wininet.lib")
#endif

#include "wx/url.h"

#include <string.h>
#include <ctype.h>
#include <wininet.h>

// this class needn't be exported
class wxWinINetURL:public wxURLNativeImp
{
public:
    wxInputStream *GetInputStream(wxURL *owner);

protected:
    // return the WinINet session handle
    static HINTERNET GetSessionHandle();
};

HINTERNET wxWinINetURL::GetSessionHandle()
{
    // this struct ensures that the session is opened when the
    // first call to GetSessionHandle is made
    // it also ensures that the session is closed when the program
    // terminates
    static struct INetSession
    {
        INetSession()
        {
            DWORD rc = InternetAttemptConnect(0);

            m_handle = InternetOpen
                       (
                        wxVERSION_STRING,
                        INTERNET_OPEN_TYPE_PRECONFIG,
                        NULL,
                        NULL,
                        rc == ERROR_SUCCESS ? 0 : INTERNET_FLAG_OFFLINE
                       );
        }

        ~INetSession()
        {
            InternetCloseHandle(m_handle);
        }

        HINTERNET m_handle;
    } session;

   return session.m_handle;
}

// this class needn't be exported
class /*WXDLLIMPEXP_NET */ wxWinINetInputStream : public wxInputStream
{
public:
    wxWinINetInputStream(HINTERNET hFile=0);
    virtual ~wxWinINetInputStream();

    void Attach(HINTERNET hFile);

    wxFileOffset SeekI( wxFileOffset WXUNUSED(pos), wxSeekMode WXUNUSED(mode) )
        { return -1; }
    wxFileOffset TellI() const
        { return -1; }
    size_t GetSize() const;

protected:
    void SetError(wxStreamError err) { m_lasterror=err; }
    HINTERNET m_hFile;
    size_t OnSysRead(void *buffer, size_t bufsize);

    DECLARE_NO_COPY_CLASS(wxWinINetInputStream)
};

size_t wxWinINetInputStream::GetSize() const
{
   DWORD contentLength = 0;
   DWORD dwSize = sizeof(contentLength);
   DWORD index = 0;

   if ( HttpQueryInfo( m_hFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &dwSize, &index) )
      return contentLength;
   else
      return 0;
}

size_t wxWinINetInputStream::OnSysRead(void *buffer, size_t bufsize)
{
    DWORD bytesread = 0;
    if ( !InternetReadFile(m_hFile, buffer, bufsize, &bytesread) )
    {
        DWORD lError = ::GetLastError();
        if ( lError != ERROR_SUCCESS )
            SetError(wxSTREAM_READ_ERROR);

        DWORD iError, bLength;
        InternetGetLastResponseInfo(&iError, NULL, &bLength);
        if ( bLength > 0 )
        {
            wxString errorString;
            InternetGetLastResponseInfo
            (
                &iError,
                wxStringBuffer(errorString, bLength),
                &bLength
            );

            wxLogError(wxT("Read failed with error %d: %s"),
                       iError, errorString.c_str());
        }
    }

    if ( bytesread == 0 )
    {
        SetError(wxSTREAM_EOF);
    }

    return bytesread;
}

wxWinINetInputStream::wxWinINetInputStream(HINTERNET hFile)
                    : m_hFile(hFile)
{
}

void wxWinINetInputStream::Attach(HINTERNET newHFile)
{
    wxCHECK_RET(m_hFile==NULL,
        wxT("cannot attach new stream when stream already exists"));
    m_hFile=newHFile;
    SetError(m_hFile!=NULL ? wxSTREAM_NO_ERROR : wxSTREAM_READ_ERROR);
}

wxWinINetInputStream::~wxWinINetInputStream()
{
    if ( m_hFile )
    {
        InternetCloseHandle(m_hFile);
        m_hFile=0;
    }
}

wxURLNativeImp *wxURL::CreateNativeImpObject()
{
    return new wxWinINetURL;
}

wxInputStream *wxWinINetURL::GetInputStream(wxURL *owner)
{
    DWORD service;
    if ( owner->GetScheme() == wxT("http") )
    {
        service = INTERNET_SERVICE_HTTP;
    }
    else if ( owner->GetScheme() == wxT("ftp") )
    {
        service = INTERNET_SERVICE_FTP;
    }
    else
    {
        // unknown protocol. Let wxURL try another method.
        return 0;
    }

    wxWinINetInputStream *newStream = new wxWinINetInputStream;
    HINTERNET newStreamHandle = InternetOpenUrl
                                (
                                    GetSessionHandle(),
                                    owner->GetURL(),
                                    NULL,
                                    0,
                                    INTERNET_FLAG_KEEP_CONNECTION |
                                    INTERNET_FLAG_PASSIVE,
                                    (DWORD_PTR)newStream
                                );
    newStream->Attach(newStreamHandle);

    return newStream;
}

#endif // wxUSE_URL_NATIVE
