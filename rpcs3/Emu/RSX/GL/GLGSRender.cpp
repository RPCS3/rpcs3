#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/Memory/Memory.h"
#include "GLGSRender.h"
#include "rsx_gl_cache.h"
#include "../rsx_methods.h"
#include "../Common/BufferUtils.h"
#include "../rsx_utils.h"

extern cfg::bool_entry g_cfg_rsx_debug_output;
extern cfg::bool_entry g_cfg_rsx_overlay;

#define DUMP_VERTEX_DATA 0

namespace
{
	u32 get_max_depth_value(rsx::surface_depth_format format)
	{
		switch (format)
		{
		case rsx::surface_depth_format::z16: return 0xFFFF;
		case rsx::surface_depth_format::z24s8: return 0xFFFFFF;
		}
		throw EXCEPTION("Unknow depth format");
	}
}

GLGSRender::GLGSRender() : GSRender(frame_type::OpenGL)
{
	init_glsl_cache_program_context(programs_cache.context);
}

u32 GLGSRender::enable(u32 condition, u32 cap)
{
	if (condition)
	{
		glEnable(cap);
	}
	else
	{
		glDisable(cap);
	}

	return condition;
}

u32 GLGSRender::enable(u32 condition, u32 cap, u32 index)
{
	if (condition)
	{
		glEnablei(cap, index);
	}
	else
	{
		glDisablei(cap, index);
	}

	return condition;
}

extern CellGcmContextData current_context;

