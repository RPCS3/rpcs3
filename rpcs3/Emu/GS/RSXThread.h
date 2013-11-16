#pragma once
#include "GCM.h"
#include "RSXVertexProgram.h"
#include "RSXFragmentProgram.h"
#include "Emu/SysCalls/Callback.h"

enum Method
{
	CELL_GCM_METHOD_FLAG_NON_INCREMENT	= 0x40000000,
	CELL_GCM_METHOD_FLAG_JUMP			= 0x20000000,
	CELL_GCM_METHOD_FLAG_CALL			= 0x00000002,
	CELL_GCM_METHOD_FLAG_RETURN			= 0x00020000,
};


class RSXTexture
{
public:
	bool m_enabled;

	u32 m_width, m_height;
	u32 m_offset;

	bool m_cubemap;
	u8 m_dimension;
	u32 m_format;
	u16 m_mipmap;

	u32 m_pitch;
	u16 m_depth;

	u16 m_minlod;
	u16 m_maxlod;
	u8 m_maxaniso;

	u8 m_wraps;
	u8 m_wrapt;
	u8 m_wrapr;
	u8 m_unsigned_remap;
	u8 m_zfunc;
	u8 m_gamma;
	u8 m_aniso_bias;
	u8 m_signed_remap;

	u16 m_bias;
	u8 m_min_filter;
	u8 m_mag_filter;
	u8 m_conv;
	u8 m_a_signed;
	u8 m_r_signed;
	u8 m_g_signed;
	u8 m_b_signed;

	u32 m_remap;

public:
	RSXTexture()
		: m_width(0), m_height(0)
		, m_offset(0)
		, m_enabled(false)

		, m_cubemap(false)
		, m_dimension(0)
		, m_format(0)
		, m_mipmap(0)
		, m_minlod(0)
		, m_maxlod(1000)
		, m_maxaniso(0)
	{
	}

	void SetRect(const u32 width, const u32 height)
	{
		m_width = width;
		m_height = height;
	}

	void SetFormat(const bool cubemap, const u8 dimension, const u32 format, const u16 mipmap)
	{
		m_cubemap = cubemap;
		m_dimension = dimension;
		m_format = format;
		m_mipmap = mipmap;
	}

	void SetAddress(u8 wraps, u8 wrapt, u8 wrapr, u8 unsigned_remap, u8 zfunc, u8 gamma, u8 aniso_bias, u8 signed_remap)
	{
		m_wraps = wraps;
		m_wrapt = wrapt;
		m_wrapr = wrapr;
		m_unsigned_remap = unsigned_remap;
		m_zfunc = zfunc;
		m_gamma = gamma;
		m_aniso_bias = aniso_bias;
		m_signed_remap = signed_remap;
	}

	void SetControl0(const bool enable, const u16 minlod, const u16 maxlod, const u8 maxaniso)
	{
		m_enabled = enable;
		m_minlod = minlod;
		m_maxlod = maxlod;
		m_maxaniso = maxaniso;
	}

	void SetControl1(u32 remap)
	{
		m_remap = remap;
	}

	void SetControl3(u16 depth, u32 pitch)
	{
		m_depth = depth;
		m_pitch = pitch;
	}

	void SetFilter(u16 bias, u8 min, u8 mag, u8 conv, u8 a_signed, u8 r_signed, u8 g_signed, u8 b_signed)
	{
		m_bias = bias;
		m_min_filter = min;
		m_mag_filter = mag;
		m_conv = conv;
		m_a_signed = a_signed;
		m_r_signed = r_signed;
		m_g_signed = g_signed;
		m_b_signed = b_signed;
	}

	u32 GetFormat() const
	{
		return m_format;
	}

	void SetOffset(const u32 offset)
	{
		m_offset = offset;
	}

	wxSize GetRect() const
	{
		return wxSize(m_width, m_height);
	}

	bool IsEnabled() const
	{
		return m_enabled;
	}

	u32 GetOffset() const
	{
		return m_offset;
	}
};

