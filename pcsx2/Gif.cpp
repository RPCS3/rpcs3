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
#include "VU.h"
#include "GS.h"
#include "Gif.h"
#include "iR5900.h"
#include "Counters.h"

#include "VifDma.h"
#include "Tags.h"

using std::min;

// A three-way toggle used to determine if the GIF is stalling (transferring) or done (finished).
// Should be a gifstate_t rather then int, but I don't feel like possibly interfering with savestates right now.
static int gifstate = GIF_STATE_READY;

//static u64 s_gstag = 0; // used for querying the last tag

// This should be a bool. Next time I feel like breaking the save state, it will be. --arcum42
static bool gspath3done = false;

static u32 gscycles = 0, prevcycles = 0, mfifocycles = 0;
static u32 gifqwc = 0;
bool gifmfifoirq = false;

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
		//Console::WriteLn("Eh? why are you still interrupting! chcr %x, qwc %x, done = %x", gif->chcr._u32, gif->qwc, done);
		return;
	}

	if ((vif1.cmd & 0x7f) == 0x51)
	{
		if (Path3progress != IMAGE_MODE) vif1Regs->stat &= ~VIF1_STAT_VGW;
	}

	if (Path3progress == STOPPED_MODE) gifRegs->stat._u32 &= ~(GIF_STAT_APATH3 | GIF_STAT_OPH); // OPH=0 | APATH=0

	if ((gif->qwc > 0) || (!gspath3done)) 
	{
		if (!dmacRegs->ctrl.DMAE) 
		{
			Console::Notice("gs dma masked, re-scheduling...");
			// re-raise the int shortly in the future
			CPU_INT( 2, 64 );
			return;
		}

		GIFdma();
		return;
	}

	gspath3done = false;
	gscycles = 0;
	gif->chcr.STR = 0;
	vif1Regs->stat &= ~VIF1_STAT_VGW;
	
	gifRegs->stat._u32 &= ~(GIF_STAT_APATH3 | GIF_STAT_OPH | GIF_STAT_P3Q | GIF_STAT_FQC); 
	
	clearFIFOstuff(false);
	hwDmacIrq(DMAC_GIF);
	GIF_LOG("GIF DMA end");

}

static u32 WRITERING_DMA(u32 *pMem, u32 qwc)
{ 
	psHu32(GIF_STAT) |= GIF_STAT_APATH3 | GIF_STAT_OPH;         

	int size   = mtgsThread->PrepDataPacket(GIF_PATH_3, pMem, qwc);
	u8* pgsmem = mtgsThread->GetDataPacketPtr();

	memcpy_aligned(pgsmem, pMem, size<<4); 
	
	mtgsThread->SendDataPacket();
	return size;
} 

int  _GIFchain() 
{
	u32 qwc = min( gifsplit, (int)gif->qwc );
	u32 *pMem;

	pMem = (u32*)dmaGetAddr(gif->madr);
	if (pMem == NULL) 
	{
		// reset path3, fixes dark cloud 2
		gsGIFSoftReset(4);

		//must increment madr and clear qwc, else it loops
		gif->madr += gif->qwc * 16;
		gif->qwc = 0;
		Console::Notice( "Hackfix - NULL GIFchain" );
		return -1;
	}

	return WRITERING_DMA(pMem, qwc);
}

static __forceinline void GIFchain() 
{
	FreezeRegs(1);  
	if (gif->qwc) gscycles+= _GIFchain(); /* guessing */
	FreezeRegs(0); 
}

static __forceinline bool checkTieBit(u32* &ptag)
{
	if (gif->chcr.TIE && (Tag::IRQ(ptag)))  //Check TIE bit of CHCR and IRQ bit of tag
	{
		GIF_LOG("dmaIrq Set");
		gspath3done = true;
		return true;
	}
	
	return false;
}

static __forceinline bool ReadTag(u32* &ptag, u32 &id)
{
	ptag = (u32*)dmaGetAddr(gif->tadr);  //Set memory pointer to TADR
	
	if (!(Tag::Transfer("Gif", gif, ptag))) return false;
	gif->madr = ptag[1];				    //MADR = ADDR field
		
	id = Tag::Id(ptag);		//ID for DmaChain copied from bit 28 of the tag
	gscycles += 2; // Add 1 cycles from the QW read for the tag
			
	gspath3done = hwDmacSrcChainWithStack(gif, id);
	return true;
}

static __forceinline void ReadTag2(u32* &ptag)
{
	ptag = (u32*)dmaGetAddr(gif->tadr);  //Set memory pointer to TADR
	
	Tag::UnsafeTransfer(gif, ptag);
	gif->madr = ptag[1];

	gspath3done = hwDmacSrcChainWithStack(gif, Tag::Id(ptag));
}