void GLGSRender::begin()
{
	rsx::thread::begin();

	init_buffers();

	std::chrono::time_point<std::chrono::system_clock> then = std::chrono::system_clock::now();

	u32 color_mask = rsx::method_registers[NV4097_SET_COLOR_MASK];
	bool color_mask_b = !!(color_mask & 0xff);
	bool color_mask_g = !!((color_mask >> 8) & 0xff);
	bool color_mask_r = !!((color_mask >> 16) & 0xff);
	bool color_mask_a = !!((color_mask >> 24) & 0xff);

	__glcheck glColorMask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);
	__glcheck glDepthMask(rsx::method_registers[NV4097_SET_DEPTH_MASK]);
	__glcheck glStencilMask(rsx::method_registers[NV4097_SET_STENCIL_MASK]);

	if (__glcheck enable(rsx::method_registers[NV4097_SET_DEPTH_TEST_ENABLE], GL_DEPTH_TEST))
	{
		__glcheck glDepthFunc(rsx::method_registers[NV4097_SET_DEPTH_FUNC]);
		__glcheck glDepthMask(rsx::method_registers[NV4097_SET_DEPTH_MASK]);
	}

	if (glDepthBoundsEXT && (__glcheck enable(rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE], GL_DEPTH_BOUNDS_TEST_EXT)))
	{
		__glcheck glDepthBoundsEXT((f32&)rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_MIN], (f32&)rsx::method_registers[NV4097_SET_DEPTH_BOUNDS_MAX]);
	}

	__glcheck glDepthRange((f32&)rsx::method_registers[NV4097_SET_CLIP_MIN], (f32&)rsx::method_registers[NV4097_SET_CLIP_MAX]);
	__glcheck enable(rsx::method_registers[NV4097_SET_DITHER_ENABLE], GL_DITHER);

	if (__glcheck enable(rsx::method_registers[NV4097_SET_BLEND_ENABLE], GL_BLEND))
	{
		u32 sfactor = rsx::method_registers[NV4097_SET_BLEND_FUNC_SFACTOR];
		u32 dfactor = rsx::method_registers[NV4097_SET_BLEND_FUNC_DFACTOR];
		u16 sfactor_rgb = sfactor;
		u16 sfactor_a = sfactor >> 16;
		u16 dfactor_rgb = dfactor;
		u16 dfactor_a = dfactor >> 16;

		__glcheck glBlendFuncSeparate(sfactor_rgb, dfactor_rgb, sfactor_a, dfactor_a);

		if (m_surface.color_format == rsx::surface_color_format::w16z16y16x16) //TODO: check another color formats
		{
			u32 blend_color = rsx::method_registers[NV4097_SET_BLEND_COLOR];
			u32 blend_color2 = rsx::method_registers[NV4097_SET_BLEND_COLOR2];

			u16 blend_color_r = blend_color;
			u16 blend_color_g = blend_color >> 16;
			u16 blend_color_b = blend_color2;
			u16 blend_color_a = blend_color2 >> 16;

			__glcheck glBlendColor(blend_color_r / 65535.f, blend_color_g / 65535.f, blend_color_b / 65535.f, blend_color_a / 65535.f);
		}
		else
		{
			u32 blend_color = rsx::method_registers[NV4097_SET_BLEND_COLOR];
			u8 blend_color_r = blend_color;
			u8 blend_color_g = blend_color >> 8;
			u8 blend_color_b = blend_color >> 16;
			u8 blend_color_a = blend_color >> 24;

			__glcheck glBlendColor(blend_color_r / 255.f, blend_color_g / 255.f, blend_color_b / 255.f, blend_color_a / 255.f);
		}

		u32 equation = rsx::method_registers[NV4097_SET_BLEND_EQUATION];
		u16 equation_rgb = equation;
		u16 equation_a = equation >> 16;

		__glcheck glBlendEquationSeparate(equation_rgb, equation_a);
	}
	
	if (__glcheck enable(rsx::method_registers[NV4097_SET_STENCIL_TEST_ENABLE], GL_STENCIL_TEST))
	{
		__glcheck glStencilFunc(rsx::method_registers[NV4097_SET_STENCIL_FUNC], rsx::method_registers[NV4097_SET_STENCIL_FUNC_REF],
			rsx::method_registers[NV4097_SET_STENCIL_FUNC_MASK]);
		__glcheck glStencilOp(rsx::method_registers[NV4097_SET_STENCIL_OP_FAIL], rsx::method_registers[NV4097_SET_STENCIL_OP_ZFAIL],
			rsx::method_registers[NV4097_SET_STENCIL_OP_ZPASS]);

		if (rsx::method_registers[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE])
		{
			__glcheck glStencilMaskSeparate(GL_BACK, rsx::method_registers[NV4097_SET_BACK_STENCIL_MASK]);
			__glcheck glStencilFuncSeparate(GL_BACK, rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC],
				rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC_REF], rsx::method_registers[NV4097_SET_BACK_STENCIL_FUNC_MASK]);
			__glcheck glStencilOpSeparate(GL_BACK, rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_FAIL],
				rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL], rsx::method_registers[NV4097_SET_BACK_STENCIL_OP_ZPASS]);
		}
	}

	if (u32 blend_mrt = rsx::method_registers[NV4097_SET_BLEND_ENABLE_MRT])
	{
		__glcheck enable(blend_mrt & 2, GL_BLEND, 1);
		__glcheck enable(blend_mrt & 4, GL_BLEND, 2);
		__glcheck enable(blend_mrt & 8, GL_BLEND, 3);
	}
	
	if (__glcheck enable(rsx::method_registers[NV4097_SET_LOGIC_OP_ENABLE], GL_COLOR_LOGIC_OP))
	{
		__glcheck glLogicOp(rsx::method_registers[NV4097_SET_LOGIC_OP]);
	}

	u32 line_width = rsx::method_registers[NV4097_SET_LINE_WIDTH];
	__glcheck glLineWidth((line_width >> 3) + (line_width & 7) / 8.f);
	__glcheck enable(rsx::method_registers[NV4097_SET_LINE_SMOOTH_ENABLE], GL_LINE_SMOOTH);

	//TODO
	//NV4097_SET_ANISO_SPREAD

	//TODO
	/*
	glcheck(glFogi(GL_FOG_MODE, rsx::method_registers[NV4097_SET_FOG_MODE]));
	f32 fog_p0 = (f32&)rsx::method_registers[NV4097_SET_FOG_PARAMS + 0];
	f32 fog_p1 = (f32&)rsx::method_registers[NV4097_SET_FOG_PARAMS + 1];

	f32 fog_start = (2 * fog_p0 - (fog_p0 - 2) / fog_p1) / (fog_p0 - 1);
	f32 fog_end = (2 * fog_p0 - 1 / fog_p1) / (fog_p0 - 1);

	glFogf(GL_FOG_START, fog_start);
	glFogf(GL_FOG_END, fog_end);
	*/
	//NV4097_SET_FOG_PARAMS

	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_POINT_ENABLE], GL_POLYGON_OFFSET_POINT);
	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_LINE_ENABLE], GL_POLYGON_OFFSET_LINE);
	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE], GL_POLYGON_OFFSET_FILL);

	__glcheck glPolygonOffset((f32&)rsx::method_registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR],
		(f32&)rsx::method_registers[NV4097_SET_POLYGON_OFFSET_BIAS]);

	//NV4097_SET_SPECULAR_ENABLE
	//NV4097_SET_TWO_SIDE_LIGHT_EN
	//NV4097_SET_FLAT_SHADE_OP
	//NV4097_SET_EDGE_FLAG

	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE], GL_POLYGON_OFFSET_FILL);

	if (__glcheck enable(rsx::method_registers[NV4097_SET_CULL_FACE_ENABLE], GL_CULL_FACE))
	{
		__glcheck glCullFace(rsx::method_registers[NV4097_SET_CULL_FACE]);
	}

	__glcheck glFrontFace(rsx::method_registers[NV4097_SET_FRONT_FACE] ^ 1);

	__glcheck enable(rsx::method_registers[NV4097_SET_POLY_SMOOTH_ENABLE], GL_POLYGON_SMOOTH);

	//NV4097_SET_COLOR_KEY_COLOR
	//NV4097_SET_SHADER_CONTROL
	//NV4097_SET_ZMIN_MAX_CONTROL
	//NV4097_SET_ANTI_ALIASING_CONTROL
	//NV4097_SET_CLIP_ID_TEST_ENABLE

	if (__glcheck enable(rsx::method_registers[NV4097_SET_RESTART_INDEX_ENABLE], GL_PRIMITIVE_RESTART))
	{
		__glcheck glPrimitiveRestartIndex(rsx::method_registers[NV4097_SET_RESTART_INDEX]);
	}

	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	m_begin_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(now - then).count();
	m_draw_calls++;
}

