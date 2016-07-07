#pragma once

#include <array>
#include <vector>

#include "GCM.h"
#include "RSXTexture.h"
#include "rsx_vertex_data.h"
#include "Utilities/geometry.h"

namespace rsx
{
	//TODO
	union alignas(4) method_registers_t
	{
		u8 _u8[0x10000];
		u32 _u32[0x10000 >> 2];
		/*
		struct alignas(4)
		{
		u8 pad[NV4097_SET_TEXTURE_OFFSET - 4];

		struct alignas(4) texture_t
		{
		u32 offset;

		union format_t
		{
		u32 _u32;

		struct
		{
		u32: 1;
		u32 location : 1;
		u32 cubemap : 1;
		u32 border_type : 1;
		u32 dimension : 4;
		u32 format : 8;
		u32 mipmap : 16;
		};
		} format;

		union address_t
		{
		u32 _u32;

		struct
		{
		u32 wrap_s : 4;
		u32 aniso_bias : 4;
		u32 wrap_t : 4;
		u32 unsigned_remap : 4;
		u32 wrap_r : 4;
		u32 gamma : 4;
		u32 signed_remap : 4;
		u32 zfunc : 4;
		};
		} address;

		u32 control0;
		u32 control1;
		u32 filter;
		u32 image_rect;
		u32 border_color;
		} textures[limits::textures_count];
		};
		*/
		u32& operator[](int index)
		{
			return _u32[index >> 2];
		}
	};

	template<typename T, size_t... N, typename Args>
	std::array<T, sizeof...(N)> fill_array(Args&& arg, std::index_sequence<N...> seq)
	{
		return{ T(N, std::forward<Args>(arg))... };
	}

	struct rsx_state
	{
	private:
		std::array<u32, 0x10000 / 4> registers;

	public:
		std::array<texture, 16> fragment_textures;
		std::array<vertex_texture, 4> vertex_textures;

		std::array<u32, 512 * 4> transform_program;

		std::unordered_map<u32, color4_base<f32>> transform_constants;

		/**
		* RSX can sources vertex attributes from 2 places:
		* - Immediate values passed by NV4097_SET_VERTEX_DATA*_M + ARRAY_ID write.
		* For a given ARRAY_ID the last command of this type defines the actual type of the immediate value.
		* Since there can be only a single value per ARRAY_ID passed this way, all vertex in the draw call
		* shares it.
		* - Vertex array values passed by offset/stride/size/format description.
		*
		* A given ARRAY_ID can have both an immediate value and a vertex array enabled at the same time
		* (See After Burner Climax intro cutscene). In such case the vertex array has precedence over the
		* immediate value. As soon as the vertex array is disabled (size set to 0) the immediate value
		* must be used if the vertex attrib mask request it.
		*
		* Note that behavior when both vertex array and immediate value system are disabled but vertex attrib mask
		* request inputs is unknow.
		*/
		std::array<data_array_format_info, 16> register_vertex_info;
		std::array<std::vector<u8>, 16> register_vertex_data;
		std::array<data_array_format_info, 16> vertex_arrays_info;

		rsx_state() :
			// unfortunatly there is no other way to fill an array with objects without default initializer
			// except by using templates.
			fragment_textures(fill_array<texture>(registers, std::make_index_sequence<16>())),
			vertex_textures(fill_array<vertex_texture>(registers, std::make_index_sequence<4>())),
			register_vertex_info(fill_array<data_array_format_info>(registers, std::make_index_sequence<16>())),
			vertex_arrays_info(fill_array<data_array_format_info>(registers, std::make_index_sequence<16>()))
		{

		}

		u32& operator[](size_t idx)
		{
			return registers[idx];
		}

		const u32& operator[](size_t idx) const
		{
			return registers[idx];
		}

