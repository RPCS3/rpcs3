#pragma once

#include <util/types.hpp>
#include <array>
#include "Emu/RSX/Common/bitfield.hpp"
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
		stencil_face_state front;
		stencil_face_state back;

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

	// Decoded pipeline state shared by every backend.
	// Written only by the core renderer. Backends read it and translate it into host API state.
	struct draw_state_t
	{
		// Drawing state groups. A group's bit in 'dirty' is set when its contents changed since the backend last consumed them.
		enum class group : u32
		{
			raster         = (1 << 0),
			depth_stencil  = (1 << 1),
			blend          = (1 << 2),
			multisample    = (1 << 3),
			viewport       = (1 << 4),
			input_assembly = (1 << 5),
			clear          = (1 << 6),
			program_env    = (1 << 7),

			all = raster | depth_stencil | blend | multisample | viewport | input_assembly | clear | program_env
		};

		bitmask_t<group> dirty;

		// Groups whose source registers changed since they were last decoded. Raised through draw_state_signals.
		bitmask_t<group> pending;

		raster_state raster;
		depth_stencil_state depth_stencil;
		blend_state blend;
		multisample_state multisample;
	};
}
