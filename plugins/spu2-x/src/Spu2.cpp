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
#include "RegTable.h"

#ifdef __LINUX__
#include "Linux.h"
#endif

void StartVoices(int core, u32 value);
void StopVoices(int core, u32 value);

void InitADSR();

#ifdef _MSC_VER
DWORD CALLBACK TimeThread(PVOID /* unused param */);
#endif

void (* _irqcallback)();
void (* dma4callback)();
void (* dma7callback)();

short *spu2regs;
short *_spu2mem;

u8 callirq;

V_CoreDebug DebugCores[2];
V_Core Cores[2];
V_SPDIF Spdif;

s16 OutPos;
s16 InputPos;
u32 Cycles;

u32* cyclePtr	= NULL;
u32  lClocks	= 0;

int PlayMode;

#ifdef _MSC_VER
HINSTANCE hInstance;
CRITICAL_SECTION threadSync;
HANDLE hThreadFunc;
u32	ThreadFuncID;
#endif

bool has_to_call_irq=false;

void SetIrqCall()
{
	has_to_call_irq=true;
}

#ifdef _MSC_VER
void SysMessage(const char *fmt, ...) 
{
	va_list list;
	char tmp[512];
	wchar_t wtmp[512];

	va_start(list,fmt);
	sprintf_s(tmp,fmt,list);
	va_end(list);
	swprintf_s(wtmp, L"%S", tmp);
	MessageBox(0, wtmp, L"SPU2-X System Message", 0);
}
#else
extern void SysMessage(const char *fmt, ...);
#endif

__forceinline s16 * __fastcall GetMemPtr(u32 addr)
{
#ifndef DEBUG_FAST
	// In case you're wondering, this assert is the reason SPU2-X
	// runs so incrediously slow in Debug mode. :P
	jASSUME( addr < 0x100000 );
#endif
	return (_spu2mem+addr);
}

__forceinline s16 __fastcall spu2M_Read( u32 addr )
{
	return *GetMemPtr( addr & 0xfffff );
}

// writes a signed value to the SPU2 ram
// Invalidates the ADPCM cache in the process.
// Optimization note: don't use __forceinline because the footprint of this
// function is a little too heavy now.  Better to let the compiler decide.
__inline void __fastcall spu2M_Write( u32 addr, s16 value )
{
	// Make sure the cache is invalidated:
	// (note to self : addr address WORDs, not bytes)

	addr &= 0xfffff;
	if( addr >= SPU2_DYN_MEMLINE )
	{
		const int cacheIdx = addr / pcm_WordsPerBlock;
		pcm_cache_data[cacheIdx].Validated = false;

		ConLog( " * SPU2 : PcmCache Block Clear at 0x%x (cacheIdx=0x%x)\n", addr, cacheIdx);
	}
	*GetMemPtr( addr ) = value;
}

// writes an unsigned value to the SPU2 ram
__inline void __fastcall spu2M_Write( u32 addr, u16 value )
{
	spu2M_Write( addr, (s16)value );
}

V_VolumeLR V_VolumeLR::Max( 0x7FFFFFFF );
V_VolumeSlideLR V_VolumeSlideLR::Max( 0x3FFF, 0x7FFFFFFF );

V_Core::V_Core()
{
}

void V_Core::Reset()
{
	memset( this, 0, sizeof(V_Core) );
 
	const int c = (this == Cores) ? 0 : 1;
 
	Regs.STATX=0;
	Regs.ATTR=0;
	ExtVol = V_VolumeLR::Max;
	InpVol = V_VolumeLR::Max;
	FxVol  = V_VolumeLR::Max;

	MasterVol = V_VolumeSlideLR::Max;

	DryGate.ExtL = -1;
	DryGate.ExtR = -1;
	WetGate.ExtL = -1;
	WetGate.ExtR = -1;
	DryGate.InpL = -1;
	DryGate.InpR = -1;
	WetGate.InpR = -1;
	WetGate.InpL = -1;
	DryGate.SndL = -1;
	DryGate.SndR = -1;
	WetGate.SndL = -1;
	WetGate.SndR = -1;
	
	Regs.MMIX = 0xFFCF;
	Regs.VMIXL = 0xFFFFFF;
	Regs.VMIXR = 0xFFFFFF;
	Regs.VMIXEL = 0xFFFFFF;
	Regs.VMIXER = 0xFFFFFF;
	EffectsStartA= 0xEFFF8 + 0x10000*c;
	EffectsEndA  = 0xEFFFF + 0x10000*c;
	FxEnable=0;
	IRQA=0xFFFF0;
	IRQEnable=1;
 
	for( uint v=0; v<NumVoices; ++v )
	{
		VoiceGates[v].DryL = -1;
		VoiceGates[v].DryR = -1;
		VoiceGates[v].WetL = -1;
		VoiceGates[v].WetR = -1;
	
		Voices[v].Volume = V_VolumeSlideLR::Max;
		
		Voices[v].ADSR.Value = 0;
		Voices[v].ADSR.Phase = 0;
		Voices[v].Pitch = 0x3FFF;
		Voices[v].NextA = 2800;
		Voices[v].StartA = 2800;
		Voices[v].LoopStartA = 2800;
	}
	DMAICounter = 0;
	AdmaInProgress = 0;
 
	Regs.STATX = 0x80;
}

