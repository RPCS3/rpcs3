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
#include "IPU.h"
#include "IPU/IPUdma.h"
#include "mpeg2lib/Mpeg.h"

#include "Vif.h"
#include "Gif.h"
#include "Vif_Dma.h"

static IPUStatus IPU1Status;
static tIPU_DMA g_nDMATransfer;

void ipuDmaReset()
{
	IPU1Status.InProgress	= false;
	IPU1Status.DMAMode		= DMA_MODE_NORMAL;
	IPU1Status.DMAFinished	= true;

	g_nDMATransfer.reset();
}

void SaveStateBase::ipuDmaFreeze()
{
	FreezeTag( "IPUdma" );
	Freeze(g_nDMATransfer);
	Freeze(IPU1Status);
}

static __fi void ipuDmacSrcChain()
{
	switch (IPU1Status.ChainMode)
	{
		case TAG_REFE: // refe
			//if(IPU1Status.InProgress == false) ipu1dma.tadr += 16;
			IPU1Status.DMAFinished = true;
			break;
		case TAG_CNT: // cnt
			// Set the taddr to the next tag
			ipu1dma.tadr = ipu1dma.madr;
			//if(IPU1Status.DMAFinished == false) IPU1Status.DMAFinished = false;
			break;

		case TAG_NEXT: // next
			ipu1dma.tadr = IPU1Status.NextMem;
			//if(IPU1Status.DMAFinished == false) IPU1Status.DMAFinished = false;
			break;

		case TAG_REF: // ref
			//if(IPU1Status.InProgress == false)ipu1dma.tadr += 16;
			//if(IPU1Status.DMAFinished == false) IPU1Status.DMAFinished = false;
			break;

		case TAG_END: // end
			//ipu1dma.tadr = ipu1dma.madr;
			IPU1Status.DMAFinished = true;
			break;
	}
}

static __fi bool WaitGSPaths()
{
	if(CHECK_IPUWAITHACK)
	{
		if(GSTransferStatus.PTH3 < STOPPED_MODE && GSTransferStatus.PTH3 != IDLE_MODE)
		{
			//GIF_LOG("Flushing gif chcr %x tadr %x madr %x qwc %x", gif->chcr._u32, gif->tadr, gif->madr, gif->qwc);
			//DevCon.WriteLn("Waiting for GIF");
			return false;
		}

		if(GSTransferStatus.PTH2 != STOPPED_MODE)
		{
			//DevCon.WriteLn("Waiting for VIF");
			return false;
		}
		if(GSTransferStatus.PTH1 != STOPPED_MODE)
		{
			//DevCon.WriteLn("Waiting for VU");
			return false;
		}
	}
	return true;
}

static __fi int IPU1chain() {

	int totalqwc = 0;

	if (ipu1dma.qwc > 0 && IPU1Status.InProgress == true)
	{
		int qwc = ipu1dma.qwc;
		u32 *pMem;

		pMem = (u32*)dmaGetAddr(ipu1dma.madr, false);

		if (pMem == NULL)
		{
			Console.Error("ipu1dma NULL!");
			return totalqwc;
		}

		//Write our data to the fifo
		qwc = ipu_fifo.in.write(pMem, qwc);
		ipu1dma.madr += qwc << 4;
		ipu1dma.qwc -= qwc;
		totalqwc += qwc;
	}

	//Update TADR etc
	if(IPU1Status.DMAMode == DMA_MODE_CHAIN) ipuDmacSrcChain();

	if( ipu1dma.qwc == 0)
		IPU1Status.InProgress = false;

	return totalqwc;
}

//static __fi bool WaitGSPaths()
//{
//	//Wait for all GS paths to be clear
//	if (GSTransferStatus._u32 != 0x2a)
//	{
//		if(GSTransferStatus.PTH3 != STOPPED_MODE && vif1Regs.mskpath3) return true;
//		IPU_LOG("Waiting for GS transfers to finish %x", GSTransferStatus._u32);
//		IPU_INT_TO(4);
//		return false;
//	}
//	return true;
//}



