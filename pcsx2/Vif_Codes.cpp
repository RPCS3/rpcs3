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
#include "GS.h"
#include "Gif.h"
#include "Vif_Dma.h"
#include "newVif.h"
#include "VUmicro.h"

#define vifOp(vifCodeName) _vifT int __fastcall vifCodeName(int pass, u32 *data)
#define pass1 if (pass == 0)
#define pass2 if (pass == 1)
#define pass3 if (pass == 2)
#define vif1Only() { if (!idx) return vifCode_Null<idx>(pass, (u32*)data); }
vifOp(vifCode_Null);

//------------------------------------------------------------------
// Vif0/Vif1 Misc Functions
//------------------------------------------------------------------

static _f void vifFlush(int idx) {
	if (!idx) vif0FLUSH();
	else	  vif1FLUSH();
}

static _f void vuExecMicro(int idx, u32 addr) {
	VURegs* VU = nVif[idx].VU;
	int startcycles = 0;
	//vifFlush(idx);

	//if(vifX.vifstalled == true) return;

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

	if(!idx)startcycles = VU0.cycle;
	else    startcycles = VU1.cycle;

	if (!idx) vu0ExecMicro(addr);
	else	  vu1ExecMicro(addr);

	if(!idx) { g_vu0Cycles += (VU0.cycle-startcycles) * BIAS; g_packetsizeonvu = vif0.vifpacketsize; }
	else     { g_vu1Cycles += (VU1.cycle-startcycles) * BIAS; g_packetsizeonvu = vif1.vifpacketsize; }
	//DevCon.Warning("Ran VU%x, VU0 Cycles %x, VU1 Cycles %x", idx, g_vu0Cycles, g_vu1Cycles);
	vifX.vifstalled = true;
}

u8 schedulepath3msk = 0;

void Vif1MskPath3() {

	vif1Regs->mskpath3 = schedulepath3msk & 0x1;
	GIF_LOG("VIF MSKPATH3 %x gif str %x path3 status %x", vif1Regs->mskpath3, gif->chcr.STR, GSTransferStatus.PTH3);
	gifRegs->stat.M3P = vif1Regs->mskpath3;

	if (!vif1Regs->mskpath3)
	{
		//if(GSTransferStatus.PTH3 > TRANSFER_MODE && gif->chcr.STR) GSTransferStatus.PTH3 = TRANSFER_MODE;
		//DevCon.Warning("Mask off");
		//if(GSTransferStatus.PTH3 >= PENDINGSTOP_MODE) GSTransferStatus.PTH3 = IDLE_MODE;
		if(gifRegs->stat.P3Q) 
		{
			gsInterrupt();//gsInterrupt();
		}
	
	}// else if(!gif->chcr.STR && GSTransferStatus.PTH3 == IDLE_MODE) GSTransferStatus.PTH3 = STOPPED_MODE;//else DevCon.Warning("Mask on");

	schedulepath3msk = 0;
}

//------------------------------------------------------------------
// Vif0/Vif1 Code Implementations
//------------------------------------------------------------------

vifOp(vifCode_Base) {
	vif1Only();
	pass1 { vif1Regs->base = vif1Regs->code & 0x3ff; vif1.cmd = 0; }
	pass3 { DevCon.WriteLn("vifCode_Base"); }
	return 0;
}

extern bool SIGNAL_IMR_Pending;

