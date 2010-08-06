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

SysTraceLogPack SysTracePack;
SysConsoleLogPack SysConsolePack;

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
	ascii.Write( "%-4s(%8.8lx %8.8lx): ", PrePrefix, cpuRegs.pc, cpuRegs.cycle );
}

void SysTraceLog_IOP::ApplyPrefix( FastFormatAscii& ascii ) const
{
	ascii.Write( "%-4s(%8.8lx %8.8lx): ", PrePrefix, psxRegs.pc, psxRegs.cycle );
}

void SysTraceLog_VIFcode::ApplyPrefix( FastFormatAscii& ascii ) const
{
	_parent::ApplyPrefix(ascii);
	ascii.Write( "vifCode_" );
}


SysConsoleLogPack::SysConsoleLogPack()
{
	// Console Logging
	
	ELF.SetShortName(L"ELF")
		.SetName(L"ELF")
		.SetDescription(wxLt("Dumps detailed information for PS2 executables (ELFs)."));

	eeRecPerf.SetShortName(L"EErecPerf")
		.SetName(L"EErec Performance")
		.SetDescription(wxLt("Logs manual protection, split blocks, and other things that might impact performance."))
		.SetColor(Color_Gray);

	eeConsole.SetShortName(L"EEout")
		.SetName(L"EE Console")
		.SetDescription(wxLt("Shows the game developer's logging text (EE processor)"))
		.SetColor(Color_Cyan);

	iopConsole.SetShortName(L"IOPout")
		.SetName(L"IOP Console")
		.SetDescription(wxLt("Shows the game developer's logging text (IOP processor)"))
		.SetColor(Color_Yellow);
		
	deci2.SetShortName(L"DECI2")
		.SetName(L"DECI2 Console")
		.SetDescription(wxLt("Shows DECI2 debugging logs (EE processor)"))
		.SetColor(Color_Cyan);
}
	
