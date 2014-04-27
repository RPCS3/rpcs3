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
#include "Hardware.h"
#include "MTVU.h"

#include "IPU/IPUdma.h"
#include "ps2/HwInternal.h"

bool DMACh::transfer(const char *s, tDMA_TAG* ptag)
{
	if (ptag == NULL)  					 // Is ptag empty?
	{
		throwBusError(s);
		return false;
	}
    chcrTransfer(ptag);

    qwcTransfer(ptag);
    return true;
}

void DMACh::unsafeTransfer(tDMA_TAG* ptag)
{
    chcrTransfer(ptag);
    qwcTransfer(ptag);
}

tDMA_TAG *DMACh::getAddr(u32 addr, u32 num, bool write)
{
	tDMA_TAG *ptr = dmaGetAddr(addr, write);
	if (ptr == NULL)
	{
		throwBusError("dmaGetAddr");
		setDmacStat(num);
		chcr.STR = false;
	}

	return ptr;
}

tDMA_TAG *DMACh::DMAtransfer(u32 addr, u32 num)
{
	tDMA_TAG *tag = getAddr(addr, num, false);

	if (tag == NULL) return NULL;

    chcrTransfer(tag);
    qwcTransfer(tag);
    return tag;
}

tDMA_TAG DMACh::dma_tag()
{
	return chcr.tag();
}

wxString DMACh::cmq_to_str() const
{
	return wxsFormat(L"chcr = %lx, madr = %lx, qwc  = %lx", chcr._u32, madr, qwc);
}

wxString DMACh::cmqt_to_str() const
{
	return wxsFormat(L"chcr = %lx, madr = %lx, qwc  = %lx, tadr = %1x", chcr._u32, madr, qwc, tadr);
}

__fi void throwBusError(const char *s)
{
    Console.Error("%s BUSERR", s);
    dmacRegs.stat.BEIS = true;
}

__fi void setDmacStat(u32 num)
{
	dmacRegs.stat.set_flags(1 << num);
}

// Note: Dma addresses are guaranteed to be aligned to 16 bytes (128 bits)
__fi tDMA_TAG* SPRdmaGetAddr(u32 addr, bool write)
{
	// if (addr & 0xf) { DMA_LOG("*PCSX2*: DMA address not 128bit aligned: %8.8x", addr); }

	//For some reason Getaway references SPR Memory from itself using SPR0, oh well, let it i guess...
	if((addr & 0x70000000) == 0x70000000)
	{
		return (tDMA_TAG*)&eeMem->Scratch[addr & 0x3ff0];
	}

	// FIXME: Why??? DMA uses physical addresses
	addr &= 0x1ffffff0;

	if (addr < Ps2MemSize::MainRam)
	{
		return (tDMA_TAG*)&eeMem->Main[addr];
	}
	else if (addr < 0x10000000)
	{
		return (tDMA_TAG*)(write ? eeMem->ZeroWrite : eeMem->ZeroRead);
	}
	else if ((addr >= 0x11000000) && (addr < 0x11010000))
	{
		if (addr >= 0x11008000 && THREAD_VU1)
		{
			DevCon.Warning("MTVU: SPR Accessing VU1 Memory");
			vu1Thread.WaitVU();
		}
		
		//Access for VU Memory

		if((addr >= 0x1100c000) && (addr < 0x11010000))
		{
			//DevCon.Warning("VU1 Mem %x", addr);
			return (tDMA_TAG*)(VU1.Mem + (addr & 0x3ff0));
		}

		if((addr >= 0x11004000) && (addr < 0x11008000))
		{
			//DevCon.Warning("VU0 Mem %x", addr);
			return (tDMA_TAG*)(VU0.Mem + (addr & 0xff0));
		}
		
		//Possibly not needed but the manual doesn't say SPR cannot access it.
		if((addr >= 0x11000000) && (addr < 0x11004000))
		{
			//DevCon.Warning("VU0 Micro %x", addr);
			return (tDMA_TAG*)(VU0.Micro + (addr & 0xff0));
		}

		if((addr >= 0x11008000) && (addr < 0x1100c000))
		{
			//DevCon.Warning("VU1 Micro %x", addr);
			return (tDMA_TAG*)(VU1.Micro + (addr & 0x3ff0));
		}
		
		
		// Unreachable
		return NULL;
	}
	else
	{
		Console.Error( "*PCSX2*: DMA error: %8.8x", addr);
		return NULL;
	}
}

