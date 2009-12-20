/*  Cpudetection lib
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
	__except(EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
	return true;
}

bool CanTestInstructionSets()
{
	return true;
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
	
	// Force Windows to timeslice (hoping this fixes some affinity issues)
	Sleep( 2 );
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
