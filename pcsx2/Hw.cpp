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

using namespace R5900;

u8  *psH; // hw mem

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

/*void hwShutdown()
{
	ipuShutdown();
}*/

void hwReset()
{
	hwInit();

	memzero_ptr<Ps2MemSize::Hardware>( PS2MEM_HW );
	//memset(PS2MEM_HW+0x2000, 0, 0x0000e000);

	psHu32(SBUS_F260) = 0x1D000060;
	// i guess this is kinda a version, it's used by some bioses
	psHu32(DMAC_ENABLEW) = 0x1201;
	psHu32(DMAC_ENABLER) = 0x1201;

	sifInit();
	sprInit();

	gsReset();
	ipuReset();
	vif0Reset();
	vif1Reset();
}

__forceinline void intcInterrupt()
{
	if ((cpuRegs.CP0.n.Status.val & 0x400) != 0x400) return;

	if ((psHu32(INTC_STAT)) == 0) {
		DevCon.Warning("*PCSX2*: intcInterrupt already cleared");
        return;
	}
	if ((psHu32(INTC_STAT) & psHu32(INTC_MASK)) == 0) return;

	HW_LOG("intcInterrupt %x", psHu32(INTC_STAT) & psHu32(INTC_MASK));
	if(psHu32(INTC_STAT) & 0x2){
		counters[0].hold = rcntRcount(0);
		counters[1].hold = rcntRcount(1);
	}

	cpuException(0x400, cpuRegs.branch);
}

__forceinline void dmacInterrupt()
{
    if ((cpuRegs.CP0.n.Status.val & 0x10807) != 0x10801) return;

	if( ((psHu16(DMAC_STAT + 2) & psHu16(DMAC_STAT)) == 0 ) &&
		( psHu16(DMAC_STAT) & 0x8000) == 0 ) return;

	if (!(dmacRegs->ctrl.DMAE)) return;

	HW_LOG("dmacInterrupt %x", (psHu16(DMAC_STAT + 2) & psHu16(DMAC_STAT) |
								  psHu16(DMAC_STAT) & 0x8000));

	cpuException(0x800, cpuRegs.branch);
}

void hwIntcIrq(int n)
{
	psHu32(INTC_STAT) |= 1<<n;
	cpuTestINTCInts();
}

void hwDmacIrq(int n)
{
	psHu32(DMAC_STAT) |= 1<<n;
	cpuTestDMACInts();
}

// Write 'size' bytes to memory address 'addr' from 'data'.
bool hwMFIFOWrite(u32 addr, u8 *data, u32 size)
{
	u32 msize = dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK + 16;
	u8 *dst;

	addr = dmacRegs->rbor.ADDR + (addr & dmacRegs->rbsr.RMSK);

	// Check if the transfer should wrap around the ring buffer
	if ((addr+size) >= msize) {
		int s1 = msize - addr;
		int s2 = size - s1;

		// it does, so first copy 's1' bytes from 'data' to 'addr'
		dst = (u8*)PSM(addr);
		if (dst == NULL) return false;
		memcpy_fast(dst, data, s1);

		// and second copy 's2' bytes from '&data[s1]' to 'maddr'
		dst = (u8*)PSM(dmacRegs->rbor.ADDR);
		if (dst == NULL) return false;
		memcpy_fast(dst, &data[s1], s2);
	}
	else {
		// it doesn't, so just copy 'size' bytes from 'data' to 'addr'
		dst = (u8*)PSM(addr);
		if (dst == NULL) return false;
		memcpy_fast(dst, data, size);
	}

	return true;
}

