/*----------------------------------------------------------*\
 *
 *	Copyright (C) 2011 Avi Halachmi
 *	avihpit (at) yahoo (dot) com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html


    This file compiles into a DLL which implements a generic GSdx CRC hack logic.
     - GSdx will (re)load this DLL at runtime whenever this DLL is modified.
     - See Notes and Usage instructions at the bottom of this file.

    - Compiles using tcc ( http://bellard.org/tcc/ ). Tested: tcc v0.9.25/Win32

    version 1.1 - initial release
    Version 1.2 - better GSdx integration, Bugfix.
    Version 1.4 - Configuration, few utils, Cleanups (dll API is unchanged)
    version 1.5 - Supports CRC passed from GSdx and util IsCRC(crc1 [, crc2, ...])

\*----------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//some common tokens not available in C
typedef int int32;
typedef unsigned int uint32;
typedef int bool;
#define true	(1)
#define false	(0)

//GSdx values available to CRC hacks
typedef struct{	uint32 FBP;	uint32 FPSM; uint32 FBMSK; uint32 TBP0; uint32 TPSM; uint32 TZTST; bool TME;} GSFrameInfo;
enum Region{CRC_NoRegion,CRC_US,CRC_EU,CRC_JP,CRC_JPUNDUB,CRC_RU,CRC_FR,CRC_DE,CRC_IT,CRC_ES,CRC_ASIA,CRC_KO,CRC_RegionCount};
enum GS_PSM{PSM_PSMCT32=0,PSM_PSMCT24=1,PSM_PSMCT16=2,PSM_PSMCT16S=10,PSM_PSMT8=19,PSM_PSMT4=20,PSM_PSMT8H=27,
            PSM_PSMT4HL=36,PSM_PSMT4HH=44,PSM_PSMZ32=48,PSM_PSMZ24=49,PSM_PSMZ16=50,PSM_PSMZ16S=58};

//trickery to make the hack function appear and behave (almost) identical to the CRC hack functions at GSState.cpp
// - see the Notes section for exceptions
#define skip (*pSkip)
#define GSUtil_HasSharedBits(a,b,c,d) sharedBits
#define GSC_AnyGame(a,b) _GSC_AnyGame()
GSFrameInfo fi; int* pSkip; uint32 g_crc_region; uint32 sharedBits; uint32 g_crc;

//utils
const int MODE_3_DELAY = 750;	// ms
void dprintf(const char* format, ...);
void dings(const int n);
bool isCornerTriggered();
bool IsIn(const DWORD val, ...);
// END is a magic number, if we have such CRC - meh
#define END (0x72951413)
#define IsCRC(...) (g_crc!=END && IsIn(g_crc, __VA_ARGS__, END))
// C99 syntax for variadic macro: #define FOO(fmt, ...) printf(fmt, __VA_ARGS__) // empty list not supported
// empty list is possible by suppressing prior comma by using ##__VA_ARGS__


// ---------- Configuration ---------------------------------
//
// Available Modes (can be changed and recompiled while PCSX2 is running):
// 0 - Disable this file (use the pre-compiled GSdx hacks as if this file never existed)
// 1 - Disable all CRC hacks (both this file and the pre-compiled GSdx hacks)
// 2 - * Enable this file (use this file instead of any pre-compiled GSdx hacks)
// 3 - Toggle repeatedly between modes 1 and 2
#define INITIAL_MODE 2

// If MOUSE_TOGGLE is 1 and INITIAL_MODE is NOT 0, you can switch between modes 1/2/3 by moving the mouse
//    pointer to the top-left corner of the screen (while PCSX2 is running). Set to 0 to disable mouse toggle.
// If MOUSE_TOGGLE is 2, it's the same as 1, but also plays beep sounds when switching modes.
#define MOUSE_TOGGLE 0


/************ Dynamic CRC hack code starts here *****************/

