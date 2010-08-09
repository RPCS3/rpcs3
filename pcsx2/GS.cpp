/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

__aligned16 u8 g_RealGSMem[0x2000];

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

extern bool SIGNAL_IMR_Pending;
extern u32 SIGNAL_Data_Pending[2];

void gsGIFReset()
{
	gifRegs->stat.reset();
	gifRegs->ctrl.reset();
	gifRegs->mode.reset();
}

void gsReset()
{
	GetMTGS().ResetGS();

	UpdateVSyncRate();
	GSTransferStatus = (STOPPED_MODE<<8) | (STOPPED_MODE<<4) | STOPPED_MODE;
	memzero(g_RealGSMem);

	SIGNAL_IMR_Pending = false;

	CSRreg.Reset();
	GSIMR = 0x7f00;

	// FIXME: This really doesn't belong here, and I seriously doubt it's needed.
	// If it is needed it should be in the GIF portion of hwReset().  --air
	gsGIFReset();
}

static __fi void gsCSRwrite( const tGS_CSR& csr )
{
	if (csr.RESET) {

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

		SIGNAL_IMR_Pending = false;
		CSRreg.Reset();
		GSIMR = 0x7F00;			//This is bits 14-8 thats all that should be 1
	}

	if(csr.FLUSH)
	{
		// Our emulated GS has no FIFO, but if it did, it would flush it here...
		//Console.WriteLn("GS_CSR FLUSH GS fifo: %x (CSRr=%x)", value, GSCSRr);
	}
	
	if(csr.SIGNAL)
	{
		// SIGNAL : What's not known here is whether or not the SIGID register should be updated
		//  here or when the IMR is cleared (below).

		if(SIGNAL_IMR_Pending == true)
		{
			//DevCon.Warning("Firing pending signal");
			GIF_LOG("GS SIGNAL (pending) data=%x_%x IMR=%x CSRr=%x",SIGNAL_Data_Pending[0], SIGNAL_Data_Pending[1], GSIMR, GSCSRr);
			GSSIGLBLID.SIGID = (GSSIGLBLID.SIGID&~SIGNAL_Data_Pending[1])|(SIGNAL_Data_Pending[0]&SIGNAL_Data_Pending[1]);

			if (!(GSIMR&0x100))
			gsIrq();

			CSRreg.SIGNAL = true; //Just to be sure :P
		}
		else CSRreg.SIGNAL = false;
		
		SIGNAL_IMR_Pending = false;

		if(gifRegs->stat.P1Q && gifRegs->stat.APATH <= GIF_APATH1) gsPath1Interrupt();
	}
	
	if(csr.FINISH)	CSRreg.FINISH	= false;
	if(csr.HSINT)	CSRreg.HSINT	= false;
	if(csr.VSINT)	CSRreg.VSINT	= false;
	if(csr.EDWINT)	CSRreg.EDWINT	= false;
}

static __fi void IMRwrite(u32 value)
{
	GSIMR = (value & 0x1f00)|0x6000;

	if(CSRreg.GetInterruptMask() & (~(GSIMR >> 8) & 0x1f))
		gsIrq();

	if( SIGNAL_IMR_Pending && !(GSIMR & 0x100))
	{
		// Note: PS2 apps are expected to write a successive 1 and 0 to the IMR in order to
		//  trigger the gsInt and clear the second pending SIGNAL interrupt -- if they fail
		//  to do so, the GS will freeze again upon the very next SIGNAL).
		//
		// What's not known here is whether or not the SIGID register should be updated
		//  here or when the GS is resumed during CSR write (above).

		//GIF_LOG("GS SIGNAL (pending) data=%x_%x IMR=%x CSRr=%x\n",CSR_SIGNAL_Data[0], CSR_SIGNAL_Data[1], GSIMR, GSCSRr);
		//GSSIGLBLID.SIGID = (GSSIGLBLID.SIGID&~CSR_SIGNAL_Data[1])|(CSR_SIGNAL_Data[0]&CSR_SIGNAL_Data[1]);

		CSRreg.SIGNAL = true;
		gsIrq();
	}
}

__fi void gsWrite8(u32 mem, u8 value)
{
	switch (mem)
	{
		// CSR 8-bit write handlers.
		// I'm quite sure these whould just write the CSR portion with the other
		// bits set to 0 (no action).  The previous implementation masked the 8-bit 
		// write value against the previous CSR write value, but that really doesn't
		// make any sense, given that the real hardware's CSR circuit probably has no
		// real "memory" where it saves anything.  (for example, you can't write to
		// and change the GS revision or ID portions -- they're all hard wired.) --air
	
		case GS_CSR: // GS_CSR
			gsCSRwrite( tGS_CSR((u32)value) );			break;
		case GS_CSR + 1: // GS_CSR
			gsCSRwrite( tGS_CSR(((u32)value) <<  8) );	break;
		case GS_CSR + 2: // GS_CSR
			gsCSRwrite( tGS_CSR(((u32)value) << 16) );	break;
		case GS_CSR + 3: // GS_CSR
			gsCSRwrite( tGS_CSR(((u32)value) << 24) );	break;

		default:
			*PS2GS_BASE(mem) = value;
		break;
	}
	GIF_LOG("GS write 8 at %8.8lx with data %8.8lx", mem, value);
}

