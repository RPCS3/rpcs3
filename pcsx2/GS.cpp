/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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

// This should be done properly with the other logs.
#ifdef DEBUG
#define MTGS_LOG SysPrintf
#else
#define MTGS_LOG 0&&
#endif

static bool m_gsOpened = false;

#ifdef PCSX2_DEVBUILD

// GS Playback
int g_SaveGSStream = 0; // save GS stream; 1 - prepare, 2 - save
int g_nLeftGSFrames = 0; // when saving, number of frames left
gzSavingState* g_fGSSave;

void GSGIFTRANSFER1(u32 *pMem, u32 addr) { 
	if( g_SaveGSStream == 2) { 
		u32 type = GSRUN_TRANS1; 
		u32 size = (0x4000-(addr))/16;
		g_fGSSave->Freeze( type );
		g_fGSSave->Freeze( size );
		g_fGSSave->FreezeMem( ((u8*)pMem)+(addr), size*16 );
	} 
	GSgifTransfer1(pMem, addr); 
}

void GSGIFTRANSFER2(u32 *pMem, u32 size) { 
	if( g_SaveGSStream == 2) { 
		u32 type = GSRUN_TRANS2; 
		u32 _size = size; 
		g_fGSSave->Freeze( type );
		g_fGSSave->Freeze( size );
		g_fGSSave->FreezeMem( pMem, _size*16 );
	} 
	GSgifTransfer2(pMem, size); 
}

void GSGIFTRANSFER3(u32 *pMem, u32 size) { 
	if( g_SaveGSStream == 2 ) { 
		u32 type = GSRUN_TRANS3; 
		u32 _size = size; 
		g_fGSSave->Freeze( type );
		g_fGSSave->Freeze( size );
		g_fGSSave->FreezeMem( pMem, _size*16 );
	} 
	GSgifTransfer3(pMem, size); 
}

__forceinline void GSVSYNC(void) { 
	if( g_SaveGSStream == 2 ) { 
		u32 type = GSRUN_VSYNC; 
		g_fGSSave->Freeze( type ); 
	} 
}
#else

__forceinline void GSGIFTRANSFER1(u32 *pMem, u32 addr) { 
	GSgifTransfer1(pMem, addr); 
}

__forceinline void GSGIFTRANSFER2(u32 *pMem, u32 size) { 
	GSgifTransfer2(pMem, size); 
}

__forceinline void GSGIFTRANSFER3(u32 *pMem, u32 size) { 
	GSgifTransfer3(pMem, size); 
}

__forceinline void GSVSYNC(void) { 
} 
#endif

u32 CSRw;

PCSX2_ALIGNED16( u8 g_RealGSMem[0x2000] );
#define PS2GS_BASE(mem) (g_RealGSMem+(mem&0x13ff))

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

	u32 frameSkipThreshold = Config.CustomFrameSkip*50;
	if( Config.CustomFrameSkip == 0)
	{
		// default: load the frameSkipThreshold with a value roughly 90% of our current framerate
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
	if( mtgsThread != NULL )
		mtgsThread->SendSimplePacket( GS_RINGTYPE_MODECHANGE, framerate, newTickrate, 0 );
	else
		_gs_ChangeTimings( framerate, newTickrate );
}

void gsSetVideoRegionType( u32 isPal )
{
	if( isPal )
	{
		if( Config.PsxType & 1 ) return;
		Console::WriteLn( "PAL Display Mode Initialized." );
		Config.PsxType |= 1;
	}
	else
	{
		if( !(Config.PsxType & 1 ) ) return;
		Console::WriteLn( "NTSC Display Mode Initialized." );
		Config.PsxType &= ~1;
	}

	// If we made it this far it means the refresh rate changed, so update the vsync timers:
	UpdateVSyncRate();
}


