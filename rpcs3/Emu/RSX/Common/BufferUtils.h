#pragma once

#include "../gcm_enums.h"
#include "Utilities/JIT.h"

#include <span>

/*
 * If primitive mode is not supported and need to be emulated (using an index buffer) returns false.
 */
bool is_primitive_native(rsx::primitive_type m_draw_mode);

/*
 * Returns true if adjacency information does not matter for this type. Allows optimizations e.g removal of primitive restart index
 */
bool is_primitive_disjointed(rsx::primitive_type draw_mode);

/**
 * Returns a fixed index count for emulated primitive, otherwise returns initial_index_count
 */
u32 get_index_count(rsx::primitive_type m_draw_mode, u32 initial_index_count);

/**
 * Returns index type size in byte
 */
u32 get_index_type_size(rsx::index_array_type type);

/**
 * Write count indexes using (first, first + count) ranges.
 * Returns min/max index found during the process and the number of valid indices written to the buffer.
 * The function expands index buffer for non native primitive type if expands(draw_mode) return true.
 */
std::tuple<u32, u32, u32> write_index_array_data_to_buffer(std::span<std::byte> dst, std::span<const std::byte> src,
	rsx::index_array_type, rsx::primitive_type draw_mode, bool restart_index_enabled, u32 restart_index,
	const std::function<bool(rsx::primitive_type)>& expands);

/**
 * Write index data needed to emulate non indexed non native primitive mode.
 */
void write_index_array_for_non_indexed_non_native_primitive_to_buffer(char* dst, rsx::primitive_type draw_mode, unsigned count);

/**
 * Stream a 128 bits vector to dst.
 */
void stream_vector(void *dst, f32 x, f32 y, f32 z, f32 w);
void stream_vector(void *dst, u32 x, u32 y, u32 z, u32 w);

/**
 * Stream a 128 bits vector from src to dst.
 */
void stream_vector_from_memory(void *dst, void *src);

// Copy and swap data in 32-bit units
extern built_function<void(*)(u32*, const u32*, u32)> copy_data_swap_u32;

// Copy and swap data in 32-bit units, return true if changed
extern built_function<bool(*)(u32*, const u32*, u32)> copy_data_swap_u32_cmp;
