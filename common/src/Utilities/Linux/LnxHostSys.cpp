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

#include <sys/mman.h>
#include <signal.h>
#include <errno.h>

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

void* HostSys::MmapReserve(uptr base, size_t size)
{
	PageSizeAssertionTest(size);

	// On linux a reserve-without-commit is performed by using mmap on a read-only
	// or anonymous source, with PROT_NONE (no-access) permission.  Since the mapping
	// is completely inaccessible, the OS will simply reserve it and will not put it
	// against the commit table.
	return mmap((void*)base, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void HostSys::MmapCommit(void* base, size_t size)
{
	// In linux, reserved memory is automatically committed when its permissions are
	// changed to something other than PROT_NONE.  Since PCSX2 code *must* do that itself
	// prior to making use of memory anyway, this call should be a NOP.
}

void HostSys::MmapReset(void* base, size_t size)
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
		"This is likely caused by multi-thread memory contention.", base, base+size
	));
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
	PageSizeAssertionTest(size);

	uint lnxmode = 0;

	if (mode.CanWrite())	lnxmode |= PROT_WRITE;
	if (mode.CanRead())		lnxmode |= PROT_READ;
	if (mode.CanExecute())	lnxmode |= PROT_EXEC | PROT_READ;

	int result = mprotect( baseaddr, size, lnxmode );

	if (result != 0)
	{
		switch(errno)
		{
			case EINVAL:
				pxFailDev(pxsFmt(L"mprotect returned EINVAL @ 0x%08X -> 0x%08X  (mode=%s)",
					baseaddr, (uptr)baseaddr+size, mode.ToString().c_str())
				);
			break;
			
			case ENOMEM:
				throw Exception::OutOfMemory( pxsFmt( L"mprotect failed @ 0x%08X -> 0x%08X  (mode=%s)",
					baseaddr, (uptr)baseaddr+size, mode.ToString().c_str())
				);
			break;
			
			case EACCES:
			break;
		}
		throw Exception::OutOfMemory();
	}
}
