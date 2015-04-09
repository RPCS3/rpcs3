#pragma once
#include "GCM.h"
#include "RSXTexture.h"
#include "RSXVertexProgram.h"
#include "RSXFragmentProgram.h"

#include <stack>
#include "Utilities/SSemaphore.h"
#include "Utilities/Thread.h"
#include "Utilities/Timer.h"
#include "types.h"

extern u32 methodRegisters[0xffff];
u32 GetAddress(u32 offset, u32 location);
u32 LinearToSwizzleAddress(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth);

struct RSXVertexData
{
	u32 frequency;
	u32 stride;
	u32 size;
	u32 type;
	u32 addr;
	u32 constant_count;

	std::vector<u8> data;

	RSXVertexData();

	void Reset();
	bool IsEnabled() const { return size > 0; }
	void Load(u32 start, u32 count, u32 baseOffset, u32 baseIndex);

	u32 GetTypeSize();
};

struct RSXIndexArrayData
{
	std::vector<u8> m_data;
	int m_type;
	u32 m_first;
	u32 m_count;
	u32 m_addr;
	u32 index_max;
	u32 index_min;

	RSXIndexArrayData()
	{
		Reset();
	}

	void Reset()
	{
		m_type = 0;
		m_first = ~0;
		m_count = 0;
		m_addr = 0;
		index_min = ~0;
		index_max = 0;
		m_data.clear();
	}
};

class RSXThread : public ThreadBase
{
public:
	static const uint m_textures_count = 16;
	static const uint m_vertex_count = 16;
	static const uint m_fragment_count = 32;
	static const uint m_tiles_count = 15;
	static const uint m_zculls_count = 8;

protected:
	std::stack<u32> m_call_stack;
	CellGcmControl* m_ctrl;
	Timer m_timer_sync;

public:
	GcmTileInfo m_tiles[m_tiles_count];
	GcmZcullInfo m_zculls[m_zculls_count];
	RSXTexture m_textures[m_textures_count];
	RSXVertexTexture m_vertex_textures[m_textures_count];
	RSXVertexData m_vertex_data[m_vertex_count];
	RSXIndexArrayData m_indexed_array;
	std::unordered_map<u32, color4_base<f32>> m_fragment_constants;
	std::unordered_map<u32, color4_base<f32>> m_transform_constants;
	
	u32 m_shader_ctrl, m_cur_fragment_prog_num;
	u32 m_vertex_program_data[512 * 4] = {};
	RSXFragmentProgram m_fragment_progs[m_fragment_count];
	RSXFragmentProgram* m_cur_fragment_prog;
	//RSXVertexProgram m_vertex_progs[m_vertex_count];
	//RSXVertexProgram* m_cur_vertex_prog;

public:
	u32 m_ioAddress, m_ioSize, m_ctrlAddress;
	int m_flip_status;
	int m_flip_mode;
	int m_debug_level;
	int m_frequency_mode;

	u32 m_tiles_addr;
	u32 m_zculls_addr;
	vm::ptr<CellGcmDisplayInfo> m_gcm_buffers;
	u32 m_gcm_buffers_count;
	u32 m_gcm_current_buffer;
	u32 m_ctxt_addr;
	u32 m_report_main_addr;
	u32 m_label_addr;

	// DMA
	u32 dma_report;

	u32 m_local_mem_addr, m_main_mem_addr;
	bool m_strict_ordering[0x1000];

public:
	uint m_draw_mode;

	u32 m_width;
	u32 m_height;
	u32 m_pitch = 4;
	u32 m_address = 0;
	u32 m_draw_array_count;
	u32 m_draw_array_first;
	double m_fps_limit = 59.94;

public:
	std::mutex m_cs_main;
	SSemaphore m_sem_flush;
	SSemaphore m_sem_flip;
	u64 m_last_flip_time;
	vm::ptr<void(u32)> m_flip_handler;
	vm::ptr<void(u32)> m_user_handler;
	u64 m_vblank_count;
	vm::ptr<void(u32)> m_vblank_handler;

public:
	// Dither
	bool m_set_dither;

