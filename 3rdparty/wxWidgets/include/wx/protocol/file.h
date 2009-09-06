/////////////////////////////////////////////////////////////////////////////
// Name:        file.h
// Purpose:     File protocol
// Author:      Guilhem Lavaux
// Modified by:
// Created:     1997
// RCS-ID:      $Id: file.h 43836 2006-12-06 19:20:40Z VZ $
// Copyright:   (c) 1997, 1998 Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __WX_PROTO_FILE_H__
#define __WX_PROTO_FILE_H__

#include "wx/defs.h"

#if wxUSE_PROTOCOL_FILE

#include "wx/protocol/protocol.h"

class WXDLLIMPEXP_NET wxFileProto: public wxProtocol {
  DECLARE_DYNAMIC_CLASS_NO_COPY(wxFileProto)
  DECLARE_PROTOCOL(wxFileProto)
protected:
  wxProtocolError m_error;
public:
  wxFileProto();
  virtual ~wxFileProto();

  wxProtocolError GetError() { return m_error; }
  bool Abort() { return TRUE; }
  wxInputStream *GetInputStream(const wxString& path);
};

#endif // wxUSE_PROTOCOL_FILE

#endif // __WX_PROTO_FILE_H__
