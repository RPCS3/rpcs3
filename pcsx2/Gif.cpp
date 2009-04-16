/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"

#include "Common.h"
#include "VU.h"
#include "GS.h"
#include "iR5900.h"
#include "Counters.h"

#include "VifDma.h"

using std::min;

#define gifsplit 128
enum gifstate_t
{
	GIF_STATE_EMPTY = 0,
	GIF_STATE_STALL,
	GIF_STATE_DONE
};

// A three-way toggle used to determine if the GIF is stalling (transferring) or done (finished).
static gifstate_t gifstate = GIF_STATE_EMPTY;

static u64 s_gstag = 0; // used for querying the last tag

// This should be a bool, as should the return value of hwDmacSrcChainWithStack.
// Next time I feel like breaking the save state, it will be. --arcum42
static int gspath3done = 0;

static u32 gscycles = 0, prevcycles = 0, mfifocycles = 0;
static u32 gifqwc = 0;
bool gifmfifoirq = FALSE;

__forceinline void gsInterrupt() {
	GIF_LOG("gsInterrupt: %8.8x", cpuRegs.cycle);

	if((gif->chcr & 0x100) == 0){
		//Console::WriteLn("Eh? why are you still interrupting! chcr %x, qwc %x, done = %x", params gif->chcr, gif->qwc, done);
		return;
	}
	if (gif->qwc > 0 || gspath3done == 0) {
		if (!(psHu32(DMAC_CTRL) & 0x1)) {
			Console::Notice("gs dma masked, re-scheduling...");
			// re-raise the int shortly in the future
			CPU_INT( 2, 64 );
			return;
		}

		GIFdma();
#ifdef GSPATH3FIX
		// re-raise the IRQ as part of the mysterious Path3fix.
		// fixme - this hack *should* have the gs_irq raised from the VIF, I think.  It would be
		// more efficient and more correct.  (air)
		/*if (!(vif1Regs->mskpath3 && (vif1ch->chcr & 0x100)) || (psHu32(GIF_MODE) & 0x1))
			CPU_INT( 2, 64 );*/
#endif
		if(gspath3done == 0 || gif->qwc > 0) return;
	}

	gspath3done = 0;
	gscycles = 0;
	Path3transfer = FALSE;
	gif->chcr &= ~0x100;
	GSCSRr &= ~0xC000; //Clear FIFO stuff
	GSCSRr |= 0x4000;  //FIFO empty
	psHu32(GIF_STAT)&= ~0xE00; // OPH=0 | APATH=0
	psHu32(GIF_STAT)&= ~0x1F000000; // QFC=0
	hwDmacIrq(DMAC_GIF);
	GIF_LOG("GIF DMA end");

}

static void WRITERING_DMA(u32 *pMem, u32 qwc)
{ 
	psHu32(GIF_STAT) |= 0xE00;         

	// Path3 transfer will be set to false by the GIFtag handler.
	Path3transfer = TRUE;

	if( mtgsThread != NULL )
	{ 
		int sizetoread = (qwc)<<4; 
		sizetoread = mtgsThread->PrepDataPacket( GIF_PATH_3, pMem, qwc );
		u8* pgsmem = mtgsThread->GetDataPacketPtr();

		/* check if page of endmem is valid (dark cloud2) */
		// fixme: this hack makes no sense, because the giftagDummy will
		// process the full length of bytes regardess of how much we copy.
		// So you'd think if we're truncating the copy to prevent DEPs, we 
		// should truncate the gif packet size too.. (air)

		// fixed? PrepDataPacket now returns the actual size of the packet.
		// VIF handles scratchpad wrapping also, so this code shouldn't be needed anymore.

		memcpy_aligned(pgsmem, pMem, sizetoread<<4); 
		
		mtgsThread->SendDataPacket();
	} 
	else 
	{ 
		GSGIFTRANSFER3(pMem, qwc);
		if (GSgetLastTag != NULL)
		{ 
			GSgetLastTag(&s_gstag); 
			if (s_gstag == 1) Path3transfer = FALSE; /* fixes SRS and others */ 
		} 
	} 
} 

