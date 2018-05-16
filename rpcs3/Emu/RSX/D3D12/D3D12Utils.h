#pragma once

#include <d3d12.h>
#include <cassert>
#include <wrl/client.h>
#include "Emu/Memory/vm.h"
#include "Emu/RSX/GCM.h"
#include <locale>
#include <comdef.h>


using namespace Microsoft::WRL;
extern ID3D12Device* g_d3d12_device;

inline std::string get_hresult_message(HRESULT hr)
{
	if (hr == DXGI_ERROR_DEVICE_REMOVED)
	{
		hr = g_d3d12_device->GetDeviceRemovedReason();
		return fmt::format("D3D12 device was removed with error status 0x%X", hr);
	}

	_com_error error(hr);
#ifndef UNICODE
	return error.ErrorMessage();
#else
	using convert_type = std::codecvt<wchar_t, char, std::mbstate_t>;
	return std::wstring_convert<convert_type>().to_bytes(error.ErrorMessage());
#endif
}

#define CHECK_HRESULT(expr) { HRESULT hr = (expr); if (FAILED(hr)) fmt::throw_exception("HRESULT = %s" HERE, get_hresult_message(hr)); }

/**
 * Send data to dst pointer without polluting cache.
 * Useful to write to mapped memory from upload heap.
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
* Useful to write to mapped memory from upload heap.
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
