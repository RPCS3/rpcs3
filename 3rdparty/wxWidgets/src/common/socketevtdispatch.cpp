///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/socketevtdispatch.cpp
// Purpose:     implements wxSocketEventDispatcher for platforms with no
//              socket events notification
// Author:      Angel Vidal
// Modified by:
// Created:     08.24.06
// RCS-ID:      $Id: socketevtdispatch.cpp 43976 2006-12-14 14:13:57Z VS $
// Copyright:   (c) 2006 Angel vidal
// License:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_SOCKETS

#include "wx/private/socketevtdispatch.h"
#include "wx/module.h"
#include "wx/unix/private.h"
#include "wx/gsocket.h"
#include "wx/unix/gsockunx.h"

#ifndef WX_PRECOMP
    #include "wx/hash.h"
#endif

#include <sys/time.h>
#include <unistd.h>

#ifdef HAVE_SYS_SELECT_H
#   include <sys/select.h>
#endif

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxSocketEventDispatcherEntry
// ----------------------------------------------------------------------------

class wxSocketEventDispatcherEntry: public wxObject
{
  public:
    wxSocketEventDispatcherEntry()
    {
        m_fdInput = -1; m_fdOutput = -1;
        m_socket = NULL;
    }

    int m_fdInput;
    int m_fdOutput;
    GSocket* m_socket;
};

// ----------------------------------------------------------------------------
// wxSocketEventDispatcher
// ----------------------------------------------------------------------------

wxSocketEventDispatcher* wxSocketEventDispatcher::ms_instance = NULL;

/* static */
wxSocketEventDispatcher& wxSocketEventDispatcher::Get()
{
    if ( !ms_instance )
        ms_instance = new wxSocketEventDispatcher;
    return *ms_instance;
}

wxSocketEventDispatcherEntry* wxSocketEventDispatcher::FindEntry(int fd)
{
    wxSocketEventDispatcherEntry* entry =
        (wxSocketEventDispatcherEntry*) wxHashTable::Get(fd);
    return entry;
}

void
wxSocketEventDispatcher::RegisterCallback(int fd,
                                          wxSocketEventDispatcherType socketType,
                                          GSocket* socket)
{
    wxSocketEventDispatcherEntry* entry = FindEntry(fd);
    if (!entry)
    {
        entry = new wxSocketEventDispatcherEntry();
        Put(fd, entry);
    }

    if (socketType == wxSocketEventDispatcherInput)
        entry->m_fdInput = fd;
    else
        entry->m_fdOutput = fd;

    entry->m_socket = socket;
}

void
wxSocketEventDispatcher::UnregisterCallback(int fd,
                                            wxSocketEventDispatcherType socketType)
{
    wxSocketEventDispatcherEntry* entry = FindEntry(fd);
    if (entry)
    {
        if (socketType == wxSocketEventDispatcherInput)
            entry->m_fdInput = -1;
        else
            entry->m_fdOutput = -1;

        if (entry->m_fdInput == -1 && entry->m_fdOutput == -1)
        {
            entry->m_socket = NULL;
            Delete(fd);
            delete entry;
        }
    }
}

int wxSocketEventDispatcher::FillSets(fd_set* readset, fd_set* writeset)
{
    int max_fd = 0;

    wxFD_ZERO(readset);
    wxFD_ZERO(writeset);

    BeginFind();
    wxHashTable::compatibility_iterator node = Next();
    while (node)
    {
        wxSocketEventDispatcherEntry* entry =
            (wxSocketEventDispatcherEntry*) node->GetData();

        if (entry->m_fdInput != -1)
        {
            wxFD_SET(entry->m_fdInput, readset);
            if (entry->m_fdInput > max_fd)
              max_fd = entry->m_fdInput;
        }

        if (entry->m_fdOutput != -1)
        {
            wxFD_SET(entry->m_fdOutput, writeset);
            if (entry->m_fdOutput > max_fd)
                max_fd = entry->m_fdOutput;
        }

        node = Next();
    }

    return max_fd;
}

void wxSocketEventDispatcher::AddEvents(fd_set* readset, fd_set* writeset)
{
    BeginFind();
    wxHashTable::compatibility_iterator node = Next();
    while (node)
    {
        // We have to store the next node here, because the event processing can 
        // destroy the object before we call Next()

        wxHashTable::compatibility_iterator next_node = Next();	

        wxSocketEventDispatcherEntry* entry =
            (wxSocketEventDispatcherEntry*) node->GetData();

        wxCHECK_RET(entry->m_socket, wxT("Critical: Processing a NULL socket in wxSocketEventDispatcher"));

        if (entry->m_fdInput != -1 && wxFD_ISSET(entry->m_fdInput, readset))
            entry->m_socket->Detected_Read();

        if (entry->m_fdOutput != -1 && wxFD_ISSET(entry->m_fdOutput, writeset))
            entry->m_socket->Detected_Write();;

        node = next_node;
    }
}

