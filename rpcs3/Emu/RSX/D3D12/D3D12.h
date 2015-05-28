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

inline
D3D12_RESOURCE_DESC getBufferResourceDesc(size_t sizeInByte)
{
	D3D12_RESOURCE_DESC BufferDesc = {};
	BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	BufferDesc.Width = (UINT)sizeInByte;
	BufferDesc.Height = 1;
	BufferDesc.DepthOrArraySize = 1;
	BufferDesc.SampleDesc.Count = 1;
	BufferDesc.MipLevels = 1;
	BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	return BufferDesc;
}

inline
D3D12_RESOURCE_DESC getTexture2DResourceDesc(size_t width, size_t height, DXGI_FORMAT dxgiFormat)
{
	D3D12_RESOURCE_DESC result;
	result = {};
	result.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	result.Width = (UINT)width;
	result.Height = (UINT)height;
	result.Format = dxgiFormat;
	result.DepthOrArraySize = 1;
	result.SampleDesc.Count = 1;
	result.MipLevels = 1;
	return result;
}

inline
D3D12_RESOURCE_BARRIER getResourceBarrierTransition(ID3D12Resource *res, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = res;
	barrier.Transition.StateBefore = stateBefore;
	barrier.Transition.StateAfter = stateAfter;
	return barrier;
}

#endif