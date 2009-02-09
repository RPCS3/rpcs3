//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "spu2.h"
#include "regtable.h"

#include "svnrev.h"

// [Air]: Adding the spu2init boolean wasn't necessary except to help me in
//   debugging the spu2 suspend/resume behavior (when user hits escape).
static bool spu2open=false;	// has spu2open plugin interface been called?
static bool spu2init=false;	// has spu2init plugin interface been called?

static s32 logvolume[16384];
static u32  pClocks=0;


static const u8 version  = PS2E_SPU2_VERSION;
static const u8 revision = 1;
static const u8 build	 = 9;	// increase that with each version

static char libraryName[256];

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	if(dwReason==DLL_PROCESS_ATTACH) hInstance=hinstDLL;
	return TRUE;
}

static void InitLibraryName()
{
#ifdef PUBLIC

	// Public Release!
	// Output a simplified string that's just our name:

	strcpy( libraryName, "SPU2ghz" );

#elif defined( SVN_REV_UNKNOWN )

	// Unknown revision.
	// Output a name that includes devbuild status but not
	// subversion revision tags:

	strcpy( libraryName, "SPU2ghz"
#	ifdef _DEBUG_FAST
		"-Debug"
#	elif defined( DEBUG )
		"-Debug/Strict"		// strict debugging is slow!
#	else
		"-Dev"
#	endif
		);
#else

	// Use TortoiseSVN's SubWCRev utility's output
	// to label the specific revision:

	sprintf_s( libraryName, "SPU2ghz r%d%s"
#	ifdef _DEBUG_FAST
		"-Debug"
#	elif defined( _DEBUG )
		"-Debug/Strict"		// strict debugging is slow!
#	else
		"-Dev"
#	endif
		,SVN_REV,
		SVN_MODS ? "m" : ""
		);
#endif

}

EXPORT_C_(u32) PS2EgetLibType() 
{
	return PS2E_LT_SPU2;
}

EXPORT_C_(char*) PS2EgetLibName() 
{
	InitLibraryName();
	return libraryName;
}

EXPORT_C_(u32) PS2EgetLibVersion2(u32 type) 
{
	return (version<<16)|(revision<<8)|build;
}

EXPORT_C_(void) SPU2configure() {
	configure();
}

EXPORT_C_(void) SPU2about() {
	InitLibraryName();
	SysMessage( libraryName );
}

EXPORT_C_(s32) SPU2test() {
	return SndTest();
}

EXPORT_C_(s32) SPU2init() 
{
#define MAKESURE(a,b) \
	/*fprintf(stderr,"%08p: %08p == %08p\n",&(regtable[a>>1]),regtable[a>>1],U16P(b));*/ \
	assert(regtable[(a)>>1]==U16P(b))

	MAKESURE(0x800,zero);

	s32 c=0,v=0;
	ReadSettings();

#ifdef SPU2_LOG
	if(AccessLog()) 
	{
		spu2Log = fopen(AccessLogFileName, "w");
		setvbuf(spu2Log, NULL,  _IONBF, 0);
		FileLog("SPU2init\n");
	}
#endif
	srand((unsigned)time(NULL));

	disableFreezes=false;

	if (spu2init)
	{
		ConLog( " * SPU2: Already initialized - Ignoring SPU2init signal." );
		return 0;
	}

	spu2init=true;

	spu2regs  = (short*)malloc(0x010000);
	_spu2mem  = (short*)malloc(0x200000);

	// adpcm decoder cache:
	//  the cache data size is determined by taking the number of adpcm blocks
	//  (2MB / 16) and multiplying it by the decoded block size (28 samples).
	//  Thus: pcm_cache_data = 7,340,032 bytes (ouch!)
	//  Expanded: 16 bytes expands to 56 bytes [3.5:1 ratio]
	//    Resulting in 2MB * 3.5.

	pcm_cache_data = (PcmCacheEntry*)calloc( pcm_BlockCount, sizeof(PcmCacheEntry) );

	if( (spu2regs == NULL) || (_spu2mem == NULL) ||
		(pcm_cache_data == NULL) )
	{
		SysMessage("SPU2: Error allocating Memory\n"); return -1;
	}

	for(int mem=0;mem<0x800;mem++)
	{
		u16 *ptr=regtable[mem>>1];
		if(!ptr) {
			regtable[mem>>1] = &(spu2Ru16(mem));
		}
	}

	memset(spu2regs,0,0x010000);
	memset(_spu2mem,0,0x200000);
	memset(&Cores,0,(sizeof(V_Core) * 2));
	CoreReset(0);
	CoreReset(1);

	DMALogOpen();

	if(WaveLog()) 
	{
		if(!wavedump_open())
		{
			SysMessage("Can't open '%s'.\nWave Log disabled.",WaveLogFileName);
		}
	}

	for(v=0;v<16384;v++)
	{
		logvolume[v]=(s32)(s32)floor(log((double)(v+1))*3376.7);
	}

	LowPassFilterInit();
	InitADSR();

#ifdef STREAM_DUMP
	il0=fopen("logs/spu2input0.pcm","wb");
	il1=fopen("logs/spu2input1.pcm","wb");
#endif

#ifdef EFFECTS_DUMP
	el0=fopen("logs/spu2fx0.pcm","wb");
	el1=fopen("logs/spu2fx1.pcm","wb");
#endif


#ifdef S2R_ENABLE
	if(!replay_mode)
		s2r_open("replay_dump.s2r");
#endif
	return 0;
}

