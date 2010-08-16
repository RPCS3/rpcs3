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

// --------------------------------------------------------------------------------------
//  Source / Tracre Logging  (high volume logging facilities)
// --------------------------------------------------------------------------------------
// This module defines functions for performing high-volume diagnostic trace logging.
// Only ASCII versions of these logging functions are provided.  Translated messages are
// not supported, and typically all logs are written to disk (ASCII), thus making the
// ASCII versions the more efficient option.

#include "PrecompiledHeader.h"

#ifndef _WIN32
#	include <sys/time.h>
#endif

#include <cstdarg>
#include <ctype.h>

#include "R3000A.h"
#include "iR5900.h"
#include "System.h"
#include "DebugTools/Debug.h"

using namespace R5900;

FILE *emuLog;
wxString emuLogName;

SysTraceLogPack SysTrace;
SysConsoleLogPack SysConsole;

typedef void Fntype_SrcLogPrefix( FastFormatAscii& dest );

// writes text directly to the logfile, no newlines appended.
void __Log( const char* fmt, ... )
{
	va_list list;
	va_start(list, fmt);

	if( emuLog != NULL )
	{
		fputs( FastFormatAscii().WriteV(fmt,list), emuLog );
		fputs( "\n", emuLog );
		fflush( emuLog );
	}

	va_end( list );
}

void SysTraceLog::DoWrite( const char *msg ) const
{
	if( emuLog == NULL ) return;

	fputs( msg, emuLog );
	fputs( "\n", emuLog );
	fflush( emuLog );
}

void SysTraceLog_EE::ApplyPrefix( FastFormatAscii& ascii ) const
{
	ascii.Write( "%-4s(%8.8lx %8.8lx): ", ((SysTraceLogDescriptor*)m_Descriptor)->Prefix, cpuRegs.pc, cpuRegs.cycle );
}

void SysTraceLog_IOP::ApplyPrefix( FastFormatAscii& ascii ) const
{
	ascii.Write( "%-4s(%8.8lx %8.8lx): ", ((SysTraceLogDescriptor*)m_Descriptor)->Prefix, psxRegs.pc, psxRegs.cycle );
}

void SysTraceLog_VIFcode::ApplyPrefix( FastFormatAscii& ascii ) const
{
	_parent::ApplyPrefix(ascii);
	ascii.Write( "vifCode_" );
}

// --------------------------------------------------------------------------------------
//  SysConsoleLogPack  (descriptions)
// --------------------------------------------------------------------------------------
static const TraceLogDescriptor

TLD_ELF = {
	L"ELF",			L"ELF",
	wxLt("Dumps detailed information for PS2 executables (ELFs).")
}, 

TLD_eeRecPerf = {
	L"EErecPerf",	L"EErec Performance",
	wxLt("Logs manual protection, split blocks, and other things that might impact performance.")
},

TLD_eeConsole = {
	L"EEout",		L"EE Console",
	wxLt("Shows the game developer's logging text (EE processor)")
},

TLD_iopConsole = {
	L"IOPout",		L"IOP Console",
	wxLt("Shows the game developer's logging text (IOP processor)")
},

TLD_deci2 = {
	L"DECI2",		L"DECI2 Console",
	wxLt("Shows DECI2 debugging logs (EE processor)")
};

SysConsoleLogPack::SysConsoleLogPack()
	: ELF		(&TLD_ELF, Color_Gray)
	, eeRecPerf	(&TLD_eeRecPerf, Color_Gray)
	, eeConsole	(&TLD_eeConsole)
	, iopConsole(&TLD_iopConsole)
	, deci2		(&TLD_deci2)
{
}

// --------------------------------------------------------------------------------------
//  SysTraceLogPack  (descriptions)
// --------------------------------------------------------------------------------------
static const SysTraceLogDescriptor
TLD_SIF = {
	L"SIF",			L"SIF (EE <-> IOP)",
	wxLt(""),
	"SIF"
};

// ----------------------------
//   EmotionEngine (EE/R5900)
// ----------------------------

static const SysTraceLogDescriptor
TLD_EE_Bios = {
	L"Bios",		L"Bios",
	wxLt("SYSCALL and DECI2 activity."),
	"EE"
},

TLD_EE_Memory = {
	L"Memory",		L"Memory",
	wxLt("Direct memory accesses to unknown or unmapped EE memory space."),
	"eMem"
},

TLD_EE_R5900 = {
	L"R5900",		L"R5900 Core",
	wxLt("Disasm of executing core instructions (excluding COPs and CACHE)."),
	"eDis"
},

TLD_EE_COP0 = {
	L"COP0",		L"COP0",
	wxLt("Disasm of COP0 instructions (MMU, cpu and dma status, etc)."),
	"eDis"
},

TLD_EE_COP1 = {
	L"FPU",			L"COP1/FPU",
	wxLt("Disasm of the EE's floating point unit (FPU) only."),
	"eDis"
},

TLD_EE_COP2 = {
	L"VUmacro",		L"COP2/VUmacro",
	wxLt("Disasm of the EE's VU0macro co-processor instructions."),
	"eDis"
},

TLD_EE_Cache = {
	L"Cache",		L"Cache",
	wxLt("Execution of EE cache instructions."),
	"eDis"
},

TLD_EE_KnownHw = {
	L"HwRegs",		L"Hardware Regs",
	wxLt("All known hardware register accesses (very slow!); not including sub filter options below."),
	"eReg"
},

TLD_EE_UnknownHw = {
	L"UnknownRegs",	L"Unknown Regs",
	wxLt("Logs only unknown, unmapped, or unimplemented register accesses."),
	"eReg"
},