bool GSC_AnyGame( const GSFrameInfo& fi, int& skip )
{

//Example: MGS3 CRC hack copied directly from GSState.cpp (see the Notes section exceptions):
if( IsCRC(0x086273D2, 0x26A6E286, 0x9F185CE1) ){ // 3 first MGS3 CRCs from GSCrc.c
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02000 && fi.FPSM == PSM_PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT24)
		{
			skip = 1000; // 76, 79
		}
		else if(fi.TME && fi.FBP == 0x02800 && fi.FPSM == PSM_PSMCT24 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSM_PSMCT32)
		{
			skip = 1000; // 69
		}
	}
	else
	{
		if(!fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01000) && fi.FPSM == PSM_PSMCT32)
		{
			skip = 0;
		}
		else if(!fi.TME && fi.FBP == fi.TBP0 && fi.TBP0 == 0x2000 && fi.FPSM == PSM_PSMCT32 && fi.TPSM == PSM_PSMCT24)
		{

			if(g_crc_region == CRC_US || g_crc_region == CRC_JP || g_crc_region == CRC_KO)
			{
				skip = 119;	//ntsc
			}
			else
			{
				skip = 136;	//pal
			}
		}
	}

	return true;
}

}

/*********** Dynamic CRC hack code ends here *****************/


// Prints to the Debugger's output window or to DebugView ( http://technet.microsoft.com/en-us/sysinternals/bb896647 )
void dprintf( const char* format, ...)
{
	#define _BUFSIZ 2048
	char buffer[_BUFSIZ];
	va_list args;
	va_start( args, format );
	if( 0 > vsnprintf( buffer, _BUFSIZ, format, args ) )
		sprintf( buffer, "%s","<too-long-to-print>\n" );
	OutputDebugString( buffer );
	va_end( args );
};

// Tests if the first argument is equal to any of the other arguments.
// - The last argument must be 'END'.
// - All values MUST be 32 bits (int/uint32/DWORD etc) or else it may crash.
// E.g. IsIn( 6,   2,4,6,8,     END) is true
// E.g. IsIn( 7,   2,4,6,8,10,  END) is false
bool IsIn( const DWORD val, ... )
{
	va_list args;
	va_start( args, val );
	DWORD test, res = 0;
	for ( ; ( test = va_arg( args, DWORD ) ) != END ; )
		if( test == val)
			{res=1; break;}
	va_end( args );
	return res;
}

// Returns true if the mouse pointer is at the top-left corner
// of the screen and it wasn't there on the previous iteration
bool isCornerTriggered()
{
	static POINT last;
	POINT coord;
	GetCursorPos( &coord );
	bool triggered = ( !coord.x && !coord.y && ( last.x || last.y ) );
	last = coord;
	return triggered;
}

DWORD _dingsLeft=0, _nextDing=0;
// Initiate n ding sounds (at 250ms intervals) while aborting any previous sequence.
// Use n=0 to stop an already playing sequence.
void dings(const int n){_dingsLeft=n; _nextDing=0;}
void processDings()
{
	if( _dingsLeft <= 0 )
		return;
	DWORD now = GetTickCount();
	if( now < _nextDing )
		return;
	_nextDing = now + 250;
	_dingsLeft--;
	MessageBeep( 0 );	// Asynchronous
}

// Executed every iteration of the DLL hack invocation, takes care of mode and sounds
bool preProcess_isAbort()
{
	static int mode = INITIAL_MODE;

	if( MOUSE_TOGGLE && isCornerTriggered() ){
		mode = 1 + mode%3;
		if( MOUSE_TOGGLE == 2 )
			dings( mode );
		dprintf( "Hack Mode: %s\n", mode==1?"Off":(mode==2?"On":"Toggle") );
	}
	processDings();

	if( mode == 1 ) return true;
	if( mode == 2 ) return false;
	return ( GetTickCount() / MODE_3_DELAY) %2;
}


DWORD WINAPI thread_PrintStats( LPVOID lpParam );
typedef struct _stats {	uint32 overall, changed, skipped, nextPrint;} Stats;

#define DLL_EXPORT __declspec(dllexport)

#define CRC_HACK     DynamicCrcHack2
#define CRC_HACK_OLD DynamicCrcHack
#if INITIAL_MODE == 0
	#define CRC_HACK Voldemort
	#define CRC_HACK_OLD Voldemort
