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
#include "Gif_Unit.h"
#include "Vif_Dma.h"

#include "iR5900.h"

using std::min;

// A three-way toggle used to determine if the GIF is stalling (transferring) or done (finished).
// Should be a gifstate_t rather then int, but I don't feel like possibly interfering with savestates right now.
static int  gifstate = GIF_STATE_READY;
static bool gspath3done = false;

static u32 gscycles = 0, prevcycles = 0, mfifocycles = 0;
static u32 gifqwc = 0;
static bool gifmfifoirq = false;

static __fi void clearFIFOstuff(bool full) {
	if (full) CSRreg.FIFO = CSR_FIFO_FULL;
	else      CSRreg.FIFO = CSR_FIFO_EMPTY;
}

void incGifChAddr(u32 qwc) {
	if (gifch.chcr.STR) {
		gifch.madr += qwc * 16;
		gifch.qwc  -= qwc;
		hwDmacSrcTadrInc(gifch);
	}
	else DevCon.Error("incGifAddr() Error!");
}

__fi void gifInterrupt()
{
	GIF_LOG("gifInterrupt caught!");
	//Required for Path3 Masking timing!
	if(gifUnit.gifPath[GIF_PATH_3].state == GIF_PATH_WAIT)
	{
		gifUnit.gifPath[GIF_PATH_3].state = GIF_PATH_IDLE;

		if(vif1Regs.stat.VGW)
		{
			CPU_INT(DMAC_GIF, 16);
			return;
		}
	}

	if (dmacRegs.ctrl.MFD == MFD_GIF) { // GIF MFIFO
		//Console.WriteLn("GIF MFIFO");
		gifMFIFOInterrupt();
		return;
	}	

	if (gifUnit.gsSIGNAL.queued) {
		//DevCon.WriteLn("Path 3 Paused");
		CPU_INT(DMAC_GIF, 128);
		return;
	}
	
	if (!(gifch.chcr.STR)) return;

	if ((gifch.qwc > 0) || (!gspath3done)) {
		if (!dmacRegs.ctrl.DMAE) {
			Console.Warning("gs dma masked, re-scheduling...");
			// re-raise the int shortly in the future
			CPU_INT( DMAC_GIF, 64 );
			return;
		}
		GIFdma();
		return;
	}

	gifRegs.stat.FQC = 0;
	gscycles		 = 0;
	gspath3done		 = false;
	gifch.chcr.STR	 = false;
	clearFIFOstuff(false);
	hwDmacIrq(DMAC_GIF);
	DMA_LOG("GIF DMA End");
}

static u32 WRITERING_DMA(u32 *pMem, u32 qwc) {
	//qwc = min(qwc, 1024u);
	uint size = gifUnit.TransferGSPacketData(GIF_TRANS_DMA, (u8*)pMem, qwc*16) / 16;
	incGifChAddr(size);
	return size;
}

int  _GIFchain()
{
	tDMA_TAG *pMem;

	pMem = dmaGetAddr(gifch.madr, false);
	if (pMem == NULL) {

#if USE_OLD_GIF == 1 // d
		// reset path3, fixes dark cloud 2
		GIFPath_Clear( GIF_PATH_3 );
#endif
		//must increment madr and clear qwc, else it loops
		gifch.madr += gifch.qwc * 16;
		gifch.qwc = 0;
		Console.Warning("Hackfix - NULL GIFchain");
		return -1;
	}

	return WRITERING_DMA((u32*)pMem, gifch.qwc);
}

static __fi void GIFchain() {
	// qwc check now done outside this function
	// Voodoocycles
	// >> 2 so Drakan and Tekken 5 don't mess up in some PATH3 transfer. Cycles to interrupt were getting huge..
	/*if (gifch.qwc)*/ gscycles+= ( _GIFchain() * BIAS); /* guessing */
}

static __fi bool checkTieBit(tDMA_TAG* &ptag)
{
	if (gifch.chcr.TIE && ptag->IRQ) {
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

bool CheckPaths(EE_EventType Channel) {
	// Can't do Path 3, so try dma again later...
	if(!gifUnit.CanDoPath3()) {
		CPU_INT(Channel, 128);
		return false;
	}
	return true;
}

void GIFdma()
{
	tDMA_TAG *ptag;
	gscycles = prevcycles;

	if (gifRegs.ctrl.PSE) { // temporarily stop
		Console.WriteLn("Gif dma temp paused? (non MFIFO GIF)");
		CPU_INT(DMAC_GIF, 16);
		return;
	}

	if ((dmacRegs.ctrl.STD == STD_GIF) && (prevcycles != 0)) {
		//Console.WriteLn("GS Stall Control Source = %x, Drain = %x\n MADR = %x, STADR = %x", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3, gifch.madr, psHu32(DMAC_STADR));
		if ((gifch.madr + (gifch.qwc * 16)) > dmacRegs.stadr.ADDR) {
			CPU_INT(DMAC_GIF, 4);
			gscycles = 0;
			return;
		}
		prevcycles = 0;
		gifch.qwc = 0;
	}

	if ((gifch.chcr.MOD == CHAIN_MODE) && (!gspath3done) && gifch.qwc == 0) // Chain Mode
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
				gifch.tadr -= 16;
				gifch.qwc = 0;
				hwDmacIrq(DMAC_STALL_SIS);
				CPU_INT(DMAC_GIF, gscycles);
				gscycles = 0;
				return;
			}
		}

		checkTieBit(ptag);
	}

	clearFIFOstuff(true);
	gifRegs.stat.FQC = min((u16)0x10, gifch.qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]

