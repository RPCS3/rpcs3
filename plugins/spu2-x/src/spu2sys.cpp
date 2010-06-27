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

// ======================================================================================
//  spu2sys.cpp -- Emulation module for the SPU2 'virtual machine'
// ======================================================================================
// This module contains (most!) stuff which is directly related to SPU2 emulation.
// Contents should be cross-platform compatible whenever possible.


#include "Global.h"
#include "Dma.h"

#include "PS2E-spu2.h"		// needed until I figure out a nice solution for irqcallback dependencies.

s16* spu2regs = NULL;
s16* _spu2mem = NULL;

V_CoreDebug	DebugCores[2];
V_Core		Cores[2];
V_SPDIF		Spdif;

s16		OutPos;
s16		InputPos;
u32		Cycles;

int		PlayMode;

bool has_to_call_irq=false;

void SetIrqCall(int core)
{
	// reset by an irq disable/enable cycle, behaviour found by
	// test programs that bizarrely only fired one interrupt
	if (Spdif.Info & 4 << core)
		return;
	Spdif.Info |= 4 << core;
	has_to_call_irq=true;
}

__forceinline s16* GetMemPtr(u32 addr)
{
#ifndef DEBUG_FAST
	// In case you're wondering, this assert is the reason SPU2-X
	// runs so incrediously slow in Debug mode. :P
	jASSUME( addr < 0x100000 );
#endif
	return (_spu2mem+addr);
}

__forceinline s16 spu2M_Read( u32 addr )
{
	return *GetMemPtr( addr & 0xfffff );
}

// writes a signed value to the SPU2 ram
// Invalidates the ADPCM cache in the process.
__forceinline void spu2M_Write( u32 addr, s16 value )
{
	// Make sure the cache is invalidated:
	// (note to self : addr address WORDs, not bytes)

	addr &= 0xfffff;
	if( addr >= SPU2_DYN_MEMLINE )
	{
		const int cacheIdx = addr / pcm_WordsPerBlock;
		pcm_cache_data[cacheIdx].Validated = false;

		ConLog( "* SPU2-X: PcmCache Block Clear at 0x%x (cacheIdx=0x%x)\n", addr, cacheIdx);
	}
	*GetMemPtr( addr ) = value;
}

// writes an unsigned value to the SPU2 ram
__forceinline void spu2M_Write( u32 addr, u16 value )
{
	spu2M_Write( addr, (s16)value );
}

V_VolumeLR		V_VolumeLR::Max( 0x7FFFFFFF );
V_VolumeSlideLR	V_VolumeSlideLR::Max( 0x3FFF, 0x7FFFFFFF );

V_Core::V_Core( int coreidx ) : Index( coreidx )
	//LogFile_AutoDMA( NULL )
{
	/*char fname[128];
	sprintf( fname, "logs/adma%d.raw", GetDmaIndex() );
	LogFile_AutoDMA = fopen( fname, "wb" );*/
}

V_Core::~V_Core() throw()
{
	// Can't use this yet because we dumb V_Core into savestates >_<
	/*if( LogFile_AutoDMA != NULL )
	{
		fclose( LogFile_AutoDMA );
		LogFile_AutoDMA = NULL;
	}*/
}

void V_Core::Init( int index )
{
	ConLog( "* SPU2-X: Init SPU2 core %d \n", index );
	memset( this, 0, sizeof(V_Core) );

	const int c = Index = index;

	Regs.STATX		= 0;
	Regs.ATTR		= 0;
	ExtVol			= V_VolumeLR::Max;
	InpVol			= V_VolumeLR::Max;
	FxVol			= V_VolumeLR::Max;

	MasterVol		= V_VolumeSlideLR::Max;

	memset( &DryGate, -1, sizeof(DryGate) );
	memset( &WetGate, -1, sizeof(WetGate) );

	Regs.MMIX		= 0xFFCF;
	Regs.VMIXL		= 0xFFFFFF;
	Regs.VMIXR		= 0xFFFFFF;
	Regs.VMIXEL		= 0xFFFFFF;
	Regs.VMIXER		= 0xFFFFFF;
	EffectsStartA	= c ? 0xEFFF8 : 0xFFFF8;
	EffectsEndA		= c ? 0xEFFFF : 0xFFFFF;

	FxEnable		= 0;
	IRQA			= 0xFFFFF;
	IRQEnable		= 0;

	for( uint v=0; v<NumVoices; ++v )
	{
		VoiceGates[v].DryL = -1;
		VoiceGates[v].DryR = -1;
		VoiceGates[v].WetL = -1;
		VoiceGates[v].WetR = -1;

		Voices[v].Volume		= V_VolumeSlideLR(0,0); // V_VolumeSlideLR::Max;
		Voices[v].SCurrent		= 28;

		Voices[v].ADSR.Value	= 0;
		Voices[v].ADSR.Phase	= 0;
		Voices[v].Pitch			= 0x3FFF;
		Voices[v].NextA			= 0x2800;
		Voices[v].StartA		= 0x2800;
		Voices[v].LoopStartA	= 0x2800;
	}

	DMAICounter		= 0;
	AdmaInProgress	= 0;

	Regs.STATX		= 0x80;
}

void V_Core::Reset( int index )
{
	ConLog( "* SPU2-X: Init SPU2 core %d \n", index );
	memset( this, 0, sizeof(V_Core) );

	const int c = Index = index;

	Regs.STATX		= 0;
	Regs.ATTR		= 0;
	ExtVol			= V_VolumeLR::Max;
	InpVol			= V_VolumeLR::Max;
	FxVol			= V_VolumeLR::Max;

	MasterVol		= V_VolumeSlideLR::Max;

	memset( &DryGate, -1, sizeof(DryGate) );
	memset( &WetGate, -1, sizeof(WetGate) );

	Regs.MMIX		= 0xFFCF;
	Regs.VMIXL		= 0xFFFFFF;
	Regs.VMIXR		= 0xFFFFFF;
	Regs.VMIXEL		= 0xFFFFFF;
	Regs.VMIXER		= 0xFFFFFF;
	EffectsStartA	= c ? 0xEFFF8 : 0xFFFF8;
	EffectsEndA		= c ? 0xEFFFF : 0xFFFFF;

	FxEnable		= 0;
	IRQA			= 0xFFFF0;
	IRQEnable		= 0;

	for( uint v=0; v<NumVoices; ++v )
	{
		VoiceGates[v].DryL = -1;
		VoiceGates[v].DryR = -1;
		VoiceGates[v].WetL = -1;
		VoiceGates[v].WetR = -1;

		Voices[v].Volume		= V_VolumeSlideLR(0,0); // V_VolumeSlideLR::Max;
		Voices[v].SCurrent		= 28;

		Voices[v].ADSR.Value	= 0;
		Voices[v].ADSR.Phase	= 0;
		Voices[v].Pitch			= 0x3FFF;
		Voices[v].NextA			= 0x2800;
		Voices[v].StartA		= 0x2800;
		Voices[v].LoopStartA	= 0x2800;
	}

	DMAICounter		= 0;
	AdmaInProgress	= 0;

	Regs.STATX		= 0x80;
}

s32 V_Core::EffectsBufferIndexer( s32 offset ) const
{
	// Should offsets be multipled by 4 or not?  Reverse-engineering of IOP code reveals
	// that it *4's all addresses before upping them to the SPU2 -- so our buffers are
	// already x4'd.  It doesn't really make sense that we should x4 them again, and this
	// seems to work. (feedback-free in bios and DDS)  --air

	//offset *= 4;

	u32 pos = EffectsStartA + offset;

	// Need to use modulus here, because games can and will drop the buffer size
	// without notice, and it leads to offsets several times past the end of the buffer.

	if( pos > EffectsEndA )
	{
		pos = EffectsStartA + (offset % EffectsBufferSize);
	}
	else if( pos < EffectsStartA )
	{
		pos = EffectsEndA+1 - (offset % EffectsBufferSize );
	}
	return pos;
}

void V_Core::UpdateFeedbackBuffersA()
{
	RevBuffers.FB_SRC_A0 = EffectsBufferIndexer( Revb.MIX_DEST_A0 - Revb.FB_SRC_A );
	RevBuffers.FB_SRC_A1 = EffectsBufferIndexer( Revb.MIX_DEST_A1 - Revb.FB_SRC_A );
}

