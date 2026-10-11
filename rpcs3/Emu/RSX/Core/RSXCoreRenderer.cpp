#include "stdafx.h"
#include "RSXCoreRenderer.h"

#include "Emu/RSX/Capture/rsx_capture.h"
#include "Emu/RSX/Common/TextureUtils.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/Utils/color_utils.hpp"
#include "Emu/RSX/rsx_methods.h"
#include "Emu/system_config.h"

#include "Emu/RSX/NV47/HW/context_accessors.define.h"

namespace rsx
{
	static void decode_raster_state(const rsx_state& regs, raster_state& state)
	{
		state.cull_face_enabled = regs.cull_face_enabled();
		state.cull_face_mode = regs.cull_face_mode();
		state.front_face_mode = regs.front_face_mode();
		state.polygon_mode_front = regs.polygon_mode_front();
		state.polygon_mode_back = regs.polygon_mode_back();

		state.line_width = regs.line_width();
		state.line_smooth_enabled = regs.line_smooth_enabled();

		state.poly_offset_point_enabled = regs.poly_offset_point_enabled();
		state.poly_offset_line_enabled = regs.poly_offset_line_enabled();
		state.poly_offset_fill_enabled = regs.poly_offset_fill_enabled();
		state.poly_offset_scale = regs.poly_offset_scale();
		state.poly_offset_bias = regs.poly_offset_bias();
	}

	static void decode_depth_stencil_state(const rsx_state& regs, depth_stencil_state& state)
	{
		state.depth_test_enabled = regs.depth_test_enabled();
		state.depth_write_enabled = regs.depth_write_enabled();

		if (state.depth_test_enabled)
		{
			state.depth_func = regs.depth_func();
		}

		state.depth_clamp_enabled = regs.depth_clamp_enabled();
		state.depth_clip_enabled = regs.depth_clip_enabled();

		state.depth_bounds_test_enabled = regs.depth_bounds_test_enabled();
		if (state.depth_bounds_test_enabled)
		{
			// Saturate the bounds into the RSX depth format's range as hardware does
			const auto depth_format = regs.surface_depth_fmt();
			state.depth_bounds_min = rsx::clamp_depth_bounds_value(depth_format, regs.depth_bounds_min());
			state.depth_bounds_max = rsx::clamp_depth_bounds_value(depth_format, regs.depth_bounds_max());
		}

		state.stencil_test_enabled = regs.stencil_test_enabled();
		state.two_sided_stencil_test_enabled = regs.two_sided_stencil_test_enabled();

		if (!state.stencil_test_enabled)
		{
			return;
		}

		state.front.func = regs.stencil_func();
		state.front.func_ref = regs.stencil_func_ref();
		state.front.func_mask = regs.stencil_func_mask();
		state.front.write_mask = regs.stencil_mask();
		state.front.op_fail = regs.stencil_op_fail();
		state.front.op_zfail = regs.stencil_op_zfail();
		state.front.op_zpass = regs.stencil_op_zpass();

		if (!state.two_sided_stencil_test_enabled)
		{
			return;
		}

		state.back.func = regs.back_stencil_func();
		state.back.func_ref = regs.back_stencil_func_ref();
		state.back.func_mask = regs.back_stencil_func_mask();
		state.back.write_mask = regs.back_stencil_mask();
		state.back.op_fail = regs.back_stencil_op_fail();
		state.back.op_zfail = regs.back_stencil_op_zfail();
		state.back.op_zpass = regs.back_stencil_op_zpass();
	}

	static std::array<f32, 4> decode_constant_blend_color(const rsx_state& regs, surface_color_format color_format)
	{
		//TODO: check another color formats (probably all integer formats with > 8-bits wide channels)
		if (color_format == rsx::surface_color_format::w16z16y16x16)
		{
			const u16 blend_color_r = regs.blend_color_16b_r();
			const u16 blend_color_g = regs.blend_color_16b_g();
			const u16 blend_color_b = regs.blend_color_16b_b();
			const u16 blend_color_a = regs.blend_color_16b_a();

			return { decode_fp16(blend_color_r), decode_fp16(blend_color_g), decode_fp16(blend_color_b), decode_fp16(blend_color_a) };
		}

		const u8 blend_color_r = regs.blend_color_8b_r();
		const u8 blend_color_g = regs.blend_color_8b_g();
		const u8 blend_color_b = regs.blend_color_8b_b();
		const u8 blend_color_a = regs.blend_color_8b_a();

		return { blend_color_r / 255.f, blend_color_g / 255.f, blend_color_b / 255.f, blend_color_a / 255.f };
	}

