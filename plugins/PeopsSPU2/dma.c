/***************************************************************************
                            dma.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2004/04/04 - Pete
// - changed plugin to emulate PS2 spu
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"
#include "externals.h"
#include "registers.h"
#include "debug.h"
extern void (CALLBACK *irqCallbackDMA4)();                  // func of main emu, called on spu irq
extern void (CALLBACK *irqCallbackDMA7)();                  // func of main emu, called on spu irq
extern void (CALLBACK *irqCallbackSPU2)();                  // func of main emu, called on spu irq
unsigned short interrupt;
extern unsigned long SPUCycles;
unsigned long SPUStartCycle[2];
unsigned long SPUTargetCycle[2];

unsigned long MemAddr[2];

ADMA Adma4;
ADMA Adma7;

////////////////////////////////////////////////////////////////////////
// READ DMA (many values)
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2readDMA4Mem(unsigned short * pusPSXMem,int iSize)
{
 int i;
#ifdef _WINDOWS
 if(iDebugMode==1)
  {
   logprintf("READDMA4 %X - %X\r\n",spuAddr2[0],iSize);

   if(spuAddr2[0]<=0x1fff)
    logprintf("# OUTPUT AREA ACCESS #############\r\n");
  }

#endif

 for(i=0;i<iSize;i++)
  {
   *pusPSXMem++=spuMem[spuAddr2[0]];                  // spu addr 0 got by writeregister
   if(spuCtrl2[0]&0x40 && spuIrq2[0] == spuAddr2[0]){
	   regArea[0x7C0] |= 0x4;
       regArea[PS2_IRQINFO] |= 0x4;
	   irqCallbackSPU2();
   }
   spuAddr2[0]++;                                     // inc spu addr
   MemAddr[0]+=2;
   if(spuAddr2[0]>0xfffff) spuAddr2[0]=0;             // wrap
  }

 spuAddr2[0]+=19; //Transfer Local To Host TSAH/L + Data Size + 20 (already +1'd)


 iSpuAsyncWait=0;

 // got from J.F. and Kanodin... is it needed?
 spuStat2[0]&=~0x80;                                     // DMA complete
 //if(regArea[(PS2_C0_ADMAS)>>1] != 1) {
	// if((regArea[(PS2_C0_ATTR)>>1] & 0x30)) {
	  SPUStartCycle[0] = SPUCycles;
	SPUTargetCycle[0] = iSize;
	interrupt |= (1<<1);
	// }
//}
 //regArea[(PS2_C0_ADMAS)>>1] = 0;
}

EXPORT_GCC void CALLBACK SPU2readDMA7Mem(unsigned short * pusPSXMem,int iSize)
{
 int i;
#ifdef _WINDOWS
 if(iDebugMode==1)
  {
   logprintf("READDMA7 %X - %X\r\n",spuAddr2[1],iSize);

   if(spuAddr2[1]<=0x1fff)
    logprintf("# OUTPUT AREA ACCESS #############\r\n");
  }
#endif

 for(i=0;i<iSize;i++)
  {
   *pusPSXMem++=spuMem[spuAddr2[1]];                   // spu addr 1 got by writeregister
   if(spuCtrl2[1]&0x40 && spuIrq2[1] == spuAddr2[1]){
	   regArea[0x7C0] |= 0x8;
       regArea[PS2_IRQINFO] |= 0x8;
	   irqCallbackSPU2();
   }
   spuAddr2[1]++;                                      // inc spu addr
   MemAddr[1]+=2;
   if(spuAddr2[1]>0xfffff) spuAddr2[1]=0;              // wrap
  }

spuAddr2[1]+=19; //Transfer Local To Host TSAH/L + Data Size + 20 (already +1'd)

 iSpuAsyncWait=0;

 // got from J.F. and Kanodin... is it needed?
 spuStat2[1]&=~0x80;                                     // DMA complete
 // if(regArea[(PS2_C1_ADMAS)>>1] != 2) {
	// if((regArea[(PS2_C1_ATTR)>>1] & 0x30)) {
	 SPUStartCycle[1] = SPUCycles;
	SPUTargetCycle[1] = iSize;
	interrupt |= (1<<2);
	// }
 //}
  //regArea[(PS2_C1_ADMAS)>>1] = 0;
}

////////////////////////////////////////////////////////////////////////
// WRITE DMA (many values)
////////////////////////////////////////////////////////////////////////


// AutoDMA's are used to transfer to the DIRECT INPUT area of the spu2 memory
// Left and Right channels are always interleaved together in the transfer so
// the AutoDMA's deinterleaves them and transfers them. An interrupt is
// generated when half of the buffer (256 short-words for left and 256
// short-words for right ) has been transferred. Another interrupt occurs at
// the end of the transfer.

int ADMAS4Write()
{
 if(interrupt & 0x2) return 0;
 if(Adma4.AmountLeft <= 0) {
	 if(Adma4.TempAmount == 0) return 1;
	 Adma4.AmountLeft = Adma4.TempAmount;
	 Adma4.MemAddr = Adma4.TempMem;
	 Adma4.TempMem = NULL;
	 Adma4.TempAmount = 0;
 }
 Adma4.TransferAmount = min(512, Adma4.AmountLeft);
 if(Adma4.ADMAPos == 512) Adma4.ADMAPos = 0;

#ifdef _WINDOWS
 if(iDebugMode==1)
  {
   logprintf("ADMAWRITE4 %X - %X\r\n",spuAddr2[0],Adma4.AmountLeft);
   if(Adma4.AmountLeft<512) logprintf("FUCK YOU %X\r\n",Adma4.AmountLeft);
  }
#endif

 // SPU2 Deinterleaves the Left and Right Channels
 memcpy((short*)(spuMem + Adma4.ADMAPos + 0x2000),(short*)Adma4.MemAddr,Adma4.TransferAmount);
 Adma4.MemAddr += Adma4.TransferAmount / 2;
 memcpy((short*)(spuMem + Adma4.ADMAPos + 0x2200),(short*)Adma4.MemAddr,Adma4.TransferAmount);
 Adma4.MemAddr += Adma4.TransferAmount / 2;

 Adma4.ADMAPos += Adma4.TransferAmount / 2;
 MemAddr[0] += Adma4.TransferAmount * 2;
 //MemAddr[0] += 1024;
 Adma4.AmountLeft-= Adma4.TransferAmount;
 spuStat2[0]&=~0x80;

 if(Adma4.AmountLeft == 0)
 {
	 if(Adma4.IRQ == 0){
		 Adma4.IRQ = 1;
		irqCallbackDMA4();
	 }
 }
 return 0;
}


int ADMAS7Write()
{
 if(interrupt & 0x4) return 0;
 if(Adma7.AmountLeft <= 0) {
	 if(Adma7.TempAmount == 0) return 1;
	 Adma7.AmountLeft = Adma7.TempAmount;
	 Adma7.MemAddr = Adma7.TempMem;
	 Adma7.TempMem = NULL;
	 Adma7.TempAmount = 0;
 }
 Adma7.TransferAmount = min(512, Adma7.AmountLeft);
 if(Adma7.ADMAPos == 512) Adma7.ADMAPos = 0;

#ifdef _WINDOWS
 if(iDebugMode==1)
  {
   logprintf("ADMAWRITE7 %X - %X\r\n",spuAddr2[1],Adma7.AmountLeft);
   if(Adma7.AmountLeft<512) logprintf("FUCK YOU %X\r\n",Adma7.AmountLeft);
  }
#endif

 // SPU2 Deinterleaves the Left and Right Channels
 memcpy((short*)(spuMem + Adma7.ADMAPos + 0x2400),(short*)Adma7.MemAddr,Adma7.TransferAmount);
 Adma7.MemAddr += Adma7.TransferAmount / 2;
 memcpy((short*)(spuMem + Adma7.ADMAPos + 0x2600),(short*)Adma7.MemAddr,Adma7.TransferAmount);
 Adma7.MemAddr += Adma7.TransferAmount / 2;

 Adma7.ADMAPos += Adma7.TransferAmount / 2;
 MemAddr[1] += Adma7.TransferAmount * 2;
 //MemAddr[1] += 1024;
 Adma7.AmountLeft-=Adma7.TransferAmount;
 spuStat2[1]&=~0x80;

 if(Adma7.AmountLeft == 0)
 {
	 if(Adma7.IRQ == 0){
		 Adma7.IRQ = 1;
		irqCallbackDMA7();
	 }
 }
 return 0;
}

#include <stdio.h>
extern FILE * LogFile;
EXPORT_GCC void CALLBACK SPU2writeDMA4Mem(short * pMem,unsigned int iSize)
{
 //if(Adma4.AmountLeft > 0) return;
 if(regArea[PS2_C0_ADMAS] & 0x1 && (spuCtrl2[0] & 0x30) == 0 && iSize)
 {
	 //fwrite(pMem,iSize<<1,1,LogFile);
	// memset(&Adma4,0,sizeof(ADMA));
	 //if( !Adma4.Enabled )
           // Adma4.Index = 0;
	 //Adma4.ADMAPos = 0;
	 if((Adma4.ADMAPos == 512 && Adma4.Index <= 256) || (Adma4.ADMAPos == 256 && Adma4.Index >= 256) || Adma4.AmountLeft >= 512) {
		 Adma4.TempMem = pMem;
		 Adma4.TempAmount = iSize;
	 } else {
		Adma4.MemAddr = pMem;
		Adma4.AmountLeft += iSize;
        ADMAS4Write();
	 }
	 return;
 }

#ifdef _WINDOWS
 if(iDebugMode==1)
  {
   logprintf("WRITEDMA4 %X - %X\r\n",spuAddr2[0],iSize);
  }
#endif

 memcpy((unsigned char*)(spuMem+spuAddr2[0]),(unsigned char*)pMem,iSize<<1);
 if(spuCtrl2[0]&0x40 && (spuIrq2[0] >= spuAddr2[0] && spuIrq2[0] <= (spuAddr2[0] + iSize))){
	   regArea[0x7C0] |= 0x4;
       regArea[PS2_IRQINFO] |= 0x4;
	   irqCallbackSPU2();
 }
 spuAddr2[0] += iSize;

 if(spuAddr2[0]>0x23FF) spuAddr2[0] = 0x2000;

 MemAddr[0] += iSize<<1;
 spuStat2[0]&=~0x80;
 SPUStartCycle[0] = SPUCycles;
 SPUTargetCycle[0] = 1;//iSize;
 interrupt |= (1<<1);
}

void LogRawSound(void* pleft, int leftstride, void* pright, int rightstride, int numsamples)
{
#ifdef _DEBUG
    static FILE* g_fLogSound = NULL;

    char* left = (char*)pleft;
    char* right = (char*)pright;
    unsigned short* tempbuf;
    int i;

    if( g_fLogSound == NULL ) {
        g_fLogSound = fopen("rawsndbuf.pcm", "wb");
        if( g_fLogSound == NULL )
            return;
    }

    tempbuf = (unsigned short*)malloc(4*numsamples);

    for(i = 0; i < numsamples; ++i) {
        tempbuf[2*i+0] = *(unsigned short*)left;
        tempbuf[2*i+1] = *(unsigned short*)right;
        left += leftstride;
        right += rightstride;
    }

    fwrite(&tempbuf[0], 4*numsamples, 1, g_fLogSound);
    free(tempbuf);
#endif
}

EXPORT_GCC void CALLBACK SPU2writeDMA7Mem(unsigned short * pMem,int iSize)
{
 // For AutoDMA, the ATTR register's bit 5 and 6 are cleared.
 // bit 5 means Data Input Thru Register
 // bit 6 means Data Input Thru DMA
 //if(Adma7.AmountLeft > 0) return;

 if((regArea[PS2_C1_ADMAS] & 0x2) && (spuCtrl2[1] & 0x30) == 0 && iSize)
 {
	//fwrite(pMem,iSize<<1,1,LogFile);
	// memset(&Adma7,0,sizeof(ADMA));
	 //if( !Adma7.Enabled )
           // Adma7.Index = 0;
	 //Adma7.ADMAPos = 0;
	 if((Adma7.ADMAPos == 512 && Adma7.Index <= 256) || (Adma7.ADMAPos == 256 && Adma7.Index >= 256) || Adma7.AmountLeft >= 512) {
		 Adma7.TempMem = pMem;
		 Adma7.TempAmount = iSize;
	 } else {
		Adma7.MemAddr = pMem;
		Adma7.AmountLeft += iSize;
        ADMAS7Write();
	 }
	 return;
 }
#ifdef _WINDOWS
 if(iDebugMode==1)
  {
   logprintf("WRITEDMA7 %X - %X\r\n",spuAddr2[1],iSize);
  }
#endif

 memcpy((short*)(spuMem+spuAddr2[1]),(short*)pMem,iSize<<1);
 if(spuCtrl2[1]&0x40 && (spuIrq2[1] >= spuAddr2[1] && spuIrq2[1] <= (spuAddr2[1] + iSize))){
   regArea[0x7C0] |= 0x8;
   regArea[PS2_IRQINFO] |= 8;
   irqCallbackSPU2();
 }
 spuAddr2[1] += iSize;

 if(spuAddr2[1]>0x27FF) spuAddr2[1] = 0x2400;

 MemAddr[1] += iSize<<1;
 spuStat2[1]&=~0x80;
 SPUStartCycle[1] = SPUCycles;
 SPUTargetCycle[1] = 1;//iSize;
 interrupt |= (1<<2);
}


////////////////////////////////////////////////////////////////////////
// INTERRUPTS
////////////////////////////////////////////////////////////////////////

void InterruptDMA4(void)
{
// taken from linuzappz NULL spu2
//	spu2Rs16(CORE0_ATTR)&= ~0x30;
//	spu2Rs16(REG__1B0) = 0;
//	spu2Rs16(SPU2_STATX_WRDY_M)|= 0x80;

#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("IRQDMA4\r\n");
#endif
	Adma4.IRQ = 0;
	spuCtrl2[0]&=~0x30;
	spuStat2[0]|=0x80;
}

EXPORT_GCC void CALLBACK SPU2interruptDMA4(void)
{
 InterruptDMA4();
}

void InterruptDMA7(void)
{
// taken from linuzappz NULL spu2
//	spu2Rs16(CORE1_ATTR)&= ~0x30;
//	spu2Rs16(REG__5B0) = 0;
//	spu2Rs16(SPU2_STATX_DREQ)|= 0x80;

#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("IRQDMA7\r\n");
#endif
	Adma7.IRQ = 0;
	spuStat2[1]|=0x80;
	spuCtrl2[1]&=~0x30;
}

EXPORT_GCC void CALLBACK SPU2interruptDMA7(void)
{
 InterruptDMA7();
}

EXPORT_GCC void CALLBACK SPU2WriteMemAddr(int core, unsigned long value)
{
	MemAddr[core] = value;
}

EXPORT_GCC unsigned long CALLBACK SPU2ReadMemAddr(int core)
{
	return MemAddr[core];
}
