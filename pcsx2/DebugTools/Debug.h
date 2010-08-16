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

#include "Utilities/TraceLog.h"

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

// this structure uses old fashioned C-style "polymorphism".  The base struct TraceLogDescriptor
// must always be the first member in the struct.
struct SysTraceLogDescriptor
{
	TraceLogDescriptor	base;
	const char*			Prefix;
};

// --------------------------------------------------------------------------------------
//  SysTraceLog
// --------------------------------------------------------------------------------------
// Default trace log for high volume VM/System logging.
// This log dumps to emuLog.txt directly and has no ability to pipe output
// to the console (due to the console's inability to handle extremely high
// logging volume).
class SysTraceLog : public TextFileTraceLog
{
public:
	const char*		PrePrefix;

public:
	TraceLog_ImplementBaseAPI(SysTraceLog)

	// Pass me a NULL and you *will* suffer!  Muahahaha.
	SysTraceLog( const SysTraceLogDescriptor* desc )
		: TextFileTraceLog( &desc->base ) {}

	void DoWrite( const char *fmt ) const;

	SysTraceLog& SetPrefix( const char* name )
	{
		PrePrefix = name;
		return *this;
	}

};

class SysTraceLog_EE : public SysTraceLog
{
	typedef SysTraceLog _parent;

public:
	SysTraceLog_EE( const SysTraceLogDescriptor* desc ) : _parent( desc ) {}

	void ApplyPrefix( FastFormatAscii& ascii ) const;
	bool IsActive() const
	{
		return EmuConfig.Trace.Enabled && Enabled && EmuConfig.Trace.EE.m_EnableAll;
	}
	
	wxString GetCategory() const { return L"EE"; }
};

class SysTraceLog_VIFcode : public SysTraceLog_EE
{
	typedef SysTraceLog_EE _parent;

public:
	SysTraceLog_VIFcode( const SysTraceLogDescriptor* desc ) : _parent( desc ) {}

	void ApplyPrefix( FastFormatAscii& ascii ) const;
};

class SysTraceLog_EE_Disasm : public SysTraceLog_EE
{
	typedef SysTraceLog_EE _parent;

public:
	SysTraceLog_EE_Disasm( const SysTraceLogDescriptor* desc ) : _parent( desc ) {}

	bool IsActive() const
	{
		return _parent::IsActive() && EmuConfig.Trace.EE.m_EnableDisasm;
	}

	wxString GetCategory() const { return _parent::GetCategory() + L".Disasm"; }
};

class SysTraceLog_EE_Registers : public SysTraceLog_EE
{
	typedef SysTraceLog_EE _parent;

public:
	SysTraceLog_EE_Registers( const SysTraceLogDescriptor* desc ) : _parent( desc ) {}

	bool IsActive() const
	{
		return _parent::IsActive() && EmuConfig.Trace.EE.m_EnableRegisters;
	}

	wxString GetCategory() const { return _parent::GetCategory() + L".Registers"; }
};

class SysTraceLog_EE_Events : public SysTraceLog_EE
{
	typedef SysTraceLog_EE _parent;

public:
	SysTraceLog_EE_Events( const SysTraceLogDescriptor* desc ) : _parent( desc ) {}

	bool IsActive() const
	{
		return _parent::IsActive() && EmuConfig.Trace.EE.m_EnableEvents;
	}

	wxString GetCategory() const { return _parent::GetCategory() + L".Events"; }
};


class SysTraceLog_IOP : public SysTraceLog
{
	typedef SysTraceLog _parent;

public:
	SysTraceLog_IOP( const SysTraceLogDescriptor* desc ) : _parent( desc ) {}

	void ApplyPrefix( FastFormatAscii& ascii ) const;
	bool IsActive() const
	{
		return EmuConfig.Trace.Enabled && Enabled && EmuConfig.Trace.IOP.m_EnableAll;
	}

	wxString GetCategory() const { return L"IOP"; }
};

class SysTraceLog_IOP_Disasm : public SysTraceLog_IOP
{
	typedef SysTraceLog_IOP _parent;

public:
	SysTraceLog_IOP_Disasm( const SysTraceLogDescriptor* desc ) : _parent( desc ) {}
	bool IsActive() const
	{
		return _parent::IsActive() && EmuConfig.Trace.IOP.m_EnableDisasm;
	}

