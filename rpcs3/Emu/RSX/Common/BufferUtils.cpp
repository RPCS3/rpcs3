#include "stdafx.h"
#include "BufferUtils.h"
#include "Utilities/Log.h"

#define MIN2(x, y) ((x) < (y)) ? (x) : (y)
#define MAX2(x, y) ((x) > (y)) ? (x) : (y)

void write_vertex_array_data_to_buffer(void *buffer, u32 first, u32 count, size_t index, const rsx::data_array_format_info &vertex_array_desc) noexcept
{
	assert(vertex_array_desc.array);

	if (vertex_array_desc.frequency > 1)
		LOG_ERROR(RSX, "%s: frequency is not null (%d, index=%d)", __FUNCTION__, vertex_array_desc.frequency, index);

	u32 offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index];
	u32 address = rsx::get_address(offset & 0x7fffffff, offset >> 31);

	u32 type_size = rsx::get_vertex_type_size(vertex_array_desc.type);
	u32 element_size = type_size * vertex_array_desc.size;

	u32 base_offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
	u32 base_index = rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_INDEX];

	for (u32 i = 0; i < count; ++i)
	{
		auto src = vm::ps3::_ptr<const u8>(address + base_offset + vertex_array_desc.stride * (first + i + base_index));
		u8* dst = (u8*)buffer + i * element_size;

		switch (type_size)
		{
		case 1:
			memcpy(dst, src, vertex_array_desc.size);
			break;

		case 2:
		{
			auto* c_src = (const be_t<u16>*)src;
			u16* c_dst = (u16*)dst;

			for (u32 j = 0; j < vertex_array_desc.size; ++j)
			{
				*c_dst++ = *c_src++;
			}
			break;
		}

		case 4:
		{
			auto* c_src = (const be_t<u32>*)src;
			u32* c_dst = (u32*)dst;

			for (u32 j = 0; j < vertex_array_desc.size; ++j)
			{
				*c_dst++ = *c_src++;
			}
			break;
		}
		}
	}
}

namespace
{
template<typename IndexType>
void uploadAsIt(char *dst, u32 address, size_t indexCount, bool is_primitive_restart_enabled, u32 primitive_restart_index, u32 &min_index, u32 &max_index) noexcept
{
	for (u32 i = 0; i < indexCount; ++i)
	{
		IndexType index = vm::ps3::_ref<IndexType>(address + i * sizeof(IndexType));
		if (is_primitive_restart_enabled && index == (IndexType)primitive_restart_index)
			index = (IndexType)-1;
		(IndexType&)dst[i * sizeof(IndexType)] = index;
		if (is_primitive_restart_enabled && index == (IndexType)-1) // Cut
			continue;
		max_index = MAX2(max_index, index);
		min_index = MIN2(min_index, index);
	}
}

// FIXME: expanded primitive type may not support primitive restart correctly

template<typename IndexType>
void expandIndexedTriangleFan(char *dst, u32 address, size_t indexCount, bool is_primitive_restart_enabled, u32 primitive_restart_index, u32 &min_index, u32 &max_index) noexcept
{
	for (unsigned i = 0; i < indexCount - 2; i++)
	{
		IndexType index0 = vm::ps3::_ref<IndexType>(address);
		if (index0 == (IndexType)primitive_restart_index)
			index0 = (IndexType)-1;
		IndexType index1 = vm::ps3::_ref<IndexType>(address + (i + 2 - 1) * sizeof(IndexType));
		if (index1 == (IndexType)primitive_restart_index)
			index1 = (IndexType)-1;
		IndexType index2 = vm::ps3::_ref<IndexType>(address + (i + 2) * sizeof(IndexType));
		if (index2 == (IndexType)primitive_restart_index)
			index2 = (IndexType)-1;

		(IndexType&)dst[(3 * i) * sizeof(IndexType)] = index0;
		(IndexType&)dst[(3 * i + 1) * sizeof(IndexType)] = index1;
		(IndexType&)dst[(3 * i + 2) * sizeof(IndexType)] = index2;

		if (!is_primitive_restart_enabled || index0 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index0);
			max_index = MAX2(max_index, index0);
		}
		if (!is_primitive_restart_enabled || index1 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index1);
			max_index = MAX2(max_index, index1);
		}
		if (!is_primitive_restart_enabled || index2 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index2);
			max_index = MAX2(max_index, index2);
		}
	}
}