template<int idx> _f int _vifCode_Direct(int pass, u8* data, bool isDirectHL) {
	pass1 {
		vif1Only();
		int vifImm    = (u16)vif1Regs->code;
		vif1.tag.size = vifImm ? (vifImm*4) : (65536*4);
		vif1.vifstalled    = true;
		gifRegs->stat.P2Q = true;
		if (gifRegs->stat.PSE)  // temporarily stop
		{
			Console.WriteLn("Gif dma temp paused? VIF DIRECT");
			vif1.GifWaitState = 3;
			vif1Regs->stat.VGW = true;
		}
		//Should cause this to split here to try and time PATH3 right.		
		return 0;
	}
	pass2 {
		vif1Only();

		if (GSTransferStatus.PTH3 < IDLE_MODE || gifRegs->stat.P1Q == true)
		{
			if(gifRegs->stat.APATH == GIF_APATH2 || ((GSTransferStatus.PTH3 <= IMAGE_MODE && gifRegs->stat.IMT && (vif1.cmd & 0x7f) == 0x50)) && gifRegs->stat.P1Q == false)
			{
				//Do nothing, allow it
				vif1Regs->stat.VGW = false;
				//if(gifRegs->stat.APATH != GIF_APATH2)DevCon.Warning("Continue DIRECT/HL %x P3 %x APATH %x P1Q %x", vif1.cmd, GSTransferStatus.PTH3, gifRegs->stat.APATH, gifRegs->stat.P1Q);
			}
			else
			{
				//DevCon.Warning("Stall DIRECT/HL %x P3 %x APATH %x P1Q %x", vif1.cmd, GSTransferStatus.PTH3, gifRegs->stat.APATH, gifRegs->stat.P1Q);
				vif1Regs->stat.VGW = true; // PATH3 is in image mode (DIRECTHL), or busy (BOTH no IMT)
				vif1.GifWaitState = 0;
				vif1.vifstalled    = true;
				return 0;
			}
		}
		if(SIGNAL_IMR_Pending == true)
		{
			DevCon.Warning("Path 2 Paused (At start)");
			vif1.vifstalled    = true;
			return 0;
		}
		if (gifRegs->stat.PSE)  // temporarily stop
		{
			Console.WriteLn("Gif dma temp paused? VIF DIRECT");
			vif1.GifWaitState = 3;
			vif1.vifstalled    = true;
			vif1Regs->stat.VGW = true;
			return 0;
		}

		// HACK ATTACK!
		// we shouldn't be clearing the queue flag here at all.  Ideally, the queue statuses
		// should be checked, handled, and cleared from the EOP check in GIFPath only. --air
		gifRegs->stat.clear_flags(GIF_STAT_P2Q);

		// the tag size should ALWAYS be 128 bits (qwc).  If it isn't, it means there's a serious bug
		// somewhere in the VIF (likely relating to +/-'ing the tag.size during processing).
		pxAssumeMsg( (vif1.tag.size & 3) == 0, "Invalid Vif1 DIRECT packet size detected!" );

		const int	minSize	 = aMin(vif1.vifpacketsize, vif1.tag.size)/4;
		uint ret;

		if (!minSize)
			DevCon.Warning("VIF DIRECT (PATH2): No Data Transfer?");

		GetMTGS().PrepDataPacket(GIF_PATH_2, minSize);
		ret = GIFPath_CopyTag(GIF_PATH_2, (u128*)data, minSize)*4;
		GetMTGS().SendDataPacket();

		vif1.tag.size -= ret;

		if(vif1.tag.size == 0) 
		{
			vif1.cmd = 0;
			gifRegs->stat.clear_flags(GIF_STAT_APATH2 | GIF_STAT_OPH);
		}
		vif1.vifstalled    = true;
		return ret;
	}
	return 0;
}

vifOp(vifCode_Direct) {
	pass3 { DevCon.WriteLn("vifCode_Direct"); }
	return _vifCode_Direct<idx>(pass, (u8*)data, 0);
}

vifOp(vifCode_DirectHL) {
	pass3 { DevCon.WriteLn("vifCode_DirectHL"); }
	return _vifCode_Direct<idx>(pass, (u8*)data, 1);
}

// ToDo: FixMe
vifOp(vifCode_Flush) {
	vif1Only();
	pass1 { vifFlush(idx);  vifX.cmd = 0; }
	pass3 { DevCon.WriteLn("vifCode_Flush"); }
	return 0;
}

// ToDo: FixMe
vifOp(vifCode_FlushA) {
	vif1Only();
	pass1 {
		vifFlush(idx);
		// Gif is already transferring so wait for it.
		if (gifRegs->stat.P1Q == true || GSTransferStatus.PTH3 <= PENDINGSTOP_MODE) {
			//DevCon.Warning("VIF FlushA Wait MSK = %x", vif1Regs->mskpath3);
			//
			
			//DevCon.WriteLn("FlushA path3 Wait! PTH3 MD %x STR %x", GSTransferStatus.PTH3, gif->chcr.STR);
			vif1Regs->stat.VGW = true;
			vif1.GifWaitState = 1;
			vifX.vifstalled    = true;
		}	// else DevCon.WriteLn("FlushA path3 no Wait! PTH3 MD %x STR %x", GSTransferStatus.PTH3, gif->chcr.STR);	
		
		
	}
	pass3 { DevCon.WriteLn("vifCode_FlushA"); }
	vifX.cmd = 0;
	return 0;
}