int  _GIFchain() {
#ifdef GSPATH3FIX
	u32 qwc = ((psHu32(GIF_MODE) & 0x4) && (vif1Regs->mskpath3)) ? min(8, (int)gif->qwc) : min( gifsplit, (int)gif->qwc );
#else
	u32 qwc = gif->qwc;
#endif
	u32 *pMem;

	pMem = (u32*)dmaGetAddr(gif->madr);
	if (pMem == NULL) {
		// reset path3, fixes dark cloud 2
		gsGIFSoftReset(4);

		//must increment madr and clear qwc, else it loops
		gif->madr+= gif->qwc*16;
		gif->qwc = 0;
		Console::Notice( "Hackfix - NULL GIFchain" );
		return -1;
	}
	WRITERING_DMA(pMem, qwc);
	
	gif->madr+= qwc*16;
	gif->qwc -= qwc;
	return (qwc)*2;
}

static __forceinline void GIFchain() 
{
	FreezeRegs(1);  
	if (gif->qwc) gscycles+= _GIFchain(); /* guessing */
	FreezeRegs(0); 
}

static __forceinline void dmaGIFend()
{
	if ((psHu32(GIF_MODE) & 0x4) && gif->qwc != 0)
		CPU_INT(2, min( 8, (int)gif->qwc ) /** BIAS*/);
	else
		CPU_INT(2, min( gifsplit, (int)gif->qwc )  /** BIAS*/);
}

// These could probably be consolidated into one function,
//  but I wasn't absolutely sure if there was a good reason 
// not to do the gif->qwc != 0 check. --arcum42
static __forceinline void GIFdmaEnd()
{
	if (psHu32(GIF_MODE) & 0x4)
		CPU_INT(2, min( 8, (int)gif->qwc ) /** BIAS*/);
	else
		CPU_INT(2, min( gifsplit, (int)gif->qwc ) /** BIAS*/);
}

