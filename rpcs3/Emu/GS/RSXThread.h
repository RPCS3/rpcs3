#pragma once
#include "GCM.h"

class ExecRSXCMDdata
{
public:
	bool m_set_color_mask;
	bool m_color_mask_r;
	bool m_color_mask_g;
	bool m_color_mask_b;
	bool m_color_mask_a;

	bool m_set_clip;
	float m_clip_min;
	float m_clip_max;

	bool m_set_depth_func;
	int m_depth_func;

	bool m_set_alpha_test;
	bool m_set_blend;
	bool m_set_depth_bounds_test;
	bool m_depth_test_enable;
	bool m_set_logic_op;
	bool m_set_cull_face_enable;
	bool m_set_dither;
	bool m_set_stencil_test;
	bool m_set_line_smooth;
	bool m_set_poly_smooth;

	bool m_set_viewport_horizontal;
	bool m_set_viewport_vertical;
	u16 m_viewport_x;
	u16 m_viewport_y;
	u16 m_viewport_w;
	u16 m_viewport_h;

	bool m_set_scissor_horizontal;
	bool m_set_scissor_vertical;
	u16 m_scissor_x;
	u16 m_scissor_y;
	u16 m_scissor_w;
	u16 m_scissor_h;

	bool m_set_front_polygon_mode;
	u32 m_front_polygon_mode;

	bool m_set_clear_surface;
	u32 m_clear_surface_mask;

	bool m_set_blend_sfactor;
	u16 m_blend_sfactor_rgb;
	u16 m_blend_sfactor_alpha;

	bool m_set_blend_dfactor;
	u16 m_blend_dfactor_rgb;
	u16 m_blend_dfactor_alpha;

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

	bool m_set_blend_equation;
	u16 m_blend_equation_rgb;
	u16 m_blend_equation_alpha;

	bool m_set_depth_mask;
	u32 m_depth_mask;

	bool m_set_line_width;
	u32 m_line_width;

	bool m_set_shade_mode;
	u32 m_shade_mode;

	bool m_set_blend_color;
	u8 m_blend_color_r;
	u8 m_blend_color_g;
	u8 m_blend_color_b;
	u8 m_blend_color_a;

	bool m_set_clear_color;
	u8 m_clear_color_r;
	u8 m_clear_color_g;
	u8 m_clear_color_b;
	u8 m_clear_color_a;

	u32 m_context_dma_img_src;
	u32 m_context_dma_img_dst;
	u32 m_dst_offset;
	u32 m_color_format;
	u16 m_color_format_src_pitch;
	u16 m_color_format_dst_pitch;

	u32 m_color_conv;
	u32 m_color_conv_fmt;
	u32 m_color_conv_op;
	u16 m_color_conv_in_x;
	u16 m_color_conv_in_y;
	u16 m_color_conv_in_w;
	u16 m_color_conv_in_h;
	u16 m_color_conv_out_x;
	u16 m_color_conv_out_y;
	u16 m_color_conv_out_w;
	u16 m_color_conv_out_h;
	u32 m_color_conv_dsdx;
	u32 m_color_conv_dtdy;

	bool m_set_semaphore_offset;
	u32 m_semaphore_offset;

	bool m_set_fog_mode;
	u32 m_fog_mode;

	bool m_set_fog_params;
	float m_fog_param0;
	float m_fog_param1;

	bool m_set_clip_plane;
	u32 m_clip_plane_0;
	u32 m_clip_plane_1;
	u32 m_clip_plane_2;
	u32 m_clip_plane_3;
	u32 m_clip_plane_4;
	u32 m_clip_plane_5;

	bool m_set_surface_format;
	u8 m_surface_color_format;
	u8 m_surface_depth_format;
	u8 m_surface_type;
	u8 m_surface_antialias;
	u8 m_surface_width;
	u8 m_surface_height;
	u32 m_surface_pitch_a;
	u32 m_surface_offset_a;
	u32 m_surface_offset_z;
	u32 m_surface_offset_b;
	u32 m_surface_pitch_b;

	bool m_set_context_dma_color_a;
	u32 m_context_dma_color_a;

	bool m_set_context_dma_color_b;
	u32 m_context_dma_color_b;

	bool m_set_context_dma_color_c;
	u32 m_context_dma_color_c;

	bool m_set_context_dma_z;
	u32 m_context_dma_z;

	bool m_set_surface_clip_horizontal;
	u16 m_surface_clip_x;
	u16 m_surface_clip_w;
	bool m_set_surface_clip_vertical;
	u16 m_surface_clip_y;
	u16 m_surface_clip_h;

	bool m_set_cull_face;
	u32 m_cull_face;

	bool m_set_alpha_func;
	u32 m_alpha_func;

	bool m_set_alpha_ref;
	u32 m_alpha_ref;

	u8 m_begin_end;

public:
	ExecRSXCMDdata()
	{
		Reset();
	}

	virtual void Reset()
	{
		m_set_color_mask = false;
		m_set_alpha_test = false;
		m_set_blend = false;
		m_set_depth_bounds_test = false;
		m_set_clip = false;
		m_set_depth_func = false;
		m_depth_test_enable = false;
		m_set_viewport_horizontal = false;
		m_set_viewport_vertical = false;
		m_set_scissor_horizontal = false;
		m_set_scissor_vertical = false;
		m_set_front_polygon_mode = false;
		m_set_clear_surface = false;
		m_set_blend_sfactor = false;
		m_set_blend_dfactor = false;
		m_set_logic_op = false;
		m_set_cull_face_enable = false;
		m_set_dither = false;
		m_set_stencil_test = false;
		m_set_stencil_mask = false;
		m_set_stencil_func = false;
		m_set_stencil_func_ref = false;
		m_set_stencil_func_mask = false;
		m_set_stencil_fail = false;
		m_set_stencil_zfail = false;
		m_set_stencil_zpass = false;
		m_set_two_sided_stencil_test_enable = false;
		m_set_back_stencil_mask = false;
		m_set_back_stencil_func = false;
		m_set_back_stencil_func_ref = false;
		m_set_back_stencil_func_mask = false;
		m_set_back_stencil_fail = false;
		m_set_back_stencil_zfail = false;
		m_set_back_stencil_zpass = false;
		m_set_blend_equation = false;
		m_set_depth_mask = false;
		m_set_line_smooth = false;
		m_set_poly_smooth = false;
		m_set_line_width = false;
		m_set_shade_mode = false;
		m_set_blend_color = false;
		m_set_clear_color = false;
		m_set_semaphore_offset = false;
		m_set_fog_mode = false;
		m_set_fog_params = false;
		m_set_clip_plane = false;
		m_set_surface_format = false;
		m_set_context_dma_color_a = false;
		m_set_context_dma_color_b = false;
		m_set_context_dma_color_c = false;
		m_set_context_dma_z = false;
		m_set_surface_clip_horizontal = false;
		m_set_surface_clip_vertical = false;
		m_set_cull_face = false;
		m_set_alpha_func = false;
		m_set_alpha_ref = false;
		m_begin_end = 0;
	}

	virtual void ExecCMD()=0;
};

class RSXThread : public ThreadBase
{
	Array<u32> call_stack;
	CellGcmControl& m_ctrl;
	u32 m_ioAddress;

protected:
	RSXThread(CellGcmControl* ctrl, u32 ioAddress);

private:
	virtual void Task();
	virtual void OnExit();
};