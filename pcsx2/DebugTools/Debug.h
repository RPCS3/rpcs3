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

#pragma once

extern FILE *emuLog;
extern wxString emuLogName;

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

extern void SourceLog( u16 protocol, u8 source, u32 cpuPc, u32 cpuCycle, const char *fmt, ...);
extern void __Log( const char* fmt, ... );

extern bool SrcLog_CPU( const char* fmt, ... );
extern bool SrcLog_COP0( const char* fmt, ... );

extern bool SrcLog_MEM( const char* fmt, ... );
extern bool SrcLog_HW( const char* fmt, ... );
extern bool SrcLog_DMA( const char* fmt, ... );
extern bool SrcLog_BIOS( const char* fmt, ... );
extern bool SrcLog_VU0( const char* fmt, ... );

extern bool SrcLog_VIF( const char* fmt, ... );
extern bool SrcLog_VIFUNPACK( const char* fmt, ... );
extern bool SrcLog_SPR( const char* fmt, ... );
extern bool SrcLog_GIF( const char* fmt, ... );
extern bool SrcLog_SIF( const char* fmt, ... );
extern bool SrcLog_IPU( const char* fmt, ... );
extern bool SrcLog_VUM( const char* fmt, ... );
extern bool SrcLog_RPC( const char* fmt, ... );
extern bool SrcLog_EECNT( const char* fmt, ... );
extern bool SrcLog_ISOFS( const char* fmt, ... );

extern bool SrcLog_PSXCPU( const char* fmt, ... );
extern bool SrcLog_PSXMEM( const char* fmt, ... );
extern bool SrcLog_PSXHW( const char* fmt, ... );
extern bool SrcLog_PSXBIOS( const char* fmt, ... );
extern bool SrcLog_PSXDMA( const char* fmt, ... );
extern bool SrcLog_PSXCNT( const char* fmt, ... );

extern bool SrcLog_MEMCARDS( const char* fmt, ... );
extern bool SrcLog_PAD( const char* fmt, ... );
extern bool SrcLog_CDR( const char* fmt, ... );
extern bool SrcLog_CDVD( const char* fmt, ... );
extern bool SrcLog_GPU( const char* fmt, ... );
extern bool SrcLog_CACHE( const char* fmt, ... );

// Helper macro for cut&paste.  Note that we intentionally use a top-level *inline* bitcheck
// against Trace.Enabled, to avoid extra overhead in Debug builds when logging is disabled.
#define macTrace EmuConfig.Trace.Enabled && EmuConfig.Trace

#define CPU_LOG			(macTrace.EE.R5900())		&& SrcLog_CPU
#define MEM_LOG			(macTrace.EE.Memory())		&& SrcLog_MEM
#define CACHE_LOG		(macTrace.EE.Cache)			&& SrcLog_CACHE
#define HW_LOG			(macTrace.EE.KnownHw())		&& SrcLog_HW
#define UnknownHW_LOG	(macTrace.EE.KnownHw())		&& SrcLog_HW
#define DMA_LOG			(macTrace.EE.DMA())			&& SrcLog_DMA

#define BIOS_LOG		(macTrace.EE.Bios())		&& SrcLog_BIOS
#define VU0_LOG			(macTrace.EE.VU0())			&& SrcLog_VU0
#define SysCtrl_LOG		(macTrace.EE.SysCtrl())		&& SrcLog_COP0
#define COP0_LOG		(macTrace.EE.COP0())		&& SrcLog_COP0
#define VIF_LOG			(macTrace.EE.VIF())			&& SrcLog_VIF
#define SPR_LOG			(macTrace.EE.SPR())			&& SrcLog_SPR
#define GIF_LOG			(macTrace.EE.GIF())			&& SrcLog_GIF
#define SIF_LOG			(macTrace.SIF)				&& SrcLog_SIF
#define IPU_LOG			(macTrace.EE.IPU())			&& SrcLog_IPU
#define VUM_LOG			(macTrace.EE.COP2())		&& SrcLog_VUM

#define EECNT_LOG		(macTrace.EE.Counters())	&& SrcLog_EECNT
#define VIFUNPACK_LOG	(macTrace.EE.VIFunpack())	&& SrcLog_VIFUNPACK


#define PSXCPU_LOG		(macTrace.IOP.R3000A())		&& SrcLog_PSXCPU
#define PSXMEM_LOG		(macTrace.IOP.Memory())		&& SrcLog_PSXMEM
#define PSXHW_LOG		(macTrace.IOP.KnownHw())	&& SrcLog_PSXHW
#define PSXUnkHW_LOG	(macTrace.IOP.UnknownHw())	&& SrcLog_PSXHW
#define PSXBIOS_LOG		(macTrace.IOP.Bios())		&& SrcLog_PSXBIOS
#define PSXDMA_LOG		(macTrace.IOP.DMA())		&& SrcLog_PSXDMA
#define PSXCNT_LOG		(macTrace.IOP.Counters())	&& SrcLog_PSXCNT

#define MEMCARDS_LOG	(macTrace.IOP.Memcards())	&& SrcLog_MEMCARDS
#define PAD_LOG			(macTrace.IOP.PAD())		&& SrcLog_PAD
#define GPU_LOG			(macTrace.IOP.GPU())		&& SrcLog_GPU
#define CDVD_LOG		(macTrace.IOP.CDVD())		&& SrcLog_CDVD

#else // PCSX2_DEVBUILD

#define CPU_LOG  0&&
#define MEM_LOG  0&&
#define HW_LOG   0&&
#define DMA_LOG  0&&
#define BIOS_LOG 0&&
#define FPU_LOG  0&&
#define MMI_LOG  0&&
#define VU0_LOG  0&&
#define COP0_LOG 0&&
#define SysCtrl_LOG 0&&
#define UnknownHW_LOG 0&&
#define VIF_LOG  0&&
#define VIFUNPACK_LOG  0&&
#define SPR_LOG  0&&
#define GIF_LOG  0&&
#define SIF_LOG  0&&
#define IPU_LOG  0&&
#define VUM_LOG  0&&
#define ISOFS_LOG  0&&

#define PSXCPU_LOG  0&&
#define PSXMEM_LOG  0&&
#define PSXHW_LOG   0&&
#define PSXUnkHW_LOG   0&&
#define PSXBIOS_LOG 0&&
#define PSXDMA_LOG  0&&

#define PAD_LOG  0&&
#define CDR_LOG  0&&
#define CDVD_LOG  0&&
#define GPU_LOG  0&&
#define PSXCNT_LOG 0&&
#define EECNT_LOG 0&&

#define EMU_LOG 0&&
#define CACHE_LOG 0&&
#define MEMCARDS_LOG 0&&
#endif

#define ELF_LOG  (EmuConfig.Log.ELF) && DevCon.WriteLn
