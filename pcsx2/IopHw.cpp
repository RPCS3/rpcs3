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


#include "PrecompiledHeader.h"
#include "IopCommon.h"

#include "iR5900.h"
#include "Sio.h"

// NOTE: Any modifications to read/write fns should also go into their const counterparts
// found in iPsxHw.cpp.

void psxHwReset() {
/*	if (Config.Sio) psxHu32(0x1070) |= 0x80;
	if (Config.SpuIrq) psxHu32(0x1070) |= 0x200;*/

	memzero_ptr<0x10000>(psxH);

//	mdecInit(); //initialize mdec decoder
	cdrReset();
	cdvdReset();
	psxRcntInit();
	sioInit();
	//sio2Reset();
}

__forceinline u8 psxHw4Read8(u32 add)
{
	u16 mem = add & 0xFF;
	u8 ret = cdvdRead(mem);
	PSXHW_LOG("HwRead8 from Cdvd [segment 0x1f40], addr 0x%02x = 0x%02x", mem, ret);
	return ret;
}

__forceinline void psxHw4Write8(u32 add, u8 value)
{
	u8 mem = (u8)add;	// only lower 8 bits are relevant (cdvd regs mirror across the page)
	cdvdWrite(mem, value);
	PSXHW_LOG("HwWrite8 to Cdvd [segment 0x1f40], addr 0x%02x = 0x%02x", mem, value);
}

void psxDmaInterrupt(int n)
{
	if (HW_DMA_ICR & (1 << (16 + n)))
	{
		HW_DMA_ICR|= (1 << (24 + n));
		psxRegs.CP0.n.Cause |= 1 << (9 + n);
		iopIntcIrq( 3 );
	}
}

void psxDmaInterrupt2(int n)
{
	if (HW_DMA_ICR2 & (1 << (16 + n)))
	{
/*		if (HW_DMA_ICR2 & (1 << (24 + n))) {
			Console.WriteLn("*PCSX2*: HW_DMA_ICR2 n=%d already set", n);
		}
		if (psxHu32(0x1070) & 8) {
			Console.WriteLn("*PCSX2*: psxHu32(0x1070) 8 already set (n=%d)", n);
		}*/
		HW_DMA_ICR2|= (1 << (24 + n));
		psxRegs.CP0.n.Cause |= 1 << (16 + n);
		iopIntcIrq( 3 );
	}
}
