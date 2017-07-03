#include "stdafx.h"
#include "BufferUtils.h"
#include "../rsx_methods.h"

#define DEBUG_VERTEX_STREAMING 0

namespace
{
	// FIXME: GSL as_span break build if template parameter is non const with current revision.
	// Replace with true as_span when fixed.
	template <typename T>
	gsl::span<T> as_span_workaround(gsl::span<gsl::byte> unformated_span)
	{
		return{ (T*)unformated_span.data(), ::narrow<int>(unformated_span.size_bytes() / sizeof(T)) };
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

	inline void stream_data_to_memory_swapped_u32(void *dst, const void *src, u32 vertex_count, u8 stride)
	{
		const __m128i mask = _mm_set_epi8(
			0xC, 0xD, 0xE, 0xF,
			0x8, 0x9, 0xA, 0xB, 
			0x4, 0x5, 0x6, 0x7, 
			0x0, 0x1, 0x2, 0x3);

		__m128i* dst_ptr = (__m128i*)dst;
		__m128i* src_ptr = (__m128i*)src;

		const u32 dword_count = (vertex_count * (stride >> 2));
		const u32 iterations = dword_count >> 2;
		const u32 remaining = dword_count % 4;
		
		for (u32 i = 0; i < iterations; ++i)
		{
			u32 *src_words = (u32*)src_ptr;
			u32 *dst_words = (u32*)dst_ptr;
			const __m128i &vector = _mm_loadu_si128(src_ptr);
			const __m128i &shuffled_vector = _mm_shuffle_epi8(vector, mask);
			_mm_stream_si128(dst_ptr, shuffled_vector);

			src_ptr++;
			dst_ptr++;
		}

		if (remaining)
		{
			u32 *src_ptr2 = (u32 *)src_ptr;
			u32 *dst_ptr2 = (u32 *)dst_ptr;

			for (u32 i = 0; i < remaining; ++i)
				dst_ptr2[i] = se_storage<u32>::swap(src_ptr2[i]);
		}
	}

	inline void stream_data_to_memory_swapped_u16(void *dst, const void *src, u32 vertex_count, u8 stride)
	{
		const __m128i mask = _mm_set_epi8(
			0xE, 0xF, 0xC, 0xD,
			0xA, 0xB, 0x8, 0x9,
			0x6, 0x7, 0x4, 0x5,
			0x2, 0x3, 0x0, 0x1);

		__m128i* dst_ptr = (__m128i*)dst;
		__m128i* src_ptr = (__m128i*)src;

		const u32 word_count = (vertex_count * (stride >> 1));
		const u32 iterations = word_count >> 3;
		const u32 remaining = word_count % 8;

		for (u32 i = 0; i < iterations; ++i)
		{
			u32 *src_words = (u32*)src_ptr;
			u32 *dst_words = (u32*)dst_ptr;
			const __m128i &vector = _mm_loadu_si128(src_ptr);
			const __m128i &shuffled_vector = _mm_shuffle_epi8(vector, mask);
			_mm_stream_si128(dst_ptr, shuffled_vector);

			src_ptr++;
			dst_ptr++;
		}

		if (remaining)
		{
			u16 *src_ptr2 = (u16 *)src_ptr;
			u16 *dst_ptr2 = (u16 *)dst_ptr;

			for (u32 i = 0; i < remaining; ++i)
				dst_ptr2[i] = se_storage<u16>::swap(src_ptr2[i]);
		}
	}

	inline void stream_data_to_memory_swapped_u32_non_continuous(void *dst, const void *src, u32 vertex_count, u8 dst_stride, u8 src_stride)
	{
		const __m128i mask = _mm_set_epi8(
			0xC, 0xD, 0xE, 0xF,
			0x8, 0x9, 0xA, 0xB,
			0x4, 0x5, 0x6, 0x7,
			0x0, 0x1, 0x2, 0x3);

		char *src_ptr = (char *)src;
		char *dst_ptr = (char *)dst;

		//Count vertices to copy
		const bool is_128_aligned = !((dst_stride | src_stride) & 15);
		
		u32 min_block_size = std::min(src_stride, dst_stride);
		if (min_block_size == 0) min_block_size = dst_stride;

		const u32 remainder = is_128_aligned? 0:  (16 - min_block_size) / min_block_size;
		const u32 iterations = is_128_aligned? vertex_count: vertex_count - remainder;

		for (u32 i = 0; i < iterations; ++i)
		{
			const __m128i &vector = _mm_loadu_si128((__m128i*)src_ptr);
			const __m128i &shuffled_vector = _mm_shuffle_epi8(vector, mask);
			_mm_storeu_si128((__m128i*)dst_ptr, shuffled_vector);

			src_ptr += src_stride;
			dst_ptr += dst_stride;
		}

		if (remainder)
		{
			const u8 attribute_sz = min_block_size >> 2;
			for (u32 n = 0; n < remainder; ++n)
			{
				for (u32 v= 0; v < attribute_sz; ++v)
					((u32*)dst_ptr)[v] = ((be_t<u32>*)src_ptr)[v];

				src_ptr += src_stride;
				dst_ptr += dst_stride;
			}
		}
	}

	inline void stream_data_to_memory_swapped_u16_non_continuous(void *dst, const void *src, u32 vertex_count, u8 dst_stride, u8 src_stride)
	{
		const __m128i mask = _mm_set_epi8(
			0xE, 0xF, 0xC, 0xD,
			0xA, 0xB, 0x8, 0x9,
			0x6, 0x7, 0x4, 0x5,
			0x2, 0x3, 0x0, 0x1);

		char *src_ptr = (char *)src;
		char *dst_ptr = (char *)dst;

		const bool is_128_aligned = !((dst_stride | src_stride) & 15);

		u32 min_block_size = std::min(src_stride, dst_stride);
		if (min_block_size == 0) min_block_size = dst_stride;

		const u32 remainder = is_128_aligned ? 0 : (16 - min_block_size) / min_block_size;
		const u32 iterations = is_128_aligned ? vertex_count : vertex_count - remainder;

		for (u32 i = 0; i < iterations; ++i)
		{
			const __m128i &vector = _mm_loadu_si128((__m128i*)src_ptr);
			const __m128i &shuffled_vector = _mm_shuffle_epi8(vector, mask);
			_mm_storeu_si128((__m128i*)dst_ptr, shuffled_vector);

			src_ptr += src_stride;
			dst_ptr += dst_stride;
		}

		if (remainder)
		{
			const u8 attribute_sz = min_block_size >> 1;
			for (u32 n = 0; n < remainder; ++n)
			{
				for (u32 v = 0; v < attribute_sz; ++v)
					((u16*)dst_ptr)[v] = ((be_t<u16>*)src_ptr)[v];

				src_ptr += src_stride;
				dst_ptr += dst_stride;
			}
		}
	}

	inline void stream_data_to_memory_u8_non_continous(void *dst, const void *src, u32 vertex_count, u8 attribute_size, u8 dst_stride, u8 src_stride)
	{
		char *src_ptr = (char *)src;
		char *dst_ptr = (char *)dst;

		switch (attribute_size)
		{
			case 4:
			{
				//Read one dword every iteration
				for (u32 vertex = 0; vertex < vertex_count; ++vertex)
				{
					*(u32*)dst_ptr = *(u32*)src_ptr;

					dst_ptr += dst_stride;
					src_ptr += src_stride;
				}

				break;
			}
			case 3:
			{
				//Read one word and one byte
				for (u32 vertex = 0; vertex < vertex_count; ++vertex)
				{
					*(u16*)dst_ptr = *(u16*)src_ptr;
					dst_ptr[2] = src_ptr[2];

					dst_ptr += dst_stride;
					src_ptr += src_stride;
				}

				break;
			}
			case 2:
			{
				//Copy u16 blocks
				for (u32 vertex = 0; vertex < vertex_count; ++vertex)
				{
					*(u32*)dst_ptr = *(u32*)src_ptr;

					dst_ptr += dst_stride;
					src_ptr += src_stride;
				}

				break;
			}
			case 1:
			{
				for (u32 vertex = 0; vertex < vertex_count; ++vertex)
				{
					dst_ptr[0] = src_ptr[0];

					dst_ptr += dst_stride;
					src_ptr += src_stride;
				}

				break;
			}
		}
	}

	template <typename T, typename U, int N>
	void copy_whole_attribute_array_impl(void *raw_dst, void *raw_src, u8 dst_stride, u32 src_stride, u32 vertex_count)
	{
		char *src_ptr = (char *)raw_src;
		char *dst_ptr = (char *)raw_dst;

		for (u32 vertex = 0; vertex < vertex_count; ++vertex)
		{
			T* typed_dst = (T*)dst_ptr;
			U* typed_src = (U*)src_ptr;

			for (u32 i = 0; i < N; ++i)
			{
				typed_dst[i] = typed_src[i];
			}

			src_ptr += src_stride;
			dst_ptr += dst_stride;
		}
	}

	/*
	 * Copies a number of src vertices, repeated over and over to fill the dst
	 * e.g repeat 2 vertices over a range of 16 verts, so 8 reps
	 */
	template <typename T, typename U, int N>
	void copy_whole_attribute_array_repeating_impl(void *raw_dst, void *raw_src, const u8 dst_stride, const u32 src_stride, const u32 vertex_count, const u32 src_vertex_count)
	{
		char *src_ptr = (char *)raw_src;
		char *dst_ptr = (char *)raw_dst;

		u32 src_offset = 0;
		u32 src_limit = src_stride * src_vertex_count;

		for (u32 vertex = 0; vertex < vertex_count; ++vertex)
		{
			T* typed_dst = (T*)dst_ptr;
			U* typed_src = (U*)(src_ptr + src_offset);

			for (u32 i = 0; i < N; ++i)
			{
				typed_dst[i] = typed_src[i];
			}

			src_offset = (src_offset + src_stride) % src_limit;
			dst_ptr += dst_stride;
		}
	}

	template <typename U, typename T>
	void copy_whole_attribute_array(void *raw_dst, void *raw_src, const u8 attribute_size, const u8 dst_stride, const u32 src_stride, const u32 vertex_count, const u32 src_vertex_count)
	{
		//Eliminate the inner loop by templating the inner loop counter N

		if (src_vertex_count == vertex_count)
		{
			switch (attribute_size)
			{
			case 1:
				copy_whole_attribute_array_impl<U, T, 1>(raw_dst, raw_src, dst_stride, src_stride, vertex_count);
				break;
			case 2:
				copy_whole_attribute_array_impl<U, T, 2>(raw_dst, raw_src, dst_stride, src_stride, vertex_count);
				break;
			case 3:
				copy_whole_attribute_array_impl<U, T, 3>(raw_dst, raw_src, dst_stride, src_stride, vertex_count);
				break;
			case 4:
				copy_whole_attribute_array_impl<U, T, 4>(raw_dst, raw_src, dst_stride, src_stride, vertex_count);
				break;
			}
		}
		else
		{
			switch (attribute_size)
			{
			case 1:
				copy_whole_attribute_array_repeating_impl<U, T, 1>(raw_dst, raw_src, dst_stride, src_stride, vertex_count, src_vertex_count);
				break;
			case 2:
				copy_whole_attribute_array_repeating_impl<U, T, 2>(raw_dst, raw_src, dst_stride, src_stride, vertex_count, src_vertex_count);
				break;
			case 3:
				copy_whole_attribute_array_repeating_impl<U, T, 3>(raw_dst, raw_src, dst_stride, src_stride, vertex_count, src_vertex_count);
				break;
			case 4:
				copy_whole_attribute_array_repeating_impl<U, T, 4>(raw_dst, raw_src, dst_stride, src_stride, vertex_count, src_vertex_count);
				break;
			}
		}
	}
}

void write_vertex_array_data_to_buffer(gsl::span<gsl::byte> raw_dst_span, gsl::span<const gsl::byte> src_ptr, u32 count, rsx::vertex_base_type type, u32 vector_element_count, u32 attribute_src_stride, u8 dst_stride)
{
	verify(HERE), (vector_element_count > 0);
	const u32 src_read_stride = rsx::get_vertex_type_size_on_host(type, vector_element_count);

	bool use_stream_no_stride = false;
	bool use_stream_with_stride = false;

	//If stride is not defined, we have a packed array
	if (attribute_src_stride == 0) attribute_src_stride = src_read_stride;

	//Sometimes, we get a vertex attribute to be repeated. Just copy the supplied vertices only
	//TODO: Stop these requests from getting here in the first place!
	//TODO: Check if it is possible to have a repeating array with more than one attribute instance
	const u32 real_count = (u32)src_ptr.size_bytes() / attribute_src_stride;
	if (real_count == 1) attribute_src_stride = 0;	//Always fetch src[0]

	//TODO: Determine favourable vertex threshold where vector setup costs become negligible
	//Tests show that even with 4 vertices, using traditional bswap is significantly slower over a large number of calls

	const u64 src_address = (u64)src_ptr.data();
	const bool sse_aligned = ((src_address & 15) == 0);
	
#if !DEBUG_VERTEX_STREAMING
	
	if (real_count >= count || real_count == 1)
	{
		if (attribute_src_stride == dst_stride && src_read_stride == dst_stride)
			use_stream_no_stride = true;
		else
			use_stream_with_stride = true;
	}

#endif

	switch (type)
	{
	case rsx::vertex_base_type::ub:
	case rsx::vertex_base_type::ub256:
	{
		if (use_stream_no_stride)
			memcpy(raw_dst_span.data(), src_ptr.data(), count * dst_stride);
		else if (use_stream_with_stride)
			stream_data_to_memory_u8_non_continous(raw_dst_span.data(), src_ptr.data(), count, vector_element_count, dst_stride, attribute_src_stride);
		else
			copy_whole_attribute_array<u8, u8>((void *)raw_dst_span.data(), (void *)src_ptr.data(), vector_element_count, dst_stride, attribute_src_stride, count, real_count);

		return;
	}
	case rsx::vertex_base_type::s1:
	case rsx::vertex_base_type::sf:
	case rsx::vertex_base_type::s32k:
	{
		if (use_stream_no_stride && sse_aligned)
			stream_data_to_memory_swapped_u16(raw_dst_span.data(), src_ptr.data(), count, attribute_src_stride);
		else if (use_stream_with_stride)
			stream_data_to_memory_swapped_u16_non_continuous(raw_dst_span.data(), src_ptr.data(), count, dst_stride, attribute_src_stride);
		else
			copy_whole_attribute_array<be_t<u16>, u16>((void *)raw_dst_span.data(), (void *)src_ptr.data(), vector_element_count, dst_stride, attribute_src_stride, count, real_count);

		return;
	}
	case rsx::vertex_base_type::f:
	{
		if (use_stream_no_stride && sse_aligned)
			stream_data_to_memory_swapped_u32(raw_dst_span.data(), src_ptr.data(), count, attribute_src_stride);
		else if (use_stream_with_stride)
			stream_data_to_memory_swapped_u32_non_continuous(raw_dst_span.data(), src_ptr.data(), count, dst_stride, attribute_src_stride);
		else
			copy_whole_attribute_array<be_t<u32>, u32>((void *)raw_dst_span.data(), (void *)src_ptr.data(), vector_element_count, dst_stride, attribute_src_stride, count, real_count);

		return;
	}
	case rsx::vertex_base_type::cmp:
	{
		gsl::span<u16> dst_span = as_span_workaround<u16>(raw_dst_span);
		for (u32 i = 0; i < count; ++i)
		{
			be_t<u32> src_value;
			memcpy(&src_value,
			    src_ptr.subspan(attribute_src_stride * i).data(),
			    sizeof(be_t<u32>));
			const auto& decoded_vector                 = decode_cmp_vector(src_value);
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

	verify(HERE), (dst.size_bytes() >= src.size_bytes());

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

	verify(HERE), (dst.size() >= 3 * (src.size() - 2));

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

	verify(HERE), (4 * dst.size_bytes() >= 6 * src.size_bytes());

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
	case rsx::primitive_type::line_strip:
	case rsx::primitive_type::triangles:
	case rsx::primitive_type::triangle_strip:
		return true;
	case rsx::primitive_type::line_loop:
	case rsx::primitive_type::polygon:
	case rsx::primitive_type::triangle_fan:
	case rsx::primitive_type::quads:
	case rsx::primitive_type::quad_strip:
		return false;
	}
	fmt::throw_exception("Wrong primitive type" HERE);
}

/** We assume that polygon is convex in polygon mode (constraints in OpenGL)
 *In such case polygon triangulation equates to triangle fan with arbitrary start vertex
 * see http://www.gamedev.net/page/resources/_/technical/graphics-programming-and-theory/polygon-triangulation-r3334
 */

u32 get_index_count(rsx::primitive_type draw_mode, u32 initial_index_count)
{
	// Index count
	if (is_primitive_native(draw_mode))
		return initial_index_count;

	switch (draw_mode)
	{
	case rsx::primitive_type::line_loop:
		return initial_index_count + 1;
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

u32 get_index_type_size(rsx::index_array_type type)
{
	switch (type)
	{
	case rsx::index_array_type::u16: return sizeof(u16);
	case rsx::index_array_type::u32: return sizeof(u32);
	}
	fmt::throw_exception("Wrong index type" HERE);
}

void write_index_array_for_non_indexed_non_native_primitive_to_buffer(char* dst, rsx::primitive_type draw_mode, unsigned count)
{
	unsigned short *typedDst = (unsigned short *)(dst);
	switch (draw_mode)
	{
	case rsx::primitive_type::line_loop:
		for (unsigned i = 0; i < count; ++i)
			typedDst[i] = i;
		typedDst[count] = 0;
		return;
	case rsx::primitive_type::triangle_fan:
	case rsx::primitive_type::polygon:
		for (unsigned i = 0; i < (count - 2); i++)
		{
			typedDst[3 * i] = 0;
			typedDst[3 * i + 1] = i + 2 - 1;
			typedDst[3 * i + 2] = i + 2;
		}
		return;
	case rsx::primitive_type::quads:
		for (unsigned i = 0; i < count / 4; i++)
		{
			// First triangle
			typedDst[6 * i] = 4 * i;
			typedDst[6 * i + 1] = 4 * i + 1;
			typedDst[6 * i + 2] = 4 * i + 2;
			// Second triangle
			typedDst[6 * i + 3] = 4 * i + 2;
			typedDst[6 * i + 4] = 4 * i + 3;
			typedDst[6 * i + 5] = 4 * i;
		}
		return;
	case rsx::primitive_type::quad_strip:
		for (unsigned i = 0; i < (count - 2) / 2; i++)
		{
			// First triangle
			typedDst[6 * i] = 2 * i;
			typedDst[6 * i + 1] = 2 * i + 1;
			typedDst[6 * i + 2] = 2 * i + 2;
			// Second triangle
			typedDst[6 * i + 3] = 2 * i + 2;
			typedDst[6 * i + 4] = 2 * i + 1;
			typedDst[6 * i + 5] = 2 * i + 3;
		}
		return;
	case rsx::primitive_type::points:
	case rsx::primitive_type::lines:
	case rsx::primitive_type::line_strip:
	case rsx::primitive_type::triangles:
	case rsx::primitive_type::triangle_strip:
		fmt::throw_exception("Native primitive type doesn't require expansion" HERE);
	}
}


namespace
{
	/**
	* Get first index and index count from a draw indexed clause.
	*/
	std::tuple<u32, u32> get_first_count_from_draw_indexed_clause(const std::vector<std::pair<u32, u32>>& first_count_arguments)
	{
		u32 first = std::get<0>(first_count_arguments.front());
		u32 count = std::get<0>(first_count_arguments.back()) + std::get<1>(first_count_arguments.back()) - first;
		return std::make_tuple(first, count);
	}


	// TODO: Unify indexed and non indexed primitive expansion ?
	template<typename T>
	std::tuple<T, T> write_index_array_data_to_buffer_impl(gsl::span<T> dst,
		gsl::span<const be_t<T>> src,
		rsx::primitive_type draw_mode, bool restart_index_enabled, u32 restart_index, const std::vector<std::pair<u32, u32> > &first_count_arguments,
		std::function<bool(rsx::primitive_type)> expands)
	{
		u32 first;
		u32 count;
		std::tie(first, count) = get_first_count_from_draw_indexed_clause(first_count_arguments);

		if (!expands(draw_mode)) return upload_untouched<T>(src, dst, restart_index_enabled, restart_index);

		switch (draw_mode)
		{
		case rsx::primitive_type::line_loop:
		{
			const auto &returnvalue = upload_untouched<T>(src, dst, restart_index_enabled, restart_index);
			dst[count] = src[0];
			return returnvalue;
		}
		case rsx::primitive_type::polygon:
		case rsx::primitive_type::triangle_fan:
			return expand_indexed_triangle_fan<T>(src, dst, restart_index_enabled, restart_index);
		case rsx::primitive_type::quads:
			return expand_indexed_quads<T>(src, dst, restart_index_enabled, restart_index);
		}
		fmt::throw_exception("Unknown draw mode (0x%x)" HERE, (u32)draw_mode);
	}
}

std::tuple<u32, u32> write_index_array_data_to_buffer(gsl::span<gsl::byte> dst,
	gsl::span<const gsl::byte> src,
	rsx::index_array_type type, rsx::primitive_type draw_mode, bool restart_index_enabled, u32 restart_index, const std::vector<std::pair<u32, u32> > &first_count_arguments,
	std::function<bool(rsx::primitive_type)> expands)
{
	switch (type)
	{
	case rsx::index_array_type::u16:
		return write_index_array_data_to_buffer_impl<u16>(as_span_workaround<u16>(dst),
			gsl::as_span<const be_t<u16>>(src), draw_mode, restart_index_enabled, restart_index, first_count_arguments, expands);
	case rsx::index_array_type::u32:
		return write_index_array_data_to_buffer_impl<u32>(as_span_workaround<u32>(dst),
			gsl::as_span<const be_t<u32>>(src), draw_mode, restart_index_enabled, restart_index, first_count_arguments, expands);
	}
	fmt::throw_exception("Unknown index type" HERE);
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
