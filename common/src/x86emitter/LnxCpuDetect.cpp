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

// Note: Apparently this solution is Linux/Solaris only.
// FreeBSD/OsX need something far more complicated (apparently)
void x86capabilities::CountLogicalCores()
{
	const uint numCPU = sysconf( _SC_NPROCESSORS_ONLN );
	if( numCPU > 0 )
	{
		//isMultiCore = numCPU > 1;
		LogicalCores = numCPU;
		PhysicalCores = ( numCPU / LogicalCoresPerPhysicalCPU ) * PhysicalCoresPerPhysicalCPU;
	}
	else
	{
		// Indeterminate?
		LogicalCores = 1;
		PhysicalCores = 1;
	}
}

bool CanEmitShit()
{
	// In Linux I'm pretty sure TLS always works, none of the funny business that Windows
	// has involving DLLs. >_<
	return true;
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