namespace
{
	GLenum get_gl_target_for_texture(const rsx::texture& tex)
	{
		switch (tex.get_extended_texture_dimension())
		{
		case rsx::texture_dimension_extended::texture_dimension_1d: return GL_TEXTURE_1D;
		case rsx::texture_dimension_extended::texture_dimension_2d: return GL_TEXTURE_2D;
		case rsx::texture_dimension_extended::texture_dimension_cubemap: return GL_TEXTURE_CUBE_MAP;
		case rsx::texture_dimension_extended::texture_dimension_3d: return GL_TEXTURE_3D;
		}
		throw EXCEPTION("Unknow texture target");
	}

	GLenum get_gl_target_for_texture(const rsx::vertex_texture& tex)
	{
		switch (tex.get_extended_texture_dimension())
		{
		case rsx::texture_dimension_extended::texture_dimension_1d: return GL_TEXTURE_1D;
		case rsx::texture_dimension_extended::texture_dimension_2d: return GL_TEXTURE_2D;
		case rsx::texture_dimension_extended::texture_dimension_cubemap: return GL_TEXTURE_CUBE_MAP;
		case rsx::texture_dimension_extended::texture_dimension_3d: return GL_TEXTURE_3D;
		}
		throw EXCEPTION("Unknow texture target");
	}
}

