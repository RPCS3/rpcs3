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
#include "SPU2.h"
#include "resource.h"
#include <assert.h>

#include "regtable.h"
#include "svnrev.h"

void StartVoices(int core, u32 value);
void StopVoices(int core, u32 value);

void InitADSR();

const unsigned char version  = PS2E_SPU2_VERSION;
const unsigned char revision = 1;
const unsigned char build	 = 9;	// increase that with each version

static __forceinline void SPU2_FastWrite( u32 rmem, u16 value );
static void SPU2writeLog(u32 rmem, u16 value);
DWORD CALLBACK TimeThread(PVOID /* unused param */);


const char *ParamNames[8]={"VOLL","VOLR","PITCH","ADSR1","ADSR2","ENVX","VOLXL","VOLXR"};
const char *AddressNames[6]={"SSAH","SSAL","LSAH","LSAL","NAXH","NAXL"};

double opitch;
int osps;

// [Air]: Adding the spu2init boolean wasn't necessary except to help me in
//   debugging the spu2 suspend/resume behavior (when user hits escape).
static bool spu2open=false;	// has spu2open plugin interface been called?
static bool spu2init=false;	// has spu2init plugin interface been called?

// [Air]: fixed the hacky part of UpdateTimer with this:
static bool resetClock = true;

// Used to make spu2 more robust at loading incompatible saves.
// Disables re-freezing of save state data.
bool disableFreezes=false;

void (* _irqcallback)();
void (* dma4callback)();
void (* dma7callback)();

short *spu2regs;
short *_spu2mem;
s32 uTicks;

u8 callirq;

HANDLE hThreadFunc;
u32	ThreadFuncID;

#ifndef PUBLIC
V_CoreDebug DebugCores[2];
#endif
V_Core Cores[2];
V_SPDIF Spdif;

s16 OutPos;
s16 InputPos;
u8 InpBuff;
u32 Cycles;
u32 Num;
u32 acumCycles;

u32* cPtr=NULL;
u32  lClocks=0;
u32  pClocks=0;

bool hasPtr=false;

int PlayMode;

s16 attrhack[2]={0,0};

HINSTANCE hInstance;

bool debugDialogOpen=false;
HWND hDebugDialog=NULL;

static char libraryName[256];

CRITICAL_SECTION threadSync;

s32 logvolume[16384];

bool has_to_call_irq=false;

void SetIrqCall()
{
	has_to_call_irq=true;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	if(dwReason==DLL_PROCESS_ATTACH) hInstance=hinstDLL;
	return TRUE;
}

void SysMessage(const char *fmt, ...) 
{
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	sprintf_s(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "SPU2ghz Msg", 0);
}

