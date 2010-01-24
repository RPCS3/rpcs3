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
#include "newVif.h"

extern void (*Vif1CMDTLB[82])();
extern int (__fastcall *Vif1TransTLB[128])(u32 *data);

Path3Modes Path3progress = STOPPED_MODE;
vifStruct vif1;

__forceinline void vif1FLUSH()
{
	int _cycles = VU1.cycle;

	// fixme: Same as above, is this a "stalling" offense?  I think the cycles should
	// be added to cpuRegs.cycle instead of g_vifCycles, but not sure (air)

	if (VU0.VI[REG_VPU_STAT].UL & 0x100)
	{
		do
		{
			CpuVU1->ExecuteBlock();
		}
		while (VU0.VI[REG_VPU_STAT].UL & 0x100);

		g_vifCycles += (VU1.cycle - _cycles) * BIAS;
	}
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

bool VIF1transfer(u32 *data, int size, bool istag)
{
	int ret;
	int transferred = vif1.vifstalled ? vif1.irqoffset : 0; // irqoffset necessary to add up the right qws, or else will spin (spiderman)
	
	// Fixme:
	// VIF1 chain mode seems to be pretty buggy. Keep it in synch with a INTC (page 28, ee user manual) fixes Transformers.
	// Test chain mode with Harry Potter, Def Jam Fight for New York (+many others, look for flickering graphics).
	// Mentioned games break by doing this, but they barely work anyway 
	// if ( vif1.vifstalled ) hwIntcIrq(VIF1intc);
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
					vif1Regs->stat.ER1 = true;
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
				// Fixme: This seems to be an important part of the VIF1 chain mode issue (see Fixme above :p )
				//Console.Warning("Interrupt on VIFcmd: %x (INTC_MASK = %x)", vif1.cmd, psHu32(INTC_MASK));
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

		if (((vif1Regs->code >> 24) & 0x7f) != 0x7) vif1Regs->stat.VIS = true; // Note: commenting this out fixes WALL-E

		if (vif1ch->qwc == 0 && (vif1.irqoffset == 0 || istag == 1)) vif1.inprogress &= ~0x1;

		// spiderman doesn't break on qw boundaries
		if (istag) return false;

		transferred = transferred >> 2;
		vif1ch->madr += (transferred << 4);
		vif1ch->qwc -= transferred;

		if ((vif1ch->qwc == 0) && (vif1.irqoffset == 0)) vif1.inprogress = 0;
		//Console.WriteLn("Stall on vif1, FromSPR = %x, Vif1MADR = %x Sif0MADR = %x STADR = %x", psHu32(0x1000d010), vif1ch->madr, psHu32(0x1000c010), psHu32(DMAC_STADR));
		return false;
	}

	vif1Regs->stat.VPS = VPS_IDLE; //Vif goes idle as the stall happened between commands;
	if (vif1.cmd) vif1Regs->stat.VPS = VPS_WAITING;  //Otherwise we wait for the data

	if (!istag)
	{
		transferred = transferred >> 2;
		vif1ch->madr += (transferred << 4);
		vif1ch->qwc -= transferred;
	}

	if (vif1Regs->stat.VGW) vif1.vifstalled = true;

	if (vif1ch->qwc == 0 && (vif1.irqoffset == 0 || istag == 1)) vif1.inprogress &= ~0x1;

	return (!(vif1.vifstalled));
}

