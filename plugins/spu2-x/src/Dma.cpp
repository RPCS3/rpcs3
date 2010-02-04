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

#include "Global.h"
#include "dma.h"

#include "PS2E-spu2.h"	// temporary until I resolve cyclePtr/TimeUpdate dependencies.

extern u8 callirq;

static FILE *DMA4LogFile		= NULL;
static FILE *DMA7LogFile		= NULL;
static FILE *ADMA4LogFile		= NULL;
static FILE *ADMA7LogFile		= NULL;
static FILE *ADMAOutLogFile	= NULL;

static FILE *REGWRTLogFile[2] = {0,0};

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

void DMALogClose()
{
	safe_fclose(DMA4LogFile);
	safe_fclose(DMA7LogFile);
	safe_fclose(REGWRTLogFile[0]);
	safe_fclose(REGWRTLogFile[1]);
	safe_fclose(ADMA4LogFile);
	safe_fclose(ADMA7LogFile);
	safe_fclose(ADMAOutLogFile);
}

void V_Core::LogAutoDMA( FILE* fp )
{
	if( !DMALog() || !fp ) return;
	fwrite( DMAPtr+InputDataProgress, 0x400, 1, fp );
}

void V_Core::AutoDMAReadBuffer(int mode) //mode: 0= split stereo; 1 = do not split stereo
{
#ifndef ENABLE_NEW_IOPDMA_SPU2
	int spos = ((InputPosRead+0xff)&0x100); //starting position of the free buffer

	LogAutoDMA( Index ? ADMA7LogFile : ADMA4LogFile );

	// HACKFIX!! DMAPtr can be invalid after a savestate load, so the savestate just forces it
	// to NULL and we ignore it here.  (used to work in old VM editions of PCSX2 with fixed
	// addressing, but new PCSX2s have dynamic memory addressing).

	if(mode)
	{		
		if( DMAPtr != NULL )
			memcpy((ADMATempBuffer+(spos<<1)),DMAPtr+InputDataProgress,0x400);
		MADR+=0x400;
		InputDataLeft-=0x200;
		InputDataProgress+=0x200;
	}
	else
	{
		if( DMAPtr != NULL )
			memcpy((ADMATempBuffer+spos),DMAPtr+InputDataProgress,0x200);
			//memcpy((spu2mem+0x2000+(core<<10)+spos),DMAPtr+InputDataProgress,0x200);
		MADR+=0x200;
		InputDataLeft-=0x100;
		InputDataProgress+=0x100;

		if( DMAPtr != NULL )
			memcpy((ADMATempBuffer+spos+0x200),DMAPtr+InputDataProgress,0x200);
			//memcpy((spu2mem+0x2200+(core<<10)+spos),DMAPtr+InputDataProgress,0x200);
		MADR+=0x200;
		InputDataLeft-=0x100;
		InputDataProgress+=0x100;
	}
	// See ReadInput at mixer.cpp for explanation on the commented out lines
	//
#endif
}

void V_Core::StartADMAWrite(u16 *pMem, u32 sz)
{
#ifndef ENABLE_NEW_IOPDMA_SPU2
	int size = (sz)&(~511);

	if(MsgAutoDMA()) ConLog(" * SPU2: DMA%c AutoDMA Transfer of %d bytes to %x (%02x %x %04x).\n",
		GetDmaIndexChar(), size<<1, TSA, DMABits, AutoDMACtrl, (~Regs.ATTR)&0x7fff);

	InputDataProgress = 0;
	if((AutoDMACtrl&(Index+1))==0)
	{
		TSA = 0x2000 + (Index<<10);
		DMAICounter = size;
	}
	else if(size>=512)
	{
		InputDataLeft = size;
		if(AdmaInProgress==0)
		{
#ifdef PCM24_S1_INTERLEAVE
			if((Index==1)&&((PlayMode&8)==8))
			{
				AutoDMAReadBuffer(Index,1);
			}
			else
			{
				AutoDMAReadBuffer(Index,0);
			}
#else
			if( ((PlayMode&4)==4) && (Index==0) )
				Cores[0].InputPosRead=0;

			AutoDMAReadBuffer(0);
#endif

			if(size==512)
				DMAICounter = size;
		}

		AdmaInProgress = 1;
	}
	else
	{
		InputDataLeft	= 0;
		DMAICounter		= 1;
	}
	TADR = MADR + (size<<1);
#endif
}

