#pragma once

#include <util/types.hpp>

namespace rsx
{
	enum pipeline_state : u32
	{
		fragment_program_ucode_dirty = (1 << 0),   // Fragment program ucode changed
		vertex_program_ucode_dirty   = (1 << 1),   // Vertex program ucode changed
		fragment_program_state_dirty = (1 << 2),   // Fragment program state changed
		vertex_program_state_dirty   = (1 << 3),   // Vertex program state changed
		fragment_state_dirty         = (1 << 4),   // Fragment state changed (alpha test, etc)
		vertex_state_dirty           = (1 << 5),   // Vertex state changed (scale_offset, clip planes, etc)
		transform_constants_dirty    = (1 << 6),   // Transform constants changed
		fragment_constants_dirty     = (1 << 7),   // Fragment constants changed
		framebuffer_reads_dirty      = (1 << 8),   // Framebuffer contents changed
		fragment_texture_state_dirty = (1 << 9),   // Fragment texture parameters changed
		vertex_texture_state_dirty   = (1 << 10),  // Fragment texture parameters changed
		scissor_config_state_dirty   = (1 << 11),  // Scissor region changed
		zclip_config_state_dirty     = (1 << 12),  // Viewport Z clip changed

		scissor_setup_invalid        = (1 << 13),  // Scissor configuration is broken
		scissor_setup_clipped        = (1 << 14),  // Scissor region is cropped by viewport constraint

		polygon_stipple_pattern_dirty = (1 << 15),  // Rasterizer stippling pattern changed
		line_stipple_pattern_dirty    = (1 << 16),  // Line stippling pattern changed

		push_buffer_arrays_dirty      = (1 << 17),   // Push buffers have data written to them (immediate mode vertex buffers)

		polygon_offset_state_dirty    = (1 << 18), // Polygon offset config was changed
		depth_bounds_state_dirty      = (1 << 19), // Depth bounds configuration changed

		pipeline_config_dirty         = (1 << 20), // Generic pipeline configuration changes. Shader peek hint.

		rtt_config_dirty              = (1 << 21), // Render target configuration changed
		rtt_config_contested          = (1 << 22), // Render target configuration is indeterminate
		rtt_config_valid              = (1 << 23), // Render target configuration is valid
		rtt_cache_state_dirty         = (1 << 24), // Texture cache state is indeterminate

		xform_instancing_state_dirty  = (1 << 25), // Transform instancing state has changed

		// TODO - Should signal that we simply need to do a FP compare before the next draw call and invalidate the ucode if the content has changed.
		// Marking as dirty to invalidate hot cache also works, it's not like there's tons of barriers per frame anyway.
		fragment_program_needs_rehash = fragment_program_ucode_dirty,

		fragment_program_dirty = fragment_program_ucode_dirty | fragment_program_state_dirty,
		vertex_program_dirty = vertex_program_ucode_dirty | vertex_program_state_dirty,
		invalidate_pipeline_bits = fragment_program_dirty | vertex_program_dirty | xform_instancing_state_dirty,
		invalidate_zclip_bits = vertex_state_dirty | zclip_config_state_dirty,
		memory_barrier_bits = framebuffer_reads_dirty,

		// Vulkan-specific signals
		invalidate_vk_dynamic_state = zclip_config_state_dirty | scissor_config_state_dirty | polygon_offset_state_dirty | depth_bounds_state_dirty,

		all_dirty = ~0u
	};
}