void vif1TransferFromMemory()
{
	int size;
	u64* pMem = (u64*)dmaGetAddr(vif1ch->madr);

	// VIF from gsMemory
	if (pMem == NULL)  						//Is vif0ptag empty?
	{
		Console.WriteLn("Vif1 Tag BUSERR");
		dmacRegs->stat.BEIS = true;      //Bus Error
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
				GetMTGS().WaitGS();
				GSreadFIFO(&psHu64(VIF1_FIFO));
			}
			pMem[0] = psHu64(VIF1_FIFO);
			pMem[1] = psHu64(VIF1_FIFO + 8);
			pMem += 2;
		}
	}
	else
	{
		GetMTGS().WaitGS();
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

bool _VIF1chain()
{
	u32 *pMem;

	if (vif1ch->qwc == 0)
	{
		vif1.inprogress = 0;
		return true;
	}

	if (vif1.dmamode == VIF_NORMAL_FROM_MEM_MODE)
	{
		vif1TransferFromMemory();
		vif1.inprogress = 0;
		return true;
	}

	pMem = (u32*)dmaGetAddr(vif1ch->madr);
	if (pMem == NULL)
	{
		vif1.cmd = 0;
		vif1.tag.size = 0;
		vif1ch->qwc = 0;
		return true;
	}

	VIF_LOG("VIF1chain size=%d, madr=%lx, tadr=%lx",
	        vif1ch->qwc, vif1ch->madr, vif1ch->tadr);

	if (vif1.vifstalled)
		return VIF1transfer(pMem + vif1.irqoffset, vif1ch->qwc * 4 - vif1.irqoffset, false);
	else
		return VIF1transfer(pMem, vif1ch->qwc * 4, false);
}

bool _chainVIF1()
{
	return vif1.done; // Return Done
}

__forceinline void vif1SetupTransfer()
{
    tDMA_TAG *ptag;
    
	switch (vif1.dmamode)
	{
		case VIF_NORMAL_TO_MEM_MODE:
		case VIF_NORMAL_FROM_MEM_MODE:
			vif1.inprogress = 1;
			vif1.done = true;
			g_vifCycles = 2;
			break;

		case VIF_CHAIN_MODE:
			ptag = dmaGetAddr(vif1ch->tadr); //Set memory pointer to TADR

			if (!(vif1ch->transfer("Vif1 Tag", ptag))) return;

			vif1ch->madr = ptag[1]._u32;            //MADR = ADDR field + SPR
			g_vifCycles += 1; // Add 1 g_vifCycles from the QW read for the tag

			// Transfer dma tag if tte is set

			VIF_LOG("VIF1 Tag %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx\n",
			        ptag[1]._u32, ptag[0]._u32, vif1ch->qwc, ptag->ID, vif1ch->madr, vif1ch->tadr);

			if (!vif1.done && ((dmacRegs->ctrl.STD == STD_VIF1) && (ptag->ID == TAG_REFS)))   // STD == VIF1
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
			    bool ret;

				if (vif1.vifstalled)
					ret = VIF1transfer((u32*)ptag + (2 + vif1.irqoffset), 2 - vif1.irqoffset, true);  //Transfer Tag on stall
				else
					ret = VIF1transfer((u32*)ptag + 2, 2, true);  //Transfer Tag

				if ((ret == false) && vif1.irqoffset < 2)
				{
					vif1.inprogress = 0; //Better clear this so it has to do it again (Jak 1)
					return;       //There has been an error or an interrupt
				}
			}

			vif1.irqoffset = 0;
			vif1.done |= hwDmacSrcChainWithStack(vif1ch, ptag->ID);

			//Check TIE bit of CHCR and IRQ bit of tag
			if (vif1ch->chcr.TIE && ptag->IRQ)
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
		    vif1Regs->stat.VGW = false;
		}
	}

	if (!(vif1ch->chcr.STR)) Console.WriteLn("Vif1 running when CHCR == %x", vif1ch->chcr._u32);

	if (vif1.irq && vif1.tag.size == 0)
	{
		vif1Regs->stat.INT = true;
		hwIntcIrq(VIF1intc);
		--vif1.irq;
		if (vif1Regs->stat.test(VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
		{
			vif1Regs->stat.FQC = 0;

			// One game doesn't like vif stalling at end, can't remember what. Spiderman isn't keen on it tho
			vif1ch->chcr.STR = false;
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
		CPU_INT(1, /*g_vifCycles*/ VifCycleVoodoo);
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

		CPU_INT(1, /*g_vifCycles*/ VifCycleVoodoo);
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

	vif1Regs->stat.VPS = VPS_IDLE; //Vif goes idle as the stall happened between commands;
	vif1ch->chcr.STR = false;
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
	        vif1ch->chcr._u32, vif1ch->madr, vif1ch->qwc,
	        vif1ch->tadr, vif1ch->asr0, vif1ch->asr1);

	g_vifCycles = 0;
	vif1.inprogress = 0;

	if (dmacRegs->ctrl.MFD == MFD_VIF1)   // VIF MFIFO
	{
		//Console.WriteLn("VIFMFIFO\n");
		// Test changed because the Final Fantasy 12 opening somehow has the tag in *Undefined* mode, which is not in the documentation that I saw.
		if (vif1ch->chcr.MOD == NORMAL_MODE) Console.WriteLn("MFIFO mode is normal (which isn't normal here)! %x", vif1ch->chcr._u32);
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
		vif1Regs->stat.FQC = min((u16)0x10, vif1ch->qwc);

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
			vif1Regs->stat.MRK = false;
			vif1Regs->mark = value;
			break;

		case VIF1_FBRST:   // FBRST
			VIF_LOG("VIF1_FBRST write32 0x%8.8x", value);

			if (FBRST(value).RST) // Reset Vif.
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
					gifRegs->stat.IMT = false;
					if (gif->chcr.STR) CPU_INT(2, 4);
				}

				vif1Regs->err.reset();
				vif1.inprogress = 0;
				vif1Regs->stat.FQC = 0;
				vif1Regs->stat.clear_flags(VIF1_STAT_FDR | VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS | VIF1_STAT_VPS);
			}

			if (FBRST(value).FBK) // Forcebreak Vif.
			{
				/* I guess we should stop the VIF dma here, but not 100% sure (linuz) */
				vif1Regs->stat.VFS = true;
				vif1Regs->stat.VPS = VPS_IDLE;
				cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
				vif1.vifstalled = true;
				Console.WriteLn("vif1 force break");
			}

			if (FBRST(value).STP) // Stop Vif.
			{
				// Not completely sure about this, can't remember what game used this, but 'draining' the VIF helped it, instead of
				//   just stoppin the VIF (linuz).
				vif1Regs->stat.VSS = true;
				vif1Regs->stat.VPS = VPS_IDLE;
				cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
				vif1.vifstalled = true;
			}

			if (FBRST(value).STC) // Cancel Vif Stall.
			{
				bool cancel = false;

				/* Cancel stall, first check if there is a stall to cancel, and then clear VIF1_STAT VSS|VFS|VIS|INT|ER0|ER1 bits */
				if (vif1Regs->stat.test(VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
				{
					cancel = true;
				}

				vif1Regs->stat.clear_flags(VIF1_STAT_VSS | VIF1_STAT_VFS | VIF1_STAT_VIS |
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
						
						vif1ch->chcr.STR = true;
					}
				}
			}
			break;

		case VIF1_ERR:   // ERR
			VIF_LOG("VIF1_ERR write32 0x%8.8x", value);

			/* Set VIF1_ERR with 'value' */
			vif1Regs->err.write(value);
			break;

		case VIF1_STAT:   // STAT
			VIF_LOG("VIF1_STAT write32 0x%8.8x", value);

#ifdef PCSX2_DEVBUILD
			/* Only FDR bit is writable, so mask the rest */
			if ((vif1Regs->stat.FDR) ^ ((tVIF_STAT&)value).FDR)
			{
				// different so can't be stalled
				if (vif1Regs->stat.test(VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
				{
					DevCon.WriteLn("changing dir when vif1 fifo stalled");
				}
			}
#endif

			vif1Regs->stat.FDR = VIF_STAT(value).FDR;
			
			if (vif1Regs->stat.FDR) // Vif transferring to memory.
			{
			    // Hack but it checks this is true before transfer? (fatal frame)
				vif1Regs->stat.FQC = 0x1;
			}
			else // Memory transferring to Vif.
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
			pxAssume((mem&0xf) == 0);
			g_vifmask.Row1[(mem>>4) & 3] = value;
			break;

		case VIF1_C0:
		case VIF1_C1:
		case VIF1_C2:
		case VIF1_C3:
			pxAssume((mem&0xf) == 0);
			g_vifmask.Col1[(mem>>4) & 3] = value;
			break;

		default:
			Console.WriteLn("Unknown Vif1 write to %x", mem);
			psHu32(mem) = value;
			break;
	}

	/* Other registers are read-only so do nothing for them */
}