void GIFdma() 
{
	u32 *ptag;
	u32 id;
	
	gscycles = prevcycles;

	if (gifRegs->ctrl.PSE)  // temporarily stop
	{
		Console::WriteLn("Gif dma temp paused?");
		return;
	}

	if ((dmacRegs->ctrl.STD == STD_GIF) && (prevcycles != 0))
	{
		Console::WriteLn("GS Stall Control Source = %x, Drain = %x\n MADR = %x, STADR = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3, gif->madr, psHu32(DMAC_STADR));

		if ((gif->madr + (gif->qwc * 16)) > dmacRegs->stadr.ADDR) 
		{
			CPU_INT(2, gscycles);
			gscycles = 0;
			return;
		}
		
		prevcycles = 0;
		gif->qwc = 0;
	}

	clearFIFOstuff(true);
	gifRegs->stat.FQC |= 0x10;// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // OPH=1 | APATH=3]
	
	//Path2 gets priority in intermittent mode
	if ((gifRegs->stat.P1Q || (vif1.cmd & 0x7f) == 0x50) && gifRegs->mode.IMT && (Path3progress == IMAGE_MODE)) 
	{
		GIF_LOG("Waiting VU %x, PATH2 %x, GIFMODE %x Progress %x", gifRegs->stat.P1Q, (vif1.cmd & 0x7f), gifRegs->mode._u32, Path3progress);
		CPU_INT(2, 16);
		return;
	}

	if (vif1Regs->mskpath3 || gifRegs->mode.M3R) 
	{
		if (gif->qwc == 0) 
		{
			if ((gif->chcr.MOD == CHAIN_MODE) && gif->chcr.STR)
			{
				if (!ReadTag(ptag, id)) return;
				GIF_LOG("PTH3 MASK gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx", ptag[1], ptag[0], gif->qwc, id, gif->madr);

				//Check TIE bit of CHCR and IRQ bit of tag
				if (checkTieBit(ptag))  GIF_LOG("PATH3 MSK dmaIrq Set");
			}
		} 
		 
		if (Path3progress == STOPPED_MODE) /*|| (vif1Regs->stat |= VIF1_STAT_VGW) == 0*/
		{
			vif1Regs->stat &= ~VIF1_STAT_VGW;
			if (gif->qwc == 0) CPU_INT(2, 16);
			return;
		}

		GIFchain();
		CPU_INT(2, gscycles * BIAS);
		return;
	}

	// Transfer Dn_QWC from Dn_MADR to GIF
	if ((gif->chcr.MOD == NORMAL_MODE) || (gif->qwc > 0)) // Normal Mode
	{ 
		
		if ((dmacRegs->ctrl.STD == STD_GIF) && (gif->chcr.MOD == NORMAL_MODE))
		{ 
			Console::WriteLn("DMA Stall Control on GIF normal");
		}

		GIFchain();	//Transfers the data set by the switch
				 
		CPU_INT(2, gscycles * BIAS);
		return;	
	}
	
	if ((gif->chcr.MOD == CHAIN_MODE) && (!gspath3done)) // Chain Mode
	{
		if (!ReadTag(ptag, id)) return;
		GIF_LOG("gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx", ptag[1], ptag[0], gif->qwc, id, gif->madr);

		if (dmacRegs->ctrl.STD == STD_GIF)
		{ 
			// there are still bugs, need to also check if gif->madr +16*qwc >= stadr, if not, stall
			if (!gspath3done && ((gif->madr + (gif->qwc * 16)) > dmacRegs->stadr.ADDR) && (id == 4)) 
			{
				// stalled
				Console::WriteLn("GS Stall Control Source = %x, Drain = %x\n MADR = %x, STADR = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3,gif->madr, psHu32(DMAC_STADR));
				prevcycles = gscycles;
				gif->tadr -= 16;
				hwDmacIrq(DMAC_STALL_SIS);
				CPU_INT(2, gscycles);
				gscycles = 0;
				return;
			}
		}
			
		checkTieBit(ptag);
	}

	prevcycles = 0;
	
	if ((!gspath3done) && (gif->qwc == 0))
	{
		ptag = (u32*)dmaGetAddr(gif->tadr);  //Set memory pointer to TADR
		
		Tag::UnsafeTransfer(gif, ptag);
		gif->madr = ptag[1];

		gspath3done = hwDmacSrcChainWithStack(gif, Tag::Id(ptag));
		
		checkTieBit(ptag);
		
		GIF_LOG("gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx", ptag[1], ptag[0], gif->qwc, (ptag[0] >> 28) & 0x7, gif->madr);
		CPU_INT(2, gscycles * BIAS);
	} 
	else 
	{
		CPU_INT(2, gscycles * BIAS);
		gscycles = 0;
	}
}

void dmaGIF() 
{
	 //We used to add wait time for the buffer to fill here, fixing some timing problems in path 3 masking
	//It takes the time of 24 QW for the BUS to become ready - The Punisher And Streetball
	GIF_LOG("dmaGIFstart chcr = %lx, madr = %lx, qwc  = %lx\n tadr = %lx, asr0 = %lx, asr1 = %lx", gif->chcr._u32, gif->madr, gif->qwc, gif->tadr, gif->asr0, gif->asr1);

	Path3progress = STOPPED_MODE;
	gspath3done = false; // For some reason this doesn't clear? So when the system starts the thread, we will clear it :)

	gifRegs->stat.P3Q = 1;
	gifRegs->stat.FQC |= 0x10;// FQC=31, hack ;) ( 31? 16! arcum42) [used to be 0xE00; // OPH=1 | APATH=3]

	clearFIFOstuff(true);
	
	if (dmacRegs->ctrl.MFD == MFD_GIF)  // GIF MFIFO
	{
		//Console::WriteLn("GIF MFIFO");
		gifMFIFOInterrupt();
		return;
	}	

	if ((gif->qwc == 0) && (gif->chcr.MOD != NORMAL_MODE))
	{
		u32 *ptag;
		
		ReadTag2(ptag);
		GIF_LOG("gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx", ptag[1], ptag[0], gif->qwc, (ptag[0] >> 28), gif->madr);

		checkTieBit(ptag);
		GIFdma();
		return;
	}

	//Halflife sets a QWC amount in chain mode, no tadr set.
	if (gif->qwc > 0) gspath3done = true;
	
	GIFdma();
}

// called from only one location, so forceinline it:
static __forceinline int mfifoGIFrbTransfer() 
{
	u32 mfifoqwc = min(gifqwc, (u32)gif->qwc);
	u32 *src;

	/* Check if the transfer should wrap around the ring buffer */
	if ((gif->madr + mfifoqwc * 16) > (dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK + 16)) 
	{
		int s1 = ((dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK + 16) - gif->madr) >> 4;
		int s2 = (mfifoqwc - s1);
		// fixme - I don't think these should use WRITERING_DMA, since our source
		// isn't the DmaGetAddr(gif->madr) address that WRITERING_DMA expects.

		/* it does (wrap around), so first copy 's1' bytes from 'addr' to 'data' */
		src = (u32*)PSM(gif->madr);
		if (src == NULL) return -1;
		s1 = WRITERING_DMA(src, s1);

		if (s1 == (mfifoqwc - s2)) 
		{
			/* and second copy 's2' bytes from 'maddr' to '&data[s1]' */
			src = (u32*)PSM(dmacRegs->rbor.ADDR);
			if (src == NULL) return -1;
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
		if (src == NULL) return -1;
		mfifoqwc = WRITERING_DMA(src, mfifoqwc);
		gif->madr = dmacRegs->rbor.ADDR + (gif->madr & dmacRegs->rbsr.RMSK);
	}

	gifqwc -= mfifoqwc;

	return 0;
}

// called from only one location, so forceinline it:
static __forceinline int mfifoGIFchain() 
{	 
	/* Is QWC = 0? if so there is nothing to transfer */
	if (gif->qwc == 0) return 0;

	if (gif->madr >= dmacRegs->rbor.ADDR &&
		gif->madr <= (dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK)) 
	{
		if (mfifoGIFrbTransfer() == -1) return -1;
	} 
	else 
	{
		int mfifoqwc = gif->qwc;
		u32 *pMem = (u32*)dmaGetAddr(gif->madr);
		if (pMem == NULL) return -1;
		mfifoqwc = WRITERING_DMA(pMem, mfifoqwc);
		mfifocycles += (mfifoqwc) * 2; /* guessing */
	}

	return 0;
}

static u32 qwctag(u32 mask)
{
	return (dmacRegs->rbor.ADDR + (mask & dmacRegs->rbsr.RMSK));
}

void mfifoGIFtransfer(int qwc) 
{
	u32 *ptag;
	int id;
	u32 temp = 0;
	
	mfifocycles = 0;
	gifmfifoirq = false;

	if(qwc > 0 ) 
	{
		gifqwc += qwc;
		if (gifstate != GIF_STATE_EMPTY) return;
		gifstate &= ~GIF_STATE_EMPTY;
	}
	
	GIF_LOG("mfifoGIFtransfer %x madr %x, tadr %x", gif->chcr._u32, gif->madr, gif->tadr);
		
	if (gif->qwc == 0) 
	{
		if (gif->tadr == spr0->madr) 
		{
			//if( gifqwc > 1 ) DevCon::WriteLn("gif mfifo tadr==madr but qwc = %d", gifqwc);
			hwDmacIrq(DMAC_MFIFO_EMPTY);
			gifstate |= GIF_STATE_EMPTY;			
			return;
		}
		
		gif->tadr = qwctag(gif->tadr);
		ptag = (u32*)dmaGetAddr(gif->tadr);
			
		Tag::UnsafeTransfer(gif, ptag);
		
		gif->madr = ptag[1];
		id =Tag::Id(ptag);
		mfifocycles += 2;
			
		GIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx mfifo qwc = %x spr0 madr = %x",
				ptag[1], ptag[0], gif->qwc, id, gif->madr, gif->tadr, gifqwc, spr0->madr);

		gifqwc--;

		switch (id) 
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
				temp = gif->madr;								//Temporarily Store ADDR
				gif->madr = qwctag(gif->tadr + 16); 					  //Set MADR to QW following the tag
				gif->tadr = temp;								//Copy temporarily stored ADDR to Tag
				gifstate = GIF_STATE_READY;
				break;

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
			
		if ((gif->chcr.TIE) && (Tag::IRQ(ptag)))
		{
			SPR_LOG("dmaIrq Set");
			gifstate = GIF_STATE_DONE;
			gifmfifoirq = TRUE;
		}
	 }
	 
	FreezeRegs(1); 
	if (mfifoGIFchain() == -1) 
	{
		Console::WriteLn("GIF dmaChain error size=%d, madr=%lx, tadr=%lx", gif->qwc, gif->madr, gif->tadr);
		gifstate = GIF_STATE_STALL;
	}
	FreezeRegs(0); 
		
	if ((gif->qwc == 0) && (gifstate == GIF_STATE_DONE)) gifstate = GIF_STATE_STALL;
	CPU_INT(11,mfifocycles);
		
	SPR_LOG("mfifoGIFtransfer end %x madr %x, tadr %x", gif->chcr._u32, gif->madr, gif->tadr);	
}

void gifMFIFOInterrupt()
{
	mfifocycles = 0;
	if (Path3progress == STOPPED_MODE) psHu32(GIF_STAT)&= ~(GIF_STAT_APATH3 | GIF_STAT_OPH); // OPH=0 | APATH=0

	if ((spr0->chcr.STR) && (spr0->qwc == 0))
	{
		spr0->chcr.STR = 0;
		hwDmacIrq(DMAC_FROM_SPR);
	}

	if (!(gif->chcr.STR)) 
	{ 
		Console::WriteLn("WTF GIFMFIFO");
		cpuRegs.interrupt &= ~(1 << 11); 
		return; 
	}

	if ((gifRegs->stat.P1Q || (vif1.cmd & 0x7f) == 0x50) && gifRegs->mode.IMT && Path3progress == IMAGE_MODE) //Path2 gets priority in intermittent mode
	{
		//GIF_LOG("Waiting VU %x, PATH2 %x, GIFMODE %x Progress %x", psHu32(GIF_STAT) & 0x100, (vif1.cmd & 0x7f), psHu32(GIF_MODE), Path3progress);
		CPU_INT(11,mfifocycles);
		return;
	}	
	
	if (gifstate != GIF_STATE_STALL) 
	{
		if (gifqwc <= 0) 
		{
			//Console::WriteLn("Empty");
			gifstate |= GIF_STATE_EMPTY;
			gifRegs->stat.IMT = 0; // OPH=0 | APATH=0
			hwDmacIrq(DMAC_MFIFO_EMPTY);
			return;
		}
		mfifoGIFtransfer(0);
		return;
	}
	
#ifdef PCSX2_DEVBUILD
	if (gifstate == GIF_STATE_READY || gif->qwc > 0) 
	{
		Console::Error("gifMFIFO Panic > Shouldn't go here!");
		return;
	}
#endif
	//if(gifqwc > 0) Console::WriteLn("GIF MFIFO ending with stuff in it %x", gifqwc);
	if (!gifmfifoirq) gifqwc = 0;

	gspath3done = false;
	gscycles = 0;
	
	gifRegs->stat._u32 &= ~(GIF_STAT_APATH3 | GIF_STAT_OPH | GIF_STAT_P3Q | GIF_STAT_FQC); // OPH, APATH, P3Q,  FQC = 0
	
	vif1Regs->stat &= ~VIF1_STAT_VGW;
	gif->chcr.STR = 0;
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

	// Note: mfifocycles is not a persistent var, so no need to save it here.
}
