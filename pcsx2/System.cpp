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
#include "VUmicro.h"
#include "newVif.h"

#include "SamplProf.h"

#include "Elfheader.h"

#include "System/RecTypes.h"

#include "Utilities/MemsetFast.inl"


// --------------------------------------------------------------------------------------
//  RecompiledCodeReserve  (implementations)
// --------------------------------------------------------------------------------------

// Constructor!
// Parameters:
//   name - a nice long name that accurately describes the contents of this reserve.
RecompiledCodeReserve::RecompiledCodeReserve( const wxString& name, uint defCommit )
	: BaseVmReserveListener( name )
{
	m_blocksize		= (1024 * 128) / __pagesize;
	m_prot_mode		= PageAccess_Any();
	m_def_commit	= defCommit / __pagesize;
	
	m_profiler_registered = false;
}

RecompiledCodeReserve::~RecompiledCodeReserve() throw()
{
	_termProfiler();
}

void RecompiledCodeReserve::_registerProfiler()
{
	if (m_profiler_name.IsEmpty() || !IsOk()) return;
	ProfilerRegisterSource( m_profiler_name, m_baseptr, GetReserveSizeInBytes() );
	m_profiler_registered = true;
}

void RecompiledCodeReserve::_termProfiler()
{
	if (m_profiler_registered)
		ProfilerTerminateSource( m_profiler_name );
}

uint RecompiledCodeReserve::_calcDefaultCommitInBlocks() const
{
	return (m_def_commit + m_blocksize - 1) / m_blocksize;
}

void* RecompiledCodeReserve::Reserve( size_t size, uptr base, uptr upper_bounds )
{
	if (!_parent::Reserve(size, base, upper_bounds)) return NULL;
	_registerProfiler();
	return m_baseptr;
}


// Sets the abbreviated name used by the profiler.  Name should be under 10 characters long.
// After a name has been set, a profiler source will be automatically registered and cleared
// in accordance with changes in the reserve area.
RecompiledCodeReserve& RecompiledCodeReserve::SetProfilerName( const wxString& shortname )
{
	m_profiler_name = shortname;
	_registerProfiler();
	return *this;
}

void RecompiledCodeReserve::DoCommitAndProtect( uptr page )
{
	CommitBlocks(page, (m_pages_commited || !m_def_commit) ? 1 : _calcDefaultCommitInBlocks() );
}

void RecompiledCodeReserve::OnCommittedBlock( void* block )
{
	if (IsDevBuild)
	{
		// Clear the recompiled code block to 0xcc (INT3) -- this helps disasm tools show
		// the assembly dump more cleanly.  We don't clear the block on Release builds since
		// it can add a noticeable amount of overhead to large block recompilations.

		memset_sse_a<0xcc>( block, m_blocksize * __pagesize );
	}
}

// This error message is shared by R5900, R3000, and microVU recompilers.  It is not used by the
// SuperVU recompiler, since it has its own customized message.
void RecompiledCodeReserve::ThrowIfNotOk() const
{
	if (IsOk()) return;

	throw Exception::OutOfMemory(m_name)
		.SetDiagMsg(pxsFmt( L"Recompiled code cache could not be mapped." ))
		.SetUserMsg( pxE( L"This recompiler was unable to reserve contiguous memory required for internal caches.  This error can be caused by low virtual memory resources, such as a small or disabled swapfile, or by another program that is hogging a lot of memory."
		));
}