int IPU1dma()
{
	int ipu1cycles = 0;
	int totalqwc = 0;

	//We need to make sure GIF has flushed before sending IPU data, it seems to REALLY screw FFX videos
    //if(!WaitGSPaths()) return totalqwc;

	if(ipu1dma.chcr.STR == false || IPU1Status.DMAMode == 2)
	{
		//We MUST stop the IPU from trying to fill the FIFO with more data if the DMA has been suspended
		//if we don't, we risk causing the data to go out of sync with the fifo and we end up losing some!
		//This is true for Dragons Quest 8 and probably others which suspend the DMA.
		DevCon.Warning("IPU1 running when IPU1 DMA disabled! CHCR %x QWC %x", ipu1dma.chcr._u32, ipu1dma.qwc);
		return 0;
	}

	IPU_LOG("IPU1 DMA Called QWC %x Finished %d In Progress %d tadr %x", ipu1dma.qwc, IPU1Status.DMAFinished, IPU1Status.InProgress, ipu1dma.tadr);

	switch(IPU1Status.DMAMode)
	{
		case DMA_MODE_NORMAL:
			{
				if(!WaitGSPaths())
				{ // legacy WaitGSPaths() for now
					IPU_INT_TO(32); //Give it a short wait.
					return totalqwc;
				}
				IPU_LOG("Processing Normal QWC left %x Finished %d In Progress %d", ipu1dma.qwc, IPU1Status.DMAFinished, IPU1Status.InProgress);
				if(IPU1Status.InProgress == true) totalqwc += IPU1chain();
			}
			break;

		case DMA_MODE_CHAIN:
			{
				if(IPU1Status.InProgress == true) //No transfer is ready to go so we need to set one up
				{
					if(!WaitGSPaths())
					{ // legacy WaitGSPaths() for now
						IPU_INT_TO(32); //Give it a short wait.
						return totalqwc;
					}
					IPU_LOG("Processing Chain QWC left %x Finished %d In Progress %d", ipu1dma.qwc, IPU1Status.DMAFinished, IPU1Status.InProgress);
					totalqwc += IPU1chain();
					//Set the TADR forward
				}


				if(IPU1Status.InProgress == false && IPU1Status.DMAFinished == false) //No transfer is ready to go so we need to set one up
				{
					tDMA_TAG* ptag = dmaGetAddr(ipu1dma.tadr, false);  //Set memory pointer to TADR

					if (!ipu1dma.transfer("IPU1", ptag))
					{
						return totalqwc;
					}

					ipu1cycles += 1; // Add 1 cycles from the QW read for the tag
					IPU1Status.ChainMode = ptag->ID;

					if(ipu1dma.chcr.TTE) DevCon.Warning("TTE?");

					switch (IPU1Status.ChainMode)
					{
						case TAG_REFE: // refe
							// do not change tadr
							//ipu1dma.tadr += 16;
							ipu1dma.tadr += 16;
							ipu1dma.madr = ptag[1]._u32;
							IPU_LOG("Tag should end on %x", ipu1dma.tadr);

							break;

						case TAG_CNT: // cnt
							ipu1dma.tadr += 16;
							ipu1dma.madr = ipu1dma.tadr;
							IPU_LOG("Tag should end on %x", ipu1dma.madr + ipu1dma.qwc * 16);
							//ipu1dma.tadr = ipu1dma.madr + (ipu1dma.qwc * 16);
							// Set the taddr to the next tag
							//IPU1Status.DMAFinished = false;
							break;

						case TAG_NEXT: // next
							ipu1dma.madr = ipu1dma.tadr + 16;
							IPU1Status.NextMem = ptag[1]._u32;
							IPU_LOG("Tag should end on %x", IPU1Status.NextMem);
							//IPU1Status.DMAFinished = false;
							break;

						case TAG_REF: // ref
							ipu1dma.madr = ptag[1]._u32;
							ipu1dma.tadr += 16;
							IPU_LOG("Tag should end on %x", ipu1dma.tadr);
							//IPU1Status.DMAFinished = false;
							break;

						case TAG_END: // end
							// do not change tadr
							ipu1dma.madr = ipu1dma.tadr + 16;
							//ipu1dma.tadr += 16;
							IPU_LOG("Tag should end on %x", ipu1dma.madr + ipu1dma.qwc * 16);

							break;

						default:
							Console.Error("IPU ERROR: different transfer mode!, Please report to PCSX2 Team");
							break;
					}

					//if(ipu1dma.qwc == 0) Console.Warning("Blank QWC!");
					if(ipu1dma.qwc > 0) IPU1Status.InProgress = true;
					IPU_LOG("dmaIPU1 dmaChain %8.8x_%8.8x size=%d, addr=%lx, fifosize=%x",
							ptag[1]._u32, ptag[0]._u32, ipu1dma.qwc, ipu1dma.madr, 8 - g_BP.IFC);

					if (ipu1dma.chcr.TIE && ptag->IRQ) //Tag Interrupt is set, so schedule the end/interrupt
						IPU1Status.DMAFinished = true;


					if(!WaitGSPaths() && ipu1dma.qwc > 0)
					{ // legacy WaitGSPaths() for now
						IPU_INT_TO(32); //Give it a short wait.
						return totalqwc;
					}
					IPU_LOG("Processing Start Chain QWC left %x Finished %d In Progress %d", ipu1dma.qwc, IPU1Status.DMAFinished, IPU1Status.InProgress);
					totalqwc += IPU1chain();
					//Set the TADR forward
				}

			}
			break;
	}

	//Do this here to prevent double settings on Chain DMA's
	if(totalqwc > 0 || ipu1dma.qwc == 0)
	{
		IPU_INT_TO(totalqwc * BIAS);
		IPUProcessInterrupt();
	}
	else 
	{
		cpuRegs.eCycle[4] = 0x9999;//IPU_INT_TO(2048);
	}

	IPU_LOG("Completed Call IPU1 DMA QWC Remaining %x Finished %d In Progress %d tadr %x", ipu1dma.qwc, IPU1Status.DMAFinished, IPU1Status.InProgress, ipu1dma.tadr);
	return totalqwc;
}

