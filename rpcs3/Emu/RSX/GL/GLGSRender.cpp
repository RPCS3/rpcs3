#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "GLGSRender.h"
#include "GLCompute.h"
#include "GLVertexProgram.h"
#include "../Overlays/Shaders/shader_loading_dialog_native.h"
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

u64 GLGSRender::get_cycles()
{
	return thread_ctrl::get_cycles(static_cast<named_thread<GLGSRender>&>(*this));
}

GLGSRender::GLGSRender() : GSRender()
{
	m_shaders_cache = std::make_unique<gl::shader_cache>(m_prog_buffer, "opengl", "v1.91");

	if (g_cfg.video.disable_vertex_cache || g_cfg.video.multithreaded_rsx)
		m_vertex_cache = std::make_unique<gl::null_vertex_cache>();
	else
		m_vertex_cache = std::make_unique<gl::weak_vertex_cache>();

	backend_config.supports_hw_a2c = false;
	backend_config.supports_hw_a2one = false;
	backend_config.supports_multidraw = true;
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
		fmt::throw_exception("Unsupported comparison op 0x%X" HERE, static_cast<u32>(op));
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
		fmt::throw_exception("Unsupported stencil op 0x%X" HERE, static_cast<u32>(op));
	}

	GLenum blend_equation(rsx::blend_equation op)
	{
		switch (op)
		{
			// Note : maybe add is signed on gl
		case rsx::blend_equation::add_signed:
			rsx_log.trace("blend equation add_signed used. Emulating using FUNC_ADD");
		case rsx::blend_equation::add: return GL_FUNC_ADD;
		case rsx::blend_equation::min: return GL_MIN;
		case rsx::blend_equation::max: return GL_MAX;
		case rsx::blend_equation::substract: return GL_FUNC_SUBTRACT;
		case rsx::blend_equation::reverse_substract_signed:
			rsx_log.trace("blend equation reverse_subtract_signed used. Emulating using FUNC_REVERSE_SUBTRACT");
		case rsx::blend_equation::reverse_substract: return GL_FUNC_REVERSE_SUBTRACT;
		case rsx::blend_equation::reverse_add_signed:
		default:
			rsx_log.error("Blend equation 0x%X is unimplemented!", static_cast<u32>(op));
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
		fmt::throw_exception("Unsupported blend factor 0x%X" HERE, static_cast<u32>(op));
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
		fmt::throw_exception("Unsupported logic op 0x%X" HERE, static_cast<u32>(op));
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
		fmt::throw_exception("Unsupported front face 0x%X" HERE, static_cast<u32>(op));
	}

	GLenum cull_face(rsx::cull_face op)
	{
		switch (op)
		{
		case rsx::cull_face::front: return GL_FRONT;
		case rsx::cull_face::back: return GL_BACK;
		case rsx::cull_face::front_and_back: return GL_FRONT_AND_BACK;
		}
		fmt::throw_exception("Unsupported cull face 0x%X" HERE, static_cast<u32>(op));
	}
}

void GLGSRender::begin()
{
	rsx::thread::begin();

	if (skip_current_frame || cond_render_ctrl.disable_rendering())
		return;

	init_buffers(rsx::framebuffer_creation_context::context_draw);
}

