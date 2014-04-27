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
#include "IopHw_Internal.h"
#include "Sif.h"
#include "Sio.h"
#include "CDVD/CdRom.h"

namespace IopMemory {

using namespace Internal;

// Template-compatible version of the psxHu macro.  Used for writing.
#define psxHu(mem)	(*(u32*)&iopHw[(mem) & 0xffff])


//////////////////////////////////////////////////////////////////////////////////////////
//
template< typename T >
static __fi void _generic_write( u32 addr, T val )
{
	//int bitsize = (sizeof(T) == 1) ? 8 : ( (sizeof(T) == 2) ? 16 : 32 );
	IopHwTraceLog<T>( addr, val, false );
	psxHu(addr) = val;
}

void __fastcall iopHwWrite8_generic( u32 addr, mem8_t val )		{ _generic_write<mem8_t>( addr, val ); }
void __fastcall iopHwWrite16_generic( u32 addr, mem16_t val )	{ _generic_write<mem16_t>( addr, val ); }
void __fastcall iopHwWrite32_generic( u32 addr, mem32_t val )	{ _generic_write<mem32_t>( addr, val ); }

//////////////////////////////////////////////////////////////////////////////////////////
//
template< typename T >
static __fi T _generic_read( u32 addr )
{
	//int bitsize = (sizeof(T) == 1) ? 8 : ( (sizeof(T) == 2) ? 16 : 32 );

	T ret = psxHu(addr);
	IopHwTraceLog<T>( addr, ret, true );
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
	pxAssert( (addr >> 12) == 0x1f801 );

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
				DbgCon.Warning( "HwWrite8 to Counter16 [ignored] @ addr 0x%08x = 0x%02x", addr, psxHu8(addr) );
				psxHu8( addr ) = val;
			}
			else if( masked_addr >= 0x480 && masked_addr < 0x4a0 )
			{
				DbgCon.Warning( "HwWrite8 to Counter32 [ignored] @ addr 0x%08x = 0x%02x", addr, psxHu8(addr) );
				psxHu8( addr ) = val;
			}
			else if( (masked_addr >= pgmsk(HW_USB_START)) && (masked_addr < pgmsk(HW_USB_END)) )
			{
				USBwrite8( addr, val );
			}
			else
			{
				psxHu8(addr) = val;
			}
		break;
	}

	IopHwTraceLog<mem8_t>( addr, val, false );
}

void __fastcall iopHwWrite8_Page3( u32 addr, mem8_t val )
{
	// all addresses are assumed to be prefixed with 0x1f803xxx:
	pxAssert( (addr >> 12) == 0x1f803 );

	if( SysConsole.iopConsole.IsActive() && (addr == 0x1f80380c) )	// STDOUT
	{
		static char pbuf[1024];
		static int pidx;
		static bool iggy_newline = false;

		if (val == '\r')
		{
			iggy_newline = true;
			pbuf[pidx++] = '\n';
		}
		else if (!iggy_newline || (val != '\n'))
		{
			iggy_newline = false;
			pbuf[pidx++] = val;
		}

		if ((pidx == ArraySize(pbuf)-1) || (pbuf[pidx-1] == '\n'))
		{
			pbuf[pidx] = 0;
			iopConLog( ShiftJIS_ConvertString(pbuf) );
			pidx = 0;
		}
	}

	psxHu8( addr ) = val;
	IopHwTraceLog<mem8_t>( addr, val, false );
}

void __fastcall iopHwWrite8_Page8( u32 addr, mem8_t val )
{
	// all addresses are assumed to be prefixed with 0x1f808xxx:
	pxAssert( (addr >> 12) == 0x1f808 );

	if( addr == HW_SIO2_DATAIN )	// sio2 serial data feed input
		sio2_serialIn( val );
	else
		psxHu8( addr ) = val;

	IopHwTraceLog<mem8_t>( addr, val, false );
}

