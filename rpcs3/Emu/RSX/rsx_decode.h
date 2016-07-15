#pragma once
#include "Utilities/types.h"
#include <tuple>
#include "GCM.h"
#include "rsx_methods.h"
#pragma warning(disable:4503)

namespace
{
	struct split_reg_half_uint_decode
	{
		static std::tuple<u16, u16> decode(u32 reg)
		{
			return std::make_tuple(reg & 0xffff, reg >> 16);
		}
	};

	template<u16 rsx::rsx_state::*FirstMember, u16 rsx::rsx_state::*SecondMember>
	struct as_u16x2
	{
		static std::tuple<u16, u16> decode(u32 reg)
		{
			return std::make_tuple(reg & 0xffff, reg >> 16);
		}

		static void commit_rsx_state(rsx::rsx_state &state, std::tuple<u16, u16> &&decoded_values)
		{
			state.*FirstMember = std::get<0>(decoded_values);
			state.*SecondMember = std::get<1>(decoded_values);
		}
	};

	template<f32 rsx::rsx_state::*Member>
	struct as_f32
	{
		static f32 decode(u32 reg)
		{
			return reinterpret_cast<f32&>(reg);
		}

		static void commit_rsx_state(rsx::rsx_state &state, f32 &&decoded_values)
		{
			state.*Member = decoded_values;
		}
	};


	template<u32 rsx::rsx_state::*Member>
	struct as_u32
	{
		static u32 decode(u32 reg)
		{
			return reg;
		}

		static void commit_rsx_state(rsx::rsx_state &state, u32 &&decoded_values)
		{
			state.*Member = decoded_values;
		}
	};

	struct as_bool
	{
		static bool decode(u32 reg)
		{
			return !!reg;
		}
	};

	struct as_unused
	{
		static u32 decode(u32 reg) { return reg; }
		static void commit_rsx_state(rsx::rsx_state &state, u32 &&) {}
	};

	std::tuple<u8, u8, u8, u8> split_reg_quad_uchar(u32 reg)
	{
		return std::make_tuple(reg & 0xff, (reg >> 8) & 0xff, (reg >> 16) & 0xff, (reg >> 24) & 0xff);
	}

	std::string get_subreg_name(u8 subreg)
	{
		return subreg == 0 ? "x" :
			subreg == 1 ? "y" :
			subreg == 2 ? "z" :
			"w";
	}

	std::string print_vertex_attribute_format(rsx::vertex_base_type type)
	{
		switch (type)
		{
		case rsx::vertex_base_type::s1: return "Signed short normalized";
		case rsx::vertex_base_type::f: return "Float";
		case rsx::vertex_base_type::sf: return "Half float";
		case rsx::vertex_base_type::ub: return "Unsigned byte normalized";
		case rsx::vertex_base_type::s32k: return "Signed short unormalized";
		case rsx::vertex_base_type::cmp: return "CMP";
		case rsx::vertex_base_type::ub256: return "Unsigned byte unormalized";
		}
		throw;
	}
}

namespace rsx
{
	std::string print_boolean(bool b);
	std::string print_comparison_function(comparison_function f);
	std::string print_stencil_op(stencil_op op);
	std::string print_fog_mode(fog_mode op);
	std::string print_logic_op(logic_op op);
	std::string print_front_face(front_face op);
	std::string print_cull_face(cull_face op);
	std::string print_surface_target(surface_target target);
	std::string print_primitive_mode(primitive_type draw_mode);
	std::string print_transfer_operation(blit_engine::transfer_operation op);
	std::string print_transfer_source_format(blit_engine::transfer_source_format op);
	std::string print_context_surface(blit_engine::context_surface op);
	std::string print_transfer_destination_format(blit_engine::transfer_destination_format op);
	std::string print_blend_op(blend_equation op);
	std::string print_blend_factor(blend_factor factor);
	std::string print_origin_mode(window_origin origin);
	std::string print_pixel_center_mode(window_pixel_center in);
	std::string print_user_clip_plane_op(user_clip_plane_op op);
	std::string print_depth_stencil_surface_format(surface_depth_format format);
	std::string print_surface_antialiasing(surface_antialiasing format);
	std::string print_surface_color_format(surface_color_format format);
	std::string print_index_type(index_array_type arg);
	std::string print_context_dma(blit_engine::context_dma op);
	std::string print_transfer_origin(blit_engine::transfer_origin op);
	std::string print_transfer_interpolator(blit_engine::transfer_interpolator op);
	std::string print_shading_mode(shading_mode op);
	std::string print_polygon_mode(polygon_mode op);
	std::string to_string(texture::format format);
	std::string to_string(texture::border_type in);
	std::string to_string(texture::layout in);
	std::string to_string(texture::coordinates in);
	std::string to_string(texture::dimension in);
	std::string to_string(texture::wrap_mode wrap);
	std::string to_string(texture::zfunc op);
	std::string to_string(texture::unsigned_remap op);
	std::string to_string(texture::signed_remap op);
	std::string to_string(texture::minify_filter op);
	std::string to_string(texture::magnify_filter op);
	std::string to_string(texture::component_remap op);

template<uint32_t Register>
struct registers_decoder
{};

template<uint32_t Register>
void commit(rsx_state &state, u32 value)
{
	registers_decoder<Register>::commit_rsx_state(state, registers_decoder<Register>::decode(value));
}

template<uint32_t Register>
std::string print_register_value(u32 value)
{
	return registers_decoder<Register>::dump(registers_decoder<Register>::decode(value));
}

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_HORIZONTAL> :
	public as_u16x2<&rsx_state::m_viewport_origin_x, &rsx_state::m_viewport_width>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Viewport: x = " + std::to_string(std::get<0>(decoded_values)) + " width = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_VERTICAL>
	: public as_u16x2<&rsx_state::m_viewport_origin_y, &rsx_state::m_viewport_height>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Viewport: y = " + std::to_string(std::get<0>(decoded_values)) + " height = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_SCISSOR_HORIZONTAL>
	: public as_u16x2<&rsx_state::m_scissor_origin_x, &rsx_state::m_scissor_width>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Scissor: x = " + std::to_string(std::get<0>(decoded_values)) + " width = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_SCISSOR_VERTICAL>
	: public as_u16x2<&rsx_state::m_scissor_origin_y, &rsx_state::m_scissor_height>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Scissor: y  = " + std::to_string(std::get<0>(decoded_values)) + " height = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_CLIP_HORIZONTAL>
	: public as_u16x2<&rsx_state::m_surface_clip_origin_x, &rsx_state::m_surface_clip_width>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Surface: clip x  = " + std::to_string(std::get<0>(decoded_values)) + " width = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder< NV4097_SET_SURFACE_CLIP_VERTICAL>
	: public as_u16x2<&rsx_state::m_surface_clip_origin_y, &rsx_state::m_surface_clip_height>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Surface: clip y  = " + std::to_string(std::get<0>(decoded_values)) + " height = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_CLEAR_RECT_HORIZONTAL>
	: public as_u16x2<&rsx_state::m_clear_rect_origin_x, &rsx_state::m_clear_rect_width>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Clear: rect x  = " + std::to_string(std::get<0>(decoded_values)) + " width = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_CLEAR_RECT_VERTICAL>
	: public as_u16x2<&rsx_state::m_clear_rect_origin_y, &rsx_state::m_clear_rect_height>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Clear: rect y  = " + std::to_string(std::get<0>(decoded_values)) + " height = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder< NV3089_CLIP_POINT>
	: public as_u16x2<&rsx_state::m_blit_engine_clip_x, &rsx_state::m_blit_engine_clip_y>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Blit engine: clip x  = " + std::to_string(std::get<0>(decoded_values)) + " y = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV3089_CLIP_SIZE>
	: public as_u16x2<&rsx_state::m_blit_engine_width, &rsx_state::m_blit_engine_height>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Blit engine: clip width  = " + std::to_string(std::get<0>(decoded_values)) + " height = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV3089_IMAGE_OUT_POINT>
	: public as_u16x2<&rsx_state::m_blit_engine_output_x, &rsx_state::m_blit_engine_output_y>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Blit engine: output x  = " + std::to_string(std::get<0>(decoded_values)) + " y = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_WINDOW_OFFSET>
	: public as_u16x2<&rsx_state::m_shader_window_offset_x, &rsx_state::m_shader_window_offset_y>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Window: offset x  = " + std::to_string(std::get<0>(decoded_values)) + " y = " + std::to_string(std::get<1>(decoded_values));
	}
};


template<>
struct registers_decoder<NV3089_IMAGE_OUT_SIZE>
	: public as_u16x2<&rsx_state::m_blit_engine_output_width, &rsx_state::m_blit_engine_output_height>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Blit engine: output width  = " + std::to_string(std::get<0>(decoded_values)) + " height = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV3089_IMAGE_IN_SIZE>
	: public as_u16x2<&rsx_state::m_blit_engine_input_width, &rsx_state::m_blit_engine_input_height>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Blit engine: input width  = " + std::to_string(std::get<0>(decoded_values)) + " height = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV3062_SET_PITCH>
	: public as_u16x2<&rsx_state::m_blit_engine_output_alignement_nv3062, &rsx_state::m_blit_engine_output_pitch_nv3062>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Blit engine: output alignment  = " + std::to_string(std::get<0>(decoded_values)) + " pitch = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder< NV308A_POINT>
	: public as_u16x2<&rsx_state::m_nv308a_x, &rsx_state::m_nv308a_y>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "NV308A: x  = " + std::to_string(std::get<0>(decoded_values)) + " y = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_VERTEX_ATTRIB_INPUT_MASK> : public split_reg_half_uint_decode
{
	static void commit_rsx_state(rsx_state &state, std::tuple<u16, u16> &&decoded_values)
	{
		std::tie(state.m_vertex_attrib_input_mask, std::ignore) = decoded_values;
	}

	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		std::string result = "Transform program enabled inputs:";
		const std::string input_names[] =
		{
			"in_pos", "in_weight", "in_normal",
			"in_diff_color", "in_spec_color",
			"in_fog",
			"in_point_size", "in_7",
			"in_tc0", "in_tc1", "in_tc2", "in_tc3",
			"in_tc4", "in_tc5", "in_tc6", "in_tc7"
		};
		for (unsigned i = 0; i < 16; i++)
			if (std::get<0>(decoded_values) & (1 << i))
				result += input_names[i] + " ";
		return result + " ? = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_FREQUENCY_DIVIDER_OPERATION> : public split_reg_half_uint_decode
{
	static void commit_rsx_state(rsx_state &state, std::tuple<u16, u16> &&decoded_values)
	{
		std::tie(state.m_frequency_divider_operation_mask, std::ignore) = decoded_values;
	}

	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		std::string result = "Frequency divider: ";
		for (unsigned i = 0; i < 16; i++)
			if (std::get<0>(decoded_values) & (1 << i))
				result += std::to_string(i) + " ";
		return result + " ? = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_TEST_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_depth_test_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Depth: test " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_MASK> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_depth_write_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Depth: write " + print_boolean(decoded_values);
	}
};


template<>
struct registers_decoder<NV4097_SET_ALPHA_TEST_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_alpha_test_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Alpha: test " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_TEST_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_stencil_test_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Stencil: test " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_RESTART_INDEX_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_restart_index_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Restart Index: " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_depth_bounds_test_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Depth: bound test " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_LOGIC_OP_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_logic_op_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Logic: " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_DITHER_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_dither_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Dither: " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_blend_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Blend: " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_LINE_SMOOTH_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_line_smooth_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Line: smooth " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_POLY_OFFSET_POINT_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_poly_offset_point_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Polygon: offset point " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_POLY_OFFSET_LINE_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_offset_line_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Polygon: offset line " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_POLY_OFFSET_FILL_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_poly_offset_fill_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Polygon: offset fill " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_CULL_FACE_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_cull_face_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Cull face: " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_POLY_SMOOTH_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_poly_smooth_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Polygon: smooth " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_two_sided_stencil_test_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Stencil: per side " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_TWO_SIDE_LIGHT_EN> : public as_bool
{
	static void commit_rsx_state(rsx_state &state, bool &&decoded_values)
	{
		state.m_two_side_light_enabled = decoded_values;
	}

