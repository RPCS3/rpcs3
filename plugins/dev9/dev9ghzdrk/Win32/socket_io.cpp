#include <stdio.h>
#include <stdarg.h>

#include <winsock2.h>
extern "C" {
#include "dev9.h"
}
#include <Iphlpapi.h>

#include "pcap_io.h"

#include <deque>
#include <queue>
#include <vector>

//extern "C" int emu_printf(const char *fmt, ...);

mac_address gateway_mac   = { 0x76, 0x6D, 0x61, 0x63, 0x30, 0x32 };
mac_address virtual_mac   = { 0x76, 0x6D, 0x61, 0x63, 0x30, 0x31 };
mac_address broadcast_mac = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

//mac_address host_mac = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

ip_address virtual_gway_ip = { 192, 168, 1, 1};
ip_address virtual_host_ip = { 192, 168, 1, 2};

char namebuff[256];

FILE*packet_log;

int pcap_io_running=0;

class packet_info 
{
public:
	int length;
	s8  data[2048];

	packet_info()
	{
		length=0;
		memset(data,0,2048);
	}

	packet_info(const packet_info& pkt)
	{
		length=pkt.length;
		memcpy(data,pkt.data,length);
	}

	packet_info(int l, void*d)
	{
		length=l;
		memcpy(data,d,l);
	}
};

std::queue<packet_info> recv_queue;


int ip_checksum(u16 *data, int length)
{
	int n=(length+1)>>1;
	int s=0;
	for(int i=0;i<n;i++)
	{
		int v=data[i];
		s=s+v;
	}
	s = (s&0xFFFF)+(s>>16);
	return s;
}

//
int pcap_io_init(char *adapter)
{
	WSADATA wsaData;

	emu_printf(" * Socket IO: Initializing virtual gateway...",adapter);

	WSAStartup(0x0200,&wsaData);

	packet_log=fopen("logs/packet.log","w");

	pcap_io_running=1;

	emu_printf("Ok.\n");
	return 0;
}

int pcap_io_send(void* packet, int plen)
{
	emu_printf(" * Socket IO: Sending %d byte packet.\n",plen);

	if(packet_log)
	{
		int i=0;
		int n=0;

		fprintf(packet_log,"PACKET SEND: %d BYTES\n",plen);
		for(i=0,n=0;i<plen;i++)
		{
			fprintf(packet_log,"%02x",((unsigned char*)packet)[i]);
			n++;
			if(n==16)
			{
				fprintf(packet_log,"\n");
				n=0;
			}
			else
			fprintf(packet_log," ");
		}
		fprintf(packet_log,"\n");
	}

	ethernet_header *eth = (ethernet_header*)packet;

	if(mac_compare(eth->dst,broadcast_mac)==0) //broadcast packets
	{
		if(eth->protocol == 0x0608) //ARP
		{
			arp_packet *arp = (arp_packet*)((s8*)packet+sizeof(ethernet_header));
			if(arp->operation == 0x0100) //ARP request
			{
				if(ip_compare(arp->p_dst,virtual_gway_ip)==0) //it's trying to resolve the virtual gateway's mac addr
				{
					full_arp_packet p;
					p.header.src = gateway_mac;
					p.header.dst = eth->src;
					p.header.protocol = 0x0608;
					p.arp.h_addr_len=6;
					p.arp.h_dst = eth->src;
					p.arp.h_src = gateway_mac;
					p.arp.p_addr_len = 4;
					p.arp.p_dst = arp->p_src;
					p.arp.p_src = virtual_gway_ip;
					p.arp.protocol = 0x0008;
					p.arp.operation = 0x0200;
					
					//packet_info pkt(sizeof(p),&p)
					recv_queue.push(packet_info(sizeof(p),&p));
				}
			}
		}
	}
	else if(mac_compare(eth->dst,gateway_mac)==0)
	{
		if(eth->protocol == 0x0008) //IP
		{
			ip_header *ip = (ip_header*)((s8*)packet+sizeof(ethernet_header));

			if((ip->proto == 0x11) && (ip->dst.bytes[0]!=192)) //UDP (non-local)
			{
				//
				//if(ip->
			}
			else
			if(ip->proto == 0x01) //ICMP
			{
				if (ip_compare(ip->dst,virtual_gway_ip)==0) //PING to gateway
				{
					static u8 icmp_packet[1024];

					memcpy(icmp_packet,packet,plen);

					ethernet_header *eh = (ethernet_header *)icmp_packet;

					eh->dst=eth->src;
					eh->src=gateway_mac;
					
					ip_header *iph = (ip_header*)(eh+1);

					iph->dst = ip->src;
					iph->src = virtual_gway_ip;

					iph->hdr_csum = 0;

					int sum = ip_checksum((u16*)iph,sizeof(ip_header));
					iph->hdr_csum = sum;

					icmp_header *ich = (icmp_header*)(iph+1);

					ich->type=0;
					ich->code=0;
					ich->csum=0;

					sum = ip_checksum((u16*)ich,iph->len-sizeof(ip_header));
					ich->csum = sum;

					recv_queue.push(packet_info(plen,&icmp_packet));

				}
				else if (ip->dst.bytes[0] != 192) //PING to external
				{
					static u8 icmp_packet[1024];

					memcpy(icmp_packet,packet,plen);

					ethernet_header *eh = (ethernet_header *)icmp_packet;

					eh->dst=eth->src;
					eh->src=gateway_mac;
					
					ip_header *iph = (ip_header*)(eh+1);

					iph->dst = ip->src;
					iph->src = virtual_gway_ip;

					iph->hdr_csum = 0;

					int sum = ip_checksum((u16*)iph,sizeof(ip_header));
					iph->hdr_csum = sum;

					icmp_header *ich = (icmp_header*)(iph+1);

					ich->type=0;
					ich->code=0;
					ich->csum=0;

					sum = ip_checksum((u16*)ich,iph->len-sizeof(ip_header));
					ich->csum = sum;

					recv_queue.push(packet_info(plen,&icmp_packet));

				}
			}
		}
	}

	return 0;
}

int pcap_io_recv(void* packet, int max_len)
{
	if(pcap_io_running<=0)
		return -1;

	if(!recv_queue.empty())
	{
		packet_info pkt(recv_queue.front());
		recv_queue.pop();

		memcpy(packet,pkt.data,pkt.length);

		if(packet_log)
		{
			int i=0;
			int n=0;
			int plen=pkt.length;

			fprintf(packet_log,"PACKET RECV: %d BYTES\n",plen);
			for(i=0,n=0;i<plen;i++)
			{
				fprintf(packet_log,"%02x",((unsigned char*)packet)[i]);
				n++;
				if(n==16)
				{
					fprintf(packet_log,"\n");
					n=0;
				}
				else
				fprintf(packet_log," ");
			}
			fprintf(packet_log,"\n");
		}

		return pkt.length;
	}

	return -1;
}

void pcap_io_close()
{
	if(packet_log)
		fclose(packet_log);
	pcap_io_running=0;
	WSACleanup();
}

int pcap_io_get_dev_num()
{
	return 1;
}

char* pcap_io_get_dev_name(int num)
{
	return "SockIO";
}

char* pcap_io_get_dev_desc(int num)
{
	return "Simulated Ethernet Adapter";
}