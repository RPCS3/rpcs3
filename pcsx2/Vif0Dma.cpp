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

#include "VifDma.h"
#include "VifDma_internal.h"

#include "VUmicro.h"

__aligned16 u32 g_vif0Masks[64];
u32 g_vif0HasMask3[4] = {0};

extern int (__fastcall *Vif0TransTLB[128])(u32 *data);
extern void (*Vif0CMDTLB[75])();

vifStruct vif0;

__forceinline void vif0FLUSH()
{
	int _cycles = VU0.cycle;

	// fixme: this code should call _vu0WaitMicro instead?  I'm not sure if
	// it's purposefully ignoring ee cycles or not (see below for more)

	vu0Finish();
	g_vifCycles += (VU0.cycle - _cycles) * BIAS;
}

void vif0Init()
{
	for (u32 i = 0; i < 256; ++i)
	{
		s_maskwrite[i] = ((i & 3) == 3) || ((i & 0xc) == 0xc) || ((i & 0x30) == 0x30) || ((i & 0xc0) == 0xc0);
	}

	SetNewMask(g_vif0Masks, g_vif0HasMask3, 0, 0xffffffff);
}

static __forceinline void vif0UNPACK(u32 *data)
{
	int vifNum;

	if ((vif0Regs->cycle.wl == 0) && (vif0Regs->cycle.wl < vif0Regs->cycle.cl))
	{
		Console.WriteLn("Vif0 CL %d, WL %d", vif0Regs->cycle.cl, vif0Regs->cycle.wl);
		vif0.cmd &= ~0x7f;
		return;
	}

	vif0FLUSH();

	vif0.tag.addr = (vif0Regs->code & 0xff) << 4;
	vif0.usn = (vif0Regs->code >> 14) & 0x1;
	vifNum = (vif0Regs->code >> 16) & 0xff;
	if (vifNum == 0) vifNum = 256;
	vif0Regs->num = vifNum;

	if (vif0Regs->cycle.wl <= vif0Regs->cycle.cl)
	{
		vif0.tag.size = ((vifNum * VIFfuncTable[ vif0.cmd & 0xf ].gsize) + 3) >> 2;
	}
	else
	{
		int n = vif0Regs->cycle.cl * (vifNum / vif0Regs->cycle.wl) +
		        _limit(vifNum % vif0Regs->cycle.wl, vif0Regs->cycle.cl);

		vif0.tag.size = ((n * VIFfuncTable[ vif0.cmd & 0xf ].gsize) + 3) >> 2;
	}

	vif0.cl = 0;
	vif0.tag.cmd = vif0.cmd;
	vif0Regs->offset = 0;
}

static __forceinline void vif0mpgTransfer(u32 addr, u32 *data, int size)
{
	/*	Console.WriteLn("vif0mpgTransfer addr=%x; size=%x", addr, size);
		{
			FILE *f = fopen("vu1.raw", "wb");
			fwrite(data, 1, size*4, f);
			fclose(f);
		}*/
	if (memcmp(VU0.Micro + addr, data, size << 2))
	{
		CpuVU0.Clear(addr, size << 2); // Clear before writing! :/ (cottonvibes)
		memcpy_fast(VU0.Micro + addr, data, size << 2);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif0 Data Transfer Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int __fastcall Vif0TransNull(u32 *data)  // Shouldnt go here
{
	Console.WriteLn("VIF0 Shouldn't go here CMD = %x", vif0Regs->code);
	vif0.cmd = 0;
	return 0;
}

static int __fastcall Vif0TransSTMask(u32 *data)  // STMASK
{
	SetNewMask(g_vif0Masks, g_vif0HasMask3, data[0], vif0Regs->mask);
	vif0Regs->mask = data[0];
	VIF_LOG("STMASK == %x", vif0Regs->mask);

	vif0.tag.size = 0;
	vif0.cmd = 0;
	return 1;
}

static int __fastcall Vif0TransSTRow(u32 *data)  // STROW
{
	int ret;

	u32* pmem = &vif0Regs->r0 + (vif0.tag.addr << 2);
	u32* pmem2 = g_vifmask.Row0 + vif0.tag.addr;
	pxAssert(vif0.tag.addr < 4);
	ret = min(4 - vif0.tag.addr, vif0.vifpacketsize);
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

			jNO_DEFAULT
	}

	vif0.tag.addr += ret;
	vif0.tag.size -= ret;
	if (vif0.tag.size == 0) vif0.cmd = 0;

	return ret;
}

static int __fastcall Vif0TransSTCol(u32 *data)  // STCOL
{
	int ret;

	u32* pmem = &vif0Regs->c0 + (vif0.tag.addr << 2);
	u32* pmem2 = g_vifmask.Col0 + vif0.tag.addr;
	ret = min(4 - vif0.tag.addr, vif0.vifpacketsize);

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

			jNO_DEFAULT
	}

	vif0.tag.addr += ret;
	vif0.tag.size -= ret;
	if (vif0.tag.size == 0) vif0.cmd = 0;
	return ret;
}

