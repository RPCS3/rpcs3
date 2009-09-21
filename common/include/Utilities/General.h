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

#pragma once

enum PageProtectionMode
{
	Protect_NoAccess = 0,
	Protect_ReadOnly,
	Protect_ReadWrite
};

//////////////////////////////////////////////////////////////////////////////////////////
// HostSys - Namespace housing general system-level implementations relating to loading
// plugins and allocating memory.  For now, these functions are all accessed via Sys*
// versions defined in System.h/cpp.
//
namespace HostSys
{
	// Maps a block of memory for use as a recompiled code buffer.
	// The allocated block has code execution privileges.
	// Returns NULL on allocation failure.
	extern void *Mmap(uptr base, u32 size);

	// Unmaps a block allocated by SysMmap
	extern void Munmap(uptr base, u32 size);

	extern void MemProtect( void* baseaddr, size_t size, PageProtectionMode mode, bool allowExecution=false );

	extern void Munmap( void* base, u32 size );
}


//////////////////////////////////////////////////////////////////////////////////////////


extern void InitCPUTicks();
extern u64  GetTickFrequency();
extern u64  GetCPUTicks();