// ToDo: FixMe
vifOp(vifCode_FlushE) {
	pass1 { vifFlush(idx); vifX.cmd = 0; }
	pass3 { DevCon.WriteLn("vifCode_FlushE"); }
	return 0;
}

vifOp(vifCode_ITop) {
	pass1 { vifXRegs->itops = vifXRegs->code & 0x3ff; vifX.cmd = 0; }
	pass3 { DevCon.WriteLn("vifCode_ITop"); }
	return 0;
}

vifOp(vifCode_Mark) {
	pass1 {
		vifXRegs->mark     = (u16)vifXRegs->code;
		vifXRegs->stat.MRK = true;
		vifX.cmd           = 0;
	}
	pass3 { DevCon.WriteLn("vifCode_Mark"); }
	return 0;
}

_f void _vifCode_MPG(int idx, u32 addr, u32 *data, int size) {
	VURegs& VUx = idx ? VU1 : VU0;
	pxAssume(VUx.Micro > 0);

	if (memcmp(VUx.Micro + addr, data, size << 2)) {
		if (!idx)  CpuVU0->Clear(addr, size << 2); // Clear before writing!
		else	   CpuVU1->Clear(addr, size << 2); // Clear before writing!
		memcpy_fast(VUx.Micro + addr, data, size << 2);
	}
}

vifOp(vifCode_MPG) {
	pass1 {
		int    vifNum =  (u8)(vifXRegs->code >> 16);
		vifX.tag.addr = (u16)(vifXRegs->code <<  3) & (idx ? 0x3fff : 0xfff);
		vifX.tag.size = vifNum ? (vifNum*2) : 512;
		//vifFlush(idx);
		return 1;
	}
	pass2 {
		if (vifX.vifpacketsize < vifX.tag.size) { // Partial Transfer
			if((vifX.tag.addr +  vifX.vifpacketsize) > (idx ? 0x4000 : 0x1000)) {
				DevCon.Warning("Vif%d MPG Split Overflow", idx);
			}
			_vifCode_MPG(idx,    vifX.tag.addr, data, vifX.vifpacketsize);
			vifX.tag.addr   +=   vifX.vifpacketsize << 2;
			vifX.tag.size   -=   vifX.vifpacketsize;
			return vifX.vifpacketsize;
		}
		else { // Full Transfer
			if((vifX.tag.addr + vifX.tag.size) > (idx ? 0x4000 : 0x1000)) {
				DevCon.Warning("Vif%d MPG Split Overflow", idx);
			}
			_vifCode_MPG(idx,  vifX.tag.addr, data, vifX.tag.size);
			int ret       = vifX.tag.size;
			vifX.tag.size = 0;
			vifX.cmd      = 0;
			return ret;
		}
	}
	pass3 { DevCon.WriteLn("vifCode_MPG"); }
	return 0;
}

vifOp(vifCode_MSCAL) {
	pass1 {  vifFlush(idx); vuExecMicro(idx, (u16)(vifXRegs->code) << 3); vifX.cmd = 0;}
	pass3 { DevCon.WriteLn("vifCode_MSCAL"); }
	return 0;
}

vifOp(vifCode_MSCALF) {
	pass1 {  vifFlush(idx); vuExecMicro(idx, (u16)(vifXRegs->code) << 3); vifX.cmd = 0; }
	pass3 { DevCon.WriteLn("vifCode_MSCALF"); }
	return 0;
}

vifOp(vifCode_MSCNT) {
	pass1 {  vifFlush(idx); vuExecMicro(idx, -1); vifX.cmd = 0; }
	pass3 { DevCon.WriteLn("vifCode_MSCNT"); }
	return 0;
}

