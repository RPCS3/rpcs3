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
#include "IopCommon.h"

#include "System/PageFaultSource.h"
#include "Utilities/EventSource.inl"

// Includes needed for cleanup, since we don't have a good system (yet) for
// cleaning up these things.
#include "sVU_zerorec.h"
#include "GameDatabase.h"
#include "Elfheader.h"

extern void closeNewVif(int idx);
extern void resetNewVif(int idx);

template class EventSource< IEventListener_PageFault >;

SrcType_PageFault Source_PageFault;

EventListener_PageFault::EventListener_PageFault()
{
	Source_PageFault.Add( *this );
}

EventListener_PageFault::~EventListener_PageFault() throw()
{
	Source_PageFault.Remove( *this );
}

void SrcType_PageFault::Dispatch( const PageFaultInfo& params )
{
	m_handled = false;
	_parent::Dispatch( params );
}

void SrcType_PageFault::_DispatchRaw( ListenerIterator iter, const ListenerIterator& iend, const PageFaultInfo& evt )
{
	do {
		(*iter)->DispatchEvent( evt, m_handled );
	} while( (++iter != iend) && !m_handled );
}


#if _MSC_VER
#	include "svnrev.h"
#endif

const Pcsx2Config EmuConfig;

// Provides an accessor for quick modification of GS options.  All GS options are allowed to be
// changed "on the fly" by the *main/gui thread only*.
Pcsx2Config::GSOptions& SetGSConfig()
{
	//DbgCon.WriteLn( "Direct modification of EmuConfig.GS detected" );
	AffinityAssert_AllowFrom_MainUI();
	return const_cast<Pcsx2Config::GSOptions&>(EmuConfig.GS);
}

// Provides an accessor for quick modification of Recompiler options.
// Used by loadGameSettings() to set clamp modes via database at game startup.
Pcsx2Config::RecompilerOptions& SetRecompilerConfig()
{
	//DbgCon.WriteLn( "Direct modification of EmuConfig.Gamefixes detected" );
	AffinityAssert_AllowFrom_MainUI();
	return const_cast<Pcsx2Config::RecompilerOptions&>(EmuConfig.Cpu.Recompiler);
}

// Provides an accessor for quick modification of Gamefix options.
// Used by loadGameSettings() to set gamefixes via database at game startup.
Pcsx2Config::GamefixOptions& SetGameFixConfig()
{
	//DbgCon.WriteLn( "Direct modification of EmuConfig.Gamefixes detected" );
	AffinityAssert_AllowFrom_MainUI();
	return const_cast<Pcsx2Config::GamefixOptions&>(EmuConfig.Gamefixes);
}

ConsoleLogFilters& SetConsoleConfig()
{
	//DbgCon.WriteLn( "Direct modification of EmuConfig.Log detected" );
	AffinityAssert_AllowFrom_MainUI();
	return const_cast<ConsoleLogFilters&>(EmuConfig.Log);
}

TraceLogFilters& SetTraceConfig()
{
	//DbgCon.WriteLn( "Direct modification of EmuConfig.TraceLog detected" );
	AffinityAssert_AllowFrom_MainUI();
	return const_cast<TraceLogFilters&>(EmuConfig.Trace);
}


