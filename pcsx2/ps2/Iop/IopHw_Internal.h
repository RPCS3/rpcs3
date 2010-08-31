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

#pragma once

#include "IopCommon.h"

namespace IopMemory {
namespace Internal {

//////////////////////////////////////////////////////////////////////////////////////////
// Masking helper so that I can use the fully qualified address for case statements.
// Switches are based on the bottom 12 bits only, since MSVC tends to optimize switches
// better when it has a limited width operand to work with. :)
//
#define pgmsk( src ) ( (src) & 0x0fff )
#define mcase( src ) case pgmsk(src)


//////////////////////////////////////////////////////////////////////////////////////////
// Helper for debug logging of IOP Registers.  Takes an input address and returns a
// register name.
//
// This list is not yet exhaustive.  If you spot something that's missing, feel free to
// fill it in any time. :)
//

template< typename T>
static __ri const char* _ioplog_GetHwName( u32 addr, T val )
{
	switch( addr )
	{
		// ------------------------------------------------------------------------
		// SSBUS -- Two Ess'es?

		case HW_SSBUS_SPD_ADDR:		return "SSBUS spd_addr";
		case HW_SSBUS_PIO_ADDR:		return "SSBUS pio_addr";
		case HW_SSBUS_SPD_DELAY:	return "SSBUS spd_delay";
		case HW_SSBUS_DEV1_DELAY:	return "SSBUS dev1_delay";
		case HW_SSBUS_ROM_DELAY:	return "SSBUS rom_delay";
		case HW_SSBUS_SPU_DELAY:	return "SSBUS spu_delay";
		case HW_SSBUS_DEV5_DELAY:	return "SSBUS dev5_delay";
		case HW_SSBUS_PIO_DELAY:	return "SSBUS pio_delay";
		case HW_SSBUS_COM_DELAY:	return "SSBUS com_delay";
		case HW_SSBUS_DEV1_ADDR:	return "SSBUS dev1_addr";
		case HW_SSBUS_SPU_ADDR:		return "SSBUS spu_addr";
		case HW_SSBUS_DEV5_ADDR:	return "SSBUS dev5_addr";
		case HW_SSBUS_SPU1_ADDR:	return "SSBUS spu1_addr";
		case HW_SSBUS_DEV9_ADDR3:	return "SSBUS dev9_addr3";
		case HW_SSBUS_SPU1_DELAY:	return "SSBUS spu1_delay";
		case HW_SSBUS_DEV9_DELAY2:	return "SSBUS dev9_delay2";
		case HW_SSBUS_DEV9_DELAY3:	return "SSBUS dev9_delay3";
		case HW_SSBUS_DEV9_DELAY1:	return "SSBUS dev9_delay1";

		// ------------------------------------------------------------------------
		case 0x1f801060:return "RAM_SIZE";

		case HW_IREG:	return "IREG";
		case HW_IREG+2:	return "IREG_hi";
		case HW_IMASK:	return "IMASK";
		case HW_IMASK+2:return "IMASK_hi";
		case HW_ICTRL:	return "ICTRL";
		case HW_ICTRL+2:return "ICTRL_hi";
		case HW_ICFG:	return "ICFG";

		case HW_SIO_DATA: return "SIO";
		case HW_SIO_STAT: return "SIO STAT";
		case HW_SIO_MODE: return ( sizeof(T) == 4 ) ? "SIO_MODE+CTRL" : "SIO MODE";
		case HW_SIO_CTRL: return "SIO CTRL";
		case HW_SIO_BAUD: return "SIO BAUD";

		case 0x1f8014c0: return "RTC_HOLDMODE";
		case HW_DEV9_DATA: return "DEV9_R_REV/DATA";

		// ------------------------------------------------------------------------
		// BCR_LABEL -- Selects label for BCR depending on operand size (BCR has hi
		// and low values of count and size, respectively)
		#define BCR_LABEL( dma ) (sizeof(T)==4) ? dma" BCR" : dma" BCR_size";

		case 0x1f8010a0: return "DMA2 MADR";
		case 0x1f8010a4: return BCR_LABEL( "DMA2" );
		case 0x1f8010a6: return "DMA2 BCR_count";
		case 0x1f8010a8: return "DMA2 CHCR";
		case 0x1f8010ac: return "DMA2 TADR";

		case 0x1f8010b0: return "DMA3 MADR";
		case 0x1f8010b4: return BCR_LABEL( "DMA3" );
		case 0x1f8010b6: return "DMA3 BCR_count";
		case 0x1f8010b8: return "DMA3 CHCR";
		case 0x1f8010bc: return "DMA3 TADR";

		case 0x1f8010c0: return "[SPU]DMA4 MADR";
		case 0x1f8010c4: return BCR_LABEL( "DMA4" );
		case 0x1f8010c6: return "[SPU]DMA4 BCR_count";
		case 0x1f8010c8: return "[SPU]DMA4 CHCR";
		case 0x1f8010cc: return "[SPU]DMA4 TADR";

		case 0x1f8010f0: return "DMA PCR";
		case 0x1f8010f4: return "DMA ICR";
		case 0x1f8010f6: return "DMA ICR_hi";

		case 0x1f801500: return "[SPU2]DMA7 MADR";
		case 0x1f801504: return BCR_LABEL( "DMA7" );
		case 0x1f801506: return "[SPU2]DMA7 BCR_count";
		case 0x1f801508: return "[SPU2]DMA7 CHCR";
		case 0x1f80150C: return "[SPU2]DMA7 TADR";

		case 0x1f801520: return "DMA9 MADR";
		case 0x1f801524: return BCR_LABEL( "DMA9" );
		case 0x1f801526: return "DMA9 BCR_count";
		case 0x1f801528: return "DMA9 CHCR";
		case 0x1f80152C: return "DMA9 TADR";

		case 0x1f801530: return "DMA10 MADR";
		case 0x1f801534: return BCR_LABEL( "DMA10" );
		case 0x1f801536: return "DMA10 BCR_count";
		case 0x1f801538: return "DMA10 CHCR";
		case 0x1f80153c: return "DMA10 TADR";

		case 0x1f801570: return "DMA PCR2";
		case 0x1f801574: return "DMA ICR2";
		case 0x1f801576: return "DMA ICR2_hi";

		case HW_CDR_DATA0:	return "CDROM DATA0";
		case HW_CDR_DATA1:	return "CDROM DATA1";
		case HW_CDR_DATA2:	return "CDROM DATA2";
		case HW_CDR_DATA3:	return "CDROM DATA3";

		case 0x1f80380c:	return "STDOUT";

		// ------------------------------------------------------------------------

		case HW_SIO2_FIFO:	return "SIO2 FIFO";
		case HW_SIO2_CTRL:	return "SIO2 CTRL";
		case HW_SIO2_RECV1:	return "SIO2 RECV1";
		case HW_SIO2_RECV2:	return "SIO2 RECV2";
		case HW_SIO2_RECV3:	return "SIO2 RECV3";
		case HW_SIO2_INTR:	return "SIO2 INTR";
		case 0x1f808278:	return "SIO2 8278";
		case 0x1f80827C:	return "SIO2 827C";

		// ------------------------------------------------------------------------
		// Check for "zoned" registers in the default case.
		// And if all that fails, return "unknown"! :)

		default:
			if( addr >= 0x1f801100 && addr < 0x1f801130 )
			{
				switch( addr & 0xf )
				{
					case 0x0: return "CNT16_COUNT";
					case 0x4: return "CNT16_MODE";
					case 0x8: return "CNT16_TARGET";

					default: return "Invalid Counter";
				}
			}
			else if( addr >= 0x1f801480 && addr < 0x1f8014b0 )
			{
				switch( addr & 0xf )
				{
					case 0x0: return "CNT32_COUNT";
					case 0x2: return "CNT32_COUNT_hi";
					case 0x4: return "CNT32_MODE";
					case 0x8: return "CNT32_TARGET";
					case 0xa: return "CNT32_TARGET_hi";

					default: return "Invalid Counter";
				}
			}
			else if( (addr >= HW_USB_START) && (addr < HW_USB_END) )
			{
				return "USB";
			}
			else if( (addr >= HW_SPU2_START) && (addr < HW_SPU2_END) )
			{
				return "SPU2";
			}
			else if( addr >= pgmsk(HW_FW_START) && addr <= pgmsk(HW_FW_END) )
			{
				return "FW";
			}
			else if( addr >= 0x1f808200 && addr < 0x1f808240 ) { return "SIO2 param"; }
			else if( addr >= 0x1f808240 && addr < 0x1f808260 ) { return "SIO2 send"; }

		return NULL; //"Unknown";
	}
}

template< typename T>
static __ri void IopHwTraceLog( u32 addr, T val, bool mode )
{
	if (!IsDevBuild) return;
	if (!EmuConfig.Trace.IOP.m_EnableRegisters) return;

	FastFormatAscii valStr;
	FastFormatAscii labelStr;
	labelStr.Write("Hw%s%u", mode ? "Read" : "Write", sizeof (T) * 8);

	switch( sizeof T )
	{
	case 1: valStr.Write("0x%02x", val); break;
	case 2: valStr.Write("0x%04x", val); break;
	case 4: valStr.Write("0x%08x", val); break;

	case 8: valStr.Write("0x%08x.%08x", ((u32*)&val)[1], ((u32*)&val)[0]); break;
	case 16: ((u128&)val).WriteTo(valStr);
	}

	static const char* temp = "%-12s @ 0x%08X/%-16s %s %s";

	if( const char* regname = _ioplog_GetHwName<T>( addr, val ) )
		PSXHW_LOG( temp, labelStr.c_str(), addr, regname, mode ? "->" : "<-", valStr.c_str() );
	else
		PSXUnkHW_LOG( temp, labelStr.c_str(), addr, "Unknown", mode ? "->" : "<-", valStr.c_str() );
}

} };
