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

#include "Spu2.h"
#include "RegTable.h"

const char *ParamNames[8]={"VOLL","VOLR","PITCH","ADSR1","ADSR2","ENVX","VOLXL","VOLXR"};
const char *AddressNames[6]={"SSAH","SSAL","LSAH","LSAL","NAXH","NAXL"};

__forceinline void _RegLog_( const char* action, int level, char *RName, u32 mem, u32 core, u16 value ) 
{
	if( level > 1 )
		FileLog("[%10d] SPU2 %s mem %08x (core %d, register %s) value %04x\n",
			Cycles, action, mem, core, RName, value);
}

#define RegLog( lev, rname, mem, core, val ) _RegLog_( action, lev, rname, mem, core, val )

void SPU2writeLog( const char* action, u32 rmem, u16 value ) 
{
	if( !IsDevBuild ) return;
	
	u32 vx=0, vc=0, core=0, omem, mem;
	omem=mem=rmem & 0x7FF; //FFFF;
	if (mem & 0x400) { omem^=0x400; core=1; }

	if( omem < 0x0180 )	 // Voice Params (VP)
	{
		const u32 voice = (omem & 0x1F0) >> 4;
		const u32 param = (omem & 0xF) >> 1;
		char dest[192];
		sprintf( dest, "Voice %d %s", voice,ParamNames[param] );
		RegLog( 2, dest, rmem, core, value );
	}
	else if ((omem >= 0x01C0) && (omem < 0x02DE))	// Voice Addressing Params (VA)
	{
		const u32 voice   = ((omem-0x01C0) / 12);
		const u32 address = ((omem-0x01C0) % 12)>>1;

		char dest[192];
		sprintf( dest, "Voice %d %s", voice, AddressNames[address] );
		RegLog( 2, dest, rmem, core, value );
	}
	else if ((mem >= 0x0760) && (mem < 0x07b0))
	{
		omem=mem; core=0;
		if (mem >= 0x0788) {omem-=0x28; core=1;}
		switch(omem)
		{
		    case REG_P_EVOLL:	RegLog(2,"EVOLL",rmem,core,value);	break;
			case REG_P_EVOLR:	RegLog(2,"EVOLR",rmem,core,value);	break;
			case REG_P_AVOLL:	if (core) { RegLog(2,"AVOLL",rmem,core,value); }	break;
			case REG_P_AVOLR:	if (core) { RegLog(2,"AVOLR",rmem,core,value); }	break;
			case REG_P_BVOLL:	RegLog(2,"BVOLL",rmem,core,value);	break;
			case REG_P_BVOLR:	RegLog(2,"BVOLR",rmem,core,value);	break;
			case REG_P_MVOLXL:	RegLog(2,"MVOLXL",rmem,core,value);	break;
			case REG_P_MVOLXR:	RegLog(2,"MVOLXR",rmem,core,value);	break;
			case R_IIR_ALPHA:	RegLog(2,"IIR_ALPHA",rmem,core,value);	break;
			case R_ACC_COEF_A:	RegLog(2,"ACC_COEF_A",rmem,core,value);	break;
			case R_ACC_COEF_B:	RegLog(2,"ACC_COEF_B",rmem,core,value);	break;
			case R_ACC_COEF_C:	RegLog(2,"ACC_COEF_C",rmem,core,value);	break;
			case R_ACC_COEF_D:	RegLog(2,"ACC_COEF_D",rmem,core,value);	break;
			case R_IIR_COEF:	RegLog(2,"IIR_COEF",rmem,core,value);	break;
			case R_FB_ALPHA:	RegLog(2,"FB_ALPHA",rmem,core,value);	break;
			case R_FB_X:	  	RegLog(2,"FB_X",rmem,core,value);	break;
			case R_IN_COEF_L:	RegLog(2,"IN_COEF_L",rmem,core,value);	break;
			case R_IN_COEF_R:	RegLog(2,"IN_COEF_R",rmem,core,value);	break;

		}
	}
	else if ((mem>=0x07C0) && (mem<0x07CE))
	{
		switch(mem)
		{
			case SPDIF_OUT:
				RegLog(2,"SPDIF_OUT",rmem,-1,value);
				break;
			case IRQINFO:
				RegLog(2,"IRQINFO",rmem,-1,value);
				break;
			case 0x7c4:
				if(Spdif.Unknown1 != value) ConLog(" * SPU2: SPDIF Unknown Register 1 set to %04x\n",value);
				RegLog(2,"SPDIF_UNKNOWN1",rmem,-1,value);
				break;
			case SPDIF_MODE:
				if(Spdif.Mode != value) ConLog(" * SPU2: SPDIF Mode set to %04x\n",value);
				RegLog(2,"SPDIF_MODE",rmem,-1,value);
				break;
			case SPDIF_MEDIA:
				if(Spdif.Media != value) ConLog(" * SPU2: SPDIF Media set to %04x\n",value);
				RegLog(2,"SPDIF_MEDIA",rmem,-1,value);
				break;
			case 0x7ca:
				if(Spdif.Unknown2 != value) ConLog(" * SPU2: SPDIF Unknown Register 2 set to %04x\n",value);
				RegLog(2,"SPDIF_UNKNOWN2",rmem,-1,value);
				break;
			case SPDIF_PROTECT:
				if(Spdif.Protection != value) ConLog(" * SPU2: SPDIF Copy set to %04x\n",value);
				RegLog(2,"SPDIF_PROTECT",rmem,-1,value);
				break;
		}
		UpdateSpdifMode();
	}
	else
	{
		switch(omem)
		{
			case REG_C_ATTR:
				RegLog(4,"ATTR",rmem,core,value);
				break;
			case REG_S_PMON:
				RegLog(1,"PMON0",rmem,core,value);
				break;
			case (REG_S_PMON + 2):
				RegLog(1,"PMON1",rmem,core,value);
				break;
			case REG_S_NON:
				RegLog(1,"NON0",rmem,core,value);
				break;
			case (REG_S_NON + 2):
				RegLog(1,"NON1",rmem,core,value);
				break;
			case REG_S_VMIXL:
				RegLog(1,"VMIXL0",rmem,core,value);
			case (REG_S_VMIXL + 2):
				RegLog(1,"VMIXL1",rmem,core,value);
				break;
			case REG_S_VMIXEL:
				RegLog(1,"VMIXEL0",rmem,core,value);
				break;
			case (REG_S_VMIXEL + 2):
				RegLog(1,"VMIXEL1",rmem,core,value);
				break;
			case REG_S_VMIXR:
				RegLog(1,"VMIXR0",rmem,core,value);
				break;
			case (REG_S_VMIXR + 2):
				RegLog(1,"VMIXR1",rmem,core,value);
				break;
			case REG_S_VMIXER:
				RegLog(1,"VMIXER0",rmem,core,value);
				break;
			case (REG_S_VMIXER + 2):
				RegLog(1,"VMIXER1",rmem,core,value);
				break;
			case REG_P_MMIX:
				RegLog(1,"MMIX",rmem,core,value);
				break;
			case REG_A_IRQA:
				RegLog(2,"IRQAH",rmem,core,value);
				break;
			case (REG_A_IRQA + 2):
				RegLog(2,"IRQAL",rmem,core,value);
				break;
			case (REG_S_KON + 2):
				RegLog(1,"KON1",rmem,core,value);
				break;
			case REG_S_KON:
				RegLog(1,"KON0",rmem,core,value);
				break;
			case (REG_S_KOFF + 2):
				RegLog(1,"KOFF1",rmem,core,value);
				break;
			case REG_S_KOFF:
				RegLog(1,"KOFF0",rmem,core,value);
				break;
			case REG_A_TSA:
				RegLog(2,"TSAH",rmem,core,value);
				break;
			case (REG_A_TSA + 2):
				RegLog(2,"TSAL",rmem,core,value);
				break;
			case REG_S_ENDX:
				//ConLog(" * SPU2: Core %d ENDX cleared!\n",core);
				RegLog(2,"ENDX0",rmem,core,value);
				break;
			case (REG_S_ENDX + 2):	
				//ConLog(" * SPU2: Core %d ENDX cleared!\n",core);
				RegLog(2,"ENDX1",rmem,core,value);
				break;
			case REG_P_MVOLL:
				RegLog(1,"MVOLL",rmem,core,value);
				break;
			case REG_P_MVOLR:
				RegLog(1,"MVOLR",rmem,core,value);
				break;
			case REG_S_ADMAS:
				RegLog(3,"ADMAS",rmem,core,value);
				//ConLog(" * SPU2: Core %d AutoDMAControl set to %d\n",core,value);
				break;
			case REG_P_STATX:
				RegLog(3,"STATX",rmem,core,value);
				break;
			case REG_A_ESA:
				RegLog(1,"ESAH",rmem,core,value);
				break;
			case (REG_A_ESA + 2):
				RegLog(1,"ESAL",rmem,core,value);
				break;
			case REG_A_EEA:
				RegLog(1,"EEAH",rmem,core,value);
				break;

	#define LOG_REVB_REG(n,t) \
			case R_##n: \
				RegLog(2,t "H",mem,core,value); \
				break; \
			case (R_##n + 2): \
				RegLog(2,t "L",mem,core,value); \
				break;

			LOG_REVB_REG(FB_SRC_A,"FB_SRC_A")
			LOG_REVB_REG(FB_SRC_B,"FB_SRC_B")
			LOG_REVB_REG(IIR_SRC_A0,"IIR_SRC_A0")
			LOG_REVB_REG(IIR_SRC_A1,"IIR_SRC_A1")
			LOG_REVB_REG(IIR_SRC_B1,"IIR_SRC_B1")
			LOG_REVB_REG(IIR_SRC_B0,"IIR_SRC_B0")
			LOG_REVB_REG(IIR_DEST_A0,"IIR_DEST_A0")
			LOG_REVB_REG(IIR_DEST_A1,"IIR_DEST_A1")
			LOG_REVB_REG(IIR_DEST_B0,"IIR_DEST_B0")
			LOG_REVB_REG(IIR_DEST_B1,"IIR_DEST_B1")
			LOG_REVB_REG(ACC_SRC_A0,"ACC_SRC_A0")
			LOG_REVB_REG(ACC_SRC_A1,"ACC_SRC_A1")
			LOG_REVB_REG(ACC_SRC_B0,"ACC_SRC_B0")
			LOG_REVB_REG(ACC_SRC_B1,"ACC_SRC_B1")
			LOG_REVB_REG(ACC_SRC_C0,"ACC_SRC_C0")
			LOG_REVB_REG(ACC_SRC_C1,"ACC_SRC_C1")
			LOG_REVB_REG(ACC_SRC_D0,"ACC_SRC_D0")
			LOG_REVB_REG(ACC_SRC_D1,"ACC_SRC_D1")
			LOG_REVB_REG(MIX_DEST_A0,"MIX_DEST_A0")
			LOG_REVB_REG(MIX_DEST_A1,"MIX_DEST_A1")
			LOG_REVB_REG(MIX_DEST_B0,"MIX_DEST_B0")
			LOG_REVB_REG(MIX_DEST_B1,"MIX_DEST_B1")

			default:
				RegLog(2,"UNKNOWN",rmem,core,value); spu2Ru16(mem) = value;
		}
	}
}

