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
#include "VUmicro.h"

static u8* m_vuAllMem = NULL;
static const uint m_vuMemSize =
	0x1000 +					// VU0micro memory
	0x4000+0x800 +				// VU0 memory and VU1 registers
	0x4000 +					// VU1 memory
	0x4000;

void vuMicroMemAlloc()
{
	if( m_vuAllMem == NULL )
		m_vuAllMem = vtlb_malloc( m_vuMemSize, 16 );

	if( m_vuAllMem == NULL )
		throw Exception::OutOfMemory( L"VU0 and VU1 on-chip memory" );

	pxAssume( sizeof( VURegs ) <= 0x800 );

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
	pxAssume( VU0.Mem != NULL );
	pxAssume( VU1.Mem != NULL );

	memMapVUmicro();

	// === VU0 Initialization ===
	memzero(VU0.ACC);
	memzero(VU0.VF);
	memzero(VU0.VI);
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
	memzero(VU1.ACC);
	memzero(VU1.VF);
	memzero(VU1.VI);
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

void SaveStateBase::vuMicroFreeze()
{
	FreezeTag( "vuMicro" );

	pxAssume( VU0.Mem != NULL );
	pxAssume( VU1.Mem != NULL );

	Freeze(VU0.ACC);
#ifdef __LINUX__
	// GCC is unable to bind packed fields.
	u32 temp_vu0_code = VU0.code;
	Freeze(temp_vu0_code);
#else
	Freeze(VU0.code);
#endif
	FreezeMem(VU0.Mem,   4*1024);
	FreezeMem(VU0.Micro, 4*1024);

	Freeze(VU0.VF);
	Freeze(VU0.VI);

	Freeze(VU1.ACC);
#ifdef __LINUX__
	// GCC is unable to bind packed fields.
	u32 temp_vu1_code = VU1.code;
	Freeze(temp_vu1_code);
#else
	Freeze(VU1.code);
#endif
	FreezeMem(VU1.Mem,   16*1024);
	FreezeMem(VU1.Micro, 16*1024);

	Freeze(VU1.VF);
	Freeze(VU1.VI);
}
