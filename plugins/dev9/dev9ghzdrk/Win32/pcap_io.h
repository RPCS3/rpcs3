#pragma once
#include "net.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push,1)

typedef struct _ip_address
{
	u_char bytes[4];
} ip_address;

typedef struct _mac_address
{
	u_char bytes[6];
} mac_address;

typedef struct _ethernet_header
{
	mac_address dst;
	mac_address src;
	u_short protocol;
} ethernet_header;

typedef struct _arp_packet
{
	u_short		hw_type;
	u_short		protocol;
	u_char		h_addr_len;
	u_char		p_addr_len;
	u_short		operation;
	mac_address h_src;
	ip_address  p_src;
	mac_address h_dst;
	ip_address  p_dst;
} arp_packet;

typedef struct _ip_header {
	u_char		ver_hlen;	/* version << 4 | header length >> 2 */
	u_char		type;		/* type of service */
	u_short		len;			/* total length */
	u_short		id;			/* identification */
	u_short		offset;		/* fragment offset field */
	u_char		ttl;			/* time to live */
	u_char		proto;		/* protocol */
	u_short		hdr_csum;	/* checksum */
	ip_address	src;      /* source and dest address */
	ip_address	dst;	
} ip_header;

/* Internet Control Message Protocol Constants and Packet Format */

/* ic_type field */
#define	ICT_ECHORP	0	/* Echo reply				*/
#define	ICT_DESTUR	3	/* Destination unreachable		*/
#define	ICT_SRCQ	4	/* Source quench			*/
#define	ICT_REDIRECT	5	/* Redirect message type		*/
#define	ICT_ECHORQ	8	/* Echo request				*/
#define	ICT_TIMEX	11	/* Time exceeded			*/
#define	ICT_PARAMP	12	/* Parameter Problem			*/
#define	ICT_TIMERQ	13	/* Timestamp request			*/
#define	ICT_TIMERP	14	/* Timestamp reply			*/
#define	ICT_INFORQ	15	/* Information request			*/
#define	ICT_INFORP	16	/* Information reply			*/
#define	ICT_MASKRQ	17	/* Mask request				*/
#define	ICT_MASKRP	18	/* Mask reply				*/

/* ic_code field */
#define	ICC_NETUR	0	/* dest unreachable, net unreachable	*/
#define	ICC_HOSTUR	1	/* dest unreachable, host unreachable	*/
#define	ICC_PROTOUR	2	/* dest unreachable, proto unreachable	*/
#define	ICC_PORTUR	3	/* dest unreachable, port unreachable	*/
#define	ICC_FNADF	4	/* dest unr, frag needed & don't frag	*/
#define	ICC_SRCRT	5	/* dest unreachable, src route failed	*/

#define	ICC_NETRD	0	/* redirect: net			*/
#define	ICC_HOSTRD	1	/* redirect: host			*/
#define	IC_TOSNRD	2	/* redirect: type of service, net	*/
#define	IC_TOSHRD	3	/* redirect: type of service, host	*/

#define	ICC_TIMEX	0	/* time exceeded, ttl			*/
#define	ICC_FTIMEX	1	/* time exceeded, frag			*/

#define	IC_HLEN		8	/* octets				*/
#define	IC_PADLEN	3	/* pad length (octets)			*/

#define	IC_RDTTL	300	/* ttl for redirect routes		*/


/* ICMP packet format (following the IP header)				*/
typedef struct _icmp_header {	/* ICMP packet			*/
	char		type;		/* type of message (ICT_* above)*/
	char		code;		/* code (ICC_* above)		*/
	short		csum;		/* checksum of ICMP header+data	*/

	union	{
		struct {
			int	ic1_id:16; /* echo type, a message id	*/
			int	ic1_seq:16;/* echo type, a seq. number	*/
		} ic1;
		ip_address	ic2_gw;		/* for redirect, gateway	*/
		struct {
			char	ic3_ptr;/* pointer, for ICT_PARAMP	*/
			char	ic3_pad[IC_PADLEN];
		} ic3;
		int	ic4_mbz;	/* must be zero			*/
	} icu;
} icmp_header;

/*typedef struct _udp_header {
  u16	src_port;
  u16	dst_port;
  u16	len;
  u16	csum;
} udp_header;*/

typedef struct _full_arp_packet
{
	ethernet_header header;
	arp_packet arp;
} full_arp_packet;

#pragma pack(pop)

#define ARP_REQUEST 0x0100 //values are big-endian

extern mac_address virtual_mac;
extern mac_address broadcast_mac;

extern ip_address virtual_ip;

#define mac_compare(a,b) (memcmp(&(a),&(b),6))
#define ip_compare(a,b) (memcmp(&(a),&(b),4))

/*
int pcap_io_init(char *adapter);
int pcap_io_send(void* packet, int plen);
int pcap_io_recv(void* packet, int max_len);
void pcap_io_close();
*/
int pcap_io_get_dev_num();
char* pcap_io_get_dev_desc(int num,int md);
char* pcap_io_get_dev_name(int num,int md);

#ifdef __cplusplus
}
#endif

class PCAPAdapter : public NetAdapter
{
public:
	PCAPAdapter();
	virtual bool blocks();
	//gets a packet.rv :true success
	virtual bool recv(NetPacket* pkt);
	//sends the packet and deletes it when done (if successful).rv :true success
	virtual bool send(NetPacket* pkt);
	virtual ~PCAPAdapter();
};