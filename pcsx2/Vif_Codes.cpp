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
#include "Gif_Unit.h"
#include "Vif_Dma.h"
#include "newVif.h"
#include "VUmicro.h"
#include "MTVU.h"

#define vifOp(vifCodeName) _vifT int __fastcall vifCodeName(int pass, const u32 *data)
#define pass1    if (pass == 0)
#define pass2    if (pass == 1)
#define pass3    if (pass == 2)
#define pass1or2 if (pass == 0 || pass == 1)
#define vif1Only() { if (!idx) return vifCode_Null<idx>(pass, (u32*)data); }
vifOp(vifCode_Null);

//------------------------------------------------------------------
// Vif0/Vif1 Misc Functions
//------------------------------------------------------------------

static __fi void vifFlush(int idx) {
	if (!idx) vif0FLUSH();
	else      vif1FLUSH();
}

static __fi void vuExecMicro(int idx, u32 addr) {
	VIFregisters& vifRegs = vifXRegs;
	int startcycles = 0;
	//vifFlush(idx);

	//if(vifX.vifstalled == true) return;

	if (vifRegs.itops  > (idx ? 0x3ffu : 0xffu)) {
		Console.WriteLn("VIF%d ITOP overrun! %x", idx, vifRegs.itops);
		vifRegs.itops &= (idx ? 0x3ffu : 0xffu);
	}

	vifRegs.itop = vifRegs.itops;

	if (idx) {
		// in case we're handling a VIF1 execMicro, set the top with the tops value
		vifRegs.top = vifRegs.tops & 0x3ff;

		// is DBF flag set in VIF_STAT?
		if (vifRegs.stat.DBF) {
			// it is, so set tops with base, and clear the stat DBF flag
			vifRegs.tops = vifRegs.base;
			vifRegs.stat.DBF = false;
		}
		else {
			// it is not, so set tops with base + offset, and set stat DBF flag
			vifRegs.tops = vifRegs.base + vifRegs.ofst;
			vifRegs.stat.DBF = true;
		}
	}

	if (!idx) startcycles = VU0.cycle;
	else      startcycles = VU1.cycle;

	if (!idx) vu0ExecMicro(addr);
	else	  vu1ExecMicro(addr);

	if (!idx) { startcycles = ((VU0.cycle-startcycles) + ( vif0ch.qwc - (vif0.vifpacketsize >> 2) )); CPU_INT(VIF_VU0_FINISH, startcycles * BIAS); }
	else      { startcycles = ((VU1.cycle-startcycles) + ( vif1ch.qwc - (vif1.vifpacketsize >> 2) )); CPU_INT(VIF_VU1_FINISH, startcycles * BIAS); }


	//DevCon.Warning("Ran VU%x, VU0 Cycles %x, VU1 Cycles %x, start %x cycle %x", idx, g_vu0Cycles, g_vu1Cycles, startcycles, VU1.cycle);
	GetVifX.vifstalled = true;
}

void ExecuteVU(int idx)
{
	vifStruct& vifX = GetVifX;

	if((vifX.cmd & 0x7f) == 0x17)
	{
		vuExecMicro(idx, -1);
		vifX.cmd = 0;
	}
	else if((vifX.cmd & 0x7f) == 0x14 || (vifX.cmd & 0x7f) == 0x15)
	{
		vuExecMicro(idx, (u16)(vifXRegs.code) << 3);
		vifX.cmd = 0;
	}
}

//------------------------------------------------------------------
// Vif0/Vif1 Code Implementations
//------------------------------------------------------------------

vifOp(vifCode_Base) {
	vif1Only();
	pass1 { vif1Regs.base = vif1Regs.code & 0x3ff; vif1.cmd = 0; }
	pass3 { VifCodeLog("Base"); }
	return 0;
}

template<int idx> __fi int _vifCode_Direct(int pass, const u8* data, bool isDirectHL) {
	vif1Only();
	pass1 {
		int vifImm    = (u16)vif1Regs.code;
		vif1.tag.size = vifImm ? (vifImm*4) : (65536*4);
		return 0;
	}
	pass2 {
		const char* name = isDirectHL ? "DirectHL" : "Direct";
		GIF_TRANSFER_TYPE tranType = isDirectHL ? GIF_TRANS_DIRECTHL : GIF_TRANS_DIRECT;
		uint size = aMin(vif1.vifpacketsize, vif1.tag.size) * 4; // Get size in bytes
		uint ret  = gifUnit.TransferGSPacketData(tranType, (u8*)data, size);

		vif1.tag.size    -= ret/4; // Convert to u32's
		vif1Regs.stat.VGW = false;

		if (ret  &  3) DevCon.Warning("Vif %s: Ret wasn't a multiple of 4!", name); // Shouldn't happen
		if (size == 0) DevCon.Warning("Vif %s: No Data Transfer?", name); // Can this happen?
		if (size != ret) { // Stall if gif didn't process all the data (path2 queued)
			GUNIT_WARN("Vif %s: Stall! [size=%d][ret=%d]", name, size, ret);
			//gifUnit.PrintInfo();
			vif1.vifstalled   = true;
			vif1Regs.stat.VGW = true;
		}
		if (vif1.tag.size == 0) {
			vif1.cmd = 0;
		}
		return ret / 4;
	}
	return 0;
}

