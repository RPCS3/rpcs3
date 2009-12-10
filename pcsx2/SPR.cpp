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

#include "SPR.h"
#include "VUmicro.h"

extern void mfifoGIFtransfer(int);

static bool spr0finished = false;
static bool spr1finished = false;

static u32 mfifotransferred = 0;

void sprInit()
{
}

static void TestClearVUs(u32 madr, u32 size)
{
	if (madr >= 0x11000000)
	{
		if (madr < 0x11004000)
		{
			DbgCon.Warning("scratch pad clearing vu0");
			CpuVU0->Clear(madr&0xfff, size);
		}
		else if (madr >= 0x11008000 && madr < 0x1100c000)
		{
			DbgCon.Warning("scratch pad clearing vu1");
			CpuVU1->Clear(madr&0x3fff, size);
		}
	}
}

int  _SPR0chain()
{
	tDMA_TAG *pMem;

	if (spr0->qwc == 0) return 0;
	pMem = (tDMA_TAG*)dmaGetAddr(spr0->madr);
	if (pMem == NULL) return -1;

	switch (dmacRegs->ctrl.MFD)
	{
		case MFD_VIF1:
		case MFD_GIF:
			if ((spr0->madr & ~dmacRegs->rbsr.RMSK) != dmacRegs->rbor.ADDR) 
				Console.WriteLn("SPR MFIFO Write outside MFIFO area");
			else 
				mfifotransferred += spr0->qwc;
			
			hwMFIFOWrite(spr0->madr, &psSu8(spr0->sadr), spr0->qwc << 4);
			spr0->madr += spr0->qwc << 4;
			spr0->madr = dmacRegs->rbor.ADDR + (spr0->madr & dmacRegs->rbsr.RMSK);
			break;
			
		case NO_MFD:
		case MFD_RESERVED:
			memcpy_fast((u8*)pMem, &psSu8(spr0->sadr), spr0->qwc << 4);
			
			// clear VU mem also!
			TestClearVUs(spr0->madr, spr0->qwc << 2); // Wtf is going on here? AFAIK, only VIF should affect VU micromem (cottonvibes)
			spr0->madr += spr0->qwc << 4;
			break;
	}
	
	spr0->sadr += spr0->qwc << 4;

	return (spr0->qwc) * BIAS; // bus is 1/2 the ee speed
}

__forceinline void SPR0chain() 
{
	_SPR0chain(); 
	spr0->qwc = 0;
}


void _SPR0interleave()
{
	int qwc = spr0->qwc;
	int sqwc = dmacRegs->sqwc.SQWC; 
	int tqwc = dmacRegs->sqwc.TQWC; 
	tDMA_TAG *pMem;
	
	if (tqwc == 0) tqwc = qwc;
	//Console.WriteLn("dmaSPR0 interleave");
	SPR_LOG("SPR0 interleave size=%d, tqwc=%d, sqwc=%d, addr=%lx sadr=%lx",
	        spr0->qwc, tqwc, sqwc, spr0->madr, spr0->sadr);

	while (qwc > 0)
	{
		spr0->qwc = std::min(tqwc, qwc);
		qwc -= spr0->qwc;
		pMem = (tDMA_TAG*)dmaGetAddr(spr0->madr);
		
		switch (dmacRegs->ctrl.MFD)
 		{
			case MFD_VIF1:
			case MFD_GIF:
				hwMFIFOWrite(spr0->madr, &psSu8(spr0->sadr), spr0->qwc << 4);
				mfifotransferred += spr0->qwc;
				break;
			
			case NO_MFD:
			case MFD_RESERVED:
				// clear VU mem also!
				TestClearVUs(spr0->madr, spr0->qwc << 2);
				memcpy_fast((u8*)pMem, &psSu8(spr0->sadr), spr0->qwc << 4);
				break;
 		}
		spr0->sadr += spr0->qwc * 16;
		spr0->madr += (sqwc + spr0->qwc) * 16;
	}

	spr0->qwc = 0;
	spr0finished = true;
}

