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

//Just some temporary bits to store Path1 transfers if another is in progress.
__aligned16 u8 Path1Buffer[0x1000000];
u32 Path1WritePos = 0;
u32 Path1ReadPos = 0;

static __fi void clearFIFOstuff(bool full)
{
	if (full)
		CSRreg.FIFO = CSR_FIFO_FULL;
	else
		CSRreg.FIFO = CSR_FIFO_EMPTY;
}

void gsPath1Interrupt()
{
	//DevCon.Warning("Path1 flush W %x, R %x", Path1WritePos, Path1ReadPos);

	

	if((gifRegs->stat.APATH <= GIF_APATH1 || (gifRegs->stat.IP3 == true && gifRegs->stat.APATH == GIF_APATH3)) && Path1WritePos > 0 && !gifRegs->stat.PSE)
	{
		gifRegs->stat.P1Q = false;

		if (uint size = (Path1WritePos - Path1ReadPos))
		{
			GetMTGS().PrepDataPacket(GIF_PATH_1, size);
			//DevCon.Warning("Flush Size = %x", size);
			while(size > 0)
			{
				uint count = GIFPath_CopyTag(GIF_PATH_1, ((u128*)Path1Buffer) + Path1ReadPos, size);
				Path1ReadPos += count;
				size -= count;

				if(GSTransferStatus.PTH1 == STOPPED_MODE)
				{	
					gifRegs->stat.OPH = false;
					gifRegs->stat.APATH = GIF_APATH_IDLE;
				}
			}
			GetMTGS().SendDataPacket();

			if(Path1ReadPos == Path1WritePos)
			{
				Path1WritePos = Path1ReadPos = 0;
			}
		}
	}
	else
	{
		if(gifRegs->stat.PSE) DevCon.Warning("Path1 paused by GIF_CTRL");
		DevCon.Warning("Looping??? IP3 %x APATH %x OPH %x", gifRegs->stat.IP3, gifRegs->stat.APATH, gifRegs->stat.OPH);
		//if(!(cpuRegs.interrupt & (1<<28)) && Path1WritePos > 0)CPU_INT(28, 128);
	}
	
}

extern bool SIGNAL_IMR_Pending;

