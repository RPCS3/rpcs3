#pragma once
#include "GCM.h"
#include "RSXTexture.h"
#include "RSXVertexProgram.h"
#include "RSXFragmentProgram.h"

#include <stack>
#include "Utilities/Semaphore.h"
#include "Utilities/Thread.h"
#include "Utilities/Timer.h"
#include "Utilities/types.h"

namespace rsx
{
	namespace limits
	{
		enum
		{
			textures_count = 16,
			vertex_textures_count = 4,
			vertex_count = 16,
			fragment_count = 32,
			tiles_count = 15,
			zculls_count = 8,
			color_buffers_count = 4
		};
	}

	extern u32 method_registers[0x10000 >> 2];

	u32 get_address(u32 offset, u32 location);
	u32 linear_to_swizzle(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth);

	u32 get_vertex_type_size(u32 type);

	struct surface_info
	{
		u8 log2height;
		u8 log2width;
		u8 antialias;
		u8 depth_format;
		u8 color_format;

		u32 width;
		u32 height;
		u32 format;

		void unpack(u32 surface_format)
		{
			format = surface_format;

			log2height = surface_format >> 24;
			log2width = (surface_format >> 16) & 0xff;
			antialias = (surface_format >> 12) & 0xf;
			depth_format = (surface_format >> 5) & 0x7;
			color_format = surface_format & 0x1f;

			width = 1 << (u32(log2width) + 1);
			height = 1 << (u32(log2width) + 1);
		}
	};

	struct data_array_format_info
	{
		u16 frequency = 0;
		u8 stride = 0;
		u8 size = 0;
		u8 type = CELL_GCM_VERTEX_F;
		bool array = false;

		void unpack(u32 data_array_format)
		{
			frequency = data_array_format >> 16;
			stride = (data_array_format >> 8) & 0xff;
			size = (data_array_format >> 4) & 0xf;
			type = data_array_format & 0xf;
		}
	};
}

enum Method
{
	CELL_GCM_METHOD_FLAG_NON_INCREMENT = 0x40000000,
	CELL_GCM_METHOD_FLAG_JUMP          = 0x20000000,
	CELL_GCM_METHOD_FLAG_CALL          = 0x00000002,
	CELL_GCM_METHOD_FLAG_RETURN        = 0x00020000,
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

class RSXThread : protected named_thread_t
{
protected:
	std::stack<u32> m_call_stack;
	CellGcmControl* m_ctrl;
	Timer m_timer_sync;

public:
	GcmTileInfo tiles[rsx::limits::tiles_count];
	GcmZcullInfo zculls[rsx::limits::zculls_count];
	rsx::texture textures[rsx::limits::textures_count];
	rsx::vertex_texture m_vertex_textures[rsx::limits::vertex_textures_count];

	rsx::data_array_format_info vertex_arrays_info[rsx::limits::vertex_count];
	std::vector<u8> vertex_arrays[rsx::limits::vertex_count];
	RSXIndexArrayData m_indexed_array;

	std::unordered_map<u32, color4_base<f32>> transform_constants;
	std::unordered_map<u32, color4_base<f32>> fragment_constants;

	u32 m_shader_ctrl;
	RSXVertexProgram m_vertex_progs[rsx::limits::vertex_count];
	RSXVertexProgram* m_cur_vertex_prog;

public:
	u32 m_ioAddress, m_ioSize, m_ctrlAddress;
	int flip_status;
	int flip_mode;
	int debug_level;
	int frequency_mode;

	u32 tiles_addr;
	u32 zculls_addr;
	u32 m_gcm_buffers_addr;
	u32 gcm_buffers_count;
	u32 gcm_current_buffer;
	u32 ctxt_addr;
	u32 report_main_addr;
	u32 label_addr;
	u32 draw_mode;

	// DMA
	u32 dma_report;

	u32 local_mem_addr, main_mem_addr;
	bool strict_ordering[0x1000];

public:
	u32 draw_array_count;
	u32 draw_array_first;

	u32 m_width;
	u32 m_height;
	float m_width_scale;
	float m_height_scale;
	double m_fps_limit = 59.94;

public:
	std::mutex cs_main;
	semaphore_t sem_flip;
	u64 last_flip_time;
	vm::ps3::ptr<void(u32)> flip_handler = { 0 };
	vm::ps3::ptr<void(u32)> user_handler = { 0 };
	vm::ps3::ptr<void(u32)> vblank_handler = { 0 };
	u64 vblank_count;

public:
	// Dither
	bool m_set_dither;

	// Clip 
	bool m_set_clip;
	float m_clip_min;
	float m_clip_max;

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
	bool m_set_scissor_horizontal;
	bool m_set_scissor_vertical;
	u16 m_scissor_x;
	u16 m_scissor_y;
	u16 m_scissor_w;
	u16 m_scissor_h;

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