static __forceinline void _dmaSPR0()
{
	if (dmacRegs->ctrl.STS == STS_fromSPR)
	{
		Console.WriteLn("SPR0 stall %d", dmacRegs->ctrl.STS);
	}

	// Transfer Dn_QWC from SPR to Dn_MADR
	switch(spr0->chcr.MOD)
	{
		case NORMAL_MODE:
		{
			SPR0chain();
			spr0finished = true;
			return;
		}
		case CHAIN_MODE:
		{
			tDMA_TAG *ptag;
			bool done = FALSE;

			if (spr0->qwc > 0)
			{
				SPR0chain();
				spr0finished = true;
				return;
			}
			// Destination Chain Mode
			ptag = (tDMA_TAG*)&psSu32(spr0->sadr);
			spr0->sadr += 16;
			
			spr0->unsafeTransfer(ptag);
			
			spr0->madr = ptag[1]._u32;					//MADR = ADDR field + SPR

			SPR_LOG("spr0 dmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx spr=%lx",
				ptag[1]._u32, ptag[0]._u32, spr0->qwc, ptag->ID, spr0->madr, spr0->sadr);

			if (dmacRegs->ctrl.STS == STS_fromSPR)   // STS == fromSPR
			{
				Console.WriteLn("SPR stall control");
			}

			switch (ptag->ID)
			{
				case TAG_CNTS: // CNTS - Transfer QWC following the tag (Stall Control)
					if (dmacRegs->ctrl.STS == STS_fromSPR) dmacRegs->stadr.ADDR = spr0->madr + (spr0->qwc * 16);					//Copy MADR to DMAC_STADR stall addr register
					break;

				case TAG_CNT: // CNT - Transfer QWC following the tag.
					done = false;
					break;

				case TAG_END: // End - Transfer QWC following the tag
					done = true;
					break;
			}
			
			SPR0chain();
			
			if (spr0->chcr.TIE && ptag->IRQ)  			 //Check TIE bit of CHCR and IRQ bit of tag
			{
				//Console.WriteLn("SPR0 TIE");
				done = true;
			}
				
			spr0finished = done;
			
			if (!done)
			{
				ptag = (tDMA_TAG*)&psSu32(spr0->sadr);		//Set memory pointer to SADR
				CPU_INT(8, ptag[0].QWC / BIAS); // the lower 16bits of the tag / BIAS);
				return;
			}
			SPR_LOG("spr0 dmaChain complete %8.8x_%8.8x size=%d, id=%d, addr=%lx spr=%lx",
				ptag[1]._u32, ptag[0]._u32, spr0->qwc, ptag->ID, spr0->madr);
			break;
		}
		//case INTERLEAVE_MODE:
		default:
		{
			_SPR0interleave();
			break;
		}
	}
}

void SPRFROMinterrupt()
{
	_dmaSPR0();

	if(mfifotransferred != 0)
	{
        switch (dmacRegs->ctrl.MFD)
		{
			case MFD_VIF1: // Most common case.
			{
				if ((spr0->madr & ~dmacRegs->rbsr.RMSK) != dmacRegs->rbor.ADDR) Console.WriteLn("VIF MFIFO Write outside MFIFO area");
				spr0->madr = dmacRegs->rbor.ADDR + (spr0->madr & dmacRegs->rbsr.RMSK);
				//Console.WriteLn("mfifoVIF1transfer %x madr %x, tadr %x", vif1ch->chcr._u32, vif1ch->madr, vif1ch->tadr);
				mfifoVIF1transfer(mfifotransferred);
				mfifotransferred = 0;
				if (vif1ch->chcr.STR) return;
				break;
			}
			case MFD_GIF:
			{
				if ((spr0->madr & ~dmacRegs->rbsr.RMSK) != dmacRegs->rbor.ADDR) Console.WriteLn("GIF MFIFO Write outside MFIFO area");
				spr0->madr = dmacRegs->rbor.ADDR + (spr0->madr & dmacRegs->rbsr.RMSK);
				//Console.WriteLn("mfifoGIFtransfer %x madr %x, tadr %x", gif->chcr._u32, gif->madr, gif->tadr);
				mfifoGIFtransfer(mfifotransferred);
				mfifotransferred = 0;
				if (gif->chcr.STR) return;
				break;
			}
			default:
				break;
		}
	}
	if (!spr0finished) return;
	spr0->chcr.STR = false;
	hwDmacIrq(DMAC_FROM_SPR);
}

void dmaSPR0()   // fromSPR
{
	SPR_LOG("dmaSPR0 chcr = %lx, madr = %lx, qwc  = %lx, sadr = %lx",
	        spr0->chcr._u32, spr0->madr, spr0->qwc, spr0->sadr);

	if ((spr0->chcr.MOD == CHAIN_MODE) && spr0->qwc == 0)
	{
		tDMA_TAG *ptag;
		ptag = (tDMA_TAG*)&psSu32(spr0->sadr);		//Set memory pointer to SADR
		CPU_INT(8, ptag[0].QWC / BIAS);
		return;
	}
	// COMPLETE HACK!!! For now at least..  FFX Videos dont rely on interrupts or reading DMA values
	// It merely assumes that the last one has finished then starts another one (broke with the DMA fix)
	// This "shouldn't" cause any problems as SPR is generally faster than the other DMAS anyway. (Refraction)
	CPU_INT(8, spr0->qwc / BIAS);
}

__forceinline static void SPR1transfer(u32 *data, int size)
{
	memcpy_fast(&psSu8(spr1->sadr), (u8*)data, size << 2);

	spr1->sadr += size << 2;
}

int  _SPR1chain()
{
	tDMA_TAG *pMem;

	if (spr1->qwc == 0) return 0;

	pMem = (tDMA_TAG*)dmaGetAddr(spr1->madr);
	if (pMem == NULL) return -1;

	SPR1transfer((u32*)pMem, spr1->qwc << 2);
	spr1->madr += spr1->qwc << 4;

	return (spr1->qwc) * BIAS;
}

