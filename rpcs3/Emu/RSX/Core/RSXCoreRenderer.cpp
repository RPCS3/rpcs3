#include "stdafx.h"
#include "RSXCoreRenderer.h"

#include "Emu/RSX/Capture/rsx_capture.h"
#include "Emu/RSX/Common/TextureUtils.h"
#include "Emu/RSX/RSXThread.h"
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
		state.depth_func = regs.depth_func();
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

		state.front.func = regs.stencil_func();
		state.front.func_ref = regs.stencil_func_ref();
		state.front.func_mask = regs.stencil_func_mask();
		state.front.write_mask = regs.stencil_mask();
		state.front.op_fail = regs.stencil_op_fail();
		state.front.op_zfail = regs.stencil_op_zfail();
		state.front.op_zpass = regs.stencil_op_zpass();

		state.back.func = regs.back_stencil_func();
		state.back.func_ref = regs.back_stencil_func_ref();
		state.back.func_mask = regs.back_stencil_func_mask();
		state.back.write_mask = regs.back_stencil_mask();
		state.back.op_fail = regs.back_stencil_op_fail();
		state.back.op_zfail = regs.back_stencil_op_zfail();
		state.back.op_zpass = regs.back_stencil_op_zpass();
	}

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

		if (m_ctx->graphics_state & rsx::pipeline_state::polygon_offset_state_dirty)
		{
			state.dirty.set(draw_state_t::group::raster);
		}

		if (m_ctx->graphics_state & rsx::pipeline_state::depth_bounds_state_dirty)
		{
			state.dirty.set(draw_state_t::group::depth_stencil);
		}

		m_ctx->graphics_state.clear(rsx::pipeline_state::polygon_offset_state_dirty | rsx::pipeline_state::depth_bounds_state_dirty);

		decode_raster_state(*REGS(m_ctx), state.raster);
		decode_depth_stencil_state(*REGS(m_ctx), state.depth_stencil);
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
