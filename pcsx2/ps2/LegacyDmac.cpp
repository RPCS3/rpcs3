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

#include "ps2/HwInternal.h"

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

		if(psHu8(DMAC_ENABLER+2) == 1) //DMA is suspended so we can allow writes to anything
		{
			//If it stops the DMA, we need to clear any pending interrupts so the DMA doesnt continue.
			if(chcr.STR == 0)
			{
				//DevCon.Warning(L"32bit %s DMA Stopped on Suspend", ChcrName(mem));
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
			//Sanity Check for possible future bug fix0rs ;p
			//Spams on Persona 4 opening.
			//if(reg.chcr.TAG != chcr.TAG) DevCon.Warning(L"32bit CHCR Tag on %s changed to %x from %x QWC = %x Channel Active", ChcrName(mem), chcr.TAG, reg.chcr.TAG, reg.qwc);
			//Here we update the LOWER CHCR, if a chain is stopped half way through, it can be manipulated in to a different mode
			//But we need to preserve the existing tag for now
			reg.chcr.set((reg.chcr.TAG << 16) | chcr.lower());
			return;
		}
		else //Else the DMA is running (Not Suspended), so we cant touch it!
		{
			//As the manual states "Fields other than STR can only be written to when the DMA is stopped"
			//Also "The DMA may not stop properly just by writing 0 to STR"
			//So the presumption is that STR can be written to (ala force stop the DMA) but nothing else

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

	}

	//if(reg.chcr.TAG != chcr.TAG && chcr.MOD == CHAIN_MODE) DevCon.Warning(L"32bit CHCR Tag on %s changed to %x from %x QWC = %x Channel Not Active", ChcrName(mem), chcr.TAG, reg.chcr.TAG, reg.qwc);

	reg.chcr.set(value);

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

// DmaExec8 should only be called for the second byte of CHCR.
// Testing Note: dark cloud 2 uses 8 bit DMAs register writes.
static __fi void DmaExec8( void (*func)(), u32 mem, u8 value )
{
	pxAssumeMsg( (mem & 0xf) == 1, "DmaExec8 should only be called for the second byte of CHCR" );

	// The calling function calls this when the second byte (bits 8->15) is written.  Only bit 8
	// is effective, and it is the STR (start) bit. :)	
	DmaExec( func, mem & ~0xf, (u32)value<<8 );
}

static __fi void DmaExec16( void (*func)(), u32 mem, u16 value )
{
	DmaExec( func, mem, (u32)value );
}

__fi u32 dmacRead32( u32 mem )
{
	// Fixme: OPH hack. Toggle the flag on each GIF_STAT access. (rama)
}

// Returns TRUE if the caller should do writeback of the register to eeHw; false if the
// register has no writeback, or if the writeback is handled internally.
template< uint page >
__fi bool dmacWrite32( u32 mem, mem32_t& value )
{
	if (IsPageFor(EEMemoryMap::VIF0_Start) && (mem >= EEMemoryMap::VIF0_Start))
	{
		return (mem >= EEMemoryMap::VIF1_Start)
			? vifWrite32<1>(mem, value)
			: vifWrite32<0>(mem, value);
	}

	iswitch(mem) {
	icase(GIF_CTRL)
	{
		psHu32(mem) = value & 0x8;

		if (value & 0x1)
			gsGIFReset();
		
		if (value & 8)
			gifRegs.stat.PSE = true;
		else
			gifRegs.stat.PSE = false;

		return false;
	}

	icase(GIF_MODE)
	{
		// need to set GIF_MODE (hamster ball)
		gifRegs.mode.write(value);

		// set/clear bits 0 and 2 as per the GIF_MODE value.
		const u32 bitmask = GIF_MODE_M3R | GIF_MODE_IMT;
		psHu32(GIF_STAT) &= ~bitmask;
		psHu32(GIF_STAT) |= (u32)value & bitmask;

		return false;
	}

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

	// fall-through: use the default writeback provided by caller.
	return true;
}

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
