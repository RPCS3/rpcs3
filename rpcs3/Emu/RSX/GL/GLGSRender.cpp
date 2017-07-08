#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GLGSRender.h"
#include "GLVertexProgram.h"
#include "../rsx_methods.h"
#include "../Common/BufferUtils.h"
#include "../rsx_utils.h"

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
		fmt::throw_exception("Unknown depth format" HERE);
	}
}

GLGSRender::GLGSRender() : GSRender()
{
	//TODO
	//shaders_cache.load(rsx::old_shaders_cache::shader_language::glsl);
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

namespace
{
	GLenum comparison_op(rsx::comparison_function op)
	{
		switch (op)
		{
		case rsx::comparison_function::never: return GL_NEVER;
		case rsx::comparison_function::less: return GL_LESS;
		case rsx::comparison_function::equal: return GL_EQUAL;
		case rsx::comparison_function::less_or_equal: return GL_LEQUAL;
		case rsx::comparison_function::greater: return GL_GREATER;
		case rsx::comparison_function::not_equal: return GL_NOTEQUAL;
		case rsx::comparison_function::greater_or_equal: return GL_GEQUAL;
		case rsx::comparison_function::always: return GL_ALWAYS;
		}
		throw;
	}

	GLenum stencil_op(rsx::stencil_op op)
	{
		switch (op)
		{
		case rsx::stencil_op::invert: return GL_INVERT;
		case rsx::stencil_op::keep: return GL_KEEP;
		case rsx::stencil_op::zero: return GL_ZERO;
		case rsx::stencil_op::replace: return GL_REPLACE;
		case rsx::stencil_op::incr: return GL_INCR;
		case rsx::stencil_op::decr: return GL_DECR;
		case rsx::stencil_op::incr_wrap: return GL_INCR_WRAP;
		case rsx::stencil_op::decr_wrap: return GL_DECR_WRAP;
		}
		throw;
	}

	GLenum blend_equation(rsx::blend_equation op)
	{
		switch (op)
		{
			// Note : maybe add is signed on gl
		case rsx::blend_equation::add: return GL_FUNC_ADD;
		case rsx::blend_equation::min: return GL_MIN;
		case rsx::blend_equation::max: return GL_MAX;
		case rsx::blend_equation::substract: return GL_FUNC_SUBTRACT;
		case rsx::blend_equation::reverse_substract: return GL_FUNC_REVERSE_SUBTRACT;
		case rsx::blend_equation::reverse_substract_signed: throw "unsupported";
		case rsx::blend_equation::add_signed: throw "unsupported";
		case rsx::blend_equation::reverse_add_signed: throw "unsupported";
		}
		throw;
	}

	GLenum blend_factor(rsx::blend_factor op)
	{
		switch (op)
		{
		case rsx::blend_factor::zero: return GL_ZERO;
		case rsx::blend_factor::one: return GL_ONE;
		case rsx::blend_factor::src_color: return GL_SRC_COLOR;
		case rsx::blend_factor::one_minus_src_color: return GL_ONE_MINUS_SRC_COLOR;
		case rsx::blend_factor::dst_color: return GL_DST_COLOR;
		case rsx::blend_factor::one_minus_dst_color: return GL_ONE_MINUS_DST_COLOR;
		case rsx::blend_factor::src_alpha: return GL_SRC_ALPHA;
		case rsx::blend_factor::one_minus_src_alpha: return GL_ONE_MINUS_SRC_ALPHA;
		case rsx::blend_factor::dst_alpha: return GL_DST_ALPHA;
		case rsx::blend_factor::one_minus_dst_alpha: return GL_ONE_MINUS_DST_ALPHA;
		case rsx::blend_factor::src_alpha_saturate: return GL_SRC_ALPHA_SATURATE;
		case rsx::blend_factor::constant_color: return GL_CONSTANT_COLOR;
		case rsx::blend_factor::one_minus_constant_color: return GL_ONE_MINUS_CONSTANT_COLOR;
		case rsx::blend_factor::constant_alpha: return GL_CONSTANT_ALPHA;
		case rsx::blend_factor::one_minus_constant_alpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
		}
		throw;
	}

	GLenum logic_op(rsx::logic_op op)
	{
		switch (op)
		{
		case rsx::logic_op::logic_clear: return GL_CLEAR;
		case rsx::logic_op::logic_and: return GL_AND;
		case rsx::logic_op::logic_and_reverse: return GL_AND_REVERSE;
		case rsx::logic_op::logic_copy: return GL_COPY;
		case rsx::logic_op::logic_and_inverted: return GL_AND_INVERTED;
		case rsx::logic_op::logic_noop: return GL_NOOP;
		case rsx::logic_op::logic_xor: return GL_XOR;
		case rsx::logic_op::logic_or: return GL_OR;
		case rsx::logic_op::logic_nor: return GL_NOR;
		case rsx::logic_op::logic_equiv: return GL_EQUIV;
		case rsx::logic_op::logic_invert: return GL_INVERT;
		case rsx::logic_op::logic_or_reverse: return GL_OR_REVERSE;
		case rsx::logic_op::logic_copy_inverted: return GL_COPY_INVERTED;
		case rsx::logic_op::logic_or_inverted: return GL_OR_INVERTED;
		case rsx::logic_op::logic_nand: return GL_NAND;
		case rsx::logic_op::logic_set: return GL_SET;
		}
		throw;
	}

	GLenum front_face(rsx::front_face op)
	{
		bool invert = (rsx::method_registers.shader_window_origin() == rsx::window_origin::bottom);

		switch (op)
		{
		case rsx::front_face::cw: return (invert ? GL_CCW : GL_CW);
		case rsx::front_face::ccw: return (invert ? GL_CW : GL_CCW);
		}
		throw;
	}

	GLenum cull_face(rsx::cull_face op)
	{
		bool invert = (rsx::method_registers.shader_window_origin() == rsx::window_origin::top);

		switch (op)
		{
		case rsx::cull_face::front: return (invert ? GL_BACK : GL_FRONT);
		case rsx::cull_face::back: return (invert ? GL_FRONT : GL_BACK);
		case rsx::cull_face::front_and_back: return GL_FRONT_AND_BACK;
		}
		throw;
	}
}

void GLGSRender::begin()
{
	rsx::thread::begin();

	if (skip_frame)
		return;

	init_buffers();

	if (!framebuffer_status_valid)
		return;

	std::chrono::time_point<steady_clock> then = steady_clock::now();

	bool color_mask_b = rsx::method_registers.color_mask_b();
	bool color_mask_g = rsx::method_registers.color_mask_g();
	bool color_mask_r = rsx::method_registers.color_mask_r();
	bool color_mask_a = rsx::method_registers.color_mask_a();

	__glcheck glColorMask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);
	__glcheck glDepthMask(rsx::method_registers.depth_write_enabled());
	__glcheck glStencilMask(rsx::method_registers.stencil_mask());

	if (__glcheck enable(rsx::method_registers.depth_test_enabled(), GL_DEPTH_TEST))
	{
		__glcheck glDepthFunc(comparison_op(rsx::method_registers.depth_func()));
	}

	if (glDepthBoundsEXT && (__glcheck enable(rsx::method_registers.depth_bounds_test_enabled(), GL_DEPTH_BOUNDS_TEST_EXT)))
	{
		__glcheck glDepthBoundsEXT(rsx::method_registers.depth_bounds_min(), rsx::method_registers.depth_bounds_max());
	}

	//__glcheck glDepthRange(rsx::method_registers.clip_min(), rsx::method_registers.clip_max());
	__glcheck enable(rsx::method_registers.dither_enabled(), GL_DITHER);

	if (__glcheck enable(rsx::method_registers.blend_enabled(), GL_BLEND))
	{
		__glcheck glBlendFuncSeparate(blend_factor(rsx::method_registers.blend_func_sfactor_rgb()),
			blend_factor(rsx::method_registers.blend_func_dfactor_rgb()),
			blend_factor(rsx::method_registers.blend_func_sfactor_a()),
			blend_factor(rsx::method_registers.blend_func_dfactor_a()));

		auto blend_colors = rsx::get_constant_blend_colors();
		__glcheck glBlendColor(blend_colors[0], blend_colors[1], blend_colors[2], blend_colors[3]);

		__glcheck glBlendEquationSeparate(blend_equation(rsx::method_registers.blend_equation_rgb()),
			blend_equation(rsx::method_registers.blend_equation_a()));
	}

	if (__glcheck enable(rsx::method_registers.stencil_test_enabled(), GL_STENCIL_TEST))
	{
		__glcheck glStencilFunc(comparison_op(rsx::method_registers.stencil_func()), rsx::method_registers.stencil_func_ref(),
			rsx::method_registers.stencil_func_mask());
		__glcheck glStencilOp(stencil_op(rsx::method_registers.stencil_op_fail()), stencil_op(rsx::method_registers.stencil_op_zfail()),
			stencil_op(rsx::method_registers.stencil_op_zpass()));

		if (rsx::method_registers.two_sided_stencil_test_enabled())
		{
			__glcheck glStencilMaskSeparate(GL_BACK, rsx::method_registers.back_stencil_mask());
			__glcheck glStencilFuncSeparate(GL_BACK, comparison_op(rsx::method_registers.back_stencil_func()),
				rsx::method_registers.back_stencil_func_ref(), rsx::method_registers.back_stencil_func_mask());
			__glcheck glStencilOpSeparate(GL_BACK, stencil_op(rsx::method_registers.back_stencil_op_fail()),
				stencil_op(rsx::method_registers.back_stencil_op_zfail()), stencil_op(rsx::method_registers.back_stencil_op_zpass()));
		}
	}

	__glcheck enable(rsx::method_registers.blend_enabled_surface_1(), GL_BLEND, 1);
	__glcheck enable(rsx::method_registers.blend_enabled_surface_2(), GL_BLEND, 2);
	__glcheck enable(rsx::method_registers.blend_enabled_surface_3(), GL_BLEND, 3);

	if (__glcheck enable(rsx::method_registers.logic_op_enabled(), GL_COLOR_LOGIC_OP))
	{
		__glcheck glLogicOp(logic_op(rsx::method_registers.logic_operation()));
	}

	__glcheck glLineWidth(rsx::method_registers.line_width());
	__glcheck enable(rsx::method_registers.line_smooth_enabled(), GL_LINE_SMOOTH);

	//TODO
	//NV4097_SET_ANISO_SPREAD

	__glcheck enable(rsx::method_registers.poly_offset_point_enabled(), GL_POLYGON_OFFSET_POINT);
	__glcheck enable(rsx::method_registers.poly_offset_line_enabled(), GL_POLYGON_OFFSET_LINE);
	__glcheck enable(rsx::method_registers.poly_offset_fill_enabled(), GL_POLYGON_OFFSET_FILL);

	__glcheck glPolygonOffset(rsx::method_registers.poly_offset_scale(),
		rsx::method_registers.poly_offset_bias());

	//NV4097_SET_SPECULAR_ENABLE
	//NV4097_SET_TWO_SIDE_LIGHT_EN
	//NV4097_SET_FLAT_SHADE_OP
	//NV4097_SET_EDGE_FLAG

	if (__glcheck enable(rsx::method_registers.cull_face_enabled(), GL_CULL_FACE))
	{
		__glcheck glCullFace(cull_face(rsx::method_registers.cull_face_mode()));
	}

	__glcheck glFrontFace(front_face(rsx::method_registers.front_face_mode()));

	//NV4097_SET_COLOR_KEY_COLOR
	//NV4097_SET_SHADER_CONTROL
	//NV4097_SET_ZMIN_MAX_CONTROL
	//NV4097_SET_ANTI_ALIASING_CONTROL
	//NV4097_SET_CLIP_ID_TEST_ENABLE

	std::chrono::time_point<steady_clock> now = steady_clock::now();
	m_begin_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(now - then).count();
}