__forceinline void SPR1chain() 
{
	_SPR1chain(); 
	spr1->qwc = 0;
}

void _SPR1interleave()
{
	int qwc = spr1->qwc;
	int sqwc = dmacRegs->sqwc.SQWC; 
	int tqwc =  dmacRegs->sqwc.TQWC; 
	tDMA_TAG *pMem;
	
	if (tqwc == 0) tqwc = qwc;
	SPR_LOG("SPR1 interleave size=%d, tqwc=%d, sqwc=%d, addr=%lx sadr=%lx",
	        spr1->qwc, tqwc, sqwc, spr1->madr, spr1->sadr);

	while (qwc > 0)
	{
		spr1->qwc = std::min(tqwc, qwc);
		qwc -= spr1->qwc;
		pMem = (tDMA_TAG*)dmaGetAddr(spr1->madr);
		memcpy_fast(&psSu8(spr1->sadr), (u8*)pMem, spr1->qwc << 4);
		spr1->sadr += spr1->qwc * 16;
		spr1->madr += (sqwc + spr1->qwc) * 16; 
	}

	spr1->qwc = 0;
	spr1finished = true;
}

void _dmaSPR1()   // toSPR work function
{
	switch(spr1->chcr.MOD)
	{
		case NORMAL_MODE:
		{
			//int cycles = 0;
			// Transfer Dn_QWC from Dn_MADR to SPR1
			SPR1chain();
			spr1finished = true;
			return;
		}
		case CHAIN_MODE:
		{
			tDMA_TAG *ptag;
			bool done = false;

			if (spr1->qwc > 0)
			{
				// Transfer Dn_QWC from Dn_MADR to SPR1
				SPR1chain();
				spr1finished = true;
				return;
			}
			// Chain Mode

			ptag = (tDMA_TAG*)dmaGetAddr(spr1->tadr);		//Set memory pointer to TADR
			
			if (!spr1->transfer("SPR1 Tag", ptag))
			{
				done = true;
				spr1finished = done;
			}

			spr1->madr = ptag[1]._u32;						//MADR = ADDR field + SPR

			// Transfer dma tag if tte is set
			if (spr1->chcr.TTE)
			{
				SPR_LOG("SPR TTE: %x_%x\n", ptag[3]._u32, ptag[2]._u32);
				SPR1transfer((u32*)ptag, 4);				//Transfer Tag
			}

			SPR_LOG("spr1 dmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx",
				ptag[1]._u32, ptag[0]._u32, spr1->qwc, ptag->ID, spr1->madr);

			done = (hwDmacSrcChain(spr1, ptag->ID) == 1);
			SPR1chain();										//Transfers the data set by the switch

			if (spr1->chcr.TIE && ptag->IRQ)  			//Check TIE bit of CHCR and IRQ bit of tag
			{
				SPR_LOG("dmaIrq Set");

				//Console.WriteLn("SPR1 TIE");
				done = true;
			}
			
			spr1finished = done;
			if (!done)
			{
				ptag = (tDMA_TAG*)dmaGetAddr(spr1->tadr);		//Set memory pointer to TADR
				CPU_INT(9, (ptag[0].QWC / BIAS));// the lower 16 bits of the tag / BIAS);
			}
			break;
		}
		//case INTERLEAVE_MODE:
		default:
		{
			_SPR1interleave();
			break;
		}
	}
}

void dmaSPR1()   // toSPR
{
	SPR_LOG("dmaSPR1 chcr = 0x%x, madr = 0x%x, qwc  = 0x%x\n"
	        "        tadr = 0x%x, sadr = 0x%x",
	        spr1->chcr._u32, spr1->madr, spr1->qwc,
	        spr1->tadr, spr1->sadr);

	if ((spr1->chcr.MOD == CHAIN_MODE) && (spr1->qwc == 0))
	{
		tDMA_TAG *ptag;
		ptag = (tDMA_TAG*)dmaGetAddr(spr1->tadr);		//Set memory pointer to TADR
		CPU_INT(9, ptag[0].QWC / BIAS);
		return;
	}
	// COMPLETE HACK!!! For now at least..  FFX Videos dont rely on interrupts or reading DMA values
	// It merely assumes that the last one has finished then starts another one (broke with the DMA fix)
	// This "shouldn't" cause any problems as SPR is generally faster than the other DMAS anyway. (Refraction)
	CPU_INT(9, spr1->qwc / BIAS);
}

void SPRTOinterrupt()
{
	_dmaSPR1();
	if (!spr1finished) return;
	spr1->chcr.STR = false;
	hwDmacIrq(DMAC_TO_SPR);
}

void SaveStateBase::sprFreeze()
{
	FreezeTag("SPRdma");

	Freeze(spr0finished);
	Freeze(spr1finished);
	Freeze(mfifotransferred);
}