#if USE_OLD_GIF == 1 // ...
	if (vif1Regs.mskpath3 || gifRegs.mode.M3R) {	
		if (GSTransferStatus.PTH3 == STOPPED_MODE) {
			MSKPATH3_LOG("Path3 Paused by VIF QWC %x", gifch.qwc);
			
			if(gifch.qwc == 0) CPU_INT(DMAC_GIF, 4);
			else gifRegs.stat.set_flags(GIF_STAT_P3Q);
			return;
		}
	}
#endif

	// Transfer Dn_QWC from Dn_MADR to GIF
	if (gifch.qwc > 0) // Normal Mode
	{
		gifRegs.stat.FQC = min((u16)0x10, gifch.qwc);// FQC=31, hack ;) (for values of 31 that equal 16) [ used to be 0xE00; // APATH=3]

		if (CheckPaths(DMAC_GIF) == false) return;

		GIFchain();	//Transfers the data set by the switch
		CPU_INT(DMAC_GIF, gscycles);
		return;
	} else if(gspath3done == false) GIFdma();

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

	if(gifch.chcr.MOD == CHAIN_MODE && gifch.qwc > 0) {
		//DevCon.Warning(L"GIF QWC on Chain " + gifch.chcr.desc());
		if ((gifch.chcr.tag().ID == TAG_REFE) || (gifch.chcr.tag().ID == TAG_END)) {
			gspath3done = true;
		}
	}

	gifInterrupt();
}

static u16 QWCinGIFMFIFO(u32 DrainADDR)
{
	u32 ret;

	GIF_LOG("GIF MFIFO Requesting %x QWC from the MFIFO Base %x, SPR MADR %x Drain %x", gifch.qwc, dmacRegs.rbor.ADDR, spr0ch.madr, DrainADDR);
	//Calculate what we have in the fifo.
	if(DrainADDR <= spr0ch.madr) {
		//Drain is below the tadr, calculate the difference between them
		ret = (spr0ch.madr - DrainADDR) >> 4;
	}
	else {
		u32 limit = dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16;
		//Drain is higher than SPR so it has looped round, 
		//calculate from base to the SPR tag addr and what is left in the top of the ring
		ret = ((spr0ch.madr - dmacRegs.rbor.ADDR) + (limit - DrainADDR)) >> 4;
	}
	GIF_LOG("%x Available of the %x requested", ret, gifch.qwc);
	if((s32)ret < 0) DevCon.Warning("GIF Returning %x!", ret);
	return ret;
}

static __fi bool mfifoGIFrbTransfer()
{
	u16 mfifoqwc = min(QWCinGIFMFIFO(gifch.madr), gifch.qwc);
	if (mfifoqwc == 0) return true; //Lets skip all this, we don't have the data

	if(!gifUnit.CanDoPath3()) {
		DevCon.Warning("mfifoGIFrbTransfer() - Can't do path3");
		return true; // Skip if can't do path3
	}

	bool needWrap = (gifch.madr + (mfifoqwc * 16)) > (dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16);
	uint s1 = ((dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16) - gifch.madr) >> 4;
	uint s2 = mfifoqwc - s1;
	uint s3 = needWrap ? s1 : mfifoqwc;

	gifch.madr = dmacRegs.rbor.ADDR + (gifch.madr & dmacRegs.rbsr.RMSK);
	u8* src = (u8*)PSM(gifch.madr);
	if (src == NULL) return false;
	u32 t1 = gifUnit.TransferGSPacketData(GIF_TRANS_DMA, src, s3*16) / 16; // First part		
	incGifChAddr(t1);
	mfifocycles += t1 * 2; // guessing

	if (needWrap && t1) { // Need to do second transfer to wrap around
		GUNIT_WARN("mfifoGIFrbTransfer() - Wrap");
		src = (u8*)PSM(dmacRegs.rbor.ADDR);
		gifch.madr = dmacRegs.rbor.ADDR;
		if (src == NULL) return false;
		u32 t2 = gifUnit.TransferGSPacketData(GIF_TRANS_DMA, src, s2*16) / 16; // Second part
		incGifChAddr(t2);
		mfifocycles += t2 * 2; // guessing
	}
	return true;
}