// HACKFIX: The BIOS breaks if we check the IRQA for both cores when issuing DMA writes.  The
// breakage is a null psxRegs.pc being loaded form some memory address (haven't traced it deeper
// yet).  We get around it by only checking the current core's IRQA, instead of doing the
// *correct* thing and checking both.  This might break some games, but having a working BIOS
// is more important for now, until a proper fix can be uncovered.
//
// This problem might be caused by bad DMA timings in the IOP or a lack of proper IRQ
// handling by the Effects Processor.  After those are implemented, let's hope it gets
// magically fixed?
//
// Note: This appears to affect DMA Writes only, so DMA Read DMAs are left intact (both core
// IRQAs are tested).  Very few games use DMA reads tho, so it could just be a case of "works
// by the grace of not being used."
//
// Update: This hack is no longer needed when we don't do a core reset. Guess the null pc was in spu2 memory?
#define NO_BIOS_HACKFIX   1			// set to 1 to disable the hackfix

void V_Core::PlainDMAWrite(u16 *pMem, u32 size)
{
	// Perform an alignment check.
	// Not really important.  Everything should work regardless,
	// but it could be indicative of an emulation foopah elsewhere.

#if 0
	uptr pa = ((uptr)pMem)&7;
	uptr pm = TSA&0x7;

	if( pa )
	{
		fprintf(stderr, "* SPU2 DMA Write > Missaligned SOURCE! Core: %d  TSA: 0x%x  TDA: 0x%x  Size: 0x%x\n", core, TSA, TDA, size);
	}

	if( pm )
	{
		fprintf(stderr, "* SPU2 DMA Write > Missaligned TARGET! Core: %d  TSA: 0x%x  TDA: 0x%x Size: 0x%x\n", core, TSA, TDA, size );
	}
#endif

	if(Index==0)
		DMA4LogWrite(pMem,size<<1);
	else
		DMA7LogWrite(pMem,size<<1);

	if(MsgDMA()) ConLog(" * SPU2: DMA%c Transfer of %d bytes to %x (%02x %x %04x).\n",GetDmaIndexChar(),size<<1,TSA,DMABits,AutoDMACtrl,(~Regs.ATTR)&0x7fff);

	TSA &= 0xfffff;

	u32 buff1end = TSA + size;
	u32 buff2end=0;
	if( buff1end > 0x100000 )
	{
		buff2end = buff1end - 0x100000;
		buff1end = 0x100000;
	}

	const int cacheIdxStart = TSA / pcm_WordsPerBlock;
	const int cacheIdxEnd = (buff1end+pcm_WordsPerBlock-1) / pcm_WordsPerBlock;
	PcmCacheEntry* cacheLine = &pcm_cache_data[cacheIdxStart];
	PcmCacheEntry& cacheEnd = pcm_cache_data[cacheIdxEnd];

	do 
	{
		cacheLine->Validated = false;
		cacheLine++;
	} while ( cacheLine != &cacheEnd );

	//ConLog( " * SPU2 : Cache Clear Range!  TSA=0x%x, TDA=0x%x (low8=0x%x, high8=0x%x, len=0x%x)\n",
	//	TSA, buff1end, flagTSA, flagTDA, clearLen );


	// First Branch needs cleared:
	// It starts at TSA and goes to buff1end.

	const u32 buff1size = (buff1end-TSA);
	memcpy( GetMemPtr( TSA ), pMem, buff1size*2 );

	if( buff2end > 0 )
	{
		// second branch needs copied:
		// It starts at the beginning of memory and moves forward to buff2end

		// endpoint cache should be irrelevant, since it's almost certainly dynamic 
		// memory below 0x2800 (registers and such)
		//const u32 endpt2 = (buff2end + roundUp) / indexer_scalar;
		//memset( pcm_cache_flags, 0, endpt2 );

		// Emulation Grayarea: Should addresses wrap around to zero, or wrap around to
		// 0x2800?  Hard to know for sure (almost no games depend on this)

		memcpy( GetMemPtr( 0 ), &pMem[buff1size], buff2end*2 );
		TDA = (buff2end+1) & 0xfffff;

		// Flag interrupt?  If IRQA occurs between start and dest, flag it.
		// Important: Test both core IRQ settings for either DMA!
		// Note: Because this buffer wraps, we use || instead of &&

#if NO_BIOS_HACKFIX
		for( int i=0; i<2; i++ )
		{
			// Note: (start is inclusive, dest exclusive -- fixes DMC1 FMVs)

			if( Cores[i].IRQEnable && (Cores[i].IRQA >= TSA) || (Cores[i].IRQA < TDA) )
			{
				Spdif.Info = 4 << i;
				SetIrqCall();
			}
		}
#else
		if( IRQEnable && (IRQA >= TSA) || (IRQA < TDA) )
		{
			Spdif.Info = 4 << Index;
			SetIrqCall();
		}
#endif
	}
	else
	{
		// Buffer doesn't wrap/overflow!
		// Just set the TDA and check for an IRQ...
		
		TDA = buff1end;

		// Flag interrupt?  If IRQA occurs between start and dest, flag it.
		// Important: Test both core IRQ settings for either DMA!

#if NO_BIOS_HACKFIX
		for( int i=0; i<2; i++ )
		{
			// Note: (start is inclusive, dest exclusive -- fixes DMC1 FMVs)

			if( Cores[i].IRQEnable && (Cores[i].IRQA >= TSA) && (Cores[i].IRQA < TDA) )
			{
				Spdif.Info = 4 << i;
				SetIrqCall();
			}
		}
#else
		if( IRQEnable && (IRQA >= TSA) && (IRQA < TDA) )
		{
			Spdif.Info = 4 << Index;
			SetIrqCall();
		}
#endif
	}

	TSA			= TDA & 0xFFFF0;
	DMAICounter	= size;
	TADR		= MADR + (size<<1);
}