// ToDo: FixMe
vifOp(vifCode_MskPath3) {
	vif1Only();
	pass1 {
		if (vif1ch->chcr.STR && vifX.lastcmd != 0x13) {
			schedulepath3msk = 0x10 | ((vif1Regs->code >> 15) & 0x1);
			vif1.vifstalled = true;
		}
		else {
			schedulepath3msk = (vif1Regs->code >> 15) & 0x1;
			Vif1MskPath3();
		}
		vifX.cmd = 0;
	}
	pass3 { DevCon.WriteLn("vifCode_MskPath3"); }
	return 0;
}

vifOp(vifCode_Nop) {
	pass1 { vifX.cmd = 0; }
	pass3 { DevCon.WriteLn("vifCode_Nop"); }
	return 0;
}

// ToDo: Review Flags
vifOp(vifCode_Null) {
	pass1 {
		// if ME1, then force the vif to interrupt
		if (!(vifXRegs->err.ME1)) { // Ignore vifcode and tag mismatch error
			Console.WriteLn("Vif%d: Unknown VifCmd! [%x]", idx, vifX.cmd);
			vifXRegs->stat.ER1 = true;
			vifX.vifstalled    = true;
			//vifX.irq++;
		}
		vifX.cmd = 0;
	}
	pass2 { Console.Error("Vif%d bad vifcode! [CMD = %x]", idx, vifX.cmd); }
	pass3 { DevCon.WriteLn("vifCode_Null"); }
	return 0;
}

vifOp(vifCode_Offset) {
	vif1Only();
	pass1 {
		vif1Regs->stat.DBF	= false;
		vif1Regs->ofst		= vif1Regs->code & 0x3ff;
		vif1Regs->tops		= vif1Regs->base;
		vifX.cmd			= 0;
	}
	pass3 { DevCon.WriteLn("vifCode_Offset"); }
	return 0;
}

template<int idx> _f int _vifCode_STColRow(u32* data, u32* pmem1, u32* pmem2) {
	int ret;
	ret = min(4 - vifX.tag.addr, vifX.vifpacketsize);
	pxAssume(vifX.tag.addr < 4);
	pxAssume(ret > 0);

	switch (ret) {
		case 4:
			pmem1[12] = data[3];
			pmem2[3]  = data[3];
		case 3:
			pmem1[8]  = data[2];
			pmem2[2]  = data[2];
		case 2:
			pmem1[4]  = data[1];
			pmem2[1]  = data[1];
		case 1:
			pmem1[0]  = data[0];
			pmem2[0]  = data[0];
			break;
		jNO_DEFAULT
	}

	vifX.tag.addr += ret;
	vifX.tag.size -= ret;
	if (!vifX.tag.size) vifX.cmd = 0;

	return ret;
}

vifOp(vifCode_STCol) {
	pass1 {
		vifX.tag.addr = 0;
		vifX.tag.size = 4;
		return 1;
	}
	pass2 {
		u32* cols  = idx ? g_vifmask.Col1 : g_vifmask.Col0;
		u32* pmem1 = &vifXRegs->c0 + (vifX.tag.addr << 2);
		u32* pmem2 = cols		   +  vifX.tag.addr;
		return _vifCode_STColRow<idx>(data, pmem1, pmem2);
	}
	pass3 { DevCon.WriteLn("vifCode_STCol"); }
	return 0;
}

vifOp(vifCode_STRow) {
	pass1 {
		vifX.tag.addr = 0;
		vifX.tag.size = 4;
		return 1;
	}
	pass2 {
		u32* rows  = idx ? g_vifmask.Row1 : g_vifmask.Row0;
		u32* pmem1 = &vifXRegs->r0 + (vifX.tag.addr << 2);
		u32* pmem2 = rows		   +  vifX.tag.addr;
		return _vifCode_STColRow<idx>(data, pmem1, pmem2);
	}
	pass3 { DevCon.WriteLn("vifCode_STRow"); }
	return 0;
}

