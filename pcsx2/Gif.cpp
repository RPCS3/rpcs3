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
extern bool SIGNAL_IMR_Pending;
void gsPath1Interrupt()
{
	//DevCon.Warning("Path1 flush W %x, R %x", Path1WritePos, Path1ReadPos);

	

	if((gifRegs.stat.APATH <= GIF_APATH1 || (gifRegs.stat.IP3 == true && gifRegs.stat.APATH == GIF_APATH3)) && Path1WritePos > 0 && !gifRegs.stat.PSE && SIGNAL_IMR_Pending == false)
	{
		gifRegs.stat.P1Q = false;

		if (uint size = (Path1WritePos - Path1ReadPos))
		{
			GetMTGS().PrepDataPacket(GIF_PATH_1, size);
			//DevCon.Warning("Flush Size = %x", size);
			while(size > 0 && SIGNAL_IMR_Pending == false)
			{
				uint count = GIFPath_CopyTag(GIF_PATH_1, ((u128*)Path1Buffer) + Path1ReadPos, size);
				Path1ReadPos += count;
				size -= count;

				if(GSTransferStatus.PTH1 == STOPPED_MODE)
				{	
					gifRegs.stat.OPH = false;
					gifRegs.stat.APATH = GIF_APATH_IDLE;
				}
			}
			GetMTGS().SendDataPacket();

			if(Path1ReadPos == Path1WritePos)
			{
				Path1WritePos = Path1ReadPos = 0;
			} 
			else 
			{
				//DevCon.Warning("Queue quitting early due to signal or EOP %x", size);
				gifRegs.stat.P1Q = true;
			}
		}
	}
	else
	{
		if(gifRegs.stat.PSE) DevCon.Warning("Path1 paused by GIF_CTRL");
		//DevCon.Warning("Looping??? IP3 %x APATH %x OPH %x", gifRegs.stat.IP3, gifRegs.stat.APATH, gifRegs.stat.OPH);
		//if(!(cpuRegs.interrupt & (1<<28)) && Path1WritePos > 0)CPU_INT(28, 128);
	}
	
}

extern bool SIGNAL_IMR_Pending;