EXPORT_C_(s32) SPU2open(void *pDsp)
{
	if( spu2open ) return 0;

	FileLog("[%10d] SPU2 Open\n",Cycles);

	/*
	if(debugDialogOpen==0)
	{
	hDebugDialog = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_DEBUG),0,DebugProc,0);
	ShowWindow(hDebugDialog,SW_SHOWNORMAL);
	debugDialogOpen=1;
	}*/

	spu2open=true;
	if (!SndInit())
	{
		srate_pv=(double)SampleRate/48000.0;

		spdif_init();

		DspLoadLibrary(dspPlugin,dspPluginModule);

		return 0;
	}
	else 
	{
		SPU2close();
		return -1;
	};
}

EXPORT_C_(void) SPU2close() 
{
	if( !spu2open ) return;
	FileLog("[%10d] SPU2 Close\n",Cycles);

	DspCloseLibrary();
	spdif_shutdown();
	SndClose();

	spu2open = false;
}

EXPORT_C_(void) SPU2shutdown() 
{
	if(!spu2init) return;

	ConLog( " * SPU2: Shutting down.\n" );

	SPU2close();

#ifdef S2R_ENABLE
	if(!replay_mode)
		s2r_close();
#endif

	DoFullDump();
#ifdef STREAM_DUMP
	fclose(il0);
	fclose(il1);
#endif
#ifdef EFFECTS_DUMP
	fclose(el0);
	fclose(el1);
#endif
	if(WaveLog() && wavedump_ok) wavedump_close();

	DMALogClose();

	spu2init = false;

	SAFE_FREE(spu2regs);
	SAFE_FREE(_spu2mem);

	SAFE_FREE( pcm_cache_data );

	spu2regs = NULL;
	_spu2mem = NULL;
	pcm_cache_data = NULL;

#ifdef SPU2_LOG
	if(!AccessLog()) return;
	FileLog("[%10d] SPU2shutdown\n",Cycles);
	if(spu2Log) fclose(spu2Log);
#endif
}

EXPORT_C_(void) SPU2setClockPtr(u32 *ptr)
{
	cPtr=ptr;
	hasPtr=(cPtr!=NULL);
}

bool numpad_minus_old=false;
bool numpad_minus = false;
bool numpad_plus = false, numpad_plus_old = false;

