/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#pragma once
#include "Vif_Unpack.h"

struct vifCode {
   u32 addr;
   u32 size;
   u32 cmd;
   u16 wl;
   u16 cl;
};

union tBITBLT {
	struct {
		u32 reserved : 8;
		u32 BLTDIVIDE : 8; // This is the value we want to work out the divider for the reverse transfer
		u32 reserved2 : 6;
		u32 TRXPOS : 16;
	};
	u32 _u32;

	
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
	tBITBLT TRXPOS;		//used for reversed fifo operations, sometimes only the GS knows how big (like on HW register fifo read)!
	u32 GSLastTRXPOS;
	
	u8 irqoffset; // 32bit offset where next vif code is
	u32 savedtag; // need this for backwards compat with save states
	u32 vifpacketsize;
	u8 inprogress;
	u8 dmamode;
};

extern vifStruct* vif;
extern vifStruct  vif0, vif1;
extern u8		  schedulepath3msk;
static const int  VifCycleVoodoo = 4;

extern void vif0Init();
extern void vif0Interrupt();
extern void vif0Write32(u32 mem, u32 value);
extern void vif0Reset();

extern void vif1Interrupt();
extern void vif1Init();
extern void Vif1MskPath3();
extern void vif1Write32(u32 mem, u32 value);
extern void vif1Reset();

extern int (__fastcall *vif0Code[128])(int pass, u32 *data);
extern int (__fastcall *vif1Code[128])(int pass, u32 *data);

__forceinline static int _limit(int a, int max)
{
	return ((a > max) ? max : a);
}

enum VifModes
{
	VIF_NORMAL_TO_MEM_MODE = 0,
	VIF_NORMAL_FROM_MEM_MODE = 1,
	VIF_CHAIN_MODE = 2
};

// Generic constants
static const unsigned int VIF0intc = 4;
static const unsigned int VIF1intc = 5;

extern int g_vifCycles;

extern void vif0FLUSH();
extern void vif1FLUSH();

//------------------------------------------------------------------
// newVif SSE-optimized Row/Col Structs
//------------------------------------------------------------------

struct VifMaskTypes
{
	u32	Row0[4], Col0[4];
	u32	Row1[4], Col1[4];
};

extern __aligned16 VifMaskTypes g_vifmask; // This struct is used by newVif