struct RSXVertexData
{
	u32 frequency;
	u32 stride;
	u32 size;
	u32 type;
	u32 addr;
	u32 constant_count;

	Array<u8> data;

	RSXVertexData();

	void Reset();
	bool IsEnabled() { return size > 0; }
	void Load(u32 start, u32 count);

	u32 GetTypeSize();
};

struct RSXIndexArrayData
{
	Array<u8> m_data;
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
		m_data.Clear();
	}
};

struct RSXTransformConstant
{
	u32 id;
	float x, y, z, w;

	RSXTransformConstant()
		: x(0.0f)
		, y(0.0f)
		, z(0.0f)
		, w(0.0f)
	{
	}

	RSXTransformConstant(u32 id, float x, float y, float z, float w)
		: id(id)
		, x(x)
		, y(y)
		, z(z)
		, w(w)
	{
	}
};

class RSXThread  : public ThreadBase
{
public:
	static const uint m_textures_count = 16;
	static const uint m_vertex_count = 16;
	static const uint m_fragment_count = 16;
	static const uint m_tiles_count = 15;

protected:
	Stack<u32> m_call_stack;
	CellGcmControl* m_ctrl;

public:
	GcmTileInfo m_tiles[m_tiles_count];
	RSXTexture m_textures[m_textures_count];
	RSXVertexData m_vertex_data[m_vertex_count];
	RSXIndexArrayData m_indexed_array;
	Array<RSXTransformConstant> m_fragment_constants;
	Array<RSXTransformConstant> m_transform_constants;

	u32 m_cur_shader_prog_num;
	RSXShaderProgram m_shader_progs[m_fragment_count];
	RSXShaderProgram* m_cur_shader_prog;
	RSXVertexProgram m_vertex_progs[m_vertex_count];
	RSXVertexProgram* m_cur_vertex_prog;

public:
	u32 m_ioAddress, m_ioSize, m_ctrlAddress;
	int m_flip_status;
	int m_flip_mode;

	u32 m_tiles_addr;
	u32 m_zculls_addr;
	u32 m_gcm_buffers_addr;
	u32 m_gcm_buffers_count;
	u32 m_gcm_current_buffer;
	u32 m_ctxt_addr;
	u32 m_report_main_addr;

	u32 m_local_mem_addr, m_main_mem_addr;
	Array<MemInfo> m_main_mem_info;

public:
	uint m_draw_mode;

	u32 m_width, m_height;
	u32 m_draw_array_count;

public:
	wxCriticalSection m_cs_main;
	wxSemaphore m_sem_flush;
	wxSemaphore m_sem_flip;
	Callback m_flip_handler;

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

	u32 m_clear_surface_mask;
	u32 m_clear_surface_z;
	u8 m_clear_surface_s;
	u8 m_clear_surface_color_r;
	u8 m_clear_surface_color_g;
	u8 m_clear_surface_color_b;
	u8 m_clear_surface_color_a;

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

	u8 m_clear_color_r;
	u8 m_clear_color_g;
	u8 m_clear_color_b;
	u8 m_clear_color_a;
	u8 m_clear_s;
	u32 m_clear_z;

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

	bool m_set_context_dma_color_a;
	u32 m_context_dma_color_a;

	bool m_set_context_dma_color_b;
	u32 m_context_dma_color_b;

	bool m_set_context_dma_color_c;
	u32 m_context_dma_color_c;

	bool m_set_context_dma_color_d;
	u32 m_context_dma_color_d;

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

	u32 m_surface_pitch_a;
	u32 m_surface_pitch_b;
	u32 m_surface_pitch_c;
	u32 m_surface_pitch_d;
	u32 m_surface_pitch_z;

	u32 m_surface_offset_a;
	u32 m_surface_offset_b;
	u32 m_surface_offset_c;
	u32 m_surface_offset_d;
	u32 m_surface_offset_z;

	u16 m_shader_window_height;
	u8 m_shader_window_origin;
	u16 m_shader_window_pixel_centers;

