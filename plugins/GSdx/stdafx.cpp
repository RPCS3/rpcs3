// stdafx.cpp : source file that includes just the standard includes
// GSdx.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

string format(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	int result = -1, length = 256;

	char* buffer = NULL;

	while(result == -1)
	{
		if(buffer) delete [] buffer;

		buffer = new char[length + 1];

		memset(buffer, 0, length + 1);

		result = vsnprintf(buffer, length, fmt, args);

		length *= 2;
	}

	va_end(args);

	string s(buffer);

	delete [] buffer;

	return s;
}

void* vmalloc(size_t size, bool code)
{
#ifdef _WINDOWS
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, code ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
#else
    // TODO: linux
    return malloc(size);
#endif
}

void vmfree(void* ptr)
{
#ifdef _WINDOWS
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    // TODO: linux
    free(ptr);
#endif
}

#if !defined(_MSC_VER) && !defined(HAVE_ALIGNED_MALLOC)

// declare linux equivalents (alignment must be power of 2 (1,2,4...2^15)

void* _aligned_malloc(size_t size, size_t alignment)
{
	ASSERT(alignment <= 0x8000);
	size_t r = (size_t)malloc(size + --alignment + 2);
	size_t o = (r + 2 + alignment) & ~(size_t)alignment;
	if(!r) return NULL;
	((uint16*)o)[-1] = (uint16)(o-r);
	return (void*)o;
}

void _aligned_free(void* p)
{
	if(!p) return;
	free((void*)((size_t)p-((uint16*)p)[-1]));
}

#endif
