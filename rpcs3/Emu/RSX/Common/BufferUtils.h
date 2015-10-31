#pragma once
#include <vector>
#include "Emu/Memory/vm.h"
#include "../RSXThread.h"


struct VertexBufferFormat
{
	std::pair<size_t, size_t> range;
	std::vector<size_t> attributeId;
	size_t elementCount;
	size_t stride;
};

/**
 * Write count vertex attributes from index array buffer starting at first, using vertex_array_desc
 */
void write_vertex_array_data_to_buffer(void *buffer, u32 first, u32 count, size_t index, const rsx::data_array_format_info &vertex_array_desc) noexcept;

/*
 * If primitive mode is not supported and need to be emulated (using an index buffer) returns false.
 */
bool is_primitive_native(unsigned m_draw_mode) noexcept;

/**
 * Returns a fixed index count for emulated primitive, otherwise returns initial_index_count
 */
size_t get_index_count(unsigned m_draw_mode, unsigned initial_index_count) noexcept;

/**
 * Returns index type size in byte
 */
size_t get_index_type_size(u32 type) noexcept;

/**
 * Write count indexes starting at first to dst buffer.
 * Returns min/max index found during the process.
 * The function expands index buffer for non native primitive type.
 */
void write_index_array_data_to_buffer(char* dst, unsigned m_draw_mode, unsigned first, unsigned count, unsigned &min_index, unsigned &max_index) noexcept;

/**
 * Write index data needed to emulate non indexed non native primitive mode.
 */
void write_index_array_for_non_indexed_non_native_primitive_to_buffer(char* dst, unsigned m_draw_mode, unsigned first, unsigned count) noexcept;

/**
 * Stream a 128 bits vector to dst.
 */
void stream_vector(void *dst, u32 x, u32 y, u32 z, u32 w) noexcept;