static __fi bool mfifoGIFchain()
{
	/* Is QWC = 0? if so there is nothing to transfer */
	if (gifch.qwc == 0) return true;

	if (gifch.madr >= dmacRegs.rbor.ADDR &&
		gifch.madr <=(dmacRegs.rbor.ADDR + dmacRegs.rbsr.RMSK + 16))
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
	else {
		int mfifoqwc;
		GIF_LOG("Non-MFIFO Location transfer doing %x Total QWC", gifch.qwc);
		tDMA_TAG *pMem = dmaGetAddr(gifch.madr, false);
		if (pMem == NULL) return false;

		mfifoqwc = WRITERING_DMA((u32*)pMem, gifch.qwc);
		mfifocycles += (mfifoqwc) * 2; /* guessing */
	}

	return true;
}

static u32 qwctag(u32 mask) {
	return (dmacRegs.rbor.ADDR + (mask & dmacRegs.rbsr.RMSK));
}

void mfifoGIFtransfer(int qwc)
{
	tDMA_TAG *ptag;
	mfifocycles = 0;
	gifmfifoirq = false;

	if (qwc > 0 ) {
		if ((gifstate & GIF_STATE_EMPTY)) {
			if(gifch.chcr.STR == true && !(cpuRegs.interrupt & (1<<DMAC_MFIFO_GIF)))
				CPU_INT(DMAC_MFIFO_GIF, 4);
			gifstate &= ~GIF_STATE_EMPTY;			
		}
		gifRegs.stat.FQC = 16;
		return;
	}

	if (gifRegs.ctrl.PSE) { // temporarily stop
		Console.WriteLn("Gif dma temp paused?");
		CPU_INT(DMAC_MFIFO_GIF, 16);
		return;
	}

	if (gifch.qwc == 0) {
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

		if ((gifch.chcr.TIE) && (ptag->IRQ)) {
			SPR_LOG("dmaIrq Set");
			gifstate = GIF_STATE_DONE;
			gifmfifoirq = true;
		}
		if (QWCinGIFMFIFO(gifch.tadr) == 0) gifstate |= GIF_STATE_EMPTY;
	 }	

#if USE_OLD_GIF == 1 // ...
	if (vif1Regs.mskpath3 || gifRegs.mode.M3R) {	
		if ((gifch.qwc == 0) && (gifstate & GIF_STATE_DONE)) gifstate |= GIF_STATE_STALL;

		if (GSTransferStatus.PTH3 == STOPPED_MODE) {
			DevCon.Warning("GIFMFIFO PTH3 MASK Paused by VIF QWC %x");
			MSKPATH3_LOG("Path3 Paused by VIF Idling");
			gifRegs.stat.set_flags(GIF_STAT_P3Q);
			return;
		}
	}
#endif

	if (!mfifoGIFchain()) {
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

	if (dmacRegs.ctrl.MFD != MFD_GIF) {
		DevCon.Warning("Not in GIF MFIFO mode! Stopping GIF MFIFO");
		return;
	}

	if (gifUnit.gsSIGNAL.queued) {
		//DevCon.WriteLn("Path 3 Paused");
		CPU_INT(DMAC_MFIFO_GIF, 128);
		return;
	}

	if((gifstate & GIF_STATE_EMPTY)) {
		FireMFIFOEmpty();
		if(!(gifstate & GIF_STATE_STALL)) return;
	}

	if (CheckPaths(DMAC_MFIFO_GIF) == false) return;

	if(!gifch.chcr.STR) {
		Console.WriteLn("WTF GIFMFIFO");
		cpuRegs.interrupt &= ~(1 << 11);
		return;
	}

	if (!(gifstate & GIF_STATE_STALL)) {

		if (QWCinGIFMFIFO(gifch.tadr) == 0) {
			gifstate |= GIF_STATE_EMPTY;
			CPU_INT(DMAC_MFIFO_GIF, 4);
			return;
		}
		mfifoGIFtransfer(0);
		return;
	}

	if ((gifstate & GIF_STATE_READY) || (gifch.qwc > 0)) {
		DevCon.Error("gifMFIFO Panic > Shouldn't go here!");
		return;
	}

	//if(gifqwc > 0) Console.WriteLn("GIF MFIFO ending with stuff in it %x", gifqwc);
	if (!gifmfifoirq) gifqwc = 0;

	gspath3done = false;
	gscycles    = 0;

	gifRegs.stat.FQC = 0;
	//vif1Regs.stat.VGW = false; // old code had this

	gifch.chcr.STR = false;
	gifstate = GIF_STATE_READY;
	hwDmacIrq(DMAC_GIF);
	DMA_LOG("GIF MFIFO DMA End");
	clearFIFOstuff(false);
}

void SaveStateBase::gifDmaFreeze() {
	// Note: mfifocycles is not a persistent var, so no need to save it here.
	FreezeTag("GIFdma");
	Freeze(gifstate);
	Freeze(gifqwc);
	Freeze(gspath3done);
	Freeze(gscycles);
}