// Note: Dma addresses are guaranteed to be aligned to 16 bytes (128 bits)
__ri tDMA_TAG *dmaGetAddr(u32 addr, bool write)
{
	// if (addr & 0xf) { DMA_LOG("*PCSX2*: DMA address not 128bit aligned: %8.8x", addr); }
	if (DMA_TAG(addr).SPR) return (tDMA_TAG*)&eeMem->Scratch[addr & 0x3ff0];

	// FIXME: Why??? DMA uses physical addresses
	addr &= 0x1ffffff0;

	if (addr < Ps2MemSize::MainRam)
	{
		return (tDMA_TAG*)&eeMem->Main[addr];
	}
	else if (addr < 0x10000000)
	{
		return (tDMA_TAG*)(write ? eeMem->ZeroWrite : eeMem->ZeroRead);
	}
	else if (addr < 0x10004000)
	{
		// Secret scratchpad address for DMA = end of maximum main memory?
		//Console.Warning("Writing to the scratchpad without the SPR flag set!");
		return (tDMA_TAG*)&eeMem->Scratch[addr & 0x3ff0];
	}
	else
	{
		Console.Error( "*PCSX2*: DMA error: %8.8x", addr);
		return NULL;
	}
}


// Returns true if the DMA is enabled and executed successfully.  Returns false if execution
// was blocked (DMAE or master DMA enabler).
static bool QuickDmaExec( void (*func)(), u32 mem)
{
	bool ret = false;
    DMACh& reg = (DMACh&)psHu32(mem);

	if (reg.chcr.STR && dmacRegs.ctrl.DMAE && !psHu8(DMAC_ENABLER+2))
	{
		func();
		ret = true;
	}

	return ret;
}


static tDMAC_QUEUE QueuedDMA(0);
static u32 oldvalue = 0;

static void StartQueuedDMA()
{
	if (QueuedDMA.VIF0) { DMA_LOG("Resuming DMA for VIF0"); QueuedDMA.VIF0 = !QuickDmaExec(dmaVIF0, D0_CHCR); }
	if (QueuedDMA.VIF1) { DMA_LOG("Resuming DMA for VIF1"); QueuedDMA.VIF1 = !QuickDmaExec(dmaVIF1, D1_CHCR); }
	if (QueuedDMA.GIF ) { DMA_LOG("Resuming DMA for GIF" ); QueuedDMA.GIF  = !QuickDmaExec(dmaGIF , D2_CHCR); }
	if (QueuedDMA.IPU0) { DMA_LOG("Resuming DMA for IPU0"); QueuedDMA.IPU0 = !QuickDmaExec(dmaIPU0, D3_CHCR); }
	if (QueuedDMA.IPU1) { DMA_LOG("Resuming DMA for IPU1"); QueuedDMA.IPU1 = !QuickDmaExec(dmaIPU1, D4_CHCR); }
	if (QueuedDMA.SIF0) { DMA_LOG("Resuming DMA for SIF0"); QueuedDMA.SIF0 = !QuickDmaExec(dmaSIF0, D5_CHCR); }
	if (QueuedDMA.SIF1) { DMA_LOG("Resuming DMA for SIF1"); QueuedDMA.SIF1 = !QuickDmaExec(dmaSIF1, D6_CHCR); }
	if (QueuedDMA.SIF2) { DMA_LOG("Resuming DMA for SIF2"); QueuedDMA.SIF2 = !QuickDmaExec(dmaSIF2, D7_CHCR); }
	if (QueuedDMA.SPR0) { DMA_LOG("Resuming DMA for SPR0"); QueuedDMA.SPR0 = !QuickDmaExec(dmaSPR0, D8_CHCR); }
	if (QueuedDMA.SPR1) { DMA_LOG("Resuming DMA for SPR1"); QueuedDMA.SPR1 = !QuickDmaExec(dmaSPR1, D9_CHCR); }
}

