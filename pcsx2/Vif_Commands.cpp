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

#define  vif1Only()	 { if (!idx) { vifCMD_Null<idx>(); return;	    } }
#define  vif1Only_() { if (!idx) { return vifTrans_Null<idx>(NULL); } }

_vifT void vifCMD_Null();

//------------------------------------------------------------------
// Vif0/Vif1 Misc Functions
//------------------------------------------------------------------

static _f void vifFlush(int idx) {
	if (!idx) vif0FLUSH();
	else	  vif1FLUSH();
}

static _f void vuExecMicro(int idx, u32 addr) {
	VURegs* VU = nVif[idx].VU;
	vifFlush(idx);

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
		CPU_INT(DMAC_GIF, 4);
	}

	schedulepath3msk = 0;
}

//------------------------------------------------------------------
// Vif0/Vif1 Data Transfer Commands
//------------------------------------------------------------------

_vifT int __fastcall vifTrans_Null(u32 *data)
{
	Console.WriteLn("VIF%d Shouldn't go here CMD = %x", idx, vifXRegs->code);
	vifX.cmd = 0;
	return 0;
}

_vifT int __fastcall vifTrans_STMask(u32 *data)
{
	vifXRegs->mask = data[0];
	VIF_LOG("STMASK == %x", vifXRegs->mask);

	vifX.tag.size = 0;
	vifX.cmd = 0;
	return 1;
}

_vifT int __fastcall vifTrans_STRow(u32 *data)
{
	int ret;
	u32* rows  = idx ? g_vifmask.Row1 : g_vifmask.Row0;
	u32* pmem  = &vifXRegs->r0 + (vifX.tag.addr << 2);
	u32* pmem2 = rows		   +  vifX.tag.addr;

	ret = min(4 - vifX.tag.addr,  vifX.vifpacketsize);
	pxAssume(vifX.tag.addr < 4);
	pxAssume(ret > 0);

	switch (ret) {
		case 4:
			pmem[12] = data[3];
			pmem2[3] = data[3];
		case 3:
			pmem[8]  = data[2];
			pmem2[2] = data[2];
		case 2:
			pmem[4]  = data[1];
			pmem2[1] = data[1];
		case 1:
			pmem[0]  = data[0];
			pmem2[0] = data[0];
			break;
		jNO_DEFAULT
	}

	vifX.tag.addr += ret;
	vifX.tag.size -= ret;
	if (!vifX.tag.size) vifX.cmd = 0;

	return ret;
}

_vifT int __fastcall vifTrans_STCol(u32 *data)
{
	int ret;
	u32* cols  = idx ? g_vifmask.Col1 : g_vifmask.Col0;
	u32* pmem  = &vifXRegs->c0 + (vifX.tag.addr << 2);
	u32* pmem2 = cols		   +  vifX.tag.addr;
	ret = min(4 - vifX.tag.addr,  vifX.vifpacketsize);

	switch (ret) {
		case 4:
			pmem[12] = data[3];
			pmem2[3] = data[3];
		case 3:
			pmem[8]  = data[2];
			pmem2[2] = data[2];
		case 2:
			pmem[4]  = data[1];
			pmem2[1] = data[1];
		case 1:
			pmem[0]  = data[0];
			pmem2[0] = data[0];
			break;
		jNO_DEFAULT
	}

	vifX.tag.addr += ret;
	vifX.tag.size -= ret;
	if (!vifX.tag.size) vifX.cmd = 0;
	return ret;
}

_f void _vifTrans_MPG(int idx, u32 addr, u32 *data, int size) 
{
	VURegs& VUx = idx ? VU1 : VU0;
	pxAssume(VUx.Micro > 0);

	if (memcmp(VUx.Micro + addr, data, size << 2)) {
		if (!idx)  CpuVU0->Clear(addr, size << 2); // Clear before writing!
		else	   CpuVU1->Clear(addr, size << 2); // Clear before writing!
		memcpy_fast(VUx.Micro + addr, data, size << 2);
	}
}