bool hwDmacSrcChainWithStack(DMACh *dma, int id) {
	switch (id) {
		case TAG_REFE: // Refe - Transfer Packet According to ADDR field
            //End Transfer
			return true;

		case TAG_CNT: // CNT - Transfer QWC following the tag.
            // Set MADR to QW afer tag, and set TADR to QW following the data.
			dma->madr = dma->tadr + 16;
			dma->tadr = dma->madr + (dma->qwc << 4);
			return false;

		case TAG_NEXT: // Next - Transfer QWC following tag. TADR = ADDR
		{
		    // Set MADR to QW following the tag, and set TADR to the address formerly in MADR.
			u32 temp = dma->madr;
			dma->madr = dma->tadr + 16;
			dma->tadr = temp;
			return false;
		}
		case TAG_REF: // Ref - Transfer QWC from ADDR field
		case TAG_REFS: // Refs - Transfer QWC from ADDR field (Stall Control)
            //Set TADR to next tag
			dma->tadr += 16;
			return false;

		case TAG_CALL: // Call - Transfer QWC following the tag, save succeeding tag
		{
		    // Store the address in MADR in temp, and set MADR to the data following the tag.
			u32 temp = dma->madr;
			dma->madr = dma->tadr + 16;

			// Stash an address on the address stack pointer.
			switch(dma->chcr.ASP)
            {
                case 0: //Check if ASR0 is empty
                    // Store the succeeding tag in asr0, and mark chcr as having 1 address.
                    dma->asr0 = dma->madr + (dma->qwc << 4);
                    dma->chcr.ASP++;
                    break;

                case 1:
                    // Store the succeeding tag in asr1, and mark chcr as having 2 addresses.
                    dma->asr1 = dma->madr + (dma->qwc << 4);
                    dma->chcr.ASP++;
                    break;

                default:
                    Console.Warning("Call Stack Overflow (report if it fixes/breaks anything)");
                    return true;
			}

			// Set TADR to the address from MADR we stored in temp.
			dma->tadr = temp;

			return false;
		}

		case TAG_RET: // Ret - Transfer QWC following the tag, load next tag
            //Set MADR to data following the tag.
			dma->madr = dma->tadr + 16;

			// Snag an address from the address stack pointer.
			switch(dma->chcr.ASP)
            {
                case 2:
                    // Pull asr1 from the stack, give it to TADR, and decrease the # of addresses.
                    dma->tadr = dma->asr1;
                    dma->asr1 = 0;
                    dma->chcr.ASP--;
                    break;

                case 1:
                    // Pull asr0 from the stack, give it to TADR, and decrease the # of addresses.
                    dma->tadr = dma->asr0;
                    dma->asr0 = 0;
                    dma->chcr.ASP--;
                    break;

                case 0:
                    // There aren't any addresses to pull, so end the transfer.
                    //dma->tadr += 16;						   //Clear tag address - Kills Klonoa 2
                    return true;

                default:
                    // If ASR1 and ASR0 are messed up, end the transfer.
                    //Console.Error("TAG_RET: ASR 1 & 0 == 1. This shouldn't happen!");
                    //dma->tadr += 16;						   //Clear tag address - Kills Klonoa 2
                    return true;
            }
			return false;

		case TAG_END: // End - Transfer QWC following the tag
            //Set MADR to data following the tag, and end the transfer.
			dma->madr = dma->tadr + 16;
			//Don't Increment tadr; breaks Soul Calibur II and III
			return true;
	}

	return false;
}

bool hwDmacSrcChain(DMACh *dma, int id)
{
	u32 temp;

	switch (id)
	{
		case TAG_REFE: // Refe - Transfer Packet According to ADDR field
            // End the transfer.
			return true;

		case TAG_CNT: // CNT - Transfer QWC following the tag.
            // Set MADR to QW after the tag, and TADR to QW following the data.
			dma->madr = dma->tadr + 16;
			dma->tadr = dma->madr + (dma->qwc << 4);
			return false;

		case TAG_NEXT: // Next - Transfer QWC following tag. TADR = ADDR
            // Set MADR to QW following the tag, and set TADR to the address formerly in MADR.
			temp = dma->madr;
			dma->madr = dma->tadr + 16;
			dma->tadr = temp;
			return false;

		case TAG_REF: // Ref - Transfer QWC from ADDR field
		case TAG_REFS: // Refs - Transfer QWC from ADDR field (Stall Control)
            //Set TADR to next tag
			dma->tadr += 16;
			return false;

		case TAG_END: // End - Transfer QWC following the tag
            //Set MADR to data following the tag, and end the transfer.
			dma->madr = dma->tadr + 16;
			//Don't Increment tadr; breaks Soul Calibur II and III
			return true;
	}

	return false;
}
