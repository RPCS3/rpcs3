/* -------------------------------------------------------------------------
 * Project:     GSocket (Generic Socket) for WX
 * Name:        gsockunx.h
 * Copyright:   (c) Guilhem Lavaux
 * Licence:     wxWindows Licence
 * Purpose:     GSocket Unix header
 * CVSID:       $Id: gsockunx.h 33948 2005-05-04 18:57:50Z JS $
 * -------------------------------------------------------------------------
 */

#ifndef __GSOCK_UNX_H
#define __GSOCK_UNX_H

#ifndef __GSOCKET_STANDALONE__
#include "wx/setup.h"
#endif

#if wxUSE_SOCKETS || defined(__GSOCKET_STANDALONE__)

#ifndef __GSOCKET_STANDALONE__
#include "wx/gsocket.h"
#else
#include "gsocket.h"
#endif

class GSocketGUIFunctionsTableConcrete: public GSocketGUIFunctionsTable
{
public:
    virtual bool OnInit();
    virtual void OnExit();
    virtual bool CanUseEventLoop();
    virtual bool Init_Socket(GSocket *socket);
    virtual void Destroy_Socket(GSocket *socket);
    virtual void Install_Callback(GSocket *socket, GSocketEvent event);
    virtual void Uninstall_Callback(GSocket *socket, GSocketEvent event);
    virtual void Enable_Events(GSocket *socket);
    virtual void Disable_Events(GSocket *socket);
};

class GSocket
{
public:
    GSocket();
    virtual ~GSocket();
    bool IsOk() { return m_ok; }
    void Close();
    void Shutdown();
    GSocketError SetLocal(GAddress *address);
    GSocketError SetPeer(GAddress *address);
    GAddress *GetLocal();
    GAddress *GetPeer();
    GSocketError SetServer();
    GSocket *WaitConnection();
    bool SetReusable();
    GSocketError Connect(GSocketStream stream);
    GSocketError SetNonOriented();
    int Read(char *buffer, int size);
    int Write(const char *buffer, int size);
    GSocketEventFlags Select(GSocketEventFlags flags);
    void SetNonBlocking(bool non_block);
    void SetTimeout(unsigned long millisec);
    GSocketError WXDLLIMPEXP_NET GetError();
    void SetCallback(GSocketEventFlags flags,
        GSocketCallback callback, char *cdata);
    void UnsetCallback(GSocketEventFlags flags);
    GSocketError GetSockOpt(int level, int optname, void *optval, int *optlen);
    GSocketError SetSockOpt(int level, int optname,
        const void *optval, int optlen);
    virtual void Detected_Read();
    virtual void Detected_Write();
protected:
    void Enable(GSocketEvent event);
    void Disable(GSocketEvent event);
    GSocketError Input_Timeout();
    GSocketError Output_Timeout();
    int Recv_Stream(char *buffer, int size);
    int Recv_Dgram(char *buffer, int size);
    int Send_Stream(const char *buffer, int size);
    int Send_Dgram(const char *buffer, int size);
    bool m_ok;
public:
    /* DFE: We can't protect these data member until the GUI code is updated */
    /* protected: */
  int m_fd;
  GAddress *m_local;
  GAddress *m_peer;
  GSocketError m_error;

  bool m_non_blocking;
  bool m_server;
  bool m_stream;
  bool m_establishing;
  bool m_reusable;
  unsigned long m_timeout;

  /* Callbacks */
  GSocketEventFlags m_detected;
  GSocketCallback m_cbacks[GSOCK_MAX_EVENT];
  char *m_data[GSOCK_MAX_EVENT];

  char *m_gui_dependent;

};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* Definition of GAddress */
struct _GAddress
{
  struct sockaddr *m_addr;
  size_t m_len;

  GAddressType m_family;
  int m_realfamily;

  GSocketError m_error;
};
#ifdef __cplusplus
}
#endif  /* __cplusplus */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* GAddress */

GSocketError _GAddress_translate_from(GAddress *address,
                                      struct sockaddr *addr, int len);
GSocketError _GAddress_translate_to  (GAddress *address,
                                      struct sockaddr **addr, int *len);
GSocketError _GAddress_Init_INET(GAddress *address);
GSocketError _GAddress_Init_UNIX(GAddress *address);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* wxUSE_SOCKETS || defined(__GSOCKET_STANDALONE__) */

#endif  /* __GSOCK_UNX_H */
