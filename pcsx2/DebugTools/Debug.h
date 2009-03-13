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


#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "Pcsx2Config.h"

extern FILE *emuLog;

extern char* disVU0MicroUF(u32 code, u32 pc);
extern char* disVU0MicroLF(u32 code, u32 pc);
extern char* disVU1MicroUF(u32 code, u32 pc);
extern char* disVU1MicroLF(u32 code, u32 pc);

extern const char * const CP2VFnames[];
extern const char * const disRNameCP2f[];
extern const char * const disRNameCP2i[];

namespace R5900
{
	// [TODO] : These function names can be de-obfuscated with the help of a little namespace love.

	void disR5900F( std::string& output, u32 code );
	void disR5900Fasm( std::string& output, u32 code, u32 pc);
	void disR5900AddSym(u32 addr, const char *name);
	const char* disR5900GetSym(u32 addr);
	const char* disR5900GetUpperSym(u32 addr);
	void disR5900FreeSyms();
	void dFindSym( std::string& output, u32 addr );

	extern const char * const disRNameGPR[];

	// A helper class for getting a quick and efficient string representation of the
	// R5900's current instruction.  This class is *not* thread safe!
	class DisR5900CurrentState
	{
	protected:
		std::string result;

	public:
		const std::string& getString();
		const char* getCString();
	};

	extern DisR5900CurrentState disR5900Current;
}

namespace R3000A
{
	extern void (*IOP_DEBUG_BSC[64])(char *buf);

	extern const char * const disRNameGPR[];
	extern char* disR3000Fasm(u32 code, u32 pc);
	extern char* disR3000AF(u32 code, u32 pc);
}

#ifdef PCSX2_DEVBUILD

struct LogSources
{
	bool
		R5900:1,		// instructions and exception vectors for the R5900 (EE)
		R3000A:1,	// instructions and exception vectors for the R3000a (IOP)
	
		Memory:1,	// memory accesses (loads and stores)
		Hardware:1,
		DMA:1,
		Bios:1,
		ELF:1,
		VU0:1,
		COP0:1,		// TLB logs, PERF logs, Debug register logs
		VIF:1,
		SPR:1,		// Scratchpad
		GIF:1,
		SIF:1,
		IPU:1,
		VUM:1,		// VU memory access logs
		RPC:1,
		Counters:1,	// EE's counters!
		
		IopMemory:1,
		IopHardware:1,
		IopBios:1,
		IopDMA:1,
		IopCnt:1,
		Memcards:1,
		Pad:1,
		CDVD:1,
		GPU:1,		// PS1's GPU (currently unimplemented)
		LogToConsole:1;
};

extern LogSources varLog;

void SourceLog( u16 protocol, u8 source, u32 cpuPc, u32 cpuCycle, const char *fmt, ...);
void __Log( const char* fmt, ... );

extern bool SrcLog_CPU( const char* fmt, ... );
extern bool SrcLog_COP0( const char* fmt, ... );

extern bool SrcLog_MEM( const char* fmt, ... );
extern bool SrcLog_HW( const char* fmt, ... );
extern bool SrcLog_DMA( const char* fmt, ... );
extern bool SrcLog_BIOS( const char* fmt, ... );
extern bool SrcLog_ELF( const char* fmt, ... );
extern bool SrcLog_VU0( const char* fmt, ... );

extern bool SrcLog_VIF( const char* fmt, ... );
extern bool SrcLog_SPR( const char* fmt, ... );
extern bool SrcLog_GIF( const char* fmt, ... );
extern bool SrcLog_SIF( const char* fmt, ... );
extern bool SrcLog_IPU( const char* fmt, ... );
extern bool SrcLog_VUM( const char* fmt, ... );
extern bool SrcLog_RPC( const char* fmt, ... );
extern bool SrcLog_EECNT( const char* fmt, ... );

extern bool SrcLog_PSXCPU( const char* fmt, ... );
extern bool SrcLog_PSXMEM( const char* fmt, ... );
extern bool SrcLog_PSXHW( const char* fmt, ... );
extern bool SrcLog_PSXBIOS( const char* fmt, ... );
extern bool SrcLog_PSXDMA( const char* fmt, ... );
extern bool SrcLog_PSXCNT( const char* fmt, ... );