vifOp(vifCode_STCycl) {
	pass1 {
		vifXRegs->cycle.cl = (u8)(vifXRegs->code);
		vifXRegs->cycle.wl = (u8)(vifXRegs->code >> 8);
		vifX.cmd		   = 0;
	}
	pass3 { DevCon.WriteLn("vifCode_STCycl"); }
	return 0;
}

vifOp(vifCode_STMask) {
	pass1 { vifX.tag.size = 1; }
	pass2 { vifXRegs->mask = data[0]; vifX.tag.size = 0; vifX.cmd = 0; }
	pass3 { DevCon.WriteLn("vifCode_STMask"); }
	return 1;
}

vifOp(vifCode_STMod) {
	pass1 { vifXRegs->mode = vifXRegs->code & 0x3; vifX.cmd = 0; }
	pass3 { DevCon.WriteLn("vifCode_STMod"); }
	return 0;
}

vifOp(vifCode_Unpack) {
	pass1 {
		if (!idx) vif0UnpackSetup(data);
		else	  vif1UnpackSetup(data);
		return 1;
	}
	pass2 { return nVifUnpack(idx, (u8*)data); }
	pass3 { DevCon.WriteLn("vifCode_Unpack");  }
	return 0;
}

//------------------------------------------------------------------
// Vif0/Vif1 Code Tables
//------------------------------------------------------------------

int (__fastcall *vif0Code[128])(int pass, u32 *data) = {
	vifCode_Nop<0>     , vifCode_STCycl<0>  , vifCode_Offset<0>	, vifCode_Base<0>   , vifCode_ITop<0>   , vifCode_STMod<0>  , vifCode_MskPath3<0>, vifCode_Mark<0>,   /*0x00*/
	vifCode_Null<0>    , vifCode_Null<0>    , vifCode_Null<0>	, vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>    , vifCode_Null<0>,   /*0x08*/
	vifCode_FlushE<0>  , vifCode_Flush<0>   , vifCode_Null<0>	, vifCode_FlushA<0> , vifCode_MSCAL<0>  , vifCode_MSCALF<0> , vifCode_Null<0>	 , vifCode_MSCNT<0>,  /*0x10*/
	vifCode_Null<0>    , vifCode_Null<0>    , vifCode_Null<0>	, vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>    , vifCode_Null<0>,   /*0x18*/
	vifCode_STMask<0>  , vifCode_Null<0>    , vifCode_Null<0>	, vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>	 , vifCode_Null<0>,   /*0x20*/
	vifCode_Null<0>    , vifCode_Null<0>    , vifCode_Null<0>	, vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>	 , vifCode_Null<0>,   /*0x28*/
	vifCode_STRow<0>   , vifCode_STCol<0>	, vifCode_Null<0>	, vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>	 , vifCode_Null<0>,   /*0x30*/
	vifCode_Null<0>    , vifCode_Null<0>    , vifCode_Null<0>	, vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>    , vifCode_Null<0>,   /*0x38*/
	vifCode_Null<0>    , vifCode_Null<0>    , vifCode_Null<0>	, vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>    , vifCode_Null<0>,   /*0x40*/
	vifCode_Null<0>    , vifCode_Null<0>    , vifCode_MPG<0>	, vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>    , vifCode_Null<0>,   /*0x48*/
	vifCode_Direct<0>  , vifCode_DirectHL<0>, vifCode_Null<0>	, vifCode_Null<0>   , vifCode_Null<0>	, vifCode_Null<0>   , vifCode_Null<0>    , vifCode_Null<0>,   /*0x50*/
	vifCode_Null<0>	   , vifCode_Null<0>	, vifCode_Null<0>	, vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>   , vifCode_Null<0>    , vifCode_Null<0>,   /*0x58*/
	vifCode_Unpack<0>  , vifCode_Unpack<0>  , vifCode_Unpack<0>	, vifCode_Unpack<0> , vifCode_Unpack<0> , vifCode_Unpack<0> , vifCode_Unpack<0>  , vifCode_Null<0>,   /*0x60*/
	vifCode_Unpack<0>  , vifCode_Unpack<0>  , vifCode_Unpack<0>	, vifCode_Unpack<0> , vifCode_Unpack<0> , vifCode_Unpack<0> , vifCode_Unpack<0>  , vifCode_Unpack<0>, /*0x68*/
	vifCode_Unpack<0>  , vifCode_Unpack<0>  , vifCode_Unpack<0>	, vifCode_Unpack<0> , vifCode_Unpack<0> , vifCode_Unpack<0> , vifCode_Unpack<0>  , vifCode_Null<0>,   /*0x70*/
	vifCode_Unpack<0>  , vifCode_Unpack<0>  , vifCode_Unpack<0>	, vifCode_Null<0>   , vifCode_Unpack<0> , vifCode_Unpack<0> , vifCode_Unpack<0>  , vifCode_Unpack<0>  /*0x78*/
};