namespace
{
	GLenum get_gl_target_for_texture(const rsx::fragment_texture& tex)
	{
		switch (tex.get_extended_texture_dimension())
		{
		case rsx::texture_dimension_extended::texture_dimension_1d: return GL_TEXTURE_1D;
		case rsx::texture_dimension_extended::texture_dimension_2d: return GL_TEXTURE_2D;
		case rsx::texture_dimension_extended::texture_dimension_cubemap: return GL_TEXTURE_CUBE_MAP;
		case rsx::texture_dimension_extended::texture_dimension_3d: return GL_TEXTURE_3D;
		}
		fmt::throw_exception("Unknown texture target" HERE);
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
		fmt::throw_exception("Unknown texture target" HERE);
	}
}

void GLGSRender::end()
{
	if (skip_frame || !framebuffer_status_valid)
	{
		rsx::thread::end();
		return;
	}

	std::chrono::time_point<steady_clock> program_start = steady_clock::now();

	//Load program here since it is dependent on vertex state
	load_program();

	std::chrono::time_point<steady_clock> program_stop = steady_clock::now();
	m_begin_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(program_stop - program_start).count();

	if (manually_flush_ring_buffers)
	{
		//Use approximations to reseve space. This path is mostly for debug purposes anyway
		u32 approx_vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
		u32 approx_working_buffer_size = approx_vertex_count * 256;

		//Allocate 256K heap if we have no approximation at this time (inlined array)
		m_attrib_ring_buffer->reserve_storage_on_heap(std::max(approx_working_buffer_size, 256 * 1024U));
		m_index_ring_buffer->reserve_storage_on_heap(16 * 1024);
	}

	//Check if depth buffer is bound and valid
	//If ds is not initialized clear it; it seems new depth textures should have depth cleared
	auto copy_rtt_contents = [](gl::render_target *surface)
	{
		//Copy data from old contents onto this one
		//1. Clip a rectangular region defning the data
		//2. Perform a GPU blit
		u16 parent_w = surface->old_contents->width();
		u16 parent_h = surface->old_contents->height();
		u16 copy_w, copy_h;

		std::tie(std::ignore, std::ignore, copy_w, copy_h) = rsx::clip_region<u16>(parent_w, parent_h, 0, 0, surface->width(), surface->height(), true);
		glCopyImageSubData(surface->old_contents->id(), GL_TEXTURE_2D, 0, 0, 0, 0, surface->id(), GL_TEXTURE_2D, 0, 0, 0, 0, copy_w, copy_h, 1);
		surface->set_cleared();
		surface->old_contents = nullptr;
	};

	gl::render_target *ds = std::get<1>(m_rtts.m_bound_depth_stencil);
	if (ds && !ds->cleared())
	{
		//Temporarily disable pixel tests
		glDisable(GL_SCISSOR_TEST);
		glDepthMask(GL_TRUE);
		
		glClearDepth(1.0);
		glClearStencil(255);

		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		if (g_cfg.video.strict_rendering_mode)
		{
			//Copy previous data if any
			if (ds->old_contents != nullptr)
				copy_rtt_contents(ds);
		}

		glDepthMask(rsx::method_registers.depth_write_enabled());
		glEnable(GL_SCISSOR_TEST);

		ds->set_cleared();
	}

	if (g_cfg.video.strict_rendering_mode)
	{
		for (auto &rtt : m_rtts.m_bound_render_targets)
		{
			if (std::get<0>(rtt) != 0)
			{
				auto surface = std::get<1>(rtt);
				if (!surface->cleared() && surface->old_contents != nullptr)
					copy_rtt_contents(surface);
			}
		}
	}

	std::chrono::time_point<steady_clock> textures_start = steady_clock::now();

	//Setup textures
	//Setting unused texture to 0 is not needed, but makes program validation happy if we choose to enforce it
	for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
	{
		int location;
		if (!rsx::method_registers.fragment_textures[i].enabled())
		{
			if (m_textures_dirty[i])
			{
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(GL_TEXTURE_2D, 0);

				m_textures_dirty[i] = false;
			}
			continue;
		}

		if (m_program->uniforms.has_location("tex" + std::to_string(i), &location))
		{
			m_gl_textures[i].set_target(get_gl_target_for_texture(rsx::method_registers.fragment_textures[i]));
			__glcheck m_gl_texture_cache.upload_texture(i, rsx::method_registers.fragment_textures[i], m_gl_textures[i], m_rtts);
			__glcheck m_gl_sampler_states[i].apply(rsx::method_registers.fragment_textures[i]);
		}
	}

	//Vertex textures
	for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
	{
		int texture_index = i + rsx::limits::fragment_textures_count;
		int location;

/*		if (!rsx::method_registers.vertex_textures[i].enabled())
		{
			glActiveTexture(GL_TEXTURE0 + texture_index);
			glBindTexture(GL_TEXTURE_2D, 0);
			continue;
		} */

		if (m_program->uniforms.has_location("vtex" + std::to_string(i), &location))
		{
			m_gl_vertex_textures[i].set_target(get_gl_target_for_texture(rsx::method_registers.vertex_textures[i]));
			__glcheck m_gl_texture_cache.upload_texture(texture_index, rsx::method_registers.vertex_textures[i], m_gl_vertex_textures[i], m_rtts);
		}
	}

	std::chrono::time_point<steady_clock> textures_end = steady_clock::now();
	m_textures_upload_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(textures_end - textures_start).count();

	u32 vertex_draw_count = m_last_vertex_count;
	std::optional<std::tuple<GLenum, u32> > indexed_draw_info;
	bool skip_upload = false;

	if (!is_probable_instanced_draw())
	{
		std::tie(vertex_draw_count, indexed_draw_info) = set_vertex_buffer();
		m_last_vertex_count = vertex_draw_count;
	}
	else
	{
		//LOG_ERROR(RSX, "No work is needed for this draw call! Muhahahahahahaha");
		skip_upload = true;
	}

	std::chrono::time_point<steady_clock> draw_start = steady_clock::now();

	if (g_cfg.video.debug_output)
	{
		m_program->validate();
	}

	if (manually_flush_ring_buffers)
	{
		m_attrib_ring_buffer->unmap();
		m_index_ring_buffer->unmap();
	}

	if (indexed_draw_info || (skip_upload && m_last_draw_indexed == true))
	{
		if (__glcheck enable(rsx::method_registers.restart_index_enabled(), GL_PRIMITIVE_RESTART))
		{
			GLenum index_type = (skip_upload)? m_last_ib_type: std::get<0>(indexed_draw_info.value());
			__glcheck glPrimitiveRestartIndex((index_type == GL_UNSIGNED_SHORT)? 0xffff: 0xffffffff);
		}

		m_last_draw_indexed = true;

		if (!skip_upload)
		{
			m_last_ib_type = std::get<0>(indexed_draw_info.value());
			m_last_index_offset = std::get<1>(indexed_draw_info.value());
		}

		__glcheck glDrawElements(gl::draw_mode(rsx::method_registers.current_draw_clause.primitive), vertex_draw_count, m_last_ib_type, (GLvoid *)(std::ptrdiff_t)m_last_index_offset);
	}
	else
	{
		draw_fbo.draw_arrays(rsx::method_registers.current_draw_clause.primitive, vertex_draw_count);
		m_last_draw_indexed = false;
	}

	m_attrib_ring_buffer->notify();
	m_index_ring_buffer->notify();
	m_scale_offset_buffer->notify();
	m_fragment_constants_buffer->notify();
	m_transform_constants_buffer->notify();

	std::chrono::time_point<steady_clock> draw_end = steady_clock::now();
	m_draw_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(draw_end - draw_start).count();

	m_draw_calls++;

	synchronize_buffers();

	rsx::thread::end();
}