// Make sure framelimiter options are in sync with the plugin's capabilities.
void gsInit()
{
	switch(CHECK_FRAMELIMIT)
	{
		case PCSX2_FRAMELIMIT_SKIP:
		case PCSX2_FRAMELIMIT_VUSKIP:
			if( GSsetFrameSkip == NULL )
			{
				Config.Options &= ~PCSX2_FRAMELIMIT_MASK;
				Console::WriteLn("Notice: Disabling frameskip -- GS plugin does not support it.");
			}
		break;
	}
}

// Opens the gsRingbuffer thread.
s32 gsOpen()
{
	if( m_gsOpened ) return 0;

	//video
	// Only bind the gsIrq if we're not running the MTGS.
	// The MTGS simulates its own gsIrq in order to maintain proper sync.

	m_gsOpened = mtgsOpen();
	if( !m_gsOpened )
	{
		// MTGS failed to init or is disabled.  Try the GS instead!
		// ... and set the memptr again just in case (for switching between GS/MTGS on the fly)

		GSsetBaseMem( PS2MEM_GS );
		GSirqCallback( gsIrq );

		m_gsOpened = !GSopen((void *)&pDsp, "PCSX2", 0);
	}

	/*if( m_gsOpened )
	{
		gsOnModeChanged(
			(Config.PsxType & 1) ? FRAMERATE_PAL : FRAMERATE_NTSC,
			UpdateVSyncRate()
		);
	}*/
	return !m_gsOpened;
}

void gsClose()
{
	if( !m_gsOpened ) return;
	m_gsOpened = false;

	// Throw an assert if our multigs setting and mtgsThread status
	// aren't synched.  It shouldn't break the code anyway but it's a
	// bad coding habit that we should catch and fix early.
	assert( !!CHECK_MULTIGS == (mtgsThread != NULL ) );

	if( mtgsThread != NULL )
	{
		mtgsThread->Close();
		safe_delete( mtgsThread );
	}
	else
		GSclose();
}

void gsReset()
{
	// Sanity check in case the plugin hasn't been initialized...
	if( !m_gsOpened ) return;

	if( mtgsThread != NULL )
		mtgsThread->Reset();
	else
	{
		Console::Notice( "GIF reset" );
		GSreset();
		GSsetFrameSkip(0);
	}

	gsOnModeChanged(
		(Config.PsxType & 1) ? FRAMERATE_PAL : FRAMERATE_NTSC,
		UpdateVSyncRate() 
	);

	memzero_obj(g_RealGSMem);

	Path3transfer = 0;

	GSCSRr = 0x551B400F;   // Set the FINISH bit to 1 for now
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

	if( mtgsThread != NULL )
		mtgsThread->GIFSoftReset( mask );
	else
		GSgifSoftReset( mask );

	return true;
}

void gsGIFReset()
{
	// fixme - should this be here? (air)
	//memzero_obj(g_RealGSMem);

	// perform a soft reset (but do not do a full reset if the soft reset API is unavailable)
	gsGIFSoftReset( 7 );

	GSCSRr = 0x551B400F;   // Set the FINISH bit to 1 for now
	GSIMR = 0x7f00;
	psHu32(GIF_STAT) = 0;
	psHu32(GIF_CTRL) = 0;
	psHu32(GIF_MODE) = 0;
}

void gsCSRwrite(u32 value)
{
	CSRw |= value & ~0x60;

	if( mtgsThread != NULL )
		mtgsThread->SendSimplePacket( GS_RINGTYPE_WRITECSR, CSRw, 0, 0 );
	else
		GSwriteCSR(CSRw);

	GSCSRr = ((GSCSRr&~value)&0x1f)|(GSCSRr&~0x1f);

	// Our emulated GS has no FIFO...
	/*if( value & 0x100 ) { // FLUSH
		//Console::WriteLn("GS_CSR FLUSH GS fifo: %x (CSRr=%x)", params value, GSCSRr);
	}*/

	if (value & 0x200) { // resetGS

		// perform a soft reset -- and fall back to doing a full reset if the plugin doesn't
		// support soft resets.

		if( !gsGIFSoftReset( 7 ) )
		{
			if( mtgsThread != NULL )
				mtgsThread->SendSimplePacket( GS_RINGTYPE_RESET, 0, 0, 0 );
			else
				GSreset();
		}

		GSCSRr = 0x551B400F;   // Set the FINISH bit to 1 - GS is always at a finish state as we don't have a FIFO(saqib)
		GSIMR = 0x7F00; //This is bits 14-8 thats all that should be 1
	}
}