	// Color mask
	bool m_set_color_mask;
	bool m_color_mask_r;
	bool m_color_mask_g;
	bool m_color_mask_b;
	bool m_color_mask_a;

	// Clip 
	bool m_set_clip;
	float m_clip_min;
	float m_clip_max;

	// Depth test
	bool m_set_depth_test;
	bool m_set_depth_func;
	int m_depth_func;
	bool m_set_depth_mask;
	u32 m_depth_mask;

	// Depth bound test
	bool m_set_depth_bounds_test;
	bool m_set_depth_bounds;
	float m_depth_bounds_min;
	float m_depth_bounds_max;

	// Primitive restart 
	bool m_set_restart_index;
	u32 m_restart_index;

	// Point 
	bool m_set_point_size;
	bool m_set_point_sprite_control;
	float m_point_size;
	u16 m_point_x;
	u16 m_point_y;

	// Line smooth
	bool m_set_line_smooth;

	// Viewport & scissor
	bool m_set_viewport_horizontal;
	bool m_set_viewport_vertical;
	u16 m_viewport_x;
	u16 m_viewport_y;
	u16 m_viewport_w;
	u16 m_viewport_h;
	bool m_set_scissor_horizontal;
	bool m_set_scissor_vertical;
	u16 m_scissor_x = 0;
	u16 m_scissor_y = 0;
	u16 m_scissor_w = 0;
	u16 m_scissor_h = 0;

	// Polygon mode/offset
	bool m_set_poly_smooth;
	bool m_set_poly_offset_fill;
	bool m_set_poly_offset_line;
	bool m_set_poly_offset_point;
	bool m_set_front_polygon_mode;
	u32 m_front_polygon_mode;
	bool m_set_back_polygon_mode;
	u32 m_back_polygon_mode;
	bool m_set_poly_offset_mode;
	float m_poly_offset_scale_factor;
	float m_poly_offset_bias;

	// Line/Polygon stipple
	bool m_set_line_stipple;
	u16 m_line_stipple_pattern;
	u16 m_line_stipple_factor;
	bool m_set_polygon_stipple;
	u32 m_polygon_stipple_pattern[32];

	// Logic Ops
	bool m_set_logic_op;
	u32 m_logic_op;
	
	// Clearing
	u32 m_clear_surface_mask;
	u32 m_clear_surface_z;
	u8 m_clear_surface_s;
	u8 m_clear_surface_color_r;
	u8 m_clear_surface_color_g;
	u8 m_clear_surface_color_b;
	u8 m_clear_surface_color_a;
	u8 m_clear_color_r;
	u8 m_clear_color_g;
	u8 m_clear_color_b;
	u8 m_clear_color_a;
	u8 m_clear_s;
	u32 m_clear_z;

	// Blending
	bool m_set_blend;
	bool m_set_blend_dfactor;
	u16 m_blend_dfactor_rgb;
	u16 m_blend_dfactor_alpha;
	bool m_set_blend_sfactor;
	u16 m_blend_sfactor_rgb;
	u16 m_blend_sfactor_alpha;
	bool m_set_blend_equation;
	u16 m_blend_equation_rgb;
	u16 m_blend_equation_alpha;
	bool m_set_blend_color;
	u8 m_blend_color_r;
	u8 m_blend_color_g;
	u8 m_blend_color_b;
	u8 m_blend_color_a;
	bool m_set_blend_mrt1;
	bool m_set_blend_mrt2;
	bool m_set_blend_mrt3;

