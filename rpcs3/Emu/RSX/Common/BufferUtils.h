#pragma once

#include <vector>

#include "Utilities/GSL.h"
#include "Emu/Memory/vm.h"
#include "../RSXThread.h"

/**
 * Write count vertex attributes from src_ptr starting at first.
 * src_ptr array layout is deduced from the type, vector element count and src_stride arguments.
 */
void write_vertex_array_data_to_buffer(gsl::span<gsl::byte> raw_dst_span, const gsl::byte *src_ptr, u32 first, u32 count, rsx::vertex_base_type type, u32 vector_element_count, u32 attribute_src_stride, u8 dst_stride);

/*
 * If primitive mode is not supported and need to be emulated (using an index buffer) returns false.
 */
bool is_primitive_native(rsx::primitive_type m_draw_mode);

/**
 * Returns a fixed index count for emulated primitive, otherwise returns initial_index_count
 */
size_t get_index_count(rsx::primitive_type m_draw_mode, unsigned initial_index_count);

/**
 * Returns index type size in byte
 */
size_t get_index_type_size(rsx::index_array_type type);

/**
 * Write count indexes using (first, first + count) ranges.
 * Returns min/max index found during the process.
 * The function expands index buffer for non native primitive type.
 */
std::tuple<u32, u32> write_index_array_data_to_buffer(gsl::span<gsl::byte> dst, rsx::index_array_type, rsx::primitive_type draw_mode, const std::vector<std::pair<u32, u32> > &first_count_arguments);


/**
 * Doesn't expand index
 */
std::tuple<u32, u32> write_index_array_data_to_buffer_untouched(gsl::span<u32, gsl::dynamic_range> dst, const std::vector<std::pair<u32, u32> > &first_count_arguments);
std::tuple<u16, u16> write_index_array_data_to_buffer_untouched(gsl::span<u16, gsl::dynamic_range> dst, const std::vector<std::pair<u32, u32> > &first_count_arguments);

/**
 * Write index data needed to emulate non indexed non native primitive mode.
 */
void write_index_array_for_non_indexed_non_native_primitive_to_buffer(char* dst, rsx::primitive_type draw_mode, unsigned first, unsigned count);

/**
 * Stream a 128 bits vector to dst.
 */
void stream_vector(void *dst, u32 x, u32 y, u32 z, u32 w);

/**
 * Stream a 128 bits vector from src to dst.
 */
void stream_vector_from_memory(void *dst, void *src);
