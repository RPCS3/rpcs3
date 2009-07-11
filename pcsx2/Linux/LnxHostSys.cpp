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

#include <sys/mman.h>
#include <signal.h>
#include "Common.h"

extern void SysPageFaultExceptionFilter( int signal, siginfo_t *info, void * );
extern void __fastcall InstallLinuxExceptionHandler();
extern void __fastcall ReleaseLinuxExceptionHandler();

#define PCSX2_MEM_PROTECT_BEGIN() InstallLinuxExceptionHandler()
#define PCSX2_MEM_PROTECT_END() ReleaseLinuxExceptionHandler()

extern void SignalExit(int sig);

static const uptr m_pagemask = getpagesize()-1;

void InstallLinuxExceptionHandler()
{
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = &SysPageFaultExceptionFilter;
	sigaction(SIGSEGV, &sa, NULL);
}

void ReleaseLinuxExceptionHandler()
{
	// Code this later.
}

// Linux implementation of SIGSEGV handler.  Bind it using sigaction().
void SysPageFaultExceptionFilter( int signal, siginfo_t *info, void * )
{
	// get bad virtual address
	uptr offset = (u8*)info->si_addr - psM;

	DevCon::Status( "Protected memory cleanup. Offset 0x%x", params offset );

	if (offset>=Ps2MemSize::Base)
	{
		// Bad mojo!  Completely invalid address.
		// Instigate a crash or abort emulation or something.
		assert( false );
	}

	mmap_ClearCpuBlock( offset & ~m_pagemask );
}