_vifT int __fastcall vifTrans_MPG(u32 *data)
{
	if (vifX.vifpacketsize < vifX.tag.size) {
		if((vifX.tag.addr +  vifX.vifpacketsize) > (idx ? 0x4000 : 0x1000)) {
			DevCon.Warning("Vif%d MPG Split Overflow", idx);
		}
		_vifTrans_MPG(idx,   vifX.tag.addr, data, vifX.vifpacketsize);
		vifX.tag.addr   +=   vifX.vifpacketsize << 2;
		vifX.tag.size   -=   vifX.vifpacketsize;
		return vifX.vifpacketsize;
	}
	else {
		int ret;
		if((vifX.tag.addr + vifX.tag.size) > (idx ? 0x4000 : 0x1000)) {
			DevCon.Warning("Vif%d MPG Split Overflow", idx);
		}
		_vifTrans_MPG(idx,  vifX.tag.addr, data, vifX.tag.size);
		ret = vifX.tag.size;
		vifX.tag.size = 0;
		vifX.cmd = 0;
		return ret;
	}
}

_vifT int __fastcall vifTrans_Unpack(u32 *data)
{
	return nVifUnpack(idx, (u8*)data);
}

// Dummy GIF-TAG Packet to Guarantee Count = 1
extern __aligned16 u32 nloop0_packet[4];
static __aligned16 u32 splittransfer[4];
static u32 splitptr = 0;

_vifT int __fastcall vifTrans_DirectHL(u32 *data)
{
	vif1Only_();
	int ret = 0;

	if ((vif1.cmd & 0x7f) == 0x51) {
		if (gif->chcr.STR && (!vif1Regs->mskpath3 && (Path3progress == IMAGE_MODE))) {
			vif1Regs->stat.VGW = true; // PATH3 is in image mode, so wait for end of transfer
			return 0;
		}
	}

	gifRegs->stat.APATH |= GIF_APATH2;
	gifRegs->stat.OPH = true;
	
	if (splitptr > 0) { // Leftover data from the last packet, filling the rest and sending to the GS

		if ((splitptr < 4) && (vif1.vifpacketsize >= (4 - splitptr))) {
			while (splitptr < 4) {
				splittransfer[splitptr++] = (u32)data++;
				ret++;
				vif1.tag.size--;
			}
		}

        Registers::Freeze();
		// copy 16 bytes the fast way:
		const u64* src = (u64*)splittransfer[0];
		GetMTGS().PrepDataPacket(GIF_PATH_2, nloop0_packet, 1);
		u64* dst = (u64*)GetMTGS().GetDataPacketPtr();
		dst[0] = src[0];
		dst[1] = src[1];

		GetMTGS().SendDataPacket();
        Registers::Thaw();

		if (vif1.tag.size == 0) vif1.cmd = 0;
		splitptr = 0;
		return ret;
	}

	if (vif1.vifpacketsize < vif1.tag.size) {
		if (vif1.vifpacketsize < 4 && splitptr != 4) {
			ret = vif1.vifpacketsize;
			while (ret > 0) { // Not a full QW left in the buffer, saving left over data
				splittransfer[splitptr++] = (u32)data++;
				vif1.tag.size--;
				ret--;
			}
			return vif1.vifpacketsize;
		}
		vif1.tag.size -= vif1.vifpacketsize;
		ret = vif1.vifpacketsize;
	}
	else {
		gifRegs->stat.clear_flags(GIF_STAT_APATH2 | GIF_STAT_OPH);
		ret = vif1.tag.size;
		vif1.tag.size = 0;
		vif1.cmd = 0;
	}

	// ToDo: ret is guaranteed to be qword aligned ?
	Registers::Freeze();

	// Round ret up, just in case it's not 128bit aligned.
	const uint count = GetMTGS().PrepDataPacket(GIF_PATH_2, data, (ret + 3) >> 2);
	memcpy_fast(GetMTGS().GetDataPacketPtr(), data, count << 4);
	GetMTGS().SendDataPacket();

	Registers::Thaw();
	return ret;
}

