/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "Common.h"
#include "deci2.h"

typedef struct tag_DECI2_DCMP_HEADER{
	DECI2_HEADER	h;		//+00
	u8				type,	//+08
					code;	//+09
	u16				_pad;	//+0A
} DECI2_DCMP_HEADER;		//=0C

extern char d2_message[100];

void D2_DCMP(char *inbuffer, char *outbuffer, char *message){
	DECI2_DCMP_HEADER	*in=(DECI2_DCMP_HEADER*)inbuffer,
				*out=(DECI2_DCMP_HEADER*)outbuffer;
	u8	*data=(u8*)in+sizeof(DECI2_DCMP_HEADER);

	memcpy(outbuffer, inbuffer, 128*1024);//BUFFERSIZE
	out->h.length=sizeof(DECI2_DCMP_HEADER);
	switch(in->type){
		case 4://[OK]
			sprintf(message, "  [DCMP] code=MESSAGE %s", data);//null terminated by the memset with 0 call
			strcpy(d2_message, data);
			break;
		default:
			sprintf(message, "  [DCMP] code=%d[unknown] result=%d", netmp->code, netmp->result);
	}
	result->code++;
	result->result=0;	//ok
}
