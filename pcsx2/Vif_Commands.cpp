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
#include "GS.h"
#include "Gif.h"
#include "Vif_Dma.h"
#include "newVif.h"
#include "VUmicro.h"

#define _vifT		template <int idx>
#define  vifX		(idx ? vif1 : vif0)
#define  vifXRegs	(idx ? (vif1Regs) : (vif0Regs))
#define  vif1Only()	{ if (!idx) { vifCMD_Null<idx>(); return; } }

_f void vuExecMicro(int idx, u32 addr) {
	VURegs* VU = nVif[idx].VU;
	if (!idx)	vif0FLUSH();
	else		vif1FLUSH();

	if (VU->vifRegs->itops  > (idx ? 0x3ffu : 0xffu)) {
		Console.WriteLn("VIF%d ITOP overrun! %x", idx, VU->vifRegs->itops);
		VU->vifRegs->itops &= (idx ? 0x3ffu : 0xffu);
	}

	VU->vifRegs->itop = VU->vifRegs->itops;

	if (idx) {
		// in case we're handling a VIF1 execMicro, set the top with the tops value
		VU->vifRegs->top = VU->vifRegs->tops & 0x3ff;

		// is DBF flag set in VIF_STAT?
		if (VU->vifRegs->stat.DBF) {
			// it is, so set tops with base, and clear the stat DBF flag
			VU->vifRegs->tops = VU->vifRegs->base;
			VU->vifRegs->stat.DBF = false;
		}
		else {
			// it is not, so set tops with base + offset, and set stat DBF flag
			VU->vifRegs->tops = VU->vifRegs->base + VU->vifRegs->ofst;
			VU->vifRegs->stat.DBF = true;
		}
	}

	if (!idx) vu0ExecMicro(addr);
	else	  vu1ExecMicro(addr);
}

_f void vifFlush(int idx) {
	if (!idx) vif0FLUSH();
	else	  vif1FLUSH();
}

u8 schedulepath3msk = 0;

void Vif1MskPath3() {

	vif1Regs->mskpath3 = schedulepath3msk & 0x1;
	//Console.WriteLn("VIF MSKPATH3 %x", vif1Regs->mskpath3);

	if (vif1Regs->mskpath3) {
		gifRegs->stat.M3P = true;
	}
	else {
		//Let the Gif know it can transfer again (making sure any vif stall isnt unset prematurely)
		Path3progress = TRANSFER_MODE;
		gifRegs->stat.IMT = false;
		CPU_INT(2, 4);
	}

	schedulepath3msk = 0;
}

//------------------------------------------------------------------
// Vif0/Vif1 Commands (VifCodes)
//------------------------------------------------------------------

_vifT void vifCMD_Base()  // BASE
{
	vif1Only();
	vif1Regs->base = vif1Regs->code & 0x3ff;
	vif1.cmd &= ~0x7f;
}

_vifT void vifCMD_DirectHL()  // DIRECT/HL
{
	vif1Only();
	int vifImm = (u16)vif1Regs->code;
	if(!vifImm) vif1.tag.size = 65536  << 2;
	else		vif1.tag.size = vifImm << 2;
}

_vifT void vifCMD_FlushE()
{
	vifFlush(idx);
	vifX.cmd &= ~0x7f;
}

_vifT void vifCMD_Flush()
{
	vif1Only();
	vifFlush(idx);
	vifX.cmd &= ~0x7f;
}

_vifT void vifCMD_FlushA()
{
	vif1Only();
	vifFlush(idx);

	// Gif is already transferring so wait for it.
	if (((Path3progress != STOPPED_MODE) || !vif1Regs->mskpath3) && gif->chcr.STR) { 
		vif1Regs->stat.VGW = true;
		CPU_INT(2, 4);
	}

	vifX.cmd &= ~0x7f;
}

_vifT void vifCMD_ITop()
{
	vifXRegs->itops = vifXRegs->code & 0x3ff;
	vifX.cmd &= ~0x7f;
}

_vifT void vifCMD_Mark()
{
	vifXRegs->mark = (u16)vifXRegs->code;
	vifXRegs->stat.MRK = true;
	vifX.cmd &= ~0x7f;
}

_vifT void vifCMD_MPG()
{
	if (!idx) vifFlush(idx); // Only Vif0 Flush!?
	int vifNum = (u8)(vifXRegs->code >> 16);
	if(!vifNum) vifNum = 256;

	vifX.tag.addr = (u16)((vifXRegs->code) << 3) & (idx ? 0x3fff : 0xfff);
	vifX.tag.size = vifNum * 2;
}

_vifT void vifCMD_MSCALF()
{
	if (idx) vif1FLUSH(); // Only Vif1 Flush!?
	vuExecMicro(idx, (u16)(vifXRegs->code) << 3);
	vifX.cmd &= ~0x7f;
}

_vifT void vifCMD_MSCNT()
{
	vuExecMicro(idx, -1);
	vifX.cmd &= ~0x7f;
}

_vifT void vifCMD_MskPath3()
{
	vif1Only();
	if (vif1ch->chcr.STR) {
		schedulepath3msk = 0x10 | ((vif1Regs->code >> 15) & 0x1);
		vif1.vifstalled = true;
	}
	else {
		schedulepath3msk = (vif1Regs->code >> 15) & 0x1;
		Vif1MskPath3();
	}
	vif1.cmd &= ~0x7f;
}

_vifT void vifCMD_Nop()
{
	vifX.cmd &= ~0x7f;
}