void GIFdma() 
{
	u32 *ptag;
	u32 id;
	
	gscycles= prevcycles ? prevcycles: gscycles;

	if ((psHu32(GIF_CTRL) & 8)) { // temporarily stop
		Console::WriteLn("Gif dma temp paused?");
		return;
	}

	

#ifndef GSPATH3FIX
	if ( !(psHu32(GIF_MODE) & 0x4) ) {
		if (vif1Regs->mskpath3 || psHu32(GIF_MODE) & 0x1) {
			gif->chcr &= ~0x100;
			psHu32(GIF_STAT)&= ~0xE00; // OPH=0 | APATH=0
			hwDmacIrq(2);
			return;
		}
	}
#endif

	if ((psHu32(DMAC_CTRL) & 0xC0) == 0x80 && prevcycles != 0) { // STD == GIF
		Console::WriteLn("GS Stall Control Source = %x, Drain = %x\n MADR = %x, STADR = %x", params (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3, gif->madr, psHu32(DMAC_STADR));

		if( gif->madr + (gif->qwc * 16) > psHu32(DMAC_STADR) ) {
			CPU_INT(2, gscycles);
			gscycles = 0;
			return;
		}
		prevcycles = 0;
		gif->qwc = 0;
	}

	GSCSRr &= ~0xC000;  //Clear FIFO stuff
	GSCSRr |= 0x8000;   //FIFO full
	psHu32(GIF_STAT)|= 0x10000000; // FQC=31, hack ;) [ used to be 0xE00; // OPH=1 | APATH=3]

#ifdef GSPATH3FIX
	if (vif1Regs->mskpath3 || (psHu32(GIF_MODE) & 0x1)) {
		if(gif->qwc == 0) {
			if((gif->chcr & 0x10e) == 0x104) {
				ptag = (u32*)dmaGetAddr(gif->tadr);  //Set memory pointer to TADR

				if (ptag == NULL) {					 //Is ptag empty?
					psHu32(DMAC_STAT) |= DMAC_STAT_BEIS;		 //If yes, set BEIS (BUSERR) in DMAC_STAT register 
					return;
				}	
				gscycles += 2;
				gif->chcr = ( gif->chcr & 0xFFFF ) | ( (*ptag) & 0xFFFF0000 );  //Transfer upper part of tag to CHCR bits 31-15
				id = (ptag[0] >> 28) & 0x7;		//ID for DmaChain copied from bit 28 of the tag
				gif->qwc  = (u16)ptag[0];			    //QWC set to lower 16bits of the tag
				gif->madr = ptag[1];				    //MADR = ADDR field	
				gspath3done = hwDmacSrcChainWithStack(gif, id);
				GIF_LOG("PTH3 MASK gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx", ptag[1], ptag[0], gif->qwc, id, gif->madr);

				if ((gif->chcr & 0x80) && ptag[0] >> 31) {			 //Check TIE bit of CHCR and IRQ bit of tag
					GIF_LOG("PATH3 MSK dmaIrq Set");
					Console::WriteLn("GIF TIE");
					gspath3done |= 1;
				}
			}
		}
		
		GIFchain(); 

		if((gif->qwc == 0) && ((gspath3done == 1) || (gif->chcr & 0xc) == 0)){ 
			gspath3done = 0;
			gif->chcr &= ~0x100;
			GSCSRr &= ~0xC000;
			GSCSRr |= 0x4000;  
			Path3transfer = FALSE;
			psHu32(GIF_STAT)&= ~0x1F000E00; // OPH=0 | APATH=0 | QFC=0
			hwDmacIrq(DMAC_GIF);
		}
		//Dont unfreeze xmm regs here, Masked PATH3 can only be called by VIF, which is already handling it.
		return;
	}
#endif
	// Transfer Dn_QWC from Dn_MADR to GIF
	if ((gif->chcr & 0xc) == 0 || gif->qwc > 0) { // Normal Mode
		if ((((psHu32(DMAC_CTRL) & 0xC0) == 0x80) && ((gif->chcr & 0xc) == 0))) { 
			Console::WriteLn("DMA Stall Control on GIF normal");
		}
		GIFchain();	//Transfers the data set by the switch
		
		if (((gif->qwc == 0) && (gif->chcr & 0xc) == 0)) 
			gspath3done = 1;
		else if(gif->qwc > 0)
		{
			GIFdmaEnd();
		return;
		
	}
	}
	if ((gif->chcr & 0xc) == 0x4 && gspath3done == 0)
	{
		// Chain Mode
		//while ((gspath3done == 0) && (gif->qwc == 0)) {		//Loop if the transfers aren't intermittent
			ptag = (u32*)dmaGetAddr(gif->tadr);  //Set memory pointer to TADR
			if (ptag == NULL) {					 //Is ptag empty?
				psHu32(DMAC_STAT)|= DMAC_STAT_BEIS;		 //If yes, set BEIS (BUSERR) in DMAC_STAT register
				return;
			}
			gscycles+=2; // Add 1 cycles from the QW read for the tag

			// We used to transfer dma tags if tte is set here

			gif->chcr = ( gif->chcr & 0xFFFF ) | ((*ptag) & 0xFFFF0000);  //Transfer upper part of tag to CHCR bits 31-15
		
			id = (ptag[0] >> 28) & 0x7;		//ID for DmaChain copied from bit 28 of the tag
			gif->qwc  = (u16)ptag[0];			    //QWC set to lower 16bits of the tag
			gif->madr = ptag[1];				    //MADR = ADDR field
			
			gspath3done = hwDmacSrcChainWithStack(gif, id);
			GIF_LOG("gifdmaChain %8.8x_%8.8x size=%d, id=%d, addr=%lx", ptag[1], ptag[0], gif->qwc, id, gif->madr);

			if ((psHu32(DMAC_CTRL) & 0xC0) == 0x80) { // STD == GIF
				// there are still bugs, need to also check if gif->madr +16*qwc >= stadr, if not, stall
				if(!gspath3done && ((gif->madr + (gif->qwc * 16)) > psHu32(DMAC_STADR)) && (id == 4)) {
					// stalled
					Console::WriteLn("GS Stall Control Source = %x, Drain = %x\n MADR = %x, STADR = %x", params (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3,gif->madr, psHu32(DMAC_STADR));
					prevcycles = gscycles;
					gif->tadr -= 16;
					hwDmacIrq(13);
					CPU_INT(2, gscycles);
					gscycles = 0;
					return;
				}
			}
			GIFchain();	//Transfers the data set by the switch

			if ((gif->chcr & 0x80) && (ptag[0] >> 31)) { //Check TIE bit of CHCR and IRQ bit of tag
				GIF_LOG("dmaIrq Set");
				gspath3done = 1;
			}
		//}
	}

	prevcycles = 0;
	if (!(vif1Regs->mskpath3 || (psHu32(GIF_MODE) & 0x1)))	{
		if (gspath3done == 0 || gif->qwc > 0)
		{
			if (gif->qwc != 0)
			{
				GIFdmaEnd();
			} 
			else
			{
				ptag = (u32*)dmaGetAddr(gif->tadr);  //Set memory pointer to TADR
				gif->qwc  = (u16)ptag[0];			    //QWC set to lower 16bits of the tag
				gif->chcr = ( gif->chcr & 0xFFFF ) | ( (*ptag) & 0xFFFF0000 );  //Transfer upper part of tag to CHCR bits 31-15
				
				GIFdmaEnd();
				gif->qwc = 0;
				return;
			}
		}
		gscycles = 0;
	}
}

void dmaGIF() {
	 //We used to addd wait time for the buffer to fill here, fixing some timing problems in path 3 masking
	//It takes the time of 24 QW for the BUS to become ready - The Punisher, And1 Streetball
	GIF_LOG("dmaGIFstart chcr = %lx, madr = %lx, qwc  = %lx\n tadr = %lx, asr0 = %lx, asr1 = %lx", gif->chcr, gif->madr, gif->qwc, gif->tadr, gif->asr0, gif->asr1);
	if ((psHu32(DMAC_CTRL) & 0xC) == 0xC ) { // GIF MFIFO
		Console::WriteLn("GIF MFIFO");
		gifMFIFOInterrupt();
		return;
	}

	gspath3done = 0; // For some reason this doesnt clear? So when the system starts the thread, we will clear it :)

	GSCSRr &= ~0xC000;  //Clear FIFO stuff
	GSCSRr |= 0x8000;   //FIFO full
	psHu32(GIF_STAT)|= 0x10000000; // FQC=31, hack ;) [used to be 0xE00; // OPH=1 | APATH=3]

	if ((gif->qwc == 0) && ((gif->chcr & 0xc) != 0)){
		u32 *ptag;
		ptag = (u32*)dmaGetAddr(gif->tadr); 
		gif->qwc  = (u16)ptag[0];			    //QWC set to lower 16bits of the tag
		gif->chcr = ( gif->chcr & 0xFFFF ) | ( (*ptag) & 0xFFFF0000 );  //Transfer upper part of tag to CHCR bits 31-15
		
		//gspath3done = hwDmacSrcChainWithStack(gif, (ptag[0] >> 28) & 0x7);
		GIFdmaEnd();
		gif->qwc = 0;
		return;
	}

	//Halflife sets a QWC amount in chain mode, no tadr set.
	if((gif->qwc > 0) && ((gif->chcr & 0x4) == 0x4)) gspath3done = 1;
	
	dmaGIFend();
}

// called from only one location, so forceinline it:
static __forceinline int mfifoGIFrbTransfer() {
	u32 qwc = (psHu32(GIF_MODE) & 0x4 && vif1Regs->mskpath3) ? min(8, (int)gif->qwc) : gif->qwc;
	int mfifoqwc = min(gifqwc, qwc);
	u32 *src;

	/* Check if the transfer should wrap around the ring buffer */
	if ((gif->madr+mfifoqwc*16) > (psHu32(DMAC_RBOR) + psHu32(DMAC_RBSR)+16)) 
	{
		int s1 = ((psHu32(DMAC_RBOR) + psHu32(DMAC_RBSR)+16) - gif->madr) >> 4;

		// fixme - I don't think these should use WRITERING_DMA, since our source
		// isn't the DmaGetAddr(gif->madr) address that WRITERING_DMA expects.

		/* it does, so first copy 's1' bytes from 'addr' to 'data' */
		src = (u32*)PSM(gif->madr);
		if (src == NULL) return -1;
		WRITERING_DMA(src, s1);

		/* and second copy 's2' bytes from 'maddr' to '&data[s1]' */
		src = (u32*)PSM(psHu32(DMAC_RBOR));
		if (src == NULL) return -1;
		WRITERING_DMA(src, (mfifoqwc - s1));
		
	} 
	else 
	{
		/* it doesn't, so just transfer 'qwc*16' words 
		   from 'gif->madr' to GS */
		src = (u32*)PSM(gif->madr);
		if (src == NULL) return -1;
		
		WRITERING_DMA(src, mfifoqwc);
		gif->madr = psHu32(DMAC_RBOR) + (gif->madr & psHu32(DMAC_RBSR));
	}

	gifqwc -= mfifoqwc;
	gif->qwc -= mfifoqwc;
	gif->madr += mfifoqwc*16;
	mfifocycles += (mfifoqwc) * 2; /* guessing */

	return 0;
}

// called from only one location, so forceinline it:
static __forceinline int mfifoGIFchain() {	 
	/* Is QWC = 0? if so there is nothing to transfer */

	if (gif->qwc == 0) return 0;
	
	if (gif->madr >= psHu32(DMAC_RBOR) &&
		gif->madr <= (psHu32(DMAC_RBOR)+psHu32(DMAC_RBSR))) 
	{
		if (mfifoGIFrbTransfer() == -1) return -1;
	} 
	else 
	{
		int mfifoqwc = (psHu32(GIF_MODE) & 0x4 && vif1Regs->mskpath3) ? min(8, (int)gif->qwc) : gif->qwc;
		u32 *pMem = (u32*)dmaGetAddr(gif->madr);
		if (pMem == NULL) return -1;

		WRITERING_DMA(pMem, mfifoqwc);
		gif->madr += mfifoqwc*16;
		gif->qwc -= mfifoqwc;
		mfifocycles += (mfifoqwc) * 2; /* guessing */
	}

	return 0;
}

void mfifoGIFtransfer(int qwc) {
	u32 *ptag;
	int id;
	u32 temp = 0;
	mfifocycles = 0;
	
	gifmfifoirq = FALSE;

	if(qwc > 0 ) {
		gifqwc += qwc;
		if (!(gif->chcr & 0x100)) return;
		if (gifstate == GIF_STATE_STALL) return;
	}
	
	SPR_LOG("mfifoGIFtransfer %x madr %x, tadr %x", gif->chcr, gif->madr, gif->tadr);
		
	if (gif->qwc == 0) {
		if (gif->tadr == spr0->madr) {
			//if( gifqwc > 1 ) DevCon::WriteLn("gif mfifo tadr==madr but qwc = %d", params gifqwc);
			//hwDmacIrq(14);
			
			return;
		}
		gif->tadr = psHu32(DMAC_RBOR) + (gif->tadr & psHu32(DMAC_RBSR));
		ptag = (u32*)dmaGetAddr(gif->tadr);
			
		id = (ptag[0] >> 28) & 0x7;
		gif->qwc  = (ptag[0] & 0xffff);
		gif->madr = ptag[1];
		mfifocycles += 2;
			
		gif->chcr = ( gif->chcr & 0xFFFF ) | ( (*ptag) & 0xFFFF0000 );
		SPR_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx mfifo qwc = %x spr0 madr = %x",
				ptag[1], ptag[0], gif->qwc, id, gif->madr, gif->tadr, gifqwc, spr0->madr);

		gifqwc--;
		switch (id) {
			case 0: // Refe - Transfer Packet According to ADDR field
				gif->tadr = psHu32(DMAC_RBOR) + ((gif->tadr + 16) & psHu32(DMAC_RBSR));
				gifstate = GIF_STATE_DONE;										//End Transfer
				break;

			case 1: // CNT - Transfer QWC following the tag.
				gif->madr = psHu32(DMAC_RBOR) + ((gif->tadr + 16) & psHu32(DMAC_RBSR));						//Set MADR to QW after Tag            
				gif->tadr = psHu32(DMAC_RBOR) + ((gif->madr + (gif->qwc << 4)) & psHu32(DMAC_RBSR));			//Set TADR to QW following the data
				gifstate = GIF_STATE_EMPTY;
				break;

			case 2: // Next - Transfer QWC following tag. TADR = ADDR
				temp = gif->madr;								//Temporarily Store ADDR
				gif->madr = psHu32(DMAC_RBOR) + ((gif->tadr + 16) & psHu32(DMAC_RBSR)); 					  //Set MADR to QW following the tag
				gif->tadr = temp;								//Copy temporarily stored ADDR to Tag
				gifstate = GIF_STATE_EMPTY;
				break;

			case 3: // Ref - Transfer QWC from ADDR field
			case 4: // Refs - Transfer QWC from ADDR field (Stall Control) 
				gif->tadr = psHu32(DMAC_RBOR) + ((gif->tadr + 16) & psHu32(DMAC_RBSR));							//Set TADR to next tag
				gifstate = GIF_STATE_EMPTY;
				break;

			case 7: // End - Transfer QWC following the tag
				gif->madr = psHu32(DMAC_RBOR) + ((gif->tadr + 16) & psHu32(DMAC_RBSR));		//Set MADR to data following the tag
				gif->tadr = psHu32(DMAC_RBOR) + ((gif->madr + (gif->qwc << 4)) & psHu32(DMAC_RBSR));			//Set TADR to QW following the data
				gifstate = GIF_STATE_DONE;						//End Transfer
				break;
			}
			
		if ((gif->chcr & 0x80) && (ptag[0] >> 31)) {
			SPR_LOG("dmaIrq Set");
			gifstate = GIF_STATE_DONE;
			gifmfifoirq = TRUE;
		}
	 }
	 
	FreezeRegs(1); 
	 
		if (mfifoGIFchain() == -1) {
			Console::WriteLn("GIF dmaChain error size=%d, madr=%lx, tadr=%lx", params
					gif->qwc, gif->madr, gif->tadr);
			gifstate = GIF_STATE_STALL;
		}
		
	FreezeRegs(0); 
		
	if(gif->qwc == 0 && gifstate == GIF_STATE_DONE) gifstate = GIF_STATE_STALL;
	CPU_INT(11,mfifocycles);
		
	SPR_LOG("mfifoGIFtransfer end %x madr %x, tadr %x", gif->chcr, gif->madr, gif->tadr);	
}

void gifMFIFOInterrupt()
{
	if (!(gif->chcr & 0x100)) { 
		Console::WriteLn("WTF GIFMFIFO");
		cpuRegs.interrupt &= ~(1 << 11); 
		return ; 
	}
	
	if(gifstate != GIF_STATE_STALL) {
		if(gifqwc <= 0) {
			//Console::WriteLn("Empty");
			psHu32(GIF_STAT)&= ~0xE00; // OPH=0 | APATH=0
			hwDmacIrq(14);
			return;
		}
		mfifoGIFtransfer(0);
		return;
	}
#ifdef PCSX2_DEVBUILD
	if(gifstate == GIF_STATE_EMPTY || gif->qwc > 0) {
		Console::Error("gifMFIFO Panic > Shouldn't go here!");
		return;
	}
#endif
	//if(gifqwc > 0) Console::WriteLn("GIF MFIFO ending with stuff in it %x", params gifqwc);
	if (!gifmfifoirq) gifqwc = 0;
	gifstate = GIF_STATE_EMPTY;
	gif->chcr &= ~0x100;
	hwDmacIrq(DMAC_GIF);
	GSCSRr &= ~0xC000; //Clear FIFO stuff
	GSCSRr |= 0x4000;  //FIFO empty
	psHu32(GIF_STAT)&= ~0xE00; // OPH=0 | APATH=0
	psHu32(GIF_STAT)&= ~0x1F000000; // QFC=0
}

void SaveState::gifFreeze()
{
	FreezeTag( "GIFdma" );

	Freeze( gifstate );
	Freeze( gifqwc );
	Freeze( gspath3done );
	Freeze( gscycles );

	// Note: mfifocycles is not a persistent var, so no need to save it here.
}