vifOp(vifCode_Direct) {
	pass3 { VifCodeLog("Direct"); }
	return _vifCode_Direct<idx>(pass, (u8*)data, 0);
}

vifOp(vifCode_DirectHL) {
	pass3 { VifCodeLog("DirectHL"); }
	return _vifCode_Direct<idx>(pass, (u8*)data, 1);
}

vifOp(vifCode_Flush) {
	vif1Only();
	vifStruct& vifX = GetVifX;
	pass1or2 {
		vif1Regs.stat.VGW = false;
		vifFlush(idx);
		if (gifUnit.checkPaths(1,1,0)) {
			GUNIT_WARN("Vif Flush: Stall!");
			//gifUnit.PrintInfo();
			vif1Regs.stat.VGW = true;
			vifX.vifstalled   = true;
		}
		else vifX.cmd = 0;
	}
	pass3 { VifCodeLog("Flush"); }
	return 0;
}

vifOp(vifCode_FlushA) {
	vif1Only();
	vifStruct& vifX = GetVifX;
	pass1or2 {
		Gif_Path& p3      = gifUnit.gifPath[GIF_PATH_3];
		u32       p1or2   = gifUnit.checkPaths(1,1,0);
		bool      doStall = false;
		vif1Regs.stat.VGW = false;
		vifFlush(idx);
		if (p3.state != GIF_PATH_IDLE || p1or2) {
			GUNIT_WARN("Vif FlushA: Stall!");
			//gifUnit.PrintInfo();
			if (p3.state != GIF_PATH_IDLE && !p1or2) { // Only path 3 left...
				GUNIT_WARN("Vif FlushA - Getting path3 to finish!");
				if (gifUnit.lastTranType == GIF_TRANS_FIFO
				&&  p3.state != GIF_PATH_IDLE && !p3.hasDataRemaining()) { 
					//p3.state= GIF_PATH_IDLE; // Does any game need this anymore?
					DevCon.Warning("Vif FlushA - path3 has no more data, but didn't EOP");
				}
				/*else { // Path 3 hasn't finished its current gs packet
					if (gifUnit.stat.APATH != 3 && gifUnit.Path3Masked()) {
						gifUnit.stat.APATH  = 3; // Hack: Force path 3 to finish (persona 3 needs this)
						//DevCon.Warning("Vif FlushA - Forcing path3 to finish current packet");
					}
					gifInterrupt();    // Feed path3 some gif dma data
					gifUnit.Execute(); // Execute path3 in-case gifInterrupt() didn't...
				}*/
				if (p3.state != GIF_PATH_IDLE) {
					doStall = true; // If path3 still isn't finished...
				}
			}
			else doStall = true;
		}
		if (doStall) {
			vif1Regs.stat.VGW = true;
			vifX.vifstalled   = true;
		}
		else vifX.cmd = 0;
	}
	pass3 { VifCodeLog("FlushA"); }
	return 0;
}

// ToDo: FixMe
vifOp(vifCode_FlushE) {
	vifStruct& vifX = GetVifX;
	pass1 { vifFlush(idx); vifX.cmd = 0; }
	pass3 { VifCodeLog("FlushE"); }
	return 0;
}

vifOp(vifCode_ITop) {
	pass1 { vifXRegs.itops = vifXRegs.code & 0x3ff; GetVifX.cmd = 0; }
	pass3 { VifCodeLog("ITop"); }
	return 0;
}

vifOp(vifCode_Mark) {
	vifStruct& vifX = GetVifX;
	pass1 {
		vifXRegs.mark     = (u16)vifXRegs.code;
		vifXRegs.stat.MRK = true;
		vifX.cmd          = 0;
	}
	pass3 { VifCodeLog("Mark"); }
	return 0;
}

static __fi void _vifCode_MPG(int idx, u32 addr, const u32 *data, int size) {
	VURegs& VUx = idx ? VU1 : VU0;
	pxAssert(VUx.Micro > 0);

	if (idx && THREAD_VU1) {
		vu1Thread.WriteMicroMem(addr, (u8*)data, size*4);
		return;
	}
	if (memcmp_mmx(VUx.Micro + addr, data, size*4)) {
		// Clear VU memory before writing!
		if (!idx)  CpuVU0->Clear(addr, size*4);
		else	   CpuVU1->Clear(addr, size*4);
		memcpy_fast(VUx.Micro + addr, data, size*4);
	}
}