void GLGSRender::end()
{
	m_profiler.start();

	if (skip_current_frame || !framebuffer_status_valid || cond_render_ctrl.disable_rendering())
	{
		execute_nop_draw();
		rsx::thread::end();
		return;
	}

	m_frame_stats.setup_time += m_profiler.duration();

	const auto do_heap_cleanup = [this]()
	{
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
	};

	gl::command_context cmd{ gl_state };
	gl::render_target *ds = std::get<1>(m_rtts.m_bound_depth_stencil);

	// Handle special memory barrier for ARGB8->D24S8 in an active DSV
	if (ds && ds->old_contents.size() == 1 &&
		ds->old_contents[0].source->get_internal_format() == gl::texture::internal_format::rgba8)
	{
		gl_state.enable(GL_FALSE, GL_SCISSOR_TEST);

		// TODO: Stencil transfer
		gl::g_hw_blitter->fast_clear_image(cmd, ds, 1.f, 0xFF);
		ds->old_contents[0].init_transfer(ds);

		m_depth_converter.run(ds->old_contents[0].src_rect(),
			ds->old_contents[0].dst_rect(),
			ds->old_contents[0].source, ds);

		ds->on_write();
	}

	// Load textures
	{
		m_profiler.start();

		std::lock_guard lock(m_sampler_mutex);
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
					*sampler_state = m_gl_texture_cache.upload_texture(cmd, rsx::method_registers.fragment_textures[i], m_rtts);

					if (m_textures_dirty[i])
						m_fs_sampler_states[i].apply(rsx::method_registers.fragment_textures[i], fs_sampler_state[i].get());
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
					*sampler_state = m_gl_texture_cache.upload_texture(cmd, rsx::method_registers.vertex_textures[i], m_rtts);

					if (m_vertex_textures_dirty[i])
						m_vs_sampler_states[i].apply(rsx::method_registers.vertex_textures[i], vs_sampler_state[i].get());
				}
				else
					*sampler_state = {};

				m_vertex_textures_dirty[i] = false;
			}
		}

		m_samplers_dirty.store(false);

		m_frame_stats.textures_upload_time += m_profiler.duration();
	}

	// NOTE: Due to common OpenGL driver architecture, vertex data has to be uploaded as far away from the draw as possible
	// TODO: Implement shaders cache prediction to avoid uploading vertex data if draw is going to skip
	if (!load_program())
	{
		// Program is not ready, skip drawing this
		std::this_thread::yield();
		execute_nop_draw();
		// m_rtts.on_write(); - breaks games for obvious reasons
		rsx::thread::end();
		return;
	}

	// Load program execution environment
	load_program_env();

	m_frame_stats.setup_time += m_profiler.duration();

	//Bind textures and resolve external copy operations
	for (int i = 0; i < rsx::limits::fragment_textures_count; ++i)
	{
		if (current_fp_metadata.referenced_textures_mask & (1 << i))
		{
			_SelectTexture(GL_FRAGMENT_TEXTURES_START + i);

			gl::texture_view* view = nullptr;
			auto sampler_state = static_cast<gl::texture_cache::sampled_image_descriptor*>(fs_sampler_state[i].get());

			if (rsx::method_registers.fragment_textures[i].enabled() &&
				sampler_state->validate())
			{
				if (view = sampler_state->image_handle; !view) [[unlikely]]
				{
					view = m_gl_texture_cache.create_temporary_subresource(cmd, sampler_state->external_subresource_desc);
				}
			}

			if (view) [[likely]]
			{
				view->bind();

				if (current_fragment_program.redirected_textures & (1 << i))
				{
					_SelectTexture(GL_STENCIL_MIRRORS_START + i);

					auto root_texture = static_cast<gl::viewable_image*>(view->image());
					auto stencil_view = root_texture->get_view(0xAAE4, rsx::default_remap_vector, gl::image_aspect::stencil);
					stencil_view->bind();
				}
			}
			else
			{
				auto target = gl::get_target(current_fragment_program.get_texture_dimension(i));
				glBindTexture(target, m_null_textures[target]->id());

				if (current_fragment_program.redirected_textures & (1 << i))
				{
					_SelectTexture(GL_STENCIL_MIRRORS_START + i);
					glBindTexture(target, m_null_textures[target]->id());
				}
			}
		}
	}

	for (int i = 0; i < rsx::limits::vertex_textures_count; ++i)
	{
		if (current_vp_metadata.referenced_textures_mask & (1 << i))
		{
			auto sampler_state = static_cast<gl::texture_cache::sampled_image_descriptor*>(vs_sampler_state[i].get());
			_SelectTexture(GL_VERTEX_TEXTURES_START + i);

			if (rsx::method_registers.vertex_textures[i].enabled() &&
				sampler_state->validate())
			{
				if (sampler_state->image_handle) [[likely]]
				{
					sampler_state->image_handle->bind();
				}
				else
				{
					m_gl_texture_cache.create_temporary_subresource(cmd, sampler_state->external_subresource_desc)->bind();
				}
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, GL_NONE);
			}
		}
	}

	m_gl_texture_cache.release_uncached_temporary_subresources();

	m_frame_stats.textures_upload_time += m_profiler.duration();

	// Optionally do memory synchronization if the texture stage has not yet triggered this
	if (true)//g_cfg.video.strict_rendering_mode)
	{
		gl_state.enable(GL_FALSE, GL_SCISSOR_TEST);

		if (ds) ds->write_barrier(cmd);

		for (auto &rtt : m_rtts.m_bound_render_targets)
		{
			if (auto surface = std::get<1>(rtt))
			{
				surface->write_barrier(cmd);
			}
		}
	}
	else
	{
		rsx::simple_array<int> buffers_to_clear;
		bool clear_all_color = true;
		bool clear_depth = false;

		for (int index = 0; index < 4; index++)
		{
			if (m_rtts.m_bound_render_targets[index].first)
			{
				if (!m_rtts.m_bound_render_targets[index].second->dirty())
					clear_all_color = false;
				else
					buffers_to_clear.push_back(index);
			}
		}

		if (ds && ds->dirty())
		{
			clear_depth = true;
		}

		if (clear_depth || !buffers_to_clear.empty())
		{
			gl_state.enable(GL_FALSE, GL_SCISSOR_TEST);
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

			if (!buffers_to_clear.empty() && !clear_all_color)
			{
				GLfloat colors[] = { 0.f, 0.f, 0.f, 0.f };
				//It is impossible for the render target to be type A or B here (clear all would have been flagged)
				for (auto &i : buffers_to_clear)
					glClearBufferfv(GL_COLOR, i, colors);
			}

			if (clear_depth)
				gl_state.depth_mask(rsx::method_registers.depth_write_enabled());
		}
	}

	// Unconditionally enable stencil test if it was disabled before
	gl_state.enable(GL_TRUE, GL_SCISSOR_TEST);

	update_draw_state();

	if (g_cfg.video.debug_output)
	{
		m_program->validate();
	}

	const GLenum draw_mode = gl::draw_mode(rsx::method_registers.current_draw_clause.primitive);
	rsx::method_registers.current_draw_clause.begin();
	int subdraw = 0;
	do
	{
		if (!subdraw)
		{
			analyse_inputs_interleaved(m_vertex_layout);
			if (!m_vertex_layout.validate())
			{
				// Execute remainining pipeline barriers with NOP draw
				do
				{
					rsx::method_registers.current_draw_clause.execute_pipeline_dependencies();
				}
				while (rsx::method_registers.current_draw_clause.next());

				rsx::method_registers.current_draw_clause.end();
				break;
			}
		}
		else
		{
			if (rsx::method_registers.current_draw_clause.execute_pipeline_dependencies() & rsx::vertex_base_changed)
			{
				// Rebase vertex bases instead of
				for (auto &info : m_vertex_layout.interleaved_blocks)
				{
					const auto vertex_base_offset = rsx::method_registers.vertex_data_base_offset();
					info.real_offset_address = rsx::get_address(rsx::get_vertex_offset_from_base(vertex_base_offset, info.base_offset), info.memory_location, HERE);
				}
			}
		}

		++subdraw;

		if (manually_flush_ring_buffers)
		{
			//Use approximations to reserve space. This path is mostly for debug purposes anyway
			u32 approx_vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
			u32 approx_working_buffer_size = approx_vertex_count * 256;

			//Allocate 256K heap if we have no approximation at this time (inlined array)
			m_attrib_ring_buffer->reserve_storage_on_heap(std::max(approx_working_buffer_size, 256 * 1024U));
			m_index_ring_buffer->reserve_storage_on_heap(16 * 1024);
		}

		//Do vertex upload before RTT prep / texture lookups to give the driver time to push data
		auto upload_info = set_vertex_buffer();
		do_heap_cleanup();

		if (upload_info.vertex_draw_count == 0)
		{
			// Malformed vertex setup; abort
			continue;
		}

		update_vertex_env(upload_info);

		if (!upload_info.index_info)
		{
			if (rsx::method_registers.current_draw_clause.is_single_draw())
			{
				glDrawArrays(draw_mode, 0, upload_info.vertex_draw_count);
			}
			else
			{
				const auto subranges = rsx::method_registers.current_draw_clause.get_subranges();
				const auto draw_count = subranges.size();
				const auto driver_caps = gl::get_driver_caps();
				bool use_draw_arrays_fallback = false;

				m_scratch_buffer.resize(draw_count * 24);
				GLint* firsts = reinterpret_cast<GLint*>(m_scratch_buffer.data());
				GLsizei* counts = (firsts + draw_count);
				const GLvoid** offsets = reinterpret_cast<const GLvoid**>(counts + draw_count);

				u32 first = 0;
				u32 dst_index = 0;
				for (const auto &range : subranges)
				{
					firsts[dst_index] = first;
					counts[dst_index] = range.count;
					offsets[dst_index++] = reinterpret_cast<const GLvoid*>(u64{first << 2});

					if (driver_caps.vendor_AMD && (first + range.count) > (0x100000 >> 2))
					{
						//Unlikely, but added here in case the identity buffer is not large enough somehow
						use_draw_arrays_fallback = true;
						break;
					}

					first += range.count;
				}

				if (use_draw_arrays_fallback)
				{
					//MultiDrawArrays is broken on some primitive types using AMD. One known type is GL_TRIANGLE_STRIP but there could be more
					for (u32 n = 0; n < draw_count; ++n)
					{
						glDrawArrays(draw_mode, firsts[n], counts[n]);
					}
				}
				else if (driver_caps.vendor_AMD)
				{
					//Use identity index buffer to fix broken vertexID on AMD
					m_identity_index_buffer->bind();
					glMultiDrawElements(draw_mode, counts, GL_UNSIGNED_INT, offsets, static_cast<GLsizei>(draw_count));
				}
				else
				{
					//Normal render
					glMultiDrawArrays(draw_mode, firsts, counts, static_cast<GLsizei>(draw_count));
				}
			}
		}
		else
		{
			const GLenum index_type = std::get<0>(*upload_info.index_info);
			const u32 index_offset = std::get<1>(*upload_info.index_info);
			const bool restarts_valid = gl::is_primitive_native(rsx::method_registers.current_draw_clause.primitive) && !rsx::method_registers.current_draw_clause.is_disjoint_primitive;

			if (gl_state.enable(restarts_valid && rsx::method_registers.restart_index_enabled(), GL_PRIMITIVE_RESTART))
			{
				glPrimitiveRestartIndex((index_type == GL_UNSIGNED_SHORT) ? 0xffff : 0xffffffff);
			}

			m_index_ring_buffer->bind();

			if (rsx::method_registers.current_draw_clause.is_single_draw())
			{
				glDrawElements(draw_mode, upload_info.vertex_draw_count, index_type, reinterpret_cast<GLvoid*>(u64{index_offset}));
			}
			else
			{
				const auto subranges = rsx::method_registers.current_draw_clause.get_subranges();
				const auto draw_count = subranges.size();
				const u32 type_scale = (index_type == GL_UNSIGNED_SHORT) ? 1 : 2;
				uintptr_t index_ptr = index_offset;
				m_scratch_buffer.resize(draw_count * 16);

				GLsizei *counts = reinterpret_cast<GLsizei*>(m_scratch_buffer.data());
				const GLvoid** offsets = reinterpret_cast<const GLvoid**>(counts + draw_count);
				int dst_index = 0;

				for (const auto &range : subranges)
				{
					const auto index_size = get_index_count(rsx::method_registers.current_draw_clause.primitive, range.count);
					counts[dst_index] = index_size;
					offsets[dst_index++] = reinterpret_cast<const GLvoid*>(index_ptr);

					index_ptr += (index_size << type_scale);
				}

				glMultiDrawElements(draw_mode, counts, index_type, offsets, static_cast<GLsizei>(draw_count));
			}
		}
	} while (rsx::method_registers.current_draw_clause.next());

	m_rtts.on_write(m_framebuffer_layout.color_write_enabled.data(), m_framebuffer_layout.zeta_write_enabled);

	m_attrib_ring_buffer->notify();
	m_index_ring_buffer->notify();
	m_fragment_env_buffer->notify();
	m_vertex_env_buffer->notify();
	m_texture_parameters_buffer->notify();
	m_vertex_layout_buffer->notify();
	m_fragment_constants_buffer->notify();
	m_transform_constants_buffer->notify();

	m_frame_stats.textures_upload_time += m_profiler.duration();

	rsx::thread::end();
}