static __ri void DmaExec( void (*func)(), u32 mem, u32 value )
{
	DMACh& reg = (DMACh&)psHu32(mem);
    tDMA_CHCR chcr(value);

	//It's invalid for the hardware to write a DMA while it is active, not without Suspending the DMAC
	if (reg.chcr.STR)
	{
		const uint channel = ChannelNumber(mem);

		//As the manual states "Fields other than STR can only be written to when the DMA is stopped"
		//Also "The DMA may not stop properly just by writing 0 to STR"
		//So the presumption is that STR can be written to (ala force stop the DMA) but nothing else
		//If the developer wishes to alter any of the other fields, it must be done AFTER the STR has been written,
		//it will not work before or during this event.
		if(chcr.STR == 0)
		{
			//DevCon.Warning(L"32bit Force Stopping %s (Current CHCR %x) while DMA active", ChcrName(mem), reg.chcr._u32, chcr._u32);
			reg.chcr.STR = 0;
			//We need to clear any existing DMA loops that are in progress else they will continue!

			if(channel == 1)
			{
				cpuClearInt( 10 );
				QueuedDMA._u16 &= ~(1 << 10); //Clear any queued DMA requests for this channel
			}
			else if(channel == 2)
			{
				cpuClearInt( 11 );
				QueuedDMA._u16 &= ~(1 << 11); //Clear any queued DMA requests for this channel
			}
				
			cpuClearInt( channel );
			QueuedDMA._u16 &= ~(1 << channel); //Clear any queued DMA requests for this channel
		}
		//else DevCon.Warning(L"32bit Attempted to change %s CHCR (Currently %x) with %x while DMA active, ignoring QWC = %x", ChcrName(mem), reg.chcr._u32, chcr._u32, reg.qwc);
		return;
	}

	//if(reg.chcr.TAG != chcr.TAG && chcr.MOD == CHAIN_MODE) DevCon.Warning(L"32bit CHCR Tag on %s changed to %x from %x QWC = %x Channel Not Active", ChcrName(mem), chcr.TAG, reg.chcr.TAG, reg.qwc);

	reg.chcr.set(value);

	//Final Fantasy XII sets the DMA Mode to 3 which doesn't exist. On some channels (like SPR) this will break logic completely. so lets assume they mean chain.
	if (reg.chcr.MOD == 0x3)
	{
		DevCon.Warning(L"%s CHCR.MOD set to 3, assuming 1 (chain)", ChcrName(mem));
		reg.chcr.MOD = 0x1;
	}
	if (reg.chcr.STR && dmacRegs.ctrl.DMAE && !psHu8(DMAC_ENABLER+2))
	{
		func();
	}
	else if(reg.chcr.STR)
	{
		//DevCon.Warning(L"32bit %s DMA Start while DMAC Disabled\n", ChcrName(mem));
		QueuedDMA._u16 |= (1 << ChannelNumber(mem)); //Queue the DMA up to be started then the DMA's are Enabled and or the Suspend is lifted
	} //else QueuedDMA._u16 &~= (1 << ChannelNumber(mem)); //
}

template< uint page >
__fi u32 dmacRead32( u32 mem )
{
	// Fixme: OPH hack. Toggle the flag on GIF_STAT access. (rama)
	if (IsPageFor(mem) && (mem == GIF_STAT) && CHECK_OPHFLAGHACK)
	{
		static unsigned counter = 1;
		if (++counter == 8)
			counter = 2;
		// Set OPH and APATH from counter, cycling paths and alternating OPH
		return gifRegs.stat._u32 & ~(7 << 9) | (counter & 1 ? counter << 9 : 0);
	}
	
	return psHu32(mem);
}

