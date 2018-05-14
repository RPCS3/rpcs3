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
	m_shaders_cache.reset(new gl::shader_cache(m_prog_buffer, "opengl", "v1.3"));

	if (g_cfg.video.disable_vertex_cache)
		m_vertex_cache.reset(new gl::null_vertex_cache());
	else
		m_vertex_cache.reset(new gl::weak_vertex_cache());

	supports_multidraw = !g_cfg.video.strict_rendering_mode;
	supports_native_ui = (bool)g_cfg.misc.use_native_interface;
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
		fmt::throw_exception("Unsupported comparison op 0x%X" HERE, (u32)op);;
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
		fmt::throw_exception("Unsupported stencil op 0x%X" HERE, (u32)op);
	}

	GLenum blend_equation(rsx::blend_equation op)
	{
		switch (op)
		{
			// Note : maybe add is signed on gl
		case rsx::blend_equation::add_signed:
			LOG_TRACE(RSX, "blend equation add_signed used. Emulating using FUNC_ADD");
		case rsx::blend_equation::add: return GL_FUNC_ADD;
		case rsx::blend_equation::min: return GL_MIN;
		case rsx::blend_equation::max: return GL_MAX;
		case rsx::blend_equation::substract: return GL_FUNC_SUBTRACT;
		case rsx::blend_equation::reverse_substract_signed:
			LOG_TRACE(RSX, "blend equation reverse_subtract_signed used. Emulating using FUNC_REVERSE_SUBTRACT");
		case rsx::blend_equation::reverse_substract: return GL_FUNC_REVERSE_SUBTRACT;
		case rsx::blend_equation::reverse_add_signed:
		default:
			LOG_ERROR(RSX, "Blend equation 0x%X is unimplemented!", (u32)op);
			return GL_FUNC_ADD;
		}
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
		fmt::throw_exception("Unsupported blend factor 0x%X" HERE, (u32)op);
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
		fmt::throw_exception("Unsupported logic op 0x%X" HERE, (u32)op);
	}

	GLenum front_face(rsx::front_face op)
	{
		//NOTE: RSX face winding is always based off of upper-left corner like vulkan, but GL is bottom left
		//shader_window_origin register does not affect this
		//verified with Outrun Online Arcade (window_origin::top) and DS2 (window_origin::bottom)
		//correctness of face winding checked using stencil test (GOW collection shadows)
		switch (op)
		{
		case rsx::front_face::cw: return GL_CCW;
		case rsx::front_face::ccw: return GL_CW;
		}
		fmt::throw_exception("Unsupported front face 0x%X" HERE, (u32)op);
	}

	GLenum cull_face(rsx::cull_face op)
	{
		switch (op)
		{
		case rsx::cull_face::front: return GL_FRONT;
		case rsx::cull_face::back: return GL_BACK;
		case rsx::cull_face::front_and_back: return GL_FRONT_AND_BACK;
		}
		fmt::throw_exception("Unsupported cull face 0x%X" HERE, (u32)op);
	}
}

void GLGSRender::begin()
{
	rsx::thread::begin();

	if (skip_frame ||
		(conditional_render_enabled && conditional_render_test_failed))
		return;

	init_buffers(rsx::framebuffer_creation_context::context_draw);
}

