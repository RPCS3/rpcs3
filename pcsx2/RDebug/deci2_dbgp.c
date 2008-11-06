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
#include "PsxCommon.h"
#include "VUmicro.h"
#include "deci2.h"

typedef struct tag_DECI2_DBGP_HEADER{
    DECI2_HEADER	h;		//+00
	u16				id;		//+08
	u8				type,	//+0A
					code,	//+0B
					result,	//+0C
					count;	//+0D
	u16				_pad;	//+0E
} DECI2_DBGP_HEADER;		//=10

typedef struct tag_DECI2_DBGP_CONF{
	u32	major_ver,			//+00
		minor_ver,			//+04
		target_id,			//+08
		_pad,				//+0C
		mem_align,			//+10
		_pad2,				//+14
		reg_size,			//+18
		nreg,				//+1C
		nbrkpt,				//+20
		ncont,				//+24
		nstep,				//+28
		nnext,				//+2C
		mem_limit_align,	//+30
		mem_limit_size,		//+34
		run_stop_state,		//+38
		hdbg_area_addr,		//+3C
		hdbg_area_size;		//+40
} DECI2_DBGP_CONF;			//=44

DECI2_DBGP_CONF
cpu={3, 0, PROTO_EDBGP, 0, 0x41F, 1, 7, 32, 32, 1, 0xFF, 0xFF, 0x1F, 0x400, 2, 0x80020c70, 0x100},
vu0={3, 0, PROTO_EDBGP, 0, 0x41F, 1, 7, 32, 32, 1, 0xFF, 0xFF, 0x1F, 0x400, 2, 0x80020c70, 0x100},
vu1={3, 0, PROTO_EDBGP, 0, 0x41F, 1, 7, 32, 32, 1, 0xFF, 0xFF, 0x1F, 0x400, 2, 0x80020c70, 0x100},
iop={3, 0, PROTO_IDBGP, 0, 0x00F, 1, 5, 62, 32, 1, 0x00, 0x00, 0x07, 0x200, 1, 0x0001E670, 0x100};
//iop={3, 0, PROTO_IDBGP, 0, 0x00F, 1, 5, 62, 0, 1, 0x00, 0x00, 0x07, 0x200, 0, 0x00006940, 0x100};

#pragma pack(2)
typedef struct tag_DECI2_DBGP_EREG{
	u8	kind,				//+00
		number;				//+01
	u16	_pad;				//+02
	u64	value[2];			//+04
} DECI2_DBGP_EREG;			//=14

typedef struct tag_DECI2_DBGP_IREG{
	u8	kind,				//+00
		number;				//+01
	u16	_pad;				//+02
	u32	value;				//+04
} DECI2_DBGP_IREG;			//=08

typedef struct tag_DECI2_DBGP_MEM{
	u8	space,				//+00
		align;				//+01
	u16	_pad;				//+02
	u32	address;			//+04
	u32	length;				//+08
} DECI2_DBGP_MEM;			//=0C

typedef struct tag_DECI2_DBGP_RUN{
	u32	entry,				//+00
		gp,					//+04
		_pad,				//+08
		_pad1,				//+0C
		argc;				//+10
	u32	argv[0];			//+14
} DECI2_DBGP_RUN;			//=14
#pragma pack()

