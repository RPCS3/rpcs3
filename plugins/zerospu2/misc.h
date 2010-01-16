/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

#ifdef __LINUX__
#include <unistd.h>
#include <gtk/gtk.h>
#include <sys/timeb.h>	// ftime(), struct timeb

#define Sleep(ms) usleep(1000*ms)

inline u32 timeGetTime()
{
#ifdef _WIN32
	_timeb t;
	_ftime(&t);
#else
	timeb t;
	ftime(&t);
#endif

	return (u32)(t.time*1000+t.millitm);
}

#include <sys/time.h>

#define InterlockedExchangeAdd _InterlockedExchangeAdd

#else
#include <windows.h>
#include <windowsx.h>

#include <sys/timeb.h>	// ftime(), struct timeb
#endif

inline u64 GetMicroTime()
{
#ifdef _WIN32
	extern LARGE_INTEGER g_counterfreq;
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	return count.QuadPart * 1000000 / g_counterfreq.QuadPart;
#else
	timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec*1000000+t.tv_usec;
#endif
}

#if !defined(_MSC_VER) && !defined(HAVE_ALIGNED_MALLOC)

#include <assert.h>

// declare linux equivalents
static  __forceinline void* pcsx2_aligned_malloc(size_t size, size_t align)
{
	assert( align < 0x10000 );
	char* p = (char*)malloc(size+align);
	int off = 2+align - ((int)(uptr)(p+2) % align);

	p += off;
	*(u16*)(p-2) = off;

	return p;
}

static __forceinline void pcsx2_aligned_free(void* pmem)
{
	if( pmem != NULL ) {
		char* p = (char*)pmem;
		free(p - (int)*(u16*)(p-2));
	}
}

#define _aligned_malloc pcsx2_aligned_malloc
#define _aligned_free pcsx2_aligned_free
#endif
