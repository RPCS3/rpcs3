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

#include "../PrecompiledHeader.h"

#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <wx/utils.h>

// Returns 0 on failure (not supported by the operating system).
u64 GetPhysicalMemory()
{
	u64 pages		= 0;

#ifdef _SC_PHYS_PAGES
	pages = sysconf(_SC_PHYS_PAGES);
#endif

	return pages * getpagesize();
}


void InitCPUTicks()
{
}

u64 GetTickFrequency()
{
	return 1000000;		// unix measures in microseconds
}

u64 GetCPUTicks()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return ((u64)t.tv_sec*GetTickFrequency())+t.tv_usec;
}

wxString GetOSVersionString()
{
	return wxGetOsDescription();
}