_vifT void vifCMD_Null() // invalid opcode
{
	// if ME1, then force the vif to interrupt
	if (!(vifXRegs->err.ME1)) //Ignore vifcode and tag mismatch error
	{
		Console.WriteLn("UNKNOWN VifCmd: %x", vifX.cmd);
		vifXRegs->stat.ER1 = true;
		vifX.irq++;
	}
	if (!idx) vifX.cmd &= ~0x7f; // FixMe: vif0/vif1 should do the same thing!?
	else	  vifX.cmd  = 0;
}

_vifT void vifCMD_Offset()
{
	vif1Only();
	vif1Regs->ofst  = vif1Regs->code & 0x3ff;
	vif1Regs->stat.DBF = false;
	vif1Regs->tops  = vif1Regs->base;
	vif1.cmd &= ~0x7f;
}

_vifT void vifCMD_STCycl()
{
	vifXRegs->cycle.cl = (u8)(vifXRegs->code);
	vifXRegs->cycle.wl = (u8)(vifXRegs->code >> 8);
	vifX.cmd &= ~0x7f;
}

_vifT void vifCMD_STMask()
{
	vifX.tag.size = 1;
}

_vifT void vifCMD_STMod()
{
	vifXRegs->mode = vifXRegs->code & 0x3;
	vifX.cmd &= ~0x7f;
}

_vifT void vifCMD_STRowCol() // STROW / STCOL
{
	vifX.tag.addr = 0;
	vifX.tag.size = 4;
}

//------------------------------------------------------------------
// Vif0/Vif1 CMD Tables
//------------------------------------------------------------------

void (*Vif0CMDTLB[82])() = {
	vifCMD_Nop<0>	  , vifCMD_STCycl<0>  , vifCMD_Offset<0>, vifCMD_Base<0>  , vifCMD_ITop<0>  , vifCMD_STMod<0> , vifCMD_MskPath3<0>, vifCMD_Mark<0> , /*0x00*/
	vifCMD_Null<0>	  , vifCMD_Null<0>    , vifCMD_Null<0>	, vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>    , vifCMD_Null<0> , /*0x08*/
	vifCMD_FlushE<0>  , vifCMD_Flush<0>   , vifCMD_Null<0>	, vifCMD_FlushA<0>, vifCMD_MSCALF<0>, vifCMD_MSCALF<0>, vifCMD_Null<0>	  , vifCMD_MSCNT<0>, /*0x10*/
	vifCMD_Null<0>    , vifCMD_Null<0>    , vifCMD_Null<0>	, vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>    , vifCMD_Null<0> , /*0x18*/
	vifCMD_STMask<0>  , vifCMD_Null<0>    , vifCMD_Null<0>	, vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>	  , vifCMD_Null<0> , /*0x20*/
	vifCMD_Null<0>    , vifCMD_Null<0>    , vifCMD_Null<0>	, vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>	  , vifCMD_Null<0> , /*0x28*/
	vifCMD_STRowCol<0>, vifCMD_STRowCol<0>, vifCMD_Null<0>	, vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>	  , vifCMD_Null<0> , /*0x30*/
	vifCMD_Null<0>    , vifCMD_Null<0>    , vifCMD_Null<0>	, vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>    , vifCMD_Null<0> , /*0x38*/
	vifCMD_Null<0>    , vifCMD_Null<0>    , vifCMD_Null<0>	, vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>    , vifCMD_Null<0> , /*0x40*/
	vifCMD_Null<0>    , vifCMD_Null<0>    , vifCMD_MPG<0>	, vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>  , vifCMD_Null<0>    , vifCMD_Null<0> , /*0x48*/
	vifCMD_DirectHL<0>, vifCMD_DirectHL<0>
};

void (*Vif1CMDTLB[82])() = {
	vifCMD_Nop<1>	  , vifCMD_STCycl<1>  , vifCMD_Offset<1>, vifCMD_Base<1>  , vifCMD_ITop<1>  , vifCMD_STMod<1> , vifCMD_MskPath3<1>, vifCMD_Mark<1> , /*0x00*/
	vifCMD_Null<1>	  , vifCMD_Null<1>    , vifCMD_Null<1>	, vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>    , vifCMD_Null<1> , /*0x08*/
	vifCMD_FlushE<1>  , vifCMD_Flush<1>   , vifCMD_Null<1>	, vifCMD_FlushA<1>, vifCMD_MSCALF<1>, vifCMD_MSCALF<1>, vifCMD_Null<1>	  , vifCMD_MSCNT<1>, /*0x10*/
	vifCMD_Null<1>    , vifCMD_Null<1>    , vifCMD_Null<1>	, vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>    , vifCMD_Null<1> , /*0x18*/
	vifCMD_STMask<1>  , vifCMD_Null<1>    , vifCMD_Null<1>	, vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>	  , vifCMD_Null<1> , /*0x20*/
	vifCMD_Null<1>    , vifCMD_Null<1>    , vifCMD_Null<1>	, vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>	  , vifCMD_Null<1> , /*0x28*/
	vifCMD_STRowCol<1>, vifCMD_STRowCol<1>, vifCMD_Null<1>	, vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>	  , vifCMD_Null<1> , /*0x30*/
	vifCMD_Null<1>    , vifCMD_Null<1>    , vifCMD_Null<1>	, vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>    , vifCMD_Null<1> , /*0x38*/
	vifCMD_Null<1>    , vifCMD_Null<1>    , vifCMD_Null<1>	, vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>    , vifCMD_Null<1> , /*0x40*/
	vifCMD_Null<1>    , vifCMD_Null<1>    , vifCMD_MPG<1>	, vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>  , vifCMD_Null<1>    , vifCMD_Null<1> , /*0x48*/
	vifCMD_DirectHL<1>, vifCMD_DirectHL<1>
};
