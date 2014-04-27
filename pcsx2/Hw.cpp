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
#include "newVif.h"
#include "IPU/IPUdma.h"
#include "Gif_Unit.h"

using namespace R5900;

const int rdram_devices = 2;	// put 8 for TOOL and 2 for PS2 and PSX
int rdram_sdevid = 0;

static bool hwInitialized = false;

void hwInit()
{
	// [TODO] / FIXME:  PCSX2 no longer works on an Init system.  It assumes that the
	// static global vars for the process will be initialized when the process is created, and
	// then issues *resets only* from then on. (reset code for various S2 components should do
	// NULL checks and allocate memory and such if the pointers are NULL only).

	if( hwInitialized ) return;

	VifUnpackSSE_Init();

	gsInit();
	sifInit();
	sprInit();
	ipuInit();

	hwInitialized = true;
}

void hwShutdown()
{
	if (!hwInitialized) return;

	VifUnpackSSE_Destroy();

	hwInitialized = false;
}

void hwReset()
{
	hwInit();

	memzero( eeHw );

	psHu32(SBUS_F260) = 0x1D000060;

	// i guess this is kinda a version, it's used by some bioses
	psHu32(DMAC_ENABLEW) = 0x1201;
	psHu32(DMAC_ENABLER) = 0x1201;

	SPU2reset();

	sifInit();
	sprInit();

	gsReset();
	gifUnit.Reset();
	ipuReset();
	vif0Reset();
	vif1Reset();

	// needed for legacy DMAC
	ipuDmaReset();
}

__fi uint intcInterrupt()
{
	if ((psHu32(INTC_STAT)) == 0) {
		//DevCon.Warning("*PCSX2*: intcInterrupt already cleared");
        return 0;
	}
	if ((psHu32(INTC_STAT) & psHu32(INTC_MASK)) == 0) 
	{
		//DevCon.Warning("*PCSX2*: No valid interrupt INTC_MASK: %x INTC_STAT: %x", psHu32(INTC_MASK), psHu32(INTC_STAT));
		return 0;
	}

	HW_LOG("intcInterrupt %x", psHu32(INTC_STAT) & psHu32(INTC_MASK));
	if(psHu32(INTC_STAT) & 0x2){
		counters[0].hold = rcntRcount(0);
		counters[1].hold = rcntRcount(1);
	}

	//cpuException(0x400, cpuRegs.branch);
	return 0x400;
}

__fi uint dmacInterrupt()
{
	if( ((psHu16(DMAC_STAT + 2) & psHu16(DMAC_STAT)) == 0 ) &&
		( psHu16(DMAC_STAT) & 0x8000) == 0 ) 
	{
		//DevCon.Warning("No valid DMAC interrupt MASK %x STAT %x", psHu16(DMAC_STAT+2), psHu16(DMAC_STAT));
		return 0;
	}

	if (!dmacRegs.ctrl.DMAE || psHu8(DMAC_ENABLER+2) == 1) 
	{
		//DevCon.Warning("DMAC Suspended or Disabled on interrupt");
		return 0;
	}

	DMA_LOG("dmacInterrupt %x",
		((psHu16(DMAC_STAT + 2) & psHu16(DMAC_STAT)) |
		 (psHu16(DMAC_STAT) & 0x8000))
	);

	//cpuException(0x800, cpuRegs.branch);
	return 0x800;
}

void hwIntcIrq(int n)
{
	psHu32(INTC_STAT) |= 1<<n;
	if(psHu32(INTC_MASK) & (1<<n))cpuTestINTCInts();
}

void hwDmacIrq(int n)
{
	psHu32(DMAC_STAT) |= 1<<n;
	if(psHu16(DMAC_STAT+2) & (1<<n))cpuTestDMACInts();
}

void FireMFIFOEmpty()
{
	SPR_LOG("MFIFO Data Empty");
	hwDmacIrq(DMAC_MFIFO_EMPTY);

	if (dmacRegs.ctrl.MFD == MFD_VIF1) vif1Regs.stat.FQC = 0;
	if (dmacRegs.ctrl.MFD == MFD_GIF)  gifRegs.stat.FQC  = 0;
}

