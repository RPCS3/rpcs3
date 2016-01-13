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

// FIXME: these functions shouldn't access rsx::method_registers (global)

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
template<typename T>
std::tuple<T, T> upload_untouched(gsl::span<to_be_t<const T>> src, gsl::span<T> dst, bool is_primitive_restart_enabled, T primitive_restart_index)
{
	T min_index = -1;
	T max_index = 0;

	Expects(dst.size_bytes() >= src.size_bytes());

	size_t dst_idx = 0;
	for (T index : src)
	{
		if (is_primitive_restart_enabled && index == primitive_restart_index)
		{
			index = -1;
		}
		else
		{
			max_index = MAX2(max_index, index);
			min_index = MIN2(min_index, index);
		}
		dst[dst_idx++] = index;
	}
	return std::make_tuple(min_index, max_index);
}

// FIXME: expanded primitive type may not support primitive restart correctly
template<typename T>
std::tuple<T, T> expand_indexed_triangle_fan(gsl::span<to_be_t<const T>> src, gsl::span<T> dst, bool is_primitive_restart_enabled, T primitive_restart_index)
{
	T min_index = -1;
	T max_index = 0;

	Expects(dst.size() >= 3 * (src.size() - 2));

	const T index0 = src[0];
	if (!is_primitive_restart_enabled || index0 != -1) // Cut
	{
		min_index = MIN2(min_index, index0);
		max_index = MAX2(max_index, index0);
	}

	size_t dst_idx = 0;
	while (src.size() > 2)
	{
		gsl::span<to_be_t<const T>> tri_indexes = src.subspan(0, 2);
		T index1 = tri_indexes[0];
		if (is_primitive_restart_enabled && index1 == primitive_restart_index)
		{
			index1 = -1;
		}
		else
		{
			min_index = MIN2(min_index, index1);
			max_index = MAX2(max_index, index1);
		}
		T index2 = tri_indexes[1];
		if (is_primitive_restart_enabled && index2 == primitive_restart_index)
		{
			index2 = -1;
		}
		else
		{
			min_index = MIN2(min_index, index2);
			max_index = MAX2(max_index, index2);
		}

		dst[dst_idx++] = index0;
		dst[dst_idx++] = index1;
		dst[dst_idx++] = index2;

		src = src.subspan(2);
	}
	return std::make_tuple(min_index, max_index);
}

// FIXME: expanded primitive type may not support primitive restart correctly
template<typename T>
std::tuple<T, T> expand_indexed_quads(gsl::span<to_be_t<const T>> src, gsl::span<T> dst, bool is_primitive_restart_enabled, T primitive_restart_index)
{
	T min_index = -1;
	T max_index = 0;

	Expects(4 * dst.size_bytes() >= 6 * src.size_bytes());

	size_t dst_idx = 0;
	while (!src.empty())
	{
		gsl::span<to_be_t<const T>> quad_indexes = src.subspan(0, 4);
		T index0 = quad_indexes[0];
		if (is_primitive_restart_enabled && index0 == primitive_restart_index)
		{
			index0 = -1;
		}
		else
		{
			min_index = MIN2(min_index, index0);
			max_index = MAX2(max_index, index0);
		}
		T index1 = quad_indexes[1];
		if (is_primitive_restart_enabled && index1 == primitive_restart_index)
		{
			index1 = -1;
		}
		else
		{
			min_index = MIN2(min_index, index1);
			max_index = MAX2(max_index, index1);
		}
		T index2 = quad_indexes[2];
		if (is_primitive_restart_enabled && index2 == primitive_restart_index)
		{
			index2 = -1;
		}
		else
		{
			min_index = MIN2(min_index, index2);
			max_index = MAX2(max_index, index2);
		}
		T index3 = quad_indexes[3];
		if (is_primitive_restart_enabled &&index3 == primitive_restart_index)
		{
			index3 = -1;
		}
		else
		{
			min_index = MIN2(min_index, index3);
			max_index = MAX2(max_index, index3);
		}

		// First triangle
		dst[dst_idx++] = index0;
		dst[dst_idx++] = index1;
		dst[dst_idx++] = index2;
		// Second triangle
		dst[dst_idx++] = index2;
		dst[dst_idx++] = index3;
		dst[dst_idx++] = index0;

		src = src.subspan(4);
	}
	return std::make_tuple(min_index, max_index);
}
}

