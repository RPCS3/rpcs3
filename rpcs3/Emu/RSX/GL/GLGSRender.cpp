#include "stdafx.h"
#include "Utilities/Config.h"
#include "Emu/Memory/Memory.h"
#include "GLGSRender.h"
#include "GLVertexProgram.h"
#include "../rsx_methods.h"
#include "../Common/BufferUtils.h"
#include "../rsx_utils.h"

extern cfg::bool_entry g_cfg_rsx_debug_output;
extern cfg::bool_entry g_cfg_rsx_overlay;
extern cfg::bool_entry g_cfg_rsx_gl_legacy_buffers;

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

GLGSRender::GLGSRender() : GSRender(frame_type::OpenGL)
{
	shaders_cache.load(rsx::old_shaders_cache::shader_language::glsl);
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

	init_buffers();

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
		__glcheck glDepthMask(rsx::method_registers.depth_write_enabled());
	}

	if (glDepthBoundsEXT && (__glcheck enable(rsx::method_registers.depth_bounds_test_enabled(), GL_DEPTH_BOUNDS_TEST_EXT)))
	{
		__glcheck glDepthBoundsEXT(rsx::method_registers.depth_bounds_min(), rsx::method_registers.depth_bounds_max());
	}

	__glcheck glDepthRange(rsx::method_registers.clip_min(), rsx::method_registers.clip_max());
	__glcheck enable(rsx::method_registers.dither_enabled(), GL_DITHER);

	if (__glcheck enable(rsx::method_registers.blend_enabled(), GL_BLEND))
	{
		__glcheck glBlendFuncSeparate(blend_factor(rsx::method_registers.blend_func_sfactor_rgb()),
			blend_factor(rsx::method_registers.blend_func_dfactor_rgb()),
			blend_factor(rsx::method_registers.blend_func_sfactor_a()),
			blend_factor(rsx::method_registers.blend_func_dfactor_a()));

		if (rsx::method_registers.surface_color() == rsx::surface_color_format::w16z16y16x16) //TODO: check another color formats
		{
			u16 blend_color_r = rsx::method_registers.blend_color_16b_r();
			u16 blend_color_g = rsx::method_registers.blend_color_16b_g();
			u16 blend_color_b = rsx::method_registers.blend_color_16b_b();
			u16 blend_color_a = rsx::method_registers.blend_color_16b_a();

			__glcheck glBlendColor(blend_color_r / 65535.f, blend_color_g / 65535.f, blend_color_b / 65535.f, blend_color_a / 65535.f);
		}
		else
		{
			u8 blend_color_r = rsx::method_registers.blend_color_8b_r();
			u8 blend_color_g = rsx::method_registers.blend_color_8b_g();
			u8 blend_color_b = rsx::method_registers.blend_color_8b_b();
			u8 blend_color_a = rsx::method_registers.blend_color_8b_a();

			__glcheck glBlendColor(blend_color_r / 255.f, blend_color_g / 255.f, blend_color_b / 255.f, blend_color_a / 255.f);
		}

		__glcheck glBlendEquationSeparate(blend_equation(rsx::method_registers.blend_equation_rgb()),
			blend_equation(rsx::method_registers.blend_equation_a()));
	}

	if (__glcheck enable(rsx::method_registers.stencil_test_enabled(), GL_STENCIL_TEST))
	{
		__glcheck glStencilFunc(comparison_op(rsx::method_registers.stencil_func()), rsx::method_registers.stencil_func_ref(),
			rsx::method_registers.stencil_func_mask());
		__glcheck glStencilOp(stencil_op(rsx::method_registers.stencil_op_fail()), stencil_op(rsx::method_registers.stencil_op_zfail()),
			stencil_op(rsx::method_registers.stencil_op_zpass()));

		if (rsx::method_registers.two_sided_stencil_test_enabled()) {
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

	auto set_clip_plane_control = [&](int index, rsx::user_clip_plane_op control)
	{
		int value = 0;
		int location;

		if (m_program->uniforms.has_location("uc_m" + std::to_string(index), &location))
		{
			switch (control)
			{
			default:
				LOG_ERROR(RSX, "bad clip plane control (0x%x)", (u8)control);

			case rsx::user_clip_plane_op::disable:
				value = 0;
				break;

			case rsx::user_clip_plane_op::greater_or_equal:
				value = 1;
				break;

			case rsx::user_clip_plane_op::less_than:
				value = -1;
				break;
			}

			__glcheck m_program->uniforms[location] = value;
		}

		__glcheck enable(value, GL_CLIP_DISTANCE0 + index);
	};

	load_program();
	set_clip_plane_control(0, rsx::method_registers.clip_plane_0_enabled());
	set_clip_plane_control(1, rsx::method_registers.clip_plane_1_enabled());
	set_clip_plane_control(2, rsx::method_registers.clip_plane_2_enabled());
	set_clip_plane_control(3, rsx::method_registers.clip_plane_3_enabled());
	set_clip_plane_control(4, rsx::method_registers.clip_plane_4_enabled());
	set_clip_plane_control(5, rsx::method_registers.clip_plane_5_enabled());

	if (__glcheck enable(rsx::method_registers.cull_face_enabled(), GL_CULL_FACE))
	{
		__glcheck glCullFace(cull_face(rsx::method_registers.cull_face_mode()));
	}

	__glcheck glFrontFace(front_face(rsx::method_registers.front_face_mode()));

	__glcheck enable(rsx::method_registers.poly_smooth_enabled(), GL_POLYGON_SMOOTH);

	//NV4097_SET_COLOR_KEY_COLOR
	//NV4097_SET_SHADER_CONTROL
	//NV4097_SET_ZMIN_MAX_CONTROL
	//NV4097_SET_ANTI_ALIASING_CONTROL
	//NV4097_SET_CLIP_ID_TEST_ENABLE

	std::chrono::time_point<steady_clock> now = steady_clock::now();
	m_begin_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(now - then).count();
	m_draw_calls++;
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
	if (!draw_fbo)
	{
		rsx::thread::end();
		return;
	}

	if (manually_flush_ring_buffers)
	{
		//Use approximations to reseve space. This path is mostly for debug purposes anyway
		u32 approx_vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
		u32 approx_working_buffer_size = approx_vertex_count * 256;

		//Allocate 256K heap if we have no approximation at this time (inlined array)
		m_attrib_ring_buffer->reserve_storage_on_heap(std::max(approx_working_buffer_size, 256 * 1024U));
		m_index_ring_buffer->reserve_storage_on_heap(16 * 1024);
	}

	draw_fbo.bind();

	//Check if depth buffer is bound and valid
	//If ds is not initialized clear it; it seems new depth textures should have depth cleared
	gl::render_target *ds = std::get<1>(m_rtts.m_bound_depth_stencil);
	if (ds && !ds->cleared())
	{
		glDepthMask(GL_TRUE);
		glClearDepth(1.f);

		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthMask(rsx::method_registers.depth_write_enabled());

		ds->set_cleared();
	}

	std::chrono::time_point<steady_clock> textures_start = steady_clock::now();

	//Setup textures
	for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
	{
		int location;
		if (!rsx::method_registers.fragment_textures[i].enabled())
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
			continue;
		}

		if (m_program->uniforms.has_location("tex" + std::to_string(i), &location))
		{
			m_gl_textures[i].set_target(get_gl_target_for_texture(rsx::method_registers.fragment_textures[i]));
			__glcheck m_gl_texture_cache.upload_texture(i, rsx::method_registers.fragment_textures[i], m_gl_textures[i], m_rtts);
		}
	}

	//Vertex textures
	for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
	{
		int texture_index = i + rsx::limits::fragment_textures_count;
		int location;

		if (!rsx::method_registers.vertex_textures[i].enabled())
		{
			glActiveTexture(GL_TEXTURE0 + texture_index);
			glBindTexture(GL_TEXTURE_2D, 0);
			continue;
		}

		if (m_program->uniforms.has_location("vtex" + std::to_string(i), &location))
		{
			m_gl_vertex_textures[i].set_target(get_gl_target_for_texture(rsx::method_registers.vertex_textures[i]));
			__glcheck m_gl_texture_cache.upload_texture(texture_index, rsx::method_registers.vertex_textures[i], m_gl_vertex_textures[i], m_rtts);
		}
	}

	std::chrono::time_point<steady_clock> textures_end = steady_clock::now();
	m_textures_upload_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(textures_end - textures_start).count();

	u32 vertex_draw_count;
	std::optional<std::tuple<GLenum, u32> > indexed_draw_info;
	std::tie(vertex_draw_count, indexed_draw_info) = set_vertex_buffer();
	m_vao.bind();

	std::chrono::time_point<steady_clock> draw_start = steady_clock::now();

	if (g_cfg_rsx_debug_output)
	{
		m_program->validate();
	}

	if (manually_flush_ring_buffers)
	{
		m_attrib_ring_buffer->unmap();
		m_index_ring_buffer->unmap();
	}

	if (indexed_draw_info)
	{
		if (__glcheck enable(rsx::method_registers.restart_index_enabled(), GL_PRIMITIVE_RESTART))
		{
			GLenum index_type = std::get<0>(indexed_draw_info.value());
			__glcheck glPrimitiveRestartIndex((index_type == GL_UNSIGNED_SHORT)? 0xffff: 0xffffffff);
		}

		__glcheck glDrawElements(gl::draw_mode(rsx::method_registers.current_draw_clause.primitive), vertex_draw_count, std::get<0>(indexed_draw_info.value()), (GLvoid *)(std::ptrdiff_t)std::get<1>(indexed_draw_info.value()));
	}
	else
	{
		draw_fbo.draw_arrays(rsx::method_registers.current_draw_clause.primitive, vertex_draw_count);
	}

	std::chrono::time_point<steady_clock> draw_end = steady_clock::now();
	m_draw_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(draw_end - draw_start).count();

	write_buffers();

	rsx::thread::end();
}

