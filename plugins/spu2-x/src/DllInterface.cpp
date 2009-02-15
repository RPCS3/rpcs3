/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "spu2.h"
#include "regtable.h"
#include "dialogs.h"

#include "svnrev.h"

// [Air]: Adding the spu2init boolean wasn't necessary except to help me in
//   debugging the spu2 suspend/resume behavior (when user hits escape).
static bool spu2open=false;	// has spu2open plugin interface been called?
static bool spu2init=false;	// has spu2init plugin interface been called?

//static s32 logvolume[16384];
static u32  pClocks=0;

// Pcsx2 expects ASNI, not unicode, so this MUST always be char...
static char libraryName[256];

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD dwReason,LPVOID lpvReserved)
{
	if( dwReason == DLL_PROCESS_ATTACH )
		hInstance = hinstDLL;
	return TRUE;
}

static void InitLibraryName()
{
#ifdef PUBLIC

	// Public Release!
	// Output a simplified string that's just our name:

	strcpy( libraryName, "SPU2-X" );

#elif defined( SVN_REV_UNKNOWN )

	// Unknown revision.
	// Output a name that includes devbuild status but not
	// subversion revision tags:

	strcpy( libraryName, "SPU2-X"
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

	sprintf_s( libraryName, "SPU2-X r%d%s"
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
	return (VersionInfo::PluginApi<<16) | (VersionInfo::Release<<8) | VersionInfo::Revision;
}

EXPORT_C_(void) SPU2configure()
{
	configure();
}

EXPORT_C_(void) SPU2about()
{
	//InitLibraryName();
	//SysMessage( libraryName );
	AboutBox();
}

EXPORT_C_(s32) SPU2test()
{
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
		spu2Log = _wfopen( AccessLogFileName, _T("w") );
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
	Cores[0].Reset();
	Cores[1].Reset();

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
		spdif_init();

		DspLoadLibrary(dspPlugin,dspPluginModule);
		
		WaveDump::Open();

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
	WaveDump::Close();

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
		RecordStop();
	else if(start==1)
		RecordStart();

	return 0;
}
