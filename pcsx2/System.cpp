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

// --------------------------------------------------------------------------------------
//  BaseVirtualMemoryReserve  (implementations)
// --------------------------------------------------------------------------------------

BaseVirtualMemoryReserve::BaseVirtualMemoryReserve( const wxString& name )
	: Name( name )
{
	m_commited		= 0;
	m_reserved		= 0;
	m_baseptr		= NULL;
	m_block_size	= __pagesize;
	m_prot_mode		= PageAccess_None();
}

// Parameters:
//   upper_bounds - criteria that must be met for the allocation to be valid.
//     If the OS refuses to allocate the memory below the specified address, the
//     object will fail to initialize and an exception will be thrown.
void* BaseVirtualMemoryReserve::Reserve( uint size, uptr base, uptr upper_bounds )
{
	if (!pxAssertDev( m_baseptr == NULL, "(VirtualMemoryReserve) Invalid object state; object has already been reserved." ))
		return m_baseptr;

	m_reserved = (size + __pagesize-4) / __pagesize;
	uptr reserved_bytes = m_reserved * __pagesize;

	m_baseptr = (void*)HostSys::MmapReserve(base, reserved_bytes);

	if (!m_baseptr && (upper_bounds != 0 && (((uptr)m_baseptr + reserved_bytes) > upper_bounds)))
	{
		if (base)
		{
			DevCon.Warning( L"%s default address 0x%08x is unavailable; falling back on OS-default address.", Name.c_str(), base );

			// Let's try again at an OS-picked memory area, and then hope it meets needed
			// boundschecking criteria below.
			SafeSysMunmap( m_baseptr, reserved_bytes );
			m_baseptr = (void*)HostSys::MmapReserve( NULL, reserved_bytes );
		}

		if ((upper_bounds != 0) && (((uptr)m_baseptr + reserved_bytes) > upper_bounds))
		{
			SafeSysMunmap( m_baseptr, reserved_bytes );
			// returns null, caller should throw an exception or handle appropriately.
		}
	}

	if (!m_baseptr) return NULL;
	
	DevCon.WriteLn( Color_Blue, L"%s mapped @ 0x%08X -> 0x%08X [%umb]", Name.c_str(),
		m_baseptr, (uptr)m_baseptr+reserved_bytes, reserved_bytes / _1mb);

	if (m_def_commit)
	{
		HostSys::MmapCommit(m_baseptr, m_def_commit*__pagesize);
		HostSys::MemProtect(m_baseptr, m_def_commit*__pagesize, m_prot_mode);
	}
	
	return m_baseptr;
}

// Clears all committed blocks, restoring the allocation to a reserve only.
void BaseVirtualMemoryReserve::Reset()
{
	if (!m_commited) return;
	
	HostSys::MemProtect(m_baseptr, m_commited*__pagesize, PageAccess_None());
	HostSys::MmapReset(m_baseptr, m_commited*__pagesize);
	m_commited = 0;
}

void BaseVirtualMemoryReserve::Free()
{
	HostSys::Munmap((uptr)m_baseptr, m_reserved*__pagesize);
}

