#include <stdio.h>
#include <conio.h>

#include "packet32.h"
#include "ntddndis.h"

#include "socks.h"
#include "DEV9.h"

#define BUFFER_SIZE	(2048)

LPADAPTER lpAdapter;
LPPACKET lpSendPacket;
LPPACKET lpRecvPacket;
u8 buffer[BUFFER_SIZE];
u8 *buf;
int lbytes;
int tbytes;
typedef struct {
	char name[256];
	char desc[256];
} _Adapter;

_Adapter AdapterList[16];

long sockOpen(char *Device) {
	lpAdapter = PacketOpenAdapter(Device);
	if (lpAdapter == NULL) return -1;

#ifdef DEV9_LOG
	DEV9_LOG("PacketOpenAdapter %s: %p\n", Device, lpAdapter);
#endif

	if(PacketSetHwFilter(lpAdapter,NDIS_PACKET_TYPE_PROMISCUOUS)==FALSE){
		SysMessage("Warning: unable to set promiscuous mode!");
	}

	if(PacketSetBuff(lpAdapter,512000)==FALSE){
		SysMessage("Unable to set the kernel buffer!");
		return -1;
	}

	if(PacketSetReadTimeout(lpAdapter,100)==FALSE){
		SysMessage("Warning: unable to set the read tiemout!");
	}

	if((lpRecvPacket = PacketAllocatePacket())==NULL){
		SysMessage("Error: failed to allocate the LPPACKET structure.");
		return (-1);
	}
	if((lpSendPacket = PacketAllocatePacket())==NULL){
		SysMessage("Error: failed to allocate the LPPACKET structure.");
		return (-1);
	}

	lbytes=0;
	tbytes=0;

	return 0;
}

void sockClose() {
	PacketCloseAdapter(lpAdapter);
}

long sockSendData(void *pData, int Size) {
	u8 *data = (u8*)pData;
//	printf("_sendPacket %d (time=%d)\n", Size, timeGetTime());
	while (Size > 0) {
		PacketInitPacket(lpSendPacket, data, Size > 1024 ? 1024 : Size);
		if(PacketSendPacket(lpAdapter,lpSendPacket,FALSE)==FALSE){
			printf("Error: PacketSendPacket failed\n");
			return (-1);
		}
		data+= 1024; Size-= 1024;
		PacketFreePacket(lpSendPacket);
	}

	return 0;
}

int _filterPacket(u8 *_buf) {
/*	DEV9_LOG("%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", _buf[5], _buf[4], _buf[3], _buf[2], _buf[1], _buf[0]);
	DEV9_LOG("%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", _buf[11], _buf[10], _buf[9], _buf[8], _buf[7], _buf[6]);
	DEV9_LOG("%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", _buf[17], _buf[16], _buf[15], _buf[14], _buf[13], _buf[12]);
	DEV9_LOG("%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", _buf[23], _buf[22], _buf[21], _buf[20], _buf[19], _buf[18]);
*/
	if (_buf[0] == 0xff && _buf[1] == 0xff && _buf[2] == 0xff &&
		_buf[3] == 0xff && _buf[4] == 0xff && _buf[5] == 0xff) {
		return 1;
	} else
	if (_buf[0] == 0x00 && _buf[1] == 0x00 && _buf[2] == 0x00 &&
		_buf[3] == 0x00 && _buf[4] == 0x00 && _buf[5] == 0x00) {
		return 1;
	} else
	if (*((u16*)&_buf[12]) == 0x0806) {
		printf("ARP\n");
		return 1;
	}

	return 0;
}

int _recvPacket(void *pData) {
	struct bpf_hdr *hdr;
	u8 *data;
	int ret=0;
	int size;

	while (lbytes > 0) {
		hdr = (struct bpf_hdr *)buf;
//		DEV9_LOG("hdr %d,%d,%d\n", hdr->bh_hdrlen, hdr->bh_caplen, hdr->bh_datalen);
//		DEV9_LOG("lbytes %d\n", lbytes);
		data = buf+hdr->bh_hdrlen;
		size = Packet_WORDALIGN(hdr->bh_hdrlen+hdr->bh_datalen);
		buf+= size; lbytes-= size;
		if (_filterPacket(data)) {
			struct bpf_stat stat;

			ret = hdr->bh_datalen;
			memcpy(pData, data, ret);
			if(PacketGetStats(lpAdapter,&stat)==FALSE){
				printf("Warning: unable to get stats from the kernel!\n");
			}
//			printf("_recvPacket %d (tbytes=%d, packets=%d, lost=%d, time=%d)\n", ret, tbytes, stat.bs_recv,stat.bs_drop, timeGetTime());
//			printf("%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", data[5], data[4], data[3], data[2], data[1], data[0]);
			break;
		}
	}

	return ret;
}

long sockRecvData(void *pData, int Size) {
	int ret;

	ret = _recvPacket(pData);
	if (ret > 0) return ret;

	PacketInitPacket(lpRecvPacket, buffer, BUFFER_SIZE);
	if(PacketReceivePacket(lpAdapter,lpRecvPacket,TRUE)==FALSE){
		printf("Error: PacketReceivePacket failed");
		return (-1);
	}
	lbytes = lpRecvPacket->ulBytesReceived;
	tbytes+= lbytes;
//	DEV9_LOG("PacketReceivePacket %d:\n", lbytes);
	if (lbytes == 0) return 0;
	memcpy(buffer, lpRecvPacket->Buffer, lbytes);
	buf = buffer;
	PacketFreePacket(lpRecvPacket);

	return _recvPacket(pData);
}

long sockGetDevicesNum() {
	char AdapterName[8192]; // string that contains a list of the network adapters
	ULONG AdapterLength;
	char *temp,*temp1;
	int i;

	AdapterLength = sizeof(AdapterName);
	if(PacketGetAdapterNames(AdapterName,&AdapterLength)==FALSE){
		printf("Unable to retrieve the list of the adapters!\n");
		return -1;
	}
	temp=AdapterName;
	temp1=AdapterName;

	i=0;
	while (temp[0] != 0) {
		strcpy(AdapterList[i++].name, temp);
		temp+= strlen(temp)+1;
	}
	i=0; temp++;
	while (temp[0] != 0) {
		strcpy(AdapterList[i++].desc, temp);
		temp+= strlen(temp)+1;
	}

	return i;
}

char *sockGetDevice(int index) {
	return AdapterList[index].name;
}

char *sockGetDeviceDesc(int index) {
	return AdapterList[index].desc;
}

