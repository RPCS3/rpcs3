#ifndef __SOCKS_H__
#define __SOCKS_H__

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>

long sockOpen(char *Device);
void sockClose();
long sockSendData(void *pData, int Size);
long sockRecvData(void *pData, int Size);

#endif /* __SOCKS_H__*/