static void IMRwrite(u32 value)
{
	GSIMR = (value & 0x1f00)|0x6000;

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

			if( mtgsThread != NULL )
				mtgsThread->SendSimplePacket(GS_RINGTYPE_MEMWRITE8, mem&0x13ff, value, 0);
	}
	GIF_LOG("GS write 8 at %8.8lx with data %8.8lx", mem, value);
}

__forceinline void _gsSMODEwrite( u32 mem, u32 value )
{
	switch (mem)
	{
		case GS_SMODE1:
			gsSetVideoRegionType( (value & 0x6000) == 0x6000 );
		break;

		case GS_SMODE2:
			if(value & 0x1)
				Config.PsxType |= 2; // Interlaced
			else
				Config.PsxType &= ~2;	// Non-Interlaced
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

	if( mtgsThread != NULL )
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

	if( mtgsThread != NULL )
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

	if( mtgsThread != NULL )
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

	if( mtgsThread != NULL )
	{
		mtgsThread->SendSimplePacket(GS_RINGTYPE_MEMWRITE64, masked_mem, srcval32[0], srcval32[1]);
		mtgsThread->SendSimplePacket(GS_RINGTYPE_MEMWRITE64, masked_mem+8, srcval32[2], srcval32[3]);
	}
}

#if 0
// This function is left in for now for debugging/reference purposes.
__forceinline void gsWrite64(u32 mem, u64 value)
{
	GIF_LOG("GS write 64 at %8.8lx with data %8.8lx_%8.8lx", mem, ((u32*)&value)[1], (u32)value);

	switch (mem)
	{
		case 0x12000010: // GS_SMODE1
			gsSetVideoRegionType( (value & 0x6000) == 0x6000 );
		break;

		case 0x12000020: // GS_SMODE2
			if(value & 0x1) Config.PsxType |= 2; // Interlaced
			else Config.PsxType &= ~2;	// Non-Interlaced
			break;

		case 0x12001000: // GS_CSR
			gsCSRwrite((u32)value);
			return;

		case 0x12001010: // GS_IMR
			IMRwrite((u32)value);
			return;
	}

	*(u64*)PS2GS_BASE(mem) = value;

	if( mtgsThread != NULL )
		mtgsThread->SendSimplePacket(GS_RINGTYPE_MEMWRITE64, mem&0x13ff, (u32)value, (u32)(value>>32));
}
#endif

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

	if( mtgsThread != NULL )
	{
		mtgsThread->SendSimplePacket(
			GS_RINGTYPE_STARTTIME,
			deltaTime,
			0,
			0
		);
	}
	else
	{
		m_iSlowStart += deltaTime;
		//m_justSkipped = false;
	}
}

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

	if( CHECK_FRAMELIMIT != PCSX2_FRAMELIMIT_SKIP && 
		CHECK_FRAMELIMIT != PCSX2_FRAMELIMIT_VUSKIP ) return;

	// FrameSkip and VU-Skip Magic!
	// Skips a sequence of consecutive frames after a sequence of rendered frames

	// This is the least number of consecutive frames we will render w/o skipping
	const int noSkipFrames = ((Config.CustomConsecutiveFrames>0) ? Config.CustomConsecutiveFrames : 1);
	// This is the number of consecutive frames we will skip				
	const int yesSkipFrames = ((Config.CustomConsecutiveSkip>0) ? Config.CustomConsecutiveSkip : 1);

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
	
	// if we've already given the EE a skipcount assignment then don't do anything more.
	// Otherwise we could start compounding the issue and skips would be too long.
	if( g_vu1SkipCount > 0 )
	{
		//Console::Status("- Already Assigned a Skipcount.. %d", params g_vu1SkipCount );
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
				//Console::Status( "Frameskip Initiated! Lateness: %d", params (int)( (sSlowDeltaTime*100) / m_iSlowTicks ) );
				
				if( CHECK_FRAMELIMIT == PCSX2_FRAMELIMIT_VUSKIP )
				{
					// For best results we have to wait for the EE to
					// tell us when to skip, so that VU skips are synched with GS skips.
					AtomicExchangeAdd( g_vu1SkipCount, yesSkipFrames+1 );
				}
				else
				{
					GSsetFrameSkip(1);
					FramesToRender = noSkipFrames+1;
					FramesToSkip = yesSkipFrames;
				}
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

	//Console::WriteLn( "Consecutive Frames -- Lateness: %d", params (int)( sSlowDeltaTime / m_iSlowTicks ) );

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

	if( mtgsThread != NULL ) 
		mtgsThread->PostVsyncEnd( updategs );
	else
	{
		GSvsync((*(u32*)(PS2MEM_GS+0x1000)&0x2000));

		// update here on single thread mode *OBSOLETE*
		if( PAD1update != NULL ) PAD1update(0);
		if( PAD2update != NULL ) PAD2update(1);

		gsFrameSkip( !updategs );
	}
}