// Write 'size' bytes to memory address 'addr' from 'data'.
__ri bool hwMFIFOWrite(u32 addr, const u128* data, uint qwc)
{
	// all FIFO addresses should always be QWC-aligned.
	pxAssert((dmacRegs.rbor.ADDR & 15) == 0);
	pxAssert((addr & 15) == 0);

	if(qwc > ((dmacRegs.rbsr.RMSK + 16) >> 4)) DevCon.Warning("MFIFO Write bigger than MFIFO! QWC=%x FifoSize=%x", qwc, ((dmacRegs.rbsr.RMSK + 16) >> 4));
	// DMAC Address resolution:  FIFO can be placed anywhere in the *physical* memory map
	// for the PS2.  Its probably a serious error for a PS2 app to have the buffer cross
	// valid/invalid page areas of ram, so realistically we only need to test the base address
	// of the FIFO for address validity.

	if (u128* dst = (u128*)PSM(dmacRegs.rbor.ADDR))
	{
		const u32 ringsize = (dmacRegs.rbsr.RMSK / 16) + 1;
		pxAssertMsg( PSM(dmacRegs.rbor.ADDR+ringsize-1) != NULL, "Scratchpad/MFIFO ringbuffer spans into invalid (unmapped) physical memory!" );
		uint startpos = (addr & dmacRegs.rbsr.RMSK)/16;
		MemCopy_WrappedDest( data, dst, startpos, ringsize, qwc );
	}
	else
	{
		SPR_LOG( "Scratchpad/MFIFO: invalid base physical address: 0x%08x", dmacRegs.rbor.ADDR );
		pxFailDev( wxsFormat( L"Scratchpad/MFIFO: Invalid base physical address: 0x%08x", dmacRegs.rbor.ADDR) );
		return false;
	}

	return true;
}

__ri bool hwDmacSrcChainWithStack(DMACh& dma, int id) {
	switch (id) {
		case TAG_REFE: // Refe - Transfer Packet According to ADDR field
			dma.tadr += 16;
            //End Transfer
			return true;

		case TAG_CNT: // CNT - Transfer QWC following the tag.
            // Set MADR to QW afer tag, and set TADR to QW following the data.
			dma.tadr += 16;
			dma.madr = dma.tadr;
			//dma.tadr = dma.madr + (dma.qwc << 4);
			return false;

		case TAG_NEXT: // Next - Transfer QWC following tag. TADR = ADDR
		{
		    // Set MADR to QW following the tag, and set TADR to the address formerly in MADR.
			u32 temp = dma.madr;
			dma.madr = dma.tadr + 16;
			dma.tadr = temp;
			return false;
		}
		case TAG_REF: // Ref - Transfer QWC from ADDR field
		case TAG_REFS: // Refs - Transfer QWC from ADDR field (Stall Control)
            //Set TADR to next tag
			dma.tadr += 16;
			return false;

		case TAG_CALL: // Call - Transfer QWC following the tag, save succeeding tag
		{
		    // Store the address in MADR in temp, and set MADR to the data following the tag.
			u32 temp = dma.madr;
			dma.madr = dma.tadr + 16;

			if(temp == 0)
			{
				DevCon.Warning("DMA Chain CALL next tag error. Tag Addr = 0");
				dma.tadr = dma.madr + (dma.qwc << 4);
				return false;
			}
			// Stash an address on the address stack pointer.
			switch(dma.chcr.ASP)
            {
                case 0: //Check if ASR0 is empty
                    // Store the succeeding tag in asr0, and mark chcr as having 1 address.
                    dma.asr0 = dma.madr + (dma.qwc << 4);
                    dma.chcr.ASP++;
                    break;

                case 1:
                    // Store the succeeding tag in asr1, and mark chcr as having 2 addresses.
                    dma.asr1 = dma.madr + (dma.qwc << 4);
                    dma.chcr.ASP++;
                    break;

                default:
                    Console.Warning("Call Stack Overflow (report if it fixes/breaks anything)");
                    return true;
			}

			// Set TADR to the address from MADR we stored in temp.
			dma.tadr = temp;

			return false;
		}

		case TAG_RET: // Ret - Transfer QWC following the tag, load next tag
            //Set MADR to data following the tag.
			dma.madr = dma.tadr + 16;

			// Snag an address from the address stack pointer.
			switch(dma.chcr.ASP)
            {
                case 2:
                    // Pull asr1 from the stack, give it to TADR, and decrease the # of addresses.
                    dma.tadr = dma.asr1;
                    dma.asr1 = 0;
                    dma.chcr.ASP--;
                    break;

                case 1:
                    // Pull asr0 from the stack, give it to TADR, and decrease the # of addresses.
                    dma.tadr = dma.asr0;
                    dma.asr0 = 0;
                    dma.chcr.ASP--;
                    break;

                case 0:
                    // There aren't any addresses to pull, so end the transfer.
                    //dma.tadr += 16;						   //Clear tag address - Kills Klonoa 2
                    return true;

                default:
                    // If ASR1 and ASR0 are messed up, end the transfer.
                    //Console.Error("TAG_RET: ASR 1 & 0 == 1. This shouldn't happen!");
                    //dma.tadr += 16;						   //Clear tag address - Kills Klonoa 2
                    return true;
            }
			return false;

		case TAG_END: // End - Transfer QWC following the tag
            //Set MADR to data following the tag, and end the transfer.
			dma.madr = dma.tadr + 16;
			//Don't Increment tadr; breaks Soul Calibur II and III
			return true;
	}

	return false;
}