void SysOutOfMemory_EmergencyResponse(uptr blocksize)
{
	// An out of memory error occurred.  All we can try to do in response is reset the various
	// recompiler caches (which can sometimes total over 120megs, so it can be quite helpful).
	// If the user is using interpreters, or if the memory allocation failure was on a very small
	// allocation, then this code could fail; but that's fine.  We're already trying harder than
	// 99.995% of all programs ever written. -- air

	if (Cpu)
	{
		Cpu->SetCacheReserve( (Cpu->GetCacheReserve() * 2) / 3 );
		Cpu->Reset();
	}

	if (CpuVU0)
	{
		CpuVU0->SetCacheReserve( (CpuVU0->GetCacheReserve() * 2) / 3 );
		CpuVU0->Reset();
	}

	if (CpuVU1)
	{
		CpuVU1->SetCacheReserve( (CpuVU1->GetCacheReserve() * 2) / 3 );
		CpuVU1->Reset();
	}

	if (psxCpu)
	{
		psxCpu->SetCacheReserve( (psxCpu->GetCacheReserve() * 2) / 3 );
		psxCpu->Reset();
	}
}


#include "svnrev.h"

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

TraceLogFilters& SetTraceConfig()
{
	//DbgCon.WriteLn( "Direct modification of EmuConfig.TraceLog detected" );
	AffinityAssert_AllowFrom_MainUI();
	return const_cast<TraceLogFilters&>(EmuConfig.Trace);
}


// This function should be called once during program execution.
void SysLogMachineCaps()
{
	if ( !PCSX2_isReleaseVersion )
	{
		Console.WriteLn(Color_StrongGreen, "PCSX2 %u.%u.%u-%lld %s - compiled on " __DATE__,
			PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo,
			SVN_REV, SVN_MODS ? "(modded)" : ""
			);
	}
	else { // shorter release version string
		Console.WriteLn(Color_StrongGreen, "PCSX2 %u.%u.%u-%lld - compiled on " __DATE__,
			PCSX2_VersionHi, PCSX2_VersionMid, PCSX2_VersionLo,
			SVN_REV );
	}

	Console.WriteLn( "Savestate version: 0x%x", g_SaveVersion);
	Console.Newline();

	Console.WriteLn( Color_StrongBlack, "Host Machine Init:" );

	Console.Indent().WriteLn(
		L"Operating System =  %s\n"
		L"Physical RAM     =  %u MB",

		GetOSVersionString().c_str(),
		(u32)(GetPhysicalMemory() / _1mb)
	);

	u32 speed = x86caps.CalculateMHz();

	Console.Indent().WriteLn(
		L"CPU name         =  %s\n"
		L"Vendor/Model     =  %s (stepping %02X)\n"
		L"CPU speed        =  %u.%03u ghz (%u logical thread%s)\n"
		L"x86PType         =  %s\n"
		L"x86Flags         =  %08x %08x\n"
		L"x86EFlags        =  %08x",
			fromUTF8( x86caps.FamilyName ).Trim().Trim(false).c_str(),
			fromUTF8( x86caps.VendorName ).c_str(), x86caps.StepID,
			speed / 1000, speed % 1000,
			x86caps.LogicalCores, (x86caps.LogicalCores==1) ? L"" : L"s",
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
	if( x86caps.hasAVX )							features[0].Add( L"AVX" );
	if( x86caps.hasFMA)								features[0].Add( L"FMA" );

	if( x86caps.hasMultimediaExtensionsExt )		features[1].Add( L"MMX2  " );
	if( x86caps.has3DNOWInstructionExtensions )		features[1].Add( L"3DNOW " );
	if( x86caps.has3DNOWInstructionExtensionsExt )	features[1].Add( L"3DNOW2" );
	if( x86caps.hasStreamingSIMD4ExtensionsA )		features[1].Add( L"SSE4a " );

	const wxString result[2] =
	{
		JoinString( features[0], L".. " ),
		JoinString( features[1], L".. " )
	};

	Console.WriteLn( Color_StrongBlack,	L"x86 Features Detected:" );
	Console.Indent().WriteLn( result[0] + (result[1].IsEmpty() ? L"" : (L"\n" + result[1])) );
	Console.Newline();
}

template< typename CpuType >
class CpuInitializer
{
public:
	ScopedPtr<CpuType>			MyCpu;
	ScopedExcept	ExThrown;
	
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
		MyCpu->Reserve();
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
	if (MyCpu)
		MyCpu->Shutdown();
}

// --------------------------------------------------------------------------------------
//  CpuInitializerSet
// --------------------------------------------------------------------------------------
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
	return pxE( L"PCSX2 is unable to allocate memory needed for the PS2 virtual machine. Close out some memory hogging background tasks and try again."
	);
}

