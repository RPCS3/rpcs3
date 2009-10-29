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
#include "VifDma.h"
#include "VifDma_internal.h"
#include "VUmicro.h"
#include "Tags.h"

__aligned16 u32 g_vif1Masks[64];
u32 g_vif1HasMask3[4] = {0};

extern void (*Vif1CMDTLB[82])();
extern int (__fastcall *Vif1TransTLB[128])(u32 *data);

Path3Modes Path3progress = STOPPED_MODE;
vifStruct vif1;
static u32 *vif1ptag;

static __aligned16 u32 splittransfer[4];
static u32 splitptr = 0;

__forceinline void vif1FLUSH()
{
	int _cycles = VU1.cycle;

	// fixme: Same as above, is this a "stalling" offense?  I think the cycles should
	// be added to cpuRegs.cycle instead of g_vifCycles, but not sure (air)

	if (VU0.VI[REG_VPU_STAT].UL & 0x100)
	{
		do
		{
			CpuVU1.ExecuteBlock();
		}
		while (VU0.VI[REG_VPU_STAT].UL & 0x100);

		g_vifCycles += (VU1.cycle - _cycles) * BIAS;
	}
}


void vif1Init()
{
	SetNewMask(g_vif1Masks, g_vif1HasMask3, 0, 0xffffffff);
}

static __forceinline void vif1UNPACK(u32 *data)
{
	int vifNum;

	if ((vif1Regs->cycle.wl == 0) && (vif1Regs->cycle.wl < vif1Regs->cycle.cl))
    {
        Console.WriteLn("Vif1 CL %d, WL %d", vif1Regs->cycle.cl, vif1Regs->cycle.wl);
		vif1.cmd &= ~0x7f;
        return;
	}
	
	//vif1FLUSH();

	vif1.usn = (vif1Regs->code >> 14) & 0x1;
	vifNum = (vif1Regs->code >> 16) & 0xff;

	if (vifNum == 0) vifNum = 256;
	vif1Regs->num = vifNum;

	if (vif1Regs->cycle.wl <= vif1Regs->cycle.cl)
	{
		vif1.tag.size = ((vifNum * VIFfuncTable[ vif1.cmd & 0xf ].gsize) + 3) >> 2;
	}
	else
	{
		int n = vif1Regs->cycle.cl * (vifNum / vif1Regs->cycle.wl) +
		        _limit(vifNum % vif1Regs->cycle.wl, vif1Regs->cycle.cl);

		vif1.tag.size = ((n * VIFfuncTable[ vif1.cmd & 0xf ].gsize) + 3) >> 2;
	}

	if ((vif1Regs->code >> 15) & 0x1)
		vif1.tag.addr = (vif1Regs->code + vif1Regs->tops) & 0x3ff;
	else
		vif1.tag.addr = vif1Regs->code & 0x3ff;

	vif1Regs->offset = 0;
	vif1.cl = 0;
	vif1.tag.addr <<= 4;
	vif1.tag.cmd  = vif1.cmd;

}