void V_Core::UpdateFeedbackBuffersB()
{
	RevBuffers.FB_SRC_B0 = EffectsBufferIndexer( Revb.MIX_DEST_B0 - Revb.FB_SRC_B );
	RevBuffers.FB_SRC_B1 = EffectsBufferIndexer( Revb.MIX_DEST_B1 - Revb.FB_SRC_B );
}

void V_Core::UpdateEffectsBufferSize()
{
	const s32 newbufsize = EffectsEndA - EffectsStartA + 1;
	//printf("Rvb Area change: ESA = %x, EEA = %x, Size(dec) = %d, Size(hex) = %x FxEnable = %d\n", EffectsStartA, EffectsEndA, newbufsize * 2, newbufsize * 2, FxEnable);
	
	if( (newbufsize*2) > 0x20000 ) // max 128kb per core
	{ 
		//printf("too big, returning\n");
		//return;
	}
	if( !RevBuffers.NeedsUpdated && (newbufsize == EffectsBufferSize) ) return;
	
	RevBuffers.NeedsUpdated = false;
	EffectsBufferSize = newbufsize;

	if( EffectsBufferSize <= 0 ) return;

	// Rebuild buffer indexers.

	RevBuffers.ACC_SRC_A0 = EffectsBufferIndexer( Revb.ACC_SRC_A0 );
	RevBuffers.ACC_SRC_A1 = EffectsBufferIndexer( Revb.ACC_SRC_A1 );
	RevBuffers.ACC_SRC_B0 = EffectsBufferIndexer( Revb.ACC_SRC_B0 );
	RevBuffers.ACC_SRC_B1 = EffectsBufferIndexer( Revb.ACC_SRC_B1 );
	RevBuffers.ACC_SRC_C0 = EffectsBufferIndexer( Revb.ACC_SRC_C0 );
	RevBuffers.ACC_SRC_C1 = EffectsBufferIndexer( Revb.ACC_SRC_C1 );
	RevBuffers.ACC_SRC_D0 = EffectsBufferIndexer( Revb.ACC_SRC_D0 );
	RevBuffers.ACC_SRC_D1 = EffectsBufferIndexer( Revb.ACC_SRC_D1 );

	UpdateFeedbackBuffersA();
	UpdateFeedbackBuffersB();

	RevBuffers.IIR_DEST_A0 = EffectsBufferIndexer( Revb.IIR_DEST_A0 );
	RevBuffers.IIR_DEST_A1 = EffectsBufferIndexer( Revb.IIR_DEST_A1 );
	RevBuffers.IIR_DEST_B0 = EffectsBufferIndexer( Revb.IIR_DEST_B0 );
	RevBuffers.IIR_DEST_B1 = EffectsBufferIndexer( Revb.IIR_DEST_B1 );

	RevBuffers.IIR_SRC_A0 = EffectsBufferIndexer( Revb.IIR_SRC_A0 );
	RevBuffers.IIR_SRC_A1 = EffectsBufferIndexer( Revb.IIR_SRC_A1 );
	RevBuffers.IIR_SRC_B0 = EffectsBufferIndexer( Revb.IIR_SRC_B0 );
	RevBuffers.IIR_SRC_B1 = EffectsBufferIndexer( Revb.IIR_SRC_B1 );

	RevBuffers.MIX_DEST_A0 = EffectsBufferIndexer( Revb.MIX_DEST_A0 );
	RevBuffers.MIX_DEST_A1 = EffectsBufferIndexer( Revb.MIX_DEST_A1 );
	RevBuffers.MIX_DEST_B0 = EffectsBufferIndexer( Revb.MIX_DEST_B0 );
	RevBuffers.MIX_DEST_B1 = EffectsBufferIndexer( Revb.MIX_DEST_B1 );
}

void V_Voice::Start()
{
	if((Cycles-PlayCycle)>=4)
	{
		if(StartA&7)
		{
			fprintf( stderr, " *** Misaligned StartA %05x!\n",StartA);
			StartA=(StartA+0xFFFF8)+0x8;
		}

		ADSR.Releasing	= false;
		ADSR.Value		= 1;
		ADSR.Phase		= 1;
		PlayCycle		= Cycles;
		SCurrent		= 28;
		LoopMode		= 0;
		LoopFlags		= 0;
		// Setting the loopstart to NextA breaks Squaresoft games (KH2 intro gets crackly)
		//LoopStartA		= StartA;
		NextA			= StartA;
		Prev1			= 0;
		Prev2			= 0;

		PV1 = PV2		= 0;
		PV3 = PV4		= 0;
		PrevAmp = 0;
		NextCrest = 0;
	}
	else
	{
		printf(" *** KeyOn after less than 4 T disregarded.\n");
	}
}

void V_Voice::Stop()
{
	ADSR.Value = 0;
	ADSR.Phase = 0;
}

uint TickInterval = 768;
static const int SanityInterval = 4800;
extern void UpdateDebugDialog();

__forceinline void TimeUpdate(u32 cClocks)
{
	u32 dClocks = cClocks - lClocks;

	// Sanity Checks:
	//  It's not totally uncommon for the IOP's clock to jump backwards a cycle or two, and in
	//  such cases we just want to ignore the TimeUpdate call.

	if( dClocks > (u32)-15 ) return;

	//  But if for some reason our clock value seems way off base (typically due to bad dma
	//  timings from PCSX2), just mix out a little bit, skip the rest, and hope the ship
	//  "rights" itself later on.

	if( dClocks > (u32)(TickInterval*SanityInterval) )
	{
		ConLog( " * SPU2 > TimeUpdate Sanity Check (Tick Delta: %d) (PS2 Ticks: %d)\n", dClocks/TickInterval, cClocks/TickInterval );
		dClocks = TickInterval * SanityInterval;
		lClocks = cClocks - dClocks;
	}

	// Uncomment for a visual debug display showing all core's activity!
	// Also need to uncomment a few lines in SPU2open
	//UpdateDebugDialog();

	if( SynchMode == 1 ) // AsyncMix on
		SndBuffer::UpdateTempoChangeAsyncMixing();
	else TickInterval = 768; // Reset to default, in case the user hotswitched from async to something else.

	//Update Mixing Progress
	while(dClocks>=TickInterval)
	{
		if(has_to_call_irq)
		{
			ConLog("* SPU2-X: Irq Called (%04x) at cycle %d.\n", Spdif.Info, Cycles);
			has_to_call_irq=false;
			if(_irqcallback) _irqcallback();
		}

		if(Cores[0].InitDelay>0)
		{
			Cores[0].InitDelay--;
			if(Cores[0].InitDelay==0)
			{
				Cores[0].Reset(0);
			}
		}

		if(Cores[1].InitDelay>0)
		{
			Cores[1].InitDelay--;
			if(Cores[1].InitDelay==0)
			{
				Cores[1].Reset(1);
			}
		}

#ifndef ENABLE_NEW_IOPDMA_SPU2
		//Update DMA4 interrupt delay counter
		if(Cores[0].DMAICounter>0)
		{
			Cores[0].DMAICounter-=TickInterval;
			if(Cores[0].DMAICounter<=0)
			{
				Cores[0].MADR=Cores[0].TADR;
				Cores[0].DMAICounter=0;
				if(dma4callback) dma4callback();
			}
			else {
				Cores[0].MADR+=TickInterval<<1;
			}
		}

		//Update DMA7 interrupt delay counter
		if(Cores[1].DMAICounter>0)
		{
			Cores[1].DMAICounter-=TickInterval;
			if(Cores[1].DMAICounter<=0)
			{
				Cores[1].MADR=Cores[1].TADR;
				Cores[1].DMAICounter=0;
				//ConLog( "* SPU2 > DMA 7 Callback!  %d\n", Cycles );
				if(dma7callback) dma7callback();
			}
			else {
				Cores[1].MADR+=TickInterval<<1;
			}
		}
#endif

		dClocks -= TickInterval;
		lClocks += TickInterval;
		Cycles++;

		// Note: IOP does not use MMX regs, so no need to save them.
		//SaveMMXRegs();
		Mix();
		//RestoreMMXRegs();
	}
}

