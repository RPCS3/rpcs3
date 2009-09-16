/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"

#include <list>

#include "Common.h"
#include "GS.h"
#include "iR5900.h"
#include "Counters.h"
#include "VifDma.h"

using namespace Threading;
using namespace std;
using namespace R5900;

u32 CSRw;

PCSX2_ALIGNED16( u8 g_RealGSMem[0x2000] );
extern int m_nCounters[];

// FrameSkipping Stuff
// Yuck, iSlowStart is needed by the MTGS, so can't make it static yet.

u64 m_iSlowStart=0;
static s64 m_iSlowTicks=0;
static bool m_justSkipped = false;
static bool m_StrictSkipping = false;

void _gs_ChangeTimings( u32 framerate, u32 iTicks )
{
	m_iSlowStart = GetCPUTicks();

	u32 frameSkipThreshold = EmuConfig.Video.FpsSkip*50;
	if( frameSkipThreshold == 0)
	{
		// default: load the frameSkipThreshold with a value roughly 90% of the PS2 native framerate
		frameSkipThreshold = ( framerate * 242 ) / 256;
	}

	m_iSlowTicks = ( GetTickFrequency() * 50 ) / frameSkipThreshold;

	// sanity check against users who set a "minimum" frame that's higher
	// than the maximum framerate.  Also, if framerates are within 1/3300th
	// of a second of each other, assume strict skipping (it's too close,
	// and could cause excessive skipping).

	if( m_iSlowTicks <= (iTicks + ((s64)GetTickFrequency()/3300)) )
	{
		m_iSlowTicks = iTicks;
		m_StrictSkipping = true;
	}
}

void gsOnModeChanged( u32 framerate, u32 newTickrate )
{
	mtgsThread->SendSimplePacket( GS_RINGTYPE_MODECHANGE, framerate, newTickrate, 0 );
}

static bool		gsIsInterlaced	= false;
GS_RegionMode	gsRegionMode	= Region_NTSC;


void gsSetRegionMode( GS_RegionMode region )
{
	if( gsRegionMode == region ) return;

	gsRegionMode = region;
	Console::WriteLn( "%s Display Mode Initialized.", (( gsRegionMode == Region_PAL ) ? "PAL" : "NTSC") );
	UpdateVSyncRate();
}


// Make sure framelimiter options are in sync with the plugin's capabilities.
void gsInit()
{
	if( EmuConfig.Video.EnableFrameSkipping && (GSsetFrameSkip == NULL) )
	{
		EmuConfig.Video.EnableFrameSkipping = false;
		Console::WriteLn("Notice: Disabling frameskipping -- GS plugin does not support it.");
	}
}

void gsReset()
{
	// Sanity check in case the plugin hasn't been initialized...
	if( mtgsThread == NULL ) return;
	mtgsThread->Reset();

	gsOnModeChanged(
		(gsRegionMode == Region_NTSC) ? FRAMERATE_NTSC : FRAMERATE_PAL,
		UpdateVSyncRate()
	);

	memzero_obj(g_RealGSMem);

	GSCSRr = 0x551B4000;   // Set the FINISH bit to 1 for now
	GSIMR = 0x7f00;
	psHu32(GIF_STAT) = 0;
	psHu32(GIF_CTRL) = 0;
	psHu32(GIF_MODE) = 0;
}

bool gsGIFSoftReset( int mask )
{
	if( GSgifSoftReset == NULL )
	{
		static bool warned = false;
		if( !warned )
		{
			Console::Notice( "GIF Warning > Soft reset requested, but the GS plugin doesn't support it!" );
			//warned = true;
		}
		return false;
	}

	mtgsThread->GIFSoftReset( mask );

	return true;
}

void gsGIFReset()
{
	// fixme - should this be here? (air)
	//memzero_obj(g_RealGSMem);
	// none of this should be here, its a GIF reset, not GS, only the dma side of it is reset. (Refraction)

	// perform a soft reset (but do not do a full reset if the soft reset API is unavailable)
	//gsGIFSoftReset( 7 );


	//GSCSRr = 0x551B4000;   // Set the FINISH bit to 1 for now
	//GSIMR = 0x7f00;
	psHu32(GIF_STAT) = 0;
	psHu32(GIF_CTRL) = 0;
	psHu32(GIF_MODE) = 0;
}

