#ifndef __SOCKS_H__
#define __SOCKS_H__

long sockOpen(char *Device);
void sockClose();
long sockSendData(void *pData, int Size);
long sockRecvData(void *pData, int Size);
long sockGetDevicesNum();
char *sockGetDevice(int index);
char *sockGetDeviceDesc(int index);

#endif /* __SOCKS_H__*/