static int __fastcall Vif0TransMPG(u32 *data)  // MPG
{
	if (vif0.vifpacketsize < vif0.tag.size)
	{
		if((vif0.tag.addr + vif0.vifpacketsize) > 0x1000) DevCon.Warning("Vif0 MPG Split Overflow");

		vif0mpgTransfer(vif0.tag.addr, data, vif0.vifpacketsize);
		vif0.tag.addr += vif0.vifpacketsize << 2;
		vif0.tag.size -= vif0.vifpacketsize;

		return vif0.vifpacketsize;
	}
	else
	{
		int ret;

		if((vif0.tag.addr + vif0.tag.size) > 0x1000) DevCon.Warning("Vif0 MPG Overflow");

		vif0mpgTransfer(vif0.tag.addr, data, vif0.tag.size);
		ret = vif0.tag.size;
		vif0.tag.size = 0;
		vif0.cmd = 0;

		return ret;
	}
}

static int __fastcall Vif0TransUnpack(u32 *data)	// UNPACK
{
	int ret;

    XMMRegisters::Freeze();
	if (vif0.vifpacketsize < vif0.tag.size)
	{
		if(vif0Regs->offset != 0 || vif0.cl != 0)
		{
			ret = vif0.tag.size;
			vif0.tag.size -= vif0.vifpacketsize - VIFalign<0>(data, &vif0.tag, vif0.vifpacketsize);
			ret = ret - vif0.tag.size;
			data += ret;

			if(vif0.vifpacketsize > 0) VIFunpack<0>(data, &vif0.tag, vif0.vifpacketsize - ret);

			ProcessMemSkip<0>((vif0.vifpacketsize - ret) << 2, (vif0.cmd & 0xf));
			vif0.tag.size -= (vif0.vifpacketsize - ret);
            XMMRegisters::Thaw();

			return vif0.vifpacketsize;
		}
		/* size is less that the total size, transfer is 'in pieces' */
		VIFunpack<0>(data, &vif0.tag, vif0.vifpacketsize);

		ProcessMemSkip<0>(vif0.vifpacketsize << 2, (vif0.cmd & 0xf));

		ret = vif0.vifpacketsize;
		vif0.tag.size -= ret;
	}
	else
	{
		/* we got all the data, transfer it fully */
		ret = vif0.tag.size;

		//Align data after a split transfer first
		if ((vif0Regs->offset != 0) || (vif0.cl != 0))
		{
			vif0.tag.size = VIFalign<0>(data, &vif0.tag, vif0.tag.size);
			data += ret - vif0.tag.size;
			if(vif0.tag.size > 0) VIFunpack<0>(data, &vif0.tag, vif0.tag.size);
		}
		else
		{
			VIFunpack<0>(data, &vif0.tag, vif0.tag.size);
		}

		vif0.tag.size = 0;
		vif0.cmd = 0;
	}

    XMMRegisters::Thaw();
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif0 CMD Base Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void Vif0CMDNop()  // NOP
{
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDSTCycl()  // STCYCL
{
	vif0Regs->cycle.cl = (u8)vif0Regs->code;
	vif0Regs->cycle.wl = (u8)(vif0Regs->code >> 8);
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDITop()  // ITOP
{
	vif0Regs->itops = vif0Regs->code & 0x3ff;
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDSTMod()  // STMOD
{
	vif0Regs->mode = vif0Regs->code & 0x3;
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDMark()  // MARK
{
	vif0Regs->mark = (u16)vif0Regs->code;
	vif0Regs->stat.MRK = true;
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDFlushE()  // FLUSHE
{
	vif0FLUSH();
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDMSCALF()  //MSCAL/F
{
	vuExecMicro<0>((u16)(vif0Regs->code) << 3);
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDMSCNT()  // MSCNT
{
	vuExecMicro<0>(-1);
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDSTMask()  // STMASK
{
	vif0.tag.size = 1;
}

static void Vif0CMDSTRowCol() // STROW / STCOL
{
	vif0.tag.addr = 0;
	vif0.tag.size = 4;
}

static void Vif0CMDMPGTransfer()  // MPG
{
	int vifNum;
	vif0FLUSH();
	vifNum = (u8)(vif0Regs->code >> 16);
	if (vifNum == 0) vifNum = 256;
	vif0.tag.addr = (u16)((vif0Regs->code) << 3) & 0xfff;
	vif0.tag.size = vifNum * 2;
}

static void Vif0CMDNull()  // invalid opcode
{
	// if ME1, then force the vif to interrupt
	if (!(vif0Regs->err.ME1))    //Ignore vifcode and tag mismatch error
	{
		Console.WriteLn("UNKNOWN VifCmd: %x", vif0.cmd);
		vif0Regs->stat.ER1 = true;
		vif0.irq++;
	}
	vif0.cmd &= ~0x7f;
}

bool VIF0transfer(u32 *data, int size, bool istag)
{
	int ret;
	int transferred = vif0.vifstalled ? vif0.irqoffset : 0; // irqoffset necessary to add up the right qws, or else will spin (spiderman)
	VIF_LOG("VIF0transfer: size %x (vif0.cmd %x)", size, vif0.cmd);

	vif0.stallontag = false;
	vif0.vifstalled = false;
	vif0.vifpacketsize = size;

	while (vif0.vifpacketsize > 0)
	{
		if (vif0.cmd)
		{
			vif0Regs->stat.VPS = VPS_TRANSFERRING; //Decompression has started

			ret = Vif0TransTLB[(vif0.cmd & 0x7f)](data);
			data += ret;
			vif0.vifpacketsize -= ret;
			if (vif0.cmd == 0) vif0Regs->stat.VPS = VPS_IDLE; //We are once again waiting for a new vifcode as the command has cleared
			continue;
		}

		if (vif0.tag.size != 0) Console.WriteLn("no vif0 cmd but tag size is left last cmd read %x", vif0Regs->code);

		// if interrupt and new cmd is NOT MARK
		if (vif0.irq) break;

		vif0.cmd = (data[0] >> 24);
		vif0Regs->code = data[0];

		vif0Regs->stat.VPS |= VPS_DECODING; //We need to set these (Onimusha needs it)

		if ((vif0.cmd & 0x60) == 0x60)
		{
			vif0UNPACK(data);
		}
		else
		{
			VIF_LOG("VIFtransfer: cmd %x, num %x, imm %x, size %x", vif0.cmd, (data[0] >> 16) & 0xff, data[0] & 0xffff, size);

			if ((vif0.cmd & 0x7f) > 0x4A)
			{
				if (!(vif0Regs->err.ME1))    //Ignore vifcode and tag mismatch error
				{
					Console.WriteLn("UNKNOWN VifCmd: %x", vif0.cmd);
					vif0Regs->stat.ER1 = true;
					vif0.irq++;
				}
				vif0.cmd = 0;
			}
			else
			{
				Vif0CMDTLB[(vif0.cmd & 0x7f)]();
			}
		}
		++data;
		--vif0.vifpacketsize;

		if ((vif0.cmd & 0x80))
		{
			vif0.cmd &= 0x7f;

			if (!(vif0Regs->err.MII)) //i bit on vifcode and not masked by VIF0_ERR
			{
				VIF_LOG("Interrupt on VIFcmd: %x (INTC_MASK = %x)", vif0.cmd, psHu32(INTC_MASK));

				++vif0.irq;

				if (istag && vif0.tag.size <= vif0.vifpacketsize) vif0.stallontag = true;

				if (vif0.tag.size == 0) break;
			}
		}
	} //End of Transfer loop

	transferred += size - vif0.vifpacketsize;
	g_vifCycles += (transferred >> 2) * BIAS; /* guessing */
	// use tag.size because some game doesn't like .cmd

	if (vif0.irq && (vif0.tag.size == 0))
	{
		vif0.vifstalled = true;

		if (((vif0Regs->code >> 24) & 0x7f) != 0x7) vif0Regs->stat.VIS = true;
		//else Console.WriteLn("VIF0 IRQ on MARK");

		// spiderman doesn't break on qw boundaries
		vif0.irqoffset = transferred % 4; // cannot lose the offset

		if (!istag)
		{
			transferred = transferred >> 2;
			vif0ch->madr += (transferred << 4);
			vif0ch->qwc -= transferred;
		}
		//else Console.WriteLn("Stall on vif0, FromSPR = %x, Vif0MADR = %x Sif0MADR = %x STADR = %x", psHu32(0x1000d010), vif0ch->madr, psHu32(0x1000c010), psHu32(DMAC_STADR));
		return false;
	}

	vif0Regs->stat.VPS = VPS_IDLE; //Vif goes idle as the stall happened between commands;
	if (vif0.cmd) vif0Regs->stat.VPS |= VPS_WAITING;  //Otherwise we wait for the data

	if (!istag)
	{
		transferred = transferred >> 2;
		vif0ch->madr += (transferred << 4);
		vif0ch->qwc -= transferred;
	}

	return true;
}

bool  _VIF0chain()
{
	u32 *pMem;

	if ((vif0ch->qwc == 0) && !vif0.vifstalled) return true;

	pMem = (u32*)dmaGetAddr(vif0ch->madr);
	if (pMem == NULL)
	{
		vif0.cmd = 0;
		vif0.tag.size = 0;
		vif0ch->qwc = 0;
		return true;
	}

	if (vif0.vifstalled)
		return VIF0transfer(pMem + vif0.irqoffset, vif0ch->qwc * 4 - vif0.irqoffset, false);
	else
		return VIF0transfer(pMem, vif0ch->qwc * 4, false);
}

bool _chainVIF0()
{
    tDMA_TAG *ptag;
    
	ptag = (tDMA_TAG*)dmaGetAddr(vif0ch->tadr); //Set memory pointer to TADR

	if (!(vif0ch->transfer("Vif0 Tag", ptag))) return false;

	vif0ch->madr = ptag[1].ADDR;		// MADR = ADDR field
    g_vifCycles += 1; 				// Increase the QW read for the tag

	VIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx",
	        ptag[1]._u32, ptag[0]._u32, vif0ch->qwc, ptag->ID, vif0ch->madr, vif0ch->tadr);

	// Transfer dma tag if tte is set
	if (vif0ch->chcr.TTE)
	{
	    bool ret;

		if (vif0.vifstalled)
			ret = VIF0transfer((u32*)ptag + (2 + vif0.irqoffset), 2 - vif0.irqoffset, true);  //Transfer Tag on stall
		else
			ret = VIF0transfer((u32*)ptag + 2, 2, true);  //Transfer Tag

		if (!(ret)) return false;        //IRQ set by VIFTransfer
	}

	vif0.done |= hwDmacSrcChainWithStack(vif0ch, ptag->ID);

	VIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx",
	        ptag[1]._u32, ptag[0]._u32, vif0ch->qwc, ptag->ID, vif0ch->madr, vif0ch->tadr);

	_VIF0chain();											   //Transfers the data set by the switch

	if (vif0ch->chcr.TIE && ptag->IRQ)  //Check TIE bit of CHCR and IRQ bit of tag
	{
		VIF_LOG("dmaIrq Set\n");
		vif0.done = true; //End Transfer
	}
	
	return vif0.done;
}

void vif0Interrupt()
{
	g_vifCycles = 0; //Reset the cycle count, Wouldn't reset on stall if put lower down.
	VIF_LOG("vif0Interrupt: %8.8x", cpuRegs.cycle);

	if (vif0.irq && (vif0.tag.size == 0))
	{
		vif0Regs->stat.INT = true;
		hwIntcIrq(VIF0intc);
		--vif0.irq;

		if (vif0Regs->stat.test(VIF0_STAT_VSS | VIF0_STAT_VIS | VIF0_STAT_VFS))
		{
			vif0Regs->stat.FQC = 0;
			vif0ch->chcr.STR = false;
			return;
		}

		if (vif0ch->qwc > 0 || vif0.irqoffset > 0)
		{
			if (vif0.stallontag)
				_chainVIF0();
			else
				_VIF0chain();

			CPU_INT(0, g_vifCycles);
			return;
		}
	}

	if (!vif0ch->chcr.STR) Console.WriteLn("Vif0 running when CHCR = %x", vif0ch->chcr._u32);

	if ((vif0ch->chcr.MOD == CHAIN_MODE) && (!vif0.done) && (!vif0.vifstalled))
	{

		if (!(dmacRegs->ctrl.DMAE))
		{
			Console.WriteLn("vif0 dma masked");
			return;
		}

		if (vif0ch->qwc > 0)
			_VIF0chain();
		else
			_chainVIF0();

		CPU_INT(0, g_vifCycles);
		return;
	}

	if (vif0ch->qwc > 0) Console.WriteLn("VIF0 Ending with QWC left");
	if (vif0.cmd != 0) Console.WriteLn("vif0.cmd still set %x", vif0.cmd);

	vif0ch->chcr.STR = false;
	hwDmacIrq(DMAC_VIF0);
	vif0Regs->stat.FQC = 0;
}

//  Vif0 Data Transfer Table
int (__fastcall *Vif0TransTLB[128])(u32 *data) =
{
	Vif0TransNull	 , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x7*/
	Vif0TransNull	 , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0xF*/
	Vif0TransNull	 , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x17*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x1F*/
	Vif0TransSTMask  , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x27*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x2F*/
	Vif0TransSTRow	 , Vif0TransSTCol	, Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x37*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x3F*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x47*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransMPG	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x4F*/
	Vif0TransNull	 , Vif0TransNull	, Vif0TransNull	  , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull	, Vif0TransNull	  , Vif0TransNull   , /*0x57*/
	Vif0TransNull	 , Vif0TransNull	, Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x5F*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransNull   , /*0x67*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , /*0x6F*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransNull   , /*0x77*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransNull   , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack   /*0x7F*/
};

// Vif0 CMD Table
void (*Vif0CMDTLB[75])() =
{
	Vif0CMDNop	   , Vif0CMDSTCycl  , Vif0CMDNull		, Vif0CMDNull , Vif0CMDITop  , Vif0CMDSTMod , Vif0CMDNull, Vif0CMDMark , /*0x7*/
	Vif0CMDNull	   , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0xF*/
	Vif0CMDFlushE   , Vif0CMDNull   , Vif0CMDNull		, Vif0CMDNull, Vif0CMDMSCALF, Vif0CMDMSCALF, Vif0CMDNull	, Vif0CMDMSCNT, /*0x17*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0x1F*/
	Vif0CMDSTMask  , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull	, Vif0CMDNull , /*0x27*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull	, Vif0CMDNull , /*0x2F*/
	Vif0CMDSTRowCol, Vif0CMDSTRowCol, Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull	, Vif0CMDNull , /*0x37*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0x3F*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0x47*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDMPGTransfer
};

void dmaVIF0()
{
	VIF_LOG("dmaVIF0 chcr = %lx, madr = %lx, qwc  = %lx\n"
	        "        tadr = %lx, asr0 = %lx, asr1 = %lx\n",
	        vif0ch->chcr._u32, vif0ch->madr, vif0ch->qwc,
	        vif0ch->tadr, vif0ch->asr0, vif0ch->asr1);

	g_vifCycles = 0;

	vif0Regs->stat.FQC = 0x8; // FQC=8

	if (!(vif0ch->chcr.MOD & 0x1) || vif0ch->qwc > 0)   // Normal Mode
	{
		if (!_VIF0chain())
		{
			Console.WriteLn(L"Stall on normal vif0 " + vif0Regs->stat.desc());

			vif0.vifstalled = true;
			return;
		}

		vif0.done = true;
		CPU_INT(0, g_vifCycles);
		return;
	}

	// Chain Mode
	vif0.done = false;
	CPU_INT(0, 0);
}

void vif0Write32(u32 mem, u32 value)
{
	switch (mem)
	{
		case VIF0_MARK:
			VIF_LOG("VIF0_MARK write32 0x%8.8x", value);

			/* Clear mark flag in VIF0_STAT and set mark with 'value' */
			vif0Regs->stat.MRK = false;
			vif0Regs->mark = value;
			break;

		case VIF0_FBRST:
			VIF_LOG("VIF0_FBRST write32 0x%8.8x", value);

			if (value & 0x1) // Reset Vif.
			{
				//Console.WriteLn("Vif0 Reset %x", vif0Regs->stat._u32);

				memzero(vif0);
				vif0ch->qwc = 0; //?
				cpuRegs.interrupt &= ~1; //Stop all vif0 DMA's
				psHu64(VIF0_FIFO) = 0;
				psHu64(VIF0_FIFO + 8) = 0;
				vif0.done = true;
				vif0Regs->err.reset();
				vif0Regs->stat.clear_flags(VIF0_STAT_FQC | VIF0_STAT_INT | VIF0_STAT_VSS | VIF0_STAT_VIS | VIF0_STAT_VFS | VIF0_STAT_VPS); // FQC=0
			}

			if (value & 0x2) // Forcebreak Vif,
			{
				/* I guess we should stop the VIF dma here, but not 100% sure (linuz) */
				cpuRegs.interrupt &= ~1; //Stop all vif0 DMA's
				vif0Regs->stat.VFS = true;
				vif0Regs->stat.VPS = VPS_IDLE;
				vif0.vifstalled = true;
				Console.WriteLn("vif0 force break");
			}

			if (value & 0x4) // Stop Vif.
			{
				// Not completely sure about this, can't remember what game, used this, but 'draining' the VIF helped it, instead of
				//  just stoppin the VIF (linuz).
				vif0Regs->stat.VSS = true;
				vif0Regs->stat.VPS = VPS_IDLE;
				vif0.vifstalled = true;
			}

			if (value & 0x8) // Cancel Vif Stall.
			{
				bool cancel = false;

				/* Cancel stall, first check if there is a stall to cancel, and then clear VIF0_STAT VSS|VFS|VIS|INT|ER0|ER1 bits */
				if (vif0Regs->stat.test(VIF0_STAT_VSS | VIF0_STAT_VIS | VIF0_STAT_VFS))
					cancel = true;

				vif0Regs->stat.clear_flags(VIF0_STAT_VSS | VIF0_STAT_VFS | VIF0_STAT_VIS |
						    VIF0_STAT_INT | VIF0_STAT_ER0 | VIF0_STAT_ER1);
				if (cancel)
				{
					if (vif0.vifstalled)
					{
						g_vifCycles = 0;

						// loop necessary for spiderman
						if (vif0.stallontag)
							_chainVIF0();
						else
							_VIF0chain();

						vif0ch->chcr.STR = true;
						CPU_INT(0, g_vifCycles); // Gets the timing right - Flatout
					}
				}
			}
			break;

		case VIF0_ERR:
			// ERR
			VIF_LOG("VIF0_ERR write32 0x%8.8x", value);

			/* Set VIF0_ERR with 'value' */
			vif0Regs->err.write(value);
			break;

		case VIF0_R0:
		case VIF0_R1:
		case VIF0_R2:
		case VIF0_R3:
			pxAssert((mem&0xf) == 0);
			g_vifmask.Row0[(mem>>4) & 3] = value;
			break;

		case VIF0_C0:
		case VIF0_C1:
		case VIF0_C2:
		case VIF0_C3:
			pxAssert((mem&0xf) == 0);
			g_vifmask.Col0[(mem>>4) & 3] = value;
			break;

		default:
			Console.WriteLn("Unknown Vif0 write to %x", mem);
			psHu32(mem) = value;
			break;
	}
	/* Other registers are read-only so do nothing for them */
}

void vif0Reset()
{
	/* Reset the whole VIF, meaning the internal pcsx2 vars and all the registers */
	memzero(vif0);
	memzero(*vif0Regs);
	SetNewMask(g_vif0Masks, g_vif0HasMask3, 0, 0xffffffff);

	psHu64(VIF0_FIFO) = 0;
	psHu64(VIF0_FIFO + 8) = 0;
	
	vif0Regs->stat.VPS = VPS_IDLE;
	vif0Regs->stat.FQC = 0;

	vif0.done = true;
}

void SaveStateBase::vif0Freeze()
{
	FreezeTag("VIFdma");

	// Dunno if this one is needed, but whatever, it's small. :)
	Freeze(g_vifCycles);

	// mask settings for VIF0 and VIF1
	Freeze(g_vifmask);

	Freeze(vif0);
	Freeze(g_vif0HasMask3);
	Freeze(g_vif0Masks);
}