static __fi void _gsSMODEwrite( u32 mem, u32 value )
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

__fi void gsWrite16(u32 mem, u16 value)
{
	GIF_LOG("GS write 16 at %8.8lx with data %8.8lx", mem, value);

	_gsSMODEwrite( mem, value );

	switch (mem)
	{
		// See note above about CSR 8 bit writes, and handling them as zero'd bits
		// for all but the written parts.
		
		case GS_CSR:
			gsCSRwrite( tGS_CSR((u32)value) );
		return; // do not write to MTGS memory

		case GS_CSR+2:
			gsCSRwrite( tGS_CSR(((u32)value) << 16) );
		return; // do not write to MTGS memory

		case GS_IMR:
			IMRwrite(value);
		return; // do not write to MTGS memory
	}

	*(u16*)PS2GS_BASE(mem) = value;
}

//////////////////////////////////////////////////////////////////////////
// GS Write 32 bit

__fi void gsWrite32(u32 mem, u32 value)
{
	pxAssume( (mem & 3) == 0 );
	GIF_LOG("GS write 32 at %8.8lx with data %8.8lx", mem, value);

	_gsSMODEwrite( mem, value );

	switch (mem)
	{
		case GS_CSR:
			gsCSRwrite(tGS_CSR(value));
		return;

		case GS_IMR:
			IMRwrite(value);
		return;
	}

	*(u32*)PS2GS_BASE(mem) = value;
}

//////////////////////////////////////////////////////////////////////////
// GS Write 64 bit

void __fastcall gsWrite64_generic( u32 mem, const mem64_t* value )
{
	const u32* const srcval32 = (u32*)value;
	GIF_LOG("GS Write64 at %8.8lx with data %8.8x_%8.8x", mem, srcval32[1], srcval32[0]);

	*(u64*)PS2GS_BASE(mem) = *value;
}

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
		case 0x12001040: //busdir

			//This is probably a complete hack, however writing to BUSDIR "should" start a transfer 
			//Only problem is it kills killzone :(.
			// (yes it *is* a complete hack; both lines here in fact --air)
			//=========================================================================
			//gifRegs->stat.OPH = true; // Bleach wants it, Killzone hates it.
			
			gifRegs->stat.DIR = (u32)value[0];
			//=========================================================================
			// BUSDIR INSANITY !! MTGS FLUSH NEEDED
			//
			// Yup folks.  BUSDIR is evil.  The only safe way to handle it is to flush the whole MTGS
			// and ensure complete MTGS and EEcore thread synchronization  This is very slow, no doubt,
			// but on the birght side BUSDIR is used quite rately, indeed.

			// Important: writeback to gsRegs area *prior* to flushing the MTGS.  The flush will sync
			// the GS and MTGS register states, and upload our screwy busdir register in the process. :)
			gsWrite64_generic( mem, value );
			GetMTGS().WaitGS();
		return;

		case GS_CSR:
			gsCSRwrite(tGS_CSR(*value));
		return;

		case GS_IMR:
			IMRwrite((u32)value[0]);
		return;
	}

	gsWrite64_generic( mem, value );
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

	CopyQWC(PS2GS_BASE(mem), value);
}

__fi u8 gsRead8(u32 mem)
{
	GIF_LOG("GS read 8 from %8.8lx  value: %8.8lx", mem, *(u8*)PS2GS_BASE(mem));
	return *(u8*)PS2GS_BASE(mem);
}

__fi u16 gsRead16(u32 mem)
{
	GIF_LOG("GS read 16 from %8.8lx  value: %8.8lx", mem, *(u16*)PS2GS_BASE(mem));
	return *(u16*)PS2GS_BASE(mem);
}

__fi u32 gsRead32(u32 mem)
{
	GIF_LOG("GS read 32 from %8.8lx  value: %8.8lx", mem, *(u32*)PS2GS_BASE(mem));
	return *(u32*)PS2GS_BASE(mem);
}

__fi u64 gsRead64(u32 mem)
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

__fi void gsFrameSkip()
{
	static int consec_skipped = 0;
	static int consec_drawn = 0;
	static bool isSkipping = false;

	if( !EmuConfig.GS.FrameSkipEnable )
	{
		if( isSkipping )
		{
			// Frameskipping disabled on-the-fly .. make sure the GS is restored to non-skip
			// behavior.
			GSsetFrameSkip( false );
			isSkipping = false;
		}
		return;
	}

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
	CSRreg.SwapField();
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
	Freeze(SIGNAL_IMR_Pending);

	if( GetVersion() > 0 )
		Freeze(gsRegionMode);

	gifPathFreeze();
}
