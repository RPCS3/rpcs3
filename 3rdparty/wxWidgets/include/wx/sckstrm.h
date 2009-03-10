/////////////////////////////////////////////////////////////////////////////
// Name:        sckstrm.h
// Purpose:     wxSocket*Stream
// Author:      Guilhem Lavaux
// Modified by:
// Created:     17/07/97
// RCS-ID:      $Id: sckstrm.h 41020 2006-09-05 20:47:48Z VZ $
// Copyright:   (c)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////
#ifndef __SCK_STREAM_H__
#define __SCK_STREAM_H__

#include "wx/stream.h"

#if wxUSE_SOCKETS && wxUSE_STREAMS

#include "wx/socket.h"

class WXDLLIMPEXP_NET wxSocketOutputStream : public wxOutputStream
{
 public:
  wxSocketOutputStream(wxSocketBase& s);
  virtual ~wxSocketOutputStream();

  wxFileOffset SeekO( wxFileOffset WXUNUSED(pos), wxSeekMode WXUNUSED(mode) )
    { return -1; }
  wxFileOffset TellO() const
    { return -1; }

 protected:
  wxSocketBase *m_o_socket;

  size_t OnSysWrite(const void *buffer, size_t bufsize);

    DECLARE_NO_COPY_CLASS(wxSocketOutputStream)
};

class WXDLLIMPEXP_NET wxSocketInputStream : public wxInputStream
{
 public:
  wxSocketInputStream(wxSocketBase& s);
  virtual ~wxSocketInputStream();

  wxFileOffset SeekI( wxFileOffset WXUNUSED(pos), wxSeekMode WXUNUSED(mode) )
    { return -1; }
  wxFileOffset TellI() const
    { return -1; }

 protected:
  wxSocketBase *m_i_socket;

  size_t OnSysRead(void *buffer, size_t bufsize);

    DECLARE_NO_COPY_CLASS(wxSocketInputStream)
};

class WXDLLIMPEXP_NET wxSocketStream : public wxSocketInputStream,
                   public wxSocketOutputStream
{
 public:
  wxSocketStream(wxSocketBase& s);
  virtual ~wxSocketStream();

  DECLARE_NO_COPY_CLASS(wxSocketStream)
};

#endif
  // wxUSE_SOCKETS && wxUSE_STREAMS

#endif
  // __SCK_STREAM_H__
