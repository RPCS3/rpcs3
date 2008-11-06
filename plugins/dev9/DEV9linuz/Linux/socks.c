#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "socks.h"

static int sock;
static int index;

long sockOpen(char *Device) {
	struct ifreq ifr;
	int opts;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		printf("socket error\n"); return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, Device, sizeof(ifr.ifr_name));

	if (ioctl(sock, SIOGIFINDEX, &ifr) == -1) {
		printf("SIOGIFINDEX error\n"); return -1;
	}

	index = ifr.ifr_ifindex;
	close(sock);

	sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sock == -1) return -1;
/*	opts = fcntl(sock, F_GETFL);
	if (opts < 0) return -1;
	opts|= O_NONBLOCK;
	fcntl(sock, F_SETFL, opts);
*/
	return 0;
}

void sockClose() {
	close(sock);
}

long sockSendData(void *pData, int Size) {
	struct sockaddr_ll addr;

	memset(&addr, 0, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = index;
	addr.sll_protocol = htons(ETH_P_ALL);

	return sendto(sock, pData, Size, 0, (struct sockaddr*)&addr, sizeof(addr));
}

long sockRecvData(void *pData, int Size) {
	struct sockaddr_ll addr;
	socklen_t len = sizeof(addr);
	int ret;

_start:
	memset(&addr, 0, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = index;
	addr.sll_protocol = htons(ETH_P_ALL);

	ret = recvfrom(sock, pData, Size, 0, (struct sockaddr*)&addr, &len);
	return ret;
}