// This function should be called once during program execution.
void SysLogMachineCaps()
{
	Console.WriteLn( Color_StrongGreen, "PCSX2 %u.%u.%u.r%d %s - compiled on " __DATE__, PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo,
		SVN_REV, SVN_MODS ? "(modded)" : ""
	);

	Console.WriteLn( "Savestate version: 0x%x", g_SaveVersion);
	Console.Newline();

	Console.WriteLn( Color_StrongBlack, "x86-32 Init:" );

	u32 speed = x86caps.CalculateMHz();

	Console.Indent().WriteLn(
		L"CPU vendor name  =  %s\n"
		L"FamilyID         =  %x\n"
		L"x86Family        =  %s\n"
		L"CPU speed        =  %d.%03d ghz\n"
		L"Cores            =  %d physical [%d logical]\n"
		L"x86PType         =  %s\n"
		L"x86Flags         =  %8.8x %8.8x\n"
		L"x86EFlags        =  %8.8x",
			fromUTF8( x86caps.VendorName ).c_str(), x86caps.StepID,
			fromUTF8( x86caps.FamilyName ).Trim().Trim(false).c_str(),
			speed / 1000, speed % 1000,
			x86caps.PhysicalCores, x86caps.LogicalCores,
			x86caps.GetTypeName().c_str(),
			x86caps.Flags, x86caps.Flags2,
			x86caps.EFlags
	);

	Console.Newline();

	wxArrayString features[2];	// 2 lines, for readability!

	if( x86caps.hasMultimediaExtensions )			features[0].Add( L"MMX" );
	if( x86caps.hasStreamingSIMDExtensions )		features[0].Add( L"SSE" );
	if( x86caps.hasStreamingSIMD2Extensions )		features[0].Add( L"SSE2" );
	if( x86caps.hasStreamingSIMD3Extensions )		features[0].Add( L"SSE3" );
	if( x86caps.hasSupplementalStreamingSIMD3Extensions ) features[0].Add( L"SSSE3" );
	if( x86caps.hasStreamingSIMD4Extensions )		features[0].Add( L"SSE4.1" );
	if( x86caps.hasStreamingSIMD4Extensions2 )		features[0].Add( L"SSE4.2" );

	if( x86caps.hasMultimediaExtensionsExt )		features[1].Add( L"MMX2  " );
	if( x86caps.has3DNOWInstructionExtensions )		features[1].Add( L"3DNOW " );
	if( x86caps.has3DNOWInstructionExtensionsExt )	features[1].Add( L"3DNOW2" );
	if( x86caps.hasStreamingSIMD4ExtensionsA )		features[1].Add( L"SSE4a " );

	wxString result[2];
	JoinString( result[0], features[0], L".. " );
	JoinString( result[1], features[1], L".. " );

	Console.WriteLn( Color_StrongBlack,	L"x86 Features Detected:" );
	Console.Indent().WriteLn( result[0] + (result[1].IsEmpty() ? L"" : (L"\n" + result[1])) );
	Console.Newline();
}

template< typename CpuType >
class CpuInitializer
{
public:
	ScopedPtr<CpuType>			MyCpu;
	ScopedPtr<BaseException>	ExThrown;
	
	CpuInitializer();
	virtual ~CpuInitializer() throw();

	bool IsAvailable() const
	{
		return !!MyCpu;
	}

	CpuType* GetPtr() { return MyCpu.GetPtr(); }
	const CpuType* GetPtr() const { return MyCpu.GetPtr(); }

	operator CpuType*() { return GetPtr(); }
	operator const CpuType*() const { return GetPtr(); }
};

// --------------------------------------------------------------------------------------
//  CpuInitializer Template
// --------------------------------------------------------------------------------------
// Helper for initializing various PCSX2 CPU providers, and handing errors and cleanup.
//
template< typename CpuType >
CpuInitializer< CpuType >::CpuInitializer()
{
	try {
		MyCpu = new CpuType();
		MyCpu->Allocate();
	}
	catch( Exception::RuntimeError& ex )
	{
		Console.Error( L"CPU provider error:\n\t" + ex.FormatDiagnosticMessage() );
		MyCpu = NULL;
		ExThrown = ex.Clone();
	}
	catch( std::runtime_error& ex )
	{
		Console.Error( L"CPU provider error (STL Exception)\n\tDetails:" + fromUTF8( ex.what() ) );
		MyCpu = NULL;
		ExThrown = new Exception::RuntimeError(ex);
	}
}

template< typename CpuType >
CpuInitializer< CpuType >::~CpuInitializer() throw()
{
	if( MyCpu )
		MyCpu->Shutdown();
}

class CpuInitializerSet
{
public:
	// Note: Allocate sVU first -- it's the most picky.

	CpuInitializer<recSuperVU0>		superVU0;
	CpuInitializer<recSuperVU1>		superVU1;

	CpuInitializer<recMicroVU0>		microVU0;
	CpuInitializer<recMicroVU1>		microVU1;

	CpuInitializer<InterpVU0>		interpVU0;
	CpuInitializer<InterpVU1>		interpVU1;

public:
	CpuInitializerSet() {}
	virtual ~CpuInitializerSet() throw() {}
};