void gsCSRwrite(u32 value)
{
	

	// Our emulated GS has no FIFO...
	/*if( value & 0x100 ) { // FLUSH
		//Console::WriteLn("GS_CSR FLUSH GS fifo: %x (CSRr=%x)", value, GSCSRr);
	}*/

	if (value & 0x200) { // resetGS

		// perform a soft reset -- and fall back to doing a full reset if the plugin doesn't
		// support soft resets.

		if( !gsGIFSoftReset( 7 ) )
			mtgsThread->SendSimplePacket( GS_RINGTYPE_RESET, 0, 0, 0 );

		CSRw |= 0x1f;
		GSCSRr = 0x551B4000;   // Set the FINISH bit to 1 - GS is always at a finish state as we don't have a FIFO(saqib)
		GSIMR = 0x7F00; //This is bits 14-8 thats all that should be 1
	} 
	else if( value & 0x100 ) // FLUSH
	{ 
		//Console::WriteLn("GS_CSR FLUSH GS fifo: %x (CSRr=%x)", value, GSCSRr);
	}
	else
	{
		CSRw |= value & 0x1f;
		mtgsThread->SendSimplePacket( GS_RINGTYPE_WRITECSR, CSRw, 0, 0 );
		GSCSRr = ((GSCSRr&~value)&0x1f)|(GSCSRr&~0x1f);
	}

}

static void IMRwrite(u32 value)
{
	GSIMR = (value & 0x1f00)|0x6000;

	if((GSCSRr & 0x1f) & (~(GSIMR >> 8) & 0x1f)) 
	{
		gsIrq();
	}
	// don't update mtgs mem
}

__forceinline void gsWrite8(u32 mem, u8 value)
{
	switch (mem)
	{
		case 0x12001000: // GS_CSR
			gsCSRwrite((CSRw & ~0x000000ff) | value); break;
		case 0x12001001: // GS_CSR
			gsCSRwrite((CSRw & ~0x0000ff00) | (value <<  8)); break;
		case 0x12001002: // GS_CSR
			gsCSRwrite((CSRw & ~0x00ff0000) | (value << 16)); break;
		case 0x12001003: // GS_CSR
			gsCSRwrite((CSRw & ~0xff000000) | (value << 24)); break;
		default:
			*PS2GS_BASE(mem) = value;
			mtgsThread->SendSimplePacket(GS_RINGTYPE_MEMWRITE8, mem&0x13ff, value, 0);
	}
	GIF_LOG("GS write 8 at %8.8lx with data %8.8lx", mem, value);
}