// Only handle quads and triangle fan now
bool is_primitive_native(Primitive_type m_draw_mode)
{
	switch (m_draw_mode)
	{
	case Primitive_type::points:
	case Primitive_type::lines:
	case Primitive_type::line_loop:
	case Primitive_type::line_strip:
	case Primitive_type::triangles:
	case Primitive_type::triangle_strip:
	case Primitive_type::quad_strip:
		return true;
	case Primitive_type::polygon:
	case Primitive_type::triangle_fan:
	case Primitive_type::quads:
		return false;
	}
	throw new EXCEPTION("Wrong primitive type");
}

/** We assume that polygon is convex in polygon mode (constraints in OpenGL)
 *In such case polygon triangulation equates to triangle fan with arbitrary start vertex
 * see http://www.gamedev.net/page/resources/_/technical/graphics-programming-and-theory/polygon-triangulation-r3334
 */

size_t get_index_count(Primitive_type m_draw_mode, unsigned initial_index_count)
{
	// Index count
	if (is_primitive_native(m_draw_mode))
		return initial_index_count;

	switch (m_draw_mode)
	{
	case Primitive_type::polygon:
	case Primitive_type::triangle_fan:
		return (initial_index_count - 2) * 3;
	case Primitive_type::quads:
		return (6 * initial_index_count) / 4;
	default:
		return 0;
	}
}

size_t get_index_type_size(Index_array_type type)
{
	switch (type)
	{
	case Index_array_type::unsigned_16b: return 2;
	case Index_array_type::unsigned_32b: return 4;
	}
	throw new EXCEPTION("Wrong index type");
}

void write_index_array_for_non_indexed_non_native_primitive_to_buffer(char* dst, Primitive_type draw_mode, unsigned first, unsigned count)
{
	unsigned short *typedDst = (unsigned short *)(dst);
	switch (draw_mode)
	{
	case Primitive_type::triangle_fan:
	case Primitive_type::polygon:
		for (unsigned i = 0; i < (count - 2); i++)
		{
			typedDst[3 * i] = first;
			typedDst[3 * i + 1] = i + 2 - 1;
			typedDst[3 * i + 2] = i + 2;
		}
		return;
	case Primitive_type::quads:
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
	case Primitive_type::points:
	case Primitive_type::lines:
	case Primitive_type::line_loop:
	case Primitive_type::line_strip:
	case Primitive_type::triangles:
	case Primitive_type::triangle_strip:
	case Primitive_type::quad_strip:
		throw new EXCEPTION("Native primitive type doesn't require expansion");
	}
}

// TODO: Unify indexed and non indexed primitive expansion ?

template<typename T>
std::tuple<T, T> write_index_array_data_to_buffer_impl(gsl::span<T, gsl::dynamic_range> dst, Primitive_type m_draw_mode, const std::vector<std::pair<u32, u32> > &first_count_arguments)
{
	u32 address = rsx::get_address(rsx::method_registers[NV4097_SET_INDEX_ARRAY_ADDRESS], rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] & 0xf);
	Index_array_type type = to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);

	u32 type_size = gsl::narrow<u32>(get_index_type_size(type));

	Expects(rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET] == 0);
	Expects(rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_INDEX] == 0);

	bool is_primitive_restart_enabled = !!rsx::method_registers[NV4097_SET_RESTART_INDEX_ENABLE];
	u32 primitive_restart_index = rsx::method_registers[NV4097_SET_RESTART_INDEX];

	// Disjoint first_counts ranges not supported atm
	for (int i = 0; i < first_count_arguments.size() - 1; i++)
	{
		const std::tuple<u32, u32> &range = first_count_arguments[i];
		const std::tuple<u32, u32> &next_range = first_count_arguments[i + 1];
		Expects(std::get<0>(range) + std::get<1>(range) == std::get<0>(next_range));
	}
	u32 first = std::get<0>(first_count_arguments.front());
	u32 count = std::get<0>(first_count_arguments.back()) + std::get<1>(first_count_arguments.back()) - first;
	auto ptr = vm::ps3::_ptr<const T>(address + first * type_size);

	switch (m_draw_mode)
	{
	case Primitive_type::points:
	case Primitive_type::lines:
	case Primitive_type::line_loop:
	case Primitive_type::line_strip:
	case Primitive_type::triangles:
	case Primitive_type::triangle_strip:
	case Primitive_type::quad_strip:
		return upload_untouched<T>({ ptr, count }, dst, is_primitive_restart_enabled, primitive_restart_index);
	case Primitive_type::polygon:
	case Primitive_type::triangle_fan:
		return expand_indexed_triangle_fan<T>({ ptr, count }, dst, is_primitive_restart_enabled, primitive_restart_index);
	case Primitive_type::quads:
		return expand_indexed_quads<T>({ ptr, count }, dst, is_primitive_restart_enabled, primitive_restart_index);
	}

	throw new EXCEPTION("Unknow draw mode");
}

