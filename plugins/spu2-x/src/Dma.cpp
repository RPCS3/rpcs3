/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Spu2.h"

extern u8 callirq;

FILE *DMA4LogFile=0;
FILE *DMA7LogFile=0;
FILE *ADMA4LogFile=0;
FILE *ADMA7LogFile=0;
FILE *ADMAOutLogFile=0;

FILE *REGWRTLogFile[2]={0,0};

int packcount=0;

u16* MBASE[2] = {0,0};

u16* DMABaseAddr;

void DMALogOpen()
{
	if(!DMALog()) return;
	DMA4LogFile    = fopen( Unicode::Convert( DMA4LogFileName ).c_str(), "wb");
	DMA7LogFile    = fopen( Unicode::Convert( DMA7LogFileName ).c_str(), "wb");
	ADMA4LogFile   = fopen( "logs/adma4.raw", "wb" );
	ADMA7LogFile   = fopen( "logs/adma7.raw", "wb" );
	ADMAOutLogFile = fopen( "logs/admaOut.raw", "wb" );
	//REGWRTLogFile[0]=fopen("logs/RegWrite0.raw","wb");
	//REGWRTLogFile[1]=fopen("logs/RegWrite1.raw","wb");
}
void DMA4LogWrite(void *lpData, u32 ulSize) {
	if(!DMALog()) return;
	if (!DMA4LogFile) return;
	fwrite(lpData,ulSize,1,DMA4LogFile);
}

void DMA7LogWrite(void *lpData, u32 ulSize) {
	if(!DMALog()) return;
	if (!DMA7LogFile) return;
	fwrite(lpData,ulSize,1,DMA7LogFile);
}

void ADMA4LogWrite(void *lpData, u32 ulSize) {
	if(!DMALog()) return;
	if (!ADMA4LogFile) return;
	fwrite(lpData,ulSize,1,ADMA4LogFile);
}
void ADMA7LogWrite(void *lpData, u32 ulSize) {
	if(!DMALog()) return;
	if (!ADMA7LogFile) return;
	fwrite(lpData,ulSize,1,ADMA7LogFile);
}
void ADMAOutLogWrite(void *lpData, u32 ulSize) {
	if(!DMALog()) return;
	if (!ADMAOutLogFile) return;
	fwrite(lpData,ulSize,1,ADMAOutLogFile);
}

void RegWriteLog(u32 core,u16 value)
{
	if(!DMALog()) return;
	if (!REGWRTLogFile[core]) return;
	fwrite(&value,2,1,REGWRTLogFile[core]);
}

void DMALogClose() {
	if(!DMALog()) return;
	if (DMA4LogFile) fclose(DMA4LogFile);
	if (DMA7LogFile) fclose(DMA7LogFile);
	if (REGWRTLogFile[0]) fclose(REGWRTLogFile[0]);
	if (REGWRTLogFile[1]) fclose(REGWRTLogFile[1]);
	if (ADMA4LogFile) fclose(ADMA4LogFile);
	if (ADMA7LogFile) fclose(ADMA7LogFile);
	if (ADMAOutLogFile) fclose(ADMAOutLogFile);
}


__forceinline u16 DmaRead(u32 core)
{
	const u16 ret = (u16)spu2M_Read(Cores[core].TDA);
	Cores[core].TDA++;
	Cores[core].TDA&=0xfffff;
	return ret;
}

__forceinline void DmaWrite(u32 core, u16 value)
{
	spu2M_Write( Cores[core].TSA, value );
	Cores[core].TSA++;
	Cores[core].TSA&=0xfffff;
}