static u16 mask = 0xFFFF;

__forceinline void UpdateSpdifMode()
{
	int OPM=PlayMode;
	u16 last = 0;

	if(mask&Spdif.Out)
	{
		last = mask & Spdif.Out;
		mask=mask&(~Spdif.Out);
	}

	if(Spdif.Out&0x4) // use 24/32bit PCM data streaming
	{
		PlayMode=8;
		ConLog("* SPU2-X: WARNING: Possibly CDDA mode set!\n");
		return;
	}

	if(Spdif.Out&SPDIF_OUT_BYPASS)
	{
		PlayMode=2;
		if(Spdif.Mode&SPDIF_MODE_BYPASS_BITSTREAM)
			PlayMode=4; //bitstream bypass
	}
	else
	{
		PlayMode=0; //normal processing
		if(Spdif.Out&SPDIF_OUT_PCM)
		{
			PlayMode=1;
		}
	}
	if(OPM!=PlayMode)
	{
		ConLog("* SPU2-X: Play Mode Set to %s (%d).\n",
			(PlayMode==0) ? "Normal" : ((PlayMode==1) ? "PCM Clone" : ((PlayMode==2) ? "PCM Bypass" : "BitStream Bypass")),PlayMode);
	}
}

// Converts an SPU2 register volume write into a 32 bit SPU2-X volume.  The value is extended
// properly into the lower 16 bits of the value to provide a full spectrum of volumes.
static s32 GetVol32( u16 src )
{
	return (((s32)src) << 16 ) | ((src<<1) & 0xffff);
}

void V_VolumeSlide::RegSet( u16 src )
{
	Value = GetVol32( src );
}

void V_Core::WriteRegPS1( u32 mem, u16 value )
{
	jASSUME( Index == 0 );		// Valid on Core 0 only!

	bool show	= true;
	u32 reg		= mem & 0xffff;

	if((reg>=0x1c00)&&(reg<0x1d80))
	{
		//voice values
		u8 voice = ((reg-0x1c00)>>4);
		u8 vval = reg&0xf;
		switch(vval)
		{
			case 0: //VOLL (Volume L)
				Voices[voice].Volume.Left.Mode = 0;
				Voices[voice].Volume.Left.RegSet( value << 1 );
				Voices[voice].Volume.Left.Reg_VOL = value;
			break;

			case 1: //VOLR (Volume R)
				Voices[voice].Volume.Right.Mode = 0;
				Voices[voice].Volume.Right.RegSet( value << 1 );
				Voices[voice].Volume.Right.Reg_VOL = value;
			break;

			case 2:	Voices[voice].Pitch = value; break;
			case 3:	Voices[voice].StartA = (u32)value<<8; break;

			case 4: // ADSR1 (Envelope)
				Voices[voice].ADSR.regADSR1 = value;
			break;

			case 5: // ADSR2 (Envelope)
				Voices[voice].ADSR.regADSR2 = value;
			break;

			case 6:
				Voices[voice].ADSR.Value = ((s32)value<<16) | value;
				ConLog( "* SPU2: Mysterious ADSR Volume Set to 0x%x", value );
			break;

			case 7:	Voices[voice].LoopStartA = (u32)value <<8;	break;

			jNO_DEFAULT;
		}
	}

	else switch(reg)
	{
		case 0x1d80://         Mainvolume left
			MasterVol.Left.Mode = 0;
			MasterVol.Left.RegSet( value );
		break;

		case 0x1d82://         Mainvolume right
			MasterVol.Right.Mode = 0;
			MasterVol.Right.RegSet( value );
		break;

		case 0x1d84://         Reverberation depth left
			FxVol.Left = GetVol32( value );
		break;

		case 0x1d86://         Reverberation depth right
			FxVol.Right = GetVol32( value );
		break;

		case 0x1d88://         Voice ON  (0-15)
			SPU2_FastWrite(REG_S_KON,value);
			break;
		case 0x1d8a://         Voice ON  (16-23)
			SPU2_FastWrite(REG_S_KON+2,value);
			break;

		case 0x1d8c://         Voice OFF (0-15)
			SPU2_FastWrite(REG_S_KOFF,value);
			break;
		case 0x1d8e://         Voice OFF (16-23)
			SPU2_FastWrite(REG_S_KOFF+2,value);
		break;

		case 0x1d90://         Channel FM (pitch lfo) mode (0-15)
			SPU2_FastWrite(REG_S_PMON,value);
		break;

		case 0x1d92://         Channel FM (pitch lfo) mode (16-23)
			SPU2_FastWrite(REG_S_PMON+2,value);
		break;


		case 0x1d94://         Channel Noise mode (0-15)
			SPU2_FastWrite(REG_S_NON,value);
		break;

		case 0x1d96://         Channel Noise mode (16-23)
			SPU2_FastWrite(REG_S_NON+2,value);
		break;

		case 0x1d98://         Channel Reverb mode (0-15)
			SPU2_FastWrite(REG_S_VMIXEL,value);
			SPU2_FastWrite(REG_S_VMIXER,value);
		break;

		case 0x1d9a://         Channel Reverb mode (16-23)
			SPU2_FastWrite(REG_S_VMIXEL+2,value);
			SPU2_FastWrite(REG_S_VMIXER+2,value);
		break;

		case 0x1d9c://         Channel Reverb mode (0-15)
			SPU2_FastWrite(REG_S_VMIXL,value);
			SPU2_FastWrite(REG_S_VMIXR,value);
		break;

		case 0x1d9e://         Channel Reverb mode (16-23)
			SPU2_FastWrite(REG_S_VMIXL+2,value);
			SPU2_FastWrite(REG_S_VMIXR+2,value);
		break;

		case 0x1da2://         Reverb work area start
		{
			u32 val = (u32)value << 8;

			SPU2_FastWrite(REG_A_ESA,  val&0xFFFF);
			SPU2_FastWrite(REG_A_ESA+2,val>>16);
		}
		break;

		case 0x1da4:
			IRQA = (u32)value<<8;
		break;

		case 0x1da6:
			TSA = (u32)value<<8;
		break;

		case 0x1daa:
			SPU2_FastWrite(REG_C_ATTR,value);
		break;

		case 0x1dae:
			SPU2_FastWrite(REG_P_STATX,value);
		break;

		case 0x1da8:// Spu Write to Memory
			DmaWrite(value);
			show=false;
		break;
	}

	if(show) FileLog("[%10d] (!) SPU write mem %08x value %04x\n",Cycles,mem,value);

	spu2Ru16(mem)=value;
}

u16 V_Core::ReadRegPS1(u32 mem)
{
	jASSUME( Index == 0 );		// Valid on Core 0 only!

	bool show=true;
	u16 value = spu2Ru16(mem);

	u32 reg = mem&0xffff;

	if((reg>=0x1c00)&&(reg<0x1d80))
	{
		//voice values
		u8 voice = ((reg-0x1c00)>>4);
		u8 vval = reg&0xf;
		switch(vval)
		{
			case 0: //VOLL (Volume L)
				//value=Voices[voice].VolumeL.Mode;
				//value=Voices[voice].VolumeL.Value;
				value = Voices[voice].Volume.Left.Reg_VOL;
			break;

			case 1: //VOLR (Volume R)
				//value=Voices[voice].VolumeR.Mode;
				//value=Voices[voice].VolumeR.Value;
				value = Voices[voice].Volume.Right.Reg_VOL;
			break;

			case 2:	value = Voices[voice].Pitch;		break;
			case 3:	value = Voices[voice].StartA;		break;
			case 4: value = Voices[voice].ADSR.regADSR1;	break;
			case 5: value = Voices[voice].ADSR.regADSR2;	break;
			case 6:	value = Voices[voice].ADSR.Value >> 16;	break;
			case 7:	value = Voices[voice].LoopStartA;	break;

			jNO_DEFAULT;
		}
	}
	else switch(reg)
	{
		case 0x1d80: value = MasterVol.Left.Value >> 16;  break;
		case 0x1d82: value = MasterVol.Right.Value >> 16; break;
		case 0x1d84: value = FxVol.Left >> 16;            break;
		case 0x1d86: value = FxVol.Right >> 16;           break;

		case 0x1d88: value = 0; break;
		case 0x1d8a: value = 0; break;
		case 0x1d8c: value = 0; break;
		case 0x1d8e: value = 0; break;

		case 0x1d90: value = Regs.PMON&0xFFFF;   break;
		case 0x1d92: value = Regs.PMON>>16;      break;

		case 0x1d94: value = Regs.NON&0xFFFF;    break;
		case 0x1d96: value = Regs.NON>>16;       break;

		case 0x1d98: value = Regs.VMIXEL&0xFFFF; break;
		case 0x1d9a: value = Regs.VMIXEL>>16;    break;
		case 0x1d9c: value = Regs.VMIXL&0xFFFF;  break;
		case 0x1d9e: value = Regs.VMIXL>>16;     break;

		case 0x1da2:
			if( value != EffectsStartA>>3 )
			{
				value = EffectsStartA>>3;
				UpdateEffectsBufferSize();
				ReverbX = 0;
			}
		break;
		case 0x1da4: value = IRQA>>3;            break;
		case 0x1da6: value = TSA>>3;             break;

		case 0x1daa:
			value = SPU2read(REG_C_ATTR);
			break;
		case 0x1dae:
			value = 0; //SPU2read(REG_P_STATX)<<3;
			break;
		case 0x1da8:
			value = DmaRead();
			show=false;
			break;
	}

	if(show) FileLog("[%10d] (!) SPU read mem %08x value %04x\n",Cycles,mem,value);
	return value;
}