static __forceinline void vif1mpgTransfer(u32 addr, u32 *data, int size)
{
	/*	Console.WriteLn("vif1mpgTransfer addr=%x; size=%x", addr, size);
		{
			FILE *f = fopen("vu1.raw", "wb");
			fwrite(data, 1, size*4, f);
			fclose(f);
		}*/
	pxAssert(VU1.Micro > 0);
	if (memcmp(VU1.Micro + addr, data, size << 2))
	{
		CpuVU1.Clear(addr, size << 2); // Clear before writing! :/
		memcpy_fast(VU1.Micro + addr, data, size << 2);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif1 Data Transfer Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int __fastcall Vif1TransNull(u32 *data)  // Shouldnt go here
{
	Console.WriteLn("VIF1 Shouldn't go here CMD = %x", vif1Regs->code);
	vif1.cmd = 0;
	return 0;
}

static int __fastcall Vif1TransSTMask(u32 *data)  // STMASK
{
	SetNewMask(g_vif1Masks, g_vif1HasMask3, data[0], vif1Regs->mask);
	vif1Regs->mask = data[0];
	VIF_LOG("STMASK == %x", vif1Regs->mask);

	vif1.tag.size = 0;
	vif1.cmd = 0;
	return 1;
}

static int __fastcall Vif1TransSTRow(u32 *data)  // STROW
{
	int ret;

	u32* pmem = &vif1Regs->r0 + (vif1.tag.addr << 2);
	u32* pmem2 = g_vifmask.Row1 + vif1.tag.addr;
	pxAssert(vif1.tag.addr < 4);
	ret = min(4 - vif1.tag.addr, vif1.vifpacketsize);
	pxAssert(ret > 0);
	
	switch (ret)
	{
		case 4:
			pmem[12] = data[3];
			pmem2[3] = data[3];
		case 3:
			pmem[8] = data[2];
			pmem2[2] = data[2];
		case 2:
			pmem[4] = data[1];
			pmem2[1] = data[1];
		case 1:
			pmem[0] = data[0];
			pmem2[0] = data[0];
			break;
		jNO_DEFAULT;
	}
	
	vif1.tag.addr += ret;
	vif1.tag.size -= ret;
	if (vif1.tag.size == 0) vif1.cmd = 0;

	return ret;
}

static int __fastcall Vif1TransSTCol(u32 *data)
{
	int ret;

	u32* pmem = &vif1Regs->c0 + (vif1.tag.addr << 2);
	u32* pmem2 = g_vifmask.Col1 + vif1.tag.addr;
	ret = min(4 - vif1.tag.addr, vif1.vifpacketsize);
	switch (ret)
	{
		case 4:
			pmem[12] = data[3];
			pmem2[3] = data[3];
		case 3:
			pmem[8] = data[2];
			pmem2[2] = data[2];
		case 2:
			pmem[4] = data[1];
			pmem2[1] = data[1];
		case 1:
			pmem[0] = data[0];
			pmem2[0] = data[0];
			break;
			jNO_DEFAULT;
	}
	vif1.tag.addr += ret;
	vif1.tag.size -= ret;
	if (vif1.tag.size == 0) vif1.cmd = 0;
	return ret;
}

static int __fastcall Vif1TransMPG(u32 *data)
{
	if (vif1.vifpacketsize < vif1.tag.size)
	{
		if((vif1.tag.addr + vif1.vifpacketsize) > 0x4000) DevCon.Warning("Vif1 MPG Split Overflow");
		vif1mpgTransfer(vif1.tag.addr, data, vif1.vifpacketsize);
		vif1.tag.addr += vif1.vifpacketsize << 2;
		vif1.tag.size -= vif1.vifpacketsize;
		return vif1.vifpacketsize;
	}
	else
	{
		int ret;
		if((vif1.tag.addr + vif1.tag.size) > 0x4000) DevCon.Warning("Vif1 MPG Overflow");
		vif1mpgTransfer(vif1.tag.addr, data, vif1.tag.size);
		ret = vif1.tag.size;
		vif1.tag.size = 0;
		vif1.cmd = 0;
		return ret;
	}
}

// Dummy GIF-TAG Packet to Guarantee Count = 1
extern __aligned16 u32 nloop0_packet[4];

static int __fastcall Vif1TransDirectHL(u32 *data)
{
	int ret = 0;

	if ((vif1.cmd & 0x7f) == 0x51)
	{
		if (gif->chcr.STR && (!vif1Regs->mskpath3 && (Path3progress == IMAGE_MODE))) //PATH3 is in image mode, so wait for end of transfer
		{
			vif1Regs->stat.VGW = 1;
			return 0;
		}
	}

	gifRegs->stat.APATH |= GIF_APATH2;
	gifRegs->stat.OPH = 1;
	

	if (splitptr > 0)  //Leftover data from the last packet, filling the rest and sending to the GS
	{
		if ((splitptr < 4) && (vif1.vifpacketsize >= (4 - splitptr)))
		{
			while (splitptr < 4)
			{
				splittransfer[splitptr++] = (u32)data++;
				ret++;
				vif1.tag.size--;
			}
		}

        Registers::Freeze();
		// copy 16 bytes the fast way:
		const u64* src = (u64*)splittransfer[0];
		mtgsThread.PrepDataPacket(GIF_PATH_2, nloop0_packet, 1);
		u64* dst = (u64*)mtgsThread.GetDataPacketPtr();
		dst[0] = src[0];
		dst[1] = src[1];

		mtgsThread.SendDataPacket();
        Registers::Thaw();

		if (vif1.tag.size == 0) vif1.cmd = 0;
		splitptr = 0;
		return ret;
	}
	
	if (vif1.vifpacketsize < vif1.tag.size)
	{
		if (vif1.vifpacketsize < 4 && splitptr != 4)   //Not a full QW left in the buffer, saving left over data
		{
			ret = vif1.vifpacketsize;
			while (ret > 0)
			{
				splittransfer[splitptr++] = (u32)data++;
				vif1.tag.size--;
				ret--;
			}
			return vif1.vifpacketsize;
		}
		vif1.tag.size -= vif1.vifpacketsize;
		ret = vif1.vifpacketsize;
	}
	else
	{
		gifRegs->stat.clear(GIF_STAT_APATH2 | GIF_STAT_OPH);
		ret = vif1.tag.size;
		vif1.tag.size = 0;
		vif1.cmd = 0;
	}

	//TODO: ret is guaranteed to be qword aligned ?

	Registers::Freeze();

	// Round ret up, just in case it's not 128bit aligned.
	const uint count = mtgsThread.PrepDataPacket(GIF_PATH_2, data, (ret + 3) >> 2);
	memcpy_fast(mtgsThread.GetDataPacketPtr(), data, count << 4);
	mtgsThread.SendDataPacket();

	Registers::Thaw();

	return ret;
}

static int  __fastcall Vif1TransUnpack(u32 *data)
{
    XMMRegisters::Freeze();

	if (vif1.vifpacketsize < vif1.tag.size)
	{
		int ret = vif1.tag.size;
		/* size is less that the total size, transfer is  'in pieces' */
		if (vif1Regs->offset != 0 || vif1.cl != 0)
		{
			vif1.tag.size -= vif1.vifpacketsize - VIFalign<1>(data, &vif1.tag, vif1.vifpacketsize);
			ret = ret - vif1.tag.size;
			data += ret;
			if ((vif1.vifpacketsize - ret) > 0) VIFunpack<1>(data, &vif1.tag, vif1.vifpacketsize - ret);
			ProcessMemSkip<1>((vif1.vifpacketsize - ret) << 2, (vif1.cmd & 0xf));
			vif1.tag.size -= (vif1.vifpacketsize - ret);
		}
		else
        {
            VIFunpack<1>(data, &vif1.tag, vif1.vifpacketsize);

            ProcessMemSkip<1>(vif1.vifpacketsize << 2, (vif1.cmd & 0xf));
            vif1.tag.size -= vif1.vifpacketsize;
		}
		
        XMMRegisters::Thaw();
		return vif1.vifpacketsize;
	}
	else
	{
		int ret = vif1.tag.size;

		if (vif1Regs->offset != 0 || vif1.cl != 0)
		{
			vif1.tag.size = VIFalign<1>(data, &vif1.tag, vif1.tag.size);
			data += ret - vif1.tag.size;
			if (vif1.tag.size > 0) VIFunpack<1>(data, &vif1.tag, vif1.tag.size);
		}
		else
		{
			/* we got all the data, transfer it fully */
			VIFunpack<1>(data, &vif1.tag, vif1.tag.size);
		}
		
        vif1.tag.size = 0;
        vif1.cmd = 0;
        XMMRegisters::Thaw();
        return ret;
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif1 CMD Base Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void Vif1CMDNop()  // NOP
{
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDSTCycl()  // STCYCL
{
	vif1Regs->cycle.cl = (u8)vif1Regs->code;
	vif1Regs->cycle.wl = (u8)(vif1Regs->code >> 8);
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDOffset()  // OFFSET
{
	vif1Regs->ofst  = vif1Regs->code & 0x3ff;
	vif1Regs->stat.DBF = 0;
	vif1Regs->tops  = vif1Regs->base;
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDBase()  // BASE
{
	vif1Regs->base = vif1Regs->code & 0x3ff;
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDITop()  // ITOP
{
	vif1Regs->itops = vif1Regs->code & 0x3ff;
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDSTMod()  // STMOD
{
	vif1Regs->mode = vif1Regs->code & 0x3;
	vif1.cmd &= ~0x7f;
}

u8 schedulepath3msk = 0;

void Vif1MskPath3()  // MSKPATH3
{
	vif1Regs->mskpath3 = schedulepath3msk & 0x1;
	//Console.WriteLn("VIF MSKPATH3 %x", vif1Regs->mskpath3);

	if (vif1Regs->mskpath3)
	{
		gifRegs->stat.M3P = 1;
	}
	else
	{
		//Let the Gif know it can transfer again (making sure any vif stall isnt unset prematurely)
		Path3progress = TRANSFER_MODE;
		gifRegs->stat.IMT = 0;
		CPU_INT(2, 4);
	}

	schedulepath3msk = 0;
}
static void Vif1CMDMskPath3()  // MSKPATH3
{
	if (vif1ch->chcr.STR)
	{
		schedulepath3msk = 0x10 | ((vif1Regs->code >> 15) & 0x1);
		vif1.vifstalled = true;
	}
	else
	{
		schedulepath3msk = (vif1Regs->code >> 15) & 0x1;
		Vif1MskPath3();
	}
	vif1.cmd &= ~0x7f;
}


static void Vif1CMDMark()  // MARK
{
	vif1Regs->mark = (u16)vif1Regs->code;
	vif1Regs->stat.MRK = 1;
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDFlush()  // FLUSH/E/A
{
	vif1FLUSH();

	if ((vif1.cmd & 0x7f) == 0x13)
	{
		// Gif is already transferring so wait for it.
		if (((Path3progress != STOPPED_MODE) || !vif1Regs->mskpath3) && gif->chcr.STR)
		{
			vif1Regs->stat.VGW = 1;
			CPU_INT(2, 4);
		}
	}

	vif1.cmd &= ~0x7f;
}

static void Vif1CMDMSCALF()  //MSCAL/F
{
	vif1FLUSH();
	vuExecMicro<1>((u16)(vif1Regs->code) << 3);
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDMSCNT()  // MSCNT
{
	vuExecMicro<1>(-1);
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDSTMask()  // STMASK
{
	vif1.tag.size = 1;
}

static void Vif1CMDSTRowCol() // STROW / STCOL
{
	vif1.tag.addr = 0;
	vif1.tag.size = 4;
}

static void Vif1CMDMPGTransfer()  // MPG
{
	int vifNum;
	//vif1FLUSH();
	vifNum = (u8)(vif1Regs->code >> 16);

	if (vifNum == 0) vifNum = 256;

	vif1.tag.addr = (u16)((vif1Regs->code) << 3) & 0x3fff;
	vif1.tag.size = vifNum * 2;
}

static void Vif1CMDDirectHL()  // DIRECT/HL
{
	int vifImm;
	vifImm = (u16)vif1Regs->code;

	if (vifImm == 0)
		vif1.tag.size = 65536 << 2;
	else
		vif1.tag.size = vifImm << 2;
}

static void Vif1CMDNull()  // invalid opcode
{
	// if ME1, then force the vif to interrupt

	if (!(vif1Regs->err.ME1))   //Ignore vifcode and tag mismatch error
	{
		Console.WriteLn("UNKNOWN VifCmd: %x\n", vif1.cmd);
		vif1Regs->stat.ER1 = 1;
		vif1.irq++;
	}
	vif1.cmd = 0;
}

//  Vif1 Data Transfer Table

int (__fastcall *Vif1TransTLB[128])(u32 *data) =
{
	Vif1TransNull	 , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x7*/
	Vif1TransNull	 , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0xF*/
	Vif1TransNull	 , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x17*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x1F*/
	Vif1TransSTMask  , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x27*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x2F*/
	Vif1TransSTRow	 , Vif1TransSTCol	, Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x37*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x3F*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x47*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransMPG	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x4F*/
	Vif1TransDirectHL, Vif1TransDirectHL, Vif1TransNull	  , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull	, Vif1TransNull	  , Vif1TransNull   , /*0x57*/
	Vif1TransNull	 , Vif1TransNull	, Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x5F*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransNull   , /*0x67*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , /*0x6F*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransNull   , /*0x77*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransNull   , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack   /*0x7F*/
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif1 CMD Table
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void (*Vif1CMDTLB[82])() =
{
	Vif1CMDNop	   , Vif1CMDSTCycl  , Vif1CMDOffset		, Vif1CMDBase , Vif1CMDITop  , Vif1CMDSTMod , Vif1CMDMskPath3, Vif1CMDMark , /*0x7*/
	Vif1CMDNull	   , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0xF*/
	Vif1CMDFlush   , Vif1CMDFlush   , Vif1CMDNull		, Vif1CMDFlush, Vif1CMDMSCALF, Vif1CMDMSCALF, Vif1CMDNull	, Vif1CMDMSCNT, /*0x17*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x1F*/
	Vif1CMDSTMask  , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull	, Vif1CMDNull , /*0x27*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull	, Vif1CMDNull , /*0x2F*/
	Vif1CMDSTRowCol, Vif1CMDSTRowCol, Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull	, Vif1CMDNull , /*0x37*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x3F*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x47*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDMPGTransfer, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x4F*/
	Vif1CMDDirectHL, Vif1CMDDirectHL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int VIF1transfer(u32 *data, int size, int istag)
{
	int ret;
	int transferred = vif1.vifstalled ? vif1.irqoffset : 0; // irqoffset necessary to add up the right qws, or else will spin (spiderman)

	VIF_LOG("VIF1transfer: size %x (vif1.cmd %x)", size, vif1.cmd);

	vif1.irqoffset = 0;
	vif1.vifstalled = false;
	vif1.stallontag = false;
	vif1.vifpacketsize = size;

	while (vif1.vifpacketsize > 0)
	{
		if(vif1Regs->stat.VGW) break;

		if (vif1.cmd)
		{
			vif1Regs->stat.VPS = VPS_TRANSFERRING; //Decompression has started

			ret = Vif1TransTLB[vif1.cmd](data);
			data += ret;
			vif1.vifpacketsize -= ret;

			//We are once again waiting for a new vifcode as the command has cleared
			if (vif1.cmd == 0) vif1Regs->stat.VPS = VPS_IDLE;
			continue;
		}

		if (vif1.tag.size != 0) DevCon.Error("no vif1 cmd but tag size is left last cmd read %x", vif1Regs->code);

		if (vif1.irq) break;

		vif1.cmd = (data[0] >> 24);
		vif1Regs->code = data[0];
		vif1Regs->stat.VPS |= VPS_DECODING;
		
		if ((vif1.cmd & 0x60) == 0x60)
		{
			vif1UNPACK(data);
		}
		else
		{
			VIF_LOG("VIFtransfer: cmd %x, num %x, imm %x, size %x", vif1.cmd, (data[0] >> 16) & 0xff, data[0] & 0xffff, vif1.vifpacketsize);

			if ((vif1.cmd & 0x7f) > 0x51)
			{
				if (!(vif0Regs->err.ME1))    //Ignore vifcode and tag mismatch error
				{
					Console.WriteLn("UNKNOWN VifCmd: %x", vif1.cmd);
					vif1Regs->stat.ER1 = 1;
					vif1.irq++;
				}
				vif1.cmd = 0;
			}
			else Vif1CMDTLB[(vif1.cmd & 0x7f)]();
		}

		++data;
		--vif1.vifpacketsize;

		if ((vif1.cmd & 0x80))
		{
			vif1.cmd &= 0x7f;

			if (!(vif1Regs->err.MII)) //i bit on vifcode and not masked by VIF1_ERR
			{
				VIF_LOG("Interrupt on VIFcmd: %x (INTC_MASK = %x)", vif1.cmd, psHu32(INTC_MASK));

				++vif1.irq;

				if (istag && vif1.tag.size <= vif1.vifpacketsize) vif1.stallontag = true;

				if (vif1.tag.size == 0) break;
			}
		}

		if(!vif1.cmd) vif1Regs->stat.VPS = VPS_IDLE;

		if((vif1Regs->stat.VGW) || vif1.vifstalled == true) break;
	} // End of Transfer loop

	transferred += size - vif1.vifpacketsize;
	g_vifCycles += (transferred >> 2) * BIAS; /* guessing */
	vif1.irqoffset = transferred % 4; // cannot lose the offset

	if (vif1.irq && vif1.cmd == 0)
	{
		vif1.vifstalled = true;

		if (((vif1Regs->code >> 24) & 0x7f) != 0x7) vif1Regs->stat.VIS = 1; // Note: commenting this out fixes WALL-E

		if (vif1ch->qwc == 0 && (vif1.irqoffset == 0 || istag == 1)) vif1.inprogress &= ~0x1;

		// spiderman doesn't break on qw boundaries
		if (istag) return -2;

		transferred = transferred >> 2;
		vif1ch->madr += (transferred << 4);
		vif1ch->qwc -= transferred;

		if ((vif1ch->qwc == 0) && (vif1.irqoffset == 0)) vif1.inprogress = 0;
		//Console.WriteLn("Stall on vif1, FromSPR = %x, Vif1MADR = %x Sif0MADR = %x STADR = %x", psHu32(0x1000d010), vif1ch->madr, psHu32(0x1000c010), psHu32(DMAC_STADR));
		return -2;
	}

	vif1Regs->stat.VPS = 0; //Vif goes idle as the stall happened between commands;
	if (vif1.cmd) vif1Regs->stat.VPS = VPS_WAITING;  //Otherwise we wait for the data

	if (!istag)
	{
		transferred = transferred >> 2;
		vif1ch->madr += (transferred << 4);
		vif1ch->qwc -= transferred;
	}

	if (vif1Regs->stat.VGW)
	{
		vif1.vifstalled = true;
	}

	if (vif1ch->qwc == 0 && (vif1.irqoffset == 0 || istag == 1)) vif1.inprogress &= ~0x1;

	return vif1.vifstalled ? -2 : 0;
}

void vif1TransferFromMemory()
{
	int size;
	u64* pMem = (u64*)dmaGetAddr(vif1ch->madr);

	// VIF from gsMemory
	if (pMem == NULL)  						//Is vif0ptag empty?
	{
		Console.WriteLn("Vif1 Tag BUSERR");
		dmacRegs->stat.BEIS = 1;      //Bus Error
		vif1Regs->stat.FQC = 0;
		
		vif1ch->qwc = 0;
		vif1.done = true;
		CPU_INT(1, 0);
		return;						   //An error has occurred.
	}

	// MTGS concerns:  The MTGS is inherently disagreeable with the idea of downloading
	// stuff from the GS.  The *only* way to handle this case safely is to flush the GS
	// completely and execute the transfer there-after.

    XMMRegisters::Freeze();

	if (GSreadFIFO2 == NULL)
	{
		for (size = vif1ch->qwc; size > 0; --size)
		{
			if (size > 1)
			{
				mtgsThread.WaitGS();
				GSreadFIFO((u64*)&PS2MEM_HW[0x5000]);
			}
			pMem[0] = psHu64(VIF1_FIFO);
			pMem[1] = psHu64(VIF1_FIFO + 8);
			pMem += 2;
		}
	}
	else
	{
		mtgsThread.WaitGS();
		GSreadFIFO2(pMem, vif1ch->qwc);

		// set incase read
		psHu64(VIF1_FIFO) = pMem[2*vif1ch->qwc-2];
		psHu64(VIF1_FIFO + 8) = pMem[2*vif1ch->qwc-1];
	}

    XMMRegisters::Thaw();

	g_vifCycles += vif1ch->qwc * 2;
	vif1ch->madr += vif1ch->qwc * 16; // mgs3 scene changes
	vif1ch->qwc = 0;
}

int  _VIF1chain()
{
	u32 *pMem;
	u32 ret;

	if (vif1ch->qwc == 0)
	{
		vif1.inprogress = 0;
		return 0;
	}

	if (vif1.dmamode == VIF_NORMAL_FROM_MEM_MODE)
	{
		vif1TransferFromMemory();
		vif1.inprogress = 0;
		return 0;
	}

	pMem = (u32*)dmaGetAddr(vif1ch->madr);
	if (pMem == NULL)
	{
		vif1.cmd = 0;
		vif1.tag.size = 0;
		vif1ch->qwc = 0;
		return 0;
	}

	VIF_LOG("VIF1chain size=%d, madr=%lx, tadr=%lx",
	        vif1ch->qwc, vif1ch->madr, vif1ch->tadr);

	if (vif1.vifstalled)
		ret = VIF1transfer(pMem + vif1.irqoffset, vif1ch->qwc * 4 - vif1.irqoffset, 0);
	else
		ret = VIF1transfer(pMem, vif1ch->qwc * 4, 0);

	return ret;
}

bool _chainVIF1()
{
	return vif1.done; // Return Done
}

__forceinline void vif1SetupTransfer()
{
	switch (vif1.dmamode)
	{
		case VIF_NORMAL_TO_MEM_MODE:
		case VIF_NORMAL_FROM_MEM_MODE:
			vif1.inprogress = 1;
			vif1.done = true;
			g_vifCycles = 2;
			break;

		case VIF_CHAIN_MODE:
			int id;
			int ret;

			vif1ptag = (u32*)dmaGetAddr(vif1ch->tadr); //Set memory pointer to TADR

			if (!(Tag::Transfer("Vif1 Tag", vif1ch, vif1ptag))) return;

			vif1ch->madr = vif1ptag[1];            //MADR = ADDR field
			g_vifCycles += 1; // Add 1 g_vifCycles from the QW read for the tag
			id = Tag::Id(vif1ptag); //ID for DmaChain copied from bit 28 of the tag

			// Transfer dma tag if tte is set

			VIF_LOG("VIF1 Tag %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx\n",
			        vif1ptag[1], vif1ptag[0], vif1ch->qwc, id, vif1ch->madr, vif1ch->tadr);

			if (!vif1.done && ((dmacRegs->ctrl.STD == STD_VIF1) && (id == 4)))   // STD == VIF1
			{
				// there are still bugs, need to also check if gif->madr +16*qwc >= stadr, if not, stall
				if ((vif1ch->madr + vif1ch->qwc * 16) >= dmacRegs->stadr.ADDR)
				{
					// stalled
					hwDmacIrq(DMAC_STALL_SIS);
					return;
				}
			}

			vif1.inprogress = 1;

			if (vif1ch->chcr.TTE)
			{

				if (vif1.vifstalled)
					ret = VIF1transfer(vif1ptag + (2 + vif1.irqoffset), 2 - vif1.irqoffset, 1);  //Transfer Tag on stall
				else
					ret = VIF1transfer(vif1ptag + 2, 2, 1);  //Transfer Tag

				if (ret < 0 && vif1.irqoffset < 2)
				{
					vif1.inprogress = 0; //Better clear this so it has to do it again (Jak 1)
					return;       //There has been an error or an interrupt
				}
			}

			vif1.irqoffset = 0;
			vif1.done |= hwDmacSrcChainWithStack(vif1ch, id);

			//Check TIE bit of CHCR and IRQ bit of tag
			if (vif1ch->chcr.TIE && (Tag::IRQ(vif1ptag)))
			{
				VIF_LOG("dmaIrq Set");

                //End Transfer
				vif1.done = true;
				return;
			}
			break;
	}
}

__forceinline void vif1Interrupt()
{
	VIF_LOG("vif1Interrupt: %8.8x", cpuRegs.cycle);

	g_vifCycles = 0;

	if (schedulepath3msk) Vif1MskPath3();

	if ((vif1Regs->stat.VGW))
	{
		if (gif->chcr.STR)
		{
			CPU_INT(1, gif->qwc * BIAS);
			return;
		}
		else 
		{
		    vif1Regs->stat.VGW = 0;
		}
	}

	if (!(vif1ch->chcr.STR)) Console.WriteLn("Vif1 running when CHCR == %x", vif1ch->chcr._u32);

	if (vif1.irq && vif1.tag.size == 0)
	{
		vif1Regs->stat.INT = 1;
		hwIntcIrq(VIF1intc);
		--vif1.irq;
		if (vif1Regs->stat.test(VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
		{
			vif1Regs->stat.FQC = 0;

			// One game doesn't like vif stalling at end, can't remember what. Spiderman isn't keen on it tho
			vif1ch->chcr.STR = 0;
			return;
		}
		else if ((vif1ch->qwc > 0) || (vif1.irqoffset > 0))
		{
			if (vif1.stallontag)
				vif1SetupTransfer();
			else
				_VIF1chain();//CPU_INT(13, vif1ch->qwc * BIAS);
		}
	}

	if (vif1.inprogress & 0x1)
	{
		_VIF1chain();
		CPU_INT(1, g_vifCycles);
		return;
	}

	if (!vif1.done)
	{

		if (!(dmacRegs->ctrl.DMAE))
		{
			Console.WriteLn("vif1 dma masked");
			return;
		}

		if ((vif1.inprogress & 0x1) == 0) vif1SetupTransfer();

		CPU_INT(1, g_vifCycles);
		return;
	}

	if (vif1.vifstalled && vif1.irq)
	{
		CPU_INT(1, 0);
		return; //Dont want to end if vif is stalled.
	}
#ifdef PCSX2_DEVBUILD
	if (vif1ch->qwc > 0) Console.WriteLn("VIF1 Ending with %x QWC left");
	if (vif1.cmd != 0) Console.WriteLn("vif1.cmd still set %x tag size %x", vif1.cmd, vif1.tag.size);
#endif

	vif1Regs->stat.VPS = 0; //Vif goes idle as the stall happened between commands;
	vif1ch->chcr.STR = 0;
	g_vifCycles = 0;
	hwDmacIrq(DMAC_VIF1);

	//Im not totally sure why Path3 Masking makes it want to see stuff in the fifo
	//Games effected by setting, Fatal Frame, KH2, Shox, Crash N Burn, GT3/4 possibly
	//Im guessing due to the full gs fifo before the reverse? (Refraction)
	//Note also this is only the condition for reverse fifo mode, normal direction clears it as normal
	if (!vif1Regs->mskpath3 || vif1ch->chcr.DIR) vif1Regs->stat.FQC = 0;
}

void dmaVIF1()
{
	VIF_LOG("dmaVIF1 chcr = %lx, madr = %lx, qwc  = %lx\n"
	        "        tadr = %lx, asr0 = %lx, asr1 = %lx",
	        vif1ch->chcr, vif1ch->madr, vif1ch->qwc,
	        vif1ch->tadr, vif1ch->asr0, vif1ch->asr1);

	g_vifCycles = 0;
	vif1.inprogress = 0;

	if (dmacRegs->ctrl.MFD == MFD_VIF1)   // VIF MFIFO
	{
		//Console.WriteLn("VIFMFIFO\n");
		// Test changed because the Final Fantasy 12 opening somehow has the tag in *Undefined* mode, which is not in the documentation that I saw.
		if (vif1ch->chcr.MOD == NORMAL_MODE) Console.WriteLn("MFIFO mode is normal (which isn't normal here)! %x", vif1ch->chcr);
		vifMFIFOInterrupt();
		return;
	}

#ifdef PCSX2_DEVBUILD
	if (dmacRegs->ctrl.STD == STD_VIF1)
	{
		//DevCon.WriteLn("VIF Stall Control Source = %x, Drain = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3);
	}
#endif

	if ((vif1ch->chcr.MOD == NORMAL_MODE) || vif1ch->qwc > 0)   // Normal Mode
	{

		if (dmacRegs->ctrl.STD == STD_VIF1)
			Console.WriteLn("DMA Stall Control on VIF1 normal");

		if (vif1ch->chcr.DIR)  // to Memory
			vif1.dmamode = VIF_NORMAL_TO_MEM_MODE;
		else
			vif1.dmamode = VIF_NORMAL_FROM_MEM_MODE;
	}
	else
	{
		vif1.dmamode = VIF_CHAIN_MODE;
	}

	if (vif1.dmamode != VIF_NORMAL_FROM_MEM_MODE)
		vif1Regs->stat.FQC = 0x10;
	else
		vif1Regs->stat.set(min((u16)16, vif1ch->qwc) << 24);

	// Chain Mode
	vif1.done = false;
	vif1Interrupt();
}

void vif1Write32(u32 mem, u32 value)
{
	switch (mem)
	{
		case VIF1_MARK:
			VIF_LOG("VIF1_MARK write32 0x%8.8x", value);

			/* Clear mark flag in VIF1_STAT and set mark with 'value' */
			vif1Regs->stat.MRK = 0;
			vif1Regs->mark = value;
			break;

		case VIF1_FBRST:   // FBRST
			VIF_LOG("VIF1_FBRST write32 0x%8.8x", value);

			if (value & 0x1) // Reset Vif.
			{
				memzero(vif1);
				cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
				vif1ch->qwc = 0; //?
				psHu64(VIF1_FIFO) = 0;
				psHu64(VIF1_FIFO + 8) = 0;
				vif1.done = true;

				if(vif1Regs->mskpath3)
				{
					vif1Regs->mskpath3 = 0;
					gifRegs->stat.IMT = 0;
					if (gif->chcr.STR) CPU_INT(2, 4);
				}

				vif1Regs->err._u32 = 0;
				vif1.inprogress = 0;
				vif1Regs->stat.clear(VIF1_STAT_FQC | VIF1_STAT_FDR | VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS | VIF1_STAT_VPS); // FQC=0
			}

			if (value & 0x2) // Forcebreak Vif.
			{
				/* I guess we should stop the VIF dma here, but not 100% sure (linuz) */
				vif1Regs->stat.VFS = 1;
				vif1Regs->stat.VPS = 0;
				cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
				vif1.vifstalled = true;
				Console.WriteLn("vif1 force break");
			}

			if (value & 0x4) // Stop Vif.
			{
				// Not completely sure about this, can't remember what game used this, but 'draining' the VIF helped it, instead of
				//   just stoppin the VIF (linuz).
				vif1Regs->stat.VSS = 1;
				vif1Regs->stat.VPS = 0;
				cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
				vif1.vifstalled = true;
			}

			if (value & 0x8) // Cancel Vif Stall.
			{
				bool cancel = false;

				/* Cancel stall, first check if there is a stall to cancel, and then clear VIF1_STAT VSS|VFS|VIS|INT|ER0|ER1 bits */
				if (vif1Regs->stat.test(VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
				{
					cancel = true;
				}

				vif1Regs->stat.clear(VIF1_STAT_VSS | VIF1_STAT_VFS | VIF1_STAT_VIS |
						VIF1_STAT_INT | VIF1_STAT_ER0 | VIF1_STAT_ER1);

				if (cancel)
				{
					if (vif1.vifstalled)
					{
						g_vifCycles = 0;
						// loop necessary for spiderman
						switch(dmacRegs->ctrl.MFD)
						{
						    case MFD_VIF1:
                                //Console.WriteLn("MFIFO Stall");
                                CPU_INT(10, vif1ch->qwc * BIAS);
                                break;
                            
                            case NO_MFD:
                            case MFD_RESERVED:
                            case MFD_GIF: // Wonder if this should be with VIF?
                                // Gets the timing right - Flatout
                                CPU_INT(1, vif1ch->qwc * BIAS);
                                break;
						}
						
						vif1ch->chcr.STR = 1;
					}
				}
			}
			break;

		case VIF1_ERR:   // ERR
			VIF_LOG("VIF1_ERR write32 0x%8.8x", value);

			/* Set VIF1_ERR with 'value' */
			vif1Regs->err._u32 = value;
			break;

		case VIF1_STAT:   // STAT
			VIF_LOG("VIF1_STAT write32 0x%8.8x", value);

#ifdef PCSX2_DEVBUILD
			/* Only FDR bit is writable, so mask the rest */
			if ((vif1Regs->stat.FDR) ^(value & VIF1_STAT_FDR))
			{
				// different so can't be stalled
				if (vif1Regs->stat.test(VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
				{
					DevCon.WriteLn("changing dir when vif1 fifo stalled");
				}
			}
#endif

			vif1Regs->stat._u32 = (vif1Regs->stat._u32 & ~VIF1_STAT_FDR) | (value & VIF1_STAT_FDR);
			if (vif1Regs->stat.FDR)
			{
				vif1Regs->stat.FQC = 1; // Hack but it checks this is true before transfer? (fatal frame)
			}
			else
			{
				vif1ch->qwc = 0;
				vif1.vifstalled = false;
				vif1.done = true;
				vif1Regs->stat.FQC = 0;
			}
			break;

		case VIF1_MODE:
			vif1Regs->mode = value;
			break;

		case VIF1_R0:
		case VIF1_R1:
		case VIF1_R2:
		case VIF1_R3:
			pxAssert((mem&0xf) == 0);
			g_vifmask.Row1[(mem>>4) & 3] = value;
			break;

		case VIF1_C0:
		case VIF1_C1:
		case VIF1_C2:
		case VIF1_C3:
			pxAssert((mem&0xf) == 0);
			g_vifmask.Col1[(mem>>4) & 3] = value;
			break;

		default:
			Console.WriteLn("Unknown Vif1 write to %x", mem);
			psHu32(mem) = value;
			break;
	}

	/* Other registers are read-only so do nothing for them */
}

void vif1Reset()
{
	/* Reset the whole VIF, meaning the internal pcsx2 vars, and all the registers */
	memzero(vif1);
	memzero(*vif1Regs);
	SetNewMask(g_vif1Masks, g_vif1HasMask3, 0, 0xffffffff);
	
	psHu64(VIF1_FIFO) = 0;
	psHu64(VIF1_FIFO + 8) = 0;
	
	vif1Regs->stat.VPS = 0;
	vif1Regs->stat.FQC = 0; // FQC=0
	
	vif1.done = true;
	cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
}

void SaveStateBase::vif1Freeze()
{
	Freeze(vif1);

	Freeze(g_vif1HasMask3);
	Freeze(g_vif1Masks);
}
