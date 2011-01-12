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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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

// declare linux equivalents (alignment must be power of 2 (1,2,4...2^15)
static __forceinline void* pcsx2_aligned_malloc(size_t size, size_t alignment) {
	assert(alignment <= 0x8000);
	uptr r = (uptr)malloc(size + --alignment + 2);
	uptr o = (r + 2 + alignment) & ~(uptr)alignment;
	if (!r) return NULL;
	((u16*)o)[-1] = (u16)(o-r);
	return (void*)o;
}

static __forceinline void pcsx2_aligned_free(void* p) {
	if (!p) return;
	free((void*)((uptr)p-((u16*)p)[-1]));
}


#define _aligned_malloc pcsx2_aligned_malloc
#define _aligned_free pcsx2_aligned_free
#endif
