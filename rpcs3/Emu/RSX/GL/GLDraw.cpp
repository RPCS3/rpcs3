#include "stdafx.h"
#include "GLGSRender.h"
#include "../Common/BufferUtils.h"

namespace gl
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
		gl_state.depth_func(gl::comparison_op(rsx::method_registers.depth_func()));
	}

	if (glDepthBoundsEXT && (gl_state.enable(rsx::method_registers.depth_bounds_test_enabled(), GL_DEPTH_BOUNDS_TEST_EXT)))
	{
		gl_state.depth_bounds(rsx::method_registers.depth_bounds_min(), rsx::method_registers.depth_bounds_max());
	}

	gl_state.enable(rsx::method_registers.dither_enabled(), GL_DITHER);

	if (gl_state.enable(rsx::method_registers.stencil_test_enabled(), GL_STENCIL_TEST))
	{
		glStencilFunc(gl::comparison_op(rsx::method_registers.stencil_func()),
			rsx::method_registers.stencil_func_ref(),
			rsx::method_registers.stencil_func_mask());

		glStencilOp(gl::stencil_op(rsx::method_registers.stencil_op_fail()), gl::stencil_op(rsx::method_registers.stencil_op_zfail()),
			gl::stencil_op(rsx::method_registers.stencil_op_zpass()));

		if (rsx::method_registers.two_sided_stencil_test_enabled())
		{
			glStencilMaskSeparate(GL_BACK, rsx::method_registers.back_stencil_mask());

			glStencilFuncSeparate(GL_BACK, gl::comparison_op(rsx::method_registers.back_stencil_func()),
				rsx::method_registers.back_stencil_func_ref(), rsx::method_registers.back_stencil_func_mask());

			glStencilOpSeparate(GL_BACK, gl::stencil_op(rsx::method_registers.back_stencil_op_fail()),
				gl::stencil_op(rsx::method_registers.back_stencil_op_zfail()), gl::stencil_op(rsx::method_registers.back_stencil_op_zpass()));
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
		glBlendFuncSeparate(gl::blend_factor(rsx::method_registers.blend_func_sfactor_rgb()),
			gl::blend_factor(rsx::method_registers.blend_func_dfactor_rgb()),
			gl::blend_factor(rsx::method_registers.blend_func_sfactor_a()),
			gl::blend_factor(rsx::method_registers.blend_func_dfactor_a()));

		auto blend_colors = rsx::get_constant_blend_colors();
		glBlendColor(blend_colors[0], blend_colors[1], blend_colors[2], blend_colors[3]);

		glBlendEquationSeparate(gl::blend_equation(rsx::method_registers.blend_equation_rgb()),
			gl::blend_equation(rsx::method_registers.blend_equation_a()));
	}

	gl_state.enablei(mrt_blend_enabled[0], GL_BLEND, 0);
	gl_state.enablei(mrt_blend_enabled[1], GL_BLEND, 1);
	gl_state.enablei(mrt_blend_enabled[2], GL_BLEND, 2);
	gl_state.enablei(mrt_blend_enabled[3], GL_BLEND, 3);

	if (gl_state.enable(rsx::method_registers.logic_op_enabled(), GL_COLOR_LOGIC_OP))
	{
		gl_state.logic_op(gl::logic_op(rsx::method_registers.logic_operation()));
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
		gl_state.cull_face(gl::cull_face(rsx::method_registers.cull_face_mode()));
	}

	gl_state.front_face(gl::front_face(rsx::method_registers.front_face_mode()));

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

void GLGSRender::load_texture_env()
{
	// Load textures
	gl::command_context cmd{ gl_state };
	bool  update_framebuffer_sourced = false;

	std::lock_guard lock(m_sampler_mutex);

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
}

void GLGSRender::bind_texture_env()
{
	// Bind textures and resolve external copy operations
	gl::command_context cmd{ gl_state };

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
}

void GLGSRender::emit_geometry(u32 sub_index)
{
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

	if (!sub_index)
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
			return;
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

	if (manually_flush_ring_buffers)
	{
		//Use approximations to reserve space. This path is mostly for debug purposes anyway
		u32 approx_vertex_count = rsx::method_registers.current_draw_clause.get_elements_count();
		u32 approx_working_buffer_size = approx_vertex_count * 256;

		//Allocate 256K heap if we have no approximation at this time (inlined array)
		m_attrib_ring_buffer->reserve_storage_on_heap(std::max(approx_working_buffer_size, 256 * 1024U));
		m_index_ring_buffer->reserve_storage_on_heap(16 * 1024);
	}

	// Do vertex upload before RTT prep / texture lookups to give the driver time to push data
	auto upload_info = set_vertex_buffer();
	do_heap_cleanup();

	if (upload_info.vertex_draw_count == 0)
	{
		// Malformed vertex setup; abort
		return;
	}

	const GLenum draw_mode = gl::draw_mode(rsx::method_registers.current_draw_clause.primitive);
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

	// Active texture environment is used to decode shaders
	m_profiler.start();
	load_texture_env();
	m_frame_stats.textures_upload_time += m_profiler.duration();

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

	bind_texture_env();
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

	rsx::method_registers.current_draw_clause.begin();
	u32 subdraw = 0u;
	do
	{
		emit_geometry(subdraw++);
	}
	while (rsx::method_registers.current_draw_clause.next());

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
