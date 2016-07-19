#include "stdafx.h"
#include "BufferUtils.h"
#include "../rsx_methods.h"

namespace
{
	// FIXME: GSL as_span break build if template parameter is non const with current revision.
	// Replace with true as_span when fixed.
	template <typename T>
	gsl::span<T> as_span_workaround(gsl::span<gsl::byte> unformated_span)
	{
		return{ (T*)unformated_span.data(), gsl::narrow<int>(unformated_span.size_bytes() / sizeof(T)) };
	}
}

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

	template<typename U, typename T>
	void copy_whole_attribute_array(gsl::span<T> dst, const gsl::byte* src_ptr, u8 attribute_size, u8 dst_stride, u32 src_stride, u32 first, u32 vertex_count)
	{
		for (u32 vertex = 0; vertex < vertex_count; ++vertex)
		{
			const U* src = reinterpret_cast<const U*>(src_ptr + src_stride * (first + vertex));
			for (u32 i = 0; i < attribute_size; ++i)
			{
				dst[vertex * dst_stride / sizeof(T) + i] = src[i];
			}
		}
	}
}

void write_vertex_array_data_to_buffer(gsl::span<gsl::byte> raw_dst_span, const gsl::byte *src_ptr, u32 first, u32 count, rsx::vertex_base_type type, u32 vector_element_count, u32 attribute_src_stride, u8 dst_stride)
{
	EXPECTS(vector_element_count > 0);

	switch (type)
	{
	case rsx::vertex_base_type::ub:
	case rsx::vertex_base_type::ub256:
	{
		gsl::span<u8> dst_span = as_span_workaround<u8>(raw_dst_span);
		copy_whole_attribute_array<u8>(dst_span, src_ptr, vector_element_count, dst_stride, attribute_src_stride, first, count);
		return;
	}
	case rsx::vertex_base_type::s1:
	case rsx::vertex_base_type::sf:
	case rsx::vertex_base_type::s32k:
	{
		gsl::span<u16> dst_span = as_span_workaround<u16>(raw_dst_span);
		copy_whole_attribute_array<be_t<u16>>(dst_span, src_ptr, vector_element_count, dst_stride, attribute_src_stride, first, count);
		return;
	}
	case rsx::vertex_base_type::f:
	{
		gsl::span<u32> dst_span = as_span_workaround<u32>(raw_dst_span);
		copy_whole_attribute_array<be_t<u32>>(dst_span, src_ptr, vector_element_count, dst_stride, attribute_src_stride, first, count);
		return;
	}
	case rsx::vertex_base_type::cmp:
	{
		gsl::span<u16> dst_span = as_span_workaround<u16>(raw_dst_span);
		for (u32 i = 0; i < count; ++i)
		{
			auto* c_src = (const be_t<u32>*)(src_ptr + attribute_src_stride * (first + i));
			const auto& decoded_vector = decode_cmp_vector(*c_src);
			dst_span[i * dst_stride / sizeof(u16)] = decoded_vector[0];
			dst_span[i * dst_stride / sizeof(u16) + 1] = decoded_vector[1];
			dst_span[i * dst_stride / sizeof(u16) + 2] = decoded_vector[2];
			dst_span[i * dst_stride / sizeof(u16) + 3] = decoded_vector[3];
		}
		return;
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

	EXPECTS(dst.size_bytes() >= src.size_bytes());

	size_t dst_idx = 0;
	for (T index : src)
	{
		if (is_primitive_restart_enabled && index == primitive_restart_index)
		{
			index = -1;
		}
		else
		{
			max_index = std::max(max_index, index);
			min_index = std::min(min_index, index);
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

	EXPECTS(dst.size() >= 3 * (src.size() - 2));

	const T index0 = src[0];
	if (!is_primitive_restart_enabled || index0 != -1) // Cut
	{
		min_index = std::min(min_index, index0);
		max_index = std::max(max_index, index0);
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
			min_index = std::min(min_index, index1);
			max_index = std::max(max_index, index1);
		}
		T index2 = tri_indexes[1];
		if (is_primitive_restart_enabled && index2 == primitive_restart_index)
		{
			index2 = -1;
		}
		else
		{
			min_index = std::min(min_index, index2);
			max_index = std::max(max_index, index2);
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

	EXPECTS(4 * dst.size_bytes() >= 6 * src.size_bytes());

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
			min_index = std::min(min_index, index0);
			max_index = std::max(max_index, index0);
		}
		T index1 = quad_indexes[1];
		if (is_primitive_restart_enabled && index1 == primitive_restart_index)
		{
			index1 = -1;
		}
		else
		{
			min_index = std::min(min_index, index1);
			max_index = std::max(max_index, index1);
		}
		T index2 = quad_indexes[2];
		if (is_primitive_restart_enabled && index2 == primitive_restart_index)
		{
			index2 = -1;
		}
		else
		{
			min_index = std::min(min_index, index2);
			max_index = std::max(max_index, index2);
		}
		T index3 = quad_indexes[3];
		if (is_primitive_restart_enabled &&index3 == primitive_restart_index)
		{
			index3 = -1;
		}
		else
		{
			min_index = std::min(min_index, index3);
			max_index = std::max(max_index, index3);
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
bool is_primitive_native(rsx::primitive_type draw_mode)
{
	switch (draw_mode)
	{
	case rsx::primitive_type::points:
	case rsx::primitive_type::lines:
	case rsx::primitive_type::line_loop:
	case rsx::primitive_type::line_strip:
	case rsx::primitive_type::triangles:
	case rsx::primitive_type::triangle_strip:
		return true;
	case rsx::primitive_type::polygon:
	case rsx::primitive_type::triangle_fan:
	case rsx::primitive_type::quads:
	case rsx::primitive_type::quad_strip:
		return false;
	}
	throw EXCEPTION("Wrong primitive type");
}

/** We assume that polygon is convex in polygon mode (constraints in OpenGL)
 *In such case polygon triangulation equates to triangle fan with arbitrary start vertex
 * see http://www.gamedev.net/page/resources/_/technical/graphics-programming-and-theory/polygon-triangulation-r3334
 */

size_t get_index_count(rsx::primitive_type draw_mode, unsigned initial_index_count)
{
	// Index count
	if (is_primitive_native(draw_mode))
		return initial_index_count;

	switch (draw_mode)
	{
	case rsx::primitive_type::polygon:
	case rsx::primitive_type::triangle_fan:
		return (initial_index_count - 2) * 3;
	case rsx::primitive_type::quads:
		return (6 * initial_index_count) / 4;
	case rsx::primitive_type::quad_strip:
		return (6 * (initial_index_count - 2)) / 2;
	default:
		return 0;
	}
}

size_t get_index_type_size(rsx::index_array_type type)
{
	switch (type)
	{
	case rsx::index_array_type::u16: return sizeof(u16);
	case rsx::index_array_type::u32: return sizeof(u32);
	}
	throw EXCEPTION("Wrong index type");
}

void write_index_array_for_non_indexed_non_native_primitive_to_buffer(char* dst, rsx::primitive_type draw_mode, unsigned first, unsigned count)
{
	unsigned short *typedDst = (unsigned short *)(dst);
	switch (draw_mode)
	{
	case rsx::primitive_type::triangle_fan:
	case rsx::primitive_type::polygon:
		for (unsigned i = 0; i < (count - 2); i++)
		{
			typedDst[3 * i] = first;
			typedDst[3 * i + 1] = i + 2 - 1;
			typedDst[3 * i + 2] = i + 2;
		}
		return;
	case rsx::primitive_type::quads:
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
	case rsx::primitive_type::quad_strip:
		for (unsigned i = 0; i < (count - 2) / 2; i++)
		{
			// First triangle
			typedDst[6 * i] = 2 * i + first;
			typedDst[6 * i + 1] = 2 * i + 1 + first;
			typedDst[6 * i + 2] = 2 * i + 2 + first;
			// Second triangle
			typedDst[6 * i + 3] = 2 * i + 2 + first;
			typedDst[6 * i + 4] = 2 * i + 1 + first;
			typedDst[6 * i + 5] = 2 * i + 3 + first;
		}
		return;
	case rsx::primitive_type::points:
	case rsx::primitive_type::lines:
	case rsx::primitive_type::line_loop:
	case rsx::primitive_type::line_strip:
	case rsx::primitive_type::triangles:
	case rsx::primitive_type::triangle_strip:
		throw EXCEPTION("Native primitive type doesn't require expansion");
	}
}

// TODO: Unify indexed and non indexed primitive expansion ?
// FIXME: these functions shouldn't access rsx::method_registers (global)
template<typename T>
std::tuple<T, T> write_index_array_data_to_buffer_impl(gsl::span<T, gsl::dynamic_range> dst, rsx::primitive_type draw_mode, const std::vector<std::pair<u32, u32> > &first_count_arguments)
{
	u32 address = rsx::get_address(rsx::method_registers.index_array_address(), rsx::method_registers.index_array_location());
	rsx::index_array_type type = rsx::method_registers.index_type();

	u32 type_size = gsl::narrow<u32>(get_index_type_size(type));


	EXPECTS(rsx::method_registers.vertex_data_base_index() == 0);

	bool is_primitive_restart_enabled = rsx::method_registers.restart_index_enabled();
	u32 primitive_restart_index = rsx::method_registers.restart_index();

	// Disjoint first_counts ranges not supported atm
	for (int i = 0; i < first_count_arguments.size() - 1; i++)
	{
		const std::tuple<u32, u32> &range = first_count_arguments[i];
		const std::tuple<u32, u32> &next_range = first_count_arguments[i + 1];
		EXPECTS(std::get<0>(range) + std::get<1>(range) == std::get<0>(next_range));
	}
	u32 first = std::get<0>(first_count_arguments.front());
	u32 count = std::get<0>(first_count_arguments.back()) + std::get<1>(first_count_arguments.back()) - first;
	auto ptr = vm::ps3::_ptr<const T>(address + first * type_size);

	switch (draw_mode)
	{
	case rsx::primitive_type::points:
	case rsx::primitive_type::lines:
	case rsx::primitive_type::line_loop:
	case rsx::primitive_type::line_strip:
	case rsx::primitive_type::triangles:
	case rsx::primitive_type::triangle_strip:
	case rsx::primitive_type::quad_strip:
		return upload_untouched<T>({ ptr, count }, dst, is_primitive_restart_enabled, primitive_restart_index);
	case rsx::primitive_type::polygon:
	case rsx::primitive_type::triangle_fan:
		return expand_indexed_triangle_fan<T>({ ptr, count }, dst, is_primitive_restart_enabled, primitive_restart_index);
	case rsx::primitive_type::quads:
		return expand_indexed_quads<T>({ ptr, count }, dst, is_primitive_restart_enabled, primitive_restart_index);
	}

	throw EXCEPTION("Unknown draw mode");
}

std::tuple<u32, u32> write_index_array_data_to_buffer(gsl::span<gsl::byte> dst, rsx::index_array_type type, rsx::primitive_type draw_mode, const std::vector<std::pair<u32, u32> > &first_count_arguments)
{
	switch (type)
	{
	case rsx::index_array_type::u16:
		return write_index_array_data_to_buffer_impl<u16>(as_span_workaround<u16>(dst), draw_mode, first_count_arguments);
	case rsx::index_array_type::u32:
		return write_index_array_data_to_buffer_impl<u32>(as_span_workaround<u32>(dst), draw_mode, first_count_arguments);
	}
	throw EXCEPTION("Unknown index type");
}

std::tuple<u32, u32> write_index_array_data_to_buffer_untouched(gsl::span<u32, gsl::dynamic_range> dst, const std::vector<std::pair<u32, u32> > &first_count_arguments)
{
	u32 address = rsx::get_address(rsx::method_registers.index_array_address(), rsx::method_registers.index_array_location());
	rsx::index_array_type type = rsx::method_registers.index_type();

	u32 type_size = gsl::narrow<u32>(get_index_type_size(type));
	bool is_primitive_restart_enabled = rsx::method_registers.restart_index_enabled();
	u32 primitive_restart_index = rsx::method_registers.restart_index();

	// Disjoint first_counts ranges not supported atm
	for (int i = 0; i < first_count_arguments.size() - 1; i++)
	{
		const std::tuple<u32, u32> &range = first_count_arguments[i];
		const std::tuple<u32, u32> &next_range = first_count_arguments[i + 1];
		EXPECTS(std::get<0>(range) + std::get<1>(range) == std::get<0>(next_range));
	}
	u32 first = std::get<0>(first_count_arguments.front());
	u32 count = std::get<0>(first_count_arguments.back()) + std::get<1>(first_count_arguments.back()) - first;
	auto ptr = vm::ps3::_ptr<const u32>(address + first * type_size);

	return upload_untouched<u32>({ ptr, count }, dst, is_primitive_restart_enabled, primitive_restart_index);
}

std::tuple<u16, u16> write_index_array_data_to_buffer_untouched(gsl::span<u16, gsl::dynamic_range> dst, const std::vector<std::pair<u32, u32> > &first_count_arguments)
{
	u32 address = rsx::get_address(rsx::method_registers.index_array_address(), rsx::method_registers.index_array_location());
	rsx::index_array_type type = rsx::method_registers.index_type();

	u32 type_size = gsl::narrow<u32>(get_index_type_size(type));
	bool is_primitive_restart_enabled = rsx::method_registers.restart_index_enabled();
	u16 primitive_restart_index = rsx::method_registers.restart_index();

	// Disjoint first_counts ranges not supported atm
	for (int i = 0; i < first_count_arguments.size() - 1; i++)
	{
		const std::tuple<u32, u32> &range = first_count_arguments[i];
		const std::tuple<u32, u32> &next_range = first_count_arguments[i + 1];
		EXPECTS(std::get<0>(range) + std::get<1>(range) == std::get<0>(next_range));
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

void stream_vector(void *dst, f32 x, f32 y, f32 z, f32 w)
{
	stream_vector(dst, (u32&)x, (u32&)y, (u32&)z, (u32&)w);
}
void stream_vector_from_memory(void *dst, void *src)
{
	const __m128i &vector = _mm_loadu_si128((__m128i*)src);
	_mm_stream_si128((__m128i*)dst, vector);
}