void GLGSRender::end()
{
	if (!draw_fbo || !load_program())
	{
		rsx::thread::end();
		return;
	}

	u32 clip_plane_control = rsx::method_registers[NV4097_SET_USER_CLIP_PLANE_CONTROL];
	u8 clip_plane_0 = clip_plane_control & 0xf;
	u8 clip_plane_1 = (clip_plane_control >> 4) & 0xf;
	u8 clip_plane_2 = (clip_plane_control >> 8) & 0xf;
	u8 clip_plane_3 = (clip_plane_control >> 12) & 0xf;
	u8 clip_plane_4 = (clip_plane_control >> 16) & 0xf;
	u8 clip_plane_5 = (clip_plane_control >> 20) & 0xf;

	auto set_clip_plane_control = [&](int index, u8 control)
	{
		int value = 0;
		int location;
		if (m_program->uniforms.has_location("uc_m" + std::to_string(index), &location))
		{
			switch (control)
			{
			default:
				LOG_ERROR(RSX, "bad clip plane control (0x%x)", control);

			case CELL_GCM_USER_CLIP_PLANE_DISABLE:
				value = 0;
				break;

			case CELL_GCM_USER_CLIP_PLANE_ENABLE_GE:
				value = 1;
				break;

			case CELL_GCM_USER_CLIP_PLANE_ENABLE_LT:
				value = -1;
				break;
			}

			__glcheck m_program->uniforms[location] = value;
		}

		__glcheck enable(value, GL_CLIP_DISTANCE0 + index);
	};

	set_clip_plane_control(0, clip_plane_0);
	set_clip_plane_control(1, clip_plane_1);
	set_clip_plane_control(2, clip_plane_2);
	set_clip_plane_control(3, clip_plane_3);
	set_clip_plane_control(4, clip_plane_4);
	set_clip_plane_control(5, clip_plane_5);

	draw_fbo.bind();
	m_program->use();

	//setup textures
	{
		for (int i = 0; i < rsx::limits::textures_count; ++i)
		{
			int location;
			if (m_program->uniforms.has_location("ftexture" + std::to_string(i), &location))
			{
				if (!textures[i].enabled())
				{
					glActiveTexture(GL_TEXTURE0 + i);
					glBindTexture(GL_TEXTURE_2D, 0);
					glProgramUniform1i(m_program->id(), location, i);

					continue;
				}

				m_gl_textures[i].set_target(get_gl_target_for_texture(textures[i]));

				__glcheck m_gl_texture_cache.upload_texture(i, textures[i], m_gl_textures[i], m_rtts);
				__glcheck glProgramUniform1i(m_program->id(), location, i);

				if (m_program->uniforms.has_location("ftexture" + std::to_string(i) + "_cm", &location))
				{
					if (textures[i].format() & CELL_GCM_TEXTURE_UN)
					{
						u32 width = std::max<u32>(textures[i].width(), 1);
						u32 height = std::max<u32>(textures[i].height(), 1);
						u32 depth = std::max<u32>(textures[i].depth(), 1);

						glProgramUniform4f(m_program->id(), location, 1.f / width, 1.f / height, 1.f / depth, 1.0f);
					}
				}
			}
		}
		
		for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
		{
			int location;
			if (m_program->uniforms.has_location("vtexture" + std::to_string(i), &location))
			{
				if (!textures[i].enabled())
				{
					glActiveTexture(GL_TEXTURE0 + i);
					glBindTexture(GL_TEXTURE_2D, 0);
					glProgramUniform1i(m_program->id(), location, i);

					continue;
				}

				m_gl_vertex_textures[i].set_target(get_gl_target_for_texture(vertex_textures[i]));

				__glcheck m_gl_texture_cache.upload_texture(i, vertex_textures[i], m_gl_vertex_textures[i], m_rtts);
				__glcheck glProgramUniform1i(m_program->id(), location, i);

				if (m_program->uniforms.has_location("vtexture" + std::to_string(i) + "_cm", &location))
				{
					if (textures[i].format() & CELL_GCM_TEXTURE_UN)
					{
						u32 width = std::max<u32>(textures[i].width(), 1);
						u32 height = std::max<u32>(textures[i].height(), 1);
						u32 depth = std::max<u32>(textures[i].depth(), 1);

						glProgramUniform4f(m_program->id(), location, 1.f / width, 1.f / height, 1.f / depth, 1.0f);
					}
				}
			}
		}
	}

	u32 offset_in_index_buffer = set_vertex_buffer();
	m_vao.bind();

	std::chrono::time_point<std::chrono::system_clock> then = std::chrono::system_clock::now();

	if (g_cfg_rsx_debug_output)
	{
		m_program->validate();
	}

	if (draw_command == rsx::draw_command::indexed)
	{
		rsx::index_array_type indexed_type = rsx::to_index_array_type(rsx::method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4);

		if (indexed_type == rsx::index_array_type::u32)
		{
			__glcheck glDrawElements(gl::draw_mode(draw_mode), vertex_draw_count, GL_UNSIGNED_INT, (GLvoid *)(std::ptrdiff_t)offset_in_index_buffer);
		}
		else if (indexed_type == rsx::index_array_type::u16)
		{
			__glcheck glDrawElements(gl::draw_mode(draw_mode), vertex_draw_count, GL_UNSIGNED_SHORT, (GLvoid *)(std::ptrdiff_t)offset_in_index_buffer);
		}
		else
		{
			throw std::logic_error("bad index array type");
		}
	}
	else if (!is_primitive_native(draw_mode))
	{
		__glcheck glDrawElements(gl::draw_mode(draw_mode), vertex_draw_count, GL_UNSIGNED_SHORT, (GLvoid *)(std::ptrdiff_t)offset_in_index_buffer);
	}
	else
	{
		draw_fbo.draw_arrays(draw_mode, vertex_draw_count);
	}

	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	m_draw_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(now - then).count();

	write_buffers();

	rsx::thread::end();
}

