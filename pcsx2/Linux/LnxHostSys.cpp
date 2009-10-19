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

#include "PrecompiledHeader.h"

#include <sys/mman.h>
#include <signal.h>
#include "Common.h"

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

	DevCon.Status( "Protected memory cleanup. Offset 0x%x", offset );

	if (offset>=Ps2MemSize::Base)
	{
		// Bad mojo!  Completely invalid address.
		// Instigate a crash or abort emulation or something.
		wxTrap();
		return;
	}

	mmap_ClearCpuBlock( offset & ~m_pagemask );
}