	u32 m_surface_colour_target;

	u8 m_begin_end;

protected:
	RSXThread()
		: ThreadBase(false, "RSXThread")
		, m_ctrl(nullptr)
		, m_flip_status(0)
		, m_flip_mode(CELL_GCM_DISPLAY_VSYNC)
		, m_main_mem_addr(0)
		, m_local_mem_addr(0)
		, m_draw_mode(0)
		, m_draw_array_count(0)
	{
		m_set_alpha_test = false;
		m_set_blend = false;
		m_set_depth_bounds_test = false;
		m_depth_test_enable = false;
		m_set_logic_op = false;
		m_set_cull_face_enable = false;
		m_set_dither = false;
		m_set_stencil_test = false;
		m_set_line_smooth = false;
		m_set_poly_smooth = false;
		m_set_two_sided_stencil_test_enable = false;
		m_set_surface_clip_horizontal = false;
		m_set_surface_clip_vertical = false;

		m_clear_color_r = 0;
		m_clear_color_g = 0;
		m_clear_color_b = 0;
		m_clear_color_a = 0;
		m_clear_z = 0xffffff;
		m_clear_s = 0;

		Reset();
	}

	void Reset()
	{
		m_set_color_mask = false;
		m_set_clip = false;
		m_set_depth_func = false;
		m_set_viewport_horizontal = false;
		m_set_viewport_vertical = false;
		m_set_scissor_horizontal = false;
		m_set_scissor_vertical = false;
		m_set_front_polygon_mode = false;
		m_clear_surface_mask = 0;
		m_set_blend_sfactor = false;
		m_set_blend_dfactor = false;
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
		m_set_blend_equation = false;
		m_set_depth_mask = false;
		m_set_line_width = false;
		m_set_shade_mode = false;
		m_set_blend_color = false;
		m_set_semaphore_offset = false;
		m_set_fog_mode = false;
		m_set_fog_params = false;
		m_set_clip_plane = false;
		m_set_surface_format = false;
		m_set_context_dma_color_a = false;
		m_set_context_dma_color_b = false;
		m_set_context_dma_color_c = false;
		m_set_context_dma_color_d = false;
		m_set_context_dma_z = false;
		m_set_cull_face = false;
		m_set_alpha_func = false;
		m_set_alpha_ref = false;

		m_clear_surface_mask = 0;
		m_begin_end = 0;
	}

	void Begin(u32 draw_mode);
	void End();

	void DoCmd(const u32 fcmd, const u32 cmd, mem32_ptr_t& args, const u32 count);

	virtual void OnInit() = 0;
	virtual void OnInitThread() = 0;
	virtual void OnExitThread() = 0;
	virtual void OnReset() = 0;
	virtual void ExecCMD() = 0;
	virtual void Flip() = 0;

	u32 GetAddress(u32 offset, u8 location)
	{
		switch(location)
		{
		case CELL_GCM_LOCATION_LOCAL: return m_local_mem_addr + offset;
		case CELL_GCM_LOCATION_MAIN: return m_main_mem_addr + offset;
		}

		ConLog.Error("GetAddress(offset=0x%x, location=0x%x)", location);
		assert(0);
		return 0;
	}

	void LoadVertexData(u32 first, u32 count)
	{
		for(u32 i=0; i<m_vertex_count; ++i)
		{
			if(!m_vertex_data[i].IsEnabled()) continue;

			m_vertex_data[i].Load(first, count);
		}
	}

	virtual void Task();

public:
	void Init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress)
	{
		m_ctrl = (CellGcmControl*)&Memory[ctrlAddress];
		m_ioAddress = ioAddress;
		m_ioSize = ioSize;
		m_ctrlAddress = ctrlAddress;
		m_local_mem_addr = localAddress;

		m_cur_vertex_prog = nullptr;
		m_cur_shader_prog = nullptr;
		m_cur_shader_prog_num = 0;

		OnInit();
		ThreadBase::Start();
	}
};