void GLGSRender::set_viewport()
{
	// NOTE: scale offset matrix already contains the viewport transformation
	const auto clip_width = rsx::apply_resolution_scale(rsx::method_registers.surface_clip_width(), true);
	const auto clip_height = rsx::apply_resolution_scale(rsx::method_registers.surface_clip_height(), true);
	glViewport(0, 0, clip_width, clip_height);
}

void GLGSRender::set_scissor(bool clip_viewport)
{
	areau scissor;
	if (get_scissor(scissor, clip_viewport))
	{
		// NOTE: window origin does not affect scissor region (probably only affects viewport matrix; already applied)
		// See LIMBO [NPUB-30373] which uses shader window origin = top
		glScissor(scissor.x1, scissor.y1, scissor.width(), scissor.height());
		gl_state.enable(GL_TRUE, GL_SCISSOR_TEST);
	}
}

void GLGSRender::on_init_thread()
{
	verify(HERE), m_frame;

	// NOTES: All contexts have to be created before any is bound to a thread
	// This allows context sharing to work (both GLRCs passed to wglShareLists have to be idle or you get ERROR_BUSY)
	m_context = m_frame->make_context();

	if (!g_cfg.video.disable_asynchronous_shader_compiler)
	{
		m_decompiler_context = m_frame->make_context();
	}

	// Bind primary context to main RSX thread
	m_frame->set_current(m_context);

	zcull_ctrl.reset(static_cast<::rsx::reports::ZCULL_control*>(this));

	gl::init();

	//Enable adaptive vsync if vsync is requested
	gl::set_swapinterval(g_cfg.video.vsync ? -1 : 0);

	if (g_cfg.video.debug_output)
		gl::enable_debugging();

	rsx_log.notice("GL RENDERER: %s (%s)", reinterpret_cast<const char*>(glGetString(GL_RENDERER)), reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	rsx_log.notice("GL VERSION: %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	rsx_log.notice("GLSL VERSION: %s", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

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
		rsx_log.warning("High precision Z buffer requested but your GPU does not support GL_ARB_depth_buffer_float. Option ignored.");
	}

	if (!gl_caps.ARB_texture_barrier_supported && !gl_caps.NV_texture_barrier_supported && !g_cfg.video.strict_rendering_mode)
	{
		rsx_log.warning("Texture barriers are not supported by your GPU. Feedback loops will have undefined results.");
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

	rsx_log.notice("Supported texel buffer size reported: %d bytes", m_max_texbuffer_size);
	if (m_max_texbuffer_size < (16 * 0x100000))
	{
		rsx_log.error("Max texture buffer size supported is less than 16M which is useless. Expect undefined behaviour.");
		m_max_texbuffer_size = (16 * 0x100000);
	}

	//Array stream buffer
	{
		m_gl_persistent_stream_buffer = std::make_unique<gl::texture>(GL_TEXTURE_BUFFER, 0, 0, 0, 0, GL_R8UI);
		_SelectTexture(GL_STREAM_BUFFER_START + 0);
		glBindTexture(GL_TEXTURE_BUFFER, m_gl_persistent_stream_buffer->id());
	}

	//Register stream buffer
	{
		m_gl_volatile_stream_buffer = std::make_unique<gl::texture>(GL_TEXTURE_BUFFER, 0, 0, 0, 0, GL_R8UI);
		_SelectTexture(GL_STREAM_BUFFER_START + 1);
		glBindTexture(GL_TEXTURE_BUFFER, m_gl_volatile_stream_buffer->id());
	}

	//Fallback null texture instead of relying on texture0
	{
		std::vector<u32> pixeldata = { 0, 0, 0, 0 };

		//1D
		auto tex1D = std::make_unique<gl::texture>(GL_TEXTURE_1D, 1, 1, 1, 1, GL_RGBA8);
		tex1D->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, {});

		//2D
		auto tex2D = std::make_unique<gl::texture>(GL_TEXTURE_2D, 1, 1, 1, 1, GL_RGBA8);
		tex2D->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, {});

		//3D
		auto tex3D = std::make_unique<gl::texture>(GL_TEXTURE_3D, 1, 1, 1, 1, GL_RGBA8);
		tex3D->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, {});

		//CUBE
		auto texCUBE = std::make_unique<gl::texture>(GL_TEXTURE_CUBE_MAP, 1, 1, 1, 1, GL_RGBA8);
		texCUBE->copy_from(pixeldata.data(), gl::texture::format::rgba, gl::texture::type::uint_8_8_8_8, {});

		m_null_textures[GL_TEXTURE_1D] = std::move(tex1D);
		m_null_textures[GL_TEXTURE_2D] = std::move(tex2D);
		m_null_textures[GL_TEXTURE_3D] = std::move(tex3D);
		m_null_textures[GL_TEXTURE_CUBE_MAP] = std::move(texCUBE);
	}

	if (!gl_caps.ARB_buffer_storage_supported)
	{
		rsx_log.warning("Forcing use of legacy OpenGL buffers because ARB_buffer_storage is not supported");
		// TODO: do not modify config options
		g_cfg.video.gl_legacy_buffers.from_string("true");
	}

	if (g_cfg.video.gl_legacy_buffers)
	{
		rsx_log.warning("Using legacy openGL buffers.");
		manually_flush_ring_buffers = true;

		m_attrib_ring_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_transform_constants_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_fragment_constants_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_fragment_env_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_vertex_env_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_texture_parameters_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_vertex_layout_buffer = std::make_unique<gl::legacy_ring_buffer>();
		m_index_ring_buffer = std::make_unique<gl::legacy_ring_buffer>();
	}
	else
	{
		m_attrib_ring_buffer = std::make_unique<gl::ring_buffer>();
		m_transform_constants_buffer = std::make_unique<gl::ring_buffer>();
		m_fragment_constants_buffer = std::make_unique<gl::ring_buffer>();
		m_fragment_env_buffer = std::make_unique<gl::ring_buffer>();
		m_vertex_env_buffer = std::make_unique<gl::ring_buffer>();
		m_texture_parameters_buffer = std::make_unique<gl::ring_buffer>();
		m_vertex_layout_buffer = std::make_unique<gl::ring_buffer>();
		m_index_ring_buffer = std::make_unique<gl::ring_buffer>();
	}

	m_attrib_ring_buffer->create(gl::buffer::target::texture, 256 * 0x100000);
	m_index_ring_buffer->create(gl::buffer::target::element_array, 64 * 0x100000);
	m_transform_constants_buffer->create(gl::buffer::target::uniform, 64 * 0x100000);
	m_fragment_constants_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_fragment_env_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_vertex_env_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_texture_parameters_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);
	m_vertex_layout_buffer->create(gl::buffer::target::uniform, 16 * 0x100000);

	if (gl_caps.vendor_AMD)
	{
		m_identity_index_buffer = std::make_unique<gl::buffer>();
		m_identity_index_buffer->create(gl::buffer::target::element_array, 1 * 0x100000, nullptr, gl::buffer::memory_type::host_visible);

		// Initialize with 256k identity entries
		auto* dst = reinterpret_cast<u32*>(m_identity_index_buffer->map(gl::buffer::access::write));
		for (u32 n = 0; n < (0x100000 >> 2); ++n)
		{
			dst[n] = n;
		}

		m_identity_index_buffer->unmap();
	}
	else if (gl_caps.vendor_NVIDIA)
	{
		// NOTE: On NVIDIA cards going back decades (including the PS3) there is a slight normalization inaccuracy in compressed formats.
		// Confirmed in BLES01916 (The Evil Within) which uses RGB565 for some virtual texturing data.
		backend_config.supports_hw_renormalization = true;
	}

	m_persistent_stream_view.update(m_attrib_ring_buffer.get(), 0, std::min<u32>(static_cast<u32>(m_attrib_ring_buffer->size()), m_max_texbuffer_size));
	m_volatile_stream_view.update(m_attrib_ring_buffer.get(), 0, std::min<u32>(static_cast<u32>(m_attrib_ring_buffer->size()), m_max_texbuffer_size));
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

	int image_unit = 0;
	for (auto &sampler : m_fs_sampler_states)
	{
		sampler.create();
		sampler.bind(image_unit++);
	}

	for (auto &sampler : m_fs_sampler_mirror_states)
	{
		sampler.create();
		sampler.apply_defaults();
		sampler.bind(image_unit++);
	}

	for (auto &sampler : m_vs_sampler_states)
	{
		sampler.create();
		sampler.bind(image_unit++);
	}

	//Occlusion query
	for (u32 i = 0; i < occlusion_query_count; ++i)
	{
		GLuint handle = 0;
		auto &query = m_occlusion_query_data[i];
		glGenQueries(1, &handle);

		query.driver_handle = handle;
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

	if (!m_overlay_manager)
	{
		m_frame->hide();
		m_shaders_cache->load(nullptr);
		m_frame->show();
	}
	else
	{
		rsx::shader_loading_dialog_native dlg(this);

		m_shaders_cache->load(&dlg);
	}
}