EXPORT_C_(void) SPU2async(u32 cycles) 
{
#ifndef PUBLIC
	u32 oldClocks = lClocks;
	static u32 timer=0,time1=0,time2=0;
	timer++;
	if (timer == 1){
		time1=timeGetTime();
	}
	if (timer == 3000){
		time2 = timeGetTime()-time1 ;
		timer=0;
	}
#endif

	DspUpdate();

	if(LimiterToggleEnabled)
	{
		numpad_minus = (GetAsyncKeyState(VK_SUBTRACT)&0x8000)!=0;

		if(numpad_minus && !numpad_minus_old)
		{
			if(LimitMode) LimitMode=0;
			else		  LimitMode=1;
			SndUpdateLimitMode();
		}
		numpad_minus_old = numpad_minus;
	}

#ifndef PUBLIC
	/*numpad_plus = (GetAsyncKeyState(VK_ADD)&0x8000)!=0;
	if(numpad_plus && !numpad_plus_old)
	{
	DoFullDump();
	}
	numpad_plus_old = numpad_plus;*/
#endif

	if(hasPtr)
	{
		TimeUpdate(*cPtr); 
	}
	else
	{
		pClocks+=cycles;
		TimeUpdate(pClocks);
	}
}

EXPORT_C_(void) SPU2irqCallback(void (*SPU2callback)(),void (*DMA4callback)(),void (*DMA7callback)())
{
	_irqcallback=SPU2callback;
	dma4callback=DMA4callback;
	dma7callback=DMA7callback;
}

EXPORT_C_(u16) SPU2read(u32 rmem) 
{
	//	if(!replay_mode)
	//		s2r_readreg(Cycles,rmem);

	if(hasPtr) TimeUpdate(*cPtr);

	u16 ret=0xDEAD; u32 core=0, mem=rmem&0xFFFF, omem=mem;
	if (mem & 0x400) { omem^=0x400; core=1; }

	if(rmem==0x1f9001AC)
	{
		ret =  DmaRead(core);
	}
	else if (rmem>>16 == 0x1f80)
	{
		ret = SPU_ps1_read(rmem);
	}
	else if ((mem&0xFFFF)>=0x800)
	{
		ret=spu2Ru16(mem);
		ConLog(" * SPU2: Read from reg>=0x800: %x value %x\n",mem,ret);
		FileLog(" * SPU2: Read from reg>=0x800: %x value %x\n",mem,ret);
	}
	else 
	{
		ret = *(regtable[(mem>>1)]);

		FileLog("[%10d] SPU2 read mem %x (core %d, register %x): %x\n",Cycles, mem, core, (omem & 0x7ff), ret);
	}

	return ret;
}

