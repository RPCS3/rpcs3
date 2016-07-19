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

	struct rsx_state
	{
	private:
		std::array<u32, 0x10000 / 4> registers;

	public:
		std::array<texture, 16> fragment_textures;
		std::array<vertex_texture, 4> vertex_textures;

		u32 m_transform_program_pointer;

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
		* request inputs is unknown.
		*/
		std::array<data_array_format_info, 16> register_vertex_info;
		std::array<std::vector<u8>, 16> register_vertex_data;
		std::array<data_array_format_info, 16> vertex_arrays_info;

		rsx_state();
		~rsx_state();

		u32& operator[](size_t idx)
		{
			return registers[idx];
		}

		const u32& operator[](size_t idx) const
		{
			return registers[idx];
		}

		void decode(u32 reg, u32 value);

		void reset();

		u16 m_viewport_origin_x;
		u16 m_viewport_origin_y;

		u16 m_viewport_width;
		u16 m_viewport_height;

		u16 m_scissor_origin_x;
		u16 m_scissor_origin_y;

		u16 m_scissor_width;
		u16 m_scissor_height;

		u16 m_clear_rect_origin_x;
		u16 m_clear_rect_origin_y;
		u16 m_clear_rect_width;
		u16 m_clear_rect_height;

		window_origin m_shader_window_origin : 1;
		window_pixel_center m_shader_window_pixel : 1;
		u16 m_shader_window_height : 12;

		u16 m_shader_window_offset_x;
		u16 m_shader_window_offset_y;

		bool m_dither_enabled : 1;
		bool m_line_smooth_enabled : 1;
		bool m_poly_offset_point_enabled : 1;
		bool m_offset_line_enabled : 1;
		bool m_poly_offset_fill_enabled : 1;
		bool m_poly_smooth_enabled : 1;
		bool m_cull_face_enabled : 1;
		bool m_two_sided_stencil_test_enabled : 1;
		bool m_color_mask_b : 1;
		bool m_color_mask_g : 1;
		bool m_color_mask_r : 1;
		bool m_color_mask_a : 1;

		rsx::shading_mode m_shading_mode : 1;

		rsx::polygon_mode m_front_polygon_mode : 2;
		rsx::polygon_mode m_back_polygon_mode : 2;

		bool m_alpha_test_enabled : 1;
		u8 m_alpha_ref;
		comparaison_function m_alpha_func : 3;

		bool m_restart_index_enabled : 1;
		u32 m_restart_index;

		u8 m_stencil_clear_value;
		u32 m_z_clear_value : 24;

		fog_mode m_fog_equation : 3;
		f32 m_fog_params_0;
		f32 m_fog_params_1;

		u8 m_index_array_location : 4;
		rsx::index_array_type m_index_type : 1;
		u32 m_index_array_address;

		u8 m_clear_color_b;
		u8 m_clear_color_r;
		u8 m_clear_color_g;
		u8 m_clear_color_a;

		f32 m_poly_offset_scale;
		f32 m_poly_offset_bias;

		bool m_depth_bounds_test_enabled : 1;
		bool m_depth_test_enabled : 1;
		bool m_depth_write_enabled : 1;
		comparaison_function m_depth_func : 3;
		f32 m_depth_bounds_min;
		f32 m_depth_bounds_max;
		f32 m_clip_min;
		f32 m_clip_max;

		bool m_stencil_test_enabled : 1;
		u8 m_stencil_mask;
		u8 m_back_stencil_mask;
		u8 m_stencil_func_ref;
		u8 m_back_stencil_func_ref;
		u8 m_stencil_func_mask;
		u8 m_back_stencil_func_mask;
		comparaison_function m_stencil_func : 3;
		comparaison_function m_back_stencil_func : 3;
		stencil_op m_stencil_op_fail : 3;
		stencil_op m_stencil_op_zfail : 3;
		stencil_op m_stencil_op_zpass : 3;
		stencil_op m_back_stencil_op_fail : 3;
		stencil_op m_back_stencil_op_zfail : 3;
		stencil_op m_back_stencil_op_zpass : 3;

		bool m_blend_enabled : 1;
		bool m_blend_enabled_surface_1 : 1;
		bool m_blend_enabled_surface_2 : 1;
		bool m_blend_enabled_surface_3 : 1;
		// Can be interpreted as 4 x 8 or 2 x 16 color
		u32 m_blend_color;
		u16 m_blend_color_16b_b;
		u16 m_blend_color_16b_a;
		blend_equation m_blend_equation_rgb : 3;
		blend_equation m_blend_equation_a : 3;
		blend_factor m_blend_func_sfactor_rgb : 4;
		blend_factor m_blend_func_sfactor_a : 4;
		blend_factor m_blend_func_dfactor_rgb : 4;
		blend_factor m_blend_func_dfactor_a : 4;

		bool m_logic_op_enabled : 1;
		logic_op m_logic_operation : 4;

		user_clip_plane_op m_clip_plane_0_enabled : 1;
		user_clip_plane_op m_clip_plane_1_enabled : 1;
		user_clip_plane_op m_clip_plane_2_enabled : 1;
		user_clip_plane_op m_clip_plane_3_enabled : 1;
		user_clip_plane_op m_clip_plane_4_enabled : 1;
		user_clip_plane_op m_clip_plane_5_enabled : 1;

		front_face m_front_face_mode : 1;

		cull_face m_cull_face_mode : 2;

		f32 m_line_width;

		surface_target m_surface_color_target : 3;

		u16 m_surface_clip_origin_x;
		u16 m_surface_clip_width;
		u16 m_surface_clip_origin_y;
		u16 m_surface_clip_height;

		u32 m_surface_a_offset;
		u32 m_surface_a_pitch;
		u32 m_surface_a_dma;
		u32 m_surface_b_offset;
		u32 m_surface_b_pitch;
		u32 m_surface_b_dma;
		u32 m_surface_c_offset;
		u32 m_surface_c_pitch;
		u32 m_surface_c_dma;
		u32 m_surface_d_offset;
		u32 m_surface_d_pitch;
		u32 m_surface_d_dma;
		u32 m_surface_z_offset;
		u32 m_surface_z_pitch;
		u32 m_surface_z_dma;

		f32 m_viewport_scale_x;
		f32 m_viewport_scale_y;
		f32 m_viewport_scale_z;
		f32 m_viewport_scale_w;
		f32 m_viewport_offset_x;
		f32 m_viewport_offset_y;
		f32 m_viewport_offset_z;
		f32 m_viewport_offset_w;

		bool m_two_side_light_enabled : 1;

		u16 m_vertex_attrib_input_mask;
		u16 m_frequency_divider_operation_mask;
		u32 m_vertex_attrib_output_mask;

		// Quite opaque
		u32 m_shader_control;

		surface_color_format m_surface_color : 4;
		surface_depth_format m_surface_depth_format : 1;
		surface_antialiasing m_surface_antialias : 2;
		u8 m_surface_log2_height;
		u8 m_surface_log2_width;

		bool m_msaa_enabled : 1;
		bool m_msaa_alpha_to_coverage : 1;
		bool m_msaa_alpha_to_one : 1;
		u16 m_msaa_sample_mask;

		u32 m_vertex_data_base_offset;
		u32 m_vertex_data_base_index;
		u32 m_shader_program_address;
		u32 m_transform_program_start;

		primitive_type m_primitive_type : 4;
		u32 m_semaphore_offset_406e;
		u32 m_semaphore_offset_4097;


		u32 m_transform_constant_file_pointer;

		blit_engine::context_dma m_context_dma_report : 1;
		blit_engine::transfer_operation m_blit_engine_operation : 3;
		u16 m_blit_engine_clip_x;
		u16 m_blit_engine_clip_y;
		u16 m_blit_engine_width;
		u16 m_blit_engine_height;
		u16 m_blit_engine_output_x;
		u16 m_blit_engine_output_y;
		u16 m_blit_engine_output_width;
		u16 m_blit_engine_output_height;
		u16 m_blit_engine_input_width;
		u16 m_blit_engine_input_height;
		u16 m_blit_engine_input_pitch;
		blit_engine::transfer_origin m_blit_engine_input_origin : 1;
		blit_engine::transfer_interpolator m_blit_engine_input_inter : 1;
		blit_engine::transfer_source_format m_blit_engine_src_color_format : 4;
		u16 m_blit_engine_in_x;
		u16 m_blit_engine_in_y;
		u32 m_blit_engine_input_offset;
		u32 m_blit_engine_input_location;
		blit_engine::context_surface m_blit_engine_context_surface : 1;
		u32 m_blit_engine_output_location_nv3062;
		blit_engine::transfer_destination_format m_blit_engine_nv3062_color_format : 2;
		u32 m_blit_engine_output_offset_nv3062;
		u16 m_blit_engine_output_alignement_nv3062;
		u16 m_blit_engine_output_pitch_nv3062;
		u32 m_blit_engine_nv309E_location;
		u32 m_blit_engine_nv309E_offset;
		blit_engine::transfer_destination_format m_blit_engine_output_format_nv309E : 2;
		u32 m_blit_engine_ds_dx;
		u32 m_blit_engine_dt_dy;
		u8 m_nv309e_sw_width_log2;
		u8 m_nv309e_sw_height_log2;
		u32 m_nv0039_input_pitch;
		u32 m_nv0039_output_pitch;
		u32 m_nv0039_line_length;
		u32 m_nv0039_line_count;
		u8 m_nv0039_input_format;
		u8 m_nv0039_output_format;
		u32 m_nv0039_output_offset;
		u32 m_nv0039_output_location;
		u32 m_nv0039_input_offset;
		u32 m_nv0039_input_location;
		u16 m_nv308a_x;
		u16 m_nv308a_y;

		u16 viewport_width() const
		{
			return m_viewport_width;
		}

		u16 viewport_origin_x() const
		{
			return m_viewport_origin_x;
		}

		u16 viewport_height() const
		{
			return m_viewport_height;
		}

		u16 viewport_origin_y() const
		{
			return m_viewport_origin_y;
		}

		u16 scissor_origin_x() const
		{
			return m_scissor_origin_x;
		}

		u16 scissor_width() const
		{
			return m_scissor_width;
		}

		u16 scissor_origin_y() const
		{
			return m_scissor_origin_y;
		}

		u16 scissor_height() const
		{
			return m_scissor_height;
		}

		window_origin shader_window_origin() const
		{
			return m_shader_window_origin;
		}

		window_pixel_center shader_window_pixel() const
		{
			return m_shader_window_pixel;
		}

		u16 shader_window_height() const
		{
			return m_shader_window_height;
		}

		u16 shader_window_offset_x() const
		{
			return m_shader_window_offset_x;
		}

		u16 shader_window_offset_y() const
		{
			return m_shader_window_offset_y;
		}

		bool depth_test_enabled() const
		{
			return m_depth_test_enabled;
		}

		bool depth_write_enabled() const
		{
			return m_depth_write_enabled;
		}

		bool alpha_test_enabled() const
		{
			return m_alpha_test_enabled;
		}

		bool stencil_test_enabled() const
		{
			return m_stencil_test_enabled;
		}

		bool restart_index_enabled() const
		{
			return m_restart_index_enabled;
		}

		u32 restart_index() const
		{
			return m_restart_index;
		}

		u32 z_clear_value() const
		{
			return m_z_clear_value;
		}

		u8 stencil_clear_value() const
		{
			return m_stencil_clear_value;
		}

		f32 fog_params_0() const
		{
			return m_fog_params_0;
		}

		f32 fog_params_1() const
		{
			return m_fog_params_1;
		}

		u8 index_array_location() const
		{
			return m_index_array_location;
		}

		rsx::index_array_type index_type() const
		{
			return m_index_type;
		}

		bool color_mask_b() const
		{
			return m_color_mask_b;
		}

		bool color_mask_g() const
		{
			return m_color_mask_g;
		}

		bool color_mask_r() const
		{
			return m_color_mask_r;
		}

		bool color_mask_a() const
		{
			return m_color_mask_a;
		}

		u8 clear_color_b() const
		{
			return m_clear_color_b;
		}

		u8 clear_color_r() const
		{
			return m_clear_color_r;
		}

		u8 clear_color_g() const
		{
			return m_clear_color_g;
		}

		u8 clear_color_a() const
		{
			return m_clear_color_a;
		}

		bool depth_bounds_test_enabled() const
		{
			return m_depth_bounds_test_enabled;
		}

		f32 depth_bounds_min() const
		{
			return m_depth_bounds_min;
		}

		f32 depth_bounds_max() const
		{
			return m_depth_bounds_max;
		}

		f32 clip_min() const
		{
			return m_clip_min;
		}

		f32 clip_max() const
		{
			return m_clip_max;
		}

		bool logic_op_enabled() const
		{
			return m_logic_op_enabled;
		}

		u8 stencil_mask() const
		{
			return m_stencil_mask;
		}

		u8 back_stencil_mask() const
		{
			return m_back_stencil_mask;
		}

		bool dither_enabled() const
		{
			return m_dither_enabled;
		}

		bool blend_enabled() const
		{
			return m_blend_enabled;
		}

		bool blend_enabled_surface_1() const
		{
			return m_blend_enabled_surface_1;
		}

		bool blend_enabled_surface_2() const
		{
			return m_blend_enabled_surface_2;
		}

		bool blend_enabled_surface_3() const
		{
			return m_blend_enabled_surface_3;
		}

		bool line_smooth_enabled() const
		{
			return m_line_smooth_enabled;
		}

		bool poly_offset_point_enabled() const
		{
			return m_poly_offset_point_enabled;
		}

		bool poly_offset_line_enabled() const
		{
			return m_offset_line_enabled;
		}

		bool poly_offset_fill_enabled() const
		{
			return m_poly_offset_fill_enabled;
		}

		f32 poly_offset_scale() const
		{
			return m_poly_offset_scale;
		}

		f32 poly_offset_bias() const
		{
			return m_poly_offset_bias;
		}

		bool cull_face_enabled() const
		{
			return m_cull_face_enabled;
		}

		bool poly_smooth_enabled() const
		{
			return m_poly_smooth_enabled;
		}

		bool two_sided_stencil_test_enabled() const
		{
			return m_two_sided_stencil_test_enabled;
		}

		comparaison_function depth_func() const
		{
			return m_depth_func;
		}

		comparaison_function stencil_func() const
		{
			return m_stencil_func;
		}

		comparaison_function back_stencil_func() const
		{
			return m_back_stencil_func;
		}

		u8 stencil_func_ref() const
		{
			return m_stencil_func_ref;
		}

		u8 back_stencil_func_ref() const
		{
			return m_back_stencil_func_ref;
		}

		u8 stencil_func_mask() const
		{
			return m_stencil_func_mask;
		}

		u8 back_stencil_func_mask() const
		{
			return m_back_stencil_func_mask;
		}

		stencil_op stencil_op_fail() const
		{
			return m_stencil_op_fail;
		}

		stencil_op stencil_op_zfail() const
		{
			return m_stencil_op_zfail;
		}

		stencil_op stencil_op_zpass() const
		{
			return m_stencil_op_zpass;
		}

		stencil_op back_stencil_op_fail() const
		{
			return m_back_stencil_op_fail;
		}

		rsx::stencil_op back_stencil_op_zfail() const
		{
			return m_back_stencil_op_zfail;
		}

		rsx::stencil_op back_stencil_op_zpass() const
		{
			return m_back_stencil_op_zpass;
		}

		u8 blend_color_8b_r() const
		{
			return m_blend_color & 0xff;
		}

		u8 blend_color_8b_g() const
		{
			return (m_blend_color >> 8) & 0xff;
		}

		u8 blend_color_8b_b() const
		{
			return (m_blend_color >> 16) & 0xff;
		}

		u8 blend_color_8b_a() const
		{
			return (m_blend_color >> 24) & 0xff;
		}

		u16 blend_color_16b_r() const
		{
			return m_blend_color & 0xffff;
		}

		u16 blend_color_16b_g() const
		{
			return (m_blend_color >> 16) & 0xffff;
		}

		u16 blend_color_16b_b() const
		{
			return m_blend_color_16b_b;
		}

		u16 blend_color_16b_a() const
		{
			return m_blend_color_16b_a;
		}

		blend_equation blend_equation_rgb() const
		{
			return m_blend_equation_rgb;
		}

		blend_equation blend_equation_a() const
		{
			return m_blend_equation_a;
		}

		blend_factor blend_func_sfactor_rgb() const
		{
			return m_blend_func_sfactor_rgb;
		}

		blend_factor blend_func_sfactor_a() const
		{
			return m_blend_func_sfactor_a;
		}

		blend_factor blend_func_dfactor_rgb() const
		{
			return m_blend_func_dfactor_rgb;
		}

		blend_factor blend_func_dfactor_a() const
		{
			return m_blend_func_dfactor_a;
		}

		logic_op logic_operation() const
		{
			return m_logic_operation;
		}

		user_clip_plane_op clip_plane_0_enabled() const
		{
			return m_clip_plane_0_enabled;
		}

		user_clip_plane_op clip_plane_1_enabled() const
		{
			return m_clip_plane_1_enabled;
		}

		user_clip_plane_op clip_plane_2_enabled() const
		{
			return m_clip_plane_2_enabled;
		}

		user_clip_plane_op clip_plane_3_enabled() const
		{
			return m_clip_plane_3_enabled;
		}

		user_clip_plane_op clip_plane_4_enabled() const
		{
			return m_clip_plane_4_enabled;
		}

		user_clip_plane_op clip_plane_5_enabled() const
		{
			return m_clip_plane_5_enabled;
		}

		front_face front_face_mode() const
		{
			return m_front_face_mode;
		}

		cull_face cull_face_mode() const
		{
			return m_cull_face_mode;
		}

		f32 line_width() const
		{
			return m_line_width;
		}

		u8 alpha_ref() const
		{
			return m_alpha_ref;
		}

		surface_target surface_color_target()
		{
			return m_surface_color_target;
		}

		u16 surface_clip_origin_x() const
		{
			return m_surface_clip_origin_x;
		}

		u16 surface_clip_width() const
		{
			return m_surface_clip_width;
		}

		u16 surface_clip_origin_y() const
		{
			return m_surface_clip_origin_y;
		}

		u16 surface_clip_height() const
		{
			return m_surface_clip_height;
		}

		u32 surface_a_offset() const
		{
			return m_surface_a_offset;
		}

		u32 surface_b_offset() const
		{
			return m_surface_b_offset;
		}

		u32 surface_c_offset() const
		{
			return m_surface_c_offset;
		}

		u32 surface_d_offset() const
		{
			return m_surface_d_offset;
		}

		u32 surface_a_pitch() const
		{
			return m_surface_a_pitch;
		}

		u32 surface_b_pitch() const
		{
			return m_surface_b_pitch;
		}

		u32 surface_c_pitch() const
		{
			return m_surface_c_pitch;
		}

		u32 surface_d_pitch() const
		{
			return m_surface_d_pitch;
		}

		u32 surface_a_dma() const
		{
			return m_surface_a_dma;
		}

		u32 surface_b_dma() const
		{
			return m_surface_b_dma;
		}

		u32 surface_c_dma() const
		{
			return m_surface_c_dma;
		}

		u32 surface_d_dma() const
		{
			return m_surface_d_dma;
		}

		u32 surface_z_offset() const
		{
			return m_surface_z_offset;
		}

		u32 surface_z_pitch() const
		{
			return m_surface_z_pitch;
		}

		u32 surface_z_dma() const
		{
			return m_surface_z_dma;
		}

		f32 viewport_scale_x() const
		{
			return m_viewport_scale_x;
		}

		f32 viewport_scale_y() const
		{
			return m_viewport_scale_y;
		}

		f32 viewport_scale_z() const
		{
			return m_viewport_scale_z;
		}

		f32 viewport_scale_w() const
		{
			return m_viewport_scale_w;
		}

		f32 viewport_offset_x() const
		{
			return m_viewport_offset_x;
		}

		f32 viewport_offset_y() const
		{
			return m_viewport_offset_y;
		}

		f32 viewport_offset_z() const
		{
			return m_viewport_offset_z;
		}

		f32 viewport_offset_w() const
		{
			return m_viewport_offset_w;
		}

		bool two_side_light_en() const
		{
			return m_two_side_light_enabled;
		}

		fog_mode fog_equation() const
		{
			return m_fog_equation;
		}

		comparaison_function alpha_func() const
		{
			return m_alpha_func;
		}

		u16 vertex_attrib_input_mask() const
		{
			return m_vertex_attrib_input_mask;
		}

		u16 frequency_divider_operation_mask() const
		{
			return m_frequency_divider_operation_mask;
		}

		u32 vertex_attrib_output_mask() const
		{
			return m_vertex_attrib_output_mask;
		}

		u32 shader_control() const
		{
			return m_shader_control;
		}

		surface_color_format surface_color() const
		{
			return m_surface_color;
		}

		surface_depth_format surface_depth_fmt() const
		{
			return m_surface_depth_format;
		}

		surface_antialiasing surface_antialias() const
		{
			return m_surface_antialias;
		}

		u8 surface_log2_height() const
		{
			return m_surface_log2_height;
		}

		u8 surface_log2_width() const
		{
			return m_surface_log2_width;
		}

		u32 vertex_data_base_offset() const
		{
			return m_vertex_data_base_offset;
		}

		u32 index_array_address() const
		{
			return m_index_array_address;
		}

		u32 vertex_data_base_index() const
		{
			return m_vertex_data_base_index;
		}

		u32 shader_program_address() const
		{
			return m_shader_program_address;
		}

		u32 transform_program_start() const
		{
			return m_transform_program_start;
		}

		primitive_type primitive_mode() const
		{
			return m_primitive_type;
		}

		u32 semaphore_offset_406e() const
		{
			return m_semaphore_offset_406e;
		}

		u32 semaphore_offset_4097() const
		{
			return m_semaphore_offset_4097;
		}

		blit_engine::context_dma context_dma_report() const
		{
			return m_context_dma_report;
		}

		blit_engine::transfer_operation blit_engine_operation() const
		{
			return m_blit_engine_operation;
		}

		/// TODO: find the purpose vs in/out equivalents
		u16 blit_engine_clip_x() const
		{
			return m_blit_engine_clip_x;
		}

		u16 blit_engine_clip_y() const
		{
			return m_blit_engine_clip_y;
		}

		u16 blit_engine_clip_width() const
		{
			return m_blit_engine_width;
		}

		u16 blit_engine_clip_height() const
		{
			return m_blit_engine_height;
		}

		u16 blit_engine_output_x() const
		{
			return m_blit_engine_output_x;
		}

		u16 blit_engine_output_y() const
		{
			return m_blit_engine_output_y;
		}

		u16 blit_engine_output_width() const
		{
			return m_blit_engine_output_width;
		}

		u16 blit_engine_output_height() const
		{
			return m_blit_engine_output_height;
		}

		// there is no x/y ?
		u16 blit_engine_input_width() const
		{
			return m_blit_engine_input_width;
		}

		u16 blit_engine_input_height() const
		{
			return m_blit_engine_input_height;
		}

		u16 blit_engine_input_pitch() const
		{
			return m_blit_engine_input_pitch;
		}

		blit_engine::transfer_origin blit_engine_input_origin() const
		{
			return m_blit_engine_input_origin;
		}

		blit_engine::transfer_interpolator blit_engine_input_inter() const
		{
			return m_blit_engine_input_inter;
		}

		blit_engine::transfer_source_format blit_engine_src_color_format() const
		{
			return m_blit_engine_src_color_format;
		}

		// ???
		f32 blit_engine_in_x() const
		{
			return m_blit_engine_in_x / 16.f;
		}

		// ???
		f32 blit_engine_in_y() const
		{
			return m_blit_engine_in_y / 16.f;
		}

		u32 blit_engine_input_offset() const
		{
			return m_blit_engine_input_offset;
		}

		u32 blit_engine_input_location() const
		{
			return m_blit_engine_input_location;
		}

		blit_engine::context_surface blit_engine_context_surface() const
		{
			return m_blit_engine_context_surface;
		}

		u32 blit_engine_output_location_nv3062() const
		{
			return m_blit_engine_output_location_nv3062;
		}

		u32 blit_engine_output_offset_nv3062() const
		{
			return m_blit_engine_output_offset_nv3062;
		}

		blit_engine::transfer_destination_format blit_engine_nv3062_color_format() const
		{
			return m_blit_engine_nv3062_color_format;
		}

		u16 blit_engine_output_alignment_nv3062() const
		{
			return m_blit_engine_output_alignement_nv3062;
		}

		u16 blit_engine_output_pitch_nv3062() const
		{
			return m_blit_engine_output_pitch_nv3062;
		}

		u32 blit_engine_nv309E_location() const
		{
			return m_blit_engine_nv309E_location;
		}

		u32 blit_engine_nv309E_offset() const
		{
			return m_blit_engine_nv309E_offset;
		}

		blit_engine::transfer_destination_format blit_engine_output_format_nv309E() const
		{
			return m_blit_engine_output_format_nv309E;
		}

		u32 blit_engine_ds_dx() const
		{
			return m_blit_engine_ds_dx;
		}

		u32 blit_engine_dt_dy() const
		{
			return m_blit_engine_dt_dy;
		}

		u8 nv309e_sw_width_log2() const
		{
			return m_nv309e_sw_width_log2;
		}

		u8 nv309e_sw_height_log2() const
		{
			return m_nv309e_sw_height_log2;
		}

		u32 nv0039_input_pitch() const
		{
			return m_nv0039_input_pitch;
		}

		u32 nv0039_output_pitch() const
		{
			return m_nv0039_output_pitch;
		}

		u32 nv0039_line_length() const
		{
			return m_nv0039_line_length;
		}

		u32 nv0039_line_count() const
		{
			return m_nv0039_line_count;
		}

		u8 nv0039_output_format() const
		{
			return m_nv0039_output_format;
		}

		u8 nv0039_input_format() const
		{
			return m_nv0039_input_format;
		}

		u32 nv0039_output_offset() const
		{
			return m_nv0039_output_offset;
		}

		u32 nv0039_output_location()
		{
			return m_nv0039_output_location;
		}

		u32 nv0039_input_offset() const
		{
			return m_nv0039_input_offset;
		}

		u32 nv0039_input_location() const
		{
			return m_nv0039_input_location;
		}

		u16 nv308a_x() const
		{
			return m_nv308a_x;
		}

		u16 nv308a_y() const
		{
			return m_nv308a_y;
		}
	};

	using rsx_method_t = void(*)(class thread*, u32);
	extern rsx_state method_registers;
	extern rsx_method_t methods[0x10000 >> 2];
}