	wxString GetCategory() const { return _parent::GetCategory() + L".Disasm"; }
};

class SysTraceLog_IOP_Registers : public SysTraceLog_IOP
{
	typedef SysTraceLog_IOP _parent;

public:
	SysTraceLog_IOP_Registers( const SysTraceLogDescriptor* desc ) : _parent( desc ) {}
	bool IsActive() const
	{
		return _parent::IsActive() && EmuConfig.Trace.IOP.m_EnableRegisters;
	}

	wxString GetCategory() const { return _parent::GetCategory() + L".Registers"; }
};

class SysTraceLog_IOP_Events : public SysTraceLog_IOP
{
	typedef SysTraceLog_IOP _parent;

public:
	SysTraceLog_IOP_Events( const SysTraceLogDescriptor* desc ) : _parent( desc ) {}
	bool IsActive() const
	{
		return _parent::IsActive() && EmuConfig.Trace.IOP.m_EnableEvents;
	}

	wxString GetCategory() const { return _parent::GetCategory() + L".Events"; }
};

// --------------------------------------------------------------------------------------
//  ConsoleLogFromVM
// --------------------------------------------------------------------------------------
// Special console logger for Virtual Machine log sources, such as the EE and IOP console
// writes (actual game developer messages and such).  These logs do *not* automatically
// append newlines, since the VM generates them manually; and they do *not* support printf
// formatting, since anything coming over the EE/IOP consoles should be considered raw
// string data.  (otherwise %'s would get mis-interpreted).
//
template< ConsoleColors conColor >
class ConsoleLogFromVM : public BaseTraceLogSource
{
	typedef BaseTraceLogSource _parent;

public:
	ConsoleLog_ImplementBaseAPI(ConsoleLogFromVM)

	ConsoleLogFromVM( const TraceLogDescriptor* desc ) : _parent( desc ) {}

	bool Write( const wxChar* msg ) const
	{
		ConsoleColorScope cs(conColor);
		Console.WriteRaw( msg );
		return false;
	}
};

// --------------------------------------------------------------------------------------
//  SysTraceLogPack
// --------------------------------------------------------------------------------------
struct SysTraceLogPack
{
	// TODO : Sif has special logging needs.. ?
	SysTraceLog	SIF;

	struct EE_PACK
	{
		SysTraceLog_EE				Bios;
		SysTraceLog_EE				Memory;
		SysTraceLog_EE				GIFtag;
		SysTraceLog_VIFcode			VIFcode;

		SysTraceLog_EE_Disasm		R5900;
		SysTraceLog_EE_Disasm		COP0;
		SysTraceLog_EE_Disasm		COP1;
		SysTraceLog_EE_Disasm		COP2;
		SysTraceLog_EE_Disasm		Cache;
		
		SysTraceLog_EE_Registers	KnownHw;
		SysTraceLog_EE_Registers	UnknownHw;
		SysTraceLog_EE_Registers	DMAhw;
		SysTraceLog_EE_Registers	IPU;

		SysTraceLog_EE_Events		DMAC;
		SysTraceLog_EE_Events		Counters;
		SysTraceLog_EE_Events		SPR;

		SysTraceLog_EE_Events		VIF;
		SysTraceLog_EE_Events		GIF;

		EE_PACK();
	} EE;
	
	struct IOP_PACK
	{
		SysTraceLog_IOP				Bios;
		SysTraceLog_IOP				Memcards;
		SysTraceLog_IOP				PAD;

		SysTraceLog_IOP_Disasm		R3000A;
		SysTraceLog_IOP_Disasm		COP2;
		SysTraceLog_IOP_Disasm		Memory;

		SysTraceLog_IOP_Registers	KnownHw;
		SysTraceLog_IOP_Registers	UnknownHw;
		SysTraceLog_IOP_Registers	DMAhw;

