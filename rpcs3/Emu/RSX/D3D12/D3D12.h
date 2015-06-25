#pragma once
#if defined(DX12_SUPPORT)

#include <d3d12.h>
#include <cassert>
#include "utilities/Log.h"
#include "Emu/Memory/vm.h"
#include "Emu/RSX/GCM.h"

#pragma comment (lib, "dxgi.lib")

#define SAFE_RELEASE(x) if (x) x->Release();

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
#pragma omp parallel for
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
	#pragma omp parallel for
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
D3D12_RESOURCE_DESC getTexture2DResourceDesc(size_t width, size_t height, DXGI_FORMAT dxgiFormat, size_t mipmapLevels)
{
	D3D12_RESOURCE_DESC result;
	result = {};
	result.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	result.Width = (UINT)width;
	result.Height = (UINT)height;
	result.Format = dxgiFormat;
	result.DepthOrArraySize = 1;
	result.SampleDesc.Count = 1;
	result.MipLevels = (UINT16)mipmapLevels;
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

/**
 * Convert GCM blend operator code to D3D12 one
 */
inline D3D12_BLEND_OP getBlendOp(u16 op)
{
	switch (op)
	{
	case CELL_GCM_FUNC_ADD: return D3D12_BLEND_OP_ADD;
	case CELL_GCM_FUNC_SUBTRACT: return D3D12_BLEND_OP_SUBTRACT;
	case CELL_GCM_FUNC_REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
	case CELL_GCM_MIN: return D3D12_BLEND_OP_MIN;
	case CELL_GCM_MAX: return D3D12_BLEND_OP_MAX;
	default:
	case CELL_GCM_FUNC_ADD_SIGNED:
	case CELL_GCM_FUNC_REVERSE_ADD_SIGNED:
	case CELL_GCM_FUNC_REVERSE_SUBTRACT_SIGNED:
		LOG_WARNING(RSX, "Unsupported Blend Op %d", op);
		return D3D12_BLEND_OP();
	}
}

/**
 * Convert GCM blend factor code to D3D12 one
 */
inline D3D12_BLEND getBlendFactor(u16 factor)
{
	switch (factor)
	{
	case CELL_GCM_ZERO: return D3D12_BLEND_ZERO;
	case CELL_GCM_ONE: return D3D12_BLEND_ONE;
	case CELL_GCM_SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
	case CELL_GCM_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
	case CELL_GCM_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
	case CELL_GCM_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
	case CELL_GCM_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
	case CELL_GCM_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
	case CELL_GCM_DST_COLOR: return D3D12_BLEND_DEST_COLOR;
	case CELL_GCM_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
	case CELL_GCM_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
	default:
	case CELL_GCM_CONSTANT_COLOR:
	case CELL_GCM_ONE_MINUS_CONSTANT_COLOR:
	case CELL_GCM_CONSTANT_ALPHA:
	case CELL_GCM_ONE_MINUS_CONSTANT_ALPHA:
		LOG_WARNING(RSX, "Unsupported Blend Factor %d", factor);
		return D3D12_BLEND();
	}
}

/**
 * Convert GCM logic op code to D3D12 one
 */
inline D3D12_LOGIC_OP getLogicOp(u32 op)
{
	switch (op)
	{
	default:
		LOG_WARNING(RSX, "Unsupported Logic Op %d", op);
		return D3D12_LOGIC_OP();
	case CELL_GCM_CLEAR: return D3D12_LOGIC_OP_CLEAR;
	case CELL_GCM_AND: return D3D12_LOGIC_OP_AND;
	case CELL_GCM_AND_REVERSE: return D3D12_LOGIC_OP_AND_REVERSE;
	case CELL_GCM_COPY: return D3D12_LOGIC_OP_COPY;
	case CELL_GCM_AND_INVERTED: return D3D12_LOGIC_OP_AND_INVERTED;
	case CELL_GCM_NOOP: return D3D12_LOGIC_OP_NOOP;
	case CELL_GCM_XOR: return D3D12_LOGIC_OP_XOR;
	case CELL_GCM_OR: return D3D12_LOGIC_OP_OR;
	case CELL_GCM_NOR: return D3D12_LOGIC_OP_NOR;
	case CELL_GCM_EQUIV: return D3D12_LOGIC_OP_EQUIV;
	case CELL_GCM_INVERT: return D3D12_LOGIC_OP_INVERT;
	case CELL_GCM_OR_REVERSE: return D3D12_LOGIC_OP_OR_REVERSE;
	case CELL_GCM_COPY_INVERTED: return D3D12_LOGIC_OP_COPY_INVERTED;
	case CELL_GCM_OR_INVERTED: return D3D12_LOGIC_OP_OR_INVERTED;
	case CELL_GCM_NAND: return D3D12_LOGIC_OP_NAND;
	}
}

/**
 * Convert GCM stencil op code to D3D12 one
 */
inline D3D12_STENCIL_OP getStencilOp(u32 op)
{
	switch (op)
	{
	case CELL_GCM_KEEP: return D3D12_STENCIL_OP_KEEP;
	case CELL_GCM_ZERO: return D3D12_STENCIL_OP_ZERO;
	case CELL_GCM_REPLACE: return D3D12_STENCIL_OP_REPLACE;
	case CELL_GCM_INCR: return D3D12_STENCIL_OP_INCR;
	case CELL_GCM_DECR: return D3D12_STENCIL_OP_DECR;
	default:
	case CELL_GCM_INCR_WRAP:
	case CELL_GCM_DECR_WRAP:
		LOG_WARNING(RSX, "Unsupported Stencil Op %d", op);
		return D3D12_STENCIL_OP();
	}
}

/**
 * Convert GCM comparison function code to D3D12 one.
 */
inline D3D12_COMPARISON_FUNC getCompareFunc(u32 op)
{
	switch (op)
	{
	case CELL_GCM_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
	case CELL_GCM_LESS: return D3D12_COMPARISON_FUNC_LESS;
	case CELL_GCM_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
	case CELL_GCM_LEQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case CELL_GCM_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
	case CELL_GCM_NOTEQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case CELL_GCM_GEQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case CELL_GCM_ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
	default:
		LOG_WARNING(RSX, "Unsupported Compare Function %d", op);
		return D3D12_COMPARISON_FUNC();
	}
}

/**
 * Convert GCM texture format to an equivalent one supported by D3D12.
 * Destination format may require a byte swap or data conversion.
 */
inline DXGI_FORMAT getTextureDXGIFormat(int format)
{
	switch (format)
	{
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
	default:
		LOG_ERROR(RSX, "Unimplemented Texture format : %x", format);
		return DXGI_FORMAT();
	case CELL_GCM_TEXTURE_B8:
		return DXGI_FORMAT_R8_UNORM;
	case CELL_GCM_TEXTURE_A1R5G5B5:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case CELL_GCM_TEXTURE_A4R4G4B4:
		return DXGI_FORMAT_B4G4R4A4_UNORM;
	case CELL_GCM_TEXTURE_R5G6B5:
		return DXGI_FORMAT_B5G6R5_UNORM;
	case CELL_GCM_TEXTURE_A8R8G8B8:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		return DXGI_FORMAT_BC1_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		return DXGI_FORMAT_BC2_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return DXGI_FORMAT_BC3_UNORM;
	case CELL_GCM_TEXTURE_G8B8:
		return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case CELL_GCM_TEXTURE_R6G5B5:
		// Not native
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case CELL_GCM_TEXTURE_DEPTH24_D8:
		return DXGI_FORMAT_R32_UINT;
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	case CELL_GCM_TEXTURE_DEPTH16:
		return DXGI_FORMAT_R16_UNORM;
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		return DXGI_FORMAT_R16_FLOAT;
	case CELL_GCM_TEXTURE_X16:
		return DXGI_FORMAT_R16_UNORM;
	case CELL_GCM_TEXTURE_Y16_X16:
		return DXGI_FORMAT_R16G16_UNORM;
	case CELL_GCM_TEXTURE_R5G5B5A1:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case CELL_GCM_TEXTURE_X32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	case CELL_GCM_TEXTURE_D1R5G5B5:
		return DXGI_FORMAT_B5G5R5A1_UNORM;
	case CELL_GCM_TEXTURE_D8R8G8B8:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		return DXGI_FORMAT_G8R8_G8B8_UNORM;
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		return DXGI_FORMAT_R8G8_B8G8_UNORM;
	}
}

#endif