void GLGSRender::set_viewport()
{
	u32 viewport_horizontal = rsx::method_registers[NV4097_SET_VIEWPORT_HORIZONTAL];
	u32 viewport_vertical = rsx::method_registers[NV4097_SET_VIEWPORT_VERTICAL];

	u16 viewport_x = viewport_horizontal & 0xffff;
	u16 viewport_y = viewport_vertical & 0xffff;
	u16 viewport_w = viewport_horizontal >> 16;
	u16 viewport_h = viewport_vertical >> 16;

	u32 scissor_horizontal = rsx::method_registers[NV4097_SET_SCISSOR_HORIZONTAL];
	u32 scissor_vertical = rsx::method_registers[NV4097_SET_SCISSOR_VERTICAL];
	u16 scissor_x = scissor_horizontal;
	u16 scissor_w = scissor_horizontal >> 16;
	u16 scissor_y = scissor_vertical;
	u16 scissor_h = scissor_vertical >> 16;

	u32 shader_window = rsx::method_registers[NV4097_SET_SHADER_WINDOW];

	rsx::window_origin shader_window_origin = rsx::to_window_origin((shader_window >> 12) & 0xf);

	if (shader_window_origin == rsx::window_origin::bottom)
	{
		__glcheck glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
		__glcheck glScissor(scissor_x, scissor_y, scissor_w, scissor_h);
	}
	else
	{
		u16 shader_window_height = shader_window & 0xfff;

		__glcheck glViewport(viewport_x, shader_window_height - viewport_y - viewport_h + 1, viewport_w, viewport_h);
		__glcheck glScissor(scissor_x, shader_window_height - scissor_y - scissor_h + 1, scissor_w, scissor_h);
	}

	glEnable(GL_SCISSOR_TEST);
}

void GLGSRender::on_init_thread()
{
	GSRender::on_init_thread();

	gl::init();
	if (g_cfg_rsx_debug_output)
		gl::enable_debugging();
	LOG_NOTICE(RSX, "%s", (const char*)glGetString(GL_VERSION));
	LOG_NOTICE(RSX, "%s", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
	LOG_NOTICE(RSX, "%s", (const char*)glGetString(GL_VENDOR));

	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_uniform_buffer_offset_align);
	glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &m_min_texbuffer_alignment);
	m_vao.create();

	for (gl::texture &tex : m_gl_attrib_buffers)
	{
		tex.create();
		tex.set_target(gl::texture::target::textureBuffer);
	}

	m_attrib_ring_buffer.create(gl::buffer::target::texture, 16 * 0x100000);
	m_uniform_ring_buffer.create(gl::buffer::target::uniform, 16 * 0x100000);
	m_index_ring_buffer.create(gl::buffer::target::element_array, 0x100000);

	m_vao.element_array_buffer = m_index_ring_buffer;
	m_gl_texture_cache.initialize_rtt_cache();
}

void GLGSRender::on_exit()
{
	glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);

	programs_cache.clear();

	if (draw_fbo)
	{
		draw_fbo.remove();
	}

	if (m_flip_fbo)
	{
		m_flip_fbo.remove();
	}

	if (m_flip_tex_color)
	{
		m_flip_tex_color.remove();
	}

	if (m_vao)
	{
		m_vao.remove();
	}

	for (gl::texture &tex : m_gl_attrib_buffers)
	{
		tex.remove();
	}

	m_attrib_ring_buffer.remove();
	m_uniform_ring_buffer.remove();
	m_index_ring_buffer.remove();
}

void nv4097_clear_surface(u32 arg, GLGSRender* renderer)
{
	//LOG_NOTICE(Log::RSX, "nv4097_clear_surface(0x%x)", arg);
	if (!rsx::method_registers[NV4097_SET_SURFACE_FORMAT])
	{
		return;
	}

	if ((arg & 0xf3) == 0)
	{
		//do nothing
		return;
	}

	/*
	u16 clear_x = rsx::method_registers[NV4097_SET_CLEAR_RECT_HORIZONTAL];
	u16 clear_y = rsx::method_registers[NV4097_SET_CLEAR_RECT_VERTICAL];
	u16 clear_w = rsx::method_registers[NV4097_SET_CLEAR_RECT_HORIZONTAL] >> 16;
	u16 clear_h = rsx::method_registers[NV4097_SET_CLEAR_RECT_VERTICAL] >> 16;
	glScissor(clear_x, clear_y, clear_w, clear_h);
	*/

	renderer->init_buffers(true);
	renderer->draw_fbo.bind();

	GLbitfield mask = 0;

	rsx::surface_depth_format surface_depth_format = rsx::to_surface_depth_format((rsx::method_registers[NV4097_SET_SURFACE_FORMAT] >> 5) & 0x7);

	if (arg & 0x1)
	{
		u32 max_depth_value = get_max_depth_value(surface_depth_format);

		u32 clear_depth = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] >> 8;

		glDepthMask(GL_TRUE);
		glClearDepth(double(clear_depth) / max_depth_value);
		mask |= GLenum(gl::buffers::depth);
	}

	if (surface_depth_format == rsx::surface_depth_format::z24s8 && (arg & 0x2))
	{
		u8 clear_stencil = rsx::method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] & 0xff;

		__glcheck glStencilMask(rsx::method_registers[NV4097_SET_STENCIL_MASK]);
		glClearStencil(clear_stencil);

		mask |= GLenum(gl::buffers::stencil);
	}

	if (arg & 0xf0)
	{
		u32 clear_color = rsx::method_registers[NV4097_SET_COLOR_CLEAR_VALUE];
		u8 clear_a = clear_color >> 24;
		u8 clear_r = clear_color >> 16;
		u8 clear_g = clear_color >> 8;
		u8 clear_b = clear_color;

		glColorMask(((arg & 0x20) ? 1 : 0), ((arg & 0x40) ? 1 : 0), ((arg & 0x80) ? 1 : 0), ((arg & 0x10) ? 1 : 0));
		glClearColor(clear_r / 255.f, clear_g / 255.f, clear_b / 255.f, clear_a / 255.f);

		mask |= GLenum(gl::buffers::color);
	}

	glClear(mask);
	renderer->write_buffers();
}

