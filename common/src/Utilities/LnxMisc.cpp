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

#include <ctype.h>
#include <time.h>
#include <sys/time.h>

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