void GLGSRender::set_viewport()
{
	//NOTE: scale offset matrix already contains the viewport transformation
	glViewport(0, 0, rsx::method_registers.surface_clip_width(), rsx::method_registers.surface_clip_height());

	u16 scissor_x = rsx::method_registers.scissor_origin_x();
	u16 scissor_w = rsx::method_registers.scissor_width();
	u16 scissor_y = rsx::method_registers.scissor_origin_y();
	u16 scissor_h = rsx::method_registers.scissor_height();

	//NOTE: window origin does not affect scissor region (probably only affects viewport matrix; already applied)
	//See LIMBO [NPUB-30373] which uses shader window origin = top
	__glcheck glScissor(scissor_x, scissor_y, scissor_w, scissor_h);
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

	const u32 texture_index_offset =
		rsx::limits::fragment_textures_count + rsx::limits::vertex_textures_count;
	for (int index = 0; index < rsx::limits::vertex_count; ++index)
	{
		auto &tex = m_gl_attrib_buffers[index];
		tex.create();
		tex.set_target(gl::texture::target::textureBuffer);

		glActiveTexture(GL_TEXTURE0 + texture_index_offset + index);
		tex.bind();
	}

	if (g_cfg_rsx_gl_legacy_buffers)
	{
		LOG_WARNING(RSX, "Using legacy openGL buffers.");
		manually_flush_ring_buffers = true;

		m_attrib_ring_buffer.reset(new gl::legacy_ring_buffer());
		m_uniform_ring_buffer.reset(new gl::legacy_ring_buffer());
		m_index_ring_buffer.reset(new gl::legacy_ring_buffer());
	}
	else
	{
		m_attrib_ring_buffer.reset(new gl::ring_buffer());
		m_uniform_ring_buffer.reset(new gl::ring_buffer());
		m_index_ring_buffer.reset(new gl::ring_buffer());
	}

	m_attrib_ring_buffer->create(gl::buffer::target::texture, 256 * 0x100000);
	m_uniform_ring_buffer->create(gl::buffer::target::uniform, 64 * 0x100000);
	m_index_ring_buffer->create(gl::buffer::target::element_array, 16 * 0x100000);

	m_vao.element_array_buffer = *m_index_ring_buffer;
	m_gl_texture_cache.initialize_rtt_cache();

	if (g_cfg_rsx_overlay)
		m_text_printer.init();
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

	m_attrib_ring_buffer->remove();
	m_uniform_ring_buffer->remove();
	m_index_ring_buffer->remove();

	m_text_printer.close();

	return GSRender::on_exit();
}

void nv4097_clear_surface(u32 arg, GLGSRender* renderer)
{
	if (rsx::method_registers.surface_color_target() == rsx::surface_target::none) return;

	if ((arg & 0xf3) == 0)
	{
		//do nothing
		return;
	}

	renderer->init_buffers(true);
	renderer->draw_fbo.bind();

	GLbitfield mask = 0;

	rsx::surface_depth_format surface_depth_format = rsx::method_registers.surface_depth_fmt();

	if (arg & 0x1)
	{
		u32 max_depth_value = get_max_depth_value(surface_depth_format);

		u32 clear_depth = rsx::method_registers.z_clear_value();

		glDepthMask(GL_TRUE);
		glClearDepth(double(clear_depth) / max_depth_value);
		mask |= GLenum(gl::buffers::depth);
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

	switch (cmd)
	{
	case NV4097_CLEAR_SURFACE:
	{
		if (arg & 0x1)
		{
			gl::render_target *ds = std::get<1>(m_rtts.m_bound_depth_stencil);
			if (ds && !ds->cleared())
				ds->set_cleared();
		}
	}
	}

	return true;
}

bool GLGSRender::load_program()
{
	RSXVertexProgram vertex_program = get_current_vertex_program();
	RSXFragmentProgram fragment_program = get_current_fragment_program();

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

	for (int i = 0; i < 16; ++i)
	{
		auto &tex = rsx::method_registers.fragment_textures[i];
		if (tex.enabled())
		{
			const u32 texaddr = rsx::get_address(tex.offset(), tex.location());
			if (m_rtts.get_texture_from_depth_stencil_if_applicable(texaddr))
			{
				//Ignore this rtt since we have an aloasing color texture that will be used
				if (m_rtts.get_texture_from_render_target_if_applicable(texaddr))
					continue;

				u32 format = tex.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
				if (format == CELL_GCM_TEXTURE_A8R8G8B8 || format == CELL_GCM_TEXTURE_D8R8G8B8)
				{
					fragment_program.redirected_textures |= (1 << i);
				}
			}
		}
	}

	auto old_program = m_program;
	m_program = &m_prog_buffer.getGraphicPipelineState(vertex_program, fragment_program, nullptr);
	m_program->use();

	if (old_program == m_program && !m_transform_constants_dirty)
	{
		//This path is taken alot so the savings are tangible
		struct scale_offset_layout
		{
			u16 clip_w, clip_h;
			float scale_x, offset_x, scale_y, offset_y, scale_z, offset_z;
			float fog0, fog1;
			u32   alpha_tested;
			float alpha_ref;
		}
		tmp = {};
		
		tmp.clip_w = rsx::method_registers.surface_clip_width();
		tmp.clip_h = rsx::method_registers.surface_clip_height();
		tmp.scale_x = rsx::method_registers.viewport_scale_x();
		tmp.offset_x = rsx::method_registers.viewport_offset_x();
		tmp.scale_y = rsx::method_registers.viewport_scale_y();
		tmp.offset_y = rsx::method_registers.viewport_offset_y();
		tmp.scale_z = rsx::method_registers.viewport_scale_z();
		tmp.offset_z = rsx::method_registers.viewport_offset_z();
		tmp.fog0 = rsx::method_registers.fog_params_0();
		tmp.fog1 = rsx::method_registers.fog_params_1();
		tmp.alpha_tested = rsx::method_registers.alpha_test_enabled();
		tmp.alpha_ref = rsx::method_registers.alpha_ref();

		size_t old_hash = m_transform_buffer_hash;
		m_transform_buffer_hash = 0;

		u8 *data = reinterpret_cast<u8*>(&tmp);
		for (int i = 0; i < sizeof(tmp); ++i)
			m_transform_buffer_hash ^= std::hash<char>()(data[i]);

		if (old_hash == m_transform_buffer_hash)
			return true;
	}

	m_transform_constants_dirty = false;

	u32 fragment_constants_size = m_prog_buffer.get_fragment_constants_buffer_size(fragment_program);
	fragment_constants_size = std::max(32U, fragment_constants_size);
	u32 max_buffer_sz = 512 + 8192 + align(fragment_constants_size, m_uniform_buffer_offset_align);

	if (manually_flush_ring_buffers)
		m_uniform_ring_buffer->reserve_storage_on_heap(align(max_buffer_sz, 512));

	u8 *buf;
	u32 scale_offset_offset;
	u32 vertex_constants_offset;
	u32 fragment_constants_offset;

	// Scale offset
	auto mapping = m_uniform_ring_buffer->alloc_from_heap(512, m_uniform_buffer_offset_align);
	buf = static_cast<u8*>(mapping.first);
	scale_offset_offset = mapping.second;
	fill_scale_offset_data(buf, false);

	// Fragment state 
	u32 is_alpha_tested = rsx::method_registers.alpha_test_enabled();
	float alpha_ref = rsx::method_registers.alpha_ref() / 255.f;
	f32 fog0 = rsx::method_registers.fog_params_0();
	f32 fog1 = rsx::method_registers.fog_params_1();
	memcpy(buf + 16 * sizeof(float), &fog0, sizeof(float));
	memcpy(buf + 17 * sizeof(float), &fog1, sizeof(float));
	memcpy(buf + 18 * sizeof(float), &is_alpha_tested, sizeof(u32));
	memcpy(buf + 19 * sizeof(float), &alpha_ref, sizeof(float));

	// Vertex constants
	mapping = m_uniform_ring_buffer->alloc_from_heap(8192, m_uniform_buffer_offset_align);
	buf = static_cast<u8*>(mapping.first);
	vertex_constants_offset = mapping.second;
	fill_vertex_program_constants_data(buf);

	// Fragment constants
	if (fragment_constants_size)
	{
		mapping = m_uniform_ring_buffer->alloc_from_heap(fragment_constants_size, m_uniform_buffer_offset_align);
		buf = static_cast<u8*>(mapping.first);
		fragment_constants_offset = mapping.second;
		m_prog_buffer.fill_fragment_constants_buffer({ reinterpret_cast<float*>(buf), gsl::narrow<int>(fragment_constants_size) }, fragment_program);
	}

	m_uniform_ring_buffer->bind_range(0, scale_offset_offset, 512);
	m_uniform_ring_buffer->bind_range(1, vertex_constants_offset, 8192);
	if (fragment_constants_size)
	{
		m_uniform_ring_buffer->bind_range(2, fragment_constants_offset, fragment_constants_size);
	}

	if (manually_flush_ring_buffers)
		m_uniform_ring_buffer->unmap();

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

	if (g_cfg_rsx_overlay)
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

	m_draw_calls = 0;
	m_begin_time = 0;
	m_draw_time = 0;
	m_vertex_upload_time = 0;
	m_textures_upload_time = 0;

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