int IPU0dma()
{
	if(!ipuRegs.ctrl.OFC) return 0;

	int readsize;
	tDMA_TAG* pMem;

	if ((!(ipu0dma.chcr.STR) || (cpuRegs.interrupt & (1 << DMAC_FROM_IPU))) || (ipu0dma.qwc == 0))
		return 0;

	pxAssert(!(ipu0dma.chcr.TTE));

	IPU_LOG("dmaIPU0 chcr = %lx, madr = %lx, qwc  = %lx",
	        ipu0dma.chcr._u32, ipu0dma.madr, ipu0dma.qwc);

	pxAssert(ipu0dma.chcr.MOD == NORMAL_MODE);

	pMem = dmaGetAddr(ipu0dma.madr, true);

	readsize = min(ipu0dma.qwc, (u16)ipuRegs.ctrl.OFC);
	ipu_fifo.out.read(pMem, readsize);

	ipu0dma.madr += readsize << 4;
	ipu0dma.qwc -= readsize; // note: qwc is u16

	if (ipu0dma.qwc == 0)
	{
		if (dmacRegs.ctrl.STS == STS_fromIPU)   // STS == fromIPU
		{
			dmacRegs.stadr.ADDR = ipu0dma.madr;
			switch (dmacRegs.ctrl.STD)
			{
				case NO_STD:
					break;
				case STD_GIF: // GIF
					Console.Warning("GIFSTALL");
					g_nDMATransfer.GIFSTALL = true;
					break;
				case STD_VIF1: // VIF
					Console.Warning("VIFSTALL");
					g_nDMATransfer.VIFSTALL = true;
					break;
				case STD_SIF1:
					Console.Warning("SIFSTALL");
					g_nDMATransfer.SIFSTALL = true;
					break;
			}
		}
		//Fixme ( voodoocycles ):
		//This was IPU_INT_FROM(readsize*BIAS );
		//This broke vids in Digital Devil Saga
		//Note that interrupting based on totalsize is just guessing..
		IPU_INT_FROM( readsize * BIAS );
	}

	return readsize;
}

__fi void dmaIPU0() // fromIPU
{
	if (ipu0dma.pad != 0)
	{
		// Note: pad is the padding right above qwc, so we're testing whether qwc
		// has overflowed into pad.
	    DevCon.Warning(L"IPU0dma's upper 16 bits set to %x", ipu0dma.pad);
		ipu0dma.qwc = ipu0dma.pad = 0;
		//If we are going to clear down IPU0, we should end it too. Going to test this scenario on the PS2 mind - Refraction
		ipu0dma.chcr.STR = false;
		hwDmacIrq(DMAC_FROM_IPU);
	}

	IPUProcessInterrupt();
}