void GLGSRender::on_exit()
{
	// Globals
	// TODO: Move these
	gl::destroy_compute_tasks();

	if (gl::g_typeless_transfer_buffer)
	{
		gl::g_typeless_transfer_buffer.remove();
	}

	zcull_ctrl.release();

	m_prog_buffer.clear();
	m_rtts.destroy();

	for (auto &fbo : m_framebuffer_cache)
	{
		fbo.remove();
	}

	m_framebuffer_cache.clear();

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

	for (auto &sampler : m_fs_sampler_states)
	{
		sampler.remove();
	}

	for (auto &sampler : m_fs_sampler_mirror_states)
	{
		sampler.remove();
	}

	for (auto &sampler : m_vs_sampler_states)
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

	if (m_fragment_env_buffer)
	{
		m_fragment_env_buffer->remove();
	}

	if (m_vertex_env_buffer)
	{
		m_vertex_env_buffer->remove();
	}

	if (m_texture_parameters_buffer)
	{
		m_texture_parameters_buffer->remove();
	}

	if (m_vertex_layout_buffer)
	{
		m_vertex_layout_buffer->remove();
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

		GLuint handle = query.driver_handle;
		glDeleteQueries(1, &handle);
		query.driver_handle = 0;
	}

	glFlush();
	glFinish();

	GSRender::on_exit();
}

