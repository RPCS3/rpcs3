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

// Linux implementation of SIGSEGV handler.  Bind it using sigaction().
static void SysPageFaultSignalFilter( int signal, siginfo_t *info, void * )
{
	// Note: Use of most stdio functions isn't safe here.  Avoid console logs,
	// assertions, file logs, or just about anything else useful.

	PageFaultInfo info( (uptr)info->si_addr & ~m_pagemask );
	Source_AccessViolation.DispatchException( info );

	// resumes execution right where we left off (re-executes instruction that
	// caused the SIGSEGV).
	if( info.handled ) return;

	// Bad mojo!  Completely invalid address.
	// Instigate a trap if we're in a debugger, and if not then do a SIGKILL.

	wxTrap();
	if( !IsDebugBuild ) raise( SIGKILL );
}

void InstallSignalHandler()
{
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = SysPageFaultSignalFilter;
	sigaction(SIGSEGV, &sa, NULL);
}

void NTFS_CompressFile( const wxString& file, bool compressStatus=true ) {}