void _gs_ResetFrameskip()
{
	g_vu1SkipCount = 0;		// set to 0 so that EE will re-enable the VU at the next vblank.
	GSsetFrameSkip( 0 );
}

// Disables the GS Frameskip at runtime without any racy mess...
void gsResetFrameSkip()
{
	if( mtgsThread != NULL )
		mtgsThread->SendSimplePacket(GS_RINGTYPE_FRAMESKIP, 0, 0, 0);
	else
		_gs_ResetFrameskip();
}

void gsDynamicSkipEnable()
{
	if( !m_StrictSkipping ) return;

	mtgsWaitGS();
	m_iSlowStart = GetCPUTicks();	
	frameLimitReset();
}

void SaveState::gsFreeze()
{
	FreezeMem(PS2MEM_GS, 0x2000);
	Freeze(CSRw);
	mtgsFreeze();
}

#ifdef PCSX2_DEVBUILD

struct GSStatePacket
{
	u32 type;
	vector<u8> mem;
};

// runs the GS
void RunGSState( gzLoadingState& f )
{
	u32 newfield;
	list< GSStatePacket > packets;

	while( !f.Finished() )
	{
		int type, size;
		f.Freeze( type );

		if( type != GSRUN_VSYNC ) f.Freeze( size );

		packets.push_back(GSStatePacket());
		GSStatePacket& p = packets.back();

		p.type = type;

		if( type != GSRUN_VSYNC ) {
			p.mem.resize(size*16);
			f.FreezeMem( &p.mem[0], size*16 );
		}
	}

	list<GSStatePacket>::iterator it = packets.begin();
	g_SaveGSStream = 3;

	//int skipfirst = 1;

	// first extract the data
	while(1) {

		switch(it->type) {
			case GSRUN_TRANS1:
				GSgifTransfer1((u32*)&it->mem[0], 0);
				break;
			case GSRUN_TRANS2:
				GSgifTransfer2((u32*)&it->mem[0], it->mem.size()/16);
				break;
			case GSRUN_TRANS3:
				GSgifTransfer3((u32*)&it->mem[0], it->mem.size()/16);
				break;
			case GSRUN_VSYNC:
				// flip
				newfield = (*(u32*)(PS2MEM_GS+0x1000)&0x2000) ? 0 : 0x2000;
				*(u32*)(PS2MEM_GS+0x1000) = (*(u32*)(PS2MEM_GS+0x1000) & ~(1<<13)) | newfield;

				GSvsync(newfield);
				SysUpdate();

				if( g_SaveGSStream != 3 )
					return;
				break;

			jNO_DEFAULT
		}

		++it;
		if( it == packets.end() )
			it = packets.begin();
	}
}

#endif

#undef GIFchain