extern bool SrcLog_MEMCARDS( const char* fmt, ... );
extern bool SrcLog_PAD( const char* fmt, ... );
extern bool SrcLog_CDR( const char* fmt, ... );
extern bool SrcLog_GPU( const char* fmt, ... );

#define CPU_LOG  (varLog.R5900) && SrcLog_CPU
#define MEM_LOG  (varLog.Memory) && SrcLog_MEM
#define HW_LOG   (varLog.Hardware) && SrcLog_HW
#define DMA_LOG  (varLog.DMA) && SrcLog_DMA
#define BIOS_LOG (varLog.Bios) && SrcLog_BIOS
#define ELF_LOG  (varLog.ELF) && SrcLog_ELF
#define VU0_LOG  (varLog.VU0) && SrcLog_VU0
#define COP0_LOG (varLog.COP0) && SrcLog_COP0
#define VIF_LOG  (varLog.VIF) && SrcLog_VIF
#define SPR_LOG  (varLog.SPR) && SrcLog_SPR
#define GIF_LOG  (varLog.GIF) && SrcLog_GIF
#define SIF_LOG  (varLog.SIF) && SrcLog_SIF
#define IPU_LOG  (varLog.IPU) && SrcLog_IPU
#define VUM_LOG  (varLog.VUM) && SrcLog_VUM
#define RPC_LOG  (varLog.RPC) && SrcLog_RPC
#define EECNT_LOG (varLog.Counters) && SrcLog_EECNT

#define PSXCPU_LOG  (varLog.R3000A) && SrcLog_PSXCPU
#define PSXMEM_LOG  (varLog.IopMemory) && SrcLog_PSXMEM
#define PSXHW_LOG   (varLog.IopHardware) && SrcLog_PSXHW
#define PSXBIOS_LOG (varLog.IopBios) && SrcLog_PSXBIOS
#define PSXDMA_LOG  (varLog.IopDMA) && SrcLog_PSXDMA
#define PSXCNT_LOG  (varLog.IopCnt) && SrcLog_PSXCNT

//memcard has the same number as PAD_LOG for now
#define MEMCARDS_LOG (varLog.Memcards) && SrcLog_MEMCARDS
#define PAD_LOG  (varLog.Pad) && SrcLog_PAD
#define CDR_LOG  (varLog.CDVD) && SrcLog_CDR
#define GPU_LOG  (varLog.GPU) && SrcLog_GPU

// fixme - currently we don't log cache
#define CACHE_LOG 0&&

#else // PCSX2_DEVBUILD

#define varLog 0

#define CPU_LOG  0&&
#define MEM_LOG  0&&
#define HW_LOG   0&&
#define DMA_LOG  0&&
#define BIOS_LOG 0&&
#define ELF_LOG  0&&
#define FPU_LOG  0&&
#define MMI_LOG  0&&
#define VU0_LOG  0&&
#define COP0_LOG 0&&
#define VIF_LOG  0&&
#define SPR_LOG  0&&
#define GIF_LOG  0&&
#define SIF_LOG  0&&
#define IPU_LOG  0&&
#define VUM_LOG  0&&
#define RPC_LOG  0&&

#define PSXCPU_LOG  0&&
#define PSXMEM_LOG  0&&
#define PSXHW_LOG   0&&
#define PSXBIOS_LOG 0&&
#define PSXDMA_LOG  0&&

#define PAD_LOG  0&&
#define GTE_LOG  0&&
#define CDR_LOG  0&&
#define GPU_LOG  0&&
#define PSXCNT_LOG 0&&
#define EECNT_LOG 0&&

#define EMU_LOG 0&&
#define CACHE_LOG 0&&
#define MEMCARDS_LOG 0&&
#endif

#ifdef VIFUNPACKDEBUG
#define VIFUNPACK_LOG VIF_LOG
#else
#define VIFUNPACK_LOG 0&&
#endif

#endif /* __DEBUG_H__ */
