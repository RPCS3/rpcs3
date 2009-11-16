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

s32 iCpuId( u32 cmd, u32 *regs )
{
	// ecx should be zero for CPUID(4)
	__asm__ __volatile__ ( "xor %ecx, %ecx" );

	__cpuid( (int*)regs, cmd );
	return 0;
}

// Note: Apparently this solution is Linux/Solaris only.
// FreeBSD/OsX need something far more complicated (apparently)
void CountLogicalCores( int LogicalCoresPerPhysicalCPU, int PhysicalCoresPerPhysicalCPU )
{
	const uint numCPU = sysconf( _SC_NPROCESSORS_ONLN );
	if( numCPU > 0 )
	{
		isMultiCore = numCPU > 1;
		x86caps.LogicalCores = numCPU;
		x86caps.PhysicalCores = ( numCPU / LogicalCoresPerPhysicalCPU ) * PhysicalCoresPerPhysicalCPU;
	}
	else
	{
		// Indeterminate?
		x86caps.LogicalCores = 1;
		x86caps.PhysicalCores = 1;
	}
}

bool CanTestInstructionSets()
{
	// Not implemented yet for linux.  (see cpudetect_internal.h for details)
	return false;
}

bool _test_instruction( void* pfnCall )
{
	// Not implemented yet for linux.  (see cpudetect_internal.h for details)
	return false;
}

// Not implemented yet for linux (see cpudetect_internal.h for details)
SingleCoreAffinity::SingleCoreAffinity() {}
SingleCoreAffinity::~SingleCoreAffinity() throw() {}