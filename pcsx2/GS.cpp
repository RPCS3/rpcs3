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
#include "Common.h"

#include <list>

#include "GS.h"
#include "Gif.h"
#include "Counters.h"

using namespace Threading;
using namespace R5900;

u32 CSRw;

__aligned16 u8 g_RealGSMem[0x2000];
extern int m_nCounters[];

void gsOnModeChanged( Fixed100 framerate, u32 newTickrate )
{
	GetMTGS().SendSimplePacket( GS_RINGTYPE_MODECHANGE, framerate.Raw, newTickrate, 0 );
}

static bool		gsIsInterlaced	= false;
GS_RegionMode	gsRegionMode	= Region_NTSC;


void gsSetRegionMode( GS_RegionMode region )
{
	if( gsRegionMode == region ) return;

	gsRegionMode = region;
	UpdateVSyncRate();
}


// Make sure framelimiter options are in sync with the plugin's capabilities.
void gsInit()
{
	memzero(g_RealGSMem);
}

void gsReset()
{
	GetMTGS().ResetGS();

	UpdateVSyncRate();
	GSTransferStatus = (STOPPED_MODE<<4) | (STOPPED_MODE<<2) | STOPPED_MODE;
	memzero(g_RealGSMem);

	GSCSRr = 0x551B4000;   // Set the FINISH bit to 1 for now
	GSIMR = 0x7f00;
	gifRegs->stat.reset();
	gifRegs->ctrl.reset();
	gifRegs->mode.reset();
}

void gsGIFReset()
{
	gifRegs->stat.reset();
	gifRegs->ctrl.reset();
	gifRegs->mode.reset();
}

void gsCSRwrite(u32 value)
{
	if (value & 0x200) { // resetGS

		// perform a soft reset -- which is a clearing of all GIFpaths -- and fall back to doing
		// a full reset if the plugin doesn't support soft resets.

		if( GSgifSoftReset != NULL )
		{
			GIFPath_Clear( GIF_PATH_1 );
			GIFPath_Clear( GIF_PATH_2 );
			GIFPath_Clear( GIF_PATH_3 );
		}
		else
		{
			GetMTGS().SendSimplePacket( GS_RINGTYPE_RESET, 0, 0, 0 );
		}
	
		CSRw |= 0x1f;
		GSCSRr = 0x551B4000;   // Set the FINISH bit to 1 - GS is always at a finish state as we don't have a FIFO(saqib)
		GSIMR = 0x7F00; //This is bits 14-8 thats all that should be 1
	} 
	else if( value & 0x100 ) // FLUSH
	{ 
		// Our emulated GS has no FIFO, but if it did, it would flush it here...
		//Console.WriteLn("GS_CSR FLUSH GS fifo: %x (CSRr=%x)", value, GSCSRr);
	}
	else
	{
		CSRw |= value & 0x1f;
		GetMTGS().SendSimplePacket( GS_RINGTYPE_WRITECSR, CSRw, 0, 0 );
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
		case GS_CSR: // GS_CSR
			gsCSRwrite((CSRw & ~0x000000ff) | value); break;
		case GS_CSR + 1: // GS_CSR
			gsCSRwrite((CSRw & ~0x0000ff00) | (value <<  8)); break;
		case GS_CSR + 2: // GS_CSR
			gsCSRwrite((CSRw & ~0x00ff0000) | (value << 16)); break;
		case GS_CSR + 3: // GS_CSR
			gsCSRwrite((CSRw & ~0xff000000) | (value << 24)); break;
		default:
			*PS2GS_BASE(mem) = value;
			GetMTGS().SendSimplePacket(GS_RINGTYPE_MEMWRITE8, mem&0x13ff, value, 0);
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
	GetMTGS().SendSimplePacket(GS_RINGTYPE_MEMWRITE16, mem&0x13ff, value, 0);
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
	GetMTGS().SendSimplePacket(GS_RINGTYPE_MEMWRITE32, mem&0x13ff, value, 0);
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
	GetMTGS().SendSimplePacket(GS_RINGTYPE_MEMWRITE64, mem&0x13ff, srcval32[0], srcval32[1]);
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

	GetMTGS().SendSimplePacket(GS_RINGTYPE_MEMWRITE64, masked_mem, srcval32[0], srcval32[1]);
	GetMTGS().SendSimplePacket(GS_RINGTYPE_MEMWRITE64, masked_mem+8, srcval32[2], srcval32[3]);
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

// --------------------------------------------------------------------------------------
//  gsFrameSkip
// --------------------------------------------------------------------------------------
// This function regulates the frameskipping status of the GS.  Our new frameskipper for
// 0.9.7 is a very simple logic pattern compared to the old mess.  The goal now is to provide
// the most compatible and efficient frameskip, instead of doing the adaptive logic of
// 0.9.6.  This is almost a necessity because of how many games treat the GS: they upload
// great amounts of data while rendering 2 frames at a time (using double buffering), and
// then use a simple pageswap to display the contents of the second frame for that vsync.
//  (this approach is mostly seen on interlace games; progressive games less so)
// The result is that any skip pattern besides a fully consistent 2on,2off would reuslt in
// tons of missing geometry, rendering frameskip useless.
//
// So instead we use a simple "always skipping" or "never skipping" logic.
//
// EE vs MTGS:
//   This function does not regulate frame limiting, meaning it does no stalling. Stalling
//   functions are performed by the EE, which itself uses thread sleep logic to avoid spin
//   waiting as much as possible (maximizes CPU resource availability for the GS).

__forceinline void gsFrameSkip()
{
	
	if( !EmuConfig.GS.FrameSkipEnable ) return;

	static int consec_skipped = 0;
	static int consec_drawn = 0;
	static bool isSkipping = false;
	
	GSsetFrameSkip( isSkipping );

	if( isSkipping )
	{
		++consec_skipped;
		if( consec_skipped >= EmuConfig.GS.FramesToSkip )
		{
			consec_skipped = 0;
			isSkipping = false;
		}
	}
	else
	{
		++consec_drawn;
		if( consec_drawn >= EmuConfig.GS.FramesToDraw )
		{
			consec_drawn = 0;
			isSkipping = true;
		}
	}
}

void gsPostVsyncEnd()
{
	*(u32*)(PS2MEM_GS+0x1000) ^= 0x2000; // swap the vsync field
	GetMTGS().PostVsyncEnd();
}

void _gs_ResetFrameskip()
{
	GSsetFrameSkip( 0 );
}

// Disables the GS Frameskip at runtime without any racy mess...
void gsResetFrameSkip()
{
	GetMTGS().SendSimplePacket(GS_RINGTYPE_FRAMESKIP, 0, 0, 0);
}

void SaveStateBase::gsFreeze()
{
	FreezeMem(PS2MEM_GS, 0x2000);
	Freeze(CSRw);
	gifPathFreeze();
}
