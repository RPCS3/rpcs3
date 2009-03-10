/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/sckstrm.cpp
// Purpose:     wxSocket*Stream
// Author:      Guilhem Lavaux
// Modified by:
// Created:     17/07/97
// RCS-ID:      $Id: sckstrm.cpp 42897 2006-11-01 10:21:24Z JS $
// Copyright:   (c)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_SOCKETS && wxUSE_STREAMS

#include "wx/sckstrm.h"

#ifndef WX_PRECOMP
    #include "wx/stream.h"
#endif

#include "wx/socket.h"

// ---------------------------------------------------------------------------
// wxSocketOutputStream
// ---------------------------------------------------------------------------

wxSocketOutputStream::wxSocketOutputStream(wxSocketBase& s)
  : m_o_socket(&s)
{
}

wxSocketOutputStream::~wxSocketOutputStream()
{
}

size_t wxSocketOutputStream::OnSysWrite(const void *buffer, size_t size)
{
    size_t ret = m_o_socket->Write((const char *)buffer, size).LastCount();
    m_lasterror = m_o_socket->Error() ? wxSTREAM_WRITE_ERROR : wxSTREAM_NO_ERROR;
    return ret;
    
    // Patch 1476893 caused Advise to hang, needs further investigation
#if 0
    const char *buf = (const char *)buffer;
    size_t count = 0;

    while ( count < size && m_o_socket->WaitForWrite() )
    {
        const size_t ret = m_o_socket->Write(buf, size - count).LastCount();

        buf += ret;
        count += ret;

        if ( m_o_socket->Error() )
        {
            if (m_o_socket->LastError() != wxSOCKET_WOULDBLOCK)
            {
                m_lasterror = wxSTREAM_WRITE_ERROR;
                return count;
            }
        }
    }

    m_lasterror = wxSTREAM_NO_ERROR;
    return count;
#endif
}

// ---------------------------------------------------------------------------
// wxSocketInputStream
// ---------------------------------------------------------------------------

wxSocketInputStream::wxSocketInputStream(wxSocketBase& s)
 : m_i_socket(&s)
{
}

wxSocketInputStream::~wxSocketInputStream()
{
}

size_t wxSocketInputStream::OnSysRead(void *buffer, size_t size)
{
    size_t ret = m_i_socket->Read((char *)buffer, size).LastCount();
    m_lasterror = m_i_socket->Error() ? wxSTREAM_READ_ERROR : wxSTREAM_NO_ERROR;
    return ret;
    
    // Patch 1476893 caused Advise to hang, needs further investigation
#if 0
    char *buf = (char *)buffer;
    size_t count = 0;

    while ( count < size && m_i_socket->WaitForRead() )
    {
        const size_t ret = m_i_socket->Read(buf, size - count).LastCount();

        buf += ret;
        count += ret;

        if ( m_i_socket->Error() )
        {
            if (m_i_socket->LastError() != wxSOCKET_WOULDBLOCK)
            {
                m_lasterror = wxSTREAM_READ_ERROR;
                return count;
            }
        }
    }

    m_lasterror = wxSTREAM_NO_ERROR;
    return count;
#endif
}

// ---------------------------------------------------------------------------
// wxSocketStream
// ---------------------------------------------------------------------------

wxSocketStream::wxSocketStream(wxSocketBase& s)
  : wxSocketInputStream(s), wxSocketOutputStream(s)
{
}

wxSocketStream::~wxSocketStream()
{
}

#endif
  // wxUSE_STREAMS && wxUSE_SOCKETS