// returns the translated error message for the Virtual Machine failing to allocate!
static wxString GetMemoryErrorVM()
{
	return pxE( ".Error:EmuCore::MemoryForVM",
		L"PCSX2 is unable to allocate memory needed for the PS2 virtual machine. "
		L"Close out some memory hogging background tasks and try again."
	);
}

SysCoreAllocations::SysCoreAllocations()
{
	InstallSignalHandler();

	Console.WriteLn( "Initializing PS2 virtual machine..." );

	try
	{
		vtlb_Core_Alloc();
		memAlloc();
		psxMemAlloc();
		vuMicroMemAlloc();
	}
	// ----------------------------------------------------------------------------
	catch( Exception::OutOfMemory& ex )
	{
		ex.UserMsg() += L"\n\n" + GetMemoryErrorVM();
		CleanupMess();
		throw;
	}
	catch( std::bad_alloc& ex )
	{
		CleanupMess();

		// re-throw std::bad_alloc as something more friendly.  This is needed since
		// much of the code uses new/delete internally, which throw std::bad_alloc on fail.

		throw Exception::OutOfMemory()
			.SetDiagMsg(
				L"std::bad_alloc caught while trying to allocate memory for the PS2 Virtual Machine.\n"
				L"Error Details: " + fromUTF8( ex.what() )
			)
			.SetUserMsg(GetMemoryErrorVM()); 	// translated
	}

	Console.WriteLn( "Allocating memory for recompilers..." );

	CpuProviders = new CpuInitializerSet();

	try {
		recCpu.Allocate();
	}
	catch( Exception::RuntimeError& ex )
	{
		m_RecExceptionEE = ex.Clone();
		Console.Error( L"EE Recompiler Allocation Failed:\n" + ex.FormatDiagnosticMessage() );
		recCpu.Shutdown();
	}

	try {
		psxRec.Allocate();
	}
	catch( Exception::RuntimeError& ex )
	{
		m_RecExceptionIOP = ex.Clone();
		Console.Error( L"IOP Recompiler Allocation Failed:\n" + ex.FormatDiagnosticMessage() );
		psxRec.Shutdown();
	}

	// hmm! : VU0 and VU1 pre-allocations should do sVU and mVU separately?  Sounds complicated. :(

	// If both VUrecs failed, then make sure the SuperVU is totally closed out, because it
	// actually initializes everything once and then shares it between both VU recs.
	if( !IsRecAvailable_SuperVU0() && !IsRecAvailable_SuperVU1() )
		SuperVUDestroy( -1 );
}

bool SysCoreAllocations::IsRecAvailable_MicroVU0() const { return CpuProviders->microVU0.IsAvailable(); }
bool SysCoreAllocations::IsRecAvailable_MicroVU1() const { return CpuProviders->microVU1.IsAvailable(); }
BaseException* SysCoreAllocations::GetException_MicroVU0() const { return CpuProviders->microVU0.ExThrown; }
BaseException* SysCoreAllocations::GetException_MicroVU1() const { return CpuProviders->microVU1.ExThrown; }

bool SysCoreAllocations::IsRecAvailable_SuperVU0() const { return CpuProviders->superVU0.IsAvailable(); }
bool SysCoreAllocations::IsRecAvailable_SuperVU1() const { return CpuProviders->superVU1.IsAvailable(); }
BaseException* SysCoreAllocations::GetException_SuperVU0() const { return CpuProviders->superVU0.ExThrown; }
BaseException* SysCoreAllocations::GetException_SuperVU1() const { return CpuProviders->superVU1.ExThrown; }


void SysCoreAllocations::CleanupMess() throw()
{
	try
	{
		closeNewVif(0);
		closeNewVif(1);

		// Special SuperVU "complete" terminator (stupid hacky recompiler)
		SuperVUDestroy( -1 );

		psxRec.Shutdown();
		recCpu.Shutdown();

		vuMicroMemShutdown();
		psxMemShutdown();
		memShutdown();
		vtlb_Core_Shutdown();
	}
	DESTRUCTOR_CATCHALL
}

SysCoreAllocations::~SysCoreAllocations() throw()
{
	CleanupMess();
}