void GLGSRender::set_viewport()
{
	//NOTE: scale offset matrix already contains the viewport transformation
	const auto clip_width = rsx::method_registers.surface_clip_width();
	const auto clip_height = rsx::method_registers.surface_clip_height();
	glViewport(0, 0, clip_width, clip_height);

	u16 scissor_x = rsx::method_registers.scissor_origin_x();
	u16 scissor_w = rsx::method_registers.scissor_width();
	u16 scissor_y = rsx::method_registers.scissor_origin_y();
	u16 scissor_h = rsx::method_registers.scissor_height();

	//Do not bother drawing anything if output is zero sized
	//TODO: Clip scissor region
	if (scissor_x >= clip_width || scissor_y >= clip_height || scissor_w == 0 || scissor_h == 0)
	{
		if (!g_cfg.video.strict_rendering_mode)
		{
			framebuffer_status_valid = false;
			return;
		}
	}

	//NOTE: window origin does not affect scissor region (probably only affects viewport matrix; already applied)
	//See LIMBO [NPUB-30373] which uses shader window origin = top
	__glcheck glScissor(scissor_x, scissor_y, scissor_w, scissor_h);
	glEnable(GL_SCISSOR_TEST);
}

void GLGSRender::on_init_thread()
{
	GSRender::on_init_thread();

	gl::init();

	if (g_cfg.video.debug_output)
		gl::enable_debugging();

	LOG_NOTICE(RSX, "%s", (const char*)glGetString(GL_VERSION));
	LOG_NOTICE(RSX, "%s", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
	LOG_NOTICE(RSX, "%s", (const char*)glGetString(GL_VENDOR));

	auto& gl_caps = gl::get_driver_caps();

	if (!gl_caps.ARB_texture_buffer_supported)
	{
		fmt::throw_exception("Failed to initialize OpenGL renderer. ARB_texture_buffer_object is required but not supported by your GPU");
	}

	if (!gl_caps.ARB_dsa_supported && !gl_caps.EXT_dsa_supported)
	{
		fmt::throw_exception("Failed to initialize OpenGL renderer. ARB_direct_state_access or EXT_direct_state_access is required but not supported by your GPU");
	}

	if (!gl_caps.ARB_depth_buffer_float_supported && g_cfg.video.force_high_precision_z_buffer)
	{
		LOG_WARNING(RSX, "High precision Z buffer requested but your GPU does not support GL_ARB_depth_buffer_float. Option ignored.");
	}

	if (!gl_caps.ARB_texture_barrier_supported && !gl_caps.NV_texture_barrier_supported && !g_cfg.video.strict_rendering_mode)
	{
		LOG_WARNING(RSX, "Texture barriers are not supported by your GPU. Feedback loops will have undefined results.");
	}

	//Use industry standard resource alignment values as defaults
	m_uniform_buffer_offset_align = 256;
	m_min_texbuffer_alignment = 256;

	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_uniform_buffer_offset_align);
	glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &m_min_texbuffer_alignment);
	m_vao.create();

	//Set min alignment to 16-bytes for SSE optimizations with aligned addresses to work
	m_min_texbuffer_alignment = std::max(m_min_texbuffer_alignment, 16);
	m_uniform_buffer_offset_align = std::max(m_uniform_buffer_offset_align, 16);

	const u32 texture_index_offset = rsx::limits::fragment_textures_count + rsx::limits::vertex_textures_count;

	for (int index = 0; index < rsx::limits::vertex_count; ++index)
	{
		auto &tex = m_gl_attrib_buffers[index];
		tex.create();
		tex.set_target(gl::texture::target::textureBuffer);

		glActiveTexture(GL_TEXTURE0 + texture_index_offset + index);
		tex.bind();
	}

	if (!gl_caps.ARB_buffer_storage_supported)
	{
		LOG_WARNING(RSX, "Forcing use of legacy OpenGL buffers because ARB_buffer_storage is not supported");
		// TODO: do not modify config options
		g_cfg.video.gl_legacy_buffers.from_string("true");
	}

	if (g_cfg.video.gl_legacy_buffers)
	{
		LOG_WARNING(RSX, "Using legacy openGL buffers.");
		manually_flush_ring_buffers = true;

		m_attrib_ring_buffer.reset(new gl::legacy_ring_buffer());
		m_transform_constants_buffer.reset(new gl::legacy_ring_buffer());
		m_fragment_constants_buffer.reset(new gl::legacy_ring_buffer());
		m_scale_offset_buffer.reset(new gl::legacy_ring_buffer());
		m_index_ring_buffer.reset(new gl::legacy_ring_buffer());
	}
	else
	{
		m_attrib_ring_buffer.reset(new gl::ring_buffer());
		m_transform_constants_buffer.reset(new gl::ring_buffer());
		m_fragment_constants_buffer.reset(new gl::ring_buffer());
		m_scale_offset_buffer.reset(new gl::ring_buffer());
		m_index_ring_buffer.reset(new gl::ring_buffer());
	}

	m_attrib_ring_buffer->create(gl::buffer::target::texture, 256 * 0x100000);
	m_index_ring_buffer->create(gl::buffer::target::element_array, 64 * 0x100000);
	m_transform_constants_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_fragment_constants_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_scale_offset_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);

	m_vao.element_array_buffer = *m_index_ring_buffer;

	if (g_cfg.video.overlay)
	{
		if (gl_caps.ARB_shader_draw_parameters_supported)
		{
			m_text_printer.init();
			m_text_printer.set_enabled(true);
		}
	}

	for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
	{
		m_gl_sampler_states[i].create();
		m_gl_sampler_states[i].bind(i);
	}

	//Clip planes are shader controlled; enable all planes driver-side
	glEnable(GL_CLIP_DISTANCE0 + 0);
	glEnable(GL_CLIP_DISTANCE0 + 1);
	glEnable(GL_CLIP_DISTANCE0 + 2);
	glEnable(GL_CLIP_DISTANCE0 + 3);
	glEnable(GL_CLIP_DISTANCE0 + 4);
	glEnable(GL_CLIP_DISTANCE0 + 5);

	m_gl_texture_cache.initialize(this);
}