// Ah the joys of endian-specific code! :D
static __forceinline void SetHiWord( u32& src, u16 value )
{
	((u16*)&src)[1] = value;
}

static __forceinline void SetLoWord( u32& src, u16 value )
{
	((u16*)&src)[0] = value;
}

static __forceinline void SetHiWord( s32& src, u16 value )
{
	((u16*)&src)[1] = value;
}

static __forceinline void SetLoWord( s32& src, u16 value )
{
	((u16*)&src)[0] = value;
}

static __forceinline u16 GetHiWord( u32& src )
{
	return ((u16*)&src)[1];
}

static __forceinline u16 GetLoWord( u32& src )
{
	return ((u16*)&src)[0];
}

static __forceinline u16 GetHiWord( s32& src )
{
	return ((u16*)&src)[1];
}

static __forceinline u16 GetLoWord( s32& src )
{
	return ((u16*)&src)[0];
}

template< int CoreIdx, int VoiceIdx, int param >
static void __fastcall RegWrite_VoiceParams( u16 value )
{
	const int core = CoreIdx;
	const int voice = VoiceIdx;

	V_Voice& thisvoice = Cores[core].Voices[voice];

	switch (param)
	{
		case 0: //VOLL (Volume L)
		case 1: //VOLR (Volume R)
		{
			V_VolumeSlide& thisvol = (param==0) ? thisvoice.Volume.Left : thisvoice.Volume.Right;
			thisvol.Reg_VOL = value;

			if (value & 0x8000)		// +Lin/-Lin/+Exp/-Exp
			{
				thisvol.Mode = (value & 0xF000)>>12;
				thisvol.Increment = (value & 0x7F);
				//printf("slides Mode = %x, Increment = %x\n",thisvol.Mode,thisvol.Increment);
			}
			else
			{
				// Constant Volume mode (no slides or envelopes)
				// Volumes range from 0x3fff to 0x7fff, with 0x4000 serving as
				// the "sign" bit, so a simple bitwise extension will do the trick:

				thisvol.RegSet( value<<1 );
				thisvol.Mode = 0;
				thisvol.Increment = 0;
			}
		}
		break;

		case 2:
			thisvoice.Pitch			= value & 0x3fff;
		break;

		case 3: // ADSR1 (Envelope)
			thisvoice.ADSR.regADSR1	= value;
		break;

		case 4: // ADSR2 (Envelope)
			thisvoice.ADSR.regADSR2	= value;
		break;

		case 5:
			// [Air] : Mysterious ADSR set code.  Too bad none of my games ever use it.
			//      (as usual... )
			thisvoice.ADSR.Value = (value << 16) | value;
			ConLog( "* SPU2: Mysterious ADSR Volume Set to 0x%x", value );
		break;

		case 6:	thisvoice.Volume.Left.RegSet( value ); break;
		case 7:	thisvoice.Volume.Right.RegSet( value ); break;

		jNO_DEFAULT;
	}
}

template< int CoreIdx, int VoiceIdx, int address >
static void __fastcall RegWrite_VoiceAddr( u16 value )
{
	const int core = CoreIdx;
	const int voice = VoiceIdx;

	V_Voice& thisvoice = Cores[core].Voices[voice];

	switch (address)
	{
		case 0:	// SSA (Waveform Start Addr) (hiword, 4 bits only)
			thisvoice.StartA = ((value & 0x0F) << 16) | (thisvoice.StartA & 0xFFF8);
			if( IsDevBuild )
				DebugCores[core].Voices[voice].lastSetStartA = thisvoice.StartA;
		break;

		case 1:	// SSA (loword)
			thisvoice.StartA = (thisvoice.StartA & 0x0F0000) | (value & 0xFFF8);
			if( IsDevBuild )
				DebugCores[core].Voices[voice].lastSetStartA = thisvoice.StartA;
		break;

		case 2:
			thisvoice.LoopStartA = ((value & 0x0F) << 16) | (thisvoice.LoopStartA & 0xFFF8);
			thisvoice.LoopMode = 3;
		break;

		case 3:
			thisvoice.LoopStartA = (thisvoice.LoopStartA & 0x0F0000) | (value & 0xFFF8);
			thisvoice.LoopMode = 3;
		break;

		// Note that there's no proof that I know of that writing to NextA is
		// even allowed or handled by the SPU2 (it might be disabled or ignored,
		// for example).  Tests should be done to find games that write to this
		// reg, and see if they're buggy or not. --air

		case 4:
			thisvoice.NextA = ((value & 0x0F) << 16) | (thisvoice.NextA & 0xFFF8);
			thisvoice.SCurrent = 28;
		break;

		case 5:
			thisvoice.NextA = (thisvoice.NextA & 0x0F0000) | (value & 0xFFF8);
			thisvoice.SCurrent = 28;
		break;
	}
}

