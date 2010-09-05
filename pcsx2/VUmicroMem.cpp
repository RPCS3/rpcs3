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

__aligned16 VURegs vuRegs[2];

static u8* m_vuAllMem = NULL;
static const uint m_vuMemSize =
	0x1000 +					// VU0micro memory
	0x4000 +					// VU0 memory
	0x4000 +					// VU1 memory
	0x4000;

void vuMicroMemAlloc()
{
	if( m_vuAllMem == NULL )
		m_vuAllMem = vtlb_malloc( m_vuMemSize, 16 );

	if( m_vuAllMem == NULL )
		throw Exception::OutOfMemory( L"VU0 and VU1 on-chip memory" );

	u8* curpos = m_vuAllMem;
	VU0.Micro	= curpos; curpos += 0x1000;
	VU0.Mem		= curpos; curpos += 0x4000;
	VU1.Micro	= curpos; curpos += 0x4000;
	VU1.Mem		= curpos;
	 //curpos += 0x4000;
}

void vuMicroMemShutdown()
{
	// -- VTLB Memory Allocation --

	vtlb_free( m_vuAllMem, m_vuMemSize );
	m_vuAllMem = NULL;
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
}

void SaveStateBase::vuMicroFreeze()
{
	FreezeTag( "vuMicro" );

	pxAssume( VU0.Mem != NULL );
	pxAssume( VU1.Mem != NULL );

	Freeze(VU0.ACC);

	// Seemingly silly and pointless use of temp var:  GCC is unable to bind packed fields
	// (appears to be a bug, tracked here: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=36566 ).
	// Dereferencing outside the context of the function (via a temp var) seems to circumvent it. --air
	
	u32& temp_vu0_code = VU0.code;
	Freeze(temp_vu0_code);

	FreezeMem(VU0.Mem,   4*1024);
	FreezeMem(VU0.Micro, 4*1024);

	Freeze(VU0.VF);
	Freeze(VU0.VI);

	Freeze(VU1.ACC);

	u32& temp_vu1_code = VU1.code;
	Freeze(temp_vu1_code);

	FreezeMem(VU1.Mem,   16*1024);
	FreezeMem(VU1.Micro, 16*1024);

	Freeze(VU1.VF);
	Freeze(VU1.VI);
}