	// Stencil Test
	bool m_set_stencil_test;
	bool m_set_stencil_mask;
	u32 m_stencil_mask;
	bool m_set_stencil_func;
	u32 m_stencil_func;
	bool m_set_stencil_func_ref;
	u32 m_stencil_func_ref;
	bool m_set_stencil_func_mask;
	u32 m_stencil_func_mask;
	bool m_set_stencil_fail;
	u32 m_stencil_fail;
	bool m_set_stencil_zfail;
	u32 m_stencil_zfail;
	bool m_set_stencil_zpass;
	u32 m_stencil_zpass;
	bool m_set_two_sided_stencil_test_enable;
	bool m_set_two_side_light_enable;
	bool m_set_back_stencil_mask;
	u32 m_back_stencil_mask;
	bool m_set_back_stencil_func;
	u32 m_back_stencil_func;
	bool m_set_back_stencil_func_ref;
	u32 m_back_stencil_func_ref;
	bool m_set_back_stencil_func_mask;
	u32 m_back_stencil_func_mask;
	bool m_set_back_stencil_fail;
	u32 m_back_stencil_fail;
	bool m_set_back_stencil_zfail;
	u32 m_back_stencil_zfail;
	bool m_set_back_stencil_zpass;
	u32 m_back_stencil_zpass;
	bool m_set_stencil_op_fail;

	// Line width
	bool m_set_line_width;
	float m_line_width;

	// Shader mode
	bool m_set_shade_mode;
	u32 m_shade_mode;

	// Lighting 
	bool m_set_specular;

	// Color
	u32 m_color_format;
	u16 m_color_format_src_pitch;
	u16 m_color_format_dst_pitch;
	u32 m_color_conv;
	u32 m_color_conv_fmt;
	u32 m_color_conv_op;
	s16 m_color_conv_clip_x;
	s16 m_color_conv_clip_y;
	u16 m_color_conv_clip_w;
	u16 m_color_conv_clip_h;
	s16 m_color_conv_out_x;
	s16 m_color_conv_out_y;
	u16 m_color_conv_out_w;
	u16 m_color_conv_out_h;
	s32 m_color_conv_dsdx;
	s32 m_color_conv_dtdy;

	// Semaphore
	bool m_set_semaphore_offset;
	u32 m_semaphore_offset;

	// Fog
	bool m_set_fog_mode;
	u32 m_fog_mode;
	bool m_set_fog_params;
	float m_fog_param0;
	float m_fog_param1;

	// Clip plane
	bool m_set_clip_plane;
	bool m_clip_plane_0;
	bool m_clip_plane_1;
	bool m_clip_plane_2;
	bool m_clip_plane_3;
	bool m_clip_plane_4;
	bool m_clip_plane_5;

	// Surface 
	bool m_set_surface_format;
	u8 m_surface_color_format;
	u8 m_surface_depth_format;
	u8 m_surface_type;
	u8 m_surface_antialias;
	u8 m_surface_width;
	u8 m_surface_height;
	bool m_set_surface_clip_horizontal;
	u16 m_surface_clip_x;
	u16 m_surface_clip_w;
	bool m_set_surface_clip_vertical;
	u16 m_surface_clip_y;
	u16 m_surface_clip_h;
	u32 m_surface_pitch[4] = {};
	u32 m_surface_pitch_z = 0;
	u32 m_surface_offset[4] = {};
	u32 m_surface_offset_z = 0;
	u32 m_surface_color_target = 0;

	// DMA context
	u32 m_context_dma_color[4] = {};
	u32 m_context_dma_z;
	u32 m_context_surface;
	u32 m_context_dma_img_src = 0xfeed0000;
	u32 m_context_dma_img_dst = 0xfeed0000;
	u32 m_context_dma_buffer_in_src;
	u32 m_context_dma_buffer_in_dst;
	u32 m_dst_offset;

	// Swizzle2D?
	u16 m_swizzle_format;
	u8 m_swizzle_width;
	u8 m_swizzle_height;
	u32 m_swizzle_offset;

	// Cull face
	bool m_set_cull_face;
	u32 m_cull_face = 0x0405;//GL_BACK

	// Alpha test
	bool m_set_alpha_test;
	bool m_set_alpha_func;
	u32 m_alpha_func;
	bool m_set_alpha_ref;
	float m_alpha_ref;

	// Shader
	u16 m_shader_window_height;
	u8 m_shader_window_origin;
	u16 m_shader_window_pixel_centers;

	// Vertex Data
	u32 m_vertex_data_base_offset;
	u32 m_vertex_data_base_index;

