#pragma once

#include "util/types.hpp"

namespace vk
{
	struct pipeline_binding_table
	{
		u8 vertex_params_bind_slot             = 0;
		u8 vertex_constant_buffers_bind_slot   = 1;
		u8 fragment_constant_buffers_bind_slot = 2;
		u8 fragment_state_bind_slot            = 3;
		u8 fragment_texture_params_bind_slot   = 4;
		u8 vertex_buffers_first_bind_slot      = 5;
		u8 conditional_render_predicate_slot   = 8;
		u8 rasterizer_env_bind_slot            = 9;
		u8 instancing_lookup_table_bind_slot   = 10;
		u8 instancing_constants_buffer_slot    = 11;
		u8 textures_first_bind_slot            = 12;
		u8 vertex_textures_first_bind_slot     = 12;                              // Invalid, has to be initialized properly
		u8 total_descriptor_bindings           = vertex_textures_first_bind_slot; // Invalid, has to be initialized properly
	};
}