std::tuple<u32, u32> write_index_array_data_to_buffer(gsl::span<u32, gsl::dynamic_range> dst, Primitive_type m_draw_mode, const std::vector<std::pair<u32, u32> > &first_count_arguments)
{
	return write_index_array_data_to_buffer_impl(dst, m_draw_mode, first_count_arguments);
}

std::tuple<u16, u16> write_index_array_data_to_buffer(gsl::span<u16, gsl::dynamic_range> dst, Primitive_type m_draw_mode, const std::vector<std::pair<u32, u32> > &first_count_arguments)
{
	return write_index_array_data_to_buffer_impl(dst, m_draw_mode, first_count_arguments);
}

std::tuple<u32, u32> write_index_array_data_to_buffer_untouched(gsl::span<u32, gsl::dynamic_range> dst, const std::vector<std::pair<u32, u32> > &first_count_arguments)
{
	u32 address = rsx::get_address(rsx::method_registers[NV4097_SET_INDEX_ARRAY_ADDRESS], rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] & 0xf);
	Index_array_type type = to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);

	u32 type_size = gsl::narrow<u32>(get_index_type_size(type));
	bool is_primitive_restart_enabled = !!rsx::method_registers[NV4097_SET_RESTART_INDEX_ENABLE];
	u32 primitive_restart_index = rsx::method_registers[NV4097_SET_RESTART_INDEX];

	// Disjoint first_counts ranges not supported atm
	for (int i = 0; i < first_count_arguments.size() - 1; i++)
	{
		const std::tuple<u32, u32> &range = first_count_arguments[i];
		const std::tuple<u32, u32> &next_range = first_count_arguments[i + 1];
		Expects(std::get<0>(range) + std::get<1>(range) == std::get<0>(next_range));
	}
	u32 first = std::get<0>(first_count_arguments.front());
	u32 count = std::get<0>(first_count_arguments.back()) + std::get<1>(first_count_arguments.back()) - first;
	auto ptr = vm::ps3::_ptr<const u32>(address + first * type_size);

	return upload_untouched<u32>({ ptr, count }, dst, is_primitive_restart_enabled, primitive_restart_index);
}

std::tuple<u16, u16> write_index_array_data_to_buffer_untouched(gsl::span<u16, gsl::dynamic_range> dst, const std::vector<std::pair<u32, u32> > &first_count_arguments)
{
	u32 address = rsx::get_address(rsx::method_registers[NV4097_SET_INDEX_ARRAY_ADDRESS], rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] & 0xf);
	Index_array_type type = to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);

	u32 type_size = gsl::narrow<u32>(get_index_type_size(type));
	bool is_primitive_restart_enabled = !!rsx::method_registers[NV4097_SET_RESTART_INDEX_ENABLE];
	u16 primitive_restart_index = rsx::method_registers[NV4097_SET_RESTART_INDEX];

	// Disjoint first_counts ranges not supported atm
	for (int i = 0; i < first_count_arguments.size() - 1; i++)
	{
		const std::tuple<u32, u32> &range = first_count_arguments[i];
		const std::tuple<u32, u32> &next_range = first_count_arguments[i + 1];
		Expects(std::get<0>(range) + std::get<1>(range) == std::get<0>(next_range));
	}
	u32 first = std::get<0>(first_count_arguments.front());
	u32 count = std::get<0>(first_count_arguments.back()) + std::get<1>(first_count_arguments.back()) - first;
	auto ptr = vm::ps3::_ptr<const u16>(address + first * type_size);

	return upload_untouched<u16>({ ptr, count }, dst, is_primitive_restart_enabled, primitive_restart_index);
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
