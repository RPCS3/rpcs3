/*  Cpudetection lib
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
#include "cpudetect_internal.h"

void CountLogicalCores( int LogicalCoresPerPhysicalCPU, int PhysicalCoresPerPhysicalCPU )
{
	DWORD vProcessCPUs;
	DWORD vSystemCPUs;

	x86caps.LogicalCores = 1;

	if( !GetProcessAffinityMask (GetCurrentProcess (),
		&vProcessCPUs, &vSystemCPUs) ) return;

	int CPUs = 0;
	DWORD bit;

	for (bit = 1; bit != 0; bit <<= 1)
	{
		if (vSystemCPUs & bit)
			CPUs++;
	}

	x86caps.LogicalCores = CPUs;
	if( LogicalCoresPerPhysicalCPU > CPUs) // for 1-socket HTT-disabled machines
		LogicalCoresPerPhysicalCPU = CPUs;

	x86caps.PhysicalCores = ( CPUs / LogicalCoresPerPhysicalCPU ) * PhysicalCoresPerPhysicalCPU;
}

bool _test_instruction( void* pfnCall )
{
	__try {
		u128 regsave;
		((void (__fastcall *)(void*))pfnCall)( &regsave );
	}
	__except(EXCEPTION_EXECUTE_HANDLER) { return false; }

	return true;
}

bool CanEmitShit()
{
	// Under Windows, pre 0.9.6 versions of PCSX2 may not initialize the TLS
	// register (FS register), so plugins (DLLs) using our x86emitter in multithreaded
	// mode will just crash/fail if it tries to do the instruction set tests.

#if x86EMIT_MULTITHREADED
	static __threadlocal int tls_failcheck;
	__try { tls_failcheck = 1; }
	__except(EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif

	return true;
}

bool CanTestInstructionSets()
{
	return CanEmitShit();
}

SingleCoreAffinity::SingleCoreAffinity()
{
	s_threadId	= NULL;
	s_oldmask	= ERROR_INVALID_PARAMETER;

	DWORD_PTR availProcCpus, availSysCpus;
	if( !GetProcessAffinityMask( GetCurrentProcess(), &availProcCpus, &availSysCpus ) ) return;

	int i;
	for( i=0; i<32; ++i )
	{
		if( availProcCpus & (1<<i) ) break;
	}

	s_threadId = GetCurrentThread();
	s_oldmask = SetThreadAffinityMask( s_threadId, (1UL<<i) );

	if( s_oldmask == ERROR_INVALID_PARAMETER )
	{
		Console.Warning(
			"CpuDetect: SetThreadAffinityMask failed...\n"
			"\tSystem Affinity : 0x%08x"
			"\tProcess Affinity: 0x%08x"
			"\tAttempted Thread Affinity CPU: i",
			availProcCpus, availSysCpus, i
		);
	}

	Sleep( 2 );

	// Sleep Explained: I arbitrarily pick Core 0 to lock to for running the CPU test.  This
	// means that the current thread will need to be switched to Core 0 if it's currently
	// scheduled on a difference cpu/core.  However, Windows does not necessarily perform
	// that scheduling immediately upon the call to SetThreadAffinityMask (seems dependent
	// on version: XP does, Win7 does not).  So by issuing a Sleep here we give Win7 time
	// to issue a timeslice and move our thread to Core 0.  Without this, it tends to move
	// the thread during the cpuSpeed test instead, causing totally wacky results.
};

SingleCoreAffinity::~SingleCoreAffinity() throw()
{
	if( s_oldmask != ERROR_INVALID_PARAMETER )
		SetThreadAffinityMask( s_threadId, s_oldmask );
}

void SetSingleAffinity()
{

}

void RestoreAffinity()
{
}