using rsx_method_impl_t = void(*)(u32, GLGSRender*);

static const std::unordered_map<u32, rsx_method_impl_t> g_gl_method_tbl =
{
	{ NV4097_CLEAR_SURFACE, nv4097_clear_surface }
};

bool GLGSRender::do_method(u32 cmd, u32 arg)
{
	auto found = g_gl_method_tbl.find(cmd);

	if (found == g_gl_method_tbl.end())
	{
		return false;
	}

	found->second(arg, this);
	return true;
}

//binding 0
struct alignas(4) glsl_matrix_buffer
{
	float viewport_matrix[4][4];
	float window_matrix[4][4];
	float normalize_matrix[4][4];
};

//binding 1
struct alignas(4) glsl_vertex_constants_buffer
{
	float vc[468][4];
};

//binding 2
struct alignas(4) glsl_fragment_constants_buffer
{
	float fc[2048][4];
};

//binding 3
struct alignas(4) glsl_fragment_state_buffer
{
	float fog_param0;
	float fog_param1;
	uint alpha_test;
	float alpha_ref;
};

static void fill_matrix_buffer(glsl_matrix_buffer *buffer)
{
	rsx::fill_viewport_matrix(buffer->viewport_matrix, true);
	rsx::fill_window_matrix(buffer->window_matrix, true);

	u32 viewport_horizontal = rsx::method_registers[NV4097_SET_VIEWPORT_HORIZONTAL];
	u32 viewport_vertical = rsx::method_registers[NV4097_SET_VIEWPORT_VERTICAL];

	f32 viewport_x = f32(viewport_horizontal & 0xffff);
	f32 viewport_y = f32(viewport_vertical & 0xffff);
	f32 viewport_w = f32(viewport_horizontal >> 16);
	f32 viewport_h = f32(viewport_vertical >> 16);

	u32 shader_window = rsx::method_registers[NV4097_SET_SHADER_WINDOW];

	rsx::window_origin shader_window_origin = rsx::to_window_origin((shader_window >> 12) & 0xf);
	u16 shader_window_height = shader_window & 0xfff;

	f32 left = viewport_x;
	f32 right = viewport_x + viewport_w;
	f32 top = viewport_y;
	f32 bottom = viewport_y + viewport_h;
	//f32 far_ = (f32&)rsx::method_registers[NV4097_SET_CLIP_MAX];
	//f32 near_ = (f32&)rsx::method_registers[NV4097_SET_CLIP_MIN];

	if (shader_window_origin == rsx::window_origin::bottom)
	{
		top = shader_window_height - (viewport_y + viewport_h) + 1;
		bottom = shader_window_height - viewport_y + 1;
	}

	f32 scale_x = 2.0f / (right - left);
	f32 scale_y = 2.0f / (top - bottom);
	f32 scale_z = 2.0f;

	f32 offset_x = -(right + left) / (right - left);
	f32 offset_y = -(top + bottom) / (top - bottom);
	f32 offset_z = -1.0;

	if (shader_window_origin == rsx::window_origin::top)
	{
		scale_y = -scale_y;
		offset_y = -offset_y;
	}

	rsx::fill_scale_offset_matrix(buffer->normalize_matrix, true, offset_x, offset_y, offset_z, scale_x, scale_y, scale_z);
}

static void fill_fragment_state_buffer(glsl_fragment_state_buffer *buffer)
{
	std::memcpy(&buffer->fog_param0, rsx::method_registers + NV4097_SET_FOG_PARAMS, sizeof(float) * 2);

	buffer->alpha_test = rsx::method_registers[NV4097_SET_ALPHA_TEST_ENABLE];
	buffer->alpha_ref = rsx::method_registers[NV4097_SET_ALPHA_REF] / 255.f;
}

