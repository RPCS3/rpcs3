#pragma once
#include "../rsx_cache.h"

struct alignas(4) glsl_shader_matrix_buffer
{
	float viewport_matrix[4 * 4];
	float window_matrix[4 * 4];
	float normalize_matrix[4 * 4];
};

void init_glsl_cache_program_context(rsx::program_cache_context &ctxt);
