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
#include "Vif_Dma.h"

#include "iR5900.h"

using std::min;

// A three-way toggle used to determine if the GIF is stalling (transferring) or done (finished).
// Should be a gifstate_t rather then int, but I don't feel like possibly interfering with savestates right now.
static int gifstate = GIF_STATE_READY;
static bool gifempty = false;

static bool gspath3done = false;

static u32 gscycles = 0, prevcycles = 0, mfifocycles = 0;
static u32 gifqwc = 0;
static bool gifmfifoirq = false;

static __forceinline void clearFIFOstuff(bool full)
{
	GSCSRr &= ~0xC000;  //Clear FIFO stuff

	if (full)
		GSCSRr |= 0x8000;   //FIFO full
	else
		GSCSRr |= 0x4000;  //FIFO empty
}

__forceinline void gsInterrupt()
{
	GIF_LOG("gsInterrupt: %8.8x", cpuRegs.cycle);

	if (!(gif->chcr.STR))
	{
		//Console.WriteLn("Eh? why are you still interrupting! chcr %x, qwc %x, done = %x", gif->chcr._u32, gif->qwc, done);
		return;
	}

	
	if (GSTransferStatus.PTH3 == STOPPED_MODE)
	{
	    gifRegs->stat.clear_flags(GIF_STAT_APATH3 | GIF_STAT_OPH);
	}

	if ((gif->qwc > 0) || (!gspath3done))
	{
		if (!dmacRegs->ctrl.DMAE)
		{
			Console.Warning("gs dma masked, re-scheduling...");
			// re-raise the int shortly in the future
			CPU_INT( DMAC_GIF, 64 );
			return;
		}

		GIFdma();
		return;
	}

	// Champions: Return to Arms prints this a lot.
	if((gif->chcr.MOD == CHAIN_MODE) && ((gif->chcr.TAG >> 12) & 0x7) != 0x0 && ((gif->chcr.TAG >> 12) & 0x7) != 0x7 && !((gif->chcr.TAG >> 12) & 0x8))
		DevCon.Warning("GIF Ending when refe or end not set! CHCR = %x", gif->chcr._u32);

	gspath3done = false;
	gscycles = 0;
	gif->chcr.STR = false;
	vif1Regs->stat.VGW = false;
	if(GSTransferStatus.PTH3 == 3) GSTransferStatus.PTH3 = STOPPED_MODE;
	gifRegs->stat.clear_flags(GIF_STAT_APATH3 | GIF_STAT_OPH | GIF_STAT_FQC);

	clearFIFOstuff(false);
	hwDmacIrq(DMAC_GIF);
	GIF_LOG("GIF DMA end");
}

static u32 WRITERING_DMA(u32 *pMem, u32 qwc)
{
	int size   = GetMTGS().PrepDataPacket(GIF_PATH_3, pMem, qwc);
	u8* pgsmem = GetMTGS().GetDataPacketPtr();

	memcpy_aligned(pgsmem, pMem, size<<4);

	GetMTGS().SendDataPacket();
	return size;
}

static u32 WRITERING_DMA(tDMA_TAG *pMem, u32 qwc)
{
    return WRITERING_DMA((u32*)pMem, qwc);
}

int  _GIFchain()
{
	u32 qwc = min( gifsplit, (int)gif->qwc );
	tDMA_TAG *pMem;

	pMem = dmaGetAddr(gif->madr);
	if (pMem == NULL)
	{
		// reset path3, fixes dark cloud 2
		GIFPath_Clear( GIF_PATH_3 );

		//must increment madr and clear qwc, else it loops
		gif->madr += gif->qwc * 16;
		gif->qwc = 0;
		Console.Warning( "Hackfix - NULL GIFchain" );
		return -1;
	}

	return WRITERING_DMA(pMem, qwc);
}

static __forceinline void GIFchain()
{
	Registers::Freeze();
	// qwc check now done outside this function
	// Voodoocycles
	// >> 2 so Drakan and Tekken 5 don't mess up in some PATH3 transfer. Cycles to interrupt were getting huge..
	/*if (gif->qwc)*/ gscycles+= ( _GIFchain() >> 2 ); /* guessing */
	Registers::Thaw();
}

static __forceinline bool checkTieBit(tDMA_TAG* &ptag)
{
	if (gif->chcr.TIE && ptag->IRQ)
	{
		GIF_LOG("dmaIrq Set");
		gspath3done = true;
		return true;
	}

	return false;
}