bool GLGSRender::load_program()
{
	if (0)
	{
		RSXVertexProgram vertex_program = get_current_vertex_program();
		RSXFragmentProgram fragment_program = get_current_fragment_program();

		GLProgramBuffer prog_buffer;
		__glcheck prog_buffer.getGraphicPipelineState(vertex_program, fragment_program, nullptr);
	}

	rsx::program_info info = programs_cache.get(get_raw_program(), rsx::decompile_language::glsl);
	m_program = (gl::glsl::program*)info.program;
	m_program->use();

	u32 fragment_constants_count = info.fragment_shader.decompiled->constants.size();
	u32 fragment_constants_size = fragment_constants_count * sizeof(rsx::fragment_program::ucode_instr);

	u32 max_buffer_sz =
		align(sizeof(glsl_matrix_buffer), m_uniform_buffer_offset_align) +
		align(sizeof(glsl_vertex_constants_buffer), m_uniform_buffer_offset_align) +
		align(sizeof(glsl_fragment_state_buffer), m_uniform_buffer_offset_align) +
		align(fragment_constants_size, m_uniform_buffer_offset_align);

	m_uniform_ring_buffer.reserve_and_map(max_buffer_sz);

	u32 scale_offset_offset;
	u32 vertex_constants_offset;
	u32 fragment_constants_offset;
	u32 fragment_state_offset;

	{
		auto mapping = m_uniform_ring_buffer.alloc_from_reserve(sizeof(glsl_matrix_buffer), m_uniform_buffer_offset_align);
		fill_matrix_buffer((glsl_matrix_buffer *)mapping.first);
		scale_offset_offset = mapping.second;
	}

	{
		auto mapping = m_uniform_ring_buffer.alloc_from_reserve(sizeof(glsl_vertex_constants_buffer), m_uniform_buffer_offset_align);
		fill_vertex_program_constants_data(mapping.first);
		vertex_constants_offset = mapping.second;
	}

	{
		auto mapping = m_uniform_ring_buffer.alloc_from_reserve(sizeof(glsl_fragment_state_buffer), m_uniform_buffer_offset_align);
		fill_fragment_state_buffer((glsl_fragment_state_buffer *)mapping.first);
		fragment_state_offset = mapping.second;
	}

	if (fragment_constants_size)
	{
		auto mapping = m_uniform_ring_buffer.alloc_from_reserve(fragment_constants_size, m_uniform_buffer_offset_align);
		fragment_constants_offset = mapping.second;

		static const __m128i mask = _mm_set_epi8(
			0xE, 0xF, 0xC, 0xD,
			0xA, 0xB, 0x8, 0x9,
			0x6, 0x7, 0x4, 0x5,
			0x2, 0x3, 0x0, 0x1);

		auto ucode = (const rsx::fragment_program::ucode_instr *)info.fragment_shader.decompiled->raw->ucode_ptr;

		auto dst = (const rsx::fragment_program::ucode_instr *)mapping.first;

		for (const auto& constant : info.fragment_shader.decompiled->constants)
		{
			const void *src = ucode + u32(constant.id / sizeof(*ucode));

			const __m128i &vector = _mm_loadu_si128((const __m128i*)src);
			const __m128i &shuffled_vector = _mm_shuffle_epi8(vector, mask);
			_mm_stream_si128((__m128i*)dst, shuffled_vector);

			if (0)
			{
				float x = ((float*)dst)[0];
				float y = ((float*)dst)[1];
				float z = ((float*)dst)[2];
				float w = ((float*)dst)[3];

				LOG_WARNING(RSX, "fc%u = {%g, %g, %g, %g}", constant.id, x, y, z, w);
			}

			++dst;
		}
	}

	m_uniform_ring_buffer.unmap();

	m_uniform_ring_buffer.bind_range(0, scale_offset_offset, sizeof(glsl_matrix_buffer));
	m_uniform_ring_buffer.bind_range(1, vertex_constants_offset, sizeof(glsl_vertex_constants_buffer));

	if (fragment_constants_size)
	{
		m_uniform_ring_buffer.bind_range(2, fragment_constants_offset, fragment_constants_size);
	}

	m_uniform_ring_buffer.bind_range(3, fragment_state_offset, sizeof(glsl_fragment_state_buffer));

	return true;
}

