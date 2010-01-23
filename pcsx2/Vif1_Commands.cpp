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
#include "Vif_Dma.h"
#include "GS.h"
#include "Gif.h"
#include "VUmicro.h"

//------------------------------------------------------------------
// Vif1 Data Transfer Commands
//------------------------------------------------------------------

static int __fastcall Vif1TransNull(u32 *data)  // Shouldnt go here
{
	Console.WriteLn("VIF1 Shouldn't go here CMD = %x", vif1Regs->code);
	vif1.cmd = 0;
	return 0;
}

static int __fastcall Vif1TransSTMask(u32 *data)  // STMASK
{
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
	pxAssume(vif1.tag.addr < 4);
	ret = min(4 - vif1.tag.addr, vif1.vifpacketsize);
	pxAssume(ret > 0);

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

static __forceinline void vif1mpgTransfer(u32 addr, u32 *data, int size)
{
	pxAssume(VU1.Micro > 0);
	if (memcmp(VU1.Micro + addr, data, size << 2))
	{
		CpuVU1->Clear(addr, size << 2); // Clear before writing! :/
		memcpy_fast(VU1.Micro + addr, data, size << 2);
	}
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
static __aligned16 u32 splittransfer[4];
static u32 splitptr = 0;

static int __fastcall Vif1TransDirectHL(u32 *data)
{
	int ret = 0;

	if ((vif1.cmd & 0x7f) == 0x51)
	{
		if (gif->chcr.STR && (!vif1Regs->mskpath3 && (Path3progress == IMAGE_MODE))) //PATH3 is in image mode, so wait for end of transfer
		{
			vif1Regs->stat.VGW = true;
			return 0;
		}
	}

	gifRegs->stat.APATH |= GIF_APATH2;
	gifRegs->stat.OPH = true;
	
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
		gifRegs->stat.clear_flags(GIF_STAT_APATH2 | GIF_STAT_OPH);
		ret = vif1.tag.size;
		vif1.tag.size = 0;
		vif1.cmd = 0;
	}

	//TODO: ret is guaranteed to be qword aligned ?

	Registers::Freeze();

	// Round ret up, just in case it's not 128bit aligned.
	const uint count = GetMTGS().PrepDataPacket(GIF_PATH_2, data, (ret + 3) >> 2);
	memcpy_fast(GetMTGS().GetDataPacketPtr(), data, count << 4);
	GetMTGS().SendDataPacket();

	Registers::Thaw();

	return ret;
}
static int  __fastcall Vif1TransUnpack(u32 *data)
{
	return nVifUnpack(1, (u8*)data);
}

//------------------------------------------------------------------
// Vif1 Data Transfer Table
//------------------------------------------------------------------

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