vifOp(vifCode_MPG) {
	vifStruct& vifX = GetVifX;
	pass1 {
		int    vifNum =  (u8)(vifXRegs.code >> 16);
		vifX.tag.addr = (u16)(vifXRegs.code <<  3) & (idx ? 0x3fff : 0xfff);
		vifX.tag.size = vifNum ? (vifNum*2) : 512;
		vifFlush(idx);
		return 1;
	}
	pass2 {
		if (vifX.vifpacketsize < vifX.tag.size) { // Partial Transfer
			if((vifX.tag.addr + vifX.vifpacketsize*4) > (idx ? 0x4000 : 0x1000)) {
				DevCon.Warning("Vif%d MPG Split Overflow", idx);
			}
			_vifCode_MPG(idx,    vifX.tag.addr, data, vifX.vifpacketsize);
			vifX.tag.addr   +=   vifX.vifpacketsize * 4;
			vifX.tag.size   -=   vifX.vifpacketsize;
			return vifX.vifpacketsize;
		}
		else { // Full Transfer
			if((vifX.tag.addr + vifX.tag.size*4) > (idx ? 0x4000 : 0x1000)) {
				DevCon.Warning("Vif%d MPG Split Overflow", idx);
			}
			_vifCode_MPG(idx,  vifX.tag.addr, data, vifX.tag.size);
			int ret       = vifX.tag.size;
			vifX.tag.size = 0;
			vifX.cmd      = 0;
			return ret;
		}
	}
	pass3 { VifCodeLog("MPG"); }
	return 0;
}

vifOp(vifCode_MSCAL) {
	vifStruct& vifX = GetVifX;
	pass1 { 
		vifFlush(idx); 

		if(vifX.waitforvu == false)
		{
			vuExecMicro(idx, (u16)(vifXRegs.code) << 3); 
			vifX.cmd = 0;
		}
	}
	pass3 { VifCodeLog("MSCAL"); }
	return 0;
}

vifOp(vifCode_MSCALF) {
	vifStruct& vifX = GetVifX;
	pass1or2 {
		vifXRegs.stat.VGW = false;
		vifFlush(idx);
		if (u32 a = gifUnit.checkPaths(1,1,0)) {
			GUNIT_WARN("Vif MSCALF: Stall! [%d,%d]", !!(a&1), !!(a&2));
			vif1Regs.stat.VGW = true;
			vifX.vifstalled   = true;
		}
		if(vifX.waitforvu == false)
		{
			vuExecMicro(idx, (u16)(vifXRegs.code) << 3);
			vifX.cmd = 0;
		}
	}
	pass3 { VifCodeLog("MSCALF"); }
	return 0;
}

vifOp(vifCode_MSCNT) {
	vifStruct& vifX = GetVifX;
	pass1 { 
		vifFlush(idx); 
		if(vifX.waitforvu == false)
		{
			vuExecMicro(idx, -1);
			vifX.cmd = 0;
		}
	}
	pass3 { VifCodeLog("MSCNT"); }
	return 0;
}

// ToDo: FixMe
vifOp(vifCode_MskPath3) {
	vif1Only();
	pass1 {		
		vif1Regs.mskpath3 = (vif1Regs.code >> 15) & 0x1;
		gifRegs.stat.M3P  = (vif1Regs.code >> 15) & 0x1;
		GUNIT_LOG("Vif1 - MskPath3 [p3 = %s]", vif1Regs.mskpath3 ? "disabled" : "enabled");
		if(!vif1Regs.mskpath3) {
			//if(!gifUnit.gifPath[GIF_PATH_3].isDone() || gifRegs.stat.P3Q || gifRegs.stat.IP3) {
				GUNIT_WARN("Path3 triggering!");
				gifInterrupt();
			//}
		}
		vif1.cmd = 0;
	}
	pass3 { VifCodeLog("MskPath3"); }
	return 0;
}

vifOp(vifCode_Nop) {
	pass1 { GetVifX.cmd = 0; }
	pass3 { VifCodeLog("Nop"); }
	return 0;
}

// ToDo: Review Flags
vifOp(vifCode_Null) {
	vifStruct& vifX = GetVifX;
	pass1 {
		// if ME1, then force the vif to interrupt
		if (!(vifXRegs.err.ME1)) { // Ignore vifcode and tag mismatch error
			Console.WriteLn("Vif%d: Unknown VifCmd! [%x]", idx, vifX.cmd);
			vifXRegs.stat.ER1 = true;
			vifX.vifstalled = true;
			//vifX.irq++;
		}
		vifX.cmd = 0;
	}
	pass2 { Console.Error("Vif%d bad vifcode! [CMD = %x]", idx, vifX.cmd); }
	pass3 { VifCodeLog("Null"); }
	return 0;
}

