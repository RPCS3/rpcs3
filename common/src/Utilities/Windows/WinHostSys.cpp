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
#include "PageFaultSource.h"

#include <winnt.h>

int SysPageFaultExceptionFilter( EXCEPTION_POINTERS* eps )
{
	if( eps->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION )
		return EXCEPTION_CONTINUE_SEARCH;

	Source_PageFault->Dispatch( PageFaultInfo( (uptr)eps->ExceptionRecord->ExceptionInformation[1] ) );
	return Source_PageFault->WasHandled() ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

void _platform_InstallSignalHandler()
{
	// NOP on Win32 systems -- we use __try{} __except{} instead.
}


static DWORD ConvertToWinApi( const PageProtectionMode& mode )
{
	DWORD winmode = PAGE_NOACCESS;

	// Windows has some really bizarre memory protection enumeration that uses bitwise
	// numbering (like flags) but is in fact not a flag value.  *Someone* from the early
	// microsoft days wasn't a very good coder, me thinks.  --air

	if (mode.CanExecute())
	{
		winmode = mode.CanWrite() ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
	}
	else if (mode.CanRead())
	{
		winmode = mode.CanWrite() ? PAGE_READWRITE : PAGE_READONLY;
	}

	return winmode;
}

void* HostSys::MmapReservePtr(void* base, size_t size)
{
	return VirtualAlloc(base, size, MEM_RESERVE, PAGE_NOACCESS);
}

bool HostSys::MmapCommitPtr(void* base, size_t size, const PageProtectionMode& mode)
{
	void* result = VirtualAlloc(base, size, MEM_COMMIT, ConvertToWinApi(mode));
	if (result) return true;

	const DWORD errcode = GetLastError();
	if (errcode == ERROR_COMMITMENT_MINIMUM)
	{
		Console.Warning("(MmapCommit) Received windows error %u {Virtual Memory Minimum Too Low}.", ERROR_COMMITMENT_MINIMUM);
		Sleep(1000);	// Cut windows some time to rework its memory...
	}
	else if (errcode != ERROR_NOT_ENOUGH_MEMORY && errcode != ERROR_OUTOFMEMORY)
	{
		pxFailDev(L"VirtualAlloc COMMIT failed: " + Exception::WinApiError().GetMsgFromWindows());
		return false;
	}

	if (!pxDoOutOfMemory) return false;
	pxDoOutOfMemory(size);
	return VirtualAlloc(base, size, MEM_COMMIT, ConvertToWinApi(mode)) != NULL;
}

void HostSys::MmapResetPtr(void* base, size_t size)
{
	VirtualFree(base, size, MEM_DECOMMIT);
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
	return VirtualAlloc((void*)base, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

void HostSys::Munmap(uptr base, size_t size)
{
	if (!base) return;
	//VirtualFree((void*)base, size, MEM_DECOMMIT);
	VirtualFree((void*)base, 0, MEM_RELEASE);
}

void HostSys::MemProtect( void* baseaddr, size_t size, const PageProtectionMode& mode )
{
	pxAssertDev( ((size & (__pagesize-1)) == 0), pxsFmt(
		L"Memory block size must be a multiple of the target platform's page size.\n"
		L"\tPage Size: 0x%04x (%d), Block Size: 0x%04x (%d)",
		__pagesize, __pagesize, size, size )
	);
	
	DWORD OldProtect;	// enjoy my uselessness, yo!
	if (!VirtualProtect( baseaddr, size, ConvertToWinApi(mode), &OldProtect ))
	{
		Exception::WinApiError apiError;
		
		apiError.SetDiagMsg(
			pxsFmt(L"VirtualProtect failed @ 0x%08X -> 0x%08X  (mode=%s)",
			baseaddr, (uptr)baseaddr + size, mode.ToString().c_str()
		));

		pxFailDev( apiError.FormatDiagnosticMessage() );
	}
}