		void reset()
		{
			//setup method registers
			std::memset(registers.data(), 0, registers.size() * sizeof(u32));

			registers[NV4097_SET_COLOR_MASK] = CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B | CELL_GCM_COLOR_MASK_A;
			registers[NV4097_SET_SCISSOR_HORIZONTAL] = (4096 << 16) | 0;
			registers[NV4097_SET_SCISSOR_VERTICAL] = (4096 << 16) | 0;

			registers[NV4097_SET_ALPHA_FUNC] = CELL_GCM_ALWAYS;
			registers[NV4097_SET_ALPHA_REF] = 0;

			registers[NV4097_SET_BLEND_FUNC_SFACTOR] = (CELL_GCM_ONE << 16) | CELL_GCM_ONE;
			registers[NV4097_SET_BLEND_FUNC_DFACTOR] = (CELL_GCM_ZERO << 16) | CELL_GCM_ZERO;
			registers[NV4097_SET_BLEND_COLOR] = 0;
			registers[NV4097_SET_BLEND_COLOR2] = 0;
			registers[NV4097_SET_BLEND_EQUATION] = (CELL_GCM_FUNC_ADD << 16) | CELL_GCM_FUNC_ADD;

			registers[NV4097_SET_STENCIL_MASK] = 0xff;
			registers[NV4097_SET_STENCIL_FUNC] = CELL_GCM_ALWAYS;
			registers[NV4097_SET_STENCIL_FUNC_REF] = 0x00;
			registers[NV4097_SET_STENCIL_FUNC_MASK] = 0xff;
			registers[NV4097_SET_STENCIL_OP_FAIL] = CELL_GCM_KEEP;
			registers[NV4097_SET_STENCIL_OP_ZFAIL] = CELL_GCM_KEEP;
			registers[NV4097_SET_STENCIL_OP_ZPASS] = CELL_GCM_KEEP;

			registers[NV4097_SET_BACK_STENCIL_MASK] = 0xff;
			registers[NV4097_SET_BACK_STENCIL_FUNC] = CELL_GCM_ALWAYS;
			registers[NV4097_SET_BACK_STENCIL_FUNC_REF] = 0x00;
			registers[NV4097_SET_BACK_STENCIL_FUNC_MASK] = 0xff;
			registers[NV4097_SET_BACK_STENCIL_OP_FAIL] = CELL_GCM_KEEP;
			registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL] = CELL_GCM_KEEP;
			registers[NV4097_SET_BACK_STENCIL_OP_ZPASS] = CELL_GCM_KEEP;

			registers[NV4097_SET_SHADE_MODE] = CELL_GCM_SMOOTH;

			registers[NV4097_SET_LOGIC_OP] = CELL_GCM_COPY;

			(f32&)registers[NV4097_SET_DEPTH_BOUNDS_MIN] = 0.f;
			(f32&)registers[NV4097_SET_DEPTH_BOUNDS_MAX] = 1.f;

			(f32&)registers[NV4097_SET_CLIP_MIN] = 0.f;
			(f32&)registers[NV4097_SET_CLIP_MAX] = 1.f;

			registers[NV4097_SET_LINE_WIDTH] = 1 << 3;

			// These defaults were found using After Burner Climax (which never set fog mode despite using fog input)
			registers[NV4097_SET_FOG_MODE] = 0x2601; // rsx::fog_mode::linear;
			(f32&)registers[NV4097_SET_FOG_PARAMS] = 1.;
			(f32&)registers[NV4097_SET_FOG_PARAMS + 1] = 1.;

			registers[NV4097_SET_DEPTH_FUNC] = CELL_GCM_LESS;
			registers[NV4097_SET_DEPTH_MASK] = CELL_GCM_TRUE;
			(f32&)registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0.f;
			(f32&)registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0.f;
			registers[NV4097_SET_FRONT_POLYGON_MODE] = CELL_GCM_POLYGON_MODE_FILL;
			registers[NV4097_SET_BACK_POLYGON_MODE] = CELL_GCM_POLYGON_MODE_FILL;
			registers[NV4097_SET_CULL_FACE] = CELL_GCM_BACK;
			registers[NV4097_SET_FRONT_FACE] = CELL_GCM_CCW;
			registers[NV4097_SET_RESTART_INDEX] = -1;

			registers[NV4097_SET_CLEAR_RECT_HORIZONTAL] = (4096 << 16) | 0;
			registers[NV4097_SET_CLEAR_RECT_VERTICAL] = (4096 << 16) | 0;

			registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] = 0xffffffff;

			registers[NV4097_SET_CONTEXT_DMA_REPORT] = CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_REPORT;
			registers[NV4097_SET_TWO_SIDE_LIGHT_EN] = true;
			registers[NV4097_SET_ALPHA_FUNC] = CELL_GCM_ALWAYS;

			// Reset vertex attrib array
			for (int i = 0; i < 16; i++)
			{
				vertex_arrays_info[i].size = 0;
			}

			// Construct Textures
			for (int i = 0; i < 16; i++)
			{
				fragment_textures[i].init(i);
			}