void AutoDMAReadBuffer(int core, int mode) //mode: 0= split stereo; 1 = do not split stereo
{
	int spos=((Cores[core].InputPos+0xff)&0x100); //starting position of the free buffer

	if(core==0)
		ADMA4LogWrite(Cores[core].DMAPtr+Cores[core].InputDataProgress,0x400);
	else
		ADMA7LogWrite(Cores[core].DMAPtr+Cores[core].InputDataProgress,0x400);

	if(mode)
	{
		//hacky :p

		memcpy((Cores[core].ADMATempBuffer+(spos<<1)),Cores[core].DMAPtr+Cores[core].InputDataProgress,0x400);
		Cores[core].MADR+=0x400;
		Cores[core].InputDataLeft-=0x200;
		Cores[core].InputDataProgress+=0x200;
	}
	else
	{
		memcpy((Cores[core].ADMATempBuffer+spos),Cores[core].DMAPtr+Cores[core].InputDataProgress,0x200);
		//memcpy((spu2mem+0x2000+(core<<10)+spos),Cores[core].DMAPtr+Cores[core].InputDataProgress,0x200);
		Cores[core].MADR+=0x200;
		Cores[core].InputDataLeft-=0x100;
		Cores[core].InputDataProgress+=0x100;

		memcpy((Cores[core].ADMATempBuffer+spos+0x200),Cores[core].DMAPtr+Cores[core].InputDataProgress,0x200);
		//memcpy((spu2mem+0x2200+(core<<10)+spos),Cores[core].DMAPtr+Cores[core].InputDataProgress,0x200);
		Cores[core].MADR+=0x200;
		Cores[core].InputDataLeft-=0x100;
		Cores[core].InputDataProgress+=0x100;
	}
	// See ReadInput at mixer.cpp for explanation on the commented out lines
	//
}

void StartADMAWrite(int core,u16 *pMem, u32 sz)
{
	int size=(sz)&(~511);

	if(MsgAutoDMA()) ConLog(" * SPU2: DMA%c AutoDMA Transfer of %d bytes to %x (%02x %x %04x).\n",
		(core==0)?'4':'7',size<<1,Cores[core].TSA,Cores[core].DMABits,Cores[core].AutoDMACtrl,(~Cores[core].Regs.ATTR)&0x7fff);

	Cores[core].InputDataProgress=0;
	if((Cores[core].AutoDMACtrl&(core+1))==0)
	{
		Cores[core].TSA=0x2000+(core<<10);
		Cores[core].DMAICounter=size;
	}
	else if(size>=512)
	{
		Cores[core].InputDataLeft=size;
		if(Cores[core].AdmaInProgress==0)
		{
#ifdef PCM24_S1_INTERLEAVE
			if((core==1)&&((PlayMode&8)==8))
			{
				AutoDMAReadBuffer(core,1);
			}
			else
			{
				AutoDMAReadBuffer(core,0);
			}
#else
			if(((PlayMode&4)==4)&&(core==0))
				Cores[0].InputPos=0;

			AutoDMAReadBuffer(core,0);
#endif

			if(size==512)
				Cores[core].DMAICounter=size;
		}

		Cores[core].AdmaInProgress=1;
	}
	else
	{
		Cores[core].InputDataLeft=0;
		Cores[core].DMAICounter=1;
	}
	Cores[core].TADR=Cores[core].MADR+(size<<1);
}

