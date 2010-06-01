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
#include "Win32.h"
#include <winnt.h>

#include "Common.h"
#include "System/PageFaultSource.h"

int SysPageFaultExceptionFilter( EXCEPTION_POINTERS* eps )
{
	if( eps->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION )
		return EXCEPTION_CONTINUE_SEARCH;

	Source_PageFault.Dispatch( PageFaultInfo( (uptr)eps->ExceptionRecord->ExceptionInformation[1] ) );
	return Source_PageFault.WasHandled() ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}

void InstallSignalHandler()
{
	// NOP on Win32 systems -- we use __try{} __except{} instead.
}