// --------------------------------------------------------------------------------------
//  SysReserveVM  (implementations)
// --------------------------------------------------------------------------------------
SysMainMemory::SysMainMemory()
{
}

SysMainMemory::~SysMainMemory() throw()
{
	ReleaseAll();
}

void SysMainMemory::ReserveAll()
{
	pxInstallSignalHandler();

	DevCon.WriteLn( Color_StrongBlue, "Mapping host memory for virtual systems..." );
	ConsoleIndentScope indent(1);

	m_ee.Reserve();
	m_iop.Reserve();
	m_vu.Reserve();
	
	reserveNewVif(0);
	reserveNewVif(1);
}

void SysMainMemory::CommitAll()
{
	vtlb_Core_Alloc();
	if (m_ee.IsCommitted() && m_iop.IsCommitted() && m_vu.IsCommitted()) return;

	DevCon.WriteLn( Color_StrongBlue, "Allocating host memory for virtual systems..." );
	ConsoleIndentScope indent(1);

	m_ee.Commit();
	m_iop.Commit();
	m_vu.Commit();
}


void SysMainMemory::ResetAll()
{
	CommitAll();

	DevCon.WriteLn( Color_StrongBlue, "Resetting host memory for virtual systems..." );
	ConsoleIndentScope indent(1);

	m_ee.Reset();
	m_iop.Reset();
	m_vu.Reset();

	// Note: newVif is reset as part of other VIF structures.
}

void SysMainMemory::DecommitAll()
{
	if (!m_ee.IsCommitted() && !m_iop.IsCommitted() && !m_vu.IsCommitted()) return;

	Console.WriteLn( Color_Blue, "Decommitting host memory for virtual systems..." );
	ConsoleIndentScope indent(1);

	m_ee.Decommit();
	m_iop.Decommit();
	m_vu.Decommit();

	closeNewVif(0);
	closeNewVif(1);
	
	vtlb_Core_Free();
}

void SysMainMemory::ReleaseAll()
{
	DecommitAll();

	Console.WriteLn( Color_Blue, "Releasing host memory maps for virtual systems..." );
	ConsoleIndentScope indent(1);

	vtlb_Core_Free();		// Just to be sure... (calling order could result in it getting missed during Decommit).

	releaseNewVif(0);
	releaseNewVif(1);

	m_ee.Decommit();
	m_iop.Decommit();
	m_vu.Decommit();

	safe_delete(Source_PageFault);
}


// --------------------------------------------------------------------------------------
//  SysCpuProviderPack  (implementations)
// --------------------------------------------------------------------------------------
SysCpuProviderPack::SysCpuProviderPack()
{
	Console.WriteLn( Color_StrongBlue, "Reserving memory for recompilers..." );
	ConsoleIndentScope indent(1);

	CpuProviders = new CpuInitializerSet();

	try {
		recCpu.Reserve();
	}
	catch( Exception::RuntimeError& ex )
	{
		m_RecExceptionEE = ex.Clone();
		Console.Error( L"EE Recompiler Reservation Failed:\n" + ex.FormatDiagnosticMessage() );
		recCpu.Shutdown();
	}

	try {
		psxRec.Reserve();
	}
	catch( Exception::RuntimeError& ex )
	{
		m_RecExceptionIOP = ex.Clone();
		Console.Error( L"IOP Recompiler Reservation Failed:\n" + ex.FormatDiagnosticMessage() );
		psxRec.Shutdown();
	}

	// hmm! : VU0 and VU1 pre-allocations should do sVU and mVU separately?  Sounds complicated. :(

	if (newVifDynaRec)
	{
		dVifReserve(0);
		dVifReserve(1);
	}
}