bool SysCoreAllocations::HadSomeFailures( const Pcsx2Config::RecompilerOptions& recOpts ) const
{
	return	(recOpts.EnableEE && !IsRecAvailable_EE()) ||
			(recOpts.EnableIOP && !IsRecAvailable_IOP()) ||
			(recOpts.EnableVU0 && recOpts.UseMicroVU0 && !IsRecAvailable_MicroVU0()) ||
			(recOpts.EnableVU1 && recOpts.UseMicroVU0 && !IsRecAvailable_MicroVU1()) ||
			(recOpts.EnableVU0 && !recOpts.UseMicroVU0 && !IsRecAvailable_SuperVU0()) ||
			(recOpts.EnableVU1 && !recOpts.UseMicroVU1 && !IsRecAvailable_SuperVU1());

}

BaseVUmicroCPU* CpuVU0 = NULL;
BaseVUmicroCPU* CpuVU1 = NULL;

void SysCoreAllocations::SelectCpuProviders() const
{
	Cpu		= CHECK_EEREC	? &recCpu : &intCpu;
	psxCpu	= CHECK_IOPREC	? &psxRec : &psxInt;

	CpuVU0 = CpuProviders->interpVU0;
	CpuVU1 = CpuProviders->interpVU1;

	if( EmuConfig.Cpu.Recompiler.EnableVU0 )
		CpuVU0 = EmuConfig.Cpu.Recompiler.UseMicroVU0 ? (BaseVUmicroCPU*)CpuProviders->microVU0 : (BaseVUmicroCPU*)CpuProviders->superVU0;

	if( EmuConfig.Cpu.Recompiler.EnableVU1 )
		CpuVU1 = EmuConfig.Cpu.Recompiler.UseMicroVU1 ? (BaseVUmicroCPU*)CpuProviders->microVU1 : (BaseVUmicroCPU*)CpuProviders->superVU1;
}


// Resets all PS2 cpu execution caches, which does not affect that actual PS2 state/condition.
// This can be called at any time outside the context of a Cpu->Execute() block without
// bad things happening (recompilers will slow down for a brief moment since rec code blocks
// are dumped).
// Use this method to reset the recs when important global pointers like the MTGS are re-assigned.
void SysClearExecutionCache()
{
	GetSysCoreAlloc().SelectCpuProviders();

	// SuperVUreset will do nothing is none of the recs are initialized.
	// But it's needed if one or the other is initialized.
	SuperVUReset(-1);

	Cpu->Reset();
	psxCpu->Reset();
	CpuVU0->Reset();
	CpuVU1->Reset();

	resetNewVif(0);
	resetNewVif(1);
}

// Maps a block of memory for use as a recompiled code buffer, and ensures that the
// allocation is below a certain memory address (specified in "bounds" parameter).
// The allocated block has code execution privileges.
// Returns NULL on allocation failure.
u8* SysMmapEx(uptr base, u32 size, uptr bounds, const char *caller)
{
	u8* Mem = (u8*)HostSys::Mmap( base, size );

	if( (Mem == NULL) || (bounds != 0 && (((uptr)Mem + size) > bounds)) )
	{
		if( base != NULL )
		{
			DbgCon.Warning( "First try failed allocating %s at address 0x%x", caller, base );

			// memory allocation *must* have the top bit clear, so let's try again
			// with NULL (let the OS pick something for us).

			SafeSysMunmap( Mem, size );

			Mem = (u8*)HostSys::Mmap( NULL, size );
		}

		if( bounds != 0 && (((uptr)Mem + size) > bounds) )
		{
			DevCon.Warning( "Second try failed allocating %s, block ptr 0x%x does not meet required criteria.", caller, Mem );
			SafeSysMunmap( Mem, size );

			// returns NULL, caller should throw an exception.
		}
	}
	return Mem;
}

// This function always returns a valid DiscID -- using the Sony serial when possible, and
// falling back on the CRC checksum of the ELF binary if the PS2 software being run is
// homebrew or some other serial-less item.
wxString SysGetDiscID()
{
	if( !DiscSerial.IsEmpty() ) return DiscSerial;
	
	if( !ElfCRC )
	{
		// FIXME: If the system is currently running the BIOS, it should return a serial based on
		// the BIOS being run (either a checksum of the BIOS roms, and/or a string based on BIOS
		// region and revision).
	}

	return wxsFormat( L"%8.8x", ElfCRC );
}
