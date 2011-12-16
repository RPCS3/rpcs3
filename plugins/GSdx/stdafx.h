/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#include "config.h"

#ifdef _WINDOWS

// The following macros define the minimum required platform.  The minimum required platform
// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run
// your application.  The macros work by enabling all features available on platform versions up to and
// including the version specified.

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.

#ifndef WINVER                          // Specifies that the minimum required platform is Windows Vista.
#define WINVER 0x0600           // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS          // Specifies that the minimum required platform is Windows 98.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE                       // Specifies that the minimum required platform is Internet Explorer 7.0.
#define _WIN32_IE 0x0700        // Change this to the appropriate value to target other versions of IE.
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <atlbase.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3d9.h>
#include <d3dx9.h>

#define D3DCOLORWRITEENABLE_RGBA (D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA)
#define D3D11_SHADER_MACRO D3D10_SHADER_MACRO
#define ID3D11Blob ID3D10Blob

#endif

// stdc

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <limits.h>

#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>

using namespace std;

#ifdef _WINDOWS

	#include <hash_map>
	#include <hash_set>

	using namespace stdext;

	#define vsnprintf _vsnprintf
	#define snprintf _snprintf

	#define DIRECTORY_SEPARATOR '\\'

#else

	#define _BACKWARD_BACKWARD_WARNING_H

	#define hash_map map
	#define hash_set set

	//#include <ext/hash_map>
	//#include <ext/hash_set>

	//using namespace __gnu_cxx;

	#define DIRECTORY_SEPARATOR '/'

#endif

#ifdef _MSC_VER

    #define __aligned(t, n) __declspec(align(n)) t

    #define EXPORT_C_(type) extern "C" __declspec(dllexport) type __stdcall
    #define EXPORT_C EXPORT_C_(void)

#else

    #define __aligned(t, n) t __attribute__((aligned(n)))
    #define __fastcall __attribute__((fastcall))

    #define EXPORT_C_(type) extern "C" __attribute__((stdcall,externally_visible,visibility("default"))) type
    #define EXPORT_C EXPORT_C_(void)

    #ifdef __GNUC__

        #include "assert.h"
        #define __forceinline __inline__ __attribute__((always_inline,unused))
        // #define __forceinline __inline__ __attribute__((__always_inline__,__gnu_inline__))
        #define __assume(c) ((void)0)

    #endif

#endif

extern string format(const char* fmt, ...);

struct delete_object {template<class T> void operator()(T& p) {delete p;}};
struct delete_first {template<class T> void operator()(T& p) {delete p.first;}};
struct delete_second {template<class T> void operator()(T& p) {delete p.second;}};
struct aligned_free_object {template<class T> void operator()(T& p) {_aligned_free(p);}};
struct aligned_free_first {template<class T> void operator()(T& p) {_aligned_free(p.first);}};
struct aligned_free_second {template<class T> void operator()(T& p) {_aligned_free(p.second);}};

// syntactic sugar

// put these into vc9/common7/ide/usertype.dat to have them highlighted

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long uint64;
typedef signed long long int64;

#define countof(a) (sizeof(a) / sizeof(a[0]))

#define ALIGN_STACK(n) __aligned(int, n) __dummy;

#ifndef RESTRICT

    #ifdef __INTEL_COMPILER

        #define RESTRICT restrict

    #elif _MSC_VER >= 1400 // TODO: gcc

        #define RESTRICT __restrict

    #else

        #define RESTRICT

    #endif

#endif

#if defined(_DEBUG) //&& defined(_MSC_VER)

	#include <assert.h>
	#define ASSERT assert

#else

	#define ASSERT(exp) ((void)0)

#endif

#ifdef __x86_64__

	#define _M_AMD64

#endif

// sse

#if !defined(_M_SSE) && (!defined(_WINDOWS) || defined(_M_AMD64) || defined(_M_IX86_FP) && _M_IX86_FP >= 2)

	#define _M_SSE 0x200

#endif