__fi void gsInterrupt()
{
	GIF_LOG("gsInterrupt: %8.8x", cpuRegs.cycle);

	if(SIGNAL_IMR_Pending == true)
	{
		//DevCon.Warning("Path 3 Paused");
		CPU_INT(DMAC_GIF, 128);
		return;
	}

	if(GSTransferStatus.PTH3 >= PENDINGSTOP_MODE && gifRegs->stat.APATH == GIF_APATH3 )
	{
		gifRegs->stat.OPH = false;
		GSTransferStatus.PTH3 = STOPPED_MODE;
		gifRegs->stat.APATH = GIF_APATH_IDLE;
		if(gifRegs->stat.P1Q) gsPath1Interrupt();
	}
	

	if (!(gif->chcr.STR))
	{
		//Console.WriteLn("Eh? why are you still interrupting! chcr %x, qwc %x, done = %x", gif->chcr._u32, gif->qwc, done);
		return;
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

	gspath3done = false;
	gscycles = 0;
	gif->chcr.STR = false;

	////
	/*gifRegs->stat.OPH = false;
	GSTransferStatus.PTH3 = STOPPED_MODE;
	gifRegs->stat.APATH = GIF_APATH_IDLE;*/
	////
	gifRegs->stat.clear_flags(GIF_STAT_FQC);
	clearFIFOstuff(false);
	hwDmacIrq(DMAC_GIF);
	//DevCon.Warning("GIF DMA end");
}

static u32 WRITERING_DMA(u32 *pMem, u32 qwc)
{
	GetMTGS().PrepDataPacket(GIF_PATH_3, qwc);
	uint size = GIFPath_CopyTag(GIF_PATH_3, (u128*)pMem, qwc );
	GetMTGS().SendDataPacket();
	return size;
}

static u32 WRITERING_DMA(tDMA_TAG *pMem, u32 qwc)
{
    return WRITERING_DMA((u32*)pMem, qwc);
}

int  _GIFchain()
{
	tDMA_TAG *pMem;

	pMem = dmaGetAddr(gif->madr, false);
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

	return WRITERING_DMA(pMem, gif->qwc);
}

static __fi void GIFchain()
{
	// qwc check now done outside this function
	// Voodoocycles
	// >> 2 so Drakan and Tekken 5 don't mess up in some PATH3 transfer. Cycles to interrupt were getting huge..
	/*if (gif->qwc)*/ gscycles+= ( _GIFchain() * BIAS); /* guessing */
}

static __fi bool checkTieBit(tDMA_TAG* &ptag)
{
	if (gif->chcr.TIE && ptag->IRQ)
	{
		GIF_LOG("dmaIrq Set");
		gspath3done = true;
		return true;
	}

	return false;
}

static __fi tDMA_TAG* ReadTag()
{
	tDMA_TAG* ptag = dmaGetAddr(gif->tadr, false);  //Set memory pointer to TADR

	if (!(gif->transfer("Gif", ptag))) return NULL;

	gif->madr = ptag[1]._u32;				    //MADR = ADDR field + SPR
	gscycles += 2; // Add 1 cycles from the QW read for the tag

	gspath3done = hwDmacSrcChainWithStack(gif, ptag->ID);
	return ptag;
}

static __fi tDMA_TAG* ReadTag2()
{
	tDMA_TAG* ptag = dmaGetAddr(gif->tadr, false);  //Set memory pointer to TADR

    gif->unsafeTransfer(ptag);
	gif->madr = ptag[1]._u32;

	gspath3done = hwDmacSrcChainWithStack(gif, ptag->ID);
	return ptag;
}

bool CheckPaths(int Channel)
{
	if(GSTransferStatus.PTH3 <= IMAGE_MODE && gifRegs->mode.IMT)
	{
		if((gifRegs->stat.P1Q == true || gifRegs->stat.P2Q == true) || (gifRegs->stat.APATH > GIF_APATH_IDLE && gifRegs->stat.APATH < GIF_APATH3))
		{
			if((vif1.cmd & 0x7f) != 0x51 || gifRegs->stat.P1Q == true)
			{
					gifRegs->stat.IP3 = true;
					if(gifRegs->stat.P1Q) gsPath1Interrupt();
					CPU_INT(DMAC_GIF, 16);
					return false;
			}
		}
	}
	else if((GSTransferStatus.PTH3 == IDLE_MODE)|| (GSTransferStatus.PTH3 == STOPPED_MODE))
	{
		//This should cover both scenarios, as DIRECTHL doesn't gain priority when image mode is running (PENDINGIMAGE_MODE == fininshed).
		if((gifRegs->stat.P1Q == true || gifRegs->stat.P2Q == true) || (gifRegs->stat.APATH > GIF_APATH_IDLE && gifRegs->stat.APATH < GIF_APATH3))
		{
			gifRegs->stat.IP3 = true;
			CPU_INT(DMAC_GIF, 16);
			return false;
		}
	}
	
	gifRegs->stat.IP3 = false;
	return true;
}

void GIFdma()
{
	tDMA_TAG *ptag;

	gscycles = prevcycles;

	if (gifRegs->ctrl.PSE)  // temporarily stop
	{
		Console.WriteLn("Gif dma temp paused? (non MFIFO GIF)");
		CPU_INT(DMAC_GIF, 16);
		return;
	}

	if ((dmacRegs->ctrl.STD == STD_GIF) && (prevcycles != 0))
	{
		//Console.WriteLn("GS Stall Control Source = %x, Drain = %x\n MADR = %x, STADR = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3, gif->madr, psHu32(DMAC_STADR));

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
	gifRegs->stat.FQC = min((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]

	
	if (vif1Regs->mskpath3 || gifRegs->mode.M3R)
	{
		if (gif->qwc == 0)
		{
			if ((gif->chcr.MOD == CHAIN_MODE) && gif->chcr.STR)
			{
				//DevCon.Warning("GIF Reading Tag Masked MSK = %x", vif1Regs->mskpath3);
			    ptag = ReadTag();
				gifRegs->stat.FQC = min((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]
				if (ptag == NULL) return;
				GIF_LOG("PTH3 MASK gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx tadr=%lx", ptag[1]._u32, ptag[0]._u32, gif->qwc, ptag->ID, gif->madr, gif->tadr);

				//Check TIE bit of CHCR and IRQ bit of tag
				if (checkTieBit(ptag))  GIF_LOG("PATH3 MSK dmaIrq Set");
			}
		}
		

		if (GSTransferStatus.PTH3 == IDLE_MODE)
		{
			GIF_LOG("PTH3 MASK Paused by VIF QWC %x", gif->qwc);
			
			//DevCon.Warning("GIF Paused by Mask MSK = %x", vif1Regs->mskpath3);
			
			if(gif->qwc == 0) gsInterrupt();
			else gifRegs->stat.set_flags(GIF_STAT_P3Q);
			return;
		}
		

	 	
		gifRegs->stat.FQC = min((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]
		//Check with Path3 masking games
		if (gif->qwc > 0) {
			gifRegs->stat.set_flags(GIF_STAT_P3Q);
			if(CheckPaths(DMAC_GIF) == false) return;
			gifRegs->stat.clear_flags(GIF_STAT_P3Q);
			GIF_LOG("PTH3 MASK Transferring");
			GIFchain();			
			/*if(GSTransferStatus.PTH3 == PENDINGSTOP_MODE && gifRegs->stat.APATH == GIF_APATH_IDLE) 
			{
				GSTransferStatus.PTH3 = STOPPED_MODE;
			}*/
		}//else DevCon.WriteLn("GIFdma() case 1, but qwc = 0!"); //Don't do 0 GIFchain and then return
		CPU_INT(DMAC_GIF, gscycles);	
		return;
		
	}
	
	// Transfer Dn_QWC from Dn_MADR to GIF
	if ((gif->chcr.MOD == NORMAL_MODE) || (gif->qwc > 0)) // Normal Mode
	{

		if ((dmacRegs->ctrl.STD == STD_GIF) && (gif->chcr.MOD == NORMAL_MODE))
		{
			//Console.WriteLn("DMA Stall Control on GIF normal");
		}
		gifRegs->stat.FQC = min((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]
		//Check with Path3 masking games
		//DevCon.Warning("GIF Transferring Normal/ChainQWC MSK = %x", vif1Regs->mskpath3);
		
		
		
		if (gif->qwc > 0) {
			gifRegs->stat.set_flags(GIF_STAT_P3Q);
			if(CheckPaths(DMAC_GIF) == false) return;
			gifRegs->stat.clear_flags(GIF_STAT_P3Q);
			GIFchain();	//Transfers the data set by the switch
			CPU_INT(DMAC_GIF, gscycles);
			return;
		} else DevCon.Warning("GIF Normalmode or QWC going to invalid case? CHCR %x", gif->chcr._u32);

		//else DevCon.WriteLn("GIFdma() case 2, but qwc = 0!"); //Don't do 0 GIFchain and then return, fixes Dual Hearts
	}

	if ((gif->chcr.MOD == CHAIN_MODE) && (!gspath3done)) // Chain Mode
	{
        ptag = ReadTag();
        if (ptag == NULL) return;
		//DevCon.Warning("GIF Reading Tag MSK = %x", vif1Regs->mskpath3);
		GIF_LOG("gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx tadr=%lx", ptag[1]._u32, ptag[0]._u32, gif->qwc, ptag->ID, gif->madr, gif->tadr);
		gifRegs->stat.FQC = min((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]
		if (dmacRegs->ctrl.STD == STD_GIF)
		{
			// there are still bugs, need to also check if gif->madr +16*qwc >= stadr, if not, stall
			if (!gspath3done && ((gif->madr + (gif->qwc * 16)) > dmacRegs->stadr.ADDR) && (ptag->ID == TAG_REFS))
			{
				// stalled.
				// We really need to test this. Pay attention to prevcycles, as it used to trigger GIFchains in the code above. (rama)
				//Console.WriteLn("GS Stall Control start Source = %x, Drain = %x\n MADR = %x, STADR = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3,gif->madr, psHu32(DMAC_STADR));
				prevcycles = gscycles;
				//gif->tadr -= 16;
				// Quake III revolution wants to see tadr move.
				// Simple Media System (homebrew) as well.
				// -16 also seems right (it shifts the bg image right if anything else).
				gif->tadr -= 16;
				// Next line also needs to be here, according to ref
				gif->qwc = 0;
				hwDmacIrq(DMAC_STALL_SIS);
				CPU_INT(DMAC_GIF, gscycles);
				gscycles = 0;
				return;
			}
		}

		checkTieBit(ptag);
		/*if(gif->qwc == 0)
		{
			gsInterrupt();
			return;
		}*/
	}

	prevcycles = 0;
	CPU_INT(DMAC_GIF, gscycles);
	gifRegs->stat.FQC = min((u16)0x10, gif->qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // OPH=1 | APATH=3]
}

void dmaGIF()
{
	 //We used to add wait time for the buffer to fill here, fixing some timing problems in path 3 masking
	//It takes the time of 24 QW for the BUS to become ready - The Punisher And Streetball
	//DevCon.Warning("dmaGIFstart chcr = %lx, madr = %lx, qwc  = %lx\n tadr = %lx, asr0 = %lx, asr1 = %lx", gif->chcr._u32, gif->madr, gif->qwc, gif->tadr, gif->asr0, gif->asr1);

	gspath3done = false; // For some reason this doesn't clear? So when the system starts the thread, we will clear it :)

	gifRegs->stat.FQC |= 0x10; // hack ;)

	if (gif->chcr.MOD == NORMAL_MODE) { //Else it really is a normal transfer and we want to quit, else it gets confused with chains
		gspath3done = true;
	}
	clearFIFOstuff(true);

	if(gif->chcr.MOD == CHAIN_MODE && gif->qwc > 0) 
	{
		//DevCon.Warning(L"GIF QWC on Chain " + gif->chcr.desc());
		if ((gif->chcr.tag().ID == TAG_REFE) || (gif->chcr.tag().ID == TAG_END))
		{
			gspath3done = true;
		}
	}

	if (dmacRegs->ctrl.MFD == MFD_GIF)  // GIF MFIFO
	{
		//Console.WriteLn("GIF MFIFO");
		gifMFIFOInterrupt();
		return;
	}	

	GIFdma();
}

// called from only one location, so forceinline it:
static __fi bool mfifoGIFrbTransfer()
{
	u32 mfifoqwc = min(gifqwc, (u32)gif->qwc);
	u32 *src;

	GetMTGS().PrepDataPacket(GIF_PATH_3, mfifoqwc);

	// TODO (minor optimization): The new GIFpath parser can do rather efficient wrapping of
	// its own internally now. We just need to groom a version of it that can wrap around MFIFO
	// memory similarly to how it wraps VU1 memory on PATH1.

	/* Check if the transfer should wrap around the ring buffer */
	if ((gif->madr + mfifoqwc * 16) > (dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK + 16))
	{
		uint s1 = ((dmacRegs->rbor.ADDR + dmacRegs->rbsr.RMSK + 16) - gif->madr) >> 4;
		uint s2 = (mfifoqwc - s1);

		/* it does (wrap around), so first copy 's1' bytes from 'addr' to 'data' */
		/* and second copy 's2' bytes from 'maddr' to '&data[s1]' */

		src = (u32*)PSM(gif->madr);
		if (src == NULL) return false;
		uint copied = GIFPath_CopyTag(GIF_PATH_3, (u128*)src, s1);

		if (copied == s1)	// but only copy second if first didn't abort prematurely for some reason.
		{
			src = (u32*)PSM(dmacRegs->rbor.ADDR);
			if (src == NULL) return false;
			copied += GIFPath_CopyTag(GIF_PATH_3, (u128*)src, s2);
		}

		mfifoqwc = copied;
	}
	else
	{
		/* it doesn't, so just transfer 'qwc*16' words from 'gif->madr' to GS */
		src = (u32*)PSM(gif->madr);
		if (src == NULL) return false;
		mfifoqwc = GIFPath_CopyTag(GIF_PATH_3, (u128*)src, mfifoqwc);
		gif->madr = dmacRegs->rbor.ADDR + (gif->madr & dmacRegs->rbsr.RMSK);
	}

	GetMTGS().SendDataPacket();
	gifqwc -= mfifoqwc;

	return true;
}

// called from only one location, so forceinline it:
static __fi bool mfifoGIFchain()
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

		tDMA_TAG *pMem = dmaGetAddr(gif->madr, false);
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

	if (gifRegs->ctrl.PSE)  // temporarily stop
	{
		Console.WriteLn("Gif dma temp paused?");
		CPU_INT(DMAC_MFIFO_GIF, 16);
		return;
	}

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

		ptag = dmaGetAddr(gif->tadr, false);
		gif->unsafeTransfer(ptag);
		gif->madr = ptag[1]._u32;

		mfifocycles += 2;

		GIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx mfifo qwc = %x spr0 madr = %x",
				ptag[1]._u32, ptag[0]._u32, gif->qwc, ptag->ID, gif->madr, gif->tadr, gifqwc, spr0->madr);

		gifqwc--;

		switch (ptag->ID)
		{
			case TAG_REFE:		// Refe - Transfer Packet According to ADDR field
				gif->tadr = qwctag(gif->tadr + 16);
				gifstate = GIF_STATE_DONE;										//End Transfer
				break;

			case TAG_CNT:		// CNT - Transfer QWC following the tag.
				gif->madr = qwctag(gif->tadr + 16);						//Set MADR to QW after Tag
				gif->tadr = qwctag(gif->madr + (gif->qwc << 4));		//Set TADR to QW following the data
				gifstate = GIF_STATE_READY;
				break;

			case TAG_NEXT:		// Next - Transfer QWC following tag. TADR = ADDR
			{
				u32 temp = gif->madr;									//Temporarily Store ADDR
				gif->madr = qwctag(gif->tadr + 16);						//Set MADR to QW following the tag
				gif->tadr = temp;										//Copy temporarily stored ADDR to Tag
				gifstate = GIF_STATE_READY;
				break;
			}

			case TAG_REF:		// Ref - Transfer QWC from ADDR field
			case TAG_REFS:		// Refs - Transfer QWC from ADDR field (Stall Control)
				gif->tadr = qwctag(gif->tadr + 16);							//Set TADR to next tag
				gifstate = GIF_STATE_READY;
				break;

			case TAG_END:		// End - Transfer QWC following the tag
				gif->madr = qwctag(gif->tadr + 16);					//Set MADR to data following the tag
				gif->tadr = qwctag(gif->madr + (gif->qwc << 4));	//Set TADR to QW following the data
				gifstate = GIF_STATE_DONE;							//End Transfer
				break;
			}

		if ((gif->chcr.TIE) && (ptag->IRQ))
		{
			SPR_LOG("dmaIrq Set");
			gifstate = GIF_STATE_DONE;
			gifmfifoirq = true;
		}
	 }

	if (!mfifoGIFchain())
	{
		Console.WriteLn("GIF dmaChain error size=%d, madr=%lx, tadr=%lx", gif->qwc, gif->madr, gif->tadr);
		gifstate = GIF_STATE_STALL;
	}

	if ((gif->qwc == 0) && (gifstate & GIF_STATE_DONE)) gifstate = GIF_STATE_STALL;
	CPU_INT(DMAC_MFIFO_GIF,mfifocycles);

	SPR_LOG("mfifoGIFtransfer end %x madr %x, tadr %x", gif->chcr._u32, gif->madr, gif->tadr);
}

void gifMFIFOInterrupt()
{
    //Console.WriteLn("gifMFIFOInterrupt");
	mfifocycles = 0;

	if(SIGNAL_IMR_Pending == true)
	{
		//DevCon.Warning("Path 3 Paused");
		CPU_INT(DMAC_MFIFO_GIF, 128);
		return;
	}

	if(GSTransferStatus.PTH3 == STOPPED_MODE && gifRegs->stat.APATH == GIF_APATH3 )
	{
		gifRegs->stat.OPH = false;
		gifRegs->stat.APATH = GIF_APATH_IDLE;
		if(gifRegs->stat.P1Q) gsPath1Interrupt();
	}

	if(CheckPaths(11) == false) return;

	if (!(gif->chcr.STR))
	{
		Console.WriteLn("WTF GIFMFIFO");
		cpuRegs.interrupt &= ~(1 << 11);
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

	gifRegs->stat.clear_flags(GIF_STAT_APATH3 | GIF_STAT_P3Q | GIF_STAT_FQC); // APATH, P3Q,  FQC = 0

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

	int bufsize = Path1WritePos - Path1ReadPos;
	Freeze(bufsize);

	if (IsSaving())
	{
		// We can just load the queued Path1 data into the front of the buffer, and
		// reset the ReadPos and WritePos accordingly.
		FreezeMem(Path1Buffer, bufsize);
		Path1ReadPos = 0;
		Path1WritePos = bufsize;
	}
	else
	{
		// Only want to save the actual Path1 data between readpos and writepos.  The
		// rest of the buffer is just unused-ness!
		FreezeMem(&Path1Buffer[Path1ReadPos], bufsize);
	}
}
