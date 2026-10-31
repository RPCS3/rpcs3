#pragma once

#include <util/types.hpp>
#include <array>
#include "Emu/RSX/Common/bitfield.hpp"
#include "Emu/RSX/Common/TextureUtils.h"
#include "Emu/RSX/gcm_enums.h"

namespace rsx
{
	struct raster_state
	{
		bool cull_face_enabled = false;
		cull_face cull_face_mode{};
		front_face front_face_mode{};
		polygon_mode polygon_mode_front{};
		polygon_mode polygon_mode_back{};

		f32 line_width = 0.f;
		bool line_smooth_enabled = false;

		bool poly_offset_point_enabled = false;
		bool poly_offset_line_enabled = false;
		bool poly_offset_fill_enabled = false;
		f32 poly_offset_scale = 0.f;
		f32 poly_offset_bias = 0.f;

		bool operator==(const raster_state&) const = default;
	};

	struct stencil_face_state
	{
		comparison_function func{};
		u8 func_ref = 0;
		u8 func_mask = 0;
		u8 write_mask = 0;
		stencil_op op_fail{};
		stencil_op op_zfail{};
		stencil_op op_zpass{};

		bool operator==(const stencil_face_state&) const = default;
	};

	struct depth_stencil_state
	{
		bool depth_test_enabled = false;
		bool depth_write_enabled = false;

		// Only decoded while the depth test is enabled
		comparison_function depth_func{};

		bool depth_clamp_enabled = false;
		bool depth_clip_enabled = false;

		// Bounds are saturated into the range of the bound RSX depth format, as hardware does.
		// Only decoded while the depth bounds test is enabled.
		bool depth_bounds_test_enabled = false;
		f32 depth_bounds_min = 0.f;
		f32 depth_bounds_max = 0.f;

		bool stencil_test_enabled = false;
		bool two_sided_stencil_test_enabled = false;

		// Only decoded while the stencil test is enabled. The back face is only decoded for two-sided stencil.
		stencil_face_state front{};
		stencil_face_state back{};

		bool operator==(const depth_stencil_state&) const = default;
	};

	struct color_write_mask
	{
		bool r = false;
		bool g = false;
		bool b = false;
		bool a = false;

		bool operator==(const color_write_mask&) const = default;
	};

	struct blend_state
	{
		// Effective write mask per color target. The RSX mask is remapped to the channel layout the surface format is stored in
		// and masked to the channels that layout has.
		std::array<color_write_mask, 4> color_mask{};

		bool dither_enabled = false;

		// LogicOp and Blend are mutually exclusive. If both are enabled, LogicOp takes precedence.
		bool logic_op_enabled = false;

		// Only decoded while the logic op is enabled
		logic_op logic_operation{};

		// One bit per color target
		u8 enable_mask = 0;

		// Only decoded while blending is enabled on at least one target
		blend_factor sfactor_rgb{};
		blend_factor sfactor_a{};
		blend_factor dfactor_rgb{};
		blend_factor dfactor_a{};
		blend_equation equation_rgb{};
		blend_equation equation_a{};
		std::array<f32, 4> constant_color{};

		bool operator==(const blend_state&) const = default;
	};

	struct multisample_state
	{
		surface_antialiasing antialias{};
		bool msaa_enabled = false;
		u16 sample_mask = 0;
		bool alpha_to_coverage_enabled = false;
		bool alpha_to_one_enabled = false;

		bool operator==(const multisample_state&) const = default;
	};

	struct viewport_state
	{
		// Size of the surface clip region, before resolution scaling.
		// The viewport transform itself is part of the vertex program's scale/offset, so the host viewport covers this region.
		u16 clip_width = 0;
		u16 clip_height = 0;

		// Depth range
		f32 z_clip_min = 0.f;
		f32 z_clip_max = 0.f;

		bool operator==(const viewport_state&) const = default;
	};

	// The primitive type and the vertex base offset/index are not part of this group.
	// The primitive comes from the draw clause, and the vertex bases are rewritten between subdraws by draw clause barriers.
	struct input_assembly_state
	{
		// False if the restart index cannot be represented in the bound index type
		bool restart_index_enabled = false;
		u32 restart_index = 0;

		// Index type of the bound index array. Immediate (inlined) index draws always use u32.
		index_array_type index_type{};

		bool operator==(const input_assembly_state&) const = default;
	};

	// Kept apart from the raster group so that pattern writes do not reload the rasterizer state
	struct stipple_state
	{
		bool enabled = false;

