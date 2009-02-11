#pragma once

#ifndef _DEBUG
#define NO_CRT
#endif

#ifdef NO_CRT
#define _CRT_ALLOCATION_DEFINED
#endif

#define UNICODE

#define PADdefs
//#define SIOdefs
#define WM_XBUTTONDOWN                  0x020B
#define WM_XBUTTONUP                    0x020C
#define WM_XBUTTONDBLCLK                0x020D

#define XBUTTON1      0x0001
#define XBUTTON2      0x0002

#define WM_MOUSEHWHEEL 0x020E

//#define WIN32_LEAN_AND_MEAN
//#define NOGDI
#define _CRT_SECURE_NO_DEPRECATE
// Actually works with 0x0400, but need this to get raw input structures.
#define WINVER 0x0501
#define _WIN32_WINNT WINVER
#define __MSCW32__

#include <stdio.h>
#include <stdlib.h>
#ifdef NDEBUG

#if (_MSC_VER<1300)
	#pragma comment(linker,"/RELEASE")
	#pragma comment(linker,"/opt:nowin98")
#endif
#endif

#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <commctrl.h>

extern HINSTANCE hInst;


#include "resource.h"
#include "PS2Etypes.h"

/*
inline void Log(char *s) {
	FILE *out2 = fopen("logs\\padLog.txt", "ab");
	if (out2) {
		fprintf(out2, "%s\n", s);
		fclose(out2);
	}
	int w = GetCurrentThreadId();
	char junk[1000];
	sprintf(junk, "logs\\padLog%i.txt", w);
	out2 = fopen(junk, "ab");
	if (out2) {
		fprintf(out2, "%s\n", s);
		fclose(out2);
	}
}
//*/

#ifdef NO_CRT

inline void * malloc(size_t size) {
	return HeapAlloc(GetProcessHeap(), 0, size);
}

inline void * calloc(size_t num, size_t size) {
	size *= num;
	void *out = malloc(size);
	if (out) memset(out, 0, size);
	return out;
}

inline void free(__inout_opt void * mem) {
	if (mem) HeapFree(GetProcessHeap(), 0, mem);
}

inline void * realloc(void *mem, size_t size) {
	if (!mem) {
		return malloc(size);
	}

	if (!size) {
		free(mem);
		return 0;
	}
	return HeapReAlloc(GetProcessHeap(), 0, mem, size);
}

inline wchar_t * __cdecl wcsdup(const wchar_t *in) {
	size_t size = sizeof(wchar_t) * (1+wcslen(in));
	wchar_t *out = (wchar_t*) malloc(size);
	if (out)
		memcpy(out, in, size);
	return out;
}

__forceinline void * __cdecl operator new(size_t lSize) {
	return HeapAlloc(GetProcessHeap(), 0, lSize);
}

__forceinline void __cdecl operator delete(void *pBlock) {
	HeapFree(GetProcessHeap(), 0, pBlock);
}

#endif

__forceinline int __cdecl wcsicmp(const wchar_t *s1, const wchar_t *s2) {
	int res = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, s1, -1, s2, -1);
	if (res) return res-2;
	return res;
}