void GLGSRender::clear_surface(u32 arg)
{
	if (skip_current_frame) return;

	// If stencil write mask is disabled, remove clear_stencil bit
	if (!rsx::method_registers.stencil_mask()) arg &= ~0x2u;

	// Ignore invalid clear flags
	if ((arg & 0xf3) == 0) return;

	u8 ctx = rsx::framebuffer_creation_context::context_draw;
	if (arg & 0xF0) ctx |= rsx::framebuffer_creation_context::context_clear_color;
	if (arg & 0x3) ctx |= rsx::framebuffer_creation_context::context_clear_depth;

	init_buffers(static_cast<rsx::framebuffer_creation_context>(ctx), true);

	if (!framebuffer_status_valid) return;

	GLbitfield mask = 0;

	gl::command_context cmd{ gl_state };
	const bool require_mem_load =
		rsx::method_registers.scissor_origin_x() > 0 ||
		rsx::method_registers.scissor_origin_y() > 0 ||
		rsx::method_registers.scissor_width() < rsx::method_registers.surface_clip_width() ||
		rsx::method_registers.scissor_height() < rsx::method_registers.surface_clip_height();

	bool update_color = false, update_z = false;
	rsx::surface_depth_format surface_depth_format = rsx::method_registers.surface_depth_fmt();

	if (auto ds = std::get<1>(m_rtts.m_bound_depth_stencil); arg & 0x3)
	{
		if (arg & 0x1)
		{
			u32 max_depth_value = get_max_depth_value(surface_depth_format);
			u32 clear_depth = rsx::method_registers.z_clear_value(surface_depth_format == rsx::surface_depth_format::z24s8);

			gl_state.depth_mask(GL_TRUE);
			gl_state.clear_depth(f32(clear_depth) / max_depth_value);
			mask |= GLenum(gl::buffers::depth);
		}

		if (surface_depth_format == rsx::surface_depth_format::z24s8)
		{
			if (arg & 0x2)
			{
				u8 clear_stencil = rsx::method_registers.stencil_clear_value();

				gl_state.stencil_mask(rsx::method_registers.stencil_mask());
				gl_state.clear_stencil(clear_stencil);
				mask |= GLenum(gl::buffers::stencil);
			}

			if ((arg & 0x3) != 0x3 && !require_mem_load && ds->dirty())
			{
				verify(HERE), mask;

				// Only one aspect was cleared. Make sure to memory intialize the other before removing dirty flag
				if (arg == 1)
				{
					// Depth was cleared, initialize stencil
					gl_state.stencil_mask(0xFF);
					gl_state.clear_stencil(0xFF);
					mask |= GLenum(gl::buffers::stencil);
				}
				else
				{
					// Stencil was cleared, initialize depth
					gl_state.depth_mask(GL_TRUE);
					gl_state.clear_depth(1.f);
					mask |= GLenum(gl::buffers::depth);
				}
			}
		}

		if (mask)
		{
			if (require_mem_load) ds->write_barrier(cmd);

			// Memory has been initialized
			update_z = true;
		}
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

			gl_state.clear_color(clear_r, clear_g, clear_b, clear_a);
			mask |= GLenum(gl::buffers::color);

			for (u8 index = m_rtts.m_bound_render_targets_config.first, count = 0;
				 count < m_rtts.m_bound_render_targets_config.second;
				 ++count, ++index)
			{
				if (require_mem_load) m_rtts.m_bound_render_targets[index].second->write_barrier(cmd);
				gl_state.color_maski(count, colormask);
			}

			update_color = true;
			break;
		}
		}
	}

	if (update_color || update_z)
	{
		const bool write_all_mask[] = { true, true, true, true };
		m_rtts.on_write(update_color ? write_all_mask : nullptr, update_z);
	}

	glClear(mask);
}