//------------------------------------------------------------------
// Vif0/Vif1 Commands (VifCodes)
//------------------------------------------------------------------

_vifT void vifCMD_Base()
{
	vif1Only();
	vif1Regs->base = vif1Regs->code & 0x3ff;
	vif1.cmd &= ~0x7f;
}

_vifT void vifCMD_DirectHL()
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
		CPU_INT(DMAC_GIF, 4);
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
	vifFlush(idx);
	int vifNum = (u8)(vifXRegs->code >> 16);
	if(!vifNum) vifNum = 256;

	vifX.tag.addr = (u16)((vifXRegs->code) << 3) & (idx ? 0x3fff : 0xfff);
	vifX.tag.size = vifNum * 2;
}

_vifT void vifCMD_MSCALF()
{
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
		Console.WriteLn("Vif%d: Unknown VifCmd! [%x]", idx, vifX.cmd);
		vifXRegs->stat.ER1 = true;
		vifX.irq++;
	}
	if (!idx) vifX.cmd &= ~0x7f; // FixMe: vif0/vif1 should do the same thing!?
	else	  vifX.cmd  = 0;
}

_vifT void vifCMD_Offset()
{
	vif1Only();
	vif1Regs->stat.DBF	= false;
	vif1Regs->ofst		= vif1Regs->code & 0x3ff;
	vif1Regs->tops		= vif1Regs->base;
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
// Vif0/Vif1 Data Transfer Tables
//------------------------------------------------------------------

int (__fastcall *Vif0TransTLB[128])(u32 *data) = {
	vifTrans_Null<0>	, vifTrans_Null<0>    , vifTrans_Null<0>	, vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>,   /*0x00*/
	vifTrans_Null<0>	, vifTrans_Null<0>    , vifTrans_Null<0>	, vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>,   /*0x08*/
	vifTrans_Null<0>	, vifTrans_Null<0>    , vifTrans_Null<0>	, vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>	, vifTrans_Null<0>,   /*0x10*/
	vifTrans_Null<0>	, vifTrans_Null<0>    , vifTrans_Null<0>	, vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>,   /*0x18*/
	vifTrans_STMask<0>  , vifTrans_Null<0>    , vifTrans_Null<0>	, vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>	, vifTrans_Null<0>,   /*0x20*/
	vifTrans_Null<0>    , vifTrans_Null<0>    , vifTrans_Null<0>	, vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>	, vifTrans_Null<0>,   /*0x28*/
	vifTrans_STRow<0>	, vifTrans_STCol<0>	  , vifTrans_Null<0>	, vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>	, vifTrans_Null<0>,   /*0x30*/
	vifTrans_Null<0>    , vifTrans_Null<0>    , vifTrans_Null<0>	, vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>,   /*0x38*/
	vifTrans_Null<0>    , vifTrans_Null<0>    , vifTrans_Null<0>	, vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>,   /*0x40*/
	vifTrans_Null<0>    , vifTrans_Null<0>    , vifTrans_MPG<0>		, vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>,   /*0x48*/
	vifTrans_DirectHL<0>, vifTrans_DirectHL<0>, vifTrans_Null<0>	, vifTrans_Null<0>   , vifTrans_Null<0>	  , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>,   /*0x50*/
	vifTrans_Null<0>	, vifTrans_Null<0>	  , vifTrans_Null<0>	, vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>   , vifTrans_Null<0>,   /*0x58*/
	vifTrans_Unpack<0>  , vifTrans_Unpack<0>  , vifTrans_Unpack<0>	, vifTrans_Unpack<0> , vifTrans_Unpack<0> , vifTrans_Unpack<0> , vifTrans_Unpack<0> , vifTrans_Null<0>,   /*0x60*/
	vifTrans_Unpack<0>  , vifTrans_Unpack<0>  , vifTrans_Unpack<0>	, vifTrans_Unpack<0> , vifTrans_Unpack<0> , vifTrans_Unpack<0> , vifTrans_Unpack<0> , vifTrans_Unpack<0>, /*0x68*/
	vifTrans_Unpack<0>  , vifTrans_Unpack<0>  , vifTrans_Unpack<0>	, vifTrans_Unpack<0> , vifTrans_Unpack<0> , vifTrans_Unpack<0> , vifTrans_Unpack<0> , vifTrans_Null<0>,   /*0x70*/
	vifTrans_Unpack<0>  , vifTrans_Unpack<0>  , vifTrans_Unpack<0>	, vifTrans_Null<0>   , vifTrans_Unpack<0> , vifTrans_Unpack<0> , vifTrans_Unpack<0> , vifTrans_Unpack<0>  /*0x78*/
};

int (__fastcall *Vif1TransTLB[128])(u32 *data) = {
	vifTrans_Null<1>	, vifTrans_Null<1>    , vifTrans_Null<1>	, vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>,   /*0x00*/
	vifTrans_Null<1>	, vifTrans_Null<1>    , vifTrans_Null<1>	, vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>,   /*0x08*/
	vifTrans_Null<1>	, vifTrans_Null<1>    , vifTrans_Null<1>	, vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>	, vifTrans_Null<1>,   /*0x10*/
	vifTrans_Null<1>	, vifTrans_Null<1>    , vifTrans_Null<1>	, vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>,   /*0x18*/
	vifTrans_STMask<1>  , vifTrans_Null<1>    , vifTrans_Null<1>	, vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>	, vifTrans_Null<1>,   /*0x20*/
	vifTrans_Null<1>    , vifTrans_Null<1>    , vifTrans_Null<1>	, vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>	, vifTrans_Null<1>,   /*0x28*/
	vifTrans_STRow<1>	, vifTrans_STCol<1>	  , vifTrans_Null<1>	, vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>	, vifTrans_Null<1>,   /*0x30*/
	vifTrans_Null<1>    , vifTrans_Null<1>    , vifTrans_Null<1>	, vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>,   /*0x38*/
	vifTrans_Null<1>    , vifTrans_Null<1>    , vifTrans_Null<1>	, vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>,   /*0x40*/
	vifTrans_Null<1>    , vifTrans_Null<1>    , vifTrans_MPG<1>		, vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>,   /*0x48*/
	vifTrans_DirectHL<1>, vifTrans_DirectHL<1>, vifTrans_Null<1>	, vifTrans_Null<1>   , vifTrans_Null<1>	  , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>,   /*0x50*/
	vifTrans_Null<1>	, vifTrans_Null<1>	  , vifTrans_Null<1>	, vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>   , vifTrans_Null<1>,   /*0x58*/
	vifTrans_Unpack<1>  , vifTrans_Unpack<1>  , vifTrans_Unpack<1>	, vifTrans_Unpack<1> , vifTrans_Unpack<1> , vifTrans_Unpack<1> , vifTrans_Unpack<1> , vifTrans_Null<1>,   /*0x60*/
	vifTrans_Unpack<1>  , vifTrans_Unpack<1>  , vifTrans_Unpack<1>	, vifTrans_Unpack<1> , vifTrans_Unpack<1> , vifTrans_Unpack<1> , vifTrans_Unpack<1> , vifTrans_Unpack<1>, /*0x68*/
	vifTrans_Unpack<1>  , vifTrans_Unpack<1>  , vifTrans_Unpack<1>	, vifTrans_Unpack<1> , vifTrans_Unpack<1> , vifTrans_Unpack<1> , vifTrans_Unpack<1> , vifTrans_Null<1>,   /*0x70*/
	vifTrans_Unpack<1>  , vifTrans_Unpack<1>  , vifTrans_Unpack<1>	, vifTrans_Null<1>   , vifTrans_Unpack<1> , vifTrans_Unpack<1> , vifTrans_Unpack<1> , vifTrans_Unpack<1>  /*0x78*/
};

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
