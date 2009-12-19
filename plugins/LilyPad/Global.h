// Includes Windows.h, has inlined versions of memory allocation and
// string comparison functions needed to avoid using CRT.  This reduces
// dll size by over 100k while avoiding any dependencies on updated CRT dlls.
#pragma once

#define DIRECTINPUT_VERSION 0x0800

#ifdef NO_CRT
#define _CRT_ALLOCATION_DEFINED

inline void * malloc(size_t size);
inline void * calloc(size_t num, size_t size);
inline void free(void * mem);
inline void * realloc(void *mem, size_t size);
#endif

#define UNICODE

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef _MSC_VER
#define EXPORT_C_(type) extern "C" __declspec(dllexport) type CALLBACK
#else
#define EXPORT_C_(type) extern "C" type
#endif

// Actually works with 0x0400, but need 0x500 to get XBUTTON defines,
// 0x501 to get raw input structures, and 0x0600 to get WM_MOUSEHWHEEL.
#define WINVER 0x0600
#define _WIN32_WINNT WINVER
#define __MSCW32__


#include <windows.h>

#ifdef PCSX2_DEBUG
#define _CRTDBG_MAPALLOC
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#else
#include <stdlib.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <commctrl.h>
// Only needed for DBT_DEVNODES_CHANGED
#include <Dbt.h>

#include "PS2Edefs.h"

extern HINSTANCE hInst;
// Needed for config screen
void GetNameAndVersionString(wchar_t *out);

typedef struct {
	unsigned char controllerType;
	unsigned short buttonStatus;
	unsigned char rightJoyX, rightJoyY, leftJoyX, leftJoyY;
	unsigned char moveX, moveY;
	unsigned char reserved[91];
} PadDataS;

EXPORT_C_(void) PADupdate(int pad);
EXPORT_C_(u32) PS2EgetLibType(void);
EXPORT_C_(u32) PS2EgetLibVersion2(u32 type);
EXPORT_C_(char*) PSEgetLibName();
EXPORT_C_(char*) PS2EgetLibName(void);
EXPORT_C_(void) PADshutdown();
EXPORT_C_(s32) PADinit(u32 flags);
EXPORT_C_(s32) PADopen(void *pDsp);
EXPORT_C_(void) PADclose();
EXPORT_C_(u8) PADstartPoll(int pad);
EXPORT_C_(u8) PADpoll(u8 value);
EXPORT_C_(u32) PADquery();
EXPORT_C_(void) PADabout();
EXPORT_C_(s32) PADtest();
EXPORT_C_(keyEvent*) PADkeyEvent();
EXPORT_C_(u32) PADreadPort1 (PadDataS* pads);
EXPORT_C_(u32) PADreadPort2 (PadDataS* pads);
EXPORT_C_(u32) PSEgetLibType();
EXPORT_C_(u32) PSEgetLibVersion();
EXPORT_C_(void) PADconfigure();
EXPORT_C_(s32) PADfreeze(int mode, freezeData *data);
EXPORT_C_(s32) PADsetSlot(u8 port, u8 slot);
EXPORT_C_(s32) PADqueryMtap(u8 port);

#ifdef NO_CRT

#define wcsdup MyWcsdup
#define wcsicmp MyWcsicmp

inline void * malloc(size_t size) {
	return HeapAlloc(GetProcessHeap(), 0, size);
}

inline void * calloc(size_t num, size_t size) {
	size *= num;
	void *out = malloc(size);
	if (out) memset(out, 0, size);
	return out;
}

inline void free(void * mem) {
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

inline void * __cdecl operator new(size_t lSize) {
	return HeapAlloc(GetProcessHeap(), 0, lSize);
}

inline void __cdecl operator delete(void *pBlock) {
	HeapFree(GetProcessHeap(), 0, pBlock);
}

inline wchar_t * __cdecl wcsdup(const wchar_t *in) {
	size_t size = sizeof(wchar_t) * (1+wcslen(in));
	wchar_t *out = (wchar_t*) malloc(size);
	if (out)
		memcpy(out, in, size);
	return out;
}

inline int __cdecl wcsicmp(const wchar_t *s1, const wchar_t *s2) {
	int res = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, s1, -1, s2, -1);
	if (res) return res-2;
	return res;
}

#endif