	static void decode_blend_state(const rsx_state& regs, blend_state& state)
	{
		const auto color_format = regs.surface_color();
		const auto output_mask = rsx::get_write_output_mask(color_format);

		for (u32 index = 0; index < state.color_mask.size(); ++index)
		{
			bool color_mask_b = regs.color_mask_b(index);
			bool color_mask_g = regs.color_mask_g(index);
			bool color_mask_r = regs.color_mask_r(index);
			bool color_mask_a = regs.color_mask_a(index);

			switch (color_format)
			{
			case rsx::surface_color_format::b8:
				rsx::get_b8_colormask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);
				break;
			case rsx::surface_color_format::g8b8:
				rsx::get_g8b8_r8g8_colormask(color_mask_r, color_mask_g, color_mask_b, color_mask_a);
				break;
			default:
				break;
			}

			state.color_mask[index] =
			{
				.r = color_mask_r && output_mask[0],
				.g = color_mask_g && output_mask[1],
				.b = color_mask_b && output_mask[2],
				.a = color_mask_a && output_mask[3]
			};
		}

		state.dither_enabled = regs.dither_enabled();

		state.logic_op_enabled = regs.logic_op_enabled();
		if (state.logic_op_enabled)
		{
			state.logic_operation = regs.logic_operation();
		}

		// Blend factors are still decoded under the logic op. Programmable blending and the VK blend constants only check the enable mask.
		state.enable_mask = static_cast<u8>(regs.blend_enabled_mask());
		if (!state.enable_mask)
		{
			return;
		}

		state.sfactor_rgb = regs.blend_func_sfactor_rgb();
		state.sfactor_a = regs.blend_func_sfactor_a();
		state.dfactor_rgb = regs.blend_func_dfactor_rgb();
		state.dfactor_a = regs.blend_func_dfactor_a();
		state.equation_rgb = regs.blend_equation_rgb();
		state.equation_a = regs.blend_equation_a();
		state.constant_color = decode_constant_blend_color(regs, color_format);
	}

	static void decode_multisample_state(const rsx_state& regs, multisample_state& state)
	{
		state.antialias = regs.surface_antialias();
		state.msaa_enabled = regs.msaa_enabled();
		state.sample_mask = regs.msaa_sample_mask();
		state.alpha_to_coverage_enabled = regs.msaa_alpha_to_coverage_enabled();
		state.alpha_to_one_enabled = regs.msaa_alpha_to_one_enabled();
	}

	static void decode_viewport_state(const rsx_state& regs, viewport_state& state)
	{
		state.clip_width = regs.surface_clip_width();
		state.clip_height = regs.surface_clip_height();
		state.z_clip_min = regs.clip_min();
		state.z_clip_max = regs.clip_max();
	}

	static void decode_clear_state(const rsx_state& regs, rsx::flags32_t mask, clear_state& state)
	{
		state.mask = mask;
		state.depth_format = regs.surface_depth_fmt();
		state.stencil_write_mask = regs.stencil_mask();

		state.scissor_covers_surface =
			regs.scissor_origin_x() == 0 &&
			regs.scissor_origin_y() == 0 &&
			regs.scissor_width() >= regs.surface_clip_width() &&
			regs.scissor_height() >= regs.surface_clip_height();

		const bool has_stencil = is_depth_stencil_format(state.depth_format);

		if (mask & RSX_GCM_CLEAR_DEPTH_BIT)
		{
			const u32 clear_depth_bits = regs.z_clear_value(has_stencil);
			state.depth_value = rsx::get_depth_clear_value(state.depth_format, clear_depth_bits);
		}

		if ((mask & RSX_GCM_CLEAR_STENCIL_BIT) && has_stencil)
		{
			state.stencil_value = regs.stencil_clear_value();
		}

		state.color_mask = mask & RSX_GCM_CLEAR_COLOR_RGBA_MASK;
		if (!state.color_mask)
		{
			return;
		}

		u8 clear_a = regs.clear_color_a();
		u8 clear_r = regs.clear_color_r();
		u8 clear_g = regs.clear_color_g();
		u8 clear_b = regs.clear_color_b();
		bool full_mask = (state.color_mask == RSX_GCM_CLEAR_COLOR_RGBA_MASK);

		switch (regs.surface_color())
		{
		case rsx::surface_color_format::x32:
		case rsx::surface_color_format::w16z16y16x16:
		case rsx::surface_color_format::w32z32y32x32:
		{
			// Nop
			state.color_mask = 0;
			return;
		}
		case rsx::surface_color_format::b8:
		{
			rsx::get_b8_clear_color(clear_r, clear_g, clear_b, clear_a);
			state.color_mask = rsx::get_b8_clearmask(state.color_mask);
			full_mask = !!(state.color_mask & RSX_GCM_CLEAR_RED_BIT);
			break;
		}
		case rsx::surface_color_format::g8b8:
		{
			rsx::get_g8b8_clear_color(clear_r, clear_g, clear_b, clear_a);
			state.color_mask = rsx::get_g8b8_r8g8_clearmask(state.color_mask);
			full_mask = ((state.color_mask & RSX_GCM_CLEAR_COLOR_RG_MASK) == RSX_GCM_CLEAR_COLOR_RG_MASK);
			break;
		}
		case rsx::surface_color_format::r5g6b5:
		{
			rsx::get_rgb565_clear_color(clear_r, clear_g, clear_b, clear_a);
			full_mask = ((state.color_mask & RSX_GCM_CLEAR_COLOR_RGB_MASK) == RSX_GCM_CLEAR_COLOR_RGB_MASK);
			break;
		}
		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		{
			rsx::get_a1rgb555_clear_color(clear_r, clear_g, clear_b, clear_a, 255);
			break;
		}
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
		{
			rsx::get_a1rgb555_clear_color(clear_r, clear_g, clear_b, clear_a, 0);
			break;
		}
		case rsx::surface_color_format::a8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		{
			rsx::get_abgr8_clear_color(clear_r, clear_g, clear_b, clear_a);
			state.color_mask = rsx::get_abgr8_clearmask(state.color_mask);
			break;
		}
		default:
		{
			break;
		}
		}

		state.color_r = clear_r;
		state.color_g = clear_g;
		state.color_b = clear_b;
		state.color_a = clear_a;
		state.color_mask_full = full_mask;
	}

