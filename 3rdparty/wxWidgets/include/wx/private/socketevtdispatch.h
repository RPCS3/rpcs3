/////////////////////////////////////////////////////////////////////////////
// Name:        wx/private/socketevtdispatch.h
// Purpose:     wxSocketEventDispatcher class
// Authors:     Angel Vidal
// Modified by:
// Created:     August 2006
// Copyright:   (c) Angel Vidal
// RCS-ID:      $Id: socketevtdispatch.h 43976 2006-12-14 14:13:57Z VS $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PRIVATE_SOCKETEVTDISPATCH_H_
#define _WX_PRIVATE_SOCKETEVTDISPATCH_H_

#include "wx/defs.h"

#if wxUSE_SOCKETS

#include "wx/hash.h"

// forward declarations
class wxSocketEventDispatcherEntry;
class GSocket;

enum wxSocketEventDispatcherType
{
    wxSocketEventDispatcherInput,
    wxSocketEventDispatcherOutput
};

class WXDLLIMPEXP_CORE wxSocketEventDispatcher : public wxHashTable
{
protected:
    wxSocketEventDispatcher() : wxHashTable(wxKEY_INTEGER) {}

public:
    // returns instance of the table
    static wxSocketEventDispatcher& Get();

    virtual ~wxSocketEventDispatcher()
    {
        WX_CLEAR_HASH_TABLE(*this)
    }

    void RegisterCallback(int fd, wxSocketEventDispatcherType socketType,
                          GSocket* socket);

    void UnregisterCallback(int fd, wxSocketEventDispatcherType socketType);

    void RunLoop(int timeout = 0);

private:
    void AddEvents(fd_set* readset, fd_set* writeset);

    int FillSets(fd_set* readset, fd_set* writeset);

    wxSocketEventDispatcherEntry* FindEntry(int fd);

private:
    static wxSocketEventDispatcher *ms_instance;

    friend class wxSocketEventDispatcherModule;
};

#endif // wxUSE_SOCKETS

#endif // _WX_PRIVATE_SOCKETEVTDISPATCH_H_