void BaseVirtualMemoryReserve::OnPageFaultEvent(const PageFaultInfo& info, bool& handled)
{
	uptr offset = (info.addr - (uptr)m_baseptr) / __pagesize;
	if (offset >= m_reserved) return;

	try	{

		if (!m_commited && m_def_commit)
		{
			const uint camt = m_def_commit * __pagesize;
			// first block being committed!  Commit the default requested
			// amount if its different from the blocksize.
			
			HostSys::MmapCommit(m_baseptr, camt);
			HostSys::MemProtect(m_baseptr, camt, m_prot_mode);

			u8* init = (u8*)m_baseptr;
			u8* endpos = init + camt;
			for( ; init<endpos; init += m_block_size*__pagesize )
				OnCommittedBlock(init);

			handled = true;
			m_commited += m_def_commit * __pagesize;

			return;
		}

		void* bleh = (u8*)m_baseptr + (offset * __pagesize);
	
		// Depending on the operating system, one or both of these could fail if the system
		// is low on either physical ram or virtual memory.
		HostSys::MmapCommit(bleh, m_block_size*__pagesize);
		HostSys::MemProtect(bleh, m_block_size*__pagesize, m_prot_mode);

		m_commited += m_block_size;
		OnCommittedBlock(bleh);

		handled = true;
	}
	catch (Exception::OutOfMemory& ex)
	{
		OnOutOfMemory( ex, (u8*)m_baseptr + (offset * __pagesize), handled );
	}
	#ifndef __WXMSW__
	// In windows we can let exceptions bubble out of the pag fault handler.  SEH will more
	// or less handle them in a semi-expected way, and might even avoid a GPF long enough
	// for the system to log the error or something.
	
	// In Linux, however, the SIGNAL handler is very limited in what it can do, and not only
	// can't we let the C++ exception try to unwind the stack, we can't really log it either.
	// We can't issue a proper assertion (requires user popup).  We can't do jack or shit,
	// *unless* its attached to a debugger;  then we can, at a bare minimum, trap it.
	catch (Exception::BaseException& ex)
	{
		wxTrap();
		handled = false;
	}
	#endif
}

RecompiledCodeReserve::RecompiledCodeReserve( const wxString& name, uint defCommit )
	: BaseVirtualMemoryReserve( name )
{
	m_block_size	= (1024 * 128) / __pagesize;
	m_prot_mode		= PageAccess_Any();
	m_def_commit	= defCommit / __pagesize;
}

template< u8 data >
__noinline void memset_sse_a( void* dest, const size_t size )
{
	const uint MZFqwc = size / 16;

	pxAssert( (size & 0xf) == 0 );

	static __aligned16 const u8 loadval[8] = { data,data,data,data,data,data,data,data };
	__m128 srcreg = _mm_load_ps( (float*)loadval );
	srcreg = _mm_loadh_pi( srcreg, (__m64*)loadval );

	float (*destxmm)[4] = (float(*)[4])dest;

#define StoreDestIdx(idx) case idx: _mm_store_ps(&destxmm[idx-1][0], srcreg)

	switch( MZFqwc & 0x07 )
	{
		StoreDestIdx(0x07);
		StoreDestIdx(0x06);
		StoreDestIdx(0x05);
		StoreDestIdx(0x04);
		StoreDestIdx(0x03);
		StoreDestIdx(0x02);
		StoreDestIdx(0x01);
	}

	destxmm += (MZFqwc & 0x07);
	for( uint i=0; i<MZFqwc / 8; ++i, destxmm+=8 )
	{
		_mm_store_ps(&destxmm[0][0], srcreg);
		_mm_store_ps(&destxmm[1][0], srcreg);
		_mm_store_ps(&destxmm[2][0], srcreg);
		_mm_store_ps(&destxmm[3][0], srcreg);
		_mm_store_ps(&destxmm[4][0], srcreg);
		_mm_store_ps(&destxmm[5][0], srcreg);
		_mm_store_ps(&destxmm[6][0], srcreg);
		_mm_store_ps(&destxmm[7][0], srcreg);
	}
};


void RecompiledCodeReserve::OnCommittedBlock( void* block )
{
	if (IsDevBuild)
	{
		// Clear the recompiled code block to 0xcc (INT3) -- this helps disasm tools show
		// the assembly dump more cleanly.  We don't clear the block on Release builds since
		// it can add a noticeable amount of overhead to large block recompilations.

		memset_sse_a<0xcc>( block, m_block_size * __pagesize );
	}
}