void GLGSRender::end()
{
	std::chrono::time_point<steady_clock> state_check_start = steady_clock::now();

	if (skip_frame || !framebuffer_status_valid ||
		(conditional_render_enabled && conditional_render_test_failed) ||
		!check_program_state())
	{
		rsx::thread::end();
		return;
	}

	std::chrono::time_point<steady_clock> state_check_end = steady_clock::now();
	m_begin_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(state_check_end - state_check_start).count();

	if (manually_flush_ring_buffers)
	{
		//Use approximations to reseve space. This path is mostly for debug purposes anyway
		u32 approx_vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
		u32 approx_working_buffer_size = approx_vertex_count * 256;

		//Allocate 256K heap if we have no approximation at this time (inlined array)
		m_attrib_ring_buffer->reserve_storage_on_heap(std::max(approx_working_buffer_size, 256 * 1024U));
		m_index_ring_buffer->reserve_storage_on_heap(16 * 1024);
	}

	//Do vertex upload before RTT prep / texture lookups to give the driver time to push data
	auto upload_info = set_vertex_buffer();

	//Check if depth buffer is bound and valid
	//If ds is not initialized clear it; it seems new depth textures should have depth cleared
	auto copy_rtt_contents = [](gl::render_target *surface)
	{
		if (surface->get_internal_format() == surface->old_contents->get_internal_format())
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
		}
		//TODO: download image contents and reupload them or do a memory cast to copy memory contents if not compatible

		surface->old_contents = nullptr;
	};

	//Check if we have any 'recycled' surfaces in memory and if so, clear them
	std::vector<int> buffers_to_clear;
	bool clear_all_color = true;
	bool clear_depth = false;

	for (int index = 0; index < 4; index++)
	{
		if (std::get<0>(m_rtts.m_bound_render_targets[index]) != 0)
		{
			if (std::get<1>(m_rtts.m_bound_render_targets[index])->cleared())
				clear_all_color = false;
			else
				buffers_to_clear.push_back(index);
		}
	}

	gl::render_target *ds = std::get<1>(m_rtts.m_bound_depth_stencil);
	if (ds && !ds->cleared())
	{
		clear_depth = true;
	}

	//Temporarily disable pixel tests
	glDisable(GL_SCISSOR_TEST);

	if (clear_depth || buffers_to_clear.size() > 0)
	{
		GLenum mask = 0;

		if (clear_depth)
		{
			gl_state.depth_mask(GL_TRUE);
			gl_state.clear_depth(1.f);
			gl_state.clear_stencil(255);
			mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
		}

		if (clear_all_color)
			mask |= GL_COLOR_BUFFER_BIT;

		glClear(mask);

		if (buffers_to_clear.size() > 0 && !clear_all_color)
		{
			GLfloat colors[] = { 0.f, 0.f, 0.f, 0.f };
			//It is impossible for the render target to be typa A or B here (clear all would have been flagged)
			for (auto &i : buffers_to_clear)
				glClearBufferfv(draw_fbo.id(), i, colors);
		}

		if (clear_depth)
			gl_state.depth_mask(rsx::method_registers.depth_write_enabled());

		ds->set_cleared();
	}

	if (ds && ds->old_contents != nullptr && ds->get_rsx_pitch() == ds->old_contents->get_rsx_pitch() &&
		ds->old_contents->get_internal_format() == gl::texture::internal_format::rgba8)
	{
		m_depth_converter.run(ds->width(), ds->height(), ds->id(), ds->old_contents->id());
		ds->old_contents = nullptr;
	}

	if (g_cfg.video.strict_rendering_mode)
	{
		if (ds && ds->old_contents != nullptr)
			copy_rtt_contents(ds);

		for (auto &rtt : m_rtts.m_bound_render_targets)
		{
			if (auto surface = std::get<1>(rtt))
			{
				if (surface->old_contents != nullptr)
					copy_rtt_contents(surface);
			}
		}
	}
	else
	{
		// Old contents are one use only. Keep the depth conversion check from firing over and over
		if (ds) ds->old_contents = nullptr;
	}

	glEnable(GL_SCISSOR_TEST);

	//Load textures
	{
		std::chrono::time_point<steady_clock> textures_start = steady_clock::now();

		std::lock_guard<shared_mutex> lock(m_sampler_mutex);
		void* unused = nullptr;
		bool  update_framebuffer_sourced = false;

		if (surface_store_tag != m_rtts.cache_tag)
		{
			update_framebuffer_sourced = true;
			surface_store_tag = m_rtts.cache_tag;
		}

		for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
		{
			if (!fs_sampler_state[i])
				fs_sampler_state[i] = std::make_unique<gl::texture_cache::sampled_image_descriptor>();

			if (m_samplers_dirty || m_textures_dirty[i] ||
				(update_framebuffer_sourced && fs_sampler_state[i]->upload_context == rsx::texture_upload_context::framebuffer_storage))
			{
				auto sampler_state = static_cast<gl::texture_cache::sampled_image_descriptor*>(fs_sampler_state[i].get());

				if (rsx::method_registers.fragment_textures[i].enabled())
				{
					*sampler_state = m_gl_texture_cache.upload_texture(unused, rsx::method_registers.fragment_textures[i], m_rtts);

					if (m_textures_dirty[i])
						m_gl_sampler_states[i].apply(rsx::method_registers.fragment_textures[i], fs_sampler_state[i].get());
				}
				else
				{
					*sampler_state = {};
				}

				m_textures_dirty[i] = false;
			}
		}

		for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
		{
			if (!vs_sampler_state[i])
				vs_sampler_state[i] = std::make_unique<gl::texture_cache::sampled_image_descriptor>();

			if (m_samplers_dirty || m_vertex_textures_dirty[i] ||
				(update_framebuffer_sourced && vs_sampler_state[i]->upload_context == rsx::texture_upload_context::framebuffer_storage))
			{
				auto sampler_state = static_cast<gl::texture_cache::sampled_image_descriptor*>(vs_sampler_state[i].get());

				if (rsx::method_registers.vertex_textures[i].enabled())
				{
					*sampler_state = m_gl_texture_cache.upload_texture(unused, rsx::method_registers.vertex_textures[i], m_rtts);
				}
				else
					*sampler_state = {};

				m_vertex_textures_dirty[i] = false;
			}
		}

		m_samplers_dirty.store(false);

		std::chrono::time_point<steady_clock> textures_end = steady_clock::now();
		m_textures_upload_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(textures_end - textures_start).count();
	}

	std::chrono::time_point<steady_clock> program_start = steady_clock::now();
	//Load program here since it is dependent on vertex state

	load_program(upload_info);

	std::chrono::time_point<steady_clock> program_stop = steady_clock::now();
	m_begin_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(program_stop - program_start).count();

	if (manually_flush_ring_buffers)
	{
		m_attrib_ring_buffer->unmap();
		m_index_ring_buffer->unmap();
	}
	else
	{
		//DMA push; not needed with MAP_COHERENT
		//glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
	}

	//Bind textures and resolve external copy operations
	std::chrono::time_point<steady_clock> textures_start = steady_clock::now();
	int unused_location;
	void *unused = nullptr;
	gl::texture_view* tmp_view;

	for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
	{
		if (m_program->uniforms.has_location(rsx::constants::fragment_texture_names[i], &unused_location))
		{
			auto sampler_state = static_cast<gl::texture_cache::sampled_image_descriptor*>(fs_sampler_state[i].get());
			auto &tex = rsx::method_registers.fragment_textures[i];

			glActiveTexture(GL_TEXTURE0 + i);

			if (tex.enabled())
			{
				if (sampler_state->image_handle)
				{
					sampler_state->image_handle->bind();
				}
				else if (sampler_state->external_subresource_desc.external_handle &&
					(tmp_view = m_gl_texture_cache.create_temporary_subresource(unused, sampler_state->external_subresource_desc)))
				{
					tmp_view->bind();
				}
				else
				{
					auto target = gl::get_target(sampler_state->image_type);
					glBindTexture(target, m_null_textures[target]->id());
				}
			}
			else
			{
				glBindTexture(GL_TEXTURE_1D, m_null_textures[GL_TEXTURE_1D]->id());
				glBindTexture(GL_TEXTURE_2D, m_null_textures[GL_TEXTURE_2D]->id());
				glBindTexture(GL_TEXTURE_3D, m_null_textures[GL_TEXTURE_3D]->id());
				glBindTexture(GL_TEXTURE_CUBE_MAP, m_null_textures[GL_TEXTURE_CUBE_MAP]->id());
			}
		}
	}

	for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
	{
		if (m_program->uniforms.has_location(rsx::constants::vertex_texture_names[i], &unused_location))
		{
			auto sampler_state = static_cast<gl::texture_cache::sampled_image_descriptor*>(vs_sampler_state[i].get());
			glActiveTexture(GL_TEXTURE0 + rsx::limits::fragment_textures_count + i);

			if (sampler_state->image_handle)
			{
				sampler_state->image_handle->bind();
			}
			else if (sampler_state->external_subresource_desc.external_handle)
			{
				void *unused = nullptr;
				m_gl_texture_cache.create_temporary_subresource(unused, sampler_state->external_subresource_desc)->bind();
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, GL_NONE);
			}
		}
	}

	std::chrono::time_point<steady_clock> textures_end = steady_clock::now();
	m_textures_upload_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(textures_end - textures_start).count();

	update_draw_state();

	std::chrono::time_point<steady_clock> draw_start = steady_clock::now();

	if (g_cfg.video.debug_output)
	{
		m_program->validate();
	}

	const GLenum draw_mode = gl::draw_mode(rsx::method_registers.current_draw_clause.primitive);
	bool single_draw = !supports_multidraw || (rsx::method_registers.current_draw_clause.first_count_commands.size() <= 1 || rsx::method_registers.current_draw_clause.is_disjoint_primitive);

	if (upload_info.index_info)
	{
		const GLenum index_type = std::get<0>(upload_info.index_info.value());
		const u32 index_offset = std::get<1>(upload_info.index_info.value());
		const bool restarts_valid = gl::is_primitive_native(rsx::method_registers.current_draw_clause.primitive) && !rsx::method_registers.current_draw_clause.is_disjoint_primitive;

		if (gl_state.enable(restarts_valid && rsx::method_registers.restart_index_enabled(), GL_PRIMITIVE_RESTART))
		{
			glPrimitiveRestartIndex((index_type == GL_UNSIGNED_SHORT)? 0xffff: 0xffffffff);
		}

		m_index_ring_buffer->bind();

		if (single_draw)
		{
			glDrawElements(draw_mode, upload_info.vertex_draw_count, index_type, (GLvoid *)(uintptr_t)index_offset);
		}
		else
		{
			const auto draw_count = rsx::method_registers.current_draw_clause.first_count_commands.size();
			const u32 type_scale = (index_type == GL_UNSIGNED_SHORT) ? 1 : 2;
			uintptr_t index_ptr = index_offset;
			m_scratch_buffer.resize(draw_count * 16);

			GLsizei *counts = (GLsizei*)m_scratch_buffer.data();
			const GLvoid** offsets = (const GLvoid**)(counts + draw_count);
			int dst_index = 0;

			for (const auto &range : rsx::method_registers.current_draw_clause.first_count_commands)
			{
				const auto index_size = get_index_count(rsx::method_registers.current_draw_clause.primitive, range.second);
				counts[dst_index] = index_size;
				offsets[dst_index++] = (const GLvoid*)index_ptr;

				index_ptr += (index_size << type_scale);
			}

			glMultiDrawElements(draw_mode, counts, index_type, offsets, (GLsizei)draw_count);
		}
	}
	else
	{
		if (single_draw)
		{
			glDrawArrays(draw_mode, 0, upload_info.vertex_draw_count);
		}
		else
		{
			const u32 base_index = rsx::method_registers.current_draw_clause.first_count_commands.front().first;
			bool use_draw_arrays_fallback = false;

			const auto draw_count = rsx::method_registers.current_draw_clause.first_count_commands.size();
			const auto driver_caps = gl::get_driver_caps();

			m_scratch_buffer.resize(draw_count * 24);
			GLint* firsts = (GLint*)m_scratch_buffer.data();
			GLsizei* counts = (GLsizei*)(firsts + draw_count);
			const GLvoid** offsets = (const GLvoid**)(counts + draw_count);
			int dst_index = 0;

			for (const auto &range : rsx::method_registers.current_draw_clause.first_count_commands)
			{
				const GLint first = range.first - base_index;
				const GLsizei count = range.second;

				firsts[dst_index] = first;
				counts[dst_index] = count;
				offsets[dst_index++] = (const GLvoid*)(first << 2);

				if (driver_caps.vendor_AMD && (first + count) > (0x100000 >> 2))
				{
					//Unlikely, but added here in case the identity buffer is not large enough somehow
					use_draw_arrays_fallback = true;
					break;
				}
			}

			if (use_draw_arrays_fallback)
			{
				//MultiDrawArrays is broken on some primitive types using AMD. One known type is GL_TRIANGLE_STRIP but there could be more
				for (const auto &range : rsx::method_registers.current_draw_clause.first_count_commands)
				{
					glDrawArrays(draw_mode, range.first - base_index, range.second);
				}
			}
			else if (driver_caps.vendor_AMD)
			{
				//Use identity index buffer to fix broken vertexID on AMD
				m_identity_index_buffer->bind();
				glMultiDrawElements(draw_mode, counts, GL_UNSIGNED_INT, offsets, (GLsizei)draw_count);
			}
			else
			{
				//Normal render
				glMultiDrawArrays(draw_mode, firsts, counts, (GLsizei)draw_count);
			}
		}
	}

	m_attrib_ring_buffer->notify();
	m_index_ring_buffer->notify();
	m_vertex_state_buffer->notify();
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
	const auto clip_width = rsx::apply_resolution_scale(rsx::method_registers.surface_clip_width(), true);
	const auto clip_height = rsx::apply_resolution_scale(rsx::method_registers.surface_clip_height(), true);
	glViewport(0, 0, clip_width, clip_height);

	u16 scissor_x = rsx::apply_resolution_scale(rsx::method_registers.scissor_origin_x(), false);
	u16 scissor_w = rsx::apply_resolution_scale(rsx::method_registers.scissor_width(), true);
	u16 scissor_y = rsx::apply_resolution_scale(rsx::method_registers.scissor_origin_y(), false);
	u16 scissor_h = rsx::apply_resolution_scale(rsx::method_registers.scissor_height(), true);

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
	glScissor(scissor_x, scissor_y, scissor_w, scissor_h);
	glEnable(GL_SCISSOR_TEST);
}