	// Front face
	u32 m_front_face = 0x0900; //GL_CW

	// Frequency divider
	u32 m_set_frequency_divider_operation;

	u8 m_begin_end;
	bool m_read_buffer;

	std::set<u32> m_used_gcm_commands;

protected:
	RSXThread()
		: ThreadBase("RSXThread")
		, m_ctrl(nullptr)
		, m_shader_ctrl(0x40)
		, m_flip_status(0)
		, m_flip_mode(CELL_GCM_DISPLAY_VSYNC)
		, m_debug_level(CELL_GCM_DEBUG_LEVEL0)
		, m_frequency_mode(CELL_GCM_DISPLAY_FREQUENCY_DISABLE)
		, m_report_main_addr(0)
		, m_main_mem_addr(0)
		, m_local_mem_addr(0)
		, m_draw_mode(0)
		, m_draw_array_count(0)
		, m_draw_array_first(~0)
		, m_gcm_current_buffer(0)
		, m_read_buffer(true)
	{
		m_flip_handler.set(0);
		m_vblank_handler.set(0);
		m_user_handler.set(0);
		m_set_depth_test = false;
		m_set_alpha_test = false;
		m_set_depth_bounds_test = false;
		m_set_blend = false;
		m_set_blend_mrt1 = false;
		m_set_blend_mrt2 = false;
		m_set_blend_mrt3 = false;
		m_set_logic_op = false;
		m_set_cull_face = false;
		m_set_dither = false;
		m_set_stencil_test = false;
		m_set_scissor_horizontal = false;
		m_set_scissor_vertical = false;
		m_set_line_smooth = false;
		m_set_poly_smooth = false;
		m_set_point_sprite_control = false;
		m_set_specular = false;
		m_set_two_sided_stencil_test_enable = false;
		m_set_two_side_light_enable = false;
		m_set_surface_clip_horizontal = false;
		m_set_surface_clip_vertical = false;
		m_set_poly_offset_fill = false;
		m_set_poly_offset_line = false;
		m_set_poly_offset_point = false;
		m_set_restart_index = false;
		m_set_line_stipple = false;
		m_set_polygon_stipple = false;

		// Default value 
		// TODO: Check against the default value on PS3 
		m_clear_color_r = 0;
		m_clear_color_g = 0;
		m_clear_color_b = 0;
		m_clear_color_a = 0;
		m_clear_z = 0xffffff;
		m_clear_s = 0;
		m_poly_offset_scale_factor = 0.0;
		m_poly_offset_bias = 0.0;
		m_restart_index = 0xffffffff;
		m_front_polygon_mode = 0x1b02; // GL_FILL
		m_back_polygon_mode = 0x1b02; // GL_FILL
		m_alpha_func = 0x0207; // GL_ALWAYS
		m_alpha_ref = 0.0f;
		m_logic_op = 0x1503; // GL_COPY
		m_shade_mode = 0x1D01; // GL_SMOOTH
		m_depth_mask = 1;
		m_depth_func = 0x0201; // GL_LESS
		m_depth_bounds_min = 0.0;
		m_depth_bounds_max = 1.0;
		m_clip_min = 0.0;
		m_clip_max = 1.0;
		m_blend_equation_rgb = 0x8006; // GL_FUNC_ADD
		m_blend_equation_alpha = 0x8006; // GL_FUNC_ADD
		m_blend_sfactor_rgb = 1; // GL_ONE
		m_blend_dfactor_rgb = 0; // GL_ZERO
		m_blend_sfactor_alpha = 1; // GL_ONE
		m_blend_dfactor_alpha = 0; // GL_ZERO
		m_point_x = 0;
		m_point_y = 0;
		m_point_size = 1.0;
		m_line_width = 1.0;
		m_line_stipple_pattern = 0xffff;
		m_line_stipple_factor = 1;
		m_vertex_data_base_offset = 0;
		m_vertex_data_base_index = 0;

		// Construct Stipple Pattern
		for (size_t i = 0; i < 32; i++)
		{
			m_polygon_stipple_pattern[i] = 0xFFFFFFFF;
		}

		// Construct Textures
		for (int i = 0; i < 16; i++)
		{
			m_textures[i] = RSXTexture(i);
		}

		Reset();
	}

