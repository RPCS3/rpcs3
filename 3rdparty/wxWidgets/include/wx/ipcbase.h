/////////////////////////////////////////////////////////////////////////////
// Name:        ipcbase.h
// Purpose:     Base classes for IPC
// Author:      Julian Smart
// Modified by:
// Created:     4/1/98
// RCS-ID:      $Id: ipcbase.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_IPCBASEH__
#define _WX_IPCBASEH__

#include "wx/defs.h"
#include "wx/object.h"
#include "wx/string.h"

enum wxIPCFormat
{
  wxIPC_INVALID =          0,
  wxIPC_TEXT =             1,  /* CF_TEXT */
  wxIPC_BITMAP =           2,  /* CF_BITMAP */
  wxIPC_METAFILE =         3,  /* CF_METAFILEPICT */
  wxIPC_SYLK =             4,
  wxIPC_DIF =              5,
  wxIPC_TIFF =             6,
  wxIPC_OEMTEXT =          7,  /* CF_OEMTEXT */
  wxIPC_DIB =              8,  /* CF_DIB */
  wxIPC_PALETTE =          9,
  wxIPC_PENDATA =          10,
  wxIPC_RIFF =             11,
  wxIPC_WAVE =             12,
  wxIPC_UNICODETEXT =      13,
  wxIPC_ENHMETAFILE =      14,
  wxIPC_FILENAME =         15, /* CF_HDROP */
  wxIPC_LOCALE =           16,
  wxIPC_PRIVATE =          20
};

class WXDLLIMPEXP_FWD_BASE wxServerBase;
class WXDLLIMPEXP_FWD_BASE wxClientBase;

class WXDLLIMPEXP_BASE wxConnectionBase: public wxObject
{
  DECLARE_CLASS(wxConnectionBase)

public:
  wxConnectionBase(wxChar *buffer, int size); // use external buffer
  wxConnectionBase(); // use internal, adaptive buffer
  wxConnectionBase(const wxConnectionBase& copy);
  virtual ~wxConnectionBase(void);

  void SetConnected( bool c ) { m_connected = c; }
  bool GetConnected() { return m_connected; }

  // Calls that CLIENT can make
  virtual bool Execute(const wxChar *data, int size = -1, wxIPCFormat format = wxIPC_TEXT ) = 0;
  virtual bool Execute(const wxString& str) { return Execute(str, -1, wxIPC_TEXT); }
  virtual wxChar *Request(const wxString& item, int *size = (int *) NULL, wxIPCFormat format = wxIPC_TEXT) = 0;
  virtual bool Poke(const wxString& item, wxChar *data, int size = -1, wxIPCFormat format = wxIPC_TEXT) = 0;
  virtual bool StartAdvise(const wxString& item) = 0;
  virtual bool StopAdvise(const wxString& item) = 0;

  // Calls that SERVER can make
  virtual bool Advise(const wxString& item, wxChar *data, int size = -1, wxIPCFormat format = wxIPC_TEXT) = 0;

  // Calls that both can make
  virtual bool Disconnect(void) = 0;

  // Callbacks to SERVER - override at will
  virtual bool OnExecute     ( const wxString& WXUNUSED(topic),
                               wxChar *WXUNUSED(data),
                               int WXUNUSED(size),
                               wxIPCFormat WXUNUSED(format) )
                             { return false; }

  virtual wxChar *OnRequest    ( const wxString& WXUNUSED(topic),
                               const wxString& WXUNUSED(item),
                               int *WXUNUSED(size),
                               wxIPCFormat WXUNUSED(format) )
                             { return (wxChar *) NULL; }

  virtual bool OnPoke        ( const wxString& WXUNUSED(topic),
                               const wxString& WXUNUSED(item),
                               wxChar *WXUNUSED(data),
                               int WXUNUSED(size),
                               wxIPCFormat WXUNUSED(format) )
                             { return false; }

  virtual bool OnStartAdvise ( const wxString& WXUNUSED(topic),
                               const wxString& WXUNUSED(item) )
                             { return false; }

  virtual bool OnStopAdvise  ( const wxString& WXUNUSED(topic),
                               const wxString& WXUNUSED(item) )
                             { return false; }

  // Callbacks to CLIENT - override at will
  virtual bool OnAdvise      ( const wxString& WXUNUSED(topic),
                               const wxString& WXUNUSED(item),
                               wxChar *WXUNUSED(data),
                               int WXUNUSED(size),
                               wxIPCFormat WXUNUSED(format) )
                             { return false; }

  // Callbacks to BOTH - override at will
  // Default behaviour is to delete connection and return true
  virtual bool OnDisconnect(void) = 0;

  // return a buffer at least this size, reallocating buffer if needed
  // returns NULL if using an inadequate user buffer - it can't be resized
  wxChar *      GetBufferAtLeast( size_t bytes );

protected:
  bool          m_connected;
private:
  wxChar *      m_buffer;
  size_t        m_buffersize;
  bool          m_deletebufferwhendone;

  // can't use DECLARE_NO_COPY_CLASS(wxConnectionBase) because we already
  // have copy ctor but still forbid the default assignment operator
  wxConnectionBase& operator=(const wxConnectionBase&);
};


class WXDLLIMPEXP_BASE wxServerBase: public wxObject
{
  DECLARE_CLASS(wxServerBase)

public:
  inline wxServerBase(void) {}
  inline ~wxServerBase(void) {}

  // Returns false on error (e.g. port number is already in use)
  virtual bool Create(const wxString& serverName) = 0;

  // Callbacks to SERVER - override at will
  virtual wxConnectionBase *OnAcceptConnection(const wxString& topic) = 0;
};

class WXDLLIMPEXP_BASE wxClientBase: public wxObject
{
  DECLARE_CLASS(wxClientBase)

public:
  inline wxClientBase(void) {}
  inline ~wxClientBase(void) {}

  virtual bool ValidHost(const wxString& host) = 0;

  // Call this to make a connection. Returns NULL if cannot.
  virtual wxConnectionBase *MakeConnection(const wxString& host,
                                           const wxString& server,
                                           const wxString& topic) = 0;

  // Callbacks to CLIENT - override at will
  virtual wxConnectionBase *OnMakeConnection(void) = 0;
};

#endif
    // _WX_IPCBASEH__
