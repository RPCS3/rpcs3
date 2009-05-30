/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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
#ifndef __VIFDMA_H__
#define __VIFDMA_H__

enum VifModes
{
	VIF_NORMAL_MODE = 0,
	VIF_NORMAL_MEM_MODE = 1,
	VIF_CHAIN_MODE = 2
};

enum VifMemoryLocations
{
	VIF0_STAT	= 0x10003800,
	VIF0_FBRST	= 0x10003810,
	VIF0_ERR		= 0x10003820,
	VIF0_MARK	= 0x10003830,
	VIF0_CYCLE	= 0x10003840,
	VIF0_MODE	= 0x10003850,
	VIF0_NUM		= 0x10003860,
	VIF0_MASK	= 0x10003870,
	VIF0_CODE	= 0x10003880,
	VIF0_ITOPS	= 0x10003890,
	VIF0_ITOP	= 0x100038d0,
	VIF0_TOP		= 0x100038e0,
	VIF0_R0		= 0x10003900,
	VIF0_R1		= 0x10003910,
	VIF0_R2		= 0x10003920,
	VIF0_R3		= 0x10003930,
	VIF0_C0		= 0x10003940,
	VIF0_C1		= 0x10003950,
	VIF0_C2		= 0x10003960,
	VIF0_C3		= 0x10003970,
	
	VIF1_STAT	= 0x10003c00,
	VIF1_FBRST	= 0x10003c10,
	VIF1_ERR		= 0x10003c20,
	VIF1_MARK	= 0x10003c30,
	VIF1_CYCLE	= 0x10003c40,
	VIF1_MODE	= 0x10003c50,
	VIF1_NUM		= 0x10003c60,
	VIF1_MASK	= 0x10003c70,
	VIF1_CODE	= 0x10003c80,
	VIF1_ITOPS	= 0x10003c90,
	VIF1_BASE	= 0x10003ca0,
	VIF1_OFST	= 0x10003cb0,
	VIF1_TOPS	= 0x10003cc0,
	VIF1_ITOP	= 0x10003cd0,
	VIF1_TOP		= 0x10003ce0,
	VIF1_R0		= 0x10003d00,
	VIF1_R1		= 0x10003d10,
	VIF1_R2		= 0x10003d20,
	VIF1_R3		= 0x10003d30,
	VIF1_C0		= 0x10003d40,
	VIF1_C1		= 0x10003d50,
	VIF1_C2		= 0x10003d60,
	VIF1_C3		= 0x10003d70
};

struct vifCode {
   u32 addr;
   u32 size;
   u32 cmd;
   u16 wl;
   u16 cl;
};

// NOTE, if debugging vif stalls, use sega classics, spyro, gt4, and taito
struct vifStruct {
	vifCode tag;
	int cmd;
	int irq;
	int cl;
	int qwcalign;
	u8 usn;
	
	bool done;
	bool vifstalled;
	bool stallontag;
	
	u8 irqoffset; // 32bit offset where next vif code is
	u32 savedtag; // need this for backwards compat with save states
	u32 vifpacketsize;
	u8 inprogress;
	u8 dmamode;
};

extern vifStruct vif0, vif1;
extern int Path3progress;

void __fastcall UNPACK_S_32( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_S_16u( u32 *dest, u32 *data, int size );
void __fastcall UNPACK_S_16s( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_S_8u( u32 *dest, u32 *data, int size );
void __fastcall UNPACK_S_8s( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_V2_32( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_V2_16u( u32 *dest, u32 *data, int size );
void __fastcall UNPACK_V2_16s( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_V2_8u( u32 *dest, u32 *data, int size );
void __fastcall UNPACK_V2_8s( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_V3_32( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_V3_16u( u32 *dest, u32 *data, int size );
void __fastcall UNPACK_V3_16s( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_V3_8u( u32 *dest, u32 *data, int size );
void __fastcall UNPACK_V3_8s( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_V4_32( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_V4_16u( u32 *dest, u32 *data, int size );
void __fastcall UNPACK_V4_16s( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_V4_8u( u32 *dest, u32 *data, int size );
void __fastcall UNPACK_V4_8s( u32 *dest, u32 *data, int size );

void __fastcall UNPACK_V4_5( u32 *dest, u32 *data, int size );

void vifDmaInit();
void vif0Init();
void vif1Init();
extern void  vif0Interrupt();
extern void  vif1Interrupt();

void vif0Write32(u32 mem, u32 value);
void vif1Write32(u32 mem, u32 value);

void vif0Reset();
void vif1Reset();

#endif