void GLGSRender::on_exit()
{
	glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);

	m_prog_buffer.clear();

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

	for (auto &sampler : m_gl_sampler_states)
	{
		sampler.remove();
	}

	if (m_attrib_ring_buffer)
	{
		m_attrib_ring_buffer->remove();
	}

	if (m_transform_constants_buffer)
	{
		m_transform_constants_buffer->remove();
	}

	if (m_fragment_constants_buffer)
	{
		m_fragment_constants_buffer->remove();
	}

	if (m_scale_offset_buffer)
	{
		m_scale_offset_buffer->remove();
	}

	if (m_index_ring_buffer)
	{
		m_index_ring_buffer->remove();
	}

	m_text_printer.close();
	m_gl_texture_cache.close();

	return GSRender::on_exit();
}

void GLGSRender::clear_surface(u32 arg)
{
	if (skip_frame || !framebuffer_status_valid) return;
	if (rsx::method_registers.surface_color_target() == rsx::surface_target::none) return;
	if ((arg & 0xf3) == 0) return;

	GLbitfield mask = 0;

	rsx::surface_depth_format surface_depth_format = rsx::method_registers.surface_depth_fmt();

	if (arg & 0x1)
	{
		u32 max_depth_value = get_max_depth_value(surface_depth_format);
		u32 clear_depth = rsx::method_registers.z_clear_value(surface_depth_format == rsx::surface_depth_format::z24s8);

		glDepthMask(GL_TRUE);
		glClearDepth(double(clear_depth) / max_depth_value);
		mask |= GLenum(gl::buffers::depth);

		gl::render_target *ds = std::get<1>(m_rtts.m_bound_depth_stencil);
		if (ds && !ds->cleared())
		{
			ds->set_cleared();
			ds->old_contents = nullptr;
		}
	}

	if (surface_depth_format == rsx::surface_depth_format::z24s8 && (arg & 0x2))
	{
		u8 clear_stencil = rsx::method_registers.stencil_clear_value();

		__glcheck glStencilMask(rsx::method_registers.stencil_mask());
		glClearStencil(clear_stencil);

		mask |= GLenum(gl::buffers::stencil);
	}

	if (arg & 0xf0)
	{
		u8 clear_a = rsx::method_registers.clear_color_a();
		u8 clear_r = rsx::method_registers.clear_color_r();
		u8 clear_g = rsx::method_registers.clear_color_g();
		u8 clear_b = rsx::method_registers.clear_color_b();

		glColorMask(((arg & 0x20) ? 1 : 0), ((arg & 0x40) ? 1 : 0), ((arg & 0x80) ? 1 : 0), ((arg & 0x10) ? 1 : 0));
		glClearColor(clear_r / 255.f, clear_g / 255.f, clear_b / 255.f, clear_a / 255.f);

		mask |= GLenum(gl::buffers::color);

		for (auto &rtt : m_rtts.m_bound_render_targets)
		{
			if (std::get<0>(rtt) != 0)
			{
				std::get<1>(rtt)->set_cleared(true);
				std::get<1>(rtt)->old_contents = nullptr;
			}
		}
	}

	glClear(mask);
}

