/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */


#include "PrecompiledHeader.h"

#include "Common.h"
#include "deci2.h"

struct DECI2_NETMP_HEADER{
	DECI2_HEADER	h;		//+00
	u8				code,	//+08
					result;	//+09
};		//=0A

struct DECI2_NETMP_CONNECT{
	u8				priority,	//+00
					_pad;		//+01
	u16				protocol;	//+02
};			//=04

char				d2_message[100];
int					d2_count=1;
DECI2_NETMP_CONNECT	d2_connect[50]={0xFF, 0, 0x400};

void D2_NETMP(const u8 *inbuffer, u8 *outbuffer, char *message){
	DECI2_NETMP_HEADER	*in=(DECI2_NETMP_HEADER*)inbuffer,
						*out=(DECI2_NETMP_HEADER*)outbuffer;
	u8	*data=(u8*)in+sizeof(DECI2_NETMP_HEADER);
	DECI2_NETMP_CONNECT	*connect=(DECI2_NETMP_CONNECT*)data;	//connect
	int					i, n;
	static char			p[100], line[1024];
	u64	EEboot, IOPboot;										//reset
	u16	node;

	memcpy(outbuffer, inbuffer, 128*1024);//BUFFERSIZE
	out->h.length=sizeof(DECI2_NETMP_HEADER);
	out->code++;
	out->result=0;	//ok
	switch(in->code){
		case 0:
			n=(in->h.length-sizeof(DECI2_NETMP_HEADER)) / sizeof(DECI2_NETMP_CONNECT);
			sprintf(line, "code=CONNECT");
			for (i=0; i<n; i++){
				sprintf(p, " %04X/%d", connect[i].protocol, connect[i].priority);
				strcat(line, p);
			}
			memcpy(&d2_connect[n], connect, n*sizeof(DECI2_NETMP_CONNECT));
			d2_count+=n;
			writeData(outbuffer);
			break;
		case 2:
									data+=2;
			EEboot =*(u64*)data;	data+=8;
			IOPboot=*(u64*)data;
			sprintf(line, "code=RESET EE=0x%I64X IOP=0x%I64X", EEboot, IOPboot);
			////////////////////////////hack
			//data=(u8*)out+sizeof(DECI2_NETMP_HEADER);
			//*data++=6;
			//out->h.length=data-(u8*)out;
			writeData(outbuffer);

			node=(u16)'I';
			sendDCMP(PROTO_DCMP, 'H', 'H', 2, 0, (char*)&node, sizeof(node));

			node=(u16)'E';
			sendDCMP(PROTO_DCMP, 'H', 'H', 2, 0, (char*)&node, sizeof(node));

			node=PROTO_ILOADP;
			sendDCMP(PROTO_DCMP, 'I', 'H', 2, 1, (char*)&node, sizeof(node));

			for (i=0; i<10; i++){
				node=PROTO_ETTYP+i;
				sendDCMP(PROTO_DCMP, 'E', 'H', 2, 1, (char*)&node, sizeof(node));

				node=PROTO_ITTYP+i;
				sendDCMP(PROTO_DCMP, 'E', 'H', 2, 1, (char*)&node, sizeof(node));
			}
			node=PROTO_ETTYP+0xF;
			sendDCMP(PROTO_DCMP, 'E', 'H', 2, 1, (char*)&node, sizeof(node));

			node=PROTO_ITTYP+0xF;
			sendDCMP(PROTO_DCMP, 'E', 'H', 2, 1, (char*)&node, sizeof(node));
			break;
		case 4://[OK]
			sprintf(line, "code=MESSAGE %s", data);//null terminated by the memset with 0 call
			strcpy(d2_message, (char*)data);
			writeData(outbuffer);
			break;
		case 6://[ok]
			sprintf(line, "code=STATUS");
			data=(u8*)out+sizeof(DECI2_NETMP_HEADER)+2;
			/*
			memcpy(data, d2_connect, 1*sizeof(DECI2_NETMP_CONNECT));
			data+=1*sizeof(DECI2_NETMP_CONNECT);
			*(u32*)data=1;//quite fast;)
			data+=4;
			memcpy(data, d2_message, strlen(d2_message));
			data+=strlen(d2_message);
			*(u32*)data=0;//null end the string on a word boundary
			data+=3;data=(u8*)((int)data & 0xFFFFFFFC);*/

			out->h.length=data-(u8*)out;
			writeData(outbuffer);
			break;
		case 8:
			sprintf(line, "code=KILL protocol=0x%04X", *(u16*)data);
			writeData(outbuffer);
			break;
		case 10:
			sprintf(line, "code=VERSION %s", data);
			data=(u8*)out+sizeof(DECI2_NETMP_HEADER);
			strcpy((char*)data, "0.2.0");data+=strlen("0.2.0");//emu version;)
			out->h.length=data-(u8*)out;
			writeData(outbuffer);
			break;
		default:
			sprintf(line, "code=%d[unknown] result=%d", in->code, in->result);
			writeData(outbuffer);
	}
	sprintf(message, "[NETMP %c->%c/%04X] %s", in->h.source, in->h.destination, in->h.length, line);
}