template< int CoreIdx, int cAddr >
static void __fastcall RegWrite_Core( u16 value )
{
	const int omem = cAddr;
	const int core = CoreIdx;
	V_Core& thiscore = Cores[core];

	switch(omem)
	{
		case REG__1AC:
			// ----------------------------------------------------------------------------
			// 0x1ac / 0x5ac : direct-write to DMA address : special register (undocumented)
			// ----------------------------------------------------------------------------
			// On the GS, DMAs are actually pushed through a hardware register.  Chances are the
			// SPU works the same way, and "technically" *all* DMA data actually passes through
			// the HW registers at 0x1ac (core0) and 0x5ac (core1).  We handle normal DMAs in
			// optimized block copy fashion elsewhere, but some games will write this register
			// directly, so handle those here:

			// Performance Note: The PS2 Bios uses this extensively right before booting games,
			// causing massive slowdown if we don't shortcut it here.

			for( int i=0; i<2; i++ )
			{
				if(Cores[i].IRQEnable && (Cores[i].IRQA == thiscore.TSA))
				{
					SetIrqCall(i);
				}
			}
			thiscore.DmaWrite( value );
		break;

		case REG_C_ATTR:
		{
			bool irqe = thiscore.IRQEnable;
			int bit0 = thiscore.AttrBit0;
			u8 oldDmaMode = thiscore.DmaMode;

			if( ((value>>15)&1) && (!thiscore.CoreEnabled) && (thiscore.InitDelay==0) ) // on init/reset
			{
				// When we have exact cycle update info from the Pcsx2 IOP unit, then use
				// the more accurate delayed initialization system.
				ConLog( "* SPU2-X: Runtime core%d reset\n", core );

				// Async mixing can cause a scheduled reset to happen untimely, ff12 hates it and dies.
				// So do the next best thing and reset the core directly.
				if(cyclePtr != NULL && SynchMode != 1) // !AsyncMix
				{
					thiscore.InitDelay  = 1;
					thiscore.Regs.STATX = 0;
				}
				else
				{
					thiscore.Reset(thiscore.Index);
				}
			}

			thiscore.AttrBit0   =(value>> 0) & 0x01; //1 bit
			thiscore.DMABits	=(value>> 1) & 0x07; //3 bits
			thiscore.DmaMode    =(value>> 4) & 0x03; //2 bit (not necessary, we get the direction from the iop)
			thiscore.IRQEnable  =(value>> 6) & 0x01; //1 bit
			thiscore.FxEnable   =(value>> 7) & 0x01; //1 bit
			thiscore.NoiseClk   =(value>> 8) & 0x3f; //6 bits
			//thiscore.Mute	   =(value>>14) & 0x01; //1 bit
			thiscore.Mute=0;
			thiscore.CoreEnabled=(value>>15) & 0x01; //1 bit
			thiscore.Regs.ATTR  =value&0x7fff;

			if(oldDmaMode != thiscore.DmaMode)
			{
				// FIXME... maybe: if this mode was cleared in the middle of a DMA, should we interrupt it?
				thiscore.Regs.STATX &= ~0x400; // ready to transfer
			}

			if(value&0x000E)
			{
				ConLog("* SPU2-X: Core %d ATTR unknown bits SET! value=%04x\n",core,value);
			}

			if(thiscore.AttrBit0!=bit0)
			{
				ConLog("* SPU2-X: ATTR bit 0 set to %d\n",thiscore.AttrBit0);
			}
			if(thiscore.IRQEnable!=irqe)
			{
				ConLog("* SPU2-X: IRQ %s at cycle %d. Current IRQA = %x\n",
					((thiscore.IRQEnable==0)?"disabled":"enabled"), Cycles, thiscore.IRQA);
				
				if(!thiscore.IRQEnable)
					Spdif.Info &= ~(4 << thiscore.Index);
			}

		}
		break;

		case REG_S_PMON:
			for( int vc=1; vc<16; ++vc )
				thiscore.Voices[vc].Modulated = (value >> vc) & 1;
			SetLoWord( thiscore.Regs.PMON, value );
		break;

		case (REG_S_PMON + 2):
			for( int vc=0; vc<8; ++vc )
				thiscore.Voices[vc+16].Modulated = (value >> vc) & 1;
			SetHiWord( thiscore.Regs.PMON, value );
		break;

		case REG_S_NON:
			for( int vc=0; vc<16; ++vc)
				thiscore.Voices[vc].Noise = (value >> vc) & 1;
			SetLoWord( thiscore.Regs.NON, value );
		break;

		case (REG_S_NON + 2):
			for( int vc=0; vc<8; ++vc )
				thiscore.Voices[vc+16].Noise = (value >> vc) & 1;
			SetHiWord( thiscore.Regs.NON, value );
		break;

// Games like to repeatedly write these regs over and over with the same value, hence
// the shortcut that skips the bitloop if the values are equal.
#define vx_SetSomeBits( reg_out, mask_out, hiword ) \
{ \
	const u32 result	= thiscore.Regs.reg_out; \
	if( hiword ) \
		SetHiWord( thiscore.Regs.reg_out, value ); \
	else \
		SetLoWord( thiscore.Regs.reg_out, value ); \
	if( result == thiscore.Regs.reg_out ) break; \
	\
	const uint start_bit	= hiword ? 16 : 0; \
	const uint end_bit		= hiword ? 24 : 16; \
	for (uint vc=start_bit, vx=1; vc<end_bit; ++vc, vx<<=1) \
		thiscore.VoiceGates[vc].mask_out = (value & vx) ? -1 : 0; \
}

		case REG_S_VMIXL:
			vx_SetSomeBits( VMIXL, DryL, false );
		break;

		case (REG_S_VMIXL + 2):
			vx_SetSomeBits( VMIXL, DryL, true );
		break;

		case REG_S_VMIXEL:
			vx_SetSomeBits( VMIXEL, WetL, false );
		break;

		case (REG_S_VMIXEL + 2):
			vx_SetSomeBits( VMIXEL, WetL, true );
		break;

		case REG_S_VMIXR:
			vx_SetSomeBits( VMIXR, DryR, false );
		break;

		case (REG_S_VMIXR + 2):
			vx_SetSomeBits( VMIXR, DryR, true );
		break;

		case REG_S_VMIXER:
			vx_SetSomeBits( VMIXER, WetR, false );
		break;

		case (REG_S_VMIXER + 2):
			vx_SetSomeBits( VMIXER, WetR, true );
		break;

		case REG_P_MMIX:
		{
			// Each MMIX gate is assigned either 0 or 0xffffffff depending on the status
			// of the MMIX bits.  I use -1 below as a shorthand for 0xffffffff. :)

			const int vx = value & ((core==0) ? 0xFF0 : 0xFFF);
			thiscore.WetGate.ExtR = (vx & 0x001) ? -1 : 0;
			thiscore.WetGate.ExtL = (vx & 0x002) ? -1 : 0;
			thiscore.DryGate.ExtR = (vx & 0x004) ? -1 : 0;
			thiscore.DryGate.ExtL = (vx & 0x008) ? -1 : 0;
			thiscore.WetGate.InpR = (vx & 0x010) ? -1 : 0;
			thiscore.WetGate.InpL = (vx & 0x020) ? -1 : 0;
			thiscore.DryGate.InpR = (vx & 0x040) ? -1 : 0;
			thiscore.DryGate.InpL = (vx & 0x080) ? -1 : 0;
			thiscore.WetGate.SndR = (vx & 0x100) ? -1 : 0;
			thiscore.WetGate.SndL = (vx & 0x200) ? -1 : 0;
			thiscore.DryGate.SndR = (vx & 0x400) ? -1 : 0;
			thiscore.DryGate.SndL = (vx & 0x800) ? -1 : 0;
			thiscore.Regs.MMIX = value;
		}
		break;

		case (REG_S_KON + 2):
			StartVoices(core,((u32)value)<<16);
		break;

		case REG_S_KON:
			StartVoices(core,((u32)value));
		break;

		case (REG_S_KOFF + 2):
			StopVoices(core,((u32)value)<<16);
		break;

		case REG_S_KOFF:
			StopVoices(core,((u32)value));
		break;

		case REG_S_ENDX:
			thiscore.Regs.ENDX&=0x00FF0000;
		break;

		case (REG_S_ENDX + 2):
			thiscore.Regs.ENDX&=0xFFFF;
		break;

		// Reverb Start and End Address Writes!
		//  * Yes, these are backwards from all the volumes -- the hiword comes FIRST (wtf!)
		//  * End position is a hiword only!  Loword is always ffff.
		//  * The Reverb buffer position resets on writes to StartA.  It probably resets
		//    on writes to End too.  Docs don't say, but they're for PSX, which couldn't
		//    change the end address anyway.

		case REG_A_ESA:
			//if (thiscore.FxEnable){printf("!! ESA\n"); return;}
			SetHiWord( thiscore.EffectsStartA, value );
			thiscore.RevBuffers.NeedsUpdated = true;
			thiscore.ReverbX = 0;
		break;

		case (REG_A_ESA + 2):
			//if (thiscore.FxEnable){printf("!! ESA\n"); return;}
			SetLoWord( thiscore.EffectsStartA, value );
			thiscore.RevBuffers.NeedsUpdated = true;
			thiscore.ReverbX = 0;
		break;

		case REG_A_EEA:
			//if (thiscore.FxEnable){printf("!! EEA\n"); return;}  
			thiscore.EffectsEndA = ((u32)value<<16) | 0xFFFF;
			thiscore.RevBuffers.NeedsUpdated = true;
			thiscore.ReverbX = 0;
		break;

		case REG_S_ADMAS:
			//ConLog("* SPU2-X: Core %d AutoDMAControl set to %d (%d)\n",core,value, Cycles);
			thiscore.AutoDMACtrl=value;

			if(value==0)
			{
				thiscore.AdmaInProgress=0;
			}
		break;

		default:
		{
			const int addr = omem | ( (core == 1) ? 0x400 : 0 );
			*(regtable[addr>>1]) = value;
		}
		break;
	}
}

