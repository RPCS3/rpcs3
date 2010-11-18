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


vuMemoryReserve::vuMemoryReserve()
	: _parent( L"VU0/1 on-chip memory", VU1_PROGSIZE + VU1_MEMSIZE + VU0_PROGSIZE + VU0_MEMSIZE )
{
}

void vuMemoryReserve::Reserve()
{
	_parent::Reserve(HostMemoryMap::VUmem);
	//_parent::Reserve(EmuConfig.HostMemMap.VUmem);

	u8* curpos = m_reserve.GetPtr();
	VU0.Micro	= curpos; curpos += VU0_PROGSIZE;
	VU0.Mem		= curpos; curpos += VU0_MEMSIZE;
	VU1.Micro	= curpos; curpos += VU1_PROGSIZE;
	VU1.Mem		= curpos; curpos += VU1_MEMSIZE;
}

void vuMemoryReserve::Release()
{
	_parent::Release();

	VU0.Micro	= VU0.Mem	= NULL;
	VU1.Micro	= VU1.Mem	= NULL;
}

void vuMemoryReserve::Reset()
{
	_parent::Reset();
	
	pxAssume( VU0.Mem );
	pxAssume( VU1.Mem );

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

	// === VU1 Initialization ===
	memzero(VU1.ACC);
	memzero(VU1.VF);
	memzero(VU1.VI);
	VU1.VF[0].f.x = 0.0f;
	VU1.VF[0].f.y = 0.0f;
	VU1.VF[0].f.z = 0.0f;
	VU1.VF[0].f.w = 1.0f;
	VU1.VI[0].UL = 0;
}

void SaveStateBase::vuMicroFreeze()
{
	FreezeTag( "vuMicroRegs" );

	Freeze(VU0.ACC);
	Freeze(VU0.code);

	Freeze(VU0.VF);
	Freeze(VU0.VI);

	Freeze(VU1.ACC);

	u32& temp_vu1_code = VU1.code;
	Freeze(temp_vu1_code);

	Freeze(VU1.VF);
	Freeze(VU1.VI);
}