s32 V_Core::EffectsBufferIndexer( s32 offset ) const
{
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
	if( !RevBuffers.NeedsUpdated && newbufsize ==  EffectsBufferSize ) return;
	
	RevBuffers.NeedsUpdated = false;
	EffectsBufferSize = newbufsize;

	if( EffectsBufferSize == 0 ) return;

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

static const int TickInterval = 768;
static const int SanityInterval = 4800;

u32 TicksCore = 0;
u32 TicksThread = 0;

PCSX2_ALIGNED16( static u64 g_globalMMXData[8] );

static __forceinline void SaveMMXRegs()
{
#ifdef _MSC_VER
	__asm {
		movntq mmword ptr [g_globalMMXData + 0], mm0
		movntq mmword ptr [g_globalMMXData + 8], mm1
		movntq mmword ptr [g_globalMMXData + 16], mm2
		movntq mmword ptr [g_globalMMXData + 24], mm3
		movntq mmword ptr [g_globalMMXData + 32], mm4
		movntq mmword ptr [g_globalMMXData + 40], mm5
		movntq mmword ptr [g_globalMMXData + 48], mm6
		movntq mmword ptr [g_globalMMXData + 56], mm7
		emms
	}
#else
    __asm__(".intel_syntax\n"
            "movq [%0+0x00], %%mm0\n"
            "movq [%0+0x08], %%mm1\n"
            "movq [%0+0x10], %%mm2\n"
            "movq [%0+0x18], %%mm3\n"
            "movq [%0+0x20], %%mm4\n"
            "movq [%0+0x28], %%mm5\n"
            "movq [%0+0x30], %%mm6\n"
            "movq [%0+0x38], %%mm7\n"
            "emms\n"
            ".att_syntax\n" : : "r"(g_globalMMXData) );
#endif
}

static __forceinline void RestoreMMXRegs()
{
#ifdef _MSC_VER
	__asm {
		movq mm0, mmword ptr [g_globalMMXData + 0]
		movq mm1, mmword ptr [g_globalMMXData + 8]
		movq mm2, mmword ptr [g_globalMMXData + 16]
		movq mm3, mmword ptr [g_globalMMXData + 24]
		movq mm4, mmword ptr [g_globalMMXData + 32]
		movq mm5, mmword ptr [g_globalMMXData + 40]
		movq mm6, mmword ptr [g_globalMMXData + 48]
		movq mm7, mmword ptr [g_globalMMXData + 56]
		emms
	}
#else
    __asm__(".intel_syntax\n"
            "movq %%mm0, [%0+0x00]\n"
            "movq %%mm1, [%0+0x08]\n"
            "movq %%mm2, [%0+0x10]\n"
            "movq %%mm3, [%0+0x18]\n"
            "movq %%mm4, [%0+0x20]\n"
            "movq %%mm5, [%0+0x28]\n"
            "movq %%mm6, [%0+0x30]\n"
            "movq %%mm7, [%0+0x38]\n"
            "emms\n"
            ".att_syntax\n" : : "r"(g_globalMMXData) );
#endif
}

__forceinline void TimeUpdate(u32 cClocks)
{
	u32 dClocks = cClocks-lClocks;

	// [Air]: Sanity Check
	//  If for some reason our clock value seems way off base, just mix
	//  out a little bit, skip the rest, and hope the ship "rights" itself later on.

	if( dClocks > TickInterval*SanityInterval )
	{
		ConLog( " * SPU2 > TimeUpdate Sanity Check (Tick Delta: %d) (PS2 Ticks: %d)\n", dClocks/TickInterval, cClocks/TickInterval );
		dClocks = TickInterval*SanityInterval;
		lClocks = cClocks-dClocks;
	}

	//UpdateDebugDialog();

	//Update Mixing Progress
	while(dClocks>=TickInterval)
	{
		if(has_to_call_irq)
		{
			ConLog(" * SPU2: Irq Called (%04x).\n",Spdif.Info);
			has_to_call_irq=false;
			if(_irqcallback) _irqcallback();
		}

		if(Cores[0].InitDelay>0)
		{
			Cores[0].InitDelay--;
			if(Cores[0].InitDelay==0)
			{
				Cores[0].Reset();
			}
		}

		if(Cores[1].InitDelay>0)
		{
			Cores[1].InitDelay--;
			if(Cores[1].InitDelay==0)
			{
				Cores[1].Reset();
			}
		}

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

		dClocks-=TickInterval;
		lClocks+=TickInterval;
		Cycles++;

		// Note: IPU does not use MMX regs, so no need to save them.
		//SaveMMXRegs();
		Mix();
		//RestoreMMXRegs();
	}
}

static u16 mask = 0xFFFF;

void UpdateSpdifMode()
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
		ConLog(" * SPU2: WARNING: Possibly CDDA mode set!\n");
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
		ConLog(" * SPU2: Play Mode Set to %s (%d).\n",
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

void SPU_ps1_write(u32 mem, u16 value) 
{
	bool show=true;

	u32 reg = mem&0xffff;

	if((reg>=0x1c00)&&(reg<0x1d80))
	{
		//voice values
		u8 voice = ((reg-0x1c00)>>4);
		u8 vval = reg&0xf;
		switch(vval)
		{
			case 0: //VOLL (Volume L)
				Cores[0].Voices[voice].Volume.Left.Mode = 0;
				Cores[0].Voices[voice].Volume.Left.RegSet( value << 1 );
				Cores[0].Voices[voice].Volume.Left.Reg_VOL = value;
			break;

			case 1: //VOLR (Volume R)
				Cores[0].Voices[voice].Volume.Right.Mode = 0;
				Cores[0].Voices[voice].Volume.Right.RegSet( value << 1 );
				Cores[0].Voices[voice].Volume.Right.Reg_VOL = value;
			break;
			
			case 2:	Cores[0].Voices[voice].Pitch = value; break;
			case 3:	Cores[0].Voices[voice].StartA = (u32)value<<8; break;

			case 4: // ADSR1 (Envelope)
				Cores[0].Voices[voice].ADSR.AttackMode = (value & 0x8000)>>15;
				Cores[0].Voices[voice].ADSR.AttackRate = (value & 0x7F00)>>8;
				Cores[0].Voices[voice].ADSR.DecayRate = (value & 0xF0)>>4;
				Cores[0].Voices[voice].ADSR.SustainLevel = (value & 0xF);
				Cores[0].Voices[voice].ADSR.Reg_ADSR1 = value;
			break;

			case 5: // ADSR2 (Envelope)
				Cores[0].Voices[voice].ADSR.SustainMode = (value & 0xE000)>>13;
				Cores[0].Voices[voice].ADSR.SustainRate = (value & 0x1FC0)>>6;
				Cores[0].Voices[voice].ADSR.ReleaseMode = (value & 0x20)>>5;
				Cores[0].Voices[voice].ADSR.ReleaseRate = (value & 0x1F);
				Cores[0].Voices[voice].ADSR.Reg_ADSR2 = value;
			break;
			
			case 6:
				Cores[0].Voices[voice].ADSR.Value = ((s32)value<<16) | value;
				ConLog( "* SPU2: Mysterious ADSR Volume Set to 0x%x", value );
			break;

			case 7:	Cores[0].Voices[voice].LoopStartA = (u32)value <<8;	break;

			jNO_DEFAULT;
		}
	}

	else switch(reg)
	{
		case 0x1d80://         Mainvolume left
			Cores[0].MasterVol.Left.Mode = 0;
			Cores[0].MasterVol.Left.RegSet( value );
		break;

		case 0x1d82://         Mainvolume right
			Cores[0].MasterVol.Right.Mode = 0;
			Cores[0].MasterVol.Right.RegSet( value );
		break;

		case 0x1d84://         Reverberation depth left
			Cores[0].FxVol.Left = GetVol32( value );
		break;

		case 0x1d86://         Reverberation depth right
			Cores[0].FxVol.Right = GetVol32( value );
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
			Cores[0].IRQA=(u32)value<<8;
		break;

		case 0x1da6:
			Cores[0].TSA=(u32)value<<8;
		break;

		case 0x1daa:
			SPU2_FastWrite(REG_C_ATTR,value);
		break;

		case 0x1dae:
			SPU2_FastWrite(REG_P_STATX,value);
		break;

		case 0x1da8:// Spu Write to Memory
			DmaWrite(0,value);
			show=false;
		break;
	}

	if(show) FileLog("[%10d] (!) SPU write mem %08x value %04x\n",Cycles,mem,value);

	spu2Ru16(mem)=value;
}

u16 SPU_ps1_read(u32 mem) 
{
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
				//value=Cores[0].Voices[voice].VolumeL.Mode;
				//value=Cores[0].Voices[voice].VolumeL.Value;
				value = Cores[0].Voices[voice].Volume.Left.Reg_VOL;
			break;
			
			case 1: //VOLR (Volume R)
				//value=Cores[0].Voices[voice].VolumeR.Mode;
				//value=Cores[0].Voices[voice].VolumeR.Value;
				value = Cores[0].Voices[voice].Volume.Right.Reg_VOL;
			break;
			
			case 2:	value = Cores[0].Voices[voice].Pitch;		break;
			case 3:	value = Cores[0].Voices[voice].StartA;		break;
			case 4: value = Cores[0].Voices[voice].ADSR.Reg_ADSR1;	break;
			case 5: value = Cores[0].Voices[voice].ADSR.Reg_ADSR2;	break;
			case 6:	value = Cores[0].Voices[voice].ADSR.Value >> 16;	break;
			case 7:	value = Cores[0].Voices[voice].LoopStartA;	break;

			jNO_DEFAULT;
		}
	}
	else switch(reg)
	{
		case 0x1d80: value = Cores[0].MasterVol.Left.Value >> 16;  break;
		case 0x1d82: value = Cores[0].MasterVol.Right.Value >> 16; break;
		case 0x1d84: value = Cores[0].FxVol.Left >> 16;            break;
		case 0x1d86: value = Cores[0].FxVol.Right >> 16;           break;

		case 0x1d88: value = 0; break;
		case 0x1d8a: value = 0; break;
		case 0x1d8c: value = 0; break;
		case 0x1d8e: value = 0; break;

		case 0x1d90: value = Cores[0].Regs.PMON&0xFFFF;   break;
		case 0x1d92: value = Cores[0].Regs.PMON>>16;      break;

		case 0x1d94: value = Cores[0].Regs.NON&0xFFFF;    break;
		case 0x1d96: value = Cores[0].Regs.NON>>16;       break;

		case 0x1d98: value = Cores[0].Regs.VMIXEL&0xFFFF; break;
		case 0x1d9a: value = Cores[0].Regs.VMIXEL>>16;    break;
		case 0x1d9c: value = Cores[0].Regs.VMIXL&0xFFFF;  break;
		case 0x1d9e: value = Cores[0].Regs.VMIXL>>16;     break;

		case 0x1da2:
			if( value != Cores[0].EffectsStartA>>3 )
			{
				value = Cores[0].EffectsStartA>>3;
				Cores[0].UpdateEffectsBufferSize();
				Cores[0].ReverbX = 0;
			}
		break;
		case 0x1da4: value = Cores[0].IRQA>>3;            break;
		case 0x1da6: value = Cores[0].TSA>>3;             break;

		case 0x1daa:
			value = SPU2read(REG_C_ATTR);
			break;
		case 0x1dae:
			value = 0; //SPU2read(REG_P_STATX)<<3;
			break;
		case 0x1da8:
			value = DmaRead(0);
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

__forceinline void SPU2_FastWrite( u32 rmem, u16 value )
{
	u32 vx=0, vc=0, core=0, omem, mem;
	omem=mem=rmem & 0x7FF; //FFFF;
	if (mem & 0x400) { omem^=0x400; core=1; }

	if (omem < 0x0180)	// Voice Params
	{ 
		const u32 voice = (omem & 0x1F0) >> 4;
		const u32 param = (omem & 0xF) >> 1;
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
					thisvol.Increment = (value & 0x3F);
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

			case 2:	thisvoice.Pitch=value;			break;
			case 3: // ADSR1 (Envelope)
				thisvoice.ADSR.AttackMode = (value & 0x8000)>>15;
				thisvoice.ADSR.AttackRate = (value & 0x7F00)>>8;
				thisvoice.ADSR.DecayRate = (value & 0xF0)>>4;
				thisvoice.ADSR.SustainLevel = (value & 0xF);
				thisvoice.ADSR.Reg_ADSR1 = value;	break;
			case 4: // ADSR2 (Envelope)
				thisvoice.ADSR.SustainMode = (value & 0xE000)>>13;
				thisvoice.ADSR.SustainRate = (value & 0x1FC0)>>6;
				thisvoice.ADSR.ReleaseMode = (value & 0x20)>>5;
				thisvoice.ADSR.ReleaseRate = (value & 0x1F);
				thisvoice.ADSR.Reg_ADSR2 = value;	break;
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
	else if ((omem >= 0x01C0) && (omem < 0x02DE))
	{
		const u32 voice   = ((omem-0x01C0) / 12);
		const u32 address = ((omem-0x01C0) % 12) >> 1;
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

			case 4:
				thisvoice.NextA = ((value & 0x0F) << 16) | (thisvoice.NextA & 0xFFF8);
			break;
			
			case 5:
				thisvoice.NextA = (thisvoice.NextA & 0x0F0000) | (value & 0xFFF8);
			break;
		}
	}
	else if((mem>=0x07C0) && (mem<0x07CE)) 
	{
		*(regtable[mem>>1]) = value;
		UpdateSpdifMode();
	}
	else if( mem >= R_FB_SRC_A && mem < REG_A_EEA )
	{
		// Signal to the Reverb code that the effects buffers need to be re-aligned.
		// This is both simple, efficient, and safe, since we only want to re-align
		// buffers after both hi and lo words have been written.

		*(regtable[mem>>1]) = value;
		Cores[core].RevBuffers.NeedsUpdated = true;
	}
	else
	{
		V_Core& thiscore = Cores[core];
		switch(omem)
		{
			case REG_C_ATTR:
			{
				int irqe = thiscore.IRQEnable;
				int bit0 = thiscore.AttrBit0;
				int bit4 = thiscore.AttrBit4;

				if( ((value>>15)&1) && (!thiscore.CoreEnabled) && (thiscore.InitDelay==0) ) // on init/reset
				{
					// When we have exact cycle update info from the Pcsx2 IOP unit, then use
					// the more accurate delayed initialization system.

					if(cyclePtr != NULL)
					{
						thiscore.InitDelay  = 1;
						thiscore.Regs.STATX = 0;
					}
					else
					{
						thiscore.Reset();
					}
				}

				thiscore.AttrBit0   =(value>> 0) & 0x01; //1 bit
				thiscore.DMABits	=(value>> 1) & 0x07; //3 bits
				thiscore.AttrBit4   =(value>> 4) & 0x01; //1 bit
				thiscore.AttrBit5   =(value>> 5) & 0x01; //1 bit
				thiscore.IRQEnable  =(value>> 6) & 0x01; //1 bit
				thiscore.FxEnable   =(value>> 7) & 0x01; //1 bit
				thiscore.NoiseClk   =(value>> 8) & 0x3f; //6 bits
				//thiscore.Mute	   =(value>>14) & 0x01; //1 bit
				thiscore.Mute=0;
				thiscore.CoreEnabled=(value>>15) & 0x01; //1 bit
				thiscore.Regs.ATTR  =value&0x7fff;

				if(value&0x000E)
				{
					ConLog(" * SPU2: Core %d ATTR unknown bits SET! value=%04x\n",core,value);
				}

				if(thiscore.AttrBit0!=bit0)
				{
					ConLog(" * SPU2: ATTR bit 0 set to %d\n",thiscore.AttrBit0);
				}
				if(thiscore.IRQEnable!=irqe)
				{
					ConLog(" * SPU2: IRQ %s\n",((thiscore.IRQEnable==0)?"disabled":"enabled"));
					if(!thiscore.IRQEnable)
						Spdif.Info=0;
				}

			}
			break;

			case REG_S_PMON:
				vx=2; for (vc=1;vc<16;vc++) { thiscore.Voices[vc].Modulated=(s8)((value & vx)/vx); vx<<=1; }
				SetLoWord( thiscore.Regs.PMON, value );
			break;

			case (REG_S_PMON + 2):
				vx=1; for (vc=16;vc<24;vc++) { thiscore.Voices[vc].Modulated=(s8)((value & vx)/vx); vx<<=1; }
				SetHiWord( thiscore.Regs.PMON, value );
			break;

			case REG_S_NON:
				vx=1; for (vc=0;vc<16;vc++) { thiscore.Voices[vc].Noise=(s8)((value & vx)/vx); vx<<=1; }
				SetLoWord( thiscore.Regs.NON, value );
			break;

			case (REG_S_NON + 2):
				vx=1; for (vc=16;vc<24;vc++) { thiscore.Voices[vc].Noise=(s8)((value & vx)/vx); vx<<=1; }
				SetHiWord( thiscore.Regs.NON, value );
			break;

// Games like to repeatedly write these regs over and over with the same value, hence
// the shortcut that skips the bitloop if the values are equal.
#define vx_SetSomeBits( reg_out, mask_out, hiword ) \
{ \
	const u32 result		= thiscore.Regs.reg_out; \
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
	
				// Each MMIX gate is assigned either 0 or 0xffffffff depending on the status
				// of the MMIX bits.  I use -1 below as a shorthand for 0xffffffff. :)
			
				vx = value;
				if (core == 0) vx&=0xFF0;
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
				SetHiWord( thiscore.EffectsStartA, value );
				thiscore.UpdateEffectsBufferSize();
				thiscore.ReverbX = 0;
			break;

			case (REG_A_ESA + 2):
				SetLoWord( thiscore.EffectsStartA, value );
				thiscore.UpdateEffectsBufferSize();
				thiscore.ReverbX = 0;
			break;

			case REG_A_EEA:
				thiscore.EffectsEndA = ((u32)value<<16) | 0xFFFF;
				thiscore.UpdateEffectsBufferSize();
				thiscore.ReverbX = 0;
			break;
			
			// Master Volume Address Write!
			
			case REG_P_MVOLL:
			case REG_P_MVOLR:
			{
				V_VolumeSlide& thisvol = (omem==REG_P_MVOLL) ? thiscore.MasterVol.Left : thiscore.MasterVol.Right;

				if( value & 0x8000 )	// +Lin/-Lin/+Exp/-Exp
				{ 
					thisvol.Mode = (value & 0xE000) / 0x2000;
					thisvol.Increment = (value & 0x7F); // | ((value & 0x800)/0x10);
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

			case REG_S_ADMAS:
				//ConLog(" * SPU2: Core %d AutoDMAControl set to %d (%d)\n",core,value, Cycles);
				thiscore.AutoDMACtrl=value;

				if(value==0)
				{
					thiscore.AdmaInProgress=0;
				}
			break;

			default:
				*(regtable[mem>>1]) = value;
			break;
		}
	}
}


void StartVoices(int core, u32 value)
{
	// Optimization: Games like to write zero to the KeyOn reg a lot, so shortcut
	// this loop if value is zero.

	if( value == 0 ) return;

	Cores[core].Regs.ENDX &= ~value;
	
	for( u8 vc=0; vc<V_Core::NumVoices; vc++ )
	{
		if ((value>>vc) & 1)
		{
			Cores[core].Voices[vc].Start();

			if( IsDevBuild )
			{
				V_Voice& thisvc( Cores[core].Voices[vc] );

				if(MsgKeyOnOff()) ConLog(" * SPU2: KeyOn: C%dV%02d: SSA: %8x; M: %s%s%s%s; H: %02x%02x; P: %04x V: %04x/%04x; ADSR: %04x%04x\n",
					core,vc,thisvc.StartA,
					(Cores[core].VoiceGates[vc].DryL)?"+":"-",(Cores[core].VoiceGates[vc].DryR)?"+":"-",
					(Cores[core].VoiceGates[vc].WetL)?"+":"-",(Cores[core].VoiceGates[vc].WetR)?"+":"-",
					*(u8*)GetMemPtr(thisvc.StartA),*(u8 *)GetMemPtr((thisvc.StartA)+1),
					thisvc.Pitch,
					thisvc.Volume.Left.Value>>16,thisvc.Volume.Right.Value>>16,
					thisvc.ADSR.Reg_ADSR1,thisvc.ADSR.Reg_ADSR2);
			}
		}
	}
}

void StopVoices(int core, u32 value)
{
	if( value == 0 ) return;
	for( u8 vc=0; vc<V_Core::NumVoices; vc++ )
	{
		if ((value>>vc) & 1)
		{
			Cores[core].Voices[vc].ADSR.Releasing = true;
			//if(MsgKeyOnOff()) ConLog(" * SPU2: KeyOff: Core %d; Voice %d.\n",core,vc);
		}
	}
}