	virtual ~RSXThread() {}

	void Reset()
	{
		m_set_dither = false;
		m_set_color_mask = false;
		m_set_clip = false;
		m_set_depth_test = false;
		m_set_depth_func = false;
		m_set_depth_mask = false;
		m_set_depth_bounds_test = false;
		m_set_depth_bounds = false;
		m_set_viewport_horizontal = false;
		m_set_viewport_vertical = false;
		m_set_scissor_horizontal = false;
		m_set_scissor_vertical = false;
		m_set_front_polygon_mode = false;
		m_set_back_polygon_mode = false;
		m_set_blend = false;
		m_set_blend_mrt1 = false;
		m_set_blend_mrt2 = false;
		m_set_blend_mrt3 = false;
		m_set_blend_sfactor = false;
		m_set_blend_dfactor = false;
		m_set_blend_equation = false;
		m_set_blend_color = false;
		m_set_stencil_test = false;
		m_set_two_sided_stencil_test_enable = false;
		m_set_two_side_light_enable = false;
		m_set_stencil_mask = false;
		m_set_stencil_func = false;
		m_set_stencil_func_ref = false;
		m_set_stencil_func_mask = false;
		m_set_stencil_fail = false;
		m_set_stencil_zfail = false;
		m_set_stencil_zpass = false;
		m_set_back_stencil_mask = false;
		m_set_back_stencil_func = false;
		m_set_back_stencil_func_ref = false;
		m_set_back_stencil_func_mask = false;
		m_set_back_stencil_fail = false;
		m_set_back_stencil_zfail = false;
		m_set_back_stencil_zpass = false;
		m_set_stencil_op_fail = false;
		m_set_point_sprite_control = false;
		m_set_point_size = false;
		m_set_line_width = false;
		m_set_line_smooth = false;
		m_set_shade_mode = false;
		m_set_semaphore_offset = false;
		m_set_fog_mode = false;
		m_set_fog_params = false;
		m_set_clip_plane = false;
		m_set_cull_face = false;
		m_set_alpha_test = false;
		m_set_alpha_func = false;
		m_set_alpha_ref = false;
		m_set_poly_smooth = false;
		m_set_poly_offset_fill = false;
		m_set_poly_offset_line = false;
		m_set_poly_offset_point = false;
		m_set_poly_offset_mode = false;
		m_set_restart_index = false;
		m_set_specular = false;
		m_set_line_stipple = false;
		m_set_polygon_stipple = false;
		m_set_logic_op = false;
		m_set_surface_format = false;
		m_set_surface_clip_horizontal = false;
		m_set_surface_clip_vertical = false;

		m_clear_surface_mask = 0;
		m_begin_end = 0;

		for (uint i = 0; i < m_textures_count; ++i)
		{
			m_textures[i].Init();
		}
	}

	void Begin(u32 draw_mode);
	void End();

	//void DoCmd(const u32 fcmd, const u32 cmd, const u32 args_addr, const u32 count);
	void update_reg(u32 reg, u32 value);

	virtual void OnInit() = 0;
	virtual void OnInitThread() = 0;
	virtual void OnExitThread() = 0;
	virtual void OnReset() = 0;
	virtual void ExecCMD() = 0;
	virtual void ExecCMD(u32 cmd) = 0;
	virtual void Flip(int buffer) = 0;

	void LoadVertexData(u32 first, u32 count)
	{
		for (u32 i = 0; i < m_vertex_count; ++i)
		{
			if (!m_vertex_data[i].IsEnabled()) continue;

			m_vertex_data[i].Load(first, count, m_vertex_data_base_offset, m_vertex_data_base_index);
		}
	}

	virtual void Task();

public:
	void Init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress);

	u32 ReadIO32(u32 addr);

	void WriteIO32(u32 addr, u32 value);
};
