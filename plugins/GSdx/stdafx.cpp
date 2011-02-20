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

#ifdef _WINDOWS

void* vmalloc(size_t size, bool code)
{
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, code ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
}

void vmfree(void* ptr, size_t size)
{
    VirtualFree(ptr, 0, MEM_RELEASE);
}

#else

#include <sys/mman.h>

void* vmalloc(size_t size, bool code)
{
    size_t mask = getpagesize() - 1;

    size = (size + mask) & ~mask;

    int flags = PROT_READ | PROT_WRITE;

    if(code)
    {
        flags |= PROT_EXEC;
    }

    return mmap(NULL, size, flags, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void vmfree(void* ptr, size_t size)
{
    size_t mask = getpagesize() - 1;

    size = (size + mask) & ~mask;

    munmap(ptr, size);
}

#endif

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