			for (int i = 0; i < 4; i++)
			{
				vertex_textures[i].init(i);
			}
		}

		u16 viewport_width() const
		{
			return registers[NV4097_SET_VIEWPORT_HORIZONTAL] >> 16;
		}

		u16 viewport_origin_x() const
		{
			return registers[NV4097_SET_VIEWPORT_HORIZONTAL] & 0xffff;
		}

		u16 viewport_height() const
		{
			return registers[NV4097_SET_VIEWPORT_VERTICAL] >> 16;
		}

		u16 viewport_origin_y() const
		{
			return registers[NV4097_SET_VIEWPORT_VERTICAL] & 0xffff;
		}

		u16 scissor_origin_x() const
		{
			return registers[NV4097_SET_SCISSOR_HORIZONTAL] & 0xffff;
		}

		u16 scissor_width() const
		{
			return registers[NV4097_SET_SCISSOR_HORIZONTAL] >> 16;
		}

		u16 scissor_origin_y() const
		{
			return registers[NV4097_SET_SCISSOR_VERTICAL] & 0xffff;
		}

		u16 scissor_height() const
		{
			return registers[NV4097_SET_SCISSOR_VERTICAL] >> 16;
		}

		rsx::window_origin shader_window_origin() const
		{
			return rsx::to_window_origin((registers[NV4097_SET_SHADER_WINDOW] >> 12) & 0xf);
		}

		rsx::window_pixel_center shader_window_pixel() const
		{
			return rsx::to_window_pixel_center((registers[NV4097_SET_SHADER_WINDOW] >> 16) & 0xf);
		}

		u16 shader_window_height() const
		{
			return registers[NV4097_SET_SHADER_WINDOW] & 0xfff;
		}

		u32 shader_window_offset_x() const
		{
			return registers[NV4097_SET_WINDOW_OFFSET] & 0xffff;
		}

		u32 shader_window_offset_y() const
		{
			return registers[NV4097_SET_WINDOW_OFFSET] >> 16;
		}

		bool depth_test_enabled() const
		{
			return !!registers[NV4097_SET_DEPTH_TEST_ENABLE];
		}

		bool depth_write_enabled() const
		{
			return !!registers[NV4097_SET_DEPTH_MASK];
		}

		bool alpha_test_enabled() const
		{
			return !!registers[NV4097_SET_ALPHA_TEST_ENABLE];
		}

		bool stencil_test_enabled() const
		{
			return !!registers[NV4097_SET_STENCIL_TEST_ENABLE];
		}

		bool restart_index_enabled() const
		{
			return !!registers[NV4097_SET_RESTART_INDEX_ENABLE];
		}

		u32 restart_index() const
		{
			return registers[NV4097_SET_RESTART_INDEX];
		}

		u32 z_clear_value() const
		{
			return registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8;
		}

		u8 stencil_clear_value() const
		{
			return registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] & 0xff;
		}

		f32 fog_params_0() const
		{
			return (f32&)registers[NV4097_SET_FOG_PARAMS];
		}

		f32 fog_params_1() const
		{
			return (f32&)registers[NV4097_SET_FOG_PARAMS + 1];
		}

		rsx::index_array_type index_type() const
		{
			return rsx::to_index_array_type(registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);
		}

		bool color_mask_b() const
		{
			return !!(registers[NV4097_SET_COLOR_MASK] & 0xff);
		}

		bool color_mask_g() const
		{
			return !!((registers[NV4097_SET_COLOR_MASK] >> 8) & 0xff);
		}

		bool color_mask_r() const
		{
			return !!((registers[NV4097_SET_COLOR_MASK] >> 16) & 0xff);
		}

		bool color_mask_a() const
		{
			return !!((registers[NV4097_SET_COLOR_MASK] >> 24) & 0xff);
		}

		u8 clear_color_b() const
		{
			return registers[NV4097_SET_COLOR_CLEAR_VALUE] & 0xff;
		}

		u8 clear_color_r() const
		{
			return (registers[NV4097_SET_COLOR_CLEAR_VALUE] >> 16) & 0xff;
		}

		u8 clear_color_g() const
		{
			return (registers[NV4097_SET_COLOR_CLEAR_VALUE] >> 8) & 0xff;
		}

		u8 clear_color_a() const
		{
			return (registers[NV4097_SET_COLOR_CLEAR_VALUE] >> 24) & 0xff;
		}

		bool depth_bounds_test_enabled() const
		{
			return !!registers[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE];
		}

		f32 depth_bounds_min() const
		{
			return (f32&)registers[NV4097_SET_DEPTH_BOUNDS_MIN];
		}

		f32 depth_bounds_max() const
		{
			return (f32&)registers[NV4097_SET_DEPTH_BOUNDS_MAX];
		}

		f32 clip_min() const
		{
			return (f32&)registers[NV4097_SET_CLIP_MIN];
		}

		f32 clip_max() const
		{
			return (f32&)registers[NV4097_SET_CLIP_MAX];
		}

		bool logic_op_enabled() const
		{
			return !!registers[NV4097_SET_LOGIC_OP_ENABLE];
		}

		u8 stencil_mask() const
		{
			return registers[NV4097_SET_STENCIL_MASK] & 0xff;
		}

		u8 back_stencil_mask() const
		{
			return registers[NV4097_SET_BACK_STENCIL_MASK];
		}

		bool dither_enabled() const
		{
			return !!registers[NV4097_SET_DITHER_ENABLE];
		}

		bool blend_enabled() const
		{
			return !!registers[NV4097_SET_BLEND_ENABLE];
		}

		bool blend_enabled_surface_1() const
		{
			return !!(registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x2);
		}

		bool blend_enabled_surface_2() const
		{
			return !!(registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x4);
		}

		bool blend_enabled_surface_3() const
		{
			return !!(registers[NV4097_SET_BLEND_ENABLE_MRT] & 0x8);
		}

		bool line_smooth_enabled() const
		{
			return !!registers[NV4097_SET_LINE_SMOOTH_ENABLE];
		}

		bool poly_offset_point_enabled() const
		{
			return !!registers[NV4097_SET_POLY_OFFSET_POINT_ENABLE];
		}

		bool poly_offset_line_enabled() const
		{
			return !!registers[NV4097_SET_POLY_OFFSET_LINE_ENABLE];
		}

		bool poly_offset_fill_enabled() const
		{
			return !!registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE];
		}

		f32 poly_offset_scale() const
		{
			return (f32&)registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR];
		}

		f32 poly_offset_bias() const
		{
			return (f32&)registers[NV4097_SET_POLYGON_OFFSET_BIAS];
		}

		bool cull_face_enabled() const
		{
			return !!registers[NV4097_SET_CULL_FACE_ENABLE];
		}

		bool poly_smooth_enabled() const
		{
			return !!registers[NV4097_SET_POLY_SMOOTH_ENABLE];
		}

		bool two_sided_stencil_test_enabled() const
		{
			return !!registers[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE];
		}

		comparaison_function depth_func() const
		{
			return to_comparaison_function(registers[NV4097_SET_DEPTH_FUNC] & 0xffff);
		}

		comparaison_function stencil_func() const
		{
			return to_comparaison_function(registers[NV4097_SET_STENCIL_FUNC] & 0xffff);
		}

		comparaison_function back_stencil_func() const
		{
			return to_comparaison_function(registers[NV4097_SET_BACK_STENCIL_FUNC] & 0xffff);
		}

		u8 stencil_func_ref() const
		{
			return registers[NV4097_SET_STENCIL_FUNC_REF];
		}

		u8 back_stencil_func_ref() const
		{
			return registers[NV4097_SET_BACK_STENCIL_FUNC_REF];
		}

		u8 stencil_func_mask() const
		{
			return registers[NV4097_SET_STENCIL_FUNC_MASK];
		}

		u8 back_stencil_func_mask() const
		{
			return registers[NV4097_SET_BACK_STENCIL_FUNC_MASK];
		}

		rsx::stencil_op stencil_op_fail() const
		{
			return to_stencil_op(registers[NV4097_SET_STENCIL_OP_FAIL] & 0xffff);
		}

		rsx::stencil_op stencil_op_zfail() const
		{
			return to_stencil_op(registers[NV4097_SET_STENCIL_OP_ZFAIL] & 0xffff);
		}

		rsx::stencil_op stencil_op_zpass() const
		{
			return to_stencil_op(registers[NV4097_SET_STENCIL_OP_ZPASS] & 0xffff);
		}

		rsx::stencil_op back_stencil_op_fail() const
		{
			return to_stencil_op(registers[NV4097_SET_BACK_STENCIL_OP_FAIL] & 0xffff);
		}

		rsx::stencil_op back_stencil_op_zfail() const
		{
			return to_stencil_op(registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL] & 0xffff);
		}

		rsx::stencil_op back_stencil_op_zpass() const
		{
			return to_stencil_op(registers[NV4097_SET_BACK_STENCIL_OP_ZPASS] & 0xffff);
		}

		u8 blend_color_8b_r() const
		{
			return registers[NV4097_SET_BLEND_COLOR] & 0xff;
		}

		u8 blend_color_8b_g() const
		{
			return (registers[NV4097_SET_BLEND_COLOR] >> 8) & 0xff;
		}

		u8 blend_color_8b_b() const
		{
			return (registers[NV4097_SET_BLEND_COLOR] >> 16) & 0xff;
		}

		u8 blend_color_8b_a() const
		{
			return (registers[NV4097_SET_BLEND_COLOR] >> 24) & 0xff;
		}

		u16 blend_color_16b_r() const
		{
			return registers[NV4097_SET_BLEND_COLOR] & 0xffff;
		}

		u16 blend_color_16b_g() const
		{
			return (registers[NV4097_SET_BLEND_COLOR] >> 16) & 0xffff;
		}

		u16 blend_color_16b_b() const
		{
			return registers[NV4097_SET_BLEND_COLOR2] & 0xffff;
		}

		u16 blend_color_16b_a() const
		{
			return (registers[NV4097_SET_BLEND_COLOR2] >> 16) & 0xffff;
		}

		blend_equation blend_equation_rgb() const
		{
			return to_blend_equation(registers[NV4097_SET_BLEND_EQUATION] & 0xffff);
		}

		blend_equation blend_equation_a() const
		{
			return to_blend_equation((registers[NV4097_SET_BLEND_EQUATION] >> 16) & 0xffff);
		}

		blend_factor blend_func_sfactor_rgb() const
		{
			return to_blend_factor(registers[NV4097_SET_BLEND_FUNC_SFACTOR] & 0xffff);
		}

		blend_factor blend_func_sfactor_a() const
		{
			return to_blend_factor((registers[NV4097_SET_BLEND_FUNC_SFACTOR] >> 16) & 0xffff);
		}

		blend_factor blend_func_dfactor_rgb() const
		{
			return to_blend_factor(registers[NV4097_SET_BLEND_FUNC_DFACTOR] & 0xffff);
		}

		blend_factor blend_func_dfactor_a() const
		{
			return to_blend_factor((registers[NV4097_SET_BLEND_FUNC_DFACTOR] >> 16) & 0xffff);
		}

		logic_op logic_operation() const
		{
			return to_logic_op(registers[NV4097_SET_LOGIC_OP]);
		}

		user_clip_plane_op clip_plane_0_enabled() const
		{
			return to_user_clip_plane_op(registers[NV4097_SET_USER_CLIP_PLANE_CONTROL] & 0xf);
		}

		user_clip_plane_op clip_plane_1_enabled() const
		{
			return to_user_clip_plane_op((registers[NV4097_SET_USER_CLIP_PLANE_CONTROL] >> 4) & 0xf);
		}

		user_clip_plane_op clip_plane_2_enabled() const
		{
			return to_user_clip_plane_op((registers[NV4097_SET_USER_CLIP_PLANE_CONTROL] >> 8) & 0xf);
		}

		user_clip_plane_op clip_plane_3_enabled() const
		{
			return to_user_clip_plane_op((registers[NV4097_SET_USER_CLIP_PLANE_CONTROL] >> 12) & 0xf);
		}

		user_clip_plane_op clip_plane_4_enabled() const
		{
			return to_user_clip_plane_op((registers[NV4097_SET_USER_CLIP_PLANE_CONTROL] >> 16) & 0xf);
		}

		user_clip_plane_op clip_plane_5_enabled() const
		{
			return to_user_clip_plane_op((registers[NV4097_SET_USER_CLIP_PLANE_CONTROL] >> 20) & 0xf);
		}

		front_face front_face_mode() const
		{
			return to_front_face(registers[NV4097_SET_FRONT_FACE] & 0xffff);
		}

		cull_face cull_face_mode() const
		{
			return to_cull_face(registers[NV4097_SET_CULL_FACE] & 0xffff);
		}

		f32 line_width() const
		{
			u32 line_width = registers[NV4097_SET_LINE_WIDTH];
			return (line_width >> 3) + (line_width & 7) / 8.f;
		}

		u8 alpha_ref() const
		{
			return registers[NV4097_SET_ALPHA_REF] & 0xff;
		}

		surface_target surface_color_target()
		{
			return rsx::to_surface_target(registers[NV4097_SET_SURFACE_COLOR_TARGET] & 0xff);
		}

		u16 surface_clip_origin_x() const
		{
			return (registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] & 0xffff);
		}

		u16 surface_clip_width() const
		{
			return ((registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16) & 0xffff);
		}

		u16 surface_clip_origin_y() const
		{
			return (registers[NV4097_SET_SURFACE_CLIP_VERTICAL] & 0xffff);
		}

		u16 surface_clip_height() const
		{
			return ((registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16) & 0xffff);
		}

		u32 surface_a_offset() const
		{
			return registers[NV4097_SET_SURFACE_COLOR_AOFFSET];
		}

		u32 surface_b_offset() const
		{
			return registers[NV4097_SET_SURFACE_COLOR_BOFFSET];
		}

		u32 surface_c_offset() const
		{
			return registers[NV4097_SET_SURFACE_COLOR_COFFSET];
		}

		u32 surface_d_offset() const
		{
			return registers[NV4097_SET_SURFACE_COLOR_DOFFSET];
		}

		u32 surface_a_pitch() const
		{
			return registers[NV4097_SET_SURFACE_PITCH_A];
		}

		u32 surface_b_pitch() const
		{
			return registers[NV4097_SET_SURFACE_PITCH_B];
		}

		u32 surface_c_pitch() const
		{
			return registers[NV4097_SET_SURFACE_PITCH_C];
		}

		u32 surface_d_pitch() const
		{
			return registers[NV4097_SET_SURFACE_PITCH_D];
		}

		u32 surface_a_dma() const
		{
			return registers[NV4097_SET_CONTEXT_DMA_COLOR_A];
		}

		u32 surface_b_dma() const
		{
			return registers[NV4097_SET_CONTEXT_DMA_COLOR_B];
		}

		u32 surface_c_dma() const
		{
			return registers[NV4097_SET_CONTEXT_DMA_COLOR_C];
		}

		u32 surface_d_dma() const
		{
			return registers[NV4097_SET_CONTEXT_DMA_COLOR_D];
		}

		u32 surface_z_offset() const
		{
			return registers[NV4097_SET_SURFACE_ZETA_OFFSET];
		}

		u32 surface_z_pitch() const
		{
			return registers[NV4097_SET_SURFACE_PITCH_Z];
		}

		u32 surface_z_dma() const
		{
			return registers[NV4097_SET_CONTEXT_DMA_ZETA];
		}

		f32 viewport_scale_x() const
		{
			return (f32&)registers[NV4097_SET_VIEWPORT_SCALE];
		}

		f32 viewport_scale_y() const
		{
			return (f32&)registers[NV4097_SET_VIEWPORT_SCALE + 1];
		}

		f32 viewport_scale_z() const
		{
			return (f32&)registers[NV4097_SET_VIEWPORT_SCALE + 2];
		}

		f32 viewport_scale_w() const
		{
			return (f32&)registers[NV4097_SET_VIEWPORT_SCALE + 3];
		}

		f32 viewport_offset_x() const
		{
			return (f32&)registers[NV4097_SET_VIEWPORT_OFFSET];
		}

		f32 viewport_offset_y() const
		{
			return (f32&)registers[NV4097_SET_VIEWPORT_OFFSET + 1];
		}

		f32 viewport_offset_z() const
		{
			return (f32&)registers[NV4097_SET_VIEWPORT_OFFSET + 2];
		}

		f32 viewport_offset_w() const
		{
			return (f32&)registers[NV4097_SET_VIEWPORT_OFFSET + 3];
		}

		bool two_side_light_en() const
		{
			return !!registers[NV4097_SET_TWO_SIDE_LIGHT_EN];
		}

		rsx::fog_mode fog_equation() const
		{
			return rsx::to_fog_mode(registers[NV4097_SET_FOG_MODE]);
		}

		rsx::comparaison_function alpha_func() const
		{
			return to_comparaison_function(registers[NV4097_SET_ALPHA_FUNC]);
		}

		u16 vertex_attrib_input_mask() const
		{
			return registers[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK] & 0xffff;
		}

		u16 frequency_divider_operation_mask() const
		{
			return registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION] & 0xffff;
		}

		u32 vertex_attrib_output_mask() const
		{
			return registers[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK];
		}

		// Quite opaque
		u32 shader_control() const
		{
			return registers[NV4097_SET_SHADER_CONTROL];
		}

		surface_color_format surface_color() const
		{
			return to_surface_color_format(registers[NV4097_SET_SURFACE_FORMAT] & 0x1f);
		}

		surface_depth_format surface_depth_fmt() const
		{
			return to_surface_depth_format((registers[NV4097_SET_SURFACE_FORMAT] >> 5) & 0x7);
		}

		surface_antialiasing surface_antialias() const
		{
			return  to_surface_antialiasing((registers[NV4097_SET_SURFACE_FORMAT] >> 12) & 0xf);
		}

		u8 surface_log2_height() const
		{
			return (registers[NV4097_SET_SURFACE_FORMAT] >> 24) & 0xff;
		}

		u8 surface_log2_width() const
		{
			return (registers[NV4097_SET_SURFACE_FORMAT] >> 16) & 0xff;
		}

		u32 vertex_data_base_offset() const
		{
			return registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
		}

		u32 index_array_address() const
		{
			return registers[NV4097_SET_INDEX_ARRAY_ADDRESS];
		}

		u8 index_array_location() const
		{
			return registers[NV4097_SET_INDEX_ARRAY_DMA] & 0xf;
		}

		u32 vertex_data_base_index() const
		{
			return registers[NV4097_SET_VERTEX_DATA_BASE_INDEX];
		}

		u32 shader_program_address() const
		{
			return registers[NV4097_SET_SHADER_PROGRAM];
		}

		u32 transform_program_start() const
		{
			return registers[NV4097_SET_TRANSFORM_PROGRAM_START];
		}

		primitive_type primitive_mode() const
		{
			return to_primitive_type(registers[NV4097_SET_BEGIN_END]);
		}

		u32 semaphore_offset_406e() const
		{
			return registers[NV406E_SEMAPHORE_OFFSET];
		}

		u32 semaphore_offset_4097() const
		{
			return registers[NV4097_SET_SEMAPHORE_OFFSET];
		}

		blit_engine::context_dma context_dma_report() const
		{
			return blit_engine::to_context_dma(registers[NV4097_SET_CONTEXT_DMA_REPORT]);
		}

		blit_engine::transfer_operation blit_engine_operation() const
		{
			return blit_engine::to_transfer_operation(registers[NV3089_SET_OPERATION]);
		}

		/// TODO: find the purpose vs in/out equivalents
		u16 blit_engine_clip_x() const
		{
			return registers[NV3089_CLIP_POINT] & 0xffff;
		}

		u16 blit_engine_clip_y() const
		{
			return registers[NV3089_CLIP_POINT] >> 16;
		}

		u16 blit_engine_clip_width() const
		{
			return registers[NV3089_CLIP_SIZE] & 0xffff;
		}

		u16 blit_engine_clip_height() const
		{
			return registers[NV3089_CLIP_SIZE] >> 16;
		}

		u16 blit_engine_output_x() const
		{
			return registers[NV3089_IMAGE_OUT_POINT] & 0xffff;
		}

		u16 blit_engine_output_y() const
		{
			return registers[NV3089_IMAGE_OUT_POINT] >> 16;
		}

		u16 blit_engine_output_width() const
		{
			return registers[NV3089_IMAGE_OUT_SIZE] & 0xffff;
		}

		u16 blit_engine_output_height() const
		{
			return registers[NV3089_IMAGE_OUT_SIZE] >> 16;
		}

		// there is no x/y ?
		u16 blit_engine_input_width() const
		{
			return registers[NV3089_IMAGE_IN_SIZE] & 0xffff;
		}

		u16 blit_engine_input_height() const
		{
			return registers[NV3089_IMAGE_IN_SIZE] >> 16;
		}

		u16 blit_engine_input_pitch() const
		{
			return registers[NV3089_IMAGE_IN_FORMAT] & 0xffff;
		}

		blit_engine::transfer_origin blit_engine_input_origin() const
		{
			return blit_engine::to_transfer_origin((registers[NV3089_IMAGE_IN_FORMAT] >> 16) & 0xff);
		}

		blit_engine::transfer_interpolator blit_engine_input_inter() const
		{
			return blit_engine::to_transfer_interpolator((registers[NV3089_IMAGE_IN_FORMAT] >> 24) & 0xff);
		}

		blit_engine::transfer_source_format blit_engine_src_color_format() const
		{
			return blit_engine::to_transfer_source_format(registers[NV3089_SET_COLOR_FORMAT]);
		}

		// ???
		f32 blit_engine_in_x() const
		{
			return (registers[NV3089_IMAGE_IN] & 0xffff) / 16.f;
		}

		// ???
		f32 blit_engine_in_y() const
		{
			return (registers[NV3089_IMAGE_IN] >> 16) / 16.f;
		}

		u32 blit_engine_input_offset() const
		{
			return registers[NV3089_IMAGE_IN_OFFSET];
		}

		u32 blit_engine_input_location() const
		{
			return registers[NV3089_SET_CONTEXT_DMA_IMAGE];
		}

		blit_engine::context_surface blit_engine_context_surface() const
		{
			return blit_engine::to_context_surface(registers[NV3089_SET_CONTEXT_SURFACE]);
		}

		u32 blit_engine_output_location_nv3062() const
		{
			return registers[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN];
		}

		u32 blit_engine_output_offset_nv3062() const
		{
			return registers[NV3062_SET_OFFSET_DESTIN];
		}

		blit_engine::transfer_destination_format blit_engine_nv3062_color_format() const
		{
			return rsx::blit_engine::to_transfer_destination_format(registers[NV3062_SET_COLOR_FORMAT]);
		}

		u16 blit_engine_output_alignment_nv3062() const
		{
			return registers[NV3062_SET_PITCH] & 0xffff;
		}

		u16 blit_engine_output_pitch_nv3062() const
		{
			return registers[NV3062_SET_PITCH] >> 16;
		}

		u32 blit_engine_nv309E_location() const
		{
			return registers[NV309E_SET_CONTEXT_DMA_IMAGE];
		}

		u32 blit_engine_nv309E_offset() const
		{
			return registers[NV309E_SET_OFFSET];
		}

		blit_engine::transfer_destination_format blit_engine_output_format_nv309E() const
		{
			return rsx::blit_engine::to_transfer_destination_format(registers[NV309E_SET_FORMAT]);
		}

		u32 blit_engine_ds_dx() const
		{
			return registers[NV3089_DS_DX];
		}

		u32 blit_engine_dt_dy() const
		{
			return registers[NV3089_DT_DY];
		}

		u8 nv309e_sw_width_log2() const
		{
			return (registers[NV309E_SET_FORMAT] >> 16) & 0xff;
		}

		u8 nv309e_sw_height_log2() const
		{
			return (registers[NV309E_SET_FORMAT] >> 24) & 0xff;
		}

		u32 nv0039_input_pitch() const
		{
			return registers[NV0039_PITCH_IN];
		}

		u32 nv0039_output_pitch() const
		{
			return registers[NV0039_PITCH_OUT];
		}

		u32 nv0039_line_length() const
		{
			return registers[NV0039_LINE_LENGTH_IN];
		}

		u32 nv0039_line_count() const
		{
			return registers[NV0039_LINE_COUNT];
		}

		u8 nv0039_output_format() const
		{
			return (registers[NV0039_FORMAT] >> 8) & 0xff;
		}

		u8 nv0039_input_format() const
		{
			return registers[NV0039_FORMAT] & 0xff;
		}

		u32 nv0039_output_offset() const
		{
			return registers[NV0039_OFFSET_OUT];
		}

		u32 nv0039_output_location()
		{
			return registers[NV0039_SET_CONTEXT_DMA_BUFFER_OUT];
		}

		u32 nv0039_input_offset() const
		{
			return registers[NV0039_OFFSET_IN];
		}

		u32 nv0039_input_location() const
		{
			return registers[NV0039_SET_CONTEXT_DMA_BUFFER_IN];
		}

		void commit_4_transform_program_instructions(u32 index)
		{
			u32& load =registers[NV4097_SET_TRANSFORM_PROGRAM_LOAD];

			transform_program[load * 4] = registers[NV4097_SET_TRANSFORM_PROGRAM + index * 4];
			transform_program[load * 4 + 1] = registers[NV4097_SET_TRANSFORM_PROGRAM + index * 4 + 1];
			transform_program[load * 4 + 2] = registers[NV4097_SET_TRANSFORM_PROGRAM + index * 4 + 2];
			transform_program[load * 4 + 3] = registers[NV4097_SET_TRANSFORM_PROGRAM + index * 4 + 3];
			load++;
		}

		void set_transform_constant(u32 index, u32 constant)
		{
			u32 load = registers[NV4097_SET_TRANSFORM_CONSTANT_LOAD];
			u32 reg = index / 4;
			u32 subreg = index % 4;
			transform_constants[load + reg].rgba[subreg] = (f32&)constant;
		}

		u16 nv308a_x() const
		{
			return registers[NV308A_POINT] & 0xffff;
		}

		u16 nv308a_y() const
		{
			return registers[NV308A_POINT] >> 16;
		}
	};

	using rsx_method_t = void(*)(class thread*, u32);
	extern rsx_state method_registers;
	extern rsx_method_t methods[0x10000 >> 2];
}
