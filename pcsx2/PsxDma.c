/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include <string.h>

#include "PsxCommon.h"

// Dma0/1   in Mdec.c
// Dma3     in CdRom.c
// Dma8     in PsxSpd.c
// Dma11/12 in PsxSio2.c
//static int spudmaenable[2];
int spu2interrupts[2];
int iopsifbusy[2] = { 0, 0 };
void psxDma4(u32 madr, u32 bcr, u32 chcr) { // SPU
	int size;

	/*if(chcr & 0x400) SysPrintf("SPU 2 DMA 4 linked list chain mode! chcr = %x madr = %x bcr = %x\n", chcr, madr, bcr);
	if(chcr & 0x40000000) SysPrintf("SPU 2 DMA 4 Unusual bit set on 'to' direction chcr = %x madr = %x bcr = %x\n", chcr, madr, bcr);
	if((chcr & 0x1) == 0) SysPrintf("SPU 2 DMA 4 loading from spu2 memory chcr = %x madr = %x bcr = %x\n", chcr, madr, bcr);
*/

	if(SPU2async)
			{	
				//if((psxRegs.cycle - psxCounters[6].sCycleT) >= psxCounters[6].CycleT){
				SPU2async(psxRegs.cycle - psxCounters[6].sCycleT);	
				//SysPrintf("cycles sent to SPU2 %x\n", psxRegs.cycle - psxCounters[6].sCycleT);
				psxCounters[6].sCycleT = psxRegs.cycle;
				psxCounters[6].CycleT = ((bcr >> 16) * (bcr & 0xFFFF)) * 3;
			//}
				psxNextCounter -= (psxRegs.cycle-psxNextsCounter);
				psxNextsCounter = psxRegs.cycle;
				if(psxCounters[6].CycleT < psxNextCounter) psxNextCounter = psxCounters[6].CycleT;

	}

	switch (chcr) {
		case 0x01000201: //cpu to spu transfer
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 4 - SPU mem2spu *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			//SysPrintf("DMA4 write blocks %x, size per block %x\n", (bcr >> 16), (bcr & 0xFFFF));
			size = (bcr >> 16) * (bcr & 0xFFFF);			// Number of blocks to transfer
			SPU2writeDMA4Mem((u16 *)PSXM(madr), size*2);
			break;
		case 0x01000200: //spu to cpu transfer
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 4 - SPU spu2mem *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			//SysPrintf("DMA4 read blocks %x, size per block %x\n", (bcr >> 16), (bcr & 0xFFFF));
			size = (bcr >> 16) * (bcr & 0xFFFF);			// Number of blocks to transfer
			SPU2readDMA4Mem((u16 *)PSXM(madr), size*2);
			psxCpu->Clear(HW_DMA4_MADR, size);
			break;

		default:
			SysPrintf("*** DMA 4 - SPU unknown *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
			break;
	}
	
	//spudmaenable[0] = size;
}

int  psxDma4Interrupt() {
		HW_DMA4_CHCR &= ~0x01000000;
		psxDmaInterrupt(4);
		psxHu32(0x1070)|= 1<<9;
		return 1;
}

void psxDma2(u32 madr, u32 bcr, u32 chcr) { // GPU
	HW_DMA2_CHCR &= ~0x01000000;
	psxDmaInterrupt(2);
}

void psxDma6(u32 madr, u32 bcr, u32 chcr) {
	u32 *mem = (u32 *)PSXM(madr);

#ifdef PSXDMA_LOG
	PSXDMA_LOG("*** DMA 6 - OT *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif

	if (chcr == 0x11000002) {
		while (bcr--) {
			*mem-- = (madr - 4) & 0xffffff;
			madr -= 4;
		}
		mem++; *mem = 0xffffff;
	} else {
		// Unknown option
#ifdef PSXDMA_LOG
		PSXDMA_LOG("*** DMA 6 - OT unknown *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
	}
	HW_DMA6_CHCR &= ~0x01000000;
	psxDmaInterrupt(6);
}

void psxDma7(u32 madr, u32 bcr, u32 chcr) {
	int size;

	if(SPU2async)
			{	
				//if((psxRegs.cycle - psxCounters[6].sCycleT) >= psxCounters[6].CycleT){
				SPU2async(psxRegs.cycle - psxCounters[6].sCycleT);	
				//SysPrintf("cycles sent to SPU2 %x\n", psxRegs.cycle - psxCounters[6].sCycleT);
				psxCounters[6].sCycleT = psxRegs.cycle;
				psxCounters[6].CycleT = ((bcr >> 16) * (bcr & 0xFFFF)) * 3;
			//}
				psxNextCounter -= (psxRegs.cycle-psxNextsCounter);
				psxNextsCounter = psxRegs.cycle;
				if(psxCounters[6].CycleT < psxNextCounter) psxNextCounter = psxCounters[6].CycleT;

	}


	switch (chcr) {
		case 0x01000201: //cpu to spu2 transfer
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 7 - SPU2 mem2spu *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			//SysPrintf("DMA7 write blocks %x, size per block %x\n", (bcr >> 16), (bcr & 0xFFFF));
			size = (bcr >> 16) * (bcr & 0xFFFF);			// Number of blocks to transfer

			SPU2writeDMA7Mem((u16 *)PSXM(madr), size*2);
			break;
		case 0x01000200: //spu2 to cpu transfer
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 7 - SPU2 spu2mem *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			//SysPrintf("DMA4 read blocks %x, size per block %x\n", (bcr >> 16), (bcr & 0xFFFF));
			size = (bcr >> 16) * (bcr & 0xFFFF);			// Number of blocks to transfer

			SPU2readDMA7Mem((u16 *)PSXM(madr), size*2);
			psxCpu->Clear(HW_DMA7_MADR, size);
			break;
		default:
			SysPrintf("*** DMA 7 - SPU unknown *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
			break;
	}
	
}

int  psxDma7Interrupt() {
	
		HW_DMA7_CHCR &= ~0x01000000;
		psxDmaInterrupt2(0);
		return 1;
	
}
extern int eesifbusy[2];
void psxDma9(u32 madr, u32 bcr, u32 chcr) {

	DMACh *dma = (DMACh*)&PS2MEM_HW[0xc000];

#ifdef SIF_LOG
	SIF_LOG("IOP: dmaSIF0 chcr = %lx, madr = %lx, bcr = %lx, tadr = %lx\n",	chcr, madr, bcr, HW_DMA9_TADR);
#endif

	iopsifbusy[0] = 1;
	psHu32(0x1000F240) |= 0x2000;
	if (eesifbusy[0] == 1 && iopsifbusy[0] == 1) {
		SIF0Dma();
		psHu32(0x1000F240) &= ~0x20;
		psHu32(0x1000F240) &= ~0x2000;
	}
}

void psxDma10(u32 madr, u32 bcr, u32 chcr) {
	DMACh *dma = (DMACh*)&PS2MEM_HW[0xc400];

#ifdef SIF_LOG
	SIF_LOG("IOP: dmaSIF1 chcr = %lx, madr = %lx, bcr = %lx\n",	chcr, madr, bcr);
#endif

	iopsifbusy[1] = 1;
	psHu32(0x1000F240) |= 0x4000;
	if (eesifbusy[1] == 1 && iopsifbusy[1] == 1) {
		SIF1Dma();
		psHu32(0x1000F240) &= ~0x40;
		psHu32(0x1000F240) &= ~0x100;
		psHu32(0x1000F240) &= ~0x4000;
	}
}

void psxDma8(u32 madr, u32 bcr, u32 chcr) {
	int size;

	switch (chcr & 0x01000201) {
		case 0x01000201: //cpu to dev9 transfer
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 8 - DEV9 mem2dev9 *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			size = (bcr >> 16) * (bcr & 0xFFFF);			// Number of blocks to transfer

			DEV9writeDMA8Mem((u32*)PSXM(madr), size*8);
			break;
		case 0x01000200: //dev9 to cpu transfer
#ifdef PSXDMA_LOG
			PSXDMA_LOG("*** DMA 8 - DEV9 dev9mem *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif
			size = (bcr >> 16) * (bcr & 0xFFFF);			// Number of blocks to transfer

			DEV9readDMA8Mem((u32*)PSXM(madr), size*8);
			break;
#ifdef PSXDMA_LOG
		default:
			PSXDMA_LOG("*** DMA 8 - DEV9 unknown *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
			break;
#endif
	}
	HW_DMA8_CHCR &= ~0x01000000;
	psxDmaInterrupt2(1);
}

void  dev9Interrupt() {
	if (dev9Handler == NULL) goto irq;
	if (dev9Handler() != 1) {
		psxRegs.interrupt&= ~(1 << 20);
		return;
		}
irq:
	psxHu32(0x1070)|= 1<<13;
	//SBUS
	hwIntcIrq(INTC_SBUS);	
	psxRegs.interrupt&= ~(1 << 20);
}

void dev9Irq(int cycles) {
	PSX_INT(20, cycles);
}

void  usbInterrupt() {
	if (usbHandler == NULL) goto irq;
	if (usbHandler() != 1) {
		psxRegs.interrupt&= ~(1 << 21);
		return;
		}
irq:
	psxHu32(0x1070)|= 1<<22;
	//SBUS
	hwIntcIrq(INTC_SBUS);
	psxRegs.interrupt&= ~(1 << 21);
}

void usbIrq(int cycles) {
	PSX_INT(21, cycles);
}

void fwIrq() {
	psxHu32(0x1070)|= 1<<24;
	//SBUS
	hwIntcIrq(INTC_SBUS);
}

void spu2DMA4Irq() {
	SPU2interruptDMA4();
	//HW_DMA4_BCR = 0;
	HW_DMA4_CHCR &= ~0x01000000;
	psxDmaInterrupt(4);
}

void spu2DMA7Irq() {
	SPU2interruptDMA7();
	//HW_DMA7_BCR = 0;
	HW_DMA7_CHCR &= ~0x01000000;
	psxDmaInterrupt2(0);
}

void spu2Irq() {
	psxHu32(0x1070)|= 1<<9;
	//SBUS
	hwIntcIrq(INTC_SBUS);
}