	// Clearing
	u32 m_clear_surface_mask;

	// Stencil Test
	bool m_set_two_side_light_enable;

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
	// PGRAPH
	u32 m_PGRAPH_semaphore_offset;
	//PFIFO
	u32 m_PFIFO_semaphore_offset;
	u32 m_PFIFO_semaphore_release_value;

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
	rsx::surface_info m_surface;
	bool m_set_surface_clip_horizontal;
	bool m_set_surface_clip_vertical;

	// DMA context
	u32 m_context_surface;
	u32 m_context_dma_img_src;
	u32 m_context_dma_img_dst;
	u32 m_context_dma_buffer_in_src;
	u32 m_context_dma_buffer_in_dst;
	u32 m_dst_offset;

	// Swizzle2D?
	u16 m_swizzle_format;
	u8 m_swizzle_width;
	u8 m_swizzle_height;
	u32 m_swizzle_offset;

	// Shader
	u16 m_shader_window_height;
	u8 m_shader_window_origin;
	u16 m_shader_window_pixel_centers;

	// Vertex Data
	u32 m_vertex_data_base_index;

	// Frequency divider
	u32 m_set_frequency_divider_operation;

	u8 m_begin_end;
	bool m_read_buffer;

	std::set<u32> m_used_gcm_commands;

protected:
	RSXThread()
		: m_ctrl(nullptr)
		, m_shader_ctrl(0x40)
		, flip_status(0)
		, flip_mode(CELL_GCM_DISPLAY_VSYNC)
		, debug_level(CELL_GCM_DEBUG_LEVEL0)
		, frequency_mode(CELL_GCM_DISPLAY_FREQUENCY_DISABLE)
		, report_main_addr(0)
		, main_mem_addr(0)
		, local_mem_addr(0)
		, draw_mode(0)
		, draw_array_count(0)
		, draw_array_first(~0)
		, gcm_current_buffer(0)
		, m_read_buffer(true)
	{
		flip_handler.set(0);
		vblank_handler.set(0);
		user_handler.set(0);
		rsx::method_registers[NV4097_SET_ALPHA_TEST_ENABLE] = false;
		m_set_depth_bounds_test = false;
		rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] = 0;
		rsx::method_registers[NV4097_SET_BLEND_ENABLE] = false;
		m_set_dither = false;
		m_set_scissor_horizontal = false;
		m_set_scissor_vertical = false;
		m_set_line_smooth = false;
		m_set_poly_smooth = false;
		m_set_point_sprite_control = false;
		m_set_specular = false;
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
		rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE] = 0;
		rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] = 0xffffff << 8;
		m_poly_offset_scale_factor = 0.0;
		m_poly_offset_bias = 0.0;
		m_restart_index = 0xffffffff;
		m_front_polygon_mode = 0x1b02; // GL_FILL
		m_back_polygon_mode = 0x1b02; // GL_FILL
		rsx::method_registers[NV4097_SET_FRONT_FACE] = 0x0901; // GL_CCW
		rsx::method_registers[NV4097_SET_CULL_FACE] = 0x0405; // GL_BACK
		rsx::method_registers[NV4097_SET_ALPHA_FUNC] = 0x0207; // GL_ALWAYS
		rsx::method_registers[NV4097_SET_ALPHA_REF] = 0.0f;
		m_shade_mode = 0x1D01; // GL_SMOOTH

		m_depth_bounds_min = 0.0;
		m_depth_bounds_max = 1.0;
		m_clip_min = 0.0;
		m_clip_max = 1.0;
		rsx::method_registers[NV4097_SET_BLEND_EQUATION] = (0x8006) | (0x8006 << 16); // GL_FUNC_ADD
		rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] = 1 | (1 << 16);
		rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] = 0;
		m_point_x = 0;
		m_point_y = 0;
		m_point_size = 1.0;
		m_line_width = 1.0;
		m_line_stipple_pattern = 0xffff;
		m_line_stipple_factor = 1;
		rsx::method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET] = 0;
		m_vertex_data_base_index = 0;

		// Construct Stipple Pattern
		for (size_t i = 0; i < 32; i++)
		{
			m_polygon_stipple_pattern[i] = 0xFFFFFFFF;
		}

		// Construct Textures
		for (int i = 0; i < 16; i++)
		{
			textures[i] = rsx::texture();
		}

		Reset();
	}

	virtual ~RSXThread() override
	{
	}

	void Reset()
	{
		rsx::method_registers[NV4097_SET_DEPTH_TEST_ENABLE] = false;
		rsx::method_registers[NV4097_SET_DEPTH_MASK] = 1;
		rsx::method_registers[NV4097_SET_DEPTH_FUNC] = 0x0201;

		m_set_dither = false;
		rsx::method_registers[NV4097_SET_COLOR_MASK] = -1;
		m_set_clip = false;
		m_set_depth_bounds_test = false;
		m_set_depth_bounds = false;
		m_set_scissor_horizontal = false;
		m_set_scissor_vertical = false;
		m_set_front_polygon_mode = false;
		m_set_back_polygon_mode = false;
		rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT] = 0;
		rsx::method_registers[NV4097_SET_BLEND_ENABLE] = false;
		m_set_two_side_light_enable = false;
		m_set_point_sprite_control = false;
		m_set_point_size = false;
		m_set_line_width = false;
		m_set_line_smooth = false;
		m_set_shade_mode = false;
		m_set_fog_mode = false;
		m_set_fog_params = false;
		m_set_clip_plane = false;
		rsx::method_registers[NV4097_SET_CULL_FACE_ENABLE] = false;
		rsx::method_registers[NV4097_SET_ALPHA_TEST_ENABLE] = false;
		rsx::method_registers[NV4097_SET_ALPHA_FUNC] = false;
		rsx::method_registers[NV4097_SET_ALPHA_REF] = false;
		m_set_poly_smooth = false;
		m_set_poly_offset_fill = false;
		m_set_poly_offset_line = false;
		m_set_poly_offset_point = false;
		m_set_poly_offset_mode = false;
		m_set_restart_index = false;
		m_set_specular = false;
		m_set_line_stipple = false;
		m_set_polygon_stipple = false;
		m_set_surface_clip_horizontal = false;
		m_set_surface_clip_vertical = false;

		m_clear_surface_mask = 0;
		m_begin_end = 0;

		for (uint i = 0; i < rsx::limits::textures_count; ++i)
		{
			textures[i].init(i);
		}
	}

	void begin(u32 draw_mode);
	void End();

	u32 OutOfArgsCount(const uint x, const u32 cmd, const u32 count, const u32 args_addr);
	void DoCmd(const u32 fcmd, const u32 cmd, const u32 args_addr, const u32 count);

	virtual void oninit() = 0;
	virtual void oninit_thread() = 0;
	virtual void onexit_thread() = 0;
	virtual void OnReset() = 0;

	/**
	 * This member is called when the backend is expected to render a draw call, either
	 * indexed or not.
	 */
	virtual void end() = 0;

	/**
	* This member is called when the backend is expected to clear a target surface.
	*/
	virtual void clear_surface(u32 arg) = 0;

	/**
	* This member is called when the backend is expected to present a target surface in
	* either local or main memory.
	*/
	virtual void flip(int buffer) = 0;

	/**
	 * This member is called when RSXThread parse a TEXTURE_READ_SEMAPHORE_RELEASE
	 * command.
	 * Backend is expected to write value at offset when current draw textures aren't
	 * needed anymore by the GPU and can be modified.
	 */
	virtual void semaphorePGRAPHTextureReadRelease(u32 offset, u32 value) = 0;
	/**
	* This member is called when RSXThread parse a BACK_END_WRITE_SEMAPHORE_RELEASE
	* command.
	* Backend is expected to write value at offset when current draw call has completed
	* and render surface can be used.
	*/
	virtual void semaphorePGRAPHBackendRelease(u32 offset, u32 value) = 0;
	/**
	* This member is called when RSXThread parse a SEMAPHORE_ACQUIRE command.
	* Backend and associated GPU is expected to wait that memory at offset is the same
	* as value. In particular buffer/texture buffers value can change while backend is
	* waiting.
	*/
	virtual void semaphorePFIFOAcquire(u32 offset, u32 value) = 0;
	/**
	 * Called when vertex or fragment shader changes.
	 * Backend can reuse same program if no change has been notified.
	 */
	virtual void notifyProgramChange() = 0;
	/**
	* Called when blend state changes.
	* Backend can reuse same program if no change has been notified.
	*/
	virtual void notifyBlendStateChange() = 0;
	/**
	* Called when depth stencil state changes.
	* Backend can reuse same program if no change has been notified.
	*/
	virtual void notifyDepthStencilStateChange() = 0;
	/**
	* Called when rasterizer state changes.
	* Rasterizer state includes culling, color masking
	* Backend can reuse same program if no change has been notified.
	*/
	virtual void notifyRasterizerStateChange() = 0;

	void LoadVertexData(u32 first, u32 count)
	{
		for (u32 i = 0; i < rsx::limits::vertex_count; ++i)
		{
//			if (!m_vertex_data[i].IsEnabled()) continue;

//			m_vertex_data[i].Load(first, count, m_vertex_data_base_offset, m_vertex_data_base_index);
		}
	}

	virtual void Task();

public:
	void Init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress);

	u32 ReadIO32(u32 addr);

	void WriteIO32(u32 addr, u32 value);
};
