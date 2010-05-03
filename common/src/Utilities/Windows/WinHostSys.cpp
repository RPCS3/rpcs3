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
#include "Utilities/RedtapeWindows.h"
#include <winnt.h>

namespace HostSys
{
	void *Mmap(uptr base, u32 size)
	{
		return VirtualAlloc((void*)base, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	}

	void Munmap(uptr base, u32 size)
	{
		if( base == NULL ) return;
		VirtualFree((void*)base, size, MEM_DECOMMIT);
		VirtualFree((void*)base, 0, MEM_RELEASE);
	}

	void MemProtect( void* baseaddr, size_t size, PageProtectionMode mode, bool allowExecution )
	{
		pxAssertDev( ((size & (__pagesize-1)) == 0), wxsFormat(
			L"Memory block size must be a multiple of the target platform's page size.\n"
			L"\tPage Size: 0x%04x (%d), Block Size: 0x%04x (%d)",
			__pagesize, __pagesize, size, size )
		);

		DWORD winmode = 0;

		switch( mode )
		{
			case Protect_NoAccess:
				winmode = ( allowExecution ) ? PAGE_EXECUTE : PAGE_NOACCESS;
			break;

			case Protect_ReadOnly:
				winmode = ( allowExecution ) ? PAGE_EXECUTE_READ : PAGE_READONLY;
			break;

			case Protect_ReadWrite:
				winmode = ( allowExecution ) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
			break;
		}

		DWORD OldProtect;	// enjoy my uselessness, yo!
		VirtualProtect( baseaddr, size, winmode, &OldProtect );
	}
}
