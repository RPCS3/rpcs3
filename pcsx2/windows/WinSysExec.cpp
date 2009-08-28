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

#include "PrecompiledHeader.h"
#include "Win32.h"

#include <winnt.h>
#include "Common.h"

#include "cdvd/CDVD.h"

int SysPageFaultExceptionFilter( EXCEPTION_POINTERS* eps )
{
	const _EXCEPTION_RECORD& ExceptionRecord = *eps->ExceptionRecord;
	
	if (ExceptionRecord.ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// get bad virtual address
	u32 offset = (u8*)ExceptionRecord.ExceptionInformation[1]-psM;

	if (offset>=Ps2MemSize::Base)
		return EXCEPTION_CONTINUE_SEARCH;

	mmap_ClearCpuBlock( offset );

	return EXCEPTION_CONTINUE_EXECUTION;
}

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
