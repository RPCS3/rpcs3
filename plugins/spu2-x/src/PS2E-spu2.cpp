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
#include "PS2E-spu2.h"
#include "Dma.h"
#include "Dialogs.h"

#ifndef __LINUX__
#include "x86emitter/tools.h"
#endif

#ifdef _MSC_VER
#	include "svnrev.h"
#endif

// PCSX2 expects ASNI, not unicode, so this MUST always be char...
static char libraryName[256];


static bool IsOpened		= false;
static bool IsInitialized	= false;

static u32  pClocks = 0;

u32*	cyclePtr	= NULL;
u32		lClocks		= 0;

#ifdef _MSC_VER
HINSTANCE hInstance;
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	if( dwReason == DLL_PROCESS_ATTACH )
	{
		hInstance = hinstDLL;
	}	
	else if( dwReason == DLL_PROCESS_DETACH )
	{
		// TODO : perform shutdown procedure, just in case PCSX2 itself failed
		// to for some reason..
	}
	return TRUE;
}
#endif

static void InitLibraryName()
{
#ifdef SPU2X_PUBLIC_RELEASE

	// Public Release!
	// Output a simplified string that's just our name:

	strcpy( libraryName, "SPU2-X" );

#else
	#ifdef SVN_REV_UNKNOWN

	// Unknown revision.
	// Output a name that includes devbuild status but not
	// subversion revision tags:

	strcpy( libraryName, "SPU2-X"
	#ifdef DEBUG_FAST
		"-Debug"
	#elif defined( PCSX2_DEBUG )
		"-Debug/Strict"		// strict debugging is slow!
	#elif defined( PCSX2_DEVBUILD )
		"-Dev"
	#else
		""
	#endif
	);

	#else

	// Use TortoiseSVN's SubWCRev utility's output
	// to label the specific revision:

	sprintf_s( libraryName, "SPU2-X r%d%s"
	#ifdef DEBUG_FAST
		"-Debug"
	#elif defined( PCSX2_DEBUG )
		"-Debug/Strict"		// strict debugging is slow!
	#elif defined( PCSX2_DEVBUILD )
		"-Dev"
	#else
		""
	#endif
		,SVN_REV,
		SVN_MODS ? "m" : ""
	);
	#endif
#endif

}

static bool cpu_detected = false;

static bool CheckSSE()
{
// I'll worry about checking sse2 in Linux later.
#ifndef __LINUX__
	if( !cpu_detected )
	{
		cpudetectInit();
		cpu_detected = true;
	}

	if( !x86caps.hasStreamingSIMDExtensions || !x86caps.hasStreamingSIMD2Extensions )
	{
		SysMessage( "Your CPU does not support SSE2 instructions.\nThe SPU2-X plugin requires SSE2 to run." );
		return false;
	}
#endif
	return true;
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
	return (PS2E_SPU2_VERSION<<16) | (VersionInfo::Release<<8) | VersionInfo::Revision;
}

EXPORT_C_(void) SPU2configure()
{
	if( !CheckSSE() ) return;
	configure();
}

EXPORT_C_(void) SPU2about()
{
	AboutBox();
}

EXPORT_C_(s32) SPU2test()
{
	if( !CheckSSE() ) return -1;

	ReadSettings();
	if( SndBuffer::Test() != 0 )
	{
		// TODO : Implement a proper dialog that allows the user to test different audio out drivers.
		const wchar_t* wtf = mods[OutputModule]->GetIdent();
		SysMessage( L"The '%s' driver test failed.  Please configure\na different SoundOut module and try again.", wtf );
		return -1;
	}

	return 0;
}

// --------------------------------------------------------------------------------------
//  DMA 4/7 Callbacks from Core Emulator
// --------------------------------------------------------------------------------------

u16* DMABaseAddr;
void (* _irqcallback)();
void (* dma4callback)();
void (* dma7callback)();

EXPORT_C_(u32) CALLBACK SPU2ReadMemAddr(int core)
{
	return Cores[core].MADR;
}
EXPORT_C_(void) CALLBACK SPU2WriteMemAddr(int core,u32 value)
{
	Cores[core].MADR = value;
}

EXPORT_C_(void) CALLBACK SPU2setDMABaseAddr(uptr baseaddr)
{
   DMABaseAddr = (u16*)baseaddr;
}

EXPORT_C_(void) CALLBACK SPU2setSettingsDir(const char* dir)
{
	CfgSetSettingsDir( dir );
}

EXPORT_C_(s32)  SPU2dmaRead(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	if(channel==4)
		return Cores[0].NewDmaRead(data,bytesLeft, bytesProcessed);
	else
		return Cores[1].NewDmaRead(data,bytesLeft, bytesProcessed);
}