TLD_EE_DMAhw = {
	L"DmaRegs",		L"DMA Regs",
	wxLt("Logs only DMA-related registers."),
	"eReg"
},

TLD_EE_IPU = {
	L"IPU",			L"IPU",
	wxLt("IPU activity: hardware registers, decoding operations, DMA status, etc."),
	"IPU"
},

TLD_EE_GIFtag = {
	L"GIFtags",		L"GIFtags",
	wxLt("All GIFtag parse activity; path index, tag type, etc."),
	"GIF"
},

TLD_EE_VIFcode = {
	L"VIFcodes",	L"VIFcodes",
	wxLt("All VIFcode processing; command, tag style, interrupts."),
	"VIF"
},

TLD_EE_SPR = {
	L"MFIFO",		L"Scratchpad MFIFO",
	wxLt("Scratchpad's MFIFO activity."),
	"SPR"
},

TLD_EE_DMAC = {
	L"DmaCtrl",		L"DMA Controller",
	wxLt("Actual data transfer logs, bus right arbitration, stalls, etc."),
	"eDmaC"
},

TLD_EE_Counters = {
	L"Counters",	L"Counters",
	wxLt("Tracks all EE counters events and some counter register activity."),
	"eCnt"
},

TLD_EE_VIF = {
	L"VIF",			L"VIF",
	wxLt("Dumps various VIF and VIFcode processing data."),
	"VIF"
},

TLD_EE_GIF = {
	L"GIF",			L"GIF",
	wxLt("Dumps various GIF and GIFtag parsing data."),
	"GIF"
};

// ----------------------------------
//   IOP - Input / Output Processor
// ----------------------------------

static const SysTraceLogDescriptor
TLD_IOP_Bios = {
	L"Bios",		L"Bios",
	wxLt("SYSCALL and IRX activity."),
	"IOP"
},

TLD_IOP_Memory = {
	L"Memory",		L"Memory",
	wxLt("Direct memory accesses to unknown or unmapped IOP memory space."),
	"iMem"
},

TLD_IOP_R3000A = {
	L"R3000A",		L"R3000A Core",
	wxLt("Disasm of executing core instructions (excluding COPs and CACHE)."),
	"iDis"
},

TLD_IOP_COP2 = {
	L"COP2/GPU",		L"COP2",
	wxLt("Disasm of the IOP's GPU co-processor instructions."),
	"iDis"
},

TLD_IOP_KnownHw = {
	L"HwRegs",			L"Hardware Regs",
	wxLt("All known hardware register accesses, not including the sub-filters below."),
	"iReg"
},

TLD_IOP_UnknownHw = {
	L"UnknownRegs",		L"Unknown Regs",
	wxLt("Logs only unknown, unmapped, or unimplemented register accesses."),
	"iReg"
},

TLD_IOP_DMAhw = {
	L"DmaRegs",			L"DMA Regs",
	wxLt("Logs only DMA-related registers."),
	"iReg"
},

TLD_IOP_Memcards = {
	L"Memorycards",		L"Memorycards",
	wxLt("Memorycard reads, writes, erases, terminators, and other processing."),
	"Mcd"
},

TLD_IOP_PAD = {
	L"Pad",				L"Pad",
	wxLt("Gamepad activity on the SIO."),
	"Pad"
},

TLD_IOP_DMAC = {
	L"DmaCrl",			L"DMA Controller",
	wxLt("Actual DMA event processing and data transfer logs."),
	"iDmaC"
},

TLD_IOP_Counters = {
	L"Counters",		L"Counters",
	wxLt("Tracks all IOP counters events and some counter register activity."),
	"iCnt"
},

TLD_IOP_CDVD = {
	L"CDVD",			L"CDVD",
	wxLt("Detailed logging of CDVD hardware."),
	"CDVD"
};

SysTraceLogPack::SysTraceLogPack()
	: SIF		(&TLD_SIF)
{
}

SysTraceLogPack::EE_PACK::EE_PACK()
	: Bios		(&TLD_EE_Bios)
	, Memory	(&TLD_EE_Memory)
	, GIFtag	(&TLD_EE_GIFtag)
	, VIFcode	(&TLD_EE_VIFcode)

	, R5900		(&TLD_EE_R5900)
	, COP0		(&TLD_EE_COP0)
	, COP1		(&TLD_EE_COP1)
	, COP2		(&TLD_EE_COP2)
	, Cache		(&TLD_EE_Cache)
		
	, KnownHw	(&TLD_EE_KnownHw)
	, UnknownHw	(&TLD_EE_UnknownHw)
	, DMAhw		(&TLD_EE_DMAhw)
	, IPU		(&TLD_EE_IPU)

	, DMAC		(&TLD_EE_DMAC)
	, Counters	(&TLD_EE_Counters)
	, SPR		(&TLD_EE_SPR)

	, VIF		(&TLD_EE_VIF)
	, GIF		(&TLD_EE_GIF)
{
}

SysTraceLogPack::IOP_PACK::IOP_PACK()
	: Bios		(&TLD_IOP_Bios)
	, Memcards	(&TLD_IOP_Memcards)
	, PAD		(&TLD_IOP_PAD)

	, R3000A	(&TLD_IOP_R3000A)
	, COP2		(&TLD_IOP_COP2)
	, Memory	(&TLD_IOP_Memory)

	, KnownHw	(&TLD_IOP_KnownHw)
	, UnknownHw	(&TLD_IOP_UnknownHw)
	, DMAhw		(&TLD_IOP_DMAhw)

	, DMAC		(&TLD_IOP_DMAC)
	, Counters	(&TLD_IOP_Counters)
	, CDVD		(&TLD_IOP_CDVD)
{
}