void V_Core::DoDMAread(u16* pMem, u32 size) 
{
#ifndef ENABLE_NEW_IOPDMA_SPU2
	TSA &= 0xffff8;

	u32 buff1end = TSA + size;
	u32 buff2end = 0;
	if( buff1end > 0x100000 )
	{
		buff2end = buff1end - 0x100000;
		buff1end = 0x100000;
	}

	const u32 buff1size = (buff1end-TSA);
	memcpy( pMem, GetMemPtr( TSA ), buff1size*2 );

	// Note on TSA's position after our copy finishes:
	// IRQA should be measured by the end of the writepos+0x20.  But the TDA
	// should be written back at the precise endpoint of the xfer.

	if( buff2end > 0 )
	{
		// second branch needs cleared:
		// It starts at the beginning of memory and moves forward to buff2end

		memcpy( &pMem[buff1size], GetMemPtr( 0 ), buff2end*2 );

		TDA = (buff2end+0x20) & 0xfffff;

		// Flag interrupt?  If IRQA occurs between start and dest, flag it.
		// Important: Test both core IRQ settings for either DMA!
		// Note: Because this buffer wraps, we use || instead of &&

		for( int i=0; i<2; i++ )
		{
			if( Cores[i].IRQEnable && (Cores[i].IRQA >= TSA) || (Cores[i].IRQA < TDA) )
			{
				Spdif.Info = 4 << i;
				SetIrqCall();
			}
		}
	}
	else
	{
		// Buffer doesn't wrap/overflow!
		// Just set the TDA and check for an IRQ...

		TDA = (buff1end + 0x20) & 0xfffff;

		// Flag interrupt?  If IRQA occurs between start and dest, flag it.
		// Important: Test both core IRQ settings for either DMA!

		for( int i=0; i<2; i++ )
		{
			if( Cores[i].IRQEnable && (Cores[i].IRQA >= TSA) && (Cores[i].IRQA < TDA) )
			{
				Spdif.Info = 4 << i;
				SetIrqCall();
			}
		}
	}

	TSA = TDA & 0xFFFFF;

	DMAICounter	= size;
	Regs.STATX &= ~0x80;
	//Regs.ATTR |= 0x30;
	TADR		= MADR + (size<<1);
#endif
}

void V_Core::DoDMAwrite(u16* pMem, u32 size) 
{
#ifndef ENABLE_NEW_IOPDMA_SPU2
	DMAPtr = pMem;

	if(size<2) {
		//if(dma7callback) dma7callback();
		Regs.STATX &= ~0x80;
		//Regs.ATTR |= 0x30;
		DMAICounter=1;

		return;
	}

	if( IsDevBuild )
		DebugCores[Index].lastsize = size;

	TSA &= ~7;

	bool adma_enable = ((AutoDMACtrl&(Index+1))==(Index+1));

	if(adma_enable)
	{
		TSA&=0x1fff;
		StartADMAWrite(pMem,size);
	}
	else
	{
		PlainDMAWrite(pMem,size);
	}
	Regs.STATX &= ~0x80;
	//Regs.ATTR |= 0x30;
#endif
}