/********TADR NOTES***********
From what i've gathered from testing tadr increment stuff (with CNT) is that we might not be 100% accurate in what
increments it and what doesnt. Previously we presumed REFE and END didn't increment the tag, but SIF and IPU never
liked this.

From what i've deduced, REFE does in fact increment, but END doesn't, after much testing, i've concluded this is how
we can standardize DMA chains, so i've modified the code to work like this.   The below function controls the increment
of the TADR along with the MADR on VIF, GIF and SPR1 when using the CNT tag, the others don't use it yet, but they 
can probably be modified to do so now.

Reason for this:- Many games  (such as clock tower 3 and FFX Videos) watched the TADR to see when a transfer has finished, 
so we need to simulate this wherever we can!  Even the FFX video gets corruption and tries to fire multiple DMA Kicks 
if this doesnt happen, which was the reasoning for the hacked up SPR timing we had, that is no longer required.

-Refraction
******************************/

void hwDmacSrcTadrInc(DMACh& dma)
{
	//Don't touch it if in normal/interleave mode.
	if (dma.chcr.STR == 0) return;
	if (dma.chcr.MOD != 1) return;
		
	u16 tagid = (dma.chcr.TAG >> 12) & 0x7;

	if (tagid == TAG_CNT)
	{
		dma.tadr = dma.madr;
	}
}
bool hwDmacSrcChain(DMACh& dma, int id)
{
	u32 temp;

	switch (id)
	{
		case TAG_REFE: // Refe - Transfer Packet According to ADDR field
			dma.tadr += 16;
            // End the transfer.
			return true;

		case TAG_CNT: // CNT - Transfer QWC following the tag.
            // Set MADR to QW after the tag, and TADR to QW following the data.
			dma.madr = dma.tadr + 16;
			dma.tadr = dma.madr;
			return false;

		case TAG_NEXT: // Next - Transfer QWC following tag. TADR = ADDR
            // Set MADR to QW following the tag, and set TADR to the address formerly in MADR.
			temp = dma.madr;
			dma.madr = dma.tadr + 16;
			dma.tadr = temp;
			return false;

		case TAG_REF: // Ref - Transfer QWC from ADDR field
		case TAG_REFS: // Refs - Transfer QWC from ADDR field (Stall Control)
            //Set TADR to next tag
			dma.tadr += 16;
			return false;

		case TAG_END: // End - Transfer QWC following the tag
            //Set MADR to data following the tag, and end the transfer.
			dma.madr = dma.tadr + 16;
			//Don't Increment tadr; breaks Soul Calibur II and III
			return true;
	}

	return false;
}