vifOp(vifCode_Offset) {
	vif1Only();
	pass1 {
		vif1Regs.stat.DBF	= false;
		vif1Regs.ofst		= vif1Regs.code & 0x3ff;
		vif1Regs.tops		= vif1Regs.base;
		vif1.cmd			= 0;
	}
	pass3 { VifCodeLog("Offset"); }
	return 0;
}

template<int idx> static __fi int _vifCode_STColRow(const u32* data, u32* pmem2) {
	vifStruct& vifX = GetVifX;

	int ret = min(4 - vifX.tag.addr, vifX.vifpacketsize);
	pxAssume(vifX.tag.addr < 4);
	pxAssume(ret > 0);

	switch (ret) {
		case 4: pmem2[3] = data[3];
		case 3: pmem2[2] = data[2];
		case 2: pmem2[1] = data[1];
		case 1: pmem2[0] = data[0];
				break;
		jNO_DEFAULT
	}

	vifX.tag.addr += ret;
	vifX.tag.size -= ret;
	if (!vifX.tag.size) vifX.cmd = 0;

	return ret;
}

vifOp(vifCode_STCol) {
	vifStruct& vifX = GetVifX;
	pass1 {
		vifX.tag.addr = 0;
		vifX.tag.size = 4;
		return 1;
	}
	pass2 {
		u32 ret = _vifCode_STColRow<idx>(data, &vifX.MaskCol._u32[vifX.tag.addr]);
		if (idx && THREAD_VU1) { vu1Thread.WriteCol(vifX); }
		return ret;
	}
	pass3 { VifCodeLog("STCol"); }
	return 0;
}

vifOp(vifCode_STRow) {
	vifStruct& vifX = GetVifX;
	pass1 {
		vifX.tag.addr = 0;
		vifX.tag.size = 4;
		return 1;
	}
	pass2 {
		u32 ret = _vifCode_STColRow<idx>(data, &vifX.MaskRow._u32[vifX.tag.addr]);
		if (idx && THREAD_VU1) { vu1Thread.WriteRow(vifX); }
		return ret;
	}
	pass3 { VifCodeLog("STRow"); }
	return 0;
}

vifOp(vifCode_STCycl) {
	vifStruct& vifX = GetVifX;
	pass1 {
		vifXRegs.cycle.cl = (u8)(vifXRegs.code);
		vifXRegs.cycle.wl = (u8)(vifXRegs.code >> 8);
		vifX.cmd		   = 0;
	}
	pass3 { VifCodeLog("STCycl"); }
	return 0;
}

vifOp(vifCode_STMask) {
	vifStruct& vifX = GetVifX;
	pass1 { vifX.tag.size = 1; }
	pass2 { vifXRegs.mask = data[0]; vifX.tag.size = 0; vifX.cmd = 0; }
	pass3 { VifCodeLog("STMask"); }
	return 1;
}

vifOp(vifCode_STMod) {
	pass1 { vifXRegs.mode = vifXRegs.code & 0x3; GetVifX.cmd = 0; }
	pass3 { VifCodeLog("STMod"); }
	return 0;
}

template< uint idx >
static uint calc_addr(bool flg)
{
	VIFregisters& vifRegs = vifXRegs;

	uint retval = vifRegs.code;
	if (idx && flg) retval += vifRegs.tops;
	return retval & (idx ? 0x3ff : 0xff);
}

vifOp(vifCode_Unpack) {
	pass1 {
		vifUnpackSetup<idx>(data);
		return 1;
	}
	pass2 { 
		return nVifUnpack<idx>((u8*)data);
	}
	pass3 {
		vifStruct& vifX = GetVifX;
		VIFregisters& vifRegs = vifXRegs;
		uint vl = vifX.cmd & 0x03;
		uint vn = (vifX.cmd >> 2) & 0x3;
		bool flg = (vifRegs.code >> 15) & 1;
		static const char* const	vntbl[] = { "S", "V2", "V3", "V4" };
		static const uint			vltbl[] = { 32,	  16,   8,    5   };

		VifCodeLog("Unpack %s_%u (%s) @ 0x%04X%s (cl=%u  wl=%u  num=0x%02X)",
			vntbl[vn], vltbl[vl], (vifX.cmd & 0x10) ? "masked" : "unmasked",
			calc_addr<idx>(flg), flg ? "(FLG)" : "",
			vifRegs.cycle.cl, vifRegs.cycle.wl, (vifXRegs.code >> 16) & 0xff
		);
	}
	return 0;
}

//------------------------------------------------------------------
// Vif0/Vif1 Code Tables
//------------------------------------------------------------------

__aligned16 FnType_VifCmdHandler* const vifCmdHandler[2][128] =
{
	{
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
	},
	{
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
	}
};
