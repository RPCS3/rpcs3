/* -------------------------------------------------------------------------
 * Project:     GSocket (Generic Socket)
 * Name:        gsocket.h
 * Author:      Guilhem Lavaux
 *              Guillermo Rodriguez Garcia <guille@iies.es> (maintainer)
 * Copyright:   (c) Guilhem Lavaux
 * Licence:     wxWindows Licence
 * Purpose:     GSocket include file (system independent)
 * CVSID:       $Id: gsocket.h 33948 2005-05-04 18:57:50Z JS $
 * -------------------------------------------------------------------------
 */

#ifndef __GSOCKET_H
#define __GSOCKET_H

#ifndef __GSOCKET_STANDALONE__
#include "wx/defs.h"

#include "wx/dlimpexp.h" /* for WXDLLIMPEXP_NET */

#endif

#if wxUSE_SOCKETS || defined(__GSOCKET_STANDALONE__)

#include <stddef.h>

/*
   Including sys/types.h under cygwin results in the warnings about "fd_set
   having been defined in sys/types.h" when winsock.h is included later and
   doesn't seem to be necessary anyhow. It's not needed under Mac neither.
 */
#if !defined(__WXMAC__) && !defined(__CYGWIN__) && !defined(__WXWINCE__)
#include <sys/types.h>
#endif

#ifdef __WXWINCE__
#include <stdlib.h>
#endif

class GSocket;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GAddress GAddress;

typedef enum {
  GSOCK_NOFAMILY = 0,
  GSOCK_INET,
  GSOCK_INET6,
  GSOCK_UNIX
} GAddressType;

typedef enum {
  GSOCK_STREAMED,
  GSOCK_UNSTREAMED
} GSocketStream;

typedef enum {
  GSOCK_NOERROR = 0,
  GSOCK_INVOP,
  GSOCK_IOERR,
  GSOCK_INVADDR,
  GSOCK_INVSOCK,
  GSOCK_NOHOST,
  GSOCK_INVPORT,
  GSOCK_WOULDBLOCK,
  GSOCK_TIMEDOUT,
  GSOCK_MEMERR,
  GSOCK_OPTERR
} GSocketError;

/* See below for an explanation on how events work.
 */
typedef enum {
  GSOCK_INPUT  = 0,
  GSOCK_OUTPUT = 1,
  GSOCK_CONNECTION = 2,
  GSOCK_LOST = 3,
  GSOCK_MAX_EVENT = 4
} GSocketEvent;

enum {
  GSOCK_INPUT_FLAG = 1 << GSOCK_INPUT,
  GSOCK_OUTPUT_FLAG = 1 << GSOCK_OUTPUT,
  GSOCK_CONNECTION_FLAG = 1 << GSOCK_CONNECTION,
  GSOCK_LOST_FLAG = 1 << GSOCK_LOST
};

typedef int GSocketEventFlags;

typedef void (*GSocketCallback)(GSocket *socket, GSocketEvent event,
                                char *cdata);


/* Functions tables for internal use by GSocket code: */

/* Actually this is a misnomer now, but reusing this name means I don't
   have to ifdef app traits or common socket code */
class GSocketGUIFunctionsTable
{
public:
    // needed since this class declares virtual members
    virtual ~GSocketGUIFunctionsTable() { }
    virtual bool OnInit() = 0;
    virtual void OnExit() = 0;
    virtual bool CanUseEventLoop() = 0;
    virtual bool Init_Socket(GSocket *socket) = 0;
    virtual void Destroy_Socket(GSocket *socket) = 0;
#ifndef __WINDOWS__
    virtual void Install_Callback(GSocket *socket, GSocketEvent event) = 0;
    virtual void Uninstall_Callback(GSocket *socket, GSocketEvent event) = 0;
#endif
    virtual void Enable_Events(GSocket *socket) = 0;
    virtual void Disable_Events(GSocket *socket) = 0;
};


/* Global initializers */

/* Sets GUI functions callbacks. Must be called *before* GSocket_Init
   if the app uses async sockets. */
void GSocket_SetGUIFunctions(GSocketGUIFunctionsTable *guifunc);

/* GSocket_Init() must be called at the beginning */
int GSocket_Init(void);

/* GSocket_Cleanup() must be called at the end */
void GSocket_Cleanup(void);


/* Constructors / Destructors */

GSocket *GSocket_new(void);


/* GAddress */

GAddress *GAddress_new(void);
GAddress *GAddress_copy(GAddress *address);
void GAddress_destroy(GAddress *address);

void GAddress_SetFamily(GAddress *address, GAddressType type);
GAddressType GAddress_GetFamily(GAddress *address);

/* The use of any of the next functions will set the address family to
 * the specific one. For example if you use GAddress_INET_SetHostName,
 * address family will be implicitly set to AF_INET.
 */

GSocketError GAddress_INET_SetHostName(GAddress *address, const char *hostname);
GSocketError GAddress_INET_SetAnyAddress(GAddress *address);
GSocketError GAddress_INET_SetHostAddress(GAddress *address,
                                          unsigned long hostaddr);
GSocketError GAddress_INET_SetPortName(GAddress *address, const char *port,
                                       const char *protocol);
GSocketError GAddress_INET_SetPort(GAddress *address, unsigned short port);

GSocketError GAddress_INET_GetHostName(GAddress *address, char *hostname,
                                       size_t sbuf);
unsigned long GAddress_INET_GetHostAddress(GAddress *address);
unsigned short GAddress_INET_GetPort(GAddress *address);

/* TODO: Define specific parts (INET6, UNIX) */

GSocketError GAddress_UNIX_SetPath(GAddress *address, const char *path);
GSocketError GAddress_UNIX_GetPath(GAddress *address, char *path, size_t sbuf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

# if defined(__WINDOWS__)
#  include "wx/msw/gsockmsw.h"
# elif defined(__WXMAC__) && !defined(__DARWIN__)
#  include "wx/mac/gsockmac.h"
# else
#  include "wx/unix/gsockunx.h"
# endif

#endif    /* wxUSE_SOCKETS || defined(__GSOCKET_STANDALONE__) */

#endif    /* __GSOCKET_H */
