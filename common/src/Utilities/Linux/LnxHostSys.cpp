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


#include "../PrecompiledHeader.h"
#include "PageFaultSource.h"

#include <wx/thread.h>

#include <sys/mman.h>
#include <signal.h>
#include <errno.h>

extern void SignalExit(int sig);

static const uptr m_pagemask = getpagesize()-1;

// Linux implementation of SIGSEGV handler.  Bind it using sigaction().
static void SysPageFaultSignalFilter( int signal, siginfo_t *siginfo, void * )
{
	// [TODO] : Add a thread ID filter to the Linux Signal handler here.
	// Rationale: On windows, the __try/__except model allows per-thread specific behavior
	// for page fault handling.  On linux, there is a single signal handler for the whole
	// process, but the handler is executed by the thread that caused the exception.


	// Stdio Usage note: SIGSEGV handling is a synchronous in-thread signal.  It is done
	// from the context of the current thread and stackframe.  So long as the thread is not
	// the main/ui thread, use of the px assertion system should be safe.  Use of stdio should
	// be safe even on the main thread.
	//  (in other words, stdio limitations only really apply to process-level asynchronous
	//   signals)
	
	// Note: Use of stdio functions isn't safe here.  Avoid console logs,
	// assertions, file logs, or just about anything else useful.

	Source_PageFault->Dispatch( PageFaultInfo( (uptr)siginfo->si_addr & ~m_pagemask ) );

	// resumes execution right where we left off (re-executes instruction that
	// caused the SIGSEGV).
	if (Source_PageFault->WasHandled()) return;

	if (!wxThread::IsMain())
	{
		pxFailRel(pxsFmt("Unhandled page fault @ 0x%08x", siginfo->si_addr));
	}

	// Bad mojo!  Completely invalid address.
	// Instigate a trap if we're in a debugger, and if not then do a SIGKILL.

	wxTrap();
	if (!IsDebugBuild) raise( SIGKILL );
}

void _platform_InstallSignalHandler()
{
	Console.WriteLn("Installing POSIX SIGSEGV handler...");
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = SysPageFaultSignalFilter;
	sigaction(SIGSEGV, &sa, NULL);
}

static __ri void PageSizeAssertionTest( size_t size )
{
	pxAssertMsg( (__pagesize == getpagesize()), pxsFmt(
		"Internal system error: Operating system pagesize does not match compiled pagesize.\n\t"
		L"\tOS Page Size: 0x%x (%d), Compiled Page Size: 0x%x (%u)",
		getpagesize(), getpagesize(), __pagesize, __pagesize )
	);

	pxAssertDev( (size & (__pagesize-1)) == 0, pxsFmt(
		L"Memory block size must be a multiple of the target platform's page size.\n"
		L"\tPage Size: 0x%x (%u), Block Size: 0x%x (%u)",
		__pagesize, __pagesize, size, size )
	);
}

// returns FALSE if the mprotect call fails with an ENOMEM.
// Raises assertions on other types of POSIX errors (since those typically reflect invalid object
// or memory states).
static bool _memprotect( void* baseaddr, size_t size, const PageProtectionMode& mode )
{
	PageSizeAssertionTest(size);

	uint lnxmode = 0;

	if (mode.CanWrite())	lnxmode |= PROT_WRITE;
	if (mode.CanRead())		lnxmode |= PROT_READ;
	if (mode.CanExecute())	lnxmode |= PROT_EXEC | PROT_READ;

	const int result = mprotect( baseaddr, size, lnxmode );

	if (result == 0) return true;
	
	switch(errno)
	{
		case EINVAL:
			pxFailDev(pxsFmt(L"mprotect returned EINVAL @ 0x%08X -> 0x%08X  (mode=%s)",
				baseaddr, (uptr)baseaddr+size, mode.ToString().c_str())
			);
		break;
		
		case EACCES:
			pxFailDev(pxsFmt(L"mprotect returned EACCES @ 0x%08X -> 0x%08X  (mode=%s)",
				baseaddr, (uptr)baseaddr+size, mode.ToString().c_str())
			);
		break;

		case ENOMEM:
			// caller handles assertion or exception, or whatever.
		break;
	}
	return false;
}

void* HostSys::MmapReservePtr(void* base, size_t size)
{
	PageSizeAssertionTest(size);

	// On linux a reserve-without-commit is performed by using mmap on a read-only
	// or anonymous source, with PROT_NONE (no-access) permission.  Since the mapping
	// is completely inaccessible, the OS will simply reserve it and will not put it
	// against the commit table.
	return mmap(base, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

bool HostSys::MmapCommitPtr(void* base, size_t size, const PageProtectionMode& mode)
{
	// In linux, reserved memory is automatically committed when its permissions are
	// changed to something other than PROT_NONE.  If the user is committing memory
	// as PROT_NONE, then just ignore this call (memory will be committed automatically
	// later when the user changes permissions to something useful via calls to MemProtect).

	if (mode.IsNone()) return false;

	if (_memprotect( base, size, mode )) return true;

	if (!pxDoOutOfMemory) return false;
	pxDoOutOfMemory(size);
	return _memprotect( base, size, mode );
}

void HostSys::MmapResetPtr(void* base, size_t size)
{
	// On linux the only way to reset the memory is to unmap and remap it as PROT_NONE.
	// That forces linux to unload all committed pages and start from scratch.

	// FIXME: Ideally this code would have some threading lock on it to prevent any other
	// malloc/free code in the current process from interfering with the operation, but I
	// can't think of any good way to do that.  (generally it shouldn't be a problem in
	// PCSX2 anyway, since MmapReset is only called when the ps2vm is suspended; so that
	// pretty well stops all PCSX2 threads anyway).

	Munmap(base, size);
	void* result = Mmap((uptr)base, size);

	pxAssertRel ((uptr)result != (uptr)base, pxsFmt(
		"Virtual memory decommit failed: memory at 0x%08X -> 0x%08X could not be remapped.  "
		"This is likely caused by multi-thread memory contention.", base, (uptr)base+size
	));
}

void* HostSys::MmapReserve(uptr base, size_t size)
{
	return MmapReservePtr((void*)base, size);
}

bool HostSys::MmapCommit(uptr base, size_t size, const PageProtectionMode& mode)
{
	return MmapCommitPtr( (void*)base, size, mode );
}

void HostSys::MmapReset(uptr base, size_t size)
{
	MmapResetPtr((void*)base, size);
}

void* HostSys::Mmap(uptr base, size_t size)
{
	PageSizeAssertionTest(size);

	// MAP_ANONYMOUS - means we have no associated file handle (or device).

	return mmap((void*)base, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void HostSys::Munmap(uptr base, size_t size)
{
	munmap((void*)base, size);
}

void HostSys::MemProtect( void* baseaddr, size_t size, const PageProtectionMode& mode )
{
	if (!_memprotect(baseaddr, size, mode))
	{
		throw Exception::OutOfMemory( L"MemProtect" )
			.SetDiagMsg(pxsFmt( L"mprotect failed @ 0x%08X -> 0x%08X  (mode=%s)",
				baseaddr, (uptr)baseaddr+size, mode.ToString().c_str()
			)
		);
	}
}