void GLGSRender::flip(int buffer)
{
	u32 buffer_width = gcm_buffers[buffer].width;
	u32 buffer_height = gcm_buffers[buffer].height;
	u32 buffer_pitch = gcm_buffers[buffer].pitch;

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);

	rsx::tiled_region buffer_region = get_tiled_address(gcm_buffers[buffer].offset, CELL_GCM_LOCATION_LOCAL);
	u32 absolute_address = buffer_region.address + buffer_region.base;

	if (0)
	{
		LOG_NOTICE(RSX, "flip(%d) -> 0x%x [0x%x]", buffer, absolute_address, rsx::get_address(gcm_buffers[1 - buffer].offset, CELL_GCM_LOCATION_LOCAL));
	}

	gl::texture *render_target_texture = m_rtts.get_texture_from_render_target_if_applicable(absolute_address);

	/**
	 * Calling read_buffers will overwrite cached content
	 */

	__glcheck m_flip_fbo.recreate();
	m_flip_fbo.bind();

	auto *flip_fbo = &m_flip_fbo;

	if (render_target_texture)
	{
		__glcheck m_flip_fbo.color = *render_target_texture;
		__glcheck m_flip_fbo.read_buffer(m_flip_fbo.color);
	}
	else if (draw_fbo)
	{
		//HACK! it's here, because textures cache isn't implemented correctly!
		flip_fbo = &draw_fbo;
	}
	else
	{
		if (!m_flip_tex_color || m_flip_tex_color.size() != sizei{ (int)buffer_width, (int)buffer_height })
		{
			m_flip_tex_color.recreate(gl::texture::target::texture2D);

			__glcheck m_flip_tex_color.config()
				.size({ (int)buffer_width, (int)buffer_height })
				.type(gl::texture::type::uint_8_8_8_8)
				.format(gl::texture::format::bgra);

			m_flip_tex_color.pixel_unpack_settings().aligment(1).row_length(buffer_pitch / 4);
		}

		if (buffer_region.tile)
		{
			std::unique_ptr<u8[]> temp(new u8[buffer_height * buffer_pitch]);
			buffer_region.read(temp.get(), buffer_width, buffer_height, buffer_pitch);
			__glcheck m_flip_tex_color.copy_from(temp.get(), gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8);
		}
		else
		{
			__glcheck m_flip_tex_color.copy_from(buffer_region.ptr, gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8);
		}

		m_flip_fbo.color = m_flip_tex_color;
		__glcheck m_flip_fbo.read_buffer(m_flip_fbo.color);
	}

	areai screen_area = coordi({}, { (int)buffer_width, (int)buffer_height });

	coordi aspect_ratio;
	if (1) //enable aspect ratio
	{
		sizei csize(m_frame->client_width(), m_frame->client_height());
		sizei new_size = csize;

		const double aq = (double)buffer_width / buffer_height;
		const double rq = (double)new_size.width / new_size.height;
		const double q = aq / rq;

		if (q > 1.0)
		{
			new_size.height = int(new_size.height / q);
			aspect_ratio.y = (csize.height - new_size.height) / 2;
		}
		else if (q < 1.0)
		{
			new_size.width = int(new_size.width * q);
			aspect_ratio.x = (csize.width - new_size.width) / 2;
		}

		aspect_ratio.size = new_size;
	}
	else
	{
		aspect_ratio.size = { m_frame->client_width(), m_frame->client_height() };
	}

	gl::screen.clear(gl::buffers::color_depth_stencil);

	__glcheck flip_fbo->blit(gl::screen, screen_area, areai(aspect_ratio).flipped_vertical());

	m_frame->flip(m_context);
	
	if (g_cfg_rsx_overlay)
	{
		//TODO: Display overlay in a cross-platform manner
		//Core context throws wgl font functions out of the window as they use display lists
		//Only show debug info if the user really requests it

		if (g_cfg_rsx_debug_output)
		{
			std::string message =
				"draw_calls: " + std::to_string(m_draw_calls) + ", " + "draw_call_setup: " + std::to_string(m_begin_time) + "us, " + "vertex_upload_time: " + std::to_string(m_vertex_upload_time) + "us, " + "draw_call_execution: " + std::to_string(m_draw_time) + "us";

			LOG_ERROR(RSX, message.c_str());
		}
	}

	m_draw_calls = 0;
	m_begin_time = 0;
	m_draw_time = 0;
	m_vertex_upload_time = 0;

	for (auto &tex : m_rtts.invalidated_resources)
	{
		tex->remove();
	}

	m_rtts.invalidated_resources.clear();
}


u64 GLGSRender::timestamp() const
{
	GLint64 result;
	glGetInteger64v(GL_TIMESTAMP, &result);
	return result;
}

bool GLGSRender::on_access_violation(u32 address, bool is_writing)
{
	if (is_writing) return m_gl_texture_cache.mark_as_dirty(address);
	return false;
}