void RecompiledCodeReserve::OnOutOfMemory( const Exception::OutOfMemory& ex, void* blockptr, bool& handled )
{
	// Truncate and reset reserves of all other in-use recompiler caches, as this should
	// help free up quite a bit of emergency memory.

	//Cpu->SetCacheReserve( (Cpu->GetCacheReserve() * 3) / 2 );
	Cpu->Reset();

	//CpuVU0->SetCacheReserve( (CpuVU0->GetCacheReserve() * 3) / 2 );
	CpuVU0->Reset();

	//CpuVU1->SetCacheReserve( (CpuVU1->GetCacheReserve() * 3) / 2 );
	CpuVU1->Reset();

	//psxCpu->SetCacheReserve( (psxCpu->GetCacheReserve() * 3) / 2 );
	psxCpu->Reset();

	// Since the recompiler is happy writing away to memory, we have to truncate the reserve
	// to include the page currently being accessed, and cannot go any smaller.  This will
	// allow the rec to finish emitting the current block of instructions, detect that it has
	// exceeded the threshold buffer, and reset the buffer on its own.
	
	// Note: We attempt to commit multiple pages first, since a single block of recompiled
	// code can pretty easily surpass 4k.  We should have enough for this, since we just
	// cleared the other rec caches above -- but who knows what could happen if the user
	// has another process sucking up RAM or if the operating system is fickle.  If even
	// that fails, give up and kill the process.

	try
	{
		uint cusion = std::min<uint>( m_block_size, 4 );
		HostSys::MmapCommit((u8*)blockptr, cusion * __pagesize);
		HostSys::MemProtect((u8*)blockptr, cusion * __pagesize, m_prot_mode);
		
		handled = true;
	}
	catch (Exception::BaseException&)
	{
		// Fickle has become our reality.  By setting handled to FALSE, the OS should kill
		// the process for us.  No point trying to log anything; this is a super-awesomely
		// serious condition that likely means the system is hosed. ;)

		handled = false;
	}
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
	return pxE( ".Error:EmuCore::MemoryForVM",
		L"PCSX2 is unable to allocate memory needed for the PS2 virtual machine. "
		L"Close out some memory hogging background tasks and try again."
	);
}

// --------------------------------------------------------------------------------------
//  SysAllocVM  (implementations)
// --------------------------------------------------------------------------------------
SysAllocVM::SysAllocVM()
{
	InstallSignalHandler();

	Console.WriteLn( "Allocating memory for the PS2 virtual machine..." );

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
}

void SysAllocVM::CleanupMess() throw()
{
	try
	{
		vuMicroMemShutdown();
		psxMemShutdown();
		memShutdown();
		vtlb_Core_Shutdown();
	}
	DESTRUCTOR_CATCHALL
}

SysAllocVM::~SysAllocVM() throw()
{
	CleanupMess();
}

// --------------------------------------------------------------------------------------
//  SysCpuProviderPack  (implementations)
// --------------------------------------------------------------------------------------
SysCpuProviderPack::SysCpuProviderPack()
{
	Console.WriteLn( "Reserving memory for recompilers..." );

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

	// If both VUrecs failed, then make sure the SuperVU is totally closed out, because it
	// actually initializes everything once and then shares it between both VU recs.
	if( !IsRecAvailable_SuperVU0() && !IsRecAvailable_SuperVU1() )
		SuperVUDestroy( -1 );
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
		closeNewVif(0);
		closeNewVif(1);

		// Special SuperVU "complete" terminator (stupid hacky recompiler)
		SuperVUDestroy( -1 );

		psxRec.Shutdown();
		recCpu.Shutdown();
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

	// SuperVUreset will do nothing is none of the recs are initialized.
	// But it's needed if one or the other is initialized.
	SuperVUReset(-1);

	Cpu->Reset();
	psxCpu->Reset();

	// mVU's VU0 needs to be properly initialized for macro mode even if it's not used for micro mode!
	if (CHECK_EEREC)
		((BaseVUmicroCPU*)GetCpuProviders().CpuProviders->microVU0)->Reset();

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

			// Let's try again at an OS-picked memory area, and then hope it meets needed
			// boundschecking criteria below.
			SafeSysMunmap( Mem, size );
			Mem = (u8*)HostSys::Mmap( NULL, size );
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
		// FIXME: If the system is currently running the BIOS, it should return a serial based on
		// the BIOS being run (either a checksum of the BIOS roms, and/or a string based on BIOS
		// region and revision).
	}

	return wxsFormat( L"%8.8x", ElfCRC );
}