template< int CoreIdx, int addr >
static void __fastcall RegWrite_CoreExt( u16 value )
{
	V_Core& thiscore = Cores[CoreIdx];
	const int core = CoreIdx;

	switch(addr)
	{
		// Master Volume Address Write!

		case REG_P_MVOLL:
		case REG_P_MVOLR:
		{
			V_VolumeSlide& thisvol = (addr==REG_P_MVOLL) ? thiscore.MasterVol.Left : thiscore.MasterVol.Right;

			if (value & 0x8000)		// +Lin/-Lin/+Exp/-Exp
			{
				thisvol.Mode = (value & 0xF000)>>12;
				thisvol.Increment = (value & 0x7F);
				//printf("slides Mode = %x, Increment = %x\n",thisvol.Mode,thisvol.Increment);
			}
			else
			{
				// Constant Volume mode (no slides or envelopes)
				// Volumes range from 0x3fff to 0x7fff, with 0x4000 serving as
				// the "sign" bit, so a simple bitwise extension will do the trick:

				thisvol.Value = GetVol32( value<<1 );
				thisvol.Mode = 0;
				thisvol.Increment = 0;
			}
			thisvol.Reg_VOL = value;
		}
		break;

		case REG_P_EVOLL:
			thiscore.FxVol.Left = GetVol32( value );
		break;

		case REG_P_EVOLR:
			thiscore.FxVol.Right = GetVol32( value );
		break;

		case REG_P_AVOLL:
			thiscore.ExtVol.Left = GetVol32( value );
		break;

		case REG_P_AVOLR:
			thiscore.ExtVol.Right = GetVol32( value );
		break;

		case REG_P_BVOLL:
			thiscore.InpVol.Left = GetVol32( value );
		break;

		case REG_P_BVOLR:
			thiscore.InpVol.Right = GetVol32( value );
		break;

		default:
		{
			const int raddr = addr + ((core==1) ? 0x28 : 0 );
			*(regtable[raddr>>1]) = value;
		}
		break;
	}
}


template< int core, int addr >
static void __fastcall RegWrite_Reverb( u16 value )
{
	// Signal to the Reverb code that the effects buffers need to be re-aligned.
	// This is both simple, efficient, and safe, since we only want to re-align
	// buffers after both hi and lo words have been written.

	*(regtable[addr>>1]) = value;
	Cores[core].RevBuffers.NeedsUpdated = true;
}

template< int addr >
static void __fastcall RegWrite_SPDIF( u16 value )
{
	*(regtable[addr>>1]) = value;
	UpdateSpdifMode();
}

template< int addr >
static void __fastcall RegWrite_Raw( u16 value )
{
	*(regtable[addr>>1]) = value;
}

// --------------------------------------------------------------------------------------
//  Macros for tbl_reg_writes
// --------------------------------------------------------------------------------------
#define VoiceParamsSet( core, voice ) \
	RegWrite_VoiceParams<core,voice,0>, RegWrite_VoiceParams<core,voice,1>, \
	RegWrite_VoiceParams<core,voice,2>, RegWrite_VoiceParams<core,voice,3>, \
	RegWrite_VoiceParams<core,voice,4>, RegWrite_VoiceParams<core,voice,5>, \
	RegWrite_VoiceParams<core,voice,6>, RegWrite_VoiceParams<core,voice,7>

#define VoiceParamsCore( core ) \
	VoiceParamsSet(core, 0), VoiceParamsSet(core, 1), VoiceParamsSet(core, 2), VoiceParamsSet(core, 3 ), \
	VoiceParamsSet(core, 4), VoiceParamsSet(core, 5), VoiceParamsSet(core, 6), VoiceParamsSet(core, 7 ), \
	VoiceParamsSet(core, 8), VoiceParamsSet(core, 9), VoiceParamsSet(core, 10),VoiceParamsSet(core, 11 ), \
	VoiceParamsSet(core, 12),VoiceParamsSet(core, 13),VoiceParamsSet(core, 14),VoiceParamsSet(core, 15 ), \
	VoiceParamsSet(core, 16),VoiceParamsSet(core, 17),VoiceParamsSet(core, 18),VoiceParamsSet(core, 19 ), \
	VoiceParamsSet(core, 20),VoiceParamsSet(core, 21),VoiceParamsSet(core, 22),VoiceParamsSet(core, 23 )

#define VoiceAddrSet( core, voice ) \
	RegWrite_VoiceAddr<core,voice,0>, RegWrite_VoiceAddr<core,voice,1>, \
	RegWrite_VoiceAddr<core,voice,2>, RegWrite_VoiceAddr<core,voice,3>, \
	RegWrite_VoiceAddr<core,voice,4>, RegWrite_VoiceAddr<core,voice,5>


#define CoreParamsPair( core, omem ) \
	RegWrite_Core<core, omem>, RegWrite_Core<core, (omem+2)>

#define ReverbPair( core, mem ) \
	RegWrite_Reverb<core, mem>, RegWrite_Core<core, (mem+2)>

#define REGRAW(addr) RegWrite_Raw<addr>

// --------------------------------------------------------------------------------------
//  tbl_reg_writes  - Register Write Function Invocation LUT
// --------------------------------------------------------------------------------------