#if defined(DRAW_STATE_DEBUG) && DRAW_STATE_DEBUG
	// Full decode on top of the current state. Fields the decoders skip keep their current value, so only relevant fields are compared.
	// A mismatch means a register feeding the group is missing from draw_state_signals.
	static void verify_draw_state(const rsx_state& regs, const draw_state_t& state)
	{
		raster_state raster = state.raster;
		decode_raster_state(regs, raster);
		ensure(raster == state.raster, "Stale raster state");

		depth_stencil_state depth_stencil = state.depth_stencil;
		decode_depth_stencil_state(regs, depth_stencil);
		ensure(depth_stencil == state.depth_stencil, "Stale depth/stencil state");

		blend_state blend = state.blend;
		decode_blend_state(regs, blend);
		ensure(blend == state.blend, "Stale blend state");

		multisample_state multisample = state.multisample;
		decode_multisample_state(regs, multisample);
		ensure(multisample == state.multisample, "Stale multisample state");

		viewport_state viewport = state.viewport;
		decode_viewport_state(regs, viewport);
		ensure(viewport == state.viewport, "Stale viewport state");
	}
#endif

	void core_renderer::begin()
	{
		// Save shader state now before prefetch and loading happens
		m_pre_begin_pipeline_state = (m_ctx->graphics_state.load() & rsx::pipeline_state::invalidate_pipeline_bits);

		if (m_ctx->cond_render.hw_cond_active)
		{
			if (!m_ctx->cond_render.eval_pending())
			{
				// End conditional rendering if still active
				RSX(m_ctx)->end_conditional_rendering();
			}

			// If hw cond render is enabled and evalutation is still pending, do nothing
		}
		else if (m_ctx->cond_render.eval_pending())
		{
			// Evaluate conditional rendering test or enable hw cond render until results are available
			if (RSX(m_ctx)->renderer()->config().supports_hw_conditional_render)
			{
				// In this mode, it is possible to skip the cond render while the backend is still processing data.
				// The backend guarantees that any draw calls emitted during this time will NOT generate any ROP writes
				ensure(!m_ctx->cond_render.hw_cond_active);

				// Pending evaluation, use hardware test
				RSX(m_ctx)->begin_conditional_rendering(m_ctx->cond_render.eval_sources);
			}
			else
			{
				// NOTE: eval_sources list is reversed with newest query first
				RSX(m_ctx)->zcull_ctrl->read_barrier(RSX(m_ctx), m_ctx->cond_render.eval_address, m_ctx->cond_render.eval_sources.front());
				ensure(!m_ctx->cond_render.eval_pending());
			}
		}

		if (!RSX(m_ctx)->renderer()->config().supports_normalized_barycentrics)
		{
			// Check for mode change between rasterized polys vs lines and points
			// Luckily this almost never happens in real games
			const auto current_mode = REGS(m_ctx)->current_draw_clause.classify_mode();
			if (current_mode != m_ctx->program.draw_mode)
			{
				m_ctx->graphics_state |= (rsx::vertex_program_state_dirty | rsx::fragment_program_state_dirty);
				m_ctx->program.draw_mode = current_mode;
			}
		}

		m_ctx->in_begin_end = true;

		if (m_ctx->frame_skip.skip_current_frame || m_ctx->cond_render.disable_rendering())
		{
			return;
		}

		// The backend binds the framebuffer here, which reads the viewport
		decode_draw_state();

		RSX(m_ctx)->renderer()->begin_draw();
	}

	void core_renderer::end()
	{
		if (should_skip_draw())
		{
			execute_nop_draw();
		}
		else
		{
			decode_draw_state();

			if (RSX(m_ctx)->renderer()->end_draw(m_pre_begin_pipeline_state))
			{
				// The backend has seen this draw's change set. If the draw was not executed, changes keep accumulating.
				m_ctx->draw_state.dirty.clear();
			}
			else
			{
				execute_nop_draw();
			}
		}

		post_draw_hook();
	}

	void core_renderer::decode_draw_state()
	{
		auto& state = m_ctx->draw_state;
		const auto& regs = *REGS(m_ctx);

		// Only groups whose source registers changed are decoded again
		if (state.pending.test(draw_state_t::group::raster))
		{
			decode_raster_state(regs, state.raster);
			state.pending.clear(draw_state_t::group::raster);
			state.dirty.set(draw_state_t::group::raster);
		}

		if (state.pending.test(draw_state_t::group::depth_stencil))
		{
			decode_depth_stencil_state(regs, state.depth_stencil);
			state.pending.clear(draw_state_t::group::depth_stencil);
			state.dirty.set(draw_state_t::group::depth_stencil);
		}

		if (state.pending.test(draw_state_t::group::blend))
		{
			decode_blend_state(regs, state.blend);
			state.pending.clear(draw_state_t::group::blend);
			state.dirty.set(draw_state_t::group::blend);
		}

		if (state.pending.test(draw_state_t::group::multisample))
		{
			decode_multisample_state(regs, state.multisample);
			state.pending.clear(draw_state_t::group::multisample);
			state.dirty.set(draw_state_t::group::multisample);
		}

		if (state.pending.test(draw_state_t::group::viewport))
		{
			decode_viewport_state(regs, state.viewport);
			state.pending.clear(draw_state_t::group::viewport);
			state.dirty.set(draw_state_t::group::viewport);
		}

#if defined(DRAW_STATE_DEBUG) && DRAW_STATE_DEBUG
		verify_draw_state(regs, state);
#endif
	}

	void core_renderer::clear(rsx::flags32_t arg)
	{
		if (m_ctx->frame_skip.skip_current_frame)
		{
			return;
		}

		const auto& regs = *REGS(m_ctx);

		// If stencil write mask is disabled, remove clear_stencil bit
		if (!regs.stencil_mask())
		{
			arg &= ~RSX_GCM_CLEAR_STENCIL_BIT;
		}

		// Ignore invalid clear flags
		if (!(arg & RSX_GCM_CLEAR_ANY_MASK))
		{
			return;
		}

		// The backend binds the framebuffer for the clear, which reads the viewport.
		// Groups decoded here stay dirty until the next executed draw.
		decode_draw_state();
		decode_clear_state(regs, arg, m_ctx->draw_state.clear);

		RSX(m_ctx)->renderer()->clear();
	}

	void core_renderer::discard_draw()
	{
		execute_nop_draw();
		post_draw_hook();
	}

	void core_renderer::post_draw_hook()
	{
		if (RSX(m_ctx)->capture_current_frame)
		{
			capture::capture_draw_memory(RSX(m_ctx));
		}

		m_ctx->in_begin_end = false;
		m_ctx->stats.draw_calls++;

		REGS(m_ctx)->current_draw_clause.post_execute_cleanup(m_ctx);

		m_ctx->graphics_state |= rsx::pipeline_state::framebuffer_reads_dirty;
		m_ctx->eng_interrupt_mask |= rsx::backend_interrupt;
		RSX(m_ctx)->ROP_sync_timestamp = rsx::get_shared_tag();

		RSX(m_ctx)->m_draw_processor.clear_push_buffers();

		RSX(m_ctx)->zcull_ctrl->on_draw();

		if (RSX(m_ctx)->capture_current_frame)
		{
			const u32 element_count = REGS(m_ctx)->current_draw_clause.get_elements_count();
			RSX(m_ctx)->capture_frame(fmt::format("Draw %s %d", REGS(m_ctx)->current_draw_clause.primitive, element_count));
		}
	}

	void core_renderer::execute_nop_draw()
	{
		auto& draw_clause = REGS(m_ctx)->current_draw_clause;
		draw_clause.begin();
		do
		{
			draw_clause.execute_pipeline_dependencies(m_ctx);
		}
		while (draw_clause.next());
	}

	bool core_renderer::should_skip_draw() const
	{
		if (m_ctx->frame_skip.skip_current_frame ||
			!m_ctx->graphics_state.test(rsx::rtt_config_valid) ||
			m_ctx->cond_render.disable_rendering())
		{
			return true;
		}

		if (m_ctx->graphics_state.test(rsx::rtt_config_no_attachments) &&
			(g_cfg.video.disable_zcull_queries || !RSX(m_ctx)->zcull_ctrl->has_active_queries()))
		{
			return true;
		}

		return false;
	}
}