int (__fastcall *vif1Code[128])(int pass, u32 *data) = {
	vifCode_Nop<1>     , vifCode_STCycl<1>  , vifCode_Offset<1>	, vifCode_Base<1>   , vifCode_ITop<1>   , vifCode_STMod<1>  , vifCode_MskPath3<1>, vifCode_Mark<1>,   /*0x00*/
	vifCode_Null<1>    , vifCode_Null<1>    , vifCode_Null<1>	, vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>    , vifCode_Null<1>,   /*0x08*/
	vifCode_FlushE<1>  , vifCode_Flush<1>   , vifCode_Null<1>	, vifCode_FlushA<1> , vifCode_MSCAL<1>  , vifCode_MSCALF<1> , vifCode_Null<1>	 , vifCode_MSCNT<1>,  /*0x10*/
	vifCode_Null<1>    , vifCode_Null<1>    , vifCode_Null<1>	, vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>    , vifCode_Null<1>,   /*0x18*/
	vifCode_STMask<1>  , vifCode_Null<1>    , vifCode_Null<1>	, vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>	 , vifCode_Null<1>,   /*0x20*/
	vifCode_Null<1>    , vifCode_Null<1>    , vifCode_Null<1>	, vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>	 , vifCode_Null<1>,   /*0x28*/
	vifCode_STRow<1>   , vifCode_STCol<1>	, vifCode_Null<1>	, vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>	 , vifCode_Null<1>,   /*0x30*/
	vifCode_Null<1>    , vifCode_Null<1>    , vifCode_Null<1>	, vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>    , vifCode_Null<1>,   /*0x38*/
	vifCode_Null<1>    , vifCode_Null<1>    , vifCode_Null<1>	, vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>    , vifCode_Null<1>,   /*0x40*/
	vifCode_Null<1>    , vifCode_Null<1>    , vifCode_MPG<1>	, vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>    , vifCode_Null<1>,   /*0x48*/
	vifCode_Direct<1>  , vifCode_DirectHL<1>, vifCode_Null<1>	, vifCode_Null<1>   , vifCode_Null<1>	, vifCode_Null<1>   , vifCode_Null<1>    , vifCode_Null<1>,   /*0x50*/
	vifCode_Null<1>	   , vifCode_Null<1>	, vifCode_Null<1>	, vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>   , vifCode_Null<1>    , vifCode_Null<1>,   /*0x58*/
	vifCode_Unpack<1>  , vifCode_Unpack<1>  , vifCode_Unpack<1>	, vifCode_Unpack<1> , vifCode_Unpack<1> , vifCode_Unpack<1> , vifCode_Unpack<1>  , vifCode_Null<1>,   /*0x60*/
	vifCode_Unpack<1>  , vifCode_Unpack<1>  , vifCode_Unpack<1>	, vifCode_Unpack<1> , vifCode_Unpack<1> , vifCode_Unpack<1> , vifCode_Unpack<1>  , vifCode_Unpack<1>, /*0x68*/
	vifCode_Unpack<1>  , vifCode_Unpack<1>  , vifCode_Unpack<1>	, vifCode_Unpack<1> , vifCode_Unpack<1> , vifCode_Unpack<1> , vifCode_Unpack<1>  , vifCode_Null<1>,   /*0x70*/
	vifCode_Unpack<1>  , vifCode_Unpack<1>  , vifCode_Unpack<1>	, vifCode_Null<1>   , vifCode_Unpack<1> , vifCode_Unpack<1> , vifCode_Unpack<1>  , vifCode_Unpack<1>  /*0x78*/
};
