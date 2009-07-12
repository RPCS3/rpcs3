///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/ole/uuid.cpp
// Purpose:     implements Uuid class, see uuid.h for details
// Author:      Vadim Zeitlin
// Modified by:
// Created:     12.09.96
// RCS-ID:      $Id: uuid.cpp 55125 2008-08-18 20:04:58Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// Declarations
// ============================================================================

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if defined(__BORLANDC__)
#pragma hdrstop
#endif

#if wxUSE_OLE && ( wxUSE_DRAG_AND_DROP || (defined(__WXDEBUG__) && wxUSE_DATAOBJ) )

#ifndef WX_PRECOMP
    #include "wx/msw/wrapwin.h"
#endif

#include  <rpc.h>                       // UUID related functions

#include  "wx/msw/ole/uuid.h"



// ============================================================================
// Implementation
// ============================================================================

// length of UUID in C format
#define   UUID_CSTRLEN  100     // real length is 66

// copy ctor
Uuid::Uuid(const Uuid& uuid)
{
  // bitwise copy Ok for UUIDs
  m_uuid = uuid.m_uuid;

  // force the string to be allocated by RPC
  // (we free it later with RpcStringFree)
#ifdef _UNICODE
  UuidToString(&m_uuid, (unsigned short **)&m_pszUuid);
#else
  UuidToString(&m_uuid, &m_pszUuid);
#endif

  // allocate new buffer
  m_pszCForm = new wxChar[UUID_CSTRLEN];
  // and fill it
  memcpy(m_pszCForm, uuid.m_pszCForm, UUID_CSTRLEN*sizeof(wxChar));
}

// assignment operator
Uuid& Uuid::operator=(const Uuid& uuid)
{
  m_uuid = uuid.m_uuid;

  // force the string to be allocated by RPC
  // (we free it later with RpcStringFree)
#ifdef _UNICODE
  UuidToString(&m_uuid, (unsigned short **)&m_pszUuid);
#else
  UuidToString(&m_uuid, &m_pszUuid);
#endif

  // allocate new buffer if not done yet
  if ( !m_pszCForm )
    m_pszCForm = new wxChar[UUID_CSTRLEN];

  // and fill it
  memcpy(m_pszCForm, uuid.m_pszCForm, UUID_CSTRLEN*sizeof(wxChar));

  return *this;
}

bool Uuid::operator==(const Uuid& uuid) const
{
    // IsEqualGUID() returns BOOL and not bool so use an explicit comparison to
    // avoid MSVC warnings about int->bool conversion
    return IsEqualGUID(m_uuid, uuid.m_uuid) == TRUE;
}

bool Uuid::operator!=(const Uuid& uuid) const
{
    return !(*this == uuid);
}

// dtor
Uuid::~Uuid()
{
  // this string must be allocated by RPC!
  // (otherwise you get a debug breakpoint deep inside RPC DLL)
  if ( m_pszUuid )
#ifdef _UNICODE
    RpcStringFree((unsigned short **)&m_pszUuid);
#else
    RpcStringFree(&m_pszUuid);
#endif

  // perhaps we should just use a static buffer and not bother
  // with new and delete?
  if ( m_pszCForm )
    delete [] m_pszCForm;
}

// update string representation of new UUID
void Uuid::Set(const UUID &uuid)
{
  m_uuid = uuid;

  // get string representation
#ifdef _UNICODE
  UuidToString(&m_uuid, (unsigned short **)&m_pszUuid);
#else
  UuidToString(&m_uuid, &m_pszUuid);
#endif

  // cache UUID in C format
  UuidToCForm();
}

// create a new UUID
void Uuid::Create()
{
  UUID uuid;

  // can't fail
  UuidCreate(&uuid);

  Set(uuid);
}

// set the value
bool Uuid::Set(const wxChar *pc)
{
  // get UUID from string
#ifdef _UNICODE
  if ( UuidFromString((unsigned short *)pc, &m_uuid) != RPC_S_OK)
#else
  if ( UuidFromString((wxUChar *)pc, &m_uuid) != RPC_S_OK)
#endif
    // failed: probably invalid string
    return false;

  // transform it back to string to normalize it
#ifdef _UNICODE
  UuidToString(&m_uuid, (unsigned short **)&m_pszUuid);
#else
  UuidToString(&m_uuid, &m_pszUuid);
#endif

  // update m_pszCForm
  UuidToCForm();

  return true;
}

// stores m_uuid in m_pszCForm in a format required by
// DEFINE_GUID macro: i.e. something like
//  0x7D8A2281L,0x4C61,0x11D0,0xBA,0xBD,0x00,0x00,0xC0,0x18,0xBA,0x27
// m_pszUuid is of the form (no, it's not quite the same UUID :-)
//  6aadc650-67b0-11d0-bac8-0000c018ba27
void Uuid::UuidToCForm()
{
  if ( m_pszCForm == NULL )
    m_pszCForm = new wxChar[UUID_CSTRLEN];

  wsprintf(m_pszCForm, wxT("0x%8.8X,0x%4.4X,0x%4.4X,0x%2.2X,0x2.2%X,0x2.2%X,0x2.2%X,0x2.2%X,0x2.2%X,0x2.2%X,0x2.2%X"),
           m_uuid.Data1, m_uuid.Data2, m_uuid.Data3,
           m_uuid.Data4[0], m_uuid.Data4[1], m_uuid.Data4[2], m_uuid.Data4[3],
           m_uuid.Data4[4], m_uuid.Data4[5], m_uuid.Data4[6], m_uuid.Data4[7]);
}

#endif
  // wxUSE_DRAG_AND_DROP
