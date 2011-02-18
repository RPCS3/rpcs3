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

		result = _vsnprintf(buffer, length, fmt, args);

		length *= 2;
	}

	va_end(args);

	string s(buffer);

	delete [] buffer;

	return s;
}

#if !defined(_MSC_VER) && !defined(HAVE_ALIGNED_MALLOC)

// declare linux equivalents (alignment must be power of 2 (1,2,4...2^15)

void* pcsx2_aligned_malloc(size_t size, size_t alignment)
{
	assert(alignment <= 0x8000);
	uptr r = (uptr)malloc(size + --alignment + 2);
	uptr o = (r + 2 + alignment) & ~(uptr)alignment;
	if (!r) return NULL;
	((u16*)o)[-1] = (u16)(o-r);
	return (void*)o;
}

void pcsx2_aligned_free(void* p)
{
	if (!p) return;
	free((void*)((uptr)p-((u16*)p)[-1]));
}

#endif
