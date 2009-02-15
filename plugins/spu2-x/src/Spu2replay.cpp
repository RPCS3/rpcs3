//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <stdio.h>

#include "spu2.h"

FILE* s2rfile;

void s2r_write16(s16 data)
{
	fwrite(&data,2,1,s2rfile);
}

void s2r_write32(u32 data)
{
	fwrite(&data,4,1,s2rfile);
}

#define EMITC(i,a) s2r_write32(((u32)(i&0xFF)<<24)|(a&0xFFFFFF))

int s2r_open(char *filename)
{
	s2rfile=fopen(filename,"wb");
	return s2rfile?0:-1;
}

void s2r_readreg(u32 ticks,u32 addr)
{
	if(!s2rfile) return;
	s2r_write32(ticks);
	EMITC(0,addr);
}

void s2r_writereg(u32 ticks,u32 addr,s16 value)
{
	if(!s2rfile) return;
	s2r_write32(ticks);
	EMITC(1,addr);
	s2r_write16(value);
}

void s2r_writedma4(u32 ticks,u16*data,u32 len)
{
	u32 i;
	if(!s2rfile) return;
	s2r_write32(ticks);
	EMITC(2,len);
	for(i=0;i<len;i++,data++)
		s2r_write16(*data);
}

void s2r_writedma7(u32 ticks,u16*data,u32 len)
{
	u32 i;
	if(!s2rfile) return;
	s2r_write32(ticks);
	EMITC(3,len);
	for(i=0;i<len;i++,data++)
		s2r_write16(*data);
}

void s2r_close()
{
	if(!s2rfile) return;
	fclose(s2rfile);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// replay code

bool replay_mode=false;

u16 dmabuffer[0xFFFFF];

u32 dsp=0;

u32 lasync;
u32 pcycles;
u32 pclocks;

u32 oldlimit;

void dummy1()
{
}

void dummy4()
{
	SPU2interruptDMA4();
}

void dummy7()
{
	SPU2interruptDMA7();
}

#define Cread(a,b,c,d) if(fread(a,b,c,d)<b) break;

void CALLBACK s2r_replay(HWND hwnd, HINSTANCE hinst, LPSTR filename, int nCmdShow)
{
	// load file
	FILE *file=fopen(filename,"rb");

	if(!file) return;
	// if successful, init the plugin

	replay_mode=true;

	SPU2init();
	SPU2irqCallback(dummy1,dummy4,dummy7);
	SPU2setClockPtr(&pclocks);
	SPU2open(&dsp);

	pclocks=0;
	pcycles=0;

	SPU2async(0);

	while(!feof(file))
	{
		u32 ccycle=0;
		u32 evid=0;
		u32 sval=0;
		u32 tval=0;

		Cread(&ccycle,4,1,file);
		Cread(&sval,4,1,file);

		evid=sval>>24;
		sval&=0xFFFFFF;

		while((ccycle-lasync)>64)
		{
			lasync+=64;
			pcycles=lasync;
			pclocks=pcycles*768;

			SPU2async(pclocks);
		}
		pcycles=ccycle;
		pclocks=pcycles*768;


		switch(evid)
		{
		case 0:
			SPU2read(sval);
			break;
		case 1:
			Cread(&tval,2,1,file);
			SPU2write(sval,tval);
			break;
		case 2:
			Cread(dmabuffer,sval,2,file);
			SPU2writeDMA4Mem(dmabuffer,sval);
			break;
		case 3:
			Cread(dmabuffer,sval,2,file);
			SPU2writeDMA7Mem(dmabuffer,sval);
			break;
		default:
			// not implemented
			return;
			break;
		}
	}
	
	//shutdown
	SPU2close();
	SPU2shutdown();
	fclose(file);

	replay_mode=false;
}
