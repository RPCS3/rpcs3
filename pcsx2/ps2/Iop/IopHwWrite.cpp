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
#include "IopHw_Internal.h"

namespace IopMemory {

using namespace Internal;

// Template-compatible version of the psxHu macro.  Used for writing.
#define psxHu(mem)	(*(u32*)&psxH[(mem) & 0xffff])


//////////////////////////////////////////////////////////////////////////////////////////
//
template< typename T >
static __forceinline void _generic_write( u32 addr, T val )
{
	int bitsize = (sizeof(T) == 1) ? 8 : ( (sizeof(T) == 2) ? 16 : 32 );
	PSXHW_LOG( "HwWrite%d to %s, addr 0x%08x = 0x%08x\n", bitsize, _log_GetIopHwName<T>(addr), addr, val );
	psxHu(addr) = val;
}

void __fastcall iopHwWrite8_generic( u32 addr, mem8_t val )		{ _generic_write<mem8_t>( addr, val ); }
void __fastcall iopHwWrite16_generic( u32 addr, mem16_t val )	{ _generic_write<mem16_t>( addr, val ); }
void __fastcall iopHwWrite32_generic( u32 addr, mem32_t val )	{ _generic_write<mem32_t>( addr, val ); }

//////////////////////////////////////////////////////////////////////////////////////////
//
template< typename T >
static __forceinline T _generic_read( u32 addr )
{
	int bitsize = (sizeof(T) == 1) ? 8 : ( (sizeof(T) == 2) ? 16 : 32 );

	T ret = psxHu(addr);
	PSXHW_LOG( "HwRead%d from %s, addr 0x%08x = 0x%08x\n", bitsize, _log_GetIopHwName<T>(addr), addr, ret );
	return ret;
}

mem8_t __fastcall iopHwRead8_generic( u32 addr )	{ return _generic_read<mem8_t>( addr ); }
mem16_t __fastcall iopHwRead16_generic( u32 addr )	{ return _generic_read<mem16_t>( addr ); }
mem32_t __fastcall iopHwRead32_generic( u32 addr )	{ return _generic_read<mem32_t>( addr ); }


//////////////////////////////////////////////////////////////////////////////////////////
//
void __fastcall iopHwWrite8_Page1( u32 addr, mem8_t val )
{
	// all addresses are assumed to be prefixed with 0x1f801xxx:
	jASSUME( (addr >> 12) == 0x1f801 );

	u32 masked_addr = pgmsk( addr );

	switch( masked_addr )
	{		
		mcase(HW_SIO_DATA): sioWrite8( val ); break;

		// for use of serial port ignore for now
		//case 0x50: serial_write8( val ); break;

		mcase(HW_DEV9_DATA): DEV9write8( addr, val ); break;

		mcase(HW_CDR_DATA0): cdrWrite0( val ); break;
		mcase(HW_CDR_DATA1): cdrWrite1( val ); break;
		mcase(HW_CDR_DATA2): cdrWrite2( val ); break;
		mcase(HW_CDR_DATA3): cdrWrite3( val ); break;

		default:
			if( masked_addr >= 0x100 && masked_addr < 0x130 )
			{
				DevCon::Notice( "HwWrite8 to Counter16 [ignored], addr 0x%08x = 0x%02x", params addr, psxHu8(addr) );
				psxHu8( addr ) = val;
			}
			else if( masked_addr >= 0x480 && masked_addr < 0x4a0 )
			{
				DevCon::Notice( "HwWrite8 to Counter32 [ignored], addr 0x%08x = 0x%02x", params addr, psxHu8(addr) );
				psxHu8( addr ) = val;
			}
			else if( masked_addr >= pgmsk(HW_USB_START) && masked_addr < pgmsk(HW_USB_END) )
			{
				USBwrite8( addr, val );
			}
			else
			{
				psxHu8(addr) = val;
			}
		break;
	}

	PSXHW_LOG( "HwWrite8 to %s, addr 0x%08x = 0x%02x\n", _log_GetIopHwName<mem8_t>(addr), addr, val );
}

static char g_pbuf[1024];
static int g_pbufi;

void __fastcall iopHwWrite8_Page3( u32 addr, mem8_t val )
{
	// all addresses are assumed to be prefixed with 0x1f803xxx:
	jASSUME( (addr >> 12) == 0x1f803 );

	if( addr == 0x1f80380c )	// STDOUT
	{
		bool flush = false;

		// Terminate lines on CR or full buffers, and ignore \n's if the string contents
		// are empty (otherwise terminate on \n too!)
		if(	( val == '\r' ) || ( g_pbufi == 1023 ) ||
			( val == '\n' && g_pbufi != 0 ) )
		{
			g_pbuf[g_pbufi] = 0;
			DevCon::WriteLn( Color_Cyan, g_pbuf );
			g_pbufi = 0;
		}
		else if( val != '\n' )
		{
			g_pbuf[g_pbufi++] = val;
		}
	}

	PSXHW_LOG( "HwWrite8 to %s, addr 0x%08x = 0x%02x", _log_GetIopHwName<mem8_t>(addr), addr, psxHu8(addr) );
	psxHu8( addr ) = val;
}

void __fastcall iopHwWrite8_Page8( u32 addr, mem8_t val )
{
	// all addresses are assumed to be prefixed with 0x1f808xxx:
	jASSUME( (addr >> 12) == 0x1f808 );

	if( addr == HW_SIO2_DATAIN )	// sio2 serial data feed input
		sio2_serialIn( val );
	else
		psxHu8( addr ) = val;

	PSXHW_LOG( "HwWrite8 to %s, addr 0x%08x = 0x%02x", _log_GetIopHwName<mem8_t>(addr), addr, psxHu8(addr) );
	
}

//////////////////////////////////////////////////////////////////////////////////////////
// Templated handler for both 32 and 16 bit write operations, to Page 1 registers.
//
template< typename T >
static __forceinline void _HwWrite_16or32_Page1( u32 addr, T val )
{
	// all addresses are assumed to be prefixed with 0x1f801xxx:
	jASSUME( (addr >> 12) == 0x1f801 );

	// all addresses should be aligned to the data operand size:
	jASSUME(
		( sizeof(T) == 2 && (addr & 1) == 0 ) ||
		( sizeof(T) == 4 && (addr & 3) == 0 )
	);

	u32 masked_addr = addr & 0x0fff;

	// ------------------------------------------------------------------------
	// Counters, 16-bit varieties!
	//
	if( masked_addr >= 0x100 && masked_addr < 0x130 )
	{
		int cntidx = ( masked_addr >> 4 ) & 0xf;
		switch( masked_addr & 0xf )
		{
			case 0x0:
				psxRcntWcount16( cntidx, val );
			break;

			case 0x4:
				psxRcntWmode16( cntidx, val );
			break;

			case 0x8:
				psxRcntWtarget16( cntidx, val );
			break;
			
			default:
				psxHu(addr) = val;
			break;
		}
	}
	// ------------------------------------------------------------------------
	// Counters, 32-bit varieties!
	//
	else if( masked_addr >= 0x480 && masked_addr < 0x4b0 )
	{
		int cntidx = (( masked_addr >> 4 ) & 0xf) - 5;
		switch( masked_addr & 0xf )
		{
			case 0x0:
				psxRcntWcount32( cntidx, val );
			break;

			case 0x2:	// Count HiWord
				psxRcntWcount32( cntidx, (u32)val << 16 );
			break;

			case 0x4:
				psxRcntWmode32( cntidx, val );
			break;

			case 0x8:
				psxRcntWtarget32( cntidx, val );
			break;

			case 0xa:	// Target HiWord
				psxRcntWtarget32( cntidx, (u32)val << 16);
			break;

			default:
				psxHu(addr) = val;
			break;
		}
	}
	// ------------------------------------------------------------------------
	// USB, with both 16 and 32 bit interfaces
	//
	else if( (masked_addr >= pgmsk(HW_USB_START)) && (masked_addr < pgmsk(HW_USB_END)) )
	{
		if( sizeof(T) == 2 ) USBwrite16( addr, val ); else USBwrite32( addr, val );
	}
	// ------------------------------------------------------------------------
	// SPU2, accessible in 16 bit mode only!
	//
	else if( (masked_addr >= pgmsk(HW_SPU2_START)) && (masked_addr < pgmsk(HW_SPU2_END)) )
	{
		if( sizeof(T) == 2 )
			SPU2write( addr, val );
		else
		{
			DevCon::Notice( "HwWrite32 to SPU2? (addr=0x%08X) .. What manner of trickery is this?!", params addr );
			//psxHu(addr) = val;
		}
	}
	else
	{
		switch( masked_addr )
		{
			// ------------------------------------------------------------------------
			mcase(HW_SIO_DATA):
				sioWrite8( (u8)val );
				sioWrite8( (u8)(val >> 8) );
				if( sizeof(T) == 4 )
				{
					// u32 gets rid of compiler warnings when using the u16 version of this template
					sioWrite8( (u8)((u32)val >> 16) );
					sioWrite8( (u8)((u32)val >> 24) );
				}
			break;

			mcase(HW_SIO_STAT):		// read-only?
				//regname = "SIO_STAT (read-only?)";
				//sio.StatReg;
			break;

			mcase(HW_SIO_MODE):
				sio.ModeReg = (u16)val;
				if( sizeof(T) == 4 )
				{
					// My guess on 32-bit accesses.  Dunno yet what the real hardware does. --air
					sio.CtrlReg = (u16)((u32)val >> 16);
				}
			break;

			mcase(HW_SIO_CTRL):
				sio.CtrlReg = (u16)val;
			break;

			mcase(HW_SIO_BAUD):
				sio.BaudReg = (u16)val;
			break;

			// ------------------------------------------------------------------------
			//Serial port stuff not support now ;P
			// case 0x050: serial_write16( val ); break;
			//	case 0x054: serial_status_write( val ); break;
			//	case 0x05a: serial_control_write( val ); break;
			//	case 0x05e: serial_baud_write( val ); break;

			mcase(HW_IREG):
				psxHu(addr) &= val;
			break;

			mcase(HW_IREG+2):
				psxHu(addr) &= val;
			break;

			mcase(HW_IMASK):
				psxHu(addr) = val;
				iopTestIntc();
			break;

			mcase(HW_IMASK+2):
				psxHu(addr) = val;
				iopTestIntc();
			break;

			mcase(HW_ICTRL):
				psxHu(addr) = val;
				iopTestIntc();
			break;

			mcase(HW_ICTRL+2):
				psxHu(addr) = val;
				iopTestIntc();
			break;

			// ------------------------------------------------------------------------
			// Soon-to-be outdated SPU2 DMA hack (spu2 manages its own DMA MADR currently,
			// as asinine as that may seem).
			//
			mcase(0x1f8010C0):
				SPU2WriteMemAddr( 0, val );
				HW_DMA4_MADR = val;
			break;

			mcase(0x1f801500):
				SPU2WriteMemAddr( 1, val );
				HW_DMA7_MADR = val;
			break;
			
			// ------------------------------------------------------------------------
			//
			/*
			mcase(0x1f801088):	// DMA0 CHCR -- MDEC IN [ignored]
				DmaExec(0);
			break;
			
			mcase(0x1f801098):	// DMA1 CHCR -- MDEC OUT [ignored]
				DmaExec(1);
			break;

			mcase(0x1f8010a8):	// DMA2 CHCR -- GPU [ignored]
				DmaExec(2);
			break;*/

			mcase(0x1f8010b8):	// DMA3 CHCR -- CDROM
				psxHu(addr) = val;
				DmaExec(3);
			break;

			mcase(0x1f8010c8):	// DMA4 CHCR -- SPU2 Core 1
				psxHu(addr) = val;
				DmaExecNew(4);
			break;

			mcase(0x1f8010e8):	// DMA6 CHCR -- OT clear
				psxHu(addr) = val;
				DmaExec(6);
			break;

			mcase(0x1f801508):	// DMA7 CHCR -- SPU2 core 2
				psxHu(addr) = val;
				DmaExecNew2(7);
			break;

			mcase(0x1f801518):	// DMA8 CHCR -- DEV9
				psxHu(addr) = val;
				DmaExec2(8);
			break;

			mcase(0x1f801528):	// DMA9 CHCR -- SIF0
				psxHu(addr) = val;
				DmaExec2(9);
			break;

			mcase(0x1f801538):	// DMA10 CHCR -- SIF1
				psxHu(addr) = val;
				DmaExec2(10);
			break;

			mcase(0x1f801548):	// DMA11 CHCR -- SIO2 IN
				psxHu(addr) = val;
				DmaExec2(11);
			break;

			mcase(0x1f801558):	// DMA12 CHCR -- SIO2 OUT
				psxHu(addr) = val;
				DmaExec2(12);
			break;

			// ------------------------------------------------------------------------
			// DMA ICR handlers -- General XOR behavior!
			
			mcase(0x1f8010f4):
			{
				u32 tmp = (~val) & HW_DMA_ICR;
				psxHu(addr) = ((tmp ^ val) & 0xffffff) ^ tmp;
			}
			break;

			mcase(0x1f8010f6):		// ICR_hi (16 bit?) [dunno if it ever happens]
			{
				const u32 val2 = (u32)val << 16;
				const u32 tmp = (~val2) & HW_DMA_ICR;
				psxHu(addr) = (((tmp ^ val2) & 0xffffff) ^ tmp) >> 16;
			}
			break;

			mcase(0x1f801574):
			{
				u32 tmp = (~val) & HW_DMA_ICR2;
				psxHu(addr) = ((tmp ^ val) & 0xffffff) ^ tmp;
			}
			break;
			
			mcase(0x1f801576):		// ICR2_hi (16 bit?) [dunno if it ever happens]
			{
				const u32 val2 = (u32)val << 16;
				const u32 tmp = (~val2) & HW_DMA_ICR2;
				psxHu(addr) = (((tmp ^ val2) & 0xffffff) ^ tmp) >> 16;
			}
			break;

			// ------------------------------------------------------------------------
			// Legacy GPU  emulation (not needed).
			// The IOP emulates the GPU itself through the EE's hardware.

			/*case 0x810:
				PSXHW_LOG("GPU DATA 32bit write %lx", value);
				GPU_writeData(value); return;
			case 0x814:
				PSXHW_LOG("GPU STATUS 32bit write %lx", value);
				GPU_writeStatus(value); return;

			case 0x820:
				mdecWrite0(value); break;
			case 0x824:
				mdecWrite1(value); break;*/

			// ------------------------------------------------------------------------

			mcase(HW_DEV9_DATA):
				DEV9write16( addr, val );
				psxHu(addr) = val;
			break;

			default:
				psxHu(addr) = val;
			break;
		}
	}
	
	PSXHW_LOG( "HwWrite%s to %s, addr 0x%08x = 0x%04x",
		sizeof(T) == 2 ? "16" : "32", _log_GetIopHwName<T>( addr ), addr, val
	);
}


//////////////////////////////////////////////////////////////////////////////////////////
//
void __fastcall iopHwWrite16_Page1( u32 addr, mem16_t val )
{
	_HwWrite_16or32_Page1<mem16_t>( addr, val );
}

void __fastcall iopHwWrite16_Page3( u32 addr, mem16_t val )
{
	// all addresses are assumed to be prefixed with 0x1f803xxx:
	jASSUME( (addr >> 12) == 0x1f803 );
	psxHu16(addr) = val;
	PSXHW_LOG( "HwWrite16 to %s, addr 0x%08x = 0x%04x", _log_GetIopHwName<mem16_t>( addr ), addr, val );
}

void __fastcall iopHwWrite16_Page8( u32 addr, mem16_t val )
{
	// all addresses are assumed to be prefixed with 0x1f808xxx:
	jASSUME( (addr >> 12) == 0x1f808 );
	psxHu16(addr) = val;
	PSXHW_LOG( "HwWrite16 to %s, addr 0x%08x = 0x%04x", _log_GetIopHwName<mem16_t>( addr ), addr, val );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void __fastcall iopHwWrite32_Page1( u32 addr, mem32_t val )
{
	_HwWrite_16or32_Page1<mem32_t >( addr, val );
}

void __fastcall iopHwWrite32_Page3( u32 addr, mem32_t val )
{
	// all addresses are assumed to be prefixed with 0x1f803xxx:
	jASSUME( (addr >> 12) == 0x1f803 );
	psxHu16(addr) = val;
	PSXHW_LOG( "HwWrite32 to %s, addr 0x%08x = 0x%04x", _log_GetIopHwName<mem32_t>( addr ), addr, val );
}

void __fastcall iopHwWrite32_Page8( u32 addr, mem32_t val )
{
	// all addresses are assumed to be prefixed with 0x1f808xxx:
	jASSUME( (addr >> 12) == 0x1f808 );

	u32 masked_addr = addr & 0x0fff;

	if( masked_addr >= 0x200 )
	{
		if( masked_addr < 0x240 )
		{
			const int parm = (masked_addr-0x200) / 4;
			sio2_setSend3( parm, val );
		}
		else if( masked_addr < 0x260 )
		{
			// SIO2 Send commands alternate registers.  First reg maps to Send1, second
			// to Send2, third to Send1, etc.  And the following clever code does this:
			
			const int parm = (masked_addr-0x240) / 8;
			if(masked_addr & 4) sio2_setSend2( parm, val ); else sio2_setSend1( parm, val );
		}
		else if( masked_addr <= 0x280 )
		{
			switch( masked_addr )
			{
				mcase(HW_SIO2_CTRL):	sio2_setCtrl( val );	break;
				mcase(0x1f808278):		sio2_set8278( val );	break;
				mcase(0x1f80827C):		sio2_set827C( val );	break;
				mcase(HW_SIO2_INTR):	sio2_setIntr( val );	break;

				// Other SIO2 registers are read-only, no-ops on write.
				default:
					psxHu32(addr) = val;
				break;					
			}
		}
		else if( masked_addr >= pgmsk(HW_FW_START) && masked_addr <= pgmsk(HW_FW_END) )
		{
			FWwrite32( addr, val );
		}
	}
	else psxHu32(addr) = val;

	PSXHW_LOG( "HwWrite32 to %s, addr 0x%08x = 0x%02x", _log_GetIopHwName<mem32_t>( addr ), addr, val );
}

}