void GLGSRender::on_init_thread()
{
	GSRender::on_init_thread();
	zcull_ctrl.reset(static_cast<::rsx::reports::ZCULL_control*>(this));

	gl::init();

	//Enable adaptive vsync if vsync is requested
	gl::set_swapinterval(g_cfg.video.vsync ? -1 : 0);

	if (g_cfg.video.debug_output)
		gl::enable_debugging();

	LOG_NOTICE(RSX, "GL RENDERER: %s (%s)", (const char*)glGetString(GL_RENDERER), (const char*)glGetString(GL_VENDOR));
	LOG_NOTICE(RSX, "GL VERSION: %s", (const char*)glGetString(GL_VERSION));
	LOG_NOTICE(RSX, "GLSL VERSION: %s", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

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
	m_max_texbuffer_size = 0;

	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &m_uniform_buffer_offset_align);
	glGetIntegerv(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, &m_min_texbuffer_alignment);
	glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &m_max_texbuffer_size);
	m_vao.create();

	//Set min alignment to 16-bytes for SSE optimizations with aligned addresses to work
	m_min_texbuffer_alignment = std::max(m_min_texbuffer_alignment, 16);
	m_uniform_buffer_offset_align = std::max(m_uniform_buffer_offset_align, 16);

	LOG_NOTICE(RSX, "Supported texel buffer size reported: %d bytes", m_max_texbuffer_size);
	if (m_max_texbuffer_size < (16 * 0x100000))
	{
		LOG_ERROR(RSX, "Max texture buffer size supported is less than 16M which is useless. Expect undefined behaviour.");
		m_max_texbuffer_size = (16 * 0x100000);
	}

	const u32 texture_index_offset = rsx::limits::fragment_textures_count + rsx::limits::vertex_textures_count;

	//Array stream buffer
	{
		m_gl_persistent_stream_buffer = std::make_unique<gl::texture>(GL_TEXTURE_BUFFER, 0, 0, 0, 0, GL_R8UI);
		glActiveTexture(GL_TEXTURE0 + texture_index_offset);
		glBindTexture(GL_TEXTURE_BUFFER, m_gl_persistent_stream_buffer->id());
	}

	//Register stream buffer
	{
		m_gl_volatile_stream_buffer = std::make_unique<gl::texture>(GL_TEXTURE_BUFFER, 0, 0, 0, 0, GL_R8UI);
		glActiveTexture(GL_TEXTURE0 + texture_index_offset + 1);
		glBindTexture(GL_TEXTURE_BUFFER, m_gl_volatile_stream_buffer->id());
	}

	//Fallback null texture instead of relying on texture0
	{
		std::vector<u32> pixeldata = {0, 0, 0, 0};

		//1D
		auto tex1D = std::make_unique<gl::texture>(GL_TEXTURE_1D, 1, 1, 1, 1, GL_RGBA8);
		tex1D->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8);

		//2D
		auto tex2D = std::make_unique<gl::texture>(GL_TEXTURE_2D, 1, 1, 1, 1, GL_RGBA8);
		tex2D->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8);

		//3D
		auto tex3D = std::make_unique<gl::texture>(GL_TEXTURE_3D, 1, 1, 1, 1, GL_RGBA8);
		tex3D->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8);

		//CUBE
		auto texCUBE = std::make_unique<gl::texture>(GL_TEXTURE_CUBE_MAP, 1, 1, 1, 1, GL_RGBA8);
		texCUBE->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8);

		m_null_textures[GL_TEXTURE_1D] = std::move(tex1D);
		m_null_textures[GL_TEXTURE_2D] = std::move(tex2D);
		m_null_textures[GL_TEXTURE_3D] = std::move(tex3D);
		m_null_textures[GL_TEXTURE_CUBE_MAP] = std::move(texCUBE);
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
		m_vertex_state_buffer.reset(new gl::legacy_ring_buffer());
		m_index_ring_buffer.reset(new gl::legacy_ring_buffer());
	}
	else
	{
		m_attrib_ring_buffer.reset(new gl::ring_buffer());
		m_transform_constants_buffer.reset(new gl::ring_buffer());
		m_fragment_constants_buffer.reset(new gl::ring_buffer());
		m_vertex_state_buffer.reset(new gl::ring_buffer());
		m_index_ring_buffer.reset(new gl::ring_buffer());
	}

	m_attrib_ring_buffer->create(gl::buffer::target::texture, 256 * 0x100000);
	m_index_ring_buffer->create(gl::buffer::target::element_array, 64 * 0x100000);
	m_transform_constants_buffer->create(gl::buffer::target::uniform, 64 * 0x100000);
	m_fragment_constants_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_vertex_state_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);

	if (gl_caps.vendor_AMD)
	{
		m_identity_index_buffer.reset(new gl::buffer);
		m_identity_index_buffer->create(gl::buffer::target::element_array, 1 * 0x100000);

		// Initialize with 256k identity entries
		auto *dst = (u32*)m_identity_index_buffer->map(gl::buffer::access::write);
		for (u32 n = 0; n < (0x100000 >> 2); ++n)
		{
			dst[n] = n;
		}

		m_identity_index_buffer->unmap();
	}

	m_persistent_stream_view.update(m_attrib_ring_buffer.get(), 0, std::min<u32>((u32)m_attrib_ring_buffer->size(), m_max_texbuffer_size));
	m_volatile_stream_view.update(m_attrib_ring_buffer.get(), 0, std::min<u32>((u32)m_attrib_ring_buffer->size(), m_max_texbuffer_size));
	m_gl_persistent_stream_buffer->copy_from(m_persistent_stream_view);
	m_gl_volatile_stream_buffer->copy_from(m_volatile_stream_view);

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

	//Occlusion query
	for (u32 i = 0; i < occlusion_query_count; ++i)
	{
		GLuint handle = 0;
		auto &query = m_occlusion_query_data[i];
		glGenQueries(1, &handle);
		
		query.driver_handle = (u64)handle;
		query.pending = false;
		query.active = false;
		query.result = 0;
	}

	//Clip planes are shader controlled; enable all planes driver-side
	glEnable(GL_CLIP_DISTANCE0 + 0);
	glEnable(GL_CLIP_DISTANCE0 + 1);
	glEnable(GL_CLIP_DISTANCE0 + 2);
	glEnable(GL_CLIP_DISTANCE0 + 3);
	glEnable(GL_CLIP_DISTANCE0 + 4);
	glEnable(GL_CLIP_DISTANCE0 + 5);

	m_depth_converter.create();
	m_ui_renderer.create();
	m_video_output_pass.create();

	m_gl_texture_cache.initialize();
	m_thread_id = std::this_thread::get_id();

	if (!supports_native_ui)
	{
		m_frame->disable_wm_event_queue();
		m_frame->hide();

		m_shaders_cache->load(nullptr);

		m_frame->enable_wm_event_queue();
		m_frame->show();
	}
	else
	{
		struct native_helper : gl::shader_cache::progress_dialog_helper
		{
			rsx::thread *owner = nullptr;
			rsx::overlays::message_dialog *dlg = nullptr;

			native_helper(GLGSRender *ptr) :
				owner(ptr) {}

			void create() override
			{
				MsgDialogType type = {};
				type.disable_cancel = true;
				type.progress_bar_count = 1;

				dlg = owner->shell_open_message_dialog();
				dlg->show("Loading precompiled shaders from disk...", type, [](s32 status)
				{
					if (status != CELL_OK)
						Emu.Stop();
				});
			}

			void update_msg(u32 processed, u32 entry_count) override
			{
				dlg->progress_bar_set_message(0, fmt::format("Loading pipeline object %u of %u", processed, entry_count));
				owner->flip(0);
			}

			void inc_value(u32 value) override
			{
				dlg->progress_bar_increment(0, (f32)value);
				owner->flip(0);
			}

			void close() override
			{
				dlg->return_code = CELL_OK;
				dlg->close();
			}
		}
		helper(this);

		m_frame->enable_wm_event_queue();
		m_shaders_cache->load(&helper);
	}
}