EXPORT_C_(s32)  SPU2dmaWrite(s32 channel, u32* data, u32 bytesLeft, u32* bytesProcessed)
{
	if(channel==4)
		return Cores[0].NewDmaWrite(data,bytesLeft, bytesProcessed);
	else
		return Cores[1].NewDmaWrite(data,bytesLeft, bytesProcessed);
}

EXPORT_C_(void) SPU2dmaInterrupt(s32 channel)
{
	if(channel==4)
		return Cores[0].NewDmaInterrupt();
	else
		return Cores[1].NewDmaInterrupt();
}

#ifdef ENABLE_NEW_IOPDMA_SPU2
EXPORT_C_(void) SPU2irqCallback(void (*SPU2callback)())
{
	_irqcallback = SPU2callback;
}
#else
EXPORT_C_(void) SPU2irqCallback(void (*SPU2callback)(),void (*DMA4callback)(),void (*DMA7callback)())
{
	_irqcallback = SPU2callback;
	dma4callback = DMA4callback;
	dma7callback = DMA7callback;
}
#endif

EXPORT_C_(void) CALLBACK SPU2readDMA4Mem(u16 *pMem, u32 size)	// size now in 16bit units
{
	if( cyclePtr != NULL ) TimeUpdate( *cyclePtr );

	FileLog("[%10d] SPU2 readDMA4Mem size %x\n",Cycles, size<<1);
	Cores[0].DoDMAread(pMem, size);
}

EXPORT_C_(void) CALLBACK SPU2writeDMA4Mem(u16* pMem, u32 size)	// size now in 16bit units
{
	if( cyclePtr != NULL ) TimeUpdate( *cyclePtr );

	FileLog("[%10d] SPU2 writeDMA4Mem size %x at address %x\n",Cycles, size<<1, Cores[0].TSA);
#ifdef S2R_ENABLE
	if(!replay_mode)
		s2r_writedma4(Cycles,pMem,size);
#endif
	Cores[0].DoDMAwrite(pMem,size);
}

EXPORT_C_(void) CALLBACK SPU2interruptDMA4()
{
	FileLog("[%10d] SPU2 interruptDMA4\n",Cycles);
	Cores[0].Regs.STATX |= 0x80;
	//Cores[0].Regs.ATTR &= ~0x30;
}

EXPORT_C_(void) CALLBACK SPU2interruptDMA7()
{
	FileLog("[%10d] SPU2 interruptDMA7\n",Cycles);
	Cores[1].Regs.STATX |= 0x80;
	//Cores[1].Regs.ATTR &= ~0x30;
}

EXPORT_C_(void) CALLBACK SPU2readDMA7Mem(u16* pMem, u32 size)
{
	if( cyclePtr != NULL ) TimeUpdate( *cyclePtr );

	FileLog("[%10d] SPU2 readDMA7Mem size %x\n",Cycles, size<<1);
	Cores[1].DoDMAread(pMem,size);
}

EXPORT_C_(void) CALLBACK SPU2writeDMA7Mem(u16* pMem, u32 size)
{
	if( cyclePtr != NULL ) TimeUpdate( *cyclePtr );

	FileLog("[%10d] SPU2 writeDMA7Mem size %x at address %x\n",Cycles, size<<1, Cores[1].TSA);
#ifdef S2R_ENABLE
	if(!replay_mode)
		s2r_writedma7(Cycles,pMem,size);
#endif
	Cores[1].DoDMAwrite(pMem,size);
}

