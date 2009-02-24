// Includes Windows.h, has inlined versions of memory allocation and
// string comparison functions needed to avoid using CRT.  This reduces
// dll size by over 100k while avoiding any dependencies on updated CRT dlls.
#pragma once

#ifdef NO_CRT
#define _CRT_ALLOCATION_DEFINED
#endif

#define UNICODE

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

// Actually works with 0x0400, but need 0x500 to get XBUTTON defines,
// 0x501 to get raw input structures, and 0x0600 to get WM_MOUSEHWHEEL.
#define WINVER 0x0600
#define _WIN32_WINNT WINVER
#define __MSCW32__


#include <windows.h>

extern HINSTANCE hInst;

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

inline void * __cdecl operator new(size_t lSize) {
	return HeapAlloc(GetProcessHeap(), 0, lSize);
}

inline void __cdecl operator delete(void *pBlock) {
	HeapFree(GetProcessHeap(), 0, pBlock);
}

inline int __cdecl wcsicmp(const wchar_t *s1, const wchar_t *s2) {
	int res = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, s1, -1, s2, -1);
	if (res) return res-2;
	return res;
}

#endif
