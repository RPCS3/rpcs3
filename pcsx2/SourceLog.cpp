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
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
 
#include "PrecompiledHeader.h"

#ifdef _WIN32
#include "RDebug/deci2.h"
#else
#include <sys/time.h>
#endif

#include <cstdarg>
#include <ctype.h>

#include "R3000A.h"
#include "iR5900.h"
#include "System.h"
#include "DebugTools/Debug.h"

using namespace R5900;

FILE *emuLog;

#ifdef PCSX2_DEVBUILD
u32 varLog;

// these used by the depreciated _old_Log only
u16 logProtocol;
u8 logSource;
#endif

int connected=0;

#define SYNC_LOGGING

// writes text directly to the logfile, no newlines appended.
void __Log( const char* fmt, ... )
{
	char tmp[2024];

	va_list list;
	va_start(list, fmt);

	// concatenate the log message after the prefix:
	int length = vsprintf(tmp, fmt, list);
	va_end( list );

	assert( length <= 2020 );
	if( length > 2020 )
	{
		Msgbox::Alert("Source Log Stack Corruption Detected.  Program execution may become unstable.");
		// fixme: should throw an exception here once we have proper exception handling implemented.
	}

	if (varLog & 0x80000000)		// log to console enabled?
	{
		Console::Write(tmp);

	} 
	else if( emuLog != NULL )		// manually write to the logfile.
	{
		fputs( tmp, emuLog );
		//fputs( "\r\n", emuLog );
		fflush( emuLog );
	}
}

static __forceinline void _vSourceLog( u16 protocol, u8 source, u32 cpuPc, u32 cpuCycle, const char *fmt, va_list list )
{
	char tmp[2024];

	int prelength = sprintf( tmp, "%c/%8.8lx %8.8lx: ", source, cpuPc, cpuCycle );

	// concatenate the log message after the prefix:
	int length = vsprintf(&tmp[prelength], fmt, list);
	assert( length <= 2020 );
	if( length > 2020 )
	{
		Msgbox::Alert("Source Log Stack Corruption Detected.  Program execution may become unstable.");
		// fixme: should throw an exception here once we have proper exception handling implemented.
	}

#ifdef PCSX2_DEVBUILD
#ifdef _WIN32
	// Send log data to the (remote?) debugger.
	if (connected && logProtocol>=0 && logProtocol<0x10)
	{
		sendTTYP(logProtocol, logSource, tmp);
	}
#endif
#endif

	if (varLog & 0x80000000)		// log to console enabled?
	{
		Console::WriteLn(tmp);

	} else if( emuLog != NULL )		// manually write to the logfile.
	{
		fputs( tmp, emuLog );
		//fputs( "\r\n", emuLog );
		fflush( emuLog );
	}
}

// Note: This function automatically appends a newline character always!
// (actually it doesn't yet because too much legacy code, will fix soon!)
void SourceLog( u16 protocol, u8 source, u32 cpuPc, u32 cpuCycle, const char *fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	_vSourceLog( protocol, source, cpuPc, cpuCycle, fmt, list );
	va_end(list);
}

// Functions with variable argument lists can't be inlined.
#define IMPLEMENT_SOURCE_LOG( unit, source, protocol ) \
	bool SrcLog_##unit( const char* fmt, ... ) \
	{ \
		va_list list; \
		va_start( list, fmt ); \
		_vSourceLog( protocol, source, \
			(source == 'E') ? cpuRegs.pc : psxRegs.pc, \
			(source == 'E') ? cpuRegs.cycle : psxRegs.cycle, fmt, list ); \
		va_end( list ); \
		return false; \
	} \

IMPLEMENT_SOURCE_LOG( EECNT, 'E', 0 ) 
IMPLEMENT_SOURCE_LOG( BIOS, 'E', 0 ) 

IMPLEMENT_SOURCE_LOG( CPU, 'E', 1 ) 
IMPLEMENT_SOURCE_LOG( FPU, 'E', 1 ) 
IMPLEMENT_SOURCE_LOG( MMI, 'E', 1 ) 
IMPLEMENT_SOURCE_LOG( COP0, 'E', 1 )

IMPLEMENT_SOURCE_LOG( MEM, 'E', 6 )
IMPLEMENT_SOURCE_LOG( HW, 'E', 6 ) 
IMPLEMENT_SOURCE_LOG( DMA, 'E', 5 ) 
IMPLEMENT_SOURCE_LOG( ELF, 'E', 7 ) 
IMPLEMENT_SOURCE_LOG( VU0, 'E', 2 ) 
IMPLEMENT_SOURCE_LOG( VIF, 'E', 3 ) 
IMPLEMENT_SOURCE_LOG( SPR, 'E', 7 ) 
IMPLEMENT_SOURCE_LOG( GIF, 'E', 4 ) 
IMPLEMENT_SOURCE_LOG( SIF, 'E', 9 ) 
IMPLEMENT_SOURCE_LOG( IPU, 'E', 8 ) 
IMPLEMENT_SOURCE_LOG( VUM, 'E', 2 ) 
IMPLEMENT_SOURCE_LOG( RPC, 'E', 9 ) 

IMPLEMENT_SOURCE_LOG( PSXCPU, 'I', 1 ) 
IMPLEMENT_SOURCE_LOG( PSXMEM, 'I', 6 ) 
IMPLEMENT_SOURCE_LOG( PSXHW, 'I', 2 ) 
IMPLEMENT_SOURCE_LOG( PSXBIOS, 'I', 0 ) 
IMPLEMENT_SOURCE_LOG( PSXDMA, 'I', 5 ) 
IMPLEMENT_SOURCE_LOG( PSXCNT, 'I', 4 ) 

IMPLEMENT_SOURCE_LOG( MEMCARDS, 'I', 7 ) 
IMPLEMENT_SOURCE_LOG( PAD, 'I', 7 ) 
IMPLEMENT_SOURCE_LOG( GTE, 'I', 3 ) 
IMPLEMENT_SOURCE_LOG( CDR, 'I', 8 ) 




