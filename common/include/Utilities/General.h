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

	static __forceinline void Munmap( void* base, u32 size )
	{
		Munmap( (uptr)base, size );
	}
}


//////////////////////////////////////////////////////////////////////////////////////////


extern void InitCPUTicks();
extern u64  GetTickFrequency();
extern u64  GetCPUTicks();