void GLGSRender::on_exit()
{
	zcull_ctrl.release();

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
		m_flip_tex_color.reset();
	}

	if (m_vao)
	{
		m_vao.remove();
	}

	m_gl_persistent_stream_buffer.reset();
	m_gl_volatile_stream_buffer.reset();

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

	if (m_vertex_state_buffer)
	{
		m_vertex_state_buffer->remove();
	}

	if (m_index_ring_buffer)
	{
		m_index_ring_buffer->remove();
	}

	if (m_identity_index_buffer)
	{
		m_identity_index_buffer->remove();
	}

	m_null_textures.clear();
	m_text_printer.close();
	m_gl_texture_cache.destroy();
	m_depth_converter.destroy();
	m_ui_renderer.destroy();
	m_video_output_pass.destroy();

	for (u32 i = 0; i < occlusion_query_count; ++i)
	{
		auto &query = m_occlusion_query_data[i];
		query.active = false;
		query.pending = false;

		GLuint handle = (GLuint)query.driver_handle;
		glDeleteQueries(1, &handle);
		query.driver_handle = 0;
	}

	glFlush();
	glFinish();

	GSRender::on_exit();
}

void GLGSRender::clear_surface(u32 arg)
{
	if (skip_frame || !framebuffer_status_valid) return;
	if ((arg & 0xf3) == 0) return;

	GLbitfield mask = 0;

	rsx::surface_depth_format surface_depth_format = rsx::method_registers.surface_depth_fmt();

	if (arg & 0x1)
	{
		u32 max_depth_value = get_max_depth_value(surface_depth_format);
		u32 clear_depth = rsx::method_registers.z_clear_value(surface_depth_format == rsx::surface_depth_format::z24s8);

		gl_state.depth_mask(GL_TRUE);
		gl_state.clear_depth(f32(clear_depth) / max_depth_value);
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

		gl_state.stencil_mask(rsx::method_registers.stencil_mask());
		gl_state.clear_stencil(clear_stencil);

		mask |= GLenum(gl::buffers::stencil);
	}

	if (auto colormask = (arg & 0xf0))
	{
		switch (rsx::method_registers.surface_color())
		{
		case rsx::surface_color_format::x32:
		case rsx::surface_color_format::w16z16y16x16:
		case rsx::surface_color_format::w32z32y32x32:
		{
			//Nop
			break;
		}
		case rsx::surface_color_format::g8b8:
		{
			colormask = rsx::get_g8b8_r8g8_colormask(colormask);
			// Fall through
		}
		default:
		{
			u8 clear_a = rsx::method_registers.clear_color_a();
			u8 clear_r = rsx::method_registers.clear_color_r();
			u8 clear_g = rsx::method_registers.clear_color_g();
			u8 clear_b = rsx::method_registers.clear_color_b();

			gl_state.color_mask(colormask);
			gl_state.clear_color(clear_r, clear_g, clear_b, clear_a);

			mask |= GLenum(gl::buffers::color);

			for (auto &rtt : m_rtts.m_bound_render_targets)
			{
				if (std::get<0>(rtt) != 0)
				{
					std::get<1>(rtt)->set_cleared(true);
					std::get<1>(rtt)->old_contents = nullptr;
				}
			}

			break;
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
		if (arg & 0xF3)
		{
			//Only do all this if we have actual work to do
			u8 ctx = rsx::framebuffer_creation_context::context_draw;
			if (arg & 0xF0) ctx |= rsx::framebuffer_creation_context::context_clear_color;
			if (arg & 0x3) ctx |= rsx::framebuffer_creation_context::context_clear_depth;

			init_buffers((rsx::framebuffer_creation_context)ctx, true);
			synchronize_buffers();
			clear_surface(arg);
		}

		return true;
	}
	case NV4097_CLEAR_ZCULL_SURFACE:
	{
		// NOP
		// Clearing zcull memory does not modify depth/stencil buffers 'bound' to the zcull region
		return true;
	}
	case NV4097_TEXTURE_READ_SEMAPHORE_RELEASE:
	case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE:
		flush_draw_buffers = true;
		return true;
	}

	return false;
}

bool GLGSRender::check_program_state()
{
	return (rsx::method_registers.shader_program_address() != 0);
}

void GLGSRender::load_program(const gl::vertex_upload_info& upload_info)
{
	if (m_graphics_state & rsx::pipeline_state::invalidate_pipeline_bits)
	{
		get_current_fragment_program(fs_sampler_state);
		verify(HERE), current_fragment_program.valid;

		get_current_vertex_program();

		current_vertex_program.skip_vertex_input_check = true;	//not needed for us since decoding is done server side
		current_fragment_program.unnormalized_coords = 0; //unused
		void* pipeline_properties = nullptr;

		m_program = &m_prog_buffer.getGraphicPipelineState(current_vertex_program, current_fragment_program, pipeline_properties);
		m_program->use();

		if (m_prog_buffer.check_cache_missed())
		{
			m_shaders_cache->store(pipeline_properties, current_vertex_program, current_fragment_program);

			//Notify the user with HUD notification
			if (g_cfg.misc.show_shader_compilation_hint)
			{
				if (!m_custom_ui)
				{
					//Create notification but do not draw it at this time. No need to spam flip requests
					m_custom_ui = std::make_unique<rsx::overlays::shader_compile_notification>();
				}
				else if (auto casted = dynamic_cast<rsx::overlays::shader_compile_notification*>(m_custom_ui.get()))
				{
					//Probe the notification
					casted->touch();
				}
			}
		}
	}

	u8 *buf;
	u32 vertex_state_offset;
	u32 vertex_constants_offset;
	u32 fragment_constants_offset;

	const u32 fragment_constants_size = (const u32)m_prog_buffer.get_fragment_constants_buffer_size(current_fragment_program);
	const u32 fragment_buffer_size = fragment_constants_size + (18 * 4 * sizeof(float));
	const bool update_transform_constants = !!(m_graphics_state & rsx::pipeline_state::transform_constants_dirty);

	if (manually_flush_ring_buffers)
	{
		m_vertex_state_buffer->reserve_storage_on_heap(512);
		m_fragment_constants_buffer->reserve_storage_on_heap(align(fragment_buffer_size, 256));
		if (update_transform_constants) m_transform_constants_buffer->reserve_storage_on_heap(8192);
	}

	// Vertex state
	auto mapping = m_vertex_state_buffer->alloc_from_heap(512, m_uniform_buffer_offset_align);
	buf = static_cast<u8*>(mapping.first);
	vertex_state_offset = mapping.second;
	fill_scale_offset_data(buf, false);
	fill_user_clip_data(buf + 64);
	*(reinterpret_cast<u32*>(buf + 128)) = rsx::method_registers.transform_branch_bits();
	*(reinterpret_cast<u32*>(buf + 132)) = upload_info.vertex_index_base;
	*(reinterpret_cast<f32*>(buf + 136)) = rsx::method_registers.point_size();
	*(reinterpret_cast<f32*>(buf + 140)) = rsx::method_registers.clip_min();
	*(reinterpret_cast<f32*>(buf + 144)) = rsx::method_registers.clip_max();
	fill_vertex_layout_state(m_vertex_layout, upload_info.allocated_vertex_count, reinterpret_cast<s32*>(buf + 160), upload_info.persistent_mapping_offset, upload_info.volatile_mapping_offset);

	if (update_transform_constants)
	{
		// Vertex constants
		mapping = m_transform_constants_buffer->alloc_from_heap(8192, m_uniform_buffer_offset_align);
		buf = static_cast<u8*>(mapping.first);
		vertex_constants_offset = mapping.second;
		fill_vertex_program_constants_data(buf);
	}

	// Fragment constants
	mapping = m_fragment_constants_buffer->alloc_from_heap(fragment_buffer_size, m_uniform_buffer_offset_align);
	buf = static_cast<u8*>(mapping.first);
	fragment_constants_offset = mapping.second;
	if (fragment_constants_size)
	{
		m_prog_buffer.fill_fragment_constants_buffer({ reinterpret_cast<float*>(buf), gsl::narrow<int>(fragment_constants_size) },
			current_fragment_program, gl::get_driver_caps().vendor_NVIDIA);
	}

	// Fragment state
	fill_fragment_state_buffer(buf+fragment_constants_size, current_fragment_program);

	m_vertex_state_buffer->bind_range(0, vertex_state_offset, 512);
	m_fragment_constants_buffer->bind_range(2, fragment_constants_offset, fragment_buffer_size);

	if (update_transform_constants) m_transform_constants_buffer->bind_range(1, vertex_constants_offset, 8192);

	if (manually_flush_ring_buffers)
	{
		m_vertex_state_buffer->unmap();
		m_fragment_constants_buffer->unmap();

		if (update_transform_constants) m_transform_constants_buffer->unmap();
	}

	m_graphics_state = 0;
}

void GLGSRender::update_draw_state()
{
	std::chrono::time_point<steady_clock> then = steady_clock::now();

	bool color_mask_b = rsx::method_registers.color_mask_b();
	bool color_mask_g = rsx::method_registers.color_mask_g();
	bool color_mask_r = rsx::method_registers.color_mask_r();
	bool color_mask_a = rsx::method_registers.color_mask_a();

	if (rsx::method_registers.surface_color() == rsx::surface_color_format::g8b8)
	{
		//Map GB components onto RG
		rsx::get_g8b8_r8g8_colormask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);
	}

	gl_state.color_mask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);
	gl_state.depth_mask(rsx::method_registers.depth_write_enabled());
	gl_state.stencil_mask(rsx::method_registers.stencil_mask());

	gl_state.enable(rsx::method_registers.depth_clamp_enabled() || !rsx::method_registers.depth_clip_enabled(), GL_DEPTH_CLAMP);

	if (gl_state.enable(rsx::method_registers.depth_test_enabled(), GL_DEPTH_TEST))
	{
		gl_state.depth_func(comparison_op(rsx::method_registers.depth_func()));
	}

	if (glDepthBoundsEXT && (gl_state.enable(rsx::method_registers.depth_bounds_test_enabled(), GL_DEPTH_BOUNDS_TEST_EXT)))
	{
		gl_state.depth_bounds(rsx::method_registers.depth_bounds_min(), rsx::method_registers.depth_bounds_max());
	}

	gl_state.enable(rsx::method_registers.dither_enabled(), GL_DITHER);

	if (gl_state.enable(rsx::method_registers.stencil_test_enabled(), GL_STENCIL_TEST))
	{
		glStencilFunc(comparison_op(rsx::method_registers.stencil_func()),
			rsx::method_registers.stencil_func_ref(),
			rsx::method_registers.stencil_func_mask());

		glStencilOp(stencil_op(rsx::method_registers.stencil_op_fail()), stencil_op(rsx::method_registers.stencil_op_zfail()),
			stencil_op(rsx::method_registers.stencil_op_zpass()));

		if (rsx::method_registers.two_sided_stencil_test_enabled())
		{
			glStencilMaskSeparate(GL_BACK, rsx::method_registers.back_stencil_mask());

			glStencilFuncSeparate(GL_BACK, comparison_op(rsx::method_registers.back_stencil_func()),
				rsx::method_registers.back_stencil_func_ref(), rsx::method_registers.back_stencil_func_mask());

			glStencilOpSeparate(GL_BACK, stencil_op(rsx::method_registers.back_stencil_op_fail()),
				stencil_op(rsx::method_registers.back_stencil_op_zfail()), stencil_op(rsx::method_registers.back_stencil_op_zpass()));
		}
	}

	bool mrt_blend_enabled[] =
	{
		rsx::method_registers.blend_enabled(),
		rsx::method_registers.blend_enabled_surface_1(),
		rsx::method_registers.blend_enabled_surface_2(),
		rsx::method_registers.blend_enabled_surface_3()
	};

	bool blend_equation_override = false;
	if (rsx::method_registers.msaa_alpha_to_coverage_enabled() &&
		!rsx::method_registers.alpha_test_enabled())
	{
		if (rsx::method_registers.msaa_enabled() &&
			rsx::method_registers.msaa_sample_mask() &&
			rsx::method_registers.surface_antialias() != rsx::surface_antialiasing::center_1_sample)
		{
			//fake alpha-to-coverage
			//blend used in conjunction with alpha test to fake order-independent edge transparency
			mrt_blend_enabled[0] = mrt_blend_enabled[1] = mrt_blend_enabled[2] = mrt_blend_enabled[3] = true;
			blend_equation_override = true;
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);
		}
	}

	if (mrt_blend_enabled[0] || mrt_blend_enabled[1] || mrt_blend_enabled[2] || mrt_blend_enabled[3])
	{
		if (!blend_equation_override)
		{
			glBlendFuncSeparate(blend_factor(rsx::method_registers.blend_func_sfactor_rgb()),
				blend_factor(rsx::method_registers.blend_func_dfactor_rgb()),
				blend_factor(rsx::method_registers.blend_func_sfactor_a()),
				blend_factor(rsx::method_registers.blend_func_dfactor_a()));

			auto blend_colors = rsx::get_constant_blend_colors();
			glBlendColor(blend_colors[0], blend_colors[1], blend_colors[2], blend_colors[3]);

			glBlendEquationSeparate(blend_equation(rsx::method_registers.blend_equation_rgb()),
				blend_equation(rsx::method_registers.blend_equation_a()));
		}
	}

	gl_state.enablei(mrt_blend_enabled[0], GL_BLEND, 0);
	gl_state.enablei(mrt_blend_enabled[1], GL_BLEND, 1);
	gl_state.enablei(mrt_blend_enabled[2], GL_BLEND, 2);
	gl_state.enablei(mrt_blend_enabled[3], GL_BLEND, 3);

	if (gl_state.enable(rsx::method_registers.logic_op_enabled(), GL_COLOR_LOGIC_OP))
	{
		gl_state.logic_op(logic_op(rsx::method_registers.logic_operation()));
	}

	gl_state.line_width(rsx::method_registers.line_width());
	gl_state.enable(rsx::method_registers.line_smooth_enabled(), GL_LINE_SMOOTH);

	gl_state.enable(rsx::method_registers.poly_offset_point_enabled(), GL_POLYGON_OFFSET_POINT);
	gl_state.enable(rsx::method_registers.poly_offset_line_enabled(), GL_POLYGON_OFFSET_LINE);
	gl_state.enable(rsx::method_registers.poly_offset_fill_enabled(), GL_POLYGON_OFFSET_FILL);

	//offset_bias is the constant factor, multiplied by the implementation factor R
	//offst_scale is the slope factor, multiplied by the triangle slope factor M
	gl_state.polygon_offset(rsx::method_registers.poly_offset_scale(), rsx::method_registers.poly_offset_bias());

	if (gl_state.enable(rsx::method_registers.cull_face_enabled(), GL_CULL_FACE))
	{
		gl_state.cull_face(cull_face(rsx::method_registers.cull_face_mode()));
	}

	gl_state.front_face(front_face(rsx::method_registers.front_face_mode()));

	//TODO
	//NV4097_SET_ANISO_SPREAD
	//NV4097_SET_SPECULAR_ENABLE
	//NV4097_SET_TWO_SIDE_LIGHT_EN
	//NV4097_SET_FLAT_SHADE_OP
	//NV4097_SET_EDGE_FLAG



	//NV4097_SET_COLOR_KEY_COLOR
	//NV4097_SET_SHADER_CONTROL
	//NV4097_SET_ZMIN_MAX_CONTROL
	//NV4097_SET_ANTI_ALIASING_CONTROL
	//NV4097_SET_CLIP_ID_TEST_ENABLE

	std::chrono::time_point<steady_clock> now = steady_clock::now();
	m_begin_time += (u32)std::chrono::duration_cast<std::chrono::microseconds>(now - then).count();
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

	gl::screen.clear(gl::buffers::color);

	u32 buffer_width = display_buffers[buffer].width;
	u32 buffer_height = display_buffers[buffer].height;
	u32 buffer_pitch = display_buffers[buffer].pitch;

	if ((u32)buffer < display_buffers_count && buffer_width && buffer_height && buffer_pitch)
	{
		// Calculate blit coordinates
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

		// Find the source image
		rsx::tiled_region buffer_region = get_tiled_address(display_buffers[buffer].offset, CELL_GCM_LOCATION_LOCAL);
		u32 absolute_address = buffer_region.address + buffer_region.base;

		GLuint image = GL_NONE;

		if (auto render_target_texture = m_rtts.get_texture_from_render_target_if_applicable(absolute_address))
		{
			buffer_width = render_target_texture->width();
			buffer_height = render_target_texture->height();

			image = render_target_texture->raw_handle();
		}
		else if (auto surface = m_gl_texture_cache.find_texture_from_dimensions(absolute_address))
		{
			//Hack - this should be the first location to check for output
			//The render might have been done offscreen or in software and a blit used to display
			image = surface->get_raw_texture()->id();
		}
		else
		{
			LOG_WARNING(RSX, "Flip texture was not found in cache. Uploading surface from CPU");

			if (!buffer_pitch) buffer_pitch = buffer_width * 4;
			gl::pixel_unpack_settings unpack_settings;
			unpack_settings.aligment(1).row_length(buffer_pitch / 4);

			if (!m_flip_tex_color || m_flip_tex_color->size2D() != sizei{ (int)buffer_width, (int)buffer_height })
			{
				m_flip_tex_color.reset(new gl::texture(GL_TEXTURE_2D, buffer_width, buffer_height, 1, 1, GL_RGBA8));
			}

			if (buffer_region.tile)
			{
				std::unique_ptr<u8[]> temp(new u8[buffer_height * buffer_pitch]);
				buffer_region.read(temp.get(), buffer_width, buffer_height, buffer_pitch);
				m_flip_tex_color->copy_from(temp.get(), gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8, unpack_settings);
			}
			else
			{
				m_flip_tex_color->copy_from(buffer_region.ptr, gl::texture::format::bgra, gl::texture::type::uint_8_8_8_8, unpack_settings);
			}

			image = m_flip_tex_color->id();
		}

		areai screen_area = coordi({}, { (int)buffer_width, (int)buffer_height });
		auto avconfig = fxm::get<rsx::avconf>();

		if (g_cfg.video.full_rgb_range_output && (!avconfig || avconfig->gamma == 1.f))
		{
			// Blit source image to the screen
			// Disable scissor test (affects blit)
			glDisable(GL_SCISSOR_TEST);

			m_flip_fbo.recreate();
			m_flip_fbo.bind();
			m_flip_fbo.color = image;
			m_flip_fbo.read_buffer(m_flip_fbo.color);
			m_flip_fbo.blit(gl::screen, screen_area, areai(aspect_ratio).flipped_vertical(), gl::buffers::color, gl::filter::linear);
		}
		else
		{
			const f32 gamma = avconfig ? avconfig->gamma : 1.f;
			const bool limited_range = !g_cfg.video.full_rgb_range_output;

			gl::screen.bind();
			glViewport(0, 0, m_frame->client_width(), m_frame->client_height());
			m_video_output_pass.run(m_frame->client_width(), m_frame->client_height(), image, areai(aspect_ratio), gamma, limited_range);
		}
	}

	if (m_custom_ui)
	{
		gl::screen.bind();
		glViewport(0, 0, m_frame->client_width(), m_frame->client_height());
		m_ui_renderer.run(m_frame->client_width(), m_frame->client_height(), 0, *m_custom_ui.get());
	}

	if (g_cfg.video.overlay)
	{
		gl::screen.bind();
		glViewport(0, 0, m_frame->client_width(), m_frame->client_height());

		m_text_printer.print_text(0, 0, m_frame->client_width(), m_frame->client_height(), "RSX Load: " + std::to_string(get_load()) + "%");
		m_text_printer.print_text(0, 18, m_frame->client_width(), m_frame->client_height(), "draw calls: " + std::to_string(m_draw_calls));
		m_text_printer.print_text(0, 36, m_frame->client_width(), m_frame->client_height(), "draw call setup: " + std::to_string(m_begin_time) + "us");
		m_text_printer.print_text(0, 54, m_frame->client_width(), m_frame->client_height(), "vertex upload time: " + std::to_string(m_vertex_upload_time) + "us");
		m_text_printer.print_text(0, 72, m_frame->client_width(), m_frame->client_height(), "textures upload time: " + std::to_string(m_textures_upload_time) + "us");
		m_text_printer.print_text(0, 90, m_frame->client_width(), m_frame->client_height(), "draw call execution: " + std::to_string(m_draw_time) + "us");

		const auto num_dirty_textures = m_gl_texture_cache.get_unreleased_textures_count();
		const auto texture_memory_size = m_gl_texture_cache.get_texture_memory_in_use() / (1024 * 1024);
		const auto num_flushes = m_gl_texture_cache.get_num_flush_requests();
		const auto num_mispredict = m_gl_texture_cache.get_num_cache_mispredictions();
		const auto cache_miss_ratio = (u32)ceil(m_gl_texture_cache.get_cache_miss_ratio() * 100);
		m_text_printer.print_text(0, 126, m_frame->client_width(), m_frame->client_height(), "Unreleased textures: " + std::to_string(num_dirty_textures));
		m_text_printer.print_text(0, 144, m_frame->client_width(), m_frame->client_height(), "Texture memory: " + std::to_string(texture_memory_size) + "M");
		m_text_printer.print_text(0, 162, m_frame->client_width(), m_frame->client_height(), fmt::format("Flush requests: %d (%d%% hard faults, %d mispredictions)", num_flushes, cache_miss_ratio, num_mispredict));
	}

	m_frame->flip(m_context);
	rsx::thread::flip(buffer);

	// Cleanup
	m_gl_texture_cache.on_frame_end();

	m_rtts.free_invalidated();
	m_vertex_cache->purge();

	//If we are skipping the next frame, do not reset perf counters
	if (skip_frame) return;

	m_draw_calls = 0;
	m_begin_time = 0;
	m_draw_time = 0;
	m_vertex_upload_time = 0;
	m_textures_upload_time = 0;
}