bool GLGSRender::do_method(u32 cmd, u32 arg)
{
	switch (cmd)
	{
	case NV4097_CLEAR_SURFACE:
	{
		init_buffers(true);
		synchronize_buffers();
		clear_surface(arg);
		return true;
	}
	case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
	case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
		flush_draw_buffers = true;
		return true;
	}

	return false;
}

bool GLGSRender::load_program()
{
	auto rtt_lookup_func = [this](u32 texaddr, rsx::fragment_texture &tex, bool is_depth) -> std::tuple<bool, u16>
	{
		gl::render_target *surface = nullptr;
		if (!is_depth)
			surface = m_rtts.get_texture_from_render_target_if_applicable(texaddr);
		else
			surface = m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr);

		if (!surface)
		{
			auto rsc = m_rtts.get_surface_subresource_if_applicable(texaddr, 0, 0, tex.pitch());
			if (!rsc.surface || rsc.is_depth_surface != is_depth)
				return std::make_tuple(false, 0);

			surface = rsc.surface;
		}

		return std::make_tuple(true, surface->get_native_pitch());
	};

	RSXVertexProgram vertex_program = get_current_vertex_program();
	RSXFragmentProgram fragment_program = get_current_fragment_program(rtt_lookup_func);

	u32 unnormalized_rtts = 0;

	for (auto &vtx : vertex_program.rsx_vertex_inputs)
	{
		auto &array_info = rsx::method_registers.vertex_arrays_info[vtx.location];
		if (array_info.type() == rsx::vertex_base_type::s1 ||
			array_info.type() == rsx::vertex_base_type::cmp)
		{
			//Some vendors do not support GL_x_SNORM buffer textures
			verify(HERE), vtx.flags == 0;
			vtx.flags |= GL_VP_FORCE_ATTRIB_SCALING | GL_VP_ATTRIB_S16_INT;
		}
	}

	auto old_program = m_program;
	m_program = &m_prog_buffer.getGraphicPipelineState(vertex_program, fragment_program, nullptr);
	m_program->use();

	u8 *buf;
	u32 scale_offset_offset;
	u32 vertex_constants_offset;
	u32 fragment_constants_offset;

	const u32 fragment_constants_size = (const u32)m_prog_buffer.get_fragment_constants_buffer_size(fragment_program);
	const u32 fragment_buffer_size = fragment_constants_size + (17 * 4 * sizeof(float));

	if (manually_flush_ring_buffers)
	{
		m_scale_offset_buffer->reserve_storage_on_heap(512);
		m_fragment_constants_buffer->reserve_storage_on_heap(align(fragment_buffer_size, 256));
		if (m_transform_constants_dirty) m_transform_constants_buffer->reserve_storage_on_heap(8192);
	}

	// Scale offset
	auto mapping = m_scale_offset_buffer->alloc_from_heap(512, m_uniform_buffer_offset_align);
	buf = static_cast<u8*>(mapping.first);
	scale_offset_offset = mapping.second;
	fill_scale_offset_data(buf, false);
	fill_user_clip_data((char *)buf + 64);

	if (m_transform_constants_dirty)
	{
		// Vertex constants
		mapping = m_transform_constants_buffer->alloc_from_heap(8192, m_uniform_buffer_offset_align);
		buf = static_cast<u8*>(mapping.first);
		vertex_constants_offset = mapping.second;
		fill_vertex_program_constants_data(buf);
		*(reinterpret_cast<u32*>(buf + (468 * 4 * sizeof(float)))) = rsx::method_registers.transform_branch_bits();
	}

	// Fragment constants
	mapping = m_fragment_constants_buffer->alloc_from_heap(fragment_buffer_size, m_uniform_buffer_offset_align);
	buf = static_cast<u8*>(mapping.first);
	fragment_constants_offset = mapping.second;
	if (fragment_constants_size)
		m_prog_buffer.fill_fragment_constants_buffer({ reinterpret_cast<float*>(buf), gsl::narrow<int>(fragment_constants_size) }, fragment_program);

	// Fragment state
	fill_fragment_state_buffer(buf+fragment_constants_size, fragment_program);

	m_scale_offset_buffer->bind_range(0, scale_offset_offset, 512);
	m_fragment_constants_buffer->bind_range(2, fragment_constants_offset, fragment_buffer_size);

	if (m_transform_constants_dirty) m_transform_constants_buffer->bind_range(1, vertex_constants_offset, 8192);

	if (manually_flush_ring_buffers)
	{
		m_scale_offset_buffer->unmap();
		m_fragment_constants_buffer->unmap();

		if (m_transform_constants_dirty) m_transform_constants_buffer->unmap();
	}

	m_transform_constants_dirty = false;
	return true;
}