template<typename IndexType>
void expandIndexedQuads(char *dst, u32 address, size_t indexCount, bool is_primitive_restart_enabled, u32 primitive_restart_index, u32 &min_index, u32 &max_index) noexcept
{
	for (unsigned i = 0; i < indexCount / 4; i++)
	{
		IndexType index0 = vm::ps3::_ref<IndexType>(address + 4 * i * sizeof(IndexType));
		if (is_primitive_restart_enabled && index0 == (IndexType)primitive_restart_index)
			index0 = (IndexType)-1;
		IndexType index1 = vm::ps3::_ref<IndexType>(address + (4 * i + 1) * sizeof(IndexType));
		if (is_primitive_restart_enabled && index1 == (IndexType)primitive_restart_index)
			index1 = (IndexType)-1;
		IndexType index2 = vm::ps3::_ref<IndexType>(address + (4 * i + 2) * sizeof(IndexType));
		if (is_primitive_restart_enabled && index2 == (IndexType)primitive_restart_index)
			index2 = (IndexType)-1;
		IndexType index3 = vm::ps3::_ref<IndexType>(address + (4 * i + 3) * sizeof(IndexType));
		if (is_primitive_restart_enabled &&index3 == (IndexType)primitive_restart_index)
			index3 = (IndexType)-1;

		// First triangle
		(IndexType&)dst[(6 * i) * sizeof(IndexType)] = index0;
		(IndexType&)dst[(6 * i + 1) * sizeof(IndexType)] = index1;
		(IndexType&)dst[(6 * i + 2) * sizeof(IndexType)] = index2;
		// Second triangle
		(IndexType&)dst[(6 * i + 3) * sizeof(IndexType)] = index2;
		(IndexType&)dst[(6 * i + 4) * sizeof(IndexType)] = index3;
		(IndexType&)dst[(6 * i + 5) * sizeof(IndexType)] = index0;

		if (!is_primitive_restart_enabled || index0 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index0);
			max_index = MAX2(max_index, index0);
		}
		if (!is_primitive_restart_enabled || index1 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index1);
			max_index = MAX2(max_index, index1);
		}
		if (!is_primitive_restart_enabled || index2 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index2);
			max_index = MAX2(max_index, index2);
		}
		if (!is_primitive_restart_enabled || index3 != (IndexType)-1) // Cut
		{
			min_index = MIN2(min_index, index3);
			max_index = MAX2(max_index, index3);
		}
	}
}
}

// Only handle quads and triangle fan now
bool is_primitive_native(unsigned m_draw_mode) noexcept
{
	switch (m_draw_mode)
	{
	default:
	case CELL_GCM_PRIMITIVE_POINTS:
	case CELL_GCM_PRIMITIVE_LINES:
	case CELL_GCM_PRIMITIVE_LINE_LOOP:
	case CELL_GCM_PRIMITIVE_LINE_STRIP:
	case CELL_GCM_PRIMITIVE_TRIANGLES:
	case CELL_GCM_PRIMITIVE_TRIANGLE_STRIP:
	case CELL_GCM_PRIMITIVE_QUAD_STRIP:
		return true;
	case CELL_GCM_PRIMITIVE_POLYGON:
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN:
	case CELL_GCM_PRIMITIVE_QUADS:
		return false;
	}
}

/** We assume that polygon is convex in polygon mode (constraints in OpenGL)
 *In such case polygon triangulation equates to triangle fan with arbitrary start vertex
 * see http://www.gamedev.net/page/resources/_/technical/graphics-programming-and-theory/polygon-triangulation-r3334
 */

size_t get_index_count(unsigned m_draw_mode, unsigned initial_index_count) noexcept
{
	// Index count
	if (is_primitive_native(m_draw_mode))
		return initial_index_count;

	switch (m_draw_mode)
	{
	case CELL_GCM_PRIMITIVE_POLYGON:
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN:
		return (initial_index_count - 2) * 3;
	case CELL_GCM_PRIMITIVE_QUADS:
		return (6 * initial_index_count) / 4;
	default:
		return 0;
	}
}

size_t get_index_type_size(u32 type) noexcept
{
	switch (type)
	{
	case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16: return 2;
	case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32: return 4;
	default: return 0;
	}
}

