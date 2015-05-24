#pragma once
#if defined(DX12_SUPPORT)

#include <d3d12.h>

inline
void check(HRESULT hr)
{
	if (hr != 0)
		abort();
}

/**
 * Send data to dst pointer without polluting cache.
 * Usefull to write to mapped memory from upload heap.
 */
inline
void streamToBuffer(void* dst, void* src, size_t sizeInBytes)
{
	for (unsigned i = 0; i < sizeInBytes / 16; i++)
	{
		__m128i *srcPtr = (__m128i*) ((char*)src + i * 16);
		_mm_stream_si128((__m128i*)((char*)dst + i * 16), *srcPtr);
	}
}

#endif