bool GLGSRender::load_program()
{
	if (m_graphics_state & rsx::pipeline_state::invalidate_pipeline_bits)
	{
		get_current_fragment_program(fs_sampler_state);
		verify(HERE), current_fragment_program.valid;

		get_current_vertex_program(vs_sampler_state);

		current_vertex_program.skip_vertex_input_check = true;	//not needed for us since decoding is done server side
		current_fragment_program.unnormalized_coords = 0; //unused
	}
	else if (m_program)
	{
		// Program already loaded
		return true;
	}

	void* pipeline_properties = nullptr;
	m_program = m_prog_buffer.get_graphics_pipeline(current_vertex_program, current_fragment_program, pipeline_properties,
			!g_cfg.video.disable_asynchronous_shader_compiler).get();

	if (m_prog_buffer.check_cache_missed())
	{
		if (m_prog_buffer.check_program_linked_flag())
		{
			// Program was linked or queued for linking
			m_shaders_cache->store(pipeline_properties, current_vertex_program, current_fragment_program);
		}

		// Notify the user with HUD notification
		if (g_cfg.misc.show_shader_compilation_hint)
		{
			if (m_overlay_manager)
			{
				if (auto dlg = m_overlay_manager->get<rsx::overlays::shader_compile_notification>())
				{
					// Extend duration
					dlg->touch();
				}
				else
				{
					// Create dialog but do not show immediately
					m_overlay_manager->create<rsx::overlays::shader_compile_notification>();
				}
			}
		}
	}

	return m_program != nullptr;
}

