// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#pragma warning(disable: 4996 4995 4324 4100 4101 4201)

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
#include <winioctl.h>

#endif

// stdc

#include <math.h>
#include <time.h>
#include <intrin.h>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <hash_map>

using namespace std;
using namespace stdext;

extern string format(const char* fmt, ...);

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

#define EXPORT_C extern "C" __declspec(dllexport) void __stdcall
#define EXPORT_C_(type) extern "C" __declspec(dllexport) type __stdcall

#define ALIGN_STACK(n) __declspec(align(n)) int __dummy;

#ifndef RESTRICT
	#ifdef __INTEL_COMPILER
		#define RESTRICT restrict
	#elif _MSC_VER >= 1400 // TODO: gcc
		#define RESTRICT __restrict
	#else
		#define RESTRICT
	#endif
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
	#define ASSERT assert
#else
	#define ASSERT(exp) ((void)0)
#endif

#ifdef _M_SSE
	#error No SSE please!
#endif
