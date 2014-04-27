/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/ipcbase.cpp
// Purpose:     IPC base classes
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: ipcbase.cpp 54615 2008-07-14 02:34:29Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
#endif

#include "wx/ipcbase.h"

IMPLEMENT_CLASS(wxServerBase, wxObject)
IMPLEMENT_CLASS(wxClientBase, wxObject)
IMPLEMENT_CLASS(wxConnectionBase, wxObject)

wxConnectionBase::wxConnectionBase(wxChar *buffer, int bytes)
    : m_connected(true),
      m_buffer(buffer),
      m_buffersize(bytes),
      m_deletebufferwhendone(false)
{
  if ( buffer == (wxChar *)NULL )
  { // behave like next constructor
    m_buffersize = 0;
    m_deletebufferwhendone = true;
  }
}

wxConnectionBase::wxConnectionBase()
    : m_connected(true),
      m_buffer(NULL),
      m_buffersize(0),
      m_deletebufferwhendone(true)
{
}

wxConnectionBase::wxConnectionBase(const wxConnectionBase& copy)
    : wxObject(),
      m_connected(copy.m_connected),
      m_buffer(copy.m_buffer),
      m_buffersize(copy.m_buffersize),
      m_deletebufferwhendone(false)

{
  // copy constructor would require ref-counted pointer to buffer
  wxFAIL_MSG( _T("Copy constructor of wxConnectionBase not implemented") );
}


wxConnectionBase::~wxConnectionBase(void)
{
  if ( m_deletebufferwhendone )
   delete [] m_buffer;
}

wxChar *wxConnectionBase::GetBufferAtLeast( size_t bytes )
{
  if ( m_buffersize >= bytes )
    return m_buffer;
  else
  {  // need to resize buffer
    if ( m_deletebufferwhendone )
    { // we're in charge of buffer, increase it
      delete [] m_buffer;
      // the argument specifies **byte size**, but m_buffer is of type
      // wxChar. Under unicode: sizeof(wxChar) > 1, so the buffer size is
      // bytes / sizeof(wxChar) rounded upwards.
      m_buffer = new wxChar[(bytes + sizeof(wxChar) - 1) / sizeof(wxChar)];
      m_buffersize = bytes;
      return m_buffer;
    } // user-supplied buffer, fail
    else
      return NULL;
  }
}