void GLGSRender::flip(int buffer)
{
	if (skip_frame)
	{
		m_frame->flip(m_context, true);
		rsx::thread::flip(buffer);

		if (!skip_frame)
		{
			m_draw_calls = 0;
			m_begin_time = 0;
			m_draw_time = 0;
			m_vertex_upload_time = 0;
			m_textures_upload_time = 0;
		}

		return;
	}

	u32 buffer_width = gcm_buffers[buffer].width;
	u32 buffer_height = gcm_buffers[buffer].height;
	u32 buffer_pitch = gcm_buffers[buffer].pitch;

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);

	rsx::tiled_region buffer_region = get_tiled_address(gcm_buffers[buffer].offset, CELL_GCM_LOCATION_LOCAL);
	u32 absolute_address = buffer_region.address + buffer_region.base;

	gl::texture *render_target_texture = m_rtts.get_texture_from_render_target_if_applicable(absolute_address);

	__glcheck m_flip_fbo.recreate();
	m_flip_fbo.bind();

	auto *flip_fbo = &m_flip_fbo;

	if (render_target_texture)
	{
		__glcheck m_flip_fbo.color = *render_target_texture;
		__glcheck m_flip_fbo.read_buffer(m_flip_fbo.color);
	}
	else
	{
		LOG_WARNING(RSX, "Flip texture was not found in cache. Uploading surface from CPU");

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

	sizei csize(m_frame->client_width(), m_frame->client_height());
	sizei new_size = csize;

	if (!g_cfg.video.stretch_to_display_area)
	{
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
	}

	aspect_ratio.size = new_size;

	gl::screen.clear(gl::buffers::color_depth_stencil);

	__glcheck flip_fbo->blit(gl::screen, screen_area, areai(aspect_ratio).flipped_vertical(), gl::buffers::color, gl::filter::linear);

	if (g_cfg.video.overlay)
	{
		gl::screen.bind();
		glViewport(0, 0, m_frame->client_width(), m_frame->client_height());

		m_text_printer.print_text(0, 0, m_frame->client_width(), m_frame->client_height(), "draw calls: " + std::to_string(m_draw_calls));
		m_text_printer.print_text(0, 18, m_frame->client_width(), m_frame->client_height(), "draw call setup: " + std::to_string(m_begin_time) + "us");
		m_text_printer.print_text(0, 36, m_frame->client_width(), m_frame->client_height(), "vertex upload time: " + std::to_string(m_vertex_upload_time) + "us");
		m_text_printer.print_text(0, 54, m_frame->client_width(), m_frame->client_height(), "textures upload time: " + std::to_string(m_textures_upload_time) + "us");
		m_text_printer.print_text(0, 72, m_frame->client_width(), m_frame->client_height(), "draw call execution: " + std::to_string(m_draw_time) + "us");
	}

	m_frame->flip(m_context);
	rsx::thread::flip(buffer);

	m_gl_texture_cache.clear_temporary_surfaces();

	for (auto &tex : m_rtts.invalidated_resources)
		tex->remove();

	m_rtts.invalidated_resources.clear();

	if (g_cfg.video.invalidate_surface_cache_every_frame)
		m_rtts.invalidate_surface_cache_data(nullptr);

	//If we are skipping the next frame, fo not reset perf counters
	if (skip_frame) return;

	m_draw_calls = 0;
	m_begin_time = 0;
	m_draw_time = 0;
	m_vertex_upload_time = 0;
	m_textures_upload_time = 0;
}