static __forceinline void SPU2_FastWrite( u32 rmem, u16 value )
{
	u32 vx=0, vc=0, core=0, omem, mem;
	omem=mem=rmem & 0x7FF; //FFFF;
	if (mem & 0x400) { omem^=0x400; core=1; }

	//else if ((omem >= 0x0000) && (omem < 0x0180)) { // Voice Params
	if (omem < 0x0180) { // Voice Params
		u32 voice=(omem & 0x1F0) >> 4;
		u32 param=(omem & 0xF)>>1;
		//FileLog("[%10d] SPU2 write mem %08x (Core %d Voice %d Param %s) value %x\n",Cycles,rmem,core,voice,ParamNames[param],value);
		switch (param) { 
			case 0: //VOLL (Volume L)
				if (value & 0x8000) {  // +Lin/-Lin/+Exp/-Exp
					Cores[core].Voices[voice].VolumeL.Mode=(value & 0xF000)>>12;
					Cores[core].Voices[voice].VolumeL.Increment=(value & 0x3F);
				}
				else {
					Cores[core].Voices[voice].VolumeL.Mode=0;
					Cores[core].Voices[voice].VolumeL.Increment=0;
					if(value&0x4000)
						value=0x3fff - (value&0x3fff);
					Cores[core].Voices[voice].VolumeL.Value=value<<1;
				}
				Cores[core].Voices[voice].VolumeL.Reg_VOL = value;	break;
			case 1: //VOLR (Volume R)
				if (value & 0x8000) {
					Cores[core].Voices[voice].VolumeR.Mode=(value & 0xF000)>>12;
					Cores[core].Voices[voice].VolumeR.Increment=(value & 0x3F);
				}
				else {
					Cores[core].Voices[voice].VolumeR.Mode=0;
					Cores[core].Voices[voice].VolumeR.Increment=0;
					Cores[core].Voices[voice].VolumeR.Value=value<<1;
				}
				Cores[core].Voices[voice].VolumeR.Reg_VOL = value;	break;
			case 2:	Cores[core].Voices[voice].Pitch=value;			break;
			case 3: // ADSR1 (Envelope)
				Cores[core].Voices[voice].ADSR.Am=(value & 0x8000)>>15;
				Cores[core].Voices[voice].ADSR.Ar=(value & 0x7F00)>>8;
				Cores[core].Voices[voice].ADSR.Dr=(value & 0xF0)>>4;
				Cores[core].Voices[voice].ADSR.Sl=(value & 0xF);
				Cores[core].Voices[voice].ADSR.Reg_ADSR1 = value;	break;
			case 4: // ADSR2 (Envelope)
				Cores[core].Voices[voice].ADSR.Sm=(value & 0xE000)>>13;
				Cores[core].Voices[voice].ADSR.Sr=(value & 0x1FC0)>>6;
				Cores[core].Voices[voice].ADSR.Rm=(value & 0x20)>>5;
				Cores[core].Voices[voice].ADSR.Rr=(value & 0x1F);
				Cores[core].Voices[voice].ADSR.Reg_ADSR2 = value;	break;
			case 5:
				// [Air] : Mysterious volume set code.  Too bad none of my games ever use it.
				//      (as usual... )
				Cores[core].Voices[voice].ADSR.Value = value << 15;
				ConLog( "* SPU2: Mysterious ADSR Volume Set to 0x%x", value );
			break;
			case 6:	Cores[core].Voices[voice].VolumeL.Value=value;	break;
			case 7:	Cores[core].Voices[voice].VolumeR.Value=value;	break;

			jNO_DEFAULT;
		}
	}
	else if ((omem >= 0x01C0) && (omem < 0x02DE)) {
		u32 voice   =((omem-0x01C0) / 12);
		u32 address =((omem-0x01C0) % 12)>>1;
		//FileLog("[%10d] SPU2 write mem %08x (Core %d Voice %d Address %s) value %x\n",Cycles,rmem,core,voice,AddressNames[address],value);
		
		switch (address) {
			case 0:	Cores[core].Voices[voice].StartA=((value & 0x0F) << 16) | (Cores[core].Voices[voice].StartA & 0xFFF8); 
					#ifndef PUBLIC
					DebugCores[core].Voices[voice].lastSetStartA = Cores[core].Voices[voice].StartA; 
					#endif
					break;
			case 1:	Cores[core].Voices[voice].StartA=(Cores[core].Voices[voice].StartA & 0x0F0000) | (value & 0xFFF8); 
					#ifndef PUBLIC
					DebugCores[core].Voices[voice].lastSetStartA = Cores[core].Voices[voice].StartA; 
					#endif
					//if(core==1) printf(" *** StartA for C%dV%02d set to 0x%05x\n",core,voice,Cores[core].Voices[voice].StartA);
					break;
			case 2:	Cores[core].Voices[voice].LoopStartA=((value & 0x0F) << 16) | (Cores[core].Voices[voice].LoopStartA & 0xFFF8);
					Cores[core].Voices[voice].LoopMode=3; break;
			case 3:	Cores[core].Voices[voice].LoopStartA=(Cores[core].Voices[voice].LoopStartA & 0x0F0000) | (value & 0xFFF8);break;
					Cores[core].Voices[voice].LoopMode=3; break;
			case 4:	Cores[core].Voices[voice].NextA=((value & 0x0F) << 16) | (Cores[core].Voices[voice].NextA & 0xFFF8);
					//printf(" *** Warning: C%dV%02d NextA MODIFIED EXTERNALLY!\n",core,voice);
					break;
			case 5:	Cores[core].Voices[voice].NextA=(Cores[core].Voices[voice].NextA & 0x0F0000) | (value & 0xFFF8);
					//printf(" *** Warning: C%dV%02d NextA MODIFIED EXTERNALLY!\n",core,voice);
					break;
		}
	}
	else
		switch(omem) {
		case REG_C_ATTR:
			RegLog(4,"ATTR",rmem,core,value);
			{
				int irqe=Cores[core].IRQEnable;
				int bit0=Cores[core].AttrBit0;
				int bit4=Cores[core].AttrBit4;

				if(((value>>15)&1)&&(!Cores[core].CoreEnabled)&&(Cores[core].InitDelay==0)) // on init/reset
				{
					if(hasPtr)
					{
						Cores[core].InitDelay=1;
						Cores[core].Regs.STATX=0;	
					}
					else
					{
						CoreReset(core);
					}
				}

				Cores[core].AttrBit0   =(value>> 0) & 0x01; //1 bit
				Cores[core].DMABits	   =(value>> 1) & 0x07; //3 bits
				Cores[core].AttrBit4   =(value>> 4) & 0x01; //1 bit
				Cores[core].AttrBit5   =(value>> 5) & 0x01; //1 bit
				Cores[core].IRQEnable  =(value>> 6) & 0x01; //1 bit
				Cores[core].FxEnable   =(value>> 7) & 0x01; //1 bit
				Cores[core].NoiseClk   =(value>> 8) & 0x3f; //6 bits
				//Cores[core].Mute	   =(value>>14) & 0x01; //1 bit
				Cores[core].Mute=0;
				Cores[core].CoreEnabled=(value>>15) & 0x01; //1 bit
				Cores[core].Regs.ATTR  =value&0x7fff;

				if(value&0x000E)
				{
					ConLog(" * SPU2: Core %d ATTR unknown bits SET! value=%04x\n",core,value);
				}

				if(Cores[core].AttrBit0!=bit0)
				{
					ConLog(" * SPU2: ATTR bit 0 set to %d\n",Cores[core].AttrBit0);
				}
				if(Cores[core].IRQEnable!=irqe)
				{
					ConLog(" * SPU2: IRQ %s\n",((Cores[core].IRQEnable==0)?"disabled":"enabled"));
					if(!Cores[core].IRQEnable)
						Spdif.Info=0;
				}

			}
			break;
		case REG_S_PMON:
			RegLog(1,"PMON0",rmem,core,value);
			vx=2; for (vc=1;vc<16;vc++) { Cores[core].Voices[vc].Modulated=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.PMON = (Cores[core].Regs.PMON & 0xFFFF0000) | value;
			break;
		case (REG_S_PMON + 2):
			RegLog(1,"PMON1",rmem,core,value);
			vx=1; for (vc=16;vc<24;vc++) { Cores[core].Voices[vc].Modulated=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.PMON = (Cores[core].Regs.PMON & 0xFFFF) | (value << 16);
			break;
		case REG_S_NON:
			RegLog(1,"NON0",rmem,core,value);
			vx=1; for (vc=0;vc<16;vc++) { Cores[core].Voices[vc].Noise=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.NON = (Cores[core].Regs.NON & 0xFFFF0000) | value;
			break;
		case (REG_S_NON + 2):
			RegLog(1,"NON1",rmem,core,value);
			vx=1; for (vc=16;vc<24;vc++) { Cores[core].Voices[vc].Noise=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.NON = (Cores[core].Regs.NON & 0xFFFF) | (value << 16);
			break;
		case REG_S_VMIXL:
			RegLog(1,"VMIXL0",rmem,core,value);
			vx=1; for (vc=0;vc<16;vc++) { Cores[core].Voices[vc].DryL=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.VMIXL = (Cores[core].Regs.VMIXL & 0xFFFF0000) | value;
		case (REG_S_VMIXL + 2):
			RegLog(1,"VMIXL1",rmem,core,value);
			vx=1; for (vc=16;vc<24;vc++) { Cores[core].Voices[vc].DryL=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.VMIXL = (Cores[core].Regs.VMIXL & 0xFFFF) | (value << 16);
		case REG_S_VMIXEL:
			RegLog(1,"VMIXEL0",rmem,core,value);
			vx=1; for (vc=0;vc<16;vc++) { Cores[core].Voices[vc].WetL=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.VMIXEL = (Cores[core].Regs.VMIXEL & 0xFFFF0000) | value;
			break;
		case (REG_S_VMIXEL + 2):
			RegLog(1,"VMIXEL1",rmem,core,value);
			vx=1; for (vc=16;vc<24;vc++) { Cores[core].Voices[vc].WetL=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.VMIXEL = (Cores[core].Regs.VMIXEL & 0xFFFF) | (value << 16);
			break;
		case REG_S_VMIXR:
			RegLog(1,"VMIXR0",rmem,core,value);
			vx=1; for (vc=0;vc<16;vc++) { Cores[core].Voices[vc].DryR=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.VMIXR = (Cores[core].Regs.VMIXR & 0xFFFF0000) | value;
			break;
		case (REG_S_VMIXR + 2):
			RegLog(1,"VMIXR1",rmem,core,value);
			vx=1; for (vc=16;vc<24;vc++) { Cores[core].Voices[vc].DryR=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.VMIXR = (Cores[core].Regs.VMIXR & 0xFFFF) | (value << 16);
			break;
		case REG_S_VMIXER:
			RegLog(1,"VMIXER0",rmem,core,value);
			vx=1; for (vc=0;vc<16;vc++) { Cores[core].Voices[vc].WetR=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.VMIXER = (Cores[core].Regs.VMIXER & 0xFFFF0000) | value;
			break;
		case (REG_S_VMIXER + 2):
			RegLog(1,"VMIXER1",rmem,core,value);
			vx=1; for (vc=16;vc<24;vc++) { Cores[core].Voices[vc].WetR=(s8)((value & vx)/vx); vx<<=1; }
			Cores[core].Regs.VMIXER = (Cores[core].Regs.VMIXER & 0xFFFF) | (value << 16);
			break;
		case REG_P_MMIX:
			RegLog(1,"MMIX",rmem,core,value);
			vx=value;
			if (core == 0) vx&=0xFF0;
			Cores[core].ExtWetR=(vx & 0x001);
			Cores[core].ExtWetL=(vx & 0x002)>>1;
			Cores[core].ExtDryR=(vx & 0x004)>>2;
			Cores[core].ExtDryL=(vx & 0x008)>>3;
			Cores[core].InpWetR=(vx & 0x010)>>4;
			Cores[core].InpWetL=(vx & 0x020)>>5;
			Cores[core].InpDryR=(vx & 0x040)>>6;
			Cores[core].InpDryL=(vx & 0x080)>>7;
			Cores[core].SndWetR=(vx & 0x100)>>8;
			Cores[core].SndWetL=(vx & 0x200)>>9;
			Cores[core].SndDryR=(vx & 0x400)>>10;
			Cores[core].SndDryL=(vx & 0x800)>>11;
			Cores[core].Regs.MMIX = value;
			break;
		case (REG_S_KON + 2):
			RegLog(2,"KON1",rmem,core,value);
			StartVoices(core,((u32)value)<<16);
			break;
		case REG_S_KON:
			RegLog(2,"KON0",rmem,core,value);
			StartVoices(core,((u32)value));
			break;
		case (REG_S_KOFF + 2):
			RegLog(2,"KOFF1",rmem,core,value);
			StopVoices(core,((u32)value)<<16);
			break;
		case REG_S_KOFF:
			RegLog(2,"KOFF0",rmem,core,value);
			StopVoices(core,((u32)value));
			break;
		case REG_S_ENDX:
			//ConLog(" * SPU2: Core %d ENDX cleared!\n",core);
			RegLog(2,"ENDX0",rmem,core,value);
			Cores[core].Regs.ENDX&=0x00FF0000; break;
		case (REG_S_ENDX + 2):	
			//ConLog(" * SPU2: Core %d ENDX cleared!\n",core);
			RegLog(2,"ENDX1",rmem,core,value);
			Cores[core].Regs.ENDX&=0xFFFF; break;
		case REG_P_MVOLL:
			RegLog(1,"MVOLL",rmem,core,value);
			if (value & 0x8000) {  // +Lin/-Lin/+Exp/-Exp
				Cores[core].MasterL.Mode=(value & 0xE000)/0x2000;
				Cores[core].MasterL.Increment=(value & 0x3F) | ((value & 0x800)/0x10);
			}
			else {
				Cores[core].MasterL.Mode=0;
				Cores[core].MasterL.Increment=0;
				Cores[core].MasterL.Value=value;
			}
			Cores[core].MasterL.Reg_VOL=value;
			break;
		case REG_P_MVOLR:
			RegLog(1,"MVOLR",rmem,core,value);
			if (value & 0x8000) {  // +Lin/-Lin/+Exp/-Exp
				Cores[core].MasterR.Mode=(value & 0xE000)/0x2000;
				Cores[core].MasterR.Increment=(value & 0x3F) | ((value & 0x800)/0x10);
			}
			else {
				Cores[core].MasterR.Mode=0;
				Cores[core].MasterR.Increment=0;
				Cores[core].MasterR.Value=value;
			}
			Cores[core].MasterR.Reg_VOL=value;
			break;
		case REG_S_ADMAS:
			RegLog(3,"ADMAS",rmem,core,value);
			//ConLog(" * SPU2: Core %d AutoDMAControl set to %d (%d)\n",core,value, Cycles);
			Cores[core].AutoDMACtrl=value;

			if(value==0)
			{
				Cores[core].AdmaInProgress=0;
			}
			break;

		default:
			*(regtable[mem>>1])=value;
			break;
	}

	SPU2writeLog(mem,value);
	if ((mem>=0x07C0) && (mem<0x07CE)) 
	{
		UpdateSpdifMode();
	}
}

EXPORT_C_(void) SPU2write(u32 rmem, u16 value) 
{
#ifdef S2R_ENABLE
	if(!replay_mode)
		s2r_writereg(Cycles,rmem,value);
#endif

	if(rmem==0x1f9001ac)
	{
		//RegWriteLog(0,value);
		if((Cores[0].IRQEnable)&&(Cores[0].TSA==Cores[0].IRQA))
		{
			Spdif.Info=4;
			SetIrqCall();
		}
		spu2M_Write( Cores[0].TSA++, value );
		Cores[0].TSA&=0xfffff;
	}
	else if(rmem==0x1f9005ac)
	{
		//RegWriteLog(1,value);
		if((Cores[0].IRQEnable)&&(Cores[0].TSA==Cores[0].IRQA))
		{
			Spdif.Info=4;
			SetIrqCall();
		}
		spu2M_Write( Cores[1].TSA++, value );
		Cores[1].TSA&=0xfffff;
	}
	else
	{
		if(hasPtr) TimeUpdate(*cPtr);

		if (rmem>>16 == 0x1f80)
			SPU_ps1_write(rmem,value);
		else
			SPU2_FastWrite( rmem, value );
	}
}

// if start is 1, starts recording spu2 data, else stops
// returns a non zero value if successful
// for now, pData is not used
EXPORT_C_(int) SPU2setupRecording(int start, void* pData)
{
	// Don't record if we have a bogus state.
	if( disableFreezes ) return 0;

	if(start==0)
	{
		//stop recording
		RecordStop();
		if(recording==0)
			return 1;
	}
	else if(start==1)
	{
		//start recording
		RecordStart();
		if(recording!=0)
			return 1;
	}
	return 0;
}
