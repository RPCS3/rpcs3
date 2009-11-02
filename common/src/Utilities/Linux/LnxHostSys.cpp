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


#include "../PrecompiledHeader.h"

#include <sys/mman.h>
#include <signal.h>

namespace HostSys
{
	static const uptr m_pagemask = getpagesize()-1;

	void *Mmap(uptr base, u32 size)
	{
		u8 *Mem;
		Mem = (u8*)mmap((uptr*)base, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
		if (Mem == MAP_FAILED) Console.Warning("Mmap Failed!");

		return Mem;
	}

	void Munmap(uptr base, u32 size)
	{
		munmap((uptr*)base, size);
	}

	void MemProtect( void* baseaddr, size_t size, PageProtectionMode mode, bool allowExecution )
	{
		pxAssertDev( ((size & ~__pagesize) == 0), wxsFormat(
			L"Memory block size must be a multiple of the target platform's page size.\n"
			L"\tPage Size: 0x%04x (%d), Block Size: 0x%04x (%d)",
			__pagesize, __pagesize, size, size )
		);

		int lnxmode = 0;

		// make sure size is aligned to the system page size:
		// Check is redundant against the assertion above, but might as well...
		size = (size + m_pagemask) & ~m_pagemask;

		switch( mode )
		{
			case Protect_NoAccess: break;
			case Protect_ReadOnly: lnxmode = PROT_READ; break;
			case Protect_ReadWrite: lnxmode = PROT_READ | PROT_WRITE; break;
		}

		if( allowExecution ) lnxmode |= PROT_EXEC;
		mprotect( baseaddr, size, lnxmode );
	}
}