SysTraceLogPack::SysTraceLogPack()
{
	SIF			.SetName(L"SIF (EE <-> IOP)")
				.SetShortName(L"SIF")
				.SetPrefix("SIF")
				.SetDescription(wxLt(""));

	// EmotionEngine (EE/R5900)

	EE.Bios		.SetName(L"Bios")
				.SetPrefix("EE")
				.SetDescription(wxLt("SYSCALL and DECI2 activity."));

	EE.Memory	.SetName(L"Memory")
				.SetPrefix("eMem")
				.SetDescription(wxLt("Direct memory accesses to unknown or unmapped EE memory space."));

	EE.R5900	.SetName(L"R5900 Core")
				.SetShortName(L"R5900")
				.SetPrefix("eDis")
				.SetDescription(wxLt("Disasm of executing core instructions (excluding COPs and CACHE)."));

	EE.COP0		.SetName(L"COP0")
				.SetPrefix("eDis")
				.SetDescription(wxLt("Disasm of COP0 instructions (MMU, cpu and dma status, etc)."));

	EE.COP1		.SetName(L"COP1/FPU")
				.SetShortName(L"FPU")
				.SetPrefix("eDis")
				.SetDescription(wxLt("Disasm of the EE's floating point unit (FPU) only."));

	EE.COP2		.SetName(L"COP2/VUmacro")
				.SetShortName(L"VUmacro")
				.SetPrefix("eDis")
				.SetDescription(wxLt("Disasm of the EE's VU0macro co-processor instructions."));

	EE.Cache	.SetName(L"Cache")
				.SetPrefix("eDis")
				.SetDescription(wxLt("Execution of EE cache instructions."));

	EE.KnownHw	.SetName(L"Hardware Regs")
				.SetShortName(L"HardwareRegs")
				.SetPrefix("eReg")
				.SetDescription(wxLt("All known hardware register accesses (very slow!); not including sub filter options below."));

	EE.UnknownHw.SetName(L"Unknown Regs")
				.SetShortName(L"UnknownRegs")
				.SetPrefix("eReg")
				.SetDescription(wxLt("Logs only unknown, unmapped, or unimplemented register accesses."));

	EE.DMAhw	.SetName(L"DMA Regs")
				.SetShortName(L"DmaRegs")
				.SetPrefix("eReg")
				.SetDescription(wxLt("Logs only DMA-related registers."));

	EE.IPU		.SetName(L"IPU")
				.SetPrefix("IPU")
				.SetDescription(wxLt("IPU activity: hardware registers, decoding operations, DMA status, etc."));

	EE.GIFtag	.SetName(L"GIFtags")
				.SetPrefix("GIF")
				.SetDescription(wxLt("All GIFtag parse activity; path index, tag type, etc."));

	EE.VIFcode	.SetName(L"VIFcodes")
				.SetPrefix("VIF")
				.SetDescription(wxLt("All VIFcode processing; command, tag style, interrupts."));

	EE.SPR		.SetName(L"Scratchpad MFIFO")
				.SetShortName(L"ScratchpadMFIFO")
				.SetPrefix("SPR")
				.SetDescription(wxLt("Scratchpad's MFIFO activity."));

	EE.DMAC		.SetName(L"DMA Controller")
				.SetShortName(L"DmaController")
				.SetPrefix("eDmaC")
				.SetDescription(wxLt("Actual data transfer logs, bus right arbitration, stalls, etc."));

	EE.Counters	.SetName(L"Counters")
				.SetPrefix("eCnt")
				.SetDescription(wxLt("Tracks all EE counters events and some counter register activity."));

	EE.VIF		.SetName(L"VIF")
				.SetPrefix("VIF")
				.SetDescription(wxLt("Dumps various VIF and VIFcode processing data."));

	EE.GIF		.SetName(L"GIF")
				.SetPrefix("GIF")
				.SetDescription(wxLt("Dumps various GIF and GIFtag parsing data."));

	// IOP - Input / Output Processor

	IOP.Bios	.SetName(L"Bios")
				.SetPrefix("IOP")
				.SetDescription(wxLt("SYSCALL and IRX activity."));

	IOP.Memory	.SetName(L"Memory")
				.SetPrefix("iMem")
				.SetDescription(wxLt("Direct memory accesses to unknown or unmapped IOP memory space."));

	IOP.R3000A	.SetName(L"R3000A Core")
				.SetShortName(L"R3000A")
				.SetPrefix("iDis")
				.SetDescription(wxLt("Disasm of executing core instructions (excluding COPs and CACHE)."));

	IOP.COP2	.SetName(L"COP2/GPU")
				.SetShortName(L"COP2")
				.SetPrefix("iDis")
				.SetDescription(wxLt("Disasm of the IOP's GPU co-processor instructions."));

	IOP.KnownHw	.SetName(L"Hardware Regs")
				.SetShortName(L"HardwareRegs")
				.SetPrefix("iReg")
				.SetDescription(wxLt("All known hardware register accesses, not including the sub-filters below."));

	IOP.UnknownHw.SetName(L"Unknown Regs")
				.SetShortName(L"UnknownRegs")
				.SetPrefix("iReg")
				.SetDescription(wxLt("Logs only unknown, unmapped, or unimplemented register accesses."));

	IOP.DMAhw	.SetName(L"DMA Regs")
				.SetShortName(L"DmaRegs")
				.SetPrefix("iReg")
				.SetDescription(wxLt("Logs only DMA-related registers."));

	IOP.Memcards.SetName(L"Memorycards")
				.SetPrefix("Mcd")
				.SetDescription(wxLt("Memorycard reads, writes, erases, terminators, and other processing."));

	IOP.PAD		.SetName(L"Pad")
				.SetPrefix("Pad")
				.SetDescription(wxLt("Gamepad activity on the SIO."));

	IOP.DMAC	.SetName(L"DMA Controller")
				.SetShortName(L"DmaController")
				.SetPrefix("iDmaC")
				.SetDescription(wxLt("Actual DMA event processing and data transfer logs."));

	IOP.Counters.SetName(L"Counters")
				.SetPrefix("iCnt")
				.SetDescription(wxLt("Tracks all IOP counters events and some counter register activity."));

	IOP.CDVD	.SetName(L"CDVD")
				.SetPrefix("CDVD")
				.SetDescription(wxLt("Detailed logging of CDVD hardware."));
}