s32 V_Core::NewDmaRead(u32* data, u32 bytesLeft, u32* bytesProcessed)
{
#ifdef ENABLE_NEW_IOPDMA_SPU2
	bool DmaStarting = !DmaStarted;
	DmaStarted = true;

	TSA &= 0xffff8;

	u16* pMem = (u16*)data;

	u32 buff1end = TSA + bytesLeft;
	u32 buff2end = 0;
	if( buff1end > 0x100000 )
	{
		buff2end = buff1end - 0x100000;
		buff1end = 0x100000;
	}

	const u32 buff1size = (buff1end-TSA);
	memcpy( pMem, GetMemPtr( TSA ), buff1size*2 );

	// Note on TSA's position after our copy finishes:
	// IRQA should be measured by the end of the writepos+0x20.  But the TDA
	// should be written back at the precise endpoint of the xfer.

	if( buff2end > 0 )
	{
		// second branch needs cleared:
		// It starts at the beginning of memory and moves forward to buff2end

		memcpy( &pMem[buff1size], GetMemPtr( 0 ), buff2end*2 );

		TDA = (buff2end+0x20) & 0xfffff;

		// Flag interrupt?  If IRQA occurs between start and dest, flag it.
		// Important: Test both core IRQ settings for either DMA!
		// Note: Because this buffer wraps, we use || instead of &&

		for( int i=0; i<2; i++ )
		{
			if( Cores[i].IRQEnable && (Cores[i].IRQA >= TSA) || (Cores[i].IRQA < TDA) )
			{
				Spdif.Info = 4 << i;
				SetIrqCall();
			}
		}
	}
	else
	{
		// Buffer doesn't wrap/overflow!
		// Just set the TDA and check for an IRQ...

		TDA = (buff1end + 0x20) & 0xfffff;

		// Flag interrupt?  If IRQA occurs between start and dest, flag it.
		// Important: Test both core IRQ settings for either DMA!

		for( int i=0; i<2; i++ )
		{
			if( Cores[i].IRQEnable && (Cores[i].IRQA >= TSA) && (Cores[i].IRQA < TDA) )
			{
				Spdif.Info = 4 << i;
				SetIrqCall();
			}
		}
	}

	TSA = TDA & 0xFFFFF;

	Regs.STATX &= ~0x80;
	//Regs.ATTR |= 0x30;

#endif
	*bytesProcessed = bytesLeft;
	return 0;
}

s32 V_Core::NewDmaWrite(u32* data, u32 bytesLeft, u32* bytesProcessed)
{
#ifdef ENABLE_NEW_IOPDMA_SPU2
	bool DmaStarting = !DmaStarted;
	DmaStarted = true;

	if(bytesLeft<2) 
	{
		// execute interrupt code early
		NewDmaInterrupt();

		*bytesProcessed = bytesLeft;
		return 0;
	}

	if( IsDevBuild )
		DebugCores[Index].lastsize = bytesLeft;

	TSA &= ~7;

	bool adma_enable = ((AutoDMACtrl&(Index+1))==(Index+1));

	if(adma_enable)
	{
		TSA&=0x1fff;
		//Console.Error(" * SPU2: AutoDMA transfers not supported yet! (core %d)\n", Index);

		if(DmaStarting)
		{
			if(MsgAutoDMA()) ConLog(" * SPU2: DMA%c AutoDMA Transfer of %d bytes to %x (%02x %x %04x).\n",
				GetDmaIndexChar(), bytesLeft<<1, TSA, DMABits, AutoDMACtrl, (~Regs.ATTR)&0x7fff);
		}

		u32 processed = 0;
		while((AutoDmaFree>0)&&(bytesLeft>=0x400))
		{
			// copy block

			LogAutoDMA( Index ? ADMA7LogFile : ADMA4LogFile );

			// HACKFIX!! DMAPtr can be invalid after a savestate load, so the savestate just forces it
			// to NULL and we ignore it here.  (used to work in old VM editions of PCSX2 with fixed
			// addressing, but new PCSX2s have dynamic memory addressing).

			s16* mptr = (s16*)data;

			if(false)//(mode)
			{		
				memcpy((ADMATempBuffer+(InputPosWrite<<1)),mptr,0x400);
				mptr+=0x200;
			}
			else
			{
				memcpy((ADMATempBuffer+InputPosWrite),mptr,0x200);
				//memcpy((spu2mem+0x2000+(core<<10)+InputPosWrite),mptr,0x200);
				mptr+=0x100;

				memcpy((ADMATempBuffer+InputPosWrite+0x200),mptr,0x200);
				//memcpy((spu2mem+0x2200+(core<<10)+InputPosWrite),mptr,0x200);
				mptr+=0x100;
			}
			// See ReadInput at mixer.cpp for explanation on the commented out lines
			//

			InputPosWrite = (InputPosWrite + 0x100) & 0x1ff;
			AutoDmaFree -= 0x200;
			processed += 0x400;
			bytesLeft -= 0x400;
		}

		if(processed==0)
		{
			*bytesProcessed = 0;
			return 768*15; // pause a bit
		}
		else
		{
			*bytesProcessed = processed;
			return 0; // auto pause
		}
	}
	else
	{
		// TODO: Sliced transfers?
		PlainDMAWrite((u16*)data,bytesLeft/2);
	}
	Regs.STATX &= ~0x80;
	//Regs.ATTR |= 0x30;
#endif
	*bytesProcessed = bytesLeft;
	return 0;
}

void V_Core::NewDmaInterrupt()
{
#ifdef ENABLE_NEW_IOPDMA_SPU2
	FileLog("[%10d] SPU2 interruptDMA4\n",Cycles);
	Regs.STATX |= 0x80;
	//Regs.ATTR &= ~0x30;
	DmaStarted = false;
#endif
}