void DoDMAWrite(int core,u16 *pMem,u32 size)
{
	// Perform an alignment check.
	// Not really important.  Everything should work regardless,
	// but it could be indicative of an emulation foopah elsewhere.

#if 0
	uptr pa = ((uptr)pMem)&7;
	uptr pm = Cores[core].TSA&0x7;

	if( pa )
	{
		fprintf(stderr, "* SPU2 DMA Write > Missaligned SOURCE! Core: %d  TSA: 0x%x  TDA: 0x%x  Size: 0x%x\n", core, Cores[core].TSA, Cores[core].TDA, size);
	}

	if( pm )
	{
		fprintf(stderr, "* SPU2 DMA Write > Missaligned TARGET! Core: %d  TSA: 0x%x  TDA: 0x%x Size: 0x%x\n", core, Cores[core].TSA, Cores[core].TDA, size );
	}
#endif

	if(core==0)
		DMA4LogWrite(pMem,size<<1);
	else
		DMA7LogWrite(pMem,size<<1);

	if(MsgDMA()) ConLog(" * SPU2: DMA%c Transfer of %d bytes to %x (%02x %x %04x).\n",(core==0)?'4':'7',size<<1,Cores[core].TSA,Cores[core].DMABits,Cores[core].AutoDMACtrl,(~Cores[core].Regs.ATTR)&0x7fff);

	Cores[core].TSA &= 0xfffff;

	u32 buff1end = Cores[core].TSA + size;
	u32 buff2end=0;
	if( buff1end > 0x100000 )
	{
		buff2end = buff1end - 0x100000;
		buff1end = 0x100000;
	}

	const int cacheIdxStart = Cores[core].TSA / pcm_WordsPerBlock;
	const int cacheIdxEnd = (buff1end+pcm_WordsPerBlock-1) / pcm_WordsPerBlock;
	PcmCacheEntry* cacheLine = &pcm_cache_data[cacheIdxStart];
	PcmCacheEntry& cacheEnd = pcm_cache_data[cacheIdxEnd];

	do 
	{
		cacheLine->Validated = false;
		cacheLine++;
	} while ( cacheLine != &cacheEnd );

	//ConLog( " * SPU2 : Cache Clear Range!  TSA=0x%x, TDA=0x%x (low8=0x%x, high8=0x%x, len=0x%x)\n",
	//	Cores[core].TSA, buff1end, flagTSA, flagTDA, clearLen );


	// First Branch needs cleared:
	// It starts at TSA and goes to buff1end.

	const u32 buff1size = (buff1end-Cores[core].TSA);
	memcpy( GetMemPtr( Cores[core].TSA ), pMem, buff1size*2 );
	
	if( buff2end > 0 )
	{
		// second branch needs copied:
		// It starts at the beginning of memory and moves forward to buff2end

		// endpoint cache should be irrelevant, since it's almost certainly dynamic 
		// memory below 0x2800 (registers and such)
		//const u32 endpt2 = (buff2end + roundUp) / indexer_scalar;
		//memset( pcm_cache_flags, 0, endpt2 );

		// Emulation Grayarea: Should addresses wrap around to zero, or wrap around to
		// 0x2800?  Hard to know for usre (almost no games depend on this)

		memcpy( GetMemPtr( 0 ), &pMem[buff1size], buff2end*2 );
		Cores[core].TDA = (buff2end+1) & 0xfffff;

		if(Cores[core].IRQEnable)
		{
			// Flag interrupt?
			// If IRQA occurs between start and dest, flag it.
			// Since the buffer wraps, the conditional might seem odd, but it works.

			if( ( Cores[core].IRQA >= Cores[core].TSA ) ||
				( Cores[core].IRQA < Cores[core].TDA ) )
			{
				Spdif.Info = 4 << core;
				SetIrqCall();
			}
		}
	}
	else
	{
		// Buffer doesn't wrap/overflow!
		// Just set the TDA and check for an IRQ...
		
		Cores[core].TDA = buff1end;

		if(Cores[core].IRQEnable)
		{
			// Flag interrupt?
			// If IRQA occurs between start and dest, flag it.
			// (start is inclusive, dest exclusive -- fixes DMC1 and hopefully won't break
			// other games. ;)

			if( ( Cores[core].IRQA >= Cores[core].TSA ) &&
				( Cores[core].IRQA < Cores[core].TDA ) )
			{
				Spdif.Info = 4 << core;
				SetIrqCall();
			}
		}
	}

	Cores[core].TSA=Cores[core].TDA&0xFFFF0;
	Cores[core].DMAICounter=size;
	Cores[core].TADR=Cores[core].MADR+(size<<1);
}

void SPU2readDMA(int core, u16* pMem, u32 size) 
{
	if( cyclePtr != NULL ) TimeUpdate( *cyclePtr );

	Cores[core].TSA &= 0xffff8;

	u32 buff1end = Cores[core].TSA + size;
	u32 buff2end = 0;
	if( buff1end > 0x100000 )
	{
		buff2end = buff1end - 0x100000;
		buff1end = 0x100000;
	}

	const u32 buff1size = (buff1end-Cores[core].TSA);
	memcpy( pMem, GetMemPtr( Cores[core].TSA ), buff1size*2 );

	// Note on TSA's position after our copy finishes:
	// IRQA should be measured by the end of the writepos+0x20.  But the TDA
	// should be written back at the precise endpoint of the xfer.

	if( buff2end > 0 )
	{
		// second branch needs cleared:
		// It starts at the beginning of memory and moves forward to buff2end

		memcpy( &pMem[buff1size], GetMemPtr( 0 ), buff2end*2 );

		Cores[core].TDA = (buff2end+0x20) & 0xfffff;

		for( int i=0; i<2; i++ )
		{
			if(Cores[i].IRQEnable)
			{
				// Flag interrupt?
				// If IRQA occurs between start and dest, flag it.
				// Since the buffer wraps, the conditional might seem odd, but it works.

				if( ( Cores[i].IRQA >= Cores[core].TSA ) ||
					( Cores[i].IRQA <= Cores[core].TDA ) )
				{
					Spdif.Info=4<<i;
					SetIrqCall();
				}
			}
		}
	}
	else
	{
		// Buffer doesn't wrap/overflow!
		// Just set the TDA and check for an IRQ...

		Cores[core].TDA = (buff1end + 0x20) & 0xfffff;

		for( int i=0; i<2; i++ )
		{
			if(Cores[i].IRQEnable)
			{
				// Flag interrupt?
				// If IRQA occurs between start and dest, flag it:

				if( ( Cores[i].IRQA >= Cores[i].TSA ) &&
					( Cores[i].IRQA < Cores[i].TDA ) )
				{
					Spdif.Info=4<<i;
					SetIrqCall();
				}
			}
		}
	}


	Cores[core].TSA=Cores[core].TDA & 0xFFFFF;

	Cores[core].DMAICounter=size;
	Cores[core].Regs.STATX &= ~0x80;
	//Cores[core].Regs.ATTR |= 0x30;
	Cores[core].TADR=Cores[core].MADR+(size<<1);

}