u64 GLGSRender::timestamp() const
{
	GLint64 result;
	glGetInteger64v(GL_TIMESTAMP, &result);
	return result;
}

bool GLGSRender::on_access_violation(u32 address, bool is_writing)
{
	if (is_writing)
		return m_gl_texture_cache.mark_as_dirty(address);
	else
		return m_gl_texture_cache.flush_section(address);
}

void GLGSRender::do_local_task()
{
	std::lock_guard<std::mutex> lock(queue_guard);

	work_queue.remove_if([](work_item &q) { return q.received; });

	for (work_item& q: work_queue)
	{
		if (q.processed) continue;

		std::unique_lock<std::mutex> lock(q.guard_mutex);

		//Check if the suggested section is valid
		if (!q.section_to_flush->is_flushed())
		{
			q.section_to_flush->flush();
			q.result = true;
		}
		else
		{
			//Another thread has unlocked this memory region already
			//Return success
			q.result = true;
		}

		q.processed = true;

		//Notify thread waiting on this
		lock.unlock();
		q.cv.notify_one();
	}
}

work_item& GLGSRender::post_flush_request(u32 address, gl::texture_cache::cached_texture_section *section)
{
	std::lock_guard<std::mutex> lock(queue_guard);

	work_queue.emplace_back();
	work_item &result = work_queue.back();
	result.address_to_flush = address;
	result.section_to_flush = section;
	return result;
}

void GLGSRender::synchronize_buffers()
{
	if (flush_draw_buffers)
	{
		write_buffers();
		flush_draw_buffers = false;
	}
}

bool GLGSRender::scaled_image_from_memory(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate)
{
	return m_gl_texture_cache.upload_scaled_image(src, dst, interpolate, m_rtts);
}