//////////////////////////////////////////////////////////////////////////////////////////
// Templated handler for both 32 and 16 bit write operations, to Page 1 registers.
//
template< typename T >
static __fi void _HwWrite_16or32_Page1( u32 addr, T val )
{
	// all addresses are assumed to be prefixed with 0x1f801xxx:
	pxAssert( (addr >> 12) == 0x1f801 );

	// all addresses should be aligned to the data operand size:
	pxAssert(
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
			DbgCon.Warning( "HwWrite32 to SPU2? @ 0x%08X .. What manner of trickery is this?!", addr );
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
#ifndef ENABLE_NEW_IOPDMA_SPU2
				SPU2WriteMemAddr( 0, val );
#endif
				HW_DMA4_MADR = val;
			break;

			mcase(0x1f801500):
#ifndef ENABLE_NEW_IOPDMA_SPU2
				SPU2WriteMemAddr( 1, val );
#endif
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
			break;*/
			mcase(0x1f8010ac):
				DevCon.Warning("SIF2 IOP TADR?? write");
				psxHu(addr) = val;
			break;

			mcase(0x1f8010a8) :	// DMA2 CHCR -- GPU [ignored]
				psxHu(addr) = val;
				DmaExec(2);
			break;

			mcase(0x1f8010b8):	// DMA3 CHCR -- CDROM
				psxHu(addr) = val;
				DmaExec(3);
			break;

			mcase(0x1f8010c8):	// DMA4 CHCR -- SPU2 Core 1
				psxHu(addr) = val;
#ifdef ENABLE_NEW_IOPDMA_SPU2
				DmaExecNew(4);
#else
				DmaExec(4);
#endif
			break;

			mcase(0x1f8010e8):	// DMA6 CHCR -- OT clear
				psxHu(addr) = val;
				DmaExec(6);
			break;

			mcase(0x1f801508):	// DMA7 CHCR -- SPU2 core 2
				psxHu(addr) = val;
#ifdef ENABLE_NEW_IOPDMA_SPU2
				DmaExecNew2(7);
#else
				DmaExec2(7);
#endif
			break;

			mcase(0x1f801518):	// DMA8 CHCR -- DEV9
				psxHu(addr) = val;
#ifdef ENABLE_NEW_IOPDMA_DEV9
				DmaExecNew2(8);
#else
				DmaExec2(8);
#endif
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
#ifdef ENABLE_NEW_IOPDMA_SIO
				DmaExecNew2(11);
#else
				DmaExec2(11);
#endif
			break;

			mcase(0x1f801558):	// DMA12 CHCR -- SIO2 OUT
				psxHu(addr) = val;
#ifdef ENABLE_NEW_IOPDMA_SIO
				DmaExecNew2(12);
#else
				DmaExec2(12);
#endif
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
			// Legacy GPU  emulation
			//

			mcase(HW_PS1_GPU_DATA) :
				DevCon.Warning("GPUDATA Write %x", val);
				/*if (val == 0x00000000)
				{
					psxHu32(HW_PS1_GPU_STATUS) = 0x14802000;
				}
				else if (val == 0x01000000)
				{
					DevCon.Warning("GP0 FIFO Clear");
					sif2.fifo.clear();
				}
			     else 
				{*/
					psxHu(HW_PS1_GPU_DATA) = val; // guess
					WriteFifoSingleWord();
						
				//}
				//GPU_writeData(value); // really old code from PCSX? (rama)
				break;
			mcase (HW_PS1_GPU_STATUS):
				DevCon.Warning("GPUSTATUS Write Command %x Param %x", (u32)val >> 24, val & 0xffffff);
			//psxHu(addr) = val; // guess
				//psxHu(HW_PS1_GPU_STATUS) = val;
				//WriteFifoSingleWord();
			//	psxHu(HW_PS1_GPU_DATA) = val; // guess
			//	WriteFifoSingleWord();
				/*if (val == 0) //Reset
				{
					psxHu32(HW_PS1_GPU_STATUS) = 0x14802000;
				}
				else if (val == 0x10000007) //Get GPU version
				{
					//DevCon.Warning("Get Version");
					psxHu(HW_PS1_GPU_DATA) = 2;
				}
				else if ((val & 0xff000000) == 0x04000000)
				{
					//psxHu32(HW_PS1_GPU_STATUS) = psxHu32(HW_PS1_GPU_STATUS) &= ~0x60000000;
					psxHu32(HW_PS1_GPU_STATUS) |= (val & 0x3) << 29;
					switch (val & 0x3)
					{
						case 0x0:
							//DevCon.Warning("Set DMA Mode OFF");
							psxHu32(HW_PS1_GPU_STATUS) &= ~0x2000000;
							break;
						case 0x1:
							//DevCon.Warning("Set DMA Mode FIFO");
							psxHu32(HW_PS1_GPU_STATUS) |= 0x2000000;
							break;
						case 0x2:
							//DevCon.Warning("Set DMA Mode CPU->GPU");
							psxHu32(HW_PS1_GPU_STATUS) = (psxHu32(HW_PS1_GPU_STATUS) & ~0x2000000) | ((psxHu32(HW_PS1_GPU_STATUS) & 0x10000000) >> 3);
							break;
						case 0x3:
							//DevCon.Warning("Set DMA Mode GPUREAD->CPU");
							psxHu32(HW_PS1_GPU_STATUS) = (psxHu32(HW_PS1_GPU_STATUS) & ~0x2000000) | ((psxHu32(HW_PS1_GPU_STATUS) & 0x8000000) >> 2);
							break;
					}
				
				}
				else if (val == 0x03000000)
				{
				//	DevCon.Warning("Turn Display on");
					psxHu32(HW_PS1_GPU_STATUS) &= ~(1 << 23);
				}
				else if ((val & 0xff000000) == 0x05000000)
				{
					DevCon.Warning("Start display area");

					//psxHu32(HW_PS1_GPU_STATUS) |= 0x80000000;
					//psxHu32(HW_PS1_GPU_STATUS) &= ~(1 << 26);
				}
				else if ((val & 0xff000000) == 0x08000000)
				{
					//DevCon.Warning("Display Mode");
					psxHu32(HW_PS1_GPU_STATUS) &= ~0x7F4000;
					psxHu32(HW_PS1_GPU_STATUS) |= (u32)(val & 0x3f) << 17;
					psxHu32(HW_PS1_GPU_STATUS) |= (u32)(val & 0x40) << 10;
					psxHu32(HW_PS1_GPU_STATUS) |= (u32)(val & 0x80) << 7;
					//psxHu32(HW_PS1_GPU_STATUS) |= 0x80000000;
					//psxHu32(HW_PS1_GPU_STATUS) &= ~(1 << 26);
				}
				else
				{
					//DevCon.Warning("Unknown GP1 Command");
				}*/
				//GPU_writeStatus(value); // really old code from PCSX? (rama)
				break;
			mcase (0x1f801820): // MDEC
				DevCon.Warning("MDEX 1820 Write %x", val);
				psxHu(addr) = val; // guess
				//mdecWrite0(value); // really old code from PCSX? (rama)
				break;
			mcase (0x1f801824): // MDEC
				DevCon.Warning("MDEX 1824 Write %x", val);
				psxHu(addr) = val; // guess
				//mdecWrite1(value); // really old code from PCSX? (rama)
				break;

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

	IopHwTraceLog<T>( addr, val, false );
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
	pxAssert( (addr >> 12) == 0x1f803 );
	psxHu16(addr) = val;
	IopHwTraceLog<mem16_t>( addr, val, false );
}

void __fastcall iopHwWrite16_Page8( u32 addr, mem16_t val )
{
	// all addresses are assumed to be prefixed with 0x1f808xxx:
	pxAssert( (addr >> 12) == 0x1f808 );
	psxHu16(addr) = val;
	IopHwTraceLog<mem16_t>( addr, val, false );
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
	pxAssert( (addr >> 12) == 0x1f803 );
	psxHu16(addr) = val;
	IopHwTraceLog<mem32_t>( addr, val, false );
}

void __fastcall iopHwWrite32_Page8( u32 addr, mem32_t val )
{
	// all addresses are assumed to be prefixed with 0x1f808xxx:
	pxAssert( (addr >> 12) == 0x1f808 );

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

	IopHwTraceLog<mem32_t>( addr, val, false );
}

}