void GLGSRender::load_program_env()
{
	if (!m_program)
	{
		fmt::throw_exception("Unreachable right now" HERE);
	}

	const u32 fragment_constants_size = current_fp_metadata.program_constants_buffer_length;

	const bool update_transform_constants = !!(m_graphics_state & rsx::pipeline_state::transform_constants_dirty);
	const bool update_fragment_constants = !!(m_graphics_state & rsx::pipeline_state::fragment_constants_dirty) && fragment_constants_size;
	const bool update_vertex_env = !!(m_graphics_state & rsx::pipeline_state::vertex_state_dirty);
	const bool update_fragment_env = !!(m_graphics_state & rsx::pipeline_state::fragment_state_dirty);
	const bool update_fragment_texture_env = !!(m_graphics_state & rsx::pipeline_state::fragment_texture_state_dirty);

	m_program->use();

	if (manually_flush_ring_buffers)
	{
		if (update_fragment_env) m_fragment_env_buffer->reserve_storage_on_heap(128);
		if (update_vertex_env) m_vertex_env_buffer->reserve_storage_on_heap(256);
		if (update_fragment_texture_env) m_texture_parameters_buffer->reserve_storage_on_heap(256);
		if (update_fragment_constants) m_fragment_constants_buffer->reserve_storage_on_heap(align(fragment_constants_size, 256));
		if (update_transform_constants) m_transform_constants_buffer->reserve_storage_on_heap(8192);
	}

	if (update_vertex_env)
	{
		// Vertex state
		auto mapping = m_vertex_env_buffer->alloc_from_heap(144, m_uniform_buffer_offset_align);
		auto buf = static_cast<u8*>(mapping.first);
		fill_scale_offset_data(buf, false);
		fill_user_clip_data(buf + 64);
		*(reinterpret_cast<u32*>(buf + 128)) = rsx::method_registers.transform_branch_bits();
		*(reinterpret_cast<f32*>(buf + 132)) = rsx::method_registers.point_size();
		*(reinterpret_cast<f32*>(buf + 136)) = rsx::method_registers.clip_min();
		*(reinterpret_cast<f32*>(buf + 140)) = rsx::method_registers.clip_max();

		m_vertex_env_buffer->bind_range(GL_VERTEX_PARAMS_BIND_SLOT, mapping.second, 144);
	}

	if (update_transform_constants)
	{
		// Vertex constants
		auto mapping = m_transform_constants_buffer->alloc_from_heap(8192, m_uniform_buffer_offset_align);
		auto buf = static_cast<u8*>(mapping.first);
		fill_vertex_program_constants_data(buf);

		m_transform_constants_buffer->bind_range(GL_VERTEX_CONSTANT_BUFFERS_BIND_SLOT, mapping.second, 8192);
	}

	if (update_fragment_constants)
	{
		// Fragment constants
		auto mapping = m_fragment_constants_buffer->alloc_from_heap(fragment_constants_size, m_uniform_buffer_offset_align);
		auto buf = static_cast<u8*>(mapping.first);

		m_prog_buffer.fill_fragment_constants_buffer({ reinterpret_cast<float*>(buf), fragment_constants_size },
			current_fragment_program, gl::get_driver_caps().vendor_NVIDIA);

		m_fragment_constants_buffer->bind_range(GL_FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT, mapping.second, fragment_constants_size);
	}

	if (update_fragment_env)
	{
		// Fragment state
		auto mapping = m_fragment_env_buffer->alloc_from_heap(32, m_uniform_buffer_offset_align);
		auto buf = static_cast<u8*>(mapping.first);
		fill_fragment_state_buffer(buf, current_fragment_program);

		m_fragment_env_buffer->bind_range(GL_FRAGMENT_STATE_BIND_SLOT, mapping.second, 32);
	}

	if (update_fragment_texture_env)
	{
		// Fragment texture parameters
		auto mapping = m_texture_parameters_buffer->alloc_from_heap(256, m_uniform_buffer_offset_align);
		auto buf = static_cast<u8*>(mapping.first);
		fill_fragment_texture_parameters(buf, current_fragment_program);

		m_texture_parameters_buffer->bind_range(GL_FRAGMENT_TEXTURE_PARAMS_BIND_SLOT, mapping.second, 256);
	}

	if (manually_flush_ring_buffers)
	{
		if (update_fragment_env) m_fragment_env_buffer->unmap();
		if (update_vertex_env) m_vertex_env_buffer->unmap();
		if (update_fragment_texture_env) m_texture_parameters_buffer->unmap();
		if (update_fragment_constants) m_fragment_constants_buffer->unmap();
		if (update_transform_constants) m_transform_constants_buffer->unmap();
	}

	const u32 handled_flags = (rsx::pipeline_state::fragment_state_dirty | rsx::pipeline_state::vertex_state_dirty | rsx::pipeline_state::transform_constants_dirty | rsx::pipeline_state::fragment_constants_dirty | rsx::pipeline_state::fragment_texture_state_dirty);
	m_graphics_state &= ~handled_flags;
}

void GLGSRender::update_vertex_env(const gl::vertex_upload_info& upload_info)
{
	if (manually_flush_ring_buffers)
	{
		m_vertex_layout_buffer->reserve_storage_on_heap(128 + 16);
	}

	// Vertex layout state
	auto mapping = m_vertex_layout_buffer->alloc_from_heap(128 + 16, m_uniform_buffer_offset_align);
	auto buf = static_cast<u32*>(mapping.first);

	buf[0] = upload_info.vertex_index_base;
	buf[1] = upload_info.vertex_index_offset;
	buf += 4;

	fill_vertex_layout_state(m_vertex_layout, upload_info.first_vertex, upload_info.allocated_vertex_count, reinterpret_cast<s32*>(buf), upload_info.persistent_mapping_offset, upload_info.volatile_mapping_offset);

	m_vertex_layout_buffer->bind_range(GL_VERTEX_LAYOUT_BIND_SLOT, mapping.second, 128 + 16);

	if (manually_flush_ring_buffers)
	{
		m_vertex_layout_buffer->unmap();
	}
}

void GLGSRender::update_draw_state()
{
	m_profiler.start();

	for (int index = 0; index < m_rtts.get_color_surface_count(); ++index)
	{
		bool color_mask_b = rsx::method_registers.color_mask_b(index);
		bool color_mask_g = rsx::method_registers.color_mask_g(index);
		bool color_mask_r = rsx::method_registers.color_mask_r(index);
		bool color_mask_a = rsx::method_registers.color_mask_a(index);

		if (rsx::method_registers.surface_color() == rsx::surface_color_format::g8b8)
		{
			//Map GB components onto RG
			rsx::get_g8b8_r8g8_colormask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);
		}

		gl_state.color_maski(index, color_mask_r, color_mask_g, color_mask_b, color_mask_a);
	}

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

	if (mrt_blend_enabled[0] || mrt_blend_enabled[1] || mrt_blend_enabled[2] || mrt_blend_enabled[3])
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
	//offset_scale is the slope factor, multiplied by the triangle slope factor M
	gl_state.polygon_offset(rsx::method_registers.poly_offset_scale(), rsx::method_registers.poly_offset_bias());

	if (gl_state.enable(rsx::method_registers.cull_face_enabled(), GL_CULL_FACE))
	{
		gl_state.cull_face(cull_face(rsx::method_registers.cull_face_mode()));
	}

	gl_state.front_face(front_face(rsx::method_registers.front_face_mode()));

	// Sample control
	// TODO: MinSampleShading
	//gl_state.enable(rsx::method_registers.msaa_enabled(), GL_MULTISAMPLE);
	//gl_state.enable(rsx::method_registers.msaa_alpha_to_coverage_enabled(), GL_SAMPLE_ALPHA_TO_COVERAGE);
	//gl_state.enable(rsx::method_registers.msaa_alpha_to_one_enabled(), GL_SAMPLE_ALPHA_TO_ONE);

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

	m_frame_stats.setup_time += m_profiler.duration();
}

