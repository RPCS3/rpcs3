#include "stdafx.h"
#include "BufferUtils.h"
#include "../rsx_methods.h"

#define MIN2(x, y) ((x) < (y)) ? (x) : (y)
#define MAX2(x, y) ((x) > (y)) ? (x) : (y)


namespace
{
	/**
	 * Convert CMP vector to RGBA16.
	 * A vector in CMP (compressed) format is stored as X11Y11Z10 and has a W component of 1.
	 * X11 and Y11 channels are int between -1024 and 1023 interpreted as -1.f, 1.f
	 * Z10 is int between -512 and 511 interpreted as -1.f, 1.f
	 */
	std::array<u16, 4> decode_cmp_vector(u32 encoded_vector)
	{
		u16 Z = encoded_vector >> 22;
		Z = Z << 6;
		u16 Y = (encoded_vector >> 11) & 0x7FF;
		Y = Y << 5;
		u16 X = encoded_vector & 0x7FF;
		X = X << 5;
		return{ X, Y, Z, 1 };
	}
}

void write_vertex_array_data_to_buffer(void *buffer, u32 first, u32 count, size_t index, const rsx::data_array_format_info &vertex_array_desc)
{
	assert(vertex_array_desc.size > 0);

	if (vertex_array_desc.frequency > 1)
		LOG_ERROR(RSX, "%s: frequency is not null (%d, index=%d)", __FUNCTION__, vertex_array_desc.frequency, index);

	u32 offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index];
	u32 address = rsx::get_address(offset & 0x7fffffff, offset >> 31);

	u32 element_size = rsx::get_vertex_type_size_on_host(vertex_array_desc.type, vertex_array_desc.size);

	u32 base_offset = rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
	u32 base_index = rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_INDEX];

	for (u32 i = 0; i < count; ++i)
	{
		auto src = vm::ps3::_ptr<const u8>(address + base_offset + vertex_array_desc.stride * (first + i + base_index));
		u8* dst = (u8*)buffer + i * element_size;

		switch (vertex_array_desc.type)
		{
		case Vertex_base_type::ub:
			memcpy(dst, src, vertex_array_desc.size);
			break;

		case Vertex_base_type::s1:
		case Vertex_base_type::sf:
		{
			auto* c_src = (const be_t<u16>*)src;
			u16* c_dst = (u16*)dst;

			for (u32 j = 0; j < vertex_array_desc.size; ++j)
			{
				*c_dst++ = *c_src++;
			}
			if (vertex_array_desc.size * sizeof(u16) < element_size)
				*c_dst++ = 0x3800;
			break;
		}

		case Vertex_base_type::f:
		case Vertex_base_type::s32k:
		case Vertex_base_type::ub256:
		{
			auto* c_src = (const be_t<u32>*)src;
			u32* c_dst = (u32*)dst;

			for (u32 j = 0; j < vertex_array_desc.size; ++j)
			{
				*c_dst++ = *c_src++;
			}
			break;
		}
		case Vertex_base_type::cmp:
		{
			auto* c_src = (const be_t<u32>*)src;
			const auto& decoded_vector = decode_cmp_vector(*c_src);
			u16* c_dst = (u16*)dst;
			c_dst[0] = decoded_vector[0];
			c_dst[1] = decoded_vector[1];
			c_dst[2] = decoded_vector[2];
			c_dst[3] = decoded_vector[3];
			break;
		}
		}
	}
}

namespace
{
template<typename IndexType>
void uploadAsIt(char *dst, u32 address, size_t indexCount, bool is_primitive_restart_enabled, u32 primitive_restart_index, u32 &min_index, u32 &max_index)
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
void expandIndexedTriangleFan(char *dst, u32 address, size_t indexCount, bool is_primitive_restart_enabled, u32 primitive_restart_index, u32 &min_index, u32 &max_index)
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
void expandIndexedQuads(char *dst, u32 address, size_t indexCount, bool is_primitive_restart_enabled, u32 primitive_restart_index, u32 &min_index, u32 &max_index)
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
bool is_primitive_native(unsigned m_draw_mode)
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

size_t get_index_count(unsigned m_draw_mode, unsigned initial_index_count)
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

size_t get_index_type_size(u32 type)
{
	switch (type)
	{
	case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16: return 2;
	case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32: return 4;
	default: return 0;
	}
}

void write_index_array_for_non_indexed_non_native_primitive_to_buffer(char* dst, unsigned draw_mode, unsigned first, unsigned count)
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

void write_index_array_data_to_buffer(char* dst, unsigned m_draw_mode, unsigned first, unsigned count, unsigned &min_index, unsigned &max_index)
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

void stream_vector(void *dst, u32 x, u32 y, u32 z, u32 w)
{
	__m128i vector = _mm_set_epi32(w, z, y, x);
	_mm_stream_si128((__m128i*)dst, vector);
}

void stream_vector_from_memory(void *dst, void *src)
{
	const __m128i &vector = _mm_loadu_si128((__m128i*)src);
	_mm_stream_si128((__m128i*)dst, vector);
}