	static std::string dump(bool &&decoded_values)
	{
		return "Light: per side " + print_boolean(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_BOUNDS_MIN>
	: as_f32<&rsx_state::m_depth_bounds_min>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Depth: bound min = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_BOUNDS_MAX>
	: as_f32<&rsx_state::m_depth_bounds_max>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Depth: bound max = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_FOG_PARAMS>
	: as_f32<&rsx_state::m_fog_params_0>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Fog: param 0 = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_FOG_PARAMS + 1>
	: as_f32<&rsx_state::m_fog_params_1>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Fog: param 1 = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_CLIP_MIN>
	: as_f32<&rsx_state::m_clip_min>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Depth: clip min = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_CLIP_MAX>
	: as_f32<&rsx_state::m_clip_max>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Depth: clip max = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR>
	: as_f32<&rsx_state::m_poly_offset_scale>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Polygon: offset scale = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_POLYGON_OFFSET_BIAS>
	: as_f32<&rsx_state::m_poly_offset_bias>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Polygon: offset bias = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_SCALE>
	: as_f32<&rsx_state::m_viewport_scale_x>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Viewport: scale x = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_SCALE + 1>
	: as_f32<&rsx_state::m_viewport_scale_y>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Viewport: scale y = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_SCALE + 2>
	: as_f32<&rsx_state::m_viewport_scale_z>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Viewport: scale z = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_SCALE + 3>
	: as_f32<&rsx_state::m_viewport_scale_w>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Viewport: scale w = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_OFFSET>
	: as_f32<&rsx_state::m_viewport_offset_x>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Viewport: offset x = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_OFFSET + 1>
	: as_f32<&rsx_state::m_viewport_offset_y>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Viewport: offset y = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_OFFSET + 2>
	: as_f32<&rsx_state::m_viewport_offset_z>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Viewport: offset z = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_VIEWPORT_OFFSET + 3>
	: as_f32<&rsx_state::m_viewport_offset_w>
{
	static std::string dump(f32 &&decoded_values)
	{
		return "Viewport: offset w = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_RESTART_INDEX>
	: as_u32<&rsx_state::m_restart_index>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Restart index: " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_COLOR_AOFFSET>
	: as_u32<&rsx_state::m_surface_a_offset>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: A offset " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_COLOR_BOFFSET>
	: as_u32<&rsx_state::m_surface_b_offset>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: B offset " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_COLOR_COFFSET>
	: as_u32<&rsx_state::m_surface_c_offset>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: C offset " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_COLOR_DOFFSET>
	: as_u32<&rsx_state::m_surface_d_offset>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: D offset " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_PITCH_A>
	: as_u32<&rsx_state::m_surface_a_pitch>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: A pitch " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_PITCH_B>
	: as_u32<&rsx_state::m_surface_b_pitch>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: B pitch " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_PITCH_C>
	: as_u32<&rsx_state::m_surface_c_pitch>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: C pitch " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_PITCH_D>
	: as_u32<&rsx_state::m_surface_d_pitch>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: D pitch " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_ZETA_OFFSET>
	: as_u32<&rsx_state::m_surface_z_offset>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: Z offset " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_PITCH_Z>
	: as_u32<&rsx_state::m_surface_z_pitch>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: Z pitch " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK>
	: as_u32<&rsx_state::m_vertex_attrib_output_mask>
{
	static std::string dump(u32 &&decoded_values)
	{
		const std::string output_names[] =
		{
			"diffuse_color",
			"specular_color",
			"back_diffuse_color",
			"back_specular_color",
			"fog",
			"point_size",
			"clip_distance[0]",
			"clip_distance[1]",
			"clip_distance[2]",
			"clip_distance[3]",
			"clip_distance[4]",
			"clip_distance[5]",
			"tc8",
			"tc9",
			"tc0",
			"tc1",
			"tc2",
			"tc3",
			"tc4",
			"tc5",
			"tc6",
			"tc7"
		};
		std::string result = "Transform program outputs:";
		for (unsigned i = 0; i < 22; i++)
			if (decoded_values & (1 << i))
				result += output_names[i] + " ";
		return result;
	}
};

template<>
struct registers_decoder<NV4097_SET_SHADER_CONTROL>
	: as_u32<&rsx_state::m_shader_control>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Shader control: raw_value =" + std::to_string(decoded_values) +
			" reg_count = " + std::to_string((decoded_values >> 24) & 0xFF) +
			((decoded_values & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT) ? " depth_replace " : "") +
			((decoded_values & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) ? " 32b_exports " : "");
	}
};

template<>
struct registers_decoder<NV4097_SET_VERTEX_DATA_BASE_OFFSET>
	: as_u32<&rsx_state::m_vertex_data_base_offset>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Vertex: base offset " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_INDEX_ARRAY_ADDRESS>
	: as_u32<&rsx_state::m_index_array_address>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Index: array offset " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_VERTEX_DATA_BASE_INDEX>
	: as_u32<&rsx_state::m_vertex_data_base_index>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Vertex: base index " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SHADER_PROGRAM>
	: as_u32<&rsx_state::m_shader_program_address>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Shader: program offset = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_TRANSFORM_PROGRAM_START>
	: as_u32<&rsx_state::m_transform_program_start>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Transform program: start = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV406E_SEMAPHORE_OFFSET>
	: as_u32<&rsx_state::m_semaphore_offset_406e>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV406E semaphore: offset = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SEMAPHORE_OFFSET>
	: as_u32<&rsx_state::m_semaphore_offset_4097>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "semaphore: offset = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV3089_IMAGE_IN_OFFSET>
	: as_u32<&rsx_state::m_blit_engine_input_offset>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV3089: input offset = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV3062_SET_OFFSET_DESTIN>
	: as_u32<&rsx_state::m_blit_engine_output_offset_nv3062>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV3062: output offset = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV309E_SET_OFFSET>
	: as_u32<&rsx_state::m_blit_engine_nv309E_offset>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV309E: offset = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV3089_DS_DX>
	: as_u32<&rsx_state::m_blit_engine_ds_dx>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV3089: dsdx = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV3089_DT_DY>
	: as_u32<&rsx_state::m_blit_engine_dt_dy>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV3089: dtdy = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV0039_PITCH_IN>
	: as_u32<&rsx_state::m_nv0039_input_pitch>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV0039: input pitch = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV0039_PITCH_OUT>
	: as_u32<&rsx_state::m_nv0039_output_pitch>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV0039: output pitch = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV0039_LINE_LENGTH_IN>
	: as_u32<&rsx_state::m_nv0039_line_length>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV0039: line lenght input = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV0039_LINE_COUNT>
	: as_u32<&rsx_state::m_nv0039_line_count>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV0039: line count = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV0039_OFFSET_OUT>
	: as_u32<&rsx_state::m_nv0039_output_offset>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV0039: output offset = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV0039_OFFSET_IN>
	: as_u32<&rsx_state::m_nv0039_input_offset>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV0039: input offset = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_DEPTH_FUNC>
{
	static auto decode(u32 value) { return to_comparison_function(value); }

	static void commit_rsx_state(rsx_state &state, comparison_function &&decoded_values)
	{
		state.m_depth_func = decoded_values;
	}

	static std::string dump(comparison_function &&decoded_values)
	{
		return "Depth: compare_function = " + print_comparison_function(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_FUNC>
{
	static auto decode(u32 value) { return to_comparison_function(value); }

	static void commit_rsx_state(rsx_state &state, comparison_function &&decoded_values)
	{
		state.m_stencil_func = decoded_values;
	}

	static std::string dump(comparison_function &&decoded_values)
	{
		return "Stencil: (front) compare_function = " + print_comparison_function(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_FUNC>
{
	static auto decode(u32 value) { return to_comparison_function(value); }

	static void commit_rsx_state(rsx_state &state, comparison_function &&decoded_values)
	{
		state.m_back_stencil_func = decoded_values;
	}

	static std::string dump(comparison_function &&decoded_values)
	{
		return "Stencil: back compare_function = " + print_comparison_function(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_ALPHA_FUNC>
{
	static auto decode(u32 value) { return to_comparison_function(value); }

	static void commit_rsx_state(rsx_state &state, comparison_function &&decoded_values)
	{
		state.m_alpha_func = decoded_values;
	}

	static std::string dump(comparison_function &&decoded_values)
	{
		return "Alpha: compare_function = " + print_comparison_function(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_OP_FAIL>
{
	static auto decode(u32 value) { return to_stencil_op(value); }

	static void commit_rsx_state(rsx_state &state, stencil_op &&decoded_values)
	{
		state.m_stencil_op_fail = decoded_values;
	}

	static std::string dump(stencil_op &&decoded_values)
	{
		return "Stencil: (front) fail op = " + print_stencil_op(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_OP_ZFAIL>
{
	static auto decode(u32 value) { return to_stencil_op(value); }

	static void commit_rsx_state(rsx_state &state, stencil_op &&decoded_values)
	{
		state.m_stencil_op_zfail = decoded_values;
	}

	static std::string dump(stencil_op &&decoded_values)
	{
		return "Stencil: (front) zfail op = " + print_stencil_op(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_OP_ZPASS>
{
	static auto decode(u32 value) { return to_stencil_op(value); }

	static void commit_rsx_state(rsx_state &state, stencil_op &&decoded_values)
	{
		state.m_stencil_op_zpass = decoded_values;
	}

	static std::string dump(stencil_op &&decoded_values)
	{
		return "Stencil: (front) zpass op = " + print_stencil_op(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_OP_FAIL>
{
	static auto decode(u32 value) { return to_stencil_op(value); }

	static void commit_rsx_state(rsx_state &state, stencil_op &&decoded_values)
	{
		state.m_back_stencil_op_fail = decoded_values;
	}

	static std::string dump(stencil_op &&decoded_values)
	{
		return "Stencil: (back) fail op = " + print_stencil_op(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_OP_ZFAIL>
{
	static auto decode(u32 value) { return to_stencil_op(value); }

	static void commit_rsx_state(rsx_state &state, stencil_op &&decoded_values)
	{
		state.m_back_stencil_op_zfail = decoded_values;
	}

	static std::string dump(stencil_op &&decoded_values)
	{
		return "Stencil: (back) zfail op = " + print_stencil_op(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_OP_ZPASS>
{
	static auto decode(u32 value) { return to_stencil_op(value); }

	static void commit_rsx_state(rsx_state &state, stencil_op &&decoded_values)
	{
		state.m_back_stencil_op_zpass = decoded_values;
	}

	static std::string dump(stencil_op &&decoded_values)
	{
		return "Stencil: (back) zpass op = " + print_stencil_op(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_FUNC_REF>
{
	static auto decode(u32 value) { return split_reg_quad_uchar(value); }

	static void commit_rsx_state(rsx_state &state, std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		state.m_stencil_func_ref = std::get<0>(decoded_values);
	}

	static std::string dump(std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		return "Stencil: (front) func ref = " + std::to_string(std::get<0>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_FUNC_REF>
{
	static auto decode(u32 value) { return split_reg_quad_uchar(value); }

	static void commit_rsx_state(rsx_state &state, std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		state.m_back_stencil_func_ref = std::get<0>(decoded_values);
	}

	static std::string dump(std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		return "Stencil: (back) func ref = " + std::to_string(std::get<0>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_FUNC_MASK>
{
	static auto decode(u32 value) { return split_reg_quad_uchar(value); }

	static void commit_rsx_state(rsx_state &state, std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		state.m_stencil_func_mask = std::get<0>(decoded_values);
	}

	static std::string dump(std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		return "Stencil: (front) func mask = " + std::to_string(std::get<0>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_FUNC_MASK>
{
	static auto decode(u32 value) { return split_reg_quad_uchar(value); }

	static void commit_rsx_state(rsx_state &state, std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		state.m_back_stencil_func_mask = std::get<0>(decoded_values);
	}

	static std::string dump(std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		return "Stencil: (back) func mask = " + std::to_string(std::get<0>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_ALPHA_REF>
{
	static auto decode(u32 value) { return split_reg_quad_uchar(value); }

	static void commit_rsx_state(rsx_state &state, std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		state.m_alpha_ref = std::get<0>(decoded_values);
	}

	static std::string dump(std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		return "Alpha: ref = " + std::to_string(std::get<0>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_COLOR_CLEAR_VALUE>
{
	static auto decode(u32 value) { return split_reg_quad_uchar(value); }

	static void commit_rsx_state(rsx_state &state, std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		std::tie(state.m_clear_color_b, state.m_clear_color_g, state.m_clear_color_r, state.m_clear_color_a) = decoded_values;
	}

	static std::string dump(std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		return "Clear: R = " + std::to_string(std::get<0>(decoded_values)) +
			" G = " + std::to_string(std::get<1>(decoded_values)) +
			" B = " + std::to_string(std::get<2>(decoded_values)) +
			" A = " + std::to_string(std::get<3>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_STENCIL_MASK>
{
	static auto decode(u32 value) { return split_reg_quad_uchar(value); }

	static void commit_rsx_state(rsx_state &state, std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		state.m_stencil_mask = std::get<0>(decoded_values);
	}

	static std::string dump(std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		return "Stencil: (front) mask = " + std::to_string(std::get<0>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_STENCIL_MASK>
{
	static auto decode(u32 value) { return split_reg_quad_uchar(value); }

	static void commit_rsx_state(rsx_state &state, std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		state.m_back_stencil_mask = std::get<0>(decoded_values);
	}

	static std::string dump(std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		return "Stencil: (back) mask = " + std::to_string(std::get<0>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_LOGIC_OP>
{
	static auto decode(u32 value) { return to_logic_op(value); }

	static void commit_rsx_state(rsx_state &state, logic_op &&decoded_values)
	{
		state.m_logic_operation = decoded_values;
	}

	static std::string dump(logic_op &&decoded_values)
	{
		return "Logic: op = " + print_logic_op(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_FRONT_FACE>
{
	static auto decode(u32 value) { return to_front_face(value); }

	static void commit_rsx_state(rsx_state &state, front_face &&decoded_values)
	{
		state.m_front_face_mode = decoded_values;
	}

	static std::string dump(front_face &&decoded_values)
	{
		return "Front Face: " + print_front_face(decoded_values);
	}
};


template<>
struct registers_decoder<NV4097_SET_CULL_FACE>
{
	static auto decode(u32 value) { return to_cull_face(value); }

	static void commit_rsx_state(rsx_state &state, cull_face &&decoded_values)
	{
		state.m_cull_face_mode = decoded_values;
	}

	static std::string dump(cull_face &&decoded_values)
	{
		return "Cull Face: " + print_cull_face(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_COLOR_TARGET>
{
	static auto decode(u32 value) { return to_surface_target(value); }

	static void commit_rsx_state(rsx_state &state, surface_target &&decoded_values)
	{
		state.m_surface_color_target = decoded_values;
	}

	static std::string dump(surface_target &&decoded_values)
	{
		return "Surface: Color target(s) = " + print_surface_target(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_FOG_MODE>
{
	static auto decode(u32 value) { return to_fog_mode(value); }

	static void commit_rsx_state(rsx_state &state, fog_mode &&decoded_values)
	{
		state.m_fog_equation = decoded_values;
	}

	static std::string dump(fog_mode &&decoded_values)
	{
		return "Fog: " + print_fog_mode(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_BEGIN_END>
{
	static auto decode(u32 value) { return to_primitive_type(value); }

	static void commit_rsx_state(rsx_state &state, primitive_type &&decoded_values)
	{
		state.m_primitive_type = decoded_values;
	}

	static std::string dump(primitive_type &&decoded_values)
	{
		return "-- " + print_primitive_mode(decoded_values) + " --";
	}
};

template<>
struct registers_decoder<NV3089_SET_OPERATION>
{
	static auto decode(u32 value) { return blit_engine::to_transfer_operation(value); }

	static void commit_rsx_state(rsx_state &state, blit_engine::transfer_operation &&decoded_values)
	{
		state.m_blit_engine_operation = decoded_values;
	}

	static std::string dump(blit_engine::transfer_operation &&decoded_values)
	{
		return "NV3089: op = " + print_transfer_operation(decoded_values);
	}
};

template<>
struct registers_decoder<NV3089_SET_COLOR_FORMAT>
{
	static auto decode(u32 value) { return blit_engine::to_transfer_source_format(value); }

	static void commit_rsx_state(rsx_state &state, blit_engine::transfer_source_format &&decoded_values)
	{
		state.m_blit_engine_src_color_format = decoded_values;
	}

	static std::string dump(blit_engine::transfer_source_format &&decoded_values)
	{
		return "NV3089: source fmt = " + print_transfer_source_format(decoded_values);
	}
};

template<>
struct registers_decoder<NV3089_SET_CONTEXT_SURFACE>
{
	static auto decode(u32 value) { return blit_engine::to_context_surface(value); }

	static void commit_rsx_state(rsx_state &state, blit_engine::context_surface &&decoded_values)
	{
		state.m_blit_engine_context_surface = decoded_values;
	}

	static std::string dump(blit_engine::context_surface &&decoded_values)
	{
		return "NV3089: context surface = " + print_context_surface(decoded_values);
	}
};

template<>
struct registers_decoder<NV3062_SET_COLOR_FORMAT>
{
	static auto decode(u32 value) { return blit_engine::to_transfer_destination_format(value); }

	static void commit_rsx_state(rsx_state &state, blit_engine::transfer_destination_format &&decoded_values)
	{
		state.m_blit_engine_nv3062_color_format = decoded_values;
	}

	static std::string dump(blit_engine::transfer_destination_format &&decoded_values)
	{
		return "NV3062: output fmt = " + print_transfer_destination_format(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_EQUATION>
{
	static auto decode(u32 value) {
		auto tmp = split_reg_half_uint_decode::decode(value);
		return std::make_tuple(to_blend_equation(std::get<0>(tmp)), to_blend_equation(std::get<1>(tmp)));
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<blend_equation, blend_equation> &&decoded_values)
	{
		state.m_blend_equation_rgb = std::get<0>(decoded_values);
		state.m_blend_equation_a = std::get<1>(decoded_values);
	}

	static std::string dump(std::tuple<blend_equation, blend_equation> &&decoded_values)
	{
		return "Blend: equation rgb = " + print_blend_op(std::get<0>(decoded_values)) + " a = " + print_blend_op(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_FUNC_SFACTOR>
{
	static auto decode(u32 value) {
		auto tmp = split_reg_half_uint_decode::decode(value);
		return std::make_tuple(to_blend_factor(std::get<0>(tmp)), to_blend_factor(std::get<1>(tmp)));
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<blend_factor, blend_factor> &&decoded_values)
	{
		state.m_blend_func_sfactor_rgb = std::get<0>(decoded_values);
		state.m_blend_func_sfactor_a = std::get<1>(decoded_values);
	}

	static std::string dump(std::tuple<blend_factor, blend_factor> &&decoded_values)
	{
		return "Blend: sfactor rgb = " + print_blend_factor(std::get<0>(decoded_values)) + " a = " + print_blend_factor(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_FUNC_DFACTOR>
{
	static auto decode(u32 value) {
		auto tmp = split_reg_half_uint_decode::decode(value);
		return std::make_tuple(to_blend_factor(std::get<0>(tmp)), to_blend_factor(std::get<1>(tmp)));
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<blend_factor, blend_factor> &&decoded_values)
	{
		state.m_blend_func_dfactor_rgb = std::get<0>(decoded_values);
		state.m_blend_func_dfactor_a = std::get<1>(decoded_values);
	}

	static std::string dump(std::tuple<blend_factor, blend_factor> &&decoded_values)
	{
		return "Blend: dfactor rgb = " + print_blend_factor(std::get<0>(decoded_values)) + " a = " + print_blend_factor(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_COLOR_MASK>
{
	static auto decode(u32 value) {
		auto tmp = split_reg_quad_uchar(value);
		return std::make_tuple(!!std::get<0>(tmp), !!std::get<1>(tmp), !!std::get<2>(tmp), !!std::get<3>(tmp));
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<bool, bool, bool, bool> &&decoded_values)
	{
		state.m_color_mask_b = std::get<0>(decoded_values);
		state.m_color_mask_g = std::get<1>(decoded_values);
		state.m_color_mask_r = std::get<2>(decoded_values);
		state.m_color_mask_a = std::get<3>(decoded_values);
	}

	static std::string dump(std::tuple<bool, bool, bool, bool> &&decoded_values)
	{
		return "Surface: color mask A = " + print_boolean(std::get<3>(decoded_values)) +
			" R = " + print_boolean(std::get<2>(decoded_values)) +
			" G = " + print_boolean(std::get<1>(decoded_values)) +
			" B = " + print_boolean(std::get<0>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_SHADER_WINDOW>
{
	static auto decode(u32 value) {
		return std::make_tuple(
			to_window_origin((value >> 12) & 0xf),
			to_window_pixel_center((value >> 16) & 0xf),
			static_cast<u16>(value & 0xfff)
		);
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<window_origin, window_pixel_center, u16> &&decoded_values)
	{
		state.m_shader_window_origin = std::get<0>(decoded_values);
		state.m_shader_window_pixel = std::get<1>(decoded_values);
		state.m_shader_window_height = std::get<2>(decoded_values);
	}

	static std::string dump(std::tuple<window_origin, window_pixel_center, u16> &&decoded_values)
	{
		return "Viewport: height = " + std::to_string(std::get<2>(decoded_values)) +
			" origin = " + print_origin_mode(std::get<0>(decoded_values)) +
			" pixel center = " + print_pixel_center_mode(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_ENABLE_MRT>
{
	static auto decode(u32 value) {
		return std::make_tuple(
			!!(value & 0x2),
			!!(value & 0x4),
			!!(value & 0x8)
		);
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<bool, bool, bool> &&decoded_values)
	{
		state.m_blend_enabled_surface_1 = std::get<0>(decoded_values);
		state.m_blend_enabled_surface_2 = std::get<1>(decoded_values);
		state.m_blend_enabled_surface_3 = std::get<2>(decoded_values);
	}

	static std::string dump(std::tuple<bool, bool, bool>&& decoded_values)
	{
		return "Blend: mrt1 = " + print_boolean(std::get<0>(decoded_values)) +
			" mrt2 = " + print_boolean(std::get<1>(decoded_values)) +
			" mrt3 = " + print_boolean(std::get<2>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_USER_CLIP_PLANE_CONTROL>
{
	static auto decode(u32 value) {
		return std::make_tuple(
			to_user_clip_plane_op(value & 0xf),
			to_user_clip_plane_op((value >> 4) & 0xf),
			to_user_clip_plane_op((value >> 8) & 0xf),
			to_user_clip_plane_op((value >> 12) & 0xf),
			to_user_clip_plane_op((value >> 16) & 0xf),
			to_user_clip_plane_op((value >> 20) & 0xf)
		);
	}

	static void commit_rsx_state(rsx_state &state,
		std::tuple<user_clip_plane_op, user_clip_plane_op, user_clip_plane_op, user_clip_plane_op, user_clip_plane_op, user_clip_plane_op> &&decoded_values)
	{
		state.m_clip_plane_0_enabled = std::get<0>(decoded_values);
		state.m_clip_plane_1_enabled = std::get<1>(decoded_values);
		state.m_clip_plane_2_enabled = std::get<2>(decoded_values);
		state.m_clip_plane_3_enabled = std::get<3>(decoded_values);
		state.m_clip_plane_4_enabled = std::get<4>(decoded_values);
		state.m_clip_plane_5_enabled = std::get<5>(decoded_values);
	}

	static std::string dump(std::tuple<user_clip_plane_op, user_clip_plane_op, user_clip_plane_op, user_clip_plane_op, user_clip_plane_op, user_clip_plane_op> &&decoded_values)
	{
		return "User clip: UC0 = " + print_user_clip_plane_op(std::get<0>(decoded_values)) +
			" UC1 = " + print_user_clip_plane_op(std::get<1>(decoded_values)) +
			" UC2 = " + print_user_clip_plane_op(std::get<2>(decoded_values)) +
			" UC2 = " + print_user_clip_plane_op(std::get<2>(decoded_values)) +
			" UC2 = " + print_user_clip_plane_op(std::get<2>(decoded_values)) +
			" UC2 = " + print_user_clip_plane_op(std::get<2>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_LINE_WIDTH>
{
	static auto decode(u32 value) {
		return (value >> 3) + (value & 7) / 8.f;
	}

	static void commit_rsx_state(rsx_state &state, f32 &&decoded_values)
	{
		state.m_line_width = decoded_values;
	}

	static std::string dump(f32 &&decoded_values)
	{
		return "Line width: " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_SURFACE_FORMAT>
{
	static auto decode(u32 value) {
		return std::make_tuple(
			to_surface_color_format(value & 0x1f),
			to_surface_depth_format((value >> 5) & 0x7),
			to_surface_antialiasing((value >> 12) & 0xf),
			static_cast<u8>((value >> 16) & 0xff),
			static_cast<u8>((value >> 24) & 0xff)
		);
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<surface_color_format, surface_depth_format, surface_antialiasing, u8, u8> &&decoded_values)
	{
		state.m_surface_color = std::get<0>(decoded_values);
		state.m_surface_depth_format = std::get<1>(decoded_values);
		state.m_surface_antialias = std::get<2>(decoded_values);
		state.m_surface_log2_width = std::get<3>(decoded_values);
		state.m_surface_log2_height = std::get<4>(decoded_values);
	}

	static std::string dump(std::tuple<surface_color_format, surface_depth_format, surface_antialiasing, u8, u8> &&decoded_values)
	{
		return "Surface: Color format = " + print_surface_color_format(std::get<0>(decoded_values)) +
			" DepthStencil format = " + print_depth_stencil_surface_format(std::get<1>(decoded_values)) +
			" Anti aliasing =" + print_surface_antialiasing(std::get<2>(decoded_values)) +
			" w = " + std::to_string(std::get<3>(decoded_values)) +
			" h = " + std::to_string(std::get<4>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_ZSTENCIL_CLEAR_VALUE>
{
	static auto decode(u32 value) {
		return std::make_tuple(value >> 8, static_cast<u8>(value & 0xff));
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<u32, u8> &&decoded_values)
	{
		state.m_z_clear_value = std::get<0>(decoded_values);
		state.m_stencil_clear_value = std::get<1>(decoded_values);
	}

	static std::string dump(std::tuple<u32, u8> &&decoded_values)
	{
		return "Clear: Z = " + std::to_string(std::get<0>(decoded_values)) +
			" Stencil = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_INDEX_ARRAY_DMA>
{
	static auto decode(u32 value) {
		return std::make_tuple(index_array_type(value >> 4), value & 0xf);
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<index_array_type, u8> &&decoded_values)
	{
		state.m_index_type = std::get<0>(decoded_values);
		state.m_index_array_location = std::get<1>(decoded_values);
	}

	static std::string dump(std::tuple<index_array_type, u8> &&decoded_values)
	{
		return "Index: type = " + print_index_type(std::get<0>(decoded_values)) +
			" dma = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_COLOR_A>
{
	static auto decode(u32 value) {
		return value;
	}

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		state.m_surface_a_dma = decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: A DMA = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_COLOR_B>
{
	static auto decode(u32 value) {
		return value;
	}

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		state.m_surface_b_dma = decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: B DMA = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_COLOR_C>
{
	static auto decode(u32 value) {
		return value;
	}

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		state.m_surface_c_dma = decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: C DMA = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_COLOR_D>
{
	static auto decode(u32 value) {
		return value;
	}

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		state.m_surface_d_dma = decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: D DMA = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_ZETA>
{
	static auto decode(u32 value) {
		return value;
	}

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		state.m_surface_z_dma = decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "Surface: Z DMA = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV3089_SET_CONTEXT_DMA_IMAGE>
{
	static auto decode(u32 value) {
		return value;
	}

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		state.m_blit_engine_input_location = decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "NV3089: input DMA = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN>
	: as_u32<&rsx_state::m_blit_engine_output_location_nv3062>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "NV3062: output DMA = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV309E_SET_CONTEXT_DMA_IMAGE>
{
	static auto decode(u32 value) {
		return value;
	}

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		state.m_blit_engine_nv309E_location = decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "NV309E: output DMA = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV0039_SET_CONTEXT_DMA_BUFFER_OUT>
{
	static auto decode(u32 value) {
		return value;
	}

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		state.m_nv0039_output_location = decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "NV0039: output DMA = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV0039_SET_CONTEXT_DMA_BUFFER_IN>
{
	static auto decode(u32 value) {
		return value;
	}

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		state.m_nv0039_input_location = decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "NV0039: input DMA = " + std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_CONTEXT_DMA_REPORT>
{
	static auto decode(u32 value) {
		return blit_engine::to_context_dma(value);
	}

	static void commit_rsx_state(rsx_state &state, blit_engine::context_dma &&decoded_values)
	{
		state.m_context_dma_report = decoded_values;
	}

	static std::string dump(blit_engine::context_dma &&decoded_values)
	{
		return "Report: context DMA = " + print_context_dma(decoded_values);
	}
};

template<>
struct registers_decoder<NV3089_IMAGE_IN_FORMAT>
{
	static auto decode(u32 value) {
		return std::make_tuple(static_cast<u16>(value),
			blit_engine::to_transfer_origin(value >> 16),
			blit_engine::to_transfer_interpolator(value >> 24));
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<u16, blit_engine::transfer_origin, blit_engine::transfer_interpolator> &&decoded_values)
	{
		state.m_blit_engine_input_pitch = std::get<0>(decoded_values);
		state.m_blit_engine_input_origin = std::get<1>(decoded_values);
		state.m_blit_engine_input_inter = std::get<2>(decoded_values);
	}

	static std::string dump(std::tuple<u16, blit_engine::transfer_origin, blit_engine::transfer_interpolator> &&decoded_values)
	{
		return "NV3089: input fmt " + std::to_string(std::get<0>(decoded_values)) +
			" origin = " + print_transfer_origin(std::get<1>(decoded_values)) +
			" interp = " + print_transfer_interpolator(std::get<2>(decoded_values));
	}
};

template<>
struct registers_decoder<NV309E_SET_FORMAT>
{
	static auto decode(u32 value) {
		return std::make_tuple(blit_engine::to_transfer_destination_format(value), static_cast<u8>(value >> 16), static_cast<u8>(value >> 24));
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<blit_engine::transfer_destination_format, u8, u8> &&decoded_values)
	{
		state.m_blit_engine_output_format_nv309E = std::get<0>(decoded_values);
		state.m_nv309e_sw_height_log2 = std::get<1>(decoded_values);
		state.m_nv309e_sw_width_log2 = std::get<2>(decoded_values);
	}

	static std::string dump(std::tuple<blit_engine::transfer_destination_format, u8, u8> &&decoded_values)
	{
		return "NV309E: output fmt = " + print_transfer_destination_format(std::get<0>(decoded_values)) +
			" log2-width = " + std::to_string(std::get<1>(decoded_values)) +
			" log2-height = " + std::to_string(std::get<2>(decoded_values));
	}
};

template<>
struct registers_decoder<NV0039_FORMAT>
{
	static auto decode(u32 value) {
		return std::make_tuple(static_cast<u8>(value & 0xff), static_cast<u8>(value >> 8));
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<u8, u8> &&decoded_values)
	{
		state.m_nv0039_input_format = std::get<0>(decoded_values);
		state.m_nv0039_output_format = std::get<1>(decoded_values);
	}

	static std::string dump(std::tuple<u8, u8> &&decoded_values)
	{
		return "NV0039: input format = " + std::to_string(std::get<0>(decoded_values)) +
			" output format = " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_COLOR2>
	: public as_u16x2<&rsx_state::m_blend_color_16b_b, &rsx_state::m_blend_color_16b_a>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "Blend color: 16b BA = " + std::to_string(std::get<0>(decoded_values)) +
			", " + std::to_string(std::get<1>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_BLEND_COLOR>
	: as_u32<&rsx_state::m_blend_color>
{
	static std::string dump(u32 &&decoded_values)
	{
		auto rgba8 = split_reg_quad_uchar(decoded_values);
		auto rg16 = split_reg_half_uint_decode::decode(decoded_values);
		return "Blend color: 8b RGBA = " +
			std::to_string(std::get<0>(rgba8)) + ", " + std::to_string(std::get<1>(rgba8)) + ", " + std::to_string(std::get<2>(rgba8)) + ", " + std::to_string(std::get<3>(rgba8)) +
			" 16b RG = " + std::to_string(std::get<0>(rg16)) + ", " + std::to_string(std::get<1>(rg16));
	}
};

template<>
struct registers_decoder<NV3089_IMAGE_IN>
	: public as_u16x2<&rsx_state::m_blit_engine_in_x, &rsx_state::m_blit_engine_in_y>
{
	static std::string dump(std::tuple<u16, u16> &&decoded_values)
	{
		return "NV3089: in x = " + std::to_string(std::get<0>(decoded_values) / 16.f) +
			" y = " + std::to_string(std::get<1>(decoded_values) / 16.f);
	}
};

template<>
struct registers_decoder<NV4097_NO_OPERATION> : public as_unused
{
	static std::string dump(u32 &&decoded_values)
	{
		return "(nop)";
	}
};

template<>
struct registers_decoder<NV4097_INVALIDATE_VERTEX_CACHE_FILE> : public as_unused
{
	static std::string dump(u32 &&decoded_values)
	{
		return "(invalidate vertex cache file)";
	}
};

template<>
struct registers_decoder<NV4097_INVALIDATE_VERTEX_FILE> : public as_unused
{
	static std::string dump(u32 &&decoded_values)
	{
		return "(invalidate vertex file)";
	}
};

template<>
struct registers_decoder<NV4097_SET_ANTI_ALIASING_CONTROL>
{
	static auto decode(u32 value)
	{
		return std::make_tuple(
			!!(value & 0x1),
			!!((value >> 4) & 0x1),
			!!((value >> 8) & 0x1),
			static_cast<u16>(value >> 16)
		);
	}

	static void commit_rsx_state(rsx_state &state, std::tuple<bool, bool, bool, u16> &&decoded_values)
	{
		state.m_msaa_enabled = std::get<0>(decoded_values);
		state.m_msaa_alpha_to_coverage = std::get<1>(decoded_values);
		state.m_msaa_alpha_to_one = std::get<2>(decoded_values);
		state.m_msaa_sample_mask = std::get<3>(decoded_values);
	}

	static std::string dump(std::tuple<bool, bool, bool, u16> &&decoded_values)
	{
		return "Anti_aliasing: " + print_boolean(std::get<0>(decoded_values)) +
			" alpha_to_coverage = " + print_boolean(std::get<1>(decoded_values)) +
			" alpha_to_one = " + print_boolean(std::get<2>(decoded_values)) +
			" sample_mask = " + std::to_string(std::get<3>(decoded_values));
	}
};

template<>
struct registers_decoder<NV4097_SET_SHADE_MODE>
{
	static auto decode(u32 value)
	{
		return to_shading_mode(value);
	}

	static void commit_rsx_state(rsx_state &state, rsx::shading_mode &&decoded_values)
	{
		state.m_shading_mode = decoded_values;
	}

	static std::string dump(rsx::shading_mode &&decoded_values)
	{
		return "Shading mode: " + print_shading_mode(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_FRONT_POLYGON_MODE>
{
	static auto decode(u32 value)
	{
		return to_polygon_mode(value);
	}

	static void commit_rsx_state(rsx_state &state, rsx::polygon_mode &&decoded_values)
	{
		state.m_front_polygon_mode = decoded_values;
	}

	static std::string dump(rsx::polygon_mode &&decoded_values)
	{
		return "Front polygon mode: " + print_polygon_mode(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_BACK_POLYGON_MODE>
{
	static auto decode(u32 value)
	{
		return to_polygon_mode(value);
	}

	static void commit_rsx_state(rsx_state &state, rsx::polygon_mode &&decoded_values)
	{
		state.m_back_polygon_mode = decoded_values;
	}

	static std::string dump(rsx::polygon_mode &&decoded_values)
	{
		return "back polygon mode: " + print_polygon_mode(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_TRANSFORM_CONSTANT_LOAD>
	: as_u32<&rsx_state::m_transform_constant_file_pointer>
{
	static std::string dump(u32 &&decoded_values)
	{
		return "Set transform constant pointer at " + std::to_string(decoded_values);
	}
};

#define EXPAND_RANGE_1(index, MACRO) \
	MACRO(index)

#define EXPAND_RANGE_2(index, MACRO) \
	EXPAND_RANGE_1((index), MACRO) \
	EXPAND_RANGE_1((index) + 1, MACRO)

#define EXPAND_RANGE_4(index, MACRO) \
	EXPAND_RANGE_2((index), MACRO) \
	EXPAND_RANGE_2((index) + 2, MACRO)

#define EXPAND_RANGE_8(index, MACRO) \
	EXPAND_RANGE_4((index), MACRO) \
	EXPAND_RANGE_4((index) + 4, MACRO)

#define EXPAND_RANGE_16(index, MACRO) \
	EXPAND_RANGE_8((index), MACRO) \
	EXPAND_RANGE_8((index) + 8, MACRO)

#define EXPAND_RANGE_32(index, MACRO) \
	EXPAND_RANGE_16((index), MACRO) \
	EXPAND_RANGE_16((index) + 16, MACRO)

#define EXPAND_RANGE_64(index, MACRO) \
	EXPAND_RANGE_32((index), MACRO) \
	EXPAND_RANGE_32((index) + 32, MACRO)

#define EXPAND_RANGE_128(index, MACRO) \
	EXPAND_RANGE_64((index), MACRO) \
	EXPAND_RANGE_64((index) + 64, MACRO)

#define EXPAND_RANGE_256(index, MACRO) \
	EXPAND_RANGE_128((index), MACRO) \
	EXPAND_RANGE_128((index) + 128, MACRO)

#define EXPAND_RANGE_512(index, MACRO) \
	EXPAND_RANGE_256((index), MACRO) \
	EXPAND_RANGE_256((index) + 256, MACRO)

template<u32 index>
struct transform_constant_helper
{
	static auto decode(u32 value) {
		return value;
	}

	static constexpr u32 reg = index / 4;
	static constexpr u8 subreg = index % 4;

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		u32 load = state.m_transform_constant_file_pointer;
		state.transform_constants[load + reg].rgba[subreg] = (f32&)decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "TransformConstant[base + " + std::to_string(reg) + "]." + get_subreg_name(subreg) + " = " + std::to_string((f32&)decoded_values);
	}
};

#define TRANSFORM_CONSTANT(index) template<> struct registers_decoder<NV4097_SET_TRANSFORM_CONSTANT + index> : public transform_constant_helper<index> {};
#define DECLARE_TRANSFORM_CONSTANT(index) NV4097_SET_TRANSFORM_CONSTANT + index,

EXPAND_RANGE_32(0, TRANSFORM_CONSTANT)

template<u32 index>
struct transform_program_helper
{
	static auto decode(u32 value) {
		return value;
	}

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		u32 &load = state.m_transform_program_pointer;
		state.transform_program[load++] = decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "Transform Program (" + std::to_string(index) + "):"+ std::to_string(decoded_values);
	}
};

template<>
struct registers_decoder<NV4097_SET_TRANSFORM_PROGRAM_LOAD>
{
	static auto decode(u32 value) {
		return value;
	}

	static void commit_rsx_state(rsx_state &state, u32 &&decoded_values)
	{
		state.m_transform_program_pointer = decoded_values << 2;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "Transform Program pointer :" + std::to_string(decoded_values);
	}
};

#define TRANSFORM_PROGRAM(index) template<> struct registers_decoder<NV4097_SET_TRANSFORM_PROGRAM + index> : public transform_program_helper<index> {};
#define DECLARE_TRANSFORM_PROGRAM(index) NV4097_SET_TRANSFORM_PROGRAM + index,
EXPAND_RANGE_512(0, TRANSFORM_PROGRAM)

template<u32 index>
struct vertex_array_helper
{
	static auto decode(u32 value)
	{
		u16 frequency = value >> 16;
		u8 stride = (value >> 8) & 0xff;
		u8 size = (value >> 4) & 0xf;
		rsx::vertex_base_type type = rsx::to_vertex_base_type(value & 0xf);
		return std::make_tuple(frequency, stride, size, type);
	}

	static void commit_rsx_state(rsx::rsx_state &state, std::tuple<u16, u8, u8, rsx::vertex_base_type> &&decoded_values)
	{
		state.vertex_arrays_info[index].frequency = std::get<0>(decoded_values);
		state.vertex_arrays_info[index].stride = std::get<1>(decoded_values);
		state.vertex_arrays_info[index].size = std::get<2>(decoded_values);
		state.vertex_arrays_info[index].type = std::get<3>(decoded_values);
	}

	static std::string dump(std::tuple<u16, u8, u8, rsx::vertex_base_type> &&decoded_values)
	{
		if (std::get<2>(decoded_values) == 0)
			return "(disabled)";

		return "Vertex array " + std::to_string(index) + ": Type = " + print_vertex_attribute_format(std::get<3>(decoded_values)) +
			" size = " + std::to_string(std::get<2>(decoded_values)) +
			" stride = " + std::to_string(std::get<1>(decoded_values)) +
			" frequency = " + std::to_string(std::get<0>(decoded_values));
	}
};

#define VERTEX_DATA_ARRAY_FORMAT(index) template<> struct registers_decoder<NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + index> : public vertex_array_helper<index> {};
#define DECLARE_VERTEX_DATA_ARRAY_FORMAT(index) NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + index,

EXPAND_RANGE_16(0, VERTEX_DATA_ARRAY_FORMAT)

template<u32 index>
struct vertex_array_offset_helper
{
	static auto decode(u32 value)
	{
		return value;
	}

	static void commit_rsx_state(rsx::rsx_state &state, u32 &&decoded_values)
	{
		state.vertex_arrays_info[index].m_offset = decoded_values;
	}

	static std::string dump(u32 &&decoded_values)
	{
		return "Vertex array " + std::to_string(index) + ": Offset = " + std::to_string(decoded_values);
	}
};

#define VERTEX_DATA_ARRAY_OFFSET(index) template<> struct registers_decoder<NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index> : public vertex_array_offset_helper<index> {};
#define DECLARE_VERTEX_DATA_ARRAY_OFFSET(index) NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index,

EXPAND_RANGE_16(0, VERTEX_DATA_ARRAY_OFFSET)

template<typename Type> struct vertex_data_type_from_element_type;
template<> struct vertex_data_type_from_element_type<float> { static constexpr rsx::vertex_base_type type = rsx::vertex_base_type::f; };
template<> struct vertex_data_type_from_element_type<f16> { static constexpr rsx::vertex_base_type type = rsx::vertex_base_type::sf; };
template<> struct vertex_data_type_from_element_type<u8> { static constexpr rsx::vertex_base_type type = rsx::vertex_base_type::ub; };
template<> struct vertex_data_type_from_element_type<u16> { static constexpr rsx::vertex_base_type type = rsx::vertex_base_type::s1; };

template<typename type, int count>
struct vertex_type_namer;

template<int count>
struct vertex_type_namer<f32, count>
{
	static std::string get()
	{
		return "float" + std::to_string(count);
	}
};

template<int count>
struct vertex_type_namer<u16, count>
{
	static std::string get()
	{
		return "short" + std::to_string(count);
	}
};

template<>
struct vertex_type_namer<u8, 4>
{
	static std::string get()
	{
		return "uchar4";
	}
};

template<u32 index, typename type, int count>
struct register_vertex_helper
{
	static auto decode(u32 value)
	{
		return value;
	}

	static const size_t increment_per_array_index = (count * sizeof(type)) / sizeof(u32);

	static const size_t attribute_index = index / increment_per_array_index;
	static const size_t vertex_subreg = index % increment_per_array_index;

	static void commit_rsx_state(rsx::rsx_state &state, u32 &&decoded_values)
	{
		auto& info = state.register_vertex_info[attribute_index];

		info.type = vertex_data_type_from_element_type<type>::type;
		info.size = count;
		info.frequency = 0;
		info.stride = 0;
		state.register_vertex_info[attribute_index].data[vertex_subreg] = decoded_values;
	}

	static std::string dump(u32&& decoded_values)
	{
		return "register vertex " + std::to_string(attribute_index) + " as " + vertex_type_namer<type, count>::get() + ": " +
			std::to_string(decoded_values);
	}
};

#define VERTEX_DATA4UB(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA4UB_M + index> : public register_vertex_helper<index, u8, 4> {};
#define VERTEX_DATA1F(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA1F_M + index> : public register_vertex_helper<index, f32, 1> {};
#define VERTEX_DATA2F(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA2F_M + index> : public register_vertex_helper<index, f32, 2> {};
#define VERTEX_DATA3F(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA3F_M + index> : public register_vertex_helper<index, f32, 3> {};
#define VERTEX_DATA4F(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA4F_M + index> : public register_vertex_helper<index, f32, 4> {};
#define VERTEX_DATA2S(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA2S_M + index> : public register_vertex_helper<index, u16, 2> {};
#define VERTEX_DATA4S(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_DATA4S_M + index> : public register_vertex_helper<index, u16, 4> {};

#define DECLARE_VERTEX_DATA4UB(index) \
	NV4097_SET_VERTEX_DATA4UB_M + index,
#define DECLARE_VERTEX_DATA1F(index) \
	NV4097_SET_VERTEX_DATA1F_M + index,
#define DECLARE_VERTEX_DATA2F(index) \
	NV4097_SET_VERTEX_DATA2F_M + index,
#define DECLARE_VERTEX_DATA3F(index) \
	NV4097_SET_VERTEX_DATA3F_M + index,
#define DECLARE_VERTEX_DATA4F(index) \
	NV4097_SET_VERTEX_DATA4F_M + index,
#define DECLARE_VERTEX_DATA2S(index) \
	NV4097_SET_VERTEX_DATA2S_M + index,
#define DECLARE_VERTEX_DATA4S(index) \
	NV4097_SET_VERTEX_DATA4S_M + index,

EXPAND_RANGE_16(0, VERTEX_DATA4UB)
EXPAND_RANGE_16(0, VERTEX_DATA1F)
EXPAND_RANGE_16(0, VERTEX_DATA2F)
EXPAND_RANGE_16(0, VERTEX_DATA3F)
EXPAND_RANGE_16(0, VERTEX_DATA4F)
EXPAND_RANGE_16(0, VERTEX_DATA2S)
EXPAND_RANGE_16(0, VERTEX_DATA4S)

template<u32 index>
struct fragment_texture_image_rect_helper : public split_reg_half_uint_decode
{
	static void commit_rsx_state(rsx::rsx_state &state, std::tuple<u16, u16> &&decoded_values)
	{
		state.fragment_textures[index].m_width = std::get<1>(decoded_values);
		state.fragment_textures[index].m_height = std::get<0>(decoded_values);
	}

	static std::string dump(std::tuple<u16, u16>&& decoded_values)
	{
		return "Texture " + std::to_string(index) + ": width = " + std::to_string(std::get<1>(decoded_values)) + " height = " + std::to_string(std::get<0>(decoded_values));
	}
};

template<u32 index>
struct fragment_texture_control3_helper
{
	static std::tuple<u32, u16> decode(u32 value)
	{
		return std::make_tuple(value & 0xfffff, value >> 20);
	}
	static void commit_rsx_state(rsx::rsx_state &state, std::tuple<u32, u16> &&decoded_values)
	{
		state.fragment_textures[index].m_depth = std::get<1>(decoded_values);
		state.fragment_textures[index].m_pitch = std::get<0>(decoded_values);
	}

	static std::string dump(std::tuple<u32, u16>&& decoded_values)
	{
		return "Texture " + std::to_string(index) +
			" depth = " + std::to_string(std::get<1>(decoded_values)) +
			" pitch = " + std::to_string(std::get<0>(decoded_values));
	}
};

template<u32 index>
struct fragment_texture_control0_helper
{
	static auto decode(u32 value)
	{
		return std::make_tuple(!!((value >> 2) & 0x1),
			rsx::texture::to_texture_max_anisotropy((value >> 4) & 0x7),
			static_cast<u16>((value >> 7) & 0xfff),
			static_cast<u16>((value >> 19) & 0xfff),
			!!((value >> 31) & 0x1)
			);
	}
	static void commit_rsx_state(rsx::rsx_state &state, std::tuple<bool, rsx::texture::max_anisotropy, u16, u16, bool> &&decoded_values)
	{
		state.fragment_textures[index].m_alpha_kill_enabled = std::get<0>(decoded_values);
		state.fragment_textures[index].m_max_aniso = std::get<1>(decoded_values);
		state.fragment_textures[index].m_max_lod = std::get<2>(decoded_values);
		state.fragment_textures[index].m_min_lod = std::get<3>(decoded_values);
		state.fragment_textures[index].m_enabled = std::get<4>(decoded_values);
	}

	static std::string get_texture_max_aniso_name(rsx::texture::max_anisotropy aniso)
	{
		switch (aniso)
		{
		case rsx::texture::max_anisotropy::x1: return "1";
		case rsx::texture::max_anisotropy::x2: return "2";
		case rsx::texture::max_anisotropy::x4: return "4";
		case rsx::texture::max_anisotropy::x6: return "6";
		case rsx::texture::max_anisotropy::x8: return "8";
		case rsx::texture::max_anisotropy::x10: return "10";
		case rsx::texture::max_anisotropy::x12: return "12";
		case rsx::texture::max_anisotropy::x16: return "16";
		}
		throw;
	}

	static std::string dump(std::tuple<bool, rsx::texture::max_anisotropy, u16, u16, bool>&& decoded_values)
	{
		std::string result = "Texture " + std::to_string(index);
		if (std::get<4>(decoded_values))
		{
			result += " min lod = " + std::to_string(std::get<3>(decoded_values)) +
				" max lod = " + std::to_string(std::get<2>(decoded_values)) +
				" max aniso = " + get_texture_max_aniso_name(std::get<1>(decoded_values)) +
				" alpha kill = " + print_boolean(std::get<0>(decoded_values));
		}
		else
			result += " (disabled)";
		return result;
	}
};

template<u32 index>
struct fragment_texture_offset_helper
{
	static u32 decode(u32 value)
	{
		return value;
	}
	static void commit_rsx_state(rsx::rsx_state &state, u32 &&decoded_values)
	{
		state.fragment_textures[index].m_offset = decoded_values;
	}

	static std::string dump(u32&& decoded_values)
	{
		return "Texture " + std::to_string(index) + ": Offset @" + std::to_string(decoded_values);
	}
};

template<u32 index>
struct fragment_texture_format_helper
{
	static auto decode(u32 value)
	{
		const auto fmt = rsx::texture::to_texture_format((value >> 8) & 0xff);
		return std::make_tuple(static_cast<u8>((value & 0x3) - 1),
			!!((value >> 2) & 0x1),
			rsx::texture::to_border_type((value >> 3) & 0x1),
			rsx::texture::to_texture_dimension((value >> 4) & 0xf),
			std::get<0>(fmt),
			std::get<1>(fmt),
			std::get<2>(fmt),
			static_cast<u16>(value >> 16)
		);
	}
	static void commit_rsx_state(rsx::rsx_state &state, std::tuple<u8, bool, rsx::texture::border_type, rsx::texture::dimension, rsx::texture::format, rsx::texture::layout, rsx::texture::coordinates, u16> &&decoded_values)
	{
		state.fragment_textures[index].m_location = std::get<0>(decoded_values);
		state.fragment_textures[index].m_cubemap = std::get<1>(decoded_values);
		state.fragment_textures[index].m_border_type = std::get<2>(decoded_values);
		state.fragment_textures[index].m_dimension = std::get<3>(decoded_values);
		state.fragment_textures[index].m_format = std::get<4>(decoded_values);
		state.fragment_textures[index].m_layout = std::get<5>(decoded_values);
		state.fragment_textures[index].m_normalization = std::get<6>(decoded_values);
		state.fragment_textures[index].m_mipmap = std::get<7>(decoded_values);
	}

	static std::string dump(std::tuple<u8, bool, rsx::texture::border_type, rsx::texture::dimension, rsx::texture::format, rsx::texture::layout, rsx::texture::coordinates, u16>&& decoded_values)
	{
		return "Texture " + std::to_string(index) + ": location = " + std::to_string(std::get<0>(decoded_values)) +
			(std::get<1>(decoded_values) ? " cubemap " : "") +
			" border type = " + to_string(std::get<2>(decoded_values)) +
			" dimension = " + to_string(std::get<3>(decoded_values)) +
			" format = " + to_string(std::get<4>(decoded_values)) +
			to_string(std::get<5>(decoded_values)) + to_string(std::get<6>(decoded_values)) +
			" mipmap levels = " + std::to_string(std::get<7>(decoded_values));
	}
};


template<u32 index>
struct fragment_texture_address_helper
{
	static auto decode(u32 value)
	{
		return std::make_tuple(
			rsx::texture::to_texture_wrap_mode(value & 0xf),
			static_cast<u8>((value >> 4) & 0xf),
			rsx::texture::to_texture_wrap_mode((value >> 8) & 0xf),
			rsx::texture::to_unsigned_remap((value >> 12) & 0xf),
			rsx::texture::to_texture_wrap_mode((value >> 16) & 0xf),
			!!((value >> 20) & 0x1),
			!!((value >> 21) & 0x1),
			!!((value >> 22) & 0x1),
			!!((value >> 23) & 0x1),
			rsx::texture::to_signed_remap((value >> 24) & 0xf),
			rsx::texture::to_texture_zfunc((value >> 28) & 0xf)
		);
	}
	static void commit_rsx_state(rsx::rsx_state &state,
		std::tuple<rsx::texture::wrap_mode, u8, rsx::texture::wrap_mode, rsx::texture::unsigned_remap, rsx::texture::wrap_mode, bool, bool, bool, bool, rsx::texture::signed_remap, rsx::texture::zfunc> &&decoded_values)
	{
		state.fragment_textures[index].m_wrap_s = std::get<0>(decoded_values);
		state.fragment_textures[index].m_aniso_bias = std::get<1>(decoded_values);
		state.fragment_textures[index].m_wrap_t = std::get<2>(decoded_values);
		state.fragment_textures[index].m_unsigned_remap = std::get<3>(decoded_values);
		state.fragment_textures[index].m_wrap_r = std::get<4>(decoded_values);
		state.fragment_textures[index].m_gamma_r = std::get<5>(decoded_values);
		state.fragment_textures[index].m_gamma_g = std::get<6>(decoded_values);
		state.fragment_textures[index].m_gamma_b = std::get<7>(decoded_values);
		state.fragment_textures[index].m_gamma_a = std::get<8>(decoded_values);
		state.fragment_textures[index].m_signed_remap = std::get<9>(decoded_values);
		state.fragment_textures[index].m_zfunc = std::get<10>(decoded_values);
	}

	static std::string dump(
		std::tuple<rsx::texture::wrap_mode, u8, rsx::texture::wrap_mode, rsx::texture::unsigned_remap, rsx::texture::wrap_mode, bool, bool, bool, bool, rsx::texture::signed_remap, rsx::texture::zfunc>&& decoded_values)
	{
		return "Texture " + std::to_string(index) + ": wrap_s = " + to_string(std::get<0>(decoded_values)) +
			" wrap_t = " + to_string(std::get<2>(decoded_values)) +
			" wrap_r = " + to_string(std::get<4>(decoded_values)) +
			" unsigned remap = " + to_string(std::get<3>(decoded_values)) +
			" zfunc = " + to_string(std::get<10>(decoded_values)) +
			" gamma r = " + print_boolean(std::get<5>(decoded_values)) +
			" g = " + print_boolean(std::get<6>(decoded_values)) +
			" b = " + print_boolean(std::get<7>(decoded_values)) +
			" a = " + print_boolean(std::get<8>(decoded_values)) +
			" aniso bias = " + std::to_string(std::get<1>(decoded_values)) +
			" signed remap = " + to_string(std::get<9>(decoded_values));
	}
};

template<u32 index>
struct fragment_texture_filter_helper
{
	static auto decode(u32 value)
	{
		return std::make_tuple(
			static_cast<u16>(value & 0x1fff),
			static_cast<u8>((value >> 13) & 0x7),
			rsx::texture::to_texture_minify_filter((value >> 16) & 0x7),
			rsx::texture::to_texture_magnify_filter((value >> 24) & 0x7),
			!!((value >> 28) & 0x1),
			!!((value >> 29) & 0x1),
			!!((value >> 30) & 0x1),
			!!((value >> 31) & 0x1)
		);
	}
	static void commit_rsx_state(rsx::rsx_state &state,
		std::tuple<u16, u8, rsx::texture::minify_filter, rsx::texture::magnify_filter, bool, bool, bool, bool> &&decoded_values)
	{
		state.fragment_textures[index].m_bias = std::get<0>(decoded_values);
		state.fragment_textures[index].m_convolution_filter = std::get<1>(decoded_values);
		state.fragment_textures[index].m_min_filter = std::get<2>(decoded_values);
		state.fragment_textures[index].m_mag_filter = std::get<3>(decoded_values);
		state.fragment_textures[index].m_a_signed = std::get<4>(decoded_values);
		state.fragment_textures[index].m_r_signed = std::get<5>(decoded_values);
		state.fragment_textures[index].m_g_signed = std::get<6>(decoded_values);
		state.fragment_textures[index].m_b_signed = std::get<7>(decoded_values);
	}

	static std::string dump(
		std::tuple<u16, u8, rsx::texture::minify_filter, rsx::texture::magnify_filter, bool, bool, bool, bool>&& decoded_values)
	{
		return "Texture " + std::to_string(index) +
			" bias = " + std::to_string(std::get<0>(decoded_values)) +
			" min_filter = " + to_string(std::get<2>(decoded_values)) +
			" mag_filter = " + to_string(std::get<3>(decoded_values)) +
			" convolution_filter = " + std::to_string(std::get<1>(decoded_values)) + "(???)" +
			" a_signed = " + std::to_string(std::get<4>(decoded_values)) +
			" r_signed = " + std::to_string(std::get<5>(decoded_values)) +
			" g_signed = " + std::to_string(std::get<6>(decoded_values)) +
			" b_signed = " + std::to_string(std::get<7>(decoded_values));
	}
};


template<u32 index>
struct fragment_texture_border_color_helper
{
	static auto decode(u32 value)
	{
		return std::make_tuple(value & 0xff,
			(value >> 8) & 0xff,
			(value >> 16) & 0xff,
			(value >> 24) & 0xff
		);
	}
	static void commit_rsx_state(rsx::rsx_state &state,
		std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		state.fragment_textures[index].m_border_color_a = std::get<0>(decoded_values);
		state.fragment_textures[index].m_border_color_r = std::get<1>(decoded_values);
		state.fragment_textures[index].m_border_color_g = std::get<2>(decoded_values);
		state.fragment_textures[index].m_border_color_b = std::get<3>(decoded_values);
	}

	static std::string dump(
		std::tuple<u8, u8, u8, u8>&& decoded_values)
	{
		return "Texture " + std::to_string(index) +
			" border color A = " + std::to_string(std::get<0>(decoded_values)) +
			" R = " + std::to_string(std::get<1>(decoded_values)) +
			" G = " + std::to_string(std::get<2>(decoded_values)) +
			" B = " + std::to_string(std::get<3>(decoded_values));
	}
};

template<u32 index>
struct fragment_texture_control1_helper
{
	static auto decode(u32 value)
	{
		return std::make_tuple(rsx::texture::to_component_remap(value & 0x3),
			rsx::texture::to_component_remap((value >> 2) & 0x3),
			rsx::texture::to_component_remap((value >> 4) & 0x3),
			rsx::texture::to_component_remap((value >> 6) & 0x3)
		);
	}
	static void commit_rsx_state(rsx::rsx_state &state,
		std::tuple<rsx::texture::component_remap, rsx::texture::component_remap, rsx::texture::component_remap, rsx::texture::component_remap> &&decoded_values)
	{
		state.fragment_textures[index].m_remap_0 = std::get<0>(decoded_values);
		state.fragment_textures[index].m_remap_1 = std::get<1>(decoded_values);
		state.fragment_textures[index].m_remap_2 = std::get<2>(decoded_values);
		state.fragment_textures[index].m_remap_3 = std::get<3>(decoded_values);
	}

	static std::string dump(
		std::tuple<rsx::texture::component_remap, rsx::texture::component_remap, rsx::texture::component_remap, rsx::texture::component_remap> && decoded_values)
	{
		return "Texture " + std::to_string(index) +
			" Component 0 = " + to_string(std::get<0>(decoded_values)) +
			" Component 1 = " + to_string(std::get<1>(decoded_values)) +
			" Component 2 = " + to_string(std::get<2>(decoded_values)) +
			" Component 3 = " + to_string(std::get<3>(decoded_values));
	}
};

#define TEXTURE_IMAGE_RECT(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_IMAGE_RECT + 8 * (index)> : public fragment_texture_image_rect_helper<index> {};
#define DECLARE_TEXTURE_IMAGE_RECT(index) \
	NV4097_SET_TEXTURE_IMAGE_RECT + 8 * (index),

#define TEXTURE_CONTROL3(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_CONTROL3 + index> : public fragment_texture_control3_helper<index> {};
#define DECLARE_TEXTURE_CONTROL3(index) \
	NV4097_SET_TEXTURE_CONTROL3 + index,

#define TEXTURE_CONTROL0(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_CONTROL0 + 8 * (index)> : public fragment_texture_control0_helper<index> {};
#define DECLARE_TEXTURE_CONTROL0(index) \
	NV4097_SET_TEXTURE_CONTROL0 + 8 * (index),

#define TEXTURE_OFFSET(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_OFFSET + 8 * (index)> : public fragment_texture_offset_helper<index> {};
#define DECLARE_TEXTURE_OFFSET(index) \
	NV4097_SET_TEXTURE_OFFSET + 8 * (index),

#define TEXTURE_FORMAT(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_FORMAT + 8 * (index)> : public fragment_texture_format_helper<index> {};
#define DECLARE_TEXTURE_FORMAT(index) \
	NV4097_SET_TEXTURE_FORMAT + 8 * (index),

#define TEXTURE_ADDRESS(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_ADDRESS + 8 * (index)> : public fragment_texture_address_helper<index> {};
#define DECLARE_TEXTURE_ADDRESS(index) \
	NV4097_SET_TEXTURE_ADDRESS + 8 * (index),

#define TEXTURE_FILTER(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_FILTER + 8 * (index)> : public fragment_texture_filter_helper<index> {};
#define DECLARE_TEXTURE_FILTER(index) \
	NV4097_SET_TEXTURE_FILTER + 8 * (index),

#define TEXTURE_BORDER_COLOR(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_BORDER_COLOR + 8 * (index)> : public fragment_texture_border_color_helper<index> {};
#define DECLARE_TEXTURE_BORDER_COLOR(index) \
	NV4097_SET_TEXTURE_BORDER_COLOR + 8 * (index),

#define TEXTURE_CONTROL1(index) \
	template<> struct registers_decoder<NV4097_SET_TEXTURE_CONTROL1 + 8 * (index)> : public fragment_texture_control1_helper<index> {};
#define DECLARE_TEXTURE_CONTROL1(index) \
	NV4097_SET_TEXTURE_CONTROL1 + 8 * (index),


EXPAND_RANGE_16(0, TEXTURE_IMAGE_RECT)
EXPAND_RANGE_16(0, TEXTURE_OFFSET)
EXPAND_RANGE_16(0, TEXTURE_CONTROL0)
EXPAND_RANGE_16(0, TEXTURE_CONTROL3)
EXPAND_RANGE_16(0, TEXTURE_FORMAT)
EXPAND_RANGE_16(0, TEXTURE_ADDRESS)
EXPAND_RANGE_16(0, TEXTURE_FILTER)
EXPAND_RANGE_16(0, TEXTURE_BORDER_COLOR)
EXPAND_RANGE_16(0, TEXTURE_CONTROL1)


template<u32 index>
struct vertex_texture_offset_helper
{
	static u32 decode(u32 value)
	{
		return value;
	}
	static void commit_rsx_state(rsx::rsx_state &state, u32 &&decoded_values)
	{
		state.vertex_textures[index].m_offset = decoded_values;
	}

	static std::string dump(u32&& decoded_values)
	{
		return "Vertex texture " + std::to_string(index) + ": Offset @" + std::to_string(decoded_values);
	}
};


template<u32 index>
struct vertex_texture_format_helper
{
	static auto decode(u32 value)
	{
		const auto fmt = rsx::texture::to_texture_format((value >> 8) & 0xff);
		return std::make_tuple(static_cast<u8>((value & 0x3) - 1),
			!!((value >> 2) & 0x1),
			rsx::texture::to_border_type((value >> 3) & 0x1),
			rsx::texture::to_texture_dimension((value >> 4) & 0xf),
			std::get<0>(fmt),
			std::get<1>(fmt),
			std::get<2>(fmt),
			static_cast<u16>(value >> 16)
		);
	}
	static void commit_rsx_state(rsx::rsx_state &state, std::tuple<u8, bool, rsx::texture::border_type, rsx::texture::dimension, rsx::texture::format, rsx::texture::layout, rsx::texture::coordinates, u16> &&decoded_values)
	{
		state.vertex_textures[index].m_location = std::get<0>(decoded_values);
		state.vertex_textures[index].m_cubemap = std::get<1>(decoded_values);
		state.vertex_textures[index].m_border_type = std::get<2>(decoded_values);
		state.vertex_textures[index].m_dimension = std::get<3>(decoded_values);
		state.vertex_textures[index].m_format = std::get<4>(decoded_values);
		state.vertex_textures[index].m_layout = std::get<5>(decoded_values);
//		state.vertex_textures[index].m_normalization = std::get<6>(decoded_values);
		state.vertex_textures[index].m_mipmap = std::get<7>(decoded_values);
	}

	static std::string dump(std::tuple<u8, bool, rsx::texture::border_type, rsx::texture::dimension, rsx::texture::format, rsx::texture::layout, rsx::texture::coordinates, u16>&& decoded_values)
	{
		return "Vertex texture " + std::to_string(index) + ": location = " + std::to_string(std::get<0>(decoded_values)) +
			(std::get<1>(decoded_values) ? " cubemap " : "") +
			" border type = " + to_string(std::get<2>(decoded_values)) +
			" dimension = " + to_string(std::get<3>(decoded_values)) +
			" format = " + to_string(std::get<4>(decoded_values)) +
			to_string(std::get<5>(decoded_values)) + to_string(std::get<6>(decoded_values)) +
			" mipmap levels = " + std::to_string(std::get<7>(decoded_values));
	}
};

template<u32 index>
struct vertex_texture_address_helper
{
	static auto decode(u32 value)
	{
		return std::make_tuple(
			static_cast<u8>((value >> 4) & 0xf),
			rsx::texture::to_unsigned_remap((value >> 12) & 0xf),
			!!((value >> 20) & 0x1),
			!!((value >> 21) & 0x1),
			!!((value >> 22) & 0x1),
			!!((value >> 23) & 0x1),
			rsx::texture::to_signed_remap((value >> 24) & 0xf),
			rsx::texture::to_texture_zfunc((value >> 28) & 0xf)
		);
	}
	static void commit_rsx_state(rsx::rsx_state &state,
		std::tuple<u8,rsx::texture::unsigned_remap, bool, bool, bool, bool, rsx::texture::signed_remap, rsx::texture::zfunc> &&decoded_values)
	{
		state.vertex_textures[index].m_aniso_bias = std::get<0>(decoded_values);
		state.vertex_textures[index].m_unsigned_remap = std::get<1>(decoded_values);
		state.vertex_textures[index].m_gamma_r = std::get<2>(decoded_values);
		state.vertex_textures[index].m_gamma_g = std::get<3>(decoded_values);
		state.vertex_textures[index].m_gamma_b = std::get<4>(decoded_values);
		state.vertex_textures[index].m_gamma_a = std::get<5>(decoded_values);
		state.vertex_textures[index].m_signed_remap = std::get<6>(decoded_values);
		state.vertex_textures[index].m_zfunc = std::get<7>(decoded_values);
	}

	static std::string dump(
		std::tuple<u8, rsx::texture::unsigned_remap, bool, bool, bool, bool, rsx::texture::signed_remap, rsx::texture::zfunc>&& decoded_values)
	{
		return "Vertex texture " + std::to_string(index) + ": unsigned remap = " + to_string(std::get<1>(decoded_values)) +
			" zfunc = " + to_string(std::get<7>(decoded_values)) +
			" gamma r = " + print_boolean(std::get<2>(decoded_values)) +
			" g = " + print_boolean(std::get<3>(decoded_values)) +
			" b = " + print_boolean(std::get<4>(decoded_values)) +
			" a = " + print_boolean(std::get<5>(decoded_values)) +
			" aniso bias = " + std::to_string(std::get<0>(decoded_values)) +
			" signed remap = " + to_string(std::get<6>(decoded_values));
	}
};

template<u32 index>
struct vertex_texture_control0_helper
{
	static auto decode(u32 value)
	{
		return std::make_tuple(!!((value >> 2) & 0x1),
			rsx::texture::to_texture_max_anisotropy((value >> 4) & 0x7),
			static_cast<u16>((value >> 7) & 0xfff),
			static_cast<u16>((value >> 19) & 0xfff),
			!!((value >> 31) & 0x1)
		);
	}
	static void commit_rsx_state(rsx::rsx_state &state, std::tuple<bool, rsx::texture::max_anisotropy, u16, u16, bool> &&decoded_values)
	{
		state.vertex_textures[index].m_alpha_kill_enabled = std::get<0>(decoded_values);
		state.vertex_textures[index].m_max_aniso = std::get<1>(decoded_values);
		state.vertex_textures[index].m_max_lod = std::get<2>(decoded_values);
		state.vertex_textures[index].m_min_lod = std::get<3>(decoded_values);
		state.vertex_textures[index].m_enabled = std::get<4>(decoded_values);
	}

	static std::string get_texture_max_aniso_name(rsx::texture::max_anisotropy aniso)
	{
		switch (aniso)
		{
		case rsx::texture::max_anisotropy::x1: return "1";
		case rsx::texture::max_anisotropy::x2: return "2";
		case rsx::texture::max_anisotropy::x4: return "4";
		case rsx::texture::max_anisotropy::x6: return "6";
		case rsx::texture::max_anisotropy::x8: return "8";
		case rsx::texture::max_anisotropy::x10: return "10";
		case rsx::texture::max_anisotropy::x12: return "12";
		case rsx::texture::max_anisotropy::x16: return "16";
		}
		throw;
	}

	static std::string dump(std::tuple<bool, rsx::texture::max_anisotropy, u16, u16, bool>&& decoded_values)
	{
		std::string result = "Vertex texture " + std::to_string(index);
		if (std::get<4>(decoded_values))
		{
			result += " min lod = " + std::to_string(std::get<3>(decoded_values)) +
				" max lod = " + std::to_string(std::get<2>(decoded_values)) +
				" max aniso = " + get_texture_max_aniso_name(std::get<1>(decoded_values)) +
				" alpha kill = " + print_boolean(std::get<0>(decoded_values));
		}
		else
			result += " (disabled)";
		return result;
	}
};

template<u32 index>
struct vertex_texture_filter_helper
{
	static auto decode(u32 value)
	{
		return std::make_tuple(
			static_cast<u16>(value & 0x1fff),
			static_cast<u8>((value >> 13) & 0x7),
			rsx::texture::to_texture_minify_filter((value >> 16) & 0x7),
			rsx::texture::to_texture_magnify_filter((value >> 24) & 0x7),
			!!((value >> 28) & 0x1),
			!!((value >> 29) & 0x1),
			!!((value >> 30) & 0x1),
			!!((value >> 31) & 0x1)
		);
	}
	static void commit_rsx_state(rsx::rsx_state &state,
		std::tuple<u16, u8, rsx::texture::minify_filter, rsx::texture::magnify_filter, bool, bool, bool, bool> &&decoded_values)
	{
		state.vertex_textures[index].m_bias = std::get<0>(decoded_values);
		state.vertex_textures[index].m_convolution_filter = std::get<1>(decoded_values);
		state.vertex_textures[index].m_min_filter = std::get<2>(decoded_values);
		state.vertex_textures[index].m_mag_filter = std::get<3>(decoded_values);
		state.vertex_textures[index].m_a_signed = std::get<4>(decoded_values);
		state.vertex_textures[index].m_r_signed = std::get<5>(decoded_values);
		state.vertex_textures[index].m_g_signed = std::get<6>(decoded_values);
		state.vertex_textures[index].m_b_signed = std::get<7>(decoded_values);
	}

	static std::string dump(
		std::tuple<u16, u8, rsx::texture::minify_filter, rsx::texture::magnify_filter, bool, bool, bool, bool>&& decoded_values)
	{
		return "Vertex texture " + std::to_string(index) +
			" bias = " + std::to_string(std::get<0>(decoded_values)) +
			" min_filter = " + to_string(std::get<2>(decoded_values)) +
			" mag_filter = " + to_string(std::get<3>(decoded_values)) +
			" convolution_filter = " + std::to_string(std::get<1>(decoded_values)) + "(???)" +
			" a_signed = " + std::to_string(std::get<4>(decoded_values)) +
			" r_signed = " + std::to_string(std::get<5>(decoded_values)) +
			" g_signed = " + std::to_string(std::get<6>(decoded_values)) +
			" b_signed = " + std::to_string(std::get<7>(decoded_values));
	}
};

template<u32 index>
struct vertex_texture_image_rect_helper : public split_reg_half_uint_decode
{
	static void commit_rsx_state(rsx::rsx_state &state, std::tuple<u16, u16> &&decoded_values)
	{
		state.vertex_textures[index].m_width = std::get<1>(decoded_values);
		state.vertex_textures[index].m_height = std::get<0>(decoded_values);
	}

	static std::string dump(std::tuple<u16, u16>&& decoded_values)
	{
		return "Vertex texture " + std::to_string(index) + ": width = " + std::to_string(std::get<1>(decoded_values)) + " height = " + std::to_string(std::get<0>(decoded_values));
	}
};

template<u32 index>
struct vertex_texture_border_color_helper
{
	static auto decode(u32 value)
	{
		return std::make_tuple(value & 0xff,
			(value >> 8) & 0xff,
			(value >> 16) & 0xff,
			(value >> 24) & 0xff
		);
	}
	static void commit_rsx_state(rsx::rsx_state &state,
		std::tuple<u8, u8, u8, u8> &&decoded_values)
	{
		state.vertex_textures[index].m_border_color_a = std::get<0>(decoded_values);
		state.vertex_textures[index].m_border_color_r = std::get<1>(decoded_values);
		state.vertex_textures[index].m_border_color_g = std::get<2>(decoded_values);
		state.vertex_textures[index].m_border_color_b = std::get<3>(decoded_values);
	}

	static std::string dump(
		std::tuple<u8, u8, u8, u8>&& decoded_values)
	{
		return "Vertex texture " + std::to_string(index) +
			" border color A = " + std::to_string(std::get<0>(decoded_values)) +
			" R = " + std::to_string(std::get<1>(decoded_values)) +
			" G = " + std::to_string(std::get<2>(decoded_values)) +
			" B = " + std::to_string(std::get<3>(decoded_values));
	}
};

template<u32 index>
struct vertex_texture_control3_helper
{
	static std::tuple<u32, u16> decode(u32 value)
	{
		return std::make_tuple(value & 0xfffff, value >> 20);
	}
	static void commit_rsx_state(rsx::rsx_state &state, std::tuple<u32, u16> &&decoded_values)
	{
		state.vertex_textures[index].m_depth = std::get<1>(decoded_values);
		state.vertex_textures[index].m_pitch = std::get<0>(decoded_values);
	}

	static std::string dump(std::tuple<u32, u16>&& decoded_values)
	{
		return "Vertex texture " + std::to_string(index) +
			" depth = " + std::to_string(std::get<1>(decoded_values)) +
			" pitch = " + std::to_string(std::get<0>(decoded_values));
	}
};


#define VERTEX_TEXTURE_OFFSET(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_TEXTURE_OFFSET + 8 * (index)> : public vertex_texture_offset_helper<index> {};
#define DECLARE_VERTEX_TEXTURE_OFFSET(index) \
	NV4097_SET_VERTEX_TEXTURE_OFFSET + 8 * (index),
#define VERTEX_TEXTURE_FORMAT(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_TEXTURE_FORMAT + 8 * (index)> : public vertex_texture_format_helper<index> {};
#define DECLARE_VERTEX_TEXTURE_FORMAT(index) \
	NV4097_SET_VERTEX_TEXTURE_FORMAT + 8 * (index),
#define VERTEX_TEXTURE_ADDRESS(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_TEXTURE_ADDRESS + 8 * (index)> : public vertex_texture_address_helper<index> {};
#define DECLARE_VERTEX_TEXTURE_ADDRESS(index) \
	NV4097_SET_VERTEX_TEXTURE_ADDRESS + 8 * (index),
#define VERTEX_TEXTURE_CONTROL0(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 8 * (index)> : public vertex_texture_control0_helper<index> {};
#define DECLARE_VERTEX_TEXTURE_CONTROL0(index) \
	NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 8 * (index),
#define VERTEX_TEXTURE_FILTER(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_TEXTURE_FILTER + 8 * (index)> : public vertex_texture_filter_helper<index> {};
#define DECLARE_VERTEX_TEXTURE_FILTER(index) \
	NV4097_SET_VERTEX_TEXTURE_FILTER + 8 * (index),
#define VERTEX_TEXTURE_IMAGE_RECT(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + 8 * (index)> : public vertex_texture_image_rect_helper<index> {};
#define DECLARE_VERTEX_TEXTURE_IMAGE_RECT(index) \
	NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + 8 * (index),
#define VERTEX_TEXTURE_BORDER_COLOR(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 8 * (index)> : public vertex_texture_border_color_helper<index> {};
#define DECLARE_VERTEX_TEXTURE_BORDER_COLOR(index) \
	NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 8 * (index),
#define VERTEX_TEXTURE_CONTROL3(index) \
	template<> struct registers_decoder<NV4097_SET_VERTEX_TEXTURE_CONTROL3 + 8 * (index)> : public vertex_texture_control3_helper<index> {};
#define DECLARE_VERTEX_TEXTURE_CONTROL3(index) \
	NV4097_SET_VERTEX_TEXTURE_CONTROL3 + 8 * (index),


EXPAND_RANGE_4(0, VERTEX_TEXTURE_OFFSET)
EXPAND_RANGE_4(0, VERTEX_TEXTURE_FORMAT)
EXPAND_RANGE_4(0, VERTEX_TEXTURE_ADDRESS)
EXPAND_RANGE_4(0, VERTEX_TEXTURE_CONTROL0)
EXPAND_RANGE_4(0, VERTEX_TEXTURE_FILTER)
EXPAND_RANGE_4(0, VERTEX_TEXTURE_IMAGE_RECT)
EXPAND_RANGE_4(0, VERTEX_TEXTURE_BORDER_COLOR)
EXPAND_RANGE_4(0, VERTEX_TEXTURE_CONTROL3)

constexpr std::integer_sequence<u32,
	NV4097_SET_VIEWPORT_HORIZONTAL,
	NV4097_SET_VIEWPORT_VERTICAL,
	NV4097_SET_SCISSOR_HORIZONTAL,
	NV4097_SET_SCISSOR_VERTICAL,
	NV4097_SET_SURFACE_CLIP_HORIZONTAL,
	NV4097_SET_SURFACE_CLIP_VERTICAL,
	NV4097_SET_CLEAR_RECT_HORIZONTAL,
	NV4097_SET_CLEAR_RECT_VERTICAL,
	NV3089_CLIP_POINT,
	NV3089_CLIP_SIZE,
	NV3089_IMAGE_OUT_POINT,
	NV3089_IMAGE_OUT_SIZE,
	NV3089_IMAGE_IN_SIZE,
	NV3062_SET_PITCH,
	NV308A_POINT,
	NV4097_SET_DEPTH_TEST_ENABLE,
	NV4097_SET_DEPTH_MASK,
	NV4097_SET_ALPHA_TEST_ENABLE,
	NV4097_SET_STENCIL_TEST_ENABLE,
	NV4097_SET_RESTART_INDEX_ENABLE,
	NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE,
	NV4097_SET_LOGIC_OP_ENABLE,
	NV4097_SET_DITHER_ENABLE,
	NV4097_SET_BLEND_ENABLE,
	NV4097_SET_LINE_SMOOTH_ENABLE,
	NV4097_SET_POLY_OFFSET_POINT_ENABLE,
	NV4097_SET_POLY_OFFSET_LINE_ENABLE,
	NV4097_SET_POLY_OFFSET_FILL_ENABLE,
	NV4097_SET_CULL_FACE_ENABLE,
	NV4097_SET_POLY_SMOOTH_ENABLE,
	NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE,
	NV4097_SET_TWO_SIDE_LIGHT_EN,
	NV4097_SET_RESTART_INDEX,
	NV4097_SET_SURFACE_COLOR_AOFFSET,
	NV4097_SET_SURFACE_COLOR_BOFFSET,
	NV4097_SET_SURFACE_COLOR_COFFSET,
	NV4097_SET_SURFACE_COLOR_DOFFSET,
	NV4097_SET_SURFACE_PITCH_A,
	NV4097_SET_SURFACE_PITCH_B,
	NV4097_SET_SURFACE_PITCH_C,
	NV4097_SET_SURFACE_PITCH_D,
	NV4097_SET_SURFACE_ZETA_OFFSET,
	NV4097_SET_SURFACE_PITCH_Z,
	NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK,
	NV4097_SET_SHADER_CONTROL,
	NV4097_SET_VERTEX_DATA_BASE_OFFSET,
	NV4097_SET_INDEX_ARRAY_ADDRESS,
	NV4097_SET_VERTEX_DATA_BASE_INDEX,
	NV4097_SET_SHADER_PROGRAM,
	NV4097_SET_TRANSFORM_PROGRAM_START,
	NV406E_SEMAPHORE_OFFSET,
	NV4097_SET_SEMAPHORE_OFFSET,
	NV3089_IMAGE_IN_OFFSET,
	NV3062_SET_OFFSET_DESTIN,
	NV309E_SET_OFFSET,
	NV3089_DS_DX,
	NV3089_DT_DY,
	NV0039_PITCH_IN,
	NV0039_PITCH_OUT,
	NV0039_LINE_LENGTH_IN,
	NV0039_LINE_COUNT,
	NV0039_OFFSET_OUT,
	NV0039_OFFSET_IN,
	NV4097_SET_VERTEX_ATTRIB_INPUT_MASK,
	NV4097_SET_FREQUENCY_DIVIDER_OPERATION,
	NV4097_SET_DEPTH_BOUNDS_MIN,
	NV4097_SET_DEPTH_BOUNDS_MAX,
	NV4097_SET_FOG_PARAMS,
	NV4097_SET_FOG_PARAMS + 1,
	NV4097_SET_CLIP_MIN,
	NV4097_SET_CLIP_MAX,
	NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR,
	NV4097_SET_POLYGON_OFFSET_BIAS,
	NV4097_SET_VIEWPORT_SCALE,
	NV4097_SET_VIEWPORT_SCALE + 1,
	NV4097_SET_VIEWPORT_SCALE + 2,
	NV4097_SET_VIEWPORT_SCALE + 3,
	NV4097_SET_VIEWPORT_OFFSET,
	NV4097_SET_VIEWPORT_OFFSET + 1,
	NV4097_SET_VIEWPORT_OFFSET + 2,
	NV4097_SET_VIEWPORT_OFFSET + 3,
	NV4097_SET_DEPTH_FUNC,
	NV4097_SET_STENCIL_FUNC,
	NV4097_SET_BACK_STENCIL_FUNC,
	NV4097_SET_STENCIL_OP_FAIL,
	NV4097_SET_STENCIL_OP_ZFAIL,
	NV4097_SET_STENCIL_OP_ZPASS,
	NV4097_SET_BACK_STENCIL_OP_FAIL,
	NV4097_SET_BACK_STENCIL_OP_ZFAIL,
	NV4097_SET_BACK_STENCIL_OP_ZPASS,
	NV4097_SET_LOGIC_OP,
	NV4097_SET_FRONT_FACE,
	NV4097_SET_CULL_FACE,
	NV4097_SET_SURFACE_COLOR_TARGET,
	NV4097_SET_FOG_MODE,
	NV4097_SET_ALPHA_FUNC,
	NV4097_SET_BEGIN_END,
	NV3089_SET_OPERATION,
	NV3089_SET_COLOR_FORMAT,
	NV3089_SET_CONTEXT_SURFACE,
	NV3062_SET_COLOR_FORMAT,
	NV4097_SET_STENCIL_FUNC_REF,
	NV4097_SET_BACK_STENCIL_FUNC_REF,
	NV4097_SET_STENCIL_FUNC_MASK,
	NV4097_SET_BACK_STENCIL_FUNC_MASK,
	NV4097_SET_ALPHA_REF,
	NV4097_SET_COLOR_CLEAR_VALUE,
	NV4097_SET_STENCIL_MASK,
	NV4097_SET_BACK_STENCIL_MASK,
	NV4097_SET_BLEND_EQUATION,
	NV4097_SET_BLEND_FUNC_SFACTOR,
	NV4097_SET_BLEND_FUNC_DFACTOR,
	NV4097_SET_COLOR_MASK,
	NV4097_SET_SHADER_WINDOW,
	NV4097_SET_BLEND_ENABLE_MRT,
	NV4097_SET_USER_CLIP_PLANE_CONTROL,
	NV4097_SET_LINE_WIDTH,
	NV4097_SET_SURFACE_FORMAT,
	NV4097_SET_WINDOW_OFFSET,
	NV4097_SET_ZSTENCIL_CLEAR_VALUE,
	NV4097_SET_INDEX_ARRAY_DMA,
	NV4097_SET_CONTEXT_DMA_COLOR_A,
	NV4097_SET_CONTEXT_DMA_COLOR_B,
	NV4097_SET_CONTEXT_DMA_COLOR_C,
	NV4097_SET_CONTEXT_DMA_COLOR_D,
	NV4097_SET_CONTEXT_DMA_ZETA,
	NV3089_SET_CONTEXT_DMA_IMAGE,
	NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN,
	NV309E_SET_CONTEXT_DMA_IMAGE,
	NV0039_SET_CONTEXT_DMA_BUFFER_OUT,
	NV0039_SET_CONTEXT_DMA_BUFFER_IN,
	NV4097_SET_CONTEXT_DMA_REPORT,
	NV3089_IMAGE_IN_FORMAT,
	NV309E_SET_FORMAT,
	NV0039_FORMAT,
	NV4097_SET_BLEND_COLOR2,
	NV4097_SET_BLEND_COLOR,
	NV3089_IMAGE_IN,
	NV4097_NO_OPERATION,
	NV4097_INVALIDATE_VERTEX_CACHE_FILE,
	NV4097_INVALIDATE_VERTEX_FILE,
	NV4097_SET_ANTI_ALIASING_CONTROL,
	NV4097_SET_FRONT_POLYGON_MODE,
	NV4097_SET_BACK_POLYGON_MODE,
	EXPAND_RANGE_16(0, DECLARE_VERTEX_DATA_ARRAY_FORMAT)
	EXPAND_RANGE_16(0, DECLARE_VERTEX_DATA_ARRAY_OFFSET)
	EXPAND_RANGE_16(0, DECLARE_VERTEX_DATA4UB)
	EXPAND_RANGE_16(0, DECLARE_VERTEX_DATA1F)
	EXPAND_RANGE_16(0, DECLARE_VERTEX_DATA2F)
	EXPAND_RANGE_16(0, DECLARE_VERTEX_DATA3F)
	EXPAND_RANGE_16(0, DECLARE_VERTEX_DATA4F)
	EXPAND_RANGE_16(0, DECLARE_VERTEX_DATA2S)
	EXPAND_RANGE_16(0, DECLARE_VERTEX_DATA4S)
	EXPAND_RANGE_16(0, DECLARE_TEXTURE_IMAGE_RECT)
	EXPAND_RANGE_16(0, DECLARE_TEXTURE_OFFSET)
	EXPAND_RANGE_16(0, DECLARE_TEXTURE_CONTROL0)
	EXPAND_RANGE_16(0, DECLARE_TEXTURE_CONTROL3)
	EXPAND_RANGE_16(0, DECLARE_TEXTURE_FORMAT)
	EXPAND_RANGE_16(0, DECLARE_TEXTURE_ADDRESS)
	EXPAND_RANGE_16(0, DECLARE_TEXTURE_FILTER)
	EXPAND_RANGE_16(0, DECLARE_TEXTURE_BORDER_COLOR)
	EXPAND_RANGE_16(0, DECLARE_TEXTURE_CONTROL1)
	EXPAND_RANGE_4(0, DECLARE_VERTEX_TEXTURE_OFFSET)
	EXPAND_RANGE_4(0, DECLARE_VERTEX_TEXTURE_FORMAT)
	EXPAND_RANGE_4(0, DECLARE_VERTEX_TEXTURE_ADDRESS)
	EXPAND_RANGE_4(0, DECLARE_VERTEX_TEXTURE_CONTROL0)
	EXPAND_RANGE_4(0, DECLARE_VERTEX_TEXTURE_FILTER)
	EXPAND_RANGE_4(0, DECLARE_VERTEX_TEXTURE_IMAGE_RECT)
	EXPAND_RANGE_4(0, DECLARE_VERTEX_TEXTURE_BORDER_COLOR)
	EXPAND_RANGE_4(0, DECLARE_VERTEX_TEXTURE_CONTROL3)
	EXPAND_RANGE_32(0, DECLARE_TRANSFORM_CONSTANT)
	NV4097_SET_TRANSFORM_CONSTANT_LOAD,
	EXPAND_RANGE_512(0, DECLARE_TRANSFORM_PROGRAM)
	NV4097_SET_TRANSFORM_PROGRAM_LOAD
> opcode_list{};
} // end namespace rsx