bool GLGSRender::on_access_violation(u32 address, bool is_writing)
{
	const bool can_flush = (std::this_thread::get_id() == m_rsx_thread);
	const rsx::invalidation_cause cause =
		is_writing ? (can_flush ? rsx::invalidation_cause::write : rsx::invalidation_cause::deferred_write)
		           : (can_flush ? rsx::invalidation_cause::read  : rsx::invalidation_cause::deferred_read);

	auto cmd = can_flush ? gl::command_context{ gl_state } : gl::command_context{};
	auto result = m_gl_texture_cache.invalidate_address(cmd, address, cause);

	if (!result.violation_handled)
		return false;

	{
		std::lock_guard lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}

	if (result.num_flushable > 0)
	{
		auto &task = post_flush_request(address, result);

		vm::temporary_unlock();
		task.producer_wait();
	}

	return true;
}

void GLGSRender::on_invalidate_memory_range(const utils::address_range &range, rsx::invalidation_cause cause)
{
	gl::command_context cmd{ gl_state };
	auto data = std::move(m_gl_texture_cache.invalidate_range(cmd, range, cause));
	AUDIT(data.empty());

	if (cause == rsx::invalidation_cause::unmap && data.violation_handled)
	{
		m_gl_texture_cache.purge_unreleased_sections();
		{
			std::lock_guard lock(m_sampler_mutex);
			m_samplers_dirty.store(true);
		}
	}
}

void GLGSRender::on_semaphore_acquire_wait()
{
	if (!work_queue.empty() ||
		(async_flip_requested & flip_request::emu_requested))
	{
		do_local_task(rsx::FIFO_state::lock_wait);
	}
}

void GLGSRender::do_local_task(rsx::FIFO_state state)
{
	if (!work_queue.empty())
	{
		std::lock_guard lock(queue_guard);

		work_queue.remove_if([](auto &q) { return q.received; });

		for (auto& q : work_queue)
		{
			if (q.processed) continue;

			gl::command_context cmd{ gl_state };
			q.result = m_gl_texture_cache.flush_all(cmd, q.section_data);
			q.processed = true;
		}
	}
	else if (!in_begin_end && state != rsx::FIFO_state::lock_wait)
	{
		if (m_graphics_state & rsx::pipeline_state::framebuffer_reads_dirty)
		{
			//This will re-engage locks and break the texture cache if another thread is waiting in access violation handler!
			//Only call when there are no waiters
			m_gl_texture_cache.do_update();
			m_graphics_state &= ~rsx::pipeline_state::framebuffer_reads_dirty;
		}
	}

	rsx::thread::do_local_task(state);

	if (state == rsx::FIFO_state::lock_wait)
	{
		// Critical check finished
		return;
	}

	if (m_overlay_manager)
	{
		if (!in_begin_end && async_flip_requested & flip_request::native_ui)
		{
			rsx::display_flip_info_t info{};
			info.buffer = current_display_buffer;
			flip(info);
		}
	}
}

gl::work_item& GLGSRender::post_flush_request(u32 address, gl::texture_cache::thrashed_set& flush_data)
{
	std::lock_guard lock(queue_guard);

	auto &result = work_queue.emplace_back();
	result.address_to_flush = address;
	result.section_data = std::move(flush_data);
	return result;
}

bool GLGSRender::scaled_image_from_memory(rsx::blit_src_info& src, rsx::blit_dst_info& dst, bool interpolate)
{
	gl::command_context cmd{ gl_state };
	if (m_gl_texture_cache.blit(cmd, src, dst, interpolate, m_rtts))
	{
		m_samplers_dirty.store(true);
		return true;
	}

	return false;
}

void GLGSRender::notify_tile_unbound(u32 tile)
{
	// TODO: Handle texture writeback
	if (false)
	{
		u32 addr = rsx::get_address(tiles[tile].offset, tiles[tile].location, HERE);
		on_notify_memory_unmapped(addr, tiles[tile].size);
		m_rtts.invalidate_surface_address(addr, false);
	}

	{
		std::lock_guard lock(m_sampler_mutex);
		m_samplers_dirty.store(true);
	}
}

void GLGSRender::begin_occlusion_query(rsx::reports::occlusion_query_info* query)
{
	query->result = 0;
	glBeginQuery(GL_ANY_SAMPLES_PASSED, query->driver_handle);
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
	glGetQueryObjectiv(query->driver_handle, GL_QUERY_RESULT_AVAILABLE, &status);

	return status != GL_FALSE;
}

void GLGSRender::get_occlusion_query_result(rsx::reports::occlusion_query_info* query)
{
	if (query->num_draws)
	{
		GLint result = 0;
		glGetQueryObjectiv(query->driver_handle, GL_QUERY_RESULT, &result);

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

void GLGSRender::on_decompiler_init()
{
	// Bind decompiler context to this thread
	m_frame->set_current(m_decompiler_context);
}

void GLGSRender::on_decompiler_exit()
{
	// Cleanup
	m_frame->delete_context(m_decompiler_context);
}

bool GLGSRender::on_decompiler_task()
{
	const auto result = m_prog_buffer.async_update(8);
	if (result.second)
	{
		// TODO: Proper synchronization with renderer
		// Finish works well enough for now but it is not a proper soulution
		glFinish();
	}

	return result.first;
}