__fi void gsInterrupt()
{
	GIF_LOG("gsInterrupt caught!");

	if (dmacRegs.ctrl.MFD == MFD_GIF)  // GIF MFIFO
	{
		//Console.WriteLn("GIF MFIFO");
		gifMFIFOInterrupt();
		return;
	}	

	if(SIGNAL_IMR_Pending == true)
	{
		//DevCon.Warning("Path 3 Paused");
		CPU_INT(DMAC_GIF, 128);
		return;
	}

	if(GSTransferStatus.PTH3 >= PENDINGSTOP_MODE && gifRegs.stat.APATH == GIF_APATH3 )
	{
		gifRegs.stat.OPH = false;
		GSTransferStatus.PTH3 = STOPPED_MODE;
		gifRegs.stat.APATH = GIF_APATH_IDLE;
		if(gifRegs.stat.P1Q) gsPath1Interrupt();
	}
	

	if (!(gifch.chcr.STR))
	{
		//Console.WriteLn("Eh? why are you still interrupting! chcr %x, qwc %x, done = %x", gifch.chcr._u32, gifch.qwc, done);
		return;
	}


	if ((gifch.qwc > 0) || (!gspath3done))
	{
		if (!dmacRegs.ctrl.DMAE)
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
	gifch.chcr.STR = false;

	////
	/*gifRegs.stat.OPH = false;
	GSTransferStatus.PTH3 = STOPPED_MODE;
	gifRegs.stat.APATH = GIF_APATH_IDLE;*/
	////
	gifRegs.stat.clear_flags(GIF_STAT_FQC);
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

	pMem = dmaGetAddr(gifch.madr, false);
	if (pMem == NULL)
	{
		// reset path3, fixes dark cloud 2
		GIFPath_Clear( GIF_PATH_3 );

		//must increment madr and clear qwc, else it loops
		gifch.madr += gifch.qwc * 16;
		gifch.qwc = 0;
		Console.Warning( "Hackfix - NULL GIFchain" );
		return -1;
	}

	return WRITERING_DMA(pMem, gifch.qwc);
}

static __fi void GIFchain()
{
	// qwc check now done outside this function
	// Voodoocycles
	// >> 2 so Drakan and Tekken 5 don't mess up in some PATH3 transfer. Cycles to interrupt were getting huge..
	/*if (gifch.qwc)*/ gscycles+= ( _GIFchain() * BIAS); /* guessing */
}

static __fi bool checkTieBit(tDMA_TAG* &ptag)
{
	if (gifch.chcr.TIE && ptag->IRQ)
	{
		GIF_LOG("dmaIrq Set");
		gspath3done = true;
		return true;
	}

	return false;
}

static __fi tDMA_TAG* ReadTag()
{
	tDMA_TAG* ptag = dmaGetAddr(gifch.tadr, false);  //Set memory pointer to TADR

	if (!(gifch.transfer("Gif", ptag))) return NULL;

	gifch.madr = ptag[1]._u32;				    //MADR = ADDR field + SPR
	gscycles += 2; // Add 1 cycles from the QW read for the tag

	gspath3done = hwDmacSrcChainWithStack(gifch, ptag->ID);
	return ptag;
}

static __fi tDMA_TAG* ReadTag2()
{
	tDMA_TAG* ptag = dmaGetAddr(gifch.tadr, false);  //Set memory pointer to TADR

    gifch.unsafeTransfer(ptag);
	gifch.madr = ptag[1]._u32;

	gspath3done = hwDmacSrcChainWithStack(gifch, ptag->ID);
	return ptag;
}

bool CheckPaths(int Channel)
{
	if(GSTransferStatus.PTH3 <= IMAGE_MODE && gifRegs.mode.IMT)
	{
		if((gifRegs.stat.P1Q == true || gifRegs.stat.P2Q == true) || (gifRegs.stat.APATH > GIF_APATH_IDLE && gifRegs.stat.APATH < GIF_APATH3))
		{
			if((vif1.cmd & 0x7f) != 0x51 || gifRegs.stat.P1Q == true)
			{
					//DevCon.Warning("GIF Stall 1 P1Q %x P2Q %x APATH %x PTH3 %x vif1cmd %x", gifRegs.stat.P1Q, gifRegs.stat.P2Q, gifRegs.stat.APATH, GSTransferStatus.PTH3, vif1.cmd);
					gifRegs.stat.IP3 = true;
					if(gifRegs.stat.P1Q) gsPath1Interrupt();
					CPU_INT(DMAC_GIF, 16);
					return false;
			}
		}
	}
	else if((GSTransferStatus.PTH3 == IDLE_MODE)|| (GSTransferStatus.PTH3 == STOPPED_MODE))
	{
		//This should cover both scenarios, as DIRECTHL doesn't gain priority when image mode is running (PENDINGIMAGE_MODE == fininshed).
		if((gifRegs.stat.P1Q == true || gifRegs.stat.P2Q == true) || (gifRegs.stat.APATH > GIF_APATH_IDLE && gifRegs.stat.APATH < GIF_APATH3))
		{
			//DevCon.Warning("GIF Stall 2 P1Q %x P2Q %x APATH %x PTH3 %x vif1cmd %x", gifRegs.stat.P1Q, gifRegs.stat.P2Q, gifRegs.stat.APATH, GSTransferStatus.PTH3, vif1.cmd);
			gifRegs.stat.IP3 = true;
			CPU_INT(DMAC_GIF, 16);
			return false;
		}
	}
	
	gifRegs.stat.IP3 = false;
	return true;
}

void GIFdma()
{
	tDMA_TAG *ptag;

	gscycles = prevcycles;

	if (gifRegs.ctrl.PSE)  // temporarily stop
	{
		Console.WriteLn("Gif dma temp paused? (non MFIFO GIF)");
		CPU_INT(DMAC_GIF, 16);
		return;
	}

	if ((dmacRegs.ctrl.STD == STD_GIF) && (prevcycles != 0))
	{
		//Console.WriteLn("GS Stall Control Source = %x, Drain = %x\n MADR = %x, STADR = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3, gifch.madr, psHu32(DMAC_STADR));

		if ((gifch.madr + (gifch.qwc * 16)) > dmacRegs.stadr.ADDR)
		{
			CPU_INT(DMAC_GIF, 4);
			gscycles = 0;
			return;
		}

		prevcycles = 0;
		gifch.qwc = 0;
	}

	clearFIFOstuff(true);
	gifRegs.stat.FQC = min((u16)0x10, gifch.qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]

	
	if (vif1Regs.mskpath3 || gifRegs.mode.M3R)
	{
		if (gifch.qwc == 0)
		{
			if ((gifch.chcr.MOD == CHAIN_MODE) && gifch.chcr.STR)
			{
				//DevCon.Warning("GIF Reading Tag Masked MSK = %x", vif1Regs.mskpath3);
			    ptag = ReadTag();
				gifRegs.stat.FQC = min((u16)0x10, gifch.qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]
				if (ptag == NULL) return;
				GIF_LOG("PTH3 MASK gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx tadr=%lx", ptag[1]._u32, ptag[0]._u32, gifch.qwc, ptag->ID, gifch.madr, gifch.tadr);

				//Check TIE bit of CHCR and IRQ bit of tag
				if (checkTieBit(ptag))  GIF_LOG("PATH3 MSK dmaIrq Set");
			}
		}
		

		if (GSTransferStatus.PTH3 == IDLE_MODE)
		{
			GIF_LOG("PTH3 MASK Paused by VIF QWC %x", gifch.qwc);
			
			//DevCon.Warning("GIF Paused by Mask MSK = %x", vif1Regs.mskpath3);
			
			if(gifch.qwc == 0) gsInterrupt();
			else gifRegs.stat.set_flags(GIF_STAT_P3Q);
			return;
		}
		

	 	
		gifRegs.stat.FQC = min((u16)0x10, gifch.qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]
		//Check with Path3 masking games
		if (gifch.qwc > 0) {
			gifRegs.stat.set_flags(GIF_STAT_P3Q);
			if(CheckPaths(DMAC_GIF) == false) return;
			gifRegs.stat.clear_flags(GIF_STAT_P3Q);
			GIF_LOG("PTH3 MASK Transferring");
			GIFchain();			
			/*if(GSTransferStatus.PTH3 == PENDINGSTOP_MODE && gifRegs.stat.APATH == GIF_APATH_IDLE) 
			{
				GSTransferStatus.PTH3 = STOPPED_MODE;
			}*/
		}//else DevCon.WriteLn("GIFdma() case 1, but qwc = 0!"); //Don't do 0 GIFchain and then return
		CPU_INT(DMAC_GIF, gscycles);	
		return;
		
	}
	
	// Transfer Dn_QWC from Dn_MADR to GIF
	if ((gifch.chcr.MOD == NORMAL_MODE) || (gifch.qwc > 0)) // Normal Mode
	{

		if ((dmacRegs.ctrl.STD == STD_GIF) && (gifch.chcr.MOD == NORMAL_MODE))
		{
			//Console.WriteLn("DMA Stall Control on GIF normal");
		}
		gifRegs.stat.FQC = min((u16)0x10, gifch.qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]
		//Check with Path3 masking games
		//DevCon.Warning("GIF Transferring Normal/ChainQWC MSK = %x", vif1Regs.mskpath3);
		
		
		
		if (gifch.qwc > 0) {
			gifRegs.stat.set_flags(GIF_STAT_P3Q);
			if(CheckPaths(DMAC_GIF) == false) return;
			gifRegs.stat.clear_flags(GIF_STAT_P3Q);
			GIFchain();	//Transfers the data set by the switch
			CPU_INT(DMAC_GIF, gscycles);
			return;
		} else DevCon.Warning("GIF Normalmode or QWC going to invalid case? CHCR %x", gifch.chcr._u32);

		//else DevCon.WriteLn("GIFdma() case 2, but qwc = 0!"); //Don't do 0 GIFchain and then return, fixes Dual Hearts
	}

	if ((gifch.chcr.MOD == CHAIN_MODE) && (!gspath3done)) // Chain Mode
	{
        ptag = ReadTag();
        if (ptag == NULL) return;
		//DevCon.Warning("GIF Reading Tag MSK = %x", vif1Regs.mskpath3);
		GIF_LOG("gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx tadr=%lx", ptag[1]._u32, ptag[0]._u32, gifch.qwc, ptag->ID, gifch.madr, gifch.tadr);
		gifRegs.stat.FQC = min((u16)0x10, gifch.qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]
		if (dmacRegs.ctrl.STD == STD_GIF)
		{
			// there are still bugs, need to also check if gifch.madr +16*qwc >= stadr, if not, stall
			if (!gspath3done && ((gifch.madr + (gifch.qwc * 16)) > dmacRegs.stadr.ADDR) && (ptag->ID == TAG_REFS))
			{
				// stalled.
				// We really need to test this. Pay attention to prevcycles, as it used to trigger GIFchains in the code above. (rama)
				//Console.WriteLn("GS Stall Control start Source = %x, Drain = %x\n MADR = %x, STADR = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3,gifch.madr, psHu32(DMAC_STADR));
				prevcycles = gscycles;
				//gifch.tadr -= 16;
				// Quake III revolution wants to see tadr move.
				// Simple Media System (homebrew) as well.
				// -16 also seems right (it shifts the bg image right if anything else).
				gifch.tadr -= 16;
				// Next line also needs to be here, according to ref
				gifch.qwc = 0;
				hwDmacIrq(DMAC_STALL_SIS);
				CPU_INT(DMAC_GIF, gscycles);
				gscycles = 0;
				return;
			}
		}

		checkTieBit(ptag);
		/*if(gifch.qwc == 0)
		{
			gsInterrupt();
			return;
		}*/
	}

	prevcycles = 0;
	CPU_INT(DMAC_GIF, gscycles);
	gifRegs.stat.FQC = min((u16)0x10, gifch.qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // OPH=1 | APATH=3]
}

void dmaGIF()
{
	 //We used to add wait time for the buffer to fill here, fixing some timing problems in path 3 masking
	//It takes the time of 24 QW for the BUS to become ready - The Punisher And Streetball
	//DevCon.Warning("dmaGIFstart chcr = %lx, madr = %lx, qwc  = %lx\n tadr = %lx, asr0 = %lx, asr1 = %lx", gifch.chcr._u32, gifch.madr, gifch.qwc, gifch.tadr, gifch.asr0, gifch.asr1);

	gspath3done = false; // For some reason this doesn't clear? So when the system starts the thread, we will clear it :)

	gifRegs.stat.FQC |= 0x10; // hack ;)

	if (gifch.chcr.MOD == NORMAL_MODE) { //Else it really is a normal transfer and we want to quit, else it gets confused with chains
		gspath3done = true;
	}
	clearFIFOstuff(true);

	if(gifch.chcr.MOD == CHAIN_MODE && gifch.qwc > 0) 
	{
		//DevCon.Warning(L"GIF QWC on Chain " + gifch.chcr.desc());
		if ((gifch.chcr.tag().ID == TAG_REFE) || (gifch.chcr.tag().ID == TAG_END))
		{
			gspath3done = true;
		}
	}

	

	gsInterrupt();
}

static u16 QWCinGIFMFIFO(u32 DrainADDR)
{
	u32 ret;
	

	GIF_LOG("GIF MFIFO Requesting %x QWC from the MFIFO Base %x, SPR MADR %x Drain %x", gifch.qwc, dmacRegs.rbor.ADDR, spr0ch.madr, DrainADDR);
	//Calculate what we have in the fifo.
	if(DrainADDR <= spr0ch.madr)
	{
		//Drain is below the tadr, calculate the difference between them
		ret = (spr0ch.madr - DrainADDR) >> 4;
	}
	else
	{
		u32 limit = dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16;
		//Drain is higher than SPR so it has looped round, 
		//calculate from base to the SPR tag addr and what is left in the top of the ring
		ret = ((spr0ch.madr - dmacRegs.rbor.ADDR) + (limit - DrainADDR)) >> 4;
	}
	GIF_LOG("%x Available of the %x requested", ret, gifch.qwc);
	if((s32)ret < 0) DevCon.Warning("GIF Returning %x!", ret);
	return ret;
}

// called from only one location, so forceinline it:
static __fi bool mfifoGIFrbTransfer()
{
	u16 mfifoqwc = min(QWCinGIFMFIFO(gifch.madr), gifch.qwc);
	u32 *src;

	if(mfifoqwc == 0) return true; //Lets skip all this, we don't have the data

	GetMTGS().PrepDataPacket(GIF_PATH_3, mfifoqwc);

	// TODO (minor optimization): The new GIFpath parser can do rather efficient wrapping of
	// its own internally now. We just need to groom a version of it that can wrap around MFIFO
	// memory similarly to how it wraps VU1 memory on PATH1.
	GIF_LOG("MFIFO QWC to Transfer %x", mfifoqwc);
	/* Check if the transfer should wrap around the ring buffer */
	if ( (gifch.madr + (mfifoqwc * 16)) > (dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16))
	{
		uint s1 = ((dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16) - gifch.madr) >> 4;
		uint s2 = (mfifoqwc - s1);
		GIF_LOG("Split transfer doing %x QWC of %x Total QWC", s1, mfifoqwc);
		/* it does (wrap around), so first copy 's1' bytes from 'addr' to 'data' */
		/* and second copy 's2' bytes from 'maddr' to '&data[s1]' */
		
		gifch.madr = dmacRegs.rbor.ADDR + (gifch.madr & dmacRegs.rbsr.RMSK);
		src = (u32*)PSM(gifch.madr);
		if (src == NULL) return false;
		uint copied = GIFPath_CopyTag(GIF_PATH_3, (u128*)src, s1);

		if (copied == s1)	// but only copy second if first didn't abort prematurely for some reason.
		{
			GIF_LOG("Transferring last %x QWC", s2);
			src = (u32*)PSM(dmacRegs.rbor.ADDR);
			gifch.madr = dmacRegs.rbor.ADDR;
			if (src == NULL) return false;
			copied += GIFPath_CopyTag(GIF_PATH_3, (u128*)src, s2);
		}

		mfifoqwc = copied;
		GIF_LOG("Copied %x QWC, %x QWC Left", mfifoqwc, gifch.qwc);
	}
	else
	{
		GIF_LOG("Direct MFIFO transfer doing %x Total QWC", mfifoqwc);
		/* it doesn't, so just transfer 'qwc*16' words from 'gifch.madr' to GS */
		
		gifch.madr = dmacRegs.rbor.ADDR + (gifch.madr & dmacRegs.rbsr.RMSK);
		src = (u32*)PSM(gifch.madr);
		if (src == NULL) return false;
		mfifoqwc = GIFPath_CopyTag(GIF_PATH_3, (u128*)src, mfifoqwc);
		GIF_LOG("%X QWC Copied direct %x QWC Left", mfifoqwc, gifch.qwc);
	}

	GetMTGS().SendDataPacket();
	//gifqwc -= mfifoqwc;
	mfifocycles += (mfifoqwc) * 2; /* guessing */

	return true;
}

// called from only one location, so forceinline it:
static __fi bool mfifoGIFchain()
{
	/* Is QWC = 0? if so there is nothing to transfer */
	if (gifch.qwc == 0) return true;

	if (gifch.madr >= dmacRegs.rbor.ADDR &&
		gifch.madr <= (dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16))
	{
		bool ret = true;
	//	if(gifch.madr == (dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16)) DevCon.Warning("Edge GIF");
		if (!mfifoGIFrbTransfer()) ret = false;
		if(QWCinGIFMFIFO(gifch.madr) == 0) gifstate |= GIF_STATE_EMPTY;

		//Make sure we wrap the addresses, dont want it being stuck outside the ring when reading from the ring!
		gifch.madr = dmacRegs.rbor.ADDR + (gifch.madr & dmacRegs.rbsr.RMSK);
		gifch.tadr = dmacRegs.rbor.ADDR + (gifch.tadr & dmacRegs.rbsr.RMSK); //Check this too, tadr can suffer the same issue.

		return ret;

	}
	else
	{
		int mfifoqwc;
		GIF_LOG("Non-MFIFO Location transfer doing %x Total QWC", gifch.qwc);
		tDMA_TAG *pMem = dmaGetAddr(gifch.madr, false);
		if (pMem == NULL) return false;

		mfifoqwc = WRITERING_DMA(pMem, gifch.qwc);
		mfifocycles += (mfifoqwc) * 2; /* guessing */
	}

	return true;
}

static u32 qwctag(u32 mask)
{
	return (dmacRegs.rbor.ADDR + (mask & dmacRegs.rbsr.RMSK));
}

void mfifoGIFtransfer(int qwc)
{
	tDMA_TAG *ptag;

	mfifocycles = 0;
	gifmfifoirq = false;

	if(qwc > 0 )
	{
		if ((gifstate & GIF_STATE_EMPTY))
		{
			if(gifch.chcr.STR == true && !(cpuRegs.interrupt & (1<<DMAC_MFIFO_GIF)))
				CPU_INT(DMAC_MFIFO_GIF, 4);

			gifstate &= ~GIF_STATE_EMPTY;			
		}
		
		gifRegs.stat.FQC = 16;
		return;
	}

	if (gifRegs.ctrl.PSE)  // temporarily stop
	{
		Console.WriteLn("Gif dma temp paused?");
		CPU_INT(DMAC_MFIFO_GIF, 16);
		return;
	}

	if (gifch.qwc == 0)
	{
		gifch.tadr = qwctag(gifch.tadr);

		ptag = dmaGetAddr(gifch.tadr, false);
		gifch.unsafeTransfer(ptag);
		gifch.madr = ptag[1]._u32;

		mfifocycles += 2;

		GIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx mfifo qwc = %x spr0 madr = %x",
				ptag[1]._u32, ptag[0]._u32, gifch.qwc, ptag->ID, gifch.madr, gifch.tadr, gifqwc, spr0ch.madr);

		gspath3done = hwDmacSrcChainWithStack(gifch, ptag->ID);

		if(gspath3done == true) gifstate = GIF_STATE_DONE;
		else gifstate = GIF_STATE_READY;

		if ((gifch.chcr.TIE) && (ptag->IRQ))
		{
			SPR_LOG("dmaIrq Set");
			gifstate = GIF_STATE_DONE;
			gifmfifoirq = true;
		}
		if (QWCinGIFMFIFO(gifch.tadr) == 0) gifstate |= GIF_STATE_EMPTY;
	 }	

	if (!mfifoGIFchain())
	{
		Console.WriteLn("GIF dmaChain error size=%d, madr=%lx, tadr=%lx", gifch.qwc, gifch.madr, gifch.tadr);
		gifstate = GIF_STATE_STALL;
	}

	if ((gifch.qwc == 0) && (gifstate & GIF_STATE_DONE)) gifstate |= GIF_STATE_STALL;
	CPU_INT(DMAC_MFIFO_GIF,mfifocycles);

	SPR_LOG("mfifoGIFtransfer end %x madr %x, tadr %x", gifch.chcr._u32, gifch.madr, gifch.tadr);
}

void gifMFIFOInterrupt()
{
    GIF_LOG("gifMFIFOInterrupt");
	mfifocycles = 0;

	if (dmacRegs.ctrl.MFD != MFD_GIF)
	{
		DevCon.Warning("Not in GIF MFIFO mode! Stopping GIF MFIFO");
		return;
	}

	if(SIGNAL_IMR_Pending == true)
	{
		//DevCon.Warning("Path 3 Paused");
		CPU_INT(DMAC_MFIFO_GIF, 128);
		return;
	}

	if(GSTransferStatus.PTH3 >= PENDINGSTOP_MODE && gifRegs.stat.APATH == GIF_APATH3 )
	{
		gifRegs.stat.OPH = false;
		GSTransferStatus.PTH3 = STOPPED_MODE;
		gifRegs.stat.APATH = GIF_APATH_IDLE;
		if(gifRegs.stat.P1Q) gsPath1Interrupt();
	}

	if((gifstate & GIF_STATE_EMPTY))
	{
		FireMFIFOEmpty();
		if(!(gifstate & GIF_STATE_STALL)) return;
	}

	if(CheckPaths(11) == false) return;

	if (!(gifch.chcr.STR))
	{
		Console.WriteLn("WTF GIFMFIFO");
		cpuRegs.interrupt &= ~(1 << 11);
		return;
	}

	if (!(gifstate & GIF_STATE_STALL))
	{
		if (QWCinGIFMFIFO(gifch.tadr) == 0) 
		{
			gifstate |= GIF_STATE_EMPTY;
			CPU_INT(DMAC_MFIFO_GIF, 4);
			return;
		}
		
		mfifoGIFtransfer(0);
		return;
	}

#ifdef PCSX2_DEVBUILD
	if ((gifstate & GIF_STATE_READY) || (gifch.qwc > 0))
	{
		Console.Error("gifMFIFO Panic > Shouldn't go here!");
		return;
	}
#endif
	//if(gifqwc > 0) Console.WriteLn("GIF MFIFO ending with stuff in it %x", gifqwc);
	if (!gifmfifoirq) gifqwc = 0;

	gspath3done = false;
	gscycles = 0;

	gifRegs.stat.clear_flags(GIF_STAT_APATH3 | GIF_STAT_P3Q | GIF_STAT_FQC); // APATH, P3Q,  FQC = 0

	vif1Regs.stat.VGW = false;
	gifch.chcr.STR = false;
	gifstate = GIF_STATE_READY;
	hwDmacIrq(DMAC_GIF);
	GIF_LOG("gifMFIFO End");
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