void wxSocketEventDispatcher::RunLoop(int timeout)
{
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout;
    fd_set readset;
    fd_set writeset;

    int max_fd = FillSets( &readset, &writeset);
    if (select( max_fd+1, &readset, &writeset, NULL, &tv ) == 0)
    {
      // No socket input/output. Don't add events.
      return;
    }
    else
    {
      AddEvents(&readset, &writeset);
    }
}

// ----------------------------------------------------------------------------
// wxSocketEventDispatcherModule
// ----------------------------------------------------------------------------

class wxSocketEventDispatcherModule: public wxModule
{
public:
    bool OnInit() { return true; }
    void OnExit() { wxDELETE(wxSocketEventDispatcher::ms_instance); }

private:
    DECLARE_DYNAMIC_CLASS(wxSocketEventDispatcherModule)
};

IMPLEMENT_DYNAMIC_CLASS(wxSocketEventDispatcherModule, wxModule)


// ----------------------------------------------------------------------------
// GSocket interface
// ----------------------------------------------------------------------------

bool GSocketGUIFunctionsTableConcrete::CanUseEventLoop()
{
    return true;
}

bool GSocketGUIFunctionsTableConcrete::OnInit(void)
{
    return 1;
}

void GSocketGUIFunctionsTableConcrete::OnExit(void)
{
}

bool GSocketGUIFunctionsTableConcrete::Init_Socket(GSocket *socket)
{
  int *m_id;

  socket->m_gui_dependent = (char *)malloc(sizeof(int)*2);
  m_id = (int *)(socket->m_gui_dependent);

  m_id[0] = -1;
  m_id[1] = -1;

  return true;
}

void GSocketGUIFunctionsTableConcrete::Destroy_Socket(GSocket *socket)
{
  free(socket->m_gui_dependent);
}

void GSocketGUIFunctionsTableConcrete::Install_Callback(GSocket *socket,
                                                        GSocketEvent event)
{
  int *m_id = (int *)(socket->m_gui_dependent);
  int c;

  if (socket->m_fd == -1)
    return;

  switch (event)
  {
    case GSOCK_LOST:       /* fall-through */
    case GSOCK_INPUT:      c = 0; break;
    case GSOCK_OUTPUT:     c = 1; break;
    case GSOCK_CONNECTION: c = ((socket->m_server) ? 0 : 1); break;
    default: return;
  }

#if 0
  if (m_id[c] != -1)
      XtRemoveInput(m_id[c]);
#endif /* 0 */

  if (c == 0)
  {
      m_id[0] = socket->m_fd;

      wxSocketEventDispatcher::Get().RegisterCallback(
              socket->m_fd, wxSocketEventDispatcherInput, socket);
  }
  else
  {
      m_id[1] = socket->m_fd;

      wxSocketEventDispatcher::Get().RegisterCallback(
              socket->m_fd, wxSocketEventDispatcherOutput, socket);
  }
}

void GSocketGUIFunctionsTableConcrete::Uninstall_Callback(GSocket *socket,
                                                          GSocketEvent event)
{
  int *m_id = (int *)(socket->m_gui_dependent);
  int c;

  switch (event)
  {
    case GSOCK_LOST:       /* fall-through */
    case GSOCK_INPUT:      c = 0; break;
    case GSOCK_OUTPUT:     c = 1; break;
    case GSOCK_CONNECTION: c = ((socket->m_server) ? 0 : 1); break;
    default: return;
  }

  if (m_id[c] != -1)
  {
      if (c == 0)
          wxSocketEventDispatcher::Get().UnregisterCallback(
                  m_id[c], wxSocketEventDispatcherInput);
      else
          wxSocketEventDispatcher::Get().UnregisterCallback(
                  m_id[c], wxSocketEventDispatcherOutput);
  }

  m_id[c] = -1;
}

void GSocketGUIFunctionsTableConcrete::Enable_Events(GSocket *socket)
{
  Install_Callback(socket, GSOCK_INPUT);
  Install_Callback(socket, GSOCK_OUTPUT);
}

void GSocketGUIFunctionsTableConcrete::Disable_Events(GSocket *socket)
{
  Uninstall_Callback(socket, GSOCK_INPUT);
  Uninstall_Callback(socket, GSOCK_OUTPUT);
}

#endif // wxUSE_SOCKETS
