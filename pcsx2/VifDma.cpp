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
#include "Common.h"

#include "VifDma_internal.h"
#include "VUmicro.h"

int g_vifCycles = 0;

template void vuExecMicro<0>(u32 addr);
template void vuExecMicro<1>(u32 addr);
template<const u32 VIFdmanum> void vuExecMicro(u32 addr)
{
	VURegs * VU;

	if (VIFdmanum == 0) {
		VU = &VU0;
		vif0FLUSH();
	}
	else {
		VU = &VU1;
		vif1FLUSH();
	}

	if (VU->vifRegs->itops > (VIFdmanum ? 0x3ffu : 0xffu))
		Console.WriteLn("VIF%d ITOP overrun! %x", VIFdmanum, VU->vifRegs->itops);

	VU->vifRegs->itop = VU->vifRegs->itops;

	if (VIFdmanum == 1)
	{
		/* in case we're handling a VIF1 execMicro, set the top with the tops value */
		VU->vifRegs->top = VU->vifRegs->tops & 0x3ff;

		/* is DBF flag set in VIF_STAT? */
		if (VU->vifRegs->stat.DBF) {
			/* it is, so set tops with base, and clear the stat DBF flag */
			VU->vifRegs->tops = VU->vifRegs->base;
			VU->vifRegs->stat.DBF = false;
		}
		else {
			/* it is not, so set tops with base + offset,  and set stat DBF flag */
			VU->vifRegs->tops = VU->vifRegs->base + VU->vifRegs->ofst;
			VU->vifRegs->stat.DBF = true;
		}
	}

	if (!VIFdmanum)	vu0ExecMicro(addr);
	else			vu1ExecMicro(addr);
}