EXPORT_C_(s32) SPU2init() 
{
	assert( regtable[0x400] == NULL );

	s32 c=0,v=0;
	ReadSettings();

#ifdef SPU2_LOG
	if(AccessLog()) 
	{
		spu2Log = fopen( Unicode::Convert( AccessLogFileName ).c_str(), "w" );
		setvbuf(spu2Log, NULL,  _IONBF, 0);
		FileLog("SPU2init\n");
	}
#endif
	srand((unsigned)time(NULL));

	if (IsInitialized)
	{
		ConLog( " * SPU2: Already initialized - Ignoring SPU2init signal." );
		return 0;
	}

	IsInitialized = true;

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

	memset(spu2regs, 0, 0x010000);
	memset(_spu2mem, 0, 0x200000);
	Cores[0].Reset(0);
	Cores[1].Reset(1);

	DMALogOpen();

	/*for(v=0;v<16384;v++)
	{
		logvolume[v]=(s32)(s32)floor(log((double)(v+1))*3376.7);
	}*/

	// Initializes lowpass filter for reverb in mixer.cpp
	//LowPassFilterInit();
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

uptr gsWindowHandle = 0;

EXPORT_C_(s32) SPU2open(void *pDsp)
{
	if( IsOpened ) return 0;

	FileLog("[%10d] SPU2 Open\n",Cycles);
	
	if( pDsp != NULL )
		gsWindowHandle = *(uptr*)pDsp;
	else
		gsWindowHandle = 0;

	/*
	if(debugDialogOpen==0)
	{
	hDebugDialog = CreateDialogParam(hInstance,MAKEINTRESOURCE(IDD_DEBUG),0,DebugProc,0);
	ShowWindow(hDebugDialog,SW_SHOWNORMAL);
	debugDialogOpen=1;
	}*/

	IsOpened = true;
	lClocks  = (cyclePtr!=NULL) ? *cyclePtr : 0;

	try
	{
		SndBuffer::Init();
		spdif_init();
#ifndef __LINUX__
		DspLoadLibrary(dspPlugin,dspPluginModule);
#endif
		WaveDump::Open();
	}
	catch( std::exception& ex )
	{
		fprintf( stderr, "SPU2-X Error: Could not initialize device, or something.\nReason: %s", ex.what() );
		SPU2close();
		return -1;
	}
	return 0;
}

EXPORT_C_(void) SPU2close() 
{
	if( !IsOpened ) return;
	IsOpened = false;

	FileLog("[%10d] SPU2 Close\n",Cycles);

#ifndef __LINUX__
	DspCloseLibrary();
#endif
	spdif_shutdown();
	SndBuffer::Cleanup();
}

EXPORT_C_(void) SPU2shutdown() 
{
	if(!IsInitialized) return;
	IsInitialized = false;

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
	WaveDump::Close();

	DMALogClose();

	safe_free(spu2regs);
	safe_free(_spu2mem);
	safe_free( pcm_cache_data );

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
	cyclePtr = ptr;
}

bool numpad_plus = false, numpad_plus_old = false;

EXPORT_C_(void) SPU2async(u32 cycles) 
{
	DspUpdate();

	if(cyclePtr != NULL)
	{
		TimeUpdate( *cyclePtr );
	}
	else
	{
		pClocks += cycles;
		TimeUpdate( pClocks );
	}
}

EXPORT_C_(u16) SPU2read(u32 rmem) 
{
	//	if(!replay_mode)
	//		s2r_readreg(Cycles,rmem);

	u16 ret=0xDEAD; u32 core=0, mem=rmem&0xFFFF, omem=mem;
	if (mem & 0x400) { omem^=0x400; core=1; }

	if(rmem==0x1f9001AC)
	{
		ret = Cores[core].DmaRead();
	}
	else
	{
		if( cyclePtr != NULL )
			TimeUpdate( *cyclePtr );

		if (rmem>>16 == 0x1f80)
		{
			ret = Cores[0].ReadRegPS1(rmem);
		}
		else if( (mem&0xFFFF) >= 0x800 )
		{
			ret = spu2Ru16(mem);
			ConLog(" * SPU2: Read from reg>=0x800: %x value %x\n",mem,ret);
		}
		else 
		{
			ret = *(regtable[(mem>>1)]);
			//FileLog("[%10d] SPU2 read mem %x (core %d, register %x): %x\n",Cycles, mem, core, (omem & 0x7ff), ret);
			SPU2writeLog( "read", rmem, ret );
		}
	}
	
	return ret;
}

EXPORT_C_(void) SPU2write(u32 rmem, u16 value) 
{
#ifdef S2R_ENABLE
	if(!replay_mode)
		s2r_writereg(Cycles,rmem,value);
#endif

	// Note: Reverb/Effects are very sensitive to having precise update timings.
	// If the SPU2 isn't in in sync with the IOP, samples can end up playing at rather
	// incorrect pitches and loop lengths.

	if( cyclePtr != NULL )
		TimeUpdate( *cyclePtr );

	if (rmem>>16 == 0x1f80)
		Cores[0].WriteRegPS1(rmem,value);
	else
	{
		SPU2writeLog( "write", rmem, value );
		SPU2_FastWrite( rmem, value );
	}
}

// if start is 1, starts recording spu2 data, else stops
// returns a non zero value if successful
// for now, pData is not used
EXPORT_C_(int) SPU2setupRecording(int start, void* pData)
{
	if(start==0)
		RecordStop();
	else if(start==1)
		RecordStart();

	return 0;
}

EXPORT_C_(s32) SPU2freeze(int mode, freezeData *data)
{
	if( mode == FREEZE_SIZE )
	{
		data->size = Savestate::SizeIt();
		return 0;
	}

	jASSUME( mode == FREEZE_LOAD || mode == FREEZE_SAVE );
	jASSUME( data != NULL );

	if( data->data == NULL ) return -1;
	Savestate::DataBlock& spud = (Savestate::DataBlock&)*(data->data);

	switch( mode )
	{
		case FREEZE_LOAD: return Savestate::ThawIt( spud );
		case FREEZE_SAVE: return Savestate::FreezeIt( spud );
		
		jNO_DEFAULT;
	}
	
	// technically unreachable, but kills a warning:
	return 0;
}
