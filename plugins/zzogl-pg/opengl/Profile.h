/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
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

#ifndef PROFILE_H_INCLUDED
#define PROFILE_H_INCLUDED

#include "zerogs.h"

#if !defined(ZEROGS_DEVBUILD)
#define g_bWriteProfile 0
#else
extern bool g_bWriteProfile;
#endif

extern u64 luPerfFreq;


// Copied from Utilities; remove later.
#ifdef __LINUX__

#include <sys/time.h>

static __forceinline void InitCPUTicks()
{
}

static __forceinline u64 GetTickFrequency()
{
	return 1000000;		// unix measures in microseconds
}

static __forceinline u64 GetCPUTicks()
{

	struct timeval t;
	gettimeofday(&t, NULL);
	return ((u64)t.tv_sec*GetTickFrequency()) + t.tv_usec;
}

#else
static __aligned16 LARGE_INTEGER lfreq;

static __forceinline void InitCPUTicks()
{
	QueryPerformanceFrequency(&lfreq);
}

static __forceinline u64 GetTickFrequency()
{
	return lfreq.QuadPart;
}

static __forceinline u64 GetCPUTicks()
{
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	return count.QuadPart;
}

#endif

// IMPORTANT: For every Register there must be an End
void DVProfRegister(char* pname);			// first checks if this profiler exists in g_listProfilers
void DVProfEnd(u32 dwUserData);
void DVProfWrite(char* pfilename, u32 frames = 0);
void DVProfClear();						// clears all the profilers

#define DVPROFILE
#ifdef DVPROFILE

class DVProfileFunc
{
	public:
		u32 dwUserData;
		DVProfileFunc(char* pname) { DVProfRegister(pname); dwUserData = 0; }
		DVProfileFunc(char* pname, u32 dwUserData) : dwUserData(dwUserData) { DVProfRegister(pname); }
		~DVProfileFunc() { DVProfEnd(dwUserData); }
};

#else

class DVProfileFunc
{

	public:
		u32 dwUserData;
		static __forceinline DVProfileFunc(char* pname) {}
		static __forceinline DVProfileFunc(char* pname, u32 dwUserData) { }
		~DVProfileFunc() {}
};

#endif


template <typename T>
class CInterfacePtr
{

	public:
		inline CInterfacePtr() : ptr(NULL) {}
		inline explicit CInterfacePtr(T* newptr) : ptr(newptr) { if (ptr != NULL) ptr->AddRef(); }
		inline ~CInterfacePtr() { if (ptr != NULL) ptr->Release(); }
		inline T* operator*() { assert(ptr != NULL); return *ptr; }
		inline T* operator->() { return ptr; }
		inline T* get() { return ptr; }

		inline void release()
		{
			if (ptr != NULL) { ptr->Release(); ptr = NULL; }
		}

		inline operator T*() { return ptr; }
		inline bool operator==(T* rhs) { return ptr == rhs; }
		inline bool operator!=(T* rhs) { return ptr != rhs; }

		inline CInterfacePtr& operator= (T* newptr)
		{
			if (ptr != NULL) ptr->Release();

			ptr = newptr;

			if (ptr != NULL) ptr->AddRef();

			return *this;
		}

	private:
		T* ptr;
};

extern void InitProfile();

#endif // PROFILE_H_INCLUDED