		// The 32 pattern registers as written. Only decoded while stipple is enabled.
		std::array<u32, 32> pattern{};

		bool operator==(const stipple_state&) const = default;
	};

	// Fragment environment fields that select host shader variants. The rest of the environment block is packed by the core.
	struct fragment_env_state
	{
		comparison_function alpha_func{};

		bool operator==(const fragment_env_state&) const = default;
	};

	// Scissor region before resolution scaling. Draws are clipped by the viewport, clears are not.
	struct scissor_state
	{
		u16 x1 = 0;
		u16 y1 = 0;
		u16 x2 = 0;
		u16 y2 = 0;
	};

	// Decoded for every clear command right before the backend's clear() hook. It is not change-tracked.
	struct clear_state
	{
		// RSX_GCM_CLEAR_* bits. The stencil bit has already been dropped if the stencil write mask is zero.
		rsx::flags32_t mask = 0;

		surface_depth_format2 depth_format{};
		u8 stencil_write_mask = 0;

		// The scissor covers the whole surface clip region
		bool scissor_covers_surface = false;

		// Only decoded when the depth bit is set. Host-specific depth encodings are not applied.
		f32 depth_value = 0.f;

		// Only decoded when the stencil bit is set and the depth format has a stencil aspect
		u8 stencil_value = 0;

		// Color mask and value, remapped to the channel layout the surface format is stored in.
		// The mask is zero when no color bit is set or the format cannot be cleared. The other color fields are only decoded when it is non-zero.
		rsx::flags32_t color_mask = 0;
		u8 color_r = 0;
		u8 color_g = 0;
		u8 color_b = 0;
		u8 color_a = 0;

		// The mask writes every channel of that layout
		bool color_mask_full = false;
	};

	// Decoded pipeline state shared by every backend.
	// Written only by the core renderer. Backends read it and translate it into host API state.
	struct draw_state_t
	{
		// Drawing state groups. A group's bit in 'dirty' is set when its contents changed since the backend last consumed them.
		// Groups raised through draw_state_signals must fit in its u8 entries, so they use bits 0-7. Signal-only groups use the bits above.
		enum class group : u32
		{
			raster                       = (1 << 0),
			depth_stencil                = (1 << 1),
			blend                        = (1 << 2),
			multisample                  = (1 << 3),
			viewport                     = (1 << 4),
			input_assembly               = (1 << 5),
			stipple                      = (1 << 6),

			// The fragment environment block inputs changed. Only alpha_func is decoded.
			fragment_env                 = (1 << 7),

			// Signal that the inputs of the vertex environment block changed. Not used for state decode.
			vertex_env                   = (1 << 8),

			// Signal that the program was resolved again for this draw. Not used for state decode.
			vertex_program               = (1 << 9),
			fragment_program             = (1 << 10),

			// Signal that the scissor region was resolved again. Not used for state decode.
			scissor                      = (1 << 11),

			// Signal that the depth surface left cyclic (read/write) use. Its writes must be made visible before the next draw.
			depth_cyclic_barrier         = (1 << 12),

			// Signal that the guest constants changed. Backends that skip an upload on purpose keep their own record of it.
			transform_constants          = (1 << 13),
			fragment_constants           = (1 << 14),

			// Signal that the texture parameters block (coordinate scale/bias/clamp of the referenced textures) changed. Not used for state decode.
			fragment_texture_env         = (1 << 15),

			// Meta groups
			decoded                      = raster | depth_stencil | blend | multisample | viewport | input_assembly | stipple | fragment_env,
			programs                     = vertex_program | fragment_program,
			constants                    = transform_constants | fragment_constants,
			pipelines_dirty_vk           = raster | depth_stencil | blend | multisample | input_assembly,
			all                          = decoded | vertex_env | fragment_env | programs | scissor | depth_cyclic_barrier | constants | fragment_texture_env,
		};

		bitmask_t<group> dirty{};

		// Groups whose source registers changed since they were last decoded. Raised through draw_state_signals.
		bitmask_t<group> pending{};

		raster_state raster{};
		depth_stencil_state depth_stencil{};
		blend_state blend{};
		multisample_state multisample{};
		viewport_state viewport{};
		input_assembly_state input_assembly{};
		stipple_state stipple{};
		fragment_env_state fragment_env{};

		// Resolved by the GRAPH engine when the framebuffer is bound, not decoded
		scissor_state scissor{};

		// Not a group: decoded per clear command
		clear_state clear{};
	};
}