static void InitLibraryName()
{
#ifdef PUBLIC

	// Public Release!
	// Output a simplified string that's just our name:

	strcpy( libraryName, "SPU2ghz Playground" );

#elif defined( SVN_REV_UNKNOWN )

	// Unknown revision.
	// Output a name that includes devbuild status but not
	// subversion revision tags:

	strcpy( libraryName, "SPU2ghz Playground"
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

	sprintf_s( libraryName, "SPU2ghz PG r%d%s"
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


__forceinline s16 * __fastcall GetMemPtr(u32 addr)
{
#ifndef _DEBUG_FAST
	// In case you're wondering, this assert is the reason spu2ghz
	// runs so incrediously slow in Debug mode. :P
	assert(addr<0x100000);
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

void AssignVolume(V_Volume& vol, s16 value)
{
	vol.Reg_VOL = value;
	if (value & 0x8000) {  // +Lin/-Lin/+Exp/-Exp
		vol.Mode=(value & 0xF000)>>12;
		vol.Increment=(value & 0x3F);
	}
	else {
		vol.Mode=0;
		vol.Increment=0;
 
		value<<=1;
		vol.Value=value;
 
	}
}

void CoreReset(int c)
{
#define DEFAULT_VOICE_VOLUME 0x3FFF
	int v=0;
 
	ConLog(" * SPU2: Initializing core %d structures... ",c);
 
	memset(Cores+c,0,sizeof(Cores[c]));
 
	Cores[c].Regs.STATX=0;
	Cores[c].Regs.ATTR=0;
	Cores[c].ExtL=0x3FFF;
	Cores[c].ExtR=0x3FFF;
	Cores[c].InpL=0x3FFF;
	Cores[c].InpR=0x3FFF;
	Cores[c].FxL=0x7FFF;
	Cores[c].FxR=0x7FFF;
	Cores[c].MasterL.Reg_VOL=0x3FFF;
	Cores[c].MasterL.Value=0x7FFF;
	Cores[c].MasterR.Reg_VOL=0x3FFF;
	Cores[c].MasterR.Value=0x7FFF;
	Cores[c].ExtWetR=1;
	Cores[c].ExtWetL=1;
	Cores[c].ExtDryR=1;
	Cores[c].ExtDryL=1;
	Cores[c].InpWetR=1;
	Cores[c].InpWetL=1;
	Cores[c].InpDryR=1;
	Cores[c].InpDryL=1;
	Cores[c].SndWetR=1;
	Cores[c].SndWetL=1;
	Cores[c].SndDryR=1;
	Cores[c].SndDryL=1;
	Cores[c].Regs.MMIX = 0xFFCF;
	Cores[c].Regs.VMIXL = 0xFFFFFF;
	Cores[c].Regs.VMIXR = 0xFFFFFF;
	Cores[c].Regs.VMIXEL = 0xFFFFFF;
	Cores[c].Regs.VMIXER = 0xFFFFFF;
	Cores[c].EffectsStartA= 0xEFFF8 + 0x10000*c;
	Cores[c].EffectsEndA  = 0xEFFFF + 0x10000*c;
	Cores[c].FxEnable=0;
	Cores[c].IRQA=0xFFFF0;
	Cores[c].IRQEnable=1;
 
	for (v=0;v<24;v++) {
		AssignVolume(Cores[c].Voices[v].VolumeL,DEFAULT_VOICE_VOLUME);
		AssignVolume(Cores[c].Voices[v].VolumeR,DEFAULT_VOICE_VOLUME);
		Cores[c].Voices[v].ADSR.Value=0;
		Cores[c].Voices[v].ADSR.Phase=0;
		Cores[c].Voices[v].Pitch=0x3FFF;
		Cores[c].Voices[v].DryL=1;
		Cores[c].Voices[v].DryR=1;
		Cores[c].Voices[v].WetL=1;
		Cores[c].Voices[v].WetR=1;
		Cores[c].Voices[v].NextA=2800;
		Cores[c].Voices[v].StartA=2800;
		Cores[c].Voices[v].LoopStartA=2800;
		//Cores[c].Voices[v].lastSetStartA=2800; fixme: this is part of debug now
	}
	Cores[c].DMAICounter=0;
	Cores[c].AdmaInProgress=0;
 
	Cores[c].Regs.STATX=0x80;
 
	ConLog("done.\n");
}

extern void LowPassFilterInit();

EXPORT_C_(s32) SPU2init() 
{
#define MAKESURE(a,b) \
		/*fprintf(stderr,"%08p: %08p == %08p\n",&(regtable[a>>1]),regtable[a>>1],U16P(b));*/ \
		assert(regtable[(a)>>1]==U16P(b))

	MAKESURE(0x800,zero);

	s32 c=0,v=0;
	ReadSettings();
	acumCycles=0;

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


static BOOL CALLBACK DebugProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;

	switch(uMsg)
	{
		case WM_PAINT:
			return FALSE;
		case WM_INITDIALOG:
			{
				debugDialogOpen=true;
			}
			break;
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDOK:
				case IDCANCEL:
					debugDialogOpen=false;
					EndDialog(hWnd,0);
					break;
				default:
					return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
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

int FillRectangle(HDC dc, int left, int top, int width, int height)
{
	RECT r = { left, top, left+width, top+height };

	return FillRect(dc, &r, (HBRUSH)GetStockObject(DC_BRUSH));
}

BOOL DrawRectangle(HDC dc, int left, int top, int width, int height)
{
	RECT r = { left, top, left+width, top+height };

	POINT p[5] = {
		{ r.left, r.top },
		{ r.right, r.top },
		{ r.right, r.bottom },
		{ r.left, r.bottom },
		{ r.left, r.top },
	};

	return Polyline(dc, p, 5);
}

#ifndef PUBLIC
HFONT hf = NULL;
int lCount=0;
void UpdateDebugDialog()
{
	if(!debugDialogOpen) return;

	lCount++;
	if(lCount>=(SampleRate/10))
	{
		HDC hdc = GetDC(hDebugDialog);

		if(!hf)
		{
			hf = CreateFont( 8, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Lucida Console");
		}

		SelectObject(hdc,hf);
		SelectObject(hdc,GetStockObject(DC_BRUSH));
		SelectObject(hdc,GetStockObject(DC_PEN));

		for(int c=0;c<2;c++)
		{
			for(int v=0;v<24;v++)
			{
				int IX = 8+256*c;
				int IY = 8+ 32*v;
				V_Voice& vc(Cores[c].Voices[v]);
				V_VoiceDebug& vcd( DebugCores[c].Voices[v] );

				SetDCBrushColor(hdc,RGB(  0,  0,  0));
				if((vc.ADSR.Phase>0)&&(vc.ADSR.Phase<6))
				{
					SetDCBrushColor(hdc,RGB(  0,  0,128));
				}
				else
				{
					if(vcd.lastStopReason==1)
					{
						SetDCBrushColor(hdc,RGB(128,  0,  0));
					}
					if(vcd.lastStopReason==2)
					{
						SetDCBrushColor(hdc,RGB(  0,128,  0));
					}
				}

				FillRectangle(hdc,IX,IY,252,30);

				SetDCPenColor(hdc,RGB(  255,  128,  32));

				DrawRectangle(hdc,IX,IY,252,30);

				SetDCBrushColor  (hdc,RGB(  0,255,  0));

				int vl = abs(vc.VolumeL.Value * 24 / 32768);
				int vr = abs(vc.VolumeR.Value * 24 / 32768);

				FillRectangle(hdc,IX+38,IY+26 - vl, 4, vl);
				FillRectangle(hdc,IX+42,IY+26 - vr, 4, vr);

				int adsr = (vc.ADSR.Value>>16) * 24 / 32768;

				FillRectangle(hdc,IX+48,IY+26 - adsr, 4, adsr);

				int peak = vcd.displayPeak * 24 / 32768;

				FillRectangle(hdc,IX+56,IY+26 - peak, 4, peak);

				SetTextColor(hdc,RGB(  0,255,  0));
				SetBkColor  (hdc,RGB(  0,  0,  0));

				static char t[1024];

				sprintf(t,"%06x",vc.StartA);
				TextOut(hdc,IX+4,IY+3,t,6);

				sprintf(t,"%06x",vc.NextA);
				TextOut(hdc,IX+4,IY+12,t,6);

				sprintf(t,"%06x",vc.LoopStartA);
				TextOut(hdc,IX+4,IY+21,t,6);

				vcd.displayPeak = 0;

				if(vcd.lastSetStartA != vc.StartA)
				{
					printf(" *** Warning! Core %d Voice %d: StartA should be %06x, and is %06x.\n",
						c,v,vcd.lastSetStartA,vc.StartA);
					vcd.lastSetStartA = vc.StartA;
				}
			}
		}
		ReleaseDC(hDebugDialog,hdc);
		lCount=0;
	}

	MSG msg;
	while(PeekMessage(&msg,hDebugDialog,0,0,PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
#endif

#define TickInterval 768
#define SanityInterval 4800

u32 TicksCore=0;
u32 TicksThread=0;

static DWORD CALLBACK TimeThread(PVOID /* unused param */)
{
	while(spu2open)
	{
		if(TicksThread>=(TicksCore+320))
		{
			Sleep(1);
		}
		else if(TicksThread>=TicksCore)
		{
			Sleep(0);
		}
		else
		{
			Mix();
			TicksThread++;
		}
	}
	return 0;
}

void __fastcall TimeUpdate(u32 cClocks)
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
				CoreReset(0);
			}
		}

		if(Cores[1].InitDelay>0)
		{
			Cores[1].InitDelay--;
			if(Cores[1].InitDelay==0)
			{
				CoreReset(1);
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

		Mix();
	}
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

u16 mask = 0xFFFF;

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
		ConLog(" * SPU2: Play Mode Set to %s (%d).\n",(PlayMode==0)?"Normal":((PlayMode==1)?"PCM Clone":((PlayMode==2)?"PCM Bypass":"BitStream Bypass")),PlayMode);
	}
}

__forceinline void RegLog(int level, char *RName,u32 mem,u32 core,u16 value) 
{
	if( level > 1 )
		FileLog("[%10d] SPU2 write mem %08x (core %d, register %s) value %04x\n",Cycles,mem,core,RName,value);
}

static void SPU_ps1_write(u32 mem, u16 value) 
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
				Cores[0].Voices[voice].VolumeL.Mode=0;
				Cores[0].Voices[voice].VolumeL.Value=value<<1;
				Cores[0].Voices[voice].VolumeL.Reg_VOL = value;	break;
			case 1: //VOLR (Volume R)
				Cores[0].Voices[voice].VolumeR.Mode=0;
				Cores[0].Voices[voice].VolumeR.Value=value<<1;
				Cores[0].Voices[voice].VolumeR.Reg_VOL = value;	break;
			case 2:	Cores[0].Voices[voice].Pitch=value;			break;
			case 3:	Cores[0].Voices[voice].StartA=(u32)value<<8;	break;
			case 4: // ADSR1 (Envelope)
				Cores[0].Voices[voice].ADSR.Am=(value & 0x8000)>>15;
				Cores[0].Voices[voice].ADSR.Ar=(value & 0x7F00)>>8;
				Cores[0].Voices[voice].ADSR.Dr=(value & 0xF0)>>4;
				Cores[0].Voices[voice].ADSR.Sl=(value & 0xF);
				Cores[0].Voices[voice].ADSR.Reg_ADSR1 = value;	break;
			case 5: // ADSR2 (Envelope)
				Cores[0].Voices[voice].ADSR.Sm=(value & 0xE000)>>13;
				Cores[0].Voices[voice].ADSR.Sr=(value & 0x1FC0)>>6;
				Cores[0].Voices[voice].ADSR.Rm=(value & 0x20)>>5;
				Cores[0].Voices[voice].ADSR.Rr=(value & 0x1F);
				Cores[0].Voices[voice].ADSR.Reg_ADSR2 = value;	break;
			case 6:
				// [Air] Experimental --> shifting value into a 31 bit range.
				//  shifting by 16 might be more correct?
				Cores[0].Voices[voice].ADSR.Value = value<<15;
				ConLog( "* SPU2: Mysterious ADSR Volume Set to 0x%x", value );
			break;
			case 7:	Cores[0].Voices[voice].LoopStartA=(u32)value <<8;	break;

			jNO_DEFAULT;
		}
	}
	else switch(reg)
	{
		case 0x1d80://         Mainvolume left
			Cores[0].MasterL.Mode=0;
			Cores[0].MasterL.Value=value;
			break;
		case 0x1d82://         Mainvolume right
			Cores[0].MasterL.Mode=0;
			Cores[0].MasterR.Value=value;
			break;
		case 0x1d84://         Reverberation depth left
			Cores[0].FxL=value;
			break;
		case 0x1d86://         Reverberation depth right
			Cores[0].FxR=value;
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
				u32 val=(u32)value <<8;

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

static u16 SPU_ps1_read(u32 mem) 
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
				value=Cores[0].Voices[voice].VolumeL.Mode;
				value=Cores[0].Voices[voice].VolumeL.Value;
				value=Cores[0].Voices[voice].VolumeL.Reg_VOL;	break;
			case 1: //VOLR (Volume R)
				value=Cores[0].Voices[voice].VolumeR.Mode;
				value=Cores[0].Voices[voice].VolumeR.Value;
				value=Cores[0].Voices[voice].VolumeR.Reg_VOL;	break;
			case 2:	value=Cores[0].Voices[voice].Pitch;			break;
			case 3:	value=Cores[0].Voices[voice].StartA;	break;
			case 4: value=Cores[0].Voices[voice].ADSR.Reg_ADSR1;	break;
			case 5: value=Cores[0].Voices[voice].ADSR.Reg_ADSR2;	break;
			case 6:	value=Cores[0].Voices[voice].ADSR.Value >> 16;	break;
			case 7:	value=Cores[0].Voices[voice].LoopStartA;	break;

			jNO_DEFAULT;
		}
	}
	else switch(reg)
	{
		case 0x1d80: value = Cores[0].MasterL.Value; break;
		case 0x1d82: value = Cores[0].MasterR.Value; break;
		case 0x1d84: value = Cores[0].FxL;           break;
		case 0x1d86: value = Cores[0].FxR;           break;

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

		case 0x1da2: value = Cores[0].EffectsStartA>>3;   break;
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

void RegWriteLog(u32 core,u16 value);

static void SPU2writeLog(u32 rmem, u16 value) 
{
#ifndef PUBLIC
	u32 vx=0, vc=0, core=0, omem=rmem, mem=rmem&0x7FF;
	omem=mem=mem&0x7FF; //FFFF;
	if (mem & 0x400) { omem^=0x400; core=1; }

	/*
	if ((omem >= 0x0000) && (omem < 0x0180)) { // Voice Params
		u32 voice=(omem & 0x1F0) >> 4;
		u32 param=(omem & 0xF)>>1;
		FileLog("[%10d] SPU2 write mem %08x (Core %d Voice %d Param %s) value %x\n",Cycles,rmem,core,voice,ParamNames[param],value);
	}
	else if ((omem >= 0x01C0) && (omem < 0x02DE)) {
		u32 voice   =((omem-0x01C0) / 12);
		u32 address =((omem-0x01C0) % 12)>>1;
		FileLog("[%10d] SPU2 write mem %08x (Core %d Voice %d Address %s) value %x\n",Cycles,rmem,core,voice,AddressNames[address],value);
	}
	*/
	if ((mem >= 0x0760) && (mem < 0x07b0)) {
		omem=mem; core=0;
		if (mem >= 0x0788) {omem-=0x28; core=1;}
		switch(omem) {
		    case REG_P_EVOLL:	RegLog(2,"EVOLL",rmem,core,value);	break;
			case REG_P_EVOLR:	RegLog(2,"EVOLR",rmem,core,value);	break;
			case REG_P_AVOLL:	if (core) { RegLog(2,"AVOLL",rmem,core,value); }	break;
			case REG_P_AVOLR:	if (core) { RegLog(2,"AVOLR",rmem,core,value); }	break;
			case REG_P_BVOLL:	RegLog(2,"BVOLL",rmem,core,value);	break;
			case REG_P_BVOLR:	RegLog(2,"BVOLR",rmem,core,value);	break;
			case REG_P_MVOLXL:	RegLog(2,"MVOLXL",rmem,core,value);	break;
			case REG_P_MVOLXR:	RegLog(2,"MVOLXR",rmem,core,value);	break;
			case R_IIR_ALPHA:	RegLog(2,"IIR_ALPHA",rmem,core,value);	break;
			case R_ACC_COEF_A:	RegLog(2,"ACC_COEF_A",rmem,core,value);	break;
			case R_ACC_COEF_B:	RegLog(2,"ACC_COEF_B",rmem,core,value);	break;
			case R_ACC_COEF_C:	RegLog(2,"ACC_COEF_C",rmem,core,value);	break;
			case R_ACC_COEF_D:	RegLog(2,"ACC_COEF_D",rmem,core,value);	break;
			case R_IIR_COEF:	RegLog(2,"IIR_COEF",rmem,core,value);	break;
			case R_FB_ALPHA:	RegLog(2,"FB_ALPHA",rmem,core,value);	break;
			case R_FB_X:	  	RegLog(2,"FB_X",rmem,core,value);	break;
			case R_IN_COEF_L:	RegLog(2,"IN_COEF_L",rmem,core,value);	break;
			case R_IN_COEF_R:	RegLog(2,"IN_COEF_R",rmem,core,value);	break;

		}
	}
	else if ((mem>=0x07C0) && (mem<0x07CE)) {
		switch(mem) {
			case SPDIF_OUT:
				RegLog(2,"SPDIF_OUT",rmem,-1,value);
				break;
			case IRQINFO:
				RegLog(2,"IRQINFO",rmem,-1,value);
				break;
			case 0x7c4:
				if(Spdif.Unknown1 != value) ConLog(" * SPU2: SPDIF Unknown Register 1 set to %04x\n",value);
				RegLog(2,"SPDIF_UNKNOWN1",rmem,-1,value);
				break;
			case SPDIF_MODE:
				if(Spdif.Mode != value) ConLog(" * SPU2: SPDIF Mode set to %04x\n",value);
				RegLog(2,"SPDIF_MODE",rmem,-1,value);
				break;
			case SPDIF_MEDIA:
				if(Spdif.Media != value) ConLog(" * SPU2: SPDIF Media set to %04x\n",value);
				RegLog(2,"SPDIF_MEDIA",rmem,-1,value);
				break;
			case 0x7ca:
				if(Spdif.Unknown2 != value) ConLog(" * SPU2: SPDIF Unknown Register 2 set to %04x\n",value);
				RegLog(2,"SPDIF_UNKNOWN2",rmem,-1,value);
				break;
			case SPDIF_COPY:
				if(Spdif.Protection != value) ConLog(" * SPU2: SPDIF Copy set to %04x\n",value);
				RegLog(2,"SPDIF_COPY",rmem,-1,value);
				break;
		}
		UpdateSpdifMode();
	}
	else
		switch(omem) {
		case REG_C_ATTR:
			RegLog(4,"ATTR",rmem,core,value);
			break;
		case REG_S_PMON:
			RegLog(1,"PMON0",rmem,core,value);
			break;
		case (REG_S_PMON + 2):
			RegLog(1,"PMON1",rmem,core,value);
			break;
		case REG_S_NON:
			RegLog(1,"NON0",rmem,core,value);
			break;
		case (REG_S_NON + 2):
			RegLog(1,"NON1",rmem,core,value);
			break;
		case REG_S_VMIXL:
			RegLog(1,"VMIXL0",rmem,core,value);
		case (REG_S_VMIXL + 2):
			RegLog(1,"VMIXL1",rmem,core,value);
			break;
		case REG_S_VMIXEL:
			RegLog(1,"VMIXEL0",rmem,core,value);
			break;
		case (REG_S_VMIXEL + 2):
			RegLog(1,"VMIXEL1",rmem,core,value);
			break;
		case REG_S_VMIXR:
			RegLog(1,"VMIXR0",rmem,core,value);
			break;
		case (REG_S_VMIXR + 2):
			RegLog(1,"VMIXR1",rmem,core,value);
			break;
		case REG_S_VMIXER:
			RegLog(1,"VMIXER0",rmem,core,value);
			break;
		case (REG_S_VMIXER + 2):
			RegLog(1,"VMIXER1",rmem,core,value);
			break;
		case REG_P_MMIX:
			RegLog(1,"MMIX",rmem,core,value);
			break;
		case REG_A_IRQA:
			RegLog(2,"IRQAH",rmem,core,value);
			break;
		case (REG_A_IRQA + 2):
			RegLog(2,"IRQAL",rmem,core,value);
			break;
		case (REG_S_KON + 2):
			RegLog(2,"KON1",rmem,core,value);
			break;
		case REG_S_KON:
			RegLog(2,"KON0",rmem,core,value);
			break;
		case (REG_S_KOFF + 2):
			RegLog(2,"KOFF1",rmem,core,value);
			break;
		case REG_S_KOFF:
			RegLog(2,"KOFF0",rmem,core,value);
			break;
		case REG_A_TSA:
			RegLog(2,"TSAH",rmem,core,value);
			break;
		case (REG_A_TSA + 2):
			RegLog(2,"TSAL",rmem,core,value);
			break;
		case REG_S_ENDX:
			//ConLog(" * SPU2: Core %d ENDX cleared!\n",core);
			RegLog(2,"ENDX0",rmem,core,value);
			break;
		case (REG_S_ENDX + 2):	
			//ConLog(" * SPU2: Core %d ENDX cleared!\n",core);
			RegLog(2,"ENDX1",rmem,core,value);
			break;
		case REG_P_MVOLL:
			RegLog(1,"MVOLL",rmem,core,value);
			break;
		case REG_P_MVOLR:
			RegLog(1,"MVOLR",rmem,core,value);
			break;
		case REG_S_ADMAS:
			RegLog(3,"ADMAS",rmem,core,value);
			ConLog(" * SPU2: Core %d AutoDMAControl set to %d\n",core,value);
			break;
		case REG_P_STATX:
			RegLog(3,"STATX",rmem,core,value);
			break;
		case REG_A_ESA:
			RegLog(1,"ESAH",rmem,core,value);
			break;
		case (REG_A_ESA + 2):
			RegLog(1,"ESAL",rmem,core,value);
			break;
		case REG_A_EEA:
			RegLog(1,"EEAH",rmem,core,value);
			break;

#define LOG_REVB_REG(n,t) \
		case R_##n: \
			RegLog(2,t "H",mem,core,value); \
			break; \
		case (R_##n + 2): \
			RegLog(2,t "L",mem,core,value); \
			break;

	LOG_REVB_REG(FB_SRC_A,"FB_SRC_A")
	LOG_REVB_REG(FB_SRC_B,"FB_SRC_B")
	LOG_REVB_REG(IIR_SRC_A0,"IIR_SRC_A0")
	LOG_REVB_REG(IIR_SRC_A1,"IIR_SRC_A1")
	LOG_REVB_REG(IIR_SRC_B1,"IIR_SRC_B1")
	LOG_REVB_REG(IIR_SRC_B0,"IIR_SRC_B0")
	LOG_REVB_REG(IIR_DEST_A0,"IIR_DEST_A0")
	LOG_REVB_REG(IIR_DEST_A1,"IIR_DEST_A1")
	LOG_REVB_REG(IIR_DEST_B0,"IIR_DEST_B0")
	LOG_REVB_REG(IIR_DEST_B1,"IIR_DEST_B1")
	LOG_REVB_REG(ACC_SRC_A0,"ACC_SRC_A0")
	LOG_REVB_REG(ACC_SRC_A1,"ACC_SRC_A1")
	LOG_REVB_REG(ACC_SRC_B0,"ACC_SRC_B0")
	LOG_REVB_REG(ACC_SRC_B1,"ACC_SRC_B1")
	LOG_REVB_REG(ACC_SRC_C0,"ACC_SRC_C0")
	LOG_REVB_REG(ACC_SRC_C1,"ACC_SRC_C1")
	LOG_REVB_REG(ACC_SRC_D0,"ACC_SRC_D0")
	LOG_REVB_REG(ACC_SRC_D1,"ACC_SRC_D1")
	LOG_REVB_REG(MIX_DEST_A0,"MIX_DEST_A0")
	LOG_REVB_REG(MIX_DEST_A1,"MIX_DEST_A1")
	LOG_REVB_REG(MIX_DEST_B0,"MIX_DEST_B0")
	LOG_REVB_REG(MIX_DEST_B1,"MIX_DEST_B1")

		default:			RegLog(2,"UNKNOWN",rmem,core,value); spu2Ru16(mem) = value;
	}
#endif
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

typedef struct 
{
	// compatibility with zerospu2 removed...

	u32 version;
	u8 unkregs[0x10000];
	u8 mem[0x200000];

	u32 id;
	V_Core Cores[2];
	V_SPDIF Spdif;
	s16 OutPos;
	s16 InputPos;
	u8 InpBuff;
	u32 Cycles;
	s32 uTicks;
	double srate_pv;
	double opitch;
	int osps;
	int PlayMode;

	int lClocks;

	PcmCacheEntry cacheData;

} SPU2freezeData;

// No more ZeroSPU compatibility...
//#define ZEROSPU_VERSION 0x70000001

#define SAVE_ID 0x73326701

// versioning for saves.
// Increment this if changes to V_Core or V_Voice structs are made.
// Chances are we'll never explicitly support older save versions,
// but might as well version them anyway.  Could come in handly someday!
#define SAVE_VERSION 0x0101

static int getFreezeSize()
{
	if( disableFreezes ) return 8;	// length of the string id "invalid" (plus a zero!)

	int size = sizeof(SPU2freezeData);

	// calculate the amount of memory consumed by our cache:

	for( int bidx=0; bidx<pcm_BlockCount; bidx++ )
	{
		if( pcm_cache_data[bidx].Validated )
			size += pcm_DecodedSamplesPerBlock*sizeof(PcmCacheEntry);
	}
	return size;
}


static void wipe_the_cache()
{
	memset( pcm_cache_data, 0, pcm_BlockCount * sizeof(PcmCacheEntry) );
}


static s16 old_state_sBuffer[pcm_DecodedSamplesPerBlock] = {0};

EXPORT_C_(s32) SPU2freeze(int mode, freezeData *data)
{
	if (mode == FREEZE_LOAD)
	{
		const SPU2freezeData *spud = (SPU2freezeData*)data->data;

		if( spud->id != SAVE_ID || spud->version < 0x100 )
		{
			printf("\n*** SPU2Ghz Warning:\n");
			printf("  The savestate you are trying to load was not made with this plugin.\n");
			printf("  The emulator will not be stable!  Find a memorycard savespot to save your\n");
			printf("  game, reset, and then continue from there.\n\n");

			disableFreezes=true;
			lClocks = 0;
			resetClock = true;

			// Do *not* reset the cores.
			// We'll need some "hints" as to how the cores should be initialized,
			// and the only way to get that is to use the game's existing core settings
			// and hope they kinda match the settings for the savestate (IRQ enables and such).
			//

			//CoreReset( 0 );
			//CoreReset( 1 );

			// adpcm cache : Clear all the cache flags and buffers.

			wipe_the_cache();
		}
		else
		{
			disableFreezes=false;

			// base stuff
			memcpy(spu2regs, spud->unkregs, 0x010000);
			memcpy(_spu2mem, spud->mem,     0x200000);

			memcpy(Cores, spud->Cores, sizeof(Cores));
			memcpy(&Spdif, &spud->Spdif, sizeof(Spdif));
			OutPos		= spud->OutPos;
			InputPos	= spud->InputPos;
			InpBuff		= spud->InpBuff;
			Cycles		= spud->Cycles;
			uTicks		= spud->uTicks;
			srate_pv	= spud->srate_pv;
			opitch		= spud->opitch;
			osps		= spud->osps;
			PlayMode	= spud->PlayMode;
			lClocks		= spud->lClocks;

			// Load the ADPCM cache:

			wipe_the_cache();
			if( spud->version == 0x100 )		// don't support 0x100 cache anymore.
			{
				printf("\n*** SPU2Ghz Warning:\n");
				printf("\tSavestate version is from an older version of this plugin.\n");
				printf("\tAudio may not recover correctly.");

				const PcmCacheEntry* pcmSrc = &spud->cacheData;
				int blksLoaded=0;

				for( int bidx=0; bidx<pcm_BlockCount; bidx++ )
				{
					if( pcm_cache_data[bidx].Validated )
					{
						// load a cache block!
						memcpy( &pcm_cache_data[bidx], pcmSrc, sizeof(PcmCacheEntry) );
						pcmSrc++;
						blksLoaded++;
					}
				}

				// Go through the V_Voice structs and recalculate SBuffer pointer from
				// the NextA setting.

				for( int c=0; c<2; c++ )
				{
					for( int v=0; v<24; v++ )
					{
						const int cacheIdx = Cores[c].Voices[v].NextA / pcm_WordsPerBlock;
						Cores[c].Voices[v].SBuffer = pcm_cache_data[cacheIdx].Sampledata;
					}
				}
			}
			else
			{
				// We don't support the cache, so make sure the SBuffer pointers
				// are safe (don't want any GPFs reading bad data)

				for( int c=0; c<2; c++ )
				{
					for( int v=0; v<24; v++ )
						Cores[c].Voices[v].SBuffer = old_state_sBuffer;
				}
			}


			//printf( " * SPU2 > FreezeLoad > Loaded %d cache blocks.\n", blksLoaded++ );
		}

	} else if (mode == FREEZE_SAVE)
	{
		if (data->data == NULL) return -1;

		if( disableFreezes )
		{
			// No point in making a save state since the SPU2
			// state is completely bogus anyway... Let's just
			// give this some random ID that no one will recognize.

			strcpy( data->data, "invalid" );
			return 0;
		}


		SPU2freezeData *spud = (SPU2freezeData*)data->data;

		spud->id=SAVE_ID;
		spud->version=SAVE_VERSION;//ZEROSPU_VERSION; //Zero compat working bad, better not save that

		memcpy(spud->unkregs, spu2regs, 0x010000);
		memcpy(spud->mem,     _spu2mem, 0x200000);
		memcpy(spud->Cores, Cores, sizeof(Cores));
		memcpy(&spud->Spdif, &Spdif, sizeof(Spdif));
		spud->OutPos		= OutPos;
		spud->InputPos		= InputPos;
		spud->InpBuff		= InpBuff;
		spud->Cycles		= Cycles;
		spud->uTicks		= uTicks;
		spud->srate_pv		= srate_pv;
		spud->opitch		= opitch;
		spud->osps			= osps;
		spud->PlayMode		= PlayMode;
		spud->lClocks		= lClocks;

		// Save our cache:
		//   We could just force the user to rebuild the cache when loading
		//   from stavestates, but for most games the cache is pretty
		//   small and compresses well.
		//
		// Potential Alternative:
		//   If the cache is not saved then it is necessary to save the
		//   decoded blocks currently in use by active voices.  This allows
		//   voices to resume seamlessly on load.

		PcmCacheEntry* pcmDst = &spud->cacheData;
		int blksSaved=0;

		for( int bidx=0; bidx<pcm_BlockCount; bidx++ )
		{
			if( pcm_cache_data[bidx].Validated )
			{
				// save a cache block!
				memcpy( pcmDst, &pcm_cache_data[bidx], sizeof(PcmCacheEntry) );
				pcmDst++;
				blksSaved++;
			}
		}

		//printf( " * SPU2 > FreezeSave > Saved %d cache blocks.\n", blksSaved++ );

	}
	else if (mode == FREEZE_SIZE)
	{
		data->size = getFreezeSize();
	}
	return 0;

}

void VoiceStart(int core,int vc)
{
	if((Cycles-Cores[core].Voices[vc].PlayCycle)>=4)
	{
		if(Cores[core].Voices[vc].StartA&7)
		{
			fprintf( stderr, " *** Missaligned StartA %05x!\n",Cores[core].Voices[vc].StartA);
			Cores[core].Voices[vc].StartA=(Cores[core].Voices[vc].StartA+0xFFFF8)+0x8;
		}

		Cores[core].Voices[vc].ADSR.Releasing=0;
		Cores[core].Voices[vc].ADSR.Value=1;
		Cores[core].Voices[vc].ADSR.Phase=1;
		Cores[core].Voices[vc].PlayCycle=Cycles;
		Cores[core].Voices[vc].SCurrent=28;
		Cores[core].Voices[vc].LoopMode=0;
		Cores[core].Voices[vc].LoopFlags=0;
		Cores[core].Voices[vc].LoopStartA=Cores[core].Voices[vc].StartA;
		Cores[core].Voices[vc].NextA=Cores[core].Voices[vc].StartA;
		Cores[core].Voices[vc].Prev1=0;
		Cores[core].Voices[vc].Prev2=0;

		Cores[core].Voices[vc].PV1=Cores[core].Voices[vc].PV2=0;
		Cores[core].Voices[vc].PV3=Cores[core].Voices[vc].PV4=0;

		Cores[core].Regs.ENDX&=~(1<<vc);

		#ifndef PUBLIC
		DebugCores[core].Voices[vc].FirstBlock=1;
		if(core==1)
		{
			if(MsgKeyOnOff()) ConLog(" * SPU2: KeyOn: C%dV%02d: SSA: %8x; M: %s%s%s%s; H: %02x%02x; P: %04x V: %04x/%04x; ADSR: %04x%04x\n",
						core,vc,Cores[core].Voices[vc].StartA,
						(Cores[core].Voices[vc].DryL)?"+":"-",(Cores[core].Voices[vc].DryR)?"+":"-",
						(Cores[core].Voices[vc].WetL)?"+":"-",(Cores[core].Voices[vc].WetR)?"+":"-",
						*(u8*)GetMemPtr(Cores[core].Voices[vc].StartA),*(u8 *)GetMemPtr((Cores[core].Voices[vc].StartA)+1),
						Cores[core].Voices[vc].Pitch,
						Cores[core].Voices[vc].VolumeL.Value,Cores[core].Voices[vc].VolumeR.Value,
						Cores[core].Voices[vc].ADSR.Reg_ADSR1,Cores[core].Voices[vc].ADSR.Reg_ADSR2);
		}
		#endif
	}
	else
	{
		printf(" *** KeyOn after less than 4 T disregarded.\n");
	}
}

void VoiceStop(int core,int vc)
{
	Cores[core].Voices[vc].ADSR.Value=0;
	Cores[core].Voices[vc].ADSR.Phase=0;

	//Cores[core].Regs.ENDX|=(1<<vc);
}

void StartVoices(int core, u32 value)
{
	int vx=1,vc=0;
	for (vc=0;vc<24;vc++) {
		if ((value>>vc) & 1) {
			VoiceStart(core,vc);
		}
	}
	Cores[core].Regs.ENDX &= ~(value);
	//Cores[core].Regs.ENDX = 0;
}

void StopVoices(int core, u32 value)
{
	u32 vx=1,vc=0;
	for (vc=0;vc<24;vc++) {
		if ((value>>vc) & 1) {
			Cores[core].Voices[vc].ADSR.Releasing=1;
			//if(MsgKeyOnOff()) ConLog(" * SPU2: KeyOff: Core %d; Voice %d.\n",core,vc);
		}
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
