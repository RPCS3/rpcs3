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

#include "PrecompiledHeader.h"

#include "Common.h"
#include "R5900.h"
#include "VUmicro.h"
#include "sVU_zerorec.h"

// The following CpuVU objects are value types instead of handles or pointers because they are
// modified on the fly to implement VU1 Skip.

VUmicroCpu CpuVU0;		// contains a working copy of the VU0 cpu functions/API
VUmicroCpu CpuVU1;		// contains a working copy of the VU1 cpu functions/API

static void DummyExecuteVU1Block(void)
{
	VU0.VI[ REG_VPU_STAT ].UL &= ~0x100;
	VU1.vifRegs->stat &= ~VIF1_STAT_VEW; // also reset the bit (grandia 3 works)
}

void vuMicroCpuReset()
{
	CpuVU0 = CHECK_VU0REC ? recVU0 : intVU0;
	CpuVU1 = CHECK_VU1REC ? recVU1 : intVU1;
	CpuVU0.Reset();
	CpuVU1.Reset();

	// SuperVUreset will do nothing is none of the recs are initialized.
	// But it's needed if one or the other is initialized.
	SuperVUReset(-1);
}

static u8* m_vuAllMem = NULL;
static const uint m_vuMemSize = 
	0x1000 +					// VU0micro memory
	0x4000+0x800 +				// VU0 memory and VU1 registers
	0x4000 +					// VU1 memory
	0x4000;/* +					// VU1micro memory
	0x4000;		*/				// HACKFIX (see below)

// HACKFIX!  (air)
// The VIFdma1 has a nasty habit of transferring data into the 4k page of memory above
// the VU1. (oops!!)  This happens to be recLUT most of the time, which causes rapid death
// of our emulator.  So we allocate some extra space here to keep VIF1 a little happier.

// fixme - When the VIF is fixed, remove the third +0x4000 above. :)

void vuMicroMemAlloc()
{
	if( m_vuAllMem == NULL )
		m_vuAllMem = vtlb_malloc( m_vuMemSize, 16 );

	if( m_vuAllMem == NULL )
		throw Exception::OutOfMemory( "vuMicroMemInit > Failed to allocate VUmicro memory." );

	jASSUME( sizeof( VURegs ) <= 0x800 );

	u8* curpos = m_vuAllMem;
	VU0.Micro	= curpos; curpos += 0x1000;
	VU0.Mem		= curpos; curpos += 0x4000;
	g_pVU1		= (VURegs*)curpos; curpos += 0x800;	
	VU1.Micro	= curpos; curpos += 0x4000;
	VU1.Mem		= curpos; 
	 //curpos += 0x4000;
}

void vuMicroMemShutdown()
{
	// -- VTLB Memory Allocation --

	vtlb_free( m_vuAllMem, m_vuMemSize );
	m_vuAllMem = NULL;
	g_pVU1 = NULL;
}

void vuMicroMemReset()
{
	jASSUME( VU0.Mem != NULL );
	jASSUME( VU1.Mem != NULL );

	memMapVUmicro();

	// === VU0 Initialization ===
	memzero_obj(VU0.ACC);
	memzero_obj(VU0.VF);
	memzero_obj(VU0.VI);
    VU0.VF[0].f.x = 0.0f;
	VU0.VF[0].f.y = 0.0f;
	VU0.VF[0].f.z = 0.0f;
	VU0.VF[0].f.w = 1.0f;
	VU0.VI[0].UL = 0;
	memzero_ptr<4*1024>(VU0.Mem);
	memzero_ptr<4*1024>(VU0.Micro);

	/* this is kinda tricky, maxmem is set to 0x4400 here,
	   tho it's not 100% accurate, since the mem goes from
	   0x0000 - 0x1000 (Mem) and 0x4000 - 0x4400 (VU1 Regs),
	   i guess it shouldn't be a problem,
	   at least hope so :) (linuz)
	*/
	VU0.maxmem = 0x4800-4; //We are allocating 0x800 for vu1 reg's
	VU0.maxmicro = 0x1000-4;
	VU0.vuExec = vu0Exec;
	VU0.vifRegs = vif0Regs;

	// === VU1 Initialization ===
	memzero_obj(VU1.ACC);
	memzero_obj(VU1.VF);
	memzero_obj(VU1.VI);
	VU1.VF[0].f.x = 0.0f;
	VU1.VF[0].f.y = 0.0f;
	VU1.VF[0].f.z = 0.0f;
	VU1.VF[0].f.w = 1.0f;
	VU1.VI[0].UL = 0;
	memzero_ptr<16*1024>(VU1.Mem);
	memzero_ptr<16*1024>(VU1.Micro);

	VU1.maxmem   = 0x4000-4;//16*1024-4;
	VU1.maxmicro = 0x4000-4;
//	VU1.VF       = (VECTOR*)(VU0.Mem + 0x4000);
//	VU1.VI       = (REG_VI*)(VU0.Mem + 0x4200);
	VU1.vuExec   = vu1Exec;
	VU1.vifRegs  = vif1Regs;
}

void SaveState::vuMicroFreeze()
{
	FreezeTag( "vuMicro" );

	jASSUME( VU0.Mem != NULL );
	jASSUME( VU1.Mem != NULL );

	Freeze(VU0.ACC);
	Freeze(VU0.code);
	FreezeMem(VU0.Mem,   4*1024);
	FreezeMem(VU0.Micro, 4*1024);

	Freeze(VU0.VF);
	Freeze(VU0.VI);

	Freeze(VU1.ACC);
	Freeze(VU1.code);
	FreezeMem(VU1.Mem,   16*1024);
	FreezeMem(VU1.Micro, 16*1024);

	Freeze(VU1.VF);
	Freeze(VU1.VI);
}