static __forceinline tDMA_TAG* ReadTag()
{
	tDMA_TAG* ptag = dmaGetAddr(gif->tadr);  //Set memory pointer to TADR

	if (!(gif->transfer("Gif", ptag))) return NULL;

	gif->madr = ptag[1]._u32;				    //MADR = ADDR field + SPR
	gscycles += 2; // Add 1 cycles from the QW read for the tag

	gspath3done = hwDmacSrcChainWithStack(gif, ptag->ID);
	return ptag;
}

static __forceinline tDMA_TAG* ReadTag2()
{
	tDMA_TAG* ptag = dmaGetAddr(gif->tadr);  //Set memory pointer to TADR

    gif->unsafeTransfer(ptag);
	gif->madr = ptag[1]._u32;

	gspath3done = hwDmacSrcChainWithStack(gif, ptag->ID);
	return ptag;
}

void GIFdma()
{
	tDMA_TAG *ptag;

	gscycles = prevcycles;

	if (gifRegs->ctrl.PSE)  // temporarily stop
	{
		Console.WriteLn("Gif dma temp paused?");
		return;
	}

	if ((dmacRegs->ctrl.STD == STD_GIF) && (prevcycles != 0))
	{
		Console.WriteLn("GS Stall Control Source = %x, Drain = %x\n MADR = %x, STADR = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3, gif->madr, psHu32(DMAC_STADR));

		if ((gif->madr + (gif->qwc * 16)) > dmacRegs->stadr.ADDR)
		{
			CPU_INT(DMAC_GIF, 4);
			gscycles = 0;
			return;
		}

		prevcycles = 0;
		gif->qwc = 0;
	}

	clearFIFOstuff(true);
	gifRegs->stat.FQC = max((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // OPH=1 | APATH=3]

	//Path2 gets priority in intermittent mode
	if (GSTransferStatus.PTH1 != STOPPED_MODE || GSTransferStatus.PTH2 != STOPPED_MODE)
	{
	    // We are in image mode doing DIRECTHL, Path 1 is in queue, and in intermittant mode.
		//GIF_LOG("Waiting VU %x, PATH2 %x, GIFMODE %x Progress %x", gifRegs->stat.P1Q, (vif1.cmd & 0x7f), gifRegs->mode._u32, GSTransferStatus.PTH3);
		/*if(GSTransferStatus.PTH3 == STOPPED_MODE)
		{
			
		} else Console.Warning("PATH3 Transfer in action while another one path is running, Path3 mode %x", GSTransferStatus.PTH3);*/
		gifRegs->stat.set_flags(GIF_STAT_P2Q);
		CPU_INT(DMAC_GIF, 16);
		return;
	}
	gifRegs->stat.clear_flags(GIF_STAT_P2Q);
	gifRegs->stat.APATH = GIF_APATH3;
	gifRegs->stat.OPH = true;

	if (vif1Regs->mskpath3 || gifRegs->mode.M3R)
	{
		if (gif->qwc == 0)
		{
			if ((gif->chcr.MOD == CHAIN_MODE) && gif->chcr.STR)
			{
			    ptag = ReadTag();
				gifRegs->stat.FQC = max((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // OPH=1 | APATH=3]
				if (ptag == NULL) return;
				GIF_LOG("PTH3 MASK gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx", ptag[1]._u32, ptag[0]._u32, gif->qwc, ptag->ID, gif->madr);

				//Check TIE bit of CHCR and IRQ bit of tag
				if (checkTieBit(ptag))  GIF_LOG("PATH3 MSK dmaIrq Set");
			}
		}

		if (GSTransferStatus.PTH3 == STOPPED_MODE) /*|| (vif1Regs->stat._u32 |= VIF1_STAT_VGW) == 0*/
		{
			GIF_LOG("PTH3 MASK Paused by VIF");
			vif1Regs->stat.VGW = false;
			if (gif->qwc == 0) CPU_INT(DMAC_GIF, 16);
			return;
		}
		gifRegs->stat.FQC = max((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // OPH=1 | APATH=3]
		//Check with Path3 masking games
		if (gif->qwc > 0) {
			GIF_LOG("PTH3 MASK Transferring");
			GIFchain();
			CPU_INT(DMAC_GIF, gscycles * BIAS);
			return;
		}
		//else DevCon.WriteLn("GIFdma() case 1, but qwc = 0!"); //Don't do 0 GIFchain and then return
	}

	// Transfer Dn_QWC from Dn_MADR to GIF
	if ((gif->chcr.MOD == NORMAL_MODE) || (gif->qwc > 0)) // Normal Mode
	{

		if ((dmacRegs->ctrl.STD == STD_GIF) && (gif->chcr.MOD == NORMAL_MODE))
		{
			Console.WriteLn("DMA Stall Control on GIF normal");
		}
		gifRegs->stat.FQC = max((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // OPH=1 | APATH=3]
		//Check with Path3 masking games
		if (gif->qwc > 0) {
			GIFchain();	//Transfers the data set by the switch
			CPU_INT(DMAC_GIF, gscycles * BIAS);
			return;
		} else { //Else it really is a normal transfer and we want to quit, else it gets confused with chains
			gspath3done = 1;
		}

		//else DevCon.WriteLn("GIFdma() case 2, but qwc = 0!"); //Don't do 0 GIFchain and then return, fixes Dual Hearts
	}

	if ((gif->chcr.MOD == CHAIN_MODE) && (!gspath3done)) // Chain Mode
	{
        ptag = ReadTag();
        if (ptag == NULL) return;
		GIF_LOG("gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx", ptag[1]._u32, ptag[0]._u32, gif->qwc, ptag->ID, gif->madr);
		gifRegs->stat.FQC = max((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // OPH=1 | APATH=3]
		if (dmacRegs->ctrl.STD == STD_GIF)
		{
			// there are still bugs, need to also check if gif->madr +16*qwc >= stadr, if not, stall
			if (!gspath3done && ((gif->madr + (gif->qwc * 16)) > dmacRegs->stadr.ADDR) && (ptag->ID == TAG_REFS))
			{
				// stalled. 
				// We really need to test this. Pay attention to prevcycles, as it used to trigger GIFchains in the code above. (rama)
				Console.WriteLn("GS Stall Control start Source = %x, Drain = %x\n MADR = %x, STADR = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3,gif->madr, psHu32(DMAC_STADR));
				prevcycles = gscycles;
				//gif->tadr -= 16;
				hwDmacIrq(DMAC_STALL_SIS);
				CPU_INT(DMAC_GIF, gscycles);
				gscycles = 0;
				return;
			}
		}

		checkTieBit(ptag);
	}

	prevcycles = 0;

	if ((!gspath3done) && (gif->qwc == 0))
	{
		ptag = dmaGetAddr(gif->tadr);  //Set memory pointer to TADR
        gif->unsafeTransfer(ptag);
		gif->madr = ptag[1]._u32;

		gspath3done = hwDmacSrcChainWithStack(gif, ptag->ID);

		checkTieBit(ptag);

		GIF_LOG("gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx", ptag[1]._u32, ptag[0]._u32, gif->qwc, ptag->ID, gif->madr);
		CPU_INT(DMAC_GIF, gscycles * BIAS);
	}
	else
	{
		CPU_INT(DMAC_GIF, gscycles * BIAS);
		gscycles = 0;
	}
	gifRegs->stat.FQC = max((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // OPH=1 | APATH=3]
}

void dmaGIF()
{
	 //We used to add wait time for the buffer to fill here, fixing some timing problems in path 3 masking
	//It takes the time of 24 QW for the BUS to become ready - The Punisher And Streetball
	GIF_LOG("dmaGIFstart chcr = %lx, madr = %lx, qwc  = %lx\n tadr = %lx, asr0 = %lx, asr1 = %lx", gif->chcr._u32, gif->madr, gif->qwc, gif->tadr, gif->asr0, gif->asr1);

	GSTransferStatus.PTH3 = STOPPED_MODE;
	gspath3done = false; // For some reason this doesn't clear? So when the system starts the thread, we will clear it :)

	gifRegs->stat.P3Q = true;
	gifRegs->stat.FQC |= 0x10; // hack ;)

	clearFIFOstuff(true);

	if (dmacRegs->ctrl.MFD == MFD_GIF)  // GIF MFIFO
	{
		//Console.WriteLn("GIF MFIFO");
		gifMFIFOInterrupt();
		return;
	}

	if ((gif->qwc == 0) && (gif->chcr.MOD != NORMAL_MODE))
	{
		tDMA_TAG* ptag = ReadTag2();
		GIF_LOG("gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx", ptag[1]._u32, ptag[0]._u32, gif->qwc, ptag->ID, gif->madr);

		checkTieBit(ptag);
		GIFdma();
		return;
	}

	//Halflife sets a QWC amount in chain mode, no tadr set.
	if (gif->qwc > 0) gspath3done = true;

	GIFdma();
}

// called from only one location, so forceinline it:
static __forceinline bool mfifoGIFrbTransfer()
{
	u32 mfifoqwc = min(gifqwc, (u32)gif->qwc);
	u32 *src;

	/* Check if the transfer should wrap around the ring buffer */
	if ((gif->madr + mfifoqwc * 16) > (dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK + 16))
	{
		uint s1 = ((dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK + 16) - gif->madr) >> 4;
		uint s2 = (mfifoqwc - s1);
		// fixme - I don't think these should use WRITERING_DMA, since our source
		// isn't the DmaGetAddr(gif->madr) address that WRITERING_DMA expects.

		/* it does (wrap around), so first copy 's1' bytes from 'addr' to 'data' */
		src = (u32*)PSM(gif->madr);
		if (src == NULL) return false;
		s1 = WRITERING_DMA(src, s1);

		if (s1 == (mfifoqwc - s2))
		{
			/* and second copy 's2' bytes from 'maddr' to '&data[s1]' */
			src = (u32*)PSM(dmacRegs->rbor.ADDR);
			if (src == NULL) return false;
			s2 = WRITERING_DMA(src, s2);
		}
		else
		{
			s2 = 0;
		}

		mfifoqwc = s1 + s2;
	}
	else
	{
		/* it doesn't, so just transfer 'qwc*16' words from 'gif->madr' to GS */
		src = (u32*)PSM(gif->madr);
		if (src == NULL) return false;
		mfifoqwc = WRITERING_DMA(src, mfifoqwc);
		gif->madr = dmacRegs->rbor.ADDR + (gif->madr & dmacRegs->rbsr.RMSK);
	}

	gifqwc -= mfifoqwc;

	return true;
}

// called from only one location, so forceinline it:
static __forceinline bool mfifoGIFchain()
{
	/* Is QWC = 0? if so there is nothing to transfer */
	if (gif->qwc == 0) return true;

	if (gif->madr >= dmacRegs->rbor.ADDR &&
		gif->madr <= (dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK))
	{
		if (!mfifoGIFrbTransfer()) return false;
	}
	else
	{
		int mfifoqwc;

		tDMA_TAG *pMem = dmaGetAddr(gif->madr);
		if (pMem == NULL) return false;

		mfifoqwc = WRITERING_DMA(pMem, gif->qwc);
		mfifocycles += (mfifoqwc) * 2; /* guessing */
	}

	return true;
}

static u32 qwctag(u32 mask)
{
	return (dmacRegs->rbor.ADDR + (mask & dmacRegs->rbsr.RMSK));
}

void mfifoGIFtransfer(int qwc)
{
	tDMA_TAG *ptag;

	mfifocycles = 0;
	gifmfifoirq = false;

	if(qwc > 0 )
	{
		gifqwc += qwc;

		if (!(gifstate & GIF_STATE_EMPTY)) return;
		// if (gifempty == false) return;
		gifstate &= ~GIF_STATE_EMPTY;
		gifempty = false;
	}

	GIF_LOG("mfifoGIFtransfer %x madr %x, tadr %x", gif->chcr._u32, gif->madr, gif->tadr);

	if (gif->qwc == 0)
	{
		if (gif->tadr == spr0->madr)
		{
			//if( gifqwc > 1 ) DevCon.WriteLn("gif mfifo tadr==madr but qwc = %d", gifqwc);
			hwDmacIrq(DMAC_MFIFO_EMPTY);
			gifstate |= GIF_STATE_EMPTY;
			gifempty = true;
			return;
		}

		gif->tadr = qwctag(gif->tadr);

		ptag = dmaGetAddr(gif->tadr);
		gif->unsafeTransfer(ptag);
		gif->madr = ptag[1]._u32;
		
		mfifocycles += 2;

		GIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx mfifo qwc = %x spr0 madr = %x",
				ptag[1]._u32, ptag[0]._u32, gif->qwc, ptag->ID, gif->madr, gif->tadr, gifqwc, spr0->madr);

		gifqwc--;

		switch (ptag->ID)
		{
			case TAG_REFE: // Refe - Transfer Packet According to ADDR field
				gif->tadr = qwctag(gif->tadr + 16);
				gifstate = GIF_STATE_DONE;										//End Transfer
				break;

			case TAG_CNT: // CNT - Transfer QWC following the tag.
				gif->madr = qwctag(gif->tadr + 16);						//Set MADR to QW after Tag
				gif->tadr = qwctag(gif->madr + (gif->qwc << 4));			//Set TADR to QW following the data
				gifstate = GIF_STATE_READY;
				break;

			case TAG_NEXT: // Next - Transfer QWC following tag. TADR = ADDR
			{
				u32 temp = gif->madr;								//Temporarily Store ADDR
				gif->madr = qwctag(gif->tadr + 16); 					  //Set MADR to QW following the tag
				gif->tadr = temp;								//Copy temporarily stored ADDR to Tag
				gifstate = GIF_STATE_READY;
				break;
			}

			case TAG_REF: // Ref - Transfer QWC from ADDR field
			case TAG_REFS: // Refs - Transfer QWC from ADDR field (Stall Control)
				gif->tadr = qwctag(gif->tadr + 16);							//Set TADR to next tag
				gifstate = GIF_STATE_READY;
				break;

			case TAG_END: // End - Transfer QWC following the tag
				gif->madr = qwctag(gif->tadr + 16);		//Set MADR to data following the tag
				gif->tadr = qwctag(gif->madr + (gif->qwc << 4));			//Set TADR to QW following the data
				gifstate = GIF_STATE_DONE;						//End Transfer
				break;
			}

		if ((gif->chcr.TIE) && (ptag->IRQ))
		{
			SPR_LOG("dmaIrq Set");
			gifstate = GIF_STATE_DONE;
			gifmfifoirq = true;
		}
	 }

	Registers::Freeze();
	if (!mfifoGIFchain())
	{
		Console.WriteLn("GIF dmaChain error size=%d, madr=%lx, tadr=%lx", gif->qwc, gif->madr, gif->tadr);
		gifstate = GIF_STATE_STALL;
	}
	Registers::Thaw();

	if ((gif->qwc == 0) && (gifstate & GIF_STATE_DONE)) gifstate = GIF_STATE_STALL;
	CPU_INT(11,mfifocycles);

	SPR_LOG("mfifoGIFtransfer end %x madr %x, tadr %x", gif->chcr._u32, gif->madr, gif->tadr);
}

void gifMFIFOInterrupt()
{
    //Console.WriteLn("gifMFIFOInterrupt");
	mfifocycles = 0;

	if (GSTransferStatus.PTH3 == STOPPED_MODE)
	{
	     gifRegs->stat.APATH = GIF_APATH_IDLE;
	     gifRegs->stat.OPH = false;
	}

	if ((spr0->chcr.STR) && (spr0->qwc == 0))
	{
		spr0->chcr.STR = false;
		hwDmacIrq(DMAC_FROM_SPR);
	}

	if (!(gif->chcr.STR))
	{
		Console.WriteLn("WTF GIFMFIFO");
		cpuRegs.interrupt &= ~(1 << 11);
		return;
	}

	if ((gifRegs->stat.P1Q || (vif1.cmd & 0x7f) == 0x50) && gifRegs->mode.IMT && GSTransferStatus.PTH3 == IMAGE_MODE) //Path2 gets priority in intermittent mode
	{
		//GIF_LOG("Waiting VU %x, PATH2 %x, GIFMODE %x Progress %x", psHu32(GIF_STAT) & 0x100, (vif1.cmd & 0x7f), psHu32(GIF_MODE), GSTransferStatus.PTH3);
		CPU_INT(11,mfifocycles);
		return;
	}

	if (!(gifstate & GIF_STATE_STALL))
	{
		if (gifqwc <= 0)
		{
			//Console.WriteLn("Empty");
			hwDmacIrq(DMAC_MFIFO_EMPTY);
			gifstate |= GIF_STATE_EMPTY;
			gifempty = true;

			gifRegs->stat.IMT = false;
			return;
		}
		mfifoGIFtransfer(0);
		return;
	}

#ifdef PCSX2_DEVBUILD
	if ((gifstate & GIF_STATE_READY) || (gif->qwc > 0))
	{
		Console.Error("gifMFIFO Panic > Shouldn't go here!");
		return;
	}
#endif
	//if(gifqwc > 0) Console.WriteLn("GIF MFIFO ending with stuff in it %x", gifqwc);
	if (!gifmfifoirq) gifqwc = 0;

	gspath3done = false;
	gscycles = 0;

	gifRegs->stat.clear_flags(GIF_STAT_APATH3 | GIF_STAT_OPH | GIF_STAT_P3Q | GIF_STAT_FQC); // OPH, APATH, P3Q,  FQC = 0

	vif1Regs->stat.VGW = false;
	gif->chcr.STR = false;
	gifstate = GIF_STATE_READY;
	hwDmacIrq(DMAC_GIF);
	clearFIFOstuff(false);
}

void SaveStateBase::gifFreeze()
{
	FreezeTag( "GIFdma" );

	Freeze( gifstate );
	Freeze( gifqwc );
	Freeze( gspath3done );
	Freeze( gscycles );
	//Freeze(gifempty);
	// Note: mfifocycles is not a persistent var, so no need to save it here.
}