bool SysCpuProviderPack::IsRecAvailable_MicroVU0() const { return CpuProviders->microVU0.IsAvailable(); }
bool SysCpuProviderPack::IsRecAvailable_MicroVU1() const { return CpuProviders->microVU1.IsAvailable(); }
BaseException* SysCpuProviderPack::GetException_MicroVU0() const { return CpuProviders->microVU0.ExThrown; }
BaseException* SysCpuProviderPack::GetException_MicroVU1() const { return CpuProviders->microVU1.ExThrown; }

bool SysCpuProviderPack::IsRecAvailable_SuperVU0() const { return CpuProviders->superVU0.IsAvailable(); }
bool SysCpuProviderPack::IsRecAvailable_SuperVU1() const { return CpuProviders->superVU1.IsAvailable(); }
BaseException* SysCpuProviderPack::GetException_SuperVU0() const { return CpuProviders->superVU0.ExThrown; }
BaseException* SysCpuProviderPack::GetException_SuperVU1() const { return CpuProviders->superVU1.ExThrown; }


void SysCpuProviderPack::CleanupMess() throw()
{
	try
	{
		psxRec.Shutdown();
		recCpu.Shutdown();

		if (newVifDynaRec)
		{
			dVifRelease(0);
			dVifRelease(1);
		}
	}
	DESTRUCTOR_CATCHALL
}

SysCpuProviderPack::~SysCpuProviderPack() throw()
{
	CleanupMess();
}

bool SysCpuProviderPack::HadSomeFailures( const Pcsx2Config::RecompilerOptions& recOpts ) const
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

void SysCpuProviderPack::ApplyConfig() const
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

// This is a semi-hacky function for convenience
BaseVUmicroCPU* SysCpuProviderPack::getVUprovider(int whichProvider, int vuIndex) const {
	switch (whichProvider) {
		case 0: return vuIndex ? (BaseVUmicroCPU*)CpuProviders->interpVU1 : (BaseVUmicroCPU*)CpuProviders->interpVU0;
		case 1: return vuIndex ? (BaseVUmicroCPU*)CpuProviders->superVU1  : (BaseVUmicroCPU*)CpuProviders->superVU0;
		case 2: return vuIndex ? (BaseVUmicroCPU*)CpuProviders->microVU1  : (BaseVUmicroCPU*)CpuProviders->microVU0;
	}
	return NULL;
}

// Resets all PS2 cpu execution caches, which does not affect that actual PS2 state/condition.
// This can be called at any time outside the context of a Cpu->Execute() block without
// bad things happening (recompilers will slow down for a brief moment since rec code blocks
// are dumped).
// Use this method to reset the recs when important global pointers like the MTGS are re-assigned.
void SysClearExecutionCache()
{
	GetCpuProviders().ApplyConfig();

	Cpu->Reset();
	psxCpu->Reset();

	// mVU's VU0 needs to be properly initialized for macro mode even if it's not used for micro mode!
	if (CHECK_EEREC)
		((BaseVUmicroCPU*)GetCpuProviders().CpuProviders->microVU0)->Reset();

	CpuVU0->Reset();
	CpuVU1->Reset();

	if (newVifDynaRec)
	{
		dVifReset(0);
		dVifReset(1);
	}
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
		if( base )
		{
			DbgCon.Warning( "First try failed allocating %s at address 0x%x", caller, base );

			// Let's try again at an OS-picked memory area, and then hope it meets needed
			// boundschecking criteria below.
			SafeSysMunmap( Mem, size );
			Mem = (u8*)HostSys::Mmap( 0, size );
		}

		if( (bounds != 0) && (((uptr)Mem + size) > bounds) )
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
		// FIXME: system is currently running the BIOS, so it should return a serial based on
		// the BIOS being run (either a checksum of the BIOS roms, and/or a string based on BIOS
		// region and revision).
		
		return wxEmptyString;
	}

	return pxsFmt( L"%08x", ElfCRC );
}