__fi void dmaIPU1() // toIPU
{
	IPU_LOG("IPU1DMAStart QWC %x, MADR %x, CHCR %x, TADR %x", ipu1dma.qwc, ipu1dma.madr, ipu1dma.chcr._u32, ipu1dma.tadr);

	if (ipu1dma.pad != 0)
	{
		// Note: pad is the padding right above qwc, so we're testing whether qwc
		// has overflowed into pad.
	    DevCon.Warning(L"IPU1dma's upper 16 bits set to %x\n", ipu1dma.pad);
		ipu1dma.qwc = ipu1dma.pad = 0;
		// If we are going to clear down IPU1, we should end it too.
		// Going to test this scenario on the PS2 mind - Refraction
		ipu1dma.chcr.STR = false;
		hwDmacIrq(DMAC_TO_IPU);
	}

	if (ipu1dma.chcr.MOD == CHAIN_MODE)  //Chain Mode
	{
		IPU_LOG("Setting up IPU1 Chain mode");
		if(ipu1dma.qwc == 0)
		{
			IPU1Status.InProgress = false;
			IPU1Status.DMAFinished = false;
		}
		else
		{   //Attempting to continue a previous chain
			IPU_LOG("Resuming DMA TAG %x", (ipu1dma.chcr.TAG >> 12));
			//We MUST check the CHCR for the tag it last knew, it can be manipulated!
			IPU1Status.ChainMode = (ipu1dma.chcr.TAG >> 12) & 0x7;
			IPU1Status.InProgress = true;
			IPU1Status.DMAFinished = ((ipu1dma.chcr.TAG >> 15) && ipu1dma.chcr.TIE) ? true : false;
		}

		IPU1Status.DMAMode = DMA_MODE_CHAIN;
		IPU1dma();
	}
	else //Normal Mode
	{
		if(ipu1dma.qwc == 0)
		{
			ipu1dma.chcr.STR = false;
				// Hack to force stop IPU
				ipuRegs.cmd.BUSY = 0;
				ipuRegs.ctrl.BUSY = 0;
				ipuRegs.topbusy = 0;
				//
			hwDmacIrq(DMAC_TO_IPU);
			Console.Warning("IPU1 Normal error!");
		}
		else
		{
			IPU_LOG("Setting up IPU1 Normal mode");
			IPU1Status.InProgress = true;
			IPU1Status.DMAFinished = true;
			IPU1Status.DMAMode = DMA_MODE_NORMAL;
			IPU1dma();
		}
	}
}

extern void GIFdma();

void ipu0Interrupt()
{
	IPU_LOG("ipu0Interrupt: %x", cpuRegs.cycle);

	if (g_nDMATransfer.FIREINT0)
	{
		g_nDMATransfer.FIREINT0 = false;
		hwIntcIrq(INTC_IPU);
	}

	if (g_nDMATransfer.GIFSTALL)
	{
		// gif
		Console.Warning("IPU GIF Stall");
		g_nDMATransfer.GIFSTALL = false;
		//if (gif->chcr.STR) GIFdma();
	}

	if (g_nDMATransfer.VIFSTALL)
	{
		// vif
		Console.Warning("IPU VIF Stall");
		g_nDMATransfer.VIFSTALL = false;
		//if (vif1ch.chcr.STR) dmaVIF1();
	}

	if (g_nDMATransfer.SIFSTALL)
	{
		// sif
		Console.Warning("IPU SIF Stall");
		g_nDMATransfer.SIFSTALL = false;

		// Not totally sure whether this needs to be done or not, so I'm
		// leaving it commented out for the moment.
		//if (sif1dma.chcr.STR) SIF1Dma();
	}

	if (g_nDMATransfer.TIE0)
	{
		g_nDMATransfer.TIE0 = false;
	}

	ipu0dma.chcr.STR = false;
	hwDmacIrq(DMAC_FROM_IPU);
}

IPU_FORCEINLINE void ipu1Interrupt()
{
	IPU_LOG("ipu1Interrupt %x:", cpuRegs.cycle);

	if(IPU1Status.DMAFinished == false || IPU1Status.InProgress == true)  //Sanity Check
	{
		IPU1dma();
		return;
	}

	IPU_LOG("ipu1 finish %x:", cpuRegs.cycle);
	ipu1dma.chcr.STR = false;
	IPU1Status.DMAMode = 2;
	hwDmacIrq(DMAC_TO_IPU);
}