#if _M_SSE >= 0x200

	#include <xmmintrin.h>
	#include <emmintrin.h>

	#ifndef _MM_DENORMALS_ARE_ZERO
	#define _MM_DENORMALS_ARE_ZERO 0x0040
	#endif

	#define MXCSR (_MM_DENORMALS_ARE_ZERO | _MM_MASK_MASK | _MM_ROUND_NEAREST | _MM_FLUSH_ZERO_ON)

	#if defined(_MSC_VER) && _MSC_VER < 1500

	__forceinline __m128i _mm_castps_si128(__m128 a) {return *(__m128i*)&a;}
	__forceinline __m128 _mm_castsi128_ps(__m128i a) {return *(__m128*)&a;}
	__forceinline __m128i _mm_castpd_si128(__m128d a) {return *(__m128i*)&a;}
	__forceinline __m128d _mm_castsi128_pd(__m128i a) {return *(__m128d*)&a;}
	__forceinline __m128d _mm_castps_pd(__m128 a) {return *(__m128d*)&a;}
	__forceinline __m128 _mm_castpd_ps(__m128d a) {return *(__m128*)&a;}

	#endif

	#define _MM_TRANSPOSE4_SI128(row0, row1, row2, row3) \
	{ \
		__m128 tmp0 = _mm_shuffle_ps(_mm_castsi128_ps(row0), _mm_castsi128_ps(row1), 0x44); \
		__m128 tmp2 = _mm_shuffle_ps(_mm_castsi128_ps(row0), _mm_castsi128_ps(row1), 0xEE); \
		__m128 tmp1 = _mm_shuffle_ps(_mm_castsi128_ps(row2), _mm_castsi128_ps(row3), 0x44); \
		__m128 tmp3 = _mm_shuffle_ps(_mm_castsi128_ps(row2), _mm_castsi128_ps(row3), 0xEE); \
		(row0) = _mm_castps_si128(_mm_shuffle_ps(tmp0, tmp1, 0x88)); \
		(row1) = _mm_castps_si128(_mm_shuffle_ps(tmp0, tmp1, 0xDD)); \
		(row2) = _mm_castps_si128(_mm_shuffle_ps(tmp2, tmp3, 0x88)); \
		(row3) = _mm_castps_si128(_mm_shuffle_ps(tmp2, tmp3, 0xDD)); \
	}

#else

#error TODO: GSVector4 and GSRasterizer needs SSE2

#endif

#if _M_SSE >= 0x301

	#include <tmmintrin.h>

#endif

#if _M_SSE >= 0x401

	#include <smmintrin.h>

#endif

#if _M_SSE >= 0x500

	#include <immintrin.h>

#endif

#undef min
#undef max
#undef abs

#if !defined(_MSC_VER)

	#if !defined(HAVE_ALIGNED_MALLOC)

	extern void* _aligned_malloc(size_t size, size_t alignment);
	extern void _aligned_free(void* p);

	#endif

	// http://svn.reactos.org/svn/reactos/trunk/reactos/include/crt/mingw32/intrin_x86.h?view=markup
	// - the other intrin_x86.h of pcsx2 is not up to date, its _interlockedbittestandreset simply does not work.

	__forceinline unsigned char _BitScanForward(unsigned long* const Index, const unsigned long Mask)
	{
		__asm__("bsfl %[Mask], %[Index]" : [Index] "=r" (*Index) : [Mask] "mr" (Mask));
		return Mask ? 1 : 0;
	}

	__forceinline unsigned char _interlockedbittestandreset(volatile long* a, const long b)
	{
		unsigned char retval;
		__asm__("lock; btrl %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval), [a] "+m" (*a) : [b] "Ir" (b) : "memory");
		return retval;
	}

	__forceinline unsigned char _interlockedbittestandset(volatile long* a, const long b)
	{
		unsigned char retval;
		__asm__("lock; btsl %[b], %[a]; setc %b[retval]" : [retval] "=q" (retval), [a] "+m" (*a) : [b] "Ir" (b) : "memory");
		return retval;
	}

	#ifdef __GNUC__

	__forceinline unsigned long long __rdtsc()
	{
		#if defined(__amd64__) || defined(__x86_64__)
		unsigned long long low, high;
		__asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
		return low | (high << 32);
		#else
		unsigned long long retval;
		__asm__ __volatile__("rdtsc" : "=A"(retval));
		return retval;
		#endif
	}

	#endif

#endif

extern void* vmalloc(size_t size, bool code);
extern void vmfree(void* ptr, size_t size);

#ifdef _WINDOWS

	#ifdef ENABLE_VTUNE

	#include <JITProfiling.h>

	#pragma comment(lib, "jitprofiling.lib")

	#endif

#endif