typedef void __fastcall RegWriteHandler( u16 value );
static RegWriteHandler * const tbl_reg_writes[0x401] =
{
	VoiceParamsCore(0),	// 0x000 -> 0x180
	CoreParamsPair(0,REG_S_PMON),
	CoreParamsPair(0,REG_S_NON),
	CoreParamsPair(0,REG_S_VMIXL),
	CoreParamsPair(0,REG_S_VMIXEL),
	CoreParamsPair(0,REG_S_VMIXR),
	CoreParamsPair(0,REG_S_VMIXER),

	RegWrite_Core<0,REG_P_MMIX>,
	RegWrite_Core<0,REG_C_ATTR>,

	CoreParamsPair(0,REG_A_IRQA),
	CoreParamsPair(0,REG_S_KON),
	CoreParamsPair(0,REG_S_KOFF),
	CoreParamsPair(0,REG_A_TSA),
	CoreParamsPair(0,REG__1AC),

	RegWrite_Core<0,REG_S_ADMAS>,
	REGRAW(0x1b2),

	REGRAW(0x1b4), REGRAW(0x1b6),
	REGRAW(0x1b8), REGRAW(0x1ba),
	REGRAW(0x1bc), REGRAW(0x1be),

	// 0x1c0!

	VoiceAddrSet(0, 0),VoiceAddrSet(0, 1),VoiceAddrSet(0, 2),VoiceAddrSet(0, 3),VoiceAddrSet(0, 4),VoiceAddrSet(0, 5),
	VoiceAddrSet(0, 6),VoiceAddrSet(0, 7),VoiceAddrSet(0, 8),VoiceAddrSet(0, 9),VoiceAddrSet(0,10),VoiceAddrSet(0,11),
	VoiceAddrSet(0,12),VoiceAddrSet(0,13),VoiceAddrSet(0,14),VoiceAddrSet(0,15),VoiceAddrSet(0,16),VoiceAddrSet(0,17),
	VoiceAddrSet(0,18),VoiceAddrSet(0,19),VoiceAddrSet(0,20),VoiceAddrSet(0,21),VoiceAddrSet(0,22),VoiceAddrSet(0,23),

	CoreParamsPair(0,REG_A_ESA),

	ReverbPair(0,R_FB_SRC_A), //       0x02E4		// Feedback Source A
	ReverbPair(0,R_FB_SRC_B), //       0x02E8		// Feedback Source B
	ReverbPair(0,R_IIR_DEST_A0), //    0x02EC
	ReverbPair(0,R_IIR_DEST_A1), //    0x02F0
	ReverbPair(0,R_ACC_SRC_A0), //     0x02F4
	ReverbPair(0,R_ACC_SRC_A1), //     0x02F8
	ReverbPair(0,R_ACC_SRC_B0), //     0x02FC
	ReverbPair(0,R_ACC_SRC_B1), //     0x0300
	ReverbPair(0,R_IIR_SRC_A0), //     0x0304
	ReverbPair(0,R_IIR_SRC_A1), //     0x0308
	ReverbPair(0,R_IIR_DEST_B0), //    0x030C
	ReverbPair(0,R_IIR_DEST_B1), //    0x0310
	ReverbPair(0,R_ACC_SRC_C0), //     0x0314
	ReverbPair(0,R_ACC_SRC_C1), //     0x0318
	ReverbPair(0,R_ACC_SRC_D0), //     0x031C
	ReverbPair(0,R_ACC_SRC_D1), //     0x0320
	ReverbPair(0,R_IIR_SRC_B0), //     0x0324
	ReverbPair(0,R_IIR_SRC_B1), //     0x0328
	ReverbPair(0,R_MIX_DEST_A0), //    0x032C
	ReverbPair(0,R_MIX_DEST_A1), //    0x0330
	ReverbPair(0,R_MIX_DEST_B0), //    0x0334
	ReverbPair(0,R_MIX_DEST_B1), //    0x0338

	RegWrite_Core<0,REG_A_EEA>, NULL,

	CoreParamsPair(0,REG_S_ENDX), //       0x0340	// End Point passed flag
	RegWrite_Core<0,REG_P_STATX>, //      0x0344 	// Status register?

	//0x346 here
	REGRAW(0x346),
	REGRAW(0x348),REGRAW(0x34A),REGRAW(0x34C),REGRAW(0x34E),
	REGRAW(0x350),REGRAW(0x352),REGRAW(0x354),REGRAW(0x356),
	REGRAW(0x358),REGRAW(0x35A),REGRAW(0x35C),REGRAW(0x35E),
	REGRAW(0x360),REGRAW(0x362),REGRAW(0x364),REGRAW(0x366),
	REGRAW(0x368),REGRAW(0x36A),REGRAW(0x36C),REGRAW(0x36E),
	REGRAW(0x370),REGRAW(0x372),REGRAW(0x374),REGRAW(0x376),
	REGRAW(0x378),REGRAW(0x37A),REGRAW(0x37C),REGRAW(0x37E),
	REGRAW(0x380),REGRAW(0x382),REGRAW(0x384),REGRAW(0x386),
	REGRAW(0x388),REGRAW(0x38A),REGRAW(0x38C),REGRAW(0x38E),
	REGRAW(0x390),REGRAW(0x392),REGRAW(0x394),REGRAW(0x396),
	REGRAW(0x398),REGRAW(0x39A),REGRAW(0x39C),REGRAW(0x39E),
	REGRAW(0x3A0),REGRAW(0x3A2),REGRAW(0x3A4),REGRAW(0x3A6),
	REGRAW(0x3A8),REGRAW(0x3AA),REGRAW(0x3AC),REGRAW(0x3AE),
	REGRAW(0x3B0),REGRAW(0x3B2),REGRAW(0x3B4),REGRAW(0x3B6),
	REGRAW(0x3B8),REGRAW(0x3BA),REGRAW(0x3BC),REGRAW(0x3BE),
	REGRAW(0x3C0),REGRAW(0x3C2),REGRAW(0x3C4),REGRAW(0x3C6),
	REGRAW(0x3C8),REGRAW(0x3CA),REGRAW(0x3CC),REGRAW(0x3CE),
	REGRAW(0x3D0),REGRAW(0x3D2),REGRAW(0x3D4),REGRAW(0x3D6),
	REGRAW(0x3D8),REGRAW(0x3DA),REGRAW(0x3DC),REGRAW(0x3DE),
	REGRAW(0x3E0),REGRAW(0x3E2),REGRAW(0x3E4),REGRAW(0x3E6),
	REGRAW(0x3E8),REGRAW(0x3EA),REGRAW(0x3EC),REGRAW(0x3EE),
	REGRAW(0x3F0),REGRAW(0x3F2),REGRAW(0x3F4),REGRAW(0x3F6),
	REGRAW(0x3F8),REGRAW(0x3FA),REGRAW(0x3FC),REGRAW(0x3FE),

	// AND... we reached 0x400!
	// Last verse, same as the first:

	VoiceParamsCore(1),	// 0x000 -> 0x180
	CoreParamsPair(1,REG_S_PMON),
	CoreParamsPair(1,REG_S_NON),
	CoreParamsPair(1,REG_S_VMIXL),
	CoreParamsPair(1,REG_S_VMIXEL),
	CoreParamsPair(1,REG_S_VMIXR),
	CoreParamsPair(1,REG_S_VMIXER),

	RegWrite_Core<1,REG_P_MMIX>,
	RegWrite_Core<1,REG_C_ATTR>,

	CoreParamsPair(1,REG_A_IRQA),
	CoreParamsPair(1,REG_S_KON),
	CoreParamsPair(1,REG_S_KOFF),
	CoreParamsPair(1,REG_A_TSA),
	CoreParamsPair(1,REG__1AC),

	RegWrite_Core<1,REG_S_ADMAS>,
	REGRAW(0x5b2),

	REGRAW(0x5b4), REGRAW(0x5b6),
	REGRAW(0x5b8), REGRAW(0x5ba),
	REGRAW(0x5bc), REGRAW(0x5be),

	// 0x1c0!

	VoiceAddrSet(1, 0),VoiceAddrSet(1, 1),VoiceAddrSet(1, 2),VoiceAddrSet(1, 3),VoiceAddrSet(1, 4),VoiceAddrSet(1, 5),
	VoiceAddrSet(1, 6),VoiceAddrSet(1, 7),VoiceAddrSet(1, 8),VoiceAddrSet(1, 9),VoiceAddrSet(1,10),VoiceAddrSet(1,11),
	VoiceAddrSet(1,12),VoiceAddrSet(1,13),VoiceAddrSet(1,14),VoiceAddrSet(1,15),VoiceAddrSet(1,16),VoiceAddrSet(1,17),
	VoiceAddrSet(1,18),VoiceAddrSet(1,19),VoiceAddrSet(1,20),VoiceAddrSet(1,21),VoiceAddrSet(1,22),VoiceAddrSet(1,23),

	CoreParamsPair(1,REG_A_ESA),

	ReverbPair(1,R_FB_SRC_A), //       0x02E4		// Feedback Source A
	ReverbPair(1,R_FB_SRC_B), //       0x02E8		// Feedback Source B
	ReverbPair(1,R_IIR_DEST_A0), //    0x02EC
	ReverbPair(1,R_IIR_DEST_A1), //    0x02F0
	ReverbPair(1,R_ACC_SRC_A0), //     0x02F4
	ReverbPair(1,R_ACC_SRC_A1), //     0x02F8
	ReverbPair(1,R_ACC_SRC_B0), //     0x02FC
	ReverbPair(1,R_ACC_SRC_B1), //     0x0300
	ReverbPair(1,R_IIR_SRC_A0), //     0x0304
	ReverbPair(1,R_IIR_SRC_A1), //     0x0308
	ReverbPair(1,R_IIR_DEST_B0), //    0x030C
	ReverbPair(1,R_IIR_DEST_B1), //    0x0310
	ReverbPair(1,R_ACC_SRC_C0), //     0x0314
	ReverbPair(1,R_ACC_SRC_C1), //     0x0318
	ReverbPair(1,R_ACC_SRC_D0), //     0x031C
	ReverbPair(1,R_ACC_SRC_D1), //     0x0320
	ReverbPair(1,R_IIR_SRC_B0), //     0x0324
	ReverbPair(1,R_IIR_SRC_B1), //     0x0328
	ReverbPair(1,R_MIX_DEST_A0), //    0x032C
	ReverbPair(1,R_MIX_DEST_A1), //    0x0330
	ReverbPair(1,R_MIX_DEST_B0), //    0x0334
	ReverbPair(1,R_MIX_DEST_B1), //    0x0338

	RegWrite_Core<1,REG_A_EEA>, NULL,

	CoreParamsPair(1,REG_S_ENDX), //       0x0340	// End Point passed flag
	RegWrite_Core<1,REG_P_STATX>, //      0x0344 	// Status register?

	REGRAW(0x746),
	REGRAW(0x748),REGRAW(0x74A),REGRAW(0x74C),REGRAW(0x74E),
	REGRAW(0x750),REGRAW(0x752),REGRAW(0x754),REGRAW(0x756),
	REGRAW(0x758),REGRAW(0x75A),REGRAW(0x75C),REGRAW(0x75E),

	// ------ -------

	RegWrite_CoreExt<0,REG_P_MVOLL>,	//     0x0760		// Master Volume Left
	RegWrite_CoreExt<0,REG_P_MVOLR>,	//     0x0762		// Master Volume Right
	RegWrite_CoreExt<0,REG_P_EVOLL>,	//     0x0764		// Effect Volume Left
	RegWrite_CoreExt<0,REG_P_EVOLR>,	//     0x0766		// Effect Volume Right
	RegWrite_CoreExt<0,REG_P_AVOLL>,	//     0x0768		// Core External Input Volume Left  (Only Core 1)
	RegWrite_CoreExt<0,REG_P_AVOLR>,	//     0x076A		// Core External Input Volume Right (Only Core 1)
	RegWrite_CoreExt<0,REG_P_BVOLL>,	//     0x076C 		// Sound Data Volume Left
	RegWrite_CoreExt<0,REG_P_BVOLR>,	//     0x076E		// Sound Data Volume Right
	RegWrite_CoreExt<0,REG_P_MVOLXL>,	//     0x0770		// Current Master Volume Left
	RegWrite_CoreExt<0,REG_P_MVOLXR>,	//     0x0772		// Current Master Volume Right

	RegWrite_CoreExt<0,R_IIR_ALPHA>,	//     0x0774		//IIR alpha (% used)
	RegWrite_CoreExt<0,R_ACC_COEF_A>,	//     0x0776
	RegWrite_CoreExt<0,R_ACC_COEF_B>,	//     0x0778
	RegWrite_CoreExt<0,R_ACC_COEF_C>,	//     0x077A
	RegWrite_CoreExt<0,R_ACC_COEF_D>,	//     0x077C
	RegWrite_CoreExt<0,R_IIR_COEF>,		//     0x077E
	RegWrite_CoreExt<0,R_FB_ALPHA>,		//     0x0780		//feedback alpha (% used)
	RegWrite_CoreExt<0,R_FB_X>,			//     0x0782		//feedback
	RegWrite_CoreExt<0,R_IN_COEF_L>,	//     0x0784
	RegWrite_CoreExt<0,R_IN_COEF_R>,	//     0x0786

	// ------ -------

	RegWrite_CoreExt<1,REG_P_MVOLL>,	//     0x0788		// Master Volume Left
	RegWrite_CoreExt<1,REG_P_MVOLR>,	//     0x078A		// Master Volume Right
	RegWrite_CoreExt<1,REG_P_EVOLL>,	//     0x0764		// Effect Volume Left
	RegWrite_CoreExt<1,REG_P_EVOLR>,	//     0x0766		// Effect Volume Right
	RegWrite_CoreExt<1,REG_P_AVOLL>,	//     0x0768		// Core External Input Volume Left  (Only Core 1)
	RegWrite_CoreExt<1,REG_P_AVOLR>,	//     0x076A		// Core External Input Volume Right (Only Core 1)
	RegWrite_CoreExt<1,REG_P_BVOLL>,	//     0x076C		// Sound Data Volume Left
	RegWrite_CoreExt<1,REG_P_BVOLR>,	//     0x076E		// Sound Data Volume Right
	RegWrite_CoreExt<1,REG_P_MVOLXL>,	//     0x0770		// Current Master Volume Left
	RegWrite_CoreExt<1,REG_P_MVOLXR>,	//     0x0772		// Current Master Volume Right

	RegWrite_CoreExt<1,R_IIR_ALPHA>,	//     0x0774		//IIR alpha (% used)
	RegWrite_CoreExt<1,R_ACC_COEF_A>,	//     0x0776
	RegWrite_CoreExt<1,R_ACC_COEF_B>,	//     0x0778
	RegWrite_CoreExt<1,R_ACC_COEF_C>,	//     0x077A
	RegWrite_CoreExt<1,R_ACC_COEF_D>,	//     0x077C
	RegWrite_CoreExt<1,R_IIR_COEF>,		//     0x077E
	RegWrite_CoreExt<1,R_FB_ALPHA>,		//     0x0780		//feedback alpha (% used)
	RegWrite_CoreExt<1,R_FB_X>,			//     0x0782		//feedback
	RegWrite_CoreExt<1,R_IN_COEF_L>,	//     0x0784
	RegWrite_CoreExt<1,R_IN_COEF_R>,	//     0x0786

	REGRAW(0x7B0),REGRAW(0x7B2),REGRAW(0x7B4),REGRAW(0x7B6),
	REGRAW(0x7B8),REGRAW(0x7BA),REGRAW(0x7BC),REGRAW(0x7BE),

	//  SPDIF interface

	RegWrite_SPDIF<SPDIF_OUT>,		//    0x07C0		// SPDIF Out: OFF/'PCM'/Bitstream/Bypass
	RegWrite_SPDIF<SPDIF_IRQINFO>,	//    0x07C2
	REGRAW(0x7C4),
	RegWrite_SPDIF<SPDIF_MODE>,		//    0x07C6
	RegWrite_SPDIF<SPDIF_MEDIA>,	//    0x07C8		// SPDIF Media: 'CD'/DVD
	REGRAW(0x7CA),
	RegWrite_SPDIF<SPDIF_PROTECT>,	//	 0x07CC		// SPDIF Copy Protection

	REGRAW(0x7CE),
	REGRAW(0x7D0),REGRAW(0x7D2),REGRAW(0x7D4),REGRAW(0x7D6),
	REGRAW(0x7D8),REGRAW(0x7DA),REGRAW(0x7DC),REGRAW(0x7DE),
	REGRAW(0x7E0),REGRAW(0x7E2),REGRAW(0x7E4),REGRAW(0x7E6),
	REGRAW(0x7E8),REGRAW(0x7EA),REGRAW(0x7EC),REGRAW(0x7EE),
	REGRAW(0x7F0),REGRAW(0x7F2),REGRAW(0x7F4),REGRAW(0x7F6),
	REGRAW(0x7F8),REGRAW(0x7FA),REGRAW(0x7FC),REGRAW(0x7FE),

	NULL		// should be at 0x400!  (we assert check it on startup)
};