// Returns TRUE if the caller should do writeback of the register to eeHw; false if the
// register has no writeback, or if the writeback is handled internally.
template< uint page >
__fi bool dmacWrite32( u32 mem, mem32_t& value )
{
	iswitch(mem) {
		icase(D0_CHCR) // dma0 - vif0
		{
			DMA_LOG("VIF0dma EXECUTE, value=0x%x", value);
			DmaExec(dmaVIF0, mem, value);
			return false;
		}

		icase(D1_CHCR) // dma1 - vif1 - chcr
		{
			DMA_LOG("VIF1dma EXECUTE, value=0x%x", value);
			DmaExec(dmaVIF1, mem, value);
			return false;
		}

		icase(D2_CHCR) // dma2 - gif
		{
			DMA_LOG("GIFdma EXECUTE, value=0x%x", value);
			DmaExec(dmaGIF, mem, value);
			return false;
		}

		icase(D3_CHCR) // dma3 - fromIPU
		{
			DMA_LOG("IPU0dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaIPU0, mem, value);
			return false;
		}

		icase(D4_CHCR) // dma4 - toIPU
		{
			DMA_LOG("IPU1dma EXECUTE, value=0x%x\n", value);
			DmaExec(dmaIPU1, mem, value);
			return false;
		}

		icase(D5_CHCR) // dma5 - sif0
		{
			DMA_LOG("SIF0dma EXECUTE, value=0x%x", value);
			DmaExec(dmaSIF0, mem, value);
			return false;
		}

		icase(D6_CHCR) // dma6 - sif1
		{
			DMA_LOG("SIF1dma EXECUTE, value=0x%x", value);
			DmaExec(dmaSIF1, mem, value);
			return false;
		}

		icase(D7_CHCR) // dma7 - sif2
		{
			DMA_LOG("SIF2dma EXECUTE, value=0x%x", value);
			DmaExec(dmaSIF2, mem, value);
			return false;
		}

		icase(D8_CHCR) // dma8 - fromSPR
		{
			DMA_LOG("SPR0dma EXECUTE (fromSPR), value=0x%x", value);
			DmaExec(dmaSPR0, mem, value);
			return false;
		}

		icase(D9_CHCR) // dma9 - toSPR
		{
			DMA_LOG("SPR1dma EXECUTE (toSPR), value=0x%x", value);
			DmaExec(dmaSPR1, mem, value);
			return false;
		}
			
		icase(DMAC_CTRL)
		{
			u32 oldvalue = psHu32(mem);

			HW_LOG("DMAC_CTRL Write 32bit %x", value);

			psHu32(mem) = value;
			//Check for DMAS that were started while the DMAC was disabled
			if (((oldvalue & 0x1) == 0) && ((value & 0x1) == 1))
			{
				if (!QueuedDMA.empty()) StartQueuedDMA();
			}
			if ((oldvalue & 0x30) != (value & 0x30))
			{
				DevCon.Warning("32bit Stall Source Changed to %x", (value & 0x30) >> 4);
			}
			if ((oldvalue & 0xC0) != (value & 0xC0))
			{
				DevCon.Warning("32bit Stall Destination Changed to %x", (value & 0xC0) >> 4);
			}
			return false;
		}

		//Midway are a bunch of idiots, writing to E100 (reserved) instead of E010
		//Which causes a CPCOND0 to fail.
		icase(DMAC_FAKESTAT)
		{
			//DevCon.Warning("Midway fixup addr=%x writing %x for DMA_STAT", mem, value);
			HW_LOG("Midways own DMAC_STAT Write 32bit %x", value);

			// lower 16 bits: clear on 1
			// upper 16 bits: reverse on 1

			psHu16(0xe010) &= ~(value & 0xffff);
			psHu16(0xe012) ^= (u16)(value >> 16);

			cpuTestDMACInts();
			return false;
		}
		icase(DMAC_STAT)
		{
			HW_LOG("DMAC_STAT Write 32bit %x", value);

			// lower 16 bits: clear on 1
			// upper 16 bits: reverse on 1

			psHu16(0xe010) &= ~(value & 0xffff);
			psHu16(0xe012) ^= (u16)(value >> 16);

			cpuTestDMACInts();
			return false;
		}

		icase(DMAC_ENABLEW)
		{
			HW_LOG("DMAC_ENABLEW Write 32bit %lx", value);
			oldvalue = psHu8(DMAC_ENABLEW + 2);
			psHu32(DMAC_ENABLEW) = value;
			psHu32(DMAC_ENABLER) = value;
			if (((oldvalue & 0x1) == 1) && (((value >> 16) & 0x1) == 0))
			{
				if (!QueuedDMA.empty()) StartQueuedDMA();
			}
			return false;
		}
	}

	// DMA Writes are invalid to everything except the STR on CHCR when it is busy
	// There's timing problems with many games. Gamefix time!
	if( CHECK_DMABUSYHACK && (mem & 0xf0) )
	{	
		if((psHu32(mem & ~0xff) & 0x100) && dmacRegs.ctrl.DMAE && !psHu8(DMAC_ENABLER+2)) 
		{
			DevCon.Warning("Gamefix: Write to DMA addr %x while STR is busy! Ignoring", mem);
			return false;
		}
	}

	// fall-through: use the default writeback provided by caller.
	return true;
}

template u32 dmacRead32<0x03>( u32 mem );

template bool dmacWrite32<0x00>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x01>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x02>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x03>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x04>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x05>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x06>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x07>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x08>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x09>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x0a>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x0b>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x0c>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x0d>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x0e>( u32 mem, mem32_t& value );
template bool dmacWrite32<0x0f>( u32 mem, mem32_t& value );