void SPU2writeDMA(int core, u16* pMem, u32 size) 
{
	if(cyclePtr != NULL) TimeUpdate(*cyclePtr);

	Cores[core].DMAPtr=pMem;

	if(size<2) {
		//if(dma7callback) dma7callback();
		Cores[core].Regs.STATX &= ~0x80;
		//Cores[core].Regs.ATTR |= 0x30;
		Cores[core].DMAICounter=1;

		return;
	}

	if( IsDevBuild )
		DebugCores[core].lastsize = size;

	Cores[core].TSA&=~7;

	bool adma_enable = ((Cores[core].AutoDMACtrl&(core+1))==(core+1));

	if(adma_enable)
	{
		Cores[core].TSA&=0x1fff;
		StartADMAWrite(core,pMem,size);
	}
	else
	{
		DoDMAWrite(core,pMem,size);
	}
	Cores[core].Regs.STATX &= ~0x80;
	//Cores[core].Regs.ATTR |= 0x30;
}

u32 CALLBACK SPU2ReadMemAddr(int core)
{
	return Cores[core].MADR;
}
void CALLBACK SPU2WriteMemAddr(int core,u32 value)
{
	Cores[core].MADR=value;
}

void CALLBACK SPU2setDMABaseAddr(uptr baseaddr)
{
   DMABaseAddr = (u16*)baseaddr;
}

void CALLBACK SPU2readDMA4Mem(u16 *pMem, u32 size) { //size now in 16bit units
	FileLog("[%10d] SPU2 readDMA4Mem size %x\n",Cycles, size<<1);
	SPU2readDMA(0,pMem,size);
}

void CALLBACK SPU2writeDMA4Mem(u16* pMem, u32 size) { //size now in 16bit units
	FileLog("[%10d] SPU2 writeDMA4Mem size %x at address %x\n",Cycles, size<<1, Cores[0].TSA);
#ifdef S2R_ENABLE
	if(!replay_mode)
		s2r_writedma4(Cycles,pMem,size);
#endif
	SPU2writeDMA(0,pMem,size);
}

void CALLBACK SPU2interruptDMA4() {
	FileLog("[%10d] SPU2 interruptDMA4\n",Cycles);
	Cores[0].Regs.STATX |= 0x80;
	//Cores[0].Regs.ATTR &= ~0x30;
}

void CALLBACK SPU2readDMA7Mem(u16* pMem, u32 size) {
	FileLog("[%10d] SPU2 readDMA7Mem size %x\n",Cycles, size<<1);

	SPU2readDMA(1,pMem,size);
}

void CALLBACK SPU2writeDMA7Mem(u16* pMem, u32 size) {
	FileLog("[%10d] SPU2 writeDMA7Mem size %x at address %x\n",Cycles, size<<1, Cores[1].TSA);
#ifdef S2R_ENABLE
	if(!replay_mode)
		s2r_writedma7(Cycles,pMem,size);
#endif
	SPU2writeDMA(1,pMem,size);
}

void CALLBACK SPU2interruptDMA7() {
	FileLog("[%10d] SPU2 interruptDMA7\n",Cycles);
	Cores[1].Regs.STATX |= 0x80;
	//Cores[1].Regs.ATTR &= ~0x30;
}