void D2_DBGP(char *inbuffer, char *outbuffer, char *message, char *eepc, char *ioppc, char *eecy, char *iopcy){
	DECI2_DBGP_HEADER	*in=(DECI2_DBGP_HEADER*)inbuffer,
						*out=(DECI2_DBGP_HEADER*)outbuffer;
	u8	*data=(u8*)in+sizeof(DECI2_DBGP_HEADER);
	DECI2_DBGP_EREG		*eregs=(DECI2_DBGP_EREG*)((u8*)out+sizeof(DECI2_DBGP_HEADER));
	DECI2_DBGP_IREG		*iregs=(DECI2_DBGP_IREG*)((u8*)out+sizeof(DECI2_DBGP_HEADER));
	DECI2_DBGP_MEM		*mem  =(DECI2_DBGP_MEM*) ((u8*)out+sizeof(DECI2_DBGP_HEADER));
	DECI2_DBGP_RUN		*run  =(DECI2_DBGP_RUN*) ((u8*)in+sizeof(DECI2_DBGP_HEADER));
	static char line[1024];
	int i, s;
	
	memcpy(outbuffer, inbuffer, 128*1024);//BUFFERSIZE
	//out->h.length=sizeof(DECI2_DBGP_HEADER);
	out->type++;
	out->result=0;	//ok
	exchangeSD((DECI2_HEADER*)out);
	switch(in->type){
		case 0x00://ok
			sprintf(line, "%s/GETCONF",	in->id==0?"CPU":in->id==1?"VU0":"VU1");
			data=(u8*)out+sizeof(DECI2_DBGP_HEADER);
			if (in->h.destination=='I'){
				memcpy(data, &iop, sizeof(DECI2_DBGP_CONF));
			}else
				switch(in->id){
				case 0:memcpy(data, &cpu, sizeof(DECI2_DBGP_CONF));break;
				case 1:memcpy(data, &vu0, sizeof(DECI2_DBGP_CONF));break;
				case 2:memcpy(data, &vu1, sizeof(DECI2_DBGP_CONF));break;
				}
			break;
		case 0x02://ok
            sprintf(line, "%s/2", in->id==0?"CPU":in->id==1?"VU0":"VU1");
			break;
		case 0x04://ok
            sprintf(line, "%s/GETREG count=%d kind[0]=%d number[0]=%d",
				in->id==0?"CPU":in->id==1?"VU0":"VU1", in->count, eregs[0].kind, eregs[0].number);
			if (in->h.destination=='I'){
				for (i=0; i<in->count; i++)
					switch (iregs[i].kind){
					case 1:switch (iregs[i].number){
							case 0:iregs[i].value=psxRegs.GPR.n.lo;break;
							case 1:iregs[i].value=psxRegs.GPR.n.hi;break;
						   }
						   break;
					case 2:iregs[i].value=psxRegs.GPR.r[iregs[i].number]; break;
					case 3:
						if (iregs[i].number==14) psxRegs.CP0.n.EPC=psxRegs.pc;
						iregs[i].value=psxRegs.CP0.r[iregs[i].number];
						break;
					case 6:iregs[i].value=psxRegs.CP2D.r[iregs[i].number]; break;
					case 7:iregs[i].value=psxRegs.CP2C.r[iregs[i].number]; break;
					default:
						iregs[0].value++;//dummy; might be assert(0)
					}
			}else
				for (i=0; i<in->count; i++)
					switch (eregs[i].kind){
					case  0:memcpy(eregs[i].value, &cpuRegs.GPR.r[eregs[i].number], 16);break;
					case  1:
						switch(eregs[i].number){
						case 0:memcpy(eregs[i].value, &cpuRegs.HI.UD[0], 8);break;
						case 1:memcpy(eregs[i].value, &cpuRegs.LO.UD[0], 8);break;
						case 2:memcpy(eregs[i].value, &cpuRegs.HI.UD[1], 8);break;
						case 3:memcpy(eregs[i].value, &cpuRegs.LO.UD[1], 8);break;
						case 4:memcpy(eregs[i].value, &cpuRegs.sa, 4);		break;
						}
					case  2:
						if (eregs[i].number==14) cpuRegs.CP0.n.EPC=cpuRegs.pc;
						memcpy(eregs[i].value, &cpuRegs.CP0.r[eregs[i].number], 4);
						break;
					case  3:break;//performance counter 32x3
					case  4:break;//hw debug reg 32x8
					case  5:memcpy(eregs[i].value, &fpuRegs.fpr[eregs[i].number], 4);break;
					case  6:memcpy(eregs[i].value, &fpuRegs.fprc[eregs[i].number], 4);break;
					case  7:memcpy(eregs[i].value, &VU0.VF[eregs[i].number], 16);break;
					case  8:memcpy(eregs[i].value, &VU0.VI[eregs[i].number], 4);break;
					case  9:memcpy(eregs[i].value, &VU1.VF[eregs[i].number], 16);break;
					case 10:memcpy(eregs[i].value, &VU1.VI[eregs[i].number], 4);break;
					default:
						eregs[0].value[0]++;//dummy; might be assert(0)
					}				
			break;
		case 0x06://ok
            sprintf(line, "%s/PUTREG count=%d kind[0]=%d number[0]=%d value=%016I64X_%016I64X",
				in->id==0?"CPU":in->id==1?"VU0":"VU1", in->count, eregs[0].kind, eregs[0].number, eregs[0].value[1], eregs[0].value[0]);
			if (in->h.destination=='I'){
				for (i=0; i<in->count; i++)
					switch (iregs[i].kind){
					case 1:switch (iregs[i].number){
							case 0:psxRegs.GPR.n.lo=iregs[i].value;break;
							case 1:psxRegs.GPR.n.hi=iregs[i].value;break;
						   }
						   break;
					case 2:psxRegs.GPR.r[iregs[i].number]=iregs[i].value; break;
					case 3:
						psxRegs.CP0.r[iregs[i].number]=iregs[i].value;
						if (iregs[i].number==14) psxRegs.pc=psxRegs.CP0.n.EPC;
						break;
					case 6:psxRegs.CP2D.r[iregs[i].number]=iregs[i].value; break;
					case 7:psxRegs.CP2C.r[iregs[i].number]=iregs[i].value; break;
					default:
						;//dummy; might be assert(0)
					}
			}else
				for (i=0; i<in->count; i++)
					switch (eregs[i].kind){
					case  0:memcpy(&cpuRegs.GPR.r[eregs[i].number], eregs[i].value, 16);break;
					case  1:
						switch(eregs[i].number){
						case 0:memcpy(&cpuRegs.HI.UD[0], eregs[i].value, 8);break;
						case 1:memcpy(&cpuRegs.LO.UD[0], eregs[i].value, 8);break;
						case 2:memcpy(&cpuRegs.HI.UD[1], eregs[i].value, 8);break;
						case 3:memcpy(&cpuRegs.LO.UD[1], eregs[i].value, 8);break;
						case 4:memcpy(&cpuRegs.sa, eregs[i].value, 4);		break;
						}
						break;
					case  2:
						memcpy(&cpuRegs.CP0.r[eregs[i].number], eregs[i].value, 4);
						if (eregs[i].number==14) cpuRegs.pc=cpuRegs.CP0.n.EPC;
						break;
					case  3:break;//performance counter 32x3
					case  4:break;//hw debug reg 32x8
					case  5:memcpy(&fpuRegs.fpr[eregs[i].number], eregs[i].value, 4);break;
					case  6:memcpy(&fpuRegs.fprc[eregs[i].number], eregs[i].value, 4);break;
					case  7:memcpy(&VU0.VF[eregs[i].number], eregs[i].value, 16);break;
					case  8:memcpy(&VU0.VI[eregs[i].number], eregs[i].value, 4);break;
					case  9:memcpy(&VU1.VF[eregs[i].number], eregs[i].value, 16);break;
					case 10:memcpy(&VU1.VI[eregs[i].number], eregs[i].value, 4);break;
					default:
						;//dummy; might be assert(0)
					}				
			break;
		case 0x08://ok
			sprintf(line, "%s/RDMEM %08X/%X",
				in->id==0?"CPU":in->id==1?"VU0":"VU1", mem->address, mem->length);
			data=(u8*)out+	//kids: don't try this at home! :D
				((sizeof(DECI2_DBGP_HEADER)+sizeof(DECI2_DBGP_MEM)+(1 << mem->align) - 1) & (0xFFFFFFFF << mem->align));
			if ((mem->address & ((1 << mem->align)-1)) ||
				(mem->length  & ((1 << mem->align)-1))){
				out->result=1;
				strcat(line, " ALIGN ERROR");
				break;
			}
			if (in->h.destination=='I')
				if (PSXM(mem->address & 0x1FFFFFFF))
					memcpy(data, PSXM(mem->address & 0x1FFFFFFF), mem->length);
				else{
					out->result=0x11;
					strcat(line, " ADDRESS ERROR");
					break;
				}
			else
				switch(mem->space){
				case 0:
					if ((mem->address & 0xF0000000) == 0x70000000)
						memcpy(data, PSM(mem->address), mem->length);
					else
					if ((((mem->address & 0x1FFFFFFF)>128*1024*1024) || ((mem->address & 0x1FFFFFFF)<32*1024*1024)) && PSM(mem->address & 0x1FFFFFFF))
						   memcpy(data, PSM(mem->address & 0x1FFFFFFF), mem->length);
					else{
						out->result=0x11;
						strcat(line, " ADDRESS ERROR");
					}
					break;
				case 1:
					if (in->id==1)
						memcpy(data, &VU0.Mem[mem->address &  0xFFF], mem->length);
					else
						memcpy(data, &VU1.Mem[mem->address & 0x3FFF], mem->length);
					break;
				case 2:
					if (in->id==1)
						memcpy(data, &VU0.Micro[mem->address &  0xFFF], mem->length);
					else
						memcpy(data, &VU1.Micro[mem->address & 0x3FFF], mem->length);
					break;
				}
			out->h.length=mem->length+data-(u8*)out;
			break;
		case 0x0a://ok
			sprintf(line, "%s/WRMEM %08X/%X",
				in->id==0?"CPU":in->id==1?"VU0":"VU1", mem->address, mem->length);
			data=(u8*)in+	//kids: don't try this at home! :D
				((sizeof(DECI2_DBGP_HEADER)+sizeof(DECI2_DBGP_MEM)+(1 << mem->align) - 1) & (0xFFFFFFFF << mem->align));
			if (mem->length==4 && *(int*)data==0x0000000D)
				strcat(line, " BREAKPOINT");
			if ((mem->address & ((1 << mem->align)-1)) ||
				(mem->length  & ((1 << mem->align)-1))){
				out->result=1;
				strcat(line, " ALIGN ERROR");
				break;
			}
			if (in->h.destination=='I')
				if (PSXM(mem->address & 0x1FFFFFFF))
					memcpy(PSXM(mem->address & 0x1FFFFFFF), data, mem->length);
				else{
					out->result=0x11;
					strcat(line, " ADDRESS ERROR");
					break;
				}				
			else
				switch(mem->space){
				case 0:
					if ((mem->address & 0xF0000000) == 0x70000000)
						memcpy(PSM(mem->address), data, mem->length);
					else
					if (PSM(mem->address & 0x1FFFFFFF))
						memcpy(PSM(mem->address & 0x1FFFFFFF), data, mem->length);
					else{
						out->result=0x11;
						strcat(line, " ADDRESS ERROR");
					}
					break;
				case 1:
					if (in->id==1)
						memcpy(&VU0.Mem[mem->address &  0xFFF], data, mem->length);
					else
						memcpy(&VU1.Mem[mem->address & 0x3FFF], data, mem->length);
					break;
				case 2:
					if (in->id==1)
						memcpy(&VU0.Micro[mem->address &  0xFFF], data, mem->length);
					else
						memcpy(&VU1.Micro[mem->address & 0x3FFF], data, mem->length);
					break;
				}
			out->h.length=sizeof(DECI2_DBGP_HEADER)+sizeof(DECI2_DBGP_MEM);
			break;
		case 0x10://ok
			sprintf(line, "%s/GETBRKPT count=%d",
				in->id==0?"CPU":in->id==1?"VU0":"VU1", in->count);
			data=(u8*)out+sizeof(DECI2_DBGP_HEADER);
			if (in->h.destination=='I')	memcpy(data, ibrk, out->count=ibrk_count);
			else						memcpy(data, ebrk, out->count=ebrk_count);
			out->h.length=sizeof(DECI2_DBGP_HEADER)+out->count*sizeof(DECI2_DBGP_BRK);
			break;
		case 0x12://ok [does not break on iop brkpts]
			sprintf(line, "%s/PUTBRKPT count=%d",
				in->id==0?"CPU":in->id==1?"VU0":"VU1", in->count);
			out->h.length=sizeof(DECI2_DBGP_HEADER);
			if (in->count>32){
				out->result=1;
				strcat(line, "TOO MANY");
				break;
			}
			if (in->h.destination=='I')	memcpy(ibrk, data, ibrk_count=in->count);
			else						memcpy(ebrk, data, ebrk_count=in->count);
			out->count=0;
			break;
		case 0x14://ok, [w/o iop]
			sprintf(line, "%s/BREAK count=%d",
				in->id==0?"CPU":in->id==1?"VU0":"VU1", in->count);
			if (in->h.destination=='I')
				;
			else{
				out->result = ( InterlockedExchange(&runStatus, STOP)==STOP ? 
					0x20 : 0x21 );
				out->code=0xFF;
				Sleep(50);
			}		
			break;
		case 0x16://ok, [w/o iop]
			sprintf(line, "%s/CONTINUE code=%s count=%d",
				in->id==0?"CPU":in->id==1?"VU0":"VU1",
				in->code==0?"CONT":in->code==1?"STEP":"NEXT", in->count);
			if (in->h.destination=='I')
				;
			else{
				InterlockedExchange(&runStatus, STOP);
				Sleep(100);//first get the run thread to Wait state
				runCount=in->count;
				runCode=in->code;
#ifdef _WIN32
				SetEvent(runEvent);//kick it
#endif
			}
			break;
		case 0x18://ok [without argc/argv stuff]
			sprintf(line, "%s/RUN code=%d count=%d entry=0x%08X gp=0x%08X argc=%d",
				in->id==0?"CPU":in->id==1?"VU0":"VU1", in->code, in->count,
				run->entry, run->gp, run->argc);
			cpuRegs.CP0.n.EPC=cpuRegs.pc=run->entry;
			cpuRegs.GPR.n.gp.UL[0]=run->gp;
//			threads_array[0].argc = run->argc;
			for (i=0, s=0; i<(int)run->argc; i++)	s+=run->argv[i];
			memcpy(PSM(0), &run->argv[run->argc], s);
//			threads_array[0].argstring = 0;
			InterlockedExchange(&runStatus, STOP);
			Sleep(1000);//first get the run thread to Wait state
			runCount=0;
			runCode=0xFF;
#ifdef _WIN32
			SetEvent(runEvent);//awake it
#endif
			out->h.length=sizeof(DECI2_DBGP_HEADER);
			break;
		default:
			sprintf(line, "type=0x%02X code=%d count=%d [unknown]", in->type, in->code, in->count);
	}
	sprintf(message, "[DBGP %c->%c/%04X] %s", in->h.source, in->h.destination, in->h.length, line);
	sprintf(eepc,  "%08X", cpuRegs.pc);
	sprintf(ioppc, "%08X", psxRegs.pc);
	sprintf(eecy,  "%d", cpuRegs.cycle);
	sprintf(iopcy, "%d", psxRegs.cycle);
	writeData(outbuffer);
}

void sendBREAK(u8 source, u16 id, u8 code, u8 result, u8 count){
	static DECI2_DBGP_HEADER tmp;
	tmp.h.length	=sizeof(DECI2_DBGP_HEADER);
	tmp.h._pad		=0;
	tmp.h.protocol	=( source=='E' ? PROTO_EDBGP : PROTO_IDBGP );
	tmp.h.source	=source;
	tmp.h.destination='H';
	tmp.id			=id;
	tmp.type		=0x15;
    tmp.code		=code;
	tmp.result		=result;
	tmp.count		=count;
	tmp._pad		=0;
	writeData((char*)&tmp);
}