		// TODO items to be added, or removed?  I can't remember which! --air
		//SysTraceLog_IOP_Registers	SPU2;
		//SysTraceLog_IOP_Registers	USB;
		//SysTraceLog_IOP_Registers	FW;

		SysTraceLog_IOP_Events		DMAC;
		SysTraceLog_IOP_Events		Counters;
		SysTraceLog_IOP_Events		CDVD;

		IOP_PACK();
	} IOP;

	SysTraceLogPack();
};

struct SysConsoleLogPack
{
	ConsoleLogSource		ELF;
	ConsoleLogSource		eeRecPerf;

	ConsoleLogFromVM<Color_Cyan>		eeConsole;
	ConsoleLogFromVM<Color_Yellow>		iopConsole;
	ConsoleLogFromVM<Color_Cyan>		deci2;

	SysConsoleLogPack();
};


extern SysTraceLogPack SysTrace;
extern SysConsoleLogPack SysConsole;


// Helper macro for cut&paste.  Note that we intentionally use a top-level *inline* bitcheck
// against Trace.Enabled, to avoid extra overhead in Debug builds when logging is disabled.
// (specifically this allows debug builds to skip havingto resolve all the parameters being
//  passed into the function)
#define macTrace(trace)	SysTrace.trace.IsActive() && SysTrace.trace.Write


#ifdef PCSX2_DEVBUILD

extern void __Log( const char* fmt, ... );

#define SIF_LOG			macTrace(SIF)

#define BIOS_LOG		macTrace(EE.Bios)
#define CPU_LOG			macTrace(EE.R5900)
#define COP0_LOG		macTrace(EE.COP0)
#define VUM_LOG			macTrace(EE.COP2)
#define MEM_LOG			macTrace(EE.Memory)
#define CACHE_LOG		macTrace(EE.Cache)
#define HW_LOG			macTrace(EE.KnownHw)
#define UnknownHW_LOG	macTrace(EE.UnknownHw)
#define DMA_LOG			macTrace(EE.DMAhw)
#define IPU_LOG			macTrace(EE.IPU)
#define VIF_LOG			macTrace(EE.VIF)
#define SPR_LOG			macTrace(EE.SPR)
#define GIF_LOG			macTrace(EE.GIF)
#define EECNT_LOG		macTrace(EE.Counters)
#define VifCodeLog		macTrace(EE.VIFcode)


#define PSXBIOS_LOG		macTrace(IOP.Bios)
#define PSXCPU_LOG		macTrace(IOP.R3000A)
#define PSXMEM_LOG		macTrace(IOP.Memory)
#define PSXHW_LOG		macTrace(IOP.KnownHw)
#define PSXUnkHW_LOG	macTrace(IOP.UnknownHw)
#define PSXDMA_LOG		macTrace(IOP.DMAhw)
#define PSXCNT_LOG		macTrace(IOP.Counters)
#define MEMCARDS_LOG	macTrace(IOP.Memcards)
#define PAD_LOG			macTrace(IOP.PAD)
#define GPU_LOG			macTrace(IOP.GPU)
#define CDVD_LOG		macTrace(IOP.CDVD)

#else // PCSX2_DEVBUILD

#define CPU_LOG		0&&
#define MEM_LOG		0&&
#define HW_LOG		0&&
#define DMA_LOG		0&&
#define BIOS_LOG	0&&
#define VU0_LOG		0&&
#define COP0_LOG	0&&
#define UnknownHW_LOG 0&&
#define VIF_LOG		0&&
#define SPR_LOG		0&&
#define GIF_LOG		0&&
#define SIF_LOG		0&&
#define IPU_LOG		0&&
#define VUM_LOG		0&&
#define VifCodeLog	0&&

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

#define ELF_LOG			SysConsole.ELF.IsActive() && SysConsole.ELF.Write
#define eeRecPerfLog	SysConsole.eeRecPerf.IsActive() && SysConsole.eeRecPerf
#define eeConLog		SysConsole.eeConsole.IsActive() && SysConsole.eeConsole.Write
#define eeDeci2Log		SysConsole.deci2.IsActive() && SysConsole.deci2.Write
#define iopConLog		SysConsole.iopConsole.IsActive() && SysConsole.iopConsole.Write