void SPU2_FastWrite( u32 rmem, u16 value )
{
	tbl_reg_writes[(rmem&0x7ff)/2]( value );
}


void StartVoices(int core, u32 value)
{
	// Optimization: Games like to write zero to the KeyOn reg a lot, so shortcut
	// this loop if value is zero.

	if( value == 0 ) return;

	Cores[core].Regs.ENDX &= ~value;

	for( u8 vc=0; vc<V_Core::NumVoices; vc++ )
	{
		if( !((value>>vc) & 1) ) continue;

		Cores[core].Voices[vc].Start();

		if( IsDevBuild )
		{
			V_Voice& thisvc( Cores[core].Voices[vc] );

			if(MsgKeyOnOff()) ConLog("* SPU2-X: KeyOn: C%dV%02d: SSA: %8x; M: %s%s%s%s; H: %02x%02x; P: %04x V: %04x/%04x; ADSR: %04x%04x\n",
				core,vc,thisvc.StartA,
				(Cores[core].VoiceGates[vc].DryL)?"+":"-",(Cores[core].VoiceGates[vc].DryR)?"+":"-",
				(Cores[core].VoiceGates[vc].WetL)?"+":"-",(Cores[core].VoiceGates[vc].WetR)?"+":"-",
				*(u8*)GetMemPtr(thisvc.StartA),*(u8 *)GetMemPtr((thisvc.StartA)+1),
				thisvc.Pitch,
				thisvc.Volume.Left.Value>>16,thisvc.Volume.Right.Value>>16,
				thisvc.ADSR.regADSR1,thisvc.ADSR.regADSR2);
		}
	}
}

void StopVoices(int core, u32 value)
{
	if( value == 0 ) return;
	for( u8 vc=0; vc<V_Core::NumVoices; vc++ )
	{
		if( !((value>>vc) & 1) ) continue;

		Cores[core].Voices[vc].ADSR.Releasing = true;
		//if(MsgKeyOnOff()) ConLog("* SPU2-X: KeyOff: Core %d; Voice %d.\n",core,vc);
	}
}