__forceinline void _gsSMODEwrite( u32 mem, u32 value )
{
	switch (mem)
	{
		case GS_SMODE1:
			gsSetRegionMode( ((value & 0x6000) == 0x6000) ? Region_PAL : Region_NTSC );
		break;

		case GS_SMODE2:
			gsIsInterlaced = (value & 0x1);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
// GS Write 16 bit

__forceinline void gsWrite16(u32 mem, u16 value)
{
	GIF_LOG("GS write 16 at %8.8lx with data %8.8lx", mem, value);

	_gsSMODEwrite( mem, value );

	switch (mem)
	{
		case GS_CSR:
			gsCSRwrite( (CSRw&0xffff0000) | value);
		return; // do not write to MTGS memory

		case GS_CSR+2:
			gsCSRwrite( (CSRw&0xffff) | ((u32)value<<16));
		return; // do not write to MTGS memory

		case GS_IMR:
			IMRwrite(value);
		return; // do not write to MTGS memory
	}

	*(u16*)PS2GS_BASE(mem) = value;
	mtgsThread->SendSimplePacket(GS_RINGTYPE_MEMWRITE16, mem&0x13ff, value, 0);
}

//////////////////////////////////////////////////////////////////////////
// GS Write 32 bit

__forceinline void gsWrite32(u32 mem, u32 value)
{
	jASSUME( (mem & 3) == 0 );
	GIF_LOG("GS write 32 at %8.8lx with data %8.8lx", mem, value);

	_gsSMODEwrite( mem, value );

	switch (mem)
	{
		case GS_CSR:
			gsCSRwrite(value);
		return;

		case GS_IMR:
			IMRwrite(value);
		return;
	}

	*(u32*)PS2GS_BASE(mem) = value;
	mtgsThread->SendSimplePacket(GS_RINGTYPE_MEMWRITE32, mem&0x13ff, value, 0);
}

//////////////////////////////////////////////////////////////////////////
// GS Write 64 bit

void __fastcall gsWrite64_page_00( u32 mem, const mem64_t* value )
{
	gsWrite64_generic( mem, value );
	_gsSMODEwrite( mem, (u32)value[0] );
}

void __fastcall gsWrite64_page_01( u32 mem, const mem64_t* value )
{
	GIF_LOG("GS Write64 at %8.8lx with data %8.8x_%8.8x", mem, (u32*)value[1], (u32*)value[0]);

	switch( mem )
	{
		case GS_CSR:
			gsCSRwrite((u32)value[0]);
		return;

		case GS_IMR:
			IMRwrite((u32)value[0]);
		return;
	}

	gsWrite64_generic( mem, value );
}

void __fastcall gsWrite64_generic( u32 mem, const mem64_t* value )
{
	const u32* const srcval32 = (u32*)value;
	GIF_LOG("GS Write64 at %8.8lx with data %8.8x_%8.8x", mem, srcval32[1], srcval32[0]);

	*(u64*)PS2GS_BASE(mem) = *value;
	mtgsThread->SendSimplePacket(GS_RINGTYPE_MEMWRITE64, mem&0x13ff, srcval32[0], srcval32[1]);
}

//////////////////////////////////////////////////////////////////////////
// GS Write 128 bit

void __fastcall gsWrite128_page_00( u32 mem, const mem128_t* value )
{
	gsWrite128_generic( mem, value );
	_gsSMODEwrite( mem, (u32)value[0] );
}

void __fastcall gsWrite128_page_01( u32 mem, const mem128_t* value )
{
	switch( mem )
	{
		case GS_CSR:
			gsCSRwrite((u32)value[0]);
		return;

		case GS_IMR:
			IMRwrite((u32)value[0]);
		return;
	}

	gsWrite128_generic( mem, value );
}

void __fastcall gsWrite128_generic( u32 mem, const mem128_t* value )
{
	const u32* const srcval32 = (u32*)value;

	GIF_LOG("GS Write128 at %8.8lx with data %8.8x_%8.8x_%8.8x_%8.8x", mem,
		srcval32[3], srcval32[2], srcval32[1], srcval32[0]);

	const uint masked_mem = mem & 0x13ff;
	u64* writeTo = (u64*)(&g_RealGSMem[masked_mem]);

	writeTo[0] = value[0];
	writeTo[1] = value[1];

	mtgsThread->SendSimplePacket(GS_RINGTYPE_MEMWRITE64, masked_mem, srcval32[0], srcval32[1]);
	mtgsThread->SendSimplePacket(GS_RINGTYPE_MEMWRITE64, masked_mem+8, srcval32[2], srcval32[3]);
}

__forceinline u8 gsRead8(u32 mem)
{
	GIF_LOG("GS read 8 from %8.8lx  value: %8.8lx", mem, *(u8*)PS2GS_BASE(mem));
	return *(u8*)PS2GS_BASE(mem);
}

__forceinline u16 gsRead16(u32 mem)
{
	GIF_LOG("GS read 16 from %8.8lx  value: %8.8lx", mem, *(u16*)PS2GS_BASE(mem));
	return *(u16*)PS2GS_BASE(mem);
}

__forceinline u32 gsRead32(u32 mem)
{
	GIF_LOG("GS read 32 from %8.8lx  value: %8.8lx", mem, *(u32*)PS2GS_BASE(mem));
	return *(u32*)PS2GS_BASE(mem);
}

__forceinline u64 gsRead64(u32 mem)
{
	// fixme - PS2GS_BASE(mem+4) = (g_RealGSMem+(mem + 4 & 0x13ff))
	GIF_LOG("GS read 64 from %8.8lx  value: %8.8lx_%8.8lx", mem, *(u32*)PS2GS_BASE(mem+4), *(u32*)PS2GS_BASE(mem) );
	return *(u64*)PS2GS_BASE(mem);
}

void gsIrq() {
	hwIntcIrq(INTC_GS);
}

void gsSyncLimiterLostTime( s32 deltaTime )
{
	// This sync issue applies only to configs that are trying to maintain
	// a perfect "specific" framerate (where both min and max fps are the same)
	// any other config will eventually equalize out.

	if( !m_StrictSkipping ) return;

	//Console::WriteLn("LostTime on the EE!");

	mtgsThread->SendSimplePacket(
		GS_RINGTYPE_STARTTIME,
		deltaTime,
		0,
		0
	);
}

/////////////////////////////////////////////////////////////////////////////////////////
// FrameSkipper - Measures delta time between calls and issues frameskips
// it the time is too long.  Also regulates the status of the EE's framelimiter.

// This function does not regulate frame limiting, meaning it does no stalling.
// Stalling functions are performed by the EE: If the MTGS were throtted and not
// the EE, the EE would fill the ringbuffer while the MTGS regulated frames --
// fine for most situations but could result in literally dozens of frames queued
// up in the ringbuffer durimg some game menu screens; which in turn would result
// in a half-second lag of keystroke actions becoming visible to the user (bad!).

// Alternative: Instead of this, I could have everything regulated here, and then
// put a framecount limit on the MTGS ringbuffer.  But that seems no less complicated
// and would also mean that aforementioned menus would still be laggy by whatever
// frame count threshold.  This method is more responsive.

__forceinline void gsFrameSkip( bool forceskip )
{
	static u8 FramesToRender = 0;
	static u8 FramesToSkip = 0;

	if( !EmuConfig.Video.EnableFrameSkipping ) return;

	// FrameSkip and VU-Skip Magic!
	// Skips a sequence of consecutive frames after a sequence of rendered frames

	// This is the least number of consecutive frames we will render w/o skipping
	const int noSkipFrames = ((EmuConfig.Video.ConsecutiveFrames>0) ? EmuConfig.Video.ConsecutiveFrames : 1);
	// This is the number of consecutive frames we will skip
	const int yesSkipFrames = ((EmuConfig.Video.ConsecutiveSkip>0) ? EmuConfig.Video.ConsecutiveSkip : 1);

	const u64 iEnd = GetCPUTicks();
	const s64 uSlowExpectedEnd = m_iSlowStart + m_iSlowTicks;
	const s64 sSlowDeltaTime = iEnd - uSlowExpectedEnd;

	m_iSlowStart = uSlowExpectedEnd;

	if( forceskip )
	{
		if( !FramesToSkip )
		{
			//Console::Status( "- Skipping some VUs!" );

			GSsetFrameSkip( 1 );
			FramesToRender = noSkipFrames;
			FramesToSkip = 1;	// just set to 1

			// We're already skipping, so FramesToSkip==1 will just restore the gsFrameSkip
			// setting and reset our delta times as needed.
		}
		return;
	}

	if( FramesToRender == 0 )
	{
		// -- Standard operation section --
		// Means neither skipping frames nor force-rendering consecutive frames.

		if( sSlowDeltaTime > 0 )
		{
			// The game is running below the minimum framerate.
			// But don't start skipping yet!  That would be too sensitive.
			// So the skipping code is only engaged if the SlowDeltaTime falls behind by
			// a full frame, or if we're already skipping (in which case we don't care
			// to avoid errant skips).

			// Note: The MTGS can go out of phase from the EE, which means that the
			// variance for a "nominal" framerate can range from 0 to m_iSlowTicks.
			// We also check for that here.

			if( (m_justSkipped && (sSlowDeltaTime > m_iSlowTicks)) ||
				(sSlowDeltaTime > m_iSlowTicks*2) )
			{
				GSsetFrameSkip(1);
				FramesToRender = noSkipFrames+1;
				FramesToSkip = yesSkipFrames;
			}
		}
		else
		{
			// Running at or above full speed, so reset the StartTime since the Limiter
			// will muck things up.  (special case: if skip and limit fps are equal then
			// we don't reset starttime since it would cause desyncing.  We let the EE
			// regulate it via calls to gsSyncLimiterStartTime).

			if( !m_StrictSkipping )
				m_iSlowStart = iEnd;
		}
		m_justSkipped = false;
		return;
	}
	else if( FramesToSkip > 0 )
	{
		// -- Frames-a-Skippin' Section --

		FramesToSkip--;

		if( FramesToSkip == 0 )
		{
			// Skipped our last frame, so restore non-skip behavior

			GSsetFrameSkip(0);

			// Note: If we lag behind by 250ms then it's time to give up on the idea
			// of catching up.  Force the game to slow down by resetting iStart to
			// something closer to iEnd.

			if( sSlowDeltaTime > (m_iSlowTicks + ((s64)GetTickFrequency() / 4)) )
			{
				//Console::Status( "Frameskip couldn't skip enough -- had to lose some time!" );
				m_iSlowStart = iEnd - m_iSlowTicks;
			}

			m_justSkipped = true;
		}
		else
			return;
	}

	//Console::WriteLn( "Consecutive Frames -- Lateness: %d", (int)( sSlowDeltaTime / m_iSlowTicks ) );

	// -- Consecutive frames section --
	// Force-render consecutive frames without skipping.

	FramesToRender--;

	if( sSlowDeltaTime < 0 )
	{
		m_iSlowStart = iEnd;
	}
}

// updategs - if FALSE the gs will skip the frame.
void gsPostVsyncEnd( bool updategs )
{
	*(u32*)(PS2MEM_GS+0x1000) ^= 0x2000; // swap the vsync field
	mtgsThread->PostVsyncEnd( updategs );
}

void _gs_ResetFrameskip()
{
	GSsetFrameSkip( 0 );
}

// Disables the GS Frameskip at runtime without any racy mess...
void gsResetFrameSkip()
{
	mtgsThread->SendSimplePacket(GS_RINGTYPE_FRAMESKIP, 0, 0, 0);
}

void gsDynamicSkipEnable()
{
	if( !m_StrictSkipping ) return;

	mtgsWaitGS();
	m_iSlowStart = GetCPUTicks();
	frameLimitReset();
}

void SaveStateBase::gsFreeze()
{
	FreezeMem(PS2MEM_GS, 0x2000);
	Freeze(CSRw);
	mtgsFreeze();
}
