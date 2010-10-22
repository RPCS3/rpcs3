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

void* HostSys::MmapReserve(uptr base, size_t size)
{
	return VirtualAlloc((void*)base, size, MEM_RESERVE, PAGE_NOACCESS);
}

void HostSys::MmapCommit(void* base, size_t size)
{
	// Execution flag for this and the Reserve should match... ?
	void* result = VirtualAlloc(base, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	pxAssumeDev(result, L"VirtualAlloc COMMIT failed: " + Exception::WinApiError().GetMsgFromWindows());
}

void HostSys::MmapReset(void* base, size_t size)
{
	// Execution flag is actually irrelevant for this operation, but whatever.
	//void* result = VirtualAlloc((void*)base, size, MEM_RESET, PAGE_EXECUTE_READWRITE);
	//pxAssumeDev(result, L"VirtualAlloc RESET failed: " + Exception::WinApiError().GetMsgFromWindows());

	VirtualFree(base, size, MEM_DECOMMIT);
}

void* HostSys::Mmap(uptr base, size_t size)
{
	return VirtualAlloc((void*)base, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

void HostSys::Munmap(uptr base, size_t size)
{
	if (!base) return;
	VirtualFree((void*)base, size, MEM_DECOMMIT);
	VirtualFree((void*)base, 0, MEM_RELEASE);
}

void HostSys::MemProtect( void* baseaddr, size_t size, const PageProtectionMode& mode )
{
	pxAssertDev( ((size & (__pagesize-1)) == 0), wxsFormat(
		L"Memory block size must be a multiple of the target platform's page size.\n"
		L"\tPage Size: 0x%04x (%d), Block Size: 0x%04x (%d)",
		__pagesize, __pagesize, size, size )
	);

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
	
	DWORD OldProtect;	// enjoy my uselessness, yo!
	if (!VirtualProtect( baseaddr, size, winmode, &OldProtect ))
	{
		throw Exception::WinApiError().SetDiagMsg(
			pxsFmt(L"VirtualProtect failed @ 0x%08X -> 0x%08X  (mode=%s)",
			baseaddr, (uptr)baseaddr + size, mode.ToString().c_str()
		));
	}
}

wxString PageProtectionMode::ToString() const
{
	wxString modeStr;

	if (m_read)		modeStr += L"Read";
	if (m_write)	modeStr += L"Write";
	if (m_exec)		modeStr += L"Exec";

	if (modeStr.Length() <= 5) modeStr += L"Only";
	
	return modeStr;
}
