#pragma once
#if defined(DX12_SUPPORT)

#include <d3d12.h>
#include <cassert>

inline
void check(HRESULT hr)
{
	if (hr != 0)
		abort();
}

/**
 * Get next value that is aligned by the corresponding power of 2
 */
inline
size_t powerOf2Align(size_t unalignedVal, size_t powerOf2)
{
	// check that powerOf2 is power of 2
	assert(!(powerOf2 & (powerOf2 - 1)));
	return (unalignedVal + powerOf2 - 1) & ~(powerOf2 - 1);
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

/**
* copy src to dst pointer without polluting cache.
* Usefull to write to mapped memory from upload heap.
*/
inline
void streamBuffer(void* dst, void* src, size_t sizeInBytes)
{
	// Assume 64 bytes cache line
	assert(powerOf2Align(sizeInBytes, 64));
	for (unsigned i = 0; i < sizeInBytes / 64; i++)
	{
		char *line = (char*)src + i * 64;
		_mm_prefetch(line, _MM_HINT_NTA);
		__m128i *srcPtr = (__m128i*) (line);
		_mm_stream_si128((__m128i*)((char*)dst + i * 64), *srcPtr);
		srcPtr = (__m128i*) (line + 16);
		_mm_stream_si128((__m128i*)((char*)dst + i * 64 + 16), *srcPtr);
		srcPtr = (__m128i*) (line + 32);
		_mm_stream_si128((__m128i*)((char*)dst + i * 64 + 32), *srcPtr);
		srcPtr = (__m128i*) (line + 48);
		_mm_stream_si128((__m128i*)((char*)dst + i * 64 + 48), *srcPtr);
	}
}

#endif