bool GLGSRender::on_access_violation(u32 address, bool is_writing)
{
	bool can_flush = (std::this_thread::get_id() == m_thread_id);
	auto result = m_gl_texture_cache.invalidate_address(address, is_writing, can_flush);

	if (!result.violation_handled)
		return false;

	{
		std::lock_guard<shared_mutex> lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}

	if (result.num_flushable > 0)
	{
		work_item &task = post_flush_request(address, result);

		vm::temporary_unlock();
		{
			std::unique_lock<std::mutex> lock(task.guard_mutex);
			task.cv.wait(lock, [&task] { return task.processed; });
		}

		task.received = true;
		return true;
	}

	return true;
}

void GLGSRender::on_notify_memory_unmapped(u32 address_base, u32 size)
{
	//Discard all memory in that range without bothering with writeback (Force it for strict?)
	if (m_gl_texture_cache.invalidate_range(address_base, size, true, true, false).violation_handled)
	{
		m_gl_texture_cache.purge_dirty();
		{
			std::lock_guard<shared_mutex> lock(m_sampler_mutex);
			m_samplers_dirty.store(true);
		}
	}
}

void GLGSRender::do_local_task(bool /*idle*/)
{
	m_frame->clear_wm_events();

	if (!work_queue.empty())
	{
		std::lock_guard<shared_mutex> lock(queue_guard);

		work_queue.remove_if([](work_item &q) { return q.received; });

		for (work_item& q : work_queue)
		{
			if (q.processed) continue;

			std::unique_lock<std::mutex> lock(q.guard_mutex);
			q.result = m_gl_texture_cache.flush_all(q.section_data);
			q.processed = true;

			//Notify thread waiting on this
			lock.unlock();
			q.cv.notify_one();
		}
	}
	else if (!in_begin_end)
	{
		//This will re-engage locks and break the texture cache if another thread is waiting in access violation handler!
		//Only call when there are no waiters
		m_gl_texture_cache.do_update();
	}

	if (m_overlay_cleanup_requests.size())
	{
		m_ui_renderer.remove_temp_resources();
		m_overlay_cleanup_requests.clear();
	}
	else if (m_custom_ui)
	{
		if (!in_begin_end && native_ui_flip_request.load())
		{
			native_ui_flip_request.store(false);
			flip((s32)current_display_buffer);
		}
	}
}