void write_index_array_for_non_indexed_non_native_primitive_to_buffer(char* dst, unsigned draw_mode, unsigned first, unsigned count) noexcept
{
	unsigned short *typedDst = (unsigned short *)(dst);
	switch (draw_mode)
	{
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN:
	case CELL_GCM_PRIMITIVE_POLYGON:
		for (unsigned i = 0; i < (count - 2); i++)
		{
			typedDst[3 * i] = first;
			typedDst[3 * i + 1] = i + 2 - 1;
			typedDst[3 * i + 2] = i + 2;
		}
		return;
	case CELL_GCM_PRIMITIVE_QUADS:
		for (unsigned i = 0; i < count / 4; i++)
		{
			// First triangle
			typedDst[6 * i] = 4 * i + first;
			typedDst[6 * i + 1] = 4 * i + 1 + first;
			typedDst[6 * i + 2] = 4 * i + 2 + first;
			// Second triangle
			typedDst[6 * i + 3] = 4 * i + 2 + first;
			typedDst[6 * i + 4] = 4 * i + 3 + first;
			typedDst[6 * i + 5] = 4 * i + first;
		}
		return;
	}
}

void write_index_array_data_to_buffer(char* dst, unsigned m_draw_mode, unsigned first, unsigned count, unsigned &min_index, unsigned &max_index) noexcept
{
	u32 address = rsx::get_address(rsx::method_registers[NV4097_SET_INDEX_ARRAY_ADDRESS], rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] & 0xf);
	u32 type = rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4;

	u32 type_size = type == CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32 ? sizeof(u32) : sizeof(u16);

	u32 base_offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
	u32 base_index = 0;//rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_INDEX];
	bool is_primitive_restart_enabled = !!rsx::method_registers[NV4097_SET_RESTART_INDEX_ENABLE];
	u32 primitive_restart_index = rsx::method_registers[NV4097_SET_RESTART_INDEX];

	switch (m_draw_mode)
	{
	case CELL_GCM_PRIMITIVE_POINTS:
	case CELL_GCM_PRIMITIVE_LINES:
	case CELL_GCM_PRIMITIVE_LINE_LOOP:
	case CELL_GCM_PRIMITIVE_LINE_STRIP:
	case CELL_GCM_PRIMITIVE_TRIANGLES:
	case CELL_GCM_PRIMITIVE_TRIANGLE_STRIP:
	case CELL_GCM_PRIMITIVE_QUAD_STRIP:
	case CELL_GCM_PRIMITIVE_POLYGON:
		switch (type)
		{
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			uploadAsIt<u32>(dst, address + (first + base_index) * sizeof(u32), count, is_primitive_restart_enabled, primitive_restart_index, min_index, max_index);
			return;
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			uploadAsIt<u16>(dst, address + (first + base_index) * sizeof(u16), count, is_primitive_restart_enabled, primitive_restart_index, min_index, max_index);
			return;
		}
		return;
	case CELL_GCM_PRIMITIVE_TRIANGLE_FAN:
		switch (type)
		{
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			expandIndexedTriangleFan<u32>(dst, address + (first + base_index) * sizeof(u32), count, is_primitive_restart_enabled, primitive_restart_index, min_index, max_index);
			return;
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			expandIndexedTriangleFan<u16>(dst, address + (first + base_index) * sizeof(u16), count, is_primitive_restart_enabled, primitive_restart_index, min_index, max_index);
			return;
		}
	case CELL_GCM_PRIMITIVE_QUADS:
		switch (type)
		{
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			expandIndexedQuads<u32>(dst, address + (first + base_index) * sizeof(u32), count, is_primitive_restart_enabled, primitive_restart_index, min_index, max_index);
			return;
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			expandIndexedQuads<u16>(dst, address + (first + base_index) * sizeof(u16), count, is_primitive_restart_enabled, primitive_restart_index, min_index, max_index);
			return;
		}
	}
}

void stream_vector(void *dst, u32 x, u32 y, u32 z, u32 w) noexcept
{
	__m128i vector = _mm_set_epi32(w, z, y, x);
	_mm_stream_si128((__m128i*)dst, vector);
}

void stream_vector_from_memory(void *dst, void *src) noexcept
{
	const __m128i &vector = _mm_loadu_si128((__m128i*)src);
	_mm_stream_si128((__m128i*)dst, vector);
}