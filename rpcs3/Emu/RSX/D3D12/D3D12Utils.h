#pragma once

#include <d3d12.h>
#include <cassert>
#include <wrl/client.h>
#include "Utilities/Log.h"
#include "Emu/Memory/vm.h"
#include "Emu/RSX/GCM.h"


// From llvm Compiler.h

// Need to be set by define
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

/// \macro LLVM_GNUC_PREREQ
/// \brief Extend the default __GNUC_PREREQ even if glibc's features.h isn't
/// available.
#ifndef LLVM_GNUC_PREREQ
# if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#define LLVM_GNUC_PREREQ(maj, min, patch) \
	((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) + __GNUC_PATCHLEVEL__ >= \
	((maj) << 20) + ((min) << 10) + (patch))
# elif defined(__GNUC__) && defined(__GNUC_MINOR__)
#define LLVM_GNUC_PREREQ(maj, min, patch) \
	((__GNUC__ << 20) + (__GNUC_MINOR__ << 10) >= ((maj) << 20) + ((min) << 10))
#else
#define LLVM_GNUC_PREREQ(maj, min, patch) 0
#endif
#endif

#ifdef __GNUC__
#define LLVM_ATTRIBUTE_NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define LLVM_ATTRIBUTE_NORETURN __declspec(noreturn)
#else
#define LLVM_ATTRIBUTE_NORETURN
#endif

#if __has_builtin(__builtin_unreachable) || LLVM_GNUC_PREREQ(4, 5, 0)
# define LLVM_BUILTIN_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
# define LLVM_BUILTIN_UNREACHABLE __assume(false)
#endif

LLVM_ATTRIBUTE_NORETURN void unreachable_internal(const char *msg = nullptr, const char *file = nullptr, unsigned line = 0);

/// Marks that the current location is not supposed to be reachable.
/// In !NDEBUG builds, prints the message and location info to stderr.
/// In NDEBUG builds, becomes an optimizer hint that the current location
/// is not supposed to be reachable.  On compilers that don't support
/// such hints, prints a reduced message instead.
///
/// Use this instead of assert(0).  It conveys intent more clearly and
/// allows compilers to omit some unnecessary code.
#ifndef NDEBUG
#define unreachable(msg) \
	unreachable_internal(msg, __FILE__, __LINE__)
#elif defined(LLVM_BUILTIN_UNREACHABLE)
#define unreachable(msg) LLVM_BUILTIN_UNREACHABLE
#else
#define unreachable(msg) unreachable_internal()
#endif

using namespace Microsoft::WRL;

// From DX12 D3D11On12 Sample (MIT Licensed)
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}

/**
 * Send data to dst pointer without polluting cache.
 * Usefull to write to mapped memory from upload heap.
 */
inline
void streamToBuffer(void* dst, void* src, size_t sizeInBytes)
{
	for (int i = 0; i < sizeInBytes / 16; i++)
	{
		const __m128i &srcPtr = _mm_loadu_si128((__m128i*) ((char*)src + i * 16));
		_mm_stream_si128((__m128i*)((char*)dst + i * 16), srcPtr);
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
	int offset = 0;
	bool isAligned = !((size_t)src & 15);
	for (offset = 0; offset < sizeInBytes - 64; offset += 64)
	{
		char *line = (char*)src + offset;
		char *dstline = (char*)dst + offset;
		// prefetch next line
		_mm_prefetch(line + 16, _MM_HINT_NTA);
		__m128i srcPtr = isAligned ? _mm_load_si128((__m128i *)line) : _mm_loadu_si128((__m128i *)line);
		_mm_stream_si128((__m128i*)dstline, srcPtr);
		srcPtr = isAligned ? _mm_load_si128((__m128i *)(line + 16)) : _mm_loadu_si128((__m128i *)(line + 16));
		_mm_stream_si128((__m128i*)(dstline + 16), srcPtr);
		srcPtr = isAligned ? _mm_load_si128((__m128i *)(line + 32)) : _mm_loadu_si128((__m128i *)(line + 32));
		_mm_stream_si128((__m128i*)(dstline + 32), srcPtr);
		srcPtr = isAligned ? _mm_load_si128((__m128i *)(line + 48)) : _mm_loadu_si128((__m128i *)(line + 48));
		_mm_stream_si128((__m128i*)(dstline + 48), srcPtr);
	}
	memcpy((char*)dst + offset, (char*)src + offset, sizeInBytes - offset);
}