#endif
DLL_EXPORT bool CRC_HACK (uint32 FBP, uint32 FPSM, uint32 FBMSK, uint32 TBP0, uint32 TPSM, uint32 TZTST,
                          uint32 TME, int* _pSkip, uint32 _g_crc_region, uint32 _sharedBits, uint32 _crc)
{

	static Stats stat={overall:0, changed:0, skipped:0, nextPrint:0};

	DWORD now=GetTickCount();
	if(stat.nextPrint <= now){
		dprintf("DH: Overall: %5d, skipped: %5d, actions:%5d\n", stat.overall, stat.skipped, stat.changed);
		stat.overall=stat.changed=stat.skipped=0;
		stat.nextPrint=now+1000;
	}
	
	stat.overall++;
	
	if(preProcess_isAbort())	// Process dings if required
		return true;			// Abort hack depending on mode

	fi.FBP=FBP; fi.FPSM=FPSM; fi.FBMSK=FBMSK; fi.TBP0=TBP0; fi.TPSM=TPSM; fi.TZTST=TZTST; fi.TME=TME;
	pSkip=_pSkip; g_crc_region=_g_crc_region; sharedBits=_sharedBits; g_crc = _crc;

	int pre=skip;
	bool res=_GSC_AnyGame();
	int post=skip;

	if(skip) stat.skipped++;
	
	if(post!=pre) stat.changed++;
	
	return res;
		
}

DLL_EXPORT bool CRC_HACK_OLD (uint32 FBP, uint32 FPSM, uint32 FBMSK, uint32 TBP0, uint32 TPSM, uint32 TZTST,
                          uint32 TME, int* _pSkip, uint32 _g_crc_region, uint32 _sharedBits)
{
	return CRC_HACK(FBP, FPSM, FBMSK, TBP0, TPSM, TZTST, TME,_pSkip, _g_crc_region,_sharedBits, END);
}

char* v[]={
	"DLL_PROCESS_DETACH",
	"DLL_PROCESS_ATTACH",
	"DLL_THREAD_ATTACH",
	"DLL_THREAD_DETACH"
};
BOOL WINAPI DllMain (HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason<4)
		dprintf("DllMain: %s\n", v[dwReason]);
	else
		dprintf("DllMain: %d\n", dwReason);
	return TRUE;
}
/*-------------------- Notes- -------------------------------*\

    1. If required, Use CRC_<region> instead of CRC::<region> (e.g. CRC_US instead of CRC::US)
       - g_crc_region will only be valid if your CRC is already associated with a hack function at GSdx
         AND it's not disabled (via CrcHacksExclusions).

    2. If required, Use GSUtil_HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM) instead of
         GSUtil::HasSharedBits(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)
       - Note: GSUtil_HasSharedBits supports only these arguments: (fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)

    3. When copying the code back to GSState.cpp, remember to restore CRC::.. and GSUtil::...

    4. GSdx v5215 onwards also sends the CRC of the game (even if it's not defined at GSdx).
       You can test the CRC using IsCRC(0x12345678) or, for few CRCs: IsCRC(0x12345678, 0x87654321, ...)
       NOTE: with GSdx v5214 and before: IsCRC(...) always returns false.

\*----------------------------------------------------------*/

/* --------------- Usage instructions --------------------------------------------*\

1. Download tcc (Tiny C Compiler) from http://bellard.org/tcc/ (Tested with v0.9.25 for Win32)
   and extract it fully to <this-folder>\tcc
   such that tcc.exe is at <this-folder>\tcc\tcc.exe

2. Compile GSdx with dynamic crc hacks support by uncommenting this line at GSState.cpp:
//#define USE_DYNAMIC_CRC_HACK

3. Configure GSdx to use this DLL by setting DYNA_DLL_PATH (The DLL is created at the same folder as this C file)
   Yes, it's not configurable via ini.

4. Run PCSX2 with a game you want to develop a CRC hack for. If the DLL exists and not disabled, you'll see
   a message at the PCSX2 console: "GSdx: Dynamic CRC-hacks: Loaded OK    (-> overriding internal hacks)"
   If the DLL doesn't exist (yet): "GSdx: Dynamic CRC-hacks: Not available    (-> using internal hacks)"
   - Note: Whenever this DLL is created or changed, GSdx would automatically reload it at runtime, and display a message.

5. Run the batch file Auto_Compile.bat
   This batch file will keep a window open and automatically compile this C file into the DLL whenever this C file
   is changes and saved. The window will display a message if the compile succeeded or failed, and will and play some beeps.

6. Open this C file with a programmer's editor of your choice, change the configuration and/or modify the contents
   of the function GSC_AnyGame. Save this file and watch the GSdx window for the effect it has. Repeat till you're happy.
   Once you're happy with the hack, copy GSC_AnyGame function to GSState.cpp to be an integral part of GSdx (see Notes).

\*-------------------------------------------------------------------------------*/