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
	int spos = ((InputPos+0xff)&0x100); //starting position of the free buffer

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
}

void V_Core::StartADMAWrite(u16 *pMem, u32 sz)
{
	int size = (sz)&(~511);

	if(MsgAutoDMA()) ConLog(" * SPU2: DMA%c AutoDMA Transfer of %d bytes to %x (%02x %x %04x).\n",
		GetDmaIndexChar(), size<<1, TSA, DMABits, AutoDMACtrl, (~Regs.ATTR)&0x7fff);

	InputDataProgress=0;
	if((AutoDMACtrl&(Index+1))==0)
	{
		TSA=0x2000+(Index<<10);
		DMAICounter=size;
	}
	else if(size>=512)
	{
		InputDataLeft=size;
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
			if(((PlayMode&4)==4)&&(Index==0))
				Cores[0].InputPos=0;

			AutoDMAReadBuffer(0);
#endif

			if(size==512)
				DMAICounter=size;
		}

		AdmaInProgress=1;
	}
	else
	{
		InputDataLeft=0;
		DMAICounter=1;
	}
	TADR=MADR+(size<<1);
}

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
		// 0x2800?  Hard to know for usre (almost no games depend on this)

		memcpy( GetMemPtr( 0 ), &pMem[buff1size], buff2end*2 );
		TDA = (buff2end+1) & 0xfffff;
	}
	else
	{
		// Buffer doesn't wrap/overflow!
		// Just set the TDA and check for an IRQ...
		
		TDA = buff1end;
	}

	// Flag interrupt?  If IRQA occurs between start and dest, flag it.
	// Important: Test both core IRQ settings for either DMA!

	for( int i=0; i<2; i++ )
	{
		// Note: (start is inclusive, dest exclusive -- fixes DMC1 FMVs)

		if( Cores[i].IRQEnable && (Cores[i].IRQA >= Cores[i].TSA) && (Cores[i].IRQA < Cores[i].TDA) )
		{
			Spdif.Info = 4 << i;
			SetIrqCall();
		}
	}

	if(IRQEnable)
	{

		if( ( IRQA >= TSA ) && ( IRQA < TDA ) )
		{
			Spdif.Info = 4 << Index;
			SetIrqCall();
		}
	}

	TSA			= TDA & 0xFFFF0;
	DMAICounter	= size;
	TADR		= MADR + (size<<1);
}

void V_Core::DoDMAread(u16* pMem, u32 size) 
{
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
	}
	else
	{
		// Buffer doesn't wrap/overflow!
		// Just set the TDA and check for an IRQ...

		TDA = (buff1end + 0x20) & 0xfffff;
	}

	// Flag interrupt?  If IRQA occurs between start and dest, flag it.
	// Important: Test both core IRQ settings for either DMA!

	for( int i=0; i<2; i++ )
	{
		if( Cores[i].IRQEnable && (Cores[i].IRQA >= Cores[i].TSA) && (Cores[i].IRQA < Cores[i].TDA) )
		{
			Spdif.Info = 4 << i;
			SetIrqCall();
		}
	}

	TSA = TDA & 0xFFFFF;

	DMAICounter	= size;
	Regs.STATX &= ~0x80;
	//Regs.ATTR |= 0x30;
	TADR		= MADR + (size<<1);
}

void V_Core::DoDMAwrite(u16* pMem, u32 size) 
{
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
}