work_item& GLGSRender::post_flush_request(u32 address, gl::texture_cache::thrashed_set& flush_data)
{
	std::lock_guard<shared_mutex> lock(queue_guard);

	work_queue.emplace_back();
	work_item &result = work_queue.back();
	result.address_to_flush = address;
	result.section_data = std::move(flush_data);
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
	m_samplers_dirty.store(true);
	return m_gl_texture_cache.blit(src, dst, interpolate, m_rtts);
}

void GLGSRender::notify_tile_unbound(u32 tile)
{
	//TODO: Handle texture writeback
	//u32 addr = rsx::get_address(tiles[tile].offset, tiles[tile].location);
	//on_notify_memory_unmapped(addr, tiles[tile].size);
	//m_rtts.invalidate_surface_address(addr, false);
}

void GLGSRender::begin_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	query->result = 0;
	glBeginQuery(GL_ANY_SAMPLES_PASSED, (GLuint)query->driver_handle);
}

void GLGSRender::end_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	verify(HERE), query->active;
	glEndQuery(GL_ANY_SAMPLES_PASSED);
}

bool GLGSRender::check_occlusion_query_status(rsx::reports::occlusion_query_info* query)
{
	if (!query->num_draws)
		return true;

	GLint status = GL_TRUE;
	glGetQueryObjectiv((GLuint)query->driver_handle, GL_QUERY_RESULT_AVAILABLE, &status);

	return status != GL_FALSE;
}

void GLGSRender::get_occlusion_query_result(rsx::reports::occlusion_query_info* query)
{
	if (query->num_draws)
	{
		GLint result = 0;
		glGetQueryObjectiv((GLuint)query->driver_handle, GL_QUERY_RESULT, &result);

		query->result += result;
	}
}

void GLGSRender::discard_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	if (query->active)
	{
		//Discard is being called on an active query, close it
		glEndQuery(GL_ANY_SAMPLES_PASSED);
	}
}

void GLGSRender::shell_do_cleanup()
{
	//TODO: Key cleanup requests with UID to identify resources to remove
	m_overlay_cleanup_requests.push_back(0);
}
