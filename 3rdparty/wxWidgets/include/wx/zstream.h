/////////////////////////////////////////////////////////////////////////////
// Name:        wx/zstream.h
// Purpose:     Memory stream classes
// Author:      Guilhem Lavaux
// Modified by: Mike Wetherell
// Created:     11/07/98
// RCS-ID:      $Id: zstream.h 54688 2008-07-18 08:06:44Z MW $
// Copyright:   (c) Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////
#ifndef _WX_WXZSTREAM_H__
#define _WX_WXZSTREAM_H__

#include "wx/defs.h"

#if wxUSE_ZLIB && wxUSE_STREAMS

#include "wx/stream.h"

// Compression level
enum {
    wxZ_DEFAULT_COMPRESSION = -1,
    wxZ_NO_COMPRESSION = 0,
    wxZ_BEST_SPEED = 1,
    wxZ_BEST_COMPRESSION = 9
};

// Flags
enum {
#if WXWIN_COMPATIBILITY_2_4
    wxZLIB_24COMPATIBLE = 4, // read v2.4.x data without error
#endif
    wxZLIB_NO_HEADER = 0,    // raw deflate stream, no header or checksum
    wxZLIB_ZLIB = 1,         // zlib header and checksum
    wxZLIB_GZIP = 2,         // gzip header and checksum, requires zlib 1.2.1+
    wxZLIB_AUTO = 3          // autodetect header zlib or gzip
};

class WXDLLIMPEXP_BASE wxZlibInputStream: public wxFilterInputStream {
 public:
  wxZlibInputStream(wxInputStream& stream, int flags = wxZLIB_AUTO);
  wxZlibInputStream(wxInputStream *stream, int flags = wxZLIB_AUTO);
  virtual ~wxZlibInputStream();

  char Peek() { return wxInputStream::Peek(); }
  wxFileOffset GetLength() const { return wxInputStream::GetLength(); }

  static bool CanHandleGZip();

 protected:
  size_t OnSysRead(void *buffer, size_t size);
  wxFileOffset OnSysTell() const { return m_pos; }

 private:
  void Init(int flags);

 protected:
  size_t m_z_size;
  unsigned char *m_z_buffer;
  struct z_stream_s *m_inflate;
  wxFileOffset m_pos;
#if WXWIN_COMPATIBILITY_2_4
  bool m_24compatibilty;
#endif

  DECLARE_NO_COPY_CLASS(wxZlibInputStream)
};

class WXDLLIMPEXP_BASE wxZlibOutputStream: public wxFilterOutputStream {
 public:
  wxZlibOutputStream(wxOutputStream& stream, int level = -1, int flags = wxZLIB_ZLIB);
  wxZlibOutputStream(wxOutputStream *stream, int level = -1, int flags = wxZLIB_ZLIB);
  virtual ~wxZlibOutputStream() { Close(); }

  void Sync() { DoFlush(false); }
  bool Close();
  wxFileOffset GetLength() const { return m_pos; }

  static bool CanHandleGZip();

 protected:
  size_t OnSysWrite(const void *buffer, size_t size);
  wxFileOffset OnSysTell() const { return m_pos; }

  virtual void DoFlush(bool final);

 private:
  void Init(int level, int flags);

 protected:
  size_t m_z_size;
  unsigned char *m_z_buffer;
  struct z_stream_s *m_deflate;
  wxFileOffset m_pos;

  DECLARE_NO_COPY_CLASS(wxZlibOutputStream)
};

class WXDLLIMPEXP_BASE wxZlibClassFactory: public wxFilterClassFactory
{
public:
    wxZlibClassFactory();

    wxFilterInputStream *NewStream(wxInputStream& stream) const
        { return new wxZlibInputStream(stream); }
    wxFilterOutputStream *NewStream(wxOutputStream& stream) const
        { return new wxZlibOutputStream(stream, -1); }
    wxFilterInputStream *NewStream(wxInputStream *stream) const
        { return new wxZlibInputStream(stream); }
    wxFilterOutputStream *NewStream(wxOutputStream *stream) const
        { return new wxZlibOutputStream(stream, -1); }

    const wxChar * const *GetProtocols(wxStreamProtocolType type
                                       = wxSTREAM_PROTOCOL) const;

private:
    DECLARE_DYNAMIC_CLASS(wxZlibClassFactory)
};

class WXDLLIMPEXP_BASE wxGzipClassFactory: public wxFilterClassFactory
{
public:
    wxGzipClassFactory();

    wxFilterInputStream *NewStream(wxInputStream& stream) const
        { return new wxZlibInputStream(stream); }
    wxFilterOutputStream *NewStream(wxOutputStream& stream) const
        { return new wxZlibOutputStream(stream, -1, wxZLIB_GZIP); }
    wxFilterInputStream *NewStream(wxInputStream *stream) const
        { return new wxZlibInputStream(stream); }
    wxFilterOutputStream *NewStream(wxOutputStream *stream) const
        { return new wxZlibOutputStream(stream, -1, wxZLIB_GZIP); }

    const wxChar * const *GetProtocols(wxStreamProtocolType type
                                       = wxSTREAM_PROTOCOL) const;

private:
    DECLARE_DYNAMIC_CLASS(wxGzipClassFactory)
};

#endif
  // wxUSE_ZLIB && wxUSE_STREAMS

#endif
   // _WX_WXZSTREAM_H__

