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
#include "MTVU.h"
#include "newVif.h"

__aligned16 VU_Thread vu1Thread(CpuVU1, VU1);

// Calls the vif unpack functions from the MTVU thread
void MTVU_Unpack(void* data, VIFregisters& vifRegs) {
	bool isFill = vifRegs.cycle.cl < vifRegs.cycle.wl;
	if (newVifDynaRec) dVifUnpack<1>((u8*)data, isFill);
	else              _nVifUnpack(1, (u8*)data, vifRegs.mode, isFill);
}

// Called on Saving/Loading states...
void SaveStateBase::mtvuFreeze() {
	FreezeTag("MTVU");
	pxAssert(vu1Thread.IsDone());
	if (!IsSaving()) vu1Thread.Reset();
	Freeze(vu1Thread.vuCycles);
	Freeze(vu1Thread.vuCycleIdx